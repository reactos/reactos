
#include <wine/config.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <strsafe.h>
#include <winerror.h>
#include <shellapi.h>
#include <winuser.h>
#include <shlobj.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <sddl.h>
#include <commdlg.h>
#include <commoncontrols.h>
#include <wine/winternl.h>

#include <userenv.h>

#include <undocshell.h>
#include "pidl.h"
#include "cpanel.h"
#include "shell32_main.h"
#include "../shresdef.h"

#include "../debughlp.h"

#include <wine/debug.h>
#include <wine/unicode.h>
