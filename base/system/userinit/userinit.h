/*
 */

#ifndef __USERINIT_H__
#define __USERINIT_H__

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <wincon.h>
#include <shellapi.h>
#include <regstr.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <undocuser.h>
#include <winnls.h>
#include <stdio.h>

#include <ndk/exfuncs.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(userinit);

#include "resource.h"


typedef enum
{
    LOCALEPAGE,
    STARTPAGE,
    DONE
} PAGESTATE;

typedef enum
{
    SHELL,
    INSTALLER,
    REBOOT
} RUN;

typedef struct _IMGINFO
{
    HBITMAP hBitmap;
    INT cxSource;
    INT cySource;
    INT iPlanes;
    INT iBits;
} IMGINFO, *PIMGINFO;

typedef struct
{
    PAGESTATE NextPage;
    RUN Run;
    IMGINFO ImageInfo;
} STATE, *PSTATE;


extern HINSTANCE hInstance;

LONG
ReadRegSzKey(
    IN HKEY hKey,
    IN LPCWSTR pszKey,
    OUT LPWSTR *pValue);

BOOL
IsLiveCD(VOID);


VOID
RunLiveCD(
    PSTATE State);

#endif /* __USERINIT_H__ */
