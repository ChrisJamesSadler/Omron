#include <common.h>
#include <ata.h>
#include <memory.h>
#include <hal.h>

uint8_t* ata_buf;
device_t* mainatadevice;

void ata_init()
{
    ata_buf = (uint8_t*)malloc(512);
	ata_init_device(ata_controller_primary, ata_drive_master);
	ata_init_device(ata_controller_primary, ata_drive_slave);
	ata_init_device(ata_controller_secondary, ata_drive_master);
	ata_init_device(ata_controller_secondary, ata_drive_slave);
}

uint8_t ata_init_device(ata_controller_id bus, ata_drive_id drive)
{
    uint16_t io = 0;
    ata_select_drive(bus, drive);
    if(bus == ata_controller_primary)
    {
        io = ata_primary_io;
    }
    else
    {
        io = ata_secondary_io;
    }
    outb(io + ata_reg_seccount0, 0);
	outb(io + ata_reg_lba0, 0);
	outb(io + ata_reg_lba1, 0);
	outb(io + ata_reg_lba2, 0);
    outb(io + ata_reg_command, ata_cmd_identify);
    uint8_t status = inb(io + ata_reg_status);
    if(status)
    {
        uint32_t trys = 0;
        while((inb(io + ata_reg_status) & ata_sr_bsy) != 0)
        {
            trys++;
            if(trys > 500000)
            {
                return 0;
            }
        }
        trys = 0;
pm_stat_read:
        trys++;
        status = inb(io + ata_reg_status);
		if(status & ata_sr_err)
		{
			return 0;
		}
        if(trys > 500000)
        {
            return 0;
        }
        while(!(status & ata_sr_drq)) goto pm_stat_read;
        for(int i = 0; i<256; i++)
        {
            *(uint16_t *)(ata_buf + i*2) = inw(io + ata_reg_data);
        }
        char *str = (char *)malloc(41);
		for(int i = 0; i < 40; i += 2)
		{
			str[i] = ata_buf[ata_ident_model + i + 1];
			str[i + 1] = ata_buf[ata_ident_model + i];
		}
		strtrim(str, ' ');
        debug("%s\n", str);
        device_t *dev = (device_t *)malloc(sizeof(device_t));
		dev->name = str;
		dev->id = DEVICE_ID_ATA;
		dev->type = DEVICE_TYPE_BLOCK;
		dev->priv = malloc(sizeof(ata_private_data_t));
		((ata_private_data_t*)dev->priv)->drive = (bus << 1) | drive;
		mainatadevice = dev;
		deviceadd(dev);
		return 1;
    }
	return 0;
}

void ata_select_drive(uint8_t bus, uint8_t i)
{
    if(bus == ata_controller_primary)
    {
        if(i == ata_drive_master)
        {
            outb(ata_primary_io + ata_reg_hddevsel, 0xA0);
        }
        else
        {
            outb(ata_primary_io + ata_reg_hddevsel, 0xB0);
        }
    }
    else
    {
        if(i == ata_drive_master)
        {
            outb(ata_secondary_io + ata_reg_hddevsel, 0xA0);
        }
        else
        {
            outb(ata_secondary_io + ata_reg_hddevsel, 0xB0);
        }
    }
}

void ata_read_block(uint8_t *buf, uint32_t lba, uint32_t ata_drive)
{
	uint16_t io = 0;
	switch(ata_drive)
	{
		case (ata_controller_primary << 1 | ata_drive_master):
    		ata_select_drive(ata_controller_primary, ata_drive_master);
			io = ata_primary_io;
			ata_drive = ata_drive_master;
			break;
		case (ata_controller_primary << 1 | ata_drive_slave):
    		ata_select_drive(ata_controller_primary, ata_drive_slave);
			io = ata_primary_io;
			ata_drive = ata_drive_slave;
			break;
		case (ata_controller_secondary << 1 | ata_drive_master):
    		ata_select_drive(ata_controller_secondary, ata_drive_master);
			io = ata_secondary_io;
			ata_drive = ata_drive_master;
			break;
		case (ata_controller_secondary << 1 | ata_drive_slave):
    		ata_select_drive(ata_controller_secondary, ata_drive_slave);
			io = ata_secondary_io;
			ata_drive = ata_drive_slave;
			break;
		default:
			return;
	}
	uint8_t cmd = (ata_drive==ata_drive_master?0xE0:0xF0);
	outb(io + ata_reg_hddevsel, (cmd | (uint8_t)((lba >> 24 & 0x0F))));
	outb(io + 1, 0x00);
	outb(io + ata_reg_seccount0, 1);
	outb(io + ata_reg_lba0, (uint8_t)((lba)));
	outb(io + ata_reg_lba1, (uint8_t)((lba) >> 8));
	outb(io + ata_reg_lba2, (uint8_t)((lba) >> 16));
	outb(io + ata_reg_command, ata_cmd_read_pio);

	ata_poll(io);
    
	for(int i = 0; i < 256; i++)
	{
		uint16_t data = inw(io + ata_reg_data);
		*(uint16_t *)(buf + i * 2) = data;
	}
    for(int i = 0;i < 4; i++)
		inb(io + ata_reg_altstatus);
}

