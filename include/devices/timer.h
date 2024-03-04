#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>

// 기본적으로 초당 100회 작동하는 시스템 타이머
/* Number of timer interrupts per second. */
#define TIMER_FREQ 100              

void timer_init (void);
void timer_calibrate (void);

int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

void timer_sleep (int64_t ticks);           // 1.1 수정함.
void timer_msleep (int64_t milliseconds);
void timer_usleep (int64_t microseconds);
void timer_nsleep (int64_t nanoseconds);

void timer_print_stats (void);

#endif /* devices/timer.h */
