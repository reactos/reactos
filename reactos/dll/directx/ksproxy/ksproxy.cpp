/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/ksproxy.c
 * PURPOSE:         ActiveMovie Proxy functions
 *
 * PROGRAMMERS:     Dmitry Chapyshev
                    Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"


const GUID CLSID_KsClockForwarder              = {0x877e4351, 0x6fea, 0x11d0, {0xb8, 0x63, 0x00, 0xaa, 0x00, 0xa2, 0x16, 0xa1}};
const GUID CLSID_KsQualityForwarder            = {0xe05592e4, 0xc0b5, 0x11d0, {0xa4, 0x39, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96}};


#ifndef _MSC_VER
const GUID CLSID_KsIBasicAudioInterfaceHandler = {0xb9f8ac3e, 0x0f71, 0x11d2, {0xb7, 0x2c, 0x00, 0xc0, 0x4f, 0xb6, 0xbd, 0x3d}};
const GUID KSPROPSETID_Pin                     = {0x8C134960, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSINTERFACESETID_Standard           = {STATIC_KSINTERFACESETID_Standard};
const GUID CLSID_Proxy                         = {0x17CCA71B, 0xECD7, 0x11D0, {0xB9, 0x08, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
#endif

static INTERFACE_TABLE InterfaceTable[] =
{
    {&MEDIATYPE_Audio, CKsDataTypeHandler_Constructor},
    {&KSINTERFACESETID_Standard, CKsInterfaceHandler_Constructor},
    {&CLSID_KsClockForwarder, CKsClockForwarder_Constructor},
    {&CLSID_KsQualityForwarder, CKsQualityForwarder_Constructor},
    {&IID_IVPConfig, CVPConfig_Constructor},
    {&IID_IVPVBIConfig, CVPVBIConfig_Constructor},
    {&CLSID_KsIBasicAudioInterfaceHandler, CKsBasicAudio_Constructor},
    {&CLSID_Proxy, CKsProxy_Constructor},
    {NULL, NULL}
};

KSDDKAPI
HRESULT
WINAPI
KsSynchronousDeviceControl(
    HANDLE     Handle,
    ULONG      IoControl,
    PVOID      InBuffer,
    ULONG      InLength,
    PVOID      OutBuffer,
    ULONG      OutLength,
    PULONG     BytesReturned)
{
    OVERLAPPED Overlapped;
    DWORD Transferred;

    /* zero overlapped */
    RtlZeroMemory(&Overlapped, sizeof(OVERLAPPED));

    /* create notification event */
    Overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    if (!Overlapped.hEvent)
    {
        /* failed */
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    if (!DeviceIoControl(Handle, IoControl, InBuffer, InLength, OutBuffer, OutLength, BytesReturned, &Overlapped))
    {
        /* operation failed */
        if (GetLastError() != ERROR_IO_PENDING)
        {
            /* failed */
            CloseHandle(Overlapped.hEvent);
            return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        }
    }

    /* get result of pending operation */
    if (!GetOverlappedResult(Handle, &Overlapped, &Transferred, TRUE))
    {
        /* failed */
        CloseHandle(Overlapped.hEvent);
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    /* store number of bytes transferred */
    *BytesReturned = Transferred;

    /* close event object */
    CloseHandle(Overlapped.hEvent);

    /* done */
    return NOERROR;
}

KSDDKAPI
HRESULT
WINAPI
KsResolveRequiredAttributes(
    PKSDATARANGE     DataRange,
    KSMULTIPLE_ITEM  *Attributes OPTIONAL)
{
    //UNIMPLEMENTED
    return NOERROR;
}

KSDDKAPI
HRESULT
WINAPI
KsOpenDefaultDevice(
    REFGUID      Category,
    ACCESS_MASK  Access,
    PHANDLE      DeviceHandle)
{
    HDEVINFO hList;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData;
    WCHAR Path[MAX_PATH+sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)];

    /* open device list */
    hList = SetupDiGetClassDevsW(&Category, NULL, NULL, DIGCF_DEVICEINTERFACE  | DIGCF_PRESENT);

    if (hList == INVALID_HANDLE_VALUE)
    {
        /* failed */
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    /* setup parameters */
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (SetupDiEnumDeviceInterfaces(hList, NULL, &Category, 0, &DeviceInterfaceData))
    {
        /* setup interface data struct */
        DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)Path;
        DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        /* get device interface details */
        if (SetupDiGetDeviceInterfaceDetailW(hList, &DeviceInterfaceData, DeviceInterfaceDetailData, sizeof(Path), NULL, NULL))
        {
            /* open device */
            *DeviceHandle = CreateFileW(DeviceInterfaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL);

            if (*DeviceHandle != INVALID_HANDLE_VALUE)
            {
                /* operation succeeded */
                SetupDiDestroyDeviceInfoList(hList);
                return NOERROR;
            }
        }
    }

    /* free device list */
    SetupDiDestroyDeviceInfoList(hList);

    /* failed */
    return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
}

KSDDKAPI
HRESULT
WINAPI
KsGetMultiplePinFactoryItems(
    HANDLE  FilterHandle,
    ULONG   PinFactoryId,
    ULONG   PropertyId,
    PVOID   *Items)
{
    KSP_PIN Property;
    ULONG BytesReturned, NumData;
    HRESULT hResult;

    /* zero pin property */
    RtlZeroMemory(&Property, sizeof(KSP_PIN));
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = PropertyId;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.PinId = PinFactoryId;

    /* query pin factory */
    hResult = KsSynchronousDeviceControl(FilterHandle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (hResult == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INSUFFICIENT_BUFFER))
    {
        /* buffer too small */
        hResult = KsSynchronousDeviceControl(FilterHandle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), (PVOID)&NumData, sizeof(ULONG), &BytesReturned);

        if (SUCCEEDED(hResult))
        {
            /* store required data size */
            BytesReturned = NumData;
            hResult = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_MORE_DATA);
        }
    }

    if (hResult == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_MORE_DATA))
    {
        /* allocate data */
        *Items = CoTaskMemAlloc(BytesReturned);

        if (!*Items)
        {
            /* no memory */
            return E_OUTOFMEMORY;
        }

        /* retry querying property */
        hResult = KsSynchronousDeviceControl(FilterHandle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), (PVOID)*Items, BytesReturned, &BytesReturned);

        /* check for success */
        if (FAILED(hResult))
        {
            /* free memory */
            CoTaskMemFree(*Items);
        }
    }

    /* done */
    return hResult;
}

