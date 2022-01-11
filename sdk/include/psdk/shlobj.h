/*
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SHLOBJ_H
#define __WINE_SHLOBJ_H

#include <ole2.h>
#include <commctrl.h>
#include <prsht.h>

#ifndef INITGUID
#include <shlguid.h>
#endif

#ifdef WINE_NO_UNICODE_MACROS
#undef GetObject
#endif

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/* Except for specific structs, this header is byte packed */
#include <pshpack1.h>

#include <shtypes.h>
#include <shobjidl.h>

#include <pshpack8.h>

typedef struct
{
    DWORD         dwSize;
    DWORD         dwMask;
    SHELLVIEWID*  pvid;
    LPSTR         pszWebViewTemplate;
    DWORD         cchWebViewTemplate;
    LPSTR         pszWebViewTemplateVersion;
    LPSTR         pszInfoTip;
    DWORD         cchInfoTip;
    CLSID*        pclsid;
    DWORD         dwFlags;
    LPSTR         pszIconFile;
    DWORD         cchIconFile;
    int           iIconIndex;
    LPSTR         pszLogo;
    DWORD         cchLogo;
} SHFOLDERCUSTOMSETTINGSA, *LPSHFOLDERCUSTOMSETTINGSA;

typedef struct
{
    DWORD         dwSize;
    DWORD         dwMask;
    SHELLVIEWID*  pvid;
    LPWSTR        pszWebViewTemplate;
    DWORD         cchWebViewTemplate;
    LPWSTR        pszWebViewTemplateVersion;
    LPWSTR        pszInfoTip;
    DWORD         cchInfoTip;
    CLSID*        pclsid;
    DWORD         dwFlags;
    LPWSTR        pszIconFile;
    DWORD         cchIconFile;
    int           iIconIndex;
    LPWSTR        pszLogo;
    DWORD         cchLogo;
} SHFOLDERCUSTOMSETTINGSW, *LPSHFOLDERCUSTOMSETTINGSW;

#include <poppack.h>

#define FCS_READ       0x00000001
#define FCS_FORCEWRITE 0x00000002

#define FCSM_ICONFILE 0x00000010

#ifndef HPSXA_DEFINED
#define HPSXA_DEFINED
DECLARE_HANDLE(HPSXA);
#endif

typedef enum
{
    KF_FLAG_DEFAULT                     = 0x00000000,
    KF_FLAG_SIMPLE_IDLIST               = 0x00000100,
    KF_FLAG_NOT_PARENT_RELATIVE         = 0x00000200,
    KF_FLAG_DEFAULT_PATH                = 0x00000400,
    KF_FLAG_INIT                        = 0x00000800,
    KF_FLAG_NO_ALIAS                    = 0x00001000,
    KF_FLAG_DONT_UNEXPAND               = 0x00002000,
    KF_FLAG_DONT_VERIFY                 = 0x00004000,
    KF_FLAG_CREATE                      = 0x00008000,
    KF_FLAG_NO_APPCONTAINER_REDIRECTION = 0x00010000,
    KF_FLAG_ALIAS_ONLY                  = 0x80000000
} KNOWN_FOLDER_FLAG;

typedef int GPFIDL_FLAGS;

UINT
WINAPI
SHAddFromPropSheetExtArray(
  _In_ HPSXA,
  _In_ LPFNADDPROPSHEETPAGE,
  LPARAM);

LPVOID       WINAPI SHAlloc(SIZE_T) __WINE_ALLOC_SIZE(1);

HRESULT
WINAPI
SHCoCreateInstance(
  _In_opt_ LPCWSTR,
  _In_opt_ const CLSID*,
  _In_opt_ IUnknown*,
  _In_ REFIID,
  _Outptr_ LPVOID*);

HPSXA        WINAPI SHCreatePropSheetExtArray(_In_ HKEY, _In_opt_ LPCWSTR, UINT);
HPSXA        WINAPI SHCreatePropSheetExtArrayEx(HKEY,LPCWSTR,UINT,IDataObject*);
HRESULT      WINAPI SHCreateQueryCancelAutoPlayMoniker(IMoniker**);

HRESULT
WINAPI
SHCreateShellItem(
  _In_opt_ PCIDLIST_ABSOLUTE,
  _In_opt_ IShellFolder*,
  _In_ PCUITEMID_CHILD,
  _Outptr_ IShellItem**);

DWORD        WINAPI SHCLSIDFromStringA(_In_ LPCSTR, _Out_ CLSID*);
DWORD        WINAPI SHCLSIDFromStringW(_In_ LPCWSTR, _Out_ CLSID*);
#define             SHCLSIDFromString WINELIB_NAME_AW(SHCLSIDFromString)

HRESULT
WINAPI
SHCreateStdEnumFmtEtc(
  _In_ UINT cfmt,
  _In_reads_(cfmt) const FORMATETC *,
  _Outptr_ IEnumFORMATETC**);

void         WINAPI SHDestroyPropSheetExtArray(_In_ HPSXA);
BOOL         WINAPI SHFindFiles(_In_opt_ PCIDLIST_ABSOLUTE, _In_opt_ PCIDLIST_ABSOLUTE);
DWORD        WINAPI SHFormatDrive(_In_ HWND, UINT, UINT, UINT);
void         WINAPI SHFree(_In_opt_ LPVOID);

BOOL
WINAPI
GetFileNameFromBrowse(
  _In_opt_ HWND,
  _Inout_updates_(cchFilePath) LPWSTR,
  UINT cchFilePath,
  _In_opt_ LPCWSTR,
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR);

_Check_return_ HRESULT WINAPI SHGetInstanceExplorer(_Outptr_ IUnknown**);

VOID         WINAPI SHSetInstanceExplorer(_In_opt_ IUnknown*);

HRESULT
WINAPI
SHGetFolderPathAndSubDirA(
  _Reserved_ HWND,
  _In_ int,
  _In_opt_ HANDLE,
  _In_ DWORD,
  _In_opt_ LPCSTR, _Out_writes_(MAX_PATH) LPSTR);

HRESULT
WINAPI
SHGetFolderPathAndSubDirW(
  _Reserved_ HWND,
  _In_ int,
  _In_opt_ HANDLE,
  _In_ DWORD,
  _In_opt_ LPCWSTR,
  _Out_writes_(MAX_PATH) LPWSTR);

#define SHGetFolderPathAndSubDir WINELIB_NAME_AW(SHGetFolderPathAndSubDir);

_Success_(return != 0)
BOOL
WINAPI
SHGetPathFromIDListA(
  _In_ PCIDLIST_ABSOLUTE,
  _Out_writes_(MAX_PATH) LPSTR);

_Success_(return != 0)
BOOL
WINAPI
SHGetPathFromIDListW(
  _In_ PCIDLIST_ABSOLUTE,
  _Out_writes_(MAX_PATH) LPWSTR);

#define SHGetPathFromIDList WINELIB_NAME_AW(SHGetPathFromIDList)

INT          WINAPI SHHandleUpdateImage(_In_ PCIDLIST_ABSOLUTE);

HRESULT
WINAPI
SHILCreateFromPath(
  _In_ PCWSTR,
  _Outptr_ PIDLIST_ABSOLUTE*,
  _Inout_opt_ DWORD*);

HRESULT      WINAPI SHLoadOLE(LPARAM);

HRESULT
WINAPI
SHParseDisplayName(
  _In_ PCWSTR,
  _In_opt_ IBindCtx*,
  _Outptr_ PIDLIST_ABSOLUTE*,
  _In_ SFGAOF,
  _Out_opt_ SFGAOF*);

HRESULT
WINAPI
SHPathPrepareForWriteA(
  _In_opt_ HWND,
  _In_opt_ IUnknown*,
  _In_ LPCSTR, DWORD);

HRESULT
WINAPI
SHPathPrepareForWriteW(
  _In_opt_ HWND,
  _In_opt_ IUnknown*,
  _In_ LPCWSTR, DWORD);

#define SHPathPrepareForWrite WINELIB_NAME_AW(SHPathPrepareForWrite);

UINT
WINAPI
SHReplaceFromPropSheetExtArray(
  _In_ HPSXA,
  UINT,
  _In_ LPFNADDPROPSHEETPAGE,
  LPARAM);

PIDLIST_ABSOLUTE WINAPI SHSimpleIDListFromPath(PCWSTR);

int
WINAPI
SHMapPIDLToSystemImageListIndex(
  _In_ IShellFolder*,
  _In_ PCUITEMID_CHILD,
  _Out_opt_ int*);

HRESULT      WINAPI SHStartNetConnectionDialog(HWND,LPCSTR,DWORD);
VOID         WINAPI SHUpdateImageA(_In_ LPCSTR, INT, UINT, INT);
VOID         WINAPI SHUpdateImageW(_In_ LPCWSTR, INT, UINT, INT);
#define             SHUpdateImage WINELIB_NAME_AW(SHUpdateImage)

INT
WINAPI
PickIconDlg(
  _In_opt_ HWND,
  _Inout_updates_(cchIconPath) LPWSTR,
  UINT cchIconPath,
  _Inout_opt_ int *);

HRESULT
WINAPI
SHLimitInputEdit(
  _In_ HWND hwnd,
  _In_ IShellFolder *folder);

int          WINAPI RestartDialog(_In_opt_ HWND, _In_opt_ LPCWSTR, DWORD);
int          WINAPI RestartDialogEx(_In_opt_ HWND, _In_opt_ LPCWSTR, DWORD, DWORD);
int          WINAPI DriveType(int);
int          WINAPI RealDriveType(int, BOOL);
int          WINAPI IsNetDrive(int);
BOOL         WINAPI IsUserAnAdmin(void);

#define KF_FLAG_DEFAULT_PATH        0x00000400

#define SHFMT_ERROR     0xFFFFFFFFL  /* Error on last format, drive may be formattable */
#define SHFMT_CANCEL    0xFFFFFFFEL  /* Last format was cancelled */
#define SHFMT_NOFORMAT  0xFFFFFFFDL  /* Drive is not formattable */

/* SHFormatDrive flags */
#define SHFMT_ID_DEFAULT	0xFFFF
#define SHFMT_OPT_FULL		1
#define SHFMT_OPT_SYSONLY	2

/* SHPathPrepareForWrite flags */
#define SHPPFW_NONE             0x00000000
#define SHPPFW_DIRCREATE        0x00000001
#define SHPPFW_DEFAULT          SHPPFW_DIRCREATE
#define SHPPFW_ASKDIRCREATE     0x00000002
#define SHPPFW_IGNOREFILENAME   0x00000004
#define SHPPFW_NOWRITECHECK     0x00000008
#define SHPPFW_MEDIACHECKONLY   0x00000010

/* SHObjectProperties flags */
#define SHOP_PRINTERNAME 0x01
#define SHOP_FILEPATH    0x02
#define SHOP_VOLUMEGUID  0x04

BOOL
WINAPI
SHObjectProperties(
  _In_opt_ HWND,
  _In_ DWORD,
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR);

HRESULT
WINAPI
SHOpenFolderAndSelectItems(
  _In_ PCIDLIST_ABSOLUTE pidlFolder,
  _In_ UINT cidl,
  _In_reads_opt_(cidl) PCUITEMID_CHILD_ARRAY,
  _In_ DWORD);

#define PCS_FATAL           0x80000000
#define PCS_REPLACEDCHAR    0x00000001
#define PCS_REMOVEDCHAR     0x00000002
#define PCS_TRUNCATED       0x00000004
#define PCS_PATHTOOLONG     0x00000008

int WINAPI PathCleanupSpec(_In_opt_ LPCWSTR, _Inout_ LPWSTR);

/*****************************************************************************
 * IContextMenu interface
 */


/* DATAOBJECT_InitShellIDList*/
#define CFSTR_SHELLIDLISTA           "Shell IDList Array"   /* CF_IDLIST */
#define CFSTR_SHELLIDLISTOFFSETA     "Shell Object Offsets" /* CF_OBJECTPOSITIONS */
#define CFSTR_NETRESOURCESA          "Net Resource"         /* CF_NETRESOURCE */
/* DATAOBJECT_InitFileGroupDesc */
#define CFSTR_FILEDESCRIPTORA        "FileGroupDescriptor"  /* CF_FILEGROUPDESCRIPTORA */
/* DATAOBJECT_InitFileContents*/
#define CFSTR_FILECONTENTSA          "FileContents"         /* CF_FILECONTENTS */
#define CFSTR_FILENAMEA              "FileName"             /* CF_FILENAMEA */
#define CFSTR_FILENAMEMAPA           "FileNameMap"          /* CF_FILENAMEMAPA */
#define CFSTR_PRINTERGROUPA          "PrinterFriendlyName"  /* CF_PRINTERS */
#define CFSTR_SHELLURLA              "UniformResourceLocator"
#define CFSTR_INETURLA               CFSTR_SHELLURLA
#define CFSTR_PREFERREDDROPEFFECTA   "Preferred DropEffect"
#define CFSTR_PERFORMEDDROPEFFECTA   "Performed DropEffect"
#define CFSTR_PASTESUCCEEDEDA        "Paste Succeeded"
#define CFSTR_INDRAGLOOPA            "InShellDragLoop"
#define CFSTR_DRAGCONTEXTA           "DragContext"
#define CFSTR_MOUNTEDVOLUMEA         "MountedVolume"
#define CFSTR_PERSISTEDDATAOBJECTA   "PersistedDataObject"
#define CFSTR_TARGETCLSIDA           "TargetCLSID"
#define CFSTR_AUTOPLAY_SHELLIDLISTSA "Autoplay Enumerated IDList Array"
#define CFSTR_LOGICALPERFORMEDDROPEFFECTA "Logical Performed DropEffect"

#if defined(__GNUC__)
# define CFSTR_SHELLIDLISTW \
    (const WCHAR []){ 'S','h','e','l','l',' ','I','D','L','i','s','t',' ','A','r','r','a','y',0 }
# define CFSTR_SHELLIDLISTOFFSETW \
    (const WCHAR []){ 'S','h','e','l','l',' ','O','b','j','e','c','t',' ','O','f','f','s','e','t','s',0 }
# define CFSTR_NETRESOURCESW \
    (const WCHAR []){ 'N','e','t',' ','R','e','s','o','u','r','c','e',0 }
# define CFSTR_FILEDESCRIPTORW \
    (const WCHAR []){ 'F','i','l','e','G','r','o','u','p','D','e','s','c','r','i','p','t','o','r','W',0 }
# define CFSTR_FILECONTENTSW \
    (const WCHAR []){ 'F','i','l','e','C','o','n','t','e','n','t','s',0 }
# define CFSTR_FILENAMEW \
    (const WCHAR []){ 'F','i','l','e','N','a','m','e','W',0 }
# define CFSTR_FILENAMEMAPW \
    (const WCHAR []){ 'F','i','l','e','N','a','m','e','M','a','p','W',0 }
