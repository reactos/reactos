/* Compound Storage
 *
 * Implemented using the documentation of the LAOLA project at
 * <URL:http://wwwwbs.cs.tu-berlin.de/~schwartz/pmh/index.html>
 * (Thanks to Martin Schwartz <schwartz@cs.tu-berlin.de>)
 *
 * Copyright 1998 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/winbase16.h"
#include "wownt32.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "wine/debug.h"

#include "ifs.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(relay);

struct storage_header {
	BYTE	magic[8];	/* 00: magic */
	BYTE	unknown1[36];	/* 08: unknown */
	DWORD	num_of_bbd_blocks;/* 2C: length of big datablocks */
	DWORD	root_startblock;/* 30: root storage first big block */
	DWORD	unknown2[2];	/* 34: unknown */
	DWORD	sbd_startblock;	/* 3C: small block depot first big block */
	DWORD	unknown3[3];	/* 40: unknown */
	DWORD	bbd_list[109];	/* 4C: big data block list (up to end of sector)*/
};
struct storage_pps_entry {
	WCHAR	pps_rawname[32];/* 00: \0 terminated widechar name */
	WORD	pps_sizeofname;	/* 40: namelength in bytes */
	BYTE	pps_type;	/* 42: flags, 1 storage/dir, 2 stream, 5 root */
	BYTE	pps_unknown0;	/* 43: unknown */
	DWORD	pps_prev;	/* 44: previous pps */
	DWORD	pps_next;	/* 48: next pps */
	DWORD	pps_dir;	/* 4C: directory pps */
	GUID	pps_guid;	/* 50: class ID */
	DWORD	pps_unknown1;	/* 60: unknown */
	FILETIME pps_ft1;	/* 64: filetime1 */
	FILETIME pps_ft2;	/* 70: filetime2 */
	DWORD	pps_sb;		/* 74: data startblock */
	DWORD	pps_size;	/* 78: datalength. (<0x1000)?small:big blocks*/
	DWORD	pps_unknown2;	/* 7C: unknown */
};

#define STORAGE_CHAINENTRY_FAT		0xfffffffd
#define STORAGE_CHAINENTRY_ENDOFCHAIN	0xfffffffe
#define STORAGE_CHAINENTRY_FREE		0xffffffff


static const BYTE STORAGE_magic[8]   ={0xd0,0xcf,0x11,0xe0,0xa1,0xb1,0x1a,0xe1};

#define BIGSIZE		512
#define SMALLSIZE		64

#define SMALLBLOCKS_PER_BIGBLOCK	(BIGSIZE/SMALLSIZE)

#define READ_HEADER(str)	STORAGE_get_big_block(str,-1,(LPBYTE)&sth);assert(!memcmp(STORAGE_magic,sth.magic,sizeof(STORAGE_magic)));
static IStorage16Vtbl stvt16;
static const IStorage16Vtbl *segstvt16 = NULL;
static IStream16Vtbl strvt16;
static const IStream16Vtbl *segstrvt16 = NULL;

/*ULONG WINAPI IStorage16_AddRef(LPSTORAGE16 this);*/
static void _create_istorage16(LPSTORAGE16 *stg);
static void _create_istream16(LPSTREAM16 *str);

#define IMPLEMENTED 1

/* The following is taken from the CorVu implementation of docfiles, and
 * documents things about the file format that are not implemented here, and
 * not documented by the LAOLA project. The CorVu implementation was posted
 * to wine-devel in February 2004, and released under the LGPL at the same
 * time. Because that implementation is in C++, it's not directly usable in
 * Wine, but does have documentation value.
 *
 *
 * #define DF_EXT_VTOC		-4
 * #define DF_VTOC_VTOC		-3
 * #define DF_VTOC_EOF		-2
 * #define DF_VTOC_FREE		-1
 * #define DF_NAMELEN	0x20	// Maximum entry name length - 31 characters plus
 * 				// a NUL terminator
 * 
 * #define DF_FT_STORAGE	1
 * #define DF_FT_STREAM		2
 * #define DF_FT_LOCKBYTES	3	// Not used -- How the bloody hell did I manage
 * #define DF_FT_PROPERTY	4	// Not Used -- to figure these two out?
 * #define DF_FT_ROOT		5
 * 
 * #define DF_BLOCK_SIZE	0x200
 * #define DF_VTOC_SIZE		0x80
 * #define DF_DE_PER_BLOCK	4
 * #define DF_STREAM_BLOCK_SIZE	0x40
 * 
 * A DocFile is divided into blocks of 512 bytes.
 * The first block contains the header.
 *
 * The file header contains The first 109 entries in the VTOC of VTOCs.
 *
 * Each block pointed to by a VTOC of VTOCs contains a VTOC, which
 * includes block chains - just like FAT. This is a somewhat poor
 * design for the following reasons:
 *
 *	1. FAT was a poor file system design to begin with, and
 *	   has long been known to be horrendously inefficient
 *	   for day to day operations.
 *
 *	2. The problem is compounded here, since the file
 *	   level streams are generally *not* read sequentially.
 *	   This means that a significant percentage of reads
 *	   require seeking from the start of the chain.
 *
 * Data chains also contain an internal VTOC. The block size for
 * the standard VTOC is 512. The block size for the internal VTOC
 * is 64.
 *
 * Now, the 109 blocks in the VTOC of VTOCs allows for files of
 * up to around 7MB. So what do you think happens if that's
 * exceeded? Well, there's an entry in the header block which
 * points to the first block used as additional storage for
 * the VTOC of VTOCs.
 *
 * Now we can get up to around 15MB. Now, guess how the file
 * format adds in another block to the VTOC of VTOCs. Come on,
 * it's no big surprise. That's right - the last entry in each
 * block extending the VTOC of VTOCs is, you guessed it, the
 * block number of the next block containing an extension to
 * the VTOC of VTOCs. The VTOC of VTOCs is chained!!!!
 *
 * So, to review:
 *
 * 1. If you are using a FAT file system, the location of
 *    your file's blocks is stored in chains.
 *
 * 2. At the abstract level, the file contains a VTOC of VTOCs,
 *    which is stored in the most inefficient possible format for
 *    random access - a chain (AKA list).
 *
 * 3. The VTOC of VTOCs contains descriptions of three file level
 *    streams:
 *
 *    a. The Directory stream
 *    b. The Data stream
 *    c. The Data VTOC stream
 *
 *    These are, of course, represented as chains.
 *
 * 4. The Data VTOC contains data describing the chains of blocks
 *    within the Data stream.
 *
 * That's right - we have a total of four levels of block chains!
 *
 * Now, is that complicated enough for you? No? OK, there's another
 * complication. If an individual stream (ie. an IStream) reaches
 * 4096 bytes in size, it gets moved from the Data Stream to
 * a new file level stream. Now, if the stream then gets truncated
 * back to less than 4096 bytes, it returns to the data stream.
 *
 * The effect of using this format can be seen very easily. Pick
 * an arbitrary application with a grid data representation that
 * can export to both Lotus 123 and Excel 5 or higher. Export
 * a large file to Lotus 123 and time it. Export the same thing
 * to Excel 5 and time that. The difference is the inefficiency
 * of the Microsoft DocFile format.
 *
 *
 * #define TOTAL_SIMPLE_VTOCS	109
 * 
 * struct	DocFile_Header
 * {
 * 	df_byte iMagic1;	// 0xd0 
 * 	df_byte iMagic2;	// 0xcf 
 * 	df_byte iMagic3;	// 0x11 
 * 	df_byte iMagic4;	// 0xe0 - Spells D0CF11E0, or DocFile 
 * 	df_byte iMagic5;	// 161	(igi upside down) 
 * 	df_byte iMagic6;	// 177	(lli upside down - see below 
 * 	df_byte iMagic7;	// 26 (gz upside down) 
 * 	df_byte iMagic8;	// 225 (szz upside down) - see below 
 * 	df_int4 aiUnknown1[4];
 * 	df_int4 iVersion;	// DocFile Version - 0x03003E	
 * 	df_int4 aiUnknown2[4];
 * 	df_int4 nVTOCs;		// Number of VTOCs 
 * 	df_int4 iFirstDirBlock; // First Directory Block 
 * 	df_int4 aiUnknown3[2];
 * 	df_int4 iFirstDataVTOC; // First data VTOC block 
 * 	df_int4 iHasData;	// 1 if there is data in the file - yes, this is important
 * 	df_int4 iExtendedVTOC;	// Extended VTOC location 
 * 	df_int4 iExtendedVTOCSize; // Size of extended VTOC (+1?) 
 * 	df_int4 aiVTOCofVTOCs[TOTAL_SIMPLE_VTOCS];
 * };
 * 
 * struct	DocFile_VTOC
 * {
 * 	df_int4 aiBlocks[DF_VTOC_SIZE];
 * };
 * 
 * 
 * The meaning of the magic numbers
 *
 * 0xd0cf11e0 is DocFile with a zero on the end (sort of)
 *
 * If you key 177161 into a calculator, then turn the calculator
 * upside down, you get igilli, which may be a reference to
 * somebody's name, or to the Hebrew word for "angel".
 *
 * If you key 26225 into a calculator, then turn it upside down, you
 * get szzgz. Microsoft has a tradition of creating nonsense words
 * using the letters s, g, z and y. We think szzgz may be one of the
 * Microsoft placeholder variables, along the lines of foo, bar and baz.
 * Alternatively, it could be 22526, which would be gzszz.
 *
 * 
 * struct	DocFile_DirEnt
 * {
 * 	df_char achEntryName[DF_NAMELEN];	// Entry Name 
 * 	df_int2 iNameLen;			// Name length in bytes, including NUL terminator 
 * 	df_byte iFileType;			// Entry type 
 * 	df_byte iColour;			// 1 = Black, 0 = Red 
 * 	df_int4 iLeftSibling;			// Next Left Sibling Entry - See below 
 * 	df_int4 iRightSibling;			// Next Right Sibling Entry 
 * 	df_int4 iFirstChild;			// First Child Entry 
 * 	df_byte achClassID[16];			// Class ID 
 * 	df_int4 iStateBits;			// [GS]etStateBits value 
 * 	df_int4 iCreatedLow;			// Low DWORD of creation time 
 * 	df_int4 iCreatedHigh;			// High DWORD of creation time 
 * 	df_int4 iModifiedLow;			// Low DWORD of modification time 
 * 	df_int4 iModifiedHigh;			// High DWORD of modification time 
 * 	df_int4 iVTOCPosition;			// VTOC Position 
 * 	df_int4 iFileSize;			// Size of the stream 
 * 	df_int4 iZero;				// We think this is part of the 64 bit stream size - must be 0 
 * };
 * 
 * Siblings
 * ========
 *
 * Siblings are stored in an obscure but incredibly elegant
 * data structure called a red-black tree. This is generally
 * defined as a 2-3-4 tree stored in a binary tree.
 *
 * A red-black tree can always be balanced very easily. The rules
 * for a red-black tree are as follows:
 *
 *	1. The root node is always black.
 *	2. The parent of a red node is always black.
 *
 * There is a Java demo of red-black trees at:
 *
 *	http://langevin.usc.edu/BST/RedBlackTree-Example.html
 *
 * This demo is an excellent tool for learning how red-black
 * trees work, without having to go through the process of
 * learning how they were derived.
 *
 * Within the tree, elements are ordered by the length of the
 * name and within that, ASCII order by name. This causes the
 * apparently bizarre reordering you see when you use dfview.
 *
 * This is a somewhat bizarre choice. It suggests that the
 * designer of the DocFile format was trying to optimise
 * searching through the directory entries. However searching
 * through directory entries is a relatively rare operation.
 * Reading and seeking within a stream are much more common
 * operations, especially within the file level streams, yet
 * these use the horrendously inefficient FAT chains.
 *
 * This suggests that the designer was probably somebody
 * fresh out of university, who had some basic knowledge of
 * basic data structures, but little knowledge of anything
 * more practical. It is bizarre to attempt to optimise
 * directory searches while not using a more efficient file
 * block locating system than FAT (seedling/sapling/tree
 * would result in a massive improvement - in fact we have
 * an alternative to docfiles that we use internally that
 * uses seedling/sapling/tree and *is* far more efficient).
 *
 * It is worth noting that the MS implementation of red-black
 * trees is incorrect (I can tell you're surprised) and
 * actually causes more operations to occur than are really
 * needed. Fortunately the fact that our implementation is
 * correct will not cause any problems - the MS implementation
 * still appears to cause the tree to satisfy the rules, albeit
 * a sequence of the same insertions in the different
 * implementations may result in a different, and possibly
 * deeper (but never shallower) tree.
 */

