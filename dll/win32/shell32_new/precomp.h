#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <malloc.h>

#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#include <windows.h>
#include <winternl.h>

#include <shlguid.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shldisp.h>
#include <commdlg.h>
#include <commctrl.h>
#include <cpl.h>
#include <objbase.h>
#include <ole2.h>
#include <ocidl.h>
#include <docobj.h>
#include <prsht.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <msi.h>
#include <appmgmt.h>
#include <ntquery.h>
#include <recyclebin.h>
#include <shtypes.h>
#include <fmifs/fmifs.h>
#include <largeint.h>
#include <sddl.h>

#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>

#include "base/shell/explorer-new/todo.h"
#include "dlgs.h"
#include "pidl.h"
#include "debughlp.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "cpanel.h"
#include "enumidlist.h"
#include "shfldr.h"
#include "version.h"
#include "shellfolder.h"
#include "xdg.h"
#include "shellapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#endif
