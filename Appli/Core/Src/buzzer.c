#include "buzzer.h"

#include "main.h"

#define BUZZER_GPIO_PORT        GPIOD
#define BUZZER_GPIO_PIN         GPIO_PIN_3
#define BUZZER_ACTIVE_STATE     GPIO_PIN_SET
#define BUZZER_INACTIVE_STATE   GPIO_PIN_RESET

void Buzzer_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOD_CLK_ENABLE();
  HAL_GPIO_ConfigPinAttributes(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, BUZZER_INACTIVE_STATE);

  GPIO_InitStruct.Pin = BUZZER_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_PORT, &GPIO_InitStruct);
}

void Buzzer_On(void)
{
  HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, BUZZER_ACTIVE_STATE);
}

void Buzzer_Off(void)
{
  HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, BUZZER_INACTIVE_STATE);
}

void Buzzer_Beep(uint8_t times, uint32_t on_ms, uint32_t off_ms)
{
  uint8_t i;

  for (i = 0U; i < times; i++)
  {
    Buzzer_On();
    /* TODO: Replace blocking delay with a non-blocking buzzer state machine. */
    HAL_Delay(on_ms);
    Buzzer_Off();

    if ((i + 1U) < times)
    {
      HAL_Delay(off_ms);
    }
  }
}
