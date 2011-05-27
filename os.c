#include "os.h"
#include "x86.h"

#define A 1
#define B 2
#define C 3
#define D 4

#define READY 0x01
#define RUNNING 0x02
#define WAITING 0x04
#define TERMINATE 0x08

#define CLK_RATE 1.193182

#define NULL 0

#define IOFF() \
asm("cli");

#define ION() \
asm("sti");

/*************************************************************************
*                                                                        *
*                                                                        *
*   1. The part about the STRUCTS                                        *
*                                                                        *
*                                                                        *
*                                                                        *
*************************************************************************/

struct process
{
    int sp;
    PID pid;
    int name;
    unsigned int level;
    int state;
    int arg;
    unsigned int wait_time; //device processes only
};

struct list
{
    struct node* loc;
    struct node* head;
    struct node* tail;
};

struct node
{
    struct node* prev_node;
    struct node* next_node;
    void* ptr;
};

struct fifo
{
    FIFO descriptor;
    int buf[FIFOSIZE];
    int write_idx;
    int read_idx;
};

struct semaphore
{
    int id;
    int avail;
    int sleepers;
    PID curr_pids[MAXPROCESS];
    struct list wait_queue;
    struct node wait_queue_mem[MAXPROCESS];
};

/*************************************************************************
*                                                                        *
*                                                                        *
*   2. The part about the PROTOTYPES                                     *
*                                                                        *
*                                                                        *
*                                                                        *
*************************************************************************/
//interrupt functions
void interrupt_handler();
void exception_handler();
void irq_handler();

//process functions
struct process* get_proc(PID);

//list functions
void add_node(struct list*, void*, int);
void* get_head(struct list*);
void remove_head(struct list*);

//scheduling functions
void set_timer(unsigned int);
int calc_interval();
void context_switch(struct process*);
void scheduler();
void schedule();

//FIFO functions
void shift_fifo(struct fifo*);
BOOL is_empty(struct fifo*);
struct fifo* get_fifo(unsigned int);

//Semaphore functions
struct semaphore* get_semaphore(int);
BOOL holds_semaphore(struct semaphore*);

//processes
void init();
void idle();
void shell_proc();

/*************************************************************************
*                                                                        *
*                                                                        *
*   3. The part about the VARIABLES                                      *
*                                                                        *
*                                                                        *
*                                                                        *
*************************************************************************/
struct process process_ptr[MAXPROCESS];
struct node sporadic_ptr[MAXPROCESS];
char workspace[MAXPROCESS * WORKSPACE];
struct fifo fifo_ptr[MAXFIFO];
struct semaphore semaphore_ptr[MAXSEM];
struct process* running_proc;
struct process* idle_proc;
struct list sporadic_list;
int PPPidx;
int PPP[] = {A, B, IDLE, C, IDLE, D, IDLE};
int PPPMax[] = {5, 3, 10, 7, 2, 7, 5};
int PPPLen;
int fake_irq;
int time_left;
struct process* device_interrupted_proc;

/*************************************************************************
*                                                                        *
*                                                                        *
*   4. The part about the FUNCTIONS                                      *
*                                                                        *
*                                                                        *
*                                                                        *
*************************************************************************/

/*************************************************************************
*                                                                        *
*   4.1. The part about PROCESSES                                        *
*                                                                        *
*                                                                        *
*************************************************************************/

//------------------------------------------------------------------------
// OS_Create
//------------------------------------------------------------------------
PID OS_Create(void (*f)(void), int arg, unsigned int level, unsigned int n)
{
    int i, *stack;
    if(level == PERIODIC)
    {
        for(i = 0; i < MAXPROCESS; i++)
        {
            if(process_ptr[i].name == n)
                return INVALIDPID;
        }
    }
    
    for(i = 0; i < MAXPROCESS; i++)
    {
        IOFF();
        if(process_ptr[i].pid == INVALIDPID)
        {
            if(level == PERIODIC || level == DEVICE)
                process_ptr[i].name = n;
            if(level == DEVICE)
                process_ptr[i].wait_time = n;
            process_ptr[i].pid = i+1;
            process_ptr[i].level = level;
            process_ptr[i].state = READY;
            process_ptr[i].arg = arg;
            stack = (int*)(workspace+i*WORKSPACE);
            stack[WORKSPACE-1] = (int)OS_Terminate;
            stack[WORKSPACE-2] = 0x206;
            stack[WORKSPACE-3] = 0x8;
            stack[WORKSPACE-4] = (int)f;
            process_ptr[i].sp = (int)(&stack[WORKSPACE-10]);
            if(level == SPORADIC)
                add_node(&sporadic_list, (void*)(process_ptr+i), MAXPROCESS);
            if(running_proc)
                ION();
            return process_ptr[i].pid;
        }
    }
    return INVALIDPID;
}

