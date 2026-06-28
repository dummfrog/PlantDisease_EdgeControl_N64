#include "app_selftest.h"

#include <stdio.h>

static uint8_t app_selftest_status[APP_SELFTEST_ITEM_COUNT];

static const char *AppSelfTest_StatusText(AppSelfTestItem_t item)
{
  return (app_selftest_status[item] != 0U) ? "OK" : "FAIL";
}

void AppSelfTest_Init(void)
{
  uint8_t item;

  for (item = 0U; item < APP_SELFTEST_ITEM_COUNT; item++)
  {
    app_selftest_status[item] = 0U;
  }
}

void AppSelfTest_Set(AppSelfTestItem_t item, uint8_t ok)
{
  if (item >= APP_SELFTEST_ITEM_COUNT)
  {
    return;
  }

  app_selftest_status[item] = (ok != 0U) ? 1U : 0U;
}

const char *AppSelfTest_GetSystemStatus(void)
{
  uint8_t item;

  for (item = 0U; item < APP_SELFTEST_ITEM_COUNT; item++)
  {
    if (app_selftest_status[item] == 0U)
    {
      return "DEGRADED";
    }
  }

  return "OK";
}

void AppSelfTest_FormatSensorStatus(char *buffer, uint32_t buffer_size)
{
  if ((buffer == NULL) || (buffer_size == 0U))
  {
    return;
  }

  (void)snprintf(buffer,
                 buffer_size,
                 "BH1750_%s,BME280_%s,INA219_%s,DS3231_%s",
                 AppSelfTest_StatusText(APP_SELFTEST_BH1750),
                 AppSelfTest_StatusText(APP_SELFTEST_BME280),
                 AppSelfTest_StatusText(APP_SELFTEST_INA219),
                 AppSelfTest_StatusText(APP_SELFTEST_DS3231));
}

void AppSelfTest_Print(void)
{
  printf("[SELFTEST] uart=%s buzzer=%s relay=%s i2c=%s "
         "bh1750=%s bme280=%s ina219=%s ds3231=%s system=%s\r\n",
         AppSelfTest_StatusText(APP_SELFTEST_UART),
         AppSelfTest_StatusText(APP_SELFTEST_BUZZER),
         AppSelfTest_StatusText(APP_SELFTEST_RELAY),
         AppSelfTest_StatusText(APP_SELFTEST_I2C),
         AppSelfTest_StatusText(APP_SELFTEST_BH1750),
         AppSelfTest_StatusText(APP_SELFTEST_BME280),
         AppSelfTest_StatusText(APP_SELFTEST_INA219),
         AppSelfTest_StatusText(APP_SELFTEST_DS3231),
         AppSelfTest_GetSystemStatus());
}
