/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/objects.c
 * PURPOSE:         Misc "Object" Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserBuildHimcList(DWORD dwUnknown1,
                    DWORD dwUnknown2,
                    DWORD dwUnknown3,
                    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
APIENTRY
NtUserBuildHwndList(HDESK hDesktop,
                    HWND hwndParent,
                    BOOLEAN bChildren,
                    ULONG dwThreadId,
                    ULONG lParam,
                    HWND* pWnd,
                    ULONG* pBufSize)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
NtUserBuildNameList(HWINSTA hWinSta,
                    ULONG dwSize,
                    PVOID lpBuffer,
                    PULONG pRequiredSize)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
NtUserBuildPropList(HWND hWnd,
                    LPVOID Buffer,
                    DWORD BufferSize,
                    DWORD *Count)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

DWORD
APIENTRY
NtUserConvertMemHandle(DWORD Unknown0,
                       DWORD Unknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserCreateLocalMemHandle(DWORD Unknown0,
                           DWORD Unknown1,
                           DWORD Unknown2,
                           DWORD Unknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetAtomName(ATOM nAtom,
                  LPWSTR lpBuffer)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserGetObjectInformation(HANDLE hObject,
                           DWORD nIndex,
                           PVOID pvInformation,
                           DWORD nLength,
                           PDWORD nLengthNeeded)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetObjectInformation(HANDLE hObject,
                           DWORD nIndex,
                           PVOID pvInformation,
                           DWORD nLength)
{
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE
APIENTRY
NtUserRemoveProp(HWND hWnd,
                 ATOM Atom)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserSetProp(HWND hWnd,
              ATOM Atom,
              HANDLE Data)
{
    UNIMPLEMENTED;
    return FALSE;
}
