/* mountlist.h -- declarations for list of mounted filesystems
   Copyright (C) 1991, 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef __MOUNTLIST_H
#define __MOUNTLIST_H

/* A mount table entry. */
struct mount_entry
{
  char *me_devname;		/* Device node pathname, including "/dev/". */
  char *me_mountdir;		/* Mount point directory pathname. */
  char *me_type;		/* "nfs", "4.2", etc. */
  dev_t me_dev;			/* Device number of me_mountdir. */
  struct mount_entry *me_next;
};

struct mount_entry *read_filesystem_list (int need_fs_type, int all_fs);
#endif 	/* __MOUNTLIST_H	*/
