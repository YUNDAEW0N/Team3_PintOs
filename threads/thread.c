#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
/* struct thread의 'magic' 멤버에 대한 랜덤 값.
   스택 오버플로우를 감지하는 데 사용됩니다. 자세한 내용은
   thread.h의 맨 위의 큰 주석을 참조하세요. */
#define THREAD_MAGIC 0xcd6abf4b

/* Random value for basic thread
   Do not modify this value. */
/* 기본 스레드를 위한 랜덤 값
   이 값을 수정하지 마십시오. */
#define THREAD_BASIC 0xd42df210

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
/* THREAD_READY 상태의 프로세스 목록, 즉,
   실행 준비가 된 프로세스 목록. */
static struct list ready_list;

/* alarm clock을 만들기 위한 sleep_list */
static struct list sleep_list;

/* Data structure for donation */
// static struct list waiters;

/* Idle thread. */
/* 현재 실행중이지 않은 대기 상태의 쓰레드.(게으른) */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
/* 초기 스레드, init.c:main()을 실행하는 스레드*/
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
/* allocate_tid()에서 사용하는 락*/
static struct lock tid_lock;

/* Thread destruction requests */
/* 스레드 소멸 요청 */
static struct list destruction_req;

bool large_func(const struct list_elem *a, const struct list_elem *b, void *aux) {
    struct thread *data_a = list_entry(a, struct thread, elem);
    struct thread *data_b = list_entry(b, struct thread, elem);
    return data_a->priority > data_b->priority;
}

/* Statistics. */
/* 통계 */
static long long idle_ticks;    /* idle 상태에서 보낸 타이머 틱의 수, of timer ticks spent idle. */
static long long kernel_ticks;  /* 커널 스레드에서 보낸 타이머 틱의 수, of timer ticks in kernel threads. */
static long long user_ticks;    /* 사용자 프로그램에서 보낸 타이머 틱의 수, of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* 각 스레드에 할당된 타이머 틱의 수, of timer ticks to give each thread. */

static unsigned thread_ticks;   /* 마지막 yield 이후의 타이머 틱의 수, of timer ticks since last yield. */

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
/* false(기본값)인 경우 라운드 로빈 스케줄러를 사용합니다.
   true인 경우 다단계 피드백 큐 스케줄러를 사용합니다.
   커널 명령줄 옵션 "-o mlfqs"로 제어됩니다. */
bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static void do_schedule(int status);
static void schedule (void);
static tid_t allocate_tid (void);

/* Returns true if T appears to point to a valid thread. */
/* T가 유효한 스레드를 가리키는지 여부를 반환합니다. */
#define is_thread(t) ((t) != NULL && (t)->magic == THREAD_MAGIC)

/* Returns the running thread.
 * Read the CPU's stack pointer `rsp', and then round that
 * down to the start of a page.  Since `struct thread' is
 * always at the beginning of a page and the stack pointer is
 * somewhere in the middle, this locates the curent thread. */
/* 현재 실행 중인 스레드를 반환합니다.
 * CPU의 스택 포인터 'rsp'를 읽은 다음
 * 페이지의 시작으로 반올림합니다. 
 * 'struct thread'가 항상 페이지의 시작에 있고,
 * 스택 포인터는 중간에 있으므로 이것은 현재 스레드를 찾습니다. */
#define running_thread() ((struct thread *) (pg_round_down (rrsp ())))

/* Global descriptor table for the thread_start.
 Because the gdt will be setup after the thread_init, we should
 setup temporal gdt first. */
/* 스레드 시작을 위한 글로벌 디스크립터 테이블. gdt는 thread_init 이후에 설정되므로 먼저 임시 gdt를 설정해야 합니다.*/
static uint64_t gdt[3] = { 0, 0x00af9a000000ffff, 0x00cf92000000ffff };

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
   /* 현재 실행 중인 코드를 스레드로 변환하여 스레드 시스템을 초기화합니다.
   일반적으로 작동하지 않을 수 있지만, loader.S가 스택의 맨 아래를 페이지 경계에 두었으므로
   이 경우에만 작동합니다. 또한 실행 대기열과 tid 락을 초기화합니다.
   이 함수를 호출한 후에는 thread_create()로 스레드를 생성하기 전에 페이지 할당기를 초기화해야 합니다.
   이 함수가 완료될 때까지 thread_current()를 호출하는 것은 안전하지 않습니다. */
