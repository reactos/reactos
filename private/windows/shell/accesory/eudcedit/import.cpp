//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
/*
 *	Import function for W31JEUDC and ETEN
 *----------------------------------------------
 *   bitmap proccessing steps
 *	1. Read bitmap
 *	2. Make outline
 *	3. Smoothing
 *	4. Rasterize -> editting bitmap image
 *	5. Make outline
 *	6. Smoothing
 *	7. Fitting
 *	8. Output TTF and bitmap
 *
 *   File proccessing
 *	1.Copy .EUF as temp to update
 *	2.Copy TTF tables as temp to update
 *	3.Make input bitamp code-rec table
 *	4.Per glyph proc.
 *	5.Replace files
 *
 *   Per glyph proccessing
 *	1.judge to merge to make glyph with input bitmap code-rec table.
 *	2.merge or make glyphdata and metrics
 *
 */

#include	"stdafx.h"
#include	"eudcedit.h"

#pragma		pack(2)
extern BOOL	SendImportMessage(unsigned int cEUDC, unsigned int nRec);

#include	"vdata.h"
#include	"ttfstruc.h"
#include	"extfunc.h"

#define		OUTLSTH		0
#define		TMPLSTH		1
#define		EUDCCODEBASE	((unsigned short)0xe000)


static void  pline(int  bmpNo,int  sx,int  sy,int  tx,int  ty);
static int  rasterize(int  lstHdl,int  bmpNo, int mesh, int outSiz);
static int  initmem(int  iSiz,int  oSiz);
static void  termmem(void);
static int  modmem(int  iSiz);
int  Import(TCHAR *eudcPath, TCHAR *bmpPath,TCHAR *ttfPath,int  oWidth,int  oHeight,int level, BOOL bIsWin95EUDC);
/* For Import static */
static	int	iBmpSiz;
static	int	oBmpSiz;
static	BYTE	*rBuf, *wkBuf, *refBuf;
static	BYTE	*oBuf, *owkBuf, *orefBuf;
static	int	iBmpNo, wkBmpNo, refBmpNo;
static	int	oBmpNo, owkBmpNo, orefBmpNo;
static  int	*recTbl=0;

