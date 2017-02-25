/*	User-level thread system
 *
 */

#include <setjmp.h>

#include "aux.h"
#include "umix.h"
#include "mythreads.h"

static int MyInitThreadsCalled = 0;	// 1 if MyInitThreads called, else 0

static struct thread {			// thread table
	int valid;			// 1 if entry is valid, else 0
	jmp_buf env;			// current context

} thread[MAXTHREADS];

int currentthread;
int newlycreated;


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
		//char s[STACKSIZE]; 		// reserve stack space
	}

	thread[0].valid = 1;			// initialize thread 0
	currentthread = 0;
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

	/* Get the currently running thread */
	int T = MyGetThread();

	//if (setjmp (thread[0].env) == 0) {	// save context of thread 0


	/* looks for the next available spot in the thread table */

	int created = 0;

	for (int i = newlycreated+1; i < MAXTHREADS; i++) {
		if (!thread[i].valid)	{
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
		
			if (!thread[i].valid)	{
				//we can place the newly created thread here
				newlycreated = i;
				thread[i].valid = 1;   //mark the entry for the new thread valid
				created = 1;
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



	/* save the context of the current running thread */
	if (setjmp (thread[T].env) == 0) {

		/* The new thread will need stack space.  Here we use the
		 * following trick: the new thread simply uses the current
		 * stack, and so there is no need to allocate space.  However,
		 * to ensure that thread 0's stack may grow and (hopefully)
		 * not bump into thread 1's stack, the top of the stack is
		 * effectively extended automatically by declaring a local
		 * variable (a large "dummy" array). This array is never
		 * actually used; to prevent an optimizing compiler from
		 * removing it, it should be referenced.  
		 */
		


		char s[STACKSIZE];	// reserve space for thread 0's stack
		void (*f)() = func;	// f saves func on top of stack
		int p = param;		// p saves param on top of stack


//??????? WHAT THIS DO ???? 
		if (((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) {
			Printf ("Stack space reservation failed\n");
			Exit ();
		} 
/*
		if (setjmp (thread[1].env) == 0) {	// save context of 1
	
			
			longjmp (thread[0].env, 1);	// back to thread 0
		}
*/

	 	/*  save the context of newly created thread */
		if (setjmp (thread[newlycreated].env) == 0) {
			longjmp (thread[T].env,1);	//back to current thread
		}


		/* here when thread 1 is scheduled for the first time */
	       	(*f) (p);			// execute func (param)
		MyExitThread ();		// thread 1 is done - exit
	}

	//thread[1].valid = 1;	// mark the entry for the new thread valid


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

/* a thread might yield to itself... */

	//Get the calling thread

	int T = MyGetThread();

        if (setjmp (thread[T].env) == 0) {
		currentthread = t;
                longjmp (thread[t].env, 1);
        }

	//get the current calling thread that is yielding last
	
	T = MyGetThread();


/* ID of the calling thread must be properly returned */
	return (T);

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

	

}

/*	MyExitThread () causes the currently running thread to exit.  
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("ExitThread: Must call InitThreads first\n");
		Exit ();
	}
}
