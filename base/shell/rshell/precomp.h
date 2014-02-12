
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

extern "C" HRESULT CMenuSite_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT CMenuBand_Constructor(REFIID riid, LPVOID *ppv);