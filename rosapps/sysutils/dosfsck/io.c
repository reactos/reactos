/* io.c  -  Virtual disk input/output */

/* Written 1993 by Werner Almesberger */

/*
 * Thu Feb 26 01:15:36 CET 1998: Martin Schulze <joey@infodrom.north.de>
 *	Fixed nasty bug that caused every file with a name like
 *	xxxxxxxx.xxx to be treated as bad name that needed to be fixed.
 */

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <sys/stat.h>
//#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
//#include <linux/fd.h>


#ifdef _WIN32
#define _WIN32_WINNT	0x0400
#include <windows.h>
#include <winioctl.h>
#define __LITTLE_ENDIAN	1234
#define __BIG_ENDIAN	4321
#define __BYTE_ORDER	__LITTLE_ENDIAN
#define inline
#define __attribute__(x)
#define BLOCK_SIZE		512
#endif


#include "dosfsck.h"
#include "common.h"
#include "io.h"


typedef struct _change {
    void *data;
    loff_t pos;
    int size;
    struct _change *next;
} CHANGE;


static CHANGE *changes,*last;
static int fd,did_change = 0;

unsigned device_no;

static int WIN32open(const char *path, int oflag, ...);
#define open	WIN32open
static int WIN32close(int fd);
#define close	WIN32close
static int WIN32read(int fd, void *buf, unsigned int len);
#define read	WIN32read
static int WIN32write(int fd, void *buf, unsigned int len);
#define write	WIN32write
static loff_t WIN32llseek(int fd, loff_t offset, int whence);
#ifdef llseek
#undef llseek
#endif
#define llseek	WIN32llseek

static int is_device = 0;

void fs_open(char *path,int rw)
{
#ifdef _WIN32
  static char dev_buf[] = "\\\\.\\X:";
#else
    struct stat stbuf;
#endif

  if (path[1] == ':' && path[2] == '\0') {
	  dev_buf[4] = path[0];
	  path = dev_buf;
//	  is_device = 1;
  }

    if ((fd = open(path,rw ? O_RDWR : O_RDONLY)) < 0)
	pdie("open %s",path);
    changes = last = NULL;
    did_change = 0;

#if 0
    if (fstat(fd,&stbuf) < 0)
	pdie("fstat %s",path);
    device_no = S_ISBLK(stbuf.st_mode) ? (stbuf.st_rdev >> 8) & 0xff : 0;
#endif
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
	char* tmpBuf = malloc(readsize_aligned);                                    // TMN:
#ifdef _MSC_VER
    if (llseek(fd,seekpos_aligned,0) != seekpos_aligned) pdie("Seek to %I64d",pos);
    if ((got = read(fd,tmpBuf,readsize_aligned)) < 0) pdie("Read %d bytes at %I64dd",size,pos);
#else
     if (llseek(fd,seekpos_aligned,0) != seekpos_aligned) pdie("Seek to %lld",pos);
     if ((got = read(fd,tmpBuf,readsize_aligned)) < 0) pdie("Read %d bytes at %lld",size,pos);
#endif
	assert(got >= size);
	got = size;
	assert(seek_delta + size <= readsize);
	memcpy(data, tmpBuf+seek_delta, size);
	free(tmpBuf);
#else // TMN:
    if (llseek(fd,pos,0) != pos) pdie("Seek to %lld",pos);
    if ((got = read(fd,data,size)) < 0) pdie("Read %d bytes at %lld",size,pos);
#endif // TMN:
    if (got != size) die("Got %d bytes instead of %d at %lld",got,size,pos);
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
	const size_t seek_delta = (size_t)(pos - seekpos_aligned);          // TMN:
	const size_t readsize = (size_t)(pos - seekpos_aligned) + readsize_aligned; // TMN: 
    scratch = alloc(readsize_aligned);
    if (llseek(fd,seekpos_aligned,0) != seekpos_aligned) pdie("Seek to %lld",pos);
    okay = read(fd,scratch,readsize_aligned) == (int)readsize_aligned;
    free(scratch);
#else // TMN:
    if (llseek(fd,pos,0) != pos) pdie("Seek to %lld",pos);
    scratch = alloc(size);
    okay = read(fd,scratch,size) == size;
    free(scratch);
#endif // TMN:
    return okay;
}