void ata_write_block(uint8_t *buf, uint32_t lba, uint32_t ata_drive)
{
    uint16_t io = 0;
	switch(ata_drive)
	{
		case (ata_controller_primary << 1 | ata_drive_master):
    		ata_select_drive(ata_controller_primary, ata_drive_master);
			io = ata_primary_io;
			ata_drive = ata_drive_master;
			break;
		case (ata_controller_primary << 1 | ata_drive_slave):
    		ata_select_drive(ata_controller_primary, ata_drive_slave);
			io = ata_primary_io;
			ata_drive = ata_drive_slave;
			break;
		case (ata_controller_secondary << 1 | ata_drive_master):
    		ata_select_drive(ata_controller_secondary, ata_drive_master);
			io = ata_secondary_io;
			ata_drive = ata_drive_master;
			break;
		case (ata_controller_secondary << 1 | ata_drive_slave):
    		ata_select_drive(ata_controller_secondary, ata_drive_slave);
			io = ata_secondary_io;
			ata_drive = ata_drive_slave;
			break;
		default:
			return;
	}
	uint8_t cmd = (ata_drive==ata_drive_master?0xE0:0xF0);
	outb(io + ata_reg_hddevsel, (cmd | (uint8_t)((lba >> 24 & 0x0F))));
	outb(io + 1, 0x00);
	outb(io + ata_reg_seccount0, 1);
	outb(io + ata_reg_lba0, (uint8_t)((lba)));
	outb(io + ata_reg_lba1, (uint8_t)((lba) >> 8));
	outb(io + ata_reg_lba2, (uint8_t)((lba) >> 16));
	outb(io + ata_reg_command, ata_cmd_write_pio);

	ata_poll(io);
    
	for(int i = 0; i < 256; i++)
	{
		outw(io + ata_reg_data, *(uint16_t *)(buf + i * 2));
	}
    for(int i = 0;i < 4; i++)
		inb(io + ata_reg_altstatus);
}

void ata_poll(uint16_t io)
{
    uint32_t trys = 0;
	for(int i=0; i< 4; i++)
		inb(io + ata_reg_altstatus);
retry:;
	uint8_t status = inb(io + ata_reg_status);
    trys++;
    if(trys > 500000)
    {
        return;
    }
	if(status & ata_sr_bsy) goto retry;
    trys = 0;
retry2:	status = inb(io + ata_reg_status);
	if(status & ata_sr_err)
	{
		return;
	}
    trys++;
    if(trys > 500000)
    {
        return;
    }
	if(!(status & ata_sr_drq)) goto retry2;
	return;
}

void ata_read(uint8_t *buf, uint32_t lba, uint32_t numsects, void* dev)
{
	if(dev == null)
	{
		return;
	}
	uint32_t d = ((ata_private_data_t*)((device_t*)dev)->priv)->drive;
	for(uint32_t i = 0; i < numsects; i++)
	{
		ata_read_block(buf, lba + i, d);
		buf += 512;
	}
}

void ata_write(uint8_t *buf, uint32_t lba, uint32_t numsects, void* dev)
{
	if(dev == null)
	{
		return;
	}
	uint32_t d = ((ata_private_data_t*)((device_t*)dev)->priv)->drive;
	for(uint32_t i = 0; i < numsects; i++)
	{
		ata_write_block(buf, lba + i, d);
		buf += 512;
	}
}