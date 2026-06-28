#include "sensor.h"

#include "app_selftest.h"
#include "bh1750.h"
#include "bmp280.h"
#include "ina219.h"
#include "main.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c2;

void Sensor_Init(void)
{
  printf("[SENSOR] init ok\r\n");
  AppSelfTest_Set(APP_SELFTEST_I2C, Sensor_I2CScan());
  AppSelfTest_Set(APP_SELFTEST_BH1750, BH1750_Init());
  AppSelfTest_Set(APP_SELFTEST_BME280, BMP280_Init());
  AppSelfTest_Set(APP_SELFTEST_INA219, INA219_Init());
}

void Sensor_Update(SensorData_t *data)
{
  uint8_t environment_read_ok;

  if (data == NULL)
  {
    return;
  }

  data->temperature_c = 28.6f;
  data->humidity_percent = 78.2f;
  data->pressure_hpa = 1013.2f;
  environment_read_ok = BMP280_Read(&data->temperature_c, &data->pressure_hpa);
  if (environment_read_ok == 0U)
  {
    data->temperature_c = 28.6f;
    data->pressure_hpa = 1013.2f;
    printf("[BMP/BME280] read failed, use mock env\r\n");
  }
  else if ((BMP280_IsBME280() != 0U) &&
           (BME280_ReadHumidity(&data->humidity_percent) == 0U))
  {
    data->humidity_percent = 78.2f;
    printf("[BME280] humidity read failed, use mock humidity\r\n");
  }
  if (BH1750_ReadLux(&data->light_lux) == 0U)
  {
    data->light_lux = 13500U;
    printf("[BH1750] read failed, use mock light\r\n");
  }
  data->soil_moisture_percent = 42U;
  (void)snprintf(data->liquid_level, sizeof(data->liquid_level), "OK");
  data->current_ma = 680U;
  if (INA219_ReadCurrentMa(&data->current_ma) == 0U)
  {
    data->current_ma = 680U;
    printf("[INA219] read failed, use mock current\r\n");
  }
  data->rain_detected = 0U;
}

void Sensor_Print(const SensorData_t *data)
{
  uint32_t temperature_x10;
  uint32_t humidity_x10;
  uint32_t pressure_x10;

  if (data == NULL)
  {
    return;
  }

  temperature_x10 = (uint32_t)((data->temperature_c * 10.0f) + 0.5f);
  humidity_x10 = (uint32_t)((data->humidity_percent * 10.0f) + 0.5f);
  pressure_x10 = (uint32_t)((data->pressure_hpa * 10.0f) + 0.5f);

  printf("[SENSOR] temperature=%lu.%lu C humidity=%lu.%lu %% pressure=%lu.%lu hPa\r\n",
         (unsigned long)(temperature_x10 / 10U),
         (unsigned long)(temperature_x10 % 10U),
         (unsigned long)(humidity_x10 / 10U),
         (unsigned long)(humidity_x10 % 10U),
         (unsigned long)(pressure_x10 / 10U),
         (unsigned long)(pressure_x10 % 10U));
  printf("[SENSOR] light=%lu lux soil=%u %% liquid=%s current=%u mA rain=%u\r\n",
         (unsigned long)data->light_lux,
         data->soil_moisture_percent,
         data->liquid_level,
         data->current_ma,
         data->rain_detected);
}

uint8_t Sensor_I2CScan(void)
{
  uint8_t address;
  uint8_t found_count = 0U;

  printf("[I2C_SCAN] start\r\n");

  for (address = 0x03U; address <= 0x77U; address++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(address << 1U), 2U, 10U) == HAL_OK)
    {
      printf("[I2C_SCAN] found device at 0x%02X\r\n", address);
      found_count++;
    }
  }

  if (found_count == 0U)
  {
    printf("[I2C_SCAN] no device found\r\n");
  }

  printf("[I2C_SCAN] done\r\n");
  return (found_count != 0U) ? 1U : 0U;
}
