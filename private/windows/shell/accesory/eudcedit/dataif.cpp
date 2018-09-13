

/*
 *	Bitmap and TTF control 
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation.
 */

#include	"stdafx.h"
#include	"eudcedit.h"


#pragma		pack(2)


#include	"vdata.h"
#include	"ttfstruc.h"
#include	"extfunc.h"

static void reverseBMP( unsigned char *mem, int siz);
static int makeNullGlyph( int lstH, struct BBX *bbx, short uPEm);

#define		OUTLSTH		0
#define		TMPLSTH		1
#define		NGLYPHLSTH	3
static int init = 0;
static int	inputSiz = 0;
/***********************************************************************
 *	initialize
 */
/* */	int
/* */	OInit( )
/*
 *	returns 0 , -1 (Failed)
 ***********************************************************************/
{
	if ( init)
		return 0;


	if (VDInit())
		return -1;
	else	{
		init = 1;
		return 0;
	}
}
/***********************************************************************
 *	initialize
 */
/* */	int
/* */	OTerm( )
/*
 *	returns 0 
 ***********************************************************************/
{
	if ( init) {
		init = 0;
		VDTerm();
	}
	return 0;
}
/***********************************************************************
 *	Make Outline
 */
/* */	int
/* */	OMakeOutline( 
/* */		unsigned char *buf, 
/* */		int	siz,
/* */		int	level)
/*
 *	returns : 0< : list handle, -1 ( error)
 ***********************************************************************/
{
	int	pb1, pb2, pb3;
struct SMOOTHPRM	prm;
	unsigned char	*tmp1, *tmp2;
	int	msiz;

	inputSiz = siz;
	reverseBMP( buf, ((siz+15)/16*2*siz));
	if ( init==0)
		if (OInit())	return -1;

	tmp1 = tmp2 = (unsigned char *)0;
	BMPInit();

	msiz = (siz+15)/16*2 * siz;

	if ( (tmp1 = (unsigned char *)malloc( msiz))==(unsigned char *)0)
		goto	ERET;
	if ( (tmp2 = (unsigned char *)malloc( msiz))==(unsigned char *)0)
		goto	ERET;
	if ( (pb1 = BMPDefine( buf, siz, siz))<0)
		goto	ERET;

	if ( (pb2 = BMPDefine( tmp1, siz, siz))<0)
		goto	ERET;
	if ( (pb3 = BMPDefine( tmp2, siz, siz))<0)
		goto	ERET;

	VDNew( OUTLSTH);
	if (BMPMkCont( pb1, pb2, pb3, OUTLSTH)<0)
		goto	ERET;
	prm.SmoothLevel = level;
	prm.UseConic = 1;
	/* “ü—Í‚Ì‚S”{‚Å¬”“_ˆÈ‰º‚Sƒrƒbƒg‚ÅŒvŽZ */
	if (SmoothVector( OUTLSTH, TMPLSTH, siz, siz,siz*4, &prm, 16))
		goto	ERET;

	VDCopy( OUTLSTH, TMPLSTH);
	RemoveFp( TMPLSTH, siz*4, 16);

	free( tmp1);
	free( tmp2);
	BMPFreDef( pb1);
	BMPFreDef( pb2);
	BMPFreDef( pb3);
	reverseBMP( buf, ((siz+15)/16*2*siz));

	/* •Ô‚·‚Ì‚ÍA“ü—Í‚Ì‚S”{‚É‚µ‚½‚à‚Ì */
	return TMPLSTH;
ERET:
	if ( tmp1)	free( tmp1);	
	if ( tmp2)	free( tmp2);	
	BMPFreDef( pb1);
	BMPFreDef( pb2);
	BMPFreDef( pb3);
	return -1;
}
/***********************************************************************
 *	check File exist
 */
