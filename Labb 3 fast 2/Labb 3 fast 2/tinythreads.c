#include <setjmp.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "tinythreads.h"

#define NULL            0
#define DISABLE()       cli()
#define ENABLE()        sei()
#define STACKSIZE       80
#define NTHREADS        4
#define SETSTACK(buf,a) *((unsigned int *)(buf)+8) = (unsigned int)(a) + STACKSIZE - 4; \
*((unsigned int *)(buf)+9) = (unsigned int)(a) + STACKSIZE - 4

struct thread_block {
	void (*function)(int);   // code to run
	int arg;                 // argument to the above
	thread next;             // for use in linked lists
	jmp_buf context;         // machine state
	char stack[STACKSIZE];   // execution stack space
};

struct thread_block threads[NTHREADS];

struct thread_block initp;

thread freeQ   = threads;
thread readyQ  = NULL;
thread current = &initp;

mutex m1 = MUTEX_INIT; // f�r lampan
mutex m2 = MUTEX_INIT; // f�r spaken
unsigned short timekeeper = 0;
int initialized = 0;

static void initialize(void) {
	int i;
	for (i=0; i<NTHREADS-1; i++)
	threads[i].next = &threads[i+1];
	threads[NTHREADS-1].next = NULL;
	PORTB = (1<<PB7);
	EIMSK = 0x80;
	PCMSK1 = 0x80;
	TCCR1A = 0xC0;
	TCCR1B = 0x18;
	
	//OC1A is set high on compare match.
	TCCR1A = (1 << COM1A0) | (1 << COM1A1);
	
	// Set timer to CTC and prescale Factor on 1024.
	TCCR1B = (1 << WGM12) | (1 << CS10) |(1 << CS12);
	
	// Set Value to around 50ms. 8000000/20480 = 390.625
	OCR1A = 3906;
	
	//clearing the TCNT1 register during initialization.
	TCNT1 = 0x0;
	
	//Compare a match interrupt Enable.
	TIMSK1 = (1 << OCIE1A);
	
	initialized = 1;
}

static void enqueue(thread p, thread *queue) {
	p->next = *queue;
	*queue = p;
}

static thread dequeue(thread *queue) { // dequeue h�mtar nu den som �r n�st l�ngst fram f�r annars byter vi aldrig tr�d
	thread p = *queue;
	if (*queue) {
		*queue = (*queue)->next;
	} else {
		// Empty queue, kernel panic!!!
		while (1) ; // not much else to do...
	}
	return p;
}

static void dispatch(thread next) {
	if (setjmp(current->context) == 0) {
		current = next;
		longjmp(next->context,1);
	}
}

void spawn(void (* function)(int), int arg) {
	thread newp;

	DISABLE();
	if (!initialized) initialize();

	newp = dequeue(&freeQ);
	newp->function = function;
	newp->arg = arg;
	newp->next = NULL;
	if (setjmp(newp->context) == 1) {
		ENABLE();
		current->function(current->arg);
		DISABLE();
		enqueue(current, &freeQ);
	}
	SETSTACK(&newp->context, &newp->stack);

	dispatch(newp);
	ENABLE();
}

void yield(void) {
	DISABLE();
	enqueue(current, &readyQ);
	dispatch(dequeue(&readyQ));
	ENABLE();
}

void lock(mutex *m) {
	DISABLE();
	if(m->locked){
		/*if(*m->waitQ == NULL){
			*m->waitQ = current;
		}else{
			thread q = *m->waitQ;
			while (q->next){
				q = q->next;
			}
			q->next = current;
		}*/
		enqueue(current, &(m->waitQ));
		dispatch(dequeue(&readyQ));
	}else {
		m->locked = 1;
	}
	ENABLE();
}

void unlock(mutex *m) {
	DISABLE();
	if(m->waitQ == NULL){
		m->locked = 0;
	}else{
		/*thread p = *m->waitQ;
		*m->waitQ = *m->waitQ->next;
		dispatch(p);*/
		enqueue(current, readyQ);
		dispatch(dequeue(&(m->waitQ)));
	}
	ENABLE();
}

//om interrupt p� pinb7 yielda
ISR(PCINT1_vect)
{
	if ((PINB >> 7) == 1)
	{
		unlock(&m2);
	}
}
//Om timern s�ger till, yielda

ISR(TIMER1_COMPA_vect)
{
	unlock(&m1);
}