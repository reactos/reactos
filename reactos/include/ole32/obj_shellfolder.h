/*
 * Defines the COM interfaces and APIs related to IShellFolder
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_SHELLFOLDER_H
#define __WINE_WINE_OBJ_SHELLFOLDER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/****************************************************************************
*  STRRET
*/
#define	STRRET_WSTR	0x0000
#define STRRET_ASTR	0x0003

#define	STRRET_OFFSETA	0x0001
#define STRRET_OFFSETW	0x0004
#define STRRET_OFFSET WINELIB_NAME_AW(STRRET_OFFSET) 

#define	STRRET_CSTRA	0x0002
#define STRRET_CSTRW	0x0005
#define STRRET_CSTR WINELIB_NAME_AW(STRRET_CSTR) 

typedef struct _STRRET
{ UINT uType;		/* STRRET_xxx */
  union
  { LPWSTR	pOleStr;	/* OLESTR that will be freed */
    LPSTR	pStr;
    UINT	uOffset;	/* OffsetINT32o SHITEMID (ANSI) */
    char	cStr[MAX_PATH];	/* Buffer to fill in */
    WCHAR	cStrW[MAX_PATH];
  } DUMMYUNIONNAME;
} STRRET,*LPSTRRET;

/*****************************************************************************
 * Predeclare the interfaces
 */
typedef struct IShellFolder IShellFolder, *LPSHELLFOLDER;

typedef struct IPersistFolder IPersistFolder, *LPPERSISTFOLDER;

DEFINE_GUID(IID_IPersistFolder2, 0x1ac3d9f0L, 0x175C, 0x11D1, 0x95, 0xBE, 0x00, 0x60, 0x97, 0x97, 0xEA, 0x4F);
typedef struct IPersistFolder2 IPersistFolder2, *LPPERSISTFOLDER2;

DEFINE_GUID(IID_IShellFolder2,  0xB82C5AA8, 0xA41B, 0x11D2, 0xBE, 0x32, 0x0, 0xc0, 0x4F, 0xB9, 0x36, 0x61);
typedef struct IShellFolder2 IShellFolder2, *LPSHELLFOLDER2;

DEFINE_GUID(IID_IEnumExtraSearch,  0xE700BE1, 0x9DB6, 0x11D1, 0xA1, 0xCE, 0x0, 0xc0, 0x4F, 0xD7, 0x5D, 0x13); 
typedef struct IEnumExtraSearch IEnumExtraSearch, *LPENUMEXTRASEARCH;

/*****************************************************************************
 * IEnumExtraSearch interface
 */

typedef struct
{
  GUID	guidSearch;
  WCHAR wszFriendlyName[80];
  WCHAR	wszMenuText[80];
  WCHAR wszHelpText[MAX_PATH];
  WCHAR wszUrl[2084];
  WCHAR wszIcon[MAX_PATH+10];
  WCHAR wszGreyIcon[MAX_PATH+10];
  WCHAR wszClrIcon[MAX_PATH+10];
} EXTRASEARCH,* LPEXTRASEARCH;

#define ICOM_INTERFACE IEnumExtraSearch
#define IEnumExtraSearch_METHODS \
    ICOM_METHOD3(HRESULT, Next, ULONG, celt, LPEXTRASEARCH*, rgelt, ULONG*, pceltFetched) \
    ICOM_METHOD1(HRESULT, Skip, ULONG, celt) \
    ICOM_METHOD (HRESULT, Reset) \
    ICOM_METHOD1(HRESULT, Clone, IEnumExtraSearch**, ppenum)
#define IEnumExtraSearch_IMETHODS \
    IUnknown_IMETHODS \
    IEnumExtraSearch_METHODS