typedef struct {
	HANDLE		hf;
	SEGPTR		lockbytes;
} stream_access16;
/* --- IStorage16 implementation struct */

typedef struct
{
        /* IUnknown fields */
        const IStorage16Vtbl           *lpVtbl;
        LONG                            ref;
        /* IStorage16 fields */
        SEGPTR                          thisptr; /* pointer to this struct as segmented */
        struct storage_pps_entry        stde;
        int                             ppsent;
	stream_access16			str;
} IStorage16Impl;


/******************************************************************************
 *		STORAGE_get_big_block	[Internal]
 *
 * Reading OLE compound storage
 */
static BOOL
STORAGE_get_big_block(stream_access16 *str,int n,BYTE *block)
{
    DWORD result;

    assert(n>=-1);
    if (str->hf) {
	if ((SetFilePointer( str->hf, (n+1)*BIGSIZE, NULL,
			     SEEK_SET ) == INVALID_SET_FILE_POINTER) && GetLastError())
	{
            WARN("(%p,%d,%p), seek failed (%d)\n",str->hf, n, block, GetLastError());
	    return FALSE;
	}
	if (!ReadFile( str->hf, block, BIGSIZE, &result, NULL ) || result != BIGSIZE)
	{
            WARN("(hf=%p, block size %d): read didn't read (%d)\n",str->hf,n,GetLastError());
	    return FALSE;
	}
    } else {
	DWORD args[6];
	HRESULT hres;
	HANDLE16 hsig;
	
	args[0] = (DWORD)str->lockbytes;	/* iface */
	args[1] = (n+1)*BIGSIZE;
	args[2] = 0;	/* ULARGE_INTEGER offset */
	args[3] = WOWGlobalAllocLock16( 0, BIGSIZE, &hsig ); /* sig */
	args[4] = BIGSIZE;
	args[5] = 0;

	if (!WOWCallback16Ex(
	    (DWORD)((const ILockBytes16Vtbl*)MapSL(
			(SEGPTR)((LPLOCKBYTES16)MapSL(str->lockbytes))->lpVtbl)
	    )->ReadAt,
	    WCB16_PASCAL,
	    6*sizeof(DWORD),
	    (LPVOID)args,
	    (LPDWORD)&hres
	)) {
            ERR("CallTo16 ILockBytes16::ReadAt() failed, hres %x\n",hres);
	    return FALSE;
	}
	memcpy(block, MapSL(args[3]), BIGSIZE);
	WOWGlobalUnlockFree16(args[3]);
    }
    return TRUE;
}

static BOOL
_ilockbytes16_writeat(SEGPTR lockbytes, DWORD offset, DWORD length, void *buffer) {
    DWORD args[6];
    HRESULT hres;

    args[0] = (DWORD)lockbytes;	/* iface */
    args[1] = offset;
    args[2] = 0;	/* ULARGE_INTEGER offset */
    args[3] = (DWORD)MapLS( buffer );
    args[4] = length;
    args[5] = 0;

    /* THIS_ ULARGE_INTEGER ulOffset, const void *pv, ULONG cb, ULONG *pcbWritten); */

    if (!WOWCallback16Ex(
	(DWORD)((const ILockBytes16Vtbl*)MapSL(
		    (SEGPTR)((LPLOCKBYTES16)MapSL(lockbytes))->lpVtbl)
	)->WriteAt,
	WCB16_PASCAL,
	6*sizeof(DWORD),
	(LPVOID)args,
	(LPDWORD)&hres
    )) {
	ERR("CallTo16 ILockBytes16::WriteAt() failed, hres %x\n",hres);
	return FALSE;
    }
    UnMapLS(args[3]);
    return TRUE;
}

/******************************************************************************
 * STORAGE_put_big_block [INTERNAL]
 */
static BOOL
STORAGE_put_big_block(stream_access16 *str,int n,BYTE *block)
{
    DWORD result;

    assert(n>=-1);
    if (str->hf) {
	if ((SetFilePointer( str->hf, (n+1)*BIGSIZE, NULL,
			     SEEK_SET ) == INVALID_SET_FILE_POINTER) && GetLastError())
	{
            WARN("seek failed (%d)\n",GetLastError());
	    return FALSE;
	}
	if (!WriteFile( str->hf, block, BIGSIZE, &result, NULL ) || result != BIGSIZE)
	{
            WARN(" write failed (%d)\n",GetLastError());
	    return FALSE;
	}
	return TRUE;
    } else {
	_ilockbytes16_writeat(str->lockbytes, (n+1)*BIGSIZE, BIGSIZE, block);
	return TRUE;
    }
}

/******************************************************************************
 * STORAGE_get_next_big_blocknr [INTERNAL]
 */
static int
STORAGE_get_next_big_blocknr(stream_access16 *str,int blocknr) {
	INT	bbs[BIGSIZE/sizeof(INT)];
	struct	storage_header	sth;

	READ_HEADER(str);

	assert(blocknr>>7<sth.num_of_bbd_blocks);
	if (sth.bbd_list[blocknr>>7]==0xffffffff)
		return -5;
	if (!STORAGE_get_big_block(str,sth.bbd_list[blocknr>>7],(LPBYTE)bbs))
		return -5;
	assert(bbs[blocknr&0x7f]!=STORAGE_CHAINENTRY_FREE);
	return bbs[blocknr&0x7f];
}

/******************************************************************************
 * STORAGE_get_nth_next_big_blocknr [INTERNAL]
 */
static int
STORAGE_get_nth_next_big_blocknr(stream_access16 *str,int blocknr,int nr) {
	INT	bbs[BIGSIZE/sizeof(INT)];
	int	lastblock = -1;
	struct storage_header sth;

	TRACE("(blocknr=%d, nr=%d)\n", blocknr, nr);
	READ_HEADER(str);

	assert(blocknr>=0);
	while (nr--) {
		assert((blocknr>>7)<sth.num_of_bbd_blocks);
		assert(sth.bbd_list[blocknr>>7]!=0xffffffff);

		/* simple caching... */
		if (lastblock!=sth.bbd_list[blocknr>>7]) {
			BOOL ret = STORAGE_get_big_block(str,sth.bbd_list[blocknr>>7],(LPBYTE)bbs);
			assert(ret);
			lastblock = sth.bbd_list[blocknr>>7];
		}
		blocknr = bbs[blocknr&0x7f];
	}
	return blocknr;
}

/******************************************************************************
 *		STORAGE_get_root_pps_entry	[Internal]
 */
static BOOL
STORAGE_get_root_pps_entry(stream_access16* str,struct storage_pps_entry *pstde) {
	int	blocknr,i;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry	*stde=(struct storage_pps_entry*)block;
	struct storage_header sth;

	READ_HEADER(str);
	blocknr = sth.root_startblock;
	TRACE("startblock is %d\n", blocknr);
	while (blocknr>=0) {
		BOOL ret = STORAGE_get_big_block(str,blocknr,block);
		assert(ret);
		for (i=0;i<4;i++) {
			if (!stde[i].pps_sizeofname)
				continue;
			if (stde[i].pps_type==5) {
				*pstde=stde[i];
				return TRUE;
			}
		}
		blocknr=STORAGE_get_next_big_blocknr(str,blocknr);
		TRACE("next block is %d\n", blocknr);
	}
	return FALSE;
}

/******************************************************************************
 * STORAGE_get_small_block [INTERNAL]
 */
static BOOL
STORAGE_get_small_block(stream_access16 *str,int blocknr,BYTE *sblock) {
	BYTE				block[BIGSIZE];
	int				bigblocknr;
	struct storage_pps_entry	root;
	BOOL ret;

	TRACE("(blocknr=%d)\n", blocknr);
	assert(blocknr>=0);
	ret = STORAGE_get_root_pps_entry(str,&root);
	assert(ret);
	bigblocknr = STORAGE_get_nth_next_big_blocknr(str,root.pps_sb,blocknr/SMALLBLOCKS_PER_BIGBLOCK);
	assert(bigblocknr>=0);
	ret = STORAGE_get_big_block(str,bigblocknr,block);
	assert(ret);

	memcpy(sblock,((LPBYTE)block)+SMALLSIZE*(blocknr&(SMALLBLOCKS_PER_BIGBLOCK-1)),SMALLSIZE);
	return TRUE;
}

