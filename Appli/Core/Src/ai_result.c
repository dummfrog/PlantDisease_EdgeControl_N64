#include "ai_result.h"

#include <stdio.h>
#include <string.h>

void AIResult_Init(void)
{
  printf("[AI] init ok\r\n");
}

void AIResult_GetMock(AIResult_t *result)
{
  if (result == NULL)
  {
    return;
  }

  result->disease_id = 1U;
  (void)snprintf(result->disease_name, sizeof(result->disease_name), "Leaf Spot");
  result->confidence = 0.976f;
  result->valid = 1U;
}

uint8_t AIResult_IsValid(const AIResult_t *result)
{
  if (result == NULL)
  {
    return 0U;
  }

  if (result->valid != 1U)
  {
    return 0U;
  }

  return (result->confidence >= 0.85f) ? 1U : 0U;
}
