#ifndef __ATA_H__
#define __ATA_H__

#include <common.h>
#include <hal.h>

typedef enum ata_controller_id
{
	ata_controller_primary = 0,
	ata_controller_secondary = 1
} ata_controller_id;

typedef enum ata_drive_id
{
	ata_drive_master = 0,
	ata_drive_slave = 1
} ata_drive_id;

typedef struct ata_private_data_t
{
	uint8_t drive;
} ata_private_data_t;

#define ata_reg_data       0x00
#define ata_reg_seccount0  0x02
#define ata_reg_lba0       0x03
#define ata_reg_lba1       0x04
#define ata_reg_lba2       0x05
#define ata_reg_hddevsel   0x06
#define ata_reg_command    0x07
#define ata_reg_status     0x07
#define ata_reg_altstatus  0x07
#define ata_cmd_identify   0xEC
#define ata_cmd_read_pio   0x20
#define ata_cmd_write_pio  0x30
#define ata_sr_bsy     	   0x80
#define ata_sr_err         0x01
#define ata_sr_drq         0x08
#define ata_ident_model    54
#define ata_primary_io 	   0x1F0
#define ata_secondary_io   0x170

extern void ata_init();
extern uint8_t ata_init_device(uint32_t id, uint32_t bus);
extern void ata_select_drive(uint8_t bus, uint8_t i);
extern void ata_poll(uint16_t io);
extern void ata_read_block(uint8_t *buf, uint32_t lba, uint32_t ata_drive);
extern void ata_write_block(uint8_t *buf, uint32_t lba, uint32_t ata_drive);
extern void ata_read(uint8_t *buf, uint32_t lba, uint32_t numsects, void *dev);

#endif