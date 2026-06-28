#ifndef __APP_SELFTEST_H
#define __APP_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  APP_SELFTEST_UART = 0,
  APP_SELFTEST_BUZZER,
  APP_SELFTEST_RELAY,
  APP_SELFTEST_I2C,
  APP_SELFTEST_BH1750,
  APP_SELFTEST_BME280,
  APP_SELFTEST_INA219,
  APP_SELFTEST_DS3231,
  APP_SELFTEST_ITEM_COUNT
} AppSelfTestItem_t;

void AppSelfTest_Init(void);
void AppSelfTest_Set(AppSelfTestItem_t item, uint8_t ok);
void AppSelfTest_Print(void);
const char *AppSelfTest_GetSystemStatus(void);
void AppSelfTest_FormatSensorStatus(char *buffer, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* __APP_SELFTEST_H */
