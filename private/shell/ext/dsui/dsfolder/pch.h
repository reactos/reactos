#ifndef _pch_h
#define _pch_h

#include "windows.h"
#include "windowsx.h"
#include "commctrl.h"
#include "commdlg.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shlobjp.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "atlbase.h"

#include "multimon.h"

#include "winuserp.h"
#include "comctrlp.h"
#include "shsemip.h"
#include "shlapip.h"
#include "shellp.h"
#include "shguidp.h"
#include "cfdefs.h"

#include "activeds.h"
#include "iadsp.h"

#include "dsclient.h"
#include "dsclintp.h"
#include "common.h"

#include "resource.h"
#include "cstrings.h"
#include "idlist.h"

#include "ui.h"
#include "dataobj.h"


//
// global instance stuff
//

extern HINSTANCE g_hInstance; 
#define GLOBAL_HINSTANCE (g_hInstance)


//
// trace flags to control show of trace etc
// 

#define TRACE_CORE          0x00000001
#define TRACE_FOLDER        0x00000002
#define TRACE_ENUM          0x00000004
#define TRACE_UI            0x00000010
#define TRACE_DATAOBJ       0x00000020
#define TRACE_IDLIST        0x00000040
#define TRACE_PARSE         0x00000080
#define TRACE_MISC          0x00000100
#define TRACE_API           0x00000200
#define TRACE_CACHE         0x00000400
#define TRACE_DCNAME        0x00000800
#define TRACE_COMPARE       0x00001000
#define TRACE_MYCONTEXTMENU 0x00002000

#define TRACE_ALWAYS        0xffffffff          // use with caution


#if DOWNLEVEL_SHELL

//
// SHAlloc and friends are not exported from shell32 (however SHGetMalloc is so,
// we can implement them).
//

EXTERN_C void* MySHAlloc(ULONG cb);
EXTERN_C void* MySHRealloc(LPVOID pv, ULONG cbNew);
EXTERN_C void  MySHFree(LPVOID pv);
EXTERN_C ULONG MySHGetSize(LPVOID pv);

#define SHAlloc   MySHAlloc
#define SHRealloc MySHRealloc
#define SHFree    MySHFree
#define SHGetSize MySHGetSize

//
// IDList helpers
//

EXTERN_C LPITEMIDLIST _MyILCreate(UINT cbSize);

#define _ILCreate _MyILCreate

//
// SHOpenPropSheet is not exported on older shell32's, so we must 
// redirect to our API.  (We don't need to worry about A/W thunks
// as we build TCHAR)
//

EXTERN_C BOOL MySHOpenPropSheet(LPCTSTR pszCaption,
                                HKEY ahkeys[],
                                UINT ckeys,
                                const CLSID * pclsidDef,    OPTIONAL
                                LPDATAOBJECT pdtobj,
                                IShellBrowser * psb,
                                LPCTSTR pStartPage)         OPTIONAL;

#ifdef  SHOpenPropSheet
#undef  SHOpenPropSheet
#endif

#define SHOpenPropSheet MySHOpenPropSheet

//
// ShellMessageBox is exposed ANSI (only) on Win9x, therefore remove the
// mapping of the name to an A/W version if building for that platform.
//

#define ShellMessageBoxA ShellMessageBox

//
// CDefFolderMenu_Create2 is not exported from older shell32's so we
// must redirect to our API.
//

EXTERN_C HRESULT MyDefFolderMenu(LPCITEMIDLIST pidlFolder, HWND hwndOwner,
                                 UINT cidl, LPCITEMIDLIST * apidl,
                                 LPSHELLFOLDER psf, LPFNDFMCALLBACK lpfn,
                                 UINT nKeys, const HKEY *ahkeyClsKeys,
                                 LPCONTEXTMENU *ppcm);

#define CDefFolderMenu_Create2 MyDefFolderMenu


#endif  // DOWNLEVEL_SHELL

//
// more constructors
//

STDAPI CDsFolder_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CDsFolderProperties_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

HRESULT CDsEnum_CreateInstance(LPCITEMIDLIST pidl, HWND hwndOwner, DWORD grfFlags, IMalloc* pm, IEnumIDList **ppeidl);


#endif
