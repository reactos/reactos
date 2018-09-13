LPITEMIDLIST  ILGetNext(LPCITEMIDLIST pidl);
UINT          ILGetSize(LPCITEMIDLIST pidl);
LPITEMIDLIST  ILCreate(void);
void          ILFree(LPITEMIDLIST pidl);
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
HRESULT       ILLoadFromStream(LPSTREAM pstm, LPITEMIDLIST *pidl);
HRESULT       ILSaveToStream(LPSTREAM pstm, LPCITEMIDLIST pidl);

LPITEMIDLIST  ILCreateFromPath(LPCSTR szPath);

// LPITEMIDLIST  ILAppendID(LPITEMIDLIST pidl, LPCSHITEMID pmkid, BOOL fAppend);

BOOL StrRetToStrN(LPSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl);

// helper macros
#define ILIsEmpty(pidl)	((pidl)->mkid.cb==0)
#define ILCreateFromID(pmkid)   ILAppendID(NULL, pmkid, TRUE)

// unsafe macros
#define _ILSkip(pidl, cb)	((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)		_ILSkip(pidl, (pidl)->mkid.cb)

#ifdef _HIDA
//===========================================================================
// HIDA -- IDList Array handle
//===========================================================================

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