/* */	int
/* */	OExistTTF( TCHAR	*path)
/*
 *	returns : 0, 1 (exist)
 ***********************************************************************/
{
	HANDLE	fh;
	fh = CreateFile(path,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fh == INVALID_HANDLE_VALUE)	return 0;
	CloseHandle( fh);
	return	1;
}
#ifdef BUILD_ON_WINNT
int OExistUserFont( TCHAR	*path)
{
	HANDLE	fh;
	fh = CreateFile(path,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fh == INVALID_HANDLE_VALUE)
		return 0;
	CloseHandle( fh);
	return	1;
}
#endif // BUILD_ON_WINNT
static void
setWIFEBBX( struct BBX *bbx, short *uPEm)
{
	
	bbx->xMin = 0;
	bbx->xMax = 255;
	bbx->yMin = 0;
	bbx->yMax = 255;
	*uPEm = 256;
}
/***********************************************************************
 *	Create EUDC TTF
 */
/* */	int
/* */	OCreateTTF( 
/* */		HDC	hDC,
/* */		TCHAR	*path,
/* */		int	fontType)
/* 		
 *	returns : 0, -1
 ***********************************************************************/
{
struct BBX	bbx;
	short	uPEm;

	if ( fontType)
		setWIFEBBX( &bbx, &uPEm);
	else {
		if (TTFGetBBX( hDC, &bbx, &uPEm))
			goto	ERET;
	}
	makeNullGlyph( NGLYPHLSTH, &bbx, uPEm);
	if (TTFCreate( hDC, path, &bbx,  NGLYPHLSTH, fontType))
		goto	ERET;
	return 0;
ERET:
	return -1;
}
/***********************************************************************
 *	Output to EUDC TTF
 */
/* */	int
/* */	OOutTTF( 
/* */		HDC	hDC,
/* */		TCHAR *path,	/* TrueType Path */
/* */		unsigned short	code,
/* */       BOOL bUnicode)
/*
 *	returns : 0, -1
 ***********************************************************************/
{
	int	mesh;
struct BBX	bbx;
	short	uPEm;
	int	sts;

	if (TTFGetEUDCBBX( path, &bbx, &uPEm))
		goto	ERET;

	mesh = uPEm;
	/* OUTLSTH mesh is inputBitmapSiz*4 , and made into ufp 4bit*/
	/* ufp : under Fixed Point */
	ConvMesh( OUTLSTH, inputSiz*4, mesh);
	RemoveFp( OUTLSTH,  mesh, 16);

	if (toTTFFrame( OUTLSTH, &bbx))
		goto	ERET;
    if (!bUnicode)
    {
	    code = sjisToUniEUDC( code);
    }
	if ( sts = TTFAddEUDCChar( path,code, &bbx,  OUTLSTH)) {
    if (sts == -3) // tte file is being used by another process.
      return -3;
		if ( TTFLastError()==-2)
			return -2;
		else	return -1;
	}
	return 0;
ERET:
	return -1;
}
static void
smtoi( unsigned short *s)
{
	unsigned short	sval;
	unsigned char	*c;

	c = (unsigned char *)s;
	sval = *c;
	sval<<=8;
	sval += (unsigned short)*c;
}
static void
reverseBMP( unsigned char *mem, int siz)
{
	while ( siz-->0)
		*mem++ ^= (unsigned char)0xff;
}
static int
makeNullGlyph( int lstH, struct BBX *bbx, short uPEm)
{
	int	width;
	int	height;
	int	cx, cy;
	int	dx, dy;
struct vecdata	vd;

	width = height = uPEm;
	cx = bbx->xMin + width/2;
	cy = bbx->yMin + height/2;
	dx = width/20;
	dy = height/20;

	VDNew( lstH);

	vd.atr = 0;
	vd.x = cx - dx;
	vd.y = cy - dy;

	if (VDSetData( lstH, &vd)) goto	ERET;
	vd.y = cy + dy;

	if (VDSetData( lstH, &vd)) goto	ERET;
	vd.x = cx + dx;

	if (VDSetData( lstH, &vd)) goto	ERET;
	vd.y = cy - dy;

	if (VDSetData( lstH, &vd)) goto	ERET;
	if ( VDClose( lstH))	goto	ERET;
	return 0;
ERET:
	return -1;
}
/* EOF */
