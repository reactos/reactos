/*
 * PROJECT:     ReactOS CSRSS
 * LICENSE:     GPL - See COPYING in the top level directory
 * COPYRIGHT:   Casper S. Hornstrup (chorns@users.sourceforge.net)
 *
 *  this file is heavily based on subsystems\win32\win32k\ntuser\input.c from trunk
 */

#define NDEBUG

#define WIN32_NO_STATUS
#define NOCRYPT
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <debug.h>
#include <ntddmou.h>
#include <ntddkbd.h>
#include <ntuser.h>

static HHOOK gKeyboardHook, gMouseHook;

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

#define INPUT_DEVICES 2

static LRESULT CALLBACK DummyHookProc( INT code, WPARAM wparam, LPARAM lparam ){
    return CallNextHookEx( 0, code, wparam, lparam );
}


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

    Ret = HeapAlloc(GetProcessHeap(), 0, Size);

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

        HeapFree(GetProcessHeap(), 0, Ret);

        Size += sizeof(KEYBOARD_INDICATOR_TRANSLATION);

        Ret = HeapAlloc(GetProcessHeap(), 0, Size);
    }

    if (!Ret)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (Status != STATUS_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, Ret);
        return Status;
    }

    *IndicatorTrans = Ret;
    return Status;
}

DWORD WINAPI InputThread()
{
    UNICODE_STRING KeyboardDeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
    UNICODE_STRING MouseDeviceName = RTL_CONSTANT_STRING(L"\\Device\\PointerClass0");
    OBJECT_ATTRIBUTES KeyboardObjectAttributes;
    OBJECT_ATTRIBUTES MouseObjectAttributes;
    IO_STATUS_BLOCK KeyboardIosb;
    IO_STATUS_BLOCK MouseIosb;
    HANDLE KeyboardDeviceHandle;
    HANDLE MouseDeviceHandle;
    NTSTATUS KeyboardStatus = STATUS_UNSUCCESSFUL;
    NTSTATUS MouseStatus = STATUS_UNSUCCESSFUL;
    PKEYBOARD_INDICATOR_TRANSLATION IndicatorTrans = NULL;
    HANDLE hInputs[INPUT_DEVICES];
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;

    ByteOffset.QuadPart = (LONGLONG)0;

    InitializeObjectAttributes(&KeyboardObjectAttributes,
                               &KeyboardDeviceName,
                               0,
                               NULL,
                               NULL);

    do
    {
        Sleep(1000);
        KeyboardStatus = NtOpenFile(&KeyboardDeviceHandle,
                            FILE_ALL_ACCESS,
                            &KeyboardObjectAttributes,
                            &KeyboardIosb,
                            0,
                            0);
    } while (!NT_SUCCESS(KeyboardStatus));


    InitializeObjectAttributes(&MouseObjectAttributes,
                              &MouseDeviceName,
                              0,
                              NULL,
                              NULL);

    do
    {
        Sleep(1000);
        MouseStatus = NtOpenFile(&MouseDeviceHandle,
                            FILE_ALL_ACCESS,
                            &MouseObjectAttributes,
                            &MouseIosb,
                            0,
                            0);
    } while (!NT_SUCCESS(MouseStatus));

    IntKeyboardGetIndicatorTrans(KeyboardDeviceHandle, &IndicatorTrans);

    while(1)
    {
        KEYBOARD_INPUT_DATA KeyInput;
        MOUSE_INPUT_DATA MouseInput;
        DWORD flags;
        INT iWaitObjects = 0;

        if(MouseStatus != STATUS_PENDING)
        {
            MouseStatus = NtReadFile(MouseDeviceHandle,
                                NULL,
                                NULL,
                                NULL,
                                &MouseIosb,
                                &MouseInput,
                                sizeof(MOUSE_INPUT_DATA),
                                &ByteOffset,
                                NULL);
        }

        if(MouseStatus == STATUS_PENDING)
            hInputs[iWaitObjects++] = MouseDeviceHandle;

        if(KeyboardStatus != STATUS_PENDING)
        {
            KeyboardStatus = NtReadFile(KeyboardDeviceHandle,
                                NULL,
                                NULL,
                                NULL,
                                &KeyboardIosb,
                                &KeyInput,
                                sizeof(KEYBOARD_INPUT_DATA),
                                &ByteOffset,
                                NULL);
        }

        if(KeyboardStatus == STATUS_PENDING)
            hInputs[iWaitObjects++] = KeyboardDeviceHandle;


        if(MouseStatus == STATUS_PENDING && KeyboardStatus == STATUS_PENDING)
        {
            /* Both devices pending, wait for them */
            Status = NtWaitForMultipleObjects(iWaitObjects, hInputs, WaitAny, FALSE, 0);

            if ((Status >= STATUS_WAIT_0) && (Status < (STATUS_WAIT_0 + iWaitObjects)))
            {
                if(hInputs[Status - STATUS_WAIT_0] == KeyboardDeviceHandle)
                {
                    KeyboardStatus = KeyboardIosb.Status;
                }
                else if(hInputs[Status - STATUS_WAIT_0] == MouseDeviceHandle)
                {
                    MouseStatus = MouseIosb.Status;
                }
            }
        }

        if(NT_SUCCESS(MouseStatus) && MouseStatus != STATUS_PENDING)
        {
            if(!gMouseHook)
                gMouseHook = SetWindowsHookEx(WH_MOUSE_LL, DummyHookProc, NULL, 0);

            ProcessMouseInputData(&MouseInput, MouseIosb.Information / sizeof(MOUSE_INPUT_DATA));
        }
        else if (MouseStatus != STATUS_PENDING)
        {
            DPRINT1("Failed to read from mouse 0x%08x\n", MouseStatus);
        }

        if(NT_SUCCESS(KeyboardStatus) && KeyboardStatus != STATUS_PENDING)
        {
            IntKeyboardUpdateLeds(KeyboardDeviceHandle, &KeyInput, IndicatorTrans);

            flags = 0;

            if (KeyInput.Flags & KEY_E0)
                flags |= KEYEVENTF_EXTENDEDKEY;

            if (KeyInput.Flags & KEY_BREAK)
                flags |= KEYEVENTF_KEYUP;

            if(!gKeyboardHook)
                gKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, DummyHookProc, NULL, 0);

            keybd_event(MapVirtualKey(KeyInput.MakeCode & 0xff, MAPVK_VSC_TO_VK), KeyInput.MakeCode & 0xff, flags , 0);
        }
        else if (KeyboardStatus != STATUS_PENDING)
        {
            DPRINT1("Failed to read from keyboard 0x%08x\n", KeyboardStatus);
        }

    }

    return KeyboardStatus;
}

DWORD_PTR WINAPI NtUserCallOneParam(DWORD_PTR Param, DWORD Routine)
{
    BOOLEAN Suspend = FALSE;

    if(Routine == ONEPARAM_ROUTINE_CREATESYSTEMTHREADS)
    {
        if(Param == 0)
        {
            Suspend = InputThread();
        }
        else
        {
            /* FIXME */
            Suspend = TRUE;
        }
    }
    else
    {
        DPRINT1("Unknown routine %u\n", Routine);
    }

    while(Suspend)
    {
        Sleep(INFINITE);
    }

    return 0;
}