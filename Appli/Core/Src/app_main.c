#include "app_main.h"

#include "ai_result.h"
#include "app_uart.h"
#include "buzzer.h"
#include "fan.h"
#include "log_upload.h"
#include "main.h"
#include "prescription.h"
#include "pump.h"
#include "relay.h"
#include "sensor.h"
#include "./LED/led.h"
#include <stdio.h>

#define BUZZER_TEST_INTERVAL_MS 5000U
#define BUZZER_TEST_ON_MS       100U
#define BUZZER_TEST_OFF_MS      50U

#define APP_BUSINESS_INTERVAL_MS 5000U
#define APP_SENSOR_INTERVAL_MS   2000U
#define APP_ACTION_COOLDOWN_MS   60000U
#define APP_ACTION_DISEASE_NONE  0xFFU

#define FAN_LOAD_TEST_ENABLE     0
#define FAN_LOAD_TEST_DELAY_MS   3000U
#define FAN_LOAD_TEST_RUN_MS     10000U

#define PUMP_LOAD_TEST_ENABLE    0
#define PUMP_LOAD_TEST_DELAY_MS  3000U
#define PUMP_LOAD_TEST_RUN_MS    3000U

static SensorData_t app_sensor_data;

#if FAN_LOAD_TEST_ENABLE
static void App_FanLoadTest_Process(uint32_t now_ms)
{
  static uint8_t test_started = 0U;
  static uint8_t off_reported = 0U;

  if ((test_started == 0U) && (now_ms >= FAN_LOAD_TEST_DELAY_MS))
  {
    test_started = 1U;
    printf("[FAN_TEST] start after 3000 ms\r\n");
    printf("[FAN_TEST] Fan_RunMs(10000)\r\n");
    Fan_RunMs(FAN_LOAD_TEST_RUN_MS);
    printf("[FAN_TEST] fan should be ON\r\n");
  }

  if ((test_started != 0U) && (off_reported == 0U) && (Fan_IsRunning() == 0U))
  {
    off_reported = 1U;
    printf("[FAN_TEST] fan should be OFF\r\n");
  }
}
#endif

#if PUMP_LOAD_TEST_ENABLE
static void App_PumpLoadTest_Process(uint32_t now_ms)
{
  static uint8_t test_started = 0U;
  static uint8_t off_reported = 0U;

  if ((test_started == 0U) && (now_ms >= PUMP_LOAD_TEST_DELAY_MS))
  {
    test_started = 1U;
    printf("[PUMP_TEST] start after 3000 ms\r\n");
    printf("[PUMP_TEST] Pump_RunMs(3000)\r\n");
    Pump_RunMs(PUMP_LOAD_TEST_RUN_MS);
    printf("[PUMP_TEST] pump should be ON\r\n");
  }

  if ((test_started != 0U) && (off_reported == 0U) && (Pump_IsRunning() == 0U))
  {
    off_reported = 1U;
    printf("[PUMP_TEST] pump should be OFF\r\n");
  }
}
#endif

#if !FAN_LOAD_TEST_ENABLE && !PUMP_LOAD_TEST_ENABLE
static uint8_t App_PrescriptionHasAction(const Prescription_t *prescription)
{
  if (prescription == NULL)
  {
    return 0U;
  }

  return ((prescription->need_spray != 0U) || (prescription->need_fan != 0U)) ? 1U : 0U;
}

static uint8_t App_ActionRequestAllowed(uint8_t disease_id, uint32_t now_ms)
{
  static uint8_t last_disease_id = APP_ACTION_DISEASE_NONE;
  static uint32_t last_request_ms = 0U;
  static uint8_t last_action_valid = 0U;

  if ((last_action_valid != 0U) &&
      (last_disease_id == disease_id) &&
      ((now_ms - last_request_ms) < APP_ACTION_COOLDOWN_MS))
  {
    printf("[ACTION] cooldown active for disease_id=%u, physical action suppressed\r\n",
           disease_id);
    return 0U;
  }

  last_disease_id = disease_id;
  last_request_ms = now_ms;
  last_action_valid = 1U;
  printf("[ACTION] request recorded, cooldown=%lu ms\r\n",
         (unsigned long)APP_ACTION_COOLDOWN_MS);
  return 1U;
}

static void App_Business_Process(uint32_t now_ms)
{
  static uint32_t last_business_ms = 0U;
  AIResult_t ai;
  const Prescription_t *prescription;
  uint32_t confidence_x10;
  uint8_t action_suppressed = 0U;

  if ((now_ms - last_business_ms) < APP_BUSINESS_INTERVAL_MS)
  {
    return;
  }

  last_business_ms = now_ms;
  AIResult_GetMock(&ai);
  confidence_x10 = (uint32_t)((ai.confidence * 1000.0f) + 0.5f);

  printf("[AI] Disease: %s\r\n", ai.disease_name);
  printf("[AI] Confidence: %lu.%lu%%\r\n",
         (unsigned long)(confidence_x10 / 10U),
         (unsigned long)(confidence_x10 % 10U));

  if (AIResult_IsValid(&ai) == 0U)
  {
    printf("[AI] invalid or low confidence, skip action\r\n");
    return;
  }

  prescription = Prescription_Find(ai.disease_id);
  Prescription_Print(prescription);

  if (prescription->need_spray != 0U)
  {
    printf("[ACTION] spray required, pump_time=%lu ms\r\n",
           (unsigned long)prescription->pump_time_ms);
  }

  if (prescription->need_fan != 0U)
  {
    printf("[ACTION] fan required, fan_time=%lu ms\r\n",
           (unsigned long)prescription->fan_time_ms);
  }

  if (App_PrescriptionHasAction(prescription) != 0U)
  {
    printf("[ACTION] request only, pump/fan not physically started\r\n");
    action_suppressed = (App_ActionRequestAllowed(ai.disease_id, now_ms) == 0U) ? 1U : 0U;
  }

  LogUpload_PrintJson(&ai, prescription, action_suppressed, &app_sensor_data);
}
#endif

static void App_Sensor_Process(uint32_t now_ms)
{
  static uint32_t last_sensor_ms = 0U;

  if ((now_ms - last_sensor_ms) < APP_SENSOR_INTERVAL_MS)
  {
    return;
  }

  last_sensor_ms = now_ms;
  Sensor_Update(&app_sensor_data);
  Sensor_Print(&app_sensor_data);
}

void App_Init(void)
{
  led_init();
  App_UART_Init(115200);
  printf("[BOOT] PlantDisease Edge Control Start\r\n");
  Buzzer_Init();
  printf("[BUZZER] init ok\r\n");
  Relay_Init();
  Pump_Init();
  Fan_Init();
  AIResult_Init();
  Prescription_Init();
  LogUpload_Init();
  Sensor_Init();
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

  Pump_Task();
  Fan_Task();
  App_Sensor_Process(now_ms);

#if FAN_LOAD_TEST_ENABLE
  App_FanLoadTest_Process(now_ms);
#elif PUMP_LOAD_TEST_ENABLE
  App_PumpLoadTest_Process(now_ms);
#else
  App_Business_Process(now_ms);
#endif

  printf("[APP] heartbeat\r\n");

  LED0(0);
  LED1(1);
  /* TODO: Replace blocking delay with a state machine when more app tasks are added. */
  HAL_Delay(500);

  LED0(1);
  LED1(0);
  HAL_Delay(500);
}
