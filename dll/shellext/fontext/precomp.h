#ifndef FONTEXT_PRECOMP_H
#define FONTEXT_PRECOMP_H


#define WIN32_NO_STATUS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlstr.h>
#include <wine/debug.h>
#include <shellutils.h>

extern const GUID CLSID_CFontExt;
extern LONG g_ModuleRefCnt;

#include "resource.h"
#include "fontpidl.hpp"
#include "CFontCache.hpp"
#include "CFontExt.hpp"

#define FONT_HIVE   HKEY_LOCAL_MACHINE
#define FONT_KEY    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"

HRESULT _CEnumFonts_CreateInstance(CFontExt* zip, DWORD flags, REFIID riid, LPVOID* ppvOut);
HRESULT _CFontMenu_CreateInstance(HWND hwnd, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
                                  IShellFolder *psf, REFIID riid, LPVOID* ppvOut);
HRESULT _CDataObject_CreateInstance(PCIDLIST_ABSOLUTE folder, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
                                    REFIID riid, LPVOID* ppvOut);

inline BOOL IsFontDotExt(LPCWSTR pchDotExt)
{
    static const LPCWSTR array[] =
    {
        L".ttf", L".ttc", L".otf", L".otc", L".fon", L".fnt", NULL
    };
    for (const LPCWSTR *pp = array; *pp; ++pp)
    {
        if (!_wcsicmp(*pp, pchDotExt))
            return TRUE;
    }
    return FALSE;
}

#endif /* FONTEXT_PRECOMP_H */
