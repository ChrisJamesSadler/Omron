#include <ioapic.h>
#include <common.h>

extern uint8_t *g_ioApicAddr;

#define IOREGSEL                        0x00
#define IOWIN                           0x10
#define IOAPICID                        0x00
#define IOAPICVER                       0x01
#define IOAPICARB                       0x02
#define IOREDTBL                        0x10

void IoApicOut(uint8_t *base, uint8_t reg, uint32_t val)
{
    MmioWrite32(base + IOREGSEL, reg);
    MmioWrite32(base + IOWIN, val);
}

uint32_t IoApicIn(uint8_t *base, uint8_t reg)
{
    MmioWrite32(base + IOREGSEL, reg);
    return MmioRead32(base + IOWIN);
}

void IoApicSetEntry(uint8_t *base, uint8_t index, uint64_t data)
{
    IoApicOut(base, IOREDTBL + index * 2, (uint32_t)data);
    uint32_t x = 32;
    IoApicOut(base, IOREDTBL + index * 2 + 1, (uint32_t)(data >> x));
}

void IoApicInit()
{
    uint32_t x = IoApicIn(g_ioApicAddr, IOAPICVER);
    uint32_t count = ((x >> 16) & 0xff) + 1;
    for (uint32_t i = 0; i < count; ++i)
    {
        IoApicSetEntry(g_ioApicAddr, i, 1 << 16);
    }
}