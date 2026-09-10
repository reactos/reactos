/*
 * PROJECT:     ReactOS Win32k subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions related to Raw Input handling
 * COPYRIGHT:   Copyright 2026 Hervé Poussineau
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC      0x01
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE     0x02
#endif

#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD  0x06
#endif

/* ReactOS win32k is hacked to only support one keyboard and mouse. */
extern HANDLE ghKeyboardDevice;
extern HANDLE ghMouseDevice;

typedef struct _RIDDevice
{
    HANDLE file;
    RID_DEVICE_INFO info;
    DWORD Type; // RIM_TYPEMOUSE or RIM_TYPEKEYBOARD
} RIDDevice;

RIDDevice RIDDevices[2] = {
    { NULL, {0}, 0 }, // Mouse
    { NULL, {0}, 0 }  // Keyboard
};

typedef struct _USER_RAWINPUT
{
    LIST_ENTRY ListEntry;
    RAWINPUT RawInput;
} USER_RAWINPUT, *PUSER_RAWINPUT;

static PAGED_LOOKASIDE_LIST gRawInputLookasideList;
PRAWINPUTDEVICE global_pRawInputDevices = NULL;
BOOLEAN RawInputEnabled = FALSE;

DWORD
APIENTRY
NtUserGetRawInputDeviceInfo(
    _In_opt_ HANDLE hDevice,
    _In_ UINT uiCommand,
    _Inout_opt_ LPVOID pData,
    _Inout_ PUINT pcbSize)
{
    PINPUT_DEVICE_INFO DeviceInfo;
    UINT cbSize, cbRequiredSize;
    UINT Ret = (UINT)-1;

    _SEH2_TRY
    {
        ProbeForRead(pcbSize, sizeof(*pcbSize), sizeof(DWORD));
        cbSize = *pcbSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return Ret);
    }
    _SEH2_END;

    AcquireDeviceInfoListMutex();
    for (DeviceInfo = gpInputDeviceInfo; DeviceInfo && DeviceInfo != hDevice; DeviceInfo = DeviceInfo->pNextDeviceInfo)
        ;
    if (!DeviceInfo)
    {
        ReleaseDeviceInfoListMutex();
        EngSetLastError(ERROR_INVALID_HANDLE);
        return Ret;
    }

    UserEnterShared();

    switch (uiCommand)
    {
        case RIDI_PREPARSEDDATA:
            if (DeviceInfo->DeviceType == RIM_TYPEHID)
                cbRequiredSize = DeviceInfo->Hid.CollectionInformation.DescriptorSize;
            else
                cbRequiredSize = 0;
            break;
        case RIDI_DEVICENAME:
            cbRequiredSize = DeviceInfo->DeviceName.Length / sizeof(WCHAR) + sizeof(ANSI_NULL);
            break;
        case RIDI_DEVICEINFO:
            cbRequiredSize = sizeof(RID_DEVICE_INFO);
            break;
        default:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            goto cleanup;
    }

    _SEH2_TRY
    {
        if (!pData)
        {
            ProbeForWrite(pcbSize, sizeof(*pcbSize), sizeof(DWORD));
            *pcbSize = cbRequiredSize;
            Ret = 0;
            _SEH2_LEAVE;
        }

        if (cbSize < cbRequiredSize)
        {
            ProbeForWrite(pcbSize, sizeof(*pcbSize), sizeof(DWORD));
            *pcbSize = cbRequiredSize;
            EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
            _SEH2_LEAVE;
        }

        ProbeForWrite(pData, cbRequiredSize, sizeof(DWORD));
        switch (uiCommand)
        {
            case RIDI_PREPARSEDDATA:
                if (DeviceInfo->DeviceType == RIM_TYPEHID)
                    RtlCopyMemory(pData, DeviceInfo->Hid.PreparsedData, cbRequiredSize);
                break;
            case RIDI_DEVICENAME:
                // As cbSize/cbRequiredSize are in chars, we didn't probe a big enough buffer
                // Do it again
                ProbeForWrite(pData, DeviceInfo->DeviceName.Length + sizeof(UNICODE_NULL), sizeof(DWORD));
                RtlCopyMemory(pData, DeviceInfo->DeviceName.Buffer, DeviceInfo->DeviceName.Length);
                ((WCHAR*)pData)[cbRequiredSize - 1] = UNICODE_NULL;
                break;
            case RIDI_DEVICEINFO:
            {
                PRID_DEVICE_INFO prdi = pData;
                ProbeForRead(&prdi->cbSize, sizeof(prdi->cbSize), sizeof(DWORD));
                if (prdi->cbSize != cbRequiredSize)
                {
                    EngSetLastError(ERROR_INVALID_PARAMETER);
                    _SEH2_YIELD(break);
                }

                ProbeForWrite(prdi, sizeof(*prdi), sizeof(DWORD));
                RtlZeroMemory(prdi, sizeof(*prdi));
                prdi->cbSize = sizeof(*prdi);
                prdi->dwType = DeviceInfo->DeviceType;

                switch (DeviceInfo->DeviceType)
                {
                    case RIM_TYPEMOUSE:
                        prdi->mouse.dwId = DeviceInfo->Mouse.Attributes.MouseIdentifier & ~HORIZONTAL_WHEEL_PRESENT;
                        prdi->mouse.dwNumberOfButtons = DeviceInfo->Mouse.Attributes.NumberOfButtons;
                        prdi->mouse.dwSampleRate = DeviceInfo->Mouse.Attributes.SampleRate;
                        prdi->mouse.fHasHorizontalWheel = !!(DeviceInfo->Mouse.Attributes.MouseIdentifier & HORIZONTAL_WHEEL_PRESENT);
                        break;

                    case RIM_TYPEKEYBOARD:
                        prdi->keyboard.dwType = DeviceInfo->Keyboard.Attributes.KeyboardIdentifier.Type;
                        prdi->keyboard.dwSubType = DeviceInfo->Keyboard.Attributes.KeyboardIdentifier.Subtype;
                        prdi->keyboard.dwKeyboardMode = DeviceInfo->Keyboard.Attributes.KeyboardMode;
                        prdi->keyboard.dwNumberOfFunctionKeys = DeviceInfo->Keyboard.Attributes.NumberOfFunctionKeys;
                        prdi->keyboard.dwNumberOfIndicators = DeviceInfo->Keyboard.Attributes.NumberOfIndicators;
                        prdi->keyboard.dwNumberOfKeysTotal = DeviceInfo->Keyboard.Attributes.NumberOfKeysTotal;
                        break;

                    case RIM_TYPEHID:
                        prdi->hid.dwVendorId = DeviceInfo->Hid.CollectionInformation.VendorID;
                        prdi->hid.dwProductId = DeviceInfo->Hid.CollectionInformation.ProductID;
                        prdi->hid.dwVersionNumber = DeviceInfo->Hid.CollectionInformation.VersionNumber;
                        prdi->hid.usUsagePage = DeviceInfo->Hid.Caps.UsagePage;
                        prdi->hid.usUsage = DeviceInfo->Hid.Caps.Usage;
                        break;

                    default:
                        break;
                }
                break;
            }
            default:
                ASSERT(FALSE);
                break;
        }
        Ret = cbRequiredSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

cleanup:
    UserLeave();
    ReleaseDeviceInfoListMutex();
    return Ret;
}


