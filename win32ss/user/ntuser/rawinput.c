/*
 * PROJECT:     ReactOS Win32k subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions related to Raw Input handling
 * COPYRIGHT:   Copyright 2026 Hervé Poussineau
 */

#include <win32k.h>

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

