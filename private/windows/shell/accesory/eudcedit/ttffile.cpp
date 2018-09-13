//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
#include	"stdafx.h"
#include	"eudcedit.h"
#include    "util.h"

#pragma		pack(2)


#include	"vdata.h"
#include	"ttfstruc.h"
#include	"extfunc.h"
/*
 *	TrueType File I/F
 */
#define		GLYPHBUFSIZ	0xFFFFL
#define		RWBUFSIZ	16384

#define		EUDCCODEBASE	((unsigned short)0xE000) /* 0xe000  uni-code */
#define		NUMTABLES	15

#define		NAMEBUFSIZ	1024
#define		NUMOFNAMEREC	8


static void  initDirEntry(struct  TableEntry *e,int  num);
static int  setDirEntry(struct  TableEntry *e,char  *tag);
void  smtoi(short  *sval);
void  lmtoi(long  *lval);
void  sitom(short  *sval);
void  litom(long  *lval);
static int  align32(int  siz);
static unsigned long  calchksum(char  *buf,int  siz);
static int  copyblk(HDC  hDC,char  *tag,int  ofHdl,long  siz,char  *buf,int  bufsiz,unsigned long  *cs);
static int  mergeblock(HDC  hDC,struct  TableEntry *entry,int  ofHdl);
static int  tableChkSum(int  fH,struct  TableEntry *te);
static int  codeToGID(unsigned short  code);
int  TTFReadHdr(HANDLE  fHdl,struct  TTFHeader *hdr);
int  TTFWriteHdr(HANDLE  fHdl,struct  TTFHeader *hdr);
int  TTFReadDirEntry(HANDLE  fHdl,struct  TableEntry *entry,int  eCnt);
int  TTFWriteDirEntry(HANDLE  fHdl,struct  TableEntry *entry,int  eCnt);
static struct  TableEntry *searchEntry(struct  TableEntry *entry,int  eCnt,char  *tag);
int  TTFGetTableEntry(HANDLE  fH,struct  TableEntry *entry,char  *tag);
int  TTFReadTable(HANDLE  fH,struct  TableEntry *entry,void  *buf,int  bufsiz);
int  TTFReadFixedTable(HANDLE  fH,char  *buf,int  bufsiz,char  *tag);
int  TTFReadVarTable(HANDLE  fH,char  * *buf,unsigned int  *bufsiz,char  *tag);
int  TTFWriteTable(HANDLE  fH,struct  TableEntry *entry,void  *buf,int  bufsiz);
int  TTFAppendTable(HANDLE  fH,struct  TableEntry *entry,void  *buf,int  siz);
static int  TTFMergeTable(HDC  hDC,HANDLE  nfh,char  *tag,struct  TableEntry *nte);
#ifdef BUILD_ON_WINNT
static int __cdecl compentry(const void  *e1,const void  *e2);
#else
static int  compentry(const void  *e1,const void  *e2);
#endif // BUILD_ON_WINNT
static void  SortEntry(struct  TableEntry *ebuf,int  num);
static int  fileChkSum(HANDLE  fh,struct  TableEntry *entry,int  numEntry,struct  TTFHeader *hdr,struct  HeadTable *head);
static void  makeTTFHeader(struct  TTFHeader *hdr,int  nTbl);
static int  makeGlyphData(int  lstH,struct  BBX *bbx,char  *glyphData,int  bufsiz,int  *gdatsiz);
static void  initsetgbuf(char  *b,int  lim);
static int  setgbuf(char  *dat,int  siz,char  * *np);
static int  termbuf(void);
static int  strwlen(char  *s);
static void  strwcat(char  *s1,char  *s2);
static void  setnamebuf(char  *buf,int  *siz, short EncodingID);
static void  modifyOS2(char  *buf);
static void  modifyhead(struct  HeadTable *head);
static void  setVhea(struct  VheaTable *vhea,struct  HheaTable *hhea,struct  BBX *bbx);
static int  updateMaxp(struct  MaxpTbl *maxp,int  lstHdl);
static int  TTFGetOrgTableEntry(HDC  hDC,struct  TableEntry *entry,char  *tag);
int  TTFReadOrgFixedTable(HDC  hDC,char  *buf,int  bufsiz,char  *tag);
int  TTFReadOrgVarTable(HDC  hDC,char  * *buf,unsigned int  *bufsiz,char  *tag);
static void  setCountryData(short EncodingID);
static int  WIFEOS2(HDC  hDC,char  * *os2buf,int  *os2TblSiz,struct  BBX *bbx);
static void  WIFEhhea(struct  HheaTable *hhea,struct  BBX *bbx);
static void  WIFEhead(struct  HeadTable *head,struct  BBX *bbx);
static void  WIFEpost(struct  postTable *post);
int  TTFCreate(HDC  hDC,TCHAR  *newf,struct  BBX *bbx,int  lstHdl,int  fontType);
int  TTFGetBBX(HDC  hDC,struct  BBX *bbx,short  *uPEm);
int  TTFTmpPath(TCHAR  *path,TCHAR  *tmpPath);
static int  copyTable(HANDLE iFh,HANDLE oFh,struct  TableEntry *te,int  nEntry,char  *tag);
static int  copyfblock(HANDLE  iFh,HANDLE oFh,unsigned long  siz,unsigned long  *cs);
static int  mergeGlyph(HANDLE  iFh,HANDLE oFh,struct  TableEntry *tep,char  *locabuf,int  glyphID,char  *glyphData,int  glyphSiz);
static void  frebuf(void);
int  TTFOpen(TCHAR  *path);
int  TTFClose(void);
int  TTFGetEUDCBBX(TCHAR  *path,struct  BBX *bbx,short  *upem);
static void  makeMetrics(int  lsthdl,struct  HMetrics *hM,struct  VMetrics *vM,struct  BBX *bbx);
int  TTFAppend(unsigned short  code,struct  BBX *bbx,int  lsthdl);
int  TTFImpCopy(TCHAR  *sPath,TCHAR  *dPath);
int  TTFImpGlyphCopy(HANDLE sFh,int  glyphID);
int  TTFImpGlyphWrite(int  glyphID, char *buf, int siz);
int  TTFLastError();
/**
 * static variables
 **/
static struct TableEntry *et;
static int	entryNum;
static int	entryCnt;

static int	numOfGlyph=0;
static int	lastErr = 0;
/**
 * static table data
 **/
static char	cvtdata[] = {0, 0};
static char	fpgmdata[]={ (char)0xb0, (char)0x00, (char)0x2c, (char)0x2d};
static char	prepdata[]={(char)0xb8, 0x1, (char)0xff, (char)0x85,
				(char)0xb0, (char)0x01, (char)0x8d};
static char	maxpdata[]={	/* JPN */
			0x00, 0x01, 0x00, 0x00,
			(char)0x07, (char)0x5a,	/* 1882  numGlyph of JPN */
			0, 4, 0, 1 ,  		/* maxP, maxC(nullGlyf value)*/
			0, 0,  0, 0,		/* maxCom*2*/
			0, 1,  0, 0,		   /* maxZones, maxTw*/
			0, 0,  0, 1,		   /* maxStorage, maxFunc*/
			0, 0,  0, 1,		   /* maxI, maxStack */
			0, 1,  0, 1,  0, 1
			};
static char	cmapdata[] = {
			0, 0,  0, 1,
			0, 3,  0, 1,
			0, 0,  0, 12,
			0, 4,  0, 32,  0, 0,  0, 4,  0, 4,  0, 1, 0, 0,
			(char)0xe7, (char)0x57, (char) 0xff,(char) 0xff, 	/* last code */
			0, 0,
			(char)0xe0, (char)0x00,  (char)0xff,(char) 0xff,
			0x20, 0x02,  0x00, 0x01,
			0, 0,  0, 0
			};

static char	namerecstr[NUMOFNAMEREC][16] ={
			{ 0, '0', 0, '0', 0, 0},
			{ 0,'E', 0,'U', 0, 'D',0,'C', 0,0},
			{ 0,'R', 0, 'e', 0,'g', 0,'u', 0,'l', 0,'a', 0,'r',0,0},
			{ 0, '0', 0,'0', 0, 0},
			{ 0,'E', 0,'U', 0, 'D',0,'C', 0,0},
			{ 0, '0', 0,'0', 0, 0},
			{ 0, '0',  0,'0',0, 0},
			{ 0, '0',  0,'0',0, 0}
		};


static void
initDirEntry( struct TableEntry *e, int num)
{
	et = e;
	entryNum = num;
	entryCnt = 0;
}
static int
setDirEntry(struct TableEntry *e, char *tag)
{
	if ( entryCnt >= entryNum)
		return -1;
	memcpy( e->tagName, tag, TAGSIZ);
	*(et+entryCnt) = *e;
	entryCnt++;

	return 0;
}
void
smtoi( short *sval)
{
	short	retval;
	unsigned char	*cp;
	
	cp = (unsigned char *)sval;
	retval = *cp++;
	retval <<=8;
	retval += *cp;
	*sval = retval;
}
void
lmtoi( long *lval)
{
	long	retval;
	unsigned char	*cp;
	int	i;

	cp = (unsigned char *)lval;
	retval = (long)*cp++;
	for(i=0; i<3; i++) {
		retval <<=8;
		retval |= (long)*cp++;
	}
	*lval = retval;
}
void
sitom( short	*sval)
{
	unsigned char *cp;
	short	setval;

	setval = *sval;
	cp = (unsigned char *)sval;
	*cp++ = (unsigned char)((setval>>8)&0xff);
	*cp++ = (unsigned char)(setval&0xff);
}
void
litom( long	*lval)
{
	long	setval;
	int	i;
	unsigned char	*cp;

	setval = *lval;
	cp = (unsigned char *)lval;
	cp += 3;
	for ( i = 0; i<4; i++) {
		*cp--=(char )(setval&0xff);
		setval >>=8;
	}
}
static int
align32( int siz)
{
	siz = (siz + 3)/4 * 4;
	return siz;
}
static unsigned long
calchksum( char *buf, int siz)
{
	unsigned long	*csp;
	unsigned long	chksum;
	unsigned long	lval;
	unsigned char	pad[4];
	int	i;
	int	padsiz;
	unsigned char	*cp;
	int	csc;

	csp = (unsigned long *)buf;
	csc = siz/4;
	chksum = 0;
	while ( csc-->0) {
		lval = *csp++;
		lmtoi( (long *)&lval);
		chksum += lval;
	}	
	padsiz = (siz+3)/4*4 - siz;

	if ( padsiz) {
		cp = (unsigned char *)csp;
		i = 0;
		while (padsiz++<4)
			pad[i++]=*cp++;
		csp = (unsigned long *)pad;
		lval = *csp;
		lmtoi( (long *)&lval);
		chksum += lval;
	}
	return	chksum;
}
static int
copyblk(HDC hDC, char *tag, HANDLE ofHdl, long siz, char *buf, int bufsiz, unsigned long *cs)
{
	int	rwsiz;
	int	filsiz;
	int	aligne;
	unsigned long	chksum;
	long	ofs;
	DWORD	dwTable, nByte;

	dwTable = *(DWORD *)tag;
	filsiz = (int)(siz % 4);
	if ( filsiz)	filsiz = 4-filsiz;
	ofs = 0L;
	aligne =0;
	chksum = 0;
	while ( siz > 0) {
		if ( siz > bufsiz)
			rwsiz = bufsiz;
		else	{
			rwsiz = (int)siz;
			if ( filsiz)
				aligne =1;
		}
		if ( GetFontData(hDC, dwTable, (DWORD)ofs, buf,(DWORD)rwsiz )
			!= (DWORD)rwsiz)
			goto	ERET;

		if ( aligne ) {
			memset( buf+siz, 0, filsiz);
			rwsiz += filsiz;
		}
		chksum += calchksum( buf, rwsiz);

		BOOL res =  WriteFile( ofHdl, buf, (unsigned int)rwsiz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)rwsiz)
			goto	ERET;
		siz -= rwsiz;
		ofs += rwsiz;
	}
	*cs = chksum;
	return 0;
