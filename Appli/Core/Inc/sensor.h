#ifndef __SENSOR_H
#define __SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
  float temperature_c;
  float humidity_percent;
  float pressure_hpa;
  uint32_t light_lux;
  uint8_t soil_moisture_percent;
  char liquid_level[8];
  uint16_t current_ma;
  uint8_t rain_detected;
} SensorData_t;

void Sensor_Init(void);
void Sensor_Update(SensorData_t *data);
void Sensor_Print(const SensorData_t *data);
uint8_t Sensor_I2CScan(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_H */
