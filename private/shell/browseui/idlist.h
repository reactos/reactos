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
LPITEMIDLIST  HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl);

#define HIDA_Free(HIDA hida) GlobalFree(hida)

#endif _HIDA

