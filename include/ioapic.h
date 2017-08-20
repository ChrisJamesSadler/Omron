#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#include <common.h>

extern uint8_t *g_ioApicAddr;

static inline uint32_t MmioRead32(uint8_t *p)
{
    return (uint32_t)*(volatile uint32_t *)(p);
}

static inline void MmioWrite32(uint8_t *p, uint32_t data)
{
    *(volatile uint32_t *)(p) = data;
}

extern void IoApicInit();
extern void IoApicSetEntry(uint8_t *base, uint8_t index, uint64_t data);

#endif