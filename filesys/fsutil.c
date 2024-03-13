#include "filesys/fsutil.h"
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/disk.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"

/* List files in the root directory. */
void
fsutil_ls (char **argv UNUSED) {
	struct dir *dir;
	char name[NAME_MAX + 1];

	printf ("Files in the root directory:\n");
	dir = dir_open_root ();
	if (dir == NULL)
		PANIC ("root dir open failed");
	while (dir_readdir (dir, name))
		printf ("%s\n", name);
	printf ("End of listing.\n");
}

/* Prints the contents of file ARGV[1] to the system console as
 * hex and ASCII. */
void
fsutil_cat (char **argv) {
	const char *file_name = argv[1];

	struct file *file;
	char *buffer;

	printf ("Printing '%s' to the console...\n", file_name);
	file = filesys_open (file_name);
	if (file == NULL)
		PANIC ("%s: open failed", file_name);
	buffer = palloc_get_page (PAL_ASSERT);
	for (;;) {
		off_t pos = file_tell (file);
		off_t n = file_read (file, buffer, PGSIZE);
		if (n == 0)
			break;

		hex_dump (pos, buffer, n, true); 
	}
	palloc_free_page (buffer);
	file_close (file);
}

/* Deletes file ARGV[1]. */
void
fsutil_rm (char **argv) {
	const char *file_name = argv[1];

	printf ("Deleting '%s'...\n", file_name);
	if (!filesys_remove (file_name))
		PANIC ("%s: delete failed\n", file_name);
}

/* Copies from the "scratch" disk, hdc or hd1:0 to file ARGV[1]
 * in the file system.
 *
 * The current sector on the scratch disk must begin with the
 * string "PUT\0" followed by a 32-bit little-endian integer
 * indicating the file size in bytes.  Subsequent sectors hold
 * the file content.
 *
 * The first call to this function will read starting at the
 * beginning of the scratch disk.  Later calls advance across the
 * disk.  This disk position is independent of that used for
 * fsutil_get(), so all `put's should precede all `get's. */

/* "scratch" 디스크인 hdc 또는 hd1:0에서 파일 ARGV[1]로 파일 시스템에 복사합니다.
스크래치 디스크의 현재 섹터는 "PUT\0" 문자열로 시작해야 하며,
그 뒤에는 파일 크기를 바이트 단위로 나타내는 32비트 리틀 엔디안 정수가 있어야 합니다.
그 다음 섹터에는 파일 내용이 저장됩니다.
이 함수의 첫 호출은 스크래치 디스크의 시작부터 읽어들입니다.
이후 호출은 디스크를 따라 이동합니다.
이 위치는 fsutil_get()에서 사용되는 위치와 독립적이므로 모든 'put'이 모든 'get'보다 앞에 위치해야 합니다. */
void
fsutil_put (char **argv) {
	static disk_sector_t sector = 0;

	const char *file_name = argv[1];
	struct disk *src;
	struct file *dst;
	off_t size;
	void *buffer;

	printf ("Putting '%s' into the file system...\n", file_name);

	/* Allocate buffer. */
	buffer = malloc (DISK_SECTOR_SIZE);
	if (buffer == NULL)
		PANIC ("couldn't allocate buffer");

	/* Open source disk and read file size. */
	src = disk_get (1, 0);
	if (src == NULL)
		PANIC ("couldn't open source disk (hdc or hd1:0)");

	/* Read file size. */
	disk_read (src, sector++, buffer);
	if (memcmp (buffer, "PUT", 4))
		PANIC ("%s: missing PUT signature on scratch disk", file_name);
	size = ((int32_t *) buffer)[1];
	if (size < 0)
		PANIC ("%s: invalid file size %d", file_name, size);

	/* Create destination file. */
	if (!filesys_create (file_name, size))
		PANIC ("%s: create failed", file_name);
	dst = filesys_open (file_name);
	if (dst == NULL)
		PANIC ("%s: open failed", file_name);

	/* Do copy. */
	while (size > 0) {
		int chunk_size = size > DISK_SECTOR_SIZE ? DISK_SECTOR_SIZE : size;
		disk_read (src, sector++, buffer);
		if (file_write (dst, buffer, chunk_size) != chunk_size)
			PANIC ("%s: write failed with %"PROTd" bytes unwritten",
					file_name, size);
		size -= chunk_size;
	}

	/* Finish up. */
	file_close (dst);
	free (buffer);
}

/* Copies file FILE_NAME from the file system to the scratch disk.
 *
 * The current sector on the scratch disk will receive "GET\0"
 * followed by the file's size in bytes as a 32-bit,
 * little-endian integer.  Subsequent sectors receive the file's
 * data.
 *
 * The first call to this function will write starting at the
 * beginning of the scratch disk.  Later calls advance across the
 * disk.  This disk position is independent of that used for
 * fsutil_put(), so all `put's should precede all `get's. */

/* 파일 시스템에서 파일 FILE_NAME을 스크래치 디스크로 복사합니다.
스크래치 디스크의 현재 섹터는 "GET\0"와 파일의 크기(32비트, 리틀 엔디안 형식)가 먼저 저장됩니다.
이후 섹터에는 파일의 데이터가 저장됩니다.
이 함수의 첫 호출은 스크래치 디스크의 시작부터 작성합니다.
이후 호출은 디스크를 따라 이동합니다.
이 위치는 fsutil_put()에서 사용되는 위치와 독립적이므로 모든 'put'은 모든 'get'보다 앞에 위치해야 합니다. */
void
fsutil_get (char **argv) {
	static disk_sector_t sector = 0;

	const char *file_name = argv[1];
	void *buffer;
	struct file *src;
	struct disk *dst;
	off_t size;

	printf ("Getting '%s' from the file system...\n", file_name);

	/* Allocate buffer. */
	buffer = malloc (DISK_SECTOR_SIZE);
	if (buffer == NULL)
		PANIC ("couldn't allocate buffer");

	/* Open source file. */
	src = filesys_open (file_name);
	if (src == NULL)
		PANIC ("%s: open failed", file_name);
	size = file_length (src);

	/* Open target disk. */
	dst = disk_get (1, 0);
	if (dst == NULL)
		PANIC ("couldn't open target disk (hdc or hd1:0)");

	/* Write size to sector 0. */
	memset (buffer, 0, DISK_SECTOR_SIZE);
	memcpy (buffer, "GET", 4);
	((int32_t *) buffer)[1] = size;
	disk_write (dst, sector++, buffer);

	/* Do copy. */
	while (size > 0) {
		int chunk_size = size > DISK_SECTOR_SIZE ? DISK_SECTOR_SIZE : size;
		if (sector >= disk_size (dst))
			PANIC ("%s: out of space on scratch disk", file_name);
		if (file_read (src, buffer, chunk_size) != chunk_size)
			PANIC ("%s: read failed with %"PROTd" bytes unread", file_name, size);
		memset (buffer + chunk_size, 0, DISK_SECTOR_SIZE - chunk_size);
		disk_write (dst, sector++, buffer);
		size -= chunk_size;
	}

	/* Finish up. */
	file_close (src);
	free (buffer);
}
