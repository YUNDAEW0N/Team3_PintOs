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
#include "filesys/file.h"
#include "filesys/inode.h"
#include "devices/input.h"
#include "threads/palloc.h"
#include "threads/synch.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

struct lock read_lock;
struct lock write_lock;
struct lock create_lock;




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


	lock_init(&read_lock);
	lock_init(&write_lock);
	lock_init(&create_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
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
		check_address(f->R.rdi);
		memcpy(&thread_current()->parent_if,f,sizeof(struct intr_frame));
		f->R.rax = fork(f->R.rdi);
		break;
	case SYS_EXEC:
		check_address(f->R.rdi);
		f->R.rax = exec(f->R.rdi);
		break;
	case SYS_WAIT:
		f->R.rax = wait(f->R.rdi);
		break;
	case SYS_CREATE:
		check_address(f->R.rdi);
		f->R.rax = create(f->R.rdi,f->R.rsi);
		break;
	case SYS_REMOVE:
		check_address(f->R.rdi);
		f->R.rax = remove(f->R.rdi);
		break;
	case SYS_OPEN:
		check_address(f->R.rdi);
		f->R.rax = open(f->R.rdi);
		break;
	case SYS_FILESIZE:
		f->R.rax = filesize(f->R.rdi);
		break;
	case SYS_READ:
		check_address(f->R.rsi);
		f->R.rax = read(f->R.rdi,f->R.rsi,f->R.rdx);
		break;
	case SYS_WRITE:
		check_address(f->R.rsi);
		f->R.rax = write(f->R.rdi, f->R.rsi,f->R.rdx);
		break;
	case SYS_SEEK:
		seek(f->R.rdi,f->R.rsi);
		break;
	case SYS_TELL:
		f->R.rax = tell(f->R.rdi);
		break;
	case SYS_CLOSE:
		close(f->R.rdi);
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
	struct thread *curr = thread_current();

	curr->exit_status = status;
	// if(curr->parent && curr->parent->is_wait == true){
	// 	curr->parent->is_wait = false;
	// }
	printf ("%s: exit(%d)\n",curr->name, status);
	thread_exit();
}

pid_t fork(const char *thread_name)
{
	return process_fork(thread_name,thread_current()->parent_if);
}
int exec (const char *cmd_line)
{
	char *cmd_copy;
	cmd_copy = palloc_get_page(PAL_ZERO);

	if(cmd_copy == NULL)
		return -1;
	strlcpy(cmd_copy,cmd_line,PGSIZE);
	
	if (process_exec(cmd_copy)<0){
		return -1;
	}
	NOT_REACHED();
}
int wait (pid_t pid)
{
	return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
	lock_acquire(&create_lock);
	bool ret= filesys_create(file,initial_size);
	lock_release(&create_lock);
	if(ret)
		return true;
	else
		return false;
}

bool remove (const char *file)
{
	if(filesys_remove(file))
		return true;
	else
		return false;
}


int open (const char *file)
{
	struct thread *curr = thread_current();
	curr->fdt[curr->curr_fd]= filesys_open(file);
	if (curr->fdt[curr->curr_fd])
		return curr->curr_fd++;
	else 
		return -1;
}

int filesize (int fd)
{
	if(fd<0 || fd>64)
		return;

	struct thread *curr = thread_current();
	struct file *file = curr->fdt[fd];

	if(file == NULL)
		return;

	return file_length(file);
}

int read (int fd, void *buffer, unsigned length)
{
	if(fd<0 || fd>64)
		return -1;
	if (fd == 1 || fd == 2)
		return -1;

	struct thread *curr = thread_current();
	int file_size=0;
	struct file *file = curr->fdt[fd];
	if (file == NULL)
		return -1;
	if (fd == 0)
		return input_getc();

	lock_acquire(&read_lock);
	file_size = file_read(file,buffer,length);
	lock_release(&read_lock);
	return file_size;
}

int write (int fd, const void *buffer, unsigned length)
{
	// putbuf(buffer, length);
	struct thread *curr = thread_current();
	off_t ret;

	if (fd<0 || fd>=64)
		return -1;
	if (fd == 0 || fd == 2)
		return -1;
	struct file *file = curr->fdt[fd];
	if(fd == 1)
		putbuf(buffer, length);
	
	if(file){
		lock_acquire(&write_lock);
		ret =file_write(file,buffer,length);
		lock_release(&write_lock);
		return ret;
	}
}
void seek (int fd, unsigned position)
{
	struct file *file = thread_current()->fdt[fd];
	if (fd<0 || fd>=0 || file == NULL)
		return;

	file_seek(file,position);
}
unsigned tell (int fd)
{
	struct file *file = thread_current()->fdt[fd];
	if (fd<0 || fd>=0 || file == NULL)
		return;

	return file_tell(file);
}
void close (int fd)
{
	if(fd<0 || fd>=64)
		return;

	struct thread *curr = thread_current();
	
	if(curr->fdt[fd]){
		file_close(curr->fdt[fd]);
		curr->fdt[fd] = NULL;
	}
}
int dup2(int oldfd, int newfd)
{

}