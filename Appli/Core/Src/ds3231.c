#include "ds3231.h"

#include "main.h"
#include <stdio.h>

#define DS3231_REG_SECONDS        0x00U
#define DS3231_TIME_REG_COUNT     7U
#define DS3231_I2C_TIMEOUT_MS     100U
#define DS3231_I2C_READY_TRIALS   3U

extern I2C_HandleTypeDef hi2c2;

static uint8_t ds3231_initialized;

static uint8_t DS3231_IsValidBcd(uint8_t value)
{
  return ((((value >> 4U) & 0x0FU) <= 9U) &&
          ((value & 0x0FU) <= 9U)) ? 1U : 0U;
}

static uint8_t DS3231_BcdToDecimal(uint8_t value)
{
  return (uint8_t)((((value >> 4U) & 0x0FU) * 10U) + (value & 0x0FU));
}

static uint8_t DS3231_DecimalToBcd(uint8_t value)
{
  return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

static uint8_t DS3231_IsLeapYear(uint16_t year)
{
  if ((year % 400U) == 0U)
  {
    return 1U;
  }

  if ((year % 100U) == 0U)
  {
    return 0U;
  }

  return ((year % 4U) == 0U) ? 1U : 0U;
}

static uint8_t DS3231_DaysInMonth(uint16_t year, uint8_t month)
{
  static const uint8_t days_per_month[12] =
  {
    31U, 28U, 31U, 30U, 31U, 30U,
    31U, 31U, 30U, 31U, 30U, 31U
  };

  if ((month == 0U) || (month > 12U))
  {
    return 0U;
  }

  if ((month == 2U) && (DS3231_IsLeapYear(year) != 0U))
  {
    return 29U;
  }

  return days_per_month[month - 1U];
}

static uint8_t DS3231_IsValidTimeFields(const DS3231_Time_t *time)
{
  if ((time == NULL) ||
      (time->year < 2000U) || (time->year > 2199U) ||
      (time->month == 0U) || (time->month > 12U) ||
      (time->day == 0U) ||
      (time->day > DS3231_DaysInMonth(time->year, time->month)) ||
      (time->hour > 23U) ||
      (time->minute > 59U) ||
      (time->second > 59U))
  {
    return 0U;
  }

  return 1U;
}

/* DS3231 weekday convention used here: Sunday=1 through Saturday=7. */
static uint8_t DS3231_CalculateWeekday(uint16_t year, uint8_t month, uint8_t day)
{
  static const uint8_t month_offset[12] =
  {
    0U, 3U, 2U, 5U, 0U, 3U,
    5U, 1U, 4U, 6U, 2U, 4U
  };
  uint32_t adjusted_year = year;
  uint32_t weekday;

  if (month < 3U)
  {
    adjusted_year--;
  }

  weekday = (adjusted_year +
             (adjusted_year / 4U) -
             (adjusted_year / 100U) +
             (adjusted_year / 400U) +
             month_offset[month - 1U] +
             day) % 7U;

  return (uint8_t)(weekday + 1U);
}

uint8_t DS3231_Init(void)
{
#if DS3231_SET_TIME_ON_BOOT
  const DS3231_Time_t boot_time =
  {
    DS3231_BOOT_YEAR,
    DS3231_BOOT_MONTH,
    DS3231_BOOT_DAY,
    DS3231_BOOT_HOUR,
    DS3231_BOOT_MINUTE,
    DS3231_BOOT_SECOND,
    1U
  };
  DS3231_Time_t readback_time;
  char timestamp[32];
#endif

  ds3231_initialized = 0U;

  if (HAL_I2C_IsDeviceReady(&hi2c2,
                            (uint16_t)DS3231_ADDR_HAL,
                            DS3231_I2C_READY_TRIALS,
                            DS3231_I2C_TIMEOUT_MS) != HAL_OK)
  {
    printf("[DS3231] init failed: device not ready\r\n");
    return 0U;
  }

  ds3231_initialized = 1U;
  printf("[DS3231] init ok: addr=0x%02X\r\n", DS3231_ADDR_7BIT);

#if DS3231_SET_TIME_ON_BOOT
  printf("[DS3231] WARNING: set time on boot is enabled\r\n");
  if ((DS3231_SetTime(&boot_time) == 0U) ||
      (DS3231_ReadTime(&readback_time) == 0U) ||
      (DS3231_FormatTimestamp(&readback_time,
                              timestamp,
                              sizeof(timestamp)) == 0U))
  {
    ds3231_initialized = 0U;
    printf("[DS3231] set time failed\r\n");
    return 0U;
  }

  printf("[DS3231] set time ok: %s\r\n", timestamp);
#endif

  return 1U;
}

uint8_t DS3231_SetTime(const DS3231_Time_t *time)
{
  uint8_t registers[DS3231_TIME_REG_COUNT];
  uint16_t year_offset;

  if ((ds3231_initialized == 0U) ||
      (DS3231_IsValidTimeFields(time) == 0U))
  {
    return 0U;
  }

  year_offset = (uint16_t)(time->year - 2000U);
  registers[0] = DS3231_DecimalToBcd(time->second);
  registers[1] = DS3231_DecimalToBcd(time->minute);
  registers[2] = DS3231_DecimalToBcd(time->hour);
  registers[3] = DS3231_CalculateWeekday(time->year,
                                         time->month,
                                         time->day);
  registers[4] = DS3231_DecimalToBcd(time->day);
  registers[5] = DS3231_DecimalToBcd(time->month);
  if (year_offset >= 100U)
  {
    registers[5] |= 0x80U;
  }
  registers[6] = DS3231_DecimalToBcd((uint8_t)(year_offset % 100U));

  if (HAL_I2C_Mem_Write(&hi2c2,
                        (uint16_t)DS3231_ADDR_HAL,
                        DS3231_REG_SECONDS,
                        I2C_MEMADD_SIZE_8BIT,
                        registers,
                        sizeof(registers),
                        DS3231_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return 0U;
  }

  return 1U;
}

uint8_t DS3231_ReadTime(DS3231_Time_t *time)
{
  uint8_t registers[DS3231_TIME_REG_COUNT];
  uint8_t seconds_bcd;
  uint8_t minutes_bcd;
  uint8_t hours_bcd;
  uint8_t date_bcd;
  uint8_t month_bcd;
  uint8_t year_bcd;
  uint8_t hour;
  uint8_t day_of_week;
  uint16_t year;

  if (time == NULL)
  {
    return 0U;
  }

  time->valid = 0U;
  if (ds3231_initialized == 0U)
  {
    return 0U;
  }

  if (HAL_I2C_Mem_Read(&hi2c2,
                       (uint16_t)DS3231_ADDR_HAL,
                       DS3231_REG_SECONDS,
                       I2C_MEMADD_SIZE_8BIT,
                       registers,
                       sizeof(registers),
                       DS3231_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return 0U;
  }

  seconds_bcd = registers[0] & 0x7FU;
  minutes_bcd = registers[1] & 0x7FU;
  date_bcd = registers[4] & 0x3FU;
  month_bcd = registers[5] & 0x1FU;
  year_bcd = registers[6];
  day_of_week = registers[3] & 0x07U;

  if ((DS3231_IsValidBcd(seconds_bcd) == 0U) ||
      (DS3231_IsValidBcd(minutes_bcd) == 0U) ||
      (DS3231_IsValidBcd(date_bcd) == 0U) ||
      (DS3231_IsValidBcd(month_bcd) == 0U) ||
      (DS3231_IsValidBcd(year_bcd) == 0U) ||
      (day_of_week == 0U) || (day_of_week > 7U))
  {
    return 0U;
  }

  if ((registers[2] & 0x40U) != 0U)
  {
    hours_bcd = registers[2] & 0x1FU;
    if (DS3231_IsValidBcd(hours_bcd) == 0U)
    {
      return 0U;
    }

    hour = DS3231_BcdToDecimal(hours_bcd);
    if ((hour == 0U) || (hour > 12U))
    {
      return 0U;
    }

    hour %= 12U;
    if ((registers[2] & 0x20U) != 0U)
    {
      hour = (uint8_t)(hour + 12U);
    }
  }
  else
  {
    hours_bcd = registers[2] & 0x3FU;
    if (DS3231_IsValidBcd(hours_bcd) == 0U)
    {
      return 0U;
    }

    hour = DS3231_BcdToDecimal(hours_bcd);
    if (hour > 23U)
    {
      return 0U;
    }
  }

  time->second = DS3231_BcdToDecimal(seconds_bcd);
  time->minute = DS3231_BcdToDecimal(minutes_bcd);
  time->month = DS3231_BcdToDecimal(month_bcd);
  time->day = DS3231_BcdToDecimal(date_bcd);
  time->hour = hour;
  year = (uint16_t)(2000U + DS3231_BcdToDecimal(year_bcd));
  if ((registers[5] & 0x80U) != 0U)
  {
    year = (uint16_t)(year + 100U);
  }
  time->year = year;

  if (DS3231_IsValidTimeFields(time) == 0U)
  {
    return 0U;
  }

  time->valid = 1U;
  return 1U;
}

uint8_t DS3231_FormatTimestamp(const DS3231_Time_t *time,
                               char *buffer,
                               uint32_t buffer_size)
{
  int written;

  if ((time == NULL) ||
      (buffer == NULL) ||
      (buffer_size == 0U) ||
      (time->valid == 0U))
  {
    return 0U;
  }

  written = snprintf(buffer,
                     buffer_size,
                     "%04u-%02u-%02uT%02u:%02u:%02u+08:00",
                     (unsigned int)time->year,
                     (unsigned int)time->month,
                     (unsigned int)time->day,
                     (unsigned int)time->hour,
                     (unsigned int)time->minute,
                     (unsigned int)time->second);

  if ((written < 0) || ((uint32_t)written >= buffer_size))
  {
    return 0U;
  }

  return 1U;
}
