/*
 * FSSP.c
 *
 *  Created on: May 1, 2014
 *      Author: oron
 */



#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.c"


#define Q 0
#define P 1
#define R 2
#define Z 3
#define M 4
#define AS 5 //Asterisk
#define F 6
#define E 7 //Empty

//#define MAX_THREAD 64;

int STATEARRAY[5][6][6]={	{	{Q,P,Q,Q,E,Q},{P,P,E,E,E,P},{Q,E,Q,E,E,E},{Q,E,E,Q,E,Q},{E,E,E,E,E,E},{Q,P,Q,Q,E,E}	}, //Q
							{	{Z,Z,R,R,E,E},{Z,E,Z,Z,E,E},{R,Z,Z,E,E,Z},{R,Z,E,Z,E,Z},{E,E,E,E,E,E},{Z,E,Z,Z,E,E}	}, //P
							{	{E,E,R,P,Z,E},{E,E,M,R,M,E},{R,M,E,E,M,E},{P,R,E,E,R,E},{Z,M,M,R,M,E},{E,E,E,E,E,E}	}, //R
							{	{E,E,Q,P,Q,E},{E,Z,E,Z,E,E},{Q,E,Q,Q,E,Q},{P,Z,Q,F,Q,F},{Q,E,E,Q,Q,Q},{E,Z,Q,F,Q,E}	}, //Z
							{	{E,E,E,E,E,E},{E,E,E,E,E,E},{E,E,R,Z,E,E},{E,E,Z,E,E,E},{E,E,E,E,E,E},{E,E,E,E,E,E}	} }; //M


int currentState[2][MAX_THREAD];
int soldiersAmount;
int state;

struct binary_semaphore arrival, departure;// binary_semaphore*
int counter;

int fatherPID;

void down()
{
	binary_semaphore_down(&arrival);
	counter++;
	if(counter < soldiersAmount)
	{
		binary_semaphore_up(&arrival);
	}
	else
	{
		binary_semaphore_up(&departure);
	}
}

void up()
{
	binary_semaphore_down(&departure);
	counter--;
	if(counter > 0)
	{
		binary_semaphore_up(&departure);
	}
	else
	{

		//sigsend(fatherPID,4);
		sigsend(fatherPID,4);
		//state++; //TODO: add signal to main, and ask main to do it
		//binary_semaphore_up(&arrival);
	}
}



void run(void *t)
{
	int myNumber= (int) t;
	int myState;
	int leftState;
	int rightState;
	while(currentState[state%2][myNumber]!=F)
	{
		down();
		myState= currentState[state%2][myNumber];
		//printf(1,"%d: myState:%d, state:%d\n",myNumber,myState,state);
		leftState=currentState[state%2][myNumber-1];
		rightState=currentState[state%2][myNumber+1];
		currentState[(state+1)%2][myNumber]= STATEARRAY[myState][leftState][rightState];

		up();
	}
}

/*void test(void *t)
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

}*/


void printArray()
{
	int i=1;
	for(;i<=soldiersAmount;i++)
		printf(1,"%d ",currentState[state%2][i]);
	printf(1,"\n");
	state++; //TODO: add signal to main, and ask main to do it
	binary_semaphore_up(&arrival);

}



int main(int argc,char** argv)
{
	int i;
	if(argc!=2)
	{
		printf(1,"please write how much soldiers do you want (LESS THEN 64)\n");
		goto out_err;
	}

	soldiersAmount=atoi(argv[1]);

	if(soldiersAmount>64 || soldiersAmount<2)
	{
		printf(1,"please write how much soldiers do you want (LESS THEN 64)\n");
		goto out_err;
	}

	currentState[0][0]=AS;
	currentState[1][0]=AS;
	currentState[0][soldiersAmount+1]=AS;
	currentState[1][soldiersAmount+1]=AS;

	state=0;
	counter=0;
	binary_semaphore_init(&arrival,1);
	binary_semaphore_init(&departure,0);

	fatherPID=getpid();
	signal(4,printArray);

	currentState[0][1]=P;
	for(i=2; i<=soldiersAmount;i++)
	{
		currentState[0][i]=Q;
	}
	uthread_init();


	int tid;
	for(i=1;i<=soldiersAmount;i++)
	{
		tid = uthread_create(run, (void *) i);
		if (!tid)
			goto out_err;
	}

	// int myState =0;
    for(i=1;i<soldiersAmount;i++)
    {
		while (currentState[0][i]!=F && currentState[1][i]!=F)
		{
			  uthread_yield();
		}
    }

	 uthread_yield();


	//printArray();
	//printArray();
	uthread_exit();

	out_err:
	printf(1,"Faild to create thread, we go bye bye\n");
	exit();
}

