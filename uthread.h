#define THREAD_QUANTA 5

/* Possible states of a thread; */
typedef enum  {T_FREE, T_RUNNING, T_RUNNABLE, T_SLEEPING} uthread_state;

#define STACK_SIZE  4096
#define MAX_THREAD  64
#define UTHREAD_QUANTA  5
#define QUEUESIZE MAX_THREAD

typedef struct uthread uthread_t, *uthread_p;

struct uthread {
	int				tid;
	int 	       	esp;        /* current stack pointer */
	int 	       	ebp;        /* current base pointer */
	char		   *stack;	    /* the thread's stack */
	uthread_state   state;     	/* running, runnable, sleeping */
	void (*func)(void *);
	void* value;
	int FirstRun;
};
 

typedef struct {
        struct uthread * q[QUEUESIZE];   /* body of queue */
        int first;                      /* position of first element */
        int last;                       /* position of last element */
        int count;                      /* number of queue elements */
} RRqueue;

struct binary_semaphore 
{
	int ticket;
	RRqueue queue;
};

void uthread_init(void);
int  uthread_create(void (*func)(void *), void* value);
void uthread_exit(void);
void uthread_yield(void);
int  uthred_self(void);
int  uthred_join(int tid);

void binary_semaphore_init(struct binary_semaphore* semaphore, int value);
void binary_semaphore_down(struct binary_semaphore* semaphore);
void binary_semaphore_up(struct binary_semaphore* semaphore);