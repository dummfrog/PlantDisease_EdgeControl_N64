#ifndef __BUZZER_H
#define __BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Beep(uint8_t times, uint32_t on_ms, uint32_t off_ms);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H */