/******************************************************************************
 * STORAGE_put_small_block [INTERNAL]
 */
static BOOL
STORAGE_put_small_block(stream_access16 *str,int blocknr,const BYTE *sblock) {
	BYTE				block[BIGSIZE];
	int				bigblocknr;
	struct storage_pps_entry	root;
	BOOL ret;

	assert(blocknr>=0);
	TRACE("(blocknr=%d)\n", blocknr);

	ret = STORAGE_get_root_pps_entry(str,&root);
	assert(ret);
	bigblocknr = STORAGE_get_nth_next_big_blocknr(str,root.pps_sb,blocknr/SMALLBLOCKS_PER_BIGBLOCK);
	assert(bigblocknr>=0);
	ret = STORAGE_get_big_block(str,bigblocknr,block);
	assert(ret);

	memcpy(((LPBYTE)block)+SMALLSIZE*(blocknr&(SMALLBLOCKS_PER_BIGBLOCK-1)),sblock,SMALLSIZE);
	ret = STORAGE_put_big_block(str,bigblocknr,block);
	assert(ret);
	return TRUE;
}

/******************************************************************************
 * STORAGE_get_next_small_blocknr [INTERNAL]
 */
static int
STORAGE_get_next_small_blocknr(stream_access16 *str,int blocknr) {
	BYTE				block[BIGSIZE];
	LPINT				sbd = (LPINT)block;
	int				bigblocknr;
	struct storage_header		sth;
	BOOL ret;

	TRACE("(blocknr=%d)\n", blocknr);
	READ_HEADER(str);
	assert(blocknr>=0);
	bigblocknr = STORAGE_get_nth_next_big_blocknr(str,sth.sbd_startblock,blocknr/128);
	assert(bigblocknr>=0);
	ret = STORAGE_get_big_block(str,bigblocknr,block);
	assert(ret);
	assert(sbd[blocknr & 127]!=STORAGE_CHAINENTRY_FREE);
	return sbd[blocknr & (128-1)];
}

/******************************************************************************
 * STORAGE_get_nth_next_small_blocknr [INTERNAL]
 */
static int
STORAGE_get_nth_next_small_blocknr(stream_access16*str,int blocknr,int nr) {
	int	lastblocknr=-1;
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	struct storage_header sth;
	BOOL ret;

	TRACE("(blocknr=%d, nr=%d)\n", blocknr, nr);
	READ_HEADER(str);
	assert(blocknr>=0);
	while ((nr--) && (blocknr>=0)) {
		if (lastblocknr/128!=blocknr/128) {
			int	bigblocknr;
			bigblocknr = STORAGE_get_nth_next_big_blocknr(str,sth.sbd_startblock,blocknr/128);
			assert(bigblocknr>=0);
			ret = STORAGE_get_big_block(str,bigblocknr,block);
			assert(ret);
			lastblocknr = blocknr;
		}
		assert(lastblocknr>=0);
		lastblocknr=blocknr;
		blocknr=sbd[blocknr & (128-1)];
		assert(blocknr!=STORAGE_CHAINENTRY_FREE);
	}
	return blocknr;
}

/******************************************************************************
 * STORAGE_get_pps_entry [INTERNAL]
 */
static int
STORAGE_get_pps_entry(stream_access16*str,int n,struct storage_pps_entry *pstde) {
	int	blocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)(((LPBYTE)block)+128*(n&3));
	struct storage_header sth;
	BOOL ret;

	TRACE("(n=%d)\n", n);
	READ_HEADER(str);
	/* we have 4 pps entries per big block */
	blocknr = STORAGE_get_nth_next_big_blocknr(str,sth.root_startblock,n/4);
	assert(blocknr>=0);
	ret = STORAGE_get_big_block(str,blocknr,block);
	assert(ret);

	*pstde=*stde;
	return 1;
}

/******************************************************************************
 *		STORAGE_put_pps_entry	[Internal]
 */
static int
STORAGE_put_pps_entry(stream_access16*str,int n,const struct storage_pps_entry *pstde) {
	int	blocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)(((LPBYTE)block)+128*(n&3));
	struct storage_header sth;
	BOOL ret;

	TRACE("(n=%d)\n", n);
	READ_HEADER(str);
	/* we have 4 pps entries per big block */
	blocknr = STORAGE_get_nth_next_big_blocknr(str,sth.root_startblock,n/4);
	assert(blocknr>=0);
	ret = STORAGE_get_big_block(str,blocknr,block);
	assert(ret);
	*stde=*pstde;
	ret = STORAGE_put_big_block(str,blocknr,block);
	assert(ret);
	return 1;
}

/******************************************************************************
 *		STORAGE_look_for_named_pps	[Internal]
 */
static int
STORAGE_look_for_named_pps(stream_access16*str,int n,LPOLESTR name) {
	struct storage_pps_entry	stde;
	int				ret;

	TRACE("(n=%d,name=%s)\n", n, debugstr_w(name));
	if (n==-1)
		return -1;
	if (1!=STORAGE_get_pps_entry(str,n,&stde))
		return -1;

	if (!lstrcmpW(name,stde.pps_rawname))
		return n;
	if (stde.pps_prev != -1) {
		ret=STORAGE_look_for_named_pps(str,stde.pps_prev,name);
		if (ret!=-1)
			return ret;
	}
	if (stde.pps_next != -1) {
		ret=STORAGE_look_for_named_pps(str,stde.pps_next,name);
		if (ret!=-1)
			return ret;
	}
	return -1;
}

/******************************************************************************
 *		STORAGE_dump_pps_entry	[Internal]
 *
 * This function is there to simplify debugging. It is otherwise unused.
 */
void
STORAGE_dump_pps_entry(struct storage_pps_entry *stde) {
    char	name[33];

    WideCharToMultiByte( CP_ACP, 0, stde->pps_rawname, -1, name, sizeof(name), NULL, NULL);
	if (!stde->pps_sizeofname)
		return;
	TRACE("name: %s\n",name);
	TRACE("type: %d\n",stde->pps_type);
	TRACE("prev pps: %d\n",stde->pps_prev);
	TRACE("next pps: %d\n",stde->pps_next);
	TRACE("dir pps: %d\n",stde->pps_dir);
	TRACE("guid: %s\n",debugstr_guid(&(stde->pps_guid)));
	if (stde->pps_type !=2) {
		time_t	t;
                DWORD dw;
		RtlTimeToSecondsSince1970((LARGE_INTEGER *)&(stde->pps_ft1),&dw);
                t = dw;
		TRACE("ts1: %s\n",ctime(&t));
		RtlTimeToSecondsSince1970((LARGE_INTEGER *)&(stde->pps_ft2),&dw);
                t = dw;
		TRACE("ts2: %s\n",ctime(&t));
	}
	TRACE("startblock: %d\n",stde->pps_sb);
	TRACE("size: %d\n",stde->pps_size);
}

/******************************************************************************
 * STORAGE_init_storage [INTERNAL]
 */
static BOOL
STORAGE_init_storage(stream_access16 *str) {
	BYTE	block[BIGSIZE];
	LPDWORD	bbs;
	struct storage_header *sth;
	struct storage_pps_entry *stde;
        DWORD result;

	if (str->hf)
	    SetFilePointer( str->hf, 0, NULL, SEEK_SET );
	/* block -1 is the storage header */
	sth = (struct storage_header*)block;
	memcpy(sth->magic,STORAGE_magic,8);
	memset(sth->unknown1,0,sizeof(sth->unknown1));
	memset(sth->unknown2,0,sizeof(sth->unknown2));
	memset(sth->unknown3,0,sizeof(sth->unknown3));
	sth->num_of_bbd_blocks	= 1;
	sth->root_startblock	= 1;
	sth->sbd_startblock	= 0xffffffff;
	memset(sth->bbd_list,0xff,sizeof(sth->bbd_list));
	sth->bbd_list[0]	= 0;
	if (str->hf) {
	    if (!WriteFile( str->hf, block, BIGSIZE, &result, NULL ) || result != BIGSIZE) return FALSE;
	} else {
	    if (!_ilockbytes16_writeat(str->lockbytes, 0, BIGSIZE, block)) return FALSE;
	}
	/* block 0 is the big block directory */
	bbs=(LPDWORD)block;
	memset(block,0xff,sizeof(block)); /* mark all blocks as free */
	bbs[0]=STORAGE_CHAINENTRY_ENDOFCHAIN; /* for this block */
	bbs[1]=STORAGE_CHAINENTRY_ENDOFCHAIN; /* for directory entry */
	if (str->hf) {
	    if (!WriteFile( str->hf, block, BIGSIZE, &result, NULL ) || result != BIGSIZE) return FALSE;
	} else {
	    if (!_ilockbytes16_writeat(str->lockbytes, BIGSIZE, BIGSIZE, block)) return FALSE;
	}
	/* block 1 is the root directory entry */
	memset(block,0x00,sizeof(block));
	stde = (struct storage_pps_entry*)block;
        MultiByteToWideChar( CP_ACP, 0, "RootEntry", -1, stde->pps_rawname,
                             sizeof(stde->pps_rawname)/sizeof(WCHAR));
	stde->pps_sizeofname	= (strlenW(stde->pps_rawname)+1) * sizeof(WCHAR);
	stde->pps_type		= 5;
	stde->pps_dir		= -1;
	stde->pps_next		= -1;
	stde->pps_prev		= -1;
	stde->pps_sb		= 0xffffffff;
	stde->pps_size		= 0;
	if (str->hf) {
	    return (WriteFile( str->hf, block, BIGSIZE, &result, NULL ) && result == BIGSIZE);
	} else {
	    return _ilockbytes16_writeat(str->lockbytes, BIGSIZE, BIGSIZE, block);
	}
}

/******************************************************************************
 *		STORAGE_set_big_chain	[Internal]
 */
static BOOL
STORAGE_set_big_chain(stream_access16*str,int blocknr,INT type) {
	BYTE	block[BIGSIZE];
	LPINT	bbd = (LPINT)block;
	int	nextblocknr,bigblocknr;
	struct storage_header sth;
	BOOL ret;

	READ_HEADER(str);
	assert(blocknr!=type);
	while (blocknr>=0) {
		bigblocknr = sth.bbd_list[blocknr/128];
		assert(bigblocknr>=0);
		ret = STORAGE_get_big_block(str,bigblocknr,block);
		assert(ret);

		nextblocknr = bbd[blocknr&(128-1)];
		bbd[blocknr&(128-1)] = type;
		if (type>=0)
			return TRUE;
		ret = STORAGE_put_big_block(str,bigblocknr,block);
		assert(ret);
		type = STORAGE_CHAINENTRY_FREE;
		blocknr = nextblocknr;
	}
	return TRUE;
}

