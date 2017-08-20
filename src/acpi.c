#include <acpi.h>
#include <common.h>


uint8_t *g_ioApicAddr;
uint8_t* g_localApicAddr = 0;
uint32_t g_acpiCpuCount;
uint8_t g_acpiCpuIds[MAX_CPU_COUNT];
AcpiMadt* s_madt;

void AcpiParseApic(AcpiMadt *madt)
{
    s_madt = madt;

    g_localApicAddr = (uint8_t *)(uint32_t*)madt->localApicAddr;

    uint8_t *p = (uint8_t *)(madt + 1);
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    while (p < end)
    {
        ApicHeader *header = (ApicHeader *)p;
        uint8_t type = header->type;
        uint8_t length = header->length;

        if (type == APIC_TYPE_LOCAL_APIC)
        {
            ApicLocalApic *s = (ApicLocalApic *)p;

            if (g_acpiCpuCount < MAX_CPU_COUNT)
            {
                g_acpiCpuIds[g_acpiCpuCount] = s->apicId;
                ++g_acpiCpuCount;
            }
        }
        else if (type == APIC_TYPE_IO_APIC)
        {
            ApicIoApic *s = (ApicIoApic *)p;

            g_ioApicAddr = (uint8_t *)(uint32_t*)s->ioApicAddress;
        }
        else if (type == APIC_TYPE_INTERRUPT_OVERRIDE)
        {
            //ApicInterruptOverride *s = (ApicInterruptOverride *)p;

        }

        p += length;
    }
}

void AcpiParseDT(AcpiHeader *header)
{
    uint32_t signature = header->signature;

    if (signature == 0x43495041)
    {
        AcpiParseApic((AcpiMadt *)header);
    }
}

void AcpiParseXsdt(AcpiHeader *xsdt)
{
    uint64_t *p = (uint64_t *)(xsdt + 1);
    uint64_t *end = (uint64_t *)((uint8_t*)xsdt + xsdt->length);

    while (p < end)
    {
        uint64_t address = *p++;
        AcpiParseDT((AcpiHeader *)(uint32_t*)address);
    }
}

uint32_t *acpiCheckRSDPtr(uint32_t *ptr)
{
      char *sig = "RSD PTR ";
      struct RSDPtr *rsdp = (struct RSDPtr *) ptr;
      uint8_t *bptr;
      uint8_t check = 0;
      unsigned int i;

      if (memcmp(sig, rsdp, 8) == 0)
      {
            // check checksum rsdpd
            bptr = (uint8_t *) ptr;
            for (i=0; i<sizeof(struct RSDPtr); i++)
            {
                  check += *bptr;
                  bptr++;
            }

            // found valid rsdpd   
            if (check == 0)
            {

                  if (rsdp->Revision == 0)
                  {
                        
                  }
                  else
                  {
                        char* p = (char*)ptr;
                        uint64_t xsdtAddr = *(uint64_t *)(p + 24);
                        if (xsdtAddr)
                        {
                              AcpiParseXsdt((AcpiHeader *)(uint32_t*)xsdtAddr);
                        }
                  }

            return (unsigned int *) rsdp->RsdtAddress;
            }
      }

      return null;
}

uint32_t *acpiGetRSDPtr(void)
{
   unsigned int *addr;
   unsigned int *rsdp;

   // search below the 1mb mark for RSDP signature
   for (addr = (unsigned int *) 0x000E0000; (int) addr<0x00100000; addr += 0x10/sizeof(addr))
   {
      rsdp = acpiCheckRSDPtr(addr);
      if (rsdp != null)
         return rsdp;
   }


   // at address 0x40:0x0E is the RM segment of the ebda
   int ebda = *((short *) 0x40E);   // get pointer
   ebda = ebda*0x10 &0x000FFFFF;   // transform segment into linear address

   // search Extended BIOS Data Area for the Root System Description Pointer signature
   for (addr = (unsigned int *) ebda; (int) addr<ebda+1024; addr+= 0x10/sizeof(addr))
   {
      rsdp = acpiCheckRSDPtr(addr);
      if (rsdp != null)
         return rsdp;
   }

   return null;
}

int32_t acpiCheckHeader(uint32_t *ptr, char* sig)
{
   if (memcmp(ptr, sig, 4) == 0)
   {
      char *checkPtr = (char *) ptr;
      int len = *(ptr + 1);
      char check = 0;
      while (0<len--)
      {
         check += *checkPtr;
         checkPtr++;
      }
      if (check == 0)
         return 0;
   }
   return -1;
}

