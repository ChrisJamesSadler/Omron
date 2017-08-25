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
#include <pic.h>
#include <fs.h>

void main()
{
    asm("movl %%ebx, %0": "=m"(multiboot));
    textscreen_clear();
    printf("Loading...\n");
    descriptors_init();
    memory_init();
    acpi_init();
    //paging_init();
    rtc_init();
    pic_init();
    pit_init();
    tasking_init();
    asm("sti");
    hal_init();
    ata_init();
    cpu_init();
    fs_init();
    services_init();
    create_thread("Shell", (uint32_t)&shell_main);
    while(true) asm("hlt");
}

module_t* findMod(char* name)
{
    if (multiboot->mods_count > 0)
    {
        for (uint32_t i = 0; i < multiboot->mods_count; ++i )
        {
            module_t* mod = (module_t*)((uint32_t*)multiboot->mods_addr + (8 * i));
            if(strends(mod->name, name))
            {
                return mod;
            }
        }
    }
    return null;
}