void fs_write(loff_t pos,int size,void *data)
{
    CHANGE *new;
    int did;

#if 1 //SAE
    void *scratch;
    const size_t readsize_aligned = (size % 512) ? (size + (512 - (size % 512))) : size;
    const loff_t seekpos_aligned = pos - (pos % 512);
    const size_t seek_delta = (size_t)(pos - seekpos_aligned);
    const size_t readsize = (size_t)(pos - seekpos_aligned) + readsize_aligned;
    scratch = alloc(readsize_aligned);

    if (write_immed) {
	did_change = 1;
	if (llseek(fd,seekpos_aligned,0) != seekpos_aligned) pdie("Seek to %lld",pos);
	if ((did = write(fd,data,readsize_aligned)) == (int)readsize_aligned)
	{
	    free(scratch);
	    return;
	}
	if (did < 0) pdie("Write %d bytes at %lld",size,pos);
	die("Wrote %d bytes instead of %d at %lld",did,size,pos);
    }
    new = alloc(sizeof(CHANGE));
    new->pos = pos;
    memcpy(new->data = alloc(new->size = readsize_aligned),data,readsize_aligned);
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
    new = alloc(sizeof(CHANGE));
    new->pos = pos;
    memcpy(new->data = alloc(new->size = size),data,size);
    new->next = NULL;
    if (last) last->next = new;
    else changes = new;
    last = new;
#endif //SAE
}


static void fs_flush(void)
{
    CHANGE *this;
    int size;

    while (changes) {
	this = changes;
	changes = changes->next;
	if (llseek(fd,this->pos,0) != this->pos)
	    fprintf(stderr,"Seek to %lld failed: %s\n  Did not write %d bytes.\n",
	      (__int64)this->pos,strerror(errno),this->size);
	else if ((size = write(fd,this->data,this->size)) < 0)
		fprintf(stderr,"Writing %d bytes at %lld failed: %s\n",this->size,
		  (__int64)this->pos,strerror(errno));
	    else if (size != this->size)
		    fprintf(stderr,"Wrote %d bytes instead of %d bytes at %lld."
		      "\n",size,this->size,(__int64)this->pos);
	free(this->data);
	free(this);
    }
}


