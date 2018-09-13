#ifndef _OFFGLUE_H_
#define _OFFGLUE_H_
#define fTrue   TRUE
#define fFalse FALSE
#define MsoImageList_Create ImageList_Create
#define MsoImageList_Destroy ImageList_Destroy
#define MsoImageList_ReplaceIcon ImageList_ReplaceIcon
#define InvalidateVBAObjects(x,y,z)

typedef struct _num
{
    CHAR    rgb[8];
} NUM;

typedef struct _ulargeint
   {
      union
      {
         struct
         {
            DWORD dw;
            DWORD dwh;
         };
         struct
         {
            WORD w0;
            WORD w1;
            WORD w2;
            WORD w3;

         };
      };
   } ULInt;


// Macro to release a COM interface
#define RELEASEINTERFACE( punk )            \
        if( punk != NULL )                  \
        {                                   \
            (punk)->lpVtbl->Release(punk);  \
            punk = NULL;                    \
        }

// Determine the elements in a fixed-sized vector
#define NUM_ELEMENTS( vector ) ( sizeof(vector) / sizeof( (vector)[0] ) )


#ifdef __cplusplus
extern TEXT("C") {
#endif // __cplusplus
//Wrapper functions to the client supplied mem alloc and free
void *PvMemAlloc(DWORD cb);
void VFreeMemP(void *pv, DWORD cb);
void *PvMemRealloc(void *pv, DWORD cbOld, DWORD cbNew);
void *PvStrDup(LPCTSTR p);
int CchGetString();

// Function to convert a ULInt to an sz without leading zero's
// Returns cch -- not including zero-terminator
WORD CchULIntToSz(ULInt, TCHAR *, WORD );
int CchTszLen(const TCHAR *psz);
int CchWszLen(const WCHAR *psz);
int CchAnsiSzLen(const CHAR *psz);
VOID FillBuf(void *p, unsigned w, unsigned cb);

// Function to scan memory for a given value
BOOL FScanMem(LPBYTE pb, byte bVal, DWORD cb);

BYTE *PbMemCopy(void *pvDst, const void *pvSrc, unsigned cb);
VOID SzCopy(void *pszDst, const void *pszSrc);
BYTE *PbSzNCopy(void *pvDst, const void *pvSrc, unsigned cb);
BOOL FFreeAndCloseisdbhead();
//Displays an alert using the give ids
int IdDoAlert(HWND, int ids, int mb);

//  Wide-Char - MBCS helpers
LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars);
LPSTR WINAPI W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars);

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus


//  Wide-Char - MBCS helpers
#define USES_CONVERSION int _convert = 0

#ifndef A2WHELPER
#define A2WHELPER A2WHelper
#define W2AHELPER W2AHelper
#endif
#define A2W(lpa) (\
	((LPCSTR)lpa == NULL) ? NULL : ( _convert = (lstrlenA(lpa)+1),\
		A2WHELPER((LPWSTR) alloca(_convert*2), lpa, _convert)))
#define W2A(lpw) (\
	((LPCWSTR)lpw == NULL) ? NULL : ( _convert = (lstrlenW(lpw)+1)*2,\
		W2AHELPER((LPSTR) alloca(_convert), lpw, _convert)))
#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))

#ifdef _UNICODE
	#define T2A W2A
	#define A2T A2W
    #define T2W(lp)     lp
	#define W2T(lp)     lp
	#define T2CA W2CA
	#define A2CT A2CW
	#define T2CW(lp)    lp
	#define W2CT(lp)    lp
#else
	#define T2W A2W
	#define W2T W2A
	#define T2A(lp)     lp
	#define A2T(lp)     lp
	#define T2CW A2CW
	#define W2CT W2CA
	#define T2CA(lp)    lp
	#define A2CT(lp)    lp
#endif // _UNICODE

#endif // _OFFGLUE_H_
