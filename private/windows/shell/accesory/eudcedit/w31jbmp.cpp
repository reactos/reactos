
/*
 *	Win3.1J EUDC fontfile i/o ( MS-Code base)
 *
 * Copyright (c) 1997-1999 Microsoft Corporation.
 */


#include	"stdafx.h"
#pragma		pack(2)

#include	"extfunc.h"
/*
 File Structure */

struct W31_Header {
	char	identify[72];
	short	segCnt;		/* ??? */
unsigned short	sCode,
		eCode;
	short	cCnt;
	long	ofsCmap;
	short	sizCmap;
	long	ofsFil;
	short	sizFil;
	long	ofsStbl;	/* search tbl*/
	short	sizStbl;
	long	ofsBdatSub;
	};

struct BDatSubTbl {
	long	tail;
	long	ptrOfs;
	long	head;
	short	filler2;
	/* Following Pointer tbl. */
	};
struct BMPHeader {
	long	bitmapSiz;
	short	xsiz, ysiz;
	};

#define		EUDCCODEBASE	((unsigned short)0xe000)


int  OpenW31JBMP(TCHAR  *path,int  omd);
int  CloseW31JBMP(void);
int  isW31JEUDCBMP(TCHAR  *path);
int  GetW31JBMPnRecs(int *nRec, int *nGlyph, int *xsiz, int *ysiz);
int  GetW31JBMPMeshSize( int *xsiz, int *ysiz);
static int  readcmap(void);
static int  rectocode(int  rec,unsigned short  *code);
static int  searchCode(unsigned short  code);
int  GetW31JBMP(unsigned short  code,LPBYTE buf,int  bufsiz,int  *xsiz,int  *ysiz);
int  GetW31JBMPRec(int  rec,LPBYTE buf,int  bufsiz,int  *xsiz,int  *ysiz,unsigned short  *code);
int  PutW31JBMPRec(int  rec,LPBYTE buf,int  xsiz,int  ysiz);
static int  ReadBMPHdr(HANDLE hdl,long  ofs,struct  BMPHeader *bhdr);
static int  WriteBMPHdr(HANDLE hdl,long  ofs,struct  BMPHeader *bhdr);

static int	init = 0;
static HANDLE	fHdl;
struct W31_Header hdr;
struct BDatSubTbl bdTbl;
static int	rwmode = 0;
static long	*ofstbl=0;
static unsigned short	*cmap=0;
static int	*recordTbl=0;
/***************************************************************
 *	Initialize
 */
/* */	int
/* */	OpenW31JBMP( TCHAR *path, int omd)
/*
 *	returns : 0, -1
 ***************************************************************/
{
	int	msiz;
	DWORD nByte;
	BOOL res;

	/* open EUDC Font File */
	rwmode = omd ? 1 : 0;
	fHdl = CreateFile(path,
					omd==0 ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fHdl == INVALID_HANDLE_VALUE)
		return -1;

	/* Read Header */
	res = ReadFile( fHdl, (LPBYTE)&hdr, sizeof(struct W31_Header), &nByte, NULL);
	if (!res || nByte !=sizeof(struct W31_Header))
		goto	ERET;

	if( hdr.cCnt > 1880)
		hdr.cCnt = 1880;

	/* allocate ofs. tbl. */
	msiz = hdr.cCnt*sizeof(long);
	if ((ofstbl = (long *)malloc( msiz))==(long *)0)
		goto	ERET;

	/* Read Ofs. tbl.*/
	if ( (long) SetFilePointer( fHdl, hdr.ofsBdatSub, NULL, FILE_BEGIN)!=hdr.ofsBdatSub)
		goto	ERET;
	res = ReadFile( fHdl, (LPBYTE)&bdTbl, sizeof(bdTbl), &nByte, NULL);
	if (!res || nByte !=sizeof(bdTbl))
		goto	ERET;

	res = ReadFile( fHdl, (LPBYTE)ofstbl, (unsigned int)msiz, &nByte, NULL);
	if (!res || nByte !=(unsigned int)msiz)
		goto	ERET;

	init = 1;
/*	
  if (fHdl != INVALID_HANDLE_VALUE)
  {
    CloseHandle(fHdl);
    fHdl = INVALID_HANDLE_VALUE;
  }
*/
	return	0;
ERET:
	if (fHdl != INVALID_HANDLE_VALUE)
  {
		CloseHandle (fHdl);
    fHdl = INVALID_HANDLE_VALUE;
  }

	if (ofstbl)
  {
		free( ofstbl);
    ofstbl = 0;
  }
	return -1;
}
/***************************************************************
 *	Terminate Close
 */
