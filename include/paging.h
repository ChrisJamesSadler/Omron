#ifndef __PAGING_H__
#define __PAGING_H__

#include <common.h>

extern void paging_init();
extern void paging_map_virtual_to_phys(uint32_t virt, uint32_t phys);

#endif