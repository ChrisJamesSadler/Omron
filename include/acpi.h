#ifndef __ACPI_H__
#define __ACPI_H__

#include <common.h>

#define MAX_CPU_COUNT 16
#define APIC_TYPE_LOCAL_APIC            0
#define APIC_TYPE_IO_APIC               1
#define APIC_TYPE_INTERRUPT_OVERRIDE    2

uint32_t *SMI_CMD;
uint8_t ACPI_ENABLE;
uint8_t ACPI_DISABLE;
uint32_t *PM1a_CNT;
uint32_t *PM1b_CNT;
uint16_t SLP_TYPa;
uint16_t SLP_TYPb;
uint16_t SLP_EN;
uint16_t SCI_EN;
uint8_t PM1_CNT_LEN;

struct RSDPtr
{
   uint8_t Signature[8];
   uint8_t CheckSum;
   uint8_t OemID[6];
   uint8_t Revision;
   uint32_t *RsdtAddress;
};

struct FACP
{
   uint8_t Signature[4];
   uint32_t Length;
   uint8_t unneded1[40 - 8];
   uint32_t *DSDT;
   uint8_t unneded2[48 - 44];
   uint32_t *SMI_CMD;
   uint8_t ACPI_ENABLE;
   uint8_t ACPI_DISABLE;
   uint8_t unneded3[64 - 54];
   uint32_t *PM1a_CNT_BLK;
   uint32_t *PM1b_CNT_BLK;
   uint8_t unneded4[89 - 72];
   uint8_t PM1_CNT_LEN;
};

typedef struct AcpiHeader
{
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem[6];
    uint8_t oemTableId[8];
    uint32_t oemRevision;
    uint32_t creatorId;
    uint32_t creatorRevision;
} __attribute__((packed)) AcpiHeader;

typedef struct AcpiMadt
{
    AcpiHeader header;
    uint32_t localApicAddr;
    uint32_t flags;
} __attribute__((packed)) AcpiMadt;

typedef struct ApicHeader
{
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) ApicHeader;

typedef struct ApicLocalApic
{
    ApicHeader header;
    uint8_t acpiProcessorId;
    uint8_t apicId;
    uint32_t flags;
} __attribute__((packed)) ApicLocalApic;

typedef struct ApicIoApic
{
    ApicHeader header;
    uint8_t ioApicId;
    uint8_t reserved;
    uint32_t ioApicAddress;
    uint32_t globalSystemInterruptBase;
} __attribute__((packed)) ApicIoApic;

typedef struct ApicInterruptOverride
{
    ApicHeader header;
    uint8_t bus;
    uint8_t source;
    uint32_t interrupt;
    uint16_t flags;
} __attribute__((packed)) ApicInterruptOverride;

extern uint8_t *g_ioApicAddr;
extern uint32_t g_acpiCpuCount;
extern uint8_t g_acpiCpuIds[];

extern void acpi_init();
extern void acpi_shutdown();
extern void acpi_reboot();

#endif