/* */	int
/* */	CloseW31JBMP()
/*
 *	returns : none
 ***************************************************************/
{
	unsigned int	siz;
	DWORD nByte;
	BOOL res;

	if ( rwmode>=1) {
		/* update ofstbl*/
		if ((long) SetFilePointer( fHdl, hdr.ofsBdatSub, NULL, FILE_BEGIN)!=hdr.ofsBdatSub)
			goto	ERET;
		res = WriteFile( fHdl, (LPBYTE)&bdTbl, sizeof( bdTbl), &nByte, NULL);
		if (!res || nByte !=sizeof(bdTbl))
			goto	ERET;
		siz = (unsigned int)hdr.cCnt*sizeof(long);
		res = WriteFile( fHdl, (LPBYTE)ofstbl, siz, &nByte, NULL);
		if (!res || nByte !=siz)
			goto	ERET;
	}
	if ( fHdl !=INVALID_HANDLE_VALUE) {
		CloseHandle( fHdl);
		fHdl = INVALID_HANDLE_VALUE;
	}
	if ( ofstbl) {
		free(ofstbl);
		ofstbl = 0;
	}
	if ( cmap) {
		free(cmap);
		cmap = 0;
	}
	if ( recordTbl) {
		free(recordTbl);
		recordTbl = 0;
	}

	init = 0;
	return 0;
ERET:
	return -1;
}
/***************************************************************
 *	is Win3.1J EUDC bitmap
 */
/* */	int
/* */	isW31JEUDCBMP( TCHAR *path)
/*
 *	returns : 0 (other), 1 (EUDC bitmap), -1(error)
 ***************************************************************/
{
	HANDLE fhdl;
struct W31_Header hdr;
	DWORD nByte;
	BOOL res;

	fhdl = CreateFile(path,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fhdl == INVALID_HANDLE_VALUE)
		return -1;
	res = ReadFile( fhdl, (LPBYTE)&hdr, sizeof(hdr), &nByte, NULL);
	if (!res || nByte !=sizeof(hdr))
		goto	NO_WIN31J;
	CloseHandle( fhdl);
  fhdl = INVALID_HANDLE_VALUE;

	/* compare idendify leading 16 byte, sCode, eCode and cCnt*/
	if (memcmp( hdr.identify, "WINEUDC2Standard", 16))
		goto	NO_WIN31J;
#if 0
	if ( hdr.sCode != 0x40f0 || hdr.eCode != 0xfcf9 || hdr.cCnt != 1880)
#endif
	if( hdr.sCode != 0x40f0)
		goto	NO_WIN31J;
	return 1;

NO_WIN31J:
  if (fhdl != INVALID_HANDLE_VALUE)
  {
    CloseHandle(fhdl);
    fhdl = INVALID_HANDLE_VALUE;
  }
	return 0;
}
/***************************************************************
 *	Get number of records
 */