static
INT
RawInputDeviceIndexFromType(
    DWORD dwType)
{
    switch (dwType)
    {
        case RIM_TYPEMOUSE:
            return 0;

        case RIM_TYPEKEYBOARD:
            return 1;

        default:
            return -1;
    }
}

static
PRAWINPUTDEVICE
RawInputRegistrationFromType(
    DWORD dwType)
{
    INT Index = RawInputDeviceIndexFromType(dwType);

    if (!global_pRawInputDevices || Index < 0)
        return NULL;

    if (global_pRawInputDevices[Index].usUsagePage != HID_USAGE_PAGE_GENERIC)
        return NULL;

    if ((dwType == RIM_TYPEMOUSE && global_pRawInputDevices[Index].usUsage != HID_USAGE_GENERIC_MOUSE) ||
        (dwType == RIM_TYPEKEYBOARD && global_pRawInputDevices[Index].usUsage != HID_USAGE_GENERIC_KEYBOARD))
        return NULL;

    return &global_pRawInputDevices[Index];
}

static
VOID
RawInputWarnUnimplementedFlags(
    const RAWINPUTDEVICE *Device)
{
    BOOL IsMouse = (Device->usUsagePage == HID_USAGE_PAGE_GENERIC &&
                    Device->usUsage == HID_USAGE_GENERIC_MOUSE);
    BOOL IsKeyboard = (Device->usUsagePage == HID_USAGE_PAGE_GENERIC &&
                       Device->usUsage == HID_USAGE_GENERIC_KEYBOARD);
    DWORD PageFlags = Device->dwFlags & (RIDEV_PAGEONLY | RIDEV_EXCLUDE);

    if ((PageFlags == RIDEV_NOLEGACY) && (IsMouse || IsKeyboard))
    {
        DPRINT1("RIDEV_NOLEGACY for raw %s is accepted but not implemented yet.\n",
                IsMouse ? "mouse" : "keyboard");
    }
    else
    {
        if (Device->dwFlags & RIDEV_PAGEONLY)
            DPRINT1("RIDEV_PAGEONLY is accepted but not implemented yet.\n");

        if (Device->dwFlags & RIDEV_EXCLUDE)
            DPRINT1("RIDEV_EXCLUDE is accepted but not implemented yet.\n");
    }

    if (IsMouse && (Device->dwFlags & RIDEV_CAPTUREMOUSE))
        DPRINT1("RIDEV_CAPTUREMOUSE for raw mouse is accepted but not implemented yet.\n");

    if (IsKeyboard && (Device->dwFlags & RIDEV_NOHOTKEYS))
        DPRINT1("RIDEV_NOHOTKEYS for raw keyboard is accepted but not implemented yet.\n");

    if (IsKeyboard && (Device->dwFlags & RIDEV_APPKEYS))
        DPRINT1("RIDEV_APPKEYS for raw keyboard is accepted but not implemented yet.\n");

    if (Device->dwFlags & RIDEV_EXINPUTSINK)
        DPRINT1("RIDEV_EXINPUTSINK is only partially implemented.\n");

    if (Device->dwFlags & RIDEV_DEVNOTIFY)
        DPRINT1("RIDEV_DEVNOTIFY is accepted but not implemented yet.\n");
}

