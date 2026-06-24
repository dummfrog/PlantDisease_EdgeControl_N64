#include "fan.h"

#include "main.h"
#include "relay.h"
#include <stdio.h>

#define FAN_RELAY_CHANNEL  2U

static uint8_t fan_running = 0U;
static uint32_t fan_stop_time_ms = 0U;

void Fan_Init(void)
{
  Relay_Off(FAN_RELAY_CHANNEL);
  fan_running = 0U;
  fan_stop_time_ms = 0U;
  printf("[FAN] init ok: relay_ch=2\r\n");
}

void Fan_On(void)
{
  Relay_On(FAN_RELAY_CHANNEL);
  fan_running = 1U;
}

void Fan_Off(void)
{
  Relay_Off(FAN_RELAY_CHANNEL);
  fan_running = 0U;
  fan_stop_time_ms = 0U;
  printf("[FAN] OFF\r\n");
}

void Fan_RunMs(uint32_t duration_ms)
{
  Fan_On();
  fan_stop_time_ms = HAL_GetTick() + duration_ms;
  printf("[FAN] ON, duration=%lu ms\r\n", (unsigned long)duration_ms);
}

void Fan_Task(void)
{
  if ((fan_running != 0U) && ((int32_t)(HAL_GetTick() - fan_stop_time_ms) >= 0))
  {
    Fan_Off();
  }
}

uint8_t Fan_IsRunning(void)
{
  return fan_running;
}
