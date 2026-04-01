 
#include <win32k.h>
#define NDEBUG
#include <debug.h>

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

    if (MessageQueue->CurrentRawInput == hRawInput)
        MessageQueue->CurrentRawInput = NULL;

    RawInputFreeEntry(RawEntry);
    return TRUE;
}

VOID
FASTCALL
UserCleanupRawInput(
    PUSER_MESSAGE_QUEUE MessageQueue)
{
    MessageQueue->CurrentRawInput = NULL;
    while (!IsListEmpty(&MessageQueue->RawInputListHead))
    {
        PUSER_RAWINPUT RawEntry;
        RawEntry = CONTAINING_RECORD(MessageQueue->RawInputListHead.Flink,
                                     USER_RAWINPUT,
                                     ListEntry);
        RawInputFreeEntry(RawEntry);
    }
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

    if (!pData)
    {
        RawEntry = RawInputEntryFromHandle(MessageQueue, MessageQueue->CurrentRawInput);
        if (RawEntry)
        {
            *pcbSize = RawEntry->RawInput.header.dwSize;
            EngSetLastError(ERROR_SUCCESS);
            return 0;
        }

        Entry = MessageQueue->HardwareMessagesListHead.Flink;
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
    RawEntry = RawInputEntryFromHandle(MessageQueue, MessageQueue->CurrentRawInput);
    if (RawEntry)
    {
        UINT RawSize = RawEntry->RawInput.header.dwSize;

        if (UsedSize + RawSize > BufferSize)
        {
            *pcbSize = RawSize;
            EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
            return ~0u;
        }

        RtlCopyMemory((PBYTE)pData + UsedSize, &RawEntry->RawInput, RawSize);
        UsedSize += RawSize;
        Count++;
        MessageQueue->CurrentRawInput = NULL;
        RawInputFreeEntry(RawEntry);
    }

    Entry = MessageQueue->HardwareMessagesListHead.Flink;
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

RIDDevice
APIENTRY
GetDeviceFromHandle(HANDLE hDevice)
{
    for(int i = 0; i < 2; i++)
    {
        if (RIDDevices[i].file == hDevice)
        {
            return RIDDevices[i];
        }
    }

    return (RIDDevice){ NULL, {0}, 0 }; // Return an empty RIDDevice if not found.
}
#define NAME L"ReactOS Raw Input Device"
DWORD
APIENTRY
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize
)
{
    WCHAR NAMELoc[] = NAME;
    DWORD len, data_len;
    len = data_len = *pcbSize;
    RIDDevice RIDDevicesLoc = GetDeviceFromHandle(hDevice);
    RID_DEVICE_INFO info;
    switch (uiCommand)
    {
    case RIDI_DEVICENAME:
          if ((len = wcslen( NAME ) + 1) <= data_len && pData)
            memcpy( pData, NAMELoc, len * sizeof(WCHAR) );
        *pcbSize = len;
        break;
        break;

    case RIDI_DEVICEINFO:
        if ((len = sizeof(info)) <= data_len && pData)
            memcpy( pData, &RIDDevicesLoc.info, len );
        *pcbSize = len;
        break;
    default:
        DPRINT1("Unknown command %d\n", uiCommand);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ~0u;
    }
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
    UINT DeviceCount = 0; // Number of valid RIT Devices.
    if (cbSize < sizeof(RAWINPUTDEVICELIST))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ~0u;
    }

    if (!puiNumDevices)
    {
        EngSetLastError(ERROR_NOACCESS);
        return ~0u;
    }

    // Manually enumerate the device list.
    HandleDeviceEnumeration(RIM_TYPEMOUSE);
    HandleDeviceEnumeration(RIM_TYPEKEYBOARD);

    /* Hacky parse */
    if (RIDDevices[0].file)
    {
        if (*puiNumDevices < ++DeviceCount || !pRawInputDeviceList) goto skip;
        DeviceCount++;
        pRawInputDeviceList->hDevice = RIDDevices[0].file;
        pRawInputDeviceList->dwType = RIDDevices[0].Type;
        pRawInputDeviceList++;
    }

    if (RIDDevices[1].file)
    {
        if (*puiNumDevices < ++DeviceCount || !pRawInputDeviceList) goto skip;
        DeviceCount++;
        pRawInputDeviceList->hDevice = RIDDevices[1].file;
        pRawInputDeviceList->dwType = RIDDevices[1].Type;
        pRawInputDeviceList++;
    }

skip:
    if (!pRawInputDeviceList)
    {
        *puiNumDevices = DeviceCount;
        EngSetLastError(ERROR_SUCCESS);
        return 0;
    }

    return 0;
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
    RawInputEnabled = TRUE;
    global_pRawInputDevices = EngAllocMem(NonPagedPool, 2 * sizeof(RAWINPUTDEVICE), 'iwar');

    return TRUE;
}
