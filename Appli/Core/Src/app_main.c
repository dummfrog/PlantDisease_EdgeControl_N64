#include "app_main.h"

#include "app_uart.h"
#include "main.h"
#include "./LED/led.h"
#include <stdio.h>

void App_Init(void)
{
  led_init();
  App_UART_Init(115200);
  printf("[BOOT] PlantDisease Edge Control Start\r\n");
}

void App_Loop(void)
{
  printf("[APP] heartbeat\r\n");

  LED0(0);
  LED1(1);
  /* TODO: Replace blocking delay with a state machine when more app tasks are added. */
  HAL_Delay(500);

  LED0(1);
  LED1(0);
  HAL_Delay(500);
}
