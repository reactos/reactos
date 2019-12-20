
#ifndef _WINE_COMDLG32_PRECOMP_H
#define _WINE_COMDLG32_PRECOMP_H

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <winternl.h>
#include <objbase.h>
#include <commdlg.h>
#include <shlobj.h>
#include <dlgs.h>
#include <cderr.h>
/* RegGetValueW is supported by Win2k3 SP1 but headers need Win Vista */
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winreg.h>
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>

#include <wine/heap.h>
#include <wine/debug.h>

#include "cdlg.h"
#include "filedlgbrowser.h"

#endif /* !_WINE_COMDLG32_PRECOMP_H */
