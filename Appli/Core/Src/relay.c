#include "relay.h"

#include "board_config.h"
#include <stdio.h>

#if RELAY_ACTIVE_LOW
#define RELAY_ON_STATE   GPIO_PIN_RESET
#define RELAY_OFF_STATE  GPIO_PIN_SET
#else
#define RELAY_ON_STATE   GPIO_PIN_SET
#define RELAY_OFF_STATE  GPIO_PIN_RESET
#endif

#define RELAY_CHANNEL_1   1U
#define RELAY_CHANNEL_2   2U

static uint8_t relay_state_ch1 = 0U;
static uint8_t relay_state_ch2 = 0U;

static void Relay_Write(uint8_t channel, GPIO_PinState state)
{
  switch (channel)
  {
    case RELAY_CHANNEL_1:
      HAL_GPIO_WritePin(RELAY1_GPIO_PORT, RELAY1_GPIO_PIN, state);
      break;

    case RELAY_CHANNEL_2:
      HAL_GPIO_WritePin(RELAY2_GPIO_PORT, RELAY2_GPIO_PIN, state);
      break;

    default:
      break;
  }
}

static void Relay_SetState(uint8_t channel, uint8_t on)
{
  GPIO_PinState pin_state = on ? RELAY_ON_STATE : RELAY_OFF_STATE;

  Relay_Write(channel, pin_state);

  if (channel == RELAY_CHANNEL_1)
  {
    relay_state_ch1 = on ? 1U : 0U;
  }
  else if (channel == RELAY_CHANNEL_2)
  {
    relay_state_ch2 = on ? 1U : 0U;
  }
}

void Relay_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  RELAY1_GPIO_CLK_ENABLE();
  RELAY2_GPIO_CLK_ENABLE();

  HAL_GPIO_ConfigPinAttributes(RELAY1_GPIO_PORT, RELAY1_GPIO_PIN, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(RELAY2_GPIO_PORT, RELAY2_GPIO_PIN, GPIO_PIN_SEC | GPIO_PIN_NPRIV);

  HAL_GPIO_WritePin(RELAY1_GPIO_PORT, RELAY1_GPIO_PIN, RELAY_OFF_STATE);
  HAL_GPIO_WritePin(RELAY2_GPIO_PORT, RELAY2_GPIO_PIN, RELAY_OFF_STATE);
  relay_state_ch1 = 0U;
  relay_state_ch2 = 0U;

  GPIO_InitStruct.Pin = RELAY1_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RELAY1_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = RELAY2_GPIO_PIN;
  HAL_GPIO_Init(RELAY2_GPIO_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(RELAY1_GPIO_PORT, RELAY1_GPIO_PIN, RELAY_OFF_STATE);
  HAL_GPIO_WritePin(RELAY2_GPIO_PORT, RELAY2_GPIO_PIN, RELAY_OFF_STATE);
  printf("[RELAY] init ok: CH1=PF11 CH2=PF12 active_low=%u\r\n", (unsigned int)RELAY_ACTIVE_LOW);
}

void Relay_On(uint8_t channel)
{
  Relay_SetState(channel, 1U);
}

void Relay_Off(uint8_t channel)
{
  Relay_SetState(channel, 0U);
}

void Relay_AllOn(void)
{
  Relay_On(RELAY_CHANNEL_1);
  Relay_On(RELAY_CHANNEL_2);
  printf("[RELAY] ALL ON / CLOSED\r\n");
}

void Relay_AllOff(void)
{
  Relay_Off(RELAY_CHANNEL_1);
  Relay_Off(RELAY_CHANNEL_2);
  printf("[RELAY] ALL OFF / OPEN\r\n");
}

void Relay_ToggleAll(void)
{
  if ((relay_state_ch1 != 0U) || (relay_state_ch2 != 0U))
  {
    Relay_AllOff();
  }
  else
  {
    Relay_AllOn();
  }
}

uint8_t Relay_GetState(uint8_t channel)
{
  if (channel == RELAY_CHANNEL_1)
  {
    return relay_state_ch1;
  }

  if (channel == RELAY_CHANNEL_2)
  {
    return relay_state_ch2;
  }

  return 0U;
}
