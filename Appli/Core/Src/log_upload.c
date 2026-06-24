#include "log_upload.h"

#include "main.h"
#include <stdio.h>

#define LOG_UPLOAD_SCHEMA_VERSION   "1.0"
#define LOG_UPLOAD_DEVICE_ID        "Node01"
#define LOG_UPLOAD_TIMESTAMP        "2026-06-23T20:30:00+08:00"
#define LOG_UPLOAD_DISEASE          "Leaf_Spot"
#define LOG_UPLOAD_DISEASE_CN       "YeBanBing"
#define LOG_UPLOAD_TEMPERATURE_X10  286U
#define LOG_UPLOAD_HUMIDITY_X10     782U
#define LOG_UPLOAD_LIGHT_LUX        13500U
#define LOG_UPLOAD_SOIL_MOISTURE    42U
#define LOG_UPLOAD_LIQUID_LEVEL     "OK"
#define LOG_UPLOAD_CURRENT_MA       680U
#define LOG_UPLOAD_ALARM            "NONE"

static const char *LogUpload_GetRiskLevel(float confidence)
{
  if (confidence >= 0.90f)
  {
    return "HIGH";
  }

  if (confidence >= 0.75f)
  {
    return "MEDIUM";
  }

  return "LOW";
}

void LogUpload_Init(void)
{
  printf("[LOG] init ok\r\n");
}

void LogUpload_PrintJson(const AIResult_t *ai,
                         const Prescription_t *prescription,
                         uint8_t action_suppressed,
                         const SensorData_t *sensor_data)
{
  char json[768];
  uint32_t confidence_milli;
  uint32_t temperature_x10;
  uint32_t humidity_x10;
  uint32_t pump_duration_s;
  uint32_t fan_duration_s;
  uint32_t light_lux;
  uint32_t soil_moisture_percent;
  uint32_t current_ma;
  const char *liquid_level;
  const char *pump_action;
  const char *fan_action;
  int written;

  if ((ai == NULL) || (prescription == NULL))
  {
    printf("[LOG] invalid input\r\n");
    return;
  }

  confidence_milli = (uint32_t)((ai->confidence * 1000.0f) + 0.5f);
  if (sensor_data != NULL)
  {
    temperature_x10 = (uint32_t)((sensor_data->temperature_c * 10.0f) + 0.5f);
    humidity_x10 = (uint32_t)((sensor_data->humidity_percent * 10.0f) + 0.5f);
    light_lux = sensor_data->light_lux;
    soil_moisture_percent = sensor_data->soil_moisture_percent;
    liquid_level = sensor_data->liquid_level;
    current_ma = sensor_data->current_ma;
  }
  else
  {
    temperature_x10 = LOG_UPLOAD_TEMPERATURE_X10;
    humidity_x10 = LOG_UPLOAD_HUMIDITY_X10;
    light_lux = LOG_UPLOAD_LIGHT_LUX;
    soil_moisture_percent = LOG_UPLOAD_SOIL_MOISTURE;
    liquid_level = LOG_UPLOAD_LIQUID_LEVEL;
    current_ma = LOG_UPLOAD_CURRENT_MA;
  }

  pump_action = "OFF";
  fan_action = "OFF";

  if (prescription->need_spray != 0U)
  {
    pump_action = (action_suppressed != 0U) ? "SUPPRESSED" : "REQUEST";
  }

  if (prescription->need_fan != 0U)
  {
    fan_action = (action_suppressed != 0U) ? "SUPPRESSED" : "REQUEST";
  }
  pump_duration_s = prescription->pump_time_ms / 1000U;
  fan_duration_s = prescription->fan_time_ms / 1000U;

  written = snprintf(json,
                     sizeof(json),
                     "{\"schema_version\":\"%s\",\"device_id\":\"%s\","
                     "\"timestamp\":\"%s\",\"uptime_ms\":%lu,"
                     "\"disease_id\":%u,\"disease\":\"%s\","
                     "\"disease_cn\":\"%s\",\"confidence\":%lu.%03lu,"
                     "\"risk_level\":\"%s\","
                     "\"temperature_c\":%lu.%lu,\"humidity_percent\":%lu.%lu,"
                     "\"light_lux\":%lu,\"soil_moisture_percent\":%lu,"
                     "\"liquid_level\":\"%s\","
                     "\"pump_action\":\"%s\",\"fan_action\":\"%s\","
                     "\"pump_duration_s\":%lu,\"fan_duration_s\":%lu,"
                     "\"current_ma\":%lu,\"alarm\":\"%s\"}",
                     LOG_UPLOAD_SCHEMA_VERSION,
                     LOG_UPLOAD_DEVICE_ID,
                     LOG_UPLOAD_TIMESTAMP,
                     (unsigned long)HAL_GetTick(),
                     ai->disease_id,
                     LOG_UPLOAD_DISEASE,
                     LOG_UPLOAD_DISEASE_CN,
                     (unsigned long)(confidence_milli / 1000U),
                     (unsigned long)(confidence_milli % 1000U),
                     LogUpload_GetRiskLevel(ai->confidence),
                     (unsigned long)(temperature_x10 / 10U),
                     (unsigned long)(temperature_x10 % 10U),
                     (unsigned long)(humidity_x10 / 10U),
                     (unsigned long)(humidity_x10 % 10U),
                     (unsigned long)light_lux,
                     (unsigned long)soil_moisture_percent,
                     liquid_level,
                     pump_action,
                     fan_action,
                     (unsigned long)pump_duration_s,
                     (unsigned long)fan_duration_s,
                     (unsigned long)current_ma,
                     LOG_UPLOAD_ALARM);

  if ((written < 0) || ((uint32_t)written >= sizeof(json)))
  {
    printf("[LOG] json buffer too small\r\n");
    return;
  }

  printf("[LOG] %s\r\n", json);
}
