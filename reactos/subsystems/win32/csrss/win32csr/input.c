/*
 * PROJECT:     ReactOS CSRSS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        subsystems/win32/csrss/api/input.c
 * PURPOSE:     CSRSS input support 
 * COPYRIGHT:   Casper S. Hornstrup (chorns@users.sourceforge.net)
 *
 *  this file is heavily based on subsystems\win32\win32k\ntuser\input.c from trunk
 */

#define NDEBUG
#include "w32csr.h"
#include <debug.h>
#include <ntddmou.h>
#include <ntddkbd.h>

#define ClearMouseInput(mi) \
  mi.dx = 0; \
  mi.dy = 0; \
  mi.mouseData = 0; \
  mi.dwFlags = 0;

#define SendMouseEvent(mi) \
  if(mi.dx != 0 || mi.dy != 0) \
    mi.dwFlags |= MOUSEEVENTF_MOVE; \
  if(mi.dwFlags) \
    mouse_event(mi.dwFlags,mi.dx,mi.dy, mi.mouseData, 0); \
  ClearMouseInput(mi);

VOID FASTCALL
ProcessMouseInputData(PMOUSE_INPUT_DATA Data, ULONG InputCount)
{
    PMOUSE_INPUT_DATA mid;
    MOUSEINPUT mi;
    ULONG i;

    ClearMouseInput(mi);
    mi.time = 0;
    mi.dwExtraInfo = 0;
    for(i = 0; i < InputCount; i++)
    {
        mid = (Data + i);
        mi.dx += mid->LastX;
        mi.dy += mid->LastY;

        /* Check if the mouse move is absolute */
        if (mid->Flags == MOUSE_MOVE_ABSOLUTE)
        {
            /* Set flag and convert to screen location */
            mi.dwFlags |= MOUSEEVENTF_ABSOLUTE;
            mi.dx = mi.dx / (65535 / (GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1));
            mi.dy = mi.dy / (65535 / (GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1));
        }

        if(mid->ButtonFlags)
        {
            if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_LEFTUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_4_DOWN)
            {
                mi.mouseData |= XBUTTON1;
                mi.dwFlags |= MOUSEEVENTF_XDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_4_UP)
            {
                mi.mouseData |= XBUTTON1;
                mi.dwFlags |= MOUSEEVENTF_XUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_5_DOWN)
            {
                mi.mouseData |= XBUTTON2;
                mi.dwFlags |= MOUSEEVENTF_XDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_5_UP)
            {
                mi.mouseData |= XBUTTON2;
                mi.dwFlags |= MOUSEEVENTF_XUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_WHEEL)
            {
                mi.mouseData = mid->ButtonData;
                mi.dwFlags |= MOUSEEVENTF_WHEEL;
                SendMouseEvent(mi);
            }
        }
    }

    SendMouseEvent(mi);
}

DWORD WINAPI MouseInputThread(LPVOID lpParameter)
{
    UNICODE_STRING MouseDeviceName = RTL_CONSTANT_STRING(L"\\Device\\PointerClass0");
    OBJECT_ATTRIBUTES MouseObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE MouseDeviceHandle;
    NTSTATUS Status;

    InitializeObjectAttributes(&MouseObjectAttributes,
                              &MouseDeviceName,
                              0,
                              NULL,
                              NULL);

    do
    {
        Sleep(1000);
        Status = NtOpenFile(&MouseDeviceHandle,
                            FILE_ALL_ACCESS,
                            &MouseObjectAttributes,
                            &Iosb,
                            0,
                            FILE_SYNCHRONOUS_IO_ALERT);
    } while (!NT_SUCCESS(Status));

    while(1)
    {
        MOUSE_INPUT_DATA MouseInput;
        Status = NtReadFile(MouseDeviceHandle,
                            NULL,
                            NULL,
                            NULL,
                            &Iosb,
                            &MouseInput,
                            sizeof(MOUSE_INPUT_DATA),
                            NULL,
                            NULL);
        //if(Status == STATUS_ALERTED)
        //{
        //   break;
        //}
        if(Status == STATUS_PENDING)
        {
            NtWaitForSingleObject(MouseDeviceHandle, FALSE, NULL);
            Status = Iosb.Status;
        }
        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Win32K: Failed to read from mouse.\n");
            return Status;
        }
        DPRINT("MouseEvent\n");

        ProcessMouseInputData(&MouseInput, Iosb.Information / sizeof(MOUSE_INPUT_DATA));
    }
}

/* Sends the keyboard commands to turn on/off the lights.
 */
