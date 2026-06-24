#include "sensor.h"

#include <stdio.h>

#define SENSOR_I2C_HANDLE_AVAILABLE  0

void Sensor_Init(void)
{
  printf("[SENSOR] init ok\r\n");
  Sensor_I2CScan();
}

void Sensor_Update(SensorData_t *data)
{
  if (data == NULL)
  {
    return;
  }

  data->temperature_c = 28.6f;
  data->humidity_percent = 78.2f;
  data->pressure_hpa = 1013.2f;
  data->light_lux = 13500U;
  data->soil_moisture_percent = 42U;
  (void)snprintf(data->liquid_level, sizeof(data->liquid_level), "OK");
  data->current_ma = 680U;
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

void Sensor_I2CScan(void)
{
#if SENSOR_I2C_HANDLE_AVAILABLE
  /* TODO: Enable when CubeMX adds an I2C handle and HAL_I2C_MODULE_ENABLED. */
#else
  printf("[SENSOR] no I2C handle found, I2C scan skipped\r\n");
#endif
}