ERET:
	return -1;
}
static int
mergeblock( HDC hDC, struct TableEntry *entry, HANDLE ofHdl)
{
	long	ofs;
	char	*mem;
	unsigned long	cs;	/* CheckSum value */

	mem = (char *)malloc((size_t) RWBUFSIZ);
	if ( mem==(char *)0)
		return -1;
	/* Obtain start offset of Output table */
	ofs = SetFilePointer( ofHdl, 0L, NULL, FILE_CURRENT);

	if (copyblk( hDC, entry->tagName, ofHdl, entry->siz, mem, RWBUFSIZ, &cs))
		goto	ERET;
	entry->ofs = ofs;
	entry->checkSum = cs;
	free( mem);
	return	0;
ERET:
	free( mem);
	return -1;
}
static int
tableChkSum( HANDLE fH, struct TableEntry *te)
{
	char	*rwbuf;
	unsigned long	cs;
	long	lsiz;
	int	rwsiz;

	if ( te->siz==0)
		return 0;
	if ((rwbuf = (char *)malloc( (size_t)RWBUFSIZ))==(char *)0)
		return -1;
	if ( (long) SetFilePointer( fH, te->ofs, NULL, FILE_BEGIN)!=te->ofs)
		goto	ERET;
	cs = 0;
	lsiz = te->siz;
	while ( lsiz > 0) {
		if (lsiz > RWBUFSIZ)
			rwsiz = RWBUFSIZ;
		else	rwsiz = (int)lsiz;
		lsiz -= rwsiz;
		cs += calchksum( rwbuf, rwsiz);
	}
	te->checkSum = cs;
	free( rwbuf);
	return 0;
ERET:
	free( rwbuf);
	return -1;
}
static int
codeToGID( unsigned short code)
{
	return	(int)( code - (unsigned short)EUDCCODEBASE +2);
}
int
TTFReadHdr ( HANDLE fHdl, struct TTFHeader *hdr)
{
	DWORD nBytesRead;
	SetFilePointer( fHdl, 0L, NULL, FILE_BEGIN);
	BOOL res=ReadFile( fHdl, hdr, sizeof(struct TTFHeader),&nBytesRead, NULL);
	if (!res || nBytesRead !=sizeof(struct TTFHeader)) {
		return -1;
	}
	smtoi( &hdr->numTables);
	smtoi( &hdr->searchRange);
	smtoi( &hdr->entrySelector);
	smtoi( &hdr->rangeShift);
	return	0;
}
int
TTFWriteHdr ( HANDLE fHdl, struct TTFHeader *hdr)
{
	struct TTFHeader whdr;

	whdr = *hdr;
	sitom( &whdr.numTables);
	sitom( &whdr.searchRange);
	sitom( &whdr.entrySelector);
	sitom( &whdr.rangeShift);

	if (SetFilePointer( fHdl, 0L, NULL,  FILE_BEGIN)!=0L)
		return -1;

	DWORD nByte;
	BOOL res = WriteFile( fHdl, (BYTE *) &whdr, sizeof(struct TTFHeader), &nByte, NULL);
	if (!res || nByte !=sizeof(struct TTFHeader))
		return -1;

	return	0;
}
int
TTFReadDirEntry( HANDLE fHdl, struct TableEntry *entry, int eCnt)
{
	long	ofs;
	DWORD nByte;
	BOOL res;


	ofs = sizeof (struct TTFHeader);
	if ( (long) SetFilePointer( fHdl, ofs, NULL, FILE_BEGIN)!=ofs)
		return -1;
	res = ReadFile( fHdl, entry, (unsigned int)(sizeof(struct TableEntry)*eCnt), &nByte, NULL);
	if (!res || nByte !=(unsigned int)sizeof(struct TableEntry)*eCnt)
		return -1;
	while ( eCnt-->0) {
		lmtoi((long *)&entry->checkSum);
		lmtoi(&entry->ofs);
		lmtoi(&entry->siz);
		entry++;
	}
	return	0;
}
int
TTFWriteDirEntry( HANDLE fHdl, struct TableEntry *entry, int eCnt)
{
	long	ofs;
struct TableEntry wentry;
	DWORD nByte;
	BOOL res;

	ofs = sizeof (struct TTFHeader);
	if ( (long) SetFilePointer( fHdl, ofs, NULL, FILE_BEGIN)!=ofs)
		return -1;

	/* Write Entries */
	while ( eCnt-->0) {
		wentry = *entry++;
		litom((long *)&wentry.checkSum);
		litom(&wentry.ofs);
		litom(&wentry.siz);
		res = WriteFile( fHdl,(char *) &wentry, sizeof wentry, &nByte, NULL);
		if (!res || nByte !=sizeof wentry) {
			return -1;
		}
	}
	return	0;
}
static struct TableEntry *
searchEntry(struct TableEntry *entry, int eCnt, char *tag)
{
	while ( eCnt-->0) {
		if (memcmp( entry->tagName, tag, TAGSIZ)==0)
			return entry;
		entry++;
	}
	return (struct TableEntry *)0;
}
int
TTFGetTableEntry( HANDLE fH, struct TableEntry *entry, char *tag)
{
struct TTFHeader	hdr;
struct TableEntry	*te, *rte;
	int	msiz;

	te = 0;
	/* Read TTF Header to get numTables */
	if (TTFReadHdr( fH, &hdr))
		goto	ERET;
	msiz = sizeof(struct TableEntry)*hdr.numTables;
	if ( (te = (struct TableEntry *)malloc((size_t)msiz))==(struct TableEntry *)0)
		goto	ERET;

	/* Read entry whole */
	if ( TTFReadDirEntry( fH, te, hdr.numTables))
		goto	ERET;

	/* Search for entry with tag */
	if ((rte = searchEntry(te, (int)hdr.numTables, tag))==0)
		goto	ERET;
	*entry = *rte;
	free(te);
	return 0;
ERET:
	if (te)	free(te);
	return -1;
}
/***********************************************************************
 *	Read Table at once with entry data
 */
/* */	int
/* */	TTFReadTable(
/* */		HANDLE fH,
/* */	struct TableEntry *entry,
/* */		void	*buf,
/* */		int	bufsiz)
/*
 *	returns :  read size
 ***********************************************************************/
{
	unsigned int	rdsiz;
	DWORD nByte;
	BOOL res;

	if ((long) SetFilePointer( fH, entry->ofs, NULL, FILE_BEGIN)!=entry->ofs)
		goto	ERET;
	rdsiz = bufsiz >= (int)entry->siz ? (unsigned int)entry->siz
						: (unsigned int)bufsiz;
	res = ReadFile( fH, buf, rdsiz, &nByte, NULL);
	if (!res || nByte!=rdsiz)
		goto	ERET;
	if ( rdsiz < (unsigned int)bufsiz)
		memset((unsigned char *)buf + rdsiz, 0, bufsiz - rdsiz);
	return (int)rdsiz;
ERET:
	return -1;
}
/***********************************************************************
 *	Read Fixed Size Table at onece with tag
 */
/* */	int
/* */	TTFReadFixedTable(
/* */		HANDLE	fH,
/* */		char	*buf,
/* */		int	bufsiz,
/* */		char	*tag)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
struct TableEntry	te;

	if (TTFGetTableEntry( fH, &te, tag))
		goto	ERET;
	if ( TTFReadTable( fH, &te, buf, bufsiz )!=bufsiz)
		goto	ERET;
	return 0;
ERET:
	return -1;
}

/***********************************************************************
 *	Read Variable Size Table at onece with tag
 */
/* */	int
/* */	TTFReadVarTable(
/* */		HANDLE	fH,
/* */		char	**buf,		/* Buffer pointer (be set) */
/* */		unsigned int	*bufsiz,	/* Buffer Size ( be set) */
/* */		char	*tag)		/* Tag name */
/*
 *	returns : 0, -1
 *	remarks : allocated memory must be free by caller
 ***********************************************************************/
{
struct TableEntry	te;
	char	*mem;

	if (TTFGetTableEntry( fH, &te, tag))
		goto	ERET;
	if ((mem = (char *)malloc( (size_t)te.siz))==(char *)0)
		goto	ERET;
	if ( TTFReadTable( fH, &te, mem, (int)te.siz )!=(int)te.siz)
		goto	ERET;
	*buf = mem;
	*bufsiz = (unsigned int)te.siz;

	return 0;
ERET:
	return -1;
}

/***********************************************************************
 *	Write Table
 */
/* */	int
/* */	TTFWriteTable(
/* */		HANDLE	fH,
/* */	struct TableEntry *entry,
/* */		void	*buf,
/* */		int	bufsiz)
/*
 *	returns :  written size
 *	remarks : update checksum
 ***********************************************************************/
{
	int	wsiz;
	DWORD nByte;
	BOOL res;

	if ((long) SetFilePointer( fH, entry->ofs, NULL, FILE_BEGIN)!=entry->ofs)
		goto	ERET;
	wsiz = bufsiz >= (int)entry->siz ? (int)entry->siz : bufsiz;
	res = WriteFile( fH, (char *)buf, (unsigned int)wsiz, &nByte, NULL);
	if (!res || nByte !=(unsigned int)wsiz)
		goto	ERET;
	entry->checkSum = calchksum( (char *)buf, bufsiz);
	return wsiz;
ERET:
	return -1;
}
/***********************************************************************
 *	Write Table
 */