# define CFSTR_PRINTERGROUPW \
    (const WCHAR []){ 'P','r','i','n','t','e','r','F','r','i','e','n','d','l','y','N','a','m','e',0 }
# define CFSTR_SHELLURLW \
    (const WCHAR []){ 'U','n','i','f','o','r','m','R','e','s','o','u','r','c','e','L','o','c','a','t','o','r',0 }
# define CFSTR_INETURLW \
    (const WCHAR []){ 'U','n','i','f','o','r','m','R','e','s','o','u','r','c','e','L','o','c','a','t','o','r','W',0 }
# define CFSTR_PREFERREDDROPEFFECTW \
    (const WCHAR []){ 'P','r','e','f','e','r','r','e','d',' ','D','r','o','p','E','f','f','e','c','t',0 }
# define CFSTR_PERFORMEDDROPEFFECTW \
    (const WCHAR []){ 'P','e','r','f','o','r','m','e','d',' ','D','r','o','p','E','f','f','e','c','t',0 }
# define CFSTR_PASTESUCCEEDEDW \
    (const WCHAR []){ 'P','a','s','t','e',' ','S','u','c','c','e','e','d','e','d',0 }
# define CFSTR_INDRAGLOOPW \
    (const WCHAR []){ 'I','n','S','h','e','l','l','D','r','a','g','L','o','o','p',0 }
# define CFSTR_DRAGCONTEXTW \
    (const WCHAR []){ 'D','r','a','g','C','o','n','t','e','x','t',0 }
# define CFSTR_MOUNTEDVOLUMEW \
    (const WCHAR []){ 'M','o','u','n','t','e','d','V','o','l','u','m','e',0 }
# define CFSTR_PERSISTEDDATAOBJECTW \
    (const WCHAR []){ 'P','e','r','s','i','s','t','e','d','D','a','t','a','O','b','j','e','c','t',0 }
# define CFSTR_TARGETCLSIDW \
    (const WCHAR []){ 'T','a','r','g','e','t','C','L','S','I','D',0 }
# define CFSTR_AUTOPLAY_SHELLIDLISTSW \
    (const WCHAR []){ 'A','u','t','o','p','l','a','y',' ','E','n','u','m','e','r','a','t','e','d',\
                      ' ','I','D','L','i','s','t',' ','A','r','r','a','y',0 }
# define CFSTR_LOGICALPERFORMEDDROPEFFECTW \
    (const WCHAR []){ 'L','o','g','i','c','a','l',' ','P','e','r','f','o','r','m','e','d',\
                      ' ','D','r','o','p','E','f','f','e','c','t',0 }
#elif defined(_MSC_VER)
# define CFSTR_SHELLIDLISTW           L"Shell IDList Array"
# define CFSTR_SHELLIDLISTOFFSETW     L"Shell Object Offsets"
# define CFSTR_NETRESOURCESW          L"Net Resource"
# define CFSTR_FILEDESCRIPTORW        L"FileGroupDescriptorW"
# define CFSTR_FILECONTENTSW          L"FileContents"
# define CFSTR_FILENAMEW              L"FileNameW"
# define CFSTR_FILENAMEMAPW           L"FileNameMapW"
# define CFSTR_PRINTERGROUPW          L"PrinterFriendlyName"
# define CFSTR_SHELLURLW              L"UniformResourceLocator"
# define CFSTR_INETURLW               L"UniformResourceLocatorW"
# define CFSTR_PREFERREDDROPEFFECTW   L"Preferred DropEffect"
# define CFSTR_PERFORMEDDROPEFFECTW   L"Performed DropEffect"
# define CFSTR_PASTESUCCEEDEDW        L"Paste Succeeded"
# define CFSTR_INDRAGLOOPW            L"InShellDragLoop"
# define CFSTR_DRAGCONTEXTW           L"DragContext"
# define CFSTR_MOUNTEDVOLUMEW         L"MountedVolume"
# define CFSTR_PERSISTEDDATAOBJECTW   L"PersistedDataObject"
# define CFSTR_TARGETCLSIDW           L"TargetCLSID"
# define CFSTR_AUTOPLAY_SHELLIDLISTSW L"Autoplay Enumerated IDList Array"
# define CFSTR_LOGICALPERFORMEDDROPEFFECTW L"Logical Performed DropEffect"
#else
static const WCHAR CFSTR_SHELLIDLISTW[] =
    { 'S','h','e','l','l',' ','I','D','L','i','s','t',' ','A','r','r','a','y',0 };
static const WCHAR CFSTR_SHELLIDLISTOFFSETW[] =
    { 'S','h','e','l','l',' ','O','b','j','e','c','t',' ','O','f','f','s','e','t','s',0 };
static const WCHAR CFSTR_NETRESOURCESW[] =
    { 'N','e','t',' ','R','e','s','o','u','r','c','e',0 };
static const WCHAR CFSTR_FILEDESCRIPTORW[] =
    { 'F','i','l','e','G','r','o','u','p','D','e','s','c','r','i','p','t','o','r','W',0 };
static const WCHAR CFSTR_FILECONTENTSW[] =
    { 'F','i','l','e','C','o','n','t','e','n','t','s',0 };
static const WCHAR CFSTR_FILENAMEW[] =
    { 'F','i','l','e','N','a','m','e','W',0 };
static const WCHAR CFSTR_FILENAMEMAPW[] =
    { 'F','i','l','e','N','a','m','e','M','a','p','W',0 };
static const WCHAR CFSTR_PRINTERGROUPW[] =
    { 'P','r','i','n','t','e','r','F','r','i','e','n','d','l','y','N','a','m','e',0 };
static const WCHAR CFSTR_SHELLURLW[] =
    { 'U','n','i','f','o','r','m','R','e','s','o','u','r','c','e','L','o','c','a','t','o','r',0 };
static const WCHAR CFSTR_INETURLW[] =
    { 'U','n','i','f','o','r','m','R','e','s','o','u','r','c','e','L','o','c','a','t','o','r','W',0 };
static const WCHAR CFSTR_PREFERREDDROPEFFECTW[] =
    { 'P','r','e','f','e','r','r','e','d',' ','D','r','o','p','E','f','f','e','c','t',0 };
static const WCHAR CFSTR_PERFORMEDDROPEFFECTW[] =
    { 'P','e','r','f','o','r','m','e','d',' ','D','r','o','p','E','f','f','e','c','t',0 };
static const WCHAR CFSTR_PASTESUCCEEDEDW[] =
    { 'P','a','s','t','e',' ','S','u','c','c','e','e','d','e','d',0 };
static const WCHAR CFSTR_INDRAGLOOPW[] =
    { 'I','n','S','h','e','l','l','D','r','a','g','L','o','o','p',0 };
static const WCHAR CFSTR_DRAGCONTEXTW[] =
    { 'D','r','a','g','C','o','n','t','e','x','t',0 };
static const WCHAR CFSTR_MOUNTEDVOLUMEW[] =
    { 'M','o','u','n','t','e','d','V','o','l','u','m','e',0 };
static const WCHAR CFSTR_PERSISTEDDATAOBJECTW[] =
    { 'P','e','r','s','i','s','t','e','d','D','a','t','a','O','b','j','e','c','t',0 };
static const WCHAR CFSTR_TARGETCLSIDW[] =
    { 'T','a','r','g','e','t','C','L','S','I','D',0 };
static const WCHAR CFSTR_AUTOPLAY_SHELLIDLISTSW[] =
    { 'A','u','t','o','p','l','a','y',' ','E','n','u','m','e','r','a','t','e','d',
      ' ','I','D','L','i','s','t',' ','A','r','r','a','y',0 };
static const WCHAR CFSTR_LOGICALPERFORMEDDROPEFFECTW[] =
    { 'L','o','g','i','c','a','l',' ','P','e','r','f','o','r','m','e','d',
      ' ','D','r','o','p','E','f','f','e','c','t',0 };
#endif

#define CFSTR_SHELLIDLIST           WINELIB_NAME_AW(CFSTR_SHELLIDLIST)
#define CFSTR_SHELLIDLISTOFFSET     WINELIB_NAME_AW(CFSTR_SHELLIDLISTOFFSET)
#define CFSTR_NETRESOURCES          WINELIB_NAME_AW(CFSTR_NETRESOURCES)
#define CFSTR_FILEDESCRIPTOR        WINELIB_NAME_AW(CFSTR_FILEDESCRIPTOR)
#define CFSTR_FILECONTENTS          WINELIB_NAME_AW(CFSTR_FILECONTENTS)
#define CFSTR_FILENAME              WINELIB_NAME_AW(CFSTR_FILENAME)
#define CFSTR_FILENAMEMAP           WINELIB_NAME_AW(CFSTR_FILENAMEMAP)
#define CFSTR_PRINTERGROUP          WINELIB_NAME_AW(CFSTR_PRINTERGROUP)
#define CFSTR_SHELLURL              WINELIB_NAME_AW(CFSTR_SHELLURL)
#define CFSTR_INETURL               WINELIB_NAME_AW(CFSTR_INETURL)
#define CFSTR_PREFERREDDROPEFFECT   WINELIB_NAME_AW(CFSTR_PREFERREDDROPEFFECT)
#define CFSTR_PERFORMEDDROPEFFECT   WINELIB_NAME_AW(CFSTR_PERFORMEDDROPEFFECT)
#define CFSTR_PASTESUCCEEDED        WINELIB_NAME_AW(CFSTR_PASTESUCCEEDED)
#define CFSTR_INDRAGLOOP            WINELIB_NAME_AW(CFSTR_INDRAGLOOP)
#define CFSTR_DRAGCONTEXT           WINELIB_NAME_AW(CFSTR_DRAGCONTEXT)
#define CFSTR_MOUNTEDVOLUME         WINELIB_NAME_AW(CFSTR_MOUNTEDVOLUME)
#define CFSTR_PERSISTEDDATAOBJECT   WINELIB_NAME_AW(CFSTR_PERSISTEDDATAOBJECT)
#define CFSTR_TARGETCLSID           WINELIB_NAME_AW(CFSTR_TARGETCLSID)
#define CFSTR_AUTOPLAY_SHELLIDLISTS WINELIB_NAME_AW(CFSTR_AUTOPLAY_SHELLIDLISTS)
#define CFSTR_LOGICALPERFORMEDDROPEFFECT WINELIB_NAME_AW(CFSTR_LOGICALPERFORMEDDROPEFFECT)

typedef struct
{	UINT cidl;
	UINT aoffset[1];
} CIDA, *LPIDA;

/************************************************************************
* IShellView interface
*/

#define FCIDM_SHVIEWFIRST       0x0000
/* undocumented */
#define FCIDM_SHVIEW_ARRANGE    0x7001
#define FCIDM_SHVIEW_DELETE     0x7011
#define FCIDM_SHVIEW_PROPERTIES 0x7013
#define FCIDM_SHVIEW_CUT        0x7018
#define FCIDM_SHVIEW_COPY       0x7019
#define FCIDM_SHVIEW_INSERT     0x701A
#define FCIDM_SHVIEW_UNDO       0x701B
#define FCIDM_SHVIEW_INSERTLINK 0x701C
#define FCIDM_SHVIEW_COPYTO     0x701E
#define FCIDM_SHVIEW_MOVETO     0x701F
#define FCIDM_SHVIEW_SELECTALL  0x7021
#define FCIDM_SHVIEW_INVERTSELECTION 0x7022

#define FCIDM_SHVIEW_BIGICON    0x7029
#define FCIDM_SHVIEW_SMALLICON  0x702A
#define FCIDM_SHVIEW_LISTVIEW   0x702B
#define FCIDM_SHVIEW_REPORTVIEW 0x702C
/* 0x7030-0x703f are used by the shellbrowser */
#define FCIDM_SHVIEW_AUTOARRANGE 0x7031
#define FCIDM_SHVIEW_SNAPTOGRID 0x7032
#define FCIDM_SHVIEW_ALIGNTOGRID 0x7033

#define FCIDM_SHVIEW_HELP       0x7041
#define FCIDM_SHVIEW_RENAME     0x7050
#define FCIDM_SHVIEW_CREATELINK 0x7051
#define FCIDM_SHVIEW_NEWLINK    0x7052
#define FCIDM_SHVIEW_NEWFOLDER  0x7053

#define FCIDM_SHVIEW_REFRESH    0x7100 /* FIXME */
#define FCIDM_SHVIEW_EXPLORE    0x7101 /* FIXME */
#define FCIDM_SHVIEW_OPEN       0x7102 /* FIXME */

#define FCIDM_SHVIEWLAST        0x7fff
#define FCIDM_BROWSERFIRST      0xA000
/* undocumented toolbar items from stddlg's*/
#define FCIDM_TB_UPFOLDER       0xA001
#define FCIDM_TB_NEWFOLDER      0xA002
#define FCIDM_TB_SMALLICON      0xA003
#define FCIDM_TB_REPORTVIEW     0xA004
#define FCIDM_TB_DESKTOP        0xA005  /* FIXME */

#define FCIDM_BROWSERLAST       0xbf00
#define FCIDM_GLOBALFIRST       0x8000
#define FCIDM_GLOBALLAST        0x9fff

/*
* Global submenu IDs and separator IDs
*/
#define FCIDM_MENU_FILE             (FCIDM_GLOBALFIRST+0x0000)
#define FCIDM_MENU_EDIT             (FCIDM_GLOBALFIRST+0x0040)
#define FCIDM_MENU_VIEW             (FCIDM_GLOBALFIRST+0x0080)
#define FCIDM_MENU_VIEW_SEP_OPTIONS (FCIDM_GLOBALFIRST+0x0081)
#define FCIDM_MENU_TOOLS            (FCIDM_GLOBALFIRST+0x00c0)
#define FCIDM_MENU_TOOLS_SEP_GOTO   (FCIDM_GLOBALFIRST+0x00c1)
#define FCIDM_MENU_HELP             (FCIDM_GLOBALFIRST+0x0100)
#define FCIDM_MENU_FIND             (FCIDM_GLOBALFIRST+0x0140)
#define FCIDM_MENU_EXPLORE          (FCIDM_GLOBALFIRST+0x0150)
#define FCIDM_MENU_FAVORITES        (FCIDM_GLOBALFIRST+0x0170)

/* control IDs known to the view */
#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)

