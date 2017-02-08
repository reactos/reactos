/*
 * Definition of IKsControl
 *
 * Copyright (C) 2012 Christian Costa
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

#ifndef _DMKSCTRL_
#define _DMKSCTRL_

#include <pshpack8.h>

#include <objbase.h>

#ifndef _KS_
#define _KS_

typedef struct {
    union {
        struct {
            GUID    Set;
            ULONG   Id;
            ULONG   Flags;
        } DUMMYSTRUCTNAME;
        LONGLONG    Alignment;
    } DUMMYUNIONNAME;
} KSIDENTIFIER, *PKSIDENTIFIER;

typedef KSIDENTIFIER KSPROPERTY, *PKSPROPERTY, KSMETHOD, *PKSMETHOD, KSEVENT, *PKSEVENT;

#define KSMETHOD_TYPE_NONE                  0x00000000
#define KSMETHOD_TYPE_READ                  0x00000001
#define KSMETHOD_TYPE_WRITE                 0x00000002
#define KSMETHOD_TYPE_MODIFY                0x00000003
#define KSMETHOD_TYPE_SOURCE                0x00000004

#define KSMETHOD_TYPE_SEND                  0x00000001
#define KSMETHOD_TYPE_SETSUPPORT            0x00000100
#define KSMETHOD_TYPE_BASICSUPPORT          0x00000200

#define KSPROPERTY_TYPE_GET                 0x00000001
#define KSPROPERTY_TYPE_SET                 0x00000002
#define KSPROPERTY_TYPE_SETSUPPORT          0x00000100
#define KSPROPERTY_TYPE_BASICSUPPORT        0x00000200
#define KSPROPERTY_TYPE_RELATIONS           0x00000400
#define KSPROPERTY_TYPE_SERIALIZESET        0x00000800
#define KSPROPERTY_TYPE_UNSERIALIZESET      0x00001000
#define KSPROPERTY_TYPE_SERIALIZERAW        0x00002000
#define KSPROPERTY_TYPE_UNSERIALIZERAW      0x00004000
#define KSPROPERTY_TYPE_SERIALIZESIZE       0x00008000
#define KSPROPERTY_TYPE_DEFAULTVALUES       0x00010000

#define KSPROPERTY_TYPE_TOPOLOGY            0x10000000

#define INTERFACE IKsControl
DECLARE_INTERFACE_(IKsControl,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IKsControl methods ***/
    STDMETHOD(KsProperty)(THIS_ PKSPROPERTY Property, ULONG PropertyLength, LPVOID PropertyData,
                          ULONG DataLength, ULONG* BytesReturned) PURE;
    STDMETHOD(KsMethod)(THIS_ PKSMETHOD Method, ULONG MethodLength, LPVOID MethodData,
                        ULONG DataLength, ULONG* BytesReturned) PURE;
    STDMETHOD(KsEvent)(THIS_ PKSEVENT Event, ULONG EventLength, LPVOID EventData,
                       ULONG DataLength, ULONG* BytesReturned) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IKsControl_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IKsControl_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IKsControl_Release(p)              (p)->lpVtbl->Release(p)
/*** IKsControl methods ***/
#define IKsControl_KsProperty(p,a,b,c,d,e) (p)->lpVtbl->KsProperty(p,a,b,c,d,e)
#define IKsControl_KsMethod(p,a,b,c,d,e)   (p)->lpVtbl->KsMethod(p,a,b,c,d,e)
#define IKsControl_KsEvent(p,a,b,c,d,e)    (p)->lpVtbl->KsEvent(p,a,b,c,d,e)
#endif

#endif /* _KS_ */

#include <poppack.h>


DEFINE_GUID(IID_IKsControl, 0x28f54685, 0x06fd, 0x11d2, 0xb2, 0x7a, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96);

#ifndef _KSMEDIA_

DEFINE_GUID(KSDATAFORMAT_SUBTYPE_MIDI, 0x1d262760, 0xe957, 0x11cf, 0xa5, 0xd6, 0x28, 0xdb, 0x04, 0xc1, 0x00, 0x00);
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_DIRECTMUSIC, 0x1a82f8bc, 0x3f8b, 0x11d2, 0xb7, 0x74, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);

#endif

#endif /* _DMKSCTRL_ */
