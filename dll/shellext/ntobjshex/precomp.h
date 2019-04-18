#ifndef _NTOBJSHEX_PRECOMP_H_
#define _NTOBJSHEX_PRECOMP_H_

#include <stdarg.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#define COBJMACROS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

extern "C"
{
#include <ndk/ntndk.h>
#include <ndk/rtlfuncs.h>
}

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <undocshell.h>
#include <shellutils.h>

#include <wine/debug.h>

#include "resource.h"

#undef DbgPrint

extern HINSTANCE g_hInstance;

#define ID_ICON_VOLUME (WM_APP + 0x4CB)

// {845B0FB2-66E0-416B-8F91-314E23F7C12D}
DEFINE_GUID(CLSID_NtObjectFolder,
    0x845b0fb2, 0x66e0, 0x416b, 0x8f, 0x91, 0x31, 0x4e, 0x23, 0xf7, 0xc1, 0x2d);

#define DFM_MERGECONTEXTMENU 1 // uFlags LPQCMINFO
#define DFM_INVOKECOMMAND 2 // idCmd pszArgs
#define DFM_INVOKECOMMANDEX 12 // idCmd PDFMICS
#define DFM_GETDEFSTATICID 14 // idCmd * 0

#define SHCIDS_ALLFIELDS 0x80000000L
#define SHCIDS_CANONICALONLY 0x10000000L

#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

WINE_DEFAULT_DEBUG_CHANNEL(ntobjshex);

#include <ntquery.h>
#include "util.h"

#include "ntobjenum.h"

#include "foldercommon.h"
#include "ntobjfolder.h"
#include "regfolder.h"

#endif /* _NTOBJSHEX_PRECOMP_H_ */
