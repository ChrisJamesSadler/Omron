#include <common.h>
#include <lapic.h>
#include <ioapic.h>

extern uint8_t* g_localApicAddr;

uint32_t LocalApicIn(uint32_t reg)
{
    return MmioRead32(g_localApicAddr + reg);
}

void LocalApicOut(uint32_t reg, uint32_t data)
{
    MmioWrite32(g_localApicAddr + reg, data);
}

void LocalApicInit()
{
    LocalApicOut(LAPIC_TPR, 0);
    LocalApicOut(LAPIC_DFR, 0xffffffff);
    LocalApicOut(LAPIC_LDR, 0x01000000);
    LocalApicOut(LAPIC_SVR, 0x100 | 0xff);
}

uint32_t LocalApicGetId()
{
    return LocalApicIn(LAPIC_ID) >> 24;
}

void LocalApicSendInit(uint32_t apic_id)
{
    LocalApicOut(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
    LocalApicOut(LAPIC_ICRLO, ICR_INIT | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);
    while (LocalApicIn(LAPIC_ICRLO) & ICR_SEND_PENDING);
}

void LocalApicSendStartup(uint32_t apic_id, uint32_t vector)
{
    LocalApicOut(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
    LocalApicOut(LAPIC_ICRLO, vector | ICR_STARTUP | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);
    while (LocalApicIn(LAPIC_ICRLO) & ICR_SEND_PENDING);
}

void LocalApicSendIPC(uint32_t apic_id, uint32_t vector)
{
    LocalApicOut(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
    LocalApicOut(LAPIC_ICRLO, vector | ICR_PHYSICAL | ICR_ASSERT | ICR_ALL_EXCLUDING_SELF | ICR_SEND_PENDING);
    while (LocalApicIn(LAPIC_ICRLO) & ICR_SEND_PENDING);
}