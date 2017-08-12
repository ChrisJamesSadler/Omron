#include <main.h>
#include <textscreen.h>
#include <common.h>
#include <descriptors.h>
#include <memory.h>
#include <tasking.h>
#include <pit.h>
#include <services.h>
#include <shell.h>
#include <acpi.h>
#include <ata.h>
#include <hal.h>
#include <rtc.h>
#include <cpu.h>

extern uint32_t g_activeCpuCount;
uint32_t gatepassed;

void main()
{
    g_activeCpuCount++;
    if(gatepassed)
    {
        printf("Core %d booted\n", g_activeCpuCount);
        while(true) asm("hlt");
    }
    gatepassed = true;
	asm("movl %%ebx, %0": "=m"(multiboot));
    textscreen_clear();
    printf("Loading...\n");
    uint32_t* aptr = (uint32_t*)0;
    *aptr = (uint32_t)&main;
    printf("Main address is %x point is %x\n", &main, *(uint32_t*)0);
    descriptors_init();
    memory_init();
    acpi_init();
    //paging_init();
    rtc_init();
    pit_init();
    tasking_init();
    asm("sti");
    hal_init();
    ata_init();
    cpu_init();
    while(true){}
    services_init();
    create_thread("Shell", (uint32_t)&shell_main);
    while(true) asm("hlt");
}