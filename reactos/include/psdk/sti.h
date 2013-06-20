/*
 * Copyright (C) 2009 Damjan Jovanovic
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_STI_H
#define __WINE_STI_H

#include <objbase.h>
/* #include <stireg.h> */
/* #include <stierr.h> */

#include <pshpack8.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(CLSID_Sti, 0xB323F8E0L, 0x2E68, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

DEFINE_GUID(IID_IStillImageW, 0x641BD880, 0x2DC8, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

DEFINE_GUID(IID_IStillImageA, 0xA7B1F740, 0x1D7F, 0x11D1, 0xAC, 0xA9, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

#define STI_VERSION_REAL         0x00000002
#define STI_VERSION_FLAG_UNICODE 0x01000000

#ifndef WINE_NO_UNICODE_MACROS
# ifdef UNICODE
#  define STI_VERSION (STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE)
# else
#  define STI_VERSION (STI_VERSION_REAL)
# endif
#endif

typedef struct IStillImageA *PSTIA;
typedef struct IStillImageW *PSTIW;
DECL_WINELIB_TYPE_AW(PSTI)
typedef struct IStillImageA *LPSTILLIMAGEA;
typedef struct IStillImageW *LPSTILLIMAGEW;
DECL_WINELIB_TYPE_AW(LPSTILLIMAGE)
typedef struct IStiDeviceA *PSTIDEVICEA;
typedef struct IStiDeviceW *PSTIDEVICEW;
DECL_WINELIB_TYPE_AW(PSTIDEVICE)

HRESULT WINAPI StiCreateInstanceA(HINSTANCE hinst, DWORD dwVer, PSTIA *ppSti, LPUNKNOWN pUnkOuter);
HRESULT WINAPI StiCreateInstanceW(HINSTANCE hinst, DWORD dwVer, PSTIW *ppSti, LPUNKNOWN pUnkOuter);
#define        StiCreateInstance WINELIB_NAME_AW(StiCreateInstance)

typedef DWORD STI_DEVICE_TYPE;
typedef enum _STI_DEVICE_MJ_TYPE
{
    StiDeviceTypeDefault           = 0,
    StiDeviceTypeScanner           = 1,
    StiDeviceTypeDigitalCamera     = 2,
    StiDeviceTypeStreamingVideo    = 3
} STI_DEVICE_MJ_TYPE;

#define GET_STIDEVICE_TYPE(dwDevType) HIWORD(dwDevType)
#define GET_STIDEVICE_SUBTYPE(dwDevType) LOWORD(dwDevType)

typedef struct _STI_DEV_CAPS {
    DWORD dwGeneric;
} STI_DEV_CAPS, *PSTI_DEV_CAPS;

#define STI_MAX_INTERNAL_NAME_LENGTH 128

typedef struct _STI_DEVICE_INFORMATIONW {
    DWORD dwSize;
    STI_DEVICE_TYPE DeviceType;
    WCHAR szDeviceInternalName[STI_MAX_INTERNAL_NAME_LENGTH];
    STI_DEV_CAPS DeviceCapabilities;
    DWORD dwHardwareConfiguration;
    LPWSTR pszVendorDescription;
    LPWSTR pszDeviceDescription;
    LPWSTR pszPortName;
    LPWSTR pszPropProvider;
    LPWSTR pszLocalName;
} STI_DEVICE_INFORMATIONW, *PSTI_DEVICE_INFORMATIONW;

typedef STI_DEVICE_INFORMATIONW STI_DEVICE_INFORMATION;
typedef PSTI_DEVICE_INFORMATIONW PSTI_DEVICE_INFORMATION;

#define MAX_NOTIFICATION_DATA 64

typedef struct _STINOTIFY {
    DWORD dwSize;
    GUID guidNotificationCode;
    BYTE abNotificationData[MAX_NOTIFICATION_DATA];
} STINOTIFY,*LPSTINOTIFY;

#define INTERFACE IStillImageW
DECLARE_INTERFACE_(IStillImageW, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IStillImageW methods ***/
    STDMETHOD(Initialize)(THIS_ HINSTANCE hinst, DWORD dwVersion) PURE;
    STDMETHOD(GetDeviceList)(THIS_ DWORD dwType, DWORD dwFlags, DWORD *pdwItemsReturned, LPVOID *ppBuffer) PURE;
    STDMETHOD(GetDeviceInfo)(THIS_ LPWSTR pwszDeviceName, LPVOID *ppBuffer) PURE;
    STDMETHOD(CreateDevice)(THIS_ LPWSTR pwszDeviceName, DWORD dwMode, PSTIDEVICEW *pDevice, LPUNKNOWN pUnkOuter) PURE;
    STDMETHOD(GetDeviceValue)(THIS_ LPWSTR pwszDeviceName, LPWSTR pValueName, LPDWORD pType, LPBYTE pData, LPDWORD cbData) PURE;
    STDMETHOD(SetDeviceValue)(THIS_ LPWSTR pwszDeviceName, LPWSTR pValueName, DWORD type, LPBYTE pData, DWORD cbData) PURE;
    STDMETHOD(GetSTILaunchInformation)(THIS_ LPWSTR pwszDeviceName, DWORD *pdwEventCode, LPWSTR pwszEventName) PURE;
    STDMETHOD(RegisterLaunchApplication)(THIS_ LPWSTR pwszAppName, LPWSTR pwszCommandLine) PURE;
    STDMETHOD(UnregisterLaunchApplication)(THIS_ LPWSTR pwszAppName) PURE;
    STDMETHOD(EnableHwNotifications)(THIS_ LPCWSTR pwszDeviceName, BOOL bNewState) PURE;
    STDMETHOD(GetHwNotificationState)(THIS_ LPCWSTR pwszDeviceName, BOOL *pbCurrentState) PURE;
    STDMETHOD(RefreshDeviceBus)(THIS_ LPCWSTR pwszDeviceName) PURE;
    STDMETHOD(LaunchApplicationForDevice)(THIS_ LPWSTR pwszDeviceName, LPWSTR pwszAppName, LPSTINOTIFY pStiNotify);
    STDMETHOD(SetupDeviceParameters)(THIS_ PSTI_DEVICE_INFORMATIONW pDevInfo);
    STDMETHOD(WriteToErrorLog)(THIS_ DWORD dwMessageType, LPCWSTR pszMessage) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IStillImage_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IStillImage_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IStillImage_Release(p)            (p)->lpVtbl->Release(p)
/*** IStillImage methods ***/
#define IStillImage_Initialize(p,a,b)                   (p)->lpVtbl->Initialize(p,a,b)
#define IStillImage_GetDeviceList(p,a,b,c,d)            (p)->lpVtbl->GetDeviceList(p,a,b,c,d)
#define IStillImage_GetDeviceInfo(p,a,b)                (p)->lpVtbl->GetDeviceInfo(p,a,b)
#define IStillImage_CreateDevice(p,a,b,c,d)             (p)->lpVtbl->CreateDevice(p,a,b,c,d)
#define IStillImage_GetDeviceValue(p,a,b,c,d,e)         (p)->lpVtbl->GetDeviceValue(p,a,b,c,d,e)
#define IStillImage_SetDeviceValue(p,a,b,c,d,e)         (p)->lpVtbl->SetDeviceValue(p,a,b,c,d,e)
#define IStillImage_GetSTILaunchInformation(p,a,b,c)    (p)->lpVtbl->GetSTILaunchInformation(p,a,b,c)
#define IStillImage_RegisterLaunchApplication(p,a,b)    (p)->lpVtbl->RegisterLaunchApplication(p,a,b)
#define IStillImage_UnregisterLaunchApplication(p,a)    (p)->lpVtbl->UnregisterLaunchApplication(p,a)
#define IStillImage_EnableHwNotifications(p,a,b)        (p)->lpVtbl->EnableHwNotifications(p,a,b)
#define IStillImage_GetHwNotificationState(p,a,b)       (p)->lpVtbl->GetHwNotificationState(p,a,b)
#define IStillImage_RefreshDeviceBus(p,a)               (p)->lpVtbl->RefreshDeviceBus(p,a)
#define IStillImage_LaunchApplicationForDevice(p,a,b,c) (p)->lpVtbl->LaunchApplicationForDevice(p,a,b,c)
#define IStillImage_SetupDeviceParameters(p,a)          (p)->lpVtbl->SetupDeviceParameters(p,a)
#define IStillImage_WriteToErrorLog(p,a,b)              (p)->lpVtbl->WriteToErrorLog(p,a,b)
#else
/*** IUnknown methods ***/
#define IStillImage_QueryInterface(p,a,b) (p)->QueryInterface(a,b)
#define IStillImage_AddRef(p)             (p)->AddRef()
#define IStillImage_Release(p)            (p)->Release()
/*** IStillImage methods ***/
#define IStillImage_Initialize(p,a,b)                   (p)->Initialize(a,b)
#define IStillImage_GetDeviceList(p,a,b,c,d)            (p)->GetDeviceList(a,b,c,d)
#define IStillImage_GetDeviceInfo(p,a,b)                (p)->GetDeviceInfo(a,b)
#define IStillImage_CreateDevice(p,a,b,c,d)             (p)->CreateDevice(a,b,c,d)
#define IStillImage_GetDeviceValue(p,a,b,c,d,e)         (p)->GetDeviceValue(a,b,c,d,e)
#define IStillImage_SetDeviceValue(p,a,b,c,d,e)         (p)->SetDeviceValue(a,b,c,d,e)
#define IStillImage_GetSTILaunchInformation(p,a,b,c)    (p)->GetSTILaunchInformation(a,b,c)
#define IStillImage_RegisterLaunchApplication(p,a,b)    (p)->RegisterLaunchApplication(a,b)
#define IStillImage_UnregisterLaunchApplication(p,a)    (p)->UnregisterLaunchApplication(a)
#define IStillImage_EnableHwNotifications(p,a,b)        (p)->EnableHwNotifications(a,b)
#define IStillImage_GetHwNotificationState(p,a,b)       (p)->GetHwNotificationState(a,b)
#define IStillImage_RefreshDeviceBus(p,a)               (p)->RefreshDeviceBus(a)
#define IStillImage_LaunchApplicationForDevice(p,a,b,c) (p)->LaunchApplicationForDevice(a,b,c)
#define IStillImage_SetupDeviceParameters(p,a)          (p)->SetupDeviceParameters(a)
#define IStillImage_WriteToErrorLog(p,a,b)              (p)->WriteToErrorLog(a,b)
#endif

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_STI_H */
