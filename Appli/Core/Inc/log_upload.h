#ifndef __LOG_UPLOAD_H
#define __LOG_UPLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ai_result.h"
#include "prescription.h"
#include "sensor.h"

void LogUpload_Init(void);
void LogUpload_PrintJson(const AIResult_t *ai,
                         const Prescription_t *prescription,
                         uint8_t action_suppressed,
                         const SensorData_t *sensor);

#ifdef __cplusplus
}
#endif

#endif /* __LOG_UPLOAD_H */
