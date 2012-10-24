/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <psapi.h>

/* External Winlogon Header */
#include <winlogon.h>

/* Internal CSRSS Headers */
#include <win/winmsg.h>
//#include <conio.h>
//#include <csrplugin.h>
#include <desktopbg.h>

/* Public Win32K Headers */
#include <ntuser.h>

#include "resource.h"

VOID
WINAPI
Win32CsrHardError(
    IN PCSR_THREAD ThreadData,
    IN PHARDERROR_MSG Message);

/* EOF */
