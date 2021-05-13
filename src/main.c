#include <string.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "rc522.h"
#include "firestore.h"
#include "wifi_utils.h"

static void _send_data(uint8_t *);
static void _tag_handler(uint8_t *);
static void _firestore_task(void *);

#define TAG                                      "APP_MAIN"

#define ESP32_SPI_MISO_PIN                       19
#define ESP32_SPI_MOSI_PIN                       23
#define ESP32_SPI_SCK_PIN                        18
#define ESP32_SPI_SDA_PIN                        21

#define SERIAL_NUMBER_MAX_SIZE                   5

#define FIRESTORE_DOC_MAX_SIZE                   64
#define FIRESTORE_COLLECTION_ID                  "devices"
#define FIRESTORE_DOCUMENT_ID                    "rfid-node"
#define FIRESTORE_DOCUMENT_EXAMPLE               "{"                                 \
                                                   "\"fields\": {"                   \
                                                     "\"sn\": {"                     \
                                                       "\"stringValue\": ABCDEF1234" \
                                                     "}"                             \
                                                   "}"                               \
                                                 "}"

typedef enum
{
  TAG_DETECTED_EVENT = 0,
  TAG_INVALID_EVENT,
}rc522_event_t;

static QueueHandle_t stQueue;
static uint32_t u32DocLength;
static char tcDoc[FIRESTORE_DOC_MAX_SIZE];
static uint8_t tcu08SerialNumber[SERIAL_NUMBER_MAX_SIZE];

static const rc522_start_args_t stStartArgs =
{
  .miso_io  = ESP32_SPI_MISO_PIN,
  .mosi_io  = ESP32_SPI_MOSI_PIN,
  .sck_io   = ESP32_SPI_SCK_PIN,
  .sda_io   = ESP32_SPI_SDA_PIN,
  .callback = &_tag_handler,
};

static const uint8_t ttu08KnownSerialNumbers[3][5] =
{
  {0x72, 0xEA, 0x5F, 0x06, 0xC1},
  {0x29, 0x57, 0x8C, 0xBB, 0x49},
  {0x76, 0x9E, 0x25, 0xF8, 0x35},
};

void app_main(void)
{
  wifi_initialise();
  wifi_wait_connected();

  stQueue = xQueueCreate(10, sizeof(rc522_event_t));
  xTaskCreate(_firestore_task, "firestore", 10240, NULL, 4, NULL);
}

static void _tag_handler(uint8_t *pu08SN)
{
  rc522_event_t eEvent;

  memcpy(tcu08SerialNumber, pu08SN, sizeof(tcu08SerialNumber));
  eEvent = TAG_DETECTED_EVENT;
  xQueueSend(stQueue, &eEvent, portMAX_DELAY);
}

static void _firestore_task(void *pvParameter)
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
        ESP_LOGI(TAG,
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
          ESP_LOGI(TAG, "Tag is recognized");
        }
        else
        {
          ESP_LOGW(TAG, "Tag is not recognized");
        }
        /* Sending data anyway */
        _send_data(tcu08SerialNumber);
        break;
      default:
        ESP_LOGW(TAG, "Unknow event");
        break;
      }
    }
    else
    {
      ESP_LOGE(TAG, "Couldn't receive item from queue");
    }
  }
}

static void _send_data(uint8_t *pu08SN)
{
  /* Format json document */
  u32DocLength = snprintf(tcDoc,
                          sizeof(tcDoc),
                          "{\"fields\":{\"sn\":{\"stringValue\":\"%X%X%X%X%X\"}}}",
                          pu08SN[0],
                          pu08SN[1],
                          pu08SN[2],
                          pu08SN[3],
                          pu08SN[4]);
  ESP_LOGI(TAG, "Document length after formatting: %d", u32DocLength);
  ESP_LOGI(TAG, "Document content after formatting:\r\n%.*s", u32DocLength, tcDoc);
  if(u32DocLength > 0)
  {
    /* Update document in firestore or create it if it doesn't already exists */
    if(FIRESTORE_OK == firestore_update_document(FIRESTORE_COLLECTION_ID,
                                                 FIRESTORE_DOCUMENT_ID,
                                                 tcDoc,
                                                 &u32DocLength))
    {
      ESP_LOGI(TAG, "Document updated successfully");
      ESP_LOGD(TAG, "Document length: %d", u32DocLength);
      ESP_LOGD(TAG, "Document content:\r\n%.*s", u32DocLength, tcDoc);
    }
    else
    {
      ESP_LOGE(TAG, "Couldn't update document");
    }
  }
  else
  {
    ESP_LOGE(TAG, "Couldn't format document");
  }
}
