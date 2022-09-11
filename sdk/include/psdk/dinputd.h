/*
 * Copyright (C) the Wine project
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

#ifndef __DINPUTD_INCLUDED__
#define __DINPUTD_INCLUDED__

#define COM_NO_WINDOWS_H
#include <objbase.h>

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif

DEFINE_GUID(IID_IDirectInputJoyConfig8, 0xEB0D7DFA,0x1990,0x4F27,0xB4,0xD6,0xED,0xF2,0xEE,0xC4,0xA4,0x4C);
DEFINE_GUID(IID_IDirectInputPIDDriver, 0xeec6993a,0xb3fd,0x11d2,0xa9,0x16,0x00,0xc0,0x4f,0xb9,0x86,0x38);

typedef struct IDirectInputJoyConfig8 *LPDIRECTINPUTJOYCONFIG8;


typedef BOOL (CALLBACK *LPDIJOYTYPECALLBACK)(LPCWSTR, LPVOID);

#define MAX_JOYSTRING 256
#ifndef MAX_JOYSTICKOEMVXDNAME
#define MAX_JOYSTICKOEMVXDNAME 260
#endif

#define JOY_POV_NUMDIRS         4
#define JOY_POVVAL_FORWARD      0
#define JOY_POVVAL_BACKWARD     1
#define JOY_POVVAL_LEFT         2
#define JOY_POVVAL_RIGHT        3

#define DIERR_NOMOREITEMS       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NO_MORE_ITEMS)

typedef struct joypos_tag
{
    DWORD dwX;
    DWORD dwY;
    DWORD dwZ;
    DWORD dwR;
    DWORD dwU;
    DWORD dwV;
} JOYPOS, *LPJOYPOS;

typedef struct joyrange_tag
{
    JOYPOS jpMin;
    JOYPOS jpMax;
    JOYPOS jpCenter;
} JOYRANGE, *LPJOYRANGE;

typedef struct joyreguservalues_tag
{
    DWORD dwTimeOut;
    JOYRANGE jrvRanges;
    JOYPOS jpDeadZone;
} JOYREGUSERVALUES, *LPJOYREGUSERVALUES;

typedef struct joyreghwsettings_tag
{
    DWORD dwFlags;
    DWORD dwNumButtons;
} JOYREGHWSETTINGS, *LPJOYHWSETTINGS;

typedef struct joyreghwvalues_tag
{
    JOYRANGE jrvHardware;
    DWORD dwPOVValues[JOY_POV_NUMDIRS];
    DWORD dwCalFlags;
} JOYREGHWVALUES, *LPJOYREGHWVALUES;

typedef struct joyreghwconfig_tag
{
    JOYREGHWSETTINGS hws;
    DWORD dwUsageSettings;
    JOYREGHWVALUES hwv;
    DWORD dwType;
    DWORD dwReserved;
} JOYREGHWCONFIG, *LPJOYREGHWCONFIG;

typedef struct DIJOYTYPEINFO_DX5
{
    DWORD dwSize;
    JOYREGHWSETTINGS hws;
    CLSID clsidConfig;
    WCHAR wszDisplayName[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTICKOEMVXDNAME];
} DIJOYTYPEINFO_DX5, *LPDIJOYTYPEINFO_DX5;
typedef const DIJOYTYPEINFO_DX5 *LPCDIJOYTYPEINFO_DX5;

typedef struct DIJOYTYPEINFO_DX6
{
    DWORD dwSize;
    JOYREGHWSETTINGS hws;
    CLSID clsidConfig;
    WCHAR wszDisplayName[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTICKOEMVXDNAME];
    WCHAR wszHardwareId[MAX_JOYSTRING];
    DWORD dwFlags1;
} DIJOYTYPEINFO_DX6, *LPDIJOYTYPEINFO_DX6;
typedef const DIJOYTYPEINFO_DX6 *LPCDIJOYTYPEINFO_DX6;

typedef struct DIJOYTYPEINFO
{
    DWORD dwSize;
    JOYREGHWSETTINGS hws;
    CLSID clsidConfig;
    WCHAR wszDisplayName[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTICKOEMVXDNAME];
    WCHAR wszHardwareId[MAX_JOYSTRING];
    DWORD dwFlags1;
    DWORD dwFlags2;
    WCHAR wszMapFile[MAX_JOYSTRING];
} DIJOYTYPEINFO, *LPDIJOYTYPEINFO;
typedef const DIJOYTYPEINFO *LPCDIJOYTYPEINFO;
#define DIJC_GUIDINSTANCE       0x00000001
#define DIJC_REGHWCONFIGTYPE    0x00000002
#define DIJC_GAIN               0x00000004
#define DIJC_CALLOUT            0x00000008
#define DIJC_WDMGAMEPORT        0x00000010

typedef struct DIJOYCONFIG_DX5
{
    DWORD dwSize;
    GUID guidInstance;
    JOYREGHWCONFIG hwc;
    DWORD dwGain;
    WCHAR wszType[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTRING];
} DIJOYCONFIG_DX5, *LPDIJOYCONFIG_DX5;
typedef const DIJOYCONFIG_DX5 *LPCDIJOYCONFIG_DX5;

typedef struct DIJOYCONFIG
{
    DWORD dwSize;
    GUID guidInstance;
    JOYREGHWCONFIG hwc;
    DWORD dwGain;
    WCHAR wszType[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTRING];
    GUID guidGameport;
} DIJOYCONFIG, *LPDIJOYCONFIG;
typedef const DIJOYCONFIG *LPCDIJOYCONFIG;

typedef struct DIJOYUSERVALUES
{
    DWORD dwSize;
    JOYREGUSERVALUES ruv;
    WCHAR wszGlobalDriver[MAX_JOYSTRING];
    WCHAR wszGameportEmulator[MAX_JOYSTRING];
} DIJOYUSERVALUES, *LPDIJOYUSERVALUES;
typedef const DIJOYUSERVALUES *LPCDIJOYUSERVALUES;


/*****************************************************************************
 * IDirectInputJoyConfig8 interface
 */
