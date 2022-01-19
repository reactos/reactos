
#ifdef _MSC_VER

// Disabling spammy warnings when compiling with /W4 or /Wall
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4201) // nonstandard extension used
#pragma warning(disable:4265) // class has virtual functions, but destructor is not virtual
#pragma warning(disable:4365) // signed/unsigned mismatch
#pragma warning(disable:4514) // unreferenced inline function
#pragma warning(disable:4710) // function was not inlined
#pragma warning(disable:4820) // padding added
#pragma warning(disable:4946) // reinterpret_cast between related classes

// Disable some warnings in headers only
#pragma warning(push)
#pragma warning(disable:4244) // possible loss of data
#pragma warning(disable:4512) // assignment operator could not be gernerated
#endif

#include <stdio.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

//#define DEBUG_CCOMOBJECT
#define DEBUG_CCOMOBJECT_CREATION 1
#define DEBUG_CCOMOBJECT_DESTRUCTION 1
#define DEBUG_CCOMOBJECT_REFCOUNTING 1

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <wincon.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlwapi.h>
#include <shlguid_undoc.h>
#include <uxtheme.h>
#include <strsafe.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocshell.h>
#include <undocuser.h>

#include <shellutils.h>
#include <ui/rosctrls.h>
#include "../shresdef.h"

#include <wine/debug.h>

#if _MSC_VER
// Restore warnings
#pragma warning(pop)
#endif

#define USE_SYSTEM_MENUDESKBAR 0
#define USE_SYSTEM_MENUSITE 0
#define USE_SYSTEM_MENUBAND 0
#define USE_SYSTEM_MERGED_FOLDERS 0

#define MERGE_FOLDERS 1

#if USE_SYSTEM_MENUDESKBAR
#define CMenuDeskBar_CreateInstance(riid, ppv) (CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER,riid, ppv))
#else
#define CMenuDeskBar_CreateInstance RSHELL_CMenuDeskBar_CreateInstance
#endif

#if USE_SYSTEM_MENUBAND
#define CMenuBand_CreateInstance(riid, ppv) (CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER,riid, ppv))
#else
#define CMenuBand_CreateInstance RSHELL_CMenuBand_CreateInstance
#endif

#if USE_SYSTEM_MENUSITE
#define CMenuSite_CreateInstance(riid, ppv) (CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER,riid, ppv))
#else
#define CMenuSite_CreateInstance RSHELL_CMenuSite_CreateInstance
#endif

#if USE_SYSTEM_MERGED_FOLDERS
#define CMergedFolder_CreateInstance(riid, ppv) (CoCreateInstance(CLSID_MergedFolder, NULL, CLSCTX_INPROC_SERVER,riid, ppv))
#else
#define CMergedFolder_CreateInstance RSHELL_CMergedFolder_CreateInstance
#endif

extern "C"
{
extern HINSTANCE shell32_hInstance;

HRESULT WINAPI RSHELL_CStartMenu_CreateInstance(REFIID riid, void **ppv);
HRESULT WINAPI RSHELL_CMenuDeskBar_CreateInstance(REFIID riid, LPVOID *ppv);
HRESULT WINAPI RSHELL_CMenuSite_CreateInstance(REFIID riid, LPVOID *ppv);
HRESULT WINAPI RSHELL_CMenuBand_CreateInstance(REFIID riid, LPVOID *ppv);
HRESULT WINAPI RSHELL_CMergedFolder_CreateInstance(REFIID riid, LPVOID *ppv);

}
