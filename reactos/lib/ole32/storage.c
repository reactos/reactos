/* Compound Storage
 *
 * Implemented using the documentation of the LAOLA project at
 * <URL:http://wwwwbs.cs.tu-berlin.de/~schwartz/pmh/index.html>
 * (Thanks to Martin Schwartz <schwartz@cs.tu-berlin.de>)
 *
 * Copyright 1998 Marcus Meissner
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <ddk/ntddk.h>
#include <ole32/ole32.h>
#include <compobj.h>
#include <storage32.h>

#include <debug.h>

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


//static const BYTE STORAGE_magic[8]   ={0xd0,0xcf,0x11,0xe0,0xa1,0xb1,0x1a,0xe1};
//static const BYTE STORAGE_notmagic[8]={0x0e,0x11,0xfc,0x0d,0xd0,0xcf,0x11,0xe0};
//static const BYTE STORAGE_oldmagic[8]={0xd0,0xcf,0x11,0xe0,0x0e,0x11,0xfc,0x0d};

#define BIGSIZE		512
#define SMALLSIZE		64

#define SMALLBLOCKS_PER_BIGBLOCK	(BIGSIZE/SMALLSIZE)

#define READ_HEADER	assert(STORAGE_get_big_block(hf,-1,(LPBYTE)&sth));assert(!memcmp(STORAGE_magic,sth.magic,sizeof(STORAGE_magic)));
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
STORAGE_get_big_block(HFILE hf,int n,BYTE *block) {
	assert(n>=-1);
	if (-1==_llseek(hf,(n+1)*BIGSIZE,SEEK_SET)) {
		Print(MID_TRACE, (" seek failed (%ld)\n",GetLastError()));
		return FALSE;
	}
	assert((n+1)*BIGSIZE==_llseek(hf,0,SEEK_CUR));
	if (BIGSIZE!=_lread(hf,block,BIGSIZE)) {
		Print(MID_TRACE, ("(block size %d): read didn't read (%ld)\n",n,GetLastError()));
		assert(0);
		return FALSE;
	}
	return TRUE;
}

/******************************************************************************
 * STORAGE_put_big_block [INTERNAL]
 */
static BOOL
STORAGE_put_big_block(HFILE hf,int n,BYTE *block) {
	assert(n>=-1);
	if (-1==_llseek(hf,(n+1)*BIGSIZE,SEEK_SET)) {
		Print(MID_TRACE, (" seek failed (%ld)\n",GetLastError()));
		return FALSE;
	}
	assert((n+1)*BIGSIZE==_llseek(hf,0,SEEK_CUR));
	if (BIGSIZE!=_lwrite(hf,block,BIGSIZE)) {
		Print(MID_TRACE, (" write failed (%ld)\n",GetLastError()));
		return FALSE;
	}
	return TRUE;
}

/******************************************************************************
 * STORAGE_get_next_big_blocknr [INTERNAL]
 */
