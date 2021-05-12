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

#define DOC_MAX_SIZE                             64
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
static char tcDoc[DOC_MAX_SIZE];
static uint8_t *pu08SerialNumber;

static const rc522_start_args_t stStartArgs =
{
  .miso_io  = ESP32_SPI_MISO_PIN,
  .mosi_io  = ESP32_SPI_MOSI_PIN,
  .sck_io   = ESP32_SPI_SCK_PIN,
  .sda_io   = ESP32_SPI_SDA_PIN,
  .callback = &_tag_handler,
};

void app_main(void)
{
  wifi_initialise();
  wifi_wait_connected();

  stQueue = xQueueCreate(5, sizeof(rc522_event_t));
  xTaskCreate(_firestore_task, "firestore", 10240, NULL, 5, NULL);
}

static void _tag_handler(uint8_t *pu08SN)
{
  rc522_event_t eEvent;

  pu08SerialNumber = pu08SN;
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
                 "Detected Tag with serial-number: 0x%X 0x%X 0x%X 0x%X 0x%X",
                 pu08SerialNumber[0],
                 pu08SerialNumber[1],
                 pu08SerialNumber[2],
                 pu08SerialNumber[3],
                 pu08SerialNumber[4]);
        _send_data(pu08SerialNumber);
        break;
      default:
        ESP_LOGW(TAG, "Unknow event");
        break;
      }
    }
  }
}

static void _send_data(uint8_t *pu08SN)
{
  /* Format document with newly fetched data */
  u32DocLength = snprintf(tcDoc,
                          sizeof(tcDoc),
                          "{\"fields\":{\"sn\":{\"stringValue\":\"%X%X%X%X%X\"}}}",
                          pu08SN[0],
                          pu08SN[1],
                          pu08SN[2],
                          pu08SN[3],
                          pu08SN[4]);
  ESP_LOGI(TAG, "Document length after formatting: %d", u32DocLength);
  if(u32DocLength > 0)
  {
    /* Update document in firestore or create it if it doesn't already exists */
    if(FIRESTORE_OK == firestore_update_document("devices",
                                                 "rfid-node",
                                                 tcDoc,
                                                 &u32DocLength))
    {
      ESP_LOGI(TAG, "Document updated successfully");
      ESP_LOGD(TAG, "document length: %d", u32DocLength);
      ESP_LOGD(TAG, "document content:\r\n%.*s", u32DocLength, tcDoc);
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
