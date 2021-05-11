#include "esp_log.h"
#include "rc522.h"

static void _tag_handler(uint8_t *pu08SerialNumber);

#define TAG                                      "APP_MAIN"
#define ESP32_SPI_MISO_PIN                       19
#define ESP32_SPI_MOSI_PIN                       23
#define ESP32_SPI_SCK_PIN                        18
#define ESP32_SPI_SDA_PIN                        21

static const rc522_start_args_t stStartArgs =
{
  .miso_io  = ESP32_SPI_MISO_PIN,
  .mosi_io  = ESP32_SPI_MOSI_PIN,
  .sck_io   = ESP32_SPI_SCK_PIN,
  .sda_io   = ESP32_SPI_SDA_PIN,
  .callback = &_tag_handler,
};

static void _tag_handler(uint8_t *pu08SerialNumber)
{
  /* Serial number is always 5 bytes long */
  ESP_LOGI(TAG,
           "Serial number: 0x%X 0x%X 0x%X 0x%X 0x%X",
           pu08SerialNumber[0],
           pu08SerialNumber[1],
           pu08SerialNumber[2],
           pu08SerialNumber[3],
           pu08SerialNumber[4]);
}

void app_main(void)
{
  rc522_start(stStartArgs);
}