static int
STORAGE_get_next_big_blocknr(HFILE hf,int blocknr) {
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
STORAGE_get_nth_next_big_blocknr(HFILE hf,int blocknr,int nr) {
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
			assert(STORAGE_get_big_block(hf,sth.bbd_list[blocknr>>7],(LPBYTE)bbs));
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
STORAGE_get_root_pps_entry(HFILE hf,struct storage_pps_entry *pstde) {
	int	blocknr,i;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry	*stde=(struct storage_pps_entry*)block;
	struct storage_header sth;

	READ_HEADER;
	blocknr = sth.root_startblock;
	while (blocknr>=0) {
		assert(STORAGE_get_big_block(hf,blocknr,block));
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
STORAGE_get_small_block(HFILE hf,int blocknr,BYTE *sblock) {
	BYTE				block[BIGSIZE];
	int				bigblocknr;
	struct storage_pps_entry	root;

	assert(blocknr>=0);
	assert(STORAGE_get_root_pps_entry(hf,&root));
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,root.pps_sb,blocknr/SMALLBLOCKS_PER_BIGBLOCK);
	assert(bigblocknr>=0);
	assert(STORAGE_get_big_block(hf,bigblocknr,block));

	memcpy(sblock,((LPBYTE)block)+SMALLSIZE*(blocknr&(SMALLBLOCKS_PER_BIGBLOCK-1)),SMALLSIZE);
	return TRUE;
}

/******************************************************************************
 * STORAGE_put_small_block [INTERNAL]
 */
static BOOL
STORAGE_put_small_block(HFILE hf,int blocknr,BYTE *sblock) {
	BYTE				block[BIGSIZE];
	int				bigblocknr;
	struct storage_pps_entry	root;

	assert(blocknr>=0);

	assert(STORAGE_get_root_pps_entry(hf,&root));
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,root.pps_sb,blocknr/SMALLBLOCKS_PER_BIGBLOCK);
	assert(bigblocknr>=0);
	assert(STORAGE_get_big_block(hf,bigblocknr,block));

	memcpy(((LPBYTE)block)+SMALLSIZE*(blocknr&(SMALLBLOCKS_PER_BIGBLOCK-1)),sblock,SMALLSIZE);
	assert(STORAGE_put_big_block(hf,bigblocknr,block));
	return TRUE;
}

/******************************************************************************
 * STORAGE_get_next_small_blocknr [INTERNAL]
 */
static int
STORAGE_get_next_small_blocknr(HFILE hf,int blocknr) {
	BYTE				block[BIGSIZE];
	LPINT				sbd = (LPINT)block;
	int				bigblocknr;
	struct storage_header		sth;

	READ_HEADER;
	assert(blocknr>=0);
	bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
	assert(bigblocknr>=0);
	assert(STORAGE_get_big_block(hf,bigblocknr,block));
	assert(sbd[blocknr & 127]!=STORAGE_CHAINENTRY_FREE);
	return sbd[blocknr & (128-1)];
}

/******************************************************************************
 * STORAGE_get_nth_next_small_blocknr [INTERNAL]
 */
static int
STORAGE_get_nth_next_small_blocknr(HFILE hf,int blocknr,int nr) {
	int	lastblocknr;
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	struct storage_header sth;

	READ_HEADER;
	lastblocknr=-1;
	assert(blocknr>=0);
	while ((nr--) && (blocknr>=0)) {
		if (lastblocknr/128!=blocknr/128) {
			int	bigblocknr;
			bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
			assert(bigblocknr>=0);
			assert(STORAGE_get_big_block(hf,bigblocknr,block));
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
STORAGE_get_pps_entry(HFILE hf,int n,struct storage_pps_entry *pstde) {
	int	blocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)(((LPBYTE)block)+128*(n&3));
	struct storage_header sth;

	READ_HEADER;
	/* we have 4 pps entries per big block */
	blocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.root_startblock,n/4);
	assert(blocknr>=0);
	assert(STORAGE_get_big_block(hf,blocknr,block));

	*pstde=*stde;
	return 1;
}

/******************************************************************************
 *		STORAGE_put_pps_entry	[Internal]
 */
static int
STORAGE_put_pps_entry(HFILE hf,int n,struct storage_pps_entry *pstde) {
	int	blocknr;
	BYTE	block[BIGSIZE];
	struct storage_pps_entry *stde = (struct storage_pps_entry*)(((LPBYTE)block)+128*(n&3));
	struct storage_header sth;

	READ_HEADER;

	/* we have 4 pps entries per big block */
	blocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.root_startblock,n/4);
	assert(blocknr>=0);
	assert(STORAGE_get_big_block(hf,blocknr,block));
	*stde=*pstde;
	assert(STORAGE_put_big_block(hf,blocknr,block));
	return 1;
}

/******************************************************************************
 *		STORAGE_look_for_named_pps	[Internal]
 */
static int
STORAGE_look_for_named_pps(HFILE hf,int n,LPOLESTR name) {
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
#if 0
    char	name[33];

    WideCharToMultiByte( CP_ACP, 0, stde->pps_rawname, -1, name, sizeof(name), NULL, NULL);
	if (!stde->pps_sizeofname)
		return;
	Print(MAX_TRACE, ("name: %s\n",name));
	Print(MAX_TRACE, ("type: %d\n",stde->pps_type));
	Print(MAX_TRACE, ("prev pps: %ld\n",stde->pps_prev));
	Print(MAX_TRACE, ("next pps: %ld\n",stde->pps_next));
	Print(MAX_TRACE, ("dir pps: %ld\n",stde->pps_dir));
	Print(MAX_TRACE, ("guid: %s\n",PRINT_GUID(&(stde->pps_guid))));
	if (stde->pps_type !=2) {
		time_t	t;
                DWORD dw;
		RtlTimeToSecondsSince1970(&(stde->pps_ft1),&dw);
                t = dw;
		Print(MAX_TRACE, ("ts1: %s\n",ctime(&t)));
		RtlTimeToSecondsSince1970(&(stde->pps_ft2),&dw);
                t = dw;
		Print(MAX_TRACE, ("ts2: %s\n",ctime(&t)));
	}
	Print(MAX_TRACE, ("startblock: %ld\n",stde->pps_sb));
	Print(MAX_TRACE, ("size: %ld\n",stde->pps_size));
#endif
}

/******************************************************************************
 * STORAGE_init_storage [INTERNAL]
 */
static BOOL 
STORAGE_init_storage(HFILE hf) {
	BYTE	block[BIGSIZE];
	LPDWORD	bbs;
	struct storage_header *sth;
	struct storage_pps_entry *stde;

	assert(-1!=_llseek(hf,0,SEEK_SET));
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
	assert(BIGSIZE==_lwrite(hf,block,BIGSIZE));
	/* block 0 is the big block directory */
	bbs=(LPDWORD)block;
	memset(block,0xff,sizeof(block)); /* mark all blocks as free */
	bbs[0]=STORAGE_CHAINENTRY_ENDOFCHAIN; /* for this block */
	bbs[1]=STORAGE_CHAINENTRY_ENDOFCHAIN; /* for directory entry */
	assert(BIGSIZE==_lwrite(hf,block,BIGSIZE));
	/* block 1 is the root directory entry */
	memset(block,0x00,sizeof(block));
	stde = (struct storage_pps_entry*)block;
        MultiByteToWideChar( CP_ACP, 0, "RootEntry", -1, stde->pps_rawname,
                             sizeof(stde->pps_rawname)/sizeof(WCHAR));
	stde->pps_sizeofname	= (lstrlenW(stde->pps_rawname)+1) * sizeof(WCHAR);
	stde->pps_type		= 5;
	stde->pps_dir		= -1;
	stde->pps_next		= -1;
	stde->pps_prev		= -1;
	stde->pps_sb		= 0xffffffff;
	stde->pps_size		= 0;
	assert(BIGSIZE==_lwrite(hf,block,BIGSIZE));
	return TRUE;
}

/******************************************************************************
 *		STORAGE_set_big_chain	[Internal]
 */
static BOOL
STORAGE_set_big_chain(HFILE hf,int blocknr,INT type) {
	BYTE	block[BIGSIZE];
	LPINT	bbd = (LPINT)block;
	int	nextblocknr,bigblocknr;
	struct storage_header sth;

	READ_HEADER;
	assert(blocknr!=type);
	while (blocknr>=0) {
		bigblocknr = sth.bbd_list[blocknr/128];
		assert(bigblocknr>=0);
		assert(STORAGE_get_big_block(hf,bigblocknr,block));

		nextblocknr = bbd[blocknr&(128-1)];
		bbd[blocknr&(128-1)] = type;
		if (type>=0)
			return TRUE;
		assert(STORAGE_put_big_block(hf,bigblocknr,block));
		type = STORAGE_CHAINENTRY_FREE;
		blocknr = nextblocknr;
	}
	return TRUE;
}

/******************************************************************************
 * STORAGE_set_small_chain [Internal]
 */
static BOOL
STORAGE_set_small_chain(HFILE hf,int blocknr,INT type) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastblocknr,nextsmallblocknr,bigblocknr;
	struct storage_header sth;

	READ_HEADER;

	assert(blocknr!=type);
	lastblocknr=-129;bigblocknr=-2;
	while (blocknr>=0) {
		/* cache block ... */
		if (lastblocknr/128!=blocknr/128) {
			bigblocknr = STORAGE_get_nth_next_big_blocknr(hf,sth.sbd_startblock,blocknr/128);
			assert(bigblocknr>=0);
			assert(STORAGE_get_big_block(hf,bigblocknr,block));
		}
		lastblocknr = blocknr;
		nextsmallblocknr = sbd[blocknr&(128-1)];
		sbd[blocknr&(128-1)] = type;
		assert(STORAGE_put_big_block(hf,bigblocknr,block));
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
STORAGE_get_free_big_blocknr(HFILE hf) {
	BYTE	block[BIGSIZE];
	LPINT	sbd = (LPINT)block;
	int	lastbigblocknr,i,curblock,bigblocknr;
	struct storage_header sth;

	READ_HEADER;
	curblock	= 0;
	lastbigblocknr	= -1;
	bigblocknr	= sth.bbd_list[curblock];
	while (curblock<sth.num_of_bbd_blocks) {
		assert(bigblocknr>=0);
		assert(STORAGE_get_big_block(hf,bigblocknr,block));
		for (i=0;i<128;i++)
			if (sbd[i]==STORAGE_CHAINENTRY_FREE) {
				sbd[i] = STORAGE_CHAINENTRY_ENDOFCHAIN;
				assert(STORAGE_put_big_block(hf,bigblocknr,block));
				memset(block,0x42,sizeof(block));
				assert(STORAGE_put_big_block(hf,i+curblock*128,block));
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
	assert(STORAGE_put_big_block(hf,bigblocknr,block));

	/* if we had a bbd block already (mostlikely) we need
	 * to link the new one into the chain 
	 */
	if (lastbigblocknr!=-1)
		assert(STORAGE_set_big_chain(hf,lastbigblocknr,bigblocknr));
	sth.bbd_list[curblock]=bigblocknr;
	sth.num_of_bbd_blocks++;
	assert(sth.num_of_bbd_blocks==curblock+1);
	assert(STORAGE_put_big_block(hf,-1,(LPBYTE)&sth));

	/* Set the end of the chain for the bigblockdepots */
	assert(STORAGE_set_big_chain(hf,bigblocknr,STORAGE_CHAINENTRY_ENDOFCHAIN));
	/* add 1, for the first entry is used for the additional big block 
	 * depot. (means we already used bigblocknr) */
	memset(block,0x42,sizeof(block));
	/* allocate this block (filled with 0x42) */
	assert(STORAGE_put_big_block(hf,bigblocknr+1,block));
	return bigblocknr+1;
}


/******************************************************************************
 *		STORAGE_get_free_small_blocknr	[Internal]
 */
static int 
STORAGE_get_free_small_blocknr(HFILE hf) {
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
STORAGE_get_free_pps_entry(HFILE hf) {
	int	blocknr,i,curblock,lastblocknr;
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

/* --- IStream32 implementation */

typedef struct
{
        /* IUnknown fields */
        ICOM_VFIELD(IStream);
        DWORD                           ref;
        /* IStream32 fields */
        struct storage_pps_entry        stde;
        int                             ppsent;
        HFILE                         hf;
        ULARGE_INTEGER                  offset;
} IStream32Impl;

/*****************************************************************************
 *		IStream32_QueryInterface	[VTABLE]
 */
HRESULT WINAPI IStream_fnQueryInterface(
	IStream* iface,REFIID refiid,LPVOID *obj
) {
	ICOM_THIS(IStream32Impl,iface);

	Print(MAX_TRACE, ("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj));
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
#if 0
	ICOM_THIS(IStream32Impl,iface);
	FlushFileBuffers(This->hf);
	This->ref--;
	if (!This->ref) {
		CloseHandle(This->hf);
		SEGPTR_FREE(This);
		return 0;
	}
	return This->ref;
#else
  UNIMPLEMENTED;
  return 0;
#endif
}


/******************************************************************************
 *	Storage API functions
 */

/******************************************************************************
 *		StgCreateDocFile16	[STORAGE.1]
 */
HRESULT WINAPI StgCreateDocFile16(
	LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved,IStorage16 **ppstgOpen
) {
  UNIMPLEMENTED;
  return S_OK;
}

/******************************************************************************
 * StgIsStorageFile16 [STORAGE.5]
 */
HRESULT WINAPI StgIsStorageFile16(LPCOLESTR16 fn) {
  UNIMPLEMENTED;
  return S_OK;
}

/******************************************************************************
 * StgIsStorageFile [OLE32.146]
 */
HRESULT WINAPI 
StgIsStorageFile(LPCOLESTR fn) 
{
#if 0
	LPOLESTR16	xfn = HEAP_strdupWtoA(GetProcessHeap(),0,fn);
	HRESULT	ret = StgIsStorageFile16(xfn);

	HeapFree(GetProcessHeap(),0,xfn);
	return ret;
#else
  UNIMPLEMENTED;
  return S_OK;
#endif
}


/******************************************************************************
 * StgOpenStorage16 [STORAGE.3]
 */
HRESULT WINAPI StgOpenStorage16(
	LPCOLESTR16 pwcsName,IStorage16 *pstgPriority,DWORD grfMode,
	SNB16 snbExclude,DWORD reserved, IStorage16 **ppstgOpen
) {
  UNIMPLEMENTED;
  return S_OK;
}


