#ifndef KSIFACE_H__
#define KSIFACE_H__

#include <ntddk.h>
#include <ks.h>

#if !defined(DEFINE_ABSTRACT_UNKNOWN)

#define DEFINE_ABSTRACT_UNKNOWN()                               \
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_                  \
        REFIID InterfaceId,                                     \
        PVOID* Interface)PURE;                                  \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;                        \
    STDMETHOD_(ULONG,Release)(THIS) PURE;
#endif

/*****************************************************************************
 * IKsFilterFactory
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsFilterFactory

DECLARE_INTERFACE_(IKsFilterFactory, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(KSFILTERFACTORY*,GetStruct)(THIS) PURE;

    STDMETHOD_(NTSTATUS,SetDeviceClassesState)(THIS_
        IN BOOLEAN Enable)PURE;
};


/*****************************************************************************
 * IKsPowerNotify
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsPowerNotify

DECLARE_INTERFACE_(IKsPowerNotify, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(VOID,Sleep)(THIS_
        IN DEVICE_POWER_STATE State) PURE;

    STDMETHOD_(VOID,Wake)(THIS) PURE;
};


/*****************************************************************************
 * IKsDevice
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsDevice

struct KSIOBJECTBAG;
struct KSPOWER_ENTRY;

DECLARE_INTERFACE_(IKsDevice, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(KSDEVICE*,GetStruct)(THIS) PURE;

    STDMETHOD_(NTSTATUS, InitializeObjectBag)(THIS_
        IN struct KSIOBJECTBAG *Bag,
        IN KMUTANT * Mutant) PURE;

    STDMETHOD_(NTSTATUS,AcquireDevice)(THIS) PURE;
    STDMETHOD_(NTSTATUS,ReleaseDevice)(THIS) PURE;

    STDMETHOD_(NTSTATUS, GetAdapterObject)(THIS_
        IN PADAPTER_OBJECT Object,
        IN PULONG Unknown1,
        IN PULONG Unknown2) PURE;

    STDMETHOD_(NTSTATUS, AddPowerEntry)(THIS_
        IN struct KSPOWER_ENTRY * Entry,
        IN IKsPowerNotify* Notify)PURE;

    STDMETHOD_(NTSTATUS, RemovePowerEntry)(THIS_
        IN struct KSPOWER_ENTRY * Entry)PURE;

    STDMETHOD_(NTSTATUS, PinStateChange)(THIS_
        IN KSPIN Pin,
        IN PIRP Irp,
        IN KSSTATE OldState,
        IN KSSTATE NewState)PURE;

    STDMETHOD_(NTSTATUS, ArbitrateAdapterChannel)(THIS_
        IN ULONG ControlCode,
        IN IO_ALLOCATION_ACTION Action,
        IN PVOID Context)PURE;

    STDMETHOD_(NTSTATUS, CheckIoCapability)(THIS_
        IN ULONG Unknown)PURE;
};

#endif
