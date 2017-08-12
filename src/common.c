#include <common.h>
#include <textscreen.h>
#include <tasking.h>
#include <services.h>
#include <memory.h>
#include <rtc.h>

void memcpy(void *dest, const void *src, uint32_t len)
{
    const uint8_t *sp = (const uint8_t *)src;
    uint8_t *dp = (uint8_t *)dest;
    for(; len != 0; len--) *dp++ = *sp++;
}

void memset(void *dest, uint8_t val, uint32_t len)
{
    uint8_t *temp = (uint8_t *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

int32_t memcmp(const void* aptr, const void* bptr, uint32_t size)
{
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (uint32_t i = 0; i < size; i++)
    {
		if (a[i] != b[i])
			return 1;
	}
	return 0;
}

uint32_t strlen(const char* str)
{
    uint32_t i = 0;
    while(*str)
    {
        str++;
        i++;
    }
    return i;
}

int32_t strcmp(const void* s1, const void* s2)
{
    char* str1 = (char*)s1;
    char* str2 = (char*)s2;
    uint32_t slen = strlen(str1);
    if(slen != strlen(str2))
    {
        return 0;
    }
    for(uint32_t i = 0; i < slen; i++)
    {
        if(str1[i] != str2[i])
        {
            return 0;
        }
    }
    return 1;
}

int32_t strbegins(const void* s1, const void* s2)
{
    char* str1 = (char*)s1;
    char* str2 = (char*)s2;
    uint32_t slen = strlen(str2);
    for(uint32_t i = 0; i < slen; i++)
    {
        if(str1[i] != str2[i])
        {
            return 0;
        }
    }
    return 1;
}

int32_t strcpy(char* dest, char* src)
{
    int32_t i = 0;
    while ((*dest++ = *src++) != 0)
        i++;
    return i;
}

void strcat(char* dest, char* src)
{
    char * end = (char*)dest + strlen(dest);
    memcpy((char*)end,(char*)src,strlen((char*)src));
    end = end + strlen((char*)src);
    *end = '\0';
}

void strtrim(char* str, char c)
{
    int len = strlen(str);
    for(int i = len; i > 0; i--)
    {
        if(str[i] != c)
        {
            break;
        }
        str[i] = 0;
    }
}

void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

void outw(uint16_t port, uint16_t value)
{
    __asm__ __volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

void outd(uint16_t port, uint32_t value)
{
    __asm__ volatile("outl %0, %w1" : : "a" (value), "Nd" (port));
}

uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ __volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ __volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

uint32_t ind(uint16_t port)
{
	uint32_t ret;
	__asm__ __volatile("inl %%dx, %%eax":"=a"(ret):"d"(port));
	return ret;
}

void printf(char* str, ...)
{
    char* s = 0;
	va_list ap;
	va_start(ap, str);
	for(uint64_t i = 0; i < strlen(str); i++)
	{
		if(str[i] == '%')
		{
			switch(str[i+1])
			{
				case 's':
					s = va_arg(ap, char*);
					textscreen_write_str(s);
					i++;
					continue;
				case 'd':
                {
					uint32_t c = va_arg(ap, int);
					textscreen_write_dec(c);
					i++;
					continue;
				}
				case 'x':
                {
					uint32_t c = va_arg(ap, int);
					textscreen_write_hex(c);
					i++;
					continue;
				}
				case 'c':
                {
					char c = (char)(va_arg(ap, int) & ~0xFFFFFF00);
					textscreen_write_char(c);
					i++;
					continue;
				}
				default:
					break;
			}
		} else {
			textscreen_write_char(str[i]);
		}
	}
	va_end(ap);
}

char* inputBuffer = 0;
uint32_t inputPosition = 0;
uint32_t inputSize = 0;
void handleKeyboard(char c, uint8_t released)
{
    if(released == 0)
    {
        if(c == 8)
        {
            if(inputPosition > 0)
            {
                inputPosition--;
                textscreen_movecursor(-1, 0);
                printf(" ");
                textscreen_movecursor(-1, 0);
                textscreen_updatecursor();
            }
            inputBuffer[inputPosition] = 0;
        }
        else if(c == 10)
        {
            inputBuffer = 0;
        }
        else
        {
            if(isalpha(c) || isdigit(c) || c == ' ')
            {
                inputBuffer[inputPosition] = c;
                inputPosition++;
                textscreen_write_char(c);
            }
        }
    }
}

mutex_t* scanf_mutex;
uint32_t scanf(const char* format, ...)
{
    if(!scanf_mutex)
    {
        scanf_mutex = mutexcreate();
    }
    mutexlock(scanf_mutex);
    services_keyboard_register(&handleKeyboard);
    inputPosition = 0;
    va_list ap;
	va_start(ap, format);
    if(strcmp(format, "%s"))
    {
        char** var = va_arg(ap, char**);
        uint32_t length = va_arg(ap, uint32_t);
        if(*var == 0)
        {
            *var = (char*)malloc(length);;
        }
        memset(*var, 0, length);
        inputBuffer = *var;
        while(inputBuffer != 0)
        {
            asm("hlt");
        }
    }
    else if(strcmp(format, "%d"))
    {
        uint32_t* num = va_arg(ap, uint32_t*);
        *num = 255;
    }
	va_end(ap);
    services_keyboard_unregister(&handleKeyboard);
    mutexunlock(scanf_mutex);
    return 0;
}

list_t* mutexlist;
mutex_t* mutexcreate()
{
    if(mutexlist == 0)
    {
        mutexlist = makelist(64);
    }
    if(!mutexlist)
    {
        return null;
    }
    mutex_t* ptr = (mutex_t*)malloc(sizeof(mutex_t));
    ptr->locked = 0;
    ptr->locker = 0;
    listadd(mutexlist, (uint32_t)ptr);
    return ptr;
}

void mutexlock(mutex_t* m)
{
    if(!m)
    {
        return;
    }
	while(m->locked)
    {
        // switch to next thread
    }
	m->locked = 1;
    m->locker = thread_current->tid;
}

void mutexunlock(mutex_t* m)
{
    if(!m)
    {
        return;
    }
	m->locked = 0;
    m->locker = 0;
}

list_t* makelist(uint32_t capacity)
{
    list_t* obj = (list_t*)malloc(sizeof(list_t));
    obj->pointer = (uint32_t*)malloc(capacity * 4);
    obj->length = capacity;
    return obj;
}

void deletelist(list_t* thelist)
{
    free(thelist->pointer);
    free(thread_list);
}

int32_t listlength(list_t* thelist)
{
    if(!thelist)
    {
        return 0;
    }
    return thelist->current;
}

void listadd(list_t* thelist, uint32_t value)
{
    if(!thelist)
    {
        return;
    }
    if(thelist->length == thelist->current)
    {
        uint32_t* ptr = (uint32_t*)malloc(thelist->length * 2);
        memcpy(ptr, thelist->pointer, thelist->length * 4);
        thelist->length *= 2;
        thelist->pointer = ptr;
    }
    thelist->pointer[thelist->current] = value;
    thelist->current++;
}

uint32_t poplast(list_t* thelist, uint32_t* output)
{
    if(!thelist)
    {
        return 0;
    }
    if(thelist->current <= 0)
    {
        return 0;
    }
    thelist->current--;
    *output = thelist->pointer[thelist->current];
    thelist->pointer[thelist->current] = 0;
    return 1;
}

uint32_t popfirst(list_t* thelist, uint32_t* output)
{
    if(!thelist)
    {
        return 0;
    }
    if(thelist->current <= 0)
    {
        return 0;
    }
    *output = thelist->pointer[0];
    memcpy(thelist->pointer, thelist->pointer + 1, thelist->current * 4);
    thelist->current--;
    return 1;
}

uint32_t popitem(list_t* thelist, uint32_t index, uint32_t* output)
{
    if(!thelist)
    {
        return 0;
    }
    if(thelist->current <= 0)
    {
        return 0;
    }
    *output = thelist->pointer[index];
    for(int i = index; i < thelist->current - 1; i++)
    {
        thelist->pointer[i] = thelist->pointer[i + 1];
    }
    thelist->current--;
    thelist->pointer[thelist->current] = 0;
    return 1;
}

uint32_t peekitem(list_t* thelist, int32_t n, uint32_t* output)
{
    if(!thelist)
    {
        return 0;
    }
    if(n <= thelist->length)
    {
        *output = thelist->pointer[n];
        return 1;
    }
    return 0;
}

void try(void* param)
{
    if(thread_current)
    {
        listadd(thread_current->catcher, (uint32_t)param);
    }
}

void completed()
{
    if(thread_current)
    {
        uint32_t output;
        poplast(thread_current->catcher, &output);
    }
}

uint32_t random_seed;
uint32_t lastSeed = 234453234;
extern uint32_t flagRND();

uint32_t rnd()
{
    if(flagRND())
    {
        uint32_t ret;
        asm ("rdrand %0" : "=a" (ret));
        return ret;
    }
    else
    {
        lastSeed += random_seed + 3556;
        return lastSeed;
    }
}

uint32_t rand(uint32_t max)
{
    static uint32_t x = 123456789;
	static uint32_t y = 362436069;
	static uint32_t z = 521288629;
	static uint32_t w = 88675123;

	uint32_t t;

	t = x ^ (x << 11);
	x = y; y = z; z = w;
	t = w = ((w ^ (w >> 19)) ^ (t ^ (t >> 8))) - rnd();
	return t % (max+1); 
}

uint32_t isalpha(char aChar)
{
    if((aChar >= 'a' && aChar <= 'z') || (aChar >= 'A' && aChar <= 'Z'))
    {
        return 1;
    }
    return 0;
}

uint32_t isdigit(char aChar)
{
    if(aChar >= '0' && aChar <= '9')
    {
        return 1;
    }
    return 0;
}

uint32_t isupper(char aChar)
{
    if(aChar >= 'A' && aChar <= 'Z')
    {
        return 1;
    }
    return 0;
}

uint32_t islower(char aChar)
{
    if(aChar >= 'a' && aChar <= 'z')
    {
        return 1;
    }
    return 0;
}

uint32_t ishex(char aChar)
{
    if(islower(aChar))
    {
        aChar -= 0x20;
    }
    if(isdigit(aChar))
    {
        return 1;
    }
    if(aChar >= 0x41 && aChar <= 0x46)
    {
        return 1;
    }
    return 0;
}

char toupper(char aChar)
{
    if(isalpha(aChar))
    {
        if(islower(aChar))
        {
            aChar -= 0x20;
        }
    }
    return aChar;
}

char tolower(char aChar)
{
    if(isalpha(aChar))
    {
        if(isupper(aChar))
        {
            aChar += 0x20;
        }
    }
    return aChar;
}

uint32_t power(uint32_t base, uint32_t exponent)
{
    uint32_t result = 0;
    if(exponent != 0)
    {
        result = 1;
    }
    while (exponent != 0)
    {
        result *= base;
        --exponent;
    }
    return result;
}

int32_t atoi(const char * str, ...)
{
	va_list ap;
	va_start(ap, str);
    char* params = va_arg(ap, char*);
    int type = 0;
    if(params[0] == '%')
    {
        if(params[1] == 'd')
        {
            type = 0;
        }
        if(params[1] == 'x')
        {
            type = 1;
        }
    }
    if(type == 0)
    {
        uint8_t neg = 0;
        if(*str == '-')
        {
            neg = 1;
            str++;
        }
        uint32_t len = strlen(str);
        int32_t out = 0;
        uint32_t i;
        uint32_t pow = 1;
        for (i = len; i > 0; --i)
        {
            if(isdigit(str[i - 1]))
            {
                out += (str[i - 1] - 48) * pow;
                pow *= 10;
            }
            else
            {
                break;
            }
        }
        if(neg)
        {
            out = -out;
        }
        return out;
    }
    else if(type == 1)
    {
        int32_t result = 0;
        int depth = 0;
        for(uint32_t i = 0; i <= strlen(str); i++)
        {
            if(!ishex(str[i]))
            {
                depth = i;
                break;
            }
        }
        for(uint32_t i = 0; i < strlen(str); i++)
        {
            if(!ishex(str[i]))
            {
                break;
            }
            int num = 0;
            char c = toupper(str[i]);
            if(!isdigit(c))
            {
                num = c - 0x41 + 10;
            }
            else
            {
                num = c - 0x30;
            }
            depth--;
            if(depth == 0)
            {
                result += num;
            }
            else
            {
                result += num * power(16, depth);
            }
        }
        
        return result;
    }
    return 0;
}

void itoa(char *buf, unsigned long int n, int base)
{
    unsigned long int tmp;
    int i, j;

    tmp = n;
    i = 0;

    do {
        tmp = n % base;
        buf[i++] = (tmp < 10) ? (tmp + '0') : (tmp + 'a' - 10);
    } while (n /= base);
    buf[i--] = 0;

    for (j = 0; j < i; j++, i--) {
        tmp = buf[j];
        buf[j] = buf[i];
        buf[i] = tmp;
    }
}

void debug(char* content, ...)
{

}

datetime_t* time()
{
    datetime_t* t = malloc(sizeof(datetime_t));
    memcpy(t, &current_datetime, sizeof(datetime_t));
    return t;
}