#ifndef COMMON_H
#define COMMON_H

typedef unsigned long  uint64_t;
typedef          long  int64_t;
typedef unsigned int   uint32_t;
typedef          int   int32_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned char  uint8_t;
typedef          char  int8_t;

#include <textscreen.h>

#define true 1
#define false 0
#define null (void*)0
#define PANIC(msg) textscreen_write_panic(msg, __FILE__, __LINE__);
#define asm(content) __asm__ __volatile(content);
#define ASCII_0_VALU 48
#define ASCII_9_VALU 57
#define ASCII_A_VALU 65
#define ASCII_F_VALU 70

typedef struct mutex_t
{
	uint8_t locked;
    uint32_t locker;
} mutex_t;

typedef struct list_t
{
    uint32_t* pointer;
    int32_t current;
    int32_t length;
} list_t;

typedef struct datetime_t
{
    uint8_t century;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
}datetime_t;

extern void memcpy(void *dest, const void *src, uint32_t len);
extern void memset(void *dest, uint8_t val, uint32_t len);
extern int32_t memcmp(const void* aptr, const void* bptr, uint32_t size);
extern uint32_t strlen(const char* str);
extern int32_t strcmp(const void* str1, const void* str2);
extern int32_t strbegins(const void* str1, const void* str2);
extern int32_t strcpy(char* dest, char* src);
extern void strcat(char* dest, char* src);
extern void strtrim(char* str, char c);
extern void outb(uint16_t port, uint8_t value);
extern void outw(uint16_t port, uint16_t value);
extern void outd(uint16_t port, uint32_t value);
extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t ind(uint16_t port);
extern void printf(char* str, ...);
extern uint32_t scanf(const char* format, ...);
extern mutex_t* mutexcreate();
extern void mutexlock(mutex_t* m);
extern void mutexunlock(mutex_t* m);
extern list_t* makelist(uint32_t capacity);
extern void deletelist(list_t* thelist);
extern int32_t listlength(list_t* thelist);
extern void listadd(list_t* thelist, uint32_t value);
extern uint32_t poplast(list_t* thelist, uint32_t* output);
extern uint32_t popfirst(list_t* thelist, uint32_t* output);
extern uint32_t popitem(list_t* thelist, uint32_t index, uint32_t* output);
uint32_t peekitem(list_t* thelist, int32_t n, uint32_t* output);
extern void try(void* param);
extern void completed();
extern uint32_t rnd();
extern uint32_t rand(uint32_t max);
extern uint32_t isalpha(char aChar);
extern uint32_t isdigit(char aChar);
extern uint32_t isupper(char aChar);
extern uint32_t islower(char aChar);
extern uint32_t ishex(char aChar);
extern char toupper(char aChar);
extern char tolower(char aChar);
extern uint32_t power(uint32_t base, uint32_t exponent);
extern int32_t atoi(const char * str, ...);
void itoa(char *buf, unsigned long int n, int base);
extern void debug(char* content, ...);
extern void* malloc(uint32_t sz);
extern void free(void* mem);
extern datetime_t* time();
extern list_t* mutexlist;

#endif