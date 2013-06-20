/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winnls.h>
#include <winreg.h>
#include <winsvc.h>
#include <wincon.h>
#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/umfuncs.h>
#include <psapi.h>

/* External Winlogon Header */
#include <winlogon.h>

/* Internal CSRSS Headers */
#include <conio.h>
#include <csrplugin.h>
#include <desktopbg.h>
#include "guiconsole.h"
#include "tuiconsole.h"

/* Public Win32K Headers */
#include <ntuser.h>

#include "resource.h"

/* shared header with console.dll */
#include "console.h"

VOID
WINAPI
Win32CsrHardError(
    IN PCSR_THREAD ThreadData,
    IN PHARDERROR_MSG Message);

/* EOF */
