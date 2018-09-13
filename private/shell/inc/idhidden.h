#ifndef _IDHIDDEN_H_
#define _IDHIDDEN_H_

//
//  internal APIs for adding Hidden IDs to pidls
//      we use this to add data that we dont want
//      to be noticed by normal namespace handlers
//

typedef enum
{
    IDLHID_EMPTY            = 0xBEEF0000,   //  where's the BEEF?!
    IDLHID_URLFRAGMENT,                     //  Fragment IDs on URLs (#anchors)
    IDLHID_URLQUERY,                        //  Query strings on URLs (?query+info)
    IDLHID_JUNCTION,                        //  Junction point data
    IDLHID_LOCALIZEDNAME,                   //  for using ILocalizedName on folders
    IDLHID_DOCFINDDATA,                     //  DocFind's private attached data (not persisted)
} IDLHID;

typedef struct _HIDDENITEMID
{
    WORD    cb;     //  hidden item size
    IDLHID  id;     //  hidden item ID
    BYTE    ab[1];  //  hidden item data
} HIDDENITEMID;

typedef UNALIGNED HIDDENITEMID *PIDHIDDEN;
typedef const UNALIGNED HIDDENITEMID *PCIDHIDDEN;

STDAPI_(LPITEMIDLIST) ILAppendHiddenID(LPITEMIDLIST pidl, PCIDHIDDEN pidhid);
STDAPI_(PCIDHIDDEN) ILFindHiddenID(LPCITEMIDLIST pidl, IDLHID id);
STDAPI_(BOOL) ILRemoveHiddenID(LPITEMIDLIST pidl, IDLHID id);

//  helpers for common data types.
STDAPI_(LPITEMIDLIST) ILAppendHiddenClsid(LPITEMIDLIST pidl, IDLHID id, CLSID *pclsid);
STDAPI_(BOOL) ILGetHiddenClsid(LPCITEMIDLIST pidl, IDLHID id, CLSID *pclsid);
STDAPI_(LPITEMIDLIST) ILAppendHiddenStringW(LPITEMIDLIST pidl, IDLHID id, LPCWSTR psz);
STDAPI_(LPITEMIDLIST) ILAppendHiddenStringA(LPITEMIDLIST pidl, IDLHID id, LPCSTR psz);
STDAPI_(BOOL) ILGetHiddenStringW(LPCITEMIDLIST pidl, IDLHID id, LPWSTR psz, DWORD cch);
STDAPI_(BOOL) ILGetHiddenStringA(LPCITEMIDLIST pidl, IDLHID id, LPSTR psz, DWORD cch);
STDAPI_(int) ILCompareHiddenString(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, IDLHID id);

#ifdef UNICODE
#define ILAppendHiddenString            ILAppendHiddenStringW
#define ILGetHiddenString               ILGetHiddenStringW
#else //!UNICODE
#define ILAppendHiddenString            ILAppendHiddenStringA
#define ILGetHiddenString               ILGetHiddenStringA
#endif //UNICODE

#endif // _IDHIDDEN_H_

