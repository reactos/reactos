#ifndef _BROWSEUI_PCH_
#define _BROWSEUI_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <wingdi.h>
#include <shlobj.h>
#include <tlogstg.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocuser.h>
#include <perhist.h>
#include <exdispid.h>
#include <strsafe.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <undocshell.h>
#include <shellutils.h>
#include <browseui_undoc.h>
#include <wine/debug.h>

#include "resource.h"

#include "aclistisf.h"
#include "aclmulti.h"
#include "addressband.h"
#include "addresseditbox.h"
#include "CAutoComplete.h"
#include "bandproxy.h"
#include "bandsite.h"
#include "bandsitemenu.h"
#include "brandband.h"
#include "internettoolbar.h"
#include "commonbrowser.h"
#include "globalfoldersettings.h"
#include "regtreeoptions.h"
#include "explorerband.h"
#include "CProgressDialog.h"
#include <stdio.h>

WINE_DEFAULT_DEBUG_CHANNEL(browseui);


#define USE_CUSTOM_MENUBAND 1

typedef HRESULT(WINAPI * PMENUBAND_CONSTRUCTOR)(REFIID riid, void **ppv);
typedef HRESULT(WINAPI * PMERGEDFOLDER_CONSTRUCTOR)(REFIID riid, void **ppv);

static inline
HRESULT CreateMergedFolder(REFIID riid, void **ppv)
{
#if 1
    HMODULE hRShell = GetModuleHandle(L"rshell.dll");
    if (!hRShell)
        hRShell = LoadLibrary(L"rshell.dll");

    if (hRShell)
    {
        PMERGEDFOLDER_CONSTRUCTOR pCMergedFolder_Constructor = (PMERGEDFOLDER_CONSTRUCTOR)
             GetProcAddress(hRShell, "CMergedFolder_Constructor");

        if (pCMergedFolder_Constructor)
        {
            return pCMergedFolder_Constructor(riid, ppv);
        }
    }
#endif
    return CoCreateInstance(CLSID_MergedFolder, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
}

static inline
HRESULT CreateMenuBand(REFIID iid, LPVOID *ppv)
{
#if USE_CUSTOM_MENUBAND
    HMODULE hRShell = GetModuleHandleW(L"rshell.dll");

    if (!hRShell) 
        hRShell = LoadLibraryW(L"rshell.dll");

    if (hRShell)
    {
        PMENUBAND_CONSTRUCTOR func = (PMENUBAND_CONSTRUCTOR) GetProcAddress(hRShell, "CMenuBand_Constructor");
        if (func)
        {
            return func(iid , ppv);
        }
    }
#endif
    return CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, iid, ppv);
}

#endif /* _BROWSEUI_PCH_ */
