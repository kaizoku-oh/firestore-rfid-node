#include <time.h>
#include <sys/time.h>

#include <esp_log.h>
#include <esp_sntp.h>

#include "app_wifi.h"

#define APP_TIME_TAG                             "TIME_APP"
#define APP_TIME_RETRIES_COUNT                   16
#define APP_TIME_BUFF_MAX_SIZE                   128

static time_t stNow;
static uint8_t u08Retries;
static struct tm stTimeInfo;
static char tcTimeFormatBuff[APP_TIME_BUFF_MAX_SIZE];

static esp_err_t _app_time_init(void)
{
  esp_err_t s32RetVal;
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
      s32RetVal = ESP_OK;
      /* Update with new time value */
      time(&stNow);
      localtime_r(&stNow, &stTimeInfo);
      setenv("TZ", "UTC", 1);
      tzset();
      strftime(tcTimeFormatBuff, sizeof(tcTimeFormatBuff), "%c", &stTimeInfo);
      ESP_LOGI(APP_TIME_TAG, "Time is set successfully");
      ESP_LOGI(APP_TIME_TAG, "The current date/time in UTC is: %s", tcTimeFormatBuff);
    }
    else
    {
      s32RetVal = ESP_FAIL;
      ESP_LOGE(APP_TIME_TAG, "Failed to get time from SNTP server");
    }
  }
  else
  {
    s32RetVal = ESP_OK;
    ESP_LOGD(APP_TIME_TAG, "Time is already set");
  }
  return s32RetVal;
}

void app_time_init(void)
{
  _app_time_init();
}

esp_err_t app_time_get_timestamp(int64_t *ps64Timestamp)
{
  esp_err_t s32RetVal;
  struct timeval stTvNow;

  if(ps64Timestamp)
  {
    if(ESP_OK == _app_time_init())
    {
      s32RetVal = (0 == gettimeofday(&stTvNow, NULL))?ESP_OK:ESP_FAIL;
      *ps64Timestamp = (int64_t)stTvNow.tv_sec * 1000LL + (int64_t)stTvNow.tv_usec / 1000LL;
      ESP_LOGD(APP_TIME_TAG, "UNIX time in ms: %lld", *ps64Timestamp);
    }
    else
    {
      ESP_LOGE(APP_TIME_TAG, "Failed to get timestamp: Time is not set");
      s32RetVal = ESP_FAIL;
    }
  }
  else
  {
    s32RetVal = ESP_ERR_INVALID_ARG;
  }
  return s32RetVal;
}
