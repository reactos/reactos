/* io.c - Virtual disk input/output

   Copyright (C) 1993 Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
   Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
   Copyright (C) 2008-2014 Daniel Baumann <mail@daniel-baumann.ch>
   Copyright (C) 2015 Andreas Bombe <aeb@debian.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.

   The complete text of the GNU General Public License
   can be found in /usr/share/common-licenses/GPL-3 file.
*/

/*
 * Thu Feb 26 01:15:36 CET 1998: Martin Schulze <joey@infodrom.north.de>
 *	Fixed nasty bug that caused every file with a name like
 *	xxxxxxxx.xxx to be treated as bad name that needed to be fixed.
 */

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */

#include "vfatlib.h"

#define NDEBUG
#include <debug.h>


#define FSCTL_IS_VOLUME_DIRTY   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _change {
    void *data;
    off_t pos;
    int size;
    struct _change *next;
} CHANGE;

static CHANGE *changes, *last;
#ifndef __REACTOS__
static int fd, did_change = 0;
#else
static int did_change = 0;
static HANDLE fd;
static LARGE_INTEGER CurrentOffset;


/**** Win32 / NT support ******************************************************/

static int WIN32close(HANDLE FileHandle)
{
    if (!NT_SUCCESS(NtClose(FileHandle)))
        return -1;
    return 0;
}
#define close	WIN32close

static int WIN32read(HANDLE FileHandle, void *buf, unsigned int len)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        buf,
                        len,
                        &CurrentOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtReadFile() failed (Status %lx)\n", Status);
        return -1;
    }

    CurrentOffset.QuadPart += len;
    return (int)len;
}
#define read	WIN32read

static int WIN32write(HANDLE FileHandle, void *buf, unsigned int len)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         buf,
                         len,
                         &CurrentOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
        return -1;
    }

    CurrentOffset.QuadPart += len;
    return (int)len;
}
#define write	WIN32write

static off_t WIN32lseek(HANDLE fd, off_t offset, int whence)
{
    LARGE_INTEGER Offset;
    Offset.QuadPart = (LONGLONG)offset;

    switch (whence)
    {
        case SEEK_SET:
            break;

        case SEEK_CUR:
            Offset.QuadPart += CurrentOffset.QuadPart;
            break;

        // case SEEK_END:
            // Offset.QuadPart += FileSize.QuadPart;
            // break;

        default:
            // errno = EINVAL;
            return (off_t)-1;
    }

    if (Offset.QuadPart < 0LL)
    {
        // errno = EINVAL;
        return (off_t)-1;
    }
    // if (Offset.QuadPart > FileSize.QuadPart)
    // {
        // // errno = EINVAL;
        // return (off_t)-1;
    // }

    CurrentOffset = Offset;

    return CurrentOffset.QuadPart;
}
#define lseek	WIN32lseek

/******************************************************************************/
#endif


#ifndef __REACTOS__
void fs_open(char *path, int rw)
{
    if ((fd = open(path, rw ? O_RDWR : O_RDONLY)) < 0) {
	perror("open");
	exit(6);
    }
    changes = last = NULL;
    did_change = 0;
}
#else
NTSTATUS fs_open(PUNICODE_STRING DriveRoot, int read_write)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;

    InitializeObjectAttributes(&ObjectAttributes,
                               DriveRoot,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&fd,
                        FILE_GENERIC_READ | (read_write ? FILE_GENERIC_WRITE : 0),
                        &ObjectAttributes,
                        &Iosb,
                        read_write ? 0 : FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed with status 0x%.08x\n", Status);
        return Status;
    }

    // If read_write is specified, then the volume should be exclusively locked
    if (read_write)
    {
        Status = fs_lock(TRUE);
    }

    // Query geometry and partition info, to have bytes per sector, etc

    CurrentOffset.QuadPart = 0LL;

    changes = last = NULL;
    did_change = 0;

    return Status;
}

