#include <common.h>
#include <tasking.h>
#include <acpi.h>
#include <memory.h>
#include <hal.h>
#include <rtc.h>
#include <shell.h>








//define the ports , taken from http://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
#define   VGA_AC_INDEX      0x3C0
#define   VGA_AC_WRITE      0x3C0
#define   VGA_AC_READ         0x3C1
#define   VGA_MISC_WRITE      0x3C2
#define VGA_SEQ_INDEX      0x3C4
#define VGA_SEQ_DATA      0x3C5
#define   VGA_DAC_READ_INDEX   0x3C7
#define   VGA_DAC_WRITE_INDEX   0x3C8
#define   VGA_DAC_DATA      0x3C9
#define   VGA_MISC_READ      0x3CC
#define VGA_GC_INDEX       0x3CE
#define VGA_GC_DATA       0x3CF
#define VGA_CRTC_INDEX      0x3D4      /* 0x3B4 */
#define VGA_CRTC_DATA      0x3D5      /* 0x3B5 */
#define   VGA_INSTAT_READ      0x3DA
#define   VGA_NUM_SEQ_REGS   5
#define   VGA_NUM_CRTC_REGS   25
#define   VGA_NUM_GC_REGS      9
#define   VGA_NUM_AC_REGS      21
#define   VGA_NUM_REGS      (1+VGA_NUM_SEQ_REGS+VGA_NUM_CRTC_REGS+VGA_NUM_GC_REGS+VGA_NUM_AC_REGS)

//the vga identifiers
uint32_t VGA_width;
uint32_t VGA_height;
uint64_t VGA_bpp;
uint8_t *VGA_address;

/**
* CREATE THE REGISTER ARRAY TAKEN FROM http://wiki.osdev.org/VGA_Hardware
*/
unsigned char mode_320_200_256[]={
   /* MISC
    *
    * 0x63 => 01100011
    * 7 6 5 4 3 2 1 0
    * 1 1 0 0 0 1 1 0
    * VSP HSP - - CS CS ERAM IOS
    * 7,6 - 480 lines
    * 5,4 - free
    * 3,2 - 28,322 MHZ Clock
    * 1 - Enable Ram
    * 0 - Map 0x3d4 to 0x3b4
    */
   0x63,
   /* SEQ */
   /**
    * index 0x00 - Reset
    * 0x03 = 11
    * Bits 1,0 Synchronous reset
    */
   0x03,
   /**
    * index 0x01
    * Clocking mode register
    * 8/9 Dot Clocks
    */
   0x01,
   /**
    * Map Mask Register, 0x02
    * 0x0F = 1111
    * Enable all 4 Maps Bits 0-3
    * chain 4 mode
    */
   0x0F,
   /**
    * map select register, 0x03
    * no character map enabled
    */
   0x00,
   /**
    * memory mode register 0x04
    * enables ch4,odd/even,extended memory
    */
   0x0E,
   /* CRTC */
   0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
   0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x9C, 0x0E, 0x8F, 0x28,   0x40, 0x96, 0xB9, 0xA3,
   0xFF,
   /* GC */
   0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
   0xFF,
   /* AC */
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x41, 0x00, 0x0F, 0x00,   0x00
};

void write_registers(unsigned char *regs){
   unsigned i;

   /* write MISCELLANEOUS reg */
   outb(VGA_MISC_WRITE, *regs);
   regs++;
   /* write SEQUENCER regs */
   for(i = 0; i < VGA_NUM_SEQ_REGS; i++)
   {
      outb(VGA_SEQ_INDEX, i);
      outb(VGA_SEQ_DATA, *regs);
      regs++;
   }
   /* unlock CRTC registers */
   outb(VGA_CRTC_INDEX, 0x03);
   outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
   outb(VGA_CRTC_INDEX, 0x11);
   outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
   /* make sure they remain unlocked */
   regs[0x03] |= 0x80;
   regs[0x11] &= ~0x80;
   /* write CRTC regs */
   for(i = 0; i < VGA_NUM_CRTC_REGS; i++)
   {
      outb(VGA_CRTC_INDEX, i);
      outb(VGA_CRTC_DATA, *regs);
      regs++;
   }
   /* write GRAPHICS CONTROLLER regs */
   for(i = 0; i < VGA_NUM_GC_REGS; i++)
   {
      outb(VGA_GC_INDEX, i);
      outb(VGA_GC_DATA, *regs);
      regs++;
   }
   /* write ATTRIBUTE CONTROLLER regs */
   for(i = 0; i < VGA_NUM_AC_REGS; i++)
   {
      (void)inb(VGA_INSTAT_READ);
      outb(VGA_AC_INDEX, i);
      outb(VGA_AC_WRITE, *regs);
      regs++;
   }
   
   /* lock 16-color palette and unblank display */
   (void)inb(VGA_INSTAT_READ);
   outb(VGA_AC_INDEX, 0x20);
}

/**
* Clears the VGA screen
*/
void VGA_clear_screen(){
   unsigned int x=0;
   unsigned int y=0;

   for(y=0; y<VGA_height; y++){
      for(x=0; x<VGA_width; x++){
         VGA_address[VGA_width*y+x]=0x00;
      }
   }
}

