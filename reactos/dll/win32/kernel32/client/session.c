/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/session.c
 * PURPOSE:         Win32 session (TS) functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *     2001-12-07 created
 */
#include <k32.h>
#define NDEBUG
#include <debug.h>
//DEBUG_CHANNEL(kernel32session); not actually used

DWORD ActiveConsoleSessionId = 0;


/*
 * @unimplemented
 */
DWORD
WINAPI
DosPathToSessionPathW(DWORD SessionID,
                      LPWSTR InPath,
                      LPWSTR *OutPath)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * From: ActiveVB.DE
 *
 * Declare Function DosPathToSessionPath _
 * Lib "kernel32.dll" _
 * Alias "DosPathToSessionPathA" ( _
 *     ByVal SessionId As Long, _
 *     ByVal pInPath As String, _
 *     ByVal ppOutPath As String ) _
 * As Long
 *
 * @unimplemented
 */
DWORD
WINAPI
DosPathToSessionPathA(DWORD SessionId,
                      LPSTR InPath,
                      LPSTR *OutPath)
{
    //DosPathToSessionPathW (SessionId,InPathW,OutPathW);
    UNIMPLEMENTED;
    return 0;
}



/*
 * @implemented
 */
DWORD
WINAPI
WTSGetActiveConsoleSessionId(VOID)
{
    return ActiveConsoleSessionId;
}

/* EOF */