BOOLEAN fs_isdirty(void)
{
    NTSTATUS Status;
    ULONG DirtyMask = 0;
    IO_STATUS_BLOCK IoSb;

    /* Check if volume is dirty */
    Status = NtFsControlFile(fd,
                             NULL, NULL, NULL, &IoSb,
                             FSCTL_IS_VOLUME_DIRTY,
                             NULL, 0, &DirtyMask, sizeof(DirtyMask));

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFsControlFile() failed with Status 0x%08x\n", Status);
        return FALSE;
    }

    /* Convert Dirty mask to a boolean value */
    return (DirtyMask & 1);
}

NTSTATUS fs_lock(BOOLEAN LockVolume)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoSb;

    /* Check if volume is dirty */
    Status = NtFsControlFile(fd,
                             NULL, NULL, NULL, &IoSb,
                             LockVolume ? FSCTL_LOCK_VOLUME
                                        : FSCTL_UNLOCK_VOLUME,
                             NULL, 0, NULL, 0);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFsControlFile() failed with Status 0x%08x\n", Status);
    }

    return Status;
}

void fs_dismount(void)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoSb;

    /* Check if volume is dirty */
    Status = NtFsControlFile(fd,
                             NULL, NULL, NULL, &IoSb,
                             FSCTL_DISMOUNT_VOLUME,
                             NULL, 0, NULL, 0);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFsControlFile() failed with Status 0x%08x\n", Status);
    }
}
#endif

/**
 * Read data from the partition, accounting for any pending updates that are
 * queued for writing.
 *
 * @param[in]   pos     Byte offset, relative to the beginning of the partition,
 *                      at which to read
 * @param[in]   size    Number of bytes to read
 * @param[out]  data    Where to put the data read
 */
void fs_read(off_t pos, int size, void *data)
{
    CHANGE *walk;
    int got;

#ifdef __REACTOS__
	const size_t readsize_aligned = (size % 512) ? (size + (512 - (size % 512))) : size;
 	const off_t seekpos_aligned = pos - (pos % 512);
 	const size_t seek_delta = (size_t)(pos - seekpos_aligned);
#if DBG
	const size_t readsize = (size_t)(pos - seekpos_aligned) + readsize_aligned;
#endif
	char* tmpBuf = alloc(readsize_aligned);
    if (lseek(fd, seekpos_aligned, 0) != seekpos_aligned) pdie("Seek to %lld",pos);
    if ((got = read(fd, tmpBuf, readsize_aligned)) < 0) pdie("Read %d bytes at %lld",size,pos);
	assert(got >= size);
	got = size;
	assert(seek_delta + size <= readsize);
	memcpy(data, tmpBuf+seek_delta, size);
	free(tmpBuf);
#else
    if (lseek(fd, pos, 0) != pos)
	pdie("Seek to %lld", (long long)pos);
    if ((got = read(fd, data, size)) < 0)
	pdie("Read %d bytes at %lld", size, (long long)pos);
#endif
    if (got != size)
	die("Got %d bytes instead of %d at %lld", got, size, (long long)pos);
    for (walk = changes; walk; walk = walk->next) {
	if (walk->pos < pos + size && walk->pos + walk->size > pos) {
	    if (walk->pos < pos)
		memcpy(data, (char *)walk->data + pos - walk->pos,
		       min(size, walk->size - pos + walk->pos));
	    else
		memcpy((char *)data + walk->pos - pos, walk->data,
		       min(walk->size, size + pos - walk->pos));
	}
    }
}

int fs_test(off_t pos, int size)
{
    void *scratch;
    int okay;

#ifdef __REACTOS__
	const size_t readsize_aligned = (size % 512) ? (size + (512 - (size % 512))) : size;        // TMN:
	const off_t seekpos_aligned = pos - (pos % 512);                   // TMN:
    scratch = alloc(readsize_aligned);
    if (lseek(fd, seekpos_aligned, 0) != seekpos_aligned) pdie("Seek to %lld",pos);
    okay = read(fd, scratch, readsize_aligned) == (int)readsize_aligned;
    free(scratch);
#else
    if (lseek(fd, pos, 0) != pos)
	pdie("Seek to %lld", (long long)pos);
    scratch = alloc(size);
    okay = read(fd, scratch, size) == size;
    free(scratch);
#endif
    return okay;
}

