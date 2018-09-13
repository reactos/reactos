/*
    init.c initialisation for MSVIDEO.DLL

    Copyright (c) Microsoft Corporation 1992. All rights reserved

*/

#include <windows.h>
#include <win32.h>
#include <verinfo.h>           // to get rup
#include "mmsystem.h"
#include "msviddrv.h"
#include "msvideo.h"
#include "msvideoi.h"

#ifdef WIN32

/* we have to allow the compman dll to perform load and unload
 * processing - it has a critsec that needs to be initialised and freed
 */
extern void IC_Load(void);
extern void IC_Unload(void);
#else
#define IC_Load()
#define IC_Unload()
#endif

extern void FAR PASCAL videoCleanup(HTASK hTask);
extern void FAR PASCAL DrawDibCleanup(HTASK hTask);
extern void FAR PASCAL ICCleanup(HTASK hTask);

/*****************************************************************************
 * @doc INTERNAL VIDEO
 *
 * DLLEntryPoint - common DLL entry point.
 *
 *  this code is called on both Win16 and Win32, libentry.asm handles
 *  this on Win16 and the system handles it on NT.
 *
 ****************************************************************************/

#ifndef DLL_PROCESS_DETACH
    #define DLL_PROCESS_DETACH  0
    #define DLL_PROCESS_ATTACH  1
    #define DLL_THREAD_ATTACH   2
    #define DLL_THREAD_DETACH   3
#endif

#ifndef NOTHUNKS
VOID FAR PASCAL InitThunks(void);
BOOL gfVideo32;
BOOL gfICM32;
#endif // NOTHUNKS


BOOL WINAPI DLLEntryPoint(HINSTANCE hInstance, ULONG Reason, LPVOID pv)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            ghInst = hInstance;
            IC_Load();
#ifndef NOTHUNKS
            DPF(("Setting up the thunk code\n"));
            InitThunks();
            DPF(("All thunks initialised:  gfVideo32=%d,  gfICM32=%d\n", gfVideo32, gfICM32));
#endif // NOTHUNKS
            break;

        case DLL_PROCESS_DETACH:
            DrawDibCleanup(NULL);
            ICCleanup(NULL);
            IC_Unload();
            videoCleanup(NULL);
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;
    }

    return TRUE;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | VideoForWindowsVersion | This function returns the version
 *   of the Microsoft Video for Windows software.
 *
 * @rdesc Returns a DWORD version, the hiword is the product version the
 *  loword is the minor revision.
 *
 * @comm currently returns 0x010A00## (1.10.00.##) ## is the internal build
 *      number.
 *
 ****************************************************************************/
#if 0
#ifdef rup
    #define MSVIDEO_VERSION     (0x01000000l+rup)       // 1.00.00.##
#else
    #define MSVIDEO_VERSION     (0x01000000l)           // 1.00.00.00
#endif
#else
    #define MSVIDEO_VERSION     (0x0L+(((DWORD)MMVERSION)<<24)+(((DWORD)MMREVISION)<<16)+((DWORD)MMRELEASE))
#endif

DWORD FAR PASCAL VideoForWindowsVersion(void)
{
    return MSVIDEO_VERSION;
}

/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * MSVIDEO=1
 *
 ****************************************************************************/

#ifdef DEBUG

#define MODNAME "MSVIDEO"

#ifndef WIN32
    #define lstrcatA lstrcat
    #define lstrcpyA lstrcpy
    #define lstrlenA lstrlen
    #define wvsprintfA      wvsprintf
    #define GetProfileIntA  GetProfileInt
    #define OutputDebugStringA OutputDebugString
#endif

#define _WINDLL
#include <stdarg.h>

void FAR CDECL dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    va_list va;

    static BOOL fDebug = -1;

    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug", MODNAME, FALSE);

    if (!fDebug)
        return;

    lstrcpyA(ach, MODNAME ": ");
    va_start(va, szFormat);
    wvsprintfA(ach+lstrlenA(ach),szFormat,(LPSTR)va);
    va_end(va);
    lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);
}

#endif

