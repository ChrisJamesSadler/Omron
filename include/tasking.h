#ifndef TASKING_H
#define TASKING_H

#define THREAD_PRIORITY_LOW 10
#define THREAD_PRIORITY_MEDIUM 15
#define THREAD_PRIORITY_HIGHT 20
#define THREAD_PRIORITY_REALTIME 30
#define THREAD_STATE_ALIVE 0
#define THREAD_STATE_ZOMBIE 1
#define THREAD_STATE_DEAD 2
#define THREAD_STATE_WAITING_SLEEP 3

#define SIG_ILL 1
#define SIG_TERM 2
#define SIG_SEGV 3
#define SIG_PRI 20

typedef struct thread_t
{
	uint32_t tid;
    char* name;
	uint32_t esp;
	uint32_t stacktop;
	uint32_t eip;
	uint32_t cr3;
	uint32_t state;
	uint32_t arg;
	void (*notify)(void*, uint32_t, uint32_t);
	uint32_t notify_esp;
	uint32_t notify_esptop;
	uint32_t age;
	uint32_t priority;
	list_t* catcher;
} thread_t;

list_t* thread_list;
thread_t* thread_current;

extern void tasking_init();
extern uint32_t create_thread(char* name, uint32_t addr);
extern uint32_t create_thread_param(char* name, uint32_t addr, uint32_t param);
extern void set_priority(uint32_t tid, uint32_t priority);
extern void tasking_switch();
extern void send_sig(uint32_t tid, uint32_t sig, uint32_t param);
extern void sleep(uint32_t ms);
extern void kill();
extern uint32_t is_thread_running(char* name);
extern uint32_t get_thread_id(char* name);

#endif