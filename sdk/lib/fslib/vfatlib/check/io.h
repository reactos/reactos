/* io.h - Virtual disk input/output

   Copyright (C) 1993 Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
   Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
   Copyright (C) 2008-2014 Daniel Baumann <mail@daniel-baumann.ch>

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

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */

#ifndef _IO_H
#define _IO_H

#ifndef __REACTOS__
#include <fcntl.h>		/* for off_t */
#endif

#ifndef __REACTOS__
void fs_open(char *path, int rw);
#else
NTSTATUS fs_open(PUNICODE_STRING DriveRoot, int read_write);
#endif

/* Opens the filesystem PATH. If RW is zero, the filesystem is opened
   read-only, otherwise, it is opened read-write. */

#ifdef __REACTOS__
BOOLEAN fs_isdirty(void);

/* Checks if filesystem is dirty */
#endif

void fs_read(off_t pos, int size, void *data);

/* Reads SIZE bytes starting at POS into DATA. Performs all applicable
   changes. */

int fs_test(off_t pos, int size);

/* Returns a non-zero integer if SIZE bytes starting at POS can be read without
   errors. Otherwise, it returns zero. */

void fs_write(off_t pos, int size, void *data);

/* If write_immed is non-zero, SIZE bytes are written from DATA to the disk,
   starting at POS. If write_immed is zero, the change is added to a list in
   memory. */

int fs_close(int write);

/* Closes the filesystem, performs all pending changes if WRITE is non-zero
   and removes the list of changes. Returns a non-zero integer if the file
   system has been changed since the last fs_open, zero otherwise. */

int fs_changed(void);

/* Determines whether the filesystem has changed. See fs_close. */

#ifdef __REACTOS__
NTSTATUS fs_lock(BOOLEAN LockVolume);

/* Lock or unlocks the volume */

void fs_dismount(void);

/* Dismounts the volume */
#endif
#endif
