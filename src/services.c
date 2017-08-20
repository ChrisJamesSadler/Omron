#include <common.h>
#include <tasking.h>
#include <descriptors.h>
#include <scanmap.h>
#include <services.h>
#include <memory.h>
#include <rtc.h>

extern list_t* mutexlist;
void services_process()
{
    while(true)
    {
        sleep(100);
        for(int32_t i = 0; i < listlength(thread_list); i++)
        {
            thread_t* crnt = (thread_t*)thread_list->pointer[i];
            if(crnt->state == THREAD_STATE_DEAD || crnt->state == THREAD_STATE_ZOMBIE)
            {
                thread_t* dead = null;
                popitem(thread_list, i, &dead);
                if(dead != null)
                {
                    listadd(thread_list_dead, (uint32_t)dead);
                }
            }
        }
        sleep(100);
        for(int32_t i = 0; i < listlength(thread_list_dead); i++)
		{
			thread_t* crnt;
            popitem(thread_list_dead, i, (uint32_t*)&crnt);
            // notify user about the location of a debug file if it is a zombie
            mutex_t* mut;
            for(int32_t m = 0; m < listlength(mutexlist); m++)
            {
                peekitem(mutexlist, m, (uint32_t*)&mut);
                if(mut->locker == crnt->tid)
                {
                    mut->locker = 0;
                    mut->locked = 0;
                    break;
                }
            }
            free((void*)crnt->stacktop);
            free(crnt);
		}
    }
}

extern uint32_t heap_begin;
extern uint32_t last_alloc;
extern mutex_t* malloc_mutex;
void services_heap()
{
    while(true)
    {
        sleep(3000);
        mutexlock(malloc_mutex);
        uint8_t *mem = (uint8_t *)heap_begin;
        while((uint32_t)mem < last_alloc)
        {
            sleep(1);
            alloc_t *a = (alloc_t *)mem;
            if(!a->size)
            {
                goto nalloc;
            }
            if(a->status)
            {
                mem += a->size;
                mem += sizeof(alloc_t);
                mem += 4;
                goto nalloc;
            }
            memset(mem + sizeof(alloc_t), 0, a->size);
            mem += a->size;
            mem += sizeof(alloc_t);
            mem += 4;
        }
        nalloc:;
        mutexunlock(malloc_mutex);
    }
}

uint32_t services_keyboard_isr_flag;
keyboard_event_t services_keyboard_isr_events[10];
uint32_t services_keyboard_isr(uint32_t oldESP)
{
    services_keyboard_isr_flag = true;
    return oldESP;
}

void services_keyboard_register(void* param)
{
    for(int i = 0; i < 10; i++)
    {
        if((uint32_t)services_keyboard_isr_events[i] == 0)
        {
            services_keyboard_isr_events[i] = (keyboard_event_t)param;
            return;
        }
    }
}

void services_keyboard_unregister(void* param)
{
    for(int i = 0; i < 10; i++)
    {
        if((uint32_t)services_keyboard_isr_events[i] == (uint32_t)param)
        {
            services_keyboard_isr_events[i] = 0;
            return;
        }
    }
}

void services_keyboard()
{
    set_int(33, &services_keyboard_isr);
    inb(0x60);
    while(true)
    {
        if(services_keyboard_isr_flag == true)
        {
            services_keyboard_isr_flag = false;
            uint32_t scan = inb(0x60);
            if(scan == 29)
            {
                kbd_ctrl = true;
            }
            if(scan == 157)
            {
                kbd_ctrl = false;
            }
            if(scan == 42)
            {
                kbd_shift = true;
            }
            if(scan == 170)
            {
                kbd_shift = false;
            }
            char c = (char)scan;
            uint8_t released;
            c = scanmap_decode(c, &released);
            if(isalpha(c))
            {
                if(kbd_shift && islower(c))
                {
                    c -= 0x20;
                }
            }
            if(c != 0)
            {
                int count = 1;
                if(c == 0x09)
                {
                    count = (textscreen_getcursorx() + 8) & ~(8-1);
                    c = ' ';
                }
                for(int t = 0; t < count; t++)
                {
                    for(int i = 0; i < 10; i++)
                    {
                        if((uint32_t)services_keyboard_isr_events[i] != 0)
                        {
                            try(&&erroredout);
                            services_keyboard_isr_events[i](c, released);
                            completed();
                            erroredout:;
                        }
                    }
                }
            }
        }
    }
}

void services_rtc()
{
    while(true)
    {
        rtc_read_datetime();
        sleep(500);
    }
}

void services_init()
{
    sleep(100);
    create_thread("Process Manager", (uint32_t)&services_process);
    create_thread("PS2Keyboard", (uint32_t)&services_keyboard);
    create_thread("Heap", (uint32_t)&services_heap);
    create_thread("RTC", (uint32_t)&services_rtc);
}