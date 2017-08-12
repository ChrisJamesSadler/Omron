#ifndef MEMORY_H
#define MEMORY_H

#include <common.h>

typedef struct alloc_t
{
	uint8_t status;
	uint32_t size;
} alloc_t;

extern uint32_t kernel_end;

extern void memory_init();
extern void* malloc(uint32_t sz);
extern void free(void* mem);

#endif