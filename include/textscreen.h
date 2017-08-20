#ifndef __TEXTSCREEN_H__
#define __TEXTSCREEN_H__

#include <common.h>

enum TUIColours
{
	TUIColourBlack			=	0x0,
	TUIColourBlue			=	0x1,
	TUIColourGreen			=	0x2,
	TUIColourCyan			=	0x3,
	TUIColourRed			=	0x4,
	TUIColourMagenta		=	0x5,
	TUIColourBrown			=	0x6,
	TUIColourLightGray		=	0x7,
	TUIColourDarkGray		=	0x8,
	TUIColourLightBlue		=	0x9,
	TUIColourLightGreen	=	0xA,
	TUIColourLightCyan		=	0xB,
	TUIColourLightRed		=	0xC,
	TUIColourLightMagenta	=	0xD,
	TUIColourYellow		=	0xE,
	TUIColourWhite			=	0xF
};

typedef void * va_list;
#define __va_size( type ) \
( ( sizeof( type ) + 3 ) & ~0x3 )

#define va_start( va_l, last ) \
( ( va_l ) = ( void * )&( last ) + __va_size( last ) )

#define va_end( va_l )

#define va_arg( va_l, type ) \
( ( va_l ) += __va_size( type ), \
*( ( type * )( ( va_l ) - __va_size( type ) ) ) )

extern void textscreen_clear();
extern void textscreen_write_str(char* aStr);
extern void textscreen_write_dec(int32_t value);
extern void textscreen_write_hex(uint32_t value);
extern void textscreen_write_char(char aChar);
extern void textscreen_write_panic(char *message, char *file, uint32_t line);
extern void textscreen_write(char* str, ...);
extern void textscreen_movecursor(uint32_t x, uint32_t y);
extern void textscreen_setcursor(uint32_t x, uint32_t y);
extern void textscreen_updatecursor();
extern uint32_t textscreen_getcursorx();
extern uint32_t textscreen_getcursory();

#endif