/* */	int
/* */	TTFAppendTable(
/* */		HANDLE	fH,
/* */	struct TableEntry *entry, 	/* value be set */
/* */		void	*buf,
/* */		int	siz)
/*
 *	returns : 0, -1
 *	remarks : enttry->siz  not aligned 32bit
 ***********************************************************************/
{
	long	ofs;
	BYTE	pad[4];
	int	padsiz;
	DWORD nByte;
	BOOL res;

	if ( (ofs=(long) SetFilePointer( fH, 0L, NULL, FILE_CURRENT))<0L)
		goto	ERET;

	res = WriteFile( fH, buf, (unsigned int)siz, &nByte, NULL);
	if (!res || nByte !=(unsigned int)siz)
		goto	ERET;
	
	/* 32 bit Word aligne */
	if ( siz % 4) {
		padsiz = 4 - siz%4;
		memset(pad,0,4);
		res = WriteFile(fH,pad, (unsigned int)padsiz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)padsiz)
			goto	ERET;
	}
	entry->ofs = ofs;
	entry->siz = siz;
	entry->checkSum = calchksum((char *) buf, siz);
	return 0;
ERET:
	return -1;
}
/***********************************************************************
 *	Merge Table
 */
/* */	static int
/* */	TTFMergeTable(
/* */		HDC	hDC,
/* */		HANDLE	nfh,
/* */		char	tag[4],
/* */	struct TableEntry	*nte)
/*
 *	returns : 0
 ***********************************************************************/
{
	if (TTFGetOrgTableEntry( hDC, nte, tag))
		return -1;
	if (mergeblock( hDC, nte, nfh))
		return -1;
	return 0;
}
int
#ifdef BUILD_ON_WINNT
__cdecl
#endif // BUILD_ON_WINNT
compentry( const void *e1, const void *e2)
{
struct TableEntry *te1, *te2;

	te1 = (struct TableEntry *)e1;
	te2 = (struct TableEntry *)e2;
	return memcmp(te1->tagName, te2->tagName,4);
}
static void
SortEntry(struct TableEntry *ebuf, int num)
{
	if ( num <=0)	return;
	qsort( ebuf, num, sizeof(struct TableEntry), compentry);
}
static int
fileChkSum(
	HANDLE fh,
	struct TableEntry *entry,
	int numEntry,
	struct TTFHeader *hdr,
	struct HeadTable *head)
{
	unsigned long	lval, cs;
	unsigned long	*lp;
	struct TableEntry *e;
	int	n;
	struct TableEntry	*headp;
	DWORD nByte;
	BOOL res;

	e = entry;
	n = numEntry;
	cs = 0;
	head->chkSum = 0;
	headp = 0;
	while ( n-->0) {
		lp = (unsigned long *)e->tagName;
		if (memcmp( e->tagName,"head", 4)==0)
			headp = e;
		lval = *lp;
		lmtoi( (long *)&lval);
		cs += lval;
		cs += e->checkSum;
		cs += e->checkSum;
		cs += e->ofs;
		cs += e->siz;
		e++;
	}
	if (headp==0)
		return -1;

	cs += calchksum( (char *)hdr, sizeof(struct TTFHeader) );

	if ((long) SetFilePointer( fh, headp->ofs, NULL, FILE_BEGIN)!=headp->ofs)
		goto	ERET;
	head->chkSum = (unsigned long)0xb1b0afbaL - cs;
	litom( (long *)&head->chkSum);
	res = WriteFile(fh,(char *)head,sizeof(struct HeadTable), &nByte, NULL);
	if (!res || nByte !=sizeof(struct HeadTable))
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static void
makeTTFHeader( struct TTFHeader *hdr, int nTbl)
{
	int	po;
	int	n;
	int	ponum;

	po = 0;
	n = nTbl;
	ponum = 1;
	while ( n>=2) {
		n /= 2;
		po++;
		ponum*=2;
	}
	hdr->sfnt_version[0]= hdr->sfnt_version[2]= hdr->sfnt_version[3]= 0;
	hdr->sfnt_version[1]= 0x01;
	hdr->numTables = (short)nTbl;
	hdr->searchRange = (short)ponum*16;
	hdr->entrySelector = (short)po;
	hdr->rangeShift = (short)( nTbl*16)-hdr->searchRange;

	return ;
}
/***********************************************************************
 *	make GlyphData from lst
 */
/* */	static int
/* */	makeGlyphData(
/* */		int lstH,
/* */		struct BBX *bbx,	/* Set value to header */
/* */		char	*glyphData,
/* */		int	bufsiz,
/* */		int	*gdatsiz)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	char	*flgp, *sflgp;
	char	flag;
	char	*dummyp;
struct VHEAD	*vhd,
		*svhd;
struct glyfHead	ghdr;
	int	nCnt;
struct VDATA	*vp;
	int	nPnt;
	int	pntSum;
	int	cnt;
	short	sval;
	short	px, py;
	short	relx, rely;
	char	cval;

	initsetgbuf ( glyphData, bufsiz);
	if( ( nCnt = VDGetNCont( lstH))<0)
		goto	ERET;

	else  if (nCnt == 0) {	/* Space Character */
			*gdatsiz = 0;
			return	0;
	
	}

	ghdr.numberOfContour = (short)nCnt;
	sitom( &ghdr.numberOfContour );
	ghdr.xMin = (short)bbx->xMin;
	sitom( &ghdr.xMin);
	ghdr.yMin = (short)bbx->yMin;
	sitom( &ghdr.yMin);
	ghdr.xMax = (short)bbx->xMax;
	sitom( &ghdr.xMax);
	ghdr.yMax = (short)bbx->yMax;
	sitom( &ghdr.yMax);
	if (setgbuf( (char *)&ghdr, sizeof ghdr, &flgp))
		goto	ERET2;
	if (VDGetHead( lstH, &vhd))
		goto	ERET;
	pntSum = 0;
	svhd = vhd;
	/* Set each Contour Last point number */
	for ( cnt = 0; cnt < nCnt; cnt++) {
		pntSum +=  vhd->nPoints;
		sval = pntSum-1;
		sitom( &sval);
		if ( setgbuf((char *)&sval, sizeof( short), &flgp))
			goto	ERET2;
		vhd = vhd->next;
	}
	/* Toatal number of instructions */
	sval = 0;
	if ( setgbuf((char *)&sval, sizeof( short), &flgp))
		goto	ERET2;

	/* Set flags */
	vhd = svhd;
	px = py = 0;
	for ( cnt = 0; cnt < nCnt; cnt++) {
		vp = vhd->headp;
		nPnt = vhd->nPoints;
		while ( nPnt-->0) {
			flag = (vp->vd.atr&1)==0 ? (char)GLYF_ON_CURVE :(char)0;
			relx = vp->vd.x - px;
			rely = vp->vd.y - py;
			if ( relx == 0)
				flag |= GLYF_X_SAME;
			else if ( relx > 0 && relx < 256)
				flag |= GLYF_X_SHORT_P;
			else if ( relx < 0 && relx > -256)
				flag |= GLYF_X_SHORT_N;
			if ( rely == 0)
				flag |= GLYF_Y_SAME;
			else if ( rely > 0 && rely < 256)
				flag |= GLYF_Y_SHORT_P;
			else if ( rely < 0 && rely > -256)
				flag |= GLYF_Y_SHORT_N;

			if (setgbuf((char *) &flag, 1, &dummyp))
				goto	ERET2;
			px = vp->vd.x;
			py = vp->vd.y;
			vp = vp->next;
		}
		vhd = vhd->next;
	}
	/* set X */
	vhd = svhd;
	sflgp = flgp;
	px = 0;
	for ( cnt = 0; cnt < nCnt; cnt++) {
		vp = vhd->headp;
		nPnt = vhd->nPoints;
		while ( nPnt-->0) {
			relx = vp->vd.x - px;
			if ( *flgp & GLYF_X_SHORT) {
				if ( (*flgp & GLYF_X_SAME)==0)
					relx = -relx;
				cval = (char)relx;
				if ( setgbuf( &cval, 1, &dummyp))
					goto	ERET2;
			}
			else if ( *flgp & GLYF_X_SAME)
				;
			else {
				sitom( &relx);
				if ( setgbuf( (char *)&relx, 2, &dummyp))
					goto	ERET2;
			}
			flgp++;
			px = vp->vd.x;
			vp = vp->next;
		}
		vhd = vhd->next;
	}
	/* set Y */
	vhd = svhd;
	flgp = sflgp;
	py = 0;
	for ( cnt = 0; cnt < nCnt; cnt++) {
		vp = vhd->headp;
		nPnt = vhd->nPoints;
		while ( nPnt-->0) {
			rely = vp->vd.y - py;
			if ( *flgp & GLYF_Y_SHORT) {
				if ( (*flgp & GLYF_Y_SAME)==0)
					rely = -rely;
				cval = (char)rely;
				if ( setgbuf( &cval, 1, &dummyp))
					goto	ERET2;
			}
			else if ( *flgp & GLYF_Y_SAME)
				;
			else {
				sitom( &rely);
				if ( setgbuf((char *)&rely, 2, &dummyp))
					goto	ERET2;
			}
			flgp++;
			py = vp->vd.y;
			vp = vp->next;
		}
		vhd = vhd->next;
	}

	*gdatsiz = termbuf();
	return 0;
ERET:
	return -1;
ERET2:
	return -2;
}
static char	*sbuf;
static int	limcnt;
static char	*setp;
static int	bcnt;
static void
initsetgbuf( char *b, int lim)
{
	setp = sbuf = b;
	limcnt = lim;
	bcnt = lim;	/* decremental counter */
	return;
}
static int
setgbuf( char *dat, int siz, char **np)
{
	if ( siz > bcnt)	return -1;
	memcpy( setp, dat, siz);
	setp += siz;
	bcnt -= siz;
	*np = setp;
	return 0;
}
static int
termbuf( )
{
	int	siz;

	siz = limcnt - bcnt;
	siz = siz % 4;
	if ( siz) {
		while ( siz++<4) {
			*setp++=0;
			bcnt--;
		}
	}
	return limcnt - bcnt;
}
static int
strwlen( char *s)
{
	int	len;

	len = 0;
	while (*s ||  *(s+1)) {
		s+=2;
		len++;
	}
	return len*2;
}
static void
strwcat( char *s1, char *s2)
{
	while (*s1 || *(s1+1))
		s1+=2;

	while ( *s2 || *(s2+1)) {
		*s1++ = *s2++;
		*s1++ = *s2++;
	}
	*s1++ = 0;
	*s1++ = 0;
	
	return;
}
static void
setnamebuf( char *buf, int *siz, short EncodingID)
{
	int	strsiz;
	short	strofs;
	char	*strp;
struct NamingTable	*nt;
struct NameRecord	*nr;
	int	rec;
	int	slen;

	strofs = sizeof(struct NamingTable)
		+ sizeof(struct NameRecord)*NUMOFNAMEREC;
	strsiz = 0;
	strp = buf +strofs;
	*strp=(char)0;
	*(strp+1)=(char)0;
	nt = (struct NamingTable *)buf;
	nt->OfsToStr = strofs;
	sitom( &nt->OfsToStr);
	nr = (struct NameRecord *)(buf + sizeof(struct NamingTable));
	/* Set name record */
	strofs = 0;
	for ( rec = 0; rec<NUMOFNAMEREC; rec++, nr++) {
		nr ->PlatformID = 3;
		sitom(&nr ->PlatformID);

		nr ->PlatformSpecEncID = EncodingID;
		sitom(&nr ->PlatformSpecEncID);

		nr ->LanguageID = (short)CountryInfo.LangID;
		sitom(&nr ->LanguageID);

		nr ->NameID = (short)rec;
		sitom(&nr ->NameID);

		strwcat( strp,namerecstr[rec]);
		slen = strwlen(namerecstr[rec]);

		nr -> StringLength = (short)slen;
		sitom(&nr -> StringLength );
		strsiz += slen;

		nr -> StringOfs = strofs;
		sitom(&nr -> StringOfs);
		strofs += (short)slen;
	}
	nt->FormSel = 0;
	nt->NRecs = NUMOFNAMEREC;
	sitom( &nt->NRecs);
	*siz = strsiz+ sizeof(struct NamingTable)
			+ sizeof(struct NameRecord)*NUMOFNAMEREC;
}
static void
modifyOS2( char *buf)
{
	/* Allow all license */
	*(buf+9) = (char)0x00;
	/* Set aulUnicodeRange  Privete Use Area bit*/
	*(buf+0x31) |= 0x31;
}
static void
modifyhead( struct HeadTable *head)
{
	head->chkSum=0L;
	memset( head->createdDate, 0, 8);
	memset( head->updatedDate, 0, 8);
	head->indexToLocFormat = 1;
	head->glyphDataFormat = 0;
	sitom( &head->indexToLocFormat );
	sitom( &head->glyphDataFormat);
}
/*
 *	Set Vertical Header with Horizontal Header and Bounding Box
 */
