/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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

#ifndef __DXCORE_INTERFACE__
#define __DXCORE_INTERFACE__

#include <stdarg.h>
#include <stdint.h>
#include <ole2.h>

#ifdef __cplusplus
#define DECLARE_ENUM_CLASS(x) enum class x : uint32_t
#define REFLUID const LUID &
#else
#define DECLARE_ENUM_CLASS(x) typedef enum x x; enum x
#define REFLUID const LUID *
#endif

DECLARE_ENUM_CLASS(DXCoreAdapterProperty)
{
    InstanceLuid = 0,
    DriverVersion = 1,
    DriverDescription = 2,
    HardwareID = 3,
    KmdModelVersion = 4,
    ComputePreemptionGranularity = 5,
    GraphicsPreemptionGranularity = 6,
    DedicatedAdapterMemory = 7,
    DedicatedSystemMemory = 8,
    SharedSystemMemory = 9,
    AcgCompatible = 10,
    IsHardware = 11,
    IsIntegrated = 12,
    IsDetachable = 13,
    HardwareIDParts = 14,
};

DECLARE_ENUM_CLASS(DXCoreAdapterState)
{
    IsDriverUpdateInProgress = 0,
    AdapterMemoryBudget = 1,
};

DECLARE_ENUM_CLASS(DXCoreSegmentGroup)
{
    Local = 0,
    NonLocal = 1,
};

DECLARE_ENUM_CLASS(DXCoreNotificationType)
{
    AdapterListStale = 0,
    AdapterNoLongerValid = 1,
    AdapterBudgetChange = 2,
    AdapterHardwareContentProtectionTeardown = 3,
};

DECLARE_ENUM_CLASS(DXCoreAdapterPreference)
{
    Hardware = 0,
    MinimumPower = 1,
    HighPerformance = 2,
};

typedef struct DXCoreHardwareID
{
    uint32_t vendorID;
    uint32_t deviceID;
    uint32_t subSysID;
    uint32_t revision;
} DXCoreHardwareID;

typedef struct DXCoreHardwareIDParts
{
    uint32_t vendorID;
    uint32_t deviceID;
    uint32_t subSystemID;
    uint32_t subVendorID;
    uint32_t revisionID;
} DXCoreHardwareIDParts;

typedef struct DXCoreAdapterMemoryBudgetNodeSegmentGroup
{
    uint32_t nodeIndex;
    DXCoreSegmentGroup segmentGroup;
} DXCoreAdapterMemoryBudgetNodeSegmentGroup;

typedef struct DXCoreAdapterMemoryBudget
{
    uint64_t budget;
    uint64_t currentUsage;
    uint64_t availableForReservation;
    uint64_t currentReservation;
} DXCoreAdapterMemoryBudget;

typedef void (STDMETHODCALLTYPE *PFN_DXCORE_NOTIFICATION_CALLBACK)(DXCoreNotificationType type, IUnknown *object, void *context);

DEFINE_GUID(IID_IDXCoreAdapterFactory, 0x78ee5945, 0xc36e, 0x4b13, 0xa6, 0x69, 0x00, 0x5d, 0xd1, 0x1c, 0x0f, 0x06);
DEFINE_GUID(IID_IDXCoreAdapterList, 0x526c7776, 0x40e9, 0x459b, 0xb7, 0x11, 0xf3, 0x2a, 0xd7, 0x6d, 0xfc, 0x28);
DEFINE_GUID(IID_IDXCoreAdapter, 0xf0db4c7f, 0xfe5a, 0x42a2, 0xbd, 0x62, 0xf2, 0xa6, 0xcf, 0x6f, 0xc8, 0x3e);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D11_GRAPHICS, 0x8c47866b, 0x7583, 0x450d, 0xf0, 0xf0, 0x6b, 0xad, 0xa8, 0x95, 0xaf, 0x4b);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, 0x0c9ece4d, 0x2f6e, 0x4f01, 0x8c, 0x96, 0xe8, 0x9e, 0x33, 0x1b, 0x47, 0xb1);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE, 0x248e2800, 0xa793, 0x4724, 0xab, 0xaa, 0x23, 0xa6, 0xde, 0x1b, 0xe0, 0x90);

