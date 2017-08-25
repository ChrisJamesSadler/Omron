#include <common.h>
#include <descriptors.h>
#include <tasking.h>

gdt_entry_t gdt_entries[5];
gdt_ptr_t   gdt_ptr;
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;
uint32_t interrupt_handlers[256];
uint8_t interrupt_flags[256];

extern void gdt_flush(uint32_t);
extern void idt_flush(uint32_t);
extern void idt_blank_handler();

char *system_exceptions[] =
{
	"Division by zero",
	"Debug",
	"Non-maskable interrupt",
	"Breakpoint",
	"Detected overflow",
	"Out-of-bounds",
	"Invalid opcode",
	"No coprocessor",
	"Double fault",
	"Coprocessor segment overrun",
	"Bad TSS",
	"Segment not present",
	"Stack fault",
	"General protection fault",
	"Page fault",
	"Unknown interrupt",
	"Coprocessor fault",
	"Alignment check",
	"Machine check",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;
    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

void idt_set_gate(uint8_t num, void* base, uint16_t sel, uint8_t flags)
{
    idt_entries[num].base_lo = (uint32_t)base & 0xFFFF;
    idt_entries[num].base_hi = ((uint32_t)base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags /* | 0x60 */;
}

void gdt_init()
{
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;
    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    gdt_flush((uint32_t)&gdt_ptr);
}

void isr_deadlock()
{
	while(1)
	{
		//asm("hlt");
	}
}

uint32_t isr_handler(uint32_t esp)
{
	registers_t* r = (registers_t*)esp;
	send_eoi(r->int_no);
	if (interrupt_handlers[r->int_no] != 0)
	{
		isr_t handler = (isr_t)interrupt_handlers[r->int_no];
		uint32_t nsp = handler(esp);
		if(nsp == 0)
		{
			nsp = esp;
		}
		return nsp;
	}
	else
	{
		if(r->int_no < 32)
		{
			if(thread_current == 0)
			{
				debug("Registers dump:\n");
				debug("eax 0x%x ebx 0x%x 0x%ecx 0x%x %edx 0x%x\n", r->eax, r->ebx, r->ecx, r->edx);
				debug("edi 0x%x esi 0x%x %ebp 0x%x %esp 0x%x\n", r->edi, r->esi, r->ebp, r->esp);
				debug("eip 0x%x cs 0x%x ss 0x%x eflags 0x%x useresp 0x%x\n", r->eip, r->ss, r->eflags, r->useresp);
				PANIC(system_exceptions[r->int_no]);
			}
			else
			{
				if(listlength(thread_current->catcher) > 0)
				{
					poplast(thread_current->catcher, &r->eip);
				}
				else
				{
					debug("Thread Error Occurred\n");
					debug("NAME  -- %s\n", thread_current->name);
					debug("TID   -- %d\n", thread_current->tid);
					debug("ERROR -- %s\n", system_exceptions[r->int_no]);
					debug("Registers dump:\n");
					debug("eax 0x%x ebx 0x%x 0x%ecx 0x%x %edx 0x%x\n", r->eax, r->ebx, r->ecx, r->edx);
					debug("edi 0x%x esi 0x%x %ebp 0x%x %esp 0x%x\n", r->edi, r->esi, r->ebp, r->esp);
					debug("eip 0x%x cs 0x%x ss 0x%x eflags 0x%x useresp 0x%x\n", r->eip, r->ss, r->eflags, r->useresp);
					r->eip = (uint32_t)&isr_deadlock;
					thread_current->arg = r->int_no;
					thread_current->state = THREAD_STATE_ZOMBIE;
					for(int i = 0 ; i < listlength(mutexlist); i++)
					{
						mutex_t* mtx;
						peekitem(mutexlist, i, (uint32_t*)&mtx);
						if(mtx->locker == thread_current->tid)
						{
							mutexunlock(mtx);
						}
					}
				}
			}
		}
		else
		{
			if(interrupt_flags[r->int_no] == 0)
			{
				interrupt_flags[r->int_no] = 1;
				debug("unhandled interrupt: 0x%x\n", r->int_no - 32);
			}
		}
	}
	return esp;
}

void idt_init()
{
    idt_ptr.limit = sizeof(idt_entry_t) * 256 -1;
    idt_ptr.base  = (uint32_t)&idt_entries;
    memset(&idt_entries, 0, sizeof(idt_entry_t)*256);
	for(int i = 0 ; i < 256; i++)
	{
		idt_set_gate(i, &idt_blank_handler, 0x08, 0x8E);
	}
    idt_set_gate(0, &_isr0, 0x08, 0x8E);
	idt_set_gate(1, &_isr1, 0x08, 0x8E);
	idt_set_gate(2, &_isr2, 0x08, 0x8E);
	idt_set_gate(3, &_isr3, 0x08, 0x8E);
	idt_set_gate(4, &_isr4, 0x08, 0x8E);
	idt_set_gate(5, &_isr5, 0x08, 0x8E);
	idt_set_gate(6, &_isr6, 0x08, 0x8E);
	idt_set_gate(7, &_isr7, 0x08, 0x8E);
	idt_set_gate(8, &_isr8, 0x08, 0x8E);
	idt_set_gate(9, &_isr9, 0x08, 0x8E);
	idt_set_gate(10, &_isr10, 0x08, 0x8E);
	idt_set_gate(11, &_isr11, 0x08, 0x8E);
	idt_set_gate(12, &_isr12, 0x08, 0x8E);
	idt_set_gate(13, &_isr13, 0x08, 0x8E);
	idt_set_gate(14, &_isr14, 0x08, 0x8E);
	idt_set_gate(15, &_isr15, 0x08, 0x8E);
	idt_set_gate(16, &_isr16, 0x08, 0x8E);
	idt_set_gate(17, &_isr17, 0x08, 0x8E);
	idt_set_gate(18, &_isr18, 0x08, 0x8E);
	idt_set_gate(19, &_isr19, 0x08, 0x8E);
	idt_set_gate(20, &_isr20, 0x08, 0x8E);
	idt_set_gate(21, &_isr21, 0x08, 0x8E);
	idt_set_gate(22, &_isr22, 0x08, 0x8E);
	idt_set_gate(23, &_isr23, 0x08, 0x8E);
	idt_set_gate(24, &_isr24, 0x08, 0x8E);
	idt_set_gate(25, &_isr25, 0x08, 0x8E);
	idt_set_gate(26, &_isr26, 0x08, 0x8E);
	idt_set_gate(27, &_isr27, 0x08, 0x8E);
	idt_set_gate(28, &_isr28, 0x08, 0x8E);
	idt_set_gate(29, &_isr29, 0x08, 0x8E);
	idt_set_gate(30, &_isr30, 0x08, 0x8E);
	idt_set_gate(31, &_isr31, 0x08, 0x8E);
    idt_set_gate(32, &_isr32, 0x08, 0x8E);
	idt_set_gate(33, &_isr33, 0x08, 0x8E);
	idt_set_gate(34, &_isr34, 0x08, 0x8E);
	idt_set_gate(35, &_isr35, 0x08, 0x8E);
	idt_set_gate(36, &_isr36, 0x08, 0x8E);
	idt_set_gate(37, &_isr37, 0x08, 0x8E);
	idt_set_gate(38, &_isr38, 0x08, 0x8E);
	idt_set_gate(39, &_isr39, 0x08, 0x8E);
	idt_set_gate(40, &_isr40, 0x08, 0x8E);
	idt_set_gate(41, &_isr41, 0x08, 0x8E);
	idt_set_gate(42, &_isr42, 0x08, 0x8E);
	idt_set_gate(43, &_isr43, 0x08, 0x8E);
	idt_set_gate(44, &_isr44, 0x08, 0x8E);
	idt_set_gate(45, &_isr45, 0x08, 0x8E);
	idt_set_gate(46, &_isr46, 0x08, 0x8E);
	idt_set_gate(47, &_isr47, 0x08, 0x8E);
    idt_flush((uint32_t)&idt_ptr);
}

void set_int(uint8_t num, void* base)
{
	debug("Registering IRQ#0x%x\n", num);
    interrupt_handlers[num] = (uint32_t)base;
}

void send_eoi(uint8_t irq)
{
	if(irq >= 8)
		outb(0xA0, 0x20);
	outb(0x20, 0x20);
}

void descriptors_init()
{
    gdt_init();
    idt_init();
}