/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/input.c
 * PURPOSE:         Input Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserAssociateInputContext(DWORD dwUnknown1,
                            DWORD dwUnknown2,
                            DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserAttachThreadInput(IN DWORD idAttach,
                        IN DWORD idAttachTo,
                        IN BOOL fAttach)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserBlockInput(BOOL BlockIt)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserCreateInputContext(DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserDestroyInputContext(DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserQueryInputContext(DWORD dwUnknown1,
                        DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

UINT
APIENTRY
NtUserSendInput(UINT nInputs,
                LPINPUT pInput,
                INT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserUpdateInputContext(DWORD Unknown0,
                         DWORD Unknown1,
                         DWORD Unknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserWaitForInputIdle(IN HANDLE hProcess,
                       IN DWORD dwMilliseconds,
                       IN BOOL Unknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputBuffer(PRAWINPUT pData,
                        PUINT pcbSize,
                        UINT cbSizeHeader)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputData(HRAWINPUT hRawInput,
                      UINT uiCommand,
                      LPVOID pData,
                      PUINT pcbSize,
                      UINT cbSizeHeader)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceInfo(HANDLE hDevice,
                            UINT uiCommand,
                            LPVOID pData,
                            PUINT pcbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceList(PRAWINPUTDEVICELIST pRawInputDeviceList,
                            PUINT puiNumDevices,
                            UINT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRegisteredRawInputDevices(PRAWINPUTDEVICE pRawInputDevices,
                                   PUINT puiNumDevices,
                                   UINT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserRegisterRawInputDevices(IN PCRAWINPUTDEVICE pRawInputDevices,
                              IN UINT uiNumDevices,
                              IN UINT cbSize)
{
    UNIMPLEMENTED;
    return FALSE;
}