int fs_close(int write)
{
    CHANGE *next;
    int changed;

    changed = !!changes;
    if (write) fs_flush();
    else while (changes) {
	    next = changes->next;
	    free(changes->data);
	    free(changes);
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


#define O_SHORT_LIVED   _O_SHORT_LIVED
//#define O_ACCMODE       3
#define O_NONE          3
#define O_BACKUP        0x10000
#define O_SHARED        0x20000

static int WIN32open(const char *path, int oflag, ...)
{
	HANDLE fh;
	DWORD desiredAccess;
	DWORD shareMode;
	DWORD creationDisposition;
	DWORD flagsAttributes = FILE_ATTRIBUTE_NORMAL;
	SECURITY_ATTRIBUTES securityAttributes;
	va_list ap;
	int pmode;
	int trunc = FALSE;

	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.lpSecurityDescriptor = NULL;
	securityAttributes.bInheritHandle = oflag & O_NOINHERIT ? FALSE : TRUE;
	switch (oflag & O_ACCMODE) {
	case O_RDONLY:
		desiredAccess = GENERIC_READ;
//		shareMode = FILE_SHARE_READ;
		shareMode = FILE_SHARE_READ|FILE_SHARE_WRITE; // TMN:
		break;
	case O_WRONLY:
		desiredAccess = GENERIC_WRITE;
		shareMode = 0;
		break;
	case O_RDWR:
		desiredAccess = GENERIC_READ|GENERIC_WRITE;
		shareMode = 0;
		break;
	case O_NONE:
		desiredAccess = 0;
		shareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;
	}
	if (oflag & O_APPEND) {
		desiredAccess |= FILE_APPEND_DATA|SYNCHRONIZE;
		shareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;
	}
	if (oflag & O_SHARED)
		shareMode |= FILE_SHARE_READ|FILE_SHARE_WRITE;
        switch (oflag & (O_CREAT|O_EXCL|O_TRUNC)) {
	case 0:
	case O_EXCL:
		creationDisposition = OPEN_EXISTING;
		break;
	case O_CREAT:
		creationDisposition = OPEN_ALWAYS;
		break;
	case O_CREAT|O_EXCL:
	case O_CREAT|O_TRUNC|O_EXCL:
		creationDisposition = CREATE_NEW;
		break;
	case O_TRUNC:
	case O_TRUNC|O_EXCL:
		creationDisposition = TRUNCATE_EXISTING;
		break;
	case O_CREAT|O_TRUNC:
		creationDisposition = OPEN_ALWAYS;
		trunc = TRUE;
		break;
        }
	if (oflag & O_CREAT) {
		va_start(ap, oflag);
		pmode = va_arg(ap, int);
		va_end(ap);
		if ((pmode & 0222) == 0)
			flagsAttributes |= FILE_ATTRIBUTE_READONLY;
	}
	if (oflag & O_TEMPORARY) {
		flagsAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
		desiredAccess |= DELETE;
	}
	if (oflag & O_SHORT_LIVED)
		flagsAttributes |= FILE_ATTRIBUTE_TEMPORARY;
	if (oflag & O_SEQUENTIAL)
		flagsAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
	else if (oflag & O_RANDOM)
		flagsAttributes |= FILE_FLAG_RANDOM_ACCESS;
	if (oflag & O_BACKUP)
		flagsAttributes |= FILE_FLAG_BACKUP_SEMANTICS;
	if ((fh = CreateFile(path, desiredAccess, shareMode, &securityAttributes,
				creationDisposition, flagsAttributes, NULL)) == INVALID_HANDLE_VALUE) {
		errno = GetLastError();
		return -1;
	}
	if (trunc) {
		if (!SetEndOfFile(fh)) {
			errno = GetLastError();
			CloseHandle(fh);
			DeleteFile(path);
			return -1;
		}
	}
	return (int)fh;
}

static int WIN32close(int fd)
{
	if (!CloseHandle((HANDLE)fd)) {
		errno = GetLastError();
		return -1;
	}
	return 0;
}

static int WIN32read(int fd, void *buf, unsigned int len)
{
	DWORD actualLen;

	if (!ReadFile((HANDLE)fd, buf, (DWORD)len, &actualLen, NULL)) {
		errno = GetLastError();
		if (errno == ERROR_BROKEN_PIPE)
			return 0;
		else
			return -1;
	}
	return (int)actualLen;
}

static int WIN32write(int fd, void *buf, unsigned int len)
{
	DWORD actualLen;

	if (!WriteFile((HANDLE)fd, buf, (DWORD)len, &actualLen, NULL)) {
		errno = GetLastError();
		return -1;
	}
	return (int)actualLen;
}

static loff_t WIN32llseek(int fd, loff_t offset, int whence)
{
	long lo, hi;
	DWORD err;

	lo = (long)(offset & 0xffffffff);
	hi = (long)(offset >> 32);
	lo = SetFilePointer((HANDLE)fd, lo, &hi, whence);
	if (lo == 0xFFFFFFFF && (err = GetLastError()) != NO_ERROR) {
		errno = err;
		return -1;
	}
	return ((loff_t)hi << 32) | (__u32)lo;
}

int fsctl(int fd, int code)
{
	DWORD ret;
	if (!DeviceIoControl((HANDLE)fd, code, NULL, 0, NULL, 0, &ret, NULL)) {
		errno = GetLastError();
		return -1;
	}
	return 0; 
}
