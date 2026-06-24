#include "pump.h"

#include "main.h"
#include "relay.h"
#include <stdio.h>

#define PUMP_RELAY_CHANNEL  1U

static uint8_t pump_running = 0U;
static uint32_t pump_stop_time_ms = 0U;

void Pump_Init(void)
{
  Relay_Off(PUMP_RELAY_CHANNEL);
  pump_running = 0U;
  pump_stop_time_ms = 0U;
  printf("[PUMP] init ok: relay_ch=1\r\n");
}

void Pump_On(void)
{
  Relay_On(PUMP_RELAY_CHANNEL);
  pump_running = 1U;
}

void Pump_Off(void)
{
  Relay_Off(PUMP_RELAY_CHANNEL);
  pump_running = 0U;
  pump_stop_time_ms = 0U;
  printf("[PUMP] OFF\r\n");
}

void Pump_RunMs(uint32_t duration_ms)
{
  Pump_On();
  pump_stop_time_ms = HAL_GetTick() + duration_ms;
  printf("[PUMP] ON, duration=%lu ms\r\n", (unsigned long)duration_ms);
}

void Pump_Task(void)
{
  if ((pump_running != 0U) && ((int32_t)(HAL_GetTick() - pump_stop_time_ms) >= 0))
  {
    Pump_Off();
  }
}

uint8_t Pump_IsRunning(void)
{
  return pump_running;
}