static void
setVhea( struct VheaTable *vhea, struct HheaTable *hhea, struct BBX *bbx)
{

	memcpy( vhea, hhea, sizeof(struct HheaTable));
	vhea->minTopSideBearing = 0;
	vhea->minBottomSideBearing = 0;
	vhea->caretSlopeRise = 0;
	vhea->caretSlopeRun = 1;
	sitom( &vhea->caretSlopeRun );
	vhea->numOfLongVerMetrics = (short)numOfGlyph;
	sitom( &vhea->numOfLongVerMetrics );
}
static int
updateMaxp( struct MaxpTbl *maxp, int lstHdl)
{
struct VHEAD	*vhd;
	short	sval;
	int	nCnt;
	int	ttlPnt;
	int	updflg;

	updflg = 0;	
	if ( (nCnt = VDGetNCont( lstHdl))<0)
		goto	ERET;
	else if ( nCnt ==0)
		goto	RET;

	sval = maxp->maxContours;
	smtoi( &sval);
	if ( sval < nCnt) {
		sval = (short)nCnt;
		sitom(&sval);
		maxp->maxContours = sval;
		updflg = 1;	
	}
	if (VDGetHead( lstHdl, &vhd))
		goto	ERET;
	ttlPnt = 0;
	while ( nCnt-->0) {
		ttlPnt += vhd->nPoints;
		vhd = vhd->next;
	}
	sval = maxp->maxPoints;
	smtoi( &sval);
	if ( sval < ttlPnt) {
		sval = (short)ttlPnt;
		sitom(&sval);
		maxp->maxPoints = sval;
		updflg = 1;	
	}
RET:
	return updflg;
ERET:
	return -1;
}
/***********************************************************************
 *	Obtain Original TTF Table Size
 */
/* */	static int
/* */	TTFGetOrgTableEntry(
/* */		HDC	hDC,
/* */		struct TableEntry	*entry,
/* */		char	*tag)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	DWORD dwTable;	/* Metric Table to request */
	DWORD 	siz;

	dwTable = *(DWORD *)tag;
	siz = GetFontData(hDC, dwTable, (DWORD)0, (char *)0, (DWORD)0);
	if ( siz<0)	return -1;
	else {
		memcpy( entry->tagName, tag, TAGSIZ);
		entry->siz = (long)siz;
		return 0;
	}
}
/***********************************************************************
 *	Read Original TTF Table
 */
/* */	int
/* */	TTFReadOrgFixedTable (
/* */		HDC	hDC,		/* Handle to DC */
/* */		char	*buf,		/* Read Buffer */
/* */		int	bufsiz,		/* Buffer Siz */
/* */		char	*tag)		/* TagName */
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	DWORD dwTable;	/* Metric Table to request */
	DWORD	siz;

	dwTable = *(DWORD *)tag;
	siz = GetFontData(hDC, dwTable, (DWORD)0, buf,(DWORD)bufsiz );
	if ( (int)siz != bufsiz)
		return -1;
	else	return 0;
}
/***********************************************************************
 *	Read Variable Size Table at onece with tag
 */
/* */	int
/* */	TTFReadOrgVarTable(
/* */		HDC	hDC,
/* */		char	**buf,		/* Buffer pointer (be set) */
/* */		unsigned int	*bufsiz,	/* Buffer Size ( be set) */
/* */		char	*tag)		/* Tag name */
/*
 *	returns : 0, -1
 *	remarks : allocated memory must be free by caller
 ***********************************************************************/
{
struct TableEntry	te;
	char	*mem;

	if (TTFGetOrgTableEntry( hDC, &te, tag))
		goto	ERET;
	if ((mem = (char *)malloc( (size_t)te.siz))==(char *)0)
		goto	ERET;
	if ( TTFReadOrgFixedTable ( hDC, mem,(int)te.siz , tag))
		goto	ERET;
	*buf = mem;
	*bufsiz = (unsigned int)te.siz;

	return 0;
ERET:
	return -1;
}
static void
setCountryData(short EncodingID)
{
	unsigned short lastCode;

	makeUniCodeTbl();

	lastCode = getMaxUniCode();

	numOfGlyph = lastCode - EUDCCODEBASE +1 + 2; /* +null, +missing */

	cmapdata[6] = (char)((EncodingID>>8)&(unsigned short)0xff);
	cmapdata[7] = (char)(EncodingID&(unsigned short)0xff);
	cmapdata[26] = (char)((lastCode>>8)&(unsigned short)0xff);
	cmapdata[27] = (char)(lastCode&(unsigned short)0xff);

	maxpdata[4] = (char)((numOfGlyph>>8)&(unsigned short)0xff);
	maxpdata[5] = (char)(numOfGlyph&(unsigned short)0xff);
}
static int
WIFEOS2( HDC hDC, char **os2buf, int *os2TblSiz, struct BBX *bbx)
{
	struct OS2Table *os2tbl;
	int	siz;
	TEXTMETRIC	tm;
static PANOSE	msminPanose= { (char)2, (char)2, (char)6, (char)9, (char)4,
			(char)2, (char)5, (char)8, (char)3, (char)4};

	if ( GetTextMetrics( hDC, &tm)==0)
		goto	ERET;
	siz = sizeof( struct OS2Table);
	if ((os2tbl = (struct OS2Table *)malloc(sizeof(struct OS2Table)))==0)
		goto	ERET;
	memset( os2tbl, 0, sizeof(struct OS2Table));

	os2tbl->version = 1;
	sitom( (short *)&os2tbl->version);
	os2tbl->xAvgCharWidth = bbx->xMax+1;
	sitom( &os2tbl->xAvgCharWidth );
	os2tbl->usWeightClass = tm.tmWeight? (unsigned short)tm.tmWeight : 500;
	sitom( (short *)&os2tbl->usWeightClass );
	os2tbl->usWidthClass = 5; /* Medium */
	sitom( (short *)&os2tbl->usWidthClass );
	os2tbl->fsType = 0x0000; /* Allow all liscence */
	sitom( &os2tbl->fsType );

	os2tbl->ySubscriptXSize = (bbx->xMax+1)/2;	/* 1/4kaku */
	sitom( &os2tbl->ySubscriptXSize );
	os2tbl->ySubscriptYSize = (bbx->yMax+1)/2;	/* 1/4kaku */
	sitom( &os2tbl->ySubscriptYSize );
	os2tbl->ySubscriptXOffset = 0;			/* 1/4kaku */
	sitom( &os2tbl->ySubscriptXOffset );
	os2tbl->ySubscriptYOffset = 0;			/* 1/4kaku */
	sitom( &os2tbl->ySubscriptYOffset );

	os2tbl->ySuperscriptXSize = (bbx->xMax+1)/2;	/* 1/4kaku */
	sitom( &os2tbl->ySuperscriptXSize );
	os2tbl->ySuperscriptYSize = (bbx->yMax+1)/2;	/* 1/4kaku */
	sitom( &os2tbl->ySuperscriptYSize );
	os2tbl->ySuperscriptXOffset = (bbx->xMax+1)/2;	/* 1/4kaku */
	sitom( &os2tbl->ySuperscriptXOffset );
	os2tbl->ySuperscriptYOffset = (bbx->yMax+1)/2;	/* 1/4kaku */
	sitom( &os2tbl->ySuperscriptYOffset );

	os2tbl->yStrikeoutSize = bbx->yMax/20;		/* 5% */
	sitom( &os2tbl->yStrikeoutSize );
	os2tbl->yStrikeoutPosition = bbx->yMax/4;	/* 25% */
	sitom( &os2tbl->yStrikeoutPosition );

	os2tbl->sFamilyClass = 0;	/* no classification */
	sitom( &os2tbl->sFamilyClass );
	os2tbl->panose = msminPanose;

	os2tbl->fsSelection = 0x40;
	sitom( (short *)&os2tbl->fsSelection );

	os2tbl->usFirstCharIndex = 0x20;
	sitom( (short *)&os2tbl->usFirstCharIndex );
	os2tbl->usLastCharIndex = 0xffe5;
	sitom( (short *)&os2tbl->usLastCharIndex );
	os2tbl->sTypoAscender = bbx->yMax+1;
	sitom( &os2tbl->sTypoAscender );
	os2tbl->sTypoDescender = -(bbx->yMax/8);
	sitom( &os2tbl->sTypoDescender );
	os2tbl->sTypoLineGap = 0;
	sitom( &os2tbl->sTypoLineGap );

	os2tbl->usWinAscent = bbx->yMax+1;
	sitom( (short *)&os2tbl->usWinAscent );
	os2tbl->usWinDescent = bbx->yMax/8;
	sitom( (short *)&os2tbl->usWinDescent );
	*os2TblSiz = sizeof( struct OS2Table);
	*os2buf = (char *)os2tbl;
	return 0;
ERET:
	return -1;
}
static void
WIFEhhea( struct HheaTable *hhea, struct BBX *bbx)
{
	memset( hhea, 0, sizeof(struct HheaTable));

	hhea->version[1] = (char)1;
	hhea->Ascender = bbx->yMax+1;
	sitom( &hhea->Ascender);
	hhea->Descender = -(bbx->yMax/8);
	sitom( &hhea->Descender);
	hhea->LineGap =  0;
	sitom( &hhea->LineGap);

	hhea->advanceWidthMax =  bbx->xMax+1;
	sitom( &hhea->advanceWidthMax);
	hhea->minLeftSideBearing =  0;
	sitom( &hhea->minLeftSideBearing);
	hhea->minRightSideBearing = 0;
	sitom( &hhea->minRightSideBearing);

	hhea->xMaxExtent =  bbx->xMax+1;
	sitom( &hhea->xMaxExtent);
	hhea->caretSlopeRise = 1;
	sitom( &hhea->caretSlopeRise);
	hhea->caretSlopeRun = 0;
	sitom( &hhea->caretSlopeRun);

	hhea->numberOfHMetrics = (short)numOfGlyph;
	sitom( &hhea->numberOfHMetrics);

}
static void
WIFEhead( struct HeadTable *head, struct BBX *bbx)
{
	memset( head, 0, sizeof( struct HeadTable));

	head->version[1] = 0x1;
	head->magicNumber=0x5f0F3CF5L;
	litom((long *)&head->magicNumber);
	head->flags = 0;
	sitom( &head->flags);
	head->unitsPerEm = bbx->xMax - bbx->xMin+1;
	head->unitsPerEm = ((bbx->xMax - bbx->xMin+1)*9+4)/8;
	sitom( &head->unitsPerEm);

	head->xMin = (short)bbx->xMin;
	sitom( &head->xMin);
	head->xMax = (short)bbx->xMax;
	sitom( &head->xMax);
//	head->yMin = (short)bbx->yMin;
	head->yMin = -(bbx->yMax/8);
	sitom( &head->yMin);
	head->yMax = (short)bbx->yMax;
	sitom( &head->yMax);

	head->lowestRecPPEM = 25;
	sitom( &head->lowestRecPPEM);
	head->fontDirectionHint = 1;
	sitom( &head->fontDirectionHint);
	head->indexToLocFormat = 1;
	sitom( &head->indexToLocFormat);

}
static void
WIFEpost( struct postTable *post)
{
	memset( post, 0, sizeof( struct postTable));
	post->FormatType[1]=0x3;
	post->underlineThickness = 12;
	sitom( &post->underlineThickness);
	post->isFixedPitch = 1;
	litom( (long *)&post->isFixedPitch);
}
static short
getEncID( HDC hDC, int fontType)
{
	if ( CountryInfo.LangID==EUDC_KRW)
		return (short)1;
	else	return (short)1;
}
/***********************************************************************
 *	Create TTF
 */
