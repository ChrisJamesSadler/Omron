#include <common.h>
#include <descriptors.h>
#include <tasking.h>
#include <memory.h>

uint32_t tid = 0;

void tasking_init()
{
	debug("Threading was successfully enabled!\n");
    thread_list = makelist(128);
    create_thread("Idle", 0);
	thread_current = (thread_t*)thread_list->pointer[0];
	thread_current->priority = THREAD_PRIORITY_REALTIME;
}

void __notified(void* ptr, uint32_t sig, uint32_t param)
{
	thread_t* this = (thread_t*)ptr;
	if(this != thread_current)
	{
		while(true)
		{
			if(this->notify_esp == 0)
			{
				this->notify_esptop = (uint32_t)malloc(1024);
				uint32_t* stack = (uint32_t *)(this->notify_esptop + 1024);
				*--stack = param;
				*--stack = sig;
				*--stack = (uint32_t)this;
				*--stack = 0x10; // ss ?
				*--stack = 0x00000202; // eflags
				*--stack = 0x8; // cs
				*--stack = (uint32_t)&__notified; // eip
				*--stack = 0; // error
				*--stack = 0; // int
				*--stack = 0; // eax
				*--stack = 0; // ebx
				*--stack = 0; // ecx
				*--stack = 0; // offset
				*--stack = 0; // edx
				*--stack = 0; // esi
				*--stack = 0; // edi
				*--stack = this->notify_esptop + 1024; //ebp
				*--stack = 0x10; // ds
				*--stack = 0x10; // fs
				*--stack = 0x10; // es
				*--stack = 0x10; // gs
				this->notify_esp = (uint32_t)stack;
				break;
			}
		}
	}
	else
	{
		switch(sig)
		{
			case SIG_ILL:
			case SIG_TERM:
			case SIG_SEGV:
				printf("[%s] Received SIGILL, terminating!\n", thread_current->name);
				kill();
				break;

				case SIG_PRI:
					thread_current->priority = param;
				break;

			default:
				printf("Received unknown SIG!\n");
				return;
		}
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
	p->notify_esp = 0;
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
	p->esp = (uint32_t)malloc(4096);
	asm("mov %%cr3, %%eax":"=a"(p->cr3));
	uint32_t* stack = (uint32_t *)(p->esp + 4096);
	p->stacktop = p->esp;
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
	debug("Created task %s with esp=0x%x eip=0x%x\n", p->name, p->esp, p->eip);
    listadd(thread_list, (uint32_t)p);
	return p->tid;
}

void tasking_switch()
{
	if(thread_current == 0)
	{
		thread_current = (thread_t*)thread_list->pointer[0];
	}
	if(thread_current)
	{
		if(thread_current->notify_esp == 0)
		{
			thread_current->esp = oldESP;
		}
		else
		{
			thread_current->notify_esp = oldESP;
		}
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
		thread_current->age = thread_current->priority;
		if(thread_current->notify_esp == 0)
		{
			oldESP = thread_current->esp;
		}
		else
		{
			oldESP = thread_current->notify_esp;
		}
	}
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
		while(thread_current->state != THREAD_STATE_ALIVE){}
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