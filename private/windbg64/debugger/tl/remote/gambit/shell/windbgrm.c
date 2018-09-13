/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    windbgrm.c

Abstract:

    This file implements the WinDbg remote debugger that doesn't use any
    USER32.DLL or csrss functionality one initialized.

Author:

    Wesley Witt (wesw) 1-Nov-1993

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "od.h"
#include "odp.h"
#include "memlist.h"
#include "windbgrm.h"

HANDLE              hEventQuit;
TLFUNC              TLFunc;
HANDLE              hTransportDll;

void
xxUnlockHlle(
    HLLE h
    )
{
    UnlockHlle(h);
}

void *
SYMalloc(
    size_t n
    )
{
    return malloc(n);
}

void *
SYRealloc(
    void * p,
    size_t n
    )
{
    return realloc(p, n);
}

void
SYFree(
    void *p
    )
{
    free(p);
}



DBF Dbf = {
    SYMalloc,         // PVOID      (OSDAPI *  lpfnMHAlloc)
    SYRealloc,        // PVOID      (OSDAPI *  lpfnMHRealloc)
    SYFree,           // VOID       (OSDAPI *  lpfnMHFree)

    LLHlliInit,         // HLLI       (OSDAPI *  lpfnLLInit)
    LLHlleCreate,       // HLLE       (OSDAPI *  lpfnLLCreate)
    LLAddHlleToLl,      // VOID       (OSDAPI *  lpfnLLAdd)
    LLInsertHlleInLl,   // VOID       (OSDAPI *  lpfnLLInsert)
    LLFDeleteHlleFromLl, // BOOL       (OSDAPI *  lpfnLLDelete)
    LLHlleFindNext,     // HLLE       (OSDAPI *  lpfnLLNext)
    LLChlleDestroyLl,   // DWORD      (OSDAPI *  lpfnLLDestroy)
    LLHlleFindLpv,      // HLLE       (OSDAPI *  lpfnLLFind)
    LLChlleInLl,        // DWORD      (OSDAPI *  lpfnLLSize)
    LLLpvFromHlle,      // PVOID      (OSDAPI *  lpfnLLLock)
    xxUnlockHlle,       // VOID       (OSDAPI *  lpfnLLUnlock)
    LLHlleGetLast,      // HLLE       (OSDAPI *  lpfnLLLast)
    LLHlleAddToHeadOfLI, // VOID       (OSDAPI *  lpfnLLAddHead)
    LLFRemoveHlleFromLl, // BOOL       (OSDAPI *  lpfnLLRemove)
    0, // int        (OSDAPI *  lpfnLBAssert)
    0, // int        (OSDAPI *  lpfnLBQuit)
    0, // LPSTR      (OSDAPI *  lpfnSHGetSymbol)
    0, // DWORD      (OSDAPI * lpfnSHGetPublicAddr)
    0, // LPSTR      (OSDAPI * lpfnSHAddrToPublicName)
    0, // LPVOID     (OSDAPI * lpfnSHGetDebugData)
    0, // PVOID      (OSDAPI *  lpfnSHLpGSNGetTable)
#ifdef NT_BUILD_ONLY
    0, // BOOL       (OSDAPI *  lpfnSHWantSymbols)
#endif
    0, // DWORD      (OSDAPI *  lpfnDHGetNumber)
    0, // MPT        (OSDAPI *  lpfnGetTargetProcessor)
    0, // LPGETSETPROFILEPROC   lpfnGetSet;
};


BOOL
LoadTransport(
    LPTRANSPORT_LAYER lpTl
    );

void
ShutdownAndUnload(
    LPTRANSPORT_LAYER lpTl
    );

void
DebugPrint(
    char * szFormat,
    ...
    );

TRANSPORT_LAYER DefaultTl= { "Gambit",  "Gambit Serial Transport Layer on COM1 at 115200 Baud", "tlgambit.dll",  NULL, 0, 0 };

#ifdef DEBUGVER
DEBUG_VERSION('W', 'R', "WinDbg Remote Shell, DEBUG")
#else
RELEASE_VERSION('W', 'R', "WinDbg Remote Shell")
#endif


LRESULT APIENTRY
MainWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
	switch (message) {
        case WM_DESTROY:
            SetEvent(hEventQuit);
            break;
    }

    return DefWindowProc( hWnd, message, wParam, lParam );
}


DWORD
MessagePumpThread(
    LPVOID lpvArg
    )
{
    MSG       msg;
    WNDCLASS  WndClass = {0};
    HMENU     hMenu;
	HANDLE    hInst;
    HWND      HWndFrame;
	LPTSTR   szAppName = "Windbgrm";

    hInst = GetModuleHandle( NULL );

    WndClass.cbClsExtra    = 0;
    WndClass.cbWndExtra    = 0;
    WndClass.lpszClassName = szAppName;
    WndClass.lpszMenuName  = szAppName;
    WndClass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    WndClass.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    WndClass.hInstance     = hInst;
    WndClass.lpfnWndProc   = MainWndProc;
    WndClass.hCursor       = LoadCursor( NULL, IDC_ARROW );
    WndClass.hIcon         = LoadIcon( hInst, "WindbgRmIcon" );

    RegisterClass( &WndClass );

    HWndFrame = CreateWindow( szAppName,
                              szAppName,
                              WS_TILEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              NULL,
                              NULL,
                              hInst,
                              NULL
                            );

    if (!HWndFrame) {
        return FALSE;
    }

    ShowWindow( HWndFrame, SW_SHOWMINNOACTIVE );

    while (GetMessage( &msg, NULL, 0, 0 )) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return 0;
}


