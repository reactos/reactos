/* io.c  -  Virtual disk input/output */

/* Written 1993 by Werner Almesberger */

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

#define FSCTL_IS_VOLUME_DIRTY           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _change {
    void *data;
    loff_t pos;
    int size;
    struct _change *next;
} CHANGE;


static CHANGE *changes,*last;
static int did_change = 0;
static HANDLE fd;
static LARGE_INTEGER CurrentOffset;

unsigned device_no;

static int WIN32close(HANDLE fd);
#define close	WIN32close
static int WIN32read(HANDLE fd, void *buf, unsigned int len);
#define read	WIN32read
static int WIN32write(HANDLE fd, void *buf, unsigned int len);
#define write	WIN32write
static loff_t WIN32llseek(HANDLE fd, loff_t offset, int whence);
#ifdef llseek
#undef llseek
#endif
#define llseek	WIN32llseek

//static int is_device = 0;

void fs_open(PUNICODE_STRING DriveRoot,int rw)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes,
        DriveRoot,
        0,
        NULL,
        NULL);

    Status = NtOpenFile(&fd,
        FILE_GENERIC_READ | (rw ? FILE_GENERIC_WRITE : 0),
        &ObjectAttributes,
        &Iosb,
        rw ? 0 : FILE_SHARE_READ,
        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed with status 0x%.08x\n", Status);
        return;
    }

    // If rw is specified, then the volume should be exclusively locked
    if (rw) fs_lock(TRUE);

    // Query geometry and partition info, to have bytes per sector, etc

    CurrentOffset.QuadPart = 0LL;

    changes = last = NULL;
    did_change = 0;
}

BOOLEAN fs_isdirty()
{
    ULONG DirtyMask = 0;
    NTSTATUS Status;
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
                             LockVolume ? FSCTL_LOCK_VOLUME :
                                          FSCTL_UNLOCK_VOLUME,
                             NULL, 0, NULL, 0);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFsControlFile() failed with Status 0x%08x\n", Status);
    }

    return Status;
}

void fs_dismount()
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

void fs_read(loff_t pos,int size,void *data)
{
    CHANGE *walk;
    int got;
#if 1 // TMN
	const size_t readsize_aligned = (size % 512) ? (size + (512 - (size % 512))) : size;        // TMN:
 	const loff_t seekpos_aligned = pos - (pos % 512);                   // TMN:
 	const size_t seek_delta = (size_t)(pos - seekpos_aligned);          // TMN:
	const size_t readsize = (size_t)(pos - seekpos_aligned) + readsize_aligned; // TMN:
	char* tmpBuf = vfalloc(readsize_aligned);                                    // TMN:
    if (llseek(fd,seekpos_aligned,0) != seekpos_aligned) pdie("Seek to %I64d",pos);
    if ((got = read(fd,tmpBuf,readsize_aligned)) < 0) pdie("Read %d bytes at %I64d",size,pos);
	assert(got >= size);
	got = size;
	assert(seek_delta + size <= readsize);
	memcpy(data, tmpBuf+seek_delta, size);
	vffree(tmpBuf);
#else // TMN:
    if (llseek(fd,pos,0) != pos) pdie("Seek to %lld",pos);
    if ((got = read(fd,data,size)) < 0) pdie("Read %d bytes at %lld",size,pos);
#endif // TMN:
    if (got != size) die("Got %d bytes instead of %d at %I64d",got,size,pos);
    for (walk = changes; walk; walk = walk->next) {
	if (walk->pos < pos+size && walk->pos+walk->size > pos) {
	    if (walk->pos < pos)
		memcpy(data,(char *) walk->data+pos-walk->pos,min((size_t)size,
		  (size_t)(walk->size-pos+walk->pos)));
	    else memcpy((char *) data+walk->pos-pos,walk->data,min((size_t)walk->size,
		  (size_t)(size+pos-walk->pos)));
	}
    }
}


