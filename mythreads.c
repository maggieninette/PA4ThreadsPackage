/*	User-level thread system
 *
 */

#include <setjmp.h>
#include <strings.h>
#include "aux.h"
#include "umix.h"
#include "mythreads.h"

static int MyInitThreadsCalled = 0;	// 1 if MyInitThreads called, else 0

static struct thread {			// thread table
	int valid;			// 1 if entry is valid, else 0
	jmp_buf env;			// current context
	jmp_buf back_up;

	void (*f)();			// function to be executed
	int parameter;			// integer parameter
	int timestamp;
} thread[MAXTHREADS];

int previousthread;
int currentthread;
int newlycreated;
int currenttime = 0;

#define STACKSIZE	65536		// maximum size of thread stack

/*	MyInitThreads () initializes the thread package. Must be the first
 *	function called by any user program that uses the thread package.  
 */

void MyInitThreads ()
{
	int i;

	if (MyInitThreadsCalled) {		// run only once
		Printf ("InitThreads: should be called only once\n");
		Exit ();
	}

	for (i = 0; i < MAXTHREADS; i++) {	// initialize thread table
		thread[i].valid = 0;
		char s[i*STACKSIZE]; 		// reserve stack space for all threads in thread table	

// not sure if we need this or not

		if (((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) {
			Printf ("Stack space reservation failed\n");
			Exit ();
		} 

	
/*
*/
		if (setjmp (thread[i].env) != 0){
			if (*thread[currentthread].f) {	       		
			thread[currentthread].f(thread[currentthread].parameter);	// execute func (param)
			MyExitThread ();		
			}
		}


		memcpy(thread[i].back_up,thread[i].env,sizeof(jmp_buf));



	}

	thread[0].valid = 1;			// initialize thread 0
	currentthread = 0;
	thread[0].timestamp = currenttime;
	currenttime++;

	newlycreated = 0;
	MyInitThreadsCalled = 1;
	
}

/*	MyCreateThread (func, param) creates a new thread to execute
 *	func (param), where func is a function with no return value and
 *	param is an integer parameter.  The new thread does not begin
 *	executing until another thread yields to it. 
 */

int MyCreateThread (func, param)
	void (*func)();			// function to be executed
	int param;			// integer parameter
{
	if (! MyInitThreadsCalled) {
		Printf ("CreateThread: Must call InitThreads first\n");
		Exit ();
	}

	/* looks for the next available spot in the thread table */

	int created = 0;

	for (int i = newlycreated+1; i < MAXTHREADS; i++) {
		if (thread[i].valid == 0)	{
			//we can place the newly created thread here
			newlycreated = i;
			created = 1;
			thread[i].valid = 1;
			break;
		}
	}
	/* start again at the beginning if it reached the end of table */
	if (created == 0) {
		for (int i = 0; i < MAXTHREADS; i++) {
		
			if (thread[i].valid == 0)	{
				//we can place the newly created thread here
				newlycreated = i;
				thread[i].valid = 1;   //mark the entry for the new thread valid
				created = 1;
				break;
			}
		}
	}
/* If there is an error, such as if
 * there are already MAXTHREADS active threads (and no more can be created
 * until one or more exit), MyCreateThread should simply return -1.
*/ 
	if (created == 0) {
		return (-1);
	}
	

	//resetting the env buffer here
	memcpy (thread[newlycreated].env,thread[newlycreated].back_up,sizeof(jmp_buf));

	//save the function pointer and param fields
	thread[newlycreated].f = func;
	thread[newlycreated].parameter = param;
		
	thread[newlycreated].timestamp = currenttime;
	currenttime++;

	return (newlycreated);		// done, return new thread ID

}

/*	MyYieldThread (t) causes the running thread, call it T, to yield to
 *	thread t.  Returns the ID of the thread that yielded to the calling
 *	thread T, or -1 if t is an invalid ID. Example: given two threads
 *	with IDs 1 and 2, if thread 1 calls MyYieldThread (2), then thread 2
 *	will resume, and if thread 2 then calls MyYieldThread (1), thread 1
 *	will resume by returning from its call to MyYieldThread (2), which
 *	will return the value 2.
 */

int MyYieldThread (t)
	int t;				// thread being yielded to
{
	if (! MyInitThreadsCalled) {
		Printf ("YieldThread: Must call InitThreads first\n");
		Exit ();
	}

	if (t < 0 || t >= MAXTHREADS) {
		Printf ("YieldThread: %d is not a valid thread ID\n", t);
		return (-1);
	}

	if (! thread[t].valid) {
		Printf ("YieldThread: Thread %d does not exist\n", t);
		return (-1);
	}


	previousthread = MyGetThread();
        if (setjmp (thread[MyGetThread()].env) == 0) {

		// "placing the calling thread at the end of the queue (setting it as oldest time)
		thread[MyGetThread()].timestamp = currenttime;
		currenttime++;

		//remove t from the queue by setting its timestamp to -1
		currentthread = t;
		thread[t].timestamp = -1;
                longjmp (thread[t].env, 1);
        }

	//get the current calling thread that is yielding last
	


/* ID of the calling thread must be properly returned */
	return (previousthread);

}

/*	MyGetThread () returns ID of currently running thread.  
 */

int MyGetThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("GetThread: Must call InitThreads first\n");
		Exit ();
	}
	return currentthread;

	

}

/*	MySchedThread () causes the running thread to simply give up the
 *	CPU and allow another thread to be scheduled.  Selecting which
 *	thread to run is determined here. Note that the same thread may
 * 	be chosen (as will be the case if there are no other threads).  
 */

void MySchedThread ()
{


	if (! MyInitThreadsCalled) {
		Printf ("SchedThread: Must call InitThreads first\n");
		Exit ();
	}


	//implements FIFO so it selects the thread with the smallest timestamp
	int min_time = 9999;


       	int T = MyGetThread();

	int sched_next = -1;

	
	for (int i = 0; i < MAXTHREADS; i++) {
		if (thread[i].valid) {
			if (thread[i].timestamp < min_time) {
				sched_next = i;
				min_time = thread[i].timestamp;
							
			}
		}
	}



	if (sched_next == -1) {
		Exit();
	}	


	MyYieldThread(sched_next);


}




/*	MyExitThread () causes the currently running thread to exit.  
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("ExitThread: Must call InitThreads first\n");
		Exit ();
	}
	thread[currentthread].valid = 0;
	


	MySchedThread();

}







