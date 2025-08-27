 
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

PRAWINPUTDEVICE global_pRawInputDevices = NULL;
BOOLEAN RawInputEnabled = FALSE;

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

/* This file is pretty deeply inspired by how wine seems to do it, but wine can't be DIRECTLY used. */
DWORD
APIENTRY
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    if (cbSizeHeader < sizeof(RAWINPUTHEADER) || !pData || !pcbSize)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    return 0;
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
    PRAWINPUT RawInput = (PRAWINPUT)hRawInput;
    ULONG OutputDataSize;
    if (RawInput->header.dwType  == RIM_TYPEMOUSE)
    {
        OutputDataSize = sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE) - sizeof(*pcbSize);
        RawInput->header.dwType = RIM_TYPEMOUSE; // Assume mouse for now.
        RawInput->header.hDevice = (HANDLE)UlongToHandle((ULONG)0xFFFF); // Device handle, not used here
        RawInput->header.wParam = RIM_INPUT ; // No wParam, not used here.
        RawInput->header.dwSize = OutputDataSize; // Size of the output data.
    
    }
    else if (RawInput->header.dwType == RIM_TYPEKEYBOARD)
    {
        OutputDataSize = sizeof(RAWINPUTHEADER) + sizeof(RAWKEYBOARD) - sizeof(*pcbSize);
        RawInput->header.dwType = RIM_TYPEKEYBOARD; // Assume keyboard for now.
        RawInput->header.hDevice = (HANDLE)UlongToHandle((ULONG)0xFFFC); // Device handle, not used here
        RawInput->header.wParam = RIM_INPUT ; // No wParam, not used here.
        RawInput->header.dwSize = OutputDataSize; // Size of the output data.
        DPRINT("RawINput Keyboard Info: keyboard.MakeCode: %d, keyboard.Flags: %d, keyboard.VKey: %d\n",
                RawInput->data.keyboard.MakeCode,
                RawInput->data.keyboard.Flags,
                RawInput->data.keyboard.VKey);
    }
    else
    {
        DPRINT1("Unknown raw input type %d\n", RawInput->header.dwType);
        EngSetLastError(ERROR_INVALID_PARAMETER);
    }

    if (!pData)
    {
        *pcbSize = OutputDataSize;
        EngSetLastError(ERROR_SUCCESS);
        return S_OK; // Return the size needed.
    }
    RtlCopyMemory(pData, RawInput, OutputDataSize);
    EngFreeMem(RawInput); // Free the memory allocated in keyboard.c or mouse.c
    return OutputDataSize;
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
