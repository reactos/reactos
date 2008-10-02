/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/include/userobj.h
 * PURPOSE:         User Structures
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

#ifndef _WIN32K_USEROBJ_H
#define _WIN32K_USEROBJ_H

//
// W32PROCESS Flags
//
#define W32PF_CONSOLEAPPLICATION      0x00000001
#define W32PF_FORCEOFFFEEDBACK        0x00000002
#define W32PF_STARTGLASS              0x00000004
#define W32PF_WOW                     0x00000008
#define W32PF_READSCREENACCESSGRANTED 0x00000010
#define W32PF_INITIALIZED             0x00000020
#define W32PF_APPSTARTING             0x00000040
#define W32PF_WOW64                   0x00000080
#define W32PF_ALLOWFOREGROUNDACTIVATE 0x00000100
#define W32PF_OWNDCCLEANUP            0x00000200
#define W32PF_SHOWSTARTGLASSCALLED    0x00000400
#define W32PF_FORCEBACKGROUNDPRIORITY 0x00000800
#define W32PF_TERMINATED              0x00001000
#define W32PF_CLASSESREGISTERED       0x00002000
#define W32PF_THREADCONNECTED         0x00004000
#define W32PF_PROCESSCONNECTED        0x00008000
#define W32PF_WAKEWOWEXEC             0x00010000
#define W32PF_WAITFORINPUTIDLE        0x00020000
#define W32PF_IOWINSTA                0x00040000
#define W32PF_CONSOLEFOREGROUND       0x00080000
#define W32PF_OLELOADED               0x00100000
#define W32PF_SCREENSAVER             0x00200000
#define W32PF_IDLESCREENSAVER         0x00400000

typedef struct _W32THREAD
{
    PETHREAD peThread;
    ULONG RefCount;
    PVOID ptlW32; //FIXME: PTL?
    PVOID pgdiDcattr;
    PVOID pgdiBrushAttr;
    PVOID pUMPDObjs;
    PVOID pUMPDHeap;
    DWORD dwEngAcquireCount;
    PVOID pSemTable;
    PVOID pUMPDObj;
} W32THREAD, *PW32THREAD;

typedef struct _W32PROCESS
{
    PEPROCESS peProcess;
    ULONG RefCount;
    FLONG W32PF_flags;
    ULONG InputIdleEvent;
    ULONG StartCursorHideTime;
    ULONG NextStart;
    PVOID pDCAttrList;
    PVOID pBrushAttrList;
    ULONG W32Pid;
    ULONG GDIHandleCount;
    ULONG UserHandleCount;
    ULONG Unknown[19];     
} W32PROCESS, *PW32PROCESS;

#endif // _WIN32K_USEROBJ_H
