
#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.c"


void test(void *t)
{
	

  int i = 0;
	while (i < 10)
	{
		printf(1,"thread child %p \n", t);
		i++;
	
		if(i==5)
		{
			if(uthred_self() == 11)
			{
				uthred_join(12);
			}
			else
			{
				uthred_join(10);
			}
		}

		sleep(4);
		//uthread_yield();
	}

}
int main(int argc,char** argv)
{
	uthread_init();
	int tid = uthread_create(test, (void *) 1);
	if (!tid)
		goto out_err;
	
	tid = uthread_create(test, (void *) 2);
	if (!tid)
		goto out_err;
    
    int i =0;
    while (i < 20){
          printf(1,"thread father\n");
          ++i;
          sleep(2);
       //   uthread_yield();
    }
    
	uthread_exit();

	out_err:
	printf(1,"Faild to create thread, we go bye bye\n");
	exit();
}
