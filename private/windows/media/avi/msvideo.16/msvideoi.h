/****************************************************************************/
/*                                                                          */
/*        MSVIDEOI.H - Internal Include file for Video APIs                 */
/*                                                                          */
/*        Note: You must include WINDOWS.H before including this file.      */
/*                                                                          */
/*        Copyright (c) 1990-1992, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/

#ifdef BUILDDLL
#undef WINAPI
#define WINAPI FAR PASCAL _loadds
#endif

/****************************************************************************

                   Digital Video Driver Structures

****************************************************************************/

#define MAXVIDEODRIVERS 10

/****************************************************************************

                            Globals

****************************************************************************/

extern UINT      wTotalVideoDevs;                  // total video devices
// The module handle is used in drawdib to load strings from the resource file
extern HINSTANCE ghInst;                           // our module handle

#ifndef NOTHUNKS
extern BOOL      gfVideo32;     // Do we have a 32-bit avicap.dll to talk to?
extern BOOL      gfICM32;       // Do we have access to 32 bit ICM thunks?
#endif // NOTHUNKS

#ifdef WIN32
//#define SZCODE TCHAR
#define HTASK HANDLE
#else
#define SZCODE char _based(_segname("_CODE"))
#endif

extern SZCODE szNull[];
extern SZCODE szVideo[];
extern SZCODE szSystemIni[];
extern SZCODE szDrivers[];

/* internal video function prototypes */
#ifdef WIN32
/*
 * don't lock pages in NT
 */
#define HugePageLock(x, y)		(TRUE)
#define HugePageUnlock(x, y)
#else
BOOL FAR PASCAL HugePageLock(LPVOID lpArea, DWORD dwLength);
void FAR PASCAL HugePageUnlock(LPVOID lpArea, DWORD dwLength);
#endif


// for correct handling of capGetDriverDescription on NT and Chicago
// this is used by the NT version of avicap.dll (16bit) but not intended for
// public use, hence not in msvideo.h
DWORD WINAPI videoCapDriverDescAndVer (
        DWORD wDriverIndex,
        LPSTR lpszName, UINT cbName,
        LPSTR lpszVer, UINT cbVer);

/****************************************************************************
****************************************************************************/

#ifdef DEBUG_RETAIL
    #define DebugErr(flags, sz)         {static SZCODE ach[] = "MSVIDEO: "sz; DebugOutput((flags)   | DBF_DRIVER, ach); }
#else
    #define DebugErr(flags, sz)
#endif

/****************************************************************************
****************************************************************************/

#ifdef DEBUG
    extern void FAR CDECL dprintf(LPSTR szFormat, ...);
    #define DPF(_x_) dprintf _x_
#else
    #define DPF(_x_)
#endif
