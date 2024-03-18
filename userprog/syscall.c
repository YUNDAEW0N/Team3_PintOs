#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "filesys/filesys.h"

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
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	// printf ("system call!\n");
	// thread_exit ();

	switch (f->R.rax)
	{
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
		exit(f->R.rdi);
		break;
	case SYS_FORK:
		//check_address(f->R.rdi);
		fork(f->R.rdi);
		break;
	case SYS_EXEC:
		/* code */
		break;
	case SYS_WAIT:
		/* code */
		break;
	case SYS_CREATE:
		check_address(f->R.rdi);
		f->R.rax = create(f->R.rdi,f->R.rsi);
		break;
	case SYS_REMOVE:
		/* code */
		break;
	case SYS_OPEN:
		check_address(f->R.rdi);
		f->R.rax = open(f->R.rdi);
		break;
	case SYS_FILESIZE:
		/* code */
		break;
	case SYS_READ:
		/* code */
		break;
	case SYS_WRITE:
		check_address(f->R.rsi);
		write(f->R.rdi, f->R.rsi,f->R.rdx);
		break;
	case SYS_SEEK:
		/* code */
		break;
	case SYS_TELL:
		/* code */
		break;
	case SYS_CLOSE:
		/* code */
		break;
	}
}

void check_address(void *addr)
{
	struct thread *cur = thread_current();
	if (is_kernel_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL)
		exit(-1);
}

void halt (void)
{
	power_off();
}
void exit (int status)
{
	printf ("%s: exit(%d)\n",thread_current()->name, status);
	thread_exit();
}
// 미완성
pid_t fork (const char *thread_name)
{
	int tid;

	tid = process_fork(thread_name);

	if (tid > 0)
		return tid;
	else if (tid < 0)
		return -1;
	else
		return 0; 
}
int exec (const char *file)
{

}
int wait (pid_t pid)
{

}

bool create (const char *file, unsigned initial_size)
{
	if(filesys_create(file,initial_size))
		return true;
	else
		return false;
}

bool remove (const char *file)
{

}

// int fd = 2;
int open (const char *file)
{
	struct thread *curr = thread_current();
	curr->fdt[curr->next_fd] = filesys_open(file);

	if (filesys_open(file)) {
		curr->next_fd++;
		return curr->next_fd;
	}
	else {
		return -1;
	}
}

int filesize (int fd)
{

}
int read (int fd, void *buffer, unsigned length)
{

}
int write (int fd, const void *buffer, unsigned length)
{
	putbuf(buffer, length);
}
void seek (int fd, unsigned position)
{

}
unsigned tell (int fd)
{

}
void close (int fd)
{
	
}
int dup2(int oldfd, int newfd)
{

}