ICOM_DEFINE(IEnumExtraSearch,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumIDList_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumIDList_AddRef(p)			ICOM_CALL (AddRef,p)
#define IEnumIDList_Release(p)			ICOM_CALL (Release,p)
/*** IEnumIDList methods ***/
#define IEnumIDList_Next(p,a,b,c)		ICOM_CALL3(Next,p,a,b,c)
#define IEnumIDList_Skip(p,a)			ICOM_CALL1(Skip,p,a)
#define IEnumIDList_Reset(p)			ICOM_CALL(Reset,p)
#define IEnumIDList_Clone(p,a)			ICOM_CALL1(Clone,p,a)

/*****************************************************************************
 * IShellFolder::GetDisplayNameOf/SetNameOf uFlags 
 */
typedef enum
{	SHGDN_NORMAL		= 0,		/* default (display purpose) */
	SHGDN_INFOLDER		= 1,		/* displayed under a folder (relative)*/
	SHGDN_FORPARSING	= 0x8000	/* for ParseDisplayName or path */
} SHGNO;

/*****************************************************************************
 * IShellFolder::EnumObjects 
 */
typedef enum tagSHCONTF
{	SHCONTF_FOLDERS		= 32,	/* for shell browser */
	SHCONTF_NONFOLDERS	= 64,	/* for default view */
	SHCONTF_INCLUDEHIDDEN	= 128	/* for hidden/system objects */
} SHCONTF;

/*****************************************************************************
 * IShellFolder::GetAttributesOf flags 
 */
#define SFGAO_CANCOPY		DROPEFFECT_COPY /* Objects can be copied */
#define SFGAO_CANMOVE		DROPEFFECT_MOVE /* Objects can be moved */
#define SFGAO_CANLINK		DROPEFFECT_LINK /* Objects can be linked */
#define SFGAO_CANRENAME		0x00000010L	/* Objects can be renamed */
#define SFGAO_CANDELETE		0x00000020L	/* Objects can be deleted */
#define SFGAO_HASPROPSHEET	0x00000040L	/* Objects have property sheets */
#define SFGAO_DROPTARGET	0x00000100L	/* Objects are drop target */
#define SFGAO_CAPABILITYMASK	0x00000177L
#define SFGAO_LINK		0x00010000L	/* Shortcut (link) */
#define SFGAO_SHARE		0x00020000L	/* shared */
#define SFGAO_READONLY		0x00040000L	/* read-only */
#define SFGAO_GHOSTED		0x00080000L	/* ghosted icon */
#define SFGAO_HIDDEN            0x00080000L	/* hidden object */
#define SFGAO_DISPLAYATTRMASK	0x000F0000L
#define SFGAO_FILESYSANCESTOR	0x10000000L	/* It contains file system folder */
#define SFGAO_FOLDER		0x20000000L	/* It's a folder. */
#define SFGAO_FILESYSTEM	0x40000000L	/* is a file system thing (file/folder/root) */
#define SFGAO_HASSUBFOLDER	0x80000000L	/* Expandable in the map pane */
#define SFGAO_CONTENTSMASK	0x80000000L
#define SFGAO_VALIDATE		0x01000000L	/* invalidate cached information */
#define SFGAO_REMOVABLE		0x02000000L	/* is this removeable media? */
#define SFGAO_BROWSABLE		0x08000000L	/* is in-place browsable */
#define SFGAO_NONENUMERATED	0x00100000L	/* is a non-enumerated object */
#define SFGAO_NEWCONTENT	0x00200000L	/* should show bold in explorer tree */

/************************************************************************
 *
 * FOLDERSETTINGS
*/

typedef LPBYTE LPVIEWSETTINGS;

/* NB Bitfields. */
/* FWF_DESKTOP implies FWF_TRANSPARENT/NOCLIENTEDGE/NOSCROLL */
typedef enum
{ FWF_AUTOARRANGE =       0x0001,
  FWF_ABBREVIATEDNAMES =  0x0002,
  FWF_SNAPTOGRID =        0x0004,
  FWF_OWNERDATA =         0x0008,
  FWF_BESTFITWINDOW =     0x0010,
  FWF_DESKTOP =           0x0020,
  FWF_SINGLESEL =         0x0040,
  FWF_NOSUBFOLDERS =      0x0080,
  FWF_TRANSPARENT  =      0x0100,
  FWF_NOCLIENTEDGE =      0x0200,
  FWF_NOSCROLL     =      0x0400,
  FWF_ALIGNLEFT    =      0x0800,
  FWF_SINGLECLICKACTIVATE=0x8000  /* TEMPORARY -- NO UI FOR THIS */
} FOLDERFLAGS;

typedef enum
{ FVM_ICON =              1,
  FVM_SMALLICON =         2,
  FVM_LIST =              3,
  FVM_DETAILS =           4
} FOLDERVIEWMODE;

typedef struct
{ UINT ViewMode;       /* View mode (FOLDERVIEWMODE values) */
  UINT fFlags;         /* View options (FOLDERFLAGS bits) */
} FOLDERSETTINGS, *LPFOLDERSETTINGS;

typedef const FOLDERSETTINGS * LPCFOLDERSETTINGS;

/************************************************************************
 * Desktopfolder
 */

extern IShellFolder * pdesktopfolder;

DWORD WINAPI SHGetDesktopFolder(IShellFolder * *);

/*****************************************************************************
 * IShellFolder interface
 */
#define ICOM_INTERFACE IShellFolder
#define IShellFolder_METHODS \
    ICOM_METHOD6( HRESULT, ParseDisplayName, HWND, hwndOwner,LPBC, pbcReserved, LPOLESTR, lpszDisplayName, ULONG *, pchEaten, LPITEMIDLIST *, ppidl, ULONG *, pdwAttributes) \
    ICOM_METHOD3( HRESULT, EnumObjects, HWND, hwndOwner, DWORD, grfFlags, LPENUMIDLIST *, ppenumIDList)\
    ICOM_METHOD4( HRESULT, BindToObject, LPCITEMIDLIST, pidl, LPBC, pbcReserved, REFIID, riid, LPVOID *, ppvOut)\
    ICOM_METHOD4( HRESULT, BindToStorage, LPCITEMIDLIST, pidl, LPBC, pbcReserved, REFIID, riid, LPVOID *, ppvObj)\
    ICOM_METHOD3( HRESULT, CompareIDs, LPARAM, lParam, LPCITEMIDLIST, pidl1, LPCITEMIDLIST, pidl2)\
    ICOM_METHOD3( HRESULT, CreateViewObject, HWND, hwndOwner, REFIID, riid, LPVOID *, ppvOut)\
    ICOM_METHOD3( HRESULT, GetAttributesOf, UINT, cidl, LPCITEMIDLIST *, apidl, ULONG *, rgfInOut)\
    ICOM_METHOD6( HRESULT, GetUIObjectOf, HWND, hwndOwner, UINT, cidl, LPCITEMIDLIST *, apidl, REFIID, riid, UINT *, prgfInOut, LPVOID *, ppvOut)\
    ICOM_METHOD3( HRESULT, GetDisplayNameOf, LPCITEMIDLIST, pidl, DWORD, uFlags, LPSTRRET, lpName)\
    ICOM_METHOD5( HRESULT, SetNameOf, HWND, hwndOwner, LPCITEMIDLIST, pidl,LPCOLESTR, lpszName, DWORD, uFlags,LPITEMIDLIST *, ppidlOut)
#define IShellFolder_IMETHODS \
    IUnknown_IMETHODS \
    IShellFolder_METHODS
ICOM_DEFINE(IShellFolder,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IShellFolder_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IShellFolder_AddRef(p)				ICOM_CALL (AddRef,p)
#define IShellFolder_Release(p)				ICOM_CALL (Release,p)
/*** IShellFolder methods ***/
#define IShellFolder_ParseDisplayName(p,a,b,c,d,e,f)	ICOM_CALL6(ParseDisplayName,p,a,b,c,d,e,f)
#define IShellFolder_EnumObjects(p,a,b,c)		ICOM_CALL3(EnumObjects,p,a,b,c)
#define IShellFolder_BindToObject(p,a,b,c,d)		ICOM_CALL4(BindToObject,p,a,b,c,d)
#define IShellFolder_BindToStorage(p,a,b,c,d)		ICOM_CALL4(BindToStorage,p,a,b,c,d)
#define IShellFolder_CompareIDs(p,a,b,c)		ICOM_CALL3(CompareIDs,p,a,b,c)
#define IShellFolder_CreateViewObject(p,a,b,c)		ICOM_CALL3(CreateViewObject,p,a,b,c)
#define IShellFolder_GetAttributesOf(p,a,b,c)		ICOM_CALL3(GetAttributesOf,p,a,b,c)
#define IShellFolder_GetUIObjectOf(p,a,b,c,d,e,f)	ICOM_CALL6(GetUIObjectOf,p,a,b,c,d,e,f)
#define IShellFolder_GetDisplayNameOf(p,a,b,c)		ICOM_CALL3(GetDisplayNameOf,p,a,b,c)
#define IShellFolder_SetNameOf(p,a,b,c,d,e)		ICOM_CALL5(SetNameOf,p,a,b,c,d,e)

/*****************************************************************************
 * IShellFolder2 interface
 */
/* IShellFolder2 */

/* GetDefaultColumnState */
typedef enum 
{
	SHCOLSTATE_TYPE_STR	= 0x00000001,
	SHCOLSTATE_TYPE_INT	= 0x00000002,
	SHCOLSTATE_TYPE_DATE	= 0x00000003,
	SHCOLSTATE_TYPEMASK	= 0x0000000F,
	SHCOLSTATE_ONBYDEFAULT	= 0x00000010,
	SHCOLSTATE_SLOW		= 0x00000020,
	SHCOLSTATE_EXTENDED	= 0x00000040,
	SHCOLSTATE_SECONDARYUI	= 0x00000080,
	SHCOLSTATE_HIDDEN	= 0x00000100,
} SHCOLSTATE;

typedef struct
{
	GUID	fmtid;
	DWORD	pid;
} SHCOLUMNID, *LPSHCOLUMNID;
typedef const SHCOLUMNID* LPCSHCOLUMNID;

/* GetDetailsEx */
#define PID_FINDDATA		0
#define PID_NETRESOURCE		1
#define PID_DESCRIPTIONID	2

typedef struct
{
	int	fmt;
	int	cxChar;
	STRRET	str;
} SHELLDETAILS, *LPSHELLDETAILS;

#define ICOM_INTERFACE IShellFolder2 
#define IShellFolder2_METHODS \
    ICOM_METHOD1( HRESULT, GetDefaultSearchGUID, LPGUID, lpguid)\
    ICOM_METHOD1( HRESULT, EnumSearches, LPENUMEXTRASEARCH *, ppEnum) \
    ICOM_METHOD3( HRESULT, GetDefaultColumn, DWORD, dwReserved, ULONG *, pSort, ULONG *, pDisplay)\
    ICOM_METHOD2( HRESULT, GetDefaultColumnState, UINT, iColumn, DWORD *, pcsFlags)\
    ICOM_METHOD3( HRESULT, GetDetailsEx, LPCITEMIDLIST, pidl, const SHCOLUMNID *, pscid, VARIANT *, pv)\
    ICOM_METHOD3( HRESULT, GetDetailsOf, LPCITEMIDLIST, pidl, UINT, iColumn, LPSHELLDETAILS, pDetails)\
    ICOM_METHOD2( HRESULT, MapNameToSCID, LPCWSTR, pwszName, SHCOLUMNID *, pscid)
#define IShellFolder2_IMETHODS \
    IShellFolder_METHODS \
    IShellFolder2_METHODS
ICOM_DEFINE(IShellFolder2, IShellFolder)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IShellFolder2_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IShellFolder2_AddRef(p)				ICOM_CALL (AddRef,p)
#define IShellFolder2_Release(p)			ICOM_CALL (Release,p)
/*** IShellFolder methods ***/
#define IShellFolder2_ParseDisplayName(p,a,b,c,d,e,f)	ICOM_CALL6(ParseDisplayName,p,a,b,c,d,e,f)
#define IShellFolder2_EnumObjects(p,a,b,c)		ICOM_CALL3(EnumObjects,p,a,b,c)
#define IShellFolder2_BindToObject(p,a,b,c,d)		ICOM_CALL4(BindToObject,p,a,b,c,d)
#define IShellFolder2_BindToStorage(p,a,b,c,d)		ICOM_CALL4(BindToStorage,p,a,b,c,d)
#define IShellFolder2_CompareIDs(p,a,b,c)		ICOM_CALL3(CompareIDs,p,a,b,c)
#define IShellFolder2_CreateViewObject(p,a,b,c)		ICOM_CALL3(CreateViewObject,p,a,b,c)
#define IShellFolder2_GetAttributesOf(p,a,b,c)		ICOM_CALL3(GetAttributesOf,p,a,b,c)
#define IShellFolder2_GetUIObjectOf(p,a,b,c,d,e,f)	ICOM_CALL6(GetUIObjectOf,p,a,b,c,d,e,f)
#define IShellFolder2_GetDisplayNameOf(p,a,b,c)		ICOM_CALL3(GetDisplayNameOf,p,a,b,c)
#define IShellFolder2_SetNameOf(p,a,b,c,d,e)		ICOM_CALL5(SetNameOf,p,a,b,c,d,e)
/*** IShellFolder2 methods ***/
#define IShellFolder2_GetDefaultSearchGUID(p,a)		ICOM_CALL1(GetDefaultSearchGUID,p,a)
#define IShellFolder2_EnumSearches(p,a)			ICOM_CALL1(EnumSearches,p,a)
#define IShellFolder2_GetDefaultColumn(p,a,b,c)		ICOM_CALL3(GetDefaultColumn,p,a,b,c)
#define IShellFolder2_GetDefaultColumnState(p,a,b)	ICOM_CALL2(GetDefaultColumnState,p,a,b)
#define IShellFolder2_GetDetailsEx(p,a,b,c)		ICOM_CALL3(GetDetailsEx,p,a,b,c)
#define IShellFolder2_GetDetailsOf(p,a,b,c)		ICOM_CALL3(GetDetailsOf,p,a,b,c)
#define IShellFolder2_MapNameToSCID(p,a,b)		ICOM_CALL2(MapNameToSCID,p,a,b)

/*****************************************************************************
 * IPersistFolder interface
 */

/* ClassID's */
DEFINE_GUID (CLSID_SFMyComp,0x20D04FE0,0x3AEA,0x1069,0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D); 
DEFINE_GUID (CLSID_SFINet,  0x871C5380,0x42A0,0x1069,0xA2,0xEA,0x08,0x00,0x2B,0x30,0x30,0x9D);
DEFINE_GUID (CLSID_SFFile,  0xF3364BA0,0x65B9,0x11CE,0xA9,0xBA,0x00,0xAA,0x00,0x4A,0xE8,0x37);

#define ICOM_INTERFACE IPersistFolder
#define IPersistFolder_METHODS \
    ICOM_METHOD1( HRESULT, Initialize, LPCITEMIDLIST, pidl)
#define IPersistFolder_IMETHODS \
    IPersist_IMETHODS \
    IPersistFolder_METHODS
ICOM_DEFINE(IPersistFolder, IPersist)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistFolder_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b) 
#define IPersistFolder_AddRef(p)		ICOM_CALL (AddRef,p)
#define IPersistFolder_Release(p)		ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistFolder_GetClassID(p,a)		ICOM_CALL1(GetClassID,p,a)
/*** IPersistFolder methods ***/
#define IPersistFolder_Initialize(p,a)		ICOM_CALL1(Initialize,p,a)

/*****************************************************************************
 * IPersistFolder2 interface
 */

#define ICOM_INTERFACE IPersistFolder2
#define IPersistFolder2_METHODS \
    ICOM_METHOD1( HRESULT, GetCurFolder, LPITEMIDLIST*, pidl)
#define IPersistFolder2_IMETHODS \
    IPersist_IMETHODS \
    IPersistFolder_METHODS \
    IPersistFolder2_METHODS
ICOM_DEFINE(IPersistFolder2, IPersistFolder)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistFolder2_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b) 
#define IPersistFolder2_AddRef(p)		ICOM_CALL (AddRef,p)
#define IPersistFolder2_Release(p)		ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistFolder2_GetClassID(p,a)		ICOM_CALL1(GetClassID,p,a)
/*** IPersistFolder methods ***/
#define IPersistFolder2_Initialize(p,a)		ICOM_CALL1(Initialize,p,a)
/*** IPersistFolder2 methods ***/
#define IPersistFolder2_GetCurFolder(p,a)	ICOM_CALL1(GetCurFolder,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SHELLFOLDER_H */
