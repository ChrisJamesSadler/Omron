#include <common.h>
#include <memory.h>
#include <main.h>

uint32_t last_alloc = 0;
alloc_t* last_returned;
uint32_t heap_end = 0;
uint32_t heap_begin = 0;
uint32_t memory_used = 0;
mutex_t* malloc_mutex;

void memory_init()
{
	last_alloc = (uint32_t)(&kernel_end) + 0x1000;
	heap_begin = last_alloc;
    heap_end = (multiboot->mem_upper + multiboot->mem_lower) * 1024;
	debug("Kernel heap starts at %dKB with %dMB available\n", last_alloc / 1024, (multiboot->mem_upper + multiboot->mem_lower) / 1024);
	malloc_mutex = mutexcreate();
}

uint32_t needs_alligning;
void* malloc(uint32_t size)
{
	if(!size) return 0;
	mutexlock(malloc_mutex);
	uint8_t *mem = (uint8_t *)heap_begin;
	while((uint32_t)mem < last_alloc)
	{
		alloc_t *a = (alloc_t *)mem;
		if(!a->size)
		{
			goto nalloc;
		}
		if(a->status)
		{
			mem += a->size;
			mem += sizeof(alloc_t);
			mem += 4;
			continue;
		}
		if(a->size >= size)
		{
			if(needs_alligning)
			{
				if(((uint32_t)mem + sizeof(alloc_t)) & 0xFFFFF000)
				{
					goto skip;
				}
			}
			a->status = 1;
			debug("RE:Allocated %d bytes from %d to %d\n", size, mem + sizeof(alloc_t), mem + sizeof(alloc_t) + size);
			memset(mem + sizeof(alloc_t), 0, size);
			memory_used += size + sizeof(alloc_t);
			mutexunlock(malloc_mutex);
			return (char *)(mem + sizeof(alloc_t));
			skip:;
		}
		mem += a->size;
		mem += sizeof(alloc_t);
		mem += 4;
	}

	nalloc:;
	if(last_alloc+size+sizeof(alloc_t) >= heap_end)
	{
		PANIC("OUT OF MEMORY");
	}
	alloc_t *alloc = (alloc_t *)last_alloc;
	alloc->status = 1;
	alloc->size = size;

	last_alloc += size;
	last_alloc += sizeof(alloc_t);
	last_alloc += 4;
	debug("Allocated %d bytes from %d to %d\n", size, (uint32_t)alloc + sizeof(alloc_t), last_alloc);
	memory_used += size + 4 + sizeof(alloc_t);
	memset((char *)((uint32_t)alloc + sizeof(alloc_t)), 0, size);
	last_returned = alloc;
	mutexunlock(malloc_mutex);
	return (char *)((uint32_t)alloc + sizeof(alloc_t));
}

void* malloc_a(uint32_t size)
{
	needs_alligning = true;
	uint32_t addr = (uint32_t)(last_returned + last_returned->size + sizeof(alloc_t) + 4);
	if(last_alloc & 0xFFFFF000)
	{
		last_alloc &= 0xFFFFF000;
        last_alloc += 0x1000;
	}
	last_alloc -= sizeof(alloc_t);
	last_returned->size = last_alloc - addr;
	needs_alligning = false;
	return malloc(size);
}

void* malloc_ap(uint32_t size, uint32_t* ptr)
{
	needs_alligning = true;
	uint32_t addr = (uint32_t)(last_returned + last_returned->size + sizeof(alloc_t) + 4);
	if(last_alloc & 0xFFFFF000)
	{
		last_alloc &= 0xFFFFF000;
        last_alloc += 0x1000;
	}
	last_alloc -= sizeof(alloc_t);
	last_returned->size = last_alloc - addr;
	needs_alligning = false;
	*ptr = (uint32_t)malloc(size);
	return (void*)*ptr;
}

void free(void* mem)
{
	debug("DE:Allocated address %x\n", (uint32_t)mem);
	alloc_t *alloc = (alloc_t*)((uint32_t)mem - sizeof(alloc_t));
	alloc->status = 0;
	memset((char *)((uint32_t)alloc + sizeof(alloc_t)), 0, alloc->size);
}