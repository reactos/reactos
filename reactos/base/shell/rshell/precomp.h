
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

#define USE_SYSTEM_MENUDESKBAR 1
#define USE_SYSTEM_MENUSITE 1
#define USE_SYSTEM_MENUBAND 1
#define USE_SYSTEM_MERGED_FOLDERS 0

#define MERGE_FOLDERS 1

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
#include <rosctrls.h>

#include <wine/debug.h>

#if _MSC_VER
// Restore warnings
#pragma warning(pop)
#endif

#define shell32_hInstance 0

extern "C" HRESULT WINAPI CStartMenu_Constructor(REFIID riid, void **ppv);
extern "C" HRESULT WINAPI CMenuDeskBar_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuSite_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuBand_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuDeskBar_Wrapper(IDeskBar * db, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuSite_Wrapper(IBandSite * bs, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuBand_Wrapper(IShellMenu * sm, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMergedFolder_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CStartMenuSite_Wrapper(ITrayPriv * trayPriv, REFIID riid, LPVOID *ppv);