#define INTERFACE IDirectInputJoyConfig8
DECLARE_INTERFACE_(IDirectInputJoyConfig8, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirectInputJoyConfig8 methods ***/
    STDMETHOD(Acquire)(THIS) PURE;
    STDMETHOD(Unacquire)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SendNotify)(THIS) PURE;
    STDMETHOD(EnumTypes)(THIS_ LPDIJOYTYPECALLBACK, LPVOID) PURE;
    STDMETHOD(GetTypeInfo)(THIS_ LPCWSTR, LPDIJOYTYPEINFO, DWORD) PURE;
    STDMETHOD(SetTypeInfo)(THIS_ LPCWSTR, LPCDIJOYTYPEINFO, DWORD, LPWSTR) PURE;
    STDMETHOD(DeleteType)(THIS_ LPCWSTR) PURE;
    STDMETHOD(GetConfig)(THIS_ UINT, LPDIJOYCONFIG, DWORD) PURE;
    STDMETHOD(SetConfig)(THIS_ UINT, LPCDIJOYCONFIG, DWORD) PURE;
    STDMETHOD(DeleteConfig)(THIS_ UINT) PURE;
    STDMETHOD(GetUserValues)(THIS_ LPDIJOYUSERVALUES, DWORD) PURE;
    STDMETHOD(SetUserValues)(THIS_ LPCDIJOYUSERVALUES, DWORD) PURE;
    STDMETHOD(AddNewHardware)(THIS_ HWND, REFGUID) PURE;
    STDMETHOD(OpenTypeKey)(THIS_ LPCWSTR, DWORD, PHKEY) PURE;
    STDMETHOD(OpenAppStatusKey)(THIS_ PHKEY) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirectInputJoyConfig8_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectInputJoyConfig8_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IDirectInputJoyConfig8_Release(p)                 (p)->lpVtbl->Release(p)
