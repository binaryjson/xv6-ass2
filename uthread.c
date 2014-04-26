#include "types.h"
#include "user.h"
#include "uthread.h"
#include "signal.h"


#define STORE_ESP(var)  asm("movl %%esp, %0;" : "=r" ( var ))
#define LOAD_ESP(var)   asm("movl %0, %%esp;" : : "r" ( var ))
#define STORE_EBP(var)  asm("movl %%ebp, %0;" : "=r" ( var ))
#define LOAD_EBP(var)   asm("movl %0, %%ebp;" : : "r" ( var ))


static struct uthread MyThreads[MAX_THREAD];
static struct uthread * CurrThread;
static int IDCounter;

#define QUEUESIZE MAX_THREAD

struct {
        struct uthread * q[QUEUESIZE];   /* body of queue */
        int first;                      /* position of first element */
        int last;                       /* position of last element */
        int count;                      /* number of queue elements */
} RRqueue;

void enqueue(struct uthread * x)
{
    if (RRqueue.count >= QUEUESIZE)
    {
          printf(1,"Warning: queue overflow enqueue ");
    }
    else 
    {
            RRqueue.last = (RRqueue.last+1) % QUEUESIZE;
            RRqueue.q[ RRqueue.last ] = x;  
            RRqueue.count = RRqueue.count + 1;
    }

}

struct uthread * dequeue()
{
    struct uthread * x;

    if (RRqueue.count <= 0) 
    {
    	printf(1, "Warning: empty queue dequeue.\n");
    }
    else 
    {
            x = RRqueue.q[ RRqueue.first ];
            RRqueue.first = (RRqueue.first+1) % QUEUESIZE;
            RRqueue.count = RRqueue.count - 1;
    }

    return(x);
}

void uthread_init()
{
	// Init queue 
	RRqueue.first = 0;
	RRqueue.last = QUEUESIZE-1;
	RRqueue.count = 0;

	int i;
	for(i=0; i < MAX_THREAD; ++i)
	{
		MyThreads[i].state = T_FREE;
	}

	IDCounter = 10;
	MyThreads[0].tid = IDCounter;

	MyThreads[0].state = T_RUNNING;
	MyThreads[0].stack = 0;
	MyThreads[0].FirstRun = 0;

	STORE_ESP(MyThreads[0].esp);
	STORE_EBP(MyThreads[0].ebp);

	CurrThread = &MyThreads[0];

	signal(SIGALRM, uthread_yield);
	alarm(UTHREAD_QUANTA);
}

int  uthread_create(void (*func)(void *), void* value)
{
	int i;
	// Find next free thread
	for(i=0; i < MAX_THREAD; ++i)
	{
		if(MyThreads[i].state == T_FREE)
		{
			break;		
		}
	}

	if(i == MAX_THREAD)
	{
		return 1; // error
	}

	++IDCounter;
	MyThreads[i].tid = IDCounter;
	MyThreads[i].state = T_RUNNABLE;
	MyThreads[i].stack = malloc(STACK_SIZE + 4); 
	MyThreads[i].esp = (int)MyThreads[i].stack + STACK_SIZE ;
	MyThreads[i].ebp = MyThreads[i].esp; 
	MyThreads[i].func = func;
	MyThreads[i].value = value;
	MyThreads[i].FirstRun = 1;

	enqueue(&MyThreads[i]);

	return IDCounter;
}

void uthread_exit()
{
	if(CurrThread->stack != 0) // Check if not null - if null probaly the main thread
	{
		free (CurrThread->stack);
	}
	
	CurrThread->state = T_FREE;
	
	uthread_yield();
	
}

void wrapper(void (*func)(void *), void* value)
{
	func(value);
	uthread_exit();	
}

void uthread_yield()
{
	alarm(UTHREAD_QUANTA);

	STORE_ESP(CurrThread->esp);
	STORE_EBP(CurrThread->ebp);

	if(CurrThread->state != T_FREE)
	{
		CurrThread->state = T_RUNNABLE;

		enqueue(CurrThread);
	}

	if(RRqueue.count > 0)
	{
		CurrThread = dequeue();

		CurrThread->state = T_RUNNING;

		LOAD_ESP(CurrThread->esp);
		LOAD_EBP(CurrThread->ebp);

		if(CurrThread->FirstRun)
		{
			CurrThread->FirstRun = 0;
			wrapper(CurrThread->func, CurrThread->value);
		}
		
		return;
	}
	else
	{
		exit();
	}
}

int  uthred_self()
{
	return CurrThread->tid;
}

int  uthred_join(int tid)
{
	int i;
	for(i=0; i < MAX_THREAD; ++i)
	{
		if(MyThreads[i].tid == tid)
		{
			break;
		}
	}

	if(i < MAX_THREAD)
	{
		while((MyThreads[i].tid == tid ) &&
			  (MyThreads[i].state != T_FREE))
		{
			CurrThread->state = T_SLEEPING;	

			uthread_yield();
		}
	}

	CurrThread->state = T_RUNNING;

	return 1;
}