#define INTERFACE IShellDetails
DECLARE_INTERFACE_(IShellDetails, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IShellDetails methods ***/
    STDMETHOD(GetDetailsOf)(THIS_ _In_opt_ PCUITEMID_CHILD pidl, UINT iColumn, _Out_ SHELLDETAILS *pDetails) PURE;
    STDMETHOD(ColumnClick)(THIS_ UINT iColumn) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IShellDetails_QueryInterface(p,a,b)    (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellDetails_AddRef(p)                (p)->lpVtbl->AddRef(p)
#define IShellDetails_Release(p)               (p)->lpVtbl->Release(p)
/*** IShellDetails methods ***/
#define IShellDetails_GetDetailsOf(p,a,b,c)    (p)->lpVtbl->GetDetailsOf(p,a,b,c)
#define IShellDetails_ColumnClick(p,a)         (p)->lpVtbl->ColumnClick(p,a)
#endif

/* IQueryInfo interface */
#define INTERFACE IQueryInfo
DECLARE_INTERFACE_(IQueryInfo,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IQueryInfo methods ***/
    STDMETHOD(GetInfoTip)(THIS_ DWORD dwFlags, _Outptr_ WCHAR** lppTips) PURE;
    STDMETHOD(GetInfoFlags)(THIS_ _Out_ DWORD* lpFlags) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IQueryInfo_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IQueryInfo_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IQueryInfo_Release(p)                 (p)->lpVtbl->Release(p)
/*** IQueryInfo methods ***/
#define IQueryInfo_GetInfoTip(p,a,b)          (p)->lpVtbl->GetInfoTip(p,a,b)
#define IQueryInfo_GetInfoFlags(p,a)          (p)->lpVtbl->GetInfoFlags(p,a)
#endif

/* IInputObject interface */
#define INTERFACE IInputObject
DECLARE_INTERFACE_(IInputObject,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IInputObject methods ***/
    STDMETHOD(UIActivateIO)(THIS_ BOOL bActivating, LPMSG lpMsg) PURE;
    STDMETHOD(HasFocusIO)(THIS) PURE;
    STDMETHOD(TranslateAcceleratorIO)(THIS_ LPMSG lpMsg) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IInputObject_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define IInputObject_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define IInputObject_Release(p)                  (p)->lpVtbl->Release(p)
/*** IInputObject methods ***/
#define IInputObject_UIActivateIO(p,a,b)         (p)->lpVtbl->UIActivateIO(p,a,b)
#define IInputObject_HasFocusIO(p)               (p)->lpVtbl->HasFocusIO(p)
#define IInputObject_TranslateAcceleratorIO(p,a) (p)->lpVtbl->TranslateAcceleratorIO(p,a)
#endif

/* IInputObjectSite interface */
#define INTERFACE IInputObjectSite
DECLARE_INTERFACE_(IInputObjectSite,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IInputObjectSite methods ***/
    STDMETHOD(OnFocusChangeIS)(THIS_ LPUNKNOWN lpUnknown, BOOL bFocus) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IInputObjectSite_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IInputObjectSite_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IInputObjectSite_Release(p)             (p)->lpVtbl->Release(p)
/*** IInputObject methods ***/
#define IInputObjectSite_OnFocusChangeIS(p,a,b) (p)->lpVtbl->OnFocusChangeIS(p,a,b)
#endif

/* IObjMgr interface */
#define INTERFACE IObjMgr
DECLARE_INTERFACE_(IObjMgr,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IObjMgr methods ***/
    STDMETHOD(Append)(THIS_ _In_ LPUNKNOWN punk) PURE;
    STDMETHOD(Remove)(THIS_ _In_ LPUNKNOWN punk) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IObjMgr_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IObjMgr_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IObjMgr_Release(p)             (p)->lpVtbl->Release(p)
/*** IObjMgr methods ***/
#define IObjMgr_Append(p,a) (p)->lpVtbl->Append(p,a)
#define IObjMgr_Remove(p,a) (p)->lpVtbl->Remove(p,a)
#endif

/* ICurrentWorkingDirectory interface */
#define INTERFACE ICurrentWorkingDirectory
DECLARE_INTERFACE_(ICurrentWorkingDirectory,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** ICurrentWorkingDirectory methods ***/
    STDMETHOD(GetDirectory)(THIS_ _Out_writes_(cchSize) PWSTR pwzPath, DWORD cchSize) PURE;
    STDMETHOD(SetDirectory)(THIS_ _In_ PCWSTR pwzPath) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define ICurrentWorkingDirectory_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define ICurrentWorkingDirectory_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define ICurrentWorkingDirectory_Release(p)             (p)->lpVtbl->Release(p)
/*** ICurrentWorkingDirectory methods ***/
#define ICurrentWorkingDirectory_GetDirectory(p,a,b)    (p)->lpVtbl->GetDirectory(p,a,b)
#define ICurrentWorkingDirectory_SetDirectory(p,a)      (p)->lpVtbl->SetDirectory(p,a)
#endif

/* IACList interface */
#define INTERFACE IACList
DECLARE_INTERFACE_(IACList,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IACList methods ***/
    STDMETHOD(Expand)(THIS_ _In_ LPCOLESTR str) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IACList_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IACList_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IACList_Release(p)             (p)->lpVtbl->Release(p)
/*** IACList methods ***/
#define IACList_Expand(p,a)             (p)->lpVtbl->Expand(p,a)
#endif

/* IACList2 interface */

#define ACLO_NONE           0x00000000
#define ACLO_CURRENTDIR     0x00000001
#define ACLO_MYCOMPUTER     0x00000002
#define ACLO_DESKTOP        0x00000004
#define ACLO_FAVORITES      0x00000008
#define ACLO_FILESYSONLY    0x00000010
#define ACLO_FILESYSDIRS    0x00000020

#define INTERFACE IACList2
DECLARE_INTERFACE_(IACList2,IACList)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IACList methods ***/
    STDMETHOD(Expand)(THIS_ LPCOLESTR str) PURE;
    /*** IACList2 methods ***/
    STDMETHOD(SetOptions)(THIS_ DWORD dwFlag) PURE;
    STDMETHOD(GetOptions)(THIS_ _Out_ DWORD* pdwFlag) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IACList2_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IACList2_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IACList2_Release(p)             (p)->lpVtbl->Release(p)
/*** IACList2 methods ***/
#define IACList2_GetOptions(p,a)        (p)->lpVtbl->GetOptions(p,a)
#define IACList2_SetOptions(p,a)        (p)->lpVtbl->SetOptions(p,a)
#endif

/****************************************************************************
 * IShellFolderViewCB interface
 */

#define INTERFACE IShellFolderViewCB
DECLARE_INTERFACE_(IShellFolderViewCB,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IShellFolderViewCB methods ***/
    STDMETHOD(MessageSFVCB)(THIS_ UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IShellFolderViewCB_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellFolderViewCB_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IShellFolderViewCB_Release(p)                 (p)->lpVtbl->Release(p)
/*** IShellFolderViewCB methods ***/
#define IShellFolderViewCB_MessageSFVCB(p,a,b,c)      (p)->lpVtbl->MessageSFVCB(p,a,b,c)
#endif

/****************************************************************************
 * IShellFolderView interface
 */

#include <pshpack8.h>

typedef struct _ITEMSPACING
{
    int cxSmall;
    int cySmall;
    int cxLarge;
    int cyLarge;
} ITEMSPACING;

#include <poppack.h>

#define INTERFACE IShellFolderView
DEFINE_GUID(IID_IShellFolderView,0x37a378c0,0xf82d,0x11ce,0xae,0x65,0x08,0x00,0x2b,0x2e,0x12,0x62);
DECLARE_INTERFACE_(IShellFolderView, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /*** IShellFolderView methods ***/
    STDMETHOD(Rearrange) (THIS_ LPARAM lParamSort) PURE;
    STDMETHOD(GetArrangeParam) (THIS_ _Out_ LPARAM *plParamSort) PURE;
    STDMETHOD(ArrangeGrid) (THIS) PURE;
    STDMETHOD(AutoArrange) (THIS) PURE;
    STDMETHOD(GetAutoArrange) (THIS) PURE;
    STDMETHOD(AddObject) (THIS_ _In_ PITEMID_CHILD pidl, _Out_ UINT *puItem) PURE;
    STDMETHOD(GetObject) (THIS_ _Outptr_ PITEMID_CHILD *ppidl, UINT uItem) PURE;
    STDMETHOD(RemoveObject) (THIS_ _In_opt_ PITEMID_CHILD pidl, _Out_ UINT *puItem) PURE;
    STDMETHOD(GetObjectCount) (THIS_ _Out_ UINT *puCount) PURE;
    STDMETHOD(SetObjectCount) (THIS_ UINT uCount, UINT dwFlags) PURE;
    STDMETHOD(UpdateObject) (THIS_ _In_ PITEMID_CHILD pidlOld, _In_ PITEMID_CHILD pidlNew, _Out_ UINT *puItem) PURE;
    STDMETHOD(RefreshObject) (THIS_ _In_ PITEMID_CHILD pidl, _Out_ UINT *puItem) PURE;
    STDMETHOD(SetRedraw) (THIS_ BOOL bRedraw) PURE;
    STDMETHOD(GetSelectedCount) (THIS_ _Out_ UINT *puSelected) PURE;
    STDMETHOD(GetSelectedObjects) (THIS_ _Outptr_result_buffer_(*puItems) PCUITEMID_CHILD **pppidl, _Out_ UINT *puItems) PURE;
    STDMETHOD(IsDropOnSource) (THIS_ _In_opt_ IDropTarget *pDropTarget) PURE;
    STDMETHOD(GetDragPoint) (THIS_ _Out_ POINT *ppt) PURE;
    STDMETHOD(GetDropPoint) (THIS_ _Out_ POINT *ppt) PURE;
    STDMETHOD(MoveIcons) (THIS_ _In_ IDataObject *pDataObject) PURE;
    STDMETHOD(SetItemPos) (THIS_ _In_ PCUITEMID_CHILD pidl, _In_ POINT *ppt) PURE;
    STDMETHOD(IsBkDropTarget) (THIS_ _In_opt_ IDropTarget *pDropTarget) PURE;
    STDMETHOD(SetClipboard) (THIS_ BOOL bMove) PURE;
    STDMETHOD(SetPoints) (THIS_ _In_ IDataObject *pDataObject) PURE;
    STDMETHOD(GetItemSpacing) (THIS_ _Out_ ITEMSPACING *pSpacing) PURE;
    STDMETHOD(SetCallback) (THIS_ _In_opt_ IShellFolderViewCB* pNewCB, _Outptr_result_maybenull_ IShellFolderViewCB** ppOldCB) PURE;
    STDMETHOD(Select) ( THIS_  UINT dwFlags ) PURE;
    STDMETHOD(QuerySupport) (THIS_ _Inout_ UINT * pdwSupport) PURE;
    STDMETHOD(SetAutomationObject)(THIS_ _In_opt_ IDispatch* pdisp) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IShellFolderView_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellFolderView_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IShellFolderView_Release(p)                 (p)->lpVtbl->Release(p)
/*** IShellFolderView methods ***/
#define IShellFolderView_Rearrange(p,a)             (p)->lpVtbl->Rearrange(p,a)
#define IShellFolderView_GetArrangeParam(p,a)       (p)->lpVtbl->GetArrangeParam(p,a)
#define IShellFolderView_ArrangeGrid(p)             (p)->lpVtbl->ArrangeGrid(p)
#define IShellFolderView_AutoArrange(p)             (p)->lpVtbl->AutoArrange(p)
#define IShellFolderView_GetAutoArrange(p)          (p)->lpVtbl->GetAutoArrange(p)
#define IShellFolderView_AddObject(p,a,b)           (p)->lpVtbl->AddObject(p,a,b)
#define IShellFolderView_GetObject(p,a,b)           (p)->lpVtbl->GetObject(p,a,b)
#define IShellFolderView_RemoveObject(p,a,b)        (p)->lpVtbl->RemoveObject(p,a,b)
#define IShellFolderView_GetObjectCount(p,a)        (p)->lpVtbl->GetObjectCount(p,a)
#define IShellFolderView_SetObjectCount(p,a,b)      (p)->lpVtbl->SetObjectCount(p,a,b)
#define IShellFolderView_UpdateObject(p,a,b,c)      (p)->lpVtbl->UpdateObject(p,a,b,c)
#define IShellFolderView_RefreshObject(p,a,b)       (p)->lpVtbl->RefreshObject(p,a,b)
#define IShellFolderView_SetRedraw(p,a)             (p)->lpVtbl->SetRedraw(p,a)
#define IShellFolderView_GetSelectedCount(p,a)      (p)->lpVtbl->GetSelectedCount(p,a)
#define IShellFolderView_GetSelectedObjects(p,a,b)  (p)->lpVtbl->GetSelectedObjects(p,a,b)
#define IShellFolderView_IsDropOnSource(p,a)        (p)->lpVtbl->IsDropOnSource(p,a)
#define IShellFolderView_GetDragPoint(p,a)          (p)->lpVtbl->GetDragPoint(p,a)
#define IShellFolderView_GetDropPoint(p,a)          (p)->lpVtbl->GetDropPoint(p,a)
#define IShellFolderView_MoveIcons(p,a)             (p)->lpVtbl->MoveIcons(p,a)
#define IShellFolderView_SetItemPos(p,a,b)          (p)->lpVtbl->SetItemPos(p,a,b)
#define IShellFolderView_DropTarget(p,a)            (p)->lpVtbl->DropTarget(p,a)
#define IShellFolderView_SetClipboard(p,a)          (p)->lpVtbl->SetClipboard(p,a)
#define IShellFolderView_SetPoints(p,a)             (p)->lpVtbl->SetPoints(p,a)
#define IShellFolderView_GetItemSpacing(p,a)        (p)->lpVtbl->GetItemSpacing(p,a)
#define IShellFolderView_SetCallback(p,a)           (p)->lpVtbl->SetCallback(p,a)
#define IShellFolderView_Select(p,a)                (p)->lpVtbl->Select(p,a)
#define IShellFolderView_QuerySupport(p,a)          (p)->lpVtbl->QuerySupport(p,a)
#define IShellFolderView_SetAutomationObject(p,a)   (p)->lpVtbl->SetAutomationObject(p,a)
#endif

/* IProgressDialog interface */
#define PROGDLG_NORMAL           0x00000000
#define PROGDLG_MODAL            0x00000001
#define PROGDLG_AUTOTIME         0x00000002
#define PROGDLG_NOTIME           0x00000004
#define PROGDLG_NOMINIMIZE       0x00000008
#define PROGDLG_NOPROGRESSBAR    0x00000010
#define PROGDLG_MARQUEEPROGRESS  0x00000020
#define PROGDLG_NOCANCEL         0x00000040

#define PDTIMER_RESET            0x00000001
#define PDTIMER_PAUSE            0x00000002
#define PDTIMER_RESUME           0x00000003

#define INTERFACE IProgressDialog
DECLARE_INTERFACE_(IProgressDialog,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IProgressDialog methods ***/
    STDMETHOD(StartProgressDialog)(THIS_ _In_opt_ HWND hwndParent, _In_opt_ IUnknown *punkEnableModeless, DWORD dwFlags, _Reserved_ LPCVOID reserved) PURE;
    STDMETHOD(StopProgressDialog)(THIS) PURE;
    STDMETHOD(SetTitle)(THIS_ _In_ LPCWSTR pwzTitle) PURE;
    STDMETHOD(SetAnimation)(THIS_ _In_opt_ HINSTANCE hInstance, UINT uiResourceId) PURE;
    STDMETHOD_(BOOL,HasUserCancelled)(THIS) PURE;
    STDMETHOD(SetProgress)(THIS_ DWORD dwCompleted, DWORD dwTotal) PURE;
    STDMETHOD(SetProgress64)(THIS_ ULONGLONG ullCompleted, ULONGLONG ullTotal) PURE;
    STDMETHOD(SetLine)(THIS_ DWORD dwLineNum, _In_ LPCWSTR pwzString, BOOL bPath, _Reserved_ LPCVOID reserved) PURE;
    STDMETHOD(SetCancelMsg)(THIS_ _In_ LPCWSTR pwzCancelMsg, _Reserved_ LPCVOID reserved) PURE;
    STDMETHOD(Timer)(THIS_ DWORD dwTimerAction, _Reserved_ LPCVOID reserved) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IProgressDialog_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IProgressDialog_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IProgressDialog_Release(p)             (p)->lpVtbl->Release(p)
/*** IProgressDialog methods ***/
#define IProgressDialog_StartProgressDialog(p,a,b,c,d)    (p)->lpVtbl->StartProgressDialog(p,a,b,c,d)
#define IProgressDialog_StopProgressDialog(p)             (p)->lpVtbl->StopProgressDialog(p)
#define IProgressDialog_SetTitle(p,a)                     (p)->lpVtbl->SetTitle(p,a)
#define IProgressDialog_SetAnimation(p,a,b)               (p)->lpVtbl->SetAnimation(p,a,b)
#define IProgressDialog_HasUserCancelled(p)               (p)->lpVtbl->HasUserCancelled(p)
#define IProgressDialog_SetProgress(p,a,b)                (p)->lpVtbl->SetProgress(p,a,b)
#define IProgressDialog_SetProgress64(p,a,b)              (p)->lpVtbl->SetProgress64(p,a,b)
#define IProgressDialog_SetLine(p,a,b,c,d)                (p)->lpVtbl->SetLine(p,a,b,c,d)
#define IProgressDialog_SetCancelMsg(p,a,b)               (p)->lpVtbl->SetCancelMsg(p,a,b)
#define IProgressDialog_Timer(p,a,b)                      (p)->lpVtbl->Timer(p,a,b)
#endif


/* IDeskBarClient interface */
#define INTERFACE IDeskBarClient
DECLARE_INTERFACE_(IDeskBarClient,IOleWindow)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleWindow methods ***/
    STDMETHOD_(HRESULT,GetWindow)(THIS_ HWND*) PURE;
    STDMETHOD_(HRESULT,ContextSensitiveHelp)(THIS_ BOOL) PURE;
    /*** IDeskBarClient methods ***/
    STDMETHOD_(HRESULT,SetDeskBarSite)(THIS_ _In_opt_ IUnknown*) PURE;
    STDMETHOD_(HRESULT,SetModeDBC)(THIS_ DWORD) PURE;
    STDMETHOD_(HRESULT,UIActivateDBC)(THIS_ DWORD) PURE;
    STDMETHOD_(HRESULT,GetSize)(THIS_ DWORD, _Out_ LPRECT) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDeskBarClient_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define IDeskBarClient_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define IDeskBarClient_Release(p)                  (p)->lpVtbl->Release(p)
/*** IOleWindow methods ***/
#define IDeskBarClient_GetWindow(p,a)              (p)->lpVtbl->GetWindow(p,a)
#define IDeskBarClient_ContextSensitiveHelp(p,a)   (p)->lpVtbl->ContextSensitiveHelp(p,a)
/*** IOleWindow IDeskBarClient ***/
#define IDeskBarClient_SetDeskBarSite(p,a)         (p)->lpVtbl->SetDeskBarSite(p,a)
#define IDeskBarClient_SetModeDBC(p,a)             (p)->lpVtbl->SetModeDBC(p,a)
#define IDeskBarClient_UIActivateDBC(p,a)          (p)->lpVtbl->UIActivateDBC(p,a)
#define IDeskBarClient_GetSize(p,a,b)              (p)->lpVtbl->GetSize(p,a,b)
#endif

#define DBC_GS_IDEAL    0
#define DBC_GS_SIZEDOWN 1

#define DBC_HIDE        0
#define DBC_SHOW        1
#define DBC_SHOWOBSCURE 2


/* As indicated by the documentation for IActiveDesktop,
   you must include wininet.h before shlobj.h */
#ifdef _WINE_WININET_H_


/* Structs are taken from msdn, and not verified!
   Only stuff needed to make it compile are here, no flags or anything */

typedef struct _tagWALLPAPEROPT
{
    DWORD dwSize;
    DWORD dwStyle;
} WALLPAPEROPT;

typedef WALLPAPEROPT *LPWALLPAPEROPT;
typedef const WALLPAPEROPT *LPCWALLPAPEROPT;

typedef struct _tagCOMPONENTSOPT
{
    DWORD dwSize;
    BOOL  fEnableComponents;
    BOOL  fActiveDesktop;
} COMPONENTSOPT;

typedef COMPONENTSOPT *LPCOMPONENTSOPT;
typedef const COMPONENTSOPT *LPCCOMPONENTSOPT;


typedef struct _tagCOMPPOS
{
    DWORD dwSize;
    int   iLeft;
    int   iTop;
    DWORD dwWidth;
    DWORD dwHeight;
    int   izIndex;
    BOOL  fCanResize;
    BOOL  fCanResizeX;
    BOOL  fCanResizeY;
    int   iPreferredLeftPercent;
    int   iPreferredTopPercent;
} COMPPOS;

typedef struct _tagCOMPSTATEINFO
{
    DWORD dwSize;
    int   iLeft;
    int   iTop;
    DWORD dwWidth;
    DWORD dwHeight;
    DWORD dwItemState;
} COMPSTATEINFO;

typedef struct _tagCOMPONENT
{
    DWORD         dwSize;
    DWORD         dwID;
    int           iComponentType;
    BOOL          fChecked;
    BOOL          fDirty;
    BOOL          fNoScroll;
    COMPPOS       cpPos;
    WCHAR         wszFriendlyName[MAX_PATH];
    WCHAR         wszSource[INTERNET_MAX_URL_LENGTH];
    WCHAR         wszSubscribedURL[INTERNET_MAX_URL_LENGTH];
    DWORD         dwCurItemState;
    COMPSTATEINFO csiOriginal;
    COMPSTATEINFO csiRestored;
} COMPONENT;

typedef COMPONENT *LPCOMPONENT;
typedef const COMPONENT *LPCCOMPONENT;

#pragma push_macro("AddDesktopItem")
#undef AddDesktopItem

/* IDeskBarClient interface */
#define INTERFACE IActiveDesktop
DECLARE_INTERFACE_(IActiveDesktop, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ _In_ REFIID riid, _Outptr_ void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /*** IActiveDesktop methods ***/
    STDMETHOD(ApplyChanges)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(GetWallpaper)(THIS_ PWSTR pwszWallpaper, UINT cchWallpaper, DWORD dwFlags) PURE;
    STDMETHOD(SetWallpaper)(THIS_ PCWSTR pwszWallpaper, DWORD dwReserved) PURE;
    STDMETHOD(GetWallpaperOptions)(THIS_ LPWALLPAPEROPT pwpo, DWORD dwReserved) PURE;
    STDMETHOD(SetWallpaperOptions)(THIS_ LPCWALLPAPEROPT pwpo, DWORD dwReserved) PURE;
    STDMETHOD(GetPattern)(THIS_ PWSTR pwszPattern, UINT cchPattern, DWORD dwReserved) PURE;
    STDMETHOD(SetPattern)(THIS_ PCWSTR pwszPattern, DWORD dwReserved) PURE;
    STDMETHOD(GetDesktopItemOptions)(THIS_ LPCOMPONENTSOPT pco, DWORD dwReserved) PURE;
    STDMETHOD(SetDesktopItemOptions)(THIS_ LPCCOMPONENTSOPT pco, DWORD dwReserved) PURE;
    STDMETHOD(AddDesktopItem)(THIS_ LPCCOMPONENT pcomp, DWORD dwReserved) PURE;
    STDMETHOD(AddDesktopItemWithUI)(THIS_ HWND hwnd, LPCOMPONENT pcomp, DWORD dwReserved) PURE;
    STDMETHOD(ModifyDesktopItem)(THIS_ LPCCOMPONENT pcomp, DWORD dwFlags) PURE;
    STDMETHOD(RemoveDesktopItem)(THIS_ LPCCOMPONENT pcomp, DWORD dwReserved) PURE;
    STDMETHOD(GetDesktopItemCount)(THIS_ int *pcItems, DWORD dwReserved) PURE;
    STDMETHOD(GetDesktopItem)(THIS_ int nComponent, LPCOMPONENT pcomp, DWORD dwReserved) PURE;
    STDMETHOD(GetDesktopItemByID)(THIS_ ULONG_PTR dwID, LPCOMPONENT pcomp, DWORD dwReserved) PURE;
    STDMETHOD(GenerateDesktopItemHtml)(THIS_ PCWSTR pwszFileName, LPCOMPONENT pcomp, DWORD dwReserved) PURE;
    STDMETHOD(AddUrl)(THIS_ HWND hwnd, PCWSTR pszSource, LPCOMPONENT pcomp, DWORD dwFlags) PURE;
    STDMETHOD(GetDesktopItemBySource)(THIS_ PCWSTR pwszSource, LPCOMPONENT pcomp, DWORD dwReserved) PURE;
};
#undef INTERFACE

#pragma pop_macro("AddDesktopItem")

#endif

/****************************************************************************
* SHAddToRecentDocs API
*/
#define SHARD_PIDL      0x00000001L
#define SHARD_PATHA     0x00000002L
#define SHARD_PATHW     0x00000003L
#define SHARD_PATH WINELIB_NAME_AW(SHARD_PATH)

void WINAPI SHAddToRecentDocs(UINT, _In_opt_ LPCVOID);

/****************************************************************************
 * SHBrowseForFolder API
 */
typedef INT (CALLBACK *BFFCALLBACK)(HWND,UINT,LPARAM,LPARAM);

#include <pshpack8.h>

typedef struct tagBROWSEINFOA {
    HWND                hwndOwner;
    PCIDLIST_ABSOLUTE   pidlRoot;
    LPSTR               pszDisplayName;
    LPCSTR              lpszTitle;
    UINT                ulFlags;
    BFFCALLBACK         lpfn;
    LPARAM              lParam;
    INT                 iImage;
} BROWSEINFOA, *PBROWSEINFOA, *LPBROWSEINFOA;

typedef struct tagBROWSEINFOW {
    HWND                hwndOwner;
    PCIDLIST_ABSOLUTE   pidlRoot;
    LPWSTR              pszDisplayName;
    LPCWSTR             lpszTitle;
    UINT                ulFlags;
    BFFCALLBACK         lpfn;
    LPARAM              lParam;
    INT                 iImage;
} BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW;

#define BROWSEINFO   WINELIB_NAME_AW(BROWSEINFO)
#define PBROWSEINFO  WINELIB_NAME_AW(PBROWSEINFO)
#define LPBROWSEINFO WINELIB_NAME_AW(LPBROWSEINFO)

#include <poppack.h>

/* Browsing for directory. */
#define BIF_RETURNONLYFSDIRS   0x0001
#define BIF_DONTGOBELOWDOMAIN  0x0002
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008
#define BIF_EDITBOX            0x0010
#define BIF_VALIDATE           0x0020
#define BIF_NEWDIALOGSTYLE     0x0040
#define BIF_USENEWUI           (BIF_NEWDIALOGSTYLE | BIF_EDITBOX)
#define BIF_BROWSEINCLUDEURLS  0x0080
#define BIF_UAHINT             0x0100
#define BIF_NONEWFOLDERBUTTON  0x0200
#define BIF_NOTRANSLATETARGETS 0x0400

#define BIF_BROWSEFORCOMPUTER  0x1000
#define BIF_BROWSEFORPRINTER   0x2000
#define BIF_BROWSEINCLUDEFILES 0x4000
#define BIF_SHAREABLE          0x8000

/* message from browser */
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2
#define BFFM_VALIDATEFAILEDA    3
#define BFFM_VALIDATEFAILEDW    4
#define BFFM_IUNKNOWN           5

/* messages to browser */
#define BFFM_SETSTATUSTEXTA     (WM_USER+100)
#define BFFM_ENABLEOK           (WM_USER+101)
#define BFFM_SETSELECTIONA      (WM_USER+102)
#define BFFM_SETSELECTIONW      (WM_USER+103)
#define BFFM_SETSTATUSTEXTW     (WM_USER+104)
#define BFFM_SETOKTEXT          (WM_USER+105)
#define BFFM_SETEXPANDED        (WM_USER+106)

PIDLIST_ABSOLUTE WINAPI SHBrowseForFolderA(_In_ LPBROWSEINFOA lpbi);
PIDLIST_ABSOLUTE WINAPI SHBrowseForFolderW(_In_ LPBROWSEINFOW lpbi);
#define SHBrowseForFolder WINELIB_NAME_AW(SHBrowseForFolder)

#define BFFM_SETSTATUSTEXT  WINELIB_NAME_AW(BFFM_SETSTATUSTEXT)
#define BFFM_SETSELECTION   WINELIB_NAME_AW(BFFM_SETSELECTION)
#define BFFM_VALIDATEFAILED WINELIB_NAME_AW(BFFM_VALIDATEFAILED)

/**********************************************************************
 * SHCreateShellFolderViewEx API
 */

typedef HRESULT
(CALLBACK *LPFNVIEWCALLBACK)(
  _In_ IShellView* dwUser,
  _In_ IShellFolder* pshf,
  _In_ HWND hWnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam);

#include <pshpack8.h>

typedef struct _CSFV
{
  UINT                 cbSize;
  IShellFolder*        pshf;
  IShellView*          psvOuter;
  PCIDLIST_ABSOLUTE    pidl;
  LONG                 lEvents;
  LPFNVIEWCALLBACK     pfnCallback;
  FOLDERVIEWMODE       fvm;
} CSFV, *LPCSFV;

#include <poppack.h>

HRESULT
WINAPI
SHCreateShellFolderViewEx(
  _In_ LPCSFV pshfvi,
  _Outptr_ IShellView **ppshv);

/* SHCreateShellFolderViewEx callback messages */
#define SFVM_MERGEMENU                 1
#define SFVM_INVOKECOMMAND             2
#define SFVM_GETHELPTEXT               3
#define SFVM_GETTOOLTIPTEXT            4
#define SFVM_GETBUTTONINFO             5
#define SFVM_GETBUTTONS                6
#define SFVM_INITMENUPOPUP             7
#define SFVM_SELECTIONCHANGED          8 /* undocumented */
#define SFVM_DRAWMENUITEM              9 /* undocumented */
#define SFVM_MEASUREMENUITEM          10 /* undocumented */
#define SFVM_EXITMENULOOP             11 /* undocumented */
#define SFVM_VIEWRELEASE              12 /* undocumented */
#define SFVM_GETNAMELENGTH            13 /* undocumented */
#define SFVM_FSNOTIFY                 14
#define SFVM_WINDOWCREATED            15
#define SFVM_WINDOWCLOSING            16 /* undocumented */
#define SFVM_LISTREFRESHED            17 /* undocumented */
#define SFVM_WINDOWFOCUSED            18 /* undocumented */
#define SFVM_REGISTERCOPYHOOK         20 /* undocumented */
#define SFVM_COPYHOOKCALLBACK         21 /* undocumented */
#define SFVM_GETDETAILSOF             23
#define SFVM_COLUMNCLICK              24
#define SFVM_QUERYFSNOTIFY            25
#define SFVM_DEFITEMCOUNT             26
#define SFVM_DEFVIEWMODE              27
#define SFVM_UNMERGEFROMMENU          28
#define SFVM_ADDINGOBJECT             29 /* undocumented */
#define SFVM_REMOVINGOBJECT           30 /* undocumented */
#define SFVM_UPDATESTATUSBAR          31
#define SFVM_BACKGROUNDENUM           32
#define SFVM_GETCOMMANDDIR            33 /* undocumented */
#define SFVM_GETCOLUMNSTREAM          34 /* undocumented */
#define SFVM_CANSELECTALL             35 /* undocumented */
#define SFVM_DIDDRAGDROP              36
#define SFVM_ISSTRICTREFRESH          37 /* undocumented */
#define SFVM_ISCHILDOBJECT            38 /* undocumented */
#define SFVM_SETISFV                  39
#define SFVM_GETEXTVIEWS              40 /* undocumented */
#define SFVM_THISIDLIST               41
#define SFVM_ADDPROPERTYPAGES         47
#define SFVM_BACKGROUNDENUMDONE       48
#define SFVM_GETNOTIFY                49
#define SFVM_GETSORTDEFAULTS          53
#define SFVM_SIZE                     57
#define SFVM_GETZONE                  58
#define SFVM_GETPANE                  59
#define SFVM_GETHELPTOPIC             63
#define SFVM_GETANIMATION             68
#define SFVM_GET_CUSTOMVIEWINFO       77 /* undocumented */
#define SFVM_ENUMERATEDITEMS          79 /* undocumented */
#define SFVM_GET_VIEW_DATA            80 /* undocumented */
#define SFVM_GET_WEBVIEW_LAYOUT       82 /* undocumented */
#define SFVM_GET_WEBVIEW_CONTENT      83 /* undocumented */
#define SFVM_GET_WEBVIEW_TASKS        84 /* undocumented */
#define SFVM_GET_WEBVIEW_THEME        86 /* undocumented */
#define SFVM_GETDEFERREDVIEWSETTINGS  92 /* undocumented */

#include <pshpack8.h>

typedef struct _SFV_CREATE
{
    UINT cbSize;
    IShellFolder *pshf;
    IShellView *psvOuter;
    IShellFolderViewCB *psfvcb;
} SFV_CREATE;

#include <poppack.h>

HRESULT
WINAPI
SHCreateShellFolderView(
  _In_ const SFV_CREATE *pscfv,
  _Outptr_ IShellView **ppsv);

/* Types and definitions for the SFM_* parameters */
#include <pshpack8.h>

#define QCMINFO_PLACE_BEFORE          0
#define QCMINFO_PLACE_AFTER           1
typedef struct _QCMINFO_IDMAP_PLACEMENT
{
    UINT id;
    UINT fFlags;
} QCMINFO_IDMAP_PLACEMENT;

typedef struct _QCMINFO_IDMAP
{
    UINT nMaxIds;
    QCMINFO_IDMAP_PLACEMENT pIdList[1];
} QCMINFO_IDMAP;

typedef struct _QCMINFO
{
    HMENU hmenu;
    UINT indexMenu;
    UINT idCmdFirst;
    UINT idCmdLast;
    QCMINFO_IDMAP const* pIdMap;
} QCMINFO, *LPQCMINFO;

#define TBIF_DEFAULT           0x00000000
#define TBIF_APPEND            0x00000000
#define TBIF_PREPEND           0x00000001
#define TBIF_REPLACE           0x00000002
#define TBIF_INTERNETBAR       0x00010000
#define TBIF_STANDARDTOOLBAR   0x00020000
#define TBIF_NOTOOLBAR         0x00030000

typedef struct _TBINFO
{
    UINT cbuttons;
    UINT uFlags;
} TBINFO, *LPTBINFO;

#include <poppack.h>

/****************************************************************************
*	SHShellFolderView_Message API
*/

LRESULT
WINAPI
SHShellFolderView_Message(
  _In_ HWND hwndCabinet,
  UINT uMessage,
  LPARAM lParam);

/* SHShellFolderView_Message messages */
#define SFVM_REARRANGE          0x0001
#define SFVM_GETARRANGECOLUMN   0x0002 /* undocumented */
#define SFVM_ADDOBJECT          0x0003
#define SFVM_GETITEMCOUNT       0x0004 /* undocumented */
#define SFVM_GETITEMPIDL        0x0005 /* undocumented */
#define SFVM_REMOVEOBJECT       0x0006
#define SFVM_UPDATEOBJECT       0x0007
#define SFVM_SETREDRAW          0x0008 /* undocumented */
#define SFVM_GETSELECTEDOBJECTS 0x0009
#define SFVM_ISDROPONSOURCE     0x000A /* undocumented */
#define SFVM_MOVEICONS          0x000B /* undocumented */
#define SFVM_GETDRAGPOINT       0x000C /* undocumented */
#define SFVM_GETDROPPOINT       0x000D /* undocumented */
#define SFVM_SETITEMPOS         0x000E
#define SFVM_ISDROPONBACKGROUND 0x000F /* undocumented */
#define SFVM_SETCLIPBOARD       0x0010
#define SFVM_TOGGLEAUTOARRANGE  0x0011 /* undocumented */
#define SFVM_LINEUPICONS        0x0012 /* undocumented */
#define SFVM_GETAUTOARRANGE     0x0013 /* undocumented */
#define SFVM_GETSELECTEDCOUNT   0x0014 /* undocumented */
#define SFVM_GETITEMSPACING     0x0015 /* undocumented */
#define SFVM_REFRESHOBJECT      0x0016 /* undocumented */
#define SFVM_SETPOINTS          0x0017

/****************************************************************************
*	SHGetDataFromIDList API
*/
#define SHGDFIL_FINDDATA        1
#define SHGDFIL_NETRESOURCE     2
#define SHGDFIL_DESCRIPTIONID   3

#define SHDID_ROOT_REGITEM          1
#define SHDID_FS_FILE               2
#define SHDID_FS_DIRECTORY          3
#define SHDID_FS_OTHER              4
#define SHDID_COMPUTER_DRIVE35      5
#define SHDID_COMPUTER_DRIVE525     6
#define SHDID_COMPUTER_REMOVABLE    7
#define SHDID_COMPUTER_FIXED        8
#define SHDID_COMPUTER_NETDRIVE     9
#define SHDID_COMPUTER_CDROM        10
#define SHDID_COMPUTER_RAMDISK      11
#define SHDID_COMPUTER_OTHER        12
#define SHDID_NET_DOMAIN            13
#define SHDID_NET_SERVER            14
#define SHDID_NET_SHARE             15
#define SHDID_NET_RESTOFNET         16
#define SHDID_NET_OTHER             17
#define SHDID_COMPUTER_IMAGING      18
#define SHDID_COMPUTER_AUDIO        19
#define SHDID_COMPUTER_SHAREDDOCS   20

#include <pshpack8.h>

typedef struct _SHDESCRIPTIONID
{   DWORD   dwDescriptionId;
    CLSID   clsid;
} SHDESCRIPTIONID, *LPSHDESCRIPTIONID;

#include <poppack.h>

HRESULT
WINAPI
SHGetDataFromIDListA(
  _In_ LPSHELLFOLDER psf,
  _In_ PCUITEMID_CHILD pidl,
  int nFormat,
  _Out_writes_bytes_(cb) LPVOID pv,
  int cb);

HRESULT
WINAPI
SHGetDataFromIDListW(
  _In_ LPSHELLFOLDER psf,
  _In_ PCUITEMID_CHILD pidl,
  int nFormat,
  _Out_writes_bytes_(cb) LPVOID pv,
  int cb);

#define SHGetDataFromIDList WINELIB_NAME_AW(SHGetDataFromIDList)

PIDLIST_ABSOLUTE
WINAPI
SHCloneSpecialIDList(
  _Reserved_ HWND hwnd,
  _In_ int csidl,
  _In_ BOOL fCreate);

_Success_(return != 0)
BOOL
WINAPI
SHGetSpecialFolderPathA(
  _Reserved_ HWND hwndOwner,
  _Out_writes_(MAX_PATH) LPSTR szPath,
  _In_ int nFolder,
  _In_ BOOL bCreate);

_Success_(return != 0)
BOOL
WINAPI
SHGetSpecialFolderPathW(
  _Reserved_ HWND hwndOwner,
  _Out_writes_(MAX_PATH) LPWSTR szPath,
  _In_ int nFolder,
  _In_ BOOL bCreate);

#define SHGetSpecialFolderPath WINELIB_NAME_AW(SHGetSpecialFolderPath)

_Check_return_ HRESULT WINAPI SHGetMalloc(_Outptr_ LPMALLOC *lpmal);

/**********************************************************************
 * SHGetSetSettings ()
 */

typedef struct
{
    BOOL  fShowAllObjects : 1;
    BOOL  fShowExtensions : 1;
    BOOL  fNoConfirmRecycle : 1;

    BOOL  fShowSysFiles : 1;
    BOOL  fShowCompColor : 1;
    BOOL  fDoubleClickInWebView : 1;
    BOOL  fDesktopHTML : 1;
    BOOL  fWin95Classic : 1;
    BOOL  fDontPrettyPath : 1;
    BOOL  fShowAttribCol : 1;
    BOOL  fMapNetDrvBtn : 1;
    BOOL  fShowInfoTip : 1;
    BOOL  fHideIcons : 1;
    BOOL  fWebView : 1;
    BOOL  fFilter : 1;
    BOOL  fShowSuperHidden : 1;
    BOOL  fNoNetCrawling : 1;

    UINT  :15; /* Required for proper binary layout with gcc */
    DWORD dwWin95Unused;
    UINT  uWin95Unused;
    LONG  lParamSort;
    int   iSortDirection;
    UINT  version;
    UINT  uNotUsed;
    BOOL  fSepProcess: 1;
    BOOL  fStartPanelOn: 1;
    BOOL  fShowStartPage: 1;
    UINT  fSpareFlags : 13;
    UINT  :15; /* Required for proper binary layout with gcc */
} SHELLSTATE, *LPSHELLSTATE;

VOID WINAPI SHGetSetSettings(LPSHELLSTATE lpss, DWORD dwMask, BOOL bSet);

/**********************************************************************
 * SHGetSettings ()
 */
typedef struct
{	BOOL fShowAllObjects : 1;
	BOOL fShowExtensions : 1;
	BOOL fNoConfirmRecycle : 1;
	BOOL fShowSysFiles : 1;

	BOOL fShowCompColor : 1;
	BOOL fDoubleClickInWebView : 1;
	BOOL fDesktopHTML : 1;
	BOOL fWin95Classic : 1;

	BOOL fDontPrettyPath : 1;
	BOOL fShowAttribCol : 1;
	BOOL fMapNetDrvBtn : 1;
	BOOL fShowInfoTip : 1;

	BOOL fHideIcons : 1;
	UINT fRestFlags : 3;
	UINT :15; /* Required for proper binary layout with gcc */
} SHELLFLAGSTATE, * LPSHELLFLAGSTATE;

VOID WINAPI SHGetSettings(_Out_ LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

#define SSF_SHOWALLOBJECTS		0x0001
#define SSF_SHOWEXTENSIONS		0x0002
#define SSF_SHOWCOMPCOLOR		0x0008
#define SSF_SHOWSYSFILES		0x0020
#define SSF_DOUBLECLICKINWEBVIEW	0x0080
#define SSF_SHOWATTRIBCOL		0x0100
#define SSF_DESKTOPHTML			0x0200
#define SSF_WIN95CLASSIC		0x0400
#define SSF_DONTPRETTYPATH		0x0800
#define SSF_SHOWINFOTIP			0x2000
#define SSF_MAPNETDRVBUTTON		0x1000
#define SSF_NOCONFIRMRECYCLE		0x8000
#define SSF_HIDEICONS			0x4000
#define SSF_SHOWSUPERHIDDEN		0x00040000
#define SSF_SEPPROCESS			0x00080000

/****************************************************************************
* SHRestricted API
*/
typedef enum RESTRICTIONS
{
	REST_NONE			= 0x00000000,
	REST_NORUN			= 0x00000001,
	REST_NOCLOSE			= 0x00000002,
	REST_NOSAVESET			= 0x00000004,
	REST_NOFILEMENU			= 0x00000008,
	REST_NOSETFOLDERS		= 0x00000010,
	REST_NOSETTASKBAR		= 0x00000020,
	REST_NODESKTOP			= 0x00000040,
	REST_NOFIND			= 0x00000080,
	REST_NODRIVES			= 0x00000100,
	REST_NODRIVEAUTORUN		= 0x00000200,
	REST_NODRIVETYPEAUTORUN		= 0x00000400,
	REST_NONETHOOD			= 0x00000800,
	REST_STARTBANNER		= 0x00001000,
	REST_RESTRICTRUN		= 0x00002000,
	REST_NOPRINTERTABS		= 0x00004000,
	REST_NOPRINTERDELETE		= 0x00008000,
	REST_NOPRINTERADD		= 0x00010000,
	REST_NOSTARTMENUSUBFOLDERS	= 0x00020000,
	REST_MYDOCSONNET		= 0x00040000,
	REST_NOEXITTODOS		= 0x00080000,
	REST_ENFORCESHELLEXTSECURITY	= 0x00100000,
	REST_LINKRESOLVEIGNORELINKINFO	= 0x00200000,
	REST_NOCOMMONGROUPS		= 0x00400000,
	REST_SEPARATEDESKTOPPROCESS	= 0x00800000,
	REST_NOWEB			= 0x01000000,
	REST_NOTRAYCONTEXTMENU		= 0x02000000,
	REST_NOVIEWCONTEXTMENU		= 0x04000000,
	REST_NONETCONNECTDISCONNECT	= 0x08000000,
	REST_STARTMENULOGOFF		= 0x10000000,
	REST_NOSETTINGSASSIST		= 0x20000000,
	REST_NOINTERNETICON		= 0x40000001,
	REST_NORECENTDOCSHISTORY,
	REST_NORECENTDOCSMENU,
	REST_NOACTIVEDESKTOP,
	REST_NOACTIVEDESKTOPCHANGES,
	REST_NOFAVORITESMENU,
	REST_CLEARRECENTDOCSONEXIT,
	REST_CLASSICSHELL,
	REST_NOCUSTOMIZEWEBVIEW,

	REST_NOHTMLWALLPAPER		= 0x40000010,
	REST_NOCHANGINGWALLPAPER,
	REST_NODESKCOMP,
	REST_NOADDDESKCOMP,
	REST_NODELDESKCOMP,
	REST_NOCLOSEDESKCOMP,
	REST_NOCLOSE_DRAGDROPBAND,
	REST_NOMOVINGBAND,
	REST_NOEDITDESKCOMP,
	REST_NORESOLVESEARCH,
	REST_NORESOLVETRACK,
	REST_FORCECOPYACLWITHFILE,
#if (NTDDI_VERSION < NTDDI_LONGHORN)
	REST_NOLOGO3CHANNELNOTIFY	= 0x4000001C,
#endif
	REST_NOFORGETSOFTWAREUPDATE	= 0x4000001D,
	REST_NOSETACTIVEDESKTOP,
	REST_NOUPDATEWINDOWS,
	REST_NOCHANGESTARMENU,		/* 0x40000020 */
	REST_NOFOLDEROPTIONS,
	REST_HASFINDCOMPUTERS,
	REST_INTELLIMENUS,
	REST_RUNDLGMEMCHECKBOX,
	REST_ARP_ShowPostSetup,
	REST_NOCSC,
	REST_NOCONTROLPANEL,
	REST_ENUMWORKGROUP,
	REST_ARP_NOARP,
	REST_ARP_NOREMOVEPAGE,
	REST_ARP_NOADDPAGE,
	REST_ARP_NOWINSETUPPAGE,
	REST_GREYMSIADS,
	REST_NOCHANGEMAPPEDDRIVELABEL,
	REST_NOCHANGEMAPPEDDRIVECOMMENT,
	REST_MaxRecentDocs,		/* 0x40000030 */
	REST_NONETWORKCONNECTIONS,
	REST_FORCESTARTMENULOGOFF,
	REST_NOWEBVIEW,
	REST_NOCUSTOMIZETHISFOLDER,
	REST_NOENCRYPTION,

	REST_ALLOWFRENCHENCRYPTION,	/* not documented */

	REST_DONTSHOWSUPERHIDDEN,
	REST_NOSHELLSEARCHBUTTON,
	REST_NOHARDWARETAB,
	REST_NORUNASINSTALLPROMPT,
	REST_PROMPTRUNASINSTALLNETPATH,
	REST_NOMANAGEMYCOMPUTERVERB,
	REST_NORECENTDOCSNETHOOD,
	REST_DISALLOWRUN,
	REST_NOWELCOMESCREEN,
	REST_RESTRICTCPL,		/* 0x40000040 */
	REST_DISALLOWCPL,
	REST_NOSMBALLOONTIP,
	REST_NOSMHELP,
	REST_NOWINKEYS,
	REST_NOENCRYPTONMOVE,
	REST_NOLOCALMACHINERUN,
	REST_NOCURRENTUSERRUN,
	REST_NOLOCALMACHINERUNONCE,
	REST_NOCURRENTUSERRUNONCE,
	REST_FORCEACTIVEDESKTOPON,
	REST_NOCOMPUTERSNEARME,
	REST_NOVIEWONDRIVE,
	REST_NONETCRAWL,
	REST_NOSHAREDDOCUMENTS,
	REST_NOSMMYDOCS,
	REST_NOSMMYPICS,		/* 0x40000050 */
	REST_ALLOWBITBUCKDRIVES,
	REST_NONLEGACYSHELLMODE,
	REST_NOCONTROLPANELBARRICADE,
	REST_NOSTARTPAGE,
	REST_NOAUTOTRAYNOTIFY,
	REST_NOTASKGROUPING,
	REST_NOCDBURNING,
	REST_MYCOMPNOPROP,
	REST_MYDOCSNOPROP,
	REST_NOSTARTPANEL,
	REST_NODISPLAYAPPEARANCEPAGE,
	REST_NOTHEMESTAB,
	REST_NOVISUALSTYLECHOICE,
	REST_NOSIZECHOICE,
	REST_NOCOLORCHOICE,
	REST_SETVISUALSTYLE,		/* 0x40000060 */
	REST_STARTRUNNOHOMEPATH,
	REST_NOUSERNAMEINSTARTPANEL,
	REST_NOMYCOMPUTERICON,
	REST_NOSMNETWORKPLACES,
	REST_NOSMPINNEDLIST,
	REST_NOSMMYMUSIC,
	REST_NOSMEJECTPC,
	REST_NOSMMOREPROGRAMS,
	REST_NOSMMFUPROGRAMS,
	REST_NOTRAYITEMSDISPLAY,
	REST_NOTOOLBARSONTASKBAR,
	/* 0x4000006C
	   0x4000006D
	   0x4000006E */
	REST_NOSMCONFIGUREPROGRAMS	= 0x4000006F,
	REST_HIDECLOCK,			/* 0x40000070 */
	REST_NOLOWDISKSPACECHECKS,
	REST_NOENTIRENETWORK,
	REST_NODESKTOPCLEANUP,
	REST_BITBUCKNUKEONDELETE,
	REST_BITBUCKCONFIRMDELETE,
	REST_BITBUCKNOPROP,
	REST_NODISPBACKGROUND,
	REST_NODISPSCREENSAVEPG,
	REST_NODISPSETTINGSPG,
	REST_NODISPSCREENSAVEPREVIEW,
	REST_NODISPLAYCPL,
	REST_HIDERUNASVERB,
	REST_NOTHUMBNAILCACHE,
	REST_NOSTRCMPLOGICAL,
	REST_NOPUBLISHWIZARD,
	REST_NOONLINEPRINTSWIZARD,	/* 0x40000080 */
	REST_NOWEBSERVICES,
	REST_ALLOWUNHASHEDWEBVIEW,
	REST_ALLOWLEGACYWEBVIEW,
	REST_REVERTWEBVIEWSECURITY,

	REST_INHERITCONSOLEHANDLES	= 0x40000086,

	REST_NODISCONNECT		= 0x41000001,
	REST_NOSECURITY,
	REST_NOFILEASSOCIATE,		/* 0x41000003 */
} RESTRICTIONS;

DWORD WINAPI SHRestricted(RESTRICTIONS rest);

/****************************************************************************
* SHChangeNotify API
*/
typedef struct _SHChangeNotifyEntry
{
    PCIDLIST_ABSOLUTE pidl;
    BOOL   fRecursive;
} SHChangeNotifyEntry;

#define SHCNE_RENAMEITEM	0x00000001
#define SHCNE_CREATE		0x00000002
#define SHCNE_DELETE		0x00000004
#define SHCNE_MKDIR		0x00000008
#define SHCNE_RMDIR		0x00000010
#define SHCNE_MEDIAINSERTED	0x00000020
#define SHCNE_MEDIAREMOVED	0x00000040
#define SHCNE_DRIVEREMOVED	0x00000080
#define SHCNE_DRIVEADD		0x00000100
#define SHCNE_NETSHARE		0x00000200
#define SHCNE_NETUNSHARE	0x00000400
#define SHCNE_ATTRIBUTES	0x00000800
#define SHCNE_UPDATEDIR		0x00001000
#define SHCNE_UPDATEITEM	0x00002000
#define SHCNE_SERVERDISCONNECT	0x00004000
#define SHCNE_UPDATEIMAGE	0x00008000
#define SHCNE_DRIVEADDGUI	0x00010000
#define SHCNE_RENAMEFOLDER	0x00020000
#define SHCNE_FREESPACE		0x00040000

#define SHCNE_EXTENDED_EVENT	0x04000000
#define SHCNE_ASSOCCHANGED	0x08000000
#define SHCNE_DISKEVENTS	0x0002381F
#define SHCNE_GLOBALEVENTS	0x0C0581E0
#define SHCNE_ALLEVENTS		0x7FFFFFFF
#define SHCNE_INTERRUPT		0x80000000

#define SHCNEE_ORDERCHANGED	0x0002L
#define SHCNEE_MSI_CHANGE	0x0004L
#define SHCNEE_MSI_UNINSTALL	0x0005L

#define SHCNF_IDLIST		0x0000
#define SHCNF_PATHA		0x0001
#define SHCNF_PRINTERA		0x0002
#define SHCNF_DWORD		0x0003
#define SHCNF_PATHW		0x0005
#define SHCNF_PRINTERW		0x0006
#define SHCNF_TYPE		0x00FF
#define SHCNF_FLUSH		0x1000
#define SHCNF_FLUSHNOWAIT	0x3000
#define SHCNF_NOTIFYRECURSIVE	0x10000

#define SHCNF_PATH              WINELIB_NAME_AW(SHCNF_PATH)
#define SHCNF_PRINTER           WINELIB_NAME_AW(SHCNF_PRINTER)

#define SHCNRF_InterruptLevel 0x0001
#define SHCNRF_ShellLevel 0x0002
#define SHCNRF_RecursiveInterrupt 0x1000
#define SHCNRF_NewDelivery 0x8000

void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);

typedef enum {
    SLDF_DEFAULT = 0x00000000,
    SLDF_HAS_ID_LIST = 0x00000001,
    SLDF_HAS_LINK_INFO = 0x00000002,
    SLDF_HAS_NAME = 0x00000004,
    SLDF_HAS_RELPATH = 0x00000008,
    SLDF_HAS_WORKINGDIR = 0x00000010,
    SLDF_HAS_ARGS = 0x00000020,
    SLDF_HAS_ICONLOCATION = 0x00000040,
    SLDF_UNICODE = 0x00000080,
    SLDF_FORCE_NO_LINKINFO = 0x00000100,
    SLDF_HAS_EXP_SZ = 0x00000200,
    SLDF_RUN_IN_SEPARATE = 0x00000400,
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    SLDF_HAS_LOGO3ID = 0x00000800,
#endif
    SLDF_HAS_DARWINID = 0x00001000,
    SLDF_RUNAS_USER = 0x00002000,
    SLDF_HAS_EXP_ICON_SZ = 0x00004000,
    SLDF_NO_PIDL_ALIAS = 0x00008000,
    SLDF_FORCE_UNCNAME = 0x00010000,
    SLDF_RUN_WITH_SHIMLAYER = 0x00020000,
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    SLDF_FORCE_NO_LINKTRACK = 0x00040000,
    SLDF_ENABLE_TARGET_METADATA = 0x00080000,
    SLDF_DISABLE_LINK_PATH_TRACKING = 0x00100000,
    SLDF_DISABLE_KNOWNFOLDER_RELATIVE_TRACKING = 0x00200000,
#if (NTDDI_VERSION >= NTDDI_WIN7)
    SLDF_NO_KF_ALIAS = 0x00400000,
    SLDF_ALLOW_LINK_TO_LINK = 0x00800000,
    SLDF_UNALIAS_ON_SAVE = 0x01000000,
    SLDF_PREFER_ENVIRONMENT_PATH = 0x02000000,
    SLDF_KEEP_LOCAL_IDLIST_FOR_UNC_TARGET = 0x04000000,
#if (NTDDI_VERSION >= NTDDI_WIN8)
    SLDF_PERSIST_VOLUME_ID_RELATIVE = 0x08000000,
    SLDF_VALID = 0x0ffff7ff, /* Windows 8 */
#else
    SLDF_VALID = 0x07fff7ff, /* Windows 7 */
#endif
#else
    SLDF_VALID = 0x003ff7ff, /* Windows Vista */
#endif
#endif
    SLDF_RESERVED = 0x80000000,
} SHELL_LINK_DATA_FLAGS;

typedef struct tagDATABLOCKHEADER
{
    DWORD cbSize;
    DWORD dwSignature;
} DATABLOCK_HEADER, *LPDATABLOCK_HEADER, *LPDBLIST;

#ifdef LF_FACESIZE
typedef struct {
    DATABLOCK_HEADER dbh;
    WORD  wFillAttribute;
    WORD  wPopupFillAttribute;
    COORD dwScreenBufferSize;
    COORD dwWindowSize;
    COORD dwWindowOrigin;
    DWORD nFont;
    DWORD nInputBufferSize;
    COORD dwFontSize;
    UINT  uFontFamily;
    UINT  uFontWeight;
    WCHAR FaceName[LF_FACESIZE];
    UINT  uCursorSize;
    BOOL  bFullScreen;
    BOOL  bQuickEdit;
    BOOL  bInsertMode;
    BOOL  bAutoPosition;
    UINT  uHistoryBufferSize;
    UINT  uNumberOfHistoryBuffers;
    BOOL  bHistoryNoDup;
    COLORREF ColorTable[16];
} NT_CONSOLE_PROPS, *LPNT_CONSOLE_PROPS;
#endif

typedef struct {
    DATABLOCK_HEADER dbh;
    UINT uCodePage;
} NT_FE_CONSOLE_PROPS, *LPNT_FE_CONSOLE_PROPS;

typedef struct {
    DWORD cbSize;
    DWORD dwSignature;
    CHAR szTarget[MAX_PATH];
    WCHAR szwTarget[MAX_PATH];
} EXP_SZ_LINK, *LPEXP_SZ_LINK;

typedef struct {
    DATABLOCK_HEADER dbh;
    CHAR szDarwinID[MAX_PATH];
    WCHAR szwDarwinID[MAX_PATH];
} EXP_DARWIN_LINK, *LPEXP_DARWIN_LINK;

typedef struct {
    DWORD cbSize;
    DWORD dwSignature;
    DWORD idSpecialFolder;
    DWORD cbOffset;
} EXP_SPECIAL_FOLDER, *LPEXP_SPECIAL_FOLDER;

typedef struct {
    DWORD cbSize;
    DWORD dwSignature;
    BYTE  abPropertyStorage[1];
} EXP_PROPERTYSTORAGE;

#define EXP_SZ_LINK_SIG         0xA0000001 /* EXP_SZ_LINK */
#define NT_CONSOLE_PROPS_SIG    0xA0000002 /* NT_CONSOLE_PROPS */
#define NT_FE_CONSOLE_PROPS_SIG 0xA0000004 /* NT_FE_CONSOLE_PROPS */
#define EXP_SPECIAL_FOLDER_SIG  0xA0000005 /* EXP_SPECIAL_FOLDER */
#define EXP_DARWIN_ID_SIG       0xA0000006 /* EXP_DARWIN_LINK */
#if (NTDDI_VERSION < NTDDI_LONGHORN)
#define EXP_LOGO3_ID_SIG        0xA0000007 /* EXP_DARWIN_LINK, for Logo3 / MS Internet Component Download (MSICD) shortcuts; old SDKs only (deprecated) */
#endif
#define EXP_SZ_ICON_SIG         0xA0000007 /* EXP_SZ_LINK */
#define EXP_PROPERTYSTORAGE_SIG 0xA0000009 /* EXP_PROPERTYSTORAGE */

typedef struct _SHChangeDWORDAsIDList {
    USHORT   cb;
    DWORD    dwItem1;
    DWORD    dwItem2;
    USHORT   cbZero;
} SHChangeDWORDAsIDList, *LPSHChangeDWORDAsIDList;

typedef struct _SHChangeProductKeyAsIDList {
    USHORT cb;
    WCHAR wszProductKey[39];
    USHORT cbZero;
} SHChangeProductKeyAsIDList, *LPSHChangeProductKeyAsIDList;

ULONG
WINAPI
SHChangeNotifyRegister(
  _In_ HWND hwnd,
  int fSources,
  LONG fEvents,
  UINT wMsg,
  int cEntries,
  _In_ SHChangeNotifyEntry *pshcne);

BOOL WINAPI SHChangeNotifyDeregister(ULONG ulID);

HANDLE
WINAPI
SHChangeNotification_Lock(
  _In_ HANDLE hChangeNotification,
  DWORD dwProcessId,
  _Outptr_opt_result_buffer_(2)_Outptr_opt_result_buffer_(2) PIDLIST_ABSOLUTE **pppidl,
  _Out_opt_ LONG *plEvent);

BOOL WINAPI SHChangeNotification_Unlock(_In_ HANDLE hLock);

HRESULT
WINAPI
SHGetRealIDL(
  _In_ IShellFolder *psf,
  _In_ PCUITEMID_CHILD pidlSimple,
  _Outptr_ PITEMID_CHILD * ppidlReal);

/****************************************************************************
* SHCreateDirectory API
*/
int WINAPI SHCreateDirectory(_In_opt_ HWND, _In_ LPCWSTR);

int
WINAPI
SHCreateDirectoryExA(
  _In_opt_ HWND,
  _In_ LPCSTR,
  _In_opt_ LPSECURITY_ATTRIBUTES);

int
WINAPI
SHCreateDirectoryExW(
  _In_opt_ HWND,
  _In_ LPCWSTR,
  _In_opt_ LPSECURITY_ATTRIBUTES);

#define SHCreateDirectoryEx WINELIB_NAME_AW(SHCreateDirectoryEx)

/****************************************************************************
* SHGetSpecialFolderLocation API
*/
_Check_return_
HRESULT
WINAPI
SHGetSpecialFolderLocation(
  _Reserved_ HWND hwndOwner,
  _In_ int nFolder,
  _Outptr_ PIDLIST_ABSOLUTE *ppidl);

HRESULT
WINAPI
SHGetFolderLocation(
  _Reserved_ HWND hwndOwner,
  _In_ int nFolder,
  _In_opt_ HANDLE hToken,
  _In_ DWORD dwReserved,
  _Outptr_ PIDLIST_ABSOLUTE *ppidl);

/****************************************************************************
* SHGetFolderPath API
*/
typedef enum {
    SHGFP_TYPE_CURRENT = 0,
    SHGFP_TYPE_DEFAULT = 1
} SHGFP_TYPE;

HRESULT
WINAPI
SHGetFolderPathA(
  _Reserved_ HWND hwnd,
  _In_ int nFolder,
  _In_opt_ HANDLE hToken,
  _In_ DWORD dwFlags,
  _Out_writes_(MAX_PATH) LPSTR pszPath);

HRESULT
WINAPI
SHGetFolderPathW(
  _Reserved_ HWND hwnd,
  _In_ int nFolder,
  _In_opt_ HANDLE hToken,
  _In_ DWORD dwFlags,
  _Out_writes_(MAX_PATH) LPWSTR pszPath);

#define SHGetFolderPath WINELIB_NAME_AW(SHGetFolderPath)

#define CSIDL_DESKTOP		0x0000
#define CSIDL_INTERNET		0x0001
#define CSIDL_PROGRAMS		0x0002
#define CSIDL_CONTROLS		0x0003
#define CSIDL_PRINTERS		0x0004
#define CSIDL_PERSONAL		0x0005
#define CSIDL_FAVORITES		0x0006
#define CSIDL_STARTUP		0x0007
#define CSIDL_RECENT		0x0008
#define CSIDL_SENDTO		0x0009
#define CSIDL_BITBUCKET		0x000a
#define CSIDL_STARTMENU		0x000b
#define CSIDL_MYDOCUMENTS	CSIDL_PERSONAL
#define CSIDL_MYMUSIC		0x000d
#define CSIDL_MYVIDEO		0x000e
#define CSIDL_DESKTOPDIRECTORY	0x0010
#define CSIDL_DRIVES		0x0011
#define CSIDL_NETWORK		0x0012
#define CSIDL_NETHOOD		0x0013
#define CSIDL_FONTS		0x0014
#define CSIDL_TEMPLATES		0x0015
#define CSIDL_COMMON_STARTMENU	0x0016
#define CSIDL_COMMON_PROGRAMS	0X0017
#define CSIDL_COMMON_STARTUP	0x0018
#define CSIDL_COMMON_DESKTOPDIRECTORY	0x0019
#define CSIDL_APPDATA		0x001a
#define CSIDL_PRINTHOOD		0x001b
#define CSIDL_LOCAL_APPDATA	0x001c
#define CSIDL_ALTSTARTUP	0x001d
#define CSIDL_COMMON_ALTSTARTUP	0x001e
#define CSIDL_COMMON_FAVORITES  0x001f
#define CSIDL_INTERNET_CACHE	0x0020
#define CSIDL_COOKIES		0x0021
#define CSIDL_HISTORY		0x0022
#define CSIDL_COMMON_APPDATA	0x0023
#define CSIDL_WINDOWS		0x0024
#define CSIDL_SYSTEM		0x0025
#define CSIDL_PROGRAM_FILES	0x0026
#define CSIDL_MYPICTURES	0x0027
#define CSIDL_PROFILE		0x0028
#define CSIDL_SYSTEMX86		0x0029
#define CSIDL_PROGRAM_FILESX86	0x002a
#define CSIDL_PROGRAM_FILES_COMMON	0x002b
#define CSIDL_PROGRAM_FILES_COMMONX86	0x002c
#define CSIDL_COMMON_TEMPLATES	0x002d
#define CSIDL_COMMON_DOCUMENTS	0x002e
#define CSIDL_COMMON_ADMINTOOLS	0x002f
#define CSIDL_ADMINTOOLS	0x0030
#define CSIDL_CONNECTIONS	0x0031
#define CSIDL_COMMON_MUSIC	0x0035
#define CSIDL_COMMON_PICTURES	0x0036
#define CSIDL_COMMON_VIDEO	0x0037
#define CSIDL_RESOURCES		0x0038
#define CSIDL_RESOURCES_LOCALIZED 0x0039
#define CSIDL_COMMON_OEM_LINKS	0x003a
#define CSIDL_CDBURN_AREA	0x003b
#define CSIDL_COMPUTERSNEARME	0x003d
#define CSIDL_PROFILES		0x003e
#define CSIDL_FOLDER_MASK	0x00ff
#define CSIDL_FLAG_PER_USER_INIT 0x0800
#define CSIDL_FLAG_NO_ALIAS	0x1000
#define CSIDL_FLAG_DONT_VERIFY	0x4000
#define CSIDL_FLAG_CREATE	0x8000

#define CSIDL_FLAG_MASK		0xff00

/****************************************************************************
 * SHGetDesktopFolder API
 */
_Check_return_ HRESULT WINAPI SHGetDesktopFolder(_Outptr_ IShellFolder * *);

/****************************************************************************
 * SHBindToParent API
 */
HRESULT
WINAPI
SHBindToParent(
  _In_ PCIDLIST_ABSOLUTE pidl,
  _In_ REFIID riid,
  _Outptr_ LPVOID *ppv,
  _Outptr_opt_ PCUITEMID_CHILD *ppidlLast);

/****************************************************************************
 * SHCreateFileExtractIcon API
 */
#if (NTDDI_VERSION >= NTDDI_WINXP)

// NOTE: Even if documented on MSDN, the SHCreateFileExtractIconA()
// ANSI function never existed on Windows!

HRESULT
WINAPI
SHCreateFileExtractIconW(
    _In_ LPCWSTR pszFile,
    _In_ DWORD dwFileAttributes,
    _In_ REFIID riid,
    _Outptr_ void **ppv);

#ifdef UNICODE
#define SHCreateFileExtractIcon  SHCreateFileExtractIconW
#endif

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

/****************************************************************************
* SHDefExtractIcon API
*/
HRESULT
WINAPI
SHDefExtractIconA(
  _In_ LPCSTR pszIconFile,
  int iIndex,
  UINT uFlags,
  _Out_opt_ HICON* phiconLarge,
  _Out_opt_ HICON* phiconSmall,
  UINT nIconSize);

HRESULT
WINAPI
SHDefExtractIconW(
  _In_ LPCWSTR pszIconFile,
  int iIndex,
  UINT uFlags,
  _Out_opt_ HICON* phiconLarge,
  _Out_opt_ HICON* phiconSmall,
  UINT nIconSize);

#define SHDefExtractIcon WINELIB_NAME_AW(SHDefExtractIcon)

/*
 * DROPFILES for CF_HDROP and CF_PRINTERS
 */
typedef struct _DROPFILES
{
  DWORD pFiles;
  POINT pt;
  BOOL  fNC;
  BOOL  fWide;
} DROPFILES, *LPDROPFILES;


/*
 * FILEDESCRIPTOR[A|W].dwFlags
 */
#define FD_CLSID        0x00000001
#define FD_SIZEPOINT    0x00000002
#define FD_ATTRIBUTES   0x00000004
#define FD_CREATETIME   0x00000008
#define FD_ACCESSTIME   0x00000010
#define FD_WRITESTIME   0x00000020
#define FD_FILESIZE     0x00000040
#define FD_PROGRESSUI   0x00004000
#define FD_LINKUI       0x00008000
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define FD_UNICODE      0x80000000
#endif

/*
 * Properties of a file in the clipboard
 */
typedef struct _FILEDESCRIPTORA {
    DWORD dwFlags;
    CLSID clsid;
    SIZEL sizel;
    POINTL pointl;
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    CHAR cFileName[MAX_PATH];
} FILEDESCRIPTORA, *LPFILEDESCRIPTORA;

typedef struct _FILEDESCRIPTORW {
    DWORD dwFlags;
    CLSID clsid;
    SIZEL sizel;
    POINTL pointl;
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    WCHAR cFileName[MAX_PATH];
} FILEDESCRIPTORW, *LPFILEDESCRIPTORW;

DECL_WINELIB_TYPE_AW(FILEDESCRIPTOR)
DECL_WINELIB_TYPE_AW(LPFILEDESCRIPTOR)

/*
 * CF_FILEGROUPDESCRIPTOR clipboard format
 */
typedef struct _FILEGROUPDESCRIPTORA {
    UINT cItems;
    FILEDESCRIPTORA fgd[1];
} FILEGROUPDESCRIPTORA, *LPFILEGROUPDESCRIPTORA;

typedef struct _FILEGROUPDESCRIPTORW {
    UINT cItems;
    FILEDESCRIPTORW fgd[1];
} FILEGROUPDESCRIPTORW, *LPFILEGROUPDESCRIPTORW;

DECL_WINELIB_TYPE_AW(FILEGROUPDESCRIPTOR)
DECL_WINELIB_TYPE_AW(LPFILEGROUPDESCRIPTOR)

/****************************************************************************
 * Cabinet functions
 */

typedef struct {
    WORD cLength;
    WORD nVersion;
    BOOL fFullPathTitle:1;
    BOOL fSaveLocalView:1;
    BOOL fNotShell:1;
    BOOL fSimpleDefault:1;
    BOOL fDontShowDescBar:1;
    BOOL fNewWindowMode:1;
    BOOL fShowCompColor:1;
    BOOL fDontPrettyNames:1;
    BOOL fAdminsCreateCommonGroups:1;
    UINT fUnusedFlags:7;
    UINT :15; /* Required for proper binary layout with gcc */
    UINT fMenuEnumFilter;
} CABINETSTATE, *LPCABINETSTATE;

#define CABINETSTATE_VERSION 2

BOOL
WINAPI
ReadCabinetState(
  _Out_writes_bytes_(cLength) CABINETSTATE *,
  int cLength);

BOOL WINAPI WriteCabinetState(_In_ CABINETSTATE *);

/****************************************************************************
 * Path Manipulation Routines
 */

/* PathProcessCommand flags */
#define PPCF_ADDQUOTES        0x01
#define PPCF_INCLUDEARGS      0x02
#define PPCF_ADDARGUMENTS     0x03
#define PPCF_NODIRECTORIES    0x10
#define PPCF_DONTRESOLVE      0x20
#define PPCF_FORCEQUALIFY     0x40
#define PPCF_LONGESTPOSSIBLE  0x80

/* PathResolve flags */
#define PRF_VERIFYEXISTS         0x01
#define PRF_EXECUTABLE           0x02
#define PRF_TRYPROGRAMEXTENSIONS (PRF_EXECUTABLE | PRF_VERIFYEXISTS)
#define PRF_FIRSTDIRDEF          0x04
#define PRF_DONTFINDLNK          0x08 // Used when PRF_TRYPROGRAMEXTENSIONS is specified
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
#define PRF_REQUIREABSOLUTE      0x10
#endif

VOID WINAPI PathGetShortPath(_Inout_updates_(MAX_PATH) LPWSTR pszPath);

LONG
WINAPI
PathProcessCommand(
  _In_ LPCWSTR,
  _Out_writes_(cchDest) LPWSTR,
  int cchDest,
  DWORD);

_Success_(return != 0)
BOOL
WINAPI
PathYetAnotherMakeUniqueName(
  _Out_writes_(MAX_PATH) LPWSTR,
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR);

/****************************************************************************
 * Drag And Drop Routines
 */

/* DAD_AutoScroll sample structure */
#define NUM_POINTS 3
typedef struct
{
    int   iNextSample;
    DWORD dwLastScroll;
    BOOL  bFull;
    POINT pts[NUM_POINTS];
    DWORD dwTimes[NUM_POINTS];
} AUTO_SCROLL_DATA;

BOOL         WINAPI DAD_SetDragImage(HIMAGELIST,LPPOINT);
BOOL         WINAPI DAD_DragEnterEx(HWND,POINT);
BOOL         WINAPI DAD_DragEnterEx2(_In_ HWND, POINT, _In_opt_ IDataObject*);
BOOL         WINAPI DAD_DragMove(POINT);
BOOL         WINAPI DAD_DragLeave(void);
BOOL         WINAPI DAD_AutoScroll(HWND,AUTO_SCROLL_DATA*,const POINT*);

HRESULT
WINAPI
SHDoDragDrop(
  _In_opt_ HWND,
  _In_ IDataObject*,
  _In_opt_ IDropSource*,
  _In_ DWORD,
  _Out_ LPDWORD);

/****************************************************************************
 * Internet shortcut properties
 */

#define PID_IS_URL         2
#define PID_IS_NAME        4
#define PID_IS_WORKINGDIR  5
#define PID_IS_HOTKEY      6
#define PID_IS_SHOWCMD     7
#define PID_IS_ICONINDEX   8
#define PID_IS_ICONFILE    9
#define PID_IS_WHATSNEW    10
#define PID_IS_AUTHOR      11
#define PID_IS_DESCRIPTION 12
#define PID_IS_COMMENT     13


PIDLIST_RELATIVE WINAPI ILAppendID(_In_opt_ PIDLIST_RELATIVE, _In_ LPCSHITEMID, BOOL);
PIDLIST_RELATIVE WINAPI ILClone(_In_ PCUIDLIST_RELATIVE);
PITEMID_CHILD WINAPI ILCloneFirst(_In_ PCUIDLIST_RELATIVE);
PIDLIST_ABSOLUTE WINAPI ILCreateFromPathA(_In_ PCSTR);
PIDLIST_ABSOLUTE WINAPI ILCreateFromPathW(_In_ PCWSTR);
#define             ILCreateFromPath WINELIB_NAME_AW(ILCreateFromPath)
PIDLIST_ABSOLUTE WINAPI ILCombine(_In_opt_ PCIDLIST_ABSOLUTE, _In_opt_ PCUIDLIST_RELATIVE);
PUIDLIST_RELATIVE WINAPI ILFindChild(_In_ PIDLIST_ABSOLUTE, _In_ PCIDLIST_ABSOLUTE);
PUITEMID_CHILD WINAPI ILFindLastID(_In_ PCUIDLIST_RELATIVE);
void         WINAPI ILFree(_In_opt_ PIDLIST_RELATIVE);
PUIDLIST_RELATIVE WINAPI ILGetNext(_In_opt_ PCUIDLIST_RELATIVE);
UINT         WINAPI ILGetSize(_In_opt_ PCUIDLIST_RELATIVE);
BOOL         WINAPI ILIsEqual(_In_ PCIDLIST_ABSOLUTE, _In_ PCIDLIST_ABSOLUTE);
BOOL         WINAPI ILIsParent(_In_ PCIDLIST_ABSOLUTE, _In_ PCIDLIST_ABSOLUTE, BOOL);
HRESULT      WINAPI ILLoadFromStream(_In_ LPSTREAM, _Inout_ PIDLIST_RELATIVE*);
BOOL         WINAPI ILRemoveLastID(_Inout_opt_ PUIDLIST_RELATIVE);
HRESULT      WINAPI ILSaveToStream(_In_ LPSTREAM, _In_ PCUIDLIST_RELATIVE);

static inline BOOL ILIsEmpty(_In_opt_ PCUIDLIST_RELATIVE pidl)
{
    return !(pidl && pidl->mkid.cb);
}

#include <poppack.h>


/* menu merging */
#define MM_ADDSEPARATOR         0x00000001L
#define MM_SUBMENUSHAVEIDS      0x00000002L
#define MM_DONTREMOVESEPS       0x00000004L

UINT
WINAPI
Shell_MergeMenus(
  _In_ HMENU hmDst,
  _In_ HMENU hmSrc,
  UINT uInsert,
  UINT uIDAdjust,
  UINT uIDAdjustMax,
  ULONG uFlags);


/****************************************************************************
 * SHCreateDefaultContextMenu API
 */

typedef struct
{
  HWND hwnd;
  IContextMenuCB *pcmcb;
  PCIDLIST_ABSOLUTE pidlFolder;
  IShellFolder *psf;
  UINT cidl;
  PCUITEMID_CHILD_ARRAY apidl;
  IUnknown *punkAssociationInfo;
  UINT cKeys;
  const HKEY *aKeys;
}DEFCONTEXTMENU;

HRESULT
WINAPI
SHCreateDefaultContextMenu(
  _In_ const DEFCONTEXTMENU *,
  _In_ REFIID,
  _Outptr_ void **ppv);

typedef HRESULT
(CALLBACK * LPFNDFMCALLBACK)(
  _In_opt_ IShellFolder*,
  _In_opt_ HWND,
  _In_opt_ IDataObject*,
  UINT,
  WPARAM,
  LPARAM);

HRESULT
WINAPI
CDefFolderMenu_Create2(
  _In_opt_ PCIDLIST_ABSOLUTE,
  _In_opt_ HWND,
  UINT cidl,
  _In_reads_opt_(cidl) PCUITEMID_CHILD_ARRAY,
  _In_opt_ IShellFolder*,
  _In_opt_ LPFNDFMCALLBACK,
  UINT nKeys,
  _In_reads_opt_(nKeys) const HKEY *,
  _Outptr_ IContextMenu **);

#define DFM_MERGECONTEXTMENU         1
#define DFM_INVOKECOMMAND            2
#define DFM_INVOKECOMMANDEX          12
#define DFM_GETDEFSTATICID           14

#define DFM_GETHELPTEXT              5
#define DFM_WM_MEASUREITEM           6
#define DFM_WM_DRAWITEM              7
#define DFM_WM_INITMENUPOPUP         8
#define DFM_VALIDATECMD              9
#define DFM_MERGECONTEXTMENU_TOP    10
#define DFM_GETHELPTEXTW            11
#define DFM_MAPCOMMANDNAME          13
#define DFM_GETVERBW                15
#define DFM_GETVERBA                16
#define DFM_MERGECONTEXTMENU_BOTTOM 17
#define DFM_MODIFYQCMFLAGS          18


#define DFM_CMD_DELETE          ((UINT)-1)
#define DFM_CMD_MOVE            ((UINT)-2)
#define DFM_CMD_COPY            ((UINT)-3)
#define DFM_CMD_LINK            ((UINT)-4)
#define DFM_CMD_PROPERTIES      ((UINT)-5)
#define DFM_CMD_NEWFOLDER       ((UINT)-6)
#define DFM_CMD_PASTE           ((UINT)-7)
#define DFM_CMD_VIEWLIST        ((UINT)-8)
#define DFM_CMD_VIEWDETAILS     ((UINT)-9)
#define DFM_CMD_PASTELINK       ((UINT)-10)
#define DFM_CMD_PASTESPECIAL    ((UINT)-11)
#define DFM_CMD_MODALPROP       ((UINT)-12)
#define DFM_CMD_RENAME          ((UINT)-13)

/****************************************************************************
 * SHCreateDefaultExtractIcon API
 */

HRESULT WINAPI
SHCreateDefaultExtractIcon(
  REFIID riid,
  void **ppv);
/****************************************************************************
 * SHCreateDataObject API
 */

HRESULT WINAPI SHCreateDataObject(
  _In_opt_ PCIDLIST_ABSOLUTE pidlFolder,
  _In_ UINT cidl,
  _In_reads_opt_(cidl) PCUITEMID_CHILD_ARRAY apidl,
  _In_opt_ IDataObject *pdtInner,
  _In_ REFIID riid,
  _Outptr_ void **ppv);

/****************************************************************************
 * CIDLData_CreateFromIDArray API
 */

HRESULT WINAPI CIDLData_CreateFromIDArray(
  _In_ PCIDLIST_ABSOLUTE pidlFolder,
  _In_ UINT cidl,
  _In_reads_opt_(cidl) PCUIDLIST_RELATIVE_ARRAY apidl,
  _Outptr_ IDataObject **ppdtobj);

/****************************************************************************
 * SHRunControlPanel
 */

BOOL
WINAPI
SHRunControlPanel(
  _In_ LPCWSTR commandLine,
  _In_opt_ HWND parent);

/****************************************************************************
 * SHGetAttributesFromDataObject
 */

HRESULT
WINAPI
SHGetAttributesFromDataObject(
    _In_opt_ IDataObject* pdo,
    DWORD dwAttributeMask,
    _Out_opt_ DWORD* pdwAttributes,
    _Out_opt_ UINT* pcItems);

/****************************************************************************
 * SHOpenWithDialog
 */

enum tagOPEN_AS_INFO_FLAGS
{
	OAIF_ALLOW_REGISTRATION = 1,
	OAIF_REGISTER_EXT       = 2,
	OAIF_EXEC               = 4,
	OAIF_FORCE_REGISTRATION = 8,
#if (NTDDI_VERSION >= NTDDI_VISTA)
	OAIF_HIDE_REGISTRATION  = 32,
	OAIF_URL_PROTOCOL       = 64,
#endif
};
typedef int OPEN_AS_INFO_FLAGS;


typedef struct tagOPENASINFO {
	LPCWSTR pcszFile;
	LPCWSTR pcszClass;
	OPEN_AS_INFO_FLAGS oaifInFlags;
} OPENASINFO;

HRESULT
WINAPI
SHOpenWithDialog(
  _In_opt_ HWND hwndParent,
  _In_ const OPENASINFO *poainfo);

#define INTERFACE   IShellIconOverlayIdentifier

DECLARE_INTERFACE_(IShellIconOverlayIdentifier, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD (IsMemberOf)(THIS_ _In_ LPCWSTR pwszPath, DWORD dwAttrib) PURE;
    STDMETHOD (GetOverlayInfo)(THIS_ _Out_writes_(cchMax) LPWSTR pwszIconFile, int cchMax, _Out_ int * pIndex, _Out_ DWORD * pdwFlags) PURE;
    STDMETHOD (GetPriority)(THIS_ _Out_ int * pIPriority) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IShellIconOverlayIdentifier_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellIconOverlayIdentifier_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IShellIconOverlayIdentifier_Release(p)                    (p)->lpVtbl->Release(p)
/*** IShellIconOverlayIdentifier methods ***/
#define IShellIconOverlayIdentifier_IsMemberOf(p,a,b)             (p)->lpVtbl->IsMemberOf(p,a,b)
#define IShellIconOverlayIdentifier_GetOverlayInfo(p,a,b,c,d)     (p)->lpVtbl->GetOverlayInfo(p,a,b,c,d)
#define IShellIconOverlayIdentifier_GetPriority(p,a)              (p)->lpVtbl->GetPriority(p,a)
#endif

#define ISIOI_ICONFILE  0x00000001
#define ISIOI_ICONINDEX 0x00000002

#undef INTERFACE

/****************************************************************************
 * Travel log
 */

#define TLOG_BACK  -1
#define TLOG_FORE   1

#define TLMENUF_INCLUDECURRENT      0x00000001
#define TLMENUF_CHECKCURRENT        (TLMENUF_INCLUDECURRENT | 0x00000002)
#define TLMENUF_BACK                0x00000010  // Default
#define TLMENUF_FORE                0x00000020
#define TLMENUF_BACKANDFORTH        (TLMENUF_BACK | TLMENUF_FORE | TLMENUF_INCLUDECURRENT)

/*****************************************************************************
 * IDockingWindowSite interface
 */
#define INTERFACE IDockingWindowSite
DECLARE_INTERFACE_(IDockingWindowSite, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(THIS_ HWND *lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL fEnterMode) PURE;

    // *** IDockingWindowSite methods ***
    STDMETHOD(GetBorderDW)(THIS_ _In_ IUnknown *punkObj, _Out_ LPRECT prcBorder) PURE;
    STDMETHOD(RequestBorderSpaceDW)(THIS_ _In_ IUnknown *punkObj, _In_ LPCBORDERWIDTHS pbw) PURE;
    STDMETHOD(SetBorderSpaceDW)(THIS_ _In_ IUnknown *punkObj, _In_ LPCBORDERWIDTHS pbw) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDockingWindowSite_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define IDockingWindowSite_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IDockingWindowSite_Release(p)                    (p)->lpVtbl->Release(p)
/*** IOleWindow methods ***/
#define IDockingWindowSite_GetWindow(p,a)                (p)->lpVtbl->GetWindow(p,a)
#define IDockingWindowSite_ContextSensitiveHelp(p,a)     (p)->lpVtbl->ContextSensitiveHelp(p,a)
/*** IDockingWindowSite methods ***/
#define IDockingWindowSite_GetBorderDW(p,a,b)            (p)->lpVtbl->GetBorderDW(p,a,b)
#define IDockingWindowSite_RequestBorderSpaceDW(p,a,b)   (p)->lpVtbl->RequestBorderSpaceDW(p,a,b)
#define IDockingWindowSite_SetBorderSpaceDW(p,a,b)       (p)->lpVtbl->SetBorderSpaceDW(p,a,b)
#endif

typedef void (CALLBACK *PFNASYNCICONTASKBALLBACK)(LPCITEMIDLIST pidl, LPVOID pvData, LPVOID pvHint, INT iIconIndex, INT iOpenIconIndex);

#define ISFB_MASK_STATE       0x00000001
#define ISFB_MASK_IDLIST      0x00000010

#define ISFB_STATE_QLINKSMODE 0x00000020
#define ISFB_STATE_NOSHOWTEXT 0x00000004

#include <pshpack8.h>

typedef struct {
    DWORD       dwMask;
    DWORD       dwStateMask;
    DWORD       dwState;
    COLORREF    crBkgnd;
    COLORREF    crBtnLt;
    COLORREF    crBtnDk;
    WORD        wViewMode;
    WORD        wAlign;
    IShellFolder * psf;
    PIDLIST_ABSOLUTE pidl;
} BANDINFOSFB, *PBANDINFOSFB;

#include <poppack.h>

#undef INTERFACE
#define INTERFACE IShellFolderBand

DECLARE_INTERFACE_(IShellFolderBand, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellFolderBand Methods ***
    STDMETHOD(InitializeSFB)(THIS_ IShellFolder *psf, PCIDLIST_ABSOLUTE pidl) PURE;
    STDMETHOD(SetBandInfoSFB)(THIS_ PBANDINFOSFB pbi) PURE;
    STDMETHOD(GetBandInfoSFB)(THIS_ PBANDINFOSFB pbi) PURE;
};
#undef INTERFACE

#if (NTDDI_VERSION >= NTDDI_WIN2K) && (NTDDI_VERSION <= NTDDI_WINXPSP2)
/*****************************************************************************
 * Control Panel functions
 */
DECLARE_HANDLE(FARPROC16);
LRESULT WINAPI CallCPLEntry16(HINSTANCE hMod, FARPROC16 pFunc, HWND dw3, UINT dw4, LPARAM dw5, LPARAM dw6);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLOBJ_H */