static
PUSER_RAWINPUT
RawInputEntryFromHandle(
    PUSER_MESSAGE_QUEUE MessageQueue,
    HRAWINPUT hRawInput)
{
    PLIST_ENTRY Entry;

    if (!MessageQueue || !hRawInput)
        return NULL;

    for (Entry = MessageQueue->RawInputListHead.Flink;
         Entry != &MessageQueue->RawInputListHead;
         Entry = Entry->Flink)
    {
        PUSER_RAWINPUT RawEntry = CONTAINING_RECORD(Entry, USER_RAWINPUT, ListEntry);
        if ((HRAWINPUT)&RawEntry->RawInput == hRawInput)
            return RawEntry;
    }

    return NULL;
}

static
VOID
RawInputFreeEntry(
    PUSER_RAWINPUT RawEntry)
{
    RemoveEntryList(&RawEntry->ListEntry);
    ExFreeToPagedLookasideList(&gRawInputLookasideList, RawEntry);
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitRawInputImpl(VOID)
{
    ExInitializePagedLookasideList(&gRawInputLookasideList,
                                   NULL,
                                   NULL,
                                   0,
                                   sizeof(USER_RAWINPUT),
                                   'iwar',
                                   256);
    return STATUS_SUCCESS;
}

HRAWINPUT
FASTCALL
UserCreateRawInput(
    PTHREADINFO pti,
    DWORD dwType,
    HANDLE hDevice,
    WPARAM wParam,
    CONST VOID *pData,
    UINT cbData)
{
    PUSER_RAWINPUT RawEntry;

    if (!pti || !pti->MessageQueue || !pData)
        return NULL;

    RawEntry = ExAllocateFromPagedLookasideList(&gRawInputLookasideList);
    if (!RawEntry)
        return NULL;

    RtlZeroMemory(RawEntry, sizeof(*RawEntry));
    RawEntry->RawInput.header.dwType = dwType;
    RawEntry->RawInput.header.dwSize = sizeof(RAWINPUTHEADER) + cbData;
    RawEntry->RawInput.header.hDevice = hDevice;
    RawEntry->RawInput.header.wParam = wParam;
    RtlCopyMemory(&RawEntry->RawInput.data, pData, cbData);

    InsertTailList(&pti->MessageQueue->RawInputListHead, &RawEntry->ListEntry);
    return (HRAWINPUT)&RawEntry->RawInput;
}

BOOL
FASTCALL
UserFreeRawInput(
    PUSER_MESSAGE_QUEUE MessageQueue,
    HRAWINPUT hRawInput)
{
    PUSER_RAWINPUT RawEntry = RawInputEntryFromHandle(MessageQueue, hRawInput);

    if (!RawEntry)
        return FALSE;

    RawInputFreeEntry(RawEntry);
    return TRUE;
}

VOID
FASTCALL
UserCleanupRawInput(
    PUSER_MESSAGE_QUEUE MessageQueue)
{
    while (!IsListEmpty(&MessageQueue->RawInputListHead))
    {
        PUSER_RAWINPUT RawEntry;
        RawEntry = CONTAINING_RECORD(MessageQueue->RawInputListHead.Flink,
                                     USER_RAWINPUT,
                                     ListEntry);
        RawInputFreeEntry(RawEntry);
    }
}

BOOL
FASTCALL
UserGetRawInputTarget(
    DWORD dwType,
    PTHREADINFO *ppti,
    WPARAM *pwParam)
{
    PRAWINPUTDEVICE Registration;
    PUSER_MESSAGE_QUEUE pFocusQueue;
    PWND pWnd = NULL;
    PTHREADINFO pti;

    Registration = RawInputRegistrationFromType(dwType);
    if (!Registration)
        return FALSE;

    pFocusQueue = IntGetFocusMessageQueue();
    if (!pFocusQueue)
        return FALSE;

    if (Registration->hwndTarget)
    {
        pWnd = UserGetWindowObject(Registration->hwndTarget);
        if (!pWnd)
            return FALSE;
    }
    else
    {
        pWnd = pFocusQueue->spwndFocus;
        if (!pWnd)
            pWnd = pFocusQueue->spwndActive;
        if (!pWnd)
            return FALSE;
    }

    pti = pWnd->head.pti;
    if (!pti)
        return FALSE;

    if (pFocusQueue->ptiKeyboard && pti->rpdesk != pFocusQueue->ptiKeyboard->rpdesk)
        return FALSE;

    if (Registration->dwFlags & RIDEV_INPUTSINK)
    {
        *pwParam = (pti->MessageQueue == pFocusQueue) ? RIM_INPUT : RIM_INPUTSINK;
        *ppti = pti;
        return TRUE;
    }

    if (Registration->dwFlags & RIDEV_EXINPUTSINK)
    {
        if (pti->MessageQueue != pFocusQueue)
            return FALSE; /* FIXME: Needs per-process arbitration like Windows */

        *pwParam = RIM_INPUT;
        *ppti = pti;
        return TRUE;
    }

    if (pti->MessageQueue != pFocusQueue)
        return FALSE;

    *pwParam = RIM_INPUT;
    *ppti = pti;
    return TRUE;
}

VOID
APIENTRY
HandleDeviceEnumeration(DWORD type)
{
    static const RID_DEVICE_INFO_KEYBOARD keyboard_info = {0, 0, 1, 12, 3, 101};
    static const RID_DEVICE_INFO_MOUSE mouse_info = {1, 5, 0, FALSE};
    RID_DEVICE_INFO info;
    RtlZeroMemory( &info, sizeof(info) );
    info.cbSize = sizeof(info);
    info.dwType = type;

    switch (type)
    {
        case RIM_TYPEMOUSE:
            info.mouse = mouse_info;
            RIDDevices[0].info = info;
            RIDDevices[0].file = ghMouseDevice;
            RIDDevices[0].Type = RIM_TYPEMOUSE;
            break;
        case RIM_TYPEKEYBOARD:
            info.keyboard = keyboard_info;
            RIDDevices[1].info = info;
            RIDDevices[1].file = ghKeyboardDevice;
            RIDDevices[1].Type = RIM_TYPEKEYBOARD;
            break;
        default:
        {
            DPRINT1("Unknown device type %d\n", type);
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return;
        }
    }
}

static
DWORD
RawInputCopyData(
    PRAWINPUT RawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    UINT OutputDataSize;

    if (cbSizeHeader != sizeof(RAWINPUTHEADER) || !pcbSize)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ~0u;
    }

    switch (uiCommand)
    {
        case RID_INPUT:
            OutputDataSize = RawInput->header.dwSize;
            break;

        case RID_HEADER:
            OutputDataSize = sizeof(RAWINPUTHEADER);
            break;

        default:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return ~0u;
    }

    if (!pData)
    {
        *pcbSize = OutputDataSize;
        EngSetLastError(ERROR_SUCCESS);
        return 0;
    }

    if (*pcbSize < OutputDataSize)
    {
        *pcbSize = OutputDataSize;
        EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
        return ~0u;
    }

    if (uiCommand == RID_HEADER)
        RtlCopyMemory(pData, &RawInput->header, OutputDataSize);
    else
        RtlCopyMemory(pData, RawInput, OutputDataSize);

    return OutputDataSize;
}

