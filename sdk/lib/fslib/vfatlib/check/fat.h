/* fat.h - Read/write access to the FAT

   Copyright (C) 1993 Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
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

   THe complete text of the GNU General Public License
   can be found in /usr/share/common-licenses/GPL-3 file.
*/

#ifndef _FAT_H
#define _FAT_H

void read_fat(DOS_FS * fs);

/* Loads the FAT of the filesystem described by FS. Initializes the FAT,
   replaces broken FATs and rejects invalid cluster entries. */

void get_fat(FAT_ENTRY * entry, void *fat, uint32_t cluster, DOS_FS * fs);

/* Retrieve the FAT entry (next chained cluster) for CLUSTER. */

void set_fat(DOS_FS * fs, uint32_t cluster, int32_t new);

/* Changes the value of the CLUSTERth cluster of the FAT of FS to NEW. Special
   values of NEW are -1 (EOF, 0xff8 or 0xfff8) and -2 (bad sector, 0xff7 or
   0xfff7) */

int bad_cluster(DOS_FS * fs, uint32_t cluster);

/* Returns a non-zero integer if the CLUSTERth cluster is marked as bad or zero
   otherwise. */

uint32_t next_cluster(DOS_FS * fs, uint32_t cluster);

/* Returns the number of the cluster following CLUSTER, or -1 if this is the
   last cluster of the respective cluster chain. CLUSTER must not be a bad
   cluster. */

off_t cluster_start(DOS_FS * fs, uint32_t cluster);

/* Returns the byte offset of CLUSTER, relative to the respective device. */

void set_owner(DOS_FS * fs, uint32_t cluster, DOS_FILE * owner);

/* Sets the owner pointer of the respective cluster to OWNER. If OWNER was NULL
   before, it can be set to NULL or any non-NULL value. Otherwise, only NULL is
   accepted as the new value. */

DOS_FILE *get_owner(DOS_FS * fs, uint32_t cluster);

/* Returns the owner of the repective cluster or NULL if the cluster has no
   owner. */

void fix_bad(DOS_FS * fs);

/* Scans the disk for currently unused bad clusters and marks them as bad. */

void reclaim_free(DOS_FS * fs);

/* Marks all allocated, but unused clusters as free. */

void reclaim_file(DOS_FS * fs);

/* Scans the FAT for chains of allocated, but unused clusters and creates files
   for them in the root directory. Also tries to fix all inconsistencies (e.g.
   loops, shared clusters, etc.) in the process. */

uint32_t update_free(DOS_FS * fs);

/* Updates free cluster count in FSINFO sector. */

#endif
