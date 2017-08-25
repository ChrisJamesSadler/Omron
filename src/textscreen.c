#include <common.h>
#include <textscreen.h>
#include <tasking.h>

uint16_t* textscreen_address = (uint16_t*)0xB8000;
uint8_t textscreen_x = 0;
uint8_t textscreen_y = 0;
uint8_t textscreen_width = 80;
uint8_t textscreen_height = 25;
uint8_t textscreen_primary = TUIColourGreen;
uint8_t textscreen_secondary = TUIColourBlack;

void textscreen_clear()
{
    if(scanf_mutex)
    {
        mutexlock(scanf_mutex);
    }
    uint64_t col = ((TUIColourBlack << 4) | (TUIColourGreen & 0x0F)) << 8;
    col = (col << 16) | col;
    col = (col << 24) | col;
    uint64_t* ptr = (uint64_t*)textscreen_address;
    for(uint32_t i = 0; i < (textscreen_width * textscreen_height) / 2; i++)
    {
        ptr[i] = col;
    }
    textscreen_updatecursor();
    textscreen_x = 0;
    textscreen_y = 0;
    if(scanf_mutex)
    {
        mutexunlock(scanf_mutex);
    }
}

void textscreen_write_str(char* aStr)
{
    while(*aStr != 0)
    {
        textscreen_write_char(*aStr);
        aStr++;
    }
}

void textscreen_write_dec(int32_t value)
{
    if(value < 0)
    {
        value = -value;
        textscreen_write_char('-');
    }
    int32_t n = value / 10;
	int32_t r = value % 10;
	if (r < 0)
	{
		r += 10;
		n--;
	}
	if (value >= 10)
	{
		textscreen_write_dec(n);
	}
	textscreen_write_char("0123456789"[r]);
}

void textscreen_write_hex(uint32_t value)
{
    int32_t n = value / 16;
	int32_t r = value % 16;
	if (r < 0)
	{
		r += 16;
		n--;
	}
	if (value >= 16)
	{
		textscreen_write_hex(n);
	}
	textscreen_write_char("0123456789ABCDEF"[r]);
}


void textscreen_write_char(char aChar)
{
    uint16_t col = ((textscreen_secondary << 4) | (textscreen_primary & 0x0F)) << 8;
    if(aChar == 0x08 && textscreen_x > 0)
    {
        textscreen_x--;
        textscreen_write_char(' ');
        textscreen_x--;
    }
    else if(aChar == '\r')
    {
        textscreen_x = 0;
    }
    else if(aChar == '\n')
    {
        textscreen_x = 0;
        textscreen_y ++;
    }
    else if(aChar >= ' ')
    {
        uint16_t* location = textscreen_address + (textscreen_y * textscreen_width) + textscreen_x;
        *location = aChar | col;
        textscreen_x ++;
    }

    if(textscreen_x >= textscreen_width)
    {
        textscreen_x = 0;
        textscreen_y++;
    }
    
    if(textscreen_y >= textscreen_height)
    {
        memcpy(textscreen_address, textscreen_address + (textscreen_width), (textscreen_width * textscreen_height) * 2);
        memset(textscreen_address + (textscreen_width * textscreen_height), 0, textscreen_width * 2);
        textscreen_y = textscreen_height - 1;
    }
    textscreen_updatecursor();
}

void textscreen_updatecursor()
{
    uint16_t offset = (textscreen_width * textscreen_y) + textscreen_x;
    outb(0x3D4, 14);
    outb(0x3D5, offset >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, offset);
}

void textscreen_movecursor(uint32_t x, uint32_t y)
{
    textscreen_x += x;
    textscreen_y += y;
}

void textscreen_setcursor(uint32_t x, uint32_t y)
{
    textscreen_x = x;
    textscreen_y = y;
}

uint32_t textscreen_getcursorx()
{
    return textscreen_x;
}

uint32_t textscreen_getcursory()
{
    return textscreen_y;
}

void textscreen_write_panic(char *message, char *file, uint32_t line)
{
    //asm("cli");
    send_sig(p_id(), SIG_PRI, THREAD_PRIORITY_REALTIME);
    printf("CRITICAL!!!  --  %s  --  %s:%d\n", message, file, line);
    while (1) asm("hlt");
}