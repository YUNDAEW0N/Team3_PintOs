#include <syscall.h>
#include <stdint.h>
#include "../syscall-nr.h"

__attribute__((always_inline))
static __inline int64_t syscall (uint64_t num_, uint64_t a1_, uint64_t a2_,
		uint64_t a3_, uint64_t a4_, uint64_t a5_, uint64_t a6_) {
	int64_t ret;
	register uint64_t *num asm ("rax") = (uint64_t *) num_;
	register uint64_t *a1 asm ("rdi") = (uint64_t *) a1_;
	register uint64_t *a2 asm ("rsi") = (uint64_t *) a2_;
	register uint64_t *a3 asm ("rdx") = (uint64_t *) a3_;
	register uint64_t *a4 asm ("r10") = (uint64_t *) a4_;
	register uint64_t *a5 asm ("r8") = (uint64_t *) a5_;
	register uint64_t *a6 asm ("r9") = (uint64_t *) a6_;

	__asm __volatile(
			"mov %1, %%rax\n"
			"mov %2, %%rdi\n"
			"mov %3, %%rsi\n"
			"mov %4, %%rdx\n"
			"mov %5, %%r10\n"
			"mov %6, %%r8\n"
			"mov %7, %%r9\n"
			"syscall\n"
			: "=a" (ret)
			: "g" (num), "g" (a1), "g" (a2), "g" (a3), "g" (a4), "g" (a5), "g" (a6)
			: "cc", "memory");
	return ret;
}

/* Invokes syscall NUMBER, passing no arguments, and returns the
   return value as an `int'. */

/* 인수를 전달하지 않고 시스템 콜 번호 NUMBER를 호출하고,
 반환값을 'int'로 반환합니다. */
#define syscall0(NUMBER) ( \
		syscall(((uint64_t) NUMBER), 0, 0, 0, 0, 0, 0))

/* Invokes syscall NUMBER, passing argument ARG0, and returns the
   return value as an `int'. */

/* 시스템 콜 번호 NUMBER를 호출하고,
 인수 ARG0를 전달하여 반환값을 'int'로 반환합니다. */
#define syscall1(NUMBER, ARG0) ( \
		syscall(((uint64_t) NUMBER), \
			((uint64_t) ARG0), 0, 0, 0, 0, 0))
/* Invokes syscall NUMBER, passing arguments ARG0 and ARG1, and
   returns the return value as an `int'. */

/* 인수 ARG0 및 ARG1을 전달하여 시스템 콜 번호 NUMBER를 호출하고,
 반환값을 'int'로 반환합니다. */
#define syscall2(NUMBER, ARG0, ARG1) ( \
		syscall(((uint64_t) NUMBER), \
			((uint64_t) ARG0), \
			((uint64_t) ARG1), \
			0, 0, 0, 0))

#define syscall3(NUMBER, ARG0, ARG1, ARG2) ( \
		syscall(((uint64_t) NUMBER), \
			((uint64_t) ARG0), \
			((uint64_t) ARG1), \
			((uint64_t) ARG2), 0, 0, 0))

#define syscall4(NUMBER, ARG0, ARG1, ARG2, ARG3) ( \
		syscall(((uint64_t *) NUMBER), \
			((uint64_t) ARG0), \
			((uint64_t) ARG1), \
			((uint64_t) ARG2), \
			((uint64_t) ARG3), 0, 0))

#define syscall5(NUMBER, ARG0, ARG1, ARG2, ARG3, ARG4) ( \
		syscall(((uint64_t) NUMBER), \
			((uint64_t) ARG0), \
			((uint64_t) ARG1), \
			((uint64_t) ARG2), \
			((uint64_t) ARG3), \
			((uint64_t) ARG4), \
			0))
void
halt (void) {
	syscall0 (SYS_HALT);
	NOT_REACHED ();
}

void
exit (int status) {
	syscall1 (SYS_EXIT, status);
	// struct thread *curr = thread_current();

	// /* Save exit status at process descriptor */
	// printf("%s: exit(%d)\n", curr->name, status);

	NOT_REACHED ();
}

pid_t
fork (const char *thread_name){
	return (pid_t) syscall1 (SYS_FORK, thread_name);
}

