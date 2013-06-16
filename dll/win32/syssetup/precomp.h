#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winsvc.h>
#include <winnls.h>
#include <winuser.h>
#include <windowsx.h>
#include <wincon.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/rtlfuncs.h>
#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>
#include <syssetup/syssetup.h>
#include <userenv.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <time.h>
#include <ntlsa.h>
#include <ntsecapi.h>
#include <ntsam.h>
#include <sddl.h>

#include "globals.h"
#include "resource.h"