/* */	int
/* */	TTFCreate(
/* */		HDC hDC, 		/* Handle to DC */
/* */		TCHAR *newf,		/* Create TTF Path */
/* */		struct BBX	*bbx,	/* the same as original 'head'*/
/* */					/* but minX should be 0 */
/* */		int	lstHdl,		/* missing glyf list */
/* */					/* bbx, hM, LstHdl is for Missing Glyf*/
/* */		int	fontType)	/* 0:TrueType, 1:WIFE */
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	HANDLE	nfh;	
struct TTFHeader	hdr;
struct TableEntry	*te,		/* Directory Entry */
			nte;
	char		*nameBuf;
	int		nameBufSiz;
struct HheaTable	hhea;
struct VheaTable	vhea;
struct HeadTable	head;
struct HMetrics		*hmet;
struct VMetrics		*vmet;
	long		*loca;
	long		ofs;
	char	*glyphData;
	int	gdatsiz;
	int	i;
	char	*os2buf;
	int	os2siz;
struct BBX	cbbx;		/* bounding box for each glyph*/
struct HMetrics		hM;	/* horizontal metrics for each glyph */
struct VMetrics		vM;	/* Vertical one */
struct postTable	postTbl; /* post table */
	short	EncodingID;
	DWORD nByte;
	BOOL res;

//	nfh = -1;
	te =  0;
	nameBuf = 0;
	hmet = 0;
	loca = 0;
	glyphData = 0;
	os2buf = 0;

	/* Determin PlatformSpcificEncodingID */
	if ((EncodingID = getEncID( hDC, fontType))<(short)0)
		goto	ERET;
	/* Set cmapdata,maxpdata,numOfGlyph,lastCode */
	setCountryData(EncodingID);

	/* Open files */
	nfh = CreateFile(newf,
					GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_DELETE,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( nfh  == INVALID_HANDLE_VALUE)
		goto	ERET;
	
	/* Write header */
	makeTTFHeader( &hdr, NUMTABLES);
	if ( TTFWriteHdr( nfh, &hdr))
		goto	ERET;

	/* Allocate dir entry area */
	if ( (te =(struct TableEntry *)malloc((size_t)sizeof(struct TableEntry)*NUMTABLES))==0)
		goto	ERET;

	/*
	 * DirEntry Dummy Write
	 */
	memset( te, 0, sizeof(struct TableEntry)*NUMTABLES);
	res = WriteFile( nfh, te, sizeof(struct TableEntry)*NUMTABLES, &nByte, NULL);
	if (!res || nByte != sizeof(struct TableEntry)*NUMTABLES)
		goto	ERET;

	initDirEntry(te, NUMTABLES );

	/*
	 * 'cvt ' Write
	 */
	if (TTFAppendTable( nfh, &nte, cvtdata, sizeof cvtdata))
		goto	ERET;
	if ( setDirEntry( &nte, "cvt "))
		goto	ERET;
	/*
	 * 'fpgm' Write
	 */
	if (TTFAppendTable( nfh, &nte, fpgmdata, sizeof fpgmdata))
		goto	ERET;
	if ( setDirEntry( &nte, "fpgm"))
		goto	ERET;
	/*
	 * 'prep' write
	 */
	if (TTFAppendTable( nfh, &nte, prepdata, sizeof prepdata))
		goto	ERET;
	if ( setDirEntry( &nte, "prep"))
		goto	ERET;
	/*
	 * 'name' write
	 */
	if ( (nameBuf = (char *)malloc((size_t)NAMEBUFSIZ))==0)
		goto	ERET;
	setnamebuf(nameBuf, &nameBufSiz, EncodingID);
	if (TTFAppendTable( nfh, &nte, nameBuf, nameBufSiz))
		goto	ERET;
	free( nameBuf);
	nameBuf = 0;
	if ( setDirEntry( &nte, "name"))
		goto	ERET;

	/*
	 * 'cmap' Write
	 */
	if (TTFAppendTable( nfh, &nte, cmapdata, sizeof cmapdata))
		goto	ERET;
	if ( setDirEntry( &nte, "cmap"))
		goto	ERET;
	/*
	 * 'OS/2' Copy -> modify
	 */
	if ( fontType==0) {
		if ( TTFReadOrgVarTable( hDC, &os2buf, (unsigned int *)&os2siz,
						"OS/2"))
			goto	ERET;
		modifyOS2( os2buf);
	}
	else {
		if (WIFEOS2( hDC, &os2buf, &os2siz, bbx))
			goto	ERET;
	}
	if (TTFAppendTable( nfh, &nte, os2buf,  os2siz))
		goto	ERET;
	if ( setDirEntry( &nte, "OS/2"))
		goto	ERET;
	free( os2buf);
	/*
	 * 'post' Copy
	 */
	if ( fontType==0) {
		if ( TTFMergeTable( hDC, nfh, "post", &nte))
			goto	ERET;
	}
	else {
		WIFEpost( &postTbl);
		if (TTFAppendTable( nfh,&nte, (char *)&postTbl,sizeof(struct postTable)))
			goto	ERET;
	}
	if ( setDirEntry( &nte, "post"))
		goto	ERET;
	/*
	 * 'hhea' Read,modify and write
	 */
	if ( fontType==0) {
		if (TTFReadOrgFixedTable( hDC, (char *)&hhea, sizeof hhea,"hhea"))
			goto	ERET;

		hhea.metricDataFormat = 0;
		hhea.numberOfHMetrics = (short)numOfGlyph;
		sitom( &hhea.numberOfHMetrics );
	}
	else {
		WIFEhhea( &hhea, bbx);
	}
	if (TTFAppendTable( nfh, &nte, &hhea, sizeof hhea))
		goto	ERET;
	if ( setDirEntry( &nte, "hhea"))
		goto	ERET;

	/*
	 * 'vhea'
	 */
	setVhea( &vhea, &hhea, bbx);
	if (TTFAppendTable( nfh, &nte, &vhea, sizeof vhea))
		goto	ERET;
	if ( setDirEntry( &nte, "vhea"))
		goto	ERET;
	/* Make metrics for missing Glyph */	
	cbbx = *bbx;
	makeMetrics( lstHdl, &hM, &vM, &cbbx);

	/*
	 * 'vmtx'
	 */
	if ((vmet=(struct VMetrics *)malloc( (size_t)sizeof(struct VMetrics)*numOfGlyph))==0)
		goto	ERET;
	memset(vmet, 0, sizeof(struct VMetrics)*numOfGlyph);

	/* for missing Glyph */

	*vmet = vM;
	sitom(&vmet->advanceHeight );
	sitom(&vmet->topSideBearing );

	/* for null Glyph */
	(vmet+1)->advanceHeight = 0;
	(vmet+1)->topSideBearing = 0;

	if (TTFAppendTable( nfh,&nte,vmet,sizeof(struct VMetrics)*numOfGlyph))
		goto	ERET;
	if ( setDirEntry( &nte, "vmtx"))
		goto	ERET;
	free( vmet);
	vmet = 0;

	/*
	 * head Read,modify and write
	 */
	if ( fontType==0) {
		if (TTFReadOrgFixedTable( hDC,(char *)&head, sizeof head,"head"))
			goto	ERET;
		modifyhead( &head);
	}
	else {
		WIFEhead( &head, bbx);
	}
	if (TTFAppendTable( nfh, &nte, &head, sizeof head))
		goto	ERET;
	if ( setDirEntry( &nte, "head"))
		goto	ERET;

	/*
	 * make hmtx
	 */
	if ((hmet=(struct HMetrics *)malloc( (size_t)sizeof(struct HMetrics)*numOfGlyph))==0)
		goto	ERET;
	memset(hmet, 0, sizeof(struct HMetrics)*numOfGlyph);
	/* for missing Glyph */
	*hmet = hM;
	sitom(&hmet->advanceWidth );
	sitom(&hmet->leftSideBearing);

	/* for null Glyph */
	(hmet+1)->advanceWidth = 0;
	(hmet+1)->leftSideBearing = 0;

	if (TTFAppendTable( nfh,&nte,hmet,sizeof(struct HMetrics)*numOfGlyph))
		goto	ERET;
	if ( setDirEntry( &nte, "hmtx"))
		goto	ERET;
	free( hmet);
	hmet = 0;

	if ((glyphData = (char *)malloc((size_t)GLYPHBUFSIZ))==0)
		goto	ERET;
	if (makeGlyphData( lstHdl, &cbbx, glyphData, GLYPHBUFSIZ, &gdatsiz ))
		goto	ERET;
	/*
	 * 'loca' make
	 */
	if ((loca = (long *)malloc( (size_t)sizeof(unsigned long)*(numOfGlyph+1)))==(long *)0)
		goto	ERET;
	*loca = 0;
	ofs = gdatsiz;
	litom(&ofs);
	for ( i = 1; i< numOfGlyph+1; i++)
		*(loca + i)=ofs;

	if (TTFAppendTable( nfh,&nte,loca,sizeof( long)*(numOfGlyph+1)))
		goto	ERET;
	if ( setDirEntry( &nte, "loca"))
		goto	ERET;
	free( loca);
	loca = 0;

	/*
	 * 'maxp' write
	 */
	if (TTFAppendTable( nfh,&nte,maxpdata,sizeof(maxpdata)))
		goto	ERET;
	if ( setDirEntry( &nte, "maxp"))
		goto	ERET;

	/*
	 * 'glyf' Write (missing glyph )
	 */
	if (TTFAppendTable( nfh, &nte, glyphData, gdatsiz))
		goto	ERET;
	if ( setDirEntry( &nte, "glyf"))
		goto	ERET;
	free( glyphData);
	glyphData = 0;
	/* Sort dir entry table */
	SortEntry( te, NUMTABLES);

	/* Write dir entry */
	if (TTFWriteDirEntry( nfh, te, NUMTABLES))
		goto	ERET;

	/* Set Check sum of file whole to head */
	if (fileChkSum( nfh, te, NUMTABLES, &hdr, &head))
		goto	ERET;
	CloseHandle( nfh);
	if ( te)	free( te);
	return 0;
ERET:
	if ( nfh != INVALID_HANDLE_VALUE)	CloseHandle( nfh);
	if ( te)	free( te);
	if ( nameBuf)	free( nameBuf);
	if ( hmet)	free( hmet);
	if ( loca)	free( loca);
	if ( os2buf)	free( os2buf);
	if ( glyphData)	free( glyphData);
	return -1;
}
/***********************************************************************
 *	Obtain BoundingBox ( from 'head' xMin, xMax, yMin, yMax)
 */
