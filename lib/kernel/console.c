#include <console.h>
#include <stdarg.h>
#include <stdio.h>
#include "devices/serial.h"
#include "devices/vga.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/synch.h"

static void vprintf_helper(char, void *);
static void putchar_have_lock(uint8_t c);

/* The console lock.
   Both the vga and serial layers do their own locking, so it's
   safe to call them at any time.
   But this lock is useful to prevent simultaneous printf() calls
   from mixing their output, which looks confusing. */

/* 콘솔 락.
VGA와 시리얼 레이어 모두 자체적으로 락을 처리하므로
언제든지 호출할 수 있습니다.
그러나 이 락은 printf() 호출이 서로 섞이는 것을 방지하는 데 유용합니다.
이는 혼동스럽게 보일 수 있습니다. */
static struct lock console_lock;

/* True in ordinary circumstances: we want to use the console
   lock to avoid mixing output between threads, as explained
   above.

   False in early boot before the point that locks are functional
   or the console lock has been initialized, or after a kernel
   panics.  In the former case, taking the lock would cause an
   assertion failure, which in turn would cause a panic, turning
   it into the latter case.  In the latter case, if it is a buggy
   lock_acquire() implementation that caused the panic, we'll
   likely just recurse. */

/* 일반적인 경우에는 true: 스레드 간 출력을 섞지 않기 위해 콘솔 락을 사용하려고 합니다.
위에서 설명한 것처럼.

초기 부팅 시에는 거짓: 락이 작동하기 전이거나
콘솔 락이 초기화되기 전이거나, 또는 커널이 패닉 상태인 경우입니다.
전자의 경우, 락을 잡는 것이 단언 실패를 일으키며,
이로 인해 패닉이 발생하여 후자의 경우로 전환됩니다.
후자의 경우, 패닉을 일으킨 것이 버그 있는 lock_acquire() 구현이라면
아마도 재귀적으로 진행될 것입니다. */
static bool use_console_lock;

/* It's possible, if you add enough debug output to Pintos, to
   try to recursively grab console_lock from a single thread.  As
   a real example, I added a printf() call to palloc_free().
   Here's a real backtrace that resulted:

   lock_console()
   vprintf()
   printf()             - palloc() tries to grab the lock again
   palloc_free()
   schedule_tail()      - another thread dying as we switch threads
   schedule()
   thread_yield()
   intr_handler()       - timer interrupt
   intr_set_level()
   serial_putc()
   putchar_have_lock()
   putbuf()
   sys_write()          - one process writing to the console
   syscall_handler()
   intr_handler()

   This kind of thing is very difficult to debug, so we avoid the
   problem by simulating a recursive lock with a depth
   counter. */

/* Pintos에 충분한 디버그 출력을 추가하면
동일한 스레드에서 console_lock을 재귀적으로 잡으려고 할 수 있습니다.
실제 예로, palloc_free()에 printf() 호출을 추가했습니다.
그 결과로 나온 실제 backtrace는 다음과 같습니다:

lock_console()
vprintf()
printf() - palloc()이 락을 다시 잡으려고 함
palloc_free()
schedule_tail() - 다른 스레드가 스레드 전환하는 동안 종료됨
schedule()
thread_yield()
intr_handler() - 타이머 인터럽트
intr_set_level()
serial_putc()
putchar_have_lock()
putbuf()
sys_write() - 콘솔에 쓰기 위해 하나의 프로세스가 씀
syscall_handler()
intr_handler()

이런 종류의 문제를 디버깅하는 것은 매우 어려우므로
깊이 카운터로 재귀적 락을 시뮬레이션하여 문제를 피합니다. */
static int console_lock_depth;

/* Number of characters written to console. */
static int64_t write_cnt;

/* Enable console locking. */
void console_init(void)
{
	lock_init(&console_lock);
	use_console_lock = true;
}

/* Notifies the console that a kernel panic is underway,
   which warns it to avoid trying to take the console lock from
   now on. */
void console_panic(void)
{
	use_console_lock = false;
}

/* Prints console statistics. */
void console_print_stats(void)
{
	printf("Console: %lld characters output\n", write_cnt);
}

/* Acquires the console lock. */
static void
acquire_console(void)
{
	if (!intr_context() && use_console_lock)
	{
		if (lock_held_by_current_thread(&console_lock))
			console_lock_depth++;
		else
			lock_acquire(&console_lock);
	}
}

/* Releases the console lock. */
static void
release_console(void)
{
	if (!intr_context() && use_console_lock)
	{
		if (console_lock_depth > 0)
			console_lock_depth--;
		else
			lock_release(&console_lock);
	}
}

/* Returns true if the current thread has the console lock,
   false otherwise. */
static bool
console_locked_by_current_thread(void)
{
	return (intr_context() || !use_console_lock || lock_held_by_current_thread(&console_lock));
}

/* The standard vprintf() function,
   which is like printf() but uses a va_list.
   Writes its output to both vga display and serial port. */
int vprintf(const char *format, va_list args)
{
	int char_cnt = 0;

	acquire_console();
	__vprintf(format, args, vprintf_helper, &char_cnt);
	release_console();

	return char_cnt;
}

/* Writes string S to the console, followed by a new-line
   character. */
int puts(const char *s)
{
	acquire_console();
	while (*s != '\0')
		putchar_have_lock(*s++);
	putchar_have_lock('\n');
	release_console();

	return 0;
}

/* Writes the N characters in BUFFER to the console. */
void putbuf(const char *buffer, size_t n)
{
	acquire_console();
	while (n-- > 0)
		putchar_have_lock(*buffer++);
	release_console();
}

/* Writes C to the vga display and serial port. */
int putchar(int c)
{
	acquire_console();
	putchar_have_lock(c);
	release_console();

	return c;
}

/* Helper function for vprintf(). */
static void
vprintf_helper(char c, void *char_cnt_)
{
	int *char_cnt = char_cnt_;
	(*char_cnt)++;
	putchar_have_lock(c);
}

/* Writes C to the vga display and serial port.
   The caller has already acquired the console lock if
   appropriate. */
static void
putchar_have_lock(uint8_t c)
{
	ASSERT(console_locked_by_current_thread());
	write_cnt++;
	serial_putc(c);
	vga_putc(c);
}