/*
cmd_line을 실행하는 프로그램을 실행합니다.
스레드를 생성하고 실행합니다. pintos의 exec()는 Unix의 fork()+exec()와 동등합니다.
실행될 프로그램에 전달할 인수를 전달합니다.
새로운 자식 프로세스의 pid를 반환합니다.
프로그램을 로드하거나 프로세스를 생성하는 데 실패하면 -1을 반환합니다.
exec을 호출하는 부모 프로세스는 자식 프로세스가 생성되고 실행 파일이 완전히 로드될 때까지 기다려야 합니다.
*/
int
 exec (const char *file) {
	// char* save_ptr;
	// char* file_name = strtok_r(file, " ", &save_ptr);
	
	// ASSERT(file_name != NULL);
	// if(file_name == NULL){
	// 	return -1;
	// }

	return (pid_t) syscall1 (SYS_EXEC, file);		// ,file_name
}

/* 
Wait for a child process pid to exit and retrieve the childs's exit status
If pid is alive, wait till it terminates, Returns the status that pid passed to exit.
If pid did not call exit, but was terminated by kernel, return -1
A parent process can call wait for the child process that the terminated.
- return exit status of the terminated child process.
After the child terminates, the parent should deallocate its process descriptor
wait fails and return -1 if
- pid does not refer to a direct child of the calling process.
- The process that calls wait has already called wait on pid.
*/

/* 
* 자식 프로세스 pid가 종료될 때까지 기다렸다가 자식 프로세스의 종료 상태를 가져옵니다.
* 만약 pid가 살아 있다면 종료될 때까지 기다립니다. 자식 프로세스가 exit에 전달한 상태를 반환합니다.
* 만약 pid가 exit을 호출하지 않았지만 커널에 의해 종료된 경우 -1을 반환합니다.
* 부모 프로세스는 종료된 자식 프로세스를 대상으로 wait을 호출할 수 있습니다.
- 종료된 자식 프로세스의 종료 상태를 반환합니다.
* (중요)자식이 종료되면 부모는 그 프로세스의 프로세스 디스크립터를 해제해야 합니다.
* wait이 실패하고 -1을 반환하는 경우는 다음과 같습니다:
- pid가 호출하는 프로세스의 직접적인 자식을 가리키지 않는 경우.
- wait을 호출한 프로세스가 이미 pid에 대해 wait을 호출한 경우.
*/
int wait (pid_t pid) {
	return syscall1 (SYS_WAIT, pid);
}

bool
create (const char *file, unsigned initial_size) {
	return syscall2 (SYS_CREATE, file, initial_size);
}

bool
remove (const char *file) {
	return syscall1 (SYS_REMOVE, file);
}

int
open (const char *file) {
	return syscall1 (SYS_OPEN, file);
}

int
filesize (int fd) {
	return syscall1 (SYS_FILESIZE, fd);
}

int
read (int fd, void *buffer, unsigned size) {
	return syscall3 (SYS_READ, fd, buffer, size);
}

int
write (int fd, const void *buffer, unsigned size) {

	return syscall3 (SYS_WRITE, fd, buffer, size);
}

void
seek (int fd, unsigned position) {
	syscall2 (SYS_SEEK, fd, position);
}

unsigned
tell (int fd) {
	return syscall1 (SYS_TELL, fd);
}

void
close (int fd) {
	syscall1 (SYS_CLOSE, fd);
}

int
dup2 (int oldfd, int newfd){
	return syscall2 (SYS_DUP2, oldfd, newfd);
}

void *
mmap (void *addr, size_t length, int writable, int fd, off_t offset) {
	return (void *) syscall5 (SYS_MMAP, addr, length, writable, fd, offset);
}

void
munmap (void *addr) {
	syscall1 (SYS_MUNMAP, addr);
}

bool
chdir (const char *dir) {
	return syscall1 (SYS_CHDIR, dir);
}

bool
mkdir (const char *dir) {
	return syscall1 (SYS_MKDIR, dir);
}

bool
readdir (int fd, char name[READDIR_MAX_LEN + 1]) {
	return syscall2 (SYS_READDIR, fd, name);
}

bool
isdir (int fd) {
	return syscall1 (SYS_ISDIR, fd);
}

int
inumber (int fd) {
	return syscall1 (SYS_INUMBER, fd);
}

int
symlink (const char* target, const char* linkpath) {
	return syscall2 (SYS_SYMLINK, target, linkpath);
}

int
mount (const char *path, int chan_no, int dev_no) {
	return syscall3 (SYS_MOUNT, path, chan_no, dev_no);
}

int
umount (const char *path) {
	return syscall1 (SYS_UMOUNT, path);
}
