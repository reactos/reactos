//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
#include	"stdafx.h"
#pragma		pack(2)

#include	"vdata.h"
#include	"ttfstruc.h"
#include	"extfunc.h"
/*
 *	Win3.1J EUDC fontfile i/o
 */
#define		EUDCCODEBASE	((unsigned short)0xe000)
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
	long	filler1;
	long	head;
	short	filler2;
	/* Following Pointer tbl. */
	};
struct BMPHeader {
	long	bitmapSiz;
	short	xsiz, ysiz;
	};


static int  ReadBdatSub(HANDLE hdl,long  ofs,struct  BDatSubTbl *tbl);
static int  WriteBdatSub(HANDLE  hdl,long  ofs,struct  BDatSubTbl *tbl);
static int  ReadBDatEntry(HANDLE  hdl,long  *ofs,long  rec);
static int  WriteBDatEntry(HANDLE  hdl,long  ofs,long  rec);
static int  ReadBMPHdr(HANDLE  hdl,long  ofs,struct  BMPHeader *hdr);
static int  WriteBMPHdr(HANDLE  hdl,long  ofs,struct  BMPHeader *hdr);


static int	init = 0;
static long	bdathead;
static long	bdatptr;
static int	maxRec;
static TCHAR fpath[128];

/***************************************************************
 *	Initialize
 */
/* */	int
/* */	OpenW31JEUDC( TCHAR *path)
/*
 *	returns : 0, -1
 ***************************************************************/
{
	HANDLE fHdl;
struct W31_Header hdr;
	DWORD nByte;
	BOOL res;

	makeUniCodeTbl();
	lstrcpy( fpath, path);
	/* open EUDC Font File */
	fHdl = CreateFile(path,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fHdl == INVALID_HANDLE_VALUE)
		return -1;

	/* Read Header */
	res = ReadFile( fHdl, &hdr, sizeof(struct W31_Header), &nByte, NULL);
	if (!res || nByte !=sizeof(struct W31_Header))
  {
    CloseHandle(fHdl);
		return -1;
  }
	bdathead = hdr.ofsBdatSub;
	bdatptr = hdr.ofsBdatSub + sizeof(struct BDatSubTbl);
	maxRec = hdr.cCnt-1;

	/* close Font File */
	CloseHandle( fHdl);
	init = 1;
	return	0;
}
/***************************************************************
 *	Terminate Close
 */
/* */	void
/* */	CloseW31JEUDC()
/*
 *	returns : none
 ***************************************************************/
{
	init = 0;
	return;
}
static int
codeToRec( unsigned short code, BOOL bUnicode)
{
	return (int)((bUnicode ? code : sjisToUniEUDC(code)) - EUDCCODEBASE);
}
/***************************************************************
 *	Read Bitmap
 */
/* */	int
/* */	GetW31JEUDCFont(
/* */		unsigned short	code,	/*  native-code */
/* */		LPBYTE buf,	/* buffer to set bitmap */
/* */		int	bufsiz,	/* Buffer Size */
/* */		int	*xsiz,	/* Bitmap X,Ysiz */
/* */		int	*ysiz,
/* */       BOOL bUnicode)
/*
 *	returns : >=0, -1
 ***************************************************************/
{
	HANDLE	fHdl;
	long	ofs;
struct BMPHeader	fhdr;
	int	bmpsiz;
	int	rdsiz;
	int	rec;
	DWORD nByte;
	BOOL res;

	rec = codeToRec( code, bUnicode);
	if (init==0)
		return -1;
	else if ( maxRec < rec || rec < 0)
		return -1;

	/* Open Font File */
	fHdl = CreateFile(fpath,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fHdl == INVALID_HANDLE_VALUE)
		return	-1;
	/* read bitmap ptr on subTable */
	ofs = bdatptr + sizeof(long)*rec;
	if ( (long) SetFilePointer( fHdl, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ECLOSE_RET;
	res = ReadFile( fHdl, &ofs, sizeof(long), &nByte, NULL);
	if (!res || nByte !=sizeof(long))
		goto	ECLOSE_RET;
	if ( ofs==0L)	
		goto	ECLOSE_RET;
	ofs += bdathead;

	/* read Bitmap Header
		bitmap is Word aligned */
	if ( (long) SetFilePointer( fHdl, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ECLOSE_RET;
	res = ReadFile( fHdl, &fhdr, sizeof(struct BMPHeader), &nByte, NULL);
	if (!res || nByte != sizeof( struct BMPHeader))
		goto	ECLOSE_RET;

	bmpsiz = ((int)fhdr.xsiz+15)/16 *2 * (int)fhdr.ysiz;
	/* Read Bitmap Body */
	rdsiz = bmpsiz > bufsiz ? bufsiz : bmpsiz;

	res = ReadFile( fHdl, buf, (unsigned short)rdsiz, &nByte, NULL);
	if (!res || nByte !=(unsigned short)rdsiz)
		goto	ECLOSE_RET;
	rdsiz = bmpsiz > bufsiz ? bmpsiz - bufsiz : 0;
	*xsiz = fhdr.xsiz;
	*ysiz = fhdr.ysiz;

	CloseHandle (fHdl);
	return rdsiz;
ECLOSE_RET:
	CloseHandle (fHdl);
	return -1;
}
/***************************************************************
 *	Write Bitmap
 */
/* */	int
/* */	PutW31JEUDCFont(
/* */		unsigned short code,	/* native code */
/* */		LPBYTE buf,	/* buffer to set bitmap */
/* */		int	xsiz,	/* Bitmap X,Ysiz */
/* */		int	ysiz,
/* */       BOOL bUnicode)
/*
 *	returns : 0, -1
 ***************************************************************/
{
	HANDLE fHdl;
	long	ofs;
struct BMPHeader	fhdr;
	int	bmpsiz;
	int	wbmpsiz;
struct BDatSubTbl subTbl;
	int	rec;
	DWORD nByte;
	BOOL res;

	rec = codeToRec( code, bUnicode);

	if (init==0)
		return -1;
	else if ( maxRec < rec || rec < 0)
		return -1;
	/* Open Font File */
	fHdl = CreateFile(fpath,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fHdl == INVALID_HANDLE_VALUE)
		return	-1;

	/* read bitmap ptr on subTable */
	if (ReadBDatEntry( fHdl, &ofs, rec))
		goto	ECLOSE_RET;

	wbmpsiz = (xsiz+15)/16 *2 * ysiz;
	if ( ofs != 0L) {
		/* read Bitmap Header
			bitmap is Word aligned */
		if ( ReadBMPHdr( fHdl, ofs, &fhdr))
			goto	ECLOSE_RET;

		bmpsiz = ((int)fhdr.xsiz+15)/16 *2 * (int)fhdr.ysiz;
		if ( bmpsiz<wbmpsiz)
			ofs = 0L;
	}
	if ( ReadBdatSub( fHdl, bdathead, &subTbl))
		goto	ECLOSE_RET;
	if ( ofs == 0L) {
		ofs = subTbl.tail;
		subTbl.tail += wbmpsiz+sizeof(fhdr);
	}
	/* Write Bitmap Header */
	fhdr.xsiz = (short)xsiz;
	fhdr.ysiz = (short)ysiz;
	fhdr.bitmapSiz = wbmpsiz+sizeof(fhdr);
	if ( WriteBMPHdr( fHdl, ofs, &fhdr))
		goto	ECLOSE_RET;

	/* Write Bitmap Body */
	res = WriteFile( fHdl, buf, (unsigned short)wbmpsiz, &nByte, NULL);
	if (!res || nByte !=(unsigned short)wbmpsiz)
		goto	ECLOSE_RET;

	/* write bitmap ptr on subTable */
	if (WriteBDatEntry( fHdl, ofs, rec))
		goto	ECLOSE_RET;

	/* write subTable */
	if ( WriteBdatSub( fHdl, bdathead, &subTbl))
		goto	ECLOSE_RET;
	CloseHandle (fHdl);

	return 0;
ECLOSE_RET:
	CloseHandle (fHdl);
	return -1;
}
static int
ReadBdatSub( HANDLE hdl, long ofs, struct BDatSubTbl *tbl)
{
	DWORD nByte;
	BOOL res;
	if ( (long) SetFilePointer( hdl, ofs, NULL, FILE_BEGIN)!= ofs)
		goto	ERET;

	res = ReadFile( hdl, tbl, sizeof (struct BDatSubTbl), &nByte, NULL);
	if (!res || nByte !=sizeof (struct BDatSubTbl))
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static int
WriteBdatSub( HANDLE hdl, long ofs, struct BDatSubTbl *tbl)
{
	DWORD nByte;
	BOOL res;
	if ( (long) SetFilePointer( hdl, ofs, NULL, FILE_BEGIN)!= ofs)
		goto	ERET;

	res = WriteFile( hdl, (char *)tbl, sizeof (struct BDatSubTbl), &nByte, NULL);
	if (!res || nByte !=sizeof (struct BDatSubTbl))
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static int
ReadBDatEntry( HANDLE hdl, long *ofs, long rec)
{
	DWORD nByte;
	BOOL res;
	long	ofsofs;

	ofsofs = bdatptr+(long)sizeof(long)*rec;
	if ( (long) SetFilePointer( hdl, ofsofs, NULL, FILE_BEGIN)!=ofsofs)
		goto	ERET;
	res = ReadFile( hdl, ofs, sizeof (long), &nByte, NULL);
	if (!res || nByte != sizeof (long))
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static int
WriteBDatEntry( HANDLE hdl, long ofs, long rec)
{
	long	ofsofs;
	DWORD nByte;
	BOOL res;

	ofsofs = bdatptr+(long)sizeof(long)*rec;
	if ( (long) SetFilePointer( hdl, ofsofs, NULL, FILE_BEGIN)!=ofsofs)
		goto	ERET;
	res = WriteFile( hdl, (char *)&ofs, sizeof(long), &nByte, NULL);
	if (!res || nByte != sizeof(long))
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static int
ReadBMPHdr( HANDLE hdl, long ofs, struct BMPHeader *hdr)
{
	DWORD nByte;
	BOOL res;

	ofs += bdathead;
	if ( (long) SetFilePointer( hdl, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ERET;
	res = ReadFile( hdl, hdr, sizeof( struct BMPHeader), &nByte, NULL);
	if (!res || nByte !=sizeof( struct BMPHeader))
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static int
WriteBMPHdr( HANDLE hdl, long ofs, struct BMPHeader *hdr)
{
	DWORD nByte;
	BOOL res;
	ofs += bdathead;
	if ( (long) SetFilePointer( hdl, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ERET;
	res = WriteFile( hdl, (char *)hdr, sizeof( struct BMPHeader), &nByte, NULL);
	if (!res || nByte !=sizeof( struct BMPHeader))
		goto	ERET;
	return 0;
ERET:
	return -1;
}

/***************************************************************
 *	is Win95 EUDC bitmap
 */
/* */	int
/* */   IsWin95EUDCBmp(LPTSTR szBmpPath)
/*
 *	returns : 0 (other), 1 (EUDC bitmap), -1(error)
 ***************************************************************/
{
	HANDLE fhdl;
struct W31_Header hdr;
	DWORD nByte;
	BOOL res;

	fhdl = CreateFile(szBmpPath,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fhdl == INVALID_HANDLE_VALUE)
    {
		return -1;
    }
	res = ReadFile( fhdl, (LPBYTE)&hdr, sizeof(hdr), &nByte, NULL);
	CloseHandle( fhdl);
	if (!res || nByte !=sizeof(hdr)){
		return 0;
	}	

	/* compare idendify leading 16 byte, sCode, eCode and cCnt*/
	if (memcmp( hdr.identify, "Windows95 EUDC", 14))
    {
		return 0;
    }
	if(hdr.sCode != 0x00e0){
		return 0;
	}
	return 1;
}
/* EOF */

////////////////////////////////////////////////////////////////////
//
// To work-around font linking in "Select Code" and "Save Char As".
// so that a typeface specific font does not show any glyph it does
// not have (from EUDC.TTE)
//
//     path   = *.euf
//     pGlyph = an array of 800 byte for 6400 EUDC chars
//
////////////////////////////////////////////////////////////////////
BOOL
GetGlyph(TCHAR *Path, BYTE* pGlyph)
{
	HANDLE fHdl;
    struct W31_Header hdr;
	DWORD  nByte;
    long   Offset;
    WORD   wc;
    long   lptr;
    TCHAR *pChar;
    TCHAR  PathEUF[MAX_PATH];
	BOOL   bRet = FALSE;

    lstrcpy(PathEUF, Path);
    pChar = PathEUF + lstrlen(PathEUF) - 3;
    *pChar = 0;
    lstrcat(PathEUF, TEXT("EUF"));

	fHdl = CreateFile(PathEUF,
                      GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);
    if(fHdl == INVALID_HANDLE_VALUE) return FALSE;

    bRet = ReadFile( fHdl, &hdr, sizeof(struct W31_Header), &nByte, NULL);
    if(!bRet && nByte !=sizeof(struct W31_Header)) goto Done;

    lptr = hdr.ofsBdatSub + sizeof(struct BDatSubTbl);
    if((long)SetFilePointer(fHdl, lptr, NULL, FILE_BEGIN) != lptr) goto Done;

    memset(pGlyph, 0, 800);

    for(wc = 0; wc < hdr.cCnt; wc++)
    {
        bRet = ReadFile( fHdl, &Offset, sizeof(long), &nByte, NULL);
        if(!bRet) goto Done;
        if(Offset == 0L || nByte !=sizeof(long)) continue;

        pGlyph[wc>>3] |= (0x80>>(wc%8));
    }

Done:
    CloseHandle(fHdl);
    return bRet;
}
