#ifndef __PRESCRIPTION_H
#define __PRESCRIPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
  uint8_t disease_id;
  char disease_name[32];
  char advice[128];
  uint8_t need_spray;
  uint8_t need_fan;
  uint32_t pump_time_ms;
  uint32_t fan_time_ms;
} Prescription_t;

void Prescription_Init(void);
const Prescription_t *Prescription_Find(uint8_t disease_id);
void Prescription_Print(const Prescription_t *prescription);

#ifdef __cplusplus
}
#endif

#endif /* __PRESCRIPTION_H */
