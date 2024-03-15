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
    // bool success;
    // int sysc_num;
    // int file_desc;
    // struct pid_t child_pid;
    // struct thread *t_child;
    
	// ASSERT(is_user_vaddr(f->rsp));
	// ASSERT(is_user_vaddr(f->R.rbp));

    // switch(f->R.rax){
    //     case 0:
    //         halt ();
    //         power_off();                        // 웬만하면 사용되선 안된다.
    //     case 1:
    //         exit (int stats);
    //         printf ("%s: exit(%d)\n", stats);
    //     case 2:
    //         child_pid = fork (const char *thread_name);
    //     case 3:
    //         file_desc = exec (const char *file);
    //     case 4:
    //         wait (pid_t pid);
    //     case 5:
    //         create (const char *file, unsigned initial_size);
    //     case 6:
    //         remove (const char *file);
    //     case 7:
    //         open (const char *file);
    //     case 8:
    //         filesize (int fd);
    //     case 9:
    //         read (int fd, void *buffer, unsigned size);
    //     case 10:
    //         write (int fd, const void *buffer, unsigned size);
    //     case 11:
    //         seek (int fd, unsigned position);
    //     case 12:
    //         tell (int fd);
    //     case 13:
    //         close (int fd);
        
    //     default:
    // }


    // // printf("---------------체크용----------------\n");
    // printf("%d\n", f->R.rax);
    // printf("-----------------끝------------------\n");
    
	// ASSERT(check_address());
	printf("---------------체크용----------------\n");
	// printf("%x, %x\n", f->rsp, ptov(f->rsp));
    printf("Registers:\n");
    printf("RSI: %x, %s\n", f->R.rsi, f->R.rsi);
    printf("RDI: %x\n", f->R.rdi);
    printf("RBP: 0x%x\n", f->R.rbp);
    printf("RDX: %d\n", f->R.rdx);
    printf("R10: %d\n", f->R.r10);
    printf("RBX: 0x%x\n", f->R.rbx);
    printf("RAX: %d\n", f->R.rax);

    // Print interrupt vector number
    printf("Interrupt vector number: %d\n", f->vec_no);

    // Print error code
    printf("Error code: 0x%x\n", f->error_code);

    // Print instruction pointer
    printf("Instruction pointer: 0x%x\n", f->rip);

    // Print code segment selector
    printf("Code segment selector: 0x%x\n", f->cs);

    // Print flags register
    printf("Flags register: 0x%x\n", f->eflags);

    // Print stack pointer
    printf("Stack pointer: 0x%x\n", f->rsp);

    // Print stack segment selector
    printf("Stack segment selector: 0x%x\n", f->ss);
	printf("-------------------end---------------\n");

	// if (is_user_vaddr(ptov(f->R.rsi)) == false){
	// 	printf("%x\n, %x\n, %c\n", ptov(f->R.rsi), f->R.rsi +1, f->R.rsi - 1);
	// 	thread_exit();
		
	// }
	// printf("------------syscall_handler-------------\n");
	hex_dump(f->rsp, f->rsp, USER_STACK - f->rsp, true);
	// printf("-------------------end------------------\n");
    

	printf ("system call!\n");

	thread_exit ();
}

// bool check_address(char* addr){

// }

/* syscall Debugging */
/* 
   printf("Registers:\n");
    printf("R15: 0x%x\n", f->R.r15);
    printf("R14: 0x%x\n", f->R.r14);
    printf("R13: 0x%x\n", f->R.r13);
    printf("R12: 0x%x\n", f->R.r12);
    printf("R11: 0x%x\n", f->R.r11);
    printf("R10: 0x%x\n", f->R.r10);
    printf("R9: 0x%x\n", f->R.r9);
    printf("R8: 0x%x\n", f->R.r8);
    printf("RSI: 0x%x\n", f->R.rsi);
    printf("RDI: 0x%x\n", f->R.rdi);
    printf("RBP: 0x%x\n", f->R.rbp);
    printf("RDX: 0x%x\n", f->R.rdx);
    printf("RCX: 0x%x\n", f->R.rcx);
    printf("RBX: 0x%x\n", f->R.rbx);
    printf("RAX: 0x%x\n", f->R.rax);

    // Print interrupt vector number
    printf("Interrupt vector number: %d\n", f->vec_no);

    // Print error code
    printf("Error code: 0x%x\n", f->error_code);

    // Print instruction pointer
    printf("Instruction pointer: 0x%x\n", f->rip);

    // Print code segment selector
    printf("Code segment selector: 0x%x\n", f->cs);

    // Print flags register
    printf("Flags register: 0x%x\n", f->eflags);

    // Print stack pointer
    printf("Stack pointer: 0x%x\n", f->rsp);

    // Print stack segment selector
    printf("Stack segment selector: 0x%x\n", f->ss);

*/