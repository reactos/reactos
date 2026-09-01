/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Functions related to Raw Input handling
 * FILE:             win32ss/user/ntuser/rawinput.c
 * PROGRAMMER:       Hervé Poussineau
 */

#include <win32k.h>

DWORD
APIENTRY
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize)
{
    PINPUT_DEVICE_INFO DeviceInfo;
    UINT cbSize, cbRequiredSize;
    UINT Ret = (UINT)-1;

    _SEH2_TRY
    {
        ProbeForRead(pcbSize, sizeof(*pcbSize), 1);
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

    UserEnterExclusive(); // FIXME: or Shared ?
    if (!DeviceInfo)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        goto cleanup;
    }

    switch (uiCommand)
    {
        case RIDI_PREPARSEDDATA:
            if (DeviceInfo->DeviceType == INPUT_DEVICE_TYPE_HID)
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
        }
        else
        {
            if (cbSize < cbRequiredSize)
            {
                ProbeForWrite(pcbSize, sizeof(*pcbSize), sizeof(DWORD));
                *pcbSize = cbRequiredSize;
                EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                ProbeForWrite(pData, cbRequiredSize, sizeof(DWORD));
                switch (uiCommand)
                {
                    case RIDI_PREPARSEDDATA:
                        if (DeviceInfo->DeviceType == INPUT_DEVICE_TYPE_HID)
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
                        }
                        else
                        {
                            ProbeForWrite(prdi, sizeof(*prdi), sizeof(DWORD));
                            RtlZeroMemory(prdi, sizeof(*prdi));
                            prdi->cbSize = sizeof(*prdi);

                            switch (DeviceInfo->DeviceType)
                            {
                                case INPUT_DEVICE_TYPE_MOUSE:
                                    prdi->dwType = RIM_TYPEMOUSE;
                                    prdi->mouse.dwId = DeviceInfo->Mouse.Attributes.MouseIdentifier & ~HORIZONTAL_WHEEL_PRESENT;
                                    prdi->mouse.dwNumberOfButtons = DeviceInfo->Mouse.Attributes.NumberOfButtons;
                                    prdi->mouse.dwSampleRate = DeviceInfo->Mouse.Attributes.SampleRate;
                                    prdi->mouse.fHasHorizontalWheel = !!(DeviceInfo->Mouse.Attributes.MouseIdentifier & HORIZONTAL_WHEEL_PRESENT);
                                    break;

                                case INPUT_DEVICE_TYPE_KEYBOARD:
                                    prdi->dwType = RIM_TYPEKEYBOARD;
                                    prdi->keyboard.dwType = DeviceInfo->Keyboard.Attributes.KeyboardIdentifier.Type;
                                    prdi->keyboard.dwSubType = DeviceInfo->Keyboard.Attributes.KeyboardIdentifier.Subtype;
                                    prdi->keyboard.dwKeyboardMode = DeviceInfo->Keyboard.Attributes.KeyboardMode;
                                    prdi->keyboard.dwNumberOfFunctionKeys = DeviceInfo->Keyboard.Attributes.NumberOfFunctionKeys;
                                    prdi->keyboard.dwNumberOfIndicators = DeviceInfo->Keyboard.Attributes.NumberOfIndicators;
                                    prdi->keyboard.dwNumberOfKeysTotal = DeviceInfo->Keyboard.Attributes.NumberOfKeysTotal;
                                    break;

                                case INPUT_DEVICE_TYPE_HID:
                                    prdi->dwType = RIM_TYPEHID;
                                    prdi->hid.dwVendorId = DeviceInfo->Hid.CollectionInformation.VendorID;
                                    prdi->hid.dwProductId = DeviceInfo->Hid.CollectionInformation.ProductID;
                                    prdi->hid.dwVersionNumber = DeviceInfo->Hid.CollectionInformation.VersionNumber;
                                    prdi->hid.usUsagePage = DeviceInfo->Hid.Caps.UsagePage;
                                    prdi->hid.usUsage = DeviceInfo->Hid.Caps.Usage;
                                    break;

                                default:
                                    ASSERT(FALSE);
                                    break;
                            }
                        }
                        break;
                    }
                    default:
                        ASSERT(FALSE);
                        break;
                }
                Ret = cbRequiredSize;
            }
        }
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

DWORD
APIENTRY
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
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

    UserEnterExclusive(); // FIXME: or Shared ?

    _SEH2_TRY
    {
        if (!pRawInputDeviceList)
        {
            ProbeForWrite(puiNumDevices, sizeof(*puiNumDevices), sizeof(DWORD));
            *puiNumDevices = cDevices;
            Ret = 0;
        }
        else
        {
            ProbeForRead(puiNumDevices, sizeof(*puiNumDevices), sizeof(DWORD));
            if (*puiNumDevices < cDevices)
            {
                ProbeForWrite(puiNumDevices, sizeof(*puiNumDevices), sizeof(DWORD));
                *puiNumDevices = cDevices;
                EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                ProbeForWrite(pRawInputDeviceList, cDevices * sizeof(*pRawInputDeviceList), sizeof(PVOID));
                for (DeviceInfo = gpInputDeviceInfo, i = 0; DeviceInfo && i < cDevices; DeviceInfo = DeviceInfo->pNextDeviceInfo, i++)
                {
                    pRawInputDeviceList[i].hDevice = DeviceInfo;
                    pRawInputDeviceList[i].dwType = DeviceInfo->DeviceType;
                }
                Ret = cDevices;
            }
        }
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

