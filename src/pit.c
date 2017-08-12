#include <common.h>
#include <descriptors.h>
#include <tasking.h>

void __pit_send_cmd(uint8_t cmd)
{
	outb(0x43, cmd);
}

void __pit_send_data(uint16_t data, uint8_t counter)
{
	uint8_t	port = (counter==0) ? 0x40 :
		((counter==0x40 ) ? 0x41 : 0x42);
	outb(port, (uint8_t)data);
}

void pit_start_counter (uint32_t freq, uint8_t counter, uint8_t mode)
{
	debug("Starting counter %d with frequency %dHz\n", counter/0x40, freq);
	uint16_t divisor = (uint16_t)( 1193181 / (uint16_t)freq);
	uint8_t ocw = 0;
	ocw = (ocw & ~0xE) | mode;
	ocw = (ocw & ~0x30) | 0x30;
	ocw = (ocw & ~0xC0 ) | counter;
	__pit_send_cmd (ocw);
	__pit_send_data (divisor & 0xff, 0);
	__pit_send_data ((divisor >> 8) & 0xff, 0);
}

void pit_irq()
{
	tasking_switch();
}

void pit_init()
{
    set_int(32, &pit_irq);
	pit_start_counter (1000, 0, 0x6);
	debug("PIT was successfully enabled!\n");
}