void fs_write(off_t pos, int size, void *data)
{
    CHANGE *new;
    int did;

#ifdef __REACTOS__
    assert(interactive || rw);

    if (FsCheckFlags & FSCHECK_IMMEDIATE_WRITE) {
        void *scratch;
        const size_t readsize_aligned = (size % 512) ? (size + (512 - (size % 512))) : size;
        const off_t seekpos_aligned = pos - (pos % 512);
        const size_t seek_delta = (size_t)(pos - seekpos_aligned);
        BOOLEAN use_read = (seek_delta != 0) || ((readsize_aligned-size) != 0);

        /* Aloc temp buffer if write is not aligned */
        if (use_read)
            scratch = alloc(readsize_aligned);
        else
            scratch = data;

        did_change = 1;
        if (lseek(fd, seekpos_aligned, 0) != seekpos_aligned) pdie("Seek to %lld",seekpos_aligned);

        if (use_read)
        {
            /* Read aligned data */
            if (read(fd, scratch, readsize_aligned) < 0) pdie("Read %d bytes at %lld",size,pos);

            /* Patch data in memory */
            memcpy((char *)scratch + seek_delta, data, size);
        }

        /* Write it back */
        if ((did = write(fd, scratch, readsize_aligned)) == (int)readsize_aligned)
        {
            if (use_read) free(scratch);
            return;
        }
        if (did < 0) pdie("Write %d bytes at %lld", size, pos);
        die("Wrote %d bytes instead of %d at %lld", did, size, pos);
    }
#else
    if (write_immed) {
	did_change = 1;
	if (lseek(fd, pos, 0) != pos)
	    pdie("Seek to %lld", (long long)pos);
	if ((did = write(fd, data, size)) == size)
	    return;
	if (did < 0)
	    pdie("Write %d bytes at %lld", size, (long long)pos);
	die("Wrote %d bytes instead of %d at %lld", did, size, (long long)pos);
    }
#endif
    new = alloc(sizeof(CHANGE));
    new->pos = pos;
    memcpy(new->data = alloc(new->size = size), data, size);
    new->next = NULL;
    if (last)
	last->next = new;
    else
	changes = new;
    last = new;
}

static void fs_flush(void)
{
#ifdef __REACTOS__

    CHANGE *this;
    int old_write_immed = (FsCheckFlags & FSCHECK_IMMEDIATE_WRITE);

    /* Disable writes to the list now */
    FsCheckFlags |= FSCHECK_IMMEDIATE_WRITE;

    while (changes) {
	this = changes;
	changes = changes->next;

    fs_write(this->pos, this->size, this->data);

	free(this->data);
	free(this);
    }

    /* Restore values */
    if (!old_write_immed) FsCheckFlags ^= FSCHECK_IMMEDIATE_WRITE;

#else
    CHANGE *this;
    int size;

    while (changes) {
	this = changes;
	changes = changes->next;
	if (lseek(fd, this->pos, 0) != this->pos)
	    fprintf(stderr,
		    "Seek to %lld failed: %s\n  Did not write %d bytes.\n",
		    (long long)this->pos, strerror(errno), this->size);
	else if ((size = write(fd, this->data, this->size)) < 0)
	    fprintf(stderr, "Writing %d bytes at %lld failed: %s\n", this->size,
		    (long long)this->pos, strerror(errno));
	else if (size != this->size)
	    fprintf(stderr, "Wrote %d bytes instead of %d bytes at %lld."
		    "\n", size, this->size, (long long)this->pos);
	free(this->data);
	free(this);
    }
#endif
}

int fs_close(int write)
{
    CHANGE *next;
    int changed;

    changed = ! !changes;
    if (write)
	fs_flush();
    else
	while (changes) {
	    next = changes->next;
	    free(changes->data);
	    free(changes);
	    changes = next;
	}
    if (close(fd) < 0)
	pdie("closing filesystem");
    return changed || did_change;
}

int fs_changed(void)
{
    return ! !changes || did_change;
}
