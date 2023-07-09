// ############################################################################
// PROTOTYPES
LPSTR GetSz(WORD wszID);
void SzCanonicalFromAE (LPSTR psz, PACCESSENTRY pAE, LPLINECOUNTRYENTRY pLCE);
int __cdecl CompareIDLookUpElements(const void *e1, const void *e2);
int __cdecl CompareCntryNameLookUpElements(const void*e1, const void*e2);
int __cdecl CompareIdxLookUpElements(const void*e1, const void*e2);
int __cdecl CompareIdxLookUpElementsFileOrder(const void *pv1, const void *pv2);
int __cdecl Compare950Entry(const void*e1, const void*e2);
BOOL FSz2Dw(LPCSTR pSz,DWORD far *dw);
BOOL FSz2W(LPCSTR pSz,WORD far *w);
BOOL FSz2B(LPCSTR pSz,BYTE far *pb);
HRESULT MakeBold (HWND hwnd);
HRESULT ReleaseBold(HWND hwnd);
#if !defined(WIN16)
DWORD DWGetWin32Platform();
DWORD DWGetWin32BuildNumber();
#endif
/*
inline BOOL FSz2Dw(PCSTR pSz,DWORD *dw);
inline BOOL FSz2W(PCSTR pSz,WORD *w);
inline BOOL FSz2B(PCSTR pSz,BYTE *pb);
*/
