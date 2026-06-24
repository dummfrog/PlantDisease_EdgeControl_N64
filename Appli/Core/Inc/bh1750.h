#ifndef __BH1750_H
#define __BH1750_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BH1750_ADDR_7BIT  0x23U
#define BH1750_ADDR_HAL   (BH1750_ADDR_7BIT << 1U)

uint8_t BH1750_Init(void);
uint8_t BH1750_ReadLux(uint32_t *lux);

#ifdef __cplusplus
}
#endif

#endif /* __BH1750_H */