#undef INTERFACE
#define INTERFACE IDXCoreAdapter
DECLARE_INTERFACE_IID_(IDXCoreAdapter, IUnknown, "f0db4c7f-fe5a-42a2-bd62-f2a6cf6fc83e")
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;
    /* IDXCoreAdapter methods */
    STDMETHOD_(BOOL, IsValid) (THIS) PURE;
    STDMETHOD_(BOOL, IsAttributeSupported) (THIS_ REFGUID attribute) PURE;
    STDMETHOD_(BOOL, IsPropertySupported) (THIS_ DXCoreAdapterProperty property) PURE;
    STDMETHOD(GetProperty) (THIS_ DXCoreAdapterProperty property, size_t buffer_size, void *buffer) PURE;
    STDMETHOD(GetPropertySize) (THIS_ DXCoreAdapterProperty property, size_t *buffer_size) PURE;
    STDMETHOD_(BOOL, IsQueryStateSupported) (THIS_ DXCoreAdapterState property) PURE;
    STDMETHOD(QueryState) (THIS_ DXCoreAdapterState state, size_t state_details_size, const void *state_details, size_t buffer_size, void *buffer) PURE;
    STDMETHOD_(BOOL, IsSetStateSupported) (THIS_ DXCoreAdapterState property) PURE;
    STDMETHOD(SetState) (THIS_ DXCoreAdapterState state, size_t state_details_size, const void *state_details, size_t buffer_size, const void *buffer) PURE;
    STDMETHOD(GetFactory) (THIS_ REFIID riid, void **ppv) PURE;

    #ifdef __cplusplus
    template <class T>
    HRESULT GetProperty(DXCoreAdapterProperty property, T *buffer)
    {
        return GetProperty(property, sizeof(T), (void *)buffer);
    }

    template <class T1, class T2>
    HRESULT QueryState(DXCoreAdapterState state, const T1 *state_details, T2 *buffer)
    {
        return QueryState(state, sizeof(T1), (const void *)state_details, sizeof(T2), (void *)buffer);
    }

    template <class T>
    HRESULT QueryState(DXCoreAdapterState state, T *buffer)
    {
        return QueryState(state, 0, NULL, sizeof(T), (void *)buffer);
    }

    template <class T1, class T2>
    HRESULT SetState(DXCoreAdapterState state, const T1 *state_details, const T2 *buffer)
    {
        return SetState(state, sizeof(T1), (const void *)state_details, sizeof(T2), (const void *)buffer);
    }

    template <class T>
    HRESULT GetFactory(T **ppv)
    {
        return GetFactory(IID_PPV_ARGS(ppv));
    }
    #endif
};

#undef INTERFACE
#define INTERFACE IDXCoreAdapterList
DECLARE_INTERFACE_IID_(IDXCoreAdapterList, IUnknown, "526c7776-40e9-459b-b711-f32ad76dfc28")
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;
    /* IDXCoreAdapterList methods */
    STDMETHOD(GetAdapter) (THIS_ uint32_t index, REFIID riid, void **ppv) PURE;
    STDMETHOD_(uint32_t, GetAdapterCount) (THIS) PURE;
    STDMETHOD_(BOOL, IsStale) (THIS) PURE;
    STDMETHOD(GetFactory) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD(Sort) (THIS_ uint32_t num_preferences, const DXCoreAdapterPreference *preferences) PURE;
    STDMETHOD_(BOOL, IsAdapterPreferenceSupported) (THIS_ DXCoreAdapterPreference preference) PURE;

    #ifdef __cplusplus
    template<class T>
    HRESULT STDMETHODCALLTYPE GetAdapter(uint32_t index, T **ppv)
    {
        return GetAdapter(index, IID_PPV_ARGS(ppv));
    }

    template <class T>
    HRESULT GetFactory(T **ppv)
    {
        return GetFactory(IID_PPV_ARGS(ppv));
    }
    #endif
};

#undef INTERFACE
#define INTERFACE IDXCoreAdapterFactory
DECLARE_INTERFACE_IID_(IDXCoreAdapterFactory, IUnknown, "78ee5945-c36e-4b13-a669-005dd11c0f06")
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;
    /* IDXCoreAdapterFactory methods */
    STDMETHOD(CreateAdapterList) (THIS_ uint32_t num_attributes, const GUID *filter_attributes, REFIID riid, void **ppv) PURE;
    STDMETHOD(GetAdapterByLuid) (THIS_ REFLUID adapter_luid, REFIID riid, void **ppv) PURE;
    STDMETHOD_(BOOL, IsNotificationTypeSupported) (THIS_ DXCoreNotificationType type) PURE;
    STDMETHOD(RegisterEventNotification) (THIS_ IUnknown *dxcore_object, DXCoreNotificationType type, PFN_DXCORE_NOTIFICATION_CALLBACK callback,
                                          void *callback_context, uint32_t *event_cookie) PURE;
    STDMETHOD(UnregisterEventNotification) (THIS_ uint32_t event_cookie) PURE;

    #ifdef __cplusplus
    template<class T>
    HRESULT STDMETHODCALLTYPE CreateAdapterList(uint32_t num_attributes, const GUID *filter_attributes, T **ppv)
    {
        return CreateAdapterList(num_attributes, filter_attributes, IID_PPV_ARGS(ppv));
    }

    template<class T>
    HRESULT STDMETHODCALLTYPE GetAdapterByLuid(const LUID &adapter_luid, T **ppv)
    {
        return GetAdapterByLuid(adapter_luid, IID_PPV_ARGS(ppv));
    }
    #endif
};

#endif /* __DXCORE_INTERFACE__ */