//------------------------------------------------------------------------
// Adds a SPORADIC process to the queue
//------------------------------------------------------------------------
void add_node(struct list* list, void* ptr, int n)
{
    int i;
    for(i = 0; i < n; i++)
    {
        if(list->loc[i].ptr == NULL)
        {
            list->loc[i].next_node = list->tail;
            list->loc[i].prev_node = NULL;
            list->loc[i].ptr = ptr;
            if(list->tail)
                list->tail->prev_node = list->loc+i;
            list->tail = list->loc+i;
            if(list->head == NULL)
                list->head = list->loc+i;
            return;
        }
    }
}

//------------------------------------------------------------------------
// Returns a pointer to the process at the front of the queue
//------------------------------------------------------------------------
void* get_head(struct list* list)
{
    return list->head->ptr;
}

//------------------------------------------------------------------------
// Removes the node at the front of the queue
//------------------------------------------------------------------------
void remove_head(struct list* list)
{
    struct node* new_head = list->head->prev_node;
    list->head->prev_node = NULL;
    list->head->next_node = NULL;
    list->head->ptr = NULL;
    list->head = new_head;
}

//------------------------------------------------------------------------
// get process by PID
//------------------------------------------------------------------------
struct process* get_proc(PID pid)
{
    int i;
    for(i = 0; i < MAXPROCESS; i++)
    {
        if(process_ptr[i].pid == pid)
            return process_ptr+i;
    }
    return NULL;
}

//------------------------------------------------------------------------
// get pid of the running process
//------------------------------------------------------------------------
int OS_GetPid()
{
	return running_proc->pid;
}

//------------------------------------------------------------------------
// init process
//------------------------------------------------------------------------
void init()
{
	OS_Create(&shell_proc, NULL, SPORADIC, NULL);
}

//------------------------------------------------------------------------
// idle process
//------------------------------------------------------------------------
void idle()
{
    while(1);
}

//------------------------------------------------------------------------
// Terminate a process
//------------------------------------------------------------------------
void OS_Terminate()
{
    IOFF();
    running_proc->state = TERMINATE;
    running_proc->pid = INVALIDPID;
    running_proc->name = 0;
    running_proc->wait_time = 0xffffffff;
    if(running_proc->level == SPORADIC)
        remove_head(&sporadic_list);
    //fake_irq = 1;
    ION();
    //asm("trap");
    asm("int $48");
}

//------------------------------------------------------------------------
// Yield a process
//------------------------------------------------------------------------
void OS_Yield()
{
    IOFF();
    if(running_proc->level == DEVICE)
        running_proc->wait_time = running_proc->name;
    if(running_proc->state != WAITING)
        running_proc->state = READY;
    if(running_proc->level == SPORADIC) //move to tail if sporadic
    {
        remove_head(&sporadic_list);
        add_node(&sporadic_list, (void*)running_proc, MAXPROCESS);
    }
    //fake_irq = 1;
    ION();
    //asm("trap");
    asm("int $48");
}

//------------------------------------------------------------------------
// Get the arg, for the running process, that was provided by OS_Create
//------------------------------------------------------------------------
int OS_GetParam()
{
	return running_proc->arg;
}

/*************************************************************************
*                                                                        *
*   4.2. The part about the SCHEDULER                                    *
*                                                                        *
*                                                                        *
*************************************************************************/

