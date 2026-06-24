#include "prescription.h"

#include <stdio.h>

static const Prescription_t prescription_table[] =
{
  {
    0U,
    "Healthy",
    "No action required",
    0U,
    0U,
    0U,
    0U
  },
  {
    1U,
    "Leaf Spot",
    "Spray pesticide and improve ventilation",
    1U,
    1U,
    5000U,
    10000U
  },
  {
    2U,
    "Powdery Mildew",
    "Apply fungicide and improve ventilation",
    1U,
    1U,
    5000U,
    15000U
  },
  {
    3U,
    "Downy Mildew",
    "Avoid leaf wetness and improve ventilation",
    0U,
    1U,
    0U,
    20000U
  }
};

void Prescription_Init(void)
{
  printf("[PRESCRIPTION] init ok\r\n");
}

const Prescription_t *Prescription_Find(uint8_t disease_id)
{
  uint32_t i;

  for (i = 0U; i < (sizeof(prescription_table) / sizeof(prescription_table[0])); i++)
  {
    if (prescription_table[i].disease_id == disease_id)
    {
      return &prescription_table[i];
    }
  }

  return &prescription_table[0];
}

void Prescription_Print(const Prescription_t *prescription)
{
  if (prescription == NULL)
  {
    return;
  }

  printf("[PRESCRIPTION] disease=%s\r\n", prescription->disease_name);
  printf("[PRESCRIPTION] advice=%s\r\n", prescription->advice);
  printf("[PRESCRIPTION] need_spray=%u need_fan=%u\r\n",
         prescription->need_spray,
         prescription->need_fan);
  printf("[PRESCRIPTION] pump_time=%lu fan_time=%lu\r\n",
         (unsigned long)prescription->pump_time_ms,
         (unsigned long)prescription->fan_time_ms);
}
