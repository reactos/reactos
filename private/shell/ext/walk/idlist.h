LPITEMIDLIST  ILGetNext(LPCITEMIDLIST pidl);
UINT          ILGetSize(LPCITEMIDLIST pidl);
LPITEMIDLIST  ILCreate(void);
LPITEMIDLIST  ILCreateFromPath(LPCSTR szPath);
BOOL 	      ILGetDisplayName(LPCITEMIDLIST pidl, LPSTR pszName);
LPITEMIDLIST  ILFindLastID(LPCITEMIDLIST pidl);
BOOL          ILRemoveLastID(LPITEMIDLIST pidl);
LPITEMIDLIST  ILClone(LPCITEMIDLIST pidl);
LPITEMIDLIST  ILCloneFirst(LPCITEMIDLIST pidl);
BOOL          ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
BOOL          ILIsEqualItemID(LPCSHITEMID pmkid1, LPCSHITEMID pmkid2);
BOOL          ILIsParent(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fImmediate);
LPITEMIDLIST  ILFindChild(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild);
LPITEMIDLIST  ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
LPITEMIDLIST  ILCreateFromPath(LPCTSTR szPath);

// LPITEMIDLIST  ILAppendID(LPITEMIDLIST pidl, LPCSHITEMID pmkid, BOOL fAppend);

STDAPI ILLoadFromStream(IStream *pstm, LPITEMIDLIST *pidl);
STDAPI ILSaveToStream(IStream *pstm, LPCITEMIDLIST pidl);

// helper macros
#define ILIsEmpty(pidl)	((pidl)->mkid.cb == 0)
#define ILCreateFromID(pmkid)   ILAppendID(NULL, pmkid, TRUE)

// unsafe macros
#define _ILSkip(pidl, cb)	((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)		_ILSkip(pidl, (pidl)->mkid.cb)

#ifdef _HIDA

typedef HGLOBAL HIDA;

HIDA HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl);
void HIDA_Free(HIDA hida);
HIDA HIDA_Clone(HIDA hida);
UINT HIDA_GetCount(HIDA hida);
UINT HIDA_GetIDList(HIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax);

LPCITEMIDLIST HIDA_GetIDListPtr(HIDA hida, UINT i);
LPITEMIDLIST  HIDA_ILClone(HIDA hida, UINT i);
LPITEMIDLIST  IDA_ILClone(LPIDA pida, UINT i);
LPITEMIDLIST  HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl);

#define HIDA_Free(HIDA hida) GlobalFree(hida)

#endif _HIDA


#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#define SHAnsiToUnicode(psz, pwsz, cchwsz)  MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchwsz);
#define SHUnicodeToAnsi(pwsz, psz, cchsz)   WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, cchsz, NULL, NULL);

#ifdef UNICODE
#define SHTCharToUnicode(wzSrc, wzDest, cchSize)                SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHTCharToUnicodeCP(uiCP, wzSrc, wzDest, cchSize)        SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHTCharToAnsi(wzSrc, szDest, cchSize)                   SHUnicodeToAnsi(wzSrc, szDest, cchSize)
#define SHTCharToAnsiCP(uiCP, wzSrc, szDest, cchSize)           SHUnicodeToAnsiCP(uiCP, wzSrc, szDest, cchSize)
#define SHUnicodeToTChar(wzSrc, wzDest, cchSize)                SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHUnicodeToTCharCP(uiCP, wzSrc, wzDest, cchSize)        SHUnicodeToUnicode(wzSrc, wzDest, cchSize)
#define SHAnsiToTChar(szSrc, wzDest, cchSize)                   SHAnsiToUnicode(szSrc, wzDest, cchSize)
#define SHAnsiToTCharCP(uiCP, szSrc, wzDest, cchSize)           SHAnsiToUnicodeCP(uiCP, szSrc, wzDest, cchSize)
#else // UNICODE
#define SHTCharToUnicode(szSrc, wzDest, cchSize)                SHAnsiToUnicode(szSrc, wzDest, cchSize)
#define SHTCharToUnicodeCP(uiCP, szSrc, wzDest, cchSize)        SHAnsiToUnicodeCP(uiCP, szSrc, wzDest, cchSize)
#define SHTCharToAnsi(szSrc, szDest, cchSize)                   SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHTCharToAnsiCP(uiCP, szSrc, szDest, cchSize)           SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHUnicodeToTChar(wzSrc, szDest, cchSize)                SHUnicodeToAnsi(wzSrc, szDest, cchSize)
#define SHUnicodeToTCharCP(uiCP, wzSrc, szDest, cchSize)        SHUnicodeToAnsiCP(uiCP, wzSrc, szDest, cchSize)
#define SHAnsiToTChar(szSrc, szDest, cchSize)                   SHAnsiToAnsi(szSrc, szDest, cchSize)
#define SHAnsiToTCharCP(uiCP, szSrc, szDest, cchSize)           SHAnsiToAnsi(szSrc, szDest, cchSize)
#endif // UNICODE
