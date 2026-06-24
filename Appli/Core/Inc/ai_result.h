#ifndef __AI_RESULT_H
#define __AI_RESULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
  uint8_t disease_id;
  char disease_name[32];
  float confidence;
  uint8_t valid;
} AIResult_t;

void AIResult_Init(void);
void AIResult_GetMock(AIResult_t *result);
uint8_t AIResult_IsValid(const AIResult_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __AI_RESULT_H */