/* */	int
/* */	TTFGetBBX(
/* */		HDC	hDC,
/* */		struct BBX	*bbx,
/* */		short		*uPEm)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
struct	HeadTable	head;
	DWORD	dwTable;	/* tagName to request */
	DWORD	siz, s;

	memcpy( &dwTable, "head", TAGSIZ);
	siz = (DWORD)sizeof( struct HeadTable);

	
	if ((s=GetFontData(hDC, dwTable, (DWORD)0, &head,(DWORD)siz))!=siz){
		DWORD err = GetLastError();
		return -1;
	}
	smtoi( &head.xMin);
	smtoi( &head.xMax);
	smtoi( &head.yMin);
	smtoi( &head.yMax);
	smtoi( &head.unitsPerEm);
	bbx->xMin = head.xMin;
	bbx->xMax = head.xMax;
	bbx->yMin = head.yMin;
	bbx->yMax = head.yMax;
	*uPEm = head.unitsPerEm;
	if ( bbx->xMin < 0 )
		bbx->xMin = 0;
	return 0;
}
int
TTFTmpPath( TCHAR *path, TCHAR *tmpPath)
{
	TCHAR	*p1;
	TCHAR	dirPath[MAX_PATH];


	lstrcpy( dirPath, path);
	p1 = Mytcsrchr( dirPath, '\\');
	if ( p1==(TCHAR *)0) {
		p1 = Mytcsrchr( dirPath, ':');
		if (p1==(TCHAR *)0)
			*dirPath=(TCHAR)0;
		else
			*(p1+1)=0;
	}
	else
		*p1=0;
	if (GetTempFileName( dirPath, TEXT("TTE"), 0, tmpPath)==0)
		return -1;
	else
		return 0;

}
static int
copyTable( HANDLE iFh,HANDLE oFh, struct TableEntry *te, int nEntry, char *tag)
{
struct TableEntry *tep;
	char	*buf;

	tep = searchEntry(te, nEntry, tag);
	if ((buf = (char *)malloc((size_t)tep->siz))==(char *)0)
		goto	ERET;
	if ( (TTFReadTable( iFh,tep, buf, (int)tep->siz)!=(int)tep->siz) )
		goto	ERET;
	if ( (TTFWriteTable( oFh,tep, buf, (int)tep->siz)!=(int)tep->siz) )
		goto	ERET;
	free(buf);
	return 0;
ERET:
	if (buf)	free(buf);
	return -1;
	
}
static int
copyfblock( HANDLE iFh, HANDLE oFh, unsigned long siz, unsigned long *cs)
{
	int	rwsiz;
	int	filsiz;
	int	aligne;
	unsigned long	chksum;
	char	*buf;

	if ((buf = (char *)malloc((size_t) RWBUFSIZ))==(char *)0)
		goto	ERET;

	filsiz = (int)(siz % 4);
	if ( filsiz)	filsiz = 4-filsiz;
	aligne =0;
	chksum = 0;
	while ( siz > 0) {
		if ( siz > RWBUFSIZ)
			rwsiz = RWBUFSIZ;
		else	{
			rwsiz = (int)siz;
			if ( filsiz)
				aligne =1;
		}
		DWORD nByte;
		BOOL res = ReadFile(iFh, buf,(unsigned int)rwsiz, &nByte, NULL );
		if (!res || nByte != (unsigned int)rwsiz)
			goto	ERET;

		if ( aligne ) {
			memset( buf+siz, 0, filsiz);
			rwsiz += filsiz;
		}
		chksum += calchksum( buf, rwsiz);

		res = WriteFile ( oFh, buf, (unsigned int)rwsiz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)rwsiz)
			goto	ERET;
		siz -= rwsiz;
	}
	*cs = chksum;
	free(buf);
	return 0;
ERET:
	if(buf)	free(buf);
	return -1;
}
static int
mergeGlyph(
	HANDLE	iFh,
	HANDLE	oFh,
	struct TableEntry *tep,
	char	*locabuf,
	int	glyphID,
	char	*glyphData,
	int	glyphSiz)
{
	long	*locp;
	long	nloc, cloc;
	long	iofs;
	long	tail;
	long	siz;
	int	gid;
	long	delta;
	unsigned long	cs;
	int	filsiz;
	long	lval;

	locp = (long *)locabuf;

	cs = 0;
	/* copy leading */
	cloc = *locp;
	lmtoi( &cloc);
	iofs = (unsigned long)tep->ofs+cloc;
	if ((long) SetFilePointer( iFh, iofs, NULL, FILE_BEGIN) != iofs)
		goto	ERET;
	if ((long) SetFilePointer( oFh, iofs, NULL, FILE_BEGIN) != iofs)
		goto	ERET;
	cloc = *(locp + glyphID);
	lmtoi( &cloc);
	nloc = *(locp + glyphID+1);
	lmtoi( &nloc);
	siz = nloc - cloc;

	if (copyfblock( iFh, oFh, cloc, &cs))
		goto	ERET;
	/* write glyphData */
	if ( glyphSiz>0L) {
		filsiz = glyphSiz % 4;
		if ( filsiz) {
			filsiz = 4 - filsiz;
			memset( glyphData+glyphSiz, 0, filsiz);
			glyphSiz += filsiz;
		}
		DWORD nByte;
		BOOL res = WriteFile( oFh, glyphData, (unsigned int)glyphSiz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)glyphSiz)
			goto	ERET;
		cs += calchksum( glyphData, glyphSiz);
	}
	iofs = nloc + tep->ofs;
	/* copy trailer */
	if ((long) SetFilePointer( iFh, iofs, NULL, FILE_BEGIN) != iofs)
		goto	ERET;
	tail = *(locp+numOfGlyph);
	lmtoi(&tail);
	siz = tail - nloc;
	if (copyfblock( iFh, oFh, siz, &cs))
		goto	ERET;
	/* update loca */
	delta = glyphSiz - (nloc - cloc);
	for ( gid = glyphID+1; gid<=numOfGlyph; gid++) {
		lval = *(locp+gid);
		lmtoi(&lval);
		lval += delta;
		litom(&lval);
		*(locp+gid)=lval;
	}
	tep->siz = tail+delta;
	return 0;
ERET:
	return -1;
}
/***********************************************************************
 *	Add EUDC Font
 */
