/*
syscall_entry 레이블을 가지는 시스템 콜 진입 함수가 시작됩니다.
callee saved registers를 저장하기 위해 rbx와 r12를 임시로 저장합니다.
rsp 값을 rbx에 저장하여 사용자 모드 스택 포인터를 보존합니다.
TSS(태스크 스태킹 세그먼트 디스크립터)에서 커널 스택의 주소를 읽어와 rsp 레지스터에 로드합니다.
push 명령어를 사용하여 인터럽트 프레임의 내용을 스택에 저장합니다.
syscall_handler를 호출하고, 호출 후에 스택에서 인터럽트 프레임 내용을 복원합니다.
temp1과 temp2에 저장된 값을 사용하여 저장된 레지스터를 복원합니다.
*/

#include "threads/loader.h"

.text
.globl syscall_entry
.type syscall_entry, @function
syscall_entry:
	/* callee saved 레지스터를 저장합니다. */
	movq %rbx, temp1(%rip)
	movq %r12, temp2(%rip)     /* callee saved 레지스터 */
	movq %rsp, %rbx            /* 사용자 모드 스택 포인터를 저장합니다. */
	movabs $tss, %r12
	movq (%r12), %r12
	movq 4(%r12), %rsp         /* TSS에서 커널 스택의 주소를 읽어와서 rsp에 로드합니다. */
    /* 이제 커널 스택 내에 있습니다. */
	push $(SEL_UDSEG)      /* if->ss */
	push %rbx              /* if->rsp */
	push %r11              /* if->eflags */
	push $(SEL_UCSEG)      /* if->cs */
	push %rcx              /* if->rip */
	subq $16, %rsp         /* error_code, vec_no를 건너뛰기 위해 스택을 16만큼 뺍니다. */
	push $(SEL_UDSEG)      /* if->ds */
	push $(SEL_UDSEG)      /* if->es */
	push %rax
	movq temp1(%rip), %rbx
	push %rbx
	pushq $0
	push %rdx
	push %rbp
	push %rdi
	push %rsi
	push %r8
	push %r9
	push %r10
	pushq $0 /* skip r11 */
	movq temp2(%rip), %r12
	push %r12
	push %r13
	push %r14
	push %r15
	movq %rsp, %rdi

check_intr:
	btsq $9, %r11          /* 인터럽트가 복구되었는지 확인합니다. */
	jnb no_sti
	sti                    /* 인터럽트를 복구합니다. */
no_sti:
	movabs $syscall_handler, %r12
	call *%r12
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rsi
	popq %rdi
	popq %rbp
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
	addq $32, %rsp
	popq %rcx              /* if->rip */
	addq $8, %rsp
	popq %r11              /* if->eflags */
	popq %rsp              /* if->rsp */
	sysretq

.section .data
.globl temp1
temp1:
.quad	0
.globl temp2
temp2:
.quad	0