void VGA_set_pallet(uint32_t index, uint32_t r, uint32_t g, uint32_t b)
{
    outb(0x3c8,index);
    outb(0x3c9,r);
    outb(0x3c9,g);
    outb(0x3c9,b);
}

/**
* Note here the vga struct must have the width 320 and height of 200
* color mode is 256
*/
void VGA_init(int width, int height, int bpp){
   //setup the vga struct
   VGA_width=(unsigned int)width;
   VGA_height=(unsigned int)height;
   VGA_bpp=bpp;
   VGA_address=(uint8_t*)0xA0000;

   //enables the mode 13 state
   write_registers(mode_320_200_256);

   //clears the screen
   VGA_clear_screen();
}







    
void shell_main()
{
    //VGA_init(320, 200, 8);
    sleep(1000);
    while(1)
    {
        if(textscreen_getcursorx() != 0)
        {
            printf("\n");
        }
        printf("CMD>>");
        char* input;
        scanf("%s", &input, 20);
        process_cmd(input);
        free(input);
    }
}

void process_cmd(char* input)
{
    if(textscreen_getcursorx() != 0)
    {
        printf("\n");
    }
    if(strcmp(input, "halt"))
    {
        acpi_shutdown();
    }
    else if(strcmp(input, "reboot"))
    {
        acpi_reboot();
    }
    else if(strcmp(input, "clear"))
    {
        textscreen_clear();
        textscreen_setcursor(0, 0);
    }
    else if(strbegins(input, "echo "))
    {
        textscreen_clear();
    }
    else if(strcmp(input, "ps"))
    {
        printf("TID       Name\n");
        for(int i = 0; i < listlength(thread_list); i++)
        {
            printf("%d     %s\n", ((thread_t*)thread_list->pointer[i])->tid, ((thread_t*)thread_list->pointer[i])->name);
        }
    }
    else if(strcmp(input, "lshw"))
    {
        for(int32_t i = 0; i < listlength(devices); i++)
        {
            device_t* dev;
            peekitem(devices, i, (uint32_t*)&dev);
            printf("%s\n", dev->name);
        }
    }
    else if(strcmp(input, "date"))
    {
        char* c = get_current_datetime_str();
        printf("%s\n", c);
        free(c);
    }
    else if(strbegins(input, "loop "))
    {
        input += 5;
        char* num = malloc(30);
        int32_t depth = 0;
        while(input[depth] != ' ' && input[depth] != 0)
        {
            num[depth] = input[depth];
            depth ++;
        }
        input += depth + 1;
        depth = atoi(num);
        free(num);
        for(int32_t i = 0; i < depth; i++)
        {
            process_cmd(input);
        }
    }
    else if(strbegins(input, "kill "))
    {
        uint32_t pid = atoi(input + 5);
        if(pid == 0)
        {
            for(int i = 0; i < listlength(thread_list); i++)
            {
                if(strcmp(((thread_t*)thread_list->pointer[i])->name, input + 5))
                {
                    pid = ((thread_t*)thread_list->pointer[i])->tid;
                }
            }
        }
        send_sig(pid, SIG_TERM, 0);
    }
    else if(strbegins(input, "memprint "))
    {
        uint32_t start = atoi(input + 9, "%x");
        uint32_t i = 9;
        for(; i < strlen(input); i++)
        {
            if(input[i] == ' ')
            {
                i++;
                break;
            }
        }
        if(i != strlen(input))
        {
            uint32_t count = atoi(input + i);
            uint8_t* ptr = (uint8_t*)start;
            for(uint32_t i = 0; i < count; i++)
            {
                printf("0x%x ", ptr[i]);
            }
            printf("\n");
        }
    }
    else if(strbegins(input, "memset "))
    {
        uint32_t start = atoi(input + 7, "%x");
        uint32_t i = 7;
        for(; i < strlen(input); i++)
        {
            if(input[i] == ' ')
            {
                i++;
                break;
            }
        }
        if(i != strlen(input))
        {
            uint32_t count = atoi(input + i);
            for(; i < strlen(input); i++)
            {
                if(input[i] == ' ')
                {
                    i++;
                    break;
                }
            }
            uint8_t value = (uint8_t)atoi(input + i);
            uint8_t* ptr = (uint8_t*)start;
            for(uint32_t i = 0; i < count; i++)
            {
                *ptr = value;
                ptr++;
            }
            printf("\n");
        }
    }
    else if(strcmp(input, "test"))
    {
        printf("Test What\n");
        free(input);
        scanf("%s", &input, 20);
        if(textscreen_getcursorx() != 0)
        {
            printf("\n");
        }
        if(strcmp(input, "mem"))
        {
            printf("Running memory test for the next 10 seconds\n");
            for(int i = 0; i < 10; i++)
            {
                void* alloc = malloc(((i % 2) * 100) + 100);
                printf("Test %d allocated %d at %d\n", i, ((i % 2) * 100) + 100, alloc);
                free(alloc);
                sleep(1000);
            }
        }
        else if(strcmp(input, "virus"))
        {
            for(int i = 0 ; i < 100; i++)
            {
                send_sig(i, SIG_TERM, 0);
            }
        }
        else
        {
            printf("No test found\n");
        }
    }
    else
    {
        if(strlen(input) > 0)
        {
            printf("Bad Command or File Name (%s)\n", input);
        }
    }
    if(textscreen_getcursorx() != 0)
    {
        printf("\n");
    }
}