#include <common.h>
#include <cpu.h>
#include <hal.h>
#include <acpi.h>
#include <tasking.h>
#include <main.h>

extern uint32_t g_acpiCpuCount;
extern multiboot_header_t* multiboot;

void cpuid(uint32_t reg, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ __volatile("cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "0" (reg));
}

void detect_cpu()
{
    device_t *dev = (device_t *)malloc(sizeof(device_t));
    cpu_private_data_t *priv = (cpu_private_data_t*)malloc(sizeof(cpu_private_data_t));
    dev->priv = priv;
    deviceadd(dev);
    // Register storage
    uint32_t eax, ebx;

    // Function 0x00 - Vendor-ID and Largest Standard Function

    uint32_t largestStandardFunc;
    dev->name = (char*)malloc(13);
    cpuid(0, &largestStandardFunc, (uint32_t *)(dev->name + 0), (uint32_t *)(dev->name + 8), (uint32_t *)(dev->name + 4));

    debug("CPU Vendor: %s\n", dev->name);

    // Function 0x01 - Feature Information

    if (largestStandardFunc >= 0x01)
    {
        cpuid(0x01, &eax, &ebx, &priv->ecx, &priv->edx);

        debug("Features:");

        if (priv->edx & EDX_PSE)      debug(" PSE");
        if (priv->edx & EDX_PAE)      debug(" PAE");
        if (priv->edx & EDX_APIC)     debug(" APIC");
        if (priv->edx & EDX_MTRR)     debug(" MTRR");

        debug("\n");

        debug("Instructions:");

        if (priv->edx & EDX_TSC)      debug(" TSC");
        if (priv->edx & EDX_MSR)      debug(" MSR");
        if (priv->edx & EDX_SSE)      debug(" SSE");
        if (priv->edx & EDX_SSE2)     debug(" SSE2");
        if (priv->ecx & ECX_SSE3)     debug(" SSE3");
        if (priv->ecx & ECX_SSSE3)    debug(" SSSE3");
        if (priv->ecx & ECX_SSE41)    debug(" SSE41");
        if (priv->ecx & ECX_SSE42)    debug(" SSE42");
        if (priv->ecx & ECX_AVX)      debug(" AVX");
        if (priv->ecx & ECX_F16C)     debug(" F16C");
        if (priv->ecx & ECX_RDRAND)   debug(" RDRAND");

        debug("\n");
    }

    // Extended Function 0x00 - Largest Extended Function

    uint32_t largestExtendedFunc;
    cpuid(0x80000000, &largestExtendedFunc, &ebx, &priv->ecx, &priv->edx);

    // Extended Function 0x01 - Extended Feature Bits

    if (largestExtendedFunc >= 0x80000001)
    {
        cpuid(0x80000001, &eax, &ebx, &priv->ecx, &priv->edx);

        if (priv->edx & EDX_64_BIT)
        {
            debug("64-bit Architecture\n");
        }
    }

    // Extended Function 0x02-0x04 - Processor Name / Brand String

    if (largestExtendedFunc >= 0x80000004)
    {
        char* name = malloc(48);
        cpuid(0x80000002, (uint32_t *)(name +  0), (uint32_t *)(name +  4), (uint32_t *)(name +  8), (uint32_t *)(name + 12));
        cpuid(0x80000003, (uint32_t *)(name + 16), (uint32_t *)(name + 20), (uint32_t *)(name + 24), (uint32_t *)(name + 28));
        cpuid(0x80000004, (uint32_t *)(name + 32), (uint32_t *)(name + 36), (uint32_t *)(name + 40), (uint32_t *)(name + 44));

        // Processor name is right justified with leading spaces
        const char *p = name;
        while (*p == ' ')
        {
            ++p;
        }
        dev->name = name;
        name = malloc(40);
        memset(name, ' ', strlen(dev->name) + 7);
        for(uint32_t i = 0; i < strlen(dev->name); i++)
        {
            if(!memcmp(dev->name + i, "CPU", 3))
            {
                memcpy(name, dev->name, i);
                name[i] = '0' + g_acpiCpuCount;
                name[i + 1] = ' ';
                name[i + 2] = 'C';
                name[i + 3] = 'o';
                name[i + 4] = 'r';
                name[i + 5] = 'e';
                name[i + 6] = 's';
                memcpy(name + i + 8, dev->name + i, strlen(dev->name) - i);
            }
        }
        free(dev->name);
        dev->name = name;
    }
}

extern uint32_t g_localApicAddr;
uint8_t g_activeCpuCount;

uint32_t LocalApicIn(uint32_t reg)
{
    return MmioRead32((uint8_t*)(g_localApicAddr + reg));
}

void LocalApicOut(uint32_t reg, uint32_t data)
{
    MmioWrite32((uint8_t*)(g_localApicAddr + reg), data);
}

void LocalApicSendInit(uint32_t apic_id)
{
    LocalApicOut(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
    LocalApicOut(LAPIC_ICRLO, ICR_INIT | ICR_PHYSICAL
        | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);

    while (LocalApicIn(LAPIC_ICRLO) & ICR_SEND_PENDING)
        ;
}

void LocalApicSendStartup(uint32_t apic_id, uint32_t vector)
{
    LocalApicOut(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
    LocalApicOut(LAPIC_ICRLO, vector | ICR_STARTUP
        | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);

    while (LocalApicIn(LAPIC_ICRLO) & ICR_SEND_PENDING)
        ;
}

uint32_t LocalApicGetId()
{
    return LocalApicIn(LAPIC_ID) >> 24;
}

void detect_cores()
{
    memcpy((void*)0x8000, (void*)((module_t*)multiboot->mods_addr)->start, ((module_t*)multiboot->mods_addr)->end - ((module_t*)multiboot->mods_addr)->start);
    printf("Waking up all CPUs\n");
    uint32_t localId = LocalApicGetId();
    sleep(1);
    for(uint32_t core = 0; core < g_acpiCpuCount; core++)
    {
        uint32_t apicId = g_acpiCpuIds[core];
        if (apicId == localId)
        {
            continue;
        }
        LocalApicSendInit(apicId);
        sleep(10);
        LocalApicSendStartup(apicId, 8);
        sleep(2000);
    }
    if (g_activeCpuCount == g_acpiCpuCount)
    {
        printf("All %d CPUs activated\n", g_acpiCpuCount - g_activeCpuCount);
    }
    else
    {
        printf("Failed to init %d CPUs\n", g_acpiCpuCount - g_activeCpuCount);
    }
}

void cpu_init()
{
	uint32_t cr4;
	asm("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= 0x200;
	asm("mov %0, %%cr4" :: "r"(cr4));
    uint16_t cw = 0x37F;
	asm("fldcw %0" :: "m"(cw));
    detect_cpu();
    detect_cores();
}