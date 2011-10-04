/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/mmtypes.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>

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
#include <win32k/ntuser.h>

#include "resource.h"

/* shared header with console.dll */
#include "console.h"

BOOL
WINAPI
Win32CsrHardError(
    IN PCSRSS_PROCESS_DATA ProcessData,
    IN PHARDERROR_MSG Message);

/* EOF */