static NTSTATUS APIENTRY
IntKeyboardUpdateLeds(HANDLE KeyboardDeviceHandle,
                      PKEYBOARD_INPUT_DATA KeyInput,
                      PKEYBOARD_INDICATOR_TRANSLATION IndicatorTrans)
{
    NTSTATUS Status;
    UINT Count;
    static KEYBOARD_INDICATOR_PARAMETERS Indicators;
    IO_STATUS_BLOCK Block;

    if (!IndicatorTrans)
        return STATUS_NOT_SUPPORTED;

    if (KeyInput->Flags & (KEY_E0 | KEY_E1 | KEY_BREAK))
        return STATUS_SUCCESS;

    for (Count = 0; Count < IndicatorTrans->NumberOfIndicatorKeys; Count++)
    {
        if (KeyInput->MakeCode == IndicatorTrans->IndicatorList[Count].MakeCode)
        {
            Indicators.LedFlags ^=
                IndicatorTrans->IndicatorList[Count].IndicatorFlags;

            /* Update the lights on the hardware */

            Status = NtDeviceIoControlFile(KeyboardDeviceHandle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &Block,
                                           IOCTL_KEYBOARD_SET_INDICATORS,
                                           &Indicators, sizeof(Indicators),
                                           NULL, 0);

            return Status;
        }
    }

    return STATUS_SUCCESS;
}

/* Asks the keyboard driver to send a small table that shows which
 * lights should connect with which scancodes
 */
static NTSTATUS APIENTRY
IntKeyboardGetIndicatorTrans(HANDLE KeyboardDeviceHandle,
                             PKEYBOARD_INDICATOR_TRANSLATION *IndicatorTrans)
{
    NTSTATUS Status;
    DWORD Size = 0;
    IO_STATUS_BLOCK Block;
    PKEYBOARD_INDICATOR_TRANSLATION Ret;

    Size = sizeof(KEYBOARD_INDICATOR_TRANSLATION);

    Ret = HeapAlloc(Win32CsrApiHeap, 0, Size);

    while (Ret)
    {
        Status = NtDeviceIoControlFile(KeyboardDeviceHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Block,
                                       IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION,
                                       NULL,
                                       0,
                                       Ret, Size);

        if (Status != STATUS_BUFFER_TOO_SMALL)
            break;

        HeapFree(Win32CsrApiHeap, 0, Ret);

        Size += sizeof(KEYBOARD_INDICATOR_TRANSLATION);

        Ret = HeapAlloc(Win32CsrApiHeap, 0, Size);
    }

    if (!Ret)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (Status != STATUS_SUCCESS)
    {
        HeapFree(Win32CsrApiHeap, 0, Ret);
        return Status;
    }

    *IndicatorTrans = Ret;
    return Status;
}

DWORD WINAPI KeyboardInputThread(LPVOID lpParameter)
{
    UNICODE_STRING KeyboardDeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
    OBJECT_ATTRIBUTES KeyboardObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE KeyboardDeviceHandle;
    NTSTATUS Status;
    PKEYBOARD_INDICATOR_TRANSLATION IndicatorTrans = NULL;

    InitializeObjectAttributes(&KeyboardObjectAttributes,
                               &KeyboardDeviceName,
                               0,
                               NULL,
                               NULL);

    do
    {
        Sleep(1000);
        Status = NtOpenFile(&KeyboardDeviceHandle,
                            FILE_ALL_ACCESS,
                            &KeyboardObjectAttributes,
                            &Iosb,
                            0,
                            FILE_SYNCHRONOUS_IO_ALERT);
    } while (!NT_SUCCESS(Status));

    IntKeyboardGetIndicatorTrans(KeyboardDeviceHandle,
        &IndicatorTrans);
    while(1)
    {
        KEYBOARD_INPUT_DATA KeyInput;
        DWORD flags;

        Status = NtReadFile(KeyboardDeviceHandle,
                            NULL,
                            NULL,
                            NULL,
                            &Iosb,
                            &KeyInput,
                            sizeof(KEYBOARD_INPUT_DATA),
                            NULL,
                            NULL);

        if(Status == STATUS_ALERTED)
        {
            break;
        }
        if(Status == STATUS_PENDING)
        {
            NtWaitForSingleObject(KeyboardDeviceHandle, FALSE, NULL);
            Status = Iosb.Status;
        }
        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Win32K: Failed to read from mouse.\n");
            return Status;
        }

        DPRINT("KeyRaw: %s %04x\n",
            (KeyInput.Flags & KEY_BREAK) ? "up" : "down",
            KeyInput.MakeCode );

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Win32K: Failed to read from keyboard.\n");
            return Status;
        }

        IntKeyboardUpdateLeds(KeyboardDeviceHandle,
                              &KeyInput,
                              IndicatorTrans);

        flags = 0;

        if (KeyInput.Flags & KEY_E0)
            flags |= KEYEVENTF_EXTENDEDKEY;

        if (KeyInput.Flags & KEY_BREAK)
            flags |= KEYEVENTF_KEYUP;

        keybd_event(MapVirtualKey(KeyInput.MakeCode & 0xff, MAPVK_VSC_TO_VK), KeyInput.MakeCode & 0xff, flags , 0);
    }

    return Status;
}


void CsrInitInputSupport()
{
    HANDLE MouseThreadHandle, KeyboardThreadHandle;

    ClipCursor(NULL);

    MouseThreadHandle = CreateThread(NULL, 0, MouseInputThread, NULL, 0,NULL);
    KeyboardThreadHandle = CreateThread(NULL, 0, KeyboardInputThread, NULL, 0,NULL);
}

/* EOF */
