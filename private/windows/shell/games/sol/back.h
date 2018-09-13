
typedef BITMAPINFOHEADER BMP;

#define DyBmp(bmp) ((int) bmp.biHeight)
#define DxBmp(bmp) ((int) bmp.biWidth)
#define CplnBmp(bmp) 1
#define OfsBits(bgnd) (bgnd.dwOfsBits)
#define CbLine(bgnd) (bgnd.cbLine)

typedef BITMAPFILEHEADER BMPHDR;

typedef struct _bgnd
{
	PT ptOrg;
	OFSTRUCT of;
	BMP bm;
	/* must folow a bm  */
	BYTE rgRGB[64];  /* bug: wont work with >16 color bmps  */
	INT cbLine;
	LONG dwOfsBits;
	BOOL fUseBitmap;
	DY dyBand;
	INT ibndMac;
	HANDLE *rghbnd;
} BGND;


/* PUBLIC routines */

BOOL FInitBgnd(TCHAR *szFile);
BOOL FDestroyBgnd();
BOOL FGetBgndFile(TCHAR *sz);
VOID DrawBgnd(X xLeft, Y yTop, X xRight, Y yBot);
VOID SetBgndOrg();



/* Macros */

extern BGND bgnd;

#define FUseBitmapBgnd() (bgnd.fUseBitmap)


#define BFT_BITMAP 0x4d42   /* 'BM' */
#define ISDIB(bft) ((bft) == BFT_BITMAP)
#define WIDTHBYTES(i)   ((i+31)/32*4)      /* ULONG aligned ! */
WORD DibNumColors(VOID FAR * pv);
