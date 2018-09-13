#ifndef DPASTUFF_H_
#define DPASTUFF_H_

typedef struct
{
    DWORD           dwSortBy;           // the sort by flag.
    IShellFolder    *psf;               // shell folder to be ordered.

    // Caller of OrderList_Merge does *not* fill in this field.
    // This field is used internally by OrderList_Merge.
    IShellFolder2   *psf2;              // IShellFolder2 version of psf
    LPARAM          lParam;             // Other user data...

} ORDERINFO, * PORDERINFO;

// see shellp.h for ORDERITEM definition
typedef void (*LPFNORDERMERGENOMATCH)(LPVOID pvParam, LPCITEMIDLIST pidl);

int CALLBACK OrderItem_Compare(LPVOID pv1, LPVOID pv2, LPARAM lParam);
LPVOID CALLBACK OrderItem_Merge(UINT uMsg, LPVOID pvDest, LPVOID pvSrc, LPARAM lParam);
void OrderList_Merge(HDPA hdpaNew, HDPA hdpaOld, int iInsertPos, LPARAM lParam, 
                     LPFNORDERMERGENOMATCH pfn, LPVOID pvParam);
void OrderList_Reorder(HDPA hdpa);
HDPA OrderList_Clone(HDPA hdpa);
PORDERITEM OrderItem_Create(LPITEMIDLIST pidl, int nOrder);
void OrderList_Destroy(HDPA *hdpa, BOOL fKillPidls = TRUE);
int OrderItem_GetSystemImageListIndex(PORDERITEM poi, IShellFolder *psf, BOOL fUseCache);
DWORD OrderItem_GetFlags(PORDERITEM poi);
void OrderItem_SetFlags(PORDERITEM poi, DWORD dwFlags);
HRESULT OrderList_SaveToStream(IStream* pstm, HDPA hdpa, IShellFolder * psf);
HRESULT OrderList_LoadFromStream(IStream* pstm, HDPA * phdpa, IShellFolder * psfParent);
void OrderItem_Free(PORDERITEM poi, BOOL fKillPidls = TRUE);
BOOL OrderList_Append(HDPA hdpa, LPITEMIDLIST pidl, int nOrder);

HRESULT COrderList_GetOrderList(HDPA * phdpa, LPCITEMIDLIST pidl, IShellFolder * psf);
HRESULT COrderList_SetOrderList(HDPA hdpa, LPCITEMIDLIST pidl, IShellFolder *psf);

#endif
