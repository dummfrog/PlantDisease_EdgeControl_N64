#ifndef __PUMP_H
#define __PUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Pump_Init(void);
void Pump_On(void);
void Pump_Off(void);
void Pump_RunMs(uint32_t duration_ms);
void Pump_Task(void);
uint8_t Pump_IsRunning(void);

#ifdef __cplusplus
}
#endif

#endif /* __PUMP_H */
