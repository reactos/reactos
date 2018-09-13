//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// cdfidl.h 
//
//   The definition of cdf idlist structures and helper functions.
//
//   History:
//
//       3/19/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _CDFIDL_H_

#define _CDFIDL_H_


//
// CDFITEMID_VERSION is used to corerectly read back persisted id lists
// CDFITEMID_ID is used to identify id lists as cdf id lists.
//

#define CDFITEMID_VERSION   0x00
#define CDFITEMID_ID        0xed071264

//
// Index values for "special" cdf nodes.
//

#define INDEX_CHANNEL_LINK  -1

//
// Types of cdf item id lists.  Note: These valuse are cast as bytes to save
// space in the item id.
//

typedef enum _tagCDFITEMTYPE {
    CDF_Folder      = 0x01,
    CDF_FolderLink  = 0x02,
    CDF_Link        = 0x03
} CDFITEMTYPE;


//
//  The structure of a cdf item id.  The szName parameter is a placeholder
//  for a variable length name string followed by zero or more additional
//  variable length strings. 
//

#pragma pack(1)

typedef struct _tagCDFITEMID
{
    USHORT       cb;
    BYTE         wVersion;
    BYTE         cdfItemType;
    DWORD        dwId;
    LONG         nIndex;
    TCHAR        szName[1];
} CDFITEMID;

#pragma pack()

typedef UNALIGNED CDFITEMID *PCDFITEMID;


typedef struct _tagCDFITEMIDLIST
{
    CDFITEMID mkid;
} CDFITEMIDLIST;

typedef UNALIGNED CDFITEMIDLIST *PCDFITEMIDLIST;

//
// Cdf item data.  Structure containing the unique elements of a cdf item id.
// Its used to create cdf item ids.
//

typedef struct _tagCDFITEM
{
    LONG         nIndex;
    CDFITEMTYPE  cdfItemType;
    BSTR         bstrName;
    BSTR         bstrURL;
} CDFITEM, *PCDFITEM;


//
// Cdf id list function prototypes.
//

PCDFITEMIDLIST CDFIDL_Create(PCDFITEM pCdfItem);

PCDFITEMIDLIST CDFIDL_CreateFromXMLElement(IXMLElement* pIXMLElement,
                                           ULONG nIndex);
PCDFITEMIDLIST CDFIDL_CreateFolderPidl(PCDFITEMIDLIST pcdfidl);

BOOL    CDFIDL_IsUnreadURL(LPTSTR szUrl);
void    CDFIDL_Free(PCDFITEMIDLIST pcdfidl);
HRESULT CDFIDL_GetDisplayName(PCDFITEMIDLIST pcdfidl, LPSTRRET pName);
LPTSTR  CDFIDL_GetName(PCDFITEMIDLIST pcdfidl);
LPTSTR  CDFIDL_GetNameId(PCDFITEMID pcdfid); 
LPTSTR  CDFIDL_GetURL(PCDFITEMIDLIST pcdfidl);
LPTSTR  CDFIDL_GetURLId(PCDFITEMID pcdfid);
ULONG   CDFIDL_GetIndex(PCDFITEMIDLIST pcdfidl);
ULONG   CDFIDL_GetIndexId(PCDFITEMID pcdfid);
BOOL    CDFIDL_IsCachedURL(LPWSTR wszUrl);
ULONG   CDFIDL_GetAttributes(IXMLElementCollection* pIXMLElementCollection,
                             PCDFITEMIDLIST pcdfidl, ULONG fAttributesFilter);

SHORT   CDFIDL_Compare(PCDFITEMIDLIST pcdfidl1, PCDFITEMIDLIST pcdfidl2);
SHORT   CDFIDL_CompareId(PCDFITEMID pcdfid1, PCDFITEMID pcdfid2);
BOOL    CDFIDL_IsValid(PCDFITEMIDLIST pcdfidl);
BOOL    CDFIDL_IsValidId(PCDFITEMID pcdfid);
BOOL    CDFIDL_IsValidSize(PCDFITEMID pcdfid);
BOOL    CDFIDL_IsValidType(PCDFITEMID pcdfid);
BOOL    CDFIDL_IsValidIndex(PCDFITEMID pcdfitemid);
BOOL    CDFIDL_IsValidStrings(PCDFITEMID pcdfitemid);
BOOL    CDFIDL_IsFolder(PCDFITEMIDLIST pcdfidl);
BOOL    CDFIDL_IsFolderId(PCDFITEMID pcdfid);
HRESULT CDFIDL_NonCdfGetDisplayName(LPCITEMIDLIST pidl, LPSTRRET pName);


#ifdef UNIX
#define ALIGN4(sz) (((sz)+3)&~3)
#endif /* UNIX */

#endif // _CDFIDL_H_
