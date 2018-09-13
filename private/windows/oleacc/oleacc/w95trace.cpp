// Copyright (c) 1996-1999 Microsoft Corporation


/*
    Implementation of Win95 tracing facility to mimic that of NT. Works on both.
*/

#pragma warning(disable:4201)	// allows nameless structs and unions
#pragma warning(disable:4514)	// don't care when unreferenced inline functions are removed
#pragma warning(disable:4706)	// we are allowed to assign within a conditional


#include "windows.h"
#include <stdio.h>
#include <stdarg.h>
#include <process.h>
#include "w95trace.h"


#ifdef _DEBUG

void OutputDebugStringW95( LPCTSTR lpOutputString, ...)
{

    HANDLE heventDBWIN;  /* DBWIN32 synchronization object */
    HANDLE heventData;   /* data passing synch object */
    HANDLE hSharedFile;  /* memory mapped file shared data */
    LPTSTR lpszSharedMem;
    TCHAR achBuffer[500];

    /* create the output buffer */
    va_list args;
    va_start(args, lpOutputString);
    wvsprintf(achBuffer, lpOutputString, args);
    va_end(args);

    /* 
        Do a regular OutputDebugString so that the output is 
        still seen in the debugger window if it exists.

        This ifdef is necessary to avoid infinite recursion 
        from the inclusion of W95TRACE.H
    */
#ifdef _UNICODE
    ::OutputDebugStringW(achBuffer);
#else
    ::OutputDebugStringA(achBuffer);
#endif

    /* bail if it's not Win95 */
    {
        OSVERSIONINFO VerInfo;
        VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&VerInfo);
        if ( VerInfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS )
            return;
    }

    /* make sure DBWIN is open and waiting */
    heventDBWIN = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("DBWIN_BUFFER_READY"));
    if ( !heventDBWIN )
    {
        //MessageBox(NULL, TEXT("DBWIN_BUFFER_READY nonexistent"), NULL, MB_OK);
        return;            
    }

    /* get a handle to the data synch object */
    heventData = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("DBWIN_DATA_READY"));
    if ( !heventData )
    {
        // MessageBox(NULL, TEXT("DBWIN_DATA_READY nonexistent"), NULL, MB_OK);
        CloseHandle(heventDBWIN);
        return;            
    }
    
    hSharedFile = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, 4096, TEXT("DBWIN_BUFFER"));
    if (!hSharedFile) 
    {
        //MessageBox(NULL, TEXT("DebugTrace: Unable to create file mapping object DBWIN_BUFFER"), TEXT("Error"), MB_OK);
        CloseHandle(heventDBWIN);
        CloseHandle(heventData);
        return;
    }

    lpszSharedMem = (LPTSTR)MapViewOfFile(hSharedFile, FILE_MAP_WRITE, 0, 0, 512);
    if (!lpszSharedMem) 
    {
        //MessageBox(NULL, "DebugTrace: Unable to map shared memory", "Error", MB_OK);
        CloseHandle(heventDBWIN);
        CloseHandle(heventData);
        return;
    }

    /* wait for buffer event */
    WaitForSingleObject(heventDBWIN, INFINITE);

    /* write it to the shared memory */
    *((LPDWORD)lpszSharedMem) = _getpid();
    wsprintf(lpszSharedMem + sizeof(DWORD), TEXT("%s"), achBuffer);

    /* signal data ready event */
    SetEvent(heventData);

    /* clean up handles */
    CloseHandle(hSharedFile);
    CloseHandle(heventData);
    CloseHandle(heventDBWIN);

    return;
}

#endif