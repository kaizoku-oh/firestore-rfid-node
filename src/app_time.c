#include <time.h>
#include <sys/time.h>

#include <esp_log.h>
#include <esp_sntp.h>

#include "app_wifi.h"

#define APP_TIME_TAG                             "TIME_APP"
#define APP_TIME_RETRIES_COUNT                   16
#define APP_TIME_BUFF_MAX_SIZE                   128

esp_err_t s32TimeStatus = ESP_FAIL;
static time_t stNow;
static uint8_t u08Retries;
static struct tm stTimeInfo;
static char tcTimeFormatBuff[APP_TIME_BUFF_MAX_SIZE];

void app_time_init()
{
  sntp_sync_status_t eStatus;

  time(&stNow);
  localtime_r(&stNow, &stTimeInfo);
  /* Is time set? If not, tm_year will be (1970 - 1900) */
  if((1970 - 1900) == stTimeInfo.tm_year)
  {
    ESP_LOGW(APP_TIME_TAG, "Time is not set yet --> Getting time over NTP");
    /* Initialize sntp */
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(NULL);
    sntp_init();
    /* Try to get new time from sntp */
    u08Retries = 0;
    while((SNTP_SYNC_STATUS_COMPLETED != (eStatus = sntp_get_sync_status())) &&
          (++u08Retries < APP_TIME_RETRIES_COUNT))
    {
      ESP_LOGI(APP_TIME_TAG,
               "Waiting for system time to be set... (%d/%d)",
               u08Retries,
               APP_TIME_RETRIES_COUNT);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    if(SNTP_SYNC_STATUS_COMPLETED == eStatus)
    {
      /* Update with new time value */
      time(&stNow);
      localtime_r(&stNow, &stTimeInfo);
      setenv("TZ", "UTC", 1);
      tzset();
      strftime(tcTimeFormatBuff, sizeof(tcTimeFormatBuff), "%c", &stTimeInfo);
      ESP_LOGI(APP_TIME_TAG, "Time is set successfully");
      ESP_LOGI(APP_TIME_TAG, "The current date/time in UTC is: %s", tcTimeFormatBuff);
      s32TimeStatus = ESP_OK;
    }
    else
    {
      ESP_LOGE(APP_TIME_TAG, "Failed to get time from SNTP server");
      s32TimeStatus = ESP_FAIL;
    }
  }
  else
  {
    ESP_LOGI(APP_TIME_TAG, "Time is already set");
    s32TimeStatus = ESP_OK;
  }
}

esp_err_t app_time_get_ts(int64_t *ps64Timestamp)
{
  esp_err_t s32RetVal;
  struct timeval stTvNow;

  if(ps64Timestamp)
  {
    if(ESP_OK == s32TimeStatus)
    {
      s32RetVal = (0 == gettimeofday(&stTvNow, NULL))?ESP_OK:ESP_FAIL;
      *ps64Timestamp = (int64_t)stTvNow.tv_sec * 1000LL + (int64_t)stTvNow.tv_usec / 1000LL;
      ESP_LOGD(APP_TIME_TAG, "UNIX time in ms: %lld", *ps64Timestamp);
    }
    else
    {
      s32RetVal = ESP_FAIL;
    }
  }
  else
  {
    s32RetVal = ESP_ERR_INVALID_ARG;
    ESP_LOGE(APP_TIME_TAG, "Failed to get timestamp");
  }
  return s32RetVal;
}