#ifndef __HAL_H__
#define __HAL_H__

#include <common.h>

#define DEVICE_ID_ATA 32
#define DEVICE_ID_PCI 33

#define DEVICE_TYPE_UNKNOWN 1
#define DEVICE_TYPE_CHAR 2
#define DEVICE_TYPE_BLOCK 3

#define PCI_CONFIG_ADDR                 0xcf8
#define PCI_CONFIG_DATA                 0xcfC

typedef uint8_t (*device_read_t)(uint8_t* buffer, uint32_t offset, uint32_t len, void* dev);
typedef uint8_t (*device_write_t)(uint8_t *buffer, uint32_t offset, uint32_t len, void* dev);

typedef struct device_t
{
	char *name;
	uint32_t unique_id;
	uint32_t dev_type;
	struct __fs_t *fs;
	device_read_t read;
	device_write_t write;
	void *priv;
} device_t;

typedef struct pci_private_data_t
{
    uint16_t vendorid;
    uint16_t deviceid;
    uint8_t classcode;
	uint32_t bus;
	uint32_t slot;
	uint32_t func;
} pci_private_data_t;

typedef enum PCIHeaderType
{
    PCIHeaderTypeNormal = 0x00,
    PCIHeaderTypeBridge = 0x01,
    PCIHeaderTypeCardbus = 0x02
} PCIHeaderType;

list_t* devices;

extern void hal_init();
extern void deviceadd(device_t* dev);

#endif