#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/vaddr.h"					// 가상주소 확인용

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

/* 시스템 콜.

이전에 시스템 콜 서비스는 인터럽트 핸들러에 의해 처리되었습니다.
(예: 리눅스에서의 int 0x80). 그러나 x86-64에서는 제조업체가
시스템 호출을 요청하는 효율적인 경로를 제공합니다. 바로 syscall 명령입니다.
syscall 명령은 Model Specific Register (MSR)에서 값을 읽어들여 동작합니다.
자세한 내용은 메뉴얼을 참조하십시오. */
#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

/* The interrupt service rountine should not serve any interrupts
 * until the syscall_entry swaps the userland stack to the kernel
 * mode stack. Therefore, we masked the FLAG_FL. */

/* 인터럽트 서비스 루틴은 syscall_entry가 사용자 랜드 스택을
 * 커널 모드 스택으로 교체할 때까지 어떤 인터럽트도 처리해서는 안 됩니다.
 * 따라서 FLAG_FL을 마스크로 설정했습니다. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	
	// ASSERT(check_address());

	// ASSERT(is_user_vaddr(ptov(f->R.rsi)));
	// if (is_user_vaddr(ptov(f->R.rsi)) == false){
	// 	printf("%x\n, %x\n, %c\n", ptov(f->R.rsi), f->R.rsi +1, f->R.rsi - 1);
	// 	thread_exit();
		
	// }

	printf ("system call!\n");

	thread_exit ();
}

// bool check_address(char* addr){

// }