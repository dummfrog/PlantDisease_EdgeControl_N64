#ifndef __FAN_H
#define __FAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Fan_Init(void);
void Fan_On(void);
void Fan_Off(void);
void Fan_RunMs(uint32_t duration_ms);
void Fan_Task(void);
uint8_t Fan_IsRunning(void);

#ifdef __cplusplus
}
#endif

#endif /* __FAN_H */
