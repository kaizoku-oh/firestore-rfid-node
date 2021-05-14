#include <string.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "rc522.h"
#include "firestore.h"

#include "app_wifi.h"
#include "app_time.h"
#include "app_ota.h"

static void _app_main_send_data(uint8_t *);
static void _app_main_tag_handler(uint8_t *);
static void _app_main_firestore_task(void *);

#define APP_MAIN_TAG                             "APP_MAIN"

#define APP_MAIN_SPI_MISO_PIN                    19
#define APP_MAIN_SPI_MOSI_PIN                    23
#define APP_MAIN_SPI_SCK_PIN                     18
#define APP_MAIN_SPI_SDA_PIN                     21

#define APP_MAIN_SERIAL_NUMBER_MAX_SIZE          5

#define APP_MAIN_FIRESTORE_QUEUE_SIZE            10
#define APP_MAIN_FIRESTORE_TASK_STACK_SIZE       10240
#define APP_MAIN_FIRESTORE_TASK_PRIORITY         4
#define APP_MAIN_RC522_TASK_PRIORITY             5
#define APP_MAIN_FIRESTORE_PERIOD_MS             2500

#define APP_MAIN_FIRESTORE_DOC_MAX_SIZE          128
#define APP_MAIN_FIRESTORE_COLLECTION_ID         "devices"
#define APP_MAIN_FIRESTORE_DOCUMENT_ID           "rfid-node"
#define APP_MAIN_FIRESTORE_DOCUMENT_EXAMPLE      "{"                                     \
                                                   "\"fields\": {"                       \
                                                     "\"sn\": {"                         \
                                                       "\"stringValue\": ABCDEF1234"     \
                                                     "},"                                \
                                                     "\"timestamp\": {"                  \
                                                       "\"integerValue\": 1621010203262" \
                                                     "}"                                 \
                                                   "}"                                   \
                                                 "}"

typedef enum
{
  TAG_DETECTED_EVENT = 0,
  TAG_INVALID_EVENT,
}rc522_event_t;

static QueueHandle_t stQueue;
static uint32_t u32DocLength;
static char tcDoc[APP_MAIN_FIRESTORE_DOC_MAX_SIZE];
static uint8_t tcu08SerialNumber[APP_MAIN_SERIAL_NUMBER_MAX_SIZE];

static const rc522_start_args_t stStartArgs =
{
  .miso_io  = APP_MAIN_SPI_MISO_PIN,
  .mosi_io  = APP_MAIN_SPI_MOSI_PIN,
  .sck_io   = APP_MAIN_SPI_SCK_PIN,
  .sda_io   = APP_MAIN_SPI_SDA_PIN,
  .callback = &_app_main_tag_handler,
  .task_priority = APP_MAIN_RC522_TASK_PRIORITY
};

static const uint8_t ttu08KnownSerialNumbers[3][5] =
{
  {0x72, 0xEA, 0x5F, 0x06, 0xC1},
  {0x29, 0x57, 0x8C, 0xBB, 0x49},
  {0x76, 0x9E, 0x25, 0xF8, 0x35},
};

void app_main(void)
{
  app_wifi_init();
  app_wifi_wait();

  app_ota_start();

  app_time_init();

  stQueue = xQueueCreate(APP_MAIN_FIRESTORE_QUEUE_SIZE, sizeof(rc522_event_t));
  xTaskCreate(_app_main_firestore_task,
              "firestore",
              APP_MAIN_FIRESTORE_TASK_STACK_SIZE,
              NULL,
              APP_MAIN_FIRESTORE_TASK_PRIORITY,
              NULL);
}

static void _app_main_tag_handler(uint8_t *pu08SN)
{
  rc522_event_t eEvent;

  memcpy(tcu08SerialNumber, pu08SN, sizeof(tcu08SerialNumber));
  eEvent = TAG_DETECTED_EVENT;
  xQueueSend(stQueue, &eEvent, portMAX_DELAY);
}

