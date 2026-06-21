#ifndef __RELAY_H
#define __RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Relay_Init(void);
void Relay_On(uint8_t channel);
void Relay_Off(uint8_t channel);
void Relay_AllOn(void);
void Relay_AllOff(void);
void Relay_ToggleAll(void);
uint8_t Relay_GetState(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* __RELAY_H */
