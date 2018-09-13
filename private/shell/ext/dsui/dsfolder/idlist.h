#ifndef __idlist_h
#define __idlist_h


/*-----------------------------------------------------------------------------
/ ITEMIDLIST functions
/----------------------------------------------------------------------------*/

// Data packed into an ITEMIDLIST

#define DSIDL_MAGIC             (DWORD)0x217a6944

#define DSIDL_ISCONTAINER       0x00000001      // = 1 => is a container, eg. folder
#define DSIDL_ISRELATIVE        0x00000002      // = 1 => pPathElement is relative

#define DSIDL_HASNAME           0x20000000      // = 1 => IDLIST has name
#define DSIDL_HASUNC            0x40000000      // = 1 => IDLIST element has UNC
#define DSIDL_HASCLASS          0x80000000      // = 1 => IDLIST element has object class

typedef struct
{
    DWORD  dwFlags;
    DWORD  dwProviderFlags;
    LPWSTR pName;                               // object name
    LPWSTR pObjectClass;                        // object class
    LPWSTR pPathElement;                        // relative, or absolute path element
    WCHAR  szObjectClass[MAX_PATH];
    LPWSTR pUNC;                                // UNC name of folder shortcut
} IDLISTDATA, * LPIDLISTDATA;

// Helper functions

HRESULT CreateIdList(LPITEMIDLIST* ppidl, LPIDLISTDATA pData, IMalloc* pMalloc);
HRESULT CreateIdListFromPath(LPITEMIDLIST* ppidl, LPWSTR pName, LPWSTR pPath, LPWSTR pObjectClass, LPWSTR pUNC, IMalloc *pm, IDsDisplaySpecifier *pdds);
HRESULT UnpackIdList(LPCITEMIDLIST pidl, DWORD dwFlags, LPIDLISTDATA pData);
ULONG   AttributesFromIdList(LPIDLISTDATA pData);

HRESULT PathFromIdList(LPCITEMIDLIST pidl, LPWSTR* ppPath, IADsPathname* pPathname);
HRESULT NameFromIdList(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlParent, LPWSTR pName, INT cchName, IADsPathname* pPathname);


#endif
