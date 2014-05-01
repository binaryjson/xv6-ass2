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

static RRqueue YieldQueue; 

void enqueue(RRqueue* pqueue, struct uthread * x)
{
    if (pqueue->count >= QUEUESIZE)
    {
          printf(1,"Warning: queue overflow enqueue ");
    }
    else 
    {
            pqueue->last = (pqueue->last+1) % QUEUESIZE;
            pqueue->q[ pqueue->last ] = x;  
            pqueue->count = pqueue->count + 1;
    }

}

struct uthread * dequeue(RRqueue* pqueue)
{
    struct uthread * x;

    if (pqueue->count <= 0) 
    {
    	printf(1, "Warning: empty queue dequeue.\n");
    }
    else 
    {
            x = pqueue->q[ pqueue->first ];
            pqueue->first = (pqueue->first+1) % QUEUESIZE;
            pqueue->count = pqueue->count - 1;
    }

    return(x);
}

void uthread_init()
{
	// Init queue 
	YieldQueue.first = 0;
	YieldQueue.last = QUEUESIZE-1;
	YieldQueue.count = 0;

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

/*
	int esp,ebp;

	STORE_ESP(esp);
	STORE_EBP(ebp);

	LOAD_ESP(MyThreads[i].esp);
	LOAD_EBP(MyThreads[i].ebp);

	asm("pushal");

	LOAD_ESP(esp);
	LOAD_EBP(ebp);*/

	enqueue(&YieldQueue,&MyThreads[i]);

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

void inner_uthread_yield(int ReturntoQueue)
{
	alarm(0);

	STORE_ESP(CurrThread->esp);
	STORE_EBP(CurrThread->ebp);

	//asm("pushal");

	if((CurrThread->state != T_FREE) && ReturntoQueue)
	{
		CurrThread->state = T_RUNNABLE;

		enqueue(&YieldQueue,CurrThread);
	}

	if(YieldQueue.count > 0)
	{
		CurrThread = dequeue(&YieldQueue);

		CurrThread->state = T_RUNNING;

		LOAD_ESP(CurrThread->esp);
		LOAD_EBP(CurrThread->ebp);

		//asm("popal");

		if(CurrThread->FirstRun)
		{
			CurrThread->FirstRun = 0;

			alarm(UTHREAD_QUANTA); 

			wrapper(CurrThread->func, CurrThread->value);
		}
		
		alarm(UTHREAD_QUANTA); 
		return;
	}
	else
	{
		exit();
	}
}

void uthread_yield()
{
	inner_uthread_yield(1);
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

void binary_semaphore_init(struct binary_semaphore* semaphore, int value)
{
	semaphore->ticket = value;

	// Init queue 
	semaphore->queue.first = 0;
	semaphore->queue.last = QUEUESIZE-1;
	semaphore->queue.count = 0;
}

void binary_semaphore_down(struct binary_semaphore* semaphore)
{
	alarm(0);

	if(semaphore->ticket)
	{
		--semaphore->ticket;	
		alarm(UTHREAD_QUANTA);
	}
	else
	{
		enqueue(&semaphore->queue, CurrThread);
		inner_uthread_yield(0);
	}
}

void binary_semaphore_up(struct binary_semaphore* semaphore)
{
	alarm(0);

	if(!semaphore->ticket)
	{
		if(semaphore->queue.count > 0)
		{
			enqueue(&YieldQueue, dequeue(&semaphore->queue));
		}
		else
		{
			++semaphore->ticket;	
		}
	}

	alarm(UTHREAD_QUANTA);
}