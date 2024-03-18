#include "filesys/file.h"
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"

/* An open file. 아래는 요소들
inode *inode, off_t pos, bool deny_write */
struct file {
	struct inode *inode;        /* File's inode. */
	off_t pos;                  /* Current position. */
	bool deny_write;            /* Has file_deny_write() been called? */
};

/* Opens a file for the given INODE, of which it takes ownership,
 * and returns the new file.  Returns a null pointer if an
 * allocation fails or if INODE is null. */

/* 주어진 INODE에 대한 파일을 열고, 소유권을 가져가고,
새 파일을 반환합니다. 할당이 실패하거나 INODE가 널인 경우
널 포인터를 반환합니다. */
struct file *file_open (struct inode *inode) {
	struct file *file = calloc (1, sizeof *file);
	if (inode != NULL && file != NULL) {
		file->inode = inode;
		file->pos = 0;
		file->deny_write = false;
		return file;
	} else {
		inode_close (inode);
		free (file);
		return NULL;
	}
}

/* Opens and returns a new file for the same inode as FILE.
 * Returns a null pointer if unsuccessful. */

/* FILE과 동일한 inode에 대한 새 파일을 열어 반환합니다.
실패한 경우 널 포인터를 반환합니다. */
struct file *
file_reopen (struct file *file) {
	return file_open (inode_reopen (file->inode));
}

/* Duplicate the file object including attributes and returns a new file for the
 * same inode as FILE. Returns a null pointer if unsuccessful. */

/* 파일의 속성을 포함하여 파일 개체를 복제하고, 
FILE과 동일한 inode에 대한 새 파일을 반환합니다.
실패한 경우 널 포인터를 반환합니다. */
struct file *
file_duplicate (struct file *file) {
	struct file *nfile = file_open (inode_reopen (file->inode));
	if (nfile) {
		nfile->pos = file->pos;
		if (file->deny_write)
			file_deny_write (nfile);
	}
	return nfile;
}

/* Closes FILE. */
void
file_close (struct file *file) {
	if (file != NULL) {
		file_allow_write (file);
		inode_close (file->inode);
		free (file);
	}
}

/* Returns the inode encapsulated by FILE. */

/* FILE에 의해 캡슐화된 inode를 반환합니다. */
struct inode *
file_get_inode (struct file *file) {
	return file->inode;
}

/* Reads SIZE bytes from FILE into BUFFER,
 * starting at the file's current position.
 * Returns the number of bytes actually read,
 * which may be less than SIZE if end of file is reached.
 * Advances FILE's position by the number of bytes read. */

/* FILE의 현재 위치에서 시작하여 BUFFER에서 SIZE 바이트를
읽어옵니다. 실제로 읽은 바이트 수를 반환하며,
파일의 끝에 도달하면 SIZE보다 작을 수 있습니다.
읽은 바이트 수만큼 FILE의 위치를 이동시킵니다. */
off_t
file_read (struct file *file, void *buffer, off_t size) {
	off_t bytes_read = inode_read_at (file->inode, buffer, size, file->pos);
	// printf("=============================================\n");
	file->pos += bytes_read;
	return bytes_read;
}

/* Reads SIZE bytes from FILE into BUFFER,
 * starting at offset FILE_OFS in the file.
 * Returns the number of bytes actually read,
 * which may be less than SIZE if end of file is reached.
 * The file's current position is unaffected. */

/* BUFFER의 SIZE 바이트를 FILE의 파일 오프셋 위치 FILE_OFS에서
시작하여 FILE에 씁니다. 실제로 쓴 바이트 수를 반환하며,
파일의 끝에 도달하면 SIZE보다 작을 수 있습니다.
(보통 이 경우 파일을 확장하지만, 파일 확장은 아직 구현되지 않았습니다.)
파일의 현재 위치에는 영향을 주지 않습니다. */
off_t
file_read_at (struct file *file, void *buffer, off_t size, off_t file_ofs) {
	return inode_read_at (file->inode, buffer, size, file_ofs);
}

