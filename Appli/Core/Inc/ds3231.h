#ifndef __DS3231_H
#define __DS3231_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DS3231_ADDR_7BIT  0x68U
#define DS3231_ADDR_HAL   (DS3231_ADDR_7BIT << 1U)

#define DS3231_SET_TIME_ON_BOOT  0
#define DS3231_BOOT_YEAR         2026U
#define DS3231_BOOT_MONTH        6U
#define DS3231_BOOT_DAY          28U
#define DS3231_BOOT_HOUR         3U
#define DS3231_BOOT_MINUTE       5U
#define DS3231_BOOT_SECOND       0U

typedef struct
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t valid;
} DS3231_Time_t;

uint8_t DS3231_Init(void);
uint8_t DS3231_SetTime(const DS3231_Time_t *time);
uint8_t DS3231_ReadTime(DS3231_Time_t *time);
uint8_t DS3231_FormatTimestamp(const DS3231_Time_t *time,
                               char *buffer,
                               uint32_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* __DS3231_H */
