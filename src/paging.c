#include <common.h>

uint32_t* page_directory = 0;
uint32_t page_dir_loc = 0;
uint32_t* last_page = 0;

void paging_map_virtual_to_phys(uint32_t virt, uint32_t phys)
{
	uint16_t id = virt >> 22;
	for(int i = 0; i < 1024; i++)
	{
		last_page[i] = phys | 3;
		phys += 4096;
	}
	page_directory[id] = ((uint32_t)last_page) | 3;
	last_page = (uint32_t *)(((uint32_t)last_page) + 4096);
	debug("Mapping %d (%d) to %d\n", virt, id, phys);
}

void paging_enable()
{
	asm("mov %%eax, %%cr3": :"a"(page_dir_loc));	
	asm("mov %cr0, %eax");
	asm("orl $0x80000000, %eax");
	asm("mov %eax, %cr0");
}

void paging_init()
{
	debug("Setting up paging\n");
	page_directory = (uint32_t*)0x400000;
	page_dir_loc = (uint32_t)page_directory;
	last_page = (uint32_t *)0x404000;
	for(int i = 0; i < 1024; i++)
	{
		page_directory[i] = 0 | 2;
	}
	paging_map_virtual_to_phys(0, 0);
	paging_map_virtual_to_phys(0x400000, 0x400000);
	paging_enable();
	debug("Paging was successfully enabled!\n");
}