/* Writes SIZE bytes from BUFFER into FILE,
 * starting at the file's current position.
 * Returns the number of bytes actually written,
 * which may be less than SIZE if end of file is reached.
 * (Normally we'd grow the file in that case, but file growth is
 * not yet implemented.)
 * Advances FILE's position by the number of bytes read. */

/* BUFFER에서 FILE로부터 현재 위치부터 SIZE 바이트를 씁니다.
실제로 쓰여진 바이트 수를 반환합니다.
파일의 끝에 도달하면 SIZE보다 적을 수 있습니다.
(보통 이 경우 파일 크기를 늘릴 것입니다만, 파일 확장은
아직 구현되지 않았습니다.)
파일의 위치를 읽은 바이트 수만큼 앞으로 이동합니다. */
off_t
file_write (struct file *file, const void *buffer, off_t size) {
	off_t bytes_written = inode_write_at (file->inode, buffer, size, file->pos);
	file->pos += bytes_written;
	return bytes_written;
}

/* Writes SIZE bytes from BUFFER into FILE,
 * starting at offset FILE_OFS in the file.
 * Returns the number of bytes actually written,
 * which may be less than SIZE if end of file is reached.
 * (Normally we'd grow the file in that case, but file growth is
 * not yet implemented.)
 * The file's current position is unaffected. */

/* BUFFER의 SIZE 바이트를 FILE의 파일 오프셋 위치 FILE_OFS에서
시작하여 FILE에 씁니다. 실제로 쓴 바이트 수를 반환하며,
파일의 끝에 도달하면 SIZE보다 작을 수 있습니다.
(보통 이 경우 파일을 확장하지만, 파일 확장은 아직 구현되지 않았습니다.)
파일의 현재 위치에는 영향을 주지 않습니다. */
off_t
file_write_at (struct file *file, const void *buffer, off_t size,
		off_t file_ofs) {
	return inode_write_at (file->inode, buffer, size, file_ofs);
}

/* Prevents write operations on FILE's underlying inode
 * until file_allow_write() is called or FILE is closed. */

/* 파일의 기본 inode에 대한 쓰기 작업을 방지합니다.
file_allow_write()가 호출되거나 FILE이 닫힐 때까지. */
void
file_deny_write (struct file *file) {
	ASSERT (file != NULL);
	if (!file->deny_write) {
		file->deny_write = true;
		inode_deny_write (file->inode);
	}
}

/* Re-enables write operations on FILE's underlying inode.
 * (Writes might still be denied by some other file that has the
 * same inode open.) */

/* 파일의 기본 inode에 대한 쓰기 작업을 다시 활성화합니다.
(동일한 inode를 열고 있는 다른 파일에 의해 쓰기가 거부될 수 있습니다.) */
void
file_allow_write (struct file *file) {
	ASSERT (file != NULL);
	if (file->deny_write) {
		file->deny_write = false;
		inode_allow_write (file->inode);
	}
}

/* Returns the size of FILE in bytes. */
off_t
file_length (struct file *file) {
	ASSERT (file != NULL);
	return inode_length (file->inode);
}

/* Sets the current position in FILE to NEW_POS bytes from the
 * start of the file. */

/* 파일의 현재 위치를 파일의 시작에서 NEW_POS 바이트로 설정합니다. */
void
file_seek (struct file *file, off_t new_pos) {
	ASSERT (file != NULL);
	ASSERT (new_pos >= 0);
	file->pos = new_pos;
}

/* Returns the current position in FILE as a byte offset from the
 * start of the file. */

/* 파일의 현재 위치를 파일의 시작부터의 바이트 오프셋으로 반환합니다. */
off_t
file_tell (struct file *file) {
	ASSERT (file != NULL);
	return file->pos;
}