void thread_init (void) {
	ASSERT (intr_get_level () == INTR_OFF);

	/* Reload the temporal gdt for the kernel
	 * This gdt does not include the user context.
	 * The kernel will rebuild the gdt with user context, in gdt_init (). */
	/* 커널을 위한 임시 gdt 다시 로드
	 * 이 gdt에는 사용자 컨텍스트가 포함되어 있지 않습니다.
	 * 커널은 gdt_init()에서 사용자 컨텍스트와 함께 gdt를 다시 구성할 것입니다. */
	struct desc_ptr gdt_ds = {
		.size = sizeof (gdt) - 1,
		.address = (uint64_t) gdt
	};
	lgdt (&gdt_ds);

	/* Init the global thread context */
	/* 전역 스레드 컨텍스트 초기화 */
	lock_init (&tid_lock);
	
	list_init (&sleep_list);				// blocked를 사용하기 위한 sleep_list
	// list_init (&waiters);					// 우선순위 기부를 위한 대기자 리스트

	list_init (&ready_list);
	list_init (&destruction_req);


	/* Set up a thread structure for the running thread. */
	/* 실행 중인 스레드를 위한 스레드 구조체 설정 */
	initial_thread = running_thread ();
	init_thread (initial_thread, "main", PRI_DEFAULT);
	initial_thread->status = THREAD_RUNNING;
	initial_thread->tid = allocate_tid ();
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
/* 인터럽트를 활성화하여 선점형 스레드 스케줄링 시작.
   또한 idle 스레드를 생성합니다. */
void thread_start (void) {
	/* Create the idle thread. */
	/* 아이들 스레드 생성. */
	struct semaphore idle_started;
	sema_init (&idle_started, 0);
	thread_create ("idle", PRI_MIN, idle, &idle_started);

	/* Start preemptive thread scheduling. */
	/* 선점형 스레드 스케줄링 시작. */
	intr_enable ();

	/* Wait for the idle thread to initialize idle_thread. */
	/* idle 스레드가 idle_thread를 초기화할 때까지 기다립니다. */
	sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
/* 타이머 인터럽트 핸들러에서 매 타이머 틱마다 호출됩니다.
   따라서 이 함수는 외부 인터럽트 컨텍스트에서 실행됩니다. */
void thread_tick (void) {
	struct thread *t = thread_current ();

	/* Update statistics. */
	if (t == idle_thread)
		idle_ticks++;
#ifdef USERPROG
	else if (t->pml4 != NULL)
		user_ticks++;
#endif
	else
		kernel_ticks++;

/* Enforce preemption. */
/* 통계 업데이트 */
	if (++thread_ticks >= TIME_SLICE)
		intr_yield_on_return ();
}

/* Prints thread statistics. */
/* 스레드 통계를 출력합니다. */
void thread_print_stats (void) {
	printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
			idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. 
   ***************************************************************/

   /* 이름이 NAME인 새로운 커널 스레드를 생성하고, 
   주어진 초기 우선 순위와 함께 FUNCTION을 실행하여 AUX를 인수로 전달하고, 준비 큐에 추가합니다. 
   새 스레드의 ID를 반환하거나 생성에 실패한 경우 TID_ERROR를 반환합니다.

   thread_start()가 호출된 경우, 새로운 스레드가 thread_create()가 반환되기 전에 스케줄될 수 있습니다.
   스레드가 thread_create()가 반환되기 전에 종료될 수도 있습니다.
   반대로, 새로운 스레드가 스케줄되기 전에 원래 스레드가 실행될 수 있습니다.
   순서를 보장해야 하는 경우 세마포어 또는 다른 형태의 동기화를 사용하십시오.

   제공된 코드는 새 스레드의 'priority' 멤버를 PRIORITY로 설정하지만 실제 우선 순위 스케줄링은 구현되지 않았습니다.
   우선 순위 스케줄링은 1-3번 문제의 목표입니다. */
   /* 새 쓰레드를 만들고, 쓰레드 루틴 AUX를 새 쓰레드의 컨텍스트 내에서 입력 인자 priority를 가지고 실행한다.
   인자는 새롭게 만들어진 쓰레드의 기본 성질을 바꾸기 위해 사용될 수 있다.
   리턴할 때 인자 tid는 새롭게 만들어진 쓰레드의 ID를 갖는다.
   새 쓰레드는 phread_self 함수를 호출해서 자신의 쓰레드 ID를 결정할 수 있다.
   */
tid_t thread_create (const char *name, int priority,
	thread_func *function, void *aux) {
	struct thread *t;
	tid_t tid;
	struct thread *curr_t = thread_current();

	ASSERT (function != NULL);

	/* Allocate thread. */
	/* 스레드 할당 */
	t = palloc_get_page (PAL_ZERO);
	if (t == NULL)
		return TID_ERROR;

	/* Initialize thread. */
	/* 스레드 초기화 */
	init_thread (t, name, priority);
	tid = t->tid = allocate_tid ();

	/* Call the kernel_thread if it scheduled.
	 * Note) rdi is 1st argument, and rsi is 2nd argument. */
	/* kernel_thread 호출시 스케줄될 경우를 대비하여 호출합니다.
	 * 참고) rdi는 1번째 인수이며, rsi는 2번째 인수입니다. */
	t->tf.rip = (uintptr_t) kernel_thread;
	t->tf.R.rdi = (uint64_t) function;
	t->tf.R.rsi = (uint64_t) aux;
	t->tf.ds = SEL_KDSEG;
	t->tf.es = SEL_KDSEG;
	t->tf.ss = SEL_KDSEG;
	t->tf.cs = SEL_KCSEG;
	t->tf.eflags = FLAG_IF;

	/* Add to run queue. */
	/* 실행 큐에 추가 */
	thread_unblock (t);
	
	if (t->priority > curr_t->priority){
		thread_yield();
	}

	return tid;
}
// 현재 쓰레드가 새로 만들어지는 쓰레드보다 우선순위가 낮을 때 우선순위가 높은 애가 먼저 실행이 돼야함, 현재 쓰레드는 중단. yield

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. 
   **********************************************************/
   /* 현재 스레드를 sleep 상태로 전환합니다.
   다시 thread_unblock()에 의해 깨어날 때까지 다시 스케줄되지 않습니다.
   이 함수는 인터럽트가 비활성화된 상태에서 호출되어야 합니다.
   대개 synch.h의 동기화 기본 요소 중 하나를 사용하는 것이 더 좋습니다. */
void thread_block (void) {
	ASSERT (!intr_context ());
	ASSERT (intr_get_level () == INTR_OFF);
	thread_current ()->status = THREAD_BLOCKED;
	schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
   /* 블록된 스레드 T를 실행 준비 상태로 전환합니다.
   T가 블록되지 않은 경우 오류입니다. (스레드를 실행 가능하게 하려면 thread_yield()를 사용하세요.)

   이 함수는 실행 중인 스레드를 선점하지 않습니다.
   이는 호출자가 스레드를 원자적으로 unblock하고
   다른 데이터를 업데이트할 수 있다고 기대할 수 있기 때문에 중요합니다. */
void thread_unblock (struct thread *t) {
	enum intr_level old_level;

	ASSERT (is_thread (t));

	old_level = intr_disable ();
	ASSERT (t->status == THREAD_BLOCKED);
	// todo list_push_back -> list_insert_ordered 방식으로 변경(priority기준)
	// list_push_back (&ready_list, &t->elem);
	list_insert_ordered(&ready_list, &t->elem, 
						large_func, NULL);

	t->status = THREAD_READY;
	intr_set_level (old_level);
}

/* 슬립 큐에서 깨울 쓰레드를 찾아 깨우는 함수 */
void thread_wakeup(int64_t ticks){
	enum intr_level old_level;
	struct list_elem *e = list_begin(&sleep_list);

	while(e != list_end(&sleep_list)){
		struct thread *t = list_entry(e,struct thread,elem);
		//printf(" t.name = %s , t.status = %d, blocked = %d \n",t->name,t->status,THREAD_BLOCKED);
		if(ticks >= t->wakeup_tick){
			e = list_remove(e);
			thread_unblock(t);
		}
		else{
			e = list_next(e);
		}
	}
	// for (e;e != list_end(&sleep_list);e = list_next(e)){
	// 	struct thread *t = list_entry(e, struct thread, elem);
	// 	if(ticks >= t->wakeup_tick){
	// 		e = list_remove(e);
	// 		thread_unblock(t);
	// 	}
	// }
	
}

/* Returns the name of the running thread. */
/* 실행 중인 스레드의 이름을 반환합니다. */
const char * thread_name (void) {
	return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
/* 실행 중인 스레드를 반환합니다.`
   이는 running_thread()에 몇 가지 안전 검사를 추가한 것입니다.
   자세한 내용은 thread.h의 맨 위의 큰 주석을 참조하세요. */

/* 현재 쓰레드를 반환한다. */
struct thread * thread_current (void) {
	struct thread *t = running_thread ();

	/* Make sure T is really a thread.
	   If either of these assertions fire, then your thread may
	   have overflowed its stack.  Each thread has less than 4 kB
	   of stack, so a few big automatic arrays or moderate
	   recursion can cause stack overflow. */
	/* T가 실제로 스레드인지 확인합니다.
	   이러한 어서션 중 하나라도 작동하면
	   스레드가 스택을 오버플로 할 수 있습니다.
	   각 스레드는 4kB 미만의 스택을 가지므로
	   큰 자동 배열이나 중간 정도의 재귀는 스택 오버플로를 일으킬 수 있습니다. */
	ASSERT (is_thread (t));
	ASSERT (t->status == THREAD_RUNNING);

	return t;
}

/* Returns the running thread's tid. */
/* 실행 중인 스레드의 tid를 반환합니다. */
tid_t thread_tid (void) {
	return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
/* TID를 사용하여 새 스레드에 대한 기본 초기화를 수행합니다. */
void thread_exit (void) {
	ASSERT (!intr_context ());

#ifdef USERPROG
	process_exit ();
#endif

	/* Just set our status to dying and schedule another process.
	   We will be destroyed during the call to schedule_tail(). */
	/* 현재 상태를 '종료 중'으로 설정하고 다른 프로세스를 스케줄합니다.
       우리는 schedule_tail() 호출 중에 제거될 것입니다. */
	intr_disable ();
	do_schedule (THREAD_DYING);
	NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
/* CPU를 양보합니다. 현재 스레드는 sleep되지 않으며,
   스케줄러의 임의에 따라 즉시 다시 스케줄될 수 있습니다. */
void thread_yield (void) {
	struct thread *curr = thread_current (); 	// pointer
	enum intr_level old_level;

	ASSERT (!intr_context ());

	old_level = intr_disable ();				// disable interrupt
	if (curr != idle_thread)
		// list_push_back (&ready_list, &curr->elem);	// ready_list의 뒤로 넣기
		list_insert_ordered(&ready_list, &curr->elem, 
						large_func, NULL);
	do_schedule (THREAD_READY);						// change thread status and scheduling
	intr_set_level (old_level);
}

/* 쓰레드의 상태를 blocked로 바꾸고 이를 슬립 큐에 삽입한 후 대기하는 함수 */
void thread_sleep(int64_t ticks){
	/* if the current thread is not idle thread,
	change the state of the caller thread to BLOCKED,
	store the local tick to wake up,
	update the global tick if necessary,
	and call schedule()*/
	/* Change the state of the caller thread to 'blocked' and put it to the sleep queue.
	When you manupulate thread list, disable interrupt! */
	// yield 참고하기
	struct thread *curr = thread_current();
	curr->wakeup_tick = ticks;
	enum intr_level old_level;

	ASSERT(!intr_context());

	old_level = intr_disable();
	if (curr != idle_thread){
		list_push_back(&sleep_list, &curr->elem);
		// list_push_back(&waiters, &curr->elem);			// waiters for donation
	}
	// do_schedule(THREAD_BLOCKED);
	thread_block();
	intr_set_level(old_level);
}

/* Sets the current thread's priority to NEW_PRIORITY. */
/* 현재 스레드의 우선 순위를 NEW_PRIORITY로 설정합니다.
그리고 현재 실행중인 쓰레드의 우선 순위와 비교하여 양보합니다. */
void thread_set_priority (int new_priority) {
	struct thread *curr = thread_current();

	curr->priority = new_priority;	
	if( new_priority <= list_entry(list_begin(&ready_list),
								struct thread, elem)->priority){ // list_entry() 공부
		thread_yield();
	}

	/*
	curr->status = THREAD_READY;
	enum intr_level old_level;

	old_level = intr_disable();
	list_insert_ordered(&ready_list, &curr->elem, 
						large_func, NULL);
	schedule();
	intr_set_level(old_level);
	*/
}

/* Returns the current thread's priority. */
/* 현재 스레드의 우선 순위를 반환합니다. */
int thread_get_priority (void) {
	return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
/* 현재 스레드의 nice 값에 NICE를 설정합니다. */
void thread_set_nice (int nice UNUSED) {
	/* TODO: Your implementation goes here */
	/* TODO: 여기에 구현을 추가하세요 */
}

/* Returns the current thread's nice value. */
/* 현재 스레드의 nice 값을 반환합니다. */
int thread_get_nice (void) {
	/* TODO: Your implementation goes here */
	/* TODO: 여기에 구현을 추가하세요 */
	return 0;
}

/* Returns 100 times the system load average. */
/* 시스템 부하의 100배를 반환합니다. */
int thread_get_load_avg (void) {
	/* TODO: Your implementation goes here */
	/* TODO: 여기에 구현을 추가하세요 */
	return 0;
}

/* Returns 100 times the current thread's recent_cpu value. */
/* 현재 스레드의 recent_cpu 값의 100배를 반환합니다. */
int thread_get_recent_cpu (void) {
	/* TODO: Your implementation goes here */
	/* TODO: 여기에 구현을 추가하세요 */
	return 0;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */

   /* Idle 스레드. 다른 스레드가 실행 준비 상태가 아닐 때 실행됩니다.
   Idle 스레드는 초기에 thread_start()에 의해 준비 목록에 추가됩니다.
   처음에 한 번 스케줄됩니다. 그 때 idle_thread를 초기화하고,
   thread_start()가 계속되도록 전달된 세마포를 'up'합니다.
   그리고 즉시 차단됩니다.
   그 후, idle 스레드는 준비 목록에 나타나지 않습니다.
   준비 목록이 비어 있는 경우에는 특별한 경우로 idle_thread를 반환합니다. */
static void idle (void *idle_started_ UNUSED) {
	struct semaphore *idle_started = idle_started_;

	idle_thread = thread_current ();
	sema_up (idle_started);

	for (;;) {
		/* Let someone else run. */
		/* 다른 스레드를 실행합니다. */
		intr_disable ();
		thread_block ();

		/* Re-enable interrupts and wait for the next one.

		   The `sti' instruction disables interrupts until the
		   completion of the next instruction, so these two
		   instructions are executed atomically.  This atomicity is
		   important; otherwise, an interrupt could be handled
		   between re-enabling interrupts and waiting for the next
		   one to occur, wasting as much as one clock tick worth of
		   time.

		   See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
		   7.11.1 "HLT Instruction". */
		    /* 인터럽트를 다시 활성화하고 다음 인터럽트를 기다립니다.

           'sti' 명령은 다음 명령의 완료까지 인터럽트를 비활성화합니다.
           따라서 이 두 개의 명령은 원자적으로 실행됩니다. 이 원자성은
           중요합니다. 그렇지 않으면, 인터럽트가 인터럽트를 다시 활성화하고
           다음 인터럽트가 발생할 때까지 기다리는 동안 처리될 수 있으며,
           최대 한 클럭 틱의 시간이 낭비될 수 있습니다.

           [IA32-v2a] "HLT", [IA32-v2b] "STI", 및 [IA32-v3a]
           7.11.1 "HLT Instruction"을 참조하십시오. */
		asm volatile ("sti; hlt" : : : "memory");
	}
}

/* Function used as the basis for a kernel thread. */

/* 커널 스레드의 기반으로 사용되는 함수입니다. */
static void kernel_thread (thread_func *function, void *aux) {
	ASSERT (function != NULL);

	intr_enable ();       /* 스케줄러는 인터럽트가 꺼진 상태로 실행됩니다. The scheduler runs with interrupts off. */
	function (aux);       /* 스레드 함수를 실행합니다. Execute the thread function. */
	thread_exit ();       /* 만약 function()이 반환되면, 스레드를 종료합니다. If function() returns, kill the thread. */
}

/* Does basic initialization of T as a blocked thread named NAME. */
/* T를 NAME으로 블록된 스레드로 기본 초기화합니다. */
static void init_thread (struct thread *t, const char *name, int priority) {
	ASSERT (t != NULL);
	ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
	ASSERT (name != NULL);

	memset (t, 0, sizeof *t);
	t->status = THREAD_BLOCKED;
	strlcpy (t->name, name, sizeof t->name);
	t->tf.rsp = (uint64_t) t + PGSIZE - sizeof (void *);
	t->priority = priority;
	t->magic = THREAD_MAGIC;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
/* 스케줄될 다음 스레드를 선택하고 반환합니다.
   실행 준비 목록에서 스레드를 반환해야 하며, 실행 준비 목록이
   비어 있는 경우 idle_thread를 반환해야 합니다. */
static struct thread * next_thread_to_run (void) {
	if (list_empty (&ready_list))
		return idle_thread;
	else
		return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* Use iretq to launch the thread */
/* 새로운 스레드를 활성화하기 위해 iretq를 사용합니다. */
void do_iret (struct intr_frame *tf) {
	__asm __volatile(
			"movq %0, %%rsp\n"
			"movq 0(%%rsp),%%r15\n"
			"movq 8(%%rsp),%%r14\n"
			"movq 16(%%rsp),%%r13\n"
			"movq 24(%%rsp),%%r12\n"
			"movq 32(%%rsp),%%r11\n"
			"movq 40(%%rsp),%%r10\n"
			"movq 48(%%rsp),%%r9\n"
			"movq 56(%%rsp),%%r8\n"
			"movq 64(%%rsp),%%rsi\n"
			"movq 72(%%rsp),%%rdi\n"
			"movq 80(%%rsp),%%rbp\n"
			"movq 88(%%rsp),%%rdx\n"
			"movq 96(%%rsp),%%rcx\n"
			"movq 104(%%rsp),%%rbx\n"
			"movq 112(%%rsp),%%rax\n"
			"addq $120,%%rsp\n"
			"movw 8(%%rsp),%%ds\n"
			"movw (%%rsp),%%es\n"
			"addq $32, %%rsp\n"
			"iretq"
			: : "g" ((uint64_t) tf) : "memory");
}

/* Switching the thread by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function. */
/* 새 스레드의 페이지 테이블을 활성화하여 스레드를 전환합니다.
   이 함수가 호출될 때, 우리는 스레드 PREV에서 전환했으며,
   새로운 스레드가 이미 실행 중이며, 인터럽트는 여전히 비활성화됩니다.

   스레드 전환이 완료되기 전까지 printf()를 호출하는 것은 안전하지 않습니다.
   실제로 printf()는 함수의 끝에 추가해야 합니다. */
static void thread_launch (struct thread *th) {
	uint64_t tf_cur = (uint64_t) &running_thread ()->tf;
	uint64_t tf = (uint64_t) &th->tf;
	ASSERT (intr_get_level () == INTR_OFF);

	/* The main switching logic.
	 * We first restore the whole execution context into the intr_frame
	 * and then switching to the next thread by calling do_iret.
	 * Note that, we SHOULD NOT use any stack from here
	 * until switching is done. */
	/* 주요 전환 로직입니다.
     * 먼저 전체 실행 컨텍스트를 intr_frame에 복원한 다음,
     * do_iret를 호출하여 다음 스레드로 전환합니다.
     * 여기서부터는 어떤 스택도 사용해서는 안 됩니다.
     * 전환이 완료될 때까지입니다. */
	__asm __volatile (
			/* Store registers that will be used. */
			/* 사용될 레지스터를 저장합니다. */
			"push %%rax\n"
			"push %%rbx\n"
			"push %%rcx\n"
			/* Fetch input once */
			/* 입력을 한 번 획득합니다. */
			"movq %0, %%rax\n"
			"movq %1, %%rcx\n"
			"movq %%r15, 0(%%rax)\n"
			"movq %%r14, 8(%%rax)\n"
			"movq %%r13, 16(%%rax)\n"
			"movq %%r12, 24(%%rax)\n"
			"movq %%r11, 32(%%rax)\n"
			"movq %%r10, 40(%%rax)\n"
			"movq %%r9, 48(%%rax)\n"
			"movq %%r8, 56(%%rax)\n"
			"movq %%rsi, 64(%%rax)\n"
			"movq %%rdi, 72(%%rax)\n"
			"movq %%rbp, 80(%%rax)\n"
			"movq %%rdx, 88(%%rax)\n"
			"pop %%rbx\n"              // Saved rcx
			"movq %%rbx, 96(%%rax)\n"
			"pop %%rbx\n"              // Saved rbx
			"movq %%rbx, 104(%%rax)\n"
			"pop %%rbx\n"              // Saved rax
			"movq %%rbx, 112(%%rax)\n"
			"addq $120, %%rax\n"
			"movw %%es, (%%rax)\n"
			"movw %%ds, 8(%%rax)\n"
			"addq $32, %%rax\n"
			"call __next\n"         // 현재 rip를 읽습니다. read the current rip. 
			"__next:\n"
			"pop %%rbx\n"
			"addq $(out_iret -  __next), %%rbx\n"
			"movq %%rbx, 0(%%rax)\n" // rip
			"movw %%cs, 8(%%rax)\n"  // cs
			"pushfq\n"
			"popq %%rbx\n"
			"mov %%rbx, 16(%%rax)\n" // eflags
			"mov %%rsp, 24(%%rax)\n" // rsp
			"movw %%ss, 32(%%rax)\n"
			"mov %%rcx, %%rdi\n"
			"call do_iret\n"
			"out_iret:\n"
			: : "g"(tf_cur), "g" (tf) : "memory"
			);
}

/* Schedules a new process. At entry, interrupts must be off.
 * This function modify current thread's status to status and then
 * finds another thread to run and switches to it.
 * It's not safe to call printf() in the schedule(). */
/* 새 프로세스를 스케줄합니다. 진입 시, 인터럽트는 비활성화되어야 합니다.
 * 이 함수는 현재 스레드의 상태를 status로 수정한 다음,
 * 다른 스레드를 찾아서 전환합니다.
 * schedule()에서 printf()를 호출하는 것은 안전하지 않습니다. */
static void do_schedule(int status) {
	ASSERT (intr_get_level () == INTR_OFF);
	ASSERT (thread_current()->status == THREAD_RUNNING);
	
	while (!list_empty (&destruction_req)) {				// 쓰레드 청소
		struct thread *victim =
			list_entry (list_pop_front (&destruction_req), struct thread, elem);
		palloc_free_page(victim);
	}
	thread_current ()->status = status;
	schedule ();
}

/* Do context switch */
static void schedule (void) {
	struct thread *curr = running_thread ();
	struct thread *next = next_thread_to_run ();

	ASSERT (intr_get_level () == INTR_OFF);
	ASSERT (curr->status != THREAD_RUNNING);
	ASSERT (is_thread (next));

	/* Mark us as running. */
	 /* 현재 상태를 실행 중으로 표시합니다. */
	next->status = THREAD_RUNNING;

	/* Start new time slice. */
	/* 새로운 시간 슬라이스를 시작합니다. */
	thread_ticks = 0;

#ifdef USERPROG
	/* Activate the new address space. */
	/* 새로운 주소 공간을 활성화합니다. */
	process_activate (next);
#endif

	if (curr != next) {
		/* If the thread we switched from is dying, destroy its struct
		   thread. This must happen late so that thread_exit() doesn't
		   pull out the rug under itself.
		   We just queuing the page free reqeust here because the page is
		   currently used by the stack.
		   The real destruction logic will be called at the beginning of the
		   schedule(). */
		/* 전환한 스레드가 종료 중인 경우, 해당하는 struct thread를 파괴합니다.
       이는 thread_exit()가 자신의 기반을 빼가지 않도록 지연되어야 합니다.
       현재 스택에 의해 현재 사용 중인 페이지 때문에 여기서 페이지 해제 요청만
       큐잉합니다.
       실제 파괴 로직은 schedule()의 시작 부분에서 호출됩니다. */
		if (curr && curr->status == THREAD_DYING && curr != initial_thread) {
			ASSERT (curr != next); 					// curr == next라면 오류 검출
			list_push_back (&destruction_req, &curr->elem);
		}

		/* Before switching the thread, we first save the information
		 * of current running. */
		/* 스레드를 전환하기 전에 현재 실행 중인 정보를 먼저 저장합니다. */
		thread_launch (next);
	}
}

/* Returns a tid to use for a new thread. */
/* 사용할 새 스레드의 tid를 반환합니다. */
static tid_t allocate_tid (void) {
	static tid_t next_tid = 1;
	tid_t tid;

	lock_acquire (&tid_lock);
	tid = next_tid++;
	lock_release (&tid_lock);

	return tid;
}
