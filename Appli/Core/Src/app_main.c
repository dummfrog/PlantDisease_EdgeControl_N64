#include "app_main.h"

#include "main.h"
#include "./LED/led.h"

void App_Init(void)
{
  led_init();
}

void App_Loop(void)
{
  LED0(0);
  LED1(1);
  /* TODO: Replace blocking delay with a state machine when more app tasks are added. */
  HAL_Delay(500);

  LED0(1);
  LED1(0);
  HAL_Delay(500);
}
