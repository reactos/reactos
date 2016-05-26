/*
 * Copyright (C) 2010 Vijay Kiran Kamuju
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

#ifndef __D3DRMWIN_H__
#define __D3DRMWIN_H__

#include <d3drm.h>
#include <ddraw.h>
#include <d3d.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Direct3DRMWinDevice interface GUID
 */

DEFINE_GUID(IID_IDirect3DRMWinDevice,       0xc5016cc0, 0xd273, 0x11ce, 0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);

typedef struct IDirect3DRMWinDevice       *LPDIRECT3DRMWINDEVICE, **LPLPDIRECT3DRMWINDEVICE;

/*****************************************************************************
 * IDirect3DRMWinDevice interface
 */
#define INTERFACE IDirect3DRMWinDevice
DECLARE_INTERFACE_(IDirect3DRMWinDevice,IDirect3DRMObject)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ IUnknown *outer, REFIID iid, void **out) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK cb, void *ctx) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK cb, void *ctx) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ const char *name) PURE;
    STDMETHOD(GetName)(THIS_ DWORD *size, char *name) PURE;
    STDMETHOD(GetClassName)(THIS_ DWORD *size, char *name) PURE;
    /*** IDirect3DRMWinDevice methods ***/
    STDMETHOD(HandlePaint)(THIS_ HDC) PURE;
    STDMETHOD(HandleActivate)(THIS_ WORD) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMWinDevice_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMWinDevice_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirect3DRMWinDevice_Release(p)                   (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMWinDevice_Clone(p,a,b,c)               (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMWinDevice_AddDestroyCallback(p,a,b)    (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMWinDevice_DeleteDestroyCallback(p,a,b) (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMWinDevice_SetAppData(p,a)              (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMWinDevice_GetAppData(p)                (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMWinDevice_SetName(p,a)                 (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMWinDevice_GetName(p,a,b)               (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMWinDevice_GetClassName(p,a,b)          (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMWinDevice methods ***/
#define IDirect3DRMWinDevice_HandlePaint(p,a)             (p)->lpVtbl->HandlePaint(p,a)
#define IDirect3DRMWinDevice_HandleActivate(p,a)          (p)->lpVtbl->HandleActivate(p,a)
#else
/*** IUnknown methods ***/
#define IDirect3DRMWinDevice_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirect3DRMWinDevice_AddRef(p)                    (p)->AddRef()
#define IDirect3DRMwinDevice_Release(p)                   (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMWinDevice_Clone(p,a,b,c)               (p)->Clone(a,b,c)
#define IDirect3DRMWinDevice_AddDestroyCallback(p,a,b)    (p)->AddDestroyCallback(a,b)
#define IDirect3DRMWinDevice_DeleteDestroyCallback(p,a,b) (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMWinDevice_SetAppData(p,a)              (p)->SetAppData(a)
#define IDirect3DRMWinDevice_GetAppData(p)                (p)->GetAppData()
#define IDirect3DRMWinDevice_SetName(p,a)                 (p)->SetName(a)
#define IDirect3DRMWinDevice_GetName(p,a,b)               (p)->GetName(a,b)
#define IDirect3DRMWinDevice_GetClassName(p,a,b)          (p)->GetClassName(a,b)
/*** IDirect3DRMWinDevice methods ***/
#define IDirect3DRMWinDevice_HandlePaint(p,a)             (p)->HandlePaint(a)
#define IDirect3DRMWinDevice_HandleActivate(p,a)          (p)->HandleActivate(a)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __D3DRMWIN_H__ */
