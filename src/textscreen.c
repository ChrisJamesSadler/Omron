#include <common.h>
#include <textscreen.h>

uint16_t* textscreen_address = (uint16_t*)0xB8000;
uint8_t textscreen_x = 0;
uint8_t textscreen_y = 0;
uint8_t textscreen_width = 80;
uint8_t textscreen_height = 25;
uint8_t textscreen_primary = TUIColourGreen;
uint8_t textscreen_secondary = TUIColourBlack;

void textscreen_clear()
{
    uint8_t p = textscreen_primary;
    uint8_t s = textscreen_secondary;
    textscreen_primary = TUIColourLightGray;
    textscreen_secondary = TUIColourBlack;
    for(int y = 0; y < textscreen_height; y++)
    {
        for(int x = 0; x < textscreen_width; x++)
        {
            textscreen_write_char(' ');
        }
    }
    textscreen_x = 0;
    textscreen_y = 0;
    textscreen_primary = p;
    textscreen_secondary = s;
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

    uint8_t attributeByte = (0 /*black*/ << textscreen_primary) | (textscreen_secondary /*white*/ & 0x0F);
    uint16_t blank = 0x20 /* space */ | (attributeByte << 8);

    if(textscreen_y >= textscreen_height)
    {
        int32_t i;
        for(i = 0; i < textscreen_width * textscreen_height; i++)
        {
            textscreen_address[i] = textscreen_address[i + 80];
        }

        for(i = textscreen_width * (textscreen_height - 1); i < textscreen_width * textscreen_height; i++)
        {
            textscreen_address[i] = blank;
        }

        textscreen_y = textscreen_height - 1;
    }
    textscreen_updatecursor();
}

void textscreen_updatecursor()
{
    return;
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
    asm("cli");
    textscreen_write_str("CRITICAL!!!  --  ");
    textscreen_write_str(message);
    textscreen_write_str("  --  ");
    textscreen_write_str(file);
    textscreen_write_str(":");
    textscreen_write_dec(line);
    textscreen_write_char('\n');
    while (1) asm("hlt");
}