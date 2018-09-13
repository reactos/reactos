//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

/*****
 * FITCONIC.C
 *****/

int  FitConic(int  inLst,int  outLst,int  level,int  ufp);

/*****
 *	BMPOPE.C
 *****/

void  BMPInit(void);
int  BMPDefine(unsigned char  *buf,int  xWid,int  yWid);
int  BMPFreDef(int  bmpno);
int  BMPMkCont(int  BMPNo,int  wkBMP,int  refBMP,int  lsthdl);
int  rdot(int  BMP,int  x,int  y);
void  wdot(int  BMP,int  x,int  y,int  onoff);
int  ReverseRight(int  BMPNo,int  x,int  y);
int  BMPReverse(int  bmpNo);
int  BMPClear(int  bmpNo);

/*****
 *	W31JEUDC.C
 *****/
int  OpenW31JEUDC(TCHAR  *path);
void  CloseW31JEUDC(void);
int  GetW31JEUDCFont(unsigned short  code, LPBYTE buf,int  bufsiz,int  *xsiz,int  *ysiz, BOOL bUnicode);
int  PutW31JEUDCFont(unsigned short  code, LPBYTE buf,int  xsiz,int  ysiz, BOOL bUniocde);
int  IsWin95EUDCBmp(LPTSTR szBmpPath);
BOOL GetGlyph(TCHAR *path, BYTE* pGlyph);

/*****
 *	SMOOTH.C
 *****/
struct SMOOTHPRM	{
	int	SmoothLevel;
	int	UseConic;
	};

#define		SMOOTHLEVELMAX	8


int  SmoothVector(int  lstHdl,int  tmpLst,int  xinMesh,int yinMesh, int  outMesh,struct  SMOOTHPRM *prm,int  fp);
int  searchanchor(int  sn,struct  VDATA *sp,struct  VDATA * *ep,int  lim);
int  RemoveFp(int  lstHdl,int  outMesh,int  uFp);
int  toTTFFrame(int  lstH,struct  BBX *bbx);
int  SmoothLight(int  ioLst,int  tmpLst,int  width,int height, int  oWidth,int  ufpVal);
int  ConvMesh(int  lstH,int  inMesh,int  outMesh);

/*****
 *	DATAIF.C
 *****/
int  OInit(void);
int  OTerm(void);
#ifdef BUILD_ON_WINNT
int  OExistUserFont( TCHAR*path);
#endif // BUILD_ON_WINNT
int  OExistTTF(  TCHAR *path);
int  OCreateTTF( HDC hDC, TCHAR *path, int fontType);
int  OMakeOutline( UCHAR  *buf,int  siz,int  level);
int  OOutTTF(HDC hDC, TCHAR  *path,unsigned short  code, BOOL bUnicode);
/*****
 *	TTFFILE.C
 *****/
void  smtoi(short  *sval);
void  lmtoi(long  *lval);
void  sitom(short  *sval);
void  litom(long  *lval);
int  TTFReadHdr(HANDLE  fHdl,struct  TTFHeader *hdr);
int  TTFWriteHdr(HANDLE  fHdl,struct  TTFHeader *hdr);
int  TTFReadDirEntry(HANDLE fHdl,struct  TableEntry *entry,int  eCnt);
int  TTFWriteDirEntry(HANDLE  fHdl,struct  TableEntry *entry,int  eCnt);
int  TTFGetTableEntry(HANDLE  fH,struct  TableEntry *entry,char  *tag);
int  TTFReadTable(HANDLE  fH,struct  TableEntry *entry,void  *buf,int  bufsiz);
int  TTFReadFixedTable(HANDLE  fH,char  *buf,int  bufsiz,char  *tag);
int  TTFReadVarTable(HANDLE  fH,char  * *buf,unsigned int  *bufsiz,char  *tag);
int  TTFWriteTable(HANDLE fH,struct  TableEntry *entry,void  *buf,int  bufsiz);
int  TTFAppendTable(HANDLE  fH,struct  TableEntry *entry,void  *buf,int  siz);
int  TTFReadOrgFixedTable(HDC  hDC,char  *buf,int  bufsiz,char  *tag);
int  TTFReadOrgVarTable(HDC  hDC,char  * *buf,unsigned int  *bufsiz,char  *tag);
int  TTFCreate(HDC  hDC,TCHAR  *newf,struct  BBX *bbx,int  lstHdl,int  fontType);
int  TTFGetBBX(HDC  hDC,struct  BBX *bbx,short  *uPEm);
int  TTFTmpPath(TCHAR  *path,TCHAR  *tmpPath);
int  TTFAddEUDCChar(TCHAR *path,unsigned short  code,struct  BBX *bbx,int  lstH);
int  TTFOpen(TCHAR  *path);
int  TTFClose(void);
int  TTFGetEUDCBBX(TCHAR  *path,struct  BBX *bbx,short  *upem);
int  TTFAppend(unsigned short  code,struct  BBX *bbx,int  lsthdl);
int  TTFImpCopy(TCHAR  *sPath,TCHAR  *dPath);
int  TTFImpGlyphCopy(HANDLE  sFh,int  glyphID);
int  TTFImpGlyphWrite(int  glyphID, char *buf, int siz);
int  TTFImpTerm( HANDLE orgFh, int glyphID);
int  TTFLastError( void);
/*
 *	Create.c
 */
int creatW31JEUDC( TCHAR *path);

/*
 *	makepoly.c
 */

int	MkPoly(	int inlst, int outLst);

/*
 *	W31JBMP.C
 */
int  isW31JEUDCBMP( TCHAR *path);
int  OpenW31JBMP(TCHAR  *path,int  omd);
int  CloseW31JBMP(void);
int  GetW31JBMPnRecs( int *nRec, int *nGlyph, int *xsiz, int *ysiz);
int  GetW31JBMP(unsigned short  code,char  *buf,int  bufsiz,int  *xsiz,int  *ysiz);
int  GetW31JBMPRec(int  rec,LPBYTE buf,int  bufsiz,int  *xsiz,int  *ysiz,unsigned short  *code);
int  PutW31JBMPRec(int  rec,LPBYTE buf,int  xsiz,int  ysiz);
int  W31JrecTbl(int  * *recTbl, BOOL bIsWin95EUDC);
int  GetW31JBMPMeshSize( int *xsiz, int *ysiz);

/*
 *	code.c
 */

void  makeUniCodeTbl(void);
unsigned short  sjisToUniEUDC(unsigned short  code);
unsigned short  getMaxUniCode(void);

/*
 *	IMPORT.C
 */

int  Import(TCHAR  *eudcPath,TCHAR *bmpPath,TCHAR *ttfPath,int  oWidth,int  oHeight, int level, BOOL bIsWin95EUDC);

/*
 *	eten.c
 */
int  openETENBMP(TCHAR  *path,int  md);
int  closeETENBMP(void);
int  createETENBMP(TCHAR *path,int  wid,int  hei);
int  getETENBMPInf(int  *nRec, int *nGlyph,int  *wid,int  *hei, char *sign, 
WORD *bID);
int  readETENBMPRec(int  rec, LPBYTE buf,int  bufsiz,unsigned short  *code);
int  appendETENBMP(LPBYTE buf,unsigned short  code);
int  isETENBMP(TCHAR *path);
int  ETENrecTbl(int  * *recTbl);

#ifdef BUILD_ON_WINNT
/*
 * EUDCRANG.CPP  
 */
void    CorrectTrailByteRange(int nIndex);
void	SetTrailByteRange(UINT LocalCP);
#endif // BUILD_ON_WINNT
