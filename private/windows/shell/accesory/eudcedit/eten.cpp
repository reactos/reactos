
/*
 *	CWIN3.1 and ETEN format file i/o
 *
 * Copyright (c) 1997-1999 Microsoft Corporation.
 */

#include	"stdafx.h"
#pragma		pack(2)


#include	"extfunc.h"
#include	"eten.h"

#define		EUDCCODEBASE	((unsigned short)0xe000)
#define		ETENBANKID	0x8001
/*
static unsigned short  getsval(unsigned char  *s);
static unsigned long  getlval(unsigned char  *l);
static void  setsval(unsigned char  *m,unsigned short  s);
static void  setlval(unsigned char  *m,unsigned long  lval);
static int  readHdr(HANDLE fhdl,struct  ETENHEADER *hdr);
static int  updHdr(void);
static int  getETENBankID( HANDLE  fh, WORD  *BankID);
int  openETENBMP(TCHAR *path,int  md);
int  closeETENBMP(void);
static void  setIniHdr(struct  R_ETENHEADER *hdr,int  width,int  height);
int  createETENBMP(TCHAR *path,int  wid,int  hei);
int  getETENBMPInf(int  *n, int *ng, int  *wid,int  *hei, char *sign,WORD *bID);
int  readETENBMPRec(int  rec,LPBYTE buf,int  bufsiz,unsigned short  *code);
int  appendETENBMP(LPBYTE buf,unsigned short  code);
int  isETENBMP(TCHAR  *path);
*/
static HANDLE	bmpFh ;
static int	openmd = -1;
static int	nChar;
static int	width, height;
static int	*recordBuf=0;
	int	uNum=0;
static int	*codep=0;
static char	UserFontSign[8];
static WORD	BankID;

