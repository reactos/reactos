#ifndef __idlist_h
#define __idlist_h


/*-----------------------------------------------------------------------------
/ ITEMIDLIST functions
/----------------------------------------------------------------------------*/

// Data packed into an ITEMIDLIST

#define MDIDL_MAGIC             (DWORD)0x03150318

// Flags for IDLIST...

#define MDIDL_ISSPECIALITEM     (DWORD)0x00000001
#define MDIDL_HASICONINFO       (DWORD)0x00000002
#define MDIDL_HASNAME           (DWORD)0x00000004
#define MDIDL_ISJUNCTION        (DWORD)0x00000010
#define MDIDL_HASNOSHELLPIDL    (DWORD)0x00000020
#define MDIDL_HASPATHINFO       (DWORD)0x00000040

#pragma pack(1)

struct _idregitem
{
    WORD    cb;
    BYTE    bFlags;
    BYTE    bReserved;      // This is to get DWORD alignment
    CLSID   clsid;
    WORD    next;
};
typedef struct _idregitem IDREGITEM;
typedef UNALIGNED IDREGITEM* LPIDREGITEM;
#define MYDOCS_SORT_INDEX       0x10
#define SHID_ROOT_REGITEM       0x1f

struct _idfsitem
{
    WORD cb;
    BYTE bFlags;
};
typedef struct _idfsitem IDFSITEM;
typedef UNALIGNED IDFSITEM* LPIDFSITEM;
#define SHID_FS_DIRECTORY         0x31
#define SHID_JUNCTION             0x80
#define SHID_FS_DIRUNICODE        0x35
#pragma pack()



// public routines...

BOOL         MDGetPathFromIDL(    LPITEMIDLIST   pidl, LPTSTR pPath, LPTSTR pRootPath );
BOOL         MDGetFullPathFromIDL(LPITEMIDLIST   pidl, LPTSTR pPath, LPTSTR pRootPath );
HRESULT      MDCreateIDLFromPath( LPITEMIDLIST* ppidl, LPTSTR pPath );
LPITEMIDLIST SHIDLFromMDIDL(      LPCITEMIDLIST  pidl );
LPITEMIDLIST MDCreateSpecialIDL(  LPTSTR pPath,  LPTSTR pName, LPTSTR pIconPath, UINT uIndex );
BOOL         MDIsSpecialIDL(      LPITEMIDLIST   pidl );
BOOL         MDIsJunctionIDL(     LPITEMIDLIST   pidl );
HRESULT      MDGetIconInfoFromIDL(LPITEMIDLIST   pidl, LPTSTR pIconPath, UINT cch, UINT * pIndex );
HRESULT      MDGetNameFromIDL(    LPITEMIDLIST   pidl, LPTSTR pName, UINT * pcch, BOOL fFullPath );
HRESULT      MDGetPathInfoFromIDL(LPITEMIDLIST   pidl, LPTSTR pPath, UINT * pcch );
BOOL         IsIDLRootOfNameSpace(LPITEMIDLIST   pidlIN );
BOOL         IsIDLFSJunction(     LPCITEMIDLIST  pidlIN, DWORD dwAttrb );



HRESULT
MDWrapShellIDLists( LPITEMIDLIST   pidlRoot,
                    UINT           celtIN,
                    LPITEMIDLIST * rgeltIN,
                    LPITEMIDLIST * rgeltOUT
                   );

HRESULT
MDUnWrapMDIDLists( UINT celtIN,
                   LPITEMIDLIST * rgeltIN,
                   LPITEMIDLIST * rgeltOUT
                  );


#endif


