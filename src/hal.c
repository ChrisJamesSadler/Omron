#include <common.h>
#include <hal.h>
#include <memory.h>

char* GetDetails(uint32_t VendorID, uint32_t DeviceID, uint32_t ClassCode)
{
    switch (VendorID)
    {
        case 0x1022: //AMD
            switch (DeviceID)
            {
                case 0x2000:
                    //type = PCIType.Network;
                    return "AMD PCnet LANCE PCI Ethernet Controller";
            }
            break;
        case 0x104B: //Sony
            switch (DeviceID)
            {
                case 0x1040:
                    //type = PCIType.Storage;
                    return "Mylex BT958 SCSI Host Adaptor";
            }
            break;
        case 0x1274: //Ensoniq
            switch (DeviceID)
            {
                case 0x1371:
                    //type = PCIType.Audio;
                    return "Ensoniq AudioPCI";
            }
            break;
        case 0x15AD: //VMware
            switch (DeviceID)
            {
                case 0x0405:
                    //type = PCIType.Graphics;
                    return "VMware NVIDIA 9500MGS";
                case 0x0770:
                    //type = PCIType.Storage;
                    return "VMware Standard Enhanced PCI to USB Host Controller";
                case 0x0790:
                    //type = PCIType.Storage;
                    return "VMware 6.0 Virtual USB 2.0 Host Controller";
                case 0x07A0:
                    //type = PCIType.Storage;
                    return "VMware PCI Express Root Port";
            }
            break;
        case 0x8086: //Intel
            switch (DeviceID)
            {
                case 0x7190:
                    //type = PCIType.Other;
                    return "Intel 440BX/ZX AGPset Host Bridge";
                case 0x7191:
                    //type = PCIType.Other;
                    return "Intel 440BX/ZX AGPset PCI-to-PCI bridge";
                case 0x7110:
                    //type = PCIType.Other;
                    return "Intel PIIX4/4E/4M ISA Bridge";
                case 0x7111:
                    return "Intel 82371AB/EB/MB PIIX4 IDE";
                case 0x7112:
                    //type = PCIType.Other;
                    return "Intel PIIX4/4E/4M USB Interface";
            }
            break;
    }

    switch (ClassCode)
    {
        //case 0x00:
        //    return "Any device";
        case 0x01:
            //type = PCIType.Storage;
            return "Mass Storage Controller";
        case 0x02:
            //type = PCIType.Network;
            return "Network Controller";
        case 0x03:
            //type = PCIType.Graphics;
            return "Display Controller";
        case 0x04:
            //type = PCIType.Other;
            return "Multimedia Controller";
        case 0x05:
            //type = PCIType.Other;
            return "Memory Controller";
        case 0x06:
            //type = PCIType.Other;
            return "Bridge Device";
        case 0x07:
            //type = PCIType.Other;
            return "Simple Communication Controller";
        case 0x08:
            //type = PCIType.Other;
            return "Base System Peripheral";
        case 0x09:
            //type = PCIType.Input;
            return "Input Device";
        case 0x0A:
            //type = PCIType.Other;
            return "Docking Station";
        case 0x0B:
            //type = PCIType.Processor;
            return "Processor";
        case 0x0C:
            //type = PCIType.Other;
            return "Serial Bus Controller";
        case 0x0D:
            //type = PCIType.Network;
            return "Wireless Controller";
        case 0x0E:
            //type = PCIType.Other;
            return "Intelligent I/O Controller";
        case 0x0F:
            //type = PCIType.Other;
            return "Satellite Communication Controller";
        case 0x10:
            //type = PCIType.Other;
            return "Encryption/Decryption Controller";
        case 0x11:
            //type = PCIType.Other;
            return "Data Acquisition and Signal Processing Controller";
        case 0xFF:
            //type = PCIType.Other;
            return "Unkown device";
    }
    //type = PCIType.Other;
    printf("UNKNOWN Vendor: %x Device: %x Class: %x", VendorID, DeviceID, ClassCode);
    return "Unkown device";
}

uint16_t pci_read_word(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset)
{
	uint64_t address;
    uint64_t lbus = (uint64_t)bus;
    uint64_t lslot = (uint64_t)slot;
    uint64_t lfunc = (uint64_t)func;
    uint16_t tmp = 0;
    address = (uint64_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
    outd (0xCF8, address);
    tmp = (uint16_t)((ind (0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}

uint16_t getVendorID(uint16_t bus, uint16_t device, uint16_t function)
{
        uint32_t r0 = pci_read_word(bus,device,function,0);
        return r0;
}

uint16_t getDeviceID(uint16_t bus, uint16_t device, uint16_t function)
{
        uint32_t r0 = pci_read_word(bus,device,function,2);
        return r0;
}

uint16_t getClassCode(uint16_t bus, uint16_t device, uint16_t function)
{
        uint32_t r0 = pci_read_word(bus,device,function,8);
        return r0;
}

uint16_t getSubCode(uint16_t bus, uint16_t device, uint16_t function)
{
        uint32_t r0 = pci_read_word(bus,device,function,12);
        return r0;
}

void pci_probe()
{
	for(uint32_t bus = 0; bus < 256; bus++)
    {
        for(uint32_t slot = 0; slot < 32; slot++)
        {
            for(uint32_t function = 0; function < 8; function++)
            {
                uint16_t vendor = getVendorID(bus, slot, function);
                if(vendor == 0xffff) continue;
                uint16_t device = getDeviceID(bus, slot, function);
                if(device == 0x7A0) continue;
                uint16_t class = getClassCode(bus, slot, function);
                //uint16_t sub = getSubCode(bus, slot, function);
                device_t* dev = malloc(sizeof(device_t));
                dev->name = GetDetails(vendor, device, class);
                pci_private_data_t* priv =  malloc(sizeof(pci_private_data_t));
                dev->priv = priv;
                priv->vendorid = vendor;
                priv->deviceid = device;
                priv->classcode = class;
                priv->bus = bus;
                priv->slot = slot;
                priv->func = function;
                deviceadd(dev);
            }
        }
    }
}

void hal_init()
{
    devices = makelist(64);
    pci_probe();
}

void deviceadd(device_t* dev)
{
    listadd(devices, (uint32_t)dev);
}