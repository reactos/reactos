/*
 * types.h - Defines for NTFS Linux kernel driver specific types.
 *	     Part of the Linux-NTFS project.
 *
 * Copyright (c) 2001,2002 Anton Altaparmakov.
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS 
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LINUX_NTFS_TYPES_H
#define _LINUX_NTFS_TYPES_H

#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define SN(X)   X	/* Struct Name */
#define SC(P,N) P.N	/* ShortCut: Prefix, Name */
#else
#define SN(X)
#define SC(P,N) N
#endif

/* 2-byte Unicode character type. */
typedef u16 uchar_t;
#define UCHAR_T_SIZE_BITS 1

/*
 * Clusters are signed 64-bit values on NTFS volumes. We define two types, LCN
 * and VCN, to allow for type checking and better code readability.
 */
typedef s64 VCN;
typedef s64 LCN;

/**
 * run_list_element - in memory vcn to lcn mapping array element
 * @vcn:	starting vcn of the current array element
 * @lcn:	starting lcn of the current array element
 * @length:	length in clusters of the current array element
 * 
 * The last vcn (in fact the last vcn + 1) is reached when length == 0.
 * 
 * When lcn == -1 this means that the count vcns starting at vcn are not 
 * physically allocated (i.e. this is a hole / data is sparse).
 */
typedef struct {	/* In memory vcn to lcn mapping structure element. */
	VCN vcn;	/* vcn = Starting virtual cluster number. */
	LCN lcn;	/* lcn = Starting logical cluster number. */
	s64 length;	/* Run length in clusters. */
} run_list_element;

/**
 * run_list - in memory vcn to lcn mapping array including a read/write lock
 * @rl:		pointer to an array of run list elements
 * @lock:	read/write spinlock for serializing access to @rl
 * 
 */
typedef struct {
	run_list_element *rl;
	struct rw_semaphore lock;
} run_list;

typedef enum {
	FALSE = 0,
	TRUE = 1
} BOOL;

typedef enum {
	CASE_SENSITIVE = 0,
	IGNORE_CASE = 1,
} IGNORE_CASE_BOOL;

#endif /* _LINUX_NTFS_TYPES_H */

