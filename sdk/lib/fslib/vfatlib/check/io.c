/****
 ** Platform-dependent file
 ****/

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
static int did_change = 0;
static HANDLE fd;
static LARGE_INTEGER CurrentOffset;

#define ROUND_DOWN(n, align) ((n) & ~((align) - 1))
#define ROUND_UP(n, align) ROUND_DOWN((n) + (align) - 1, (align))

#define IS_ALIGNED(n, align) (((n) & ((align) - 1)) == 0)


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
#if 1
        /* FIXME: ReactOS HACK for 1stage due to IopParseDevice() hack */
        if (Status == STATUS_INVALID_DEVICE_REQUEST)
        {
            Status = STATUS_ACCESS_DENIED;
        }
#endif
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

#if 1 // TMN

    void *data_aligned = NULL;
    off_t seekpos_aligned = pos;
    size_t size_aligned = (size_t)size;

    /* If the output buffer & lengths are not aligned, align those */
    if (// !IS_ALIGNED(data, 512) ||
        !IS_ALIGNED(size, (size_t)512) || !IS_ALIGNED(pos, (off_t)512))
    {
        /*
         * Align the offset and the buffer length to sector boundaries,
         * taking into account for cross-sector regions.
         */
        seekpos_aligned = ROUND_DOWN(pos, (off_t)512);
        size_aligned = (size_t)(ROUND_UP(pos + size, (size_t)512) - seekpos_aligned);

        data_aligned = alloc(size_aligned);
        if (!data_aligned)
            pdie("Not enough memory to allocate aligned buffer %d bytes for read", size_aligned);
    }

    /* Read it */
    if (lseek(fd, seekpos_aligned, 0) != seekpos_aligned)
        pdie("Seek to %lld", seekpos_aligned);
    got = read(fd, data_aligned ? data_aligned : data, size_aligned);
    if (got < 0)
        pdie("Read %d bytes at %lld", size, pos);
    assert(got >= size);
    got = size;

    /*
     * If an aligned buffer was used, copy its contents
     * back into the user buffer and free it.
     */
    if (data_aligned)
    {
        /*
         * Compute the offset between the user's buffer start and
         * its aligned value, and store it in 'seekpos_aligned'.
         */
        seekpos_aligned = pos - seekpos_aligned;

        /* Be sure the read data actually intersects the user's buffer area */
        if (size_aligned > seekpos_aligned)
        {
            size_aligned = min(size_aligned - seekpos_aligned, size);
            memcpy(data, (char*)data_aligned + seekpos_aligned, size_aligned);
        }
        else
        {
            size_aligned = 0;
        }

        free(data_aligned);
    }

#else // TMN:

    if (lseek(fd, pos, 0) != pos)
	pdie("Seek to %lld", pos);
    if ((got = read(fd, data, size)) < 0)
	pdie("Read %d bytes at %lld", size, pos);

#endif // TMN:

    if (got != size)
	die("Got %d bytes instead of %d at %lld", got, size, pos);
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

#if 1 // TMN

    /*
     * Align the offset and the buffer length to sector boundaries,
     * taking into account for cross-sector regions.
     */
    const off_t seekpos_aligned = ROUND_DOWN(pos, (off_t)512);
    const size_t size_aligned = (size_t)(ROUND_UP(pos + size, (size_t)512) - seekpos_aligned);

    scratch = alloc(size_aligned);
    if (!scratch)
        pdie("Not enough memory to allocate aligned buffer %d bytes for read", size_aligned);

    /* Read it */
    if (lseek(fd, seekpos_aligned, 0) != seekpos_aligned)
        pdie("Seek to %lld", pos);
    okay = (read(fd, scratch, size_aligned) == (int)size_aligned);

    /* Free the aligned buffer */
    free(scratch);

#else // TMN:

    if (lseek(fd, pos, 0) != pos)
	pdie("Seek to %lld", pos);
    scratch = alloc(size);
    okay = read(fd, scratch, size) == size;
    free(scratch);

#endif // TMN:
    return okay;
}


void fs_write(off_t pos, int size, void *data)
{
    CHANGE *new;
    int did;

    assert(interactive || rw);

#if 1 //SAE

    if (FsCheckFlags & FSCHECK_IMMEDIATE_WRITE)
    {
        void *data_aligned = NULL;
        off_t seekpos_aligned = pos;
        size_t size_aligned = (size_t)size;

        /* If the input buffer & lengths are not aligned, align those */
        if (// !IS_ALIGNED(data, 512) ||
            !IS_ALIGNED(size, (size_t)512) || !IS_ALIGNED(pos, (off_t)512))
        {
            /*
             * Align the offset and the buffer length to sector boundaries,
             * taking into account for cross-sector regions.
             */
            seekpos_aligned = ROUND_DOWN(pos, (off_t)512);
            size_aligned = (size_t)(ROUND_UP(pos + size, (size_t)512) - seekpos_aligned);

            data_aligned = alloc(size_aligned);
            if (!data_aligned)
                pdie("Not enough memory to allocate aligned buffer %d bytes for write", size_aligned);

            /*
             * Fetch full sectors into the aligned buffer,
             * then patch it with user data.
             */
            did_change = 1;
            if (lseek(fd, seekpos_aligned, 0) != seekpos_aligned)
                pdie("Seek to %lld", seekpos_aligned);
            if (read(fd, data_aligned, size_aligned) < 0)
                pdie("Read %d bytes at %lld", size, pos);

            memcpy((char *)data_aligned + pos - seekpos_aligned, data, size);
        }

        /* Write it back */
        did_change = 1;
        if (lseek(fd, seekpos_aligned, 0) != seekpos_aligned)
            pdie("Seek to %lld", seekpos_aligned);
        did = write(fd, data_aligned ? data_aligned : data, size_aligned);

        /* Free the aligned buffer */
        if (data_aligned)
            free(data_aligned);

        if (did == (int)size_aligned)
            return;

        if (did < 0) pdie("Write %d bytes at %lld", size, pos);
        die("Wrote %d bytes instead of %d at %lld", did, size, pos);
    }

#else //SAE

    if (write_immed) {
	did_change = 1;
	if (lseek(fd, pos, 0) != pos)
	    pdie("Seek to %lld", pos);
	if ((did = write(fd, data, size)) == size)
	    return;
	if (did < 0)
	    pdie("Write %d bytes at %lld", size, pos);
	die("Wrote %d bytes instead of %d at %lld", did, size, pos);
    }

#endif //SAE

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
#if 1

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
    {
	    // printf("Seek to %lld failed: %s\n  Did not write %d bytes.\n",
		    // (long long)this->pos, strerror(errno), this->size);
	    printf("Seek to %lld failed\n  Did not write %d bytes.\n",
		    (long long)this->pos, this->size);
    }
	else if ((size = write(fd, this->data, this->size)) < 0)
    {
	    // printf("Writing %d bytes at %lld failed: %s\n", this->size,
		    // (long long)this->pos, strerror(errno));
	    printf("Writing %d bytes at %lld failed\n",
		    this->size, (long long)this->pos);
    }
	else if (size != this->size)
	    printf("Wrote %d bytes instead of %d bytes at %lld.\n",
		    size, this->size, (long long)this->pos);
	free(this->data);
	free(this);
    }

#endif
}

int fs_close(int write)
{
    CHANGE *next;
    int changed;

    changed = !!changes;
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
    return !!changes || did_change;
}