//------------------------------------------------------------------------
// scheduler
//------------------------------------------------------------------------
void scheduler()
{
/*
    if there is a DEVICE process with 0 wait time pass it to context_switch()
    if that periodic name is not tied to a process or is IDLE schedule a SPORADIC process
    if there are no SPORADIC processes: idle
*/
    int ms;
    
    //finishing off the end of a periodic time slice
    if(running_proc && (running_proc->state == TERMINATE || running_proc->state == WAITING || running_proc->state == READY) && running_proc->level != DEVICE)
    {
        if(sporadic_list.head)
        {
            struct process* proc = get_head(&sporadic_list);
			while(proc->state == WAITING)
			{
				remove_head(&sporadic_list);
				add_node(&sporadic_list, (void*)proc, MAXPROCESS);
				proc = get_head(&sporadic_list);
			}
            context_switch(proc);
        }
        else
            context_switch(idle_proc);
        return;
    } //only gets past here if there was a timer interrupt or a device process yeilded
    
    //handle device process interruption
    int i;
    for(i = 0; i < MAXPROCESS; i++)
    {
        if(process_ptr[i].level == DEVICE && process_ptr[i].wait_time == 0)
        {
            if(running_proc->level != DEVICE)
                device_interrupted_proc = running_proc;
            context_switch(process_ptr+i);
            return;
        }
    }
    
    ms = calc_interval();
    
    //schedule process that the device process interrupted
    if(running_proc && running_proc->level == DEVICE)
    {
        set_timer(ms);
        context_switch(device_interrupted_proc);
        return;
    }
    
    if(PPP[PPPidx] != IDLE)
    {
        int i;
        for(i = 0; i < MAXPROCESS; i++)
        {
            if(process_ptr[i].name == PPP[PPPidx])
            {
                if(process_ptr[i].state == WAITING)
                    break;
                set_timer(ms);
                context_switch(process_ptr + i);
                return;
            }
        }
    }
    
    struct process* proc = get_head(&sporadic_list);
    while(sporadic_list.head && proc->state == WAITING)
    {
        remove_head(&sporadic_list);
        add_node(&sporadic_list, (void*)proc, MAXPROCESS);
        proc = get_head(&sporadic_list);
    }
    
    set_timer(ms);
    
    if(sporadic_list.head)
    {
        context_switch(proc);
        return;
    }
    
    //idle time when no sporadic processes
    context_switch(idle_proc);
}

//------------------------------------------------------------------------
// sets the interval timer
//------------------------------------------------------------------------
void set_timer(unsigned int ms)
{
    unsigned short count = CLK_RATE * ms * 1000;

    //decrement device process wait times
    int i;
    for(i = 0; i < MAXPROCESS; i++)
    {
        if(process_ptr[i].level == DEVICE)
            process_ptr[i].wait_time -= ms;
    }

    outportb(0x43, 0x30);
    outportb(0x40, count & 0xff);
    outportb(0x40, count >> 8);
}

//------------------------------------------------------------------------
// calculate the next time interval
//------------------------------------------------------------------------
int calc_interval()
{
    unsigned int time;
    if(running_proc->level == DEVICE)
        time = time_left; //time left for the device interrupted process
    else
    {
        //if not comming in off a device process switch to next slice
        PPPidx = (PPPidx+1)%PPPLen;
        time = PPPMax[PPPidx];
    }
    int i;
    for(i = 0; i < MAXPROCESS; i++)
    {
        if(process_ptr[i].level == DEVICE && process_ptr[i].wait_time < time)
        {
            time_left = time - process_ptr[i].wait_time;
            time = process_ptr[i].wait_time;
        }
    }
    return time;
}
        

//------------------------------------------------------------------------
// switch context to a given process
//------------------------------------------------------------------------
void context_switch(struct process* process_ptr)
{
    if(running_proc->state == RUNNING)
        running_proc->state = READY;
    running_proc = process_ptr;
    running_proc->state = RUNNING;
    outportb(0x20, 0x20);
}

/*************************************************************************
*                                                                        *
*   4.3. The part about INTERRUPTS                                       *
*                                                                        *
*                                                                        *
*************************************************************************/

//------------------------------------------------------------------------
// exception handler 
//------------------------------------------------------------------------
void handle_exception()
{
    //do something
    while(1);
}