KSDDKAPI
HRESULT
WINAPI
KsGetMediaTypeCount(
    HANDLE  FilterHandle,
    ULONG   PinFactoryId,
    ULONG   *MediaTypeCount)
{
    PKSMULTIPLE_ITEM MultipleItem;
    HRESULT hr;

    /* try get contrained data ranges */
    hr = KsGetMultiplePinFactoryItems(FilterHandle, PinFactoryId, KSPROPERTY_PIN_CONSTRAINEDDATARANGES, (PVOID*)&MultipleItem);

    /* check for failure*/
    if (FAILED(hr))
    {
        /* try getting default data ranges */
        hr = KsGetMultiplePinFactoryItems(FilterHandle, PinFactoryId, KSPROPERTY_PIN_DATARANGES, (PVOID*)&MultipleItem);
    }

    if (SUCCEEDED(hr))
    {
        /* store number of media types */
        *MediaTypeCount = MultipleItem->Count;

        /* free memory */
        CoTaskMemFree(MultipleItem);
    }

    /* done */
    return hr;
}

KSDDKAPI
HRESULT
WINAPI
KsGetMediaType(
    int  Position,
    AM_MEDIA_TYPE  *AmMediaType,
    HANDLE         FilterHandle,
    ULONG          PinFactoryId)
{
    HRESULT hr;
    PKSMULTIPLE_ITEM ItemList;
    int i = 0;
    PKSDATAFORMAT DataFormat;

    if (Position < 0)
        return E_INVALIDARG;

    // get current supported ranges
    hr = KsGetMultiplePinFactoryItems(FilterHandle, PinFactoryId, KSPROPERTY_PIN_CONSTRAINEDDATARANGES, (PVOID*)&ItemList);
    if (FAILED(hr))
    {
        // get standard dataranges
        hr = KsGetMultiplePinFactoryItems(FilterHandle, PinFactoryId, KSPROPERTY_PIN_DATARANGES, (PVOID*)&ItemList);

        //check for success
        if (FAILED(hr))
            return hr;
    }

    if ((ULONG)Position >= ItemList->Count)
    {
        // out of bounds
        CoTaskMemFree(ItemList);
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NO_MORE_ITEMS);
    }

    // goto first datarange
    DataFormat = (PKSDATAFORMAT)(ItemList + 1);

    while(i != Position)
    {
        // goto next format;
        DataFormat = (PKSDATAFORMAT)(ULONG_PTR)(DataFormat + DataFormat->FormatSize);
        i++;
    }


    DataFormat->FormatSize -= sizeof(KSDATAFORMAT);
    if (DataFormat->FormatSize)
    {
         // copy extra format buffer
        AmMediaType->pbFormat = (BYTE*)CoTaskMemAlloc(DataFormat->FormatSize);
        if (!AmMediaType->pbFormat)
        {
            // not enough memory
            CoTaskMemFree(ItemList);
            return E_OUTOFMEMORY;
        }
        // copy format buffer
        CopyMemory(AmMediaType->pbFormat, (DataFormat + 1), DataFormat->FormatSize);
        AmMediaType->cbFormat = DataFormat->FormatSize;
    }
    else
    {
        // no format buffer
        AmMediaType->pbFormat = NULL;
        AmMediaType->cbFormat = 0;
    }

    // copy type info
    CopyMemory(&AmMediaType->majortype, &DataFormat->MajorFormat, sizeof(GUID));
    CopyMemory(&AmMediaType->subtype, &DataFormat->SubFormat, sizeof(GUID));
    CopyMemory(&AmMediaType->formattype, &DataFormat->Specifier, sizeof(GUID));
    AmMediaType->bTemporalCompression = FALSE; //FIXME verify
    AmMediaType->pUnk = NULL; //FIXME
    AmMediaType->lSampleSize = DataFormat->SampleSize;
    AmMediaType->bFixedSizeSamples = (AmMediaType->lSampleSize) ? TRUE : FALSE;

    // free dataformat list
    CoTaskMemFree(ItemList);

    return NOERROR;
}

extern "C"
KSDDKAPI
HRESULT
WINAPI
DllUnregisterServer(void)
{
    return S_OK;
}

extern "C"
KSDDKAPI
HRESULT
WINAPI
DllRegisterServer(void)
{
    return S_OK;
}

KSDDKAPI
HRESULT
WINAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv)
{
    UINT i;
    HRESULT hres = E_OUTOFMEMORY;
    IClassFactory * pcf = NULL;	

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    for (i = 0; InterfaceTable[i].riid; i++) 
    {
        if (IsEqualIID(*InterfaceTable[i].riid, rclsid)) 
        {
            pcf = CClassFactory_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
            break;
        }
    }

    if (!pcf) 
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    hres = pcf->QueryInterface(riid, ppv);
    pcf->Release();

    return hres;
}

KSDDKAPI
HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return S_OK;
}

