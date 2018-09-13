#ifndef __DEFPROP_H__
#define __DEFPROP_H__

#include "PTsrv32.h"

//-------------------------------------//
//  Helper functions
EXTERN_C HRESULT LoadDefItemTags( UINT nIDS, BSTR& bstrName, BSTR& bstrDisplayName, BSTR& bstrQtip ) ;


//------------------------------------------//
//  Advanced properties default folder items
//------------------------------------------//

typedef struct tagDEFFOLDERITEM
{
    const PFID* ppfid ;
    UINT        nIDStringRes ;
    ULONG       dwSrcType ;
	ULONG		mask ;
	ULONG		dwAccess ;
    ULONG       Reserved ;
} DEFFOLDERITEM, *PDEFFOLDERITEM, *LPDEFFOLDERITEM ;

//  Folder item API
EXTERN_C HRESULT MakeDefFolderItem( long iDefFolder, PROPFOLDERITEM* pItem, LPARAM lParamSrc ) ;
EXTERN_C BOOL    IsDefFolderPFID( long iDefFolder, REFPFID refpfid ) ;
EXTERN_C BOOL    IsDefFolderStgType( long iDefFolder, ULONG dwSrcType ) ;
EXTERN_C BOOL    IsDefFolderDocType( long iDefFolder, ULONG dwSrcType ) ;
EXTERN_C BOOL    IsDefFolderSrcType( long iDefFolder, ULONG dwSrcType ) ;
EXTERN_C int     DefFolderCount() ;

//-------------------------------------//
//  Advanced properties default value items
//-------------------------------------//
// Important: the header portion of this structure
// must match the PROPVARIANT structure for cross-casting purposes.
typedef struct tagDEFVAL
{
    VARTYPE     vt ;                
    WORD wReserved1;
    WORD wReserved2;
    WORD wReserved3;
    union       {
        //  some token union members.
        short               i2;
        UCHAR               u1;
        unsigned short      u2;
        unsigned long       u4;
        double              d ;
        unsigned __int64    ull ;
        PVOID               pv ;
        CAUL                caul ;
    } val ;
    BOOL        fValStrRes  ;    // TRUE if u2 represents a string resource ID for a string type, otherwise FALSE.
    BOOL        fDisplayStrRes ; // TRUE: lpDisplay = sring res ID, otherwise lpDisplay = string literal.
    LPARAM      lpDisplay ;
} DEFVAL, *PDEFVAL, *LPDEFVAL;

//-------------------------------------//
//  Advanced properties default property items
//-------------------------------------//
typedef struct tagDEFPROPERTYITEM
{
    LPWSTR       pszName ;
    const FMTID* pFmtID ;
    PROPID       propID ;
    ULONG        nIDStringRes ;
	VARTYPE		 vt ;
    ULONG        dwSrcType ;
    const PFID*  ppfid ;
    ULONG        mask ;
    ULONG        dwAccess ;
    ULONG		 dwFlags ;
	ULONG		 ctlID ;
	BSTR		 bstrDisplayFmt ;
    ULONG        cDefVals ;
    const DEFVAL*      pDefVals ;
} DEFPROPERTYITEM, *PDEFPROPERTYITEM, *LPDEFPROPERTYITEM ; 

//-------------------------------------//
//  Advanced properties default value list.
//  We classify property sources in terms of the complement of properties
//  displayed for them:
//  Bits 0-15: document type
//  Bits 16-23: storage type
//  Bits 24-31: context
#define DOCTYPEMASK        0x0000FFFF
#define STGTYPEMASK        0x00FF0000
#define CXTTYPEMASK        0xFF000000


#define PST_UNSUPPORTED    0x00000000

#define PST_PROPTREE       0x01000000
#define PST_SHELLCOLUMN    0x02000000

#define PST_NOSTG          0x00010000  // No property storage backing the file.
#define PST_OLESS          0x00020000  // OLE compound file property set storage.
#define PST_NSS            0x00040000  // NTFS 5.0+ property set storage.

#define PST_UNKNOWNDOC     0x00000001  // Unknown document type
#define PST_DOCFILE        0x00000002  // Registered OLE document type.
#define PST_IMAGEFILE      0x00000004  // Image file
#define PST_MEDIAFILE      0x00000008  // avi, wav.