/*** IDirectInputJoyConfig8 methods ***/
#define IDirectInputJoyConfig8_Acquire(p)                 (p)->lpVtbl->Acquire(p)
#define IDirectInputJoyConfig8_Unacquire(p)               (p)->lpVtbl->Unacquire(p)
#define IDirectInputJoyConfig8_SetCooperativeLevel(p,a,b) (p)->lpVtbl->SetCooperativeLevel(p,a,b)
#define IDirectInputJoyConfig8_SendNotify(p)              (p)->lpVtbl->SendNotify(p)
#define IDirectInputJoyConfig8_EnumTypes(p,a,b)           (p)->lpVtbl->EnumTypes(p,a,b)
#define IDirectInputJoyConfig8_GetTypeInfo(p,a,b,c)       (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IDirectInputJoyConfig8_SetTypeInfo(p,a,b,c,d)     (p)->lpVtbl->SetTypeInfo(p,a,b,c,d)
#define IDirectInputJoyConfig8_DeleteType(p,a)            (p)->lpVtbl->DeleteType(p,a)
#define IDirectInputJoyConfig8_GetConfig(p,a,b,c)         (p)->lpVtbl->GetConfig(p,a,b,c)
#define IDirectInputJoyConfig8_SetConfig(p,a,b,c)         (p)->lpVtbl->SetConfig(p,a,b,c)
#define IDirectInputJoyConfig8_DeleteConfig(p,a)          (p)->lpVtbl->DeleteConfig(p,a)
#define IDirectInputJoyConfig8_GetUserValues(p,a,b)       (p)->lpVtbl->GetUserValues(p,a,b)
#define IDirectInputJoyConfig8_SetUserValues(p,a,b)       (p)->lpVtbl->SetUserValues(p,a,b)
#define IDirectInputJoyConfig8_AddNewHardware(p,a,b)      (p)->lpVtbl->AddNewHardware(p,a,b)
#define IDirectInputJoyConfig8_OpenTypeKey(p,a,b,c)       (p)->lpVtbl->OpenTypeKey(p,a,b,c)
#define IDirectInputJoyConfig8_OpenAppStatusKey(p,a)      (p)->lpVtbl->OpenAppStatusKey(p,a)
#else
/*** IUnknown methods ***/
#define IDirectInputJoyConfig8_QueryInterface(p,a,b)      (p)->QueryInterface(a,b)
#define IDirectInputJoyConfig8_AddRef(p)                  (p)->AddRef()
#define IDirectInputJoyConfig8_Release(p)                 (p)->Release()
/*** IDirectInputJoyConfig8 methods ***/
#define IDirectInputJoyConfig8_Acquire(p)                 (p)->Acquire()
#define IDirectInputJoyConfig8_Unacquire(p)               (p)->Unacquire()
#define IDirectInputJoyConfig8_SetCooperativeLevel(p,a,b) (p)->SetCooperativeLevel(a,b)
#define IDirectInputJoyConfig8_SendNotify(p)              (p)->SendNotify()
#define IDirectInputJoyConfig8_EnumTypes(p,a,b)           (p)->EnumTypes(a,b)
#define IDirectInputJoyConfig8_GetTypeInfo(p,a,b,c)       (p)->GetTypeInfo(a,b,c)
#define IDirectInputJoyConfig8_SetTypeInfo(p,a,b,c,d)     (p)->SetTypeInfo(a,b,c,d)
#define IDirectInputJoyConfig8_DeleteType(p,a)            (p)->DeleteType(a)
#define IDirectInputJoyConfig8_GetConfig(p,a,b,c)         (p)->GetConfig(a,b,c)
#define IDirectInputJoyConfig8_SetConfig(p,a,b,c)         (p)->SetConfig(a,b,c)
#define IDirectInputJoyConfig8_DeleteConfig(p,a)          (p)->DeleteConfig(a)
#define IDirectInputJoyConfig8_GetUserValues(p,a,b)       (p)->GetUserValues(a,b)
#define IDirectInputJoyConfig8_SetUserValues(p,a,b)       (p)->SetUserValues(a,b)
#define IDirectInputJoyConfig8_AddNewHardware(p,a,b)      (p)->AddNewHardware(a,b)
#define IDirectInputJoyConfig8_OpenTypeKey(p,a,b,c)       (p)->OpenTypeKey(a,b,c)
#define IDirectInputJoyConfig8_OpenAppStatusKey(p,a)      (p)->OpenAppStatusKey(a)
#endif

#endif /* __DINPUTD_INCLUDED__ */
