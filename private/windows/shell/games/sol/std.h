#include <memory.h>


typedef INT X;
typedef INT Y;
typedef INT DX;
typedef INT DY;


#define fTrue  1
#define fFalse 0

/* PoinT structure */
typedef struct _pt
{
    X x;
    Y y;
} PT;



/* DEL structure */
typedef struct _del
{
    DX dx;
    DY dy;
} DEL;


/* ReCt structure  */
typedef struct _rc
{
    X xLeft;
    Y yTop;
    X xRight;
    Y yBot;
} RC;


#ifdef DEBUG
#define VSZASSERT static TCHAR *vszAssert = TEXT(__FILE__);
#define Assert(f) { if (!(f)) { AssertFailed(vszAssert, __LINE__); } }
#define SideAssert(f) { if (!(f)) { AssertFailed(vszAssert, __LINE__); } }
#else
#define Assert(f)
#define SideAssert(f) (f)
#define VSZASSERT
#endif



VOID *PAlloc(INT cb);
VOID FreeP( VOID * );

INT CchString();
TCHAR *PszCopy(TCHAR *pszFrom, TCHAR *rgchTo);
INT CchDecodeInt(TCHAR *rgch, INT_PTR w);
VOID Error(TCHAR *sz);
VOID ErrorIds(INT ids);
INT WMin(INT w1, INT w2);
INT WMax(INT w1, INT w2);
// INT WParseLpch(TCHAR[ 	]*FAR[ 	]***plpch);
BOOL FInRange(INT w, INT wFirst, INT wLast);
INT PegRange(INT w, INT wFirst, INT wLast);
VOID NYI( VOID );
INT CchString(TCHAR *sz, INT ids);
VOID InvertRc(RC *prc);
VOID OffsetPt(PT *ppt, DEL *pdel, PT *pptDest);
BOOL FRectAllVisible(HDC hdc, RC *prc);

// Removed so it will build on NT...<chriswil>
//
// INT APIENTRY MulDiv( INT, INT, INT );


#ifdef DEBUG
VOID AssertFailed(TCHAR *szFile, INT li);
#endif

#define bltb(pb1, pb2, cb) memcpy(pb2, pb1, cb)


extern HWND hwndApp;
extern HANDLE hinstApp;



BOOL FWriteIniString(INT idsTopic, INT idsItem, TCHAR *szValue);
BOOL FWriteIniInt(INT idsTopic, INT idsItem, WORD2DWORD w);
BOOL FGetIniString(INT idsTopic, INT idsItem, TCHAR *sz, TCHAR *szDefault, INT cchMax);
WORD2DWORD GetIniInt(INT idsTopic, INT idsItem, INT wDefault);



VOID CrdRcFromPt(PT *ppt, RC *prc);
