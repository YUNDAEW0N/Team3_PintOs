#ifndef THREADS_INTERRUPT_H
#define THREADS_INTERRUPT_H

#include <stdbool.h>
#include <stdint.h>

/* Interrupts on or off? */
enum intr_level {
	INTR_OFF,             /* Interrupts disabled. */
	INTR_ON               /* Interrupts enabled. */
};

enum intr_level intr_get_level (void);
enum intr_level intr_set_level (enum intr_level);
enum intr_level intr_enable (void);
enum intr_level intr_disable (void);

/* Interrupt stack frame. */
struct gp_registers {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;				// 핀토스에서의 네번째 인수(arg)의 값
	uint64_t r9;
	uint64_t r8;
	uint64_t rsi;				// 두번째 인수(arg)의 값
	uint64_t rdi;				// 첫번째 인수(arg)의 값
	uint64_t rbp;				// Base Pointer, 스택 프레임의 시작 주소
	uint64_t rdx;				// 세번째 인수(arg)의 값
	uint64_t rcx;				// 네번째 인수(arg)의 값 // 핀토스에서는 아님
	uint64_t rbx;				// Base Register, 주로 메모리 주소를 가르킴
	uint64_t rax;				// 함수 호출 후 반환값 저장
} __attribute__((packed));

struct intr_frame {
	/* Pushed by intr_entry in intr-stubs.S.
	   These are the interrupted task's saved registers. */
	struct gp_registers R;
	uint16_t es;
	uint16_t __pad1;
	uint32_t __pad2;
	uint16_t ds;
	uint16_t __pad3;
	uint32_t __pad4;
	/* Pushed by intrNN_stub in intr-stubs.S. */
	uint64_t vec_no; /* Interrupt vector number. */
/* Sometimes pushed by the CPU,
   otherwise for consistency pushed as 0 by intrNN_stub.
   The CPU puts it just under `eip', but we move it here. */
	uint64_t error_code;		// 에러코드, 인터럽트의 발생 원인
/* Pushed by the CPU.
   These are the interrupted task's saved registers. */
	uintptr_t rip;				// Instruction pointer, 다음으로 실행된 명령어의 주소
	uint16_t cs;				// 코드 세그먼트의 선택자, 현재 실행 중인 코드 세그먼트를 식별
	uint16_t __pad5;
	uint32_t __pad6;
	uint64_t eflags;			// 다양한 상태와 제어 플래그를 저장
	uintptr_t rsp;				// 스택 포인터로, 현재 스택 상단의 주소
	uint16_t ss;				// 스택 세그먼트 선택자, 현재 사용되는 스택 세그먼트를 식별
	uint16_t __pad7;
	uint32_t __pad8;
} __attribute__((packed));

typedef void intr_handler_func (struct intr_frame *);

void intr_init (void);
void intr_register_ext (uint8_t vec, intr_handler_func *, const char *name);
void intr_register_int (uint8_t vec, int dpl, enum intr_level,
                        intr_handler_func *, const char *name);
bool intr_context (void);
void intr_yield_on_return (void);

void intr_dump_frame (const struct intr_frame *);
const char *intr_name (uint8_t vec);

#endif /* threads/interrupt.h */