int fs_test(loff_t pos,int size)
{
    void *scratch;
    int okay;

#if 1 // TMN
	const size_t readsize_aligned = (size % 512) ? (size + (512 - (size % 512))) : size;        // TMN:
	const loff_t seekpos_aligned = pos - (pos % 512);                   // TMN:
    scratch = vfalloc(readsize_aligned);
    if (llseek(fd,seekpos_aligned,0) != seekpos_aligned) pdie("Seek to %lld",pos);
    okay = read(fd,scratch,readsize_aligned) == (int)readsize_aligned;
    vffree(scratch);
#else // TMN:
    if (llseek(fd,pos,0) != pos) pdie("Seek to %lld",pos);
    scratch = vfalloc(size);
    okay = read(fd,scratch,size) == size;
    vffree(scratch);
#endif // TMN:
    return okay;
}


void fs_write(loff_t pos,int size,void *data)
{
    CHANGE *new;
    int did;

#if 1 //SAE
    if (FsCheckFlags & FSCHECK_IMMEDIATE_WRITE) {
        void *scratch;
        const size_t readsize_aligned = (size % 512) ? (size + (512 - (size % 512))) : size;
        const loff_t seekpos_aligned = pos - (pos % 512);
        const size_t seek_delta = (size_t)(pos - seekpos_aligned);
        boolean use_read = (seek_delta != 0) || ((readsize_aligned-size) != 0);

        /* Aloc temp buffer if write is not aligned */
        if (use_read)
            scratch = vfalloc(readsize_aligned);
        else
            scratch = data;

        did_change = 1;
        if (llseek(fd,seekpos_aligned,0) != seekpos_aligned) pdie("Seek to %I64d",seekpos_aligned);

        if (use_read)
        {
            /* Read aligned data */
            if (read(fd,scratch,readsize_aligned) < 0) pdie("Read %d bytes at %I64d",size,pos);

            /* Patch data in memory */
            memcpy((char *)scratch+seek_delta, data, size);
        }

        /* Write it back */
        if ((did = write(fd,scratch,readsize_aligned)) == (int)readsize_aligned)
        {
            if (use_read) vffree(scratch);
            return;
        }
        if (did < 0) pdie("Write %d bytes at %I64d",size,pos);
        die("Wrote %d bytes instead of %d at %I64d",did,size,pos);
    }

    new = vfalloc(sizeof(CHANGE));
    new->pos = pos;
    memcpy(new->data = vfalloc(new->size = size),data,size);
    new->next = NULL;
    if (last) last->next = new;
    else changes = new;
    last = new;

#else //SAE
    if (write_immed) {
	did_change = 1;
	if (llseek(fd,pos,0) != pos) pdie("Seek to %lld",pos);
	if ((did = write(fd,data,size)) == size) return;
	if (did < 0) pdie("Write %d bytes at %lld",size,pos);
	die("Wrote %d bytes instead of %d at %lld",did,size,pos);
    }
    new = vfalloc(sizeof(CHANGE));
    new->pos = pos;
    memcpy(new->data = vfalloc(new->size = size),data,size);
    new->next = NULL;
    if (last) last->next = new;
    else changes = new;
    last = new;
#endif //SAE
}


static void fs_flush(void)
{
    CHANGE *this;
    int old_write_immed = (FsCheckFlags & FSCHECK_IMMEDIATE_WRITE);

    /* Disable writes to the list now */
    FsCheckFlags |= FSCHECK_IMMEDIATE_WRITE;

    while (changes) {
	this = changes;
	changes = changes->next;

    fs_write(this->pos, this->size, this->data);

	vffree(this->data);
	vffree(this);
    }

    /* Restore values */
    if (!old_write_immed) FsCheckFlags ^= FSCHECK_IMMEDIATE_WRITE;
}


int fs_close(int write)
{
    CHANGE *next;
    int changed;

    changed = !!changes;
    if (write) fs_flush();
    else while (changes) {
	    next = changes->next;
	    vffree(changes->data);
	    vffree(changes);
	    changes = next;
	}
    if (close(fd) < 0) pdie("closing file system");
    return changed || did_change;
}


int fs_changed(void)
{
    return !!changes || did_change;
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */

static int WIN32close(HANDLE FileHandle)
{
    if (!NT_SUCCESS(NtClose(FileHandle))) return -1;

    return 0;
}

static int WIN32read(HANDLE FileHandle, void *buf, unsigned int len)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

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

static int WIN32write(HANDLE FileHandle, void *buf, unsigned int len)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

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

static loff_t WIN32llseek(HANDLE fd, loff_t offset, int whence)
{
    CurrentOffset.QuadPart = (ULONGLONG)offset;

    return offset;
}
