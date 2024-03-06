/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
   */

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
   decrement it.

   - up or "V": increment the value (and wake up one waiting
   thread, if any). */

// 전달받은 value로 semaphore를 초기화합니다.
void sema_init (struct semaphore *sema, unsigned value) {
	ASSERT (sema != NULL);

	sema->value = value;
	list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. This is
   sema_down function. */

/* 세마포어에 대한 "P" 또는 down 연산입니다. SEMA의 값이 양수가 될 때까지 기다렸다가
   원자적으로 값을 감소시킵니다.
   
   이 함수는 sleep할 수 있으므로 인터럽트 핸들러 내에서 호출해서는 안 됩니다.
   이 함수는 인터럽트가 비활성화된 상태에서 호출할 수 있지만, sleep하면 다음에 스케줄된
   스레드가 아마도 다시 인터럽트를 활성화할 것입니다. 이것이 sema_down 함수입니다. */

// 세마포어를 요청합니다. 만약 세마포어를 획득한 경우, value를 1 낮춥니다.
void sema_down (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);
	ASSERT (!intr_context ());

	old_level = intr_disable ();		// 인터럽트 제한걸기
	while (sema->value == 0) {
		//list_push_back (&sema->waiters, &thread_current ()->elem);
      
      list_insert_ordered(&sema->waiters, &thread_current()->elem, list_large_func, NULL);
		thread_block ();
	}
	sema->value--;						
	intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.
   This function may be called from an interrupt handler. */

/* 세마포어에 대한 "P" 또는 down 연산입니다. 그러나 세마포어가 이미 0이 아닌 경우에만 수행됩니다.
   세마포어가 감소되면 true를 반환하고, 그렇지 않으면 false를 반환합니다.
   
   이 함수는 인터럽트 핸들러 내에서 호출될 수 있습니다. */
bool
sema_try_down (struct semaphore *sema) {
	enum intr_level old_level;
	bool success;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level (old_level);

	return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.
   This function may be called from an interrupt handler. */

   /* 세마포어에 대한 "V" 또는 up 연산입니다. SEMA의 값을 증가시키고 대기 중인 스레드 중
   하나를 깨웁니다(있을 경우).
   이 함수는 인터럽트 핸들러 내에서 호출될 수 있습니다. */

// 세마포어를 반환하고 value를 1 올립니다.
void sema_up (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
   sema->value++;

	if (!list_empty (&sema->waiters))
   {
// 	thread_unblock (list_entry (list_pop_front (&sema->waiters),
// 				struct thread, elem));
      list_sort(&sema->waiters,list_large_func,NULL);
      struct thread *t = list_entry(list_begin(&sema->waiters),struct thread, elem);
		thread_unblock (list_entry (list_pop_front (&sema->waiters),
					      struct thread, elem)); 
      
      if(t->priority > thread_get_priority()){
         thread_yield();
      }
   }

	intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */

   /* 세마포어에 대한 자체 테스트로, 두 개의 스레드 간에 제어를 "핑-퐁"하게 만듭니다.
   무엇이 진행 중인지 확인하기 위해 printf() 호출을 삽입하세요. */
void sema_self_test (void) {
	struct semaphore sema[2];
	int i;

	printf ("Testing semaphores...");
	sema_init (&sema[0], 0);
	sema_init (&sema[1], 0);
	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up (&sema[0]);
		sema_down (&sema[1]);
	}
	printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) {
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down (&sema[0]);
		sema_up (&sema[1]);
	}
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */

/* LOCK을 초기화합니다. LOCK은 한 번에 하나의 스레드만 보유할 수 있는데,
   현재 스레드가 보유하고 있으면 해당 스레드만 사용할 수 있습니다.
   락은 초기값이 1인 세마포어의 특수화입니다. 락과 이러한 세마포어의 차이점은 두 가지입니다.
   첫째, 세마포어는 값이 1보다 큰 값을 가질 수 있지만 락은 한 번에 한
   스레드만 보유할 수 있습니다. 둘째, 세마포어에는 소유자가 없지만 락에는 소유자가
   있습니다. 즉, 하나의 스레드는 락을 동시에 획득하고 해제해야 합니다.
   이러한 제약이 부담스러울 때는 락 대신 세마포어를 사용하는 것이 좋습니다. */
// lock 데이터 구조체를 초기화합니다.
void lock_init (struct lock *lock) {
	ASSERT (lock != NULL);

	lock->holder = NULL;
	sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */

/* LOCK을 획득하여 사용 가능할 때까지 대기합니다. 현재 스레드가 이미 락을 보유하고 있지 않아야 합니다.
   이 함수는 sleep할 수 있으므로 인터럽트 핸들러 내에서 호출해서는 안 됩니다.
   이 함수는 인터럽트가 비활성화된 상태에서 호출할 수 있지만, sleep하면 다음에 스케줄된
   스레드가 아마도 다시 인터럽트를 활성화할 것입니다. */
// lock을 요청합니다.
void lock_acquire (struct lock *lock) {
	ASSERT (lock != NULL);							// lock은 유효한가?
	ASSERT (!intr_context ());						// 인터럽트 핸들러 내에서 호출되지 않았는지
	ASSERT (!lock_held_by_current_thread (lock));	// 현재 스레드가 이미 해당 락을 보유하고 있는지?
	
	sema_down (&lock->semaphore);
	lock->holder = thread_current ();
	
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.
   This function will not sleep, so it may be called within an
   interrupt handler. */

   /* LOCK을 시도하고 성공하면 true를 반환하고 실패하면 false를 반환합니다.
   현재 스레드가 락을 보유하고 있지 않아야 합니다.

   이 함수는 sleep하지 않으므로 인터럽트 핸들러 내에서 호출할 수 있습니다. */
bool lock_try_acquire (struct lock *lock) {
	bool success;

	ASSERT (lock != NULL);
	ASSERT (!lock_held_by_current_thread (lock));

	success = sema_try_down (&lock->semaphore);
	if (success)
		lock->holder = thread_current ();
	return success;
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */

/* 현재 스레드가 소유한 LOCK을 해제합니다.
   인터럽트 핸들러는 락을 획득할 수 없으므로 인터럽트 핸들러 내에서 락을 
   해제하려고 시도하는 것은 의미가 없습니다. */
// lock을 반환합니다.
void lock_release (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (lock_held_by_current_thread (lock));

	lock->holder = NULL;
	sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */

   /* 현재 스레드가 LOCK을 보유하는지 여부를 반환합니다.
   (다른 스레드가 락을 보유하고 있는지 테스트하는 것은 경쟁 상태를 초래할 수 있습니다.) */
bool lock_held_by_current_thread (const struct lock *lock) {
	ASSERT (lock != NULL);

	return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem {
	struct list_elem elem;              /* List element. */
	struct semaphore semaphore;         /* This semaphore. */
};

/* 두 개의 리스트 요소(struct list_elem)를 비교하여 우선 순위가 더 높은 스레드가 있는지 판별합니다.
 이 함수는 리스트 소파에 사용되는 비교 함수로서, 리스트의 요소를 정렬하는 데 사용될 수 있습니다.  */
bool greater_priority_cond (const struct list_elem *a, const struct list_elem *b, void *aux) 
{
   struct semaphore_elem *sema_a = list_entry(a, struct semaphore_elem,elem);
   struct semaphore_elem *sema_b = list_entry(b, struct semaphore_elem,elem);
   struct list_elem *elem_a = list_begin(&sema_a->semaphore.waiters);
   struct list_elem *elem_b = list_begin(&sema_b->semaphore.waiters);
   struct thread *thread_a = list_entry(elem_a, struct thread, elem);
   struct thread *thread_b = list_entry(elem_b, struct thread, elem);
   return thread_a->priority > thread_b->priority;
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */

/* 조건 변수 COND를 초기화합니다. 조건 변수는 한 코드 조각이 조건을 신호하고,
   협력 코드가 신호를 수신하고 이에 따라 작동할 수 있도록 합니다. */
   // condition 변수 데이터 구조체를 초기화합니다.
void cond_init (struct condition *cond) {
	ASSERT (cond != NULL);

	list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */

/* LOCK을 원자적으로 해제하고 다른 코드에서 COND가 신호를 받을 때까지 대기합니다.
   COND가 신호를 받으면 LOCK이 반환됩니다.

   이 함수로 구현된 모니터는 "Mesa" 스타일입니다. "Hoare" 스타일과 달리, 신호를 보내고
   수신하는 것은 원자적 작업이 아닙니다. 따라서 일반적으로 호출자는 대기가 완료된 후 조건을 다시
   확인하고, 필요한 경우 다시 대기해야 합니다.

   주어진 조건 변수는 하나의 락과만 연관되어 있지만 하나의 락은 여러 조건 변수와 연관될 수 있습니다.
   즉, 락에서 조건 변수로의 일대다 매핑이 있습니다.

   이 함수는 sleep할 수 있으므로 인터럽트 핸들러 내에서 호출해서는 안 됩니다.
   이 함수는 인터럽트가 비활성화된 상태에서 호출할 수 있지만, sleep하면 다음에 스케줄된
   스레드가 아마도 다시 인터럽트를 활성화할 것입니다. */
   // condition 변수에 따라 시그널을 기다립니다.
void cond_wait (struct condition *cond, struct lock *lock) {
	struct semaphore_elem waiter;

	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	sema_init (&waiter.semaphore, 0);
   list_insert_ordered(&cond->waiters,&waiter.elem,
		               greater_priority_cond,NULL); 
	// list_push_back (&cond->waiters, &waiter.elem);
	lock_release (lock);
	sema_down (&waiter.semaphore);
	lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */

/* 만약 LOCK으로 보호되는 COND에 대기 중인 스레드가 있다면,
   그 중 하나에게 깨어날 시그널을 보냅니다.
   이 함수를 호출하기 전에 LOCK이 보유되어야 합니다.

   인터럽트 핸들러는 락을 획득할 수 없으므로 인터럽트 핸들러 내에서
   조건 변수를 신호하는 것은 의미가 없습니다. */
   // 컨디션 변수 구조체에서 기다리는 가장 높은 우선순위의 쓰레드에게 signal을 보냅니다.
void cond_signal (struct condition *cond, struct lock *lock UNUSED) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	if (!list_empty (&cond->waiters)){
      // struct thread *t = list_entry(list_begin(&list_entry(list_begin(&cond->waiters),
      // struct semaphore_elem,elem)->semaphore.waiters)
      // ,struct thread,elem);
      list_sort(&cond->waiters, greater_priority_cond, NULL);
		sema_up (&list_entry (list_pop_front (&cond->waiters),
					struct semaphore_elem, elem)->semaphore);
      // if(t->priority > thread_get_priority()){
      //    thread_yield();
      // }
   }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */

/* 만약 LOCK으로 보호되는 COND에 대기 중인 스레드가 있다면,
   모든 스레드에게 깨어날 시그널을 보냅니다.
   이 함수를 호출하기 전에 LOCK이 보유되어야 합니다.

   인터럽트 핸들러는 락을 획득할 수 없으므로 인터럽트 핸들러 내에서
   조건 변수를 신호하는 것은 의미가 없습니다. */
   // condition 변수 구조체에서 기다리는 모든 쓰레드에게 시그널을 보냅니다.
void cond_broadcast (struct condition *cond, struct lock *lock) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);

	while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
}
