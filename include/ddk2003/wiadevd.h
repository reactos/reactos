

#ifndef _WIADEVD_H_INCLUDED
#define _WIADEVD_H_INCLUDED

#include "wia.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include <pshpack8.h>

#define SHELLEX_WIAUIEXTENSION_NAME TEXT("WiaDialogExtensionHandlers")
#define CFSTR_WIAITEMNAMES TEXT("WIAItemNames")
#define CFSTR_WIAITEMPTR   TEXT("WIAItemPointer")

typedef struct tagDEVICEDIALOGDATA
{
    DWORD cbSize;
    HWND hwndParent;
    IWiaItem *pIWiaItemRoot;
    DWORD dwFlags;
    LONG lIntent;
    LONG lItemCount;
    IWiaItem **ppWiaItems;
} DEVICEDIALOGDATA, *LPDEVICEDIALOGDATA, *PDEVICEDIALOGDATA;

HRESULT WINAPI DeviceDialog( PDEVICEDIALOGDATA pDeviceDialogData );

#undef  INTERFACE
#define INTERFACE IWiaUIExtension
DECLARE_INTERFACE_(IWiaUIExtension, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(DeviceDialog)( THIS_ PDEVICEDIALOGDATA pDeviceDialogData ) PURE;
    STDMETHOD(GetDeviceIcon)(THIS_ BSTR bstrDeviceId, HICON *phIcon, ULONG nSize ) PURE;
    STDMETHOD(GetDeviceBitmapLogo)(THIS_ BSTR bstrDeviceId, HBITMAP *phBitmap, ULONG nMaxWidth, ULONG nMaxHeight ) PURE;
};

DEFINE_GUID(IID_IWiaUIExtension, 0xDA319113, 0x50EE, 0x4C80, 0xB4, 0x60, 0x57, 0xD0, 0x05, 0xD4, 0x4A, 0x2C);

typedef HRESULT (WINAPI *DeviceDialogFunction)(PDEVICEDIALOGDATA);

#include <poppack.h>

#if defined(__cplusplus)
};
#endif

#endif