static unsigned short
getsval( unsigned char *s)
{
	unsigned short	sval;

	sval = (unsigned short )*(s+1);
	sval <<=8;
	sval |= (unsigned short )*s;
	return sval;
}
static unsigned long
getlval( unsigned char *l)
{
	unsigned long	lval;
	int	i;

	lval = (unsigned long)*(l+3);
	for ( i=2; i>=0; i--) {
		lval<<=8;
		lval |=(unsigned long)*(l+i);
	}
	return lval;
}
static void
setsval( unsigned char *m, unsigned short s)
{
	*m = (unsigned char)(s & 0xff);
	*(m+1) = (unsigned char)((s>>8) & 0xff);
}
static void
setlval( unsigned char *m, unsigned long lval)
{
	int	i;

	for ( i=0; i<4; i++) {
		*m++ = (unsigned char)(lval & 0xff);
		lval>>=8;
	}
}
static int
readHdr( HANDLE fhdl, struct ETENHEADER *hdr)
{
struct R_ETENHEADER	rhdr;
DWORD nByte;
BOOL res;

	res = ReadFile( fhdl, &rhdr, 256, &nByte, NULL);
	if (!res || nByte !=256)
		goto	ERET;

	memset( hdr, 0, sizeof(struct ETENHEADER));

	/* Set values to hdr */
	hdr->uHeaderSize = getsval( rhdr.uHeaderSize);
	memcpy( hdr->idUserFontSign, rhdr.idUserFontSign, 8);
	hdr->idMajor = rhdr.idMajor;
	hdr->idMinor = rhdr.idMinor;
	hdr->ulCharCount = getlval( rhdr.ulCharCount);
	hdr->uCharWidth = getsval( rhdr.uCharWidth);
	hdr->uCharHeight = getsval( rhdr.uCharHeight);
	hdr->cPatternSize = getlval( rhdr.cPatternSize);
	hdr->uchBankID = rhdr.uchBankID;
	hdr->idInternalBankID = getsval( rhdr.idInternalBankID);
	hdr->sFontInfo.uInfoSize = getsval(rhdr.sFontInfo.uInfoSize);
	hdr->sFontInfo.idCP = getsval(rhdr.sFontInfo.idCP);
	hdr->sFontInfo.idCharSet = rhdr.sFontInfo.idCharSet;
	hdr->sFontInfo.fbTypeFace = rhdr.sFontInfo.fbTypeFace;
	memcpy( hdr->sFontInfo.achFontName , rhdr.sFontInfo.achFontName,12);
	hdr->sFontInfo.ulCharDefine = getlval(rhdr.sFontInfo.ulCharDefine);
	hdr->sFontInfo.uCellWidth = getsval(rhdr.sFontInfo.uCellWidth);
	hdr->sFontInfo.uCellHeight = getsval(rhdr.sFontInfo.uCellHeight);
	hdr->sFontInfo.uCharHeight = getsval(rhdr.sFontInfo.uCharHeight);
	hdr->sFontInfo.uBaseLine = getsval(rhdr.sFontInfo.uBaseLine);
	hdr->sFontInfo.uUnderLine = getsval(rhdr.sFontInfo.uUnderLine);
	hdr->sFontInfo.uUnlnHeight = getsval(rhdr.sFontInfo.uUnlnHeight);
	hdr->sFontInfo.fchStrokeWeight = rhdr.sFontInfo.fchStrokeWeight;
	hdr->sFontInfo.fCharStyle = getsval(rhdr.sFontInfo.fCharStyle);
	hdr->sFontInfo.fbFontAttrib = rhdr.sFontInfo.fbFontAttrib;
	hdr->sFontInfo.ulCellWidthMax = getlval(rhdr.sFontInfo.ulCellWidthMax);
	hdr->sFontInfo.ulCellHeightMax= getlval(rhdr.sFontInfo.ulCellHeightMax);
	return 0;
ERET:
	return -1;
}
static int
updHdr( )
{
struct R_ETENHEADER	rhdr;
DWORD nByte;
BOOL res;
	
	if ( (long) SetFilePointer( bmpFh, 0L, NULL, FILE_BEGIN)!=0L)
		goto	ERET;
	res = ReadFile( bmpFh, &rhdr, 256, &nByte, NULL);
	if (!res || nByte !=256)
		goto	ERET;

	setlval( rhdr.ulCharCount, (long)nChar);
	setlval( rhdr.sFontInfo.ulCharDefine, (long)nChar);
	if ( (long) SetFilePointer( bmpFh, 0L, NULL, FILE_BEGIN)!=0L)
		goto	ERET;
	res = WriteFile(bmpFh, (char *)&rhdr, 256,&nByte, NULL);
	if (!res || nByte !=256)
		goto	ERET;
	return 0;
ERET:
	return -1;
}
static int
getETENBankID( HANDLE fh, WORD *BankID)
{
struct R_CODEELEMENT	cElm;
	long	ofs;
	DWORD nByte;
	BOOL res;

	ofs = sizeof(struct R_ETENHEADER);
	if ((long) SetFilePointer( fh, ofs, NULL, FILE_BEGIN) != ofs)
		goto	ERET;

	res = ReadFile( fh, &cElm, sizeof(struct R_CODEELEMENT), &nByte, NULL);
	if (!res || nByte !=sizeof(struct R_CODEELEMENT))
		goto	ERET;
	*BankID = getsval( cElm.nBankID);
	return 0;
ERET:
	return -1;
}
/***
	recBuf
	+-------+
	| rec#	| E000
	+-------+
	|	| E001
	+-------+
	    |
	+-------+
	|	| maxUCode
	+-------+
****/
static int
scanETENBMP( int **recBuf, unsigned int maxUCode, int  nRec)
{
	long	ofs;
	int	recsiz, bmpsiz;
struct R_CODEELEMENT *bhd;
	int	rec;
	char	*rbuf;
	unsigned short	code;
	unsigned short	ucode;
	int	urec;
	int	*recp;
	DWORD nByte;
	BOOL res;
	
	recp = 0;
	rbuf = 0;
	if (  maxUCode < EUDCCODEBASE)
		return -1;
	else if ( nRec <=0)
		return -1;
	uNum = maxUCode - EUDCCODEBASE+1;
	if ( (codep = (int *)malloc( uNum*sizeof(int)))==0)
		goto	ERET;
		
	ofs = sizeof( struct R_ETENHEADER);
	if ( (long) SetFilePointer( bmpFh, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ERET;
	bmpsiz = (width+7)/8*height;
	recsiz =bmpsiz+sizeof (bhd);
	if ((rbuf = (char *)malloc( recsiz))==(char *)0)
		goto	ERET;
	for ( code = EUDCCODEBASE; code <= maxUCode; code++)
		codep[code-EUDCCODEBASE] = -1;
	bhd = (struct R_CODEELEMENT *)rbuf;

	for ( rec = 0; rec < nRec; rec++) {
		res = ReadFile( bmpFh, rbuf, (unsigned int)recsiz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)recsiz)
			goto	ERET;
		code = getsval( bhd->nInternalCode);
		if( memcmp(UserFontSign, "CMEX_PTN", 8) ||
		    BankID != ETENBANKID){
			ucode = sjisToUniEUDC( code);
		}else	ucode = code;

		if( ucode > maxUCode || ucode < EUDCCODEBASE)
			continue;
		urec = (int)(ucode - EUDCCODEBASE);
		codep[urec] = rec;
	}
	free( rbuf);

	if ( (recp = (int *)malloc( nRec*sizeof(int)))==0)
		goto	ERET;

	*recBuf=recp;
	for ( rec=0; rec < uNum; rec++) {
		if ( codep[rec]>0)
			*recp++ = codep[rec];
	}
	return 0;
ERET:
	if ( codep)	free( codep);
	if ( recp)	free( recp);
	if ( rbuf)	free( rbuf);
	return -1;
}
int
openETENBMP( TCHAR *path, int md)
{
	HANDLE	fh;
struct ETENHEADER hdr;

	makeUniCodeTbl();
	if ( md) {
		fh = CreateFile(path,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

		if ( fh == INVALID_HANDLE_VALUE)
			goto	ERET;
		bmpFh = fh;
		openmd = 1;
	}
	else {
		fh = CreateFile(path,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

		if ( fh == INVALID_HANDLE_VALUE)
			goto	ERET;
		bmpFh = fh;
		openmd = 0;
	}
	if (readHdr( fh, &hdr))
		goto	ERET;
	if (getETENBankID( fh, &BankID))
		goto 	ERET;

	nChar = (int)hdr.ulCharCount;
	width =  (int)hdr.uCharWidth;
	height =  (int)hdr.uCharHeight;
	memcpy((char *)UserFontSign, hdr.idUserFontSign, 8);

	if ( scanETENBMP( &recordBuf, getMaxUniCode(), nChar))
		goto	ERET;

	return 0;
ERET:
  if (fh != INVALID_HANDLE_VALUE)
    CloseHandle(fh);
	return -1;
}
int
closeETENBMP( )
{
	int	sts;
	if ( openmd)
		sts = updHdr();
	else	sts = 0;
	if ( bmpFh!=INVALID_HANDLE_VALUE)
		CloseHandle( bmpFh);
	if(recordBuf)	{
		free(recordBuf);
		recordBuf = 0;
	}
	if ( codep) {
		free(codep);
		codep = 0;
	}
	return sts;
}
static void
setIniHdr( struct R_ETENHEADER *hdr, int width, int height)
{
	memset( hdr, 0, sizeof(struct R_ETENHEADER));
	setsval( hdr->uHeaderSize, sizeof(struct R_ETENHEADER));
	memcpy( hdr->idUserFontSign, "CWIN_PTN", 8);
	hdr->idMajor = 1;
	hdr->idMinor = 0;
	setsval( hdr->uCharWidth, (unsigned short)width);
	setsval( hdr->uCharHeight, (unsigned short)height);
	setlval( hdr->cPatternSize, (unsigned long)(((width+7)/8)*height));
	setsval( hdr->sFontInfo.uInfoSize,
			(unsigned short)sizeof(struct R_CFONTINFO));
	setsval( hdr->sFontInfo.idCP, 938);
	hdr->sFontInfo.idCharSet = (char)0x88;
	setsval( hdr->sFontInfo.uCellWidth, (unsigned short)width);
	setsval( hdr->sFontInfo.uCellHeight, (unsigned short)height);
	setsval( hdr->sFontInfo.uCharHeight, (unsigned short)height);
	setlval( hdr->sFontInfo.ulCellWidthMax, (unsigned long)width);
	setlval( hdr->sFontInfo.ulCellHeightMax, (unsigned long)height);
}
int
createETENBMP( TCHAR *path, int wid, int hei)
{
	HANDLE	fh;
struct R_ETENHEADER	hdr;
	DWORD nByte;
	BOOL res;

	 fh = CreateFile(path,
					GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fh == INVALID_HANDLE_VALUE)
		goto	ERET;

	width = wid;
	height = hei;
	setIniHdr( &hdr, width, height);
	res = WriteFile( fh, (char *)&hdr, sizeof(hdr), &nByte, NULL);
	if (!res || nByte !=sizeof(hdr))
		goto	ERET;
	bmpFh = fh;
	openmd = 1;
	nChar =0;
	return 0;
ERET:
  if (fh != INVALID_HANDLE_VALUE)
    CloseHandle(fh);
	return -1;
}
int
getETENBMPInf( int *nRec, int *nGlyph, int *wid, int *hei, char *sign,WORD *bID)
{
	if ( bmpFh <0)	return -1;
	*nRec = uNum;
	*nGlyph = nChar;
	*wid = width;
	*hei = height;
	*bID = BankID;
	memcpy( sign, UserFontSign, 8);
	return 0;
}
int
readETENBMPRec( int rec, LPBYTE buf, int bufsiz, unsigned short *code)
{
	long	ofs;
	int	recsiz;
struct R_CODEELEMENT bhd;
	int	rdsiz;
	int	bWid, wWid;
	int	y, ylim;
	unsigned char	*rbuf;
	DWORD nByte;
	BOOL res;

	bWid = (width+7)/8;
	wWid = (bWid+1)/2*2;
	recsiz = (width+7)/8*height;
	ofs = sizeof( struct R_ETENHEADER)+(long)(recsiz+sizeof (bhd))*rec;
	if ( (long) SetFilePointer( bmpFh, ofs, NULL, FILE_BEGIN)!=ofs)
		goto	ERET;
	res = ReadFile( bmpFh, &bhd, sizeof(bhd), &nByte, NULL);
	if (!res || nByte !=sizeof(bhd))
		goto	ERET;
	if ( bufsiz<recsiz)	rdsiz = bufsiz;
	else			rdsiz = recsiz;

	if ( bWid!=wWid) {
		BYTE	*src, *dst;
		if ((rbuf = (unsigned char *)malloc( recsiz))==(unsigned char *)0)
			goto	ERET;
		res = ReadFile( bmpFh, (char *)rbuf, (unsigned int)recsiz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)recsiz) {
			free(rbuf);
			goto	ERET;
		}
		ylim = rdsiz / bWid;
		src = (LPBYTE)rbuf;
		dst = buf;
		memset( buf, 0xff, rdsiz);
		for ( y = 0; y < ylim; y++, src+=bWid, dst+=wWid)
			memcpy(dst , src , bWid);

		free( rbuf);
	}
	else {
		res = ReadFile( bmpFh, (char *)buf, (unsigned int)rdsiz, &nByte, NULL);
		if (!res || nByte !=(unsigned int)rdsiz)
			goto	ERET;
	}

	*code = getsval( bhd.nInternalCode);
	return 0;
ERET:
	return -1;
}
int
appendETENBMP( LPBYTE buf, unsigned short code)
{
struct R_CODEELEMENT bhd;
	int	bmpsiz;
	DWORD nByte;
	BOOL res;

	SetFilePointer( bmpFh, 0L, NULL, FILE_END);
	bmpsiz = (width+7)/8*height;
	setsval( bhd.nBankID, 1);
	setsval( bhd.nInternalCode, code);
	res = WriteFile( bmpFh, (LPBYTE)(&bhd), sizeof( bhd), &nByte, NULL);
	if (!res || nByte !=sizeof(bhd))
		goto	ERET;
	res = WriteFile( bmpFh, (LPBYTE)buf, (unsigned int)bmpsiz, &nByte, NULL);
	if (!res || nByte != (unsigned int)bmpsiz)
		goto	ERET;

	nChar++;
	return 0;
ERET:
	return -1;
}
int
isETENBMP(TCHAR *path)
{
struct ETENHEADER	hdr;
	HANDLE fhdl;
	fhdl = CreateFile(path,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fhdl == INVALID_HANDLE_VALUE)
		return -1;
	if ( readHdr( fhdl, &hdr)) {
		CloseHandle( fhdl);
		return -1;
	}
	CloseHandle( fhdl);
	/* check Header size and keyWord*/
	if ( hdr.uHeaderSize != sizeof(struct R_ETENHEADER))
		goto	NO_ETEN;

	if ( memcmp(hdr.idUserFontSign, "CWIN_PTN", 8) &&
	     memcmp(hdr.idUserFontSign, "CMEX_PTN", 8))
		goto	NO_ETEN;
	return 1;
NO_ETEN:
	return 0;
}

int
ETENrecTbl( int **recTbl)
{
	*recTbl=codep;
	return 0;
}
/* EOF */
/* For test  +/
static int
dispHdr( struct ETENHEADER *hdr)
{
	printf("hdr->uHeaderSize= %d\n", hdr->uHeaderSize );
	printf("hdr->idMajor %d\n", hdr->idMajor );
	printf("hdr->idMinor  %d\n", hdr->idMinor );
	printf("hdr->ulCharCout  %ld\n", hdr->ulCharCount );
	printf("hdr->uCharWidth  %d\n", hdr->uCharWidth );
	printf(" hdr->uCharHeight  %d\n", hdr->uCharHeight );
	printf(" hdr->cPatternSize %d\n", hdr->cPatternSize);
	printf("hdr->uchBankID %d\n", hdr->uchBankID);
	printf("hdr->idInternalBankID %d\n", hdr->idInternalBankID);
	printf("hdr->sFontInfo.uInfoSize %d\n", hdr->sFontInfo.uInfoSize);
	printf("hdr->sFontInfo.idCP %d\n", hdr->sFontInfo.idCP);
	printf("hdr->sFontInfo.idCharSet %d\n", hdr->sFontInfo.idCharSet);
	printf("hdr->sFontInfo.fbTypeFace %d\n", hdr->sFontInfo.fbTypeFace);
	printf("hdr->sFontInfo.ulCharDefine %ld\n", hdr->sFontInfo.ulCharDefine);
	printf("hdr->sFontInfo.uCellWidth %d\n", hdr->sFontInfo.uCellWidth);
	printf("hdr->sFontInfo.uCellHeight %d\n", hdr->sFontInfo.uCellHeight);
	printf("hdr->sFontInfo.uCharHeight %d\n", hdr->sFontInfo.uCharHeight);
	printf("hdr->sFontInfo.uBaseLine %d\n", hdr->sFontInfo.uBaseLine);
	printf("hdr->sFontInfo.uUnderLine %d\n", hdr->sFontInfo.uUnderLine);
	printf("hdr->sFontInfo.uUnlnHeight %d\n", hdr->sFontInfo.uUnlnHeight);
	printf("hdr->sFontInfo.fchStrokeWeight %d\n", hdr->sFontInfo.fchStrokeWeight);
	printf("hdr->sFontInfo.fCharStyle %d\n", hdr->sFontInfo.fCharStyle);
	printf("hdr->sFontInfo.fbFontAttrib %d\n", hdr->sFontInfo.fbFontAttrib);
	printf("hdr->sFontInfo.ulCellWidthMax %ld\n", hdr->sFontInfo.ulCellWidthMax);
	printf("hdr->sFontInfo.ulCellHeightMax %ld\n", hdr->sFontInfo.ulCellHeightMax);
}
main( int argc, char *argv[])
{
	int	fh;
struct ETENHEADER hdr;
	fh = _lopen( argv[1], O_RDONLY | O_BINARY);
	
	readHdr( fh, &hdr);
	dispHdr( &hdr);
	_lclose(fh);
	exit(0);
}
/+ */
/* EOF */
