#include "bh1750.h"

#include "main.h"
#include <stdio.h>

#define BH1750_CMD_POWER_ON         0x01U
#define BH1750_CMD_RESET            0x07U
#define BH1750_CMD_CONT_H_RES_MODE  0x10U
#define BH1750_I2C_TIMEOUT_MS       100U

extern I2C_HandleTypeDef hi2c2;

static uint8_t BH1750_SendCommand(uint8_t command)
{
  return (HAL_I2C_Master_Transmit(&hi2c2,
                                  (uint16_t)BH1750_ADDR_HAL,
                                  &command,
                                  1U,
                                  BH1750_I2C_TIMEOUT_MS) == HAL_OK) ? 1U : 0U;
}

uint8_t BH1750_Init(void)
{
  if (BH1750_SendCommand(BH1750_CMD_POWER_ON) == 0U)
  {
    printf("[BH1750] init failed: power on\r\n");
    return 0U;
  }

  if (BH1750_SendCommand(BH1750_CMD_RESET) == 0U)
  {
    printf("[BH1750] init failed: reset\r\n");
    return 0U;
  }

  if (BH1750_SendCommand(BH1750_CMD_CONT_H_RES_MODE) == 0U)
  {
    printf("[BH1750] init failed: mode\r\n");
    return 0U;
  }

  printf("[BH1750] init ok: addr=0x%02X\r\n", BH1750_ADDR_7BIT);
  return 1U;
}

uint8_t BH1750_ReadLux(uint32_t *lux)
{
  uint8_t buffer[2];
  uint16_t raw;

  if (lux == NULL)
  {
    return 0U;
  }

  if (HAL_I2C_Master_Receive(&hi2c2,
                             (uint16_t)BH1750_ADDR_HAL,
                             buffer,
                             sizeof(buffer),
                             BH1750_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return 0U;
  }

  raw = (uint16_t)(((uint16_t)buffer[0] << 8U) | buffer[1]);
  *lux = ((uint32_t)raw * 10U) / 12U;
  return 1U;
}
