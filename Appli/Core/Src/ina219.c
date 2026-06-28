#include "ina219.h"

#include "main.h"
#include <stdio.h>

#define INA219_REG_CONFIGURATION  0x00U
#define INA219_REG_SHUNT_VOLTAGE  0x01U
#define INA219_REG_BUS_VOLTAGE    0x02U
#define INA219_REG_CURRENT        0x04U
#define INA219_REG_CALIBRATION    0x05U

#define INA219_CONFIG_VALUE       0x399FU
#define INA219_I2C_TIMEOUT_MS     100U

extern I2C_HandleTypeDef hi2c2;

static uint8_t ina219_initialized;

static uint8_t INA219_WriteRegister(uint8_t reg, uint16_t value)
{
  uint8_t bytes[2];

  bytes[0] = (uint8_t)(value >> 8U);
  bytes[1] = (uint8_t)(value & 0xFFU);

  return (HAL_I2C_Mem_Write(&hi2c2,
                            INA219_ADDR_HAL,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            bytes,
                            sizeof(bytes),
                            INA219_I2C_TIMEOUT_MS) == HAL_OK) ? 1U : 0U;
}

static uint8_t INA219_ReadRegister(uint8_t reg, uint16_t *value)
{
  uint8_t bytes[2];

  if (value == NULL)
  {
    return 0U;
  }

  if (HAL_I2C_Mem_Read(&hi2c2,
                       INA219_ADDR_HAL,
                       reg,
                       I2C_MEMADD_SIZE_8BIT,
                       bytes,
                       sizeof(bytes),
                       INA219_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return 0U;
  }

  *value = ((uint16_t)bytes[0] << 8U) | bytes[1];
  return 1U;
}

uint8_t INA219_Init(void)
{
  uint16_t calibration_readback;

  ina219_initialized = 0U;

  if (HAL_I2C_IsDeviceReady(&hi2c2,
                            INA219_ADDR_HAL,
                            3U,
                            INA219_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return 0U;
  }

  if ((INA219_WriteRegister(INA219_REG_CONFIGURATION,
                            INA219_CONFIG_VALUE) == 0U) ||
      (INA219_WriteRegister(INA219_REG_CALIBRATION,
                            INA219_CALIBRATION_VALUE) == 0U) ||
      (INA219_ReadRegister(INA219_REG_CALIBRATION,
                           &calibration_readback) == 0U) ||
      (calibration_readback != INA219_CALIBRATION_VALUE))
  {
    return 0U;
  }

  ina219_initialized = 1U;
  printf("[INA219] init ok: addr=0x40\r\n");
  return 1U;
}

uint8_t INA219_ReadCurrentMa(uint16_t *current_ma)
{
  uint16_t raw_value;
  int16_t signed_raw;

  if ((current_ma == NULL) || (ina219_initialized == 0U) ||
      (INA219_ReadRegister(INA219_REG_CURRENT, &raw_value) == 0U))
  {
    return 0U;
  }

  signed_raw = (int16_t)raw_value;
  if (signed_raw <= 0)
  {
    *current_ma = 0U;
  }
  else
  {
    *current_ma = (uint16_t)(((uint16_t)signed_raw + 5U) / 10U);
  }

  return 1U;
}

uint8_t INA219_ReadBusVoltageMv(uint16_t *bus_mv)
{
  uint16_t raw_value;

  if ((bus_mv == NULL) || (ina219_initialized == 0U) ||
      (INA219_ReadRegister(INA219_REG_BUS_VOLTAGE, &raw_value) == 0U))
  {
    return 0U;
  }

  *bus_mv = (uint16_t)((raw_value >> 3U) * 4U);
  return 1U;
}

uint8_t INA219_ReadShuntVoltageUv(int32_t *shunt_uv)
{
  uint16_t raw_value;

  if ((shunt_uv == NULL) || (ina219_initialized == 0U) ||
      (INA219_ReadRegister(INA219_REG_SHUNT_VOLTAGE, &raw_value) == 0U))
  {
    return 0U;
  }

  *shunt_uv = (int32_t)((int16_t)raw_value) * 10;
  return 1U;
}
