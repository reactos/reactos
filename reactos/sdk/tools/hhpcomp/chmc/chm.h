/*

  Copyright (C) 2010 Alex Andreotti <alex.andreotti@gmail.com>

  This file is part of chmc.

  chmc is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  chmc is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with chmc.  If not, see <http://www.gnu.org/licenses/>.

  NOTE this file is mainly based on chm_lib.c from chmLib by Jed Wing
  http://www.jedrea.com/chmlib/

*/
#ifndef CHMC_CHM_H
#define CHMC_CHM_H

/*
 * architecture specific defines
 *
 * Note: as soon as C99 is more widespread, the below defines should
 * probably just use the C99 sized-int types.
 *
 * The following settings will probably work for many platforms.  The sizes
 * don't have to be exactly correct, but the types must accommodate at least as
 * many bits as they specify.
 */

/* i386, 32-bit/64-bit, Windows */
#ifdef _MSC_VER
typedef unsigned char           UChar;
typedef __int16                 Int16;
typedef unsigned __int16        UInt16;
typedef __int32                 Int32;
typedef unsigned __int32        UInt32;
typedef __int64                 Int64;
typedef unsigned __int64        UInt64;

/* I386, 32-bit, non-Windows */
/* Sparc        */
/* MIPS         */
/* PPC          */
#elif __i386__ || __sun || __sgi || __ppc__
typedef unsigned char           UChar;
typedef short                   Int16;
typedef unsigned short          UInt16;
typedef long                    Int32;
typedef unsigned long           UInt32;
typedef long long               Int64;
typedef unsigned long long      UInt64;

/* x86-64 */
/* Note that these may be appropriate for other 64-bit machines. */
#elif __x86_64__ || __ia64__
typedef unsigned char           UChar;
typedef short                   Int16;
typedef unsigned short          UInt16;
typedef int                     Int32;
typedef unsigned int            UInt32;
typedef long                    Int64;
typedef unsigned long           UInt64;

#else

/* yielding an error is preferable to yielding incorrect behavior */
#error "Please define the sized types for your platform"
#endif

/* GCC */
#ifdef __GNUC__
#define memcmp __builtin_memcmp
#define memset __builtin_memset
#define memcpy __builtin_memcpy
#define strlen __builtin_strlen
#endif

#define _CHMC_ITSF_V3_LEN (0x60)
struct chmcItsfHeader {
	char        signature[4];           /*  0 (ITSF) */
	Int32       version;                /*  4 */
	Int32       header_len;             /*  8 */
	Int32       unknown_000c;           /*  c */
	UInt32      last_modified;          /* 10 */
	UInt32      lang_id;                /* 14 */
	UChar       dir_uuid[16];           /* 18 */
	UChar       stream_uuid[16];        /* 28 */
	UInt64      sect0_offset;           /* 38 */
	UInt64      sect0_len;              /* 40 */
	UInt64      dir_offset;             /* 48 */
	UInt64      dir_len;                /* 50 */
	UInt64      data_offset;            /* 58 (Not present before V3) */
}; /* __attribute__ ((aligned (1))); */

#define _CHMC_SECT0_LEN (0x18)
struct chmcSect0 {
	Int32       unknown_0000;  /*  0 */
	Int32       unknown_0004;  /*  4 */
	UInt64      file_len;      /*  8 */
	Int32       unknown_0010;  /* 10 */
	Int32       unknown_0014;  /* 14 */
};

#define CHM_IDX_INTVL 2

/* structure of ITSP headers */
#define _CHMC_ITSP_V1_LEN (0x54)
struct chmcItspHeader {
	char        signature[4];           /*  0 (ITSP) */
	Int32       version;                /*  4 */
	Int32       header_len;             /*  8 */
	Int32       unknown_000c;           /*  c */
	UInt32      block_len;              /* 10 */
	Int32       blockidx_intvl;         /* 14 */
	Int32       index_depth;            /* 18 */
	Int32       index_root;             /* 1c */
	Int32       index_head;             /* 20 */
	Int32       index_last;             /* 24 */
	Int32       unknown_0028;           /* 28 */
	UInt32      num_blocks;             /* 2c */
	UInt32      lang_id;                /* 30 */
	UChar       system_uuid[16];        /* 34 */
	UInt32      header_len2;            /* 44 */
	UChar       unknown_0048[12];       /* 48 */
}; /* __attribute__ ((aligned (1))); */

/* structure of PMGL headers */
#define _CHMC_PMGL_LEN (0x14)
struct chmcPmglHeader
{
	char        signature[4];           /*  0 (PMGL) */
	UInt32      free_space;             /*  4 */
	UInt32      unknown_0008;           /*  8 */
	Int32       block_prev;             /*  c */
	Int32       block_next;             /* 10 */
}; /* __attribute__ ((aligned (1))); */

#define _CHMC_PMGI_LEN (0x08)
struct chmcPmgiHeader {
	char        signature[4];           /*  0 (PMGI) */
	UInt32      free_space;             /*  4 */
}; /* __attribute__ ((aligned (1))); */

/* structure of LZXC reset table */
#define _CHMC_LZXC_RESETTABLE_V1_LEN (0x28)
struct chmcLzxcResetTable {
	UInt32      version;
	UInt32      block_count;
	UInt32      entry_size;
	UInt32      table_offset;
	UInt64      uncompressed_len;
	UInt64      compressed_len;
	UInt64      block_len;
}; /* __attribute__ ((aligned (1))); */

/* structure of LZXC control data block */
#define _CHMC_LZXC_MIN_LEN (0x18)
#define _CHMC_LZXC_V2_LEN (0x1c)
struct chmcLzxcControlData {
	UInt32      size;                   /*  0        */
	char        signature[4];           /*  4 (LZXC) */
	UInt32      version;                /*  8        */
	UInt32      resetInterval;          /*  c        */
	UInt32      windowSize;             /* 10        */
	UInt32      windowsPerReset;        /* 14        */
	UInt32      unknown_18;             /* 18        */
};

#endif /* CHMC_CHM_H */
