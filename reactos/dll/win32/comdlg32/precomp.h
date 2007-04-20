#ifndef PRECOMP_H__
#define PRECOMP_H__

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define NO_SHLWAPI_STREAM
#define COM_NO_WINDOWS_H
#define _COMDLG32_

//#if defined (_MSC_VER)
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <guiddef.h>
#include <dlgs.h>
#include <cderr.h>
//#endif

#if !defined (_MSC_VER)
#include "psdk/shlguid.h"
#include "psdk/shlobj.h"
#endif

#include "shlguid.h"
#include "shlobj.h"

#if !defined (_MSC_VER)
#include "psdk/shlguid.h"
#include "psdk/shlwapi.h"
#endif

#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/dlgs.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include <cderr.h>
#include <cdlg.h>
#include <winspool.h>
#include <winerror.h>
#include <winnls.h>
#include <winreg.h>
#include <winternl.h>

//local headers
#include "cdlg.h"
#include "printdlg.h"
#include "filedlgbrowser.h"
#include "cdlg.h"
#include "servprov.h"
#include "filedlg31.h"

#endif
