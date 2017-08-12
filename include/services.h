#ifndef SERVICES_H
#define SERVICES_H

typedef void (*keyboard_event_t)(char aChar, uint8_t released);

extern void services_init();
extern void services_keyboard();
extern void services_keyboard_register(void* param);
extern void services_keyboard_unregister(void* param);

uint32_t kbd_ctrl;
uint32_t kbd_shift;

#endif