int acpiEnable()
{
   // check if acpi is enabled
   if ( (inw((unsigned int) PM1a_CNT) &SCI_EN) == 0 )
   {
      // check if acpi can be enabled
      if (SMI_CMD != 0 && ACPI_ENABLE != 0)
      {
         outb((unsigned int) SMI_CMD, ACPI_ENABLE); // send acpi enable command
         // give 3 seconds time to enable acpi
         int i;
         for (i=0; i<300; i++ )
         {
            if ( (inw((unsigned int) PM1a_CNT) &SCI_EN) == 1 )
               break;
         }
         if (PM1b_CNT != 0)
            for (; i<300; i++ )
            {
               if ( (inw((unsigned int) PM1b_CNT) &SCI_EN) == 1 )
                  break;
            }
         if (i<300) {
            return 0;
         } else {
            return -1;
         }
      } else {
         return -1;
      }
   } else {
      return 0;
   }
}

void acpi_init()
{
  unsigned int *ptr = acpiGetRSDPtr();

   // check if address is correct  ( if acpi is available on this pc )
   if (ptr != null && acpiCheckHeader(ptr, "RSDT") == 0)
   {
      // the RSDT contains an unknown number of pointers to acpi tables
      int entrys = *(ptr + 1);
      entrys = (entrys-36) /4;
      ptr += 36/4;   // skip header information

      while (0<entrys--)
      {
         // check if the desired table is reached
         if (acpiCheckHeader((unsigned int *) *ptr, "FACP") == 0)
         {
            struct FACP *facp = (struct FACP *) *ptr;
            
            if (acpiCheckHeader((unsigned int *) facp->DSDT, "DSDT") == 0)
            {
               // search the \_S5 package in the DSDT
               char *S5Addr = (char *) facp->DSDT +36; // skip header
               int dsdtLength = *(facp->DSDT+1) -36;
               while (0 < dsdtLength--)
               {
                  if (memcmp(S5Addr, "_S5_", 4) == 0)
                     break;
                  S5Addr++;
               }
               // check if \_S5 was found
               if (dsdtLength > 0)
               {
                  // check for valid AML structure
                  if ( ( *(S5Addr-1) == 0x08 || ( *(S5Addr-2) == 0x08 && *(S5Addr-1) == '\\') ) && *(S5Addr+4) == 0x12 )
                  {
                     S5Addr += 5;
                     S5Addr += ((*S5Addr &0xC0)>>6) +2;   // calculate PkgLength size

                     if (*S5Addr == 0x0A)
                        S5Addr++;   // skip uint8prefix
                     SLP_TYPa = *(S5Addr)<<10;
                     S5Addr++;

                     if (*S5Addr == 0x0A)
                        S5Addr++;   // skip uint8prefix
                     SLP_TYPb = *(S5Addr)<<10;

                     SMI_CMD = facp->SMI_CMD;

                     ACPI_ENABLE = facp->ACPI_ENABLE;
                     ACPI_DISABLE = facp->ACPI_DISABLE;

                     PM1a_CNT = facp->PM1a_CNT_BLK;
                     PM1b_CNT = facp->PM1b_CNT_BLK;
                     
                     PM1_CNT_LEN = facp->PM1_CNT_LEN;

                     SLP_EN = 1<<13;
                     SCI_EN = 1;
                     acpiEnable();
                  }
               }
            }
         }
         if (acpiCheckHeader((unsigned int *) *ptr, "HPET") == 0)
         {
             
         }
         ptr++;
      }
   }
}

uint32_t AcpiRemapIrq(uint32_t irq)
{
    AcpiMadt *madt = s_madt;

    uint8_t *p = (uint8_t *)(madt + 1);
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    while (p < end)
    {
        ApicHeader *header = (ApicHeader *)p;
        uint8_t type = header->type;
        uint8_t length = header->length;

        if (type == APIC_TYPE_INTERRUPT_OVERRIDE)
        {
            ApicInterruptOverride *s = (ApicInterruptOverride *)p;

            if (s->source == irq)
            {
                return s->interrupt;
            }
        }

        p += length;
    }

    return irq;
}

void acpi_shutdown()
{
  if (SCI_EN == 0)
      return;
    outw((unsigned int) PM1a_CNT, SLP_TYPa | SLP_EN );
   if ( PM1b_CNT != 0 )
      outw((unsigned int) PM1b_CNT, SLP_TYPb | SLP_EN );
}

void acpi_reboot()
{
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
}