/* */	int
/* */	TTFAddEUDCChar(
/* */		TCHAR	*path,	/* EUDC FontFile path */
/* */		unsigned short	code, /* Charcode */
/* */	struct BBX	*bbx,	/* Bounding Box */
/* */		int	lstH) /* List Handle for glyph */
/*
 *	returns :0, -1
 ***********************************************************************/
{
struct TTFHeader	hdr;
struct HeadTable	head;
struct MaxpTbl		maxp;
	HANDLE	fH;
	HANDLE	tmpFh;
struct TableEntry	*te;
struct TableEntry	*tep;
struct TableEntry	*maxpTep;
struct TableEntry	*locaTep;
	int		glyphID;
struct HMetrics		hmet;
struct VMetrics		vmet;
	int		nEntry;
	char		*updbuf;
	char		*glyphData;
	int		bufsiz;
	int		gdatsiz;
	short		sval;
struct BBX	cbbx;
	TCHAR	tmpPath[MAX_PATH];
	TCHAR	savPath[MAX_PATH];
	int	sts;
  HINSTANCE hInst = AfxGetInstanceHandle();
  TCHAR szMessage[256];

	lastErr = 0;
	te = ( struct TableEntry *)0;
	updbuf = (char *)0;
	glyphData = (char *)0;

	fH = CreateFile(path,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fH == INVALID_HANDLE_VALUE)
		goto	ERET;

	TTFTmpPath( path, tmpPath);
	tmpFh = CreateFile(tmpPath,
					GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( tmpFh == INVALID_HANDLE_VALUE)
		goto	ERET;

	if ( TTFReadHdr( fH, &hdr))
		goto	ERET;
	if ( TTFWriteHdr( tmpFh, &hdr))
		goto	ERET;
	nEntry = hdr.numTables;
	glyphID = codeToGID( code);

	/* Read Table entries */
	if ( (te = (struct TableEntry *)malloc((size_t)sizeof(struct TableEntry)*NUMTABLES))==0)
		goto	ERET;

	if (TTFReadDirEntry( fH, te, nEntry))
		goto	ERET;
	if (TTFWriteDirEntry( tmpFh, te, nEntry))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "cvt "))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "fpgm"))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "prep"))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "name"))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "cmap"))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "OS/2"))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "post"))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "hhea"))
		goto	ERET;
	if (copyTable( fH, tmpFh, te, nEntry, "vhea"))
		goto	ERET;

	/* Read maxp Table */
	if ((maxpTep = searchEntry(te, nEntry, "maxp"))==0)
		goto	ERET;
	if ( TTFReadTable( fH, maxpTep, &maxp, sizeof maxp)!=(sizeof maxp) )
		goto	ERET;
	sval = maxp.numGlyph;
	smtoi( &sval);
	numOfGlyph = sval;

	bufsiz = sizeof(struct HMetrics)*(numOfGlyph);
	bufsiz = align32( bufsiz);
	if ( (updbuf = (char *)malloc( (size_t)bufsiz))==0)
		goto	ERET;

	/* make metrics for the glyph */
	cbbx = *bbx;
	makeMetrics( lstH, &hmet, &vmet, &cbbx);

	/* make glyph data */
	if ((glyphData = (char *)malloc((size_t)GLYPHBUFSIZ))==0)
		goto	ERET;
	if (sts=makeGlyphData(lstH, &cbbx, glyphData, GLYPHBUFSIZ, &gdatsiz)) {
		lastErr = sts;
		goto	ERET;
	}
	/* set hmetrics */
	sitom( &hmet.advanceWidth);
	sitom( &hmet.leftSideBearing);
	if ((tep = searchEntry(te, nEntry, "hmtx"))==0)
		goto	ERET;
	if ( TTFReadTable( fH, tep, updbuf, bufsiz)<=0)
		goto	ERET;
	*((struct HMetrics *)updbuf + glyphID) = hmet;
	if ( TTFWriteTable( tmpFh, tep, updbuf, bufsiz)<=0)
		goto	ERET;
	free( updbuf);
	updbuf = 0;

	/* Update 'vmtx' */
	bufsiz = sizeof(struct VMetrics)*(numOfGlyph);
	bufsiz = align32( bufsiz);
	if ( (updbuf = (char *)malloc( (size_t)bufsiz))==0)
		goto	ERET;

	/* set vmetrics */
	sitom( &vmet.advanceHeight);
	sitom( &vmet.topSideBearing);
	if ((tep = searchEntry(te, nEntry, "vmtx"))==0)
		goto	ERET;
	if ( TTFReadTable( fH, tep, updbuf, bufsiz)<=0)
		goto	ERET;
	*((struct VMetrics *)updbuf + glyphID) = vmet;
	if ( TTFWriteTable( tmpFh, tep, updbuf, bufsiz)<=0)
		goto	ERET;
	free( updbuf);
	updbuf = 0;

	/* Read loca */
	bufsiz = sizeof(long )*(numOfGlyph+1);
	bufsiz = align32( bufsiz);
	if ( (updbuf = (char *)malloc((size_t)bufsiz)) ==(char *)0)
		goto	ERET;
	if ((locaTep = searchEntry(te, nEntry, "loca"))==0)
		goto	ERET;
	if ( TTFReadTable( fH, locaTep, updbuf, bufsiz)<=0)
		goto	ERET;

	/* move glyf data */
	if ((tep = searchEntry(te, nEntry, "glyf"))==0)
		goto	ERET;
	if (mergeGlyph( fH, tmpFh, tep, updbuf, glyphID, glyphData, gdatsiz ))
		goto	ERET;
	/* read and cal checkSum of 'glyf' */
	if (tableChkSum( tmpFh, tep))
		goto	ERET;

	free( glyphData);
	glyphData =(char *)0;
	/* write loca */
	if ( TTFWriteTable( tmpFh, locaTep, updbuf, bufsiz)<=0)
		goto	ERET;

	/* update maxp */
	if ( updateMaxp( &maxp,  lstH) < 0)
		goto	ERET;
	if (TTFWriteTable(tmpFh, maxpTep, &maxp, sizeof maxp)!=(sizeof maxp))
		goto	ERET;

	/* update directory entry */
	if (TTFWriteDirEntry( tmpFh, te, nEntry))
		goto	ERET;

	/* Set Check sum of file whole to head */
	if ((tep = searchEntry(te, nEntry, "head"))==0)
		goto	ERET;
	if ( TTFReadTable( fH, tep, &head, sizeof head)<=0)
		goto	ERET;
	tep->checkSum = 0L;
	if ( TTFWriteTable( tmpFh, tep, &head, sizeof head)<=0)
		goto	ERET;
	if (fileChkSum(tmpFh , te, nEntry, &hdr, &head))
		goto	ERET;
	if ( TTFWriteTable( tmpFh, tep, &head, sizeof head)<=0)
		goto	ERET;

	CloseHandle( fH);
	CloseHandle( tmpFh);