/******************************************************************************
 * STORAGE_set_small_chain [Internal]
 */
static BOOL
STORAGE_set_small_chain(stream_access16*str,int blocknr,INT type) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastblocknr,nextsmallblocknr,bigblocknr;
	struct storage_header sth;
	BOOL ret;

	READ_HEADER(str);

	assert(blocknr!=type);
	lastblocknr=-129;bigblocknr=-2;
	while (blocknr>=0) {
		/* cache block ... */
		if (lastblocknr/128!=blocknr/128) {
			bigblocknr = STORAGE_get_nth_next_big_blocknr(str,sth.sbd_startblock,blocknr/128);
			assert(bigblocknr>=0);
			ret = STORAGE_get_big_block(str,bigblocknr,block);
			assert(ret);
		}
		lastblocknr = blocknr;
		nextsmallblocknr = sbd[blocknr&(128-1)];
		sbd[blocknr&(128-1)] = type;
		ret = STORAGE_put_big_block(str,bigblocknr,block);
		assert(ret);
		if (type>=0)
			return TRUE;
		type = STORAGE_CHAINENTRY_FREE;
		blocknr = nextsmallblocknr;
	}
	return TRUE;
}

/******************************************************************************
 *		STORAGE_get_free_big_blocknr	[Internal]
 */
static int
STORAGE_get_free_big_blocknr(stream_access16 *str) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastbigblocknr,i,bigblocknr;
	unsigned int curblock;
	struct storage_header sth;
	BOOL ret;

	READ_HEADER(str);
	curblock	= 0;
	lastbigblocknr	= -1;
	bigblocknr	= sth.bbd_list[curblock];
	while (curblock<sth.num_of_bbd_blocks) {
		assert(bigblocknr>=0);
		ret = STORAGE_get_big_block(str,bigblocknr,block);
		assert(ret);
		for (i=0;i<128;i++)
			if (sbd[i]==STORAGE_CHAINENTRY_FREE) {
				sbd[i] = STORAGE_CHAINENTRY_ENDOFCHAIN;
				ret = STORAGE_put_big_block(str,bigblocknr,block);
				assert(ret);
				memset(block,0x42,sizeof(block));
				ret = STORAGE_put_big_block(str,i+curblock*128,block);
				assert(ret);
				return i+curblock*128;
			}
		lastbigblocknr = bigblocknr;
		bigblocknr = sth.bbd_list[++curblock];
	}
	bigblocknr = curblock*128;
	/* since we have marked all blocks from 0 up to curblock*128-1
	 * the next free one is curblock*128, where we happily put our
	 * next large block depot.
	 */
	memset(block,0xff,sizeof(block));
	/* mark the block allocated and returned by this function */
	sbd[1] = STORAGE_CHAINENTRY_ENDOFCHAIN;
	ret = STORAGE_put_big_block(str,bigblocknr,block);
	assert(ret);

	/* if we had a bbd block already (mostlikely) we need
	 * to link the new one into the chain
	 */
	if (lastbigblocknr!=-1) {
		ret = STORAGE_set_big_chain(str,lastbigblocknr,bigblocknr);
		assert(ret);
	}
	sth.bbd_list[curblock]=bigblocknr;
	sth.num_of_bbd_blocks++;
	assert(sth.num_of_bbd_blocks==curblock+1);
	ret = STORAGE_put_big_block(str,-1,(LPBYTE)&sth);
	assert(ret);

	/* Set the end of the chain for the bigblockdepots */
	ret = STORAGE_set_big_chain(str,bigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN);
	assert(ret);
	/* add 1, for the first entry is used for the additional big block
	 * depot. (means we already used bigblocknr) */
	memset(block,0x42,sizeof(block));
	/* allocate this block (filled with 0x42) */
	ret = STORAGE_put_big_block(str,bigblocknr+1,block);
	assert(ret);
	return bigblocknr+1;
}


/******************************************************************************
 *		STORAGE_get_free_small_blocknr	[Internal]
 */
