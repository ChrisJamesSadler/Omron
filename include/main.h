#ifndef __MAIN_H__
#define __MAIN_H__

#include <common.h>

typedef struct mmap_field_t
{
    uint32_t size;
    uint32_t base_addr;
    uint32_t base_addr_high;
    uint32_t length;
    uint32_t length_high;
    uint32_t type;
}__attribute__ ((packed)) mmap_field_t;

typedef struct module_t
{
    uint32_t start;
    uint32_t end;
    char* name;
    int32_t reserved;
}__attribute__ ((packed)) module_t;

typedef struct multiboot_header_t
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    char *cmdline;
    uint32_t mods_count;
    struct module_t *mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    struct mmap_field *mmap_addr;
    uint32_t drives_length;
    uint32_t *drives_addr;
    uint32_t config_table;
    char *bootloader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
}__attribute__ ((packed)) multiboot_header_t;

multiboot_header_t* multiboot;
extern module_t* findMod(char* name);

#endif