//------------------------------------------------------------------------
// irq handler 
//------------------------------------------------------------------------
void handle_irq()
{
    //do something
    outportb(0xA0, 0x20);
    outportb(0x20, 0x20);
}


/*************************************************************************
*                                                                        *
*   4.4. The part about the FIFOS                                        *
*                                                                        *
*                                                                        *
*************************************************************************/

//------------------------------------------------------------------------
// Initialize a fifo
//------------------------------------------------------------------------
FIFO  OS_InitFiFo()
{
    
    int i;
    for (i = 0; i < MAXFIFO; i++)
    {
        if (fifo_ptr[i].descriptor == INVALIDFIFO)
        {
            fifo_ptr[i].descriptor = i+1;
            fifo_ptr[i].write_idx = FIFOSIZE-1;
            fifo_ptr[i].read_idx = FIFOSIZE-1;
            return fifo_ptr[i].descriptor;
        }
    }
    return INVALIDFIFO;
}

//------------------------------------------------------------------------
// Write to a fifo
//------------------------------------------------------------------------
void  OS_Write( FIFO f, int val )
{
    struct fifo* fifo = get_fifo(f);
    if(!fifo)
        return;
    if (fifo->write_idx == -1)
        shift_fifo(fifo);

    fifo->buf[fifo->write_idx] = val;
	fifo->write_idx--;
}

//------------------------------------------------------------------------
// Read from a fifo
//------------------------------------------------------------------------
BOOL  OS_Read( FIFO f, int *val )
{
    struct fifo* fifo = get_fifo(f);
    if(!fifo || is_empty(fifo))
        return FALSE;
    *val = fifo->buf[fifo->read_idx];
    fifo->read_idx--;
    return TRUE;
}

//------------------------------------------------------------------------
// Reorganize the fifo buffer
//------------------------------------------------------------------------
void shift_fifo(struct fifo* fifo)
{
    int shift = FIFOSIZE-1 - fifo->read_idx;
    if(!shift)
    {
        fifo->read_idx--;
        shift = 1;
    }
    while(fifo->read_idx > fifo->write_idx)
    {
        fifo->buf[fifo->read_idx + shift] = fifo->buf[fifo->read_idx];
        fifo->read_idx--;
    }
	fifo->read_idx = FIFOSIZE-1;
    fifo->write_idx += shift;
}

//------------------------------------------------------------------------
// Check if a fifo is empty
//------------------------------------------------------------------------
BOOL is_empty(struct fifo* fifo)
{
    return fifo->read_idx == fifo->write_idx;
}

//------------------------------------------------------------------------
// Return the fifo with a given descriptor
//------------------------------------------------------------------------
struct fifo* get_fifo(unsigned int descriptor)
{
    int i;
    for(i=0; i < MAXFIFO; i++)
    {
        if(fifo_ptr[i].descriptor == descriptor)
            return fifo_ptr+i;
    }
    return NULL;
}

/*************************************************************************
*                                                                        *
*   4.5. The part about SEMAPHORES                                       *
*                                                                        *
*                                                                        *
*************************************************************************/

//------------------------------------------------------------------------
// Init a semaphore
//------------------------------------------------------------------------
void OS_InitSem(int s, int n)
{    
    int i;
    for (i = 0; i < MAXSEM; i++)
    {
        if (semaphore_ptr[i].id == -1)
        {
            semaphore_ptr[i].id = s;
            semaphore_ptr[i].avail = n;
            semaphore_ptr[i].sleepers = 0;
            semaphore_ptr[i].wait_queue.loc = semaphore_ptr[i].wait_queue_mem;
            int j;
            for (j = 0; j < MAXPROCESS; j++)
            {
                semaphore_ptr[i].curr_pids[j] = INVALIDPID;
            }
            return;
        }
    }
}

//------------------------------------------------------------------------
// Wait on a semaphore
//------------------------------------------------------------------------
void OS_Wait(int s)
{
    IOFF();
    
    struct semaphore* semaphore = get_semaphore(s);
    if(semaphore == NULL)
        return;
    semaphore->avail--;
    if(semaphore->avail < semaphore->sleepers)
    {
        add_node(&semaphore->wait_queue, (void*)running_proc, MAXPROCESS);
        semaphore->sleepers++;
        running_proc->state = WAITING;
        OS_Yield();
        remove_head(&semaphore->wait_queue);
        semaphore->sleepers--;
    }
    int i;
    for (i = 0; i < MAXPROCESS; i++)
    {
        if(semaphore->curr_pids[i] == INVALIDPID)
        {
            semaphore->curr_pids[i] = running_proc->pid;
            break;
        }
    }        
    ION();
}

