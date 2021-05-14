#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>

#include <cJSON.h>

#include "app_wifi.h"

#define APP_OTA_TAG                              "APP_OTA"

#define APP_OTA_BASE_URL                         "https://github-ota-api.herokuapp.com"
#define APP_OTA_ENDPOINT                         "/firmware/latest"
#define APP_OTA_GITHUB_USERNAME                  "kaizoku-oh"
#define APP_OTA_GITHUB_REPOSITORY                "firestore-rfid-node"
#define APP_OTA_DEVICE_CURRENT_FW_VERSION        APP_VERSION

#define APP_OTA_HTTP_INTERNAL_TX_BUFFER_SIZE     1024
#define APP_OTA_HTTP_INTERNAL_RX_BUFFER_SIZE     1024
#define APP_OTA_HTTP_APP_RX_BUFFER_SIZE          1024

#define APP_OTA_TASK_STACK_SIZE                  8192
#define APP_OTA_TASK_PRIORITY                    6
#define APP_OTA_TASK_PERIOD_MS                   60*1000

static const char *pcApiUrl = APP_OTA_BASE_URL APP_OTA_ENDPOINT
                              "?github_username="APP_OTA_GITHUB_USERNAME
                              "&github_repository="APP_OTA_GITHUB_REPOSITORY
                              "&device_current_fw_version="APP_OTA_DEVICE_CURRENT_FW_VERSION;

/* Certificates */
/* $ openssl s_client -showcerts -verify 5 -connect github-releases.githubusercontent.com:443 < /dev/null */
extern const char tcGithubReleaseCertPemStart[] asm("_binary_github_cert_pem_start");
extern const char tcGithubReleaseCertPemEnd[] asm("_binary_github_cert_pem_end");
/* $ openssl s_client -showcerts -verify 5 -connect herokuapp.com:443 < /dev/null */
extern const char tcHerokuCertPemStart[] asm("_binary_heroku_cert_pem_start");
extern const char tcHerokuCertPemEnd[] asm("_binary_heroku_cert_pem_end");

/* HTTP receive buffer */
char tcHttpRcvBuffer[APP_OTA_HTTP_APP_RX_BUFFER_SIZE];

static esp_err_t _app_ota_http_event_handler(esp_http_client_event_t *pstEvent)
{
  switch(pstEvent->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGD(APP_OTA_TAG, "HTTP error");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(APP_OTA_TAG, "HTTP connected to server");
    break;
  case HTTP_EVENT_HEADERS_SENT:
    ESP_LOGD(APP_OTA_TAG, "All HTTP headers are sent to server");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(APP_OTA_TAG, "Received HTTP header from server");
    printf("%.*s", pstEvent->data_len, (char*)pstEvent->data);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(APP_OTA_TAG, "Received data from server, len=%d", pstEvent->data_len);
    if(!esp_http_client_is_chunked_response(pstEvent->client))
    {
      strncpy(tcHttpRcvBuffer, (char*)pstEvent->data, pstEvent->data_len);
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(APP_OTA_TAG, "HTTP session is finished");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(APP_OTA_TAG, "HTTP connection is closed");
    break;
  }
  return ESP_OK;
}

static char* _app_ota_get_download_url(void)
{
  int s32HttpCode;
  esp_err_t s32RetVal;
  char* pcDownloadUrl;
  cJSON *pstJsonObject;
  cJSON *pstJsonDownloadUrl;
  esp_http_client_handle_t pstClient;

  pcDownloadUrl = NULL;
  esp_http_client_config_t config =
  {
    .url = pcApiUrl,
    .buffer_size = APP_OTA_HTTP_INTERNAL_RX_BUFFER_SIZE,
    .event_handler = _app_ota_http_event_handler,
    .cert_pem = tcHerokuCertPemStart,
  };
  pstClient = esp_http_client_init(&config);
  s32RetVal = esp_http_client_perform(pstClient);
  if(ESP_OK == s32RetVal)
  {
    ESP_LOGD(APP_OTA_TAG,
             "Status = %d, content_length = %d",
             esp_http_client_get_status_code(pstClient),
             esp_http_client_get_content_length(pstClient));
    s32HttpCode = esp_http_client_get_status_code(pstClient);
    if(204 == s32HttpCode)
    {
      ESP_LOGI(APP_OTA_TAG, "Device is already running the latest firmware");
    }
    else if(200 == s32HttpCode)
    {
      ESP_LOGD(APP_OTA_TAG, "tcHttpRcvBuffer: %s\n", tcHttpRcvBuffer);
      /* parse the http json respose */
      pstJsonObject = cJSON_Parse(tcHttpRcvBuffer);
      if(pstJsonObject == NULL)
      {
        ESP_LOGW(APP_OTA_TAG, "Response does not contain valid json, aborting...");
      }
      else
      {
        pstJsonDownloadUrl = cJSON_GetObjectItemCaseSensitive(pstJsonObject, "download_url");
        if(cJSON_IsString(pstJsonDownloadUrl) && (pstJsonDownloadUrl->valuestring != NULL))
        {
          pcDownloadUrl = pstJsonDownloadUrl->valuestring;
          ESP_LOGD(APP_OTA_TAG, "download_url length: %d", strlen(pcDownloadUrl));
        }
        else
        {
          ESP_LOGW(APP_OTA_TAG, "Unable to read the download_url, aborting...");
        }
      }
    }
    else
    {
      ESP_LOGW(APP_OTA_TAG, "Failed to get URL with HTTP code: %d", s32HttpCode);
    }
  }
  esp_http_client_cleanup(pstClient);
  return pcDownloadUrl;
}

static void _app_ota_check_update_task(void *pvParameter)
{
  char* pcDownloadUrl;

  while(1)
  {
    pcDownloadUrl = _app_ota_get_download_url();
    if(pcDownloadUrl != NULL)
    {
      ESP_LOGD(APP_OTA_TAG, "download_url: %s", pcDownloadUrl);
      ESP_LOGD(APP_OTA_TAG, "Downloading and installing new firmware");
      esp_http_client_config_t ota_client_config =
      {
        .url = pcDownloadUrl,
        .cert_pem = tcGithubReleaseCertPemStart,
        .buffer_size = APP_OTA_HTTP_INTERNAL_RX_BUFFER_SIZE,
        .buffer_size_tx = APP_OTA_HTTP_INTERNAL_TX_BUFFER_SIZE,
      };
      esp_err_t ret = esp_https_ota(&ota_client_config);
      if (ret == ESP_OK)
      {
        ESP_LOGI(APP_OTA_TAG, "OTA OK, restarting...");
        esp_restart();
      }
      else
      {
        ESP_LOGE(APP_OTA_TAG, "OTA failed...");
      }
    }
    else
    {
      ESP_LOGW(APP_OTA_TAG, "Could not get download url");
    }
    ESP_LOGI(APP_OTA_TAG, "Device is running App version: %s", APP_OTA_DEVICE_CURRENT_FW_VERSION);
    vTaskDelay(APP_OTA_TASK_PERIOD_MS / portTICK_PERIOD_MS);
  }
}

void app_ota_start(void)
{
  xTaskCreate(&_app_ota_check_update_task,
              "check_update",
              APP_OTA_TASK_STACK_SIZE,
              NULL,
              APP_OTA_TASK_PRIORITY,
              NULL);
}