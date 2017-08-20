#include <common.h>
#include <descriptors.h>
#include <tasking.h>
#include <memory.h>
#include <acpi.h>

uint32_t tid = 0;
uint32_t lage = 0;

void tasking_init()
{
	debug("Threading was successfully enabled!\n");
    thread_list = makelist(128);
    create_thread("Idle", 0);
	thread_current = (thread_t*)thread_list->pointer[0];
	thread_current->priority = THREAD_PRIORITY_REALTIME;
	thread_list_dead = makelist(16);
}

void __notified(void* ptr, uint32_t sig, uint32_t param)
{
	thread_t* this = (thread_t*)ptr;
	switch(sig)
	{
		case SIG_ILL:
		case SIG_TERM:
		case SIG_SEGV:
			printf("[%s] Received SIGILL, terminating!\n", this->name);
			this->state = THREAD_STATE_DEAD;
			break;

		case SIG_PRI:
			this->priority = param;
			break;

		default:
			printf("Received unknown SIG!\n");
			break;
	}
}

uint32_t create_thread(char* name, uint32_t addr)
{
    return create_thread_param(name, addr, 0);
}

uint32_t create_thread_param(char* name, uint32_t addr, uint32_t param)
{
    thread_t* p = (thread_t *)malloc(sizeof(thread_t));
	memset(p, 0, sizeof(thread_t));
    p->name = name;
	p->notify = __notified;
	tryagain:;
	p->tid = 10000 + rand(89999);// ++tid;
	for(int32_t i = 0; i < listlength(thread_list); i++)
	{
		thread_t* crnt = (thread_t*)thread_list->pointer[i];
		if(crnt->tid == p->tid)
		{
			goto tryagain;
		}
	}
	p->catcher = makelist(20);
	p->priority = THREAD_PRIORITY_MEDIUM;
	p->eip = addr;
	p->state = THREAD_STATE_ALIVE;
	if(thread_current)
	{
		p->esp = (uint32_t)malloc(4096);
		asm("mov %%cr3, %%eax":"=a"(p->cr3));
		uint32_t* stack = (uint32_t *)(p->esp + 4096);
		p->stacktop = p->esp;
		*--stack = 0; // trash
		*--stack = 0; // trash
		*--stack = 0; // trash
		*--stack = 0; // trash
		*--stack = 0; // trash
		*--stack = 0; // trash
		*--stack = param;
		*--stack = 0x10; // ss ?
		*--stack = 0x00000202; // eflags
		*--stack = 0x8; // cs
		*--stack = addr; // eip
		*--stack = 0; // error
		*--stack = 0; // int
		*--stack = 0; // eax
		*--stack = 0; // ebx
		*--stack = 0; // ecx
		*--stack = 0; // offset
		*--stack = 0; // edx
		*--stack = 0; // esi
		*--stack = 0; // edi
		*--stack = p->esp + 4096; //ebp
		*--stack = 0x10; // ds
		*--stack = 0x10; // fs
		*--stack = 0x10; // es
		*--stack = 0x10; // gs
		p->esp = (uint32_t)stack;
	}
	debug("Created task %s with esp=0x%x eip=0x%x\n", p->name, p->esp, p->eip);
    listadd(thread_list, (uint32_t)p);
	return p->tid;
}

void tasking_emergency_countdown()
{
	sleep(3000);
	int sec = 10;
	for(int i = 0 ; i < sec; i++)
	{
		printf("Shutting down in %d  \r", sec - i);
		sleep(1000);
	}
	acpi_shutdown();
}

void tasking_emergency()
{
	create_thread("PANIC Count", (uint32_t)&tasking_emergency_countdown);
	PANIC("All Kernel Threads Have Stopped");
	while(true) asm("hlt");
}

uint32_t tasking_switch(uint32_t oldESP)
{
	if(thread_current == 0)
	{
		thread_current = (thread_t*)thread_list->pointer[0];
	}
	if(thread_current)
	{
		if(listlength(thread_list) == 0)
		{
			create_thread("Panic", (uint32_t)&tasking_emergency);
			thread_current = (thread_t*)thread_list->pointer[0];
			thread_current->priority = THREAD_PRIORITY_REALTIME;
			oldESP = thread_current->esp;
		}
		else
		{
			thread_current->esp = oldESP;
			for(int32_t i = 0; i < listlength(thread_list); i++)
			{
				thread_t* crnt = (thread_t*)thread_list->pointer[i];
				if(crnt->state == THREAD_STATE_WAITING_SLEEP)
				{
					crnt->arg--;
					if(crnt->arg <= 0)
					{
						crnt->state = THREAD_STATE_ALIVE;
					}
				}
				crnt->age++;
				if(crnt->age > thread_current->age && crnt->state == THREAD_STATE_ALIVE)
				{
					thread_current = crnt;
				}
			}
			lage = thread_current->age;
			thread_current->age = thread_current->priority;
			oldESP = thread_current->esp;
		}
	}
	return oldESP;
}

void send_sig(uint32_t tid, uint32_t sig, uint32_t param)
{
	for(int32_t i = 0; i < listlength(thread_list); i++)
	{
		thread_t* output;
		if(peekitem(thread_list, i, (uint32_t*)&output))
		{
			if(output->tid == tid)
			{
				output->notify(output, sig, param);
			}
		}
	}
}

void sleep(uint32_t ms)
{
	if(thread_current)
	{
		thread_current->arg = ms;
		thread_current->state = THREAD_STATE_WAITING_SLEEP;
		asm ("int $0x20");
		while(thread_current->state != THREAD_STATE_ALIVE) { }
	}
}

void kill()
{
	if(thread_current)
	{
		thread_current->state = THREAD_STATE_DEAD;
		while(true)
		{
			asm("hlt");
		}
	}
}

uint32_t is_thread_running(char* name)
{
	for(int32_t i = 0; i < listlength(thread_list); i++)
	{
		thread_t* output;
		if(peekitem(thread_list, i, (uint32_t*)&output))
		{
			if(strcmp(output->name, name))
			{
				return true;
			}
		}
	}
	return false;
}

uint32_t get_thread_id(char* name)
{
	for(int32_t i = 0; i < listlength(thread_list); i++)
	{
		thread_t* output;
		if(peekitem(thread_list, i, (uint32_t*)&output))
		{
			if(strcmp(output->name, name))
			{
				return output->tid;
			}
		}
	}
	return false;
}

uint32_t p_id()
{
	if(!thread_current)
	{
		return 0;
	}
	return thread_current->tid;
}

char* p_name()
{
	if(!thread_current)
	{
		return 0;
	}
	return thread_current->name;
}