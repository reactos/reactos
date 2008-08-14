/* fat.h  -  Read/write access to the FAT */

/* Written 1993 by Werner Almesberger */


#ifndef _FAT_H
#define _FAT_H

void read_fat(DOS_FS *fs);

/* Loads the FAT of the file system described by FS. Initializes the FAT,
   replaces broken FATs and rejects invalid cluster entries. */

void set_fat(DOS_FS *fs,unsigned long cluster,unsigned long new);

/* Changes the value of the CLUSTERth cluster of the FAT of FS to NEW. Special
   values of NEW are -1 (EOF, 0xff8 or 0xfff8) and -2 (bad sector, 0xff7 or
   0xfff7) */

int bad_cluster(DOS_FS *fs,unsigned long cluster);

/* Returns a non-zero integer if the CLUSTERth cluster is marked as bad or zero
   otherwise. */

unsigned long next_cluster(DOS_FS *fs,unsigned long cluster);

/* Returns the number of the cluster following CLUSTER, or -1 if this is the
   last cluster of the respective cluster chain. CLUSTER must not be a bad
   cluster. */

loff_t cluster_start(DOS_FS *fs,unsigned long cluster);

/* Returns the byte offset of CLUSTER, relative to the respective device. */

void set_owner(DOS_FS *fs,unsigned long cluster,DOS_FILE *owner);

/* Sets the owner pointer of the respective cluster to OWNER. If OWNER was NULL
   before, it can be set to NULL or any non-NULL value. Otherwise, only NULL is
   accepted as the new value. */

DOS_FILE *get_owner(DOS_FS *fs,unsigned long cluster);

/* Returns the owner of the repective cluster or NULL if the cluster has no
   owner. */

void fix_bad(DOS_FS *fs);

/* Scans the disk for currently unused bad clusters and marks them as bad. */

void reclaim_free(DOS_FS *fs);

/* Marks all allocated, but unused clusters as free. */

void reclaim_file(DOS_FS *fs);

/* Scans the FAT for chains of allocated, but unused clusters and creates files
   for them in the root directory. Also tries to fix all inconsistencies (e.g.
   loops, shared clusters, etc.) in the process. */

unsigned long update_free(DOS_FS *fs);

/* Updates free cluster count in FSINFO sector. */

#endif
