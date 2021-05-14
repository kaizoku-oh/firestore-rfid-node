#ifndef _APP_TIME_H_
#define _APP_TIME_H_

#include <esp_err.h>

void app_time_init(void);
esp_err_t app_time_get_ts(int64_t *);

#endif /* _APP_TIME_H_ */