/* This file is pretty deeply inspired by how wine seems to do it, but wine can't be DIRECTLY used. */
DWORD
APIENTRY
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    PTHREADINFO pti;
    PUSER_MESSAGE_QUEUE MessageQueue;
    PUSER_RAWINPUT RawEntry;
    PLIST_ENTRY Entry, NextEntry;
    UINT BufferSize, UsedSize = 0, Count = 0;

    if (cbSizeHeader != sizeof(RAWINPUTHEADER) || !pcbSize)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ~0u;
    }

    pti = PsGetCurrentThreadWin32Thread();
    MessageQueue = pti ? pti->MessageQueue : NULL;
    if (!MessageQueue)
    {
        *pcbSize = 0;
        return 0;
    }

    Entry = MessageQueue->HardwareMessagesListHead.Flink;
    if (!pData)
    {
        while (Entry != &MessageQueue->HardwareMessagesListHead)
        {
            PUSER_MESSAGE Message = CONTAINING_RECORD(Entry, USER_MESSAGE, ListEntry);
            if (Message->Msg.message == WM_INPUT)
            {
                RawEntry = RawInputEntryFromHandle(MessageQueue, (HRAWINPUT)Message->Msg.lParam);
                *pcbSize = RawEntry ? RawEntry->RawInput.header.dwSize : 0;
                EngSetLastError(ERROR_SUCCESS);
                return 0;
            }
            Entry = Entry->Flink;
        }

        *pcbSize = 0;
        return 0;
    }

    BufferSize = *pcbSize;
    while (Entry != &MessageQueue->HardwareMessagesListHead)
    {
        PUSER_MESSAGE Message;
        UINT RawSize;

        NextEntry = Entry->Flink;
        Message = CONTAINING_RECORD(Entry, USER_MESSAGE, ListEntry);
        Entry = NextEntry;

        if (Message->Msg.message != WM_INPUT)
            continue;

        RawEntry = RawInputEntryFromHandle(MessageQueue, (HRAWINPUT)Message->Msg.lParam);
        if (!RawEntry)
            continue;

        RawSize = RawEntry->RawInput.header.dwSize;
        if (UsedSize + RawSize > BufferSize)
        {
            if (Count == 0)
            {
                *pcbSize = RawSize;
                EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
                return ~0u;
            }
            break;
        }

        RtlCopyMemory((PBYTE)pData + UsedSize, &RawEntry->RawInput, RawSize);
        UsedSize += RawSize;
        Count++;

        ClearMsgBitsMask(pti, Message->QS_Flags);
        MsqDestroyMessage(Message);
        RawInputFreeEntry(RawEntry);
    }

    return Count;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceList(
    _Out_opt_ PRAWINPUTDEVICELIST pRawInputDeviceList,
    _Inout_ PUINT puiNumDevices,
    _In_ UINT cbSize)
{
    PINPUT_DEVICE_INFO DeviceInfo;
    UINT cDevices = 0, i, Ret = (UINT)-1;

    if (cbSize != sizeof(RAWINPUTDEVICELIST))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return Ret;
    }

    AcquireDeviceInfoListMutex();
    for (DeviceInfo = gpInputDeviceInfo; DeviceInfo; DeviceInfo = DeviceInfo->pNextDeviceInfo)
        cDevices++;

    UserEnterShared();

    _SEH2_TRY
    {
        if (!pRawInputDeviceList)
        {
            ProbeForWrite(puiNumDevices, sizeof(*puiNumDevices), sizeof(DWORD));
            *puiNumDevices = cDevices;
            Ret = 0;
            _SEH2_LEAVE;
        }

        ProbeForRead(puiNumDevices, sizeof(*puiNumDevices), sizeof(DWORD));
        if (*puiNumDevices < cDevices)
        {
            ProbeForWrite(puiNumDevices, sizeof(*puiNumDevices), sizeof(DWORD));
            *puiNumDevices = cDevices;
            EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
            _SEH2_LEAVE;
        }

        ProbeForWrite(pRawInputDeviceList, cDevices * sizeof(*pRawInputDeviceList), sizeof(PVOID));
        for (DeviceInfo = gpInputDeviceInfo, i = 0; DeviceInfo && i < cDevices; DeviceInfo = DeviceInfo->pNextDeviceInfo, i++)
        {
            pRawInputDeviceList[i].hDevice = DeviceInfo;
            pRawInputDeviceList[i].dwType = DeviceInfo->DeviceType;
        }
        Ret = cDevices;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    UserLeave();
    ReleaseDeviceInfoListMutex();
    return Ret;
}