//	fH = tmpFh = -1;

  TTFTmpPath( path, savPath); 
  if( DeleteFile( savPath)==0) {
    TCHAR szTitle[256];
    LoadString(hInst, IDS_MAINFRAMETITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
    LoadString(hInst, IDS_NOMEM_MSG, szMessage, sizeof(szMessage) / sizeof(TCHAR));
    MessageBox( AfxGetMainWnd()->GetSafeHwnd(), szMessage, szTitle, MB_OK);
		goto	ERET2;
  }
	if( MoveFile( path, savPath)==0) {
    TCHAR szTitle[256];
    LoadString(hInst, IDS_MAINFRAMETITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
    LoadString(hInst, IDS_NOMEM_MSG, szMessage, sizeof(szMessage) / sizeof(TCHAR));
    MessageBox( AfxGetMainWnd()->GetSafeHwnd(), szMessage, szTitle, MB_OK);
    goto	ERET2;
	}
  if( MoveFile( tmpPath, path)==0) {
    TCHAR szTitle[256];
    LoadString(hInst, IDS_MAINFRAMETITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
    LoadString(hInst, IDS_NOMEM_MSG, szMessage, sizeof(szMessage) / sizeof(TCHAR));
    MessageBox( AfxGetMainWnd()->GetSafeHwnd(), szMessage, szTitle, MB_OK);
		goto	ERET2;
  }
  if ( DeleteFile( savPath) ==0) {
    DeleteFile(path);
    MoveFile(savPath, path);
	  if ( te)	free(te);
    if ( updbuf)	free(updbuf);
	  if ( glyphData)	free(glyphData);
    return -3; //tte file is being used by another process
  }

  free( te);
	free( updbuf);
	return 0;

ERET:
	if ( fH != INVALID_HANDLE_VALUE)	CloseHandle(fH);
	if ( tmpFh != INVALID_HANDLE_VALUE)	CloseHandle(tmpFh);
	if ( te)	free(te);
	if ( updbuf)	free(updbuf);
	if ( glyphData)	free(glyphData);
	return -1;

ERET2:
	if ( fH != INVALID_HANDLE_VALUE)	CloseHandle(fH);
	if ( tmpFh != INVALID_HANDLE_VALUE)	CloseHandle(tmpFh);
	if ( te)	free(te);
	if ( updbuf)	free(updbuf);
	if ( glyphData)	free(glyphData);
	DeleteFile( tmpPath);	/* donot check status */
	return -1;
}


/***********************************************************************
 *	For Import
 */
static HANDLE eudcFh;
static char *locaBuf = 0;
static char *maxpBuf = 0;
static char	*hmtxBuf = 0;
static char	*vmtxBuf = 0;
static char	*glyphBuf = 0;

static void
frebuf()
{
	if ( locaBuf) {
		free( locaBuf);
		locaBuf = 0;
	}
	if ( maxpBuf) {
		free( maxpBuf);
		maxpBuf = 0;
	}
	if ( vmtxBuf) {
		free( vmtxBuf);
		vmtxBuf = 0;
	}
	if ( hmtxBuf) {
		free( hmtxBuf);
		hmtxBuf = 0;
	}
	if ( glyphBuf) {
		free( glyphBuf);
		glyphBuf = 0;
	}
	if ( et) {
		free( et);
		glyphBuf = 0;
	}
	return;
}
int
TTFOpen( TCHAR *path)
{
struct TTFHeader	hdr;
unsigned int	bufsiz;
unsigned short	sval;

	et = 0;
	eudcFh = CreateFile(path,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( eudcFh == INVALID_HANDLE_VALUE)
		goto	ERET;

	if (TTFReadHdr(eudcFh,&hdr))
		goto	ERET;

	if ((et = (struct TableEntry *)malloc( sizeof (struct TableEntry)*hdr.numTables))
			==0)
		goto	ERET;
	entryNum = hdr.numTables;
	if (TTFReadDirEntry(eudcFh, et, hdr.numTables))
		goto	ERET;

	if (TTFReadVarTable( eudcFh, &maxpBuf, &bufsiz, "maxp"))
		goto	ERET;
	sval = ((struct MaxpTbl *)maxpBuf) ->numGlyph;
	smtoi( (short *)&sval);
	numOfGlyph = sval;

	if (TTFReadVarTable( eudcFh, &locaBuf, &bufsiz, "loca"))
		goto	ERET;
	if (TTFReadVarTable( eudcFh, &hmtxBuf, &bufsiz, "hmtx"))
		goto	ERET;
	if (TTFReadVarTable( eudcFh, &vmtxBuf, &bufsiz, "vmtx"))
		goto	ERET;
	if ((glyphBuf = (char *)malloc( GLYPHBUFSIZ))==0)
		goto	ERET;
	return 0;
ERET:
	if ( eudcFh != INVALID_HANDLE_VALUE) {
		CloseHandle( eudcFh);
		eudcFh = INVALID_HANDLE_VALUE;
	}
	frebuf();
	return -1;
}
int
TTFClose()
{
struct TableEntry	*tep;
struct HeadTable	head;
struct TTFHeader	hdr;
	long		ofs;
	if ( eudcFh == INVALID_HANDLE_VALUE)
		return -1;
	if (TTFReadHdr( eudcFh, &hdr))
		goto	ERET;
	/* obtain glyph table size */
	ofs = SetFilePointer( eudcFh, 0L, NULL, FILE_END);

	/* Update Glyph checkSum */
	if ((tep = searchEntry(et, entryNum, "glyf"))==0)
		goto	ERET;
	tep->siz = ofs - tep->ofs;

	/* read and cal checkSum of 'glyf' */
	if (tableChkSum( eudcFh, tep))
		goto	ERET;

	if ((tep = searchEntry(et, entryNum, "maxp"))==0)
		goto	ERET;

	if (TTFWriteTable( eudcFh, tep, maxpBuf, (int)tep->siz)!=(int)tep->siz)
		goto	ERET;

	if ((tep = searchEntry(et, entryNum, "loca"))==0)
		goto	ERET;

	if (TTFWriteTable( eudcFh, tep, locaBuf, (int)tep->siz)!=(int)tep->siz)
		goto	ERET;
	
	if ((tep = searchEntry(et, entryNum, "hmtx"))==0)
		goto	ERET;
	if (TTFWriteTable( eudcFh, tep, hmtxBuf, (int)tep->siz)!=(int)tep->siz)
		goto	ERET;
	if ((tep = searchEntry(et, entryNum, "vmtx"))==0)
		goto	ERET;
	if (TTFWriteTable( eudcFh, tep, vmtxBuf, (int)tep->siz)!=(int)tep->siz)
		goto	ERET;
	if ((tep = searchEntry(et, entryNum, "head"))==0)
		goto	ERET;

	if ( TTFReadTable(eudcFh, tep, &head, sizeof(head))!=sizeof(head))
		goto	ERET;

	if ( fileChkSum(eudcFh, et, entryNum, &hdr, &head))
		goto	ERET;

	if ( TTFWriteDirEntry( eudcFh, et, entryNum))
		goto	ERET;
	CloseHandle( eudcFh);
	frebuf();
	return 0;
ERET:
	CloseHandle( eudcFh);
	frebuf();
	return -1;
}
int
TTFGetEUDCBBX(TCHAR *path, struct BBX *bbx, short *upem)
{
struct	HeadTable	head;
	HANDLE	fH;

	fH = CreateFile(path,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fH == INVALID_HANDLE_VALUE)
		goto	ERET;

	if ( TTFReadFixedTable(fH, (char *)&head,sizeof head,"head"))
		goto	ERET;
	smtoi( &head.xMin);
	smtoi( &head.xMax);
	smtoi( &head.yMin);
	smtoi( &head.yMax);
	smtoi( &head.unitsPerEm);
	bbx->xMin = head.xMin;
	bbx->xMax = head.xMax;
	bbx->yMin = head.yMin;
	bbx->yMax = head.yMax;
	*upem = head.unitsPerEm;
	if ( bbx->xMin < 0 )
		bbx->xMin = 0;
	CloseHandle( fH);
	return 0;
ERET:
	if (fH!=INVALID_HANDLE_VALUE)
		CloseHandle( fH);
	return -1;
}
static void
makeMetrics(int lsthdl, struct HMetrics *hM, struct VMetrics *vM, struct BBX *bbx)
{
struct VHEAD	*vhd;
struct VDATA	*vp;
	int	np;
	int	sts;
	int	xmin, ymin, xmax, ymax;


	if ( (sts = VDGetHead( lsthdl, &vhd))!=0)
		goto	RET;
	if ( (sts = VDGetNCont( lsthdl))<=0)
		goto	SPACE_CHAR;
	xmin = xmax = vhd->headp->vd.x;
	ymin = ymax = vhd->headp->vd.y;
	while ( vhd->next != NIL) {
		vp = vhd->headp;
		np = vhd->nPoints;
		while ( np-->0) {
			if (vp->vd.x > xmax)
				xmax = vp->vd.x;
			else if(vp->vd.x < xmin)
				xmin = vp->vd.x;
			if (vp->vd.y > ymax)
				ymax = vp->vd.y;
			else if(vp->vd.y < ymin)
				ymin = vp->vd.y;
			vp = vp->next;
		}
		vhd = vhd->next;
	}
//	if ( xmin < 0)
//		xmin = 0;
	hM->leftSideBearing = (short)xmin;
  hM->advanceWidth = bbx->xMax - bbx->xMin;
  {
    int hmw = hM->advanceWidth + 1;
    int ii = 0;
    while (hmw >> 1)
      {
        hmw >>= 1;
        ii++;
      }
    hmw = 1;
    while (ii--) hmw *= 2;
    int gap1 = hM->advanceWidth - hmw;
    int gap2 = hmw*2 - hM->advanceWidth;
    hmw = gap1 > gap2 ? hmw * 2 : hmw;
    hM->advanceWidth = (short)hmw;
  } 
	vM->topSideBearing = bbx->yMax - ymax;
	if (vM->topSideBearing < 0)
		vM->topSideBearing = 0;
	vM->advanceHeight = bbx->yMax - bbx->yMin;
	bbx->xMin = xmin;
	bbx->yMin = ymin;
	bbx->xMax = xmax;
	bbx->yMax = ymax;
	return;
RET:

SPACE_CHAR:
	hM->leftSideBearing = (short)bbx->xMax;
	hM->advanceWidth = bbx->xMax - bbx->xMin;
	vM->topSideBearing = bbx->yMax - bbx->yMin;
	vM->advanceHeight = bbx->yMax - bbx->yMin;
	bbx->xMin = bbx->xMax;
	bbx->yMax = bbx->yMin;
	return;
}

int
TTFAppend( unsigned short code,  struct BBX *bbx, int lsthdl)
{
	int	glyphID;
struct HMetrics	hmet;
struct VMetrics	vmet;
	int	updflg;
	int	gdatsiz;
struct BBX	cbbx;
	int	sts;

	glyphID = codeToGID( code);
	lastErr = 0;
	/* make metrics */
	cbbx = *bbx;
	makeMetrics( lsthdl, &hmet, &vmet, &cbbx);

	/* make glyph data */
	if (sts =makeGlyphData(lsthdl, &cbbx, glyphBuf, GLYPHBUFSIZ, &gdatsiz)) {
		lastErr = sts;
		goto	ERET;
	}
	/* set hmetrics */
	sitom( &hmet.advanceWidth);
	sitom( &hmet.leftSideBearing);
	*((struct HMetrics *)hmtxBuf + glyphID) = hmet;

	/* Update 'vmtx' */
	sitom( &vmet.advanceHeight);
	sitom( &vmet.topSideBearing);
	*((struct VMetrics *)vmtxBuf + glyphID) = vmet;

	/* write glyf data */
	if (TTFImpGlyphWrite( glyphID, glyphBuf, gdatsiz))
		goto	ERET;
	/* update maxp */
	if ((updflg = updateMaxp( (struct MaxpTbl *)maxpBuf,  lsthdl)) < 0)
		goto	ERET;
	return 0;
ERET:
	return -1;
}
/*
 * Copy
 */
int
TTFImpCopy( TCHAR *sPath, TCHAR *dPath)
{
	HANDLE	sFh, dFh;
struct TTFHeader	hdr;
struct TableEntry	*te;
	int		tblSiz;
	int		ntbl;
	int		tcnt;
	char	*buf;
	int	bufsiz;

	sFh = dFh = 0;
	te = 0;
	buf  = 0;

  /* Open src */
	sFh = CreateFile(sPath,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
	if (sFh == INVALID_HANDLE_VALUE)
		goto	ERET;
	/* create open destination*/
	dFh = CreateFile(dPath,
					GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
	if (dFh == INVALID_HANDLE_VALUE)
		goto	ERET;
	
	/* read header */
	if ( TTFReadHdr( sFh, &hdr))
		goto	ERET;
	ntbl = (int)hdr.numTables;
	/* write header */
	if ( TTFWriteHdr( dFh, &hdr))
		goto	ERET;
	
	/* read directory entry */
	tblSiz = sizeof( struct TableEntry)*ntbl;
	if ((te = (struct TableEntry *)malloc(tblSiz))==(struct TableEntry *)0)
		goto	ERET;

	if ( TTFReadDirEntry(sFh, te, ntbl))
		goto	ERET;

	/* write directory entry */
	if ( TTFWriteDirEntry(dFh, te, ntbl))
		goto	ERET;

	/* copy each table (except glyph)*/
	for ( tcnt = 0; tcnt < ntbl; tcnt++) {
		if ( memcmp((te+tcnt)->tagName,"glyf", 4)) {
			bufsiz = (int)(te+tcnt)->siz;
			buf = (char *)malloc((int)(te+tcnt)->siz);
			if (TTFReadTable( sFh, te+tcnt, buf, bufsiz)!=bufsiz)
				goto	ERET;
			if (TTFWriteTable( dFh, te+tcnt, buf, bufsiz)!=bufsiz)
				goto	ERET;
			free( buf);
			buf = 0;
		}
	}
	/* close */
	CloseHandle( sFh);
	CloseHandle( dFh);
	free( te);
	return 0;
ERET:
  if (sFh != INVALID_HANDLE_VALUE) CloseHandle(sFh);
  if (dFh != INVALID_HANDLE_VALUE) CloseHandle(dFh);
	if (te)
		free( te);
	if ( buf)	free( buf);
	return -1;
}
int
TTFImpGlyphCopy(
	HANDLE	sFh,
	int	glyphID)
{
struct TableEntry *tep;
	long	*locap;
	long	cloc, nloc;
	long	siz;
	long	ofs;
	long	wofs;
	char	*buf;
	DWORD	nByte;

	buf = 0;
	if ((tep = searchEntry( et, entryNum, "glyf"))==0)
		goto	ERET;
	locap = (long *)locaBuf;
	cloc = *(locap+glyphID);
	lmtoi( &cloc);
	nloc = *(locap+glyphID+1);
	lmtoi( &nloc);
	siz = nloc - cloc;
	ofs = tep->ofs + cloc;

	if ( (long) SetFilePointer( sFh, ofs, NULL, FILE_BEGIN) != ofs)
		goto	ERET;

	wofs = SetFilePointer( eudcFh, 0L, NULL, FILE_END);
	wofs -= tep->ofs;
	litom( &wofs);
	*(locap+glyphID) = wofs;
	if ( siz) {
		if ( (buf = (char *)malloc((int)siz))==(char *)0)
			goto	ERET;

		BOOL res = ReadFile( sFh, buf, (unsigned int)siz, &nByte, NULL);
		if (!res ||	nByte !=(unsigned int)siz)
			goto	ERET;

		res = WriteFile( eudcFh, buf,(unsigned int)siz, &nByte, NULL);
		if (!res ||	nByte !=(unsigned int)siz)
			goto	ERET;
	}
	free( buf);
	return 0;
ERET:
	if ( buf)	free( buf);

	return -1;
}
int
TTFImpGlyphWrite(
	int	glyphID, char *glyph, int siz)
{
struct TableEntry *tep;
	long	ofs;
	long	wofs;
	long	*locap;
	DWORD	nByte;

	if ( glyphID >= numOfGlyph)
		return -1;
	if ((tep = searchEntry( et, entryNum, "glyf"))==0)
		goto	ERET;
	
	wofs =  SetFilePointer( eudcFh, 0L, NULL, FILE_END);
	ofs = wofs - tep->ofs;
	litom( &ofs);
	locap = (long *)locaBuf;
	*(locap+glyphID) = ofs;
	if ( siz) {
		BOOL res = WriteFile(eudcFh, glyph, (unsigned int)siz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)siz)
			goto	ERET;
	}
	return 0;
ERET:
	return -1;
}

int
TTFImpTerm(HANDLE orgFh, int glyphID)
{
struct TableEntry *tep;
	
	long	ofs;
	long	wofs;
	long	*locap;
	int		gid;
	
	//
	// copy the rest of the glyph data over.
	//
	for ( gid = glyphID; gid < numOfGlyph; gid++)
	{
		if (TTFImpGlyphCopy(orgFh, gid))
			return -1;
	}

	if ((tep = searchEntry( et, entryNum, "glyf"))==0)
		return -1;
	locap = (long *)locaBuf;
	wofs =  SetFilePointer( eudcFh, 0L,NULL, FILE_END);
	ofs = wofs - tep->ofs;
	litom( &ofs);
	*(locap + numOfGlyph) = ofs;
	tep->siz = wofs - tep->ofs;

	return 0;

}
int
TTFLastError()
{
	return lastErr;
}
/* EOF */
