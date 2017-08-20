#include <common.h>
#include <cpu.h>
#include <hal.h>
#include <acpi.h>
#include <tasking.h>
#include <main.h>
#include <descriptors.h>
#include <lapic.h>
#include <ioapic.h>
#include <pic.h>
#include <pit.h>

extern uint32_t g_acpiCpuCount;
extern uint32_t g_localApicAddr;
extern multiboot_header_t* multiboot;
extern uint8_t textscreen_height;
uint8_t g_activeCpuCount;

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

    if (largestExtendedFunc >= 0x80000004)
    {
        char* name = malloc(48);
        cpuid(0x80000002, (uint32_t *)(name +  0), (uint32_t *)(name +  4), (uint32_t *)(name +  8), (uint32_t *)(name + 12));
        cpuid(0x80000003, (uint32_t *)(name + 16), (uint32_t *)(name + 20), (uint32_t *)(name + 24), (uint32_t *)(name + 28));
        cpuid(0x80000004, (uint32_t *)(name + 32), (uint32_t *)(name + 36), (uint32_t *)(name + 40), (uint32_t *)(name + 44));

        const char *p = name;
        while (*p == ' ')
        {
            ++p;
        }
        dev->name = name;
    }
}

void ap_entry()
{
    asm("cli");
    descriptors_init();
    LocalApicInit();
    IoApicInit();
    asm("sti");
    g_activeCpuCount++;
    //textscreen_height = 50;
    while(true) asm("hlt");
}

uint8_t detect_flag;
void detect_timeout()
{
    sleep(500);
    detect_flag = true;
}

void detect_cores()
{
    module_t* module = findMod("smp");
    if(!module)
    {
        printf("No mod");
        return;
    }
    uint32_t* ptr = (uint32_t*)0x1000;
    *ptr = (uint32_t)&ap_entry;
    uint32_t modsize = module->end - module->start;
    uint8_t* stores = malloc(modsize);
    memcpy(stores, (void*)0, modsize);
    memcpy((void*)0, (void*)module->start, modsize);
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
        LocalApicSendStartup(apicId, 0);
        sleep(10);
    }
    memcpy((void*)0, stores, modsize);
    free(stores);
    detect_flag = false;
    create_thread("detect cores timeout", (uint32_t)&detect_timeout);
    while(g_activeCpuCount != g_acpiCpuCount && detect_flag == false) asm("hlt");
}

void cpu_init()
{
	uint32_t cr4;
	asm("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= 0x200;
	asm("mov %0, %%cr4" :: "r"(cr4));
    uint16_t cw = 0x37F;
	asm("fldcw %0" :: "m"(cw));
    detect_cores();
    detect_cpu();
    LocalApicInit();
    IoApicInit();
}