//------------------------------------------------------------------------
// Release semaphore
//------------------------------------------------------------------------
void OS_Signal(int s)
{
    IOFF();
    struct semaphore* semaphore = get_semaphore(s);
    if(!semaphore | !holds_semaphore(semaphore))
        return;
        
    semaphore->avail++;
    if(semaphore->avail <= 1)
    {
        if(semaphore->wait_queue.head)
        {
            struct node* node = semaphore->wait_queue.head;
            while(((struct process*)(node->ptr))->state == READY)
            {
				if(!node->prev_node)
					break;
                node = node->prev_node;
            }
            ((struct process*)(node->ptr))->state = READY;
        }
    }
    
    int i;
    for (i = 0; i < MAXPROCESS; i++)
    {
        if(semaphore->curr_pids[i] == running_proc->pid)
        {
            semaphore->curr_pids[i] = INVALIDPID;
            break;
        }
    }        
    ION();
}

//------------------------------------------------------------------------
// get a semaphore by id
//------------------------------------------------------------------------
struct semaphore* get_semaphore(int id)
{
    int i;
    for(i=0; i < MAXSEM; i++)
    {
        if(semaphore_ptr[i].id == id)
            return semaphore_ptr+i;
    }
    return NULL;
}

//------------------------------------------------------------------------
// check if the running process holds the semaphore
//------------------------------------------------------------------------
BOOL holds_semaphore(struct semaphore* semaphore)
{
    int i;
    for (i = 0; i < MAXPROCESS; i++)
    {
        if(semaphore->curr_pids[i] == running_proc->pid)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*************************************************************************
*                                                                        *
*   4.6 The part about INITIALIZATION                                   *
*                                                                        *
*                                                                        *
*************************************************************************/

//------------------------------------------------------------------------
// OS_Init
//------------------------------------------------------------------------
void OS_Init()
{

    gdt_install();
    idt_install();
    irq_install();

    PPPLen  = 7;
    PPPidx = -1;

    /*sporadic_ptr = (struct node*)(process_ptr + MAXPROCESS);
    workspace = (char*)(sporadic_ptr + MAXPROCESS);
    fifo_ptr = (struct fifo*)(workspace + MAXPROCESS * WORKSPACE);
    semaphore_ptr = (struct semaphore*)(fifo_ptr + MAXFIFO);*/
    
    sporadic_list.loc = sporadic_ptr;
    sporadic_list.head = NULL;
    sporadic_list.tail = NULL;
    
    running_proc = NULL;
    fake_irq = 0;
    
    //NIOS2_WRITE_IENABLE(0x1);//Enable Timer Interrupt
    
    int i;
    for(i = 0; i < MAXPROCESS; i++)
    {
        sporadic_ptr[i].next_node = NULL;
        sporadic_ptr[i].prev_node = NULL;
        sporadic_ptr[i].ptr = NULL;
        process_ptr[i].pid = INVALIDPID;
        process_ptr[i].name = 0;
        process_ptr[i].wait_time = 0xFFFFFFFF;
        fifo_ptr[i].descriptor = INVALIDFIFO;
        semaphore_ptr[i].id = -1;
    }

    OS_Create(&init, NULL, SPORADIC, NULL);
    PID pid = OS_Create(&idle, NULL, PERIODIC, IDLE);
    idle_proc = get_proc(pid);
}

//------------------------------------------------------------------------
// OS_Abort
//------------------------------------------------------------------------
void OS_Abort()
{
	//reset();
}

//------------------------------------------------------------------------
// OS_Start
//------------------------------------------------------------------------
asm("OS_Start:");
asm("int $48");

//------------------------------------------------------------------------
// main
//------------------------------------------------------------------------
void kmain()
{
    OS_Init();
    OS_Start();
}
