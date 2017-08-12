#ifndef PAGING_H
#define PAGING_H

#include <common.h>

extern void paging_init();
extern void paging_map_virtual_to_phys(uint32_t virt, uint32_t phys);

#endif