static void _app_main_firestore_task(void *pvParameter)
{
  rc522_event_t eEvent;

  firestore_init();
  rc522_start(stStartArgs);
  while(1)
  {
    if(pdPASS == xQueueReceive(stQueue, &eEvent, portMAX_DELAY))
    {
      switch(eEvent)
      {
      case TAG_DETECTED_EVENT:
        ESP_LOGI(APP_MAIN_TAG,
                 "Detected Tag with serial-number: %X%X%X%X%X",
                 tcu08SerialNumber[0],
                 tcu08SerialNumber[1],
                 tcu08SerialNumber[2],
                 tcu08SerialNumber[3],
                 tcu08SerialNumber[4]);
        if((0 == memcmp(tcu08SerialNumber, ttu08KnownSerialNumbers[0], sizeof(tcu08SerialNumber))) ||
           (0 == memcmp(tcu08SerialNumber, ttu08KnownSerialNumbers[1], sizeof(tcu08SerialNumber))) ||
           (0 == memcmp(tcu08SerialNumber, ttu08KnownSerialNumbers[2], sizeof(tcu08SerialNumber))))
        {
          ESP_LOGI(APP_MAIN_TAG, "Tag is recognized");
        }
        else
        {
          ESP_LOGW(APP_MAIN_TAG, "Tag is not recognized");
        }
        /* Sending data anyway */
        _app_main_send_data(tcu08SerialNumber);
        break;
      default:
        ESP_LOGW(APP_MAIN_TAG, "Unknow event");
        break;
      }
    }
    else
    {
      ESP_LOGE(APP_MAIN_TAG, "Couldn't receive item from queue");
    }
  }
}

static void _app_main_send_data(uint8_t *pu08SN)
{
  int64_t s64Timestamp;

  /* Format json document */
  if(ESP_OK == app_time_get_timestamp(&s64Timestamp))
  {
    u32DocLength = snprintf(tcDoc,
                            sizeof(tcDoc),
                            "{\"fields\":{\"sn\":{\"stringValue\":\"%X%X%X%X%X\"},\"timestamp\":{\"integerValue\":%lld}}}",
                            pu08SN[0],
                            pu08SN[1],
                            pu08SN[2],
                            pu08SN[3],
                            pu08SN[4],
                            s64Timestamp);
  }
  else
  {
    ESP_LOGW(APP_MAIN_TAG, "Failed to get timestamp --> formatting data without timestamp");
    u32DocLength = snprintf(tcDoc,
                            sizeof(tcDoc),
                            "{\"fields\":{\"sn\":{\"stringValue\":\"%X%X%X%X%X\"}}}",
                            pu08SN[0],
                            pu08SN[1],
                            pu08SN[2],
                            pu08SN[3],
                            pu08SN[4]);
  }
  ESP_LOGD(APP_MAIN_TAG, "Document length after formatting: %d", u32DocLength);
  ESP_LOGD(APP_MAIN_TAG, "Document content after formatting:\r\n%.*s", u32DocLength, tcDoc);
  if(u32DocLength > 0)
  {
    /* Update document in firestore or create it if it doesn't already exists */
    if(FIRESTORE_OK == firestore_update_document(APP_MAIN_FIRESTORE_COLLECTION_ID,
                                                 APP_MAIN_FIRESTORE_DOCUMENT_ID,
                                                 tcDoc,
                                                 &u32DocLength))
    {
      ESP_LOGI(APP_MAIN_TAG, "Document updated successfully");
      ESP_LOGD(APP_MAIN_TAG, "Document length: %d", u32DocLength);
      ESP_LOGD(APP_MAIN_TAG, "Document content:\r\n%.*s", u32DocLength, tcDoc);
    }
    else
    {
      ESP_LOGE(APP_MAIN_TAG, "Couldn't update document");
    }
  }
  else
  {
    ESP_LOGE(APP_MAIN_TAG, "Couldn't format document");
  }
}
