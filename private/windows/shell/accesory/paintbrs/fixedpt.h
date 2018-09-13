/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/* define a type for NUM and the base */
typedef long NUM;
extern NUM nmLeft, nmTop;
extern NUM nmRight, nmBottom;

#define BASE 100L

/* rounding options for NumToShort */
#define  NUMFLOOR      0
#define  NUMROUND      (BASE/2)
#define  NUMCEILING    (BASE-1)

/* shortcuts for rounding operations */
#define  FLOOR(x)  NumToShort(x,NUMFLOOR)
#define  ROUND(x)  NumToShort(x,NUMROUND)
#define  CEILING(x)  NumToShort(x,NUMCEILING)

/* Unit conversion */
#define  InchesToCM(x)  (((x) * 254L + 50) / 100)
#define  CMToInches(x)  (((x) * 100L + 127) / 254)

/* converting in/out of fixed point */
#define  ShortToNum(x)   ((x) * BASE)
#define  NumToShort(x,s)   (LOWORD(((x) + (s)) / BASE))
#define  NumRemToShort(x)  (LOWORD((x) % BASE))

/* Pixel unit conversion */
LONG NumToPels(LONG lNum, BOOL bHoriz, BOOL bInches);
LONG PelsToNum(LONG lNum, BOOL bHoriz, BOOL bInches);

/* conversion to/from strings */
BOOL StrToNum(LPTSTR num, LONG FAR *lpNum);
BOOL NumToStr(LPTSTR num, LONG lNum, BOOL bDecimal);

/* routines to deal with fixed point numbers in dialog boxes */
BOOL GetDlgItemNum(HWND hDlg, int nItemID, LONG FAR * lpNum);
BOOL SetDlgItemNum(HWND hDlg, int nItemID, LONG lNum, BOOL bDecimal);

/* Operations on fixed point numbers */
#define ToN(x)	   (NUM)((x) * BASE)
#define ToNND(n,d)	   (NUM)(((n) * BASE) /(d))
#define NegN(x)    (-(x))
#define NAddN(x,y) ((x) + (y))
#define NMulN(x,y) ((x) * (y) / BASE)
#define NDivN(x,y) ((x) * BASE / (y))
#define NAddI(x,y) ((x) + (y) * BASE)
#define NMulI(x,y) ((x) * (y))
#define NDivI(x,y) ((x) / (y))
#define IDivN(x,y) ((x) * (BASE * BASE) / (y))

#define NLTN(x,y)  ((x) < (y))
#define NLEN(x,y)  ((x) <= (y))
#define NEQN(x,y)  ((x) == (y))
#define NGEN(x,y)  ((x) >= (y))
#define NGTN(x,y)  ((x) > (y))

#define NLTI(x,y)  ((x) < ToN(y))
#define NLEI(x,y)  ((x) <= ToN(y))
#define NEQI(x,y)  ((x) == ToN(y))
#define NGEI(x,y)  ((x) >= ToN(y))
#define NGTI(x,y)  ((x) > ToN(y))