#define GETCXTTYPE( srctype )       ((srctype) & CXTTYPEMASK)
#define GETSTGTYPE( srctype )       ((srctype) & STGTYPEMASK)
#define GETDOCTYPE( srctype )       ((srctype) & DOCTYPEMASK)
#define SETSTGTYPE( pST, stgtype )  (*(pST) = (*(pST) & ~STGTYPEMASK)|(stgtype))
#define SETDOCTYPE( pST, doctype )  (*(pST) = (*(pST) & ~DOCTYPEMASK)|(doctype))
#define SETCXTTYPE( pST, cxttype )  (*(pST) = (*(pST) & ~CXTTYPEMASK)|(cxttype))

//  Property item API
EXTERN_C long    DefPropCount() ;
EXTERN_C BOOL    IsDefPropStgType( long iDefFolder, ULONG dwStgType ) ;
EXTERN_C BOOL    IsDefPropDocType( long iDefFolder, ULONG dwDocType ) ;
EXTERN_C BOOL    IsDefPropCxtType( long iDefFolder, ULONG dwCxtType ) ;
EXTERN_C BOOL    IsDefPropSrcType( long iDefFolder, ULONG dwSrcType ) ;
EXTERN_C BOOL    DefPropHasFlags( long iDefProp, ULONG dwFlags ) ;
EXTERN_C long    FindDefPropertyItem( IN REFFMTID fmtID, IN PROPID propID, IN VARTYPE vt ) ;
EXTERN_C long    FindDefPropertyItemLite( IN REFFMTID fmtID, IN PROPID propID ) ;
EXTERN_C HRESULT GetDefPropertyItem( IN long iDefProp, OUT DEFPROPERTYITEM* pItem ) ;
EXTERN_C HRESULT MakeDefPropertyItem( IN long iDefProp, OUT PROPERTYITEM* pItem, IN LPARAM lParam ) ;
EXTERN_C HRESULT MakeDefPropertyItemEx( IN REFFMTID fmtID, IN PROPID propID, IN VARTYPE vt, OUT PROPERTYITEM* pItem, IN LPARAM lParam ) ;
EXTERN_C HRESULT GetDefPropItemID( IN long iDefProp, OUT const FMTID** pFmtID, OUT PROPID* pPropID, OUT VARTYPE* pVt ) ;
EXTERN_C HRESULT GetDefPropItemValues( IN long iDefProp, OUT DEFVAL const** paDefVals, OUT ULONG* pcDefVals ) ;

#define MAX_STRINGRES 255

//-------------------------------------------------------------------------//
//  Implementation helpers
#define BEGIN_DEFFOLDER_MAP( mapname ) \
    static const DEFFOLDERITEM mapname[] = {
#define DEFFOLDER_ENTRY( pfid, strResID, dwSrcType, mask, access ) \
    { &pfid, strResID, dwSrcType, mask, access, 0L },
#define END_DEFFOLDER_MAP() \
    } ;
#define DEFFOLDER_ENTRY_COUNT( mapname )\
    (sizeof(mapname)/sizeof(DEFFOLDERITEM))

#define BEGIN_DEFPROP_MAP( mapname ) \
    static const DEFPROPERTYITEM mapname[] = {
#define DEFPROP_ENTRY( name, fmtid, propid, strResID, type, srctype, pfid, mask, access, flags, ctlID, fmt ) \
    { name, &fmtid, propid, strResID, type, srctype, &pfid, mask, access, flags, ctlID, fmt, 0, 0 },
#define DEFPROP_ENUM_ENTRY( name, fmtid, propid, strResID, type, srctype, pfid, mask, access, flags, ctlID, fmt, cVals, vals ) \
    { name, &fmtid, propid, strResID, type, srctype, &pfid, mask, access, flags|PTPIF_ENUM, PTCTLID_DROPLIST_COMBO, fmt, cVals, vals },
#define END_DEFPROP_MAP() \
    } ;
#define DEFPROP_ENTRY_COUNT( mapname )\
    (sizeof(mapname)/sizeof(DEFPROPERTYITEM))
#define VALID_DEFPROP_ENTRY( i, mapname ) \
    ((i)>=0 && (i)< DEFPROP_ENTRY_COUNT( mapname ))

#define BEGIN_DEFVALLIST( name ) \
    static const DEFVAL name[] = { 
#define DEFVALLIST_STRING_ENTRY( val, vt, fValStrRes, fDisplStrRes, display ) \
    { vt, 0,0,0, val, fValStrRes, fDisplStrRes, display },
#define DEFVALLIST_BOOL_ENTRY( val, fDisplStrRes, display ) \
    { VT_BOOL, 0,0,0, val, FALSE, fDisplStrRes, display },
#define DEFVALLIST_COUNT( name )\
    (sizeof(name)/sizeof(DEFVAL))
#define END_DEFVALLIST()    } ;

 



#endif __DEFPROP_H__
