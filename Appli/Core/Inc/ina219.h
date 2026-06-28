#ifndef __INA219_H
#define __INA219_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define INA219_ADDR_7BIT          0x40U
#define INA219_ADDR_HAL           (INA219_ADDR_7BIT << 1U)
#define INA219_R_SHUNT_OHMS       0.1f
#define INA219_CURRENT_LSB_UA     100U
#define INA219_CALIBRATION_VALUE  4096U

uint8_t INA219_Init(void);
uint8_t INA219_ReadCurrentMa(uint16_t *current_ma);
uint8_t INA219_ReadBusVoltageMv(uint16_t *bus_mv);
uint8_t INA219_ReadShuntVoltageUv(int32_t *shunt_uv);

#ifdef __cplusplus
}
#endif

#endif /* __INA219_H */
