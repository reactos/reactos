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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "winreg.h"
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

#define READ_HEADER	STORAGE_get_big_block(hf,-1,(LPBYTE)&sth);assert(!memcmp(STORAGE_magic,sth.magic,sizeof(STORAGE_magic)));
static ICOM_VTABLE(IStorage16) stvt16;
static ICOM_VTABLE(IStorage16) *segstvt16 = NULL;
static ICOM_VTABLE(IStream16) strvt16;
static ICOM_VTABLE(IStream16) *segstrvt16 = NULL;

/*ULONG WINAPI IStorage16_AddRef(LPSTORAGE16 this);*/
static void _create_istorage16(LPSTORAGE16 *stg);
static void _create_istream16(LPSTREAM16 *str);

#define IMPLEMENTED 1


/******************************************************************************
 *		STORAGE_get_big_block	[Internal]
 *
 * Reading OLE compound storage
 */
static BOOL
STORAGE_get_big_block(HANDLE hf,int n,BYTE *block)
{
    DWORD result;

    assert(n>=-1);
    if ((SetFilePointer( hf, (n+1)*BIGSIZE, NULL,
                         SEEK_SET ) == INVALID_SET_FILE_POINTER) && GetLastError())
    {
        WARN(" seek failed (%ld)\n",GetLastError());
        return FALSE;
    }
    if (!ReadFile( hf, block, BIGSIZE, &result, NULL ) || result != BIGSIZE)
    {
        WARN("(block size %d): read didn't read (%ld)\n",n,GetLastError());
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * STORAGE_put_big_block [INTERNAL]
 */
static BOOL
STORAGE_put_big_block(HANDLE hf,int n,BYTE *block)
{
    DWORD result;

    assert(n>=-1);
    if ((SetFilePointer( hf, (n+1)*BIGSIZE, NULL,
                         SEEK_SET ) == INVALID_SET_FILE_POINTER) && GetLastError())
    {
        WARN("seek failed (%ld)\n",GetLastError());
        return FALSE;
    }
    if (!WriteFile( hf, block, BIGSIZE, &result, NULL ) || result != BIGSIZE)
    {
        WARN(" write failed (%ld)\n",GetLastError());
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * STORAGE_get_next_big_blocknr [INTERNAL]
 */
static int
STORAGE_get_next_big_blocknr(HANDLE hf,int blocknr) {
	INT	bbs[BIGSIZE/sizeof(INT)];
	struct	storage_header	sth;

	READ_HEADER;

	assert(blocknr>>7<sth.num_of_bbd_blocks);
	if (sth.bbd_list[blocknr>>7]==0xffffffff)
		return -5;
	if (!STORAGE_get_big_block(hf,sth.bbd_list[blocknr>>7],(LPBYTE)bbs))
		return -5;
	assert(bbs[blocknr&0x7f]!=STORAGE_CHAINENTRY_FREE);
	return bbs[blocknr&0x7f];
}

/******************************************************************************
 * STORAGE_get_nth_next_big_blocknr [INTERNAL]
 */
static int
STORAGE_get_nth_next_big_blocknr(HANDLE hf,int blocknr,int nr) {
	INT	bbs[BIGSIZE/sizeof(INT)];
	int	lastblock = -1;
	struct storage_header sth;

	READ_HEADER;

	assert(blocknr>=0);
	while (nr--) {
		assert((blocknr>>7)<sth.num_of_bbd_blocks);
		assert(sth.bbd_list[blocknr>>7]!=0xffffffff);

		/* simple caching... */
		if (lastblock!=sth.bbd_list[blocknr>>7]) {
			BOOL ret = STORAGE_get_big_block(hf,sth.bbd_list[blocknr>>7],(LPBYTE)bbs);
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
STORAGE_get_root_pps_entry(HANDLE hf,struct storage_pps_entry *pstde) {
	int	blocknr,i;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry	*stde=(struct storage_pps_entry*)block;
	struct storage_header sth;

	READ_HEADER;
	blocknr = sth.root_startblock;
	while (blocknr>=0) {
		BOOL ret = STORAGE_get_big_block(hf,blocknr,block);
		assert(ret);
		for (i=0;i<4;i++) {
			if (!stde[i].pps_sizeofname)
				continue;
			if (stde[i].pps_type==5) {
				*pstde=stde[i];
				return TRUE;
			}
		}
		blocknr=STORAGE_get_next_big_blocknr(hf,blocknr);
	}
	return FALSE;
}

/******************************************************************************
 * STORAGE_get_small_block [INTERNAL]
 */
static BOOL
STORAGE_get_small_block(HANDLE hf,int blocknr,BYTE *sblock) {
	BYTE				block[BIGSIZE];
	int				bigblocknr;
	struct storage_pps_entry	root;
	BOOL ret;

	assert(blocknr>=0);
	ret = STORAGE_get_root_pps_entry(hf,&root);
	assert(ret);
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,root.pps_sb,blocknr/SMALLBLOCKS_PER_BIGBLOCK);
	assert(bigblocknr>=0);
	ret = STORAGE_get_big_block(hf,bigblocknr,block);
	assert(ret);

	memcpy(sblock,((LPBYTE)block)+SMALLSIZE*(blocknr&(SMALLBLOCKS_PER_BIGBLOCK-1)),SMALLSIZE);
	return TRUE;
}

/******************************************************************************
 * STORAGE_put_small_block [INTERNAL]
 */
static BOOL
STORAGE_put_small_block(HANDLE hf,int blocknr,BYTE *sblock) {
	BYTE				block[BIGSIZE];
	int				bigblocknr;
	struct storage_pps_entry	root;
	BOOL ret;

	assert(blocknr>=0);

	ret = STORAGE_get_root_pps_entry(hf,&root);
	assert(ret);
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,root.pps_sb,blocknr/SMALLBLOCKS_PER_BIGBLOCK);
	assert(bigblocknr>=0);
	ret = STORAGE_get_big_block(hf,bigblocknr,block);
	assert(ret);

	memcpy(((LPBYTE)block)+SMALLSIZE*(blocknr&(SMALLBLOCKS_PER_BIGBLOCK-1)),sblock,SMALLSIZE);
	ret = STORAGE_put_big_block(hf,bigblocknr,block);
	assert(ret);
	return TRUE;
}

/******************************************************************************
 * STORAGE_get_next_small_blocknr [INTERNAL]
 */
static int
STORAGE_get_next_small_blocknr(HANDLE hf,int blocknr) {
	BYTE				block[BIGSIZE];
	LPINT				sbd = (LPINT)block;
	int				bigblocknr;
	struct storage_header		sth;
	BOOL ret;

	READ_HEADER;
	assert(blocknr>=0);
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
	assert(bigblocknr>=0);
	ret = STORAGE_get_big_block(hf,bigblocknr,block);
	assert(ret);
	assert(sbd[blocknr & 127]!=STORAGE_CHAINENTRY_FREE);
	return sbd[blocknr & (128-1)];
}

/******************************************************************************
 * STORAGE_get_nth_next_small_blocknr [INTERNAL]
 */
static int
STORAGE_get_nth_next_small_blocknr(HANDLE hf,int blocknr,int nr) {
	int	lastblocknr=-1;
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	struct storage_header sth;
	BOOL ret;

	READ_HEADER;
	assert(blocknr>=0);
	while ((nr--) && (blocknr>=0)) {
		if (lastblocknr/128!=blocknr/128) {
			int	bigblocknr;
			bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
			assert(bigblocknr>=0);
			ret = STORAGE_get_big_block(hf,bigblocknr,block);
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
STORAGE_get_pps_entry(HANDLE hf,int n,struct storage_pps_entry *pstde) {
	int	blocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)(((LPBYTE)block)+128*(n&3));
	struct storage_header sth;
	BOOL ret;

	READ_HEADER;
	/* we have 4 pps entries per big block */
	blocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.root_startblock,n/4);
	assert(blocknr>=0);
	ret = STORAGE_get_big_block(hf,blocknr,block);
	assert(ret);

	*pstde=*stde;
	return 1;
}

/******************************************************************************
 *		STORAGE_put_pps_entry	[Internal]
 */
static int
STORAGE_put_pps_entry(HANDLE hf,int n,struct storage_pps_entry *pstde) {
	int	blocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)(((LPBYTE)block)+128*(n&3));
	struct storage_header sth;
	BOOL ret;

	READ_HEADER;

	/* we have 4 pps entries per big block */
	blocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.root_startblock,n/4);
	assert(blocknr>=0);
	ret = STORAGE_get_big_block(hf,blocknr,block);
	assert(ret);
	*stde=*pstde;
	ret = STORAGE_put_big_block(hf,blocknr,block);
	assert(ret);
	return 1;
}

/******************************************************************************
 *		STORAGE_look_for_named_pps	[Internal]
 */
static int
STORAGE_look_for_named_pps(HANDLE hf,int n,LPOLESTR name) {
	struct storage_pps_entry	stde;
	int				ret;

	if (n==-1)
		return -1;
	if (1!=STORAGE_get_pps_entry(hf,n,&stde))
		return -1;

	if (!lstrcmpW(name,stde.pps_rawname))
		return n;
	if (stde.pps_prev != -1) {
		ret=STORAGE_look_for_named_pps(hf,stde.pps_prev,name);
		if (ret!=-1)
			return ret;
	}
	if (stde.pps_next != -1) {
		ret=STORAGE_look_for_named_pps(hf,stde.pps_next,name);
		if (ret!=-1)
			return ret;
	}
	return -1;
}

/******************************************************************************
 *		STORAGE_dump_pps_entry	[Internal]
 *
 * FIXME
 *    Function is unused
 */
void
STORAGE_dump_pps_entry(struct storage_pps_entry *stde) {
    char	name[33];

    WideCharToMultiByte( CP_ACP, 0, stde->pps_rawname, -1, name, sizeof(name), NULL, NULL);
	if (!stde->pps_sizeofname)
		return;
	DPRINTF("name: %s\n",name);
	DPRINTF("type: %d\n",stde->pps_type);
	DPRINTF("prev pps: %ld\n",stde->pps_prev);
	DPRINTF("next pps: %ld\n",stde->pps_next);
	DPRINTF("dir pps: %ld\n",stde->pps_dir);
	DPRINTF("guid: %s\n",debugstr_guid(&(stde->pps_guid)));
	if (stde->pps_type !=2) {
		time_t	t;
                DWORD dw;
		RtlTimeToSecondsSince1970((LARGE_INTEGER *)&(stde->pps_ft1),&dw);
                t = dw;
		DPRINTF("ts1: %s\n",ctime(&t));
		RtlTimeToSecondsSince1970((LARGE_INTEGER *)&(stde->pps_ft2),&dw);
                t = dw;
		DPRINTF("ts2: %s\n",ctime(&t));
	}
	DPRINTF("startblock: %ld\n",stde->pps_sb);
	DPRINTF("size: %ld\n",stde->pps_size);
}

/******************************************************************************
 * STORAGE_init_storage [INTERNAL]
 */
static BOOL
STORAGE_init_storage(HANDLE hf) {
	BYTE	block[BIGSIZE];
	LPDWORD	bbs;
	struct storage_header *sth;
	struct storage_pps_entry *stde;
        DWORD result;

        SetFilePointer( hf, 0, NULL, SEEK_SET );
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
        if (!WriteFile( hf, block, BIGSIZE, &result, NULL ) || result != BIGSIZE) return FALSE;
	/* block 0 is the big block directory */
	bbs=(LPDWORD)block;
	memset(block,0xff,sizeof(block)); /* mark all blocks as free */
	bbs[0]=STORAGE_CHAINENTRY_ENDOFCHAIN; /* for this block */
	bbs[1]=STORAGE_CHAINENTRY_ENDOFCHAIN; /* for directory entry */
        if (!WriteFile( hf, block, BIGSIZE, &result, NULL ) || result != BIGSIZE) return FALSE;
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
        return (WriteFile( hf, block, BIGSIZE, &result, NULL ) && result == BIGSIZE);
}

/******************************************************************************
 *		STORAGE_set_big_chain	[Internal]
 */
static BOOL
STORAGE_set_big_chain(HANDLE hf,int blocknr,INT type) {
	BYTE	block[BIGSIZE];
	LPINT	bbd = (LPINT)block;
	int	nextblocknr,bigblocknr;
	struct storage_header sth;
	BOOL ret;

	READ_HEADER;
	assert(blocknr!=type);
	while (blocknr>=0) {
		bigblocknr = sth.bbd_list[blocknr/128];
		assert(bigblocknr>=0);
		ret = STORAGE_get_big_block(hf,bigblocknr,block);
		assert(ret);

		nextblocknr = bbd[blocknr&(128-1)];
		bbd[blocknr&(128-1)] = type;
		if (type>=0)
			return TRUE;
		ret = STORAGE_put_big_block(hf,bigblocknr,block);
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
STORAGE_set_small_chain(HANDLE hf,int blocknr,INT type) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastblocknr,nextsmallblocknr,bigblocknr;
	struct storage_header sth;
	BOOL ret;

	READ_HEADER;

	assert(blocknr!=type);
	lastblocknr=-129;bigblocknr=-2;
	while (blocknr>=0) {
		/* cache block ... */
		if (lastblocknr/128!=blocknr/128) {
			bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
			assert(bigblocknr>=0);
			ret = STORAGE_get_big_block(hf,bigblocknr,block);
			assert(ret);
		}
		lastblocknr = blocknr;
		nextsmallblocknr = sbd[blocknr&(128-1)];
		sbd[blocknr&(128-1)] = type;
		ret = STORAGE_put_big_block(hf,bigblocknr,block);
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
STORAGE_get_free_big_blocknr(HANDLE hf) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastbigblocknr,i,curblock,bigblocknr;
	struct storage_header sth;
	BOOL ret;

	READ_HEADER;
	curblock	= 0;
	lastbigblocknr	= -1;
	bigblocknr	= sth.bbd_list[curblock];
	while (curblock<sth.num_of_bbd_blocks) {
		assert(bigblocknr>=0);
		ret = STORAGE_get_big_block(hf,bigblocknr,block);
		assert(ret);
		for (i=0;i<128;i++)
			if (sbd[i]==STORAGE_CHAINENTRY_FREE) {
				sbd[i] = STORAGE_CHAINENTRY_ENDOFCHAIN;
				ret = STORAGE_put_big_block(hf,bigblocknr,block);
				assert(ret);
				memset(block,0x42,sizeof(block));
				ret = STORAGE_put_big_block(hf,i+curblock*128,block);
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
	ret = STORAGE_put_big_block(hf,bigblocknr,block);
	assert(ret);

	/* if we had a bbd block already (mostlikely) we need
	 * to link the new one into the chain
	 */
	if (lastbigblocknr!=-1) {
		ret = STORAGE_set_big_chain(hf,lastbigblocknr,bigblocknr);
		assert(ret);
	}
	sth.bbd_list[curblock]=bigblocknr;
	sth.num_of_bbd_blocks++;
	assert(sth.num_of_bbd_blocks==curblock+1);
	ret = STORAGE_put_big_block(hf,-1,(LPBYTE)&sth);
	assert(ret);

	/* Set the end of the chain for the bigblockdepots */
	ret = STORAGE_set_big_chain(hf,bigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN);
	assert(ret);
	/* add 1, for the first entry is used for the additional big block
	 * depot. (means we already used bigblocknr) */
	memset(block,0x42,sizeof(block));
	/* allocate this block (filled with 0x42) */
	ret = STORAGE_put_big_block(hf,bigblocknr+1,block);
	assert(ret);
	return bigblocknr+1;
}


/******************************************************************************
 *		STORAGE_get_free_small_blocknr	[Internal]
 */
static int
STORAGE_get_free_small_blocknr(HANDLE hf) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastbigblocknr,newblocknr,i,curblock,bigblocknr;
	struct storage_pps_entry	root;
	struct storage_header sth;

	READ_HEADER;
	bigblocknr	= sth.sbd_startblock;
	curblock	= 0;
	lastbigblocknr	= -1;
	newblocknr	= -1;
	while (bigblocknr>=0) {
		if (!STORAGE_get_big_block(hf,bigblocknr,block))
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
		bigblocknr = STORAGE_get_next_big_blocknr(hf,bigblocknr);
		curblock++;
	}
	if (newblocknr==-1) {
		bigblocknr = STORAGE_get_free_big_blocknr(hf);
		if (bigblocknr<0)
			return -1;
		READ_HEADER;
		memset(block,0xff,sizeof(block));
		sbd[0]=STORAGE_CHAINENTRY_ENDOFCHAIN;
		if (!STORAGE_put_big_block(hf,bigblocknr,block))
			return -1;
		if (lastbigblocknr==-1) {
			sth.sbd_startblock = bigblocknr;
			if (!STORAGE_put_big_block(hf,-1,(LPBYTE)&sth)) /* need to write it */
				return -1;
		} else {
			if (!STORAGE_set_big_chain(hf,lastbigblocknr,bigblocknr))
				return -1;
		}
		if (!STORAGE_set_big_chain(hf,bigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
			return -1;
		newblocknr = curblock*128;
	}
	/* allocate enough big blocks for storing the allocated small block */
	if (!STORAGE_get_root_pps_entry(hf,&root))
		return -1;
	if (root.pps_sb==-1)
		lastbigblocknr	= -1;
	else
		lastbigblocknr	= STORAGE_get_nth_next_big_blocknr(hf,root.pps_sb,(root.pps_size-1)/BIGSIZE);
	while (root.pps_size < (newblocknr*SMALLSIZE+SMALLSIZE-1)) {
		/* we need to allocate more stuff */
		bigblocknr = STORAGE_get_free_big_blocknr(hf);
		if (bigblocknr<0)
			return -1;
		READ_HEADER;
		if (root.pps_sb==-1) {
			root.pps_sb	 = bigblocknr;
			root.pps_size	+= BIGSIZE;
		} else {
			if (!STORAGE_set_big_chain(hf,lastbigblocknr,bigblocknr))
				return -1;
			root.pps_size	+= BIGSIZE;
		}
		lastbigblocknr = bigblocknr;
	}
	if (!STORAGE_set_big_chain(hf,lastbigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
		return -1;
	if (!STORAGE_put_pps_entry(hf,0,&root))
		return -1;
	return newblocknr;
}

/******************************************************************************
 *		STORAGE_get_free_pps_entry	[Internal]
 */
static int
STORAGE_get_free_pps_entry(HANDLE hf) {
	int	blocknr, i, curblock, lastblocknr=-1;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)block;
	struct storage_header sth;

	READ_HEADER;
	blocknr = sth.root_startblock;
	assert(blocknr>=0);
	curblock=0;
	while (blocknr>=0) {
		if (!STORAGE_get_big_block(hf,blocknr,block))
			return -1;
		for (i=0;i<4;i++)
			if (stde[i].pps_sizeofname==0) /* free */
				return curblock*4+i;
		lastblocknr = blocknr;
		blocknr = STORAGE_get_next_big_blocknr(hf,blocknr);
		curblock++;
	}
	assert(blocknr==STORAGE_CHAINENTRY_ENDOFCHAIN);
	blocknr = STORAGE_get_free_big_blocknr(hf);
	/* sth invalidated */
	if (blocknr<0)
		return -1;

	if (!STORAGE_set_big_chain(hf,lastblocknr,blocknr))
		return -1;
	if (!STORAGE_set_big_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
		return -1;
	memset(block,0,sizeof(block));
	STORAGE_put_big_block(hf,blocknr,block);
	return curblock*4;
}

/* --- IStream16 implementation */

typedef struct
{
        /* IUnknown fields */
        ICOM_VFIELD(IStream16);
        DWORD                           ref;
        /* IStream16 fields */
        SEGPTR                          thisptr; /* pointer to this struct as segmented */
        struct storage_pps_entry        stde;
        int                             ppsent;
        HANDLE                         hf;
        ULARGE_INTEGER                  offset;
} IStream16Impl;

/******************************************************************************
 *		IStream16_QueryInterface	[STORAGE.518]
 */
HRESULT WINAPI IStream16_fnQueryInterface(
	IStream16* iface,REFIID refiid,LPVOID *obj
) {
	ICOM_THIS(IStream16Impl,iface);
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
ULONG WINAPI IStream16_fnAddRef(IStream16* iface) {
	ICOM_THIS(IStream16Impl,iface);
	return ++(This->ref);
}

/******************************************************************************
 * IStream16_Release [STORAGE.520]
 */
ULONG WINAPI IStream16_fnRelease(IStream16* iface) {
	ICOM_THIS(IStream16Impl,iface);
	FlushFileBuffers(This->hf);
	This->ref--;
	if (!This->ref) {
		CloseHandle(This->hf);
                UnMapLS( This->thisptr );
		HeapFree( GetProcessHeap(), 0, This );
		return 0;
	}
	return This->ref;
}

/******************************************************************************
 *		IStream16_Seek	[STORAGE.523]
 *
 * FIXME
 *    Does not handle 64 bits
 */
HRESULT WINAPI IStream16_fnSeek(
	IStream16* iface,LARGE_INTEGER offset,DWORD whence,ULARGE_INTEGER *newpos
) {
	ICOM_THIS(IStream16Impl,iface);
	TRACE_(relay)("(%p)->([%ld.%ld],%ld,%p)\n",This,offset.u.HighPart,offset.u.LowPart,whence,newpos);

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
HRESULT WINAPI IStream16_fnRead(
        IStream16* iface,void  *pv,ULONG cb,ULONG  *pcbRead
) {
	ICOM_THIS(IStream16Impl,iface);
	BYTE	block[BIGSIZE];
	ULONG	*bytesread=pcbRead,xxread;
	int	blocknr;

	TRACE_(relay)("(%p)->(%p,%ld,%p)\n",This,pv,cb,pcbRead);
	if (!pcbRead) bytesread=&xxread;
	*bytesread = 0;

	if (cb>This->stde.pps_size-This->offset.u.LowPart)
		cb=This->stde.pps_size-This->offset.u.LowPart;
	if (This->stde.pps_size < 0x1000) {
		/* use small block reader */
		blocknr = STORAGE_get_nth_next_small_blocknr(This->hf,This->stde.pps_sb,This->offset.u.LowPart/SMALLSIZE);
		while (cb) {
			int	cc;

			if (!STORAGE_get_small_block(This->hf,blocknr,block)) {
			   WARN("small block read failed!!!\n");
				return E_FAIL;
			}
			cc = cb;
			if (cc>SMALLSIZE-(This->offset.u.LowPart&(SMALLSIZE-1)))
				cc=SMALLSIZE-(This->offset.u.LowPart&(SMALLSIZE-1));
			memcpy((LPBYTE)pv,block+(This->offset.u.LowPart&(SMALLSIZE-1)),cc);
			This->offset.u.LowPart+=cc;
			(LPBYTE)pv+=cc;
			*bytesread+=cc;
			cb-=cc;
			blocknr = STORAGE_get_next_small_blocknr(This->hf,blocknr);
		}
	} else {
		/* use big block reader */
		blocknr = STORAGE_get_nth_next_big_blocknr(This->hf,This->stde.pps_sb,This->offset.u.LowPart/BIGSIZE);
		while (cb) {
			int	cc;

			if (!STORAGE_get_big_block(This->hf,blocknr,block)) {
				WARN("big block read failed!!!\n");
				return E_FAIL;
			}
			cc = cb;
			if (cc>BIGSIZE-(This->offset.u.LowPart&(BIGSIZE-1)))
				cc=BIGSIZE-(This->offset.u.LowPart&(BIGSIZE-1));
			memcpy((LPBYTE)pv,block+(This->offset.u.LowPart&(BIGSIZE-1)),cc);
			This->offset.u.LowPart+=cc;
			(LPBYTE)pv+=cc;
			*bytesread+=cc;
			cb-=cc;
			blocknr=STORAGE_get_next_big_blocknr(This->hf,blocknr);
		}
	}
	return S_OK;
}

/******************************************************************************
 *		IStream16_Write	[STORAGE.522]
 */
HRESULT WINAPI IStream16_fnWrite(
        IStream16* iface,const void *pv,ULONG cb,ULONG *pcbWrite
) {
	ICOM_THIS(IStream16Impl,iface);
	BYTE	block[BIGSIZE];
	ULONG	*byteswritten=pcbWrite,xxwritten;
	int	oldsize,newsize,i,curoffset=0,lastblocknr,blocknr,cc;
	HANDLE	hf = This->hf;

	if (!pcbWrite) byteswritten=&xxwritten;
	*byteswritten = 0;

	TRACE_(relay)("(%p)->(%p,%ld,%p)\n",This,pv,cb,pcbWrite);
	/* do we need to junk some blocks? */
	newsize	= This->offset.u.LowPart+cb;
	oldsize	= This->stde.pps_size;
	if (newsize < oldsize) {
		if (oldsize < 0x1000) {
			/* only small blocks */
			blocknr=STORAGE_get_nth_next_small_blocknr(hf,This->stde.pps_sb,newsize/SMALLSIZE);

			assert(blocknr>=0);

			/* will set the rest of the chain to 'free' */
			if (!STORAGE_set_small_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
				return E_FAIL;
		} else {
			if (newsize >= 0x1000) {
				blocknr=STORAGE_get_nth_next_big_blocknr(hf,This->stde.pps_sb,newsize/BIGSIZE);
				assert(blocknr>=0);

				/* will set the rest of the chain to 'free' */
				if (!STORAGE_set_big_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
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
					if (!STORAGE_get_big_block(hf,blocknr,curdata)) {
						HeapFree(GetProcessHeap(),0,data);
						return E_FAIL;
					}
					curdata	+= BIGSIZE;
					cc	-= BIGSIZE;
					blocknr	 = STORAGE_get_next_big_blocknr(hf,blocknr);
				}
				/* frees complete chain for this stream */
				if (!STORAGE_set_big_chain(hf,This->stde.pps_sb,STORAGE_CHAINENTRY_FREE))
					goto err;
				curdata	= data;
				blocknr = This->stde.pps_sb = STORAGE_get_free_small_blocknr(hf);
				if (blocknr<0)
					goto err;
				cc	= newsize;
				while (cc>0) {
					if (!STORAGE_put_small_block(hf,blocknr,curdata))
						goto err;
					cc	-= SMALLSIZE;
					if (cc<=0) {
						if (!STORAGE_set_small_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
							goto err;
						break;
					} else {
						int newblocknr = STORAGE_get_free_small_blocknr(hf);
						if (newblocknr<0)
							goto err;
						if (!STORAGE_set_small_chain(hf,blocknr,newblocknr))
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
			blocknr = STORAGE_get_nth_next_big_blocknr(hf,This->stde.pps_sb,This->stde.pps_size/BIGSIZE);
			assert(blocknr>=0);
			lastblocknr	= blocknr;
			for (i=oldsize/BIGSIZE;i<newsize/BIGSIZE;i++) {
				blocknr = STORAGE_get_free_big_blocknr(hf);
				if (blocknr<0)
					return E_FAIL;
				if (!STORAGE_set_big_chain(hf,lastblocknr,blocknr))
					return E_FAIL;
				lastblocknr = blocknr;
			}
			if (!STORAGE_set_big_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
				return E_FAIL;
		} else {
			if (newsize < 0x1000) {
				/* find startblock */
				if (!oldsize)
					This->stde.pps_sb = blocknr = STORAGE_get_free_small_blocknr(hf);
				else
					blocknr = STORAGE_get_nth_next_small_blocknr(hf,This->stde.pps_sb,This->stde.pps_size/SMALLSIZE);
				if (blocknr<0)
					return E_FAIL;

				/* allocate required new small blocks */
				lastblocknr = blocknr;
				for (i=oldsize/SMALLSIZE;i<newsize/SMALLSIZE;i++) {
					blocknr = STORAGE_get_free_small_blocknr(hf);
					if (blocknr<0)
						return E_FAIL;
					if (!STORAGE_set_small_chain(hf,lastblocknr,blocknr))
						return E_FAIL;
					lastblocknr = blocknr;
				}
				/* and terminate the chain */
				if (!STORAGE_set_small_chain(hf,lastblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
					return E_FAIL;
			} else {
				if (!oldsize) {
					/* no single block allocated yet */
					blocknr=STORAGE_get_free_big_blocknr(hf);
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
						if (!STORAGE_get_small_block(hf,blocknr,curdata))
							goto err2;
						curdata	+= SMALLSIZE;
						cc	-= SMALLSIZE;
						blocknr	 = STORAGE_get_next_small_blocknr(hf,blocknr);
					}
					/* free small block chain */
					if (!STORAGE_set_small_chain(hf,This->stde.pps_sb,STORAGE_CHAINENTRY_FREE))
						goto err2;
					curdata	= data;
					blocknr = This->stde.pps_sb = STORAGE_get_free_big_blocknr(hf);
					if (blocknr<0)
						goto err2;
					/* put the data into the big blocks */
					cc	= This->stde.pps_size;
					while (cc>0) {
						if (!STORAGE_put_big_block(hf,blocknr,curdata))
							goto err2;
						cc	-= BIGSIZE;
						if (cc<=0) {
							if (!STORAGE_set_big_chain(hf,blocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
								goto err2;
							break;
						} else {
							int newblocknr = STORAGE_get_free_big_blocknr(hf);
							if (newblocknr<0)
								goto err2;
							if (!STORAGE_set_big_chain(hf,blocknr,newblocknr))
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
					blocknr = STORAGE_get_free_big_blocknr(hf);
					if (blocknr<0)
						return E_FAIL;
					if (!STORAGE_set_big_chain(hf,lastblocknr,blocknr))
						return E_FAIL;
					lastblocknr = blocknr;
				}
				/* terminate chain */
				if (!STORAGE_set_big_chain(hf,lastblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN))
					return E_FAIL;
			}
		}
		This->stde.pps_size = newsize;
	}

	/* There are just some cases where we didn't modify it, we write it out
	 * everytime
	 */
	if (!STORAGE_put_pps_entry(hf,This->ppsent,&(This->stde)))
		return E_FAIL;

	/* finally the write pass */
	if (This->stde.pps_size < 0x1000) {
		blocknr = STORAGE_get_nth_next_small_blocknr(hf,This->stde.pps_sb,This->offset.u.LowPart/SMALLSIZE);
		assert(blocknr>=0);
		while (cb>0) {
			/* we ensured that it is allocated above */
			assert(blocknr>=0);
			/* Read old block everytime, since we can have
			 * overlapping data at START and END of the write
			 */
			if (!STORAGE_get_small_block(hf,blocknr,block))
				return E_FAIL;

			cc = SMALLSIZE-(This->offset.u.LowPart&(SMALLSIZE-1));
			if (cc>cb)
				cc=cb;
			memcpy(	((LPBYTE)block)+(This->offset.u.LowPart&(SMALLSIZE-1)),
				(LPBYTE)((char *) pv+curoffset),
				cc
			);
			if (!STORAGE_put_small_block(hf,blocknr,block))
				return E_FAIL;
			cb			-= cc;
			curoffset		+= cc;
			(LPBYTE)pv		+= cc;
			This->offset.u.LowPart	+= cc;
			*byteswritten		+= cc;
			blocknr = STORAGE_get_next_small_blocknr(hf,blocknr);
		}
	} else {
		blocknr = STORAGE_get_nth_next_big_blocknr(hf,This->stde.pps_sb,This->offset.u.LowPart/BIGSIZE);
		assert(blocknr>=0);
		while (cb>0) {
			/* we ensured that it is allocated above, so it better is */
			assert(blocknr>=0);
			/* read old block everytime, since we can have
			 * overlapping data at START and END of the write
			 */
			if (!STORAGE_get_big_block(hf,blocknr,block))
				return E_FAIL;

			cc = BIGSIZE-(This->offset.u.LowPart&(BIGSIZE-1));
			if (cc>cb)
				cc=cb;
			memcpy(	((LPBYTE)block)+(This->offset.u.LowPart&(BIGSIZE-1)),
				(LPBYTE)((char *) pv+curoffset),
				cc
			);
			if (!STORAGE_put_big_block(hf,blocknr,block))
				return E_FAIL;
			cb			-= cc;
			curoffset		+= cc;
			(LPBYTE)pv		+= cc;
			This->offset.u.LowPart	+= cc;
			*byteswritten		+= cc;
			blocknr = STORAGE_get_next_big_blocknr(hf,blocknr);
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
			segstrvt16 = (ICOM_VTABLE(IStream16)*)MapLS( &strvt16 );
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
	*str = (void*)lpst->thisptr;
}


/* --- IStream32 implementation */

typedef struct
{
        /* IUnknown fields */
        ICOM_VFIELD(IStream);
        DWORD                           ref;
        /* IStream32 fields */
        struct storage_pps_entry        stde;
        int                             ppsent;
        HANDLE                         hf;
        ULARGE_INTEGER                  offset;
} IStream32Impl;

/*****************************************************************************
 *		IStream32_QueryInterface	[VTABLE]
 */
HRESULT WINAPI IStream_fnQueryInterface(
	IStream* iface,REFIID refiid,LPVOID *obj
) {
	ICOM_THIS(IStream32Impl,iface);

	TRACE_(relay)("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = This;
		return 0;
	}
	return OLE_E_ENUM_NOMORE;

}

/******************************************************************************
 * IStream32_AddRef [VTABLE]
 */
ULONG WINAPI IStream_fnAddRef(IStream* iface) {
	ICOM_THIS(IStream32Impl,iface);
	return ++(This->ref);
}

/******************************************************************************
 * IStream32_Release [VTABLE]
 */
ULONG WINAPI IStream_fnRelease(IStream* iface) {
	ICOM_THIS(IStream32Impl,iface);
	FlushFileBuffers(This->hf);
	This->ref--;
	if (!This->ref) {
		CloseHandle(This->hf);
		HeapFree( GetProcessHeap(), 0, This );
		return 0;
	}
	return This->ref;
}

/* --- IStorage16 implementation */

typedef struct
{
        /* IUnknown fields */
        ICOM_VFIELD(IStorage16);
        DWORD                           ref;
        /* IStorage16 fields */
        SEGPTR                          thisptr; /* pointer to this struct as segmented */
        struct storage_pps_entry        stde;
        int                             ppsent;
        HANDLE                         hf;
} IStorage16Impl;

/******************************************************************************
 *		IStorage16_QueryInterface	[STORAGE.500]
 */
HRESULT WINAPI IStorage16_fnQueryInterface(
	IStorage16* iface,REFIID refiid,LPVOID *obj
) {
	ICOM_THIS(IStorage16Impl,iface);

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
ULONG WINAPI IStorage16_fnAddRef(IStorage16* iface) {
	ICOM_THIS(IStorage16Impl,iface);
	return ++(This->ref);
}

/******************************************************************************
 * IStorage16_Release [STORAGE.502]
 */
ULONG WINAPI IStorage16_fnRelease(IStorage16* iface) {
	ICOM_THIS(IStorage16Impl,iface);
	This->ref--;
	if (This->ref)
		return This->ref;
        UnMapLS( This->thisptr );
        HeapFree( GetProcessHeap(), 0, This );
	return 0;
}

/******************************************************************************
 * IStorage16_Stat [STORAGE.517]
 */
HRESULT WINAPI IStorage16_fnStat(
        LPSTORAGE16 iface,STATSTG16 *pstatstg, DWORD grfStatFlag
) {
	ICOM_THIS(IStorage16Impl,iface);
        DWORD len = WideCharToMultiByte( CP_ACP, 0, This->stde.pps_rawname, -1, NULL, 0, NULL, NULL );
        LPSTR nameA = HeapAlloc( GetProcessHeap(), 0, len );

	TRACE("(%p)->(%p,0x%08lx)\n",
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
HRESULT WINAPI IStorage16_fnCommit(
        LPSTORAGE16 iface,DWORD commitflags
) {
	ICOM_THIS(IStorage16Impl,iface);
	FIXME("(%p)->(0x%08lx),STUB!\n",
		This,commitflags
	);
	return S_OK;
}

/******************************************************************************
 * IStorage16_CopyTo [STORAGE.507]
 */
HRESULT WINAPI IStorage16_fnCopyTo(LPSTORAGE16 iface,DWORD ciidExclude,const IID *rgiidExclude,SNB16 SNB16Exclude,IStorage16 *pstgDest) {
	ICOM_THIS(IStorage16Impl,iface);
	FIXME("IStorage16(%p)->(0x%08lx,%s,%p,%p),stub!\n",
		This,ciidExclude,debugstr_guid(rgiidExclude),SNB16Exclude,pstgDest
	);
	return S_OK;
}


/******************************************************************************
 * IStorage16_CreateStorage [STORAGE.505]
 */
HRESULT WINAPI IStorage16_fnCreateStorage(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName,DWORD grfMode,DWORD dwStgFormat,DWORD reserved2, IStorage16 **ppstg
) {
	ICOM_THIS(IStorage16Impl,iface);
	IStorage16Impl*	lpstg;
	int		ppsent,x;
	struct storage_pps_entry	stde;
	struct storage_header sth;
	HANDLE		hf=This->hf;
	BOOL ret;
	int	 nPPSEntries;

	READ_HEADER;

	TRACE("(%p)->(%s,0x%08lx,0x%08lx,0x%08lx,%p)\n",
		This,pwcsName,grfMode,dwStgFormat,reserved2,ppstg
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME("We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istorage16(ppstg);
	lpstg = MapSL((SEGPTR)*ppstg);
	lpstg->hf		= This->hf;

	ppsent=STORAGE_get_free_pps_entry(lpstg->hf);
	if (ppsent<0)
		return E_FAIL;
	stde=This->stde;
	if (stde.pps_dir==-1) {
		stde.pps_dir = ppsent;
		x = This->ppsent;
	} else {
		FIXME(" use prev chain too ?\n");
		x=stde.pps_dir;
		if (1!=STORAGE_get_pps_entry(lpstg->hf,x,&stde))
			return E_FAIL;
		while (stde.pps_next!=-1) {
			x=stde.pps_next;
			if (1!=STORAGE_get_pps_entry(lpstg->hf,x,&stde))
				return E_FAIL;
		}
		stde.pps_next = ppsent;
	}
	ret = STORAGE_put_pps_entry(lpstg->hf,x,&stde);
	assert(ret);
	nPPSEntries = STORAGE_get_pps_entry(lpstg->hf,ppsent,&(lpstg->stde));
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
	if (!STORAGE_put_pps_entry(lpstg->hf,ppsent,&(lpstg->stde)))
		return E_FAIL;
	return S_OK;
}

/******************************************************************************
 *		IStorage16_CreateStream	[STORAGE.503]
 */
HRESULT WINAPI IStorage16_fnCreateStream(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2, IStream16 **ppstm
) {
	ICOM_THIS(IStorage16Impl,iface);
	IStream16Impl*	lpstr;
	int		ppsent,x;
	struct storage_pps_entry	stde;
	BOOL ret;
	int	 nPPSEntries;

	TRACE("(%p)->(%s,0x%08lx,0x%08lx,0x%08lx,%p)\n",
		This,pwcsName,grfMode,reserved1,reserved2,ppstm
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME("We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istream16(ppstm);
	lpstr = MapSL((SEGPTR)*ppstm);
        DuplicateHandle( GetCurrentProcess(), This->hf, GetCurrentProcess(),
                         &lpstr->hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
	lpstr->offset.u.LowPart	= 0;
	lpstr->offset.u.HighPart	= 0;

	ppsent=STORAGE_get_free_pps_entry(lpstr->hf);
	if (ppsent<0)
		return E_FAIL;
	stde=This->stde;
	if (stde.pps_next==-1)
		x=This->ppsent;
	else
		while (stde.pps_next!=-1) {
			x=stde.pps_next;
			if (1!=STORAGE_get_pps_entry(lpstr->hf,x,&stde))
				return E_FAIL;
		}
	stde.pps_next = ppsent;
	ret = STORAGE_put_pps_entry(lpstr->hf,x,&stde);
	assert(ret);
	nPPSEntries = STORAGE_get_pps_entry(lpstr->hf,ppsent,&(lpstr->stde));
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
	if (!STORAGE_put_pps_entry(lpstr->hf,ppsent,&(lpstr->stde)))
		return E_FAIL;
	return S_OK;
}

/******************************************************************************
 *		IStorage16_OpenStorage	[STORAGE.506]
 */
HRESULT WINAPI IStorage16_fnOpenStorage(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName, IStorage16 *pstgPrio, DWORD grfMode, SNB16 snbExclude, DWORD reserved, IStorage16 **ppstg
) {
	ICOM_THIS(IStorage16Impl,iface);
	IStream16Impl*	lpstg;
	WCHAR		name[33];
	int		newpps;

	TRACE_(relay)("(%p)->(%s,%p,0x%08lx,%p,0x%08lx,%p)\n",
		This,pwcsName,pstgPrio,grfMode,snbExclude,reserved,ppstg
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME("We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istorage16(ppstg);
	lpstg = MapSL((SEGPTR)*ppstg);
        DuplicateHandle( GetCurrentProcess(), This->hf, GetCurrentProcess(),
                         &lpstg->hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
        MultiByteToWideChar( CP_ACP, 0, pwcsName, -1, name, sizeof(name)/sizeof(WCHAR));
	newpps = STORAGE_look_for_named_pps(lpstg->hf,This->stde.pps_dir,name);
	if (newpps==-1) {
		IStream16_fnRelease((IStream16*)lpstg);
		return E_FAIL;
	}

	if (1!=STORAGE_get_pps_entry(lpstg->hf,newpps,&(lpstg->stde))) {
		IStream16_fnRelease((IStream16*)lpstg);
		return E_FAIL;
	}
	lpstg->ppsent		= newpps;
	return S_OK;
}

/******************************************************************************
 * IStorage16_OpenStream [STORAGE.504]
 */
HRESULT WINAPI IStorage16_fnOpenStream(
	LPSTORAGE16 iface,LPCOLESTR16 pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream16 **ppstm
) {
	ICOM_THIS(IStorage16Impl,iface);
	IStream16Impl*	lpstr;
	WCHAR		name[33];
	int		newpps;

	TRACE_(relay)("(%p)->(%s,%p,0x%08lx,0x%08lx,%p)\n",
		This,pwcsName,reserved1,grfMode,reserved2,ppstm
	);
	if (grfMode & STGM_TRANSACTED)
		FIXME("We do not support transacted Compound Storage. Using direct mode.\n");
	_create_istream16(ppstm);
	lpstr = MapSL((SEGPTR)*ppstm);
        DuplicateHandle( GetCurrentProcess(), This->hf, GetCurrentProcess(),
                         &lpstr->hf, 0, TRUE, DUPLICATE_SAME_ACCESS );
        MultiByteToWideChar( CP_ACP, 0, pwcsName, -1, name, sizeof(name)/sizeof(WCHAR));
	newpps = STORAGE_look_for_named_pps(lpstr->hf,This->stde.pps_dir,name);
	if (newpps==-1) {
		IStream16_fnRelease((IStream16*)lpstr);
		return E_FAIL;
	}

	if (1!=STORAGE_get_pps_entry(lpstr->hf,newpps,&(lpstr->stde))) {
		IStream16_fnRelease((IStream16*)lpstr);
		return E_FAIL;
	}
	lpstr->offset.u.LowPart	= 0;
	lpstr->offset.u.HighPart	= 0;
	lpstr->ppsent		= newpps;
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
			segstvt16 = (ICOM_VTABLE(IStorage16)*)MapLS( &stvt16 );
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

	TRACE("(%s,0x%08lx,0x%08lx,%p)\n",
		pwcsName,grfMode,reserved,ppstgOpen
	);
	_create_istorage16(ppstgOpen);
	hf = CreateFileA(pwcsName,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_NEW,0,0);
	if (hf==INVALID_HANDLE_VALUE) {
		WARN("couldn't open file for storage:%ld\n",GetLastError());
		return E_FAIL;
	}
	lpstg = MapSL((SEGPTR)*ppstgOpen);
	lpstg->hf = hf;
	/* FIXME: check for existence before overwriting? */
	if (!STORAGE_init_storage(hf)) {
		CloseHandle(hf);
		return E_FAIL;
	}
	i=0;ret=0;
	while (!ret) { /* neither 1 nor <0 */
		ret=STORAGE_get_pps_entry(hf,i,&stde);
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

	TRACE("(%s,%p,0x%08lx,%p,%ld,%p)\n",
              pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstgOpen
	);
	_create_istorage16(ppstgOpen);
	hf = CreateFileA(pwcsName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (hf==INVALID_HANDLE_VALUE) {
		WARN("Couldn't open file for storage\n");
		return E_FAIL;
	}
	lpstg = MapSL((SEGPTR)*ppstgOpen);
	lpstg->hf = hf;

	i=0;ret=0;
	while (!ret) { /* neither 1 nor <0 */
		ret=STORAGE_get_pps_entry(hf,i,&stde);
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
  args[3] = (DWORD)K32WOWGlobalAllocLock16( 0, 8, &hsig ); /* sig */
  args[4] = 8;
  args[5] = 0;

  if (!K32WOWCallback16Ex(
      (DWORD)((ICOM_VTABLE(ILockBytes16)*)MapSL(
                  (SEGPTR)((LPLOCKBYTES16)MapSL(plkbyt))->lpVtbl)
      )->ReadAt,
      WCB16_PASCAL,
      6*sizeof(DWORD),
      (LPVOID)args,
      (LPDWORD)&hres
  )) {
      ERR("CallTo16 ILockBytes16::ReadAt() failed, hres %lx\n",hres);
      return hres;
  }
  if (memcmp(MapSL(args[3]), STORAGE_magic, sizeof(STORAGE_magic)) == 0) {
    K32WOWGlobalUnlockFree16(args[3]);
    return S_OK;
  }
  K32WOWGlobalUnlockFree16(args[3]);
  return S_FALSE;
}

/******************************************************************************
 *    StgOpenStorageOnILockBytes    [STORAGE.4]
 */
HRESULT WINAPI StgOpenStorageOnILockBytes16(
      ILockBytes16 *plkbyt,
      IStorage16 *pstgPriority,
      DWORD grfMode,
      SNB16 snbExclude,
      DWORD reserved,
      IStorage16 **ppstgOpen)
{
  IStorage16Impl*	lpstg;

  if ((plkbyt == 0) || (ppstgOpen == 0))
    return STG_E_INVALIDPOINTER;

  *ppstgOpen = 0;

  _create_istorage16(ppstgOpen);
  lpstg = MapSL((SEGPTR)*ppstgOpen);

  /* just teach it to use HANDLE instead of ilockbytes :/ */

  return S_OK;
}
