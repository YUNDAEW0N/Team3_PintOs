/* Verifies that lowering a thread's priority so that it is no
   longer the highest-priority thread in the system causes it to
   yield immediately. */

/* 스레드의 우선순위를 낮추어 시스템에서 더 이상 최우선 스레드가 되지 않도록
 한 후에 즉시 양보되는지 확인합니다. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/thread.h"

static thread_func changing_thread;

void
test_priority_change (void) 
{
  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  msg ("Creating a high-priority thread 2.");
  // printf("now Thread %d priority %d\n", thread_tid(), thread_get_priority());
  thread_create ("thread 2", PRI_DEFAULT + 1, changing_thread, NULL);
  msg ("Thread 2 should have just lowered its priority.");
  thread_set_priority (PRI_DEFAULT - 2);
  // printf("now Thread %d priority %d\n", thread_tid(), thread_get_priority());
  msg ("Thread 2 should have just exited.");
}

static void
changing_thread (void *aux UNUSED) 
{
  msg ("Thread 2 now lowering priority.");
  // printf("now Thread %d priority %d\n", thread_tid(), thread_get_priority());
  thread_set_priority (PRI_DEFAULT - 1);
  // printf("now Thread %d priority %d\n", thread_tid(), thread_get_priority());
  msg ("Thread 2 exiting.");
}

