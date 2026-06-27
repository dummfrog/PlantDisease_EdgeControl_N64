#ifndef __BMP280_H
#define __BMP280_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BMP280_ADDR_7BIT  0x76U
#define BMP280_ADDR_HAL   (BMP280_ADDR_7BIT << 1U)
#define BMP280_CHIP_ID     0x58U
#define BME280_CHIP_ID     0x60U

uint8_t BMP280_Init(void);
uint8_t BMP280_Read(float *temperature_c, float *pressure_hpa);
uint8_t BMP280_IsBME280(void);
uint8_t BME280_ReadHumidity(float *humidity_percent);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H */
