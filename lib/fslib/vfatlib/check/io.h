/* io.h  -  Virtual disk input/output */

/* Written 1993 by Werner Almesberger */

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */


#ifndef _IO_H
#define _IO_H

//#include <sys/types.h> /* for loff_t */
#include "dosfsck.h"

/* In earlier versions, an own llseek() was used, but glibc lseek() is
 * sufficient (or even better :) for 64 bit offsets in the meantime */
#define llseek lseek

void fs_open(PUNICODE_STRING DriveRoot,int rw);

/* Opens the file system PATH. If RW is zero, the file system is opened
   read-only, otherwise, it is opened read-write. */

BOOLEAN fs_isdirty();

/* Checks if filesystem is dirty */

void fs_read(loff_t pos,int size,void *data);

/* Reads SIZE bytes starting at POS into DATA. Performs all applicable
   changes. */

int fs_test(loff_t pos,int size);

/* Returns a non-zero integer if SIZE bytes starting at POS can be read without
   errors. Otherwise, it returns zero. */

void fs_write(loff_t pos,int size,void *data);

/* If write_immed is non-zero, SIZE bytes are written from DATA to the disk,
   starting at POS. If write_immed is zero, the change is added to a list in
   memory. */

int fs_close(int write);

/* Closes the file system, performs all pending changes if WRITE is non-zero
   and removes the list of changes. Returns a non-zero integer if the file
   system has been changed since the last fs_open, zero otherwise. */

int fs_changed(void);

/* Determines whether the file system has changed. See fs_close. */

NTSTATUS fs_lock(BOOLEAN LockVolume);

/* Lock or unlocks the volume */

void fs_dismount();

/* Dismounts the volume */

extern unsigned device_no;

/* Major number of device (0 if file) and size (in 512 byte sectors) */

#endif