static int
STORAGE_get_free_small_blocknr(stream_access16 *str) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastbigblocknr,newblocknr,i,curblock,bigblocknr;
	struct storage_pps_entry	root;
	struct storage_header sth;

	READ_HEADER(str);
	bigblocknr	= sth.sbd_startblock;
	curblock	= 0;
	lastbigblocknr	= -1;
	newblocknr	= -1;
	while (bigblocknr>=0) {
		if (!STORAGE_get_big_block(str,bigblocknr,block))
			return -1;
		for (i=0;i<128;i++)
			if (sbd[i]==STORAGE_CHAINENTRY_FREE) {
				sbd[i]=STORAGE_CHAINENTRY_ENDOFCHAIN;
				newblocknr = i+curblock*128;
				break;
			}
		if (i!=128)
			break;
		lastbigblocknr = bigblocknr;
		bigblocknr = STORAGE_get_next_big_blocknr(str,bigblocknr);
		curblock++;
	}
	if (newblocknr==-1) {
		bigblocknr = STORAGE_get_free_big_blocknr(str);
		if (bigblocknr<0)
			return -1;
		READ_HEADER(str);
		memset(block,0xff,sizeof(block));
		sbd[0]=STORAGE_CHAINENTRY_ENDOFCHAIN;
		if (!STORAGE_put_big_block(str,bigblocknr,block))
			return -1;
		if (lastbigblocknr==-1) {
			sth.sbd_startblock = bigblocknr;
			if (!STORAGE_put_big_block(str,-1,(LPBYTE)&sth)) /* need to write it */
				return -1;
		} else {
			if (!STORAGE_set_big_chain(str,lastbigblocknr,bigblocknr))
				return -1;
		}
		if (!STORAGE_set_big_chain(str,bigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
			return -1;
		newblocknr = curblock*128;
	}
	/* allocate enough big blocks for storing the allocated small block */
	if (!STORAGE_get_root_pps_entry(str,&root))
		return -1;
	if (root.pps_sb==-1)
		lastbigblocknr	= -1;
	else
		lastbigblocknr	= STORAGE_get_nth_next_big_blocknr(str,root.pps_sb,(root.pps_size-1)/BIGSIZE);
	while (root.pps_size < (newblocknr*SMALLSIZE+SMALLSIZE-1)) {
		/* we need to allocate more stuff */
		bigblocknr = STORAGE_get_free_big_blocknr(str);
		if (bigblocknr<0)
			return -1;
		READ_HEADER(str);
		if (root.pps_sb==-1) {
			root.pps_sb	 = bigblocknr;
			root.pps_size	+= BIGSIZE;
		} else {
			if (!STORAGE_set_big_chain(str,lastbigblocknr,bigblocknr))
				return -1;
			root.pps_size	+= BIGSIZE;
		}
		lastbigblocknr = bigblocknr;
	}
	if (!STORAGE_set_big_chain(str,lastbigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
		return -1;
	if (!STORAGE_put_pps_entry(str,0,&root))
		return -1;
	return newblocknr;
}

/******************************************************************************
 *		STORAGE_get_free_pps_entry	[Internal]
 */
static int
STORAGE_get_free_pps_entry(stream_access16*str) {
	int	blocknr, i, curblock, lastblocknr=-1;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)block;
	struct storage_header sth;

	READ_HEADER(str);
	blocknr = sth.root_startblock;
	assert(blocknr>=0);
	curblock=0;
	while (blocknr>=0) {
		if (!STORAGE_get_big_block(str,blocknr,block))
			return -1;
		for (i=0;i<4;i++)
			if (stde[i].pps_sizeofname==0) /* free */
				return curblock*4+i;
		lastblocknr = blocknr;
		blocknr = STORAGE_get_next_big_blocknr(str,blocknr);
		curblock++;
	}
	assert(blocknr==STORAGE_CHAINENTRY_ENDOFCHAIN);
	blocknr = STORAGE_get_free_big_blocknr(str);
	/* sth invalidated */
	if (blocknr<0)
		return -1;

	if (!STORAGE_set_big_chain(str,lastblocknr,blocknr))
		return -1;
	if (!STORAGE_set_big_chain(str,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
		return -1;
	memset(block,0,sizeof(block));
	STORAGE_put_big_block(str,blocknr,block);
	return curblock*4;
}

/* --- IStream16 implementation */

typedef struct
{
        /* IUnknown fields */
        const IStream16Vtbl            *lpVtbl;
        LONG                            ref;
        /* IStream16 fields */
        SEGPTR                          thisptr; /* pointer to this struct as segmented */
        struct storage_pps_entry        stde;
        int                             ppsent;
        ULARGE_INTEGER                  offset;
	stream_access16			str;
} IStream16Impl;

/******************************************************************************
 *		IStream16_QueryInterface	[STORAGE.518]
 */
HRESULT CDECL IStream16_fnQueryInterface(
	IStream16* iface,REFIID refiid,LPVOID *obj
) {
	IStream16Impl *This = (IStream16Impl *)iface;
	TRACE_(relay)("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = This;
		return 0;
	}
	return OLE_E_ENUM_NOMORE;

}

/******************************************************************************
 * IStream16_AddRef [STORAGE.519]
 */
ULONG CDECL IStream16_fnAddRef(IStream16* iface) {
	IStream16Impl *This = (IStream16Impl *)iface;
	return InterlockedIncrement(&This->ref);
}

static void
_ilockbytes16_addref(SEGPTR lockbytes) {
    DWORD args[1];
    HRESULT hres;
    
    args[0] = (DWORD)lockbytes;	/* iface */
    if (!WOWCallback16Ex(
	(DWORD)((const ILockBytes16Vtbl*)MapSL(
		    (SEGPTR)((LPLOCKBYTES16)MapSL(lockbytes))->lpVtbl)
	)->AddRef,
	WCB16_PASCAL,
	1*sizeof(DWORD),
	(LPVOID)args,
	(LPDWORD)&hres
    ))
	ERR("CallTo16 ILockBytes16::AddRef() failed, hres %x\n",hres);
}

static void
_ilockbytes16_release(SEGPTR lockbytes) {
    DWORD args[1];
    HRESULT hres;
    
    args[0] = (DWORD)lockbytes;	/* iface */
    if (!WOWCallback16Ex(
	(DWORD)((const ILockBytes16Vtbl*)MapSL(
		    (SEGPTR)((LPLOCKBYTES16)MapSL(lockbytes))->lpVtbl)
	)->Release,
	WCB16_PASCAL,
	1*sizeof(DWORD),
	(LPVOID)args,
	(LPDWORD)&hres
    ))
	ERR("CallTo16 ILockBytes16::Release() failed, hres %x\n",hres);
}

static void
_ilockbytes16_flush(SEGPTR lockbytes) {
    DWORD args[1];
    HRESULT hres;
    
    args[0] = (DWORD)lockbytes;	/* iface */
    if (!WOWCallback16Ex(
	(DWORD)((const ILockBytes16Vtbl*)MapSL(
		    (SEGPTR)((LPLOCKBYTES16)MapSL(lockbytes))->lpVtbl)
	)->Flush,
	WCB16_PASCAL,
	1*sizeof(DWORD),
	(LPVOID)args,
	(LPDWORD)&hres
    ))
	ERR("CallTo16 ILockBytes16::Flush() failed, hres %x\n",hres);
}

/******************************************************************************
 * IStream16_Release [STORAGE.520]
 */
ULONG CDECL IStream16_fnRelease(IStream16* iface) {
	IStream16Impl *This = (IStream16Impl *)iface;
        ULONG ref;

	if (This->str.hf)
	    FlushFileBuffers(This->str.hf);
	else
	    _ilockbytes16_flush(This->str.lockbytes);
        ref = InterlockedDecrement(&This->ref);
	if (ref)
	    return ref;

	if (This->str.hf)
	    CloseHandle(This->str.hf);
	else
	    _ilockbytes16_release(This->str.lockbytes);
        UnMapLS( This->thisptr );
	HeapFree( GetProcessHeap(), 0, This );
	return 0;
}

/******************************************************************************
 *		IStream16_Seek	[STORAGE.523]
 *
 * FIXME
 *    Does not handle 64 bits
 */
HRESULT CDECL IStream16_fnSeek(
	IStream16* iface,LARGE_INTEGER offset,DWORD whence,ULARGE_INTEGER *newpos
) {
	IStream16Impl *This = (IStream16Impl *)iface;
	TRACE_(relay)("(%p)->([%d.%d],%d,%p)\n",This,offset.u.HighPart,offset.u.LowPart,whence,newpos);

	switch (whence) {
	/* unix SEEK_xx should be the same as win95 ones */
	case SEEK_SET:
		/* offset must be ==0 (<0 is invalid, and >0 cannot be handled
		 * right now.
		 */
		assert(offset.u.HighPart==0);
		This->offset.u.HighPart = offset.u.HighPart;
		This->offset.u.LowPart = offset.u.LowPart;
		break;
	case SEEK_CUR:
		if (offset.u.HighPart < 0) {
			/* FIXME: is this negation correct ? */
			offset.u.HighPart = -offset.u.HighPart;
			offset.u.LowPart = (0xffffffff ^ offset.u.LowPart)+1;

			assert(offset.u.HighPart==0);
			assert(This->offset.u.LowPart >= offset.u.LowPart);
			This->offset.u.LowPart -= offset.u.LowPart;
		} else {
			assert(offset.u.HighPart==0);
			This->offset.u.LowPart+= offset.u.LowPart;
		}
		break;
	case SEEK_END:
		assert(offset.u.HighPart==0);
		This->offset.u.LowPart = This->stde.pps_size-offset.u.LowPart;
		break;
	}
	if (This->offset.u.LowPart>This->stde.pps_size)
		This->offset.u.LowPart=This->stde.pps_size;
	if (newpos) *newpos = This->offset;
	return S_OK;
}

/******************************************************************************
 *		IStream16_Read	[STORAGE.521]
 */
HRESULT CDECL IStream16_fnRead(
        IStream16* iface,void  *pv,ULONG cb,ULONG  *pcbRead
) {
	IStream16Impl *This = (IStream16Impl *)iface;
	BYTE	block[BIGSIZE];
	ULONG	*bytesread=pcbRead,xxread;
	int	blocknr;
	LPBYTE	pbv = pv;

	TRACE_(relay)("(%p)->(%p,%d,%p)\n",This,pv,cb,pcbRead);
	if (!pcbRead) bytesread=&xxread;
	*bytesread = 0;

	if (cb>This->stde.pps_size-This->offset.u.LowPart)
		cb=This->stde.pps_size-This->offset.u.LowPart;
	if (This->stde.pps_size < 0x1000) {
		/* use small block reader */
		blocknr = STORAGE_get_nth_next_small_blocknr(&This->str,This->stde.pps_sb,This->offset.u.LowPart/SMALLSIZE);
		while (cb) {
			unsigned int cc;

			if (!STORAGE_get_small_block(&This->str,blocknr,block)) {
			   WARN("small block read failed!!!\n");
				return E_FAIL;
			}
			cc = cb;
			if (cc>SMALLSIZE-(This->offset.u.LowPart&(SMALLSIZE-1)))
				cc=SMALLSIZE-(This->offset.u.LowPart&(SMALLSIZE-1));
			memcpy(pbv,block+(This->offset.u.LowPart&(SMALLSIZE-1)),cc);
			This->offset.u.LowPart+=cc;
			pbv+=cc;
			*bytesread+=cc;
			cb-=cc;
			blocknr = STORAGE_get_next_small_blocknr(&This->str,blocknr);
		}
	} else {
		/* use big block reader */
		blocknr = STORAGE_get_nth_next_big_blocknr(&This->str,This->stde.pps_sb,This->offset.u.LowPart/BIGSIZE);
		while (cb) {
			unsigned int cc;

			if (!STORAGE_get_big_block(&This->str,blocknr,block)) {
				WARN("big block read failed!!!\n");
				return E_FAIL;
			}
			cc = cb;
			if (cc>BIGSIZE-(This->offset.u.LowPart&(BIGSIZE-1)))
				cc=BIGSIZE-(This->offset.u.LowPart&(BIGSIZE-1));
			memcpy(pbv,block+(This->offset.u.LowPart&(BIGSIZE-1)),cc);
			This->offset.u.LowPart+=cc;
			pbv+=cc;
			*bytesread+=cc;
			cb-=cc;
			blocknr=STORAGE_get_next_big_blocknr(&This->str,blocknr);
		}
	}
	return S_OK;
}

/******************************************************************************
 *		IStream16_Write	[STORAGE.522]
 */
HRESULT CDECL IStream16_fnWrite(
        IStream16* iface,const void *pv,ULONG cb,ULONG *pcbWrite
) {
	IStream16Impl *This = (IStream16Impl *)iface;
	BYTE	block[BIGSIZE];
	ULONG	*byteswritten=pcbWrite,xxwritten;
	int	oldsize,newsize,i,curoffset=0,lastblocknr,blocknr,cc;
	const BYTE* pbv = pv;

	if (!pcbWrite) byteswritten=&xxwritten;
	*byteswritten = 0;

	TRACE_(relay)("(%p)->(%p,%d,%p)\n",This,pv,cb,pcbWrite);
	/* do we need to junk some blocks? */
	newsize	= This->offset.u.LowPart+cb;
	oldsize	= This->stde.pps_size;
	if (newsize < oldsize) {
		if (oldsize < 0x1000) {
			/* only small blocks */
			blocknr=STORAGE_get_nth_next_small_blocknr(&This->str,This->stde.pps_sb,newsize/SMALLSIZE);

			assert(blocknr>=0);

			/* will set the rest of the chain to 'free' */
			if (!STORAGE_set_small_chain(&This->str,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
				return E_FAIL;
		} else {
			if (newsize >= 0x1000) {
				blocknr=STORAGE_get_nth_next_big_blocknr(&This->str,This->stde.pps_sb,newsize/BIGSIZE);
				assert(blocknr>=0);

				/* will set the rest of the chain to 'free' */
				if (!STORAGE_set_big_chain(&This->str,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
					return E_FAIL;
			} else {
				/* Migrate large blocks to small blocks
				 * (we just migrate newsize bytes)
				 */
				LPBYTE	curdata,data = HeapAlloc(GetProcessHeap(),0,newsize+BIGSIZE);
				HRESULT r = E_FAIL;

				cc	= newsize;
				blocknr = This->stde.pps_sb;
				curdata = data;
				while (cc>0) {
					if (!STORAGE_get_big_block(&This->str,blocknr,curdata)) {
						HeapFree(GetProcessHeap(),0,data);
						return E_FAIL;
					}
					curdata	+= BIGSIZE;
					cc	-= BIGSIZE;
					blocknr	 = STORAGE_get_next_big_blocknr(&This->str,blocknr);
				}
				/* frees complete chain for this stream */
				if (!STORAGE_set_big_chain(&This->str,This->stde.pps_sb,STORAGE_CHAINENTRY_FREE))
					goto err;
				curdata	= data;
				blocknr = This->stde.pps_sb = STORAGE_get_free_small_blocknr(&This->str);
				if (blocknr<0)
					goto err;
				cc	= newsize;
				while (cc>0) {
					if (!STORAGE_put_small_block(&This->str,blocknr,curdata))
						goto err;
					cc	-= SMALLSIZE;
					if (cc<=0) {
						if (!STORAGE_set_small_chain(&This->str,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
							goto err;
						break;
					} else {
						int newblocknr = STORAGE_get_free_small_blocknr(&This->str);
						if (newblocknr<0)
							goto err;
						if (!STORAGE_set_small_chain(&This->str,blocknr,newblocknr))
							goto err;
						blocknr = newblocknr;
					}
					curdata	+= SMALLSIZE;
				}
				r = S_OK;
			err:
				HeapFree(GetProcessHeap(),0,data);
				if(r != S_OK)
					return r;
			}
		}
		This->stde.pps_size = newsize;
	}

	if (newsize > oldsize) {
		if (oldsize >= 0x1000) {
			/* should return the block right before the 'endofchain' */
			blocknr = STORAGE_get_nth_next_big_blocknr(&This->str,This->stde.pps_sb,This->stde.pps_size/BIGSIZE);
			assert(blocknr>=0);
			lastblocknr	= blocknr;
			for (i=oldsize/BIGSIZE;i<newsize/BIGSIZE;i++) {
				blocknr = STORAGE_get_free_big_blocknr(&This->str);
				if (blocknr<0)
					return E_FAIL;
				if (!STORAGE_set_big_chain(&This->str,lastblocknr,blocknr))
					return E_FAIL;
				lastblocknr = blocknr;
			}
			if (!STORAGE_set_big_chain(&This->str,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
				return E_FAIL;
		} else {
			if (newsize < 0x1000) {
				/* find startblock */
				if (!oldsize)
					This->stde.pps_sb = blocknr = STORAGE_get_free_small_blocknr(&This->str);
				else
					blocknr = STORAGE_get_nth_next_small_blocknr(&This->str,This->stde.pps_sb,This->stde.pps_size/SMALLSIZE);
				if (blocknr<0)
					return E_FAIL;

				/* allocate required new small blocks */
				lastblocknr = blocknr;
				for (i=oldsize/SMALLSIZE;i<newsize/SMALLSIZE;i++) {
					blocknr = STORAGE_get_free_small_blocknr(&This->str);
					if (blocknr<0)
						return E_FAIL;
					if (!STORAGE_set_small_chain(&This->str,lastblocknr,blocknr))
						return E_FAIL;
					lastblocknr = blocknr;
				}
				/* and terminate the chain */
				if (!STORAGE_set_small_chain(&This->str,lastblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
					return E_FAIL;
			} else {
				if (!oldsize) {
					/* no single block allocated yet */
					blocknr=STORAGE_get_free_big_blocknr(&This->str);
					if (blocknr<0)
						return E_FAIL;
					This->stde.pps_sb = blocknr;
				} else {
					/* Migrate small blocks to big blocks */
					LPBYTE	curdata,data = HeapAlloc(GetProcessHeap(),0,oldsize+BIGSIZE);
					HRESULT r = E_FAIL;

					cc	= oldsize;
					blocknr = This->stde.pps_sb;
					curdata = data;
					/* slurp in */
					while (cc>0) {
						if (!STORAGE_get_small_block(&This->str,blocknr,curdata))
							goto err2;
						curdata	+= SMALLSIZE;
						cc	-= SMALLSIZE;
						blocknr	 = STORAGE_get_next_small_blocknr(&This->str,blocknr);
					}
					/* free small block chain */
					if (!STORAGE_set_small_chain(&This->str,This->stde.pps_sb,STORAGE_CHAINENTRY_FREE))
						goto err2;
					curdata	= data;
					blocknr = This->stde.pps_sb = STORAGE_get_free_big_blocknr(&This->str);
					if (blocknr<0)
						goto err2;
					/* put the data into the big blocks */
					cc	= This->stde.pps_size;
					while (cc>0) {
						if (!STORAGE_put_big_block(&This->str,blocknr,curdata))
							goto err2;
						cc	-= BIGSIZE;
						if (cc<=0) {
							if (!STORAGE_set_big_chain(&This->str,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
								goto err2;
							break;
						} else {
							int newblocknr = STORAGE_get_free_big_blocknr(&This->str);
							if (newblocknr<0)
								goto err2;
							if (!STORAGE_set_big_chain(&This->str,blocknr,newblocknr))
								goto err2;
							blocknr = newblocknr;
						}
						curdata	+= BIGSIZE;
					}
					r = S_OK;
				err2:
					HeapFree(GetProcessHeap(),0,data);
					if(r != S_OK)
						return r;
				}
				/* generate big blocks to fit the new data */
				lastblocknr	= blocknr;
				for (i=oldsize/BIGSIZE;i<newsize/BIGSIZE;i++) {
					blocknr = STORAGE_get_free_big_blocknr(&This->str);
					if (blocknr<0)
						return E_FAIL;
					if (!STORAGE_set_big_chain(&This->str,lastblocknr,blocknr))
						return E_FAIL;
					lastblocknr = blocknr;
				}
				/* terminate chain */
				if (!STORAGE_set_big_chain(&This->str,lastblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
					return E_FAIL;
			}
		}
		This->stde.pps_size = newsize;
	}

	/* There are just some cases where we didn't modify it, we write it out
	 * everytime
	 */
	if (!STORAGE_put_pps_entry(&This->str,This->ppsent,&(This->stde)))
		return E_FAIL;

	/* finally the write pass */
	if (This->stde.pps_size < 0x1000) {
		blocknr = STORAGE_get_nth_next_small_blocknr(&This->str,This->stde.pps_sb,This->offset.u.LowPart/SMALLSIZE);
		assert(blocknr>=0);
		while (cb>0) {
			/* we ensured that it is allocated above */
			assert(blocknr>=0);
			/* Read old block everytime, since we can have
			 * overlapping data at START and END of the write
			 */
			if (!STORAGE_get_small_block(&This->str,blocknr,block))
				return E_FAIL;

			cc = SMALLSIZE-(This->offset.u.LowPart&(SMALLSIZE-1));
			if (cc>cb)
				cc=cb;
			memcpy(	((LPBYTE)block)+(This->offset.u.LowPart&(SMALLSIZE-1)),
				pbv+curoffset,
				cc
			);
			if (!STORAGE_put_small_block(&This->str,blocknr,block))
				return E_FAIL;
			cb			-= cc;
			curoffset		+= cc;
			pbv			+= cc;
			This->offset.u.LowPart	+= cc;
			*byteswritten		+= cc;
			blocknr = STORAGE_get_next_small_blocknr(&This->str,blocknr);
		}
	} else {
		blocknr = STORAGE_get_nth_next_big_blocknr(&This->str,This->stde.pps_sb,This->offset.u.LowPart/BIGSIZE);
		assert(blocknr>=0);
		while (cb>0) {
			/* we ensured that it is allocated above, so it better is */
			assert(blocknr>=0);
			/* read old block everytime, since we can have
			 * overlapping data at START and END of the write
			 */
			if (!STORAGE_get_big_block(&This->str,blocknr,block))
				return E_FAIL;

			cc = BIGSIZE-(This->offset.u.LowPart&(BIGSIZE-1));
			if (cc>cb)
				cc=cb;
			memcpy(	((LPBYTE)block)+(This->offset.u.LowPart&(BIGSIZE-1)),
				pbv+curoffset,
				cc
			);
			if (!STORAGE_put_big_block(&This->str,blocknr,block))
				return E_FAIL;
			cb			-= cc;
			curoffset		+= cc;
			pbv			+= cc;
			This->offset.u.LowPart	+= cc;
			*byteswritten		+= cc;
			blocknr = STORAGE_get_next_big_blocknr(&This->str,blocknr);
		}
	}
	return S_OK;
}

/******************************************************************************
 *		_create_istream16	[Internal]
 */
static void _create_istream16(LPSTREAM16 *str) {
	IStream16Impl*	lpst;

	if (!strvt16.QueryInterface) {
		HMODULE16	wp = GetModuleHandle16("STORAGE");
		if (wp>=32) {
		  /* FIXME: what is This GetProcAddress16. Should the name be IStream16_QueryInterface of IStream16_fnQueryInterface */
#define VTENT(xfn)  strvt16.xfn = (void*)GetProcAddress16(wp,"IStream16_"#xfn);assert(strvt16.xfn)
			VTENT(QueryInterface);
			VTENT(AddRef);
			VTENT(Release);
			VTENT(Read);
			VTENT(Write);
			VTENT(Seek);
			VTENT(SetSize);
			VTENT(CopyTo);
			VTENT(Commit);
			VTENT(Revert);
			VTENT(LockRegion);
			VTENT(UnlockRegion);
			VTENT(Stat);
			VTENT(Clone);
#undef VTENT
			segstrvt16 = (const IStream16Vtbl*)MapLS( &strvt16 );
		} else {
#define VTENT(xfn) strvt16.xfn = IStream16_fn##xfn;
			VTENT(QueryInterface);
			VTENT(AddRef);
			VTENT(Release);
			VTENT(Read);
			VTENT(Write);
			VTENT(Seek);
	/*
			VTENT(CopyTo);
			VTENT(Commit);
			VTENT(SetSize);
			VTENT(Revert);
			VTENT(LockRegion);
			VTENT(UnlockRegion);
			VTENT(Stat);
			VTENT(Clone);
	*/
#undef VTENT
			segstrvt16 = &strvt16;
		}
	}
	lpst = HeapAlloc( GetProcessHeap(), 0, sizeof(*lpst) );
	lpst->lpVtbl	= segstrvt16;
	lpst->ref	= 1;
	lpst->thisptr	= MapLS( lpst );
	lpst->str.hf	= NULL;
	lpst->str.lockbytes	= 0;
	*str = (void*)lpst->thisptr;
}


/* --- IStream32 implementation */

typedef struct
{
        /* IUnknown fields */
        const IStreamVtbl              *lpVtbl;
        LONG                            ref;
        /* IStream32 fields */
        struct storage_pps_entry        stde;
        int                             ppsent;
        HANDLE                          hf;
        ULARGE_INTEGER                  offset;
} IStream32Impl;

/******************************************************************************
 *		IStorage16_QueryInterface	[STORAGE.500]
 */
HRESULT CDECL IStorage16_fnQueryInterface(
	IStorage16* iface,REFIID refiid,LPVOID *obj
) {
	IStorage16Impl *This = (IStorage16Impl *)iface;

	TRACE_(relay)("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);

	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = This;
		return 0;
	}
	return OLE_E_ENUM_NOMORE;
}

/******************************************************************************
 * IStorage16_AddRef [STORAGE.501]
 */
ULONG CDECL IStorage16_fnAddRef(IStorage16* iface) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
	return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 * IStorage16_Release [STORAGE.502]
 */
ULONG CDECL IStorage16_fnRelease(IStorage16* iface) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
        ULONG ref;
        ref = InterlockedDecrement(&This->ref);
        if (!ref)
        {
            UnMapLS( This->thisptr );
            HeapFree( GetProcessHeap(), 0, This );
        }
        return ref;
}

/******************************************************************************
 * IStorage16_Stat [STORAGE.517]
 */
HRESULT CDECL IStorage16_fnStat(
        LPSTORAGE16 iface,STATSTG16 *pstatstg, DWORD grfStatFlag
) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
        DWORD len = WideCharToMultiByte( CP_ACP, 0, This->stde.pps_rawname, -1, NULL, 0, NULL, NULL );
        LPSTR nameA = HeapAlloc( GetProcessHeap(), 0, len );

	TRACE("(%p)->(%p,0x%08x)\n",
		This,pstatstg,grfStatFlag
	);
        WideCharToMultiByte( CP_ACP, 0, This->stde.pps_rawname, -1, nameA, len, NULL, NULL );
	pstatstg->pwcsName=(LPOLESTR16)MapLS( nameA );
	pstatstg->type = This->stde.pps_type;
	pstatstg->cbSize.u.LowPart = This->stde.pps_size;
	pstatstg->mtime = This->stde.pps_ft1; /* FIXME */ /* why? */
	pstatstg->atime = This->stde.pps_ft2; /* FIXME */
	pstatstg->ctime = This->stde.pps_ft2; /* FIXME */
	pstatstg->grfMode	= 0; /* FIXME */
	pstatstg->grfLocksSupported = 0; /* FIXME */
	pstatstg->clsid		= This->stde.pps_guid;
	pstatstg->grfStateBits	= 0; /* FIXME */
	pstatstg->reserved	= 0;
	return S_OK;
}

/******************************************************************************
 *		IStorage16_Commit	[STORAGE.509]
 */
HRESULT CDECL IStorage16_fnCommit(
        LPSTORAGE16 iface,DWORD commitflags
) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
	FIXME("(%p)->(0x%08x),STUB!\n",
		This,commitflags
	);
	return S_OK;
}

/******************************************************************************
 * IStorage16_CopyTo [STORAGE.507]
 */
HRESULT CDECL IStorage16_fnCopyTo(LPSTORAGE16 iface,DWORD ciidExclude,const IID *rgiidExclude,SNB16 SNB16Exclude,IStorage16 *pstgDest) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
	FIXME("IStorage16(%p)->(0x%08x,%s,%p,%p),stub!\n",
		This,ciidExclude,debugstr_guid(rgiidExclude),SNB16Exclude,pstgDest
	);
	return S_OK;
}


/******************************************************************************
 * IStorage16_CreateStorage [STORAGE.505]
 */
HRESULT CDECL IStorage16_fnCreateStorage(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName,DWORD grfMode,DWORD dwStgFormat,DWORD reserved2, IStorage16 **ppstg
) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
	IStorage16Impl*	lpstg;
	int		ppsent,x;
	struct storage_pps_entry	stde;
	struct storage_header sth;
	BOOL ret;
	int	 nPPSEntries;

	READ_HEADER(&This->str);
	TRACE("(%p)->(%s,0x%08x,0x%08x,0x%08x,%p)\n",
		This,pwcsName,grfMode,dwStgFormat,reserved2,ppstg
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME("We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istorage16(ppstg);
	lpstg = MapSL((SEGPTR)*ppstg);
	if (This->str.hf) {
	    DuplicateHandle( GetCurrentProcess(), This->str.hf, GetCurrentProcess(),
			     &lpstg->str.hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
	} else {
	    lpstg->str.lockbytes = This->str.lockbytes;
	    _ilockbytes16_addref(This->str.lockbytes);
	}

	ppsent=STORAGE_get_free_pps_entry(&lpstg->str);
	if (ppsent<0)
		return E_FAIL;
	stde=This->stde;
	if (stde.pps_dir==-1) {
		stde.pps_dir = ppsent;
		x = This->ppsent;
	} else {
		FIXME(" use prev chain too ?\n");
		x=stde.pps_dir;
		if (1!=STORAGE_get_pps_entry(&lpstg->str,x,&stde))
			return E_FAIL;
		while (stde.pps_next!=-1) {
			x=stde.pps_next;
			if (1!=STORAGE_get_pps_entry(&lpstg->str,x,&stde))
				return E_FAIL;
		}
		stde.pps_next = ppsent;
	}
	ret = STORAGE_put_pps_entry(&lpstg->str,x,&stde);
	assert(ret);
	nPPSEntries = STORAGE_get_pps_entry(&lpstg->str,ppsent,&(lpstg->stde));
	assert(nPPSEntries == 1);
        MultiByteToWideChar( CP_ACP, 0, pwcsName, -1, lpstg->stde.pps_rawname,
                             sizeof(lpstg->stde.pps_rawname)/sizeof(WCHAR));
	lpstg->stde.pps_sizeofname = (strlenW(lpstg->stde.pps_rawname)+1)*sizeof(WCHAR);
	lpstg->stde.pps_next	= -1;
	lpstg->stde.pps_prev	= -1;
	lpstg->stde.pps_dir	= -1;
	lpstg->stde.pps_sb	= -1;
	lpstg->stde.pps_size	=  0;
	lpstg->stde.pps_type	=  1;
	lpstg->ppsent		= ppsent;
	/* FIXME: timestamps? */
	if (!STORAGE_put_pps_entry(&lpstg->str,ppsent,&(lpstg->stde)))
		return E_FAIL;
	return S_OK;
}

/******************************************************************************
 *		IStorage16_CreateStream	[STORAGE.503]
 */
HRESULT CDECL IStorage16_fnCreateStream(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2, IStream16 **ppstm
) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
	IStream16Impl*	lpstr;
	int		ppsent,x;
	struct storage_pps_entry	stde;
	BOOL ret;
	int	 nPPSEntries;

	TRACE("(%p)->(%s,0x%08x,0x%08x,0x%08x,%p)\n",
		This,pwcsName,grfMode,reserved1,reserved2,ppstm
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME("We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istream16(ppstm);
	lpstr = MapSL((SEGPTR)*ppstm);
	if (This->str.hf) {
	    DuplicateHandle( GetCurrentProcess(), This->str.hf, GetCurrentProcess(),
			     &lpstr->str.hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
	} else {
	    lpstr->str.lockbytes = This->str.lockbytes;
	    _ilockbytes16_addref(This->str.lockbytes);
	}
	lpstr->offset.u.LowPart	= 0;
	lpstr->offset.u.HighPart= 0;

	ppsent=STORAGE_get_free_pps_entry(&lpstr->str);
	if (ppsent<0)
		return E_FAIL;
	stde=This->stde;
	if (stde.pps_next==-1)
		x=This->ppsent;
	else
		while (stde.pps_next!=-1) {
			x=stde.pps_next;
			if (1!=STORAGE_get_pps_entry(&lpstr->str,x,&stde))
				return E_FAIL;
		}
	stde.pps_next = ppsent;
	ret = STORAGE_put_pps_entry(&lpstr->str,x,&stde);
	assert(ret);
	nPPSEntries = STORAGE_get_pps_entry(&lpstr->str,ppsent,&(lpstr->stde));
	assert(nPPSEntries == 1);
        MultiByteToWideChar( CP_ACP, 0, pwcsName, -1, lpstr->stde.pps_rawname,
                             sizeof(lpstr->stde.pps_rawname)/sizeof(WCHAR));
	lpstr->stde.pps_sizeofname = (strlenW(lpstr->stde.pps_rawname)+1) * sizeof(WCHAR);
	lpstr->stde.pps_next	= -1;
	lpstr->stde.pps_prev	= -1;
	lpstr->stde.pps_dir	= -1;
	lpstr->stde.pps_sb	= -1;
	lpstr->stde.pps_size	=  0;
	lpstr->stde.pps_type	=  2;
	lpstr->ppsent		= ppsent;

	/* FIXME: timestamps? */
	if (!STORAGE_put_pps_entry(&lpstr->str,ppsent,&(lpstr->stde)))
		return E_FAIL;
	return S_OK;
}

/******************************************************************************
 *		IStorage16_OpenStorage	[STORAGE.506]
 */
HRESULT CDECL IStorage16_fnOpenStorage(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName, IStorage16 *pstgPrio, DWORD grfMode, SNB16 snbExclude, DWORD reserved, IStorage16 **ppstg
) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
	IStream16Impl*	lpstg;
	WCHAR		name[33];
	int		newpps;

	TRACE("(%p)->(%s,%p,0x%08x,%p,0x%08x,%p)\n",
		This,pwcsName,pstgPrio,grfMode,snbExclude,reserved,ppstg
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME("We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istorage16(ppstg);
	lpstg = MapSL((SEGPTR)*ppstg);
	if (This->str.hf) {
	    DuplicateHandle( GetCurrentProcess(), This->str.hf, GetCurrentProcess(),
			     &lpstg->str.hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
	} else {
	    lpstg->str.lockbytes = This->str.lockbytes;
	    _ilockbytes16_addref(This->str.lockbytes);
	}
        MultiByteToWideChar( CP_ACP, 0, pwcsName, -1, name, sizeof(name)/sizeof(WCHAR));
	newpps = STORAGE_look_for_named_pps(&lpstg->str,This->stde.pps_dir,name);
	if (newpps==-1) {
		IStream16_fnRelease((IStream16*)lpstg);
		return E_FAIL;
	}

	if (1!=STORAGE_get_pps_entry(&lpstg->str,newpps,&(lpstg->stde))) {
		IStream16_fnRelease((IStream16*)lpstg);
		return E_FAIL;
	}
	lpstg->ppsent		= newpps;
	return S_OK;
}

/******************************************************************************
 * IStorage16_OpenStream [STORAGE.504]
 */
HRESULT CDECL IStorage16_fnOpenStream(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream16 **ppstm
) {
	IStorage16Impl *This = (IStorage16Impl *)iface;
	IStream16Impl*	lpstr;
	WCHAR		name[33];
	int		newpps;

	TRACE("(%p)->(%s,%p,0x%08x,0x%08x,%p)\n",
		This,pwcsName,reserved1,grfMode,reserved2,ppstm
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME("We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istream16(ppstm);
	lpstr = MapSL((SEGPTR)*ppstm);
	if (This->str.hf) {
	    DuplicateHandle( GetCurrentProcess(), This->str.hf, GetCurrentProcess(),
			     &lpstr->str.hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
	} else {
	    lpstr->str.lockbytes = This->str.lockbytes;
	    _ilockbytes16_addref(This->str.lockbytes);
	}
        MultiByteToWideChar( CP_ACP, 0, pwcsName, -1, name, sizeof(name)/sizeof(WCHAR));
	newpps = STORAGE_look_for_named_pps(&lpstr->str,This->stde.pps_dir,name);
	if (newpps==-1) {
		IStream16_fnRelease((IStream16*)lpstr);
		return E_FAIL;
	}

	if (1!=STORAGE_get_pps_entry(&lpstr->str,newpps,&(lpstr->stde))) {
		IStream16_fnRelease((IStream16*)lpstr);
		return E_FAIL;
	}
	lpstr->offset.u.LowPart		= 0;
	lpstr->offset.u.HighPart	= 0;
	lpstr->ppsent			= newpps;
	return S_OK;
}

/******************************************************************************
 * _create_istorage16 [INTERNAL]
 */
static void _create_istorage16(LPSTORAGE16 *stg) {
	IStorage16Impl*	lpst;

	if (!stvt16.QueryInterface) {
		HMODULE16	wp = GetModuleHandle16("STORAGE");
		if (wp>=32) {
#define VTENT(xfn)  stvt16.xfn = (void*)GetProcAddress16(wp,"IStorage16_"#xfn);
			VTENT(QueryInterface)
			VTENT(AddRef)
			VTENT(Release)
			VTENT(CreateStream)
			VTENT(OpenStream)
			VTENT(CreateStorage)
			VTENT(OpenStorage)
			VTENT(CopyTo)
			VTENT(MoveElementTo)
			VTENT(Commit)
			VTENT(Revert)
			VTENT(EnumElements)
			VTENT(DestroyElement)
			VTENT(RenameElement)
			VTENT(SetElementTimes)
			VTENT(SetClass)
			VTENT(SetStateBits)
			VTENT(Stat)
#undef VTENT
			segstvt16 = (const IStorage16Vtbl*)MapLS( &stvt16 );
		} else {
#define VTENT(xfn) stvt16.xfn = IStorage16_fn##xfn;
			VTENT(QueryInterface)
			VTENT(AddRef)
			VTENT(Release)
			VTENT(CreateStream)
			VTENT(OpenStream)
			VTENT(CreateStorage)
			VTENT(OpenStorage)
			VTENT(CopyTo)
			VTENT(Commit)
	/*  not (yet) implemented ...
			VTENT(MoveElementTo)
			VTENT(Revert)
			VTENT(EnumElements)
			VTENT(DestroyElement)
			VTENT(RenameElement)
			VTENT(SetElementTimes)
			VTENT(SetClass)
			VTENT(SetStateBits)
			VTENT(Stat)
	*/
#undef VTENT
			segstvt16 = &stvt16;
		}
	}
	lpst = HeapAlloc( GetProcessHeap(), 0, sizeof(*lpst) );
	lpst->lpVtbl	= segstvt16;
	lpst->str.hf	= NULL;
	lpst->str.lockbytes	= 0;
	lpst->ref	= 1;
	lpst->thisptr	= MapLS(lpst);
	*stg = (void*)lpst->thisptr;
}

/******************************************************************************
 *	Storage API functions
 */

/******************************************************************************
 *		StgCreateDocFileA	[STORAGE.1]
 */
HRESULT WINAPI StgCreateDocFile16(
	LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved,IStorage16 **ppstgOpen
) {
	HANDLE		hf;
	int		i,ret;
	IStorage16Impl*	lpstg;
	struct storage_pps_entry	stde;

	TRACE("(%s,0x%08x,0x%08x,%p)\n",
		pwcsName,grfMode,reserved,ppstgOpen
	);
	_create_istorage16(ppstgOpen);
	hf = CreateFileA(pwcsName,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_NEW,0,0);
	if (hf==INVALID_HANDLE_VALUE) {
		WARN("couldn't open file for storage:%d\n",GetLastError());
		return E_FAIL;
	}
	lpstg = MapSL((SEGPTR)*ppstgOpen);
	lpstg->str.hf = hf;
	lpstg->str.lockbytes = 0;
	/* FIXME: check for existence before overwriting? */
	if (!STORAGE_init_storage(&lpstg->str)) {
		CloseHandle(hf);
		return E_FAIL;
	}
	i=0;ret=0;
	while (!ret) { /* neither 1 nor <0 */
		ret=STORAGE_get_pps_entry(&lpstg->str,i,&stde);
		if ((ret==1) && (stde.pps_type==5)) {
			lpstg->stde	= stde;
			lpstg->ppsent	= i;
			break;
		}
		i++;
	}
	if (ret!=1) {
		IStorage16_fnRelease((IStorage16*)lpstg); /* will remove it */
		return E_FAIL;
	}

	return S_OK;
}

/******************************************************************************
 * StgIsStorageFile [STORAGE.5]
 */
HRESULT WINAPI StgIsStorageFile16(LPCOLESTR16 fn) {
	UNICODE_STRING strW;
	HRESULT ret;

	RtlCreateUnicodeStringFromAsciiz(&strW, fn);
	ret = StgIsStorageFile( strW.Buffer );
	RtlFreeUnicodeString( &strW );

	return ret;
}

/******************************************************************************
 * StgOpenStorage [STORAGE.3]
 */
HRESULT WINAPI StgOpenStorage16(
	LPCOLESTR16 pwcsName,IStorage16 *pstgPriority,DWORD grfMode,
	SNB16 snbExclude,DWORD reserved, IStorage16 **ppstgOpen
) {
	HANDLE		hf;
	int		ret,i;
	IStorage16Impl*	lpstg;
	struct storage_pps_entry	stde;

	TRACE("(%s,%p,0x%08x,%p,%d,%p)\n",
              pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstgOpen
	);
	_create_istorage16(ppstgOpen);
	hf = CreateFileA(pwcsName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (hf==INVALID_HANDLE_VALUE) {
		WARN("Couldn't open file for storage\n");
		return E_FAIL;
	}
	lpstg = MapSL((SEGPTR)*ppstgOpen);
	lpstg->str.hf = hf;

	i=0;ret=0;
	while (!ret) { /* neither 1 nor <0 */
		ret=STORAGE_get_pps_entry(&lpstg->str,i,&stde);
		if ((ret==1) && (stde.pps_type==5)) {
			lpstg->stde=stde;
			break;
		}
		i++;
	}
	if (ret!=1) {
		IStorage16_fnRelease((IStorage16*)lpstg); /* will remove it */
		return E_FAIL;
	}
	return S_OK;

}

/******************************************************************************
 *              StgIsStorageILockBytes        [STORAGE.6]
 *
 * Determines if the ILockBytes contains a storage object.
 */
HRESULT WINAPI StgIsStorageILockBytes16(SEGPTR plkbyt)
{
  DWORD args[6];
  HRESULT hres;
  HANDLE16 hsig;
  
  args[0] = (DWORD)plkbyt;	/* iface */
  args[1] = args[2] = 0;	/* ULARGE_INTEGER offset */
  args[3] = WOWGlobalAllocLock16( 0, 8, &hsig ); /* sig */
  args[4] = 8;
  args[5] = 0;

  if (!WOWCallback16Ex(
      (DWORD)((const ILockBytes16Vtbl*)MapSL(
                  (SEGPTR)((LPLOCKBYTES16)MapSL(plkbyt))->lpVtbl)
      )->ReadAt,
      WCB16_PASCAL,
      6*sizeof(DWORD),
      (LPVOID)args,
      (LPDWORD)&hres
  )) {
      ERR("CallTo16 ILockBytes16::ReadAt() failed, hres %x\n",hres);
      return hres;
  }
  if (memcmp(MapSL(args[3]), STORAGE_magic, sizeof(STORAGE_magic)) == 0) {
    WOWGlobalUnlockFree16(args[3]);
    return S_OK;
  }
  WOWGlobalUnlockFree16(args[3]);
  return S_FALSE;
}

/******************************************************************************
 *    StgOpenStorageOnILockBytes    [STORAGE.4]
 *
 * PARAMS
 *  plkbyt  FIXME: Should probably be an ILockBytes16 *.
 */
HRESULT WINAPI StgOpenStorageOnILockBytes16(
	SEGPTR plkbyt,
	IStorage16 *pstgPriority,
	DWORD grfMode,
	SNB16 snbExclude,
	DWORD reserved,
	IStorage16 **ppstgOpen)
{
	IStorage16Impl*	lpstg;
	int i,ret;
	struct storage_pps_entry	stde;

	FIXME("(%x, %p, 0x%08x, %d, %x, %p)\n", plkbyt, pstgPriority, grfMode, (int)snbExclude, reserved, ppstgOpen);
	if ((plkbyt == 0) || (ppstgOpen == 0))
		return STG_E_INVALIDPOINTER;

	*ppstgOpen = 0;

	_create_istorage16(ppstgOpen);
	lpstg = MapSL((SEGPTR)*ppstgOpen);
	lpstg->str.hf = NULL;
	lpstg->str.lockbytes = plkbyt;
	i=0;ret=0;
	while (!ret) { /* neither 1 nor <0 */
		ret=STORAGE_get_pps_entry(&lpstg->str,i,&stde);
		if ((ret==1) && (stde.pps_type==5)) {
			lpstg->stde=stde;
			break;
		}
		i++;
	}
	if (ret!=1) {
		IStorage16_fnRelease((IStorage16*)lpstg); /* will remove it */
		return E_FAIL;
	}
	return S_OK;
}

/***********************************************************************
 *    ReadClassStg (OLE2.18)
 *
 * This method reads the CLSID previously written to a storage object with
 * the WriteClassStg.
 *
 * PARAMS
 *  pstg    [I] Segmented LPSTORAGE pointer.
 *  pclsid  [O] Pointer to where the CLSID is written
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI ReadClassStg16(SEGPTR pstg, CLSID *pclsid)
{
	STATSTG16 statstg;
	HANDLE16 hstatstg;
	HRESULT	hres;
	DWORD args[3];

	TRACE("(%x, %p)\n", pstg, pclsid);

	if(pclsid==NULL)
		return E_POINTER;
	/*
	 * read a STATSTG structure (contains the clsid) from the storage
	 */
	args[0] = (DWORD)pstg;	/* iface */
	args[1] = WOWGlobalAllocLock16( 0, sizeof(STATSTG16), &hstatstg );
	args[2] = STATFLAG_DEFAULT;

	if (!WOWCallback16Ex(
	    (DWORD)((const IStorage16Vtbl*)MapSL(
			(SEGPTR)((LPSTORAGE16)MapSL(pstg))->lpVtbl)
	    )->Stat,
	    WCB16_PASCAL,
	    3*sizeof(DWORD),
	    (LPVOID)args,
	    (LPDWORD)&hres
	)) {
	    WOWGlobalUnlockFree16(args[1]);
            ERR("CallTo16 IStorage16::Stat() failed, hres %x\n",hres);
	    return hres;
	}
	memcpy(&statstg, MapSL(args[1]), sizeof(STATSTG16));
	WOWGlobalUnlockFree16(args[1]);

	if(SUCCEEDED(hres)) {
		*pclsid=statstg.clsid;
		TRACE("clsid is %s\n", debugstr_guid(&statstg.clsid));
	}
	return hres;
}

/***********************************************************************
 *              GetConvertStg (OLE2.82)
 */
HRESULT WINAPI GetConvertStg16(LPSTORAGE stg) {
    FIXME("unimplemented stub!\n");
    return E_FAIL;
}
