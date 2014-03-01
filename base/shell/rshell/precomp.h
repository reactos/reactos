
#define USE_SYSTEM_MENUDESKBAR 0
#define USE_SYSTEM_MENUSITE 0
#define USE_SYSTEM_MENUBAND 0

#define WRAP_MENUDESKBAR 0
#define WRAP_MENUSITE 0
#define WRAP_MENUBAND 0
#define WRAP_TRAYPRIV 0

#define MERGE_FOLDERS 0

#include <stdio.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

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
#include <wine/debug.h>

#define shell32_hInstance 0
#define SMC_EXEC 4
extern "C" INT WINAPI Shell_GetCachedImageIndex(LPCWSTR szPath, INT nIndex, UINT bSimulateDoc);


extern "C" HRESULT WINAPI CStartMenu_Constructor(REFIID riid, void **ppv);
extern "C" HRESULT WINAPI CMenuDeskBar_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuSite_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuBand_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuDeskBar_Wrapper(IDeskBar * db, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuSite_Wrapper(IBandSite * bs, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuBand_Wrapper(IShellMenu * sm, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMergedFolder_Constructor(IShellFolder* userLocal, IShellFolder* allUsers, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CStartMenuSite_Wrapper(ITrayPriv * trayPriv, REFIID riid, LPVOID *ppv);

static __inline ULONG
Win32DbgPrint(const char *filename, int line, const char *lpFormat, ...)
{
    char szMsg[512];
    char *szMsgStart;
    const char *fname;
    va_list vl;
    ULONG uRet;

    fname = strrchr(filename, '\\');
    if (fname == NULL)
    {
        fname = strrchr(filename, '/');
        if (fname != NULL)
            fname++;
    }
    else
        fname++;

    if (fname == NULL)
        fname = filename;

    szMsgStart = szMsg + sprintf(szMsg, "%s:%d: ", fname, line);

    va_start(vl, lpFormat);
    uRet = (ULONG) vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    return uRet;
}

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
