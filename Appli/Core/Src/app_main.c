#include "app_main.h"

#include "app_uart.h"
#include "buzzer.h"
#include "main.h"
#include "relay.h"
#include "./LED/led.h"
#include <stdio.h>

#define BUZZER_TEST_INTERVAL_MS 5000U
#define BUZZER_TEST_ON_MS       100U
#define BUZZER_TEST_OFF_MS      50U

#define RELAY_TEST_INTERVAL_MS  10000U

static void App_RelayTest_Process(uint32_t now_ms)
{
  static uint32_t last_toggle_ms = 0U;

  if ((now_ms - last_toggle_ms) >= RELAY_TEST_INTERVAL_MS)
  {
    last_toggle_ms = now_ms;
    Relay_ToggleAll();
    printf("[RELAY_TEST] t=%lu ms state=%s\r\n",
           (unsigned long)now_ms,
           (Relay_GetState(1U) != 0U) ? "ON" : "OFF");
    printf("[RELAY_TEST] readback CH1=%u CH2=%u\r\n",
           (HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_11) == GPIO_PIN_SET) ? 1U : 0U,
           (HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_12) == GPIO_PIN_SET) ? 1U : 0U);
  }
}

void App_Init(void)
{
  led_init();
  App_UART_Init(115200);
  printf("[BOOT] PlantDisease Edge Control Start\r\n");
  Buzzer_Init();
  printf("[BUZZER] init ok\r\n");
  Relay_Init();
  Relay_AllOff();
}

void App_Loop(void)
{
  static uint32_t last_buzzer_test_ms = 0U;
  uint32_t now_ms = HAL_GetTick();

  if ((now_ms - last_buzzer_test_ms) >= BUZZER_TEST_INTERVAL_MS)
  {
    last_buzzer_test_ms = now_ms;
    Buzzer_Beep(1U, BUZZER_TEST_ON_MS, BUZZER_TEST_OFF_MS);
  }

  App_RelayTest_Process(now_ms);

  printf("[APP] heartbeat\r\n");

  LED0(0);
  LED1(1);
  /* TODO: Replace blocking delay with a state machine when more app tasks are added. */
  HAL_Delay(500);

  LED0(1);
  LED1(0);
  HAL_Delay(500);
}
