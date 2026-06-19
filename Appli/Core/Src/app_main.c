#include "app_main.h"

#include "app_uart.h"
#include "buzzer.h"
#include "main.h"
#include "./LED/led.h"
#include <stdio.h>

#define BUZZER_TEST_INTERVAL_MS 5000U
#define BUZZER_TEST_ON_MS       100U
#define BUZZER_TEST_OFF_MS      50U

void App_Init(void)
{
  led_init();
  App_UART_Init(115200);
  printf("[BOOT] PlantDisease Edge Control Start\r\n");
  Buzzer_Init();
  printf("[BUZZER] init ok\r\n");
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

  printf("[APP] heartbeat\r\n");

  LED0(0);
  LED1(1);
  /* TODO: Replace blocking delay with a state machine when more app tasks are added. */
  HAL_Delay(500);

  LED0(1);
  LED1(0);
  HAL_Delay(500);
}