NtUserGetRawInputData(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    PTHREADINFO pti;
    PUSER_MESSAGE_QUEUE MessageQueue;
    PUSER_RAWINPUT RawEntry;

    if (cbSizeHeader != sizeof(RAWINPUTHEADER) || !pcbSize)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ~0u;
    }

    if (!hRawInput)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ~0u;
    }

    pti = PsGetCurrentThreadWin32Thread();
    MessageQueue = pti ? pti->MessageQueue : NULL;
    RawEntry = RawInputEntryFromHandle(MessageQueue, hRawInput);
    if (!RawEntry)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ~0u;
    }

    return RawInputCopyData(&RawEntry->RawInput, uiCommand, pData, pcbSize, cbSizeHeader);
}


DWORD
APIENTRY
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
    *puiNumDevices = 2;
    if (!pRawInputDevices)
    {
        EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
        return -1;
    }
    
    RtlCopyMemory(pRawInputDevices, global_pRawInputDevices, 2 * sizeof(RAWINPUTDEVICE));
    return 2;
}

BOOL
APIENTRY
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize)
{
    UINT i;

    if (!pRawInputDevices || !uiNumDevices || cbSize != sizeof(RAWINPUTDEVICE))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!global_pRawInputDevices)
    {
        global_pRawInputDevices = EngAllocMem(NonPagedPool,
                                              2 * sizeof(RAWINPUTDEVICE),
                                              'iwar');
        if (!global_pRawInputDevices)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        RtlZeroMemory(global_pRawInputDevices, 2 * sizeof(RAWINPUTDEVICE));
    }

    for (i = 0; i < uiNumDevices; ++i)
    {
        INT Index;
        const RAWINPUTDEVICE *Device = &pRawInputDevices[i];

        if ((Device->dwFlags & RIDEV_INPUTSINK) && !Device->hwndTarget)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if ((Device->dwFlags & RIDEV_REMOVE) && Device->hwndTarget)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        RawInputWarnUnimplementedFlags(Device);

        if (Device->usUsagePage != HID_USAGE_PAGE_GENERIC)
            continue;

        if (Device->usUsage == HID_USAGE_GENERIC_MOUSE)
            Index = 0;
        else if (Device->usUsage == HID_USAGE_GENERIC_KEYBOARD)
            Index = 1;
        else
            continue;

        if (Device->dwFlags & RIDEV_REMOVE)
            RtlZeroMemory(&global_pRawInputDevices[Index], sizeof(RAWINPUTDEVICE));
        else
            global_pRawInputDevices[Index] = *Device;
    }

    RawInputEnabled =
        (global_pRawInputDevices[0].usUsage != 0) ||
        (global_pRawInputDevices[1].usUsage != 0);

    return TRUE;
}
