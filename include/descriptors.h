#ifndef __DESCRIPTORS_H__
#define __DESCRIPTORS_H__

#include <common.h>

#define IRQ_BASE                        0x20

#define IRQ_TIMER                       0x00
#define IRQ_KEYBOARD                    0x01
#define IRQ_COM2                        0x03
#define IRQ_COM1                        0x04
#define IRQ_FLOPPY                      0x06
#define IRQ_ATA0                        0x0e
#define IRQ_ATA1                        0x0f

#define INT_TIMER                       0x20
#define INT_SPURIOUS                    0xff

struct gdt_entry_struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

typedef struct gdt_ptr_t
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed))gdt_ptr_t;

typedef struct idt_entry_t
{
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed))idt_entry_t;

typedef struct idt_ptr_t
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed))idt_ptr_t;

typedef struct registers_t
{
	uint32_t gs, fs, es, ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32_t int_no, err_code;
	uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef uint32_t (*isr_t)(uint32_t oldEsp);

extern void descriptors_init();
extern void set_int(uint8_t num, void* base);
extern void send_eoi(uint8_t irq);
extern void gdt_init();
extern void idt_init();
extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();
extern void _isr15();
extern void _isr16();
extern void _isr17();
extern void _isr18();
extern void _isr19();
extern void _isr20();
extern void _isr21();
extern void _isr22();
extern void _isr23();
extern void _isr24();
extern void _isr25();
extern void _isr26();
extern void _isr27();
extern void _isr28();
extern void _isr29();
extern void _isr30();
extern void _isr31();
extern void _isr32();
extern void _isr33();
extern void _isr34();
extern void _isr35();
extern void _isr36();
extern void _isr37();
extern void _isr38();
extern void _isr39();
extern void _isr40();
extern void _isr41();
extern void _isr42();
extern void _isr43();
extern void _isr44();
extern void _isr45();
extern void _isr46();
extern void _isr47();

extern gdt_entry_t gdt_entries[5];
extern gdt_ptr_t   gdt_ptr;
extern idt_entry_t idt_entries[256];
extern idt_ptr_t   idt_ptr;
extern uint32_t interrupt_handlers[256];
extern uint8_t interrupt_flags[256];
extern void send_eoi(uint8_t irq);

#endif