static void
pline( int bmpNo, int sx, int sy, int tx, int ty)
{
	int	dx, dy;
	int	dx2, dy2;
	int	exy;
	int	tmp;
	

	dx = abs( sx - tx);
	dy = abs( sy - ty);
	dx2 = dx*2;
	dy2 = dy*2;

	if ( dx==0) {
		if( sy>ty) {
			while ( sy>ty) {
				sy--;
				ReverseRight( bmpNo, sx, sy);
			}
		}
		else if ( sy < ty) {
			while ( sy < ty) {
				ReverseRight( bmpNo, sx, sy);
				sy++;
			}
		}
	}
	else if ( dy==0)
		;
/*Loose*/
	else if ( dx >= dy){
		if (sx > tx) {
			tmp = tx;
			tx = sx;
			sx = tmp;
			tmp = ty;
			ty = sy;
			sy = tmp;
		}
		exy = -dx ;
	
		if ( sy < ty ) {
			while ( sx <= tx) {
				exy += dy2;
				sx++;
				if ( exy > 0) {
					exy -= dx2;
					if ( sy!=ty)
						ReverseRight( bmpNo, sx, sy);
					sy++;
				}
			}
		}
		else {
			while ( sx <= tx) {
				exy += dy2;
				sx++;
				if ( exy > 0) {
					exy -= dx2;
					sy--;
					if ( sy >= ty)
						ReverseRight( bmpNo, sx, sy);
				}
			}
		}
		
/*Steep*/	
	}
	else {	
		if (sy > ty) {
			tmp = tx;
			tx = sx;
			sx = tmp;
			tmp = ty;
			ty = sy;
			sy = tmp;
		}
		exy = -dy ;
	/*	while ( sy <= ty) { */
		while ( sy < ty) { 
			ReverseRight( bmpNo, sx, sy);
			exy += dx2;
			if ( exy >= 0) {	
				exy -= dy2;
				if ( sx < tx)	sx++;
				else		sx--;
			}
			sy++;
		}
	}
}
static int
rasterize( int lstHdl, int bmpNo, int mesh, int outSiz)
/* lstHdl : abs coord*/
{
	int	nliais, nelm;
	int	liais;
struct VHEAD	*vhead;
struct VDATA	*vp;
struct vecdata	lvd, cvd;
	if ( (nliais = VDGetNCont( lstHdl))<0)
		goto	ERET;

	BMPClear( bmpNo);
	if ( VDGetHead( lstHdl, &vhead)) 
		goto	ERET;
	for ( liais = 0; liais < nliais; liais++) {
		nelm = vhead->nPoints;
		lvd = vhead->headp->vd;
		lvd.x = (lvd.x * outSiz+mesh/2)/mesh;
		lvd.y = (lvd.y * outSiz+mesh/2)/mesh;
		vp = vhead->headp->next;
		while ( nelm-- > 0) {
			cvd = vp->vd;
			cvd.x = (cvd.x * outSiz+mesh/2)/mesh;
			cvd.y = (cvd.y * outSiz+mesh/2)/mesh;
			pline( bmpNo, lvd.x, lvd.y, cvd.x, cvd.y);
			lvd = cvd;
			vp = vp->next;
		}
		vhead = vhead->next;
	}
	return 0;
ERET:
	return -1;
}
static int
initmem( int iSiz,  int oSiz)
{
	iBmpSiz = (iSiz+15)/16*2*iSiz;
	
	rBuf = wkBuf = refBuf = 0;
	oBuf = owkBuf = orefBuf = 0;

	if ( (rBuf = (LPBYTE)malloc( iBmpSiz))==0)
		goto	ERET;
	if ( (wkBuf =(LPBYTE)malloc( iBmpSiz))==0)
		goto	ERET;
	if ( (refBuf =(LPBYTE)malloc( iBmpSiz))==0)
		goto	ERET;

	if ( (iBmpNo = BMPDefine( rBuf, iSiz, iSiz))<0)
		goto	ERET;
	if ( (wkBmpNo = BMPDefine( wkBuf, iSiz, iSiz))<0)
		goto	ERET;
	if ( (refBmpNo = BMPDefine( refBuf, iSiz, iSiz))<0)
		goto	ERET;

	oBmpSiz = (oSiz+15)/16*2*oSiz;

	if ( (oBuf = (LPBYTE)malloc( oBmpSiz))==0)
		goto	ERET;
	if ( (owkBuf = (LPBYTE)malloc( oBmpSiz))==0)
		goto	ERET;
	if ( (orefBuf = (LPBYTE)malloc( oBmpSiz))==0)
		goto	ERET;

	if ( (oBmpNo = BMPDefine( oBuf, oSiz, oSiz))<0)
		goto	ERET;
	if ( (owkBmpNo = BMPDefine( owkBuf, oSiz, oSiz))<0)
		goto	ERET;
	if ( (orefBmpNo = BMPDefine( orefBuf, oSiz, oSiz))<0)
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static void
termmem()
{
	if ( rBuf )	free( rBuf);
	if ( refBuf )	free( refBuf);
	if ( wkBuf )	free( wkBuf);
	if ( oBuf )	free( oBuf);
	if ( orefBuf )	free( orefBuf);
	if ( owkBuf )	free( owkBuf);

	oBuf = wkBuf = refBuf = 0;
	rBuf = orefBuf = owkBuf = 0;
	recTbl = 0;
}
static int
modmem( int iSiz)
{
	free( rBuf);
	free( wkBuf);
	free( refBuf);
	BMPFreDef( iBmpNo);
	BMPFreDef( wkBmpNo);
	BMPFreDef( refBmpNo);
	if ( (rBuf = (LPBYTE)malloc( iBmpSiz))==0)
		goto	ERET;
	if ( (wkBuf = (LPBYTE)malloc( iBmpSiz))==0)
		goto	ERET;
	if ( (refBuf = (LPBYTE)malloc( iBmpSiz))==0)
		goto	ERET;

	if ( (iBmpNo = BMPDefine( rBuf, iSiz, iSiz))<0)
		goto	ERET;
	if ( (wkBmpNo = BMPDefine( wkBuf, iSiz, iSiz))<0)
		goto	ERET;
	if ( (refBmpNo = BMPDefine( refBuf, iSiz, iSiz))<0)
		goto	ERET;
	return 0;
ERET:
	return -1;
}
/*********************************************************************
 *	Make rec-gid table of input bitmap
 */
/* */	static int
/* */	makeRecTbl(
/* */	 	int	nRec,
/* */		BOOL bIsWin95EUDC)
/*
 *	returns : none
 *********************************************************************/
{
	int	sts;

	if ( CountryInfo.LangID == EUDC_JPN || bIsWin95EUDC)
		sts = W31JrecTbl(&recTbl, bIsWin95EUDC);
	else
		sts = ETENrecTbl(&recTbl);
				
	return sts;
}
static int
impSub( 
	int	rec,
struct	BBX	*bbx,
	short	uPEm,
	int	oWidth, 	/* output bmp width */
	int	oHeight,	/* output bmp height(==width) */
struct SMOOTHPRM *prm,
	BOOL bIsWin95EUDC)
{
	int	rdsiz;
	int	width, height;
	char	UserFontSign[8];
	WORD	BankID;
unsigned short	code;
	int	sts;
	int	nRec;
	int	nGlyph;
	BOOL bUnicode;

	/* Read EUDC Bitmap */
	if ( CountryInfo.LangID == EUDC_JPN || bIsWin95EUDC) {
		rdsiz = GetW31JBMPRec( rec, (LPBYTE)rBuf, iBmpSiz, &width, &height, &code);
		if ( rdsiz < 0)
			goto	ERET;
		else if ( rdsiz==0)
			return 0;
		if ( rdsiz > iBmpSiz) {
			iBmpSiz = rdsiz;
			modmem( width);
			if ( GetW31JBMPRec( rec, (LPBYTE)rBuf, iBmpSiz,
					 &width, &height, &code)<0)
				goto	ERET;
		}

	}
	else {
		if ( getETENBMPInf( &nRec, &nGlyph, &width, &height,
		     UserFontSign, &BankID)) {
			sts = -2;
			goto	ERET;
		}
		iBmpSiz = (width+7)/8*height;
		if (readETENBMPRec( rec, (LPBYTE)rBuf, iBmpSiz, &code)) {
			sts = -3;
			goto	ERET;
		}
	}	
	if( !memcmp( UserFontSign,"CMEX_PTN", 8) && BankID == 0x8001 || bIsWin95EUDC)
		bUnicode = TRUE;
	else	bUnicode = FALSE;

	/* vectorize */
	if( memcmp( UserFontSign,"CMEX_PTN", 8))
		BMPReverse( iBmpNo);
	if ( (BMPMkCont(  iBmpNo, wkBmpNo, refBmpNo, OUTLSTH))<0) {
		sts = -4;
		goto	ERET;
	}

	/* Smoothing */
	if (SmoothLight( OUTLSTH, TMPLSTH, width, height, oWidth*4, 16)) {
		sts = -5;
		goto	ERET;
	}
	rasterize( OUTLSTH, oBmpNo, oWidth*4, oWidth);

	/* Write Bitmap */
	BMPReverse( oBmpNo);

	if (PutW31JEUDCFont(code,(LPBYTE)oBuf,  oWidth, oWidth, bUnicode)) {
		sts = -6;
		goto	ERET;
	}
	BMPReverse( oBmpNo);

	if ( BMPMkCont(  oBmpNo, owkBmpNo, orefBmpNo, OUTLSTH)<0) {
		sts = -7;
		goto	ERET;
	}
	if (SmoothVector( OUTLSTH, TMPLSTH, oWidth, oHeight, oWidth*4,prm , 16)) {
		sts = -8;
		goto	ERET;
	}

	if (ConvMesh( OUTLSTH,oWidth*4, uPEm)) {
		sts = -9;
		goto	ERET;
	}
	if ( RemoveFp( OUTLSTH, uPEm, 16)) {
		sts = -10;
		goto	ERET;
	}
	if ( toTTFFrame( OUTLSTH, bbx)) {
		sts = -11;
		goto	ERET;
	}

	if( !bUnicode) 
		code = sjisToUniEUDC( code);

	/* write TTF */
	if ( TTFAppend( code, bbx, OUTLSTH)) {
		sts = -12;
		goto	ERET;
	}
	return 0;
ERET:
	return sts;
}
/*********************************************************************
 *	Import WIN31J EUDC or ETEN contiguous
 */
/* */	int
/* */	Import( 
/* */		TCHAR	*eudcPath, 	/* W31J EUDC Bitmap .fon*/
/* */		TCHAR	*bmpPath, 	/* Win95 EUDCEDIT bitmap .euf*/
/* */		TCHAR	*ttfPath,	/* TTF EUDC .ttf */
/* */		int	oWidth, 	/* output bmp width */
/* */		int	oHeight,	/* output bmp height(==width) */
/* */		int	level,
/* */		BOOL bIsWin95EUDC)
/*
 *	returns : 0, -1
 *********************************************************************/
{
	int	nRec;
	int	rec;
	int	width, height;
	char	UserFontSign[8];
	short	uPEm;
struct BBX	bbx;
	WORD	BankID;
	unsigned short	maxC;
	TCHAR	tmpPath[MAX_PATH];
	TCHAR	savPath[MAX_PATH];
	HANDLE	orgFh=INVALID_HANDLE_VALUE;
	int	sts;
struct SMOOTHPRM	prm;
	int	nGlyph;
	int	gCnt;
	int	cancelFlg;

//	orgFh = 0;
	BMPInit();
	VDInit();
	makeUniCodeTbl();
	maxC = getMaxUniCode();
	prm.SmoothLevel = level;
	prm.UseConic = 1;

	TTFTmpPath( ttfPath, tmpPath);
	if ( TTFImpCopy( ttfPath, tmpPath))
		goto	ERET;

	/* Open W31J EUDC bitmap font file userfont.fon or CWin31 ETEN*/
	if ( CountryInfo.LangID == EUDC_JPN || bIsWin95EUDC) {
		if (OpenW31JBMP( eudcPath, 0))
			goto	ERET;
	}
	else {
		if (openETENBMP( eudcPath, 0))
			goto	ERET;
	}

	/* Open EUDCEDIT .EUF File */
	if ( OpenW31JEUDC( bmpPath))
  {
    if (creatW31JEUDC(bmpPath))
		  goto	ERET;
    else
      if (OpenW31JEUDC( bmpPath))
        goto ERET;
  }

	if ( CountryInfo.LangID == EUDC_JPN || bIsWin95EUDC) {
		/* get number of record */
		if ( GetW31JBMPnRecs(&nRec, &nGlyph, &width, &height))
			goto	ERET;
		iBmpSiz = (width + 7)/8 * height;
	}
	else{
		if ( getETENBMPInf( &nRec, &nGlyph, &width, &height, 
		     UserFontSign, &BankID))
			goto	ERET;
		iBmpSiz = (width+7)/8*height;

	}

	/* Limit nRec */
	if ( nRec > (int)( maxC-EUDCCODEBASE+1))
		nRec = (int)( maxC-EUDCCODEBASE+1);
	initmem( width, oWidth);

	if ( makeRecTbl( nRec, bIsWin95EUDC))
		goto	ERET;

	/* Get BBX */
	if ( TTFGetEUDCBBX( ttfPath, &bbx, &uPEm))
		goto	ERET;
	/* Open temporaly */
	if ( TTFOpen( tmpPath))
		goto	ERET;

	/* Open Original */
	orgFh = CreateFile(ttfPath,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( orgFh == INVALID_HANDLE_VALUE)
		goto	ERET;
	/* copy missing glyph*/
	TTFImpGlyphCopy(orgFh, 0);
	/* per glyph */
	gCnt = 0;
	cancelFlg = 0;
	for ( rec = 0; rec < nRec; rec++) {
		if ( recTbl[rec]>= 0) {
			gCnt++;
			if ( gCnt < nGlyph) {
				if (SendImportMessage((unsigned int)gCnt,
						(unsigned int)nGlyph)==0)
					cancelFlg=1;
			}
		}
		if ( cancelFlg==0 && recTbl[rec]>= 0) {
			if ((sts = impSub(recTbl[rec],&bbx,uPEm,oWidth, oHeight,&prm,bIsWin95EUDC))<0)
				goto	ERET;
	 		else if (sts >0)
				break;
		}
		else {
			if (TTFImpGlyphCopy(orgFh, rec+2)) 
				goto	ERET;
		}
	}

	
	SendImportMessage((unsigned int)nGlyph, (unsigned int)nGlyph);

	if ( TTFImpTerm(orgFh, rec+2))
		goto ERET;
	
	
	CloseHandle( orgFh);
	
	if ( TTFClose())
		goto ERET;

	if ( CountryInfo.LangID == EUDC_JPN || bIsWin95EUDC) {
		if (CloseW31JBMP())
			goto	ERET;
	
	}
	else {
		if (closeETENBMP())
			goto	ERET;
	}
	CloseW31JEUDC();

	/* Replace file */
	TTFTmpPath( ttfPath, savPath);
	if ( DeleteFile( savPath)==0)
		goto	ERET;
	if (MoveFile( ttfPath, savPath)==0)
		goto	ERET;
	if (MoveFile( tmpPath, ttfPath)==0)
		goto	ERET;

	if ( DeleteFile( savPath)==0)
		goto	ERET;
	VDTerm();
	termmem();
	return 0;
ERET:
	if ( orgFh != INVALID_HANDLE_VALUE) {
		CloseHandle( orgFh);
		orgFh = INVALID_HANDLE_VALUE;
	}
	TTFClose();
	CloseW31JBMP();
	CloseW31JEUDC();
	VDTerm();
	termmem();
	return -1;
}
/* EOF */