void _cdecl main()

{

	HANDLE hMessageThread;
	DWORD tid;
    
	DEBUG_OUT("Windbgrm init\n");
    SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

    hMessageThread = CreateThread( NULL, 0, MessagePumpThread, 0, 0, &tid );
    SetThreadPriority( hMessageThread, THREAD_PRIORITY_ABOVE_NORMAL );

	if (!LoadTransport(&DefaultTl)) {
	    DEBUG_OUT("Windbrgm - cannot load Tl\n");
		ExitProcess(1);
	}

    hEventQuit = CreateEvent( NULL, FALSE, FALSE, NULL );

	if (TLFunc(tlfConnect, NULL, 0,0) != xosdNone) {
	    DEBUG_OUT("Windbrgm - cannot create transport\n");
        ShutdownAndUnload(&DefaultTl);
		ExitProcess(2);
    }

	if (TLFunc(tlfConnect, hEventQuit, 0,0) != xosdNone) {
	    DEBUG_OUT("Windbrgm - cannot connect transport\n");
        ShutdownAndUnload(&DefaultTl);
		ExitProcess(3);
    }

	WaitForSingleObject(hEventQuit,INFINITE);
    DEBUG_OUT("Windbrgm - TL disconnected\n");
    ShutdownAndUnload(&DefaultTl);

    ExitProcess(0);

}

BOOL
LoadTransport(
    LPTRANSPORT_LAYER lpTl
    )

/*++

Routine Description:

    Loads the named pipes transport layer and does the necessary
    initialization of the TL.

Arguments:

    None.

Return Value:

    None.

--*/

{
    extern AVS      Avs;
    DBGVERSIONPROC  pVerProc;
    LPAVS           pavs;
    CHAR            szDmName[16];


    if ((hTransportDll = LoadLibrary(lpTl->szDllName)) == NULL) {
        goto LoadTransportError;
    }

    pVerProc = (DBGVERSIONPROC)GetProcAddress(hTransportDll, DBGVERSIONPROCNAME);
    if (!pVerProc) {
        goto LoadTransportError;
    }

    pavs = (*pVerProc)();

    if (pavs->rgchType[0] != 'T' || pavs->rgchType[1] != 'L') {
        goto LoadTransportError;
    }

    if (Avs.rlvt != pavs->rlvt) {
        goto LoadTransportError;
    }

    if (Avs.iRmj != pavs->iRmj) {
        goto LoadTransportError;
    }

    if ((TLFunc = (TLFUNC)GetProcAddress(hTransportDll, "TLFunc")) == NULL) {
        goto LoadTransportError;
    }

    if (TLFunc (tlfInit, NULL, (LPARAM)&Dbf, 0 ) != xosdNone) {
        goto LoadTransportError;
    }

    return TRUE;

LoadTransportError:
    if (hTransportDll) {
        FreeLibrary( hTransportDll );
        hTransportDll = NULL;
    }

    return FALSE;
}

void
ShutdownAndUnload(
    LPTRANSPORT_LAYER lpTl
    )
{
  DEBUG_OUT( "WinDbgRm shutting down\n" );
    if (TLFunc) {
        TLFunc(tlfRemoteQuit, 0, 0, 0);
        TLFunc(tlfDestroy, 0, 0, 0);
    }
    FreeLibrary( hTransportDll );
    hTransportDll = NULL;
}


void
DebugPrint(
    char * szFormat,
    ...
    )

/*++

Routine Description:

    Provides a printf style function for doing outputdebugstring.

Arguments:

    None.

Return Value:

    None.

--*/

{
    va_list  marker;
    int n;
    char        rgchDebug[4096];

    va_start( marker, szFormat );
    n = _vsnprintf(rgchDebug, sizeof(rgchDebug), szFormat, marker );
    va_end( marker);

    if (n == -1) {
        rgchDebug[sizeof(rgchDebug)-1] = '\0';
    }

    OutputDebugString( rgchDebug );
    return;
}

void
ShowAssert(
    LPSTR condition,
    UINT  line,
    LPSTR file
    )
{
    char text[4096];
    int  id;

    sprintf(text, "Assertion failed - Line:%u, File:%Fs, Condition:%Fs", line, file, condition);
    DebugPrint( "%s\r\n", text );
    id = MessageBox( NULL, text, "WinDbgRm", MB_YESNO | MB_ICONHAND | MB_TASKMODAL | MB_SETFOREGROUND );
    if (id != IDYES) {
        DebugBreak();
    }

    return;
}