/* */	int
/* */	GetW31JBMPnRecs( int *nRec, int *nGlyph, int *xsiz, int *ysiz)
/*
 *	returns : 0, -1
 ***************************************************************/
{
struct BMPHeader	fhdr;
	long	ofs;
	BOOL	bFirst;
	int	rec;
	int	gc;
	DWORD nByte;
	BOOL res;

	bFirst = FALSE;
	if ( init==0 || fHdl == INVALID_HANDLE_VALUE)
		return -1;
	else {
		gc = 0;
		for ( rec = 0; rec < (int)hdr.cCnt; rec++) {
			if( *(ofstbl+rec)){
				if( !bFirst){
					ofs = *(ofstbl+rec);
					ofs += hdr.ofsBdatSub;
					if ( (DWORD) SetFilePointer( fHdl,ofs, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
          {
            DWORD dwErr = GetLastError();
						goto	ERET;
          }

					res = ReadFile( fHdl, (LPBYTE)&fhdr,
					     sizeof(struct BMPHeader), &nByte, NULL);
					if (!res || nByte != sizeof( struct BMPHeader))
						goto	ERET;

					bFirst = TRUE;
				}
				gc++;
			}
		}
		*nRec = (int)hdr.cCnt;
		*nGlyph = gc;
		*xsiz = fhdr.xsiz;
		*ysiz = fhdr.ysiz;
		return 0;
	}
ERET:
	return( -1);
}
static int
readcmap()
{
	unsigned int	msiz;
	DWORD nByte;
	BOOL res;
	msiz = (unsigned int)hdr.cCnt*sizeof(unsigned short);
	if ((cmap = (unsigned short*)malloc(msiz))==(unsigned short *)0)
		goto	ERET;
	if ((long) SetFilePointer( fHdl, hdr.ofsCmap, NULL, FILE_BEGIN)!=hdr.ofsCmap)
		goto	ERET;
	res = ReadFile( fHdl, (LPBYTE)cmap, msiz, &nByte, NULL);
	if (!res || nByte !=msiz)
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static int
rectocode( int rec, unsigned short *code)
{
	if ( cmap==0) {
		if (readcmap())
			return -1;
	}
	*code = *(cmap+rec);
	return 0;
}
static int
searchCode( unsigned short code)
{
	int	high, low, mid;

	if ( cmap==(unsigned short *)0) {
		if (readcmap())
			goto	ERET;
	}
	high = hdr.cCnt-1;
	low = 0;
	while ( high >= low) {
		mid = (high+low)/2;
		if ( *(cmap+mid)==code)
			return mid;
		else if ( *(cmap+mid)>code)
			high = mid-1;
		else
			low = mid+1;
	}
ERET:
	return -1;
}
/***************************************************************
 *	Read Bitmap by code number
 */
/* */	int
/* */	GetW31JBMP(
/* */		unsigned short	code,	/* code Number */
/* */		LPBYTE buf,	/* buffer to set bitmap */
/* */		int	bufsiz,	/* Buffer Size */
/* */		int	*xsiz,	/* Bitmap X,Ysiz */
/* */		int	*ysiz)
/*
 *	returns : >=0, -1
 ***************************************************************/
{
	int	rec;
	int	sts;
	unsigned short	rcode;
	/* search code */
	if ( (rec = searchCode( code)) <0)
		return -1;
	else {
		sts = GetW31JBMPRec( rec, buf, bufsiz, xsiz, ysiz, &rcode);
		return sts;
	}
}
/****************************************/
/*					*/
/*	Get W31JEUDC's Bmp Mesh Size	*/
/*					*/
/****************************************/
int
GetW31JBMPMeshSize(
int	*xsiz,
int	*ysiz)
{
	long	ofs;
struct BMPHeader	fhdr;
	int	bmpsiz;
	DWORD nByte;
	BOOL res;

	if (init==0)
		return -1;

	ofs = *(ofstbl);
	if ( ofs==0L)	
		return 0;
	ofs += hdr.ofsBdatSub;

	if ( (long) SetFilePointer(fHdl, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ERET;
	res = ReadFile( fHdl, (LPBYTE)&fhdr, sizeof(struct BMPHeader), &nByte, NULL);
	if (!res || nByte != sizeof( struct BMPHeader))
		goto	ERET;

	*xsiz = fhdr.xsiz;
	*ysiz = fhdr.ysiz;
	bmpsiz = ((int)fhdr.xsiz+15)/16 *2 * (int)fhdr.ysiz;

	return bmpsiz;
ERET:
	return (-1);
}
/***************************************************************
 *	Read Bitmap by record number
 */
/* */	int
/* */	GetW31JBMPRec(
/* */		int	rec,	/* Record Number */
/* */		LPBYTE buf,	/* buffer to set bitmap */
/* */		int	bufsiz,	/* Buffer Size */
/* */		int	*xsiz,	/* Bitmap X,Ysiz */
/* */		int	*ysiz,
/* */		unsigned short	*code)
/*
 *	returns : bitmapsiz >=0, -1
 ***************************************************************/
{
	long	ofs;
struct BMPHeader	fhdr;
	int	bmpsiz;
	int	rdsiz;
	DWORD nByte;
	BOOL res;

	if (init==0)
		return -1;

	ofs = *(ofstbl+rec);
	if ( ofs==0L)	
		return 0;
	ofs += hdr.ofsBdatSub;

	/* read Bitmap Header
		bitmap is Word aligned */
	if ( (long) SetFilePointer( fHdl, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ERET;
	res = ReadFile( fHdl, (LPBYTE)&fhdr, sizeof(struct BMPHeader), &nByte, NULL);
	if (!res || nByte != sizeof( struct BMPHeader))
		goto	ERET;

	bmpsiz = ((int)fhdr.xsiz+15)/16 *2 * (int)fhdr.ysiz;
	/* Read Bitmap Body */
	rdsiz = bmpsiz > bufsiz ? bufsiz : bmpsiz;
	if ( rdsiz > 0) {
		res = ReadFile( fHdl, buf, (unsigned int)rdsiz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)rdsiz)
			goto	ERET;
	}
	*xsiz = fhdr.xsiz;
	*ysiz = fhdr.ysiz;
	if ( rectocode( rec, code))
		goto	ERET;
	return bmpsiz;
ERET:
	return -1;
}
/***************************************************************
 *	Write Bitmap by record number
 */
/* */	int
/* */	PutW31JBMPRec(
/* */		int	rec,	/* Record Number */
/* */		 LPBYTE buf,	/* buffer to set bitmap */
/* */		int	xsiz,	/* Bitmap X,Ysiz */
/* */		int	ysiz)
/*
 *	returns : 0, -1
 ***************************************************************/
{
	long	ofs;
struct BMPHeader	fhdr;
	int	bmpsiz;
	unsigned int	wbmpsiz;
	DWORD nByte;
	BOOL res;

	if (init==0)
		return -1;
	else if ( rwmode==0)
		return -1;
	rwmode = 2;
	wbmpsiz = (unsigned int) ((xsiz+15)/16 *2 * ysiz);
	ofs = *(ofstbl+rec);
	if ( ofs != 0L) {
		/* read Bitmap Header
			bitmap is Word aligned */
		if ( ReadBMPHdr( fHdl, ofs, &fhdr))
			goto	ERET;

		bmpsiz = ((int)fhdr.xsiz+15)/16 *2 * (int)fhdr.ysiz;
		if ( bmpsiz<(int)wbmpsiz)
			ofs = 0L;
	}
	if ( ofs == 0L)
		ofs = bdTbl.tail;

	/* Write Bitmap Header */
	fhdr.xsiz = (short)xsiz;
	fhdr.ysiz = (short)ysiz;
	fhdr.bitmapSiz = wbmpsiz+sizeof(fhdr);

	if ( WriteBMPHdr( fHdl, ofs, &fhdr))
		goto	ERET;

	/* Write Bitmap Body */
	res = WriteFile( fHdl, buf, wbmpsiz, &nByte, NULL);
	if (!res || nByte !=wbmpsiz)
		goto	ERET;

	/* write bitmap ptr on subTable */
	*(ofstbl+rec) = ofs;

	bdTbl.tail = ofs + wbmpsiz+sizeof(fhdr);

	return 0;
ERET:
	return -1;
}
static int
ReadBMPHdr( HANDLE hdl, long ofs, struct BMPHeader *bhdr)
{
	DWORD nByte;
	BOOL res;

	ofs += hdr.ofsBdatSub;
	if ( (long) SetFilePointer( hdl, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ERET;
	res = ReadFile( hdl, (LPBYTE) bhdr, sizeof( struct BMPHeader), &nByte, NULL);
	if (!res || nByte !=sizeof( struct BMPHeader))
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static int
WriteBMPHdr( HANDLE hdl, long ofs, struct BMPHeader *bhdr)
{
	DWORD nByte;
	BOOL res;
	ofs += hdr.ofsBdatSub;
	if ( (long) SetFilePointer( hdl, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ERET;
	res = WriteFile(hdl, (LPBYTE )bhdr, sizeof( struct BMPHeader), &nByte, NULL);
	if (!res || nByte !=sizeof( struct BMPHeader))
		goto	ERET;
	return 0;
ERET:
	return -1;
}

int
W31JrecTbl( int **recTbl, BOOL bIsWin95EUDC)
{
	int	rec;
	int	*tp;
	unsigned short code;
	if ( cmap==0) {
		if (readcmap())
			return -1;
	}
	if ( (tp = (int *)malloc( sizeof(int)*hdr.cCnt))==(int *)0)
		return -1;
	for ( rec = 0; rec < hdr.cCnt; rec++) {
		if ( *(ofstbl + rec)!=0L) {

			code = *(cmap+rec);
			if (!bIsWin95EUDC)
				code = sjisToUniEUDC( code);
			tp[(int)(code - EUDCCODEBASE)] = rec;
		}
		else	
			tp[rec] = -1;
	}
	*recTbl = recordTbl = tp;

	return 0;
}
/* EOF */
