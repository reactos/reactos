#ifndef INTERFACES_H__
#define INTERFACES_H__

DEFINE_GUID(IID_IIrpTarget,        0xB4C90A60, 0x5791, 0x11D0, 0x86, 0xF9, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);
DEFINE_GUID(IID_ISubdevice,        0xB4C90A61, 0x5791, 0x11D0, 0x86, 0xF9, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);
DEFINE_GUID(IID_IIrpTargetFactory, 0xB4C90A62, 0x5791, 0x11D0, 0x86, 0xF9, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);

/*****************************************************************************
 * ISubdevice
 *****************************************************************************
 */
struct SUBDEVICE_DESCRIPTOR;
struct IIrpTarget;
struct IIrpTargetFactory;

#undef INTERFACE
#define INTERFACE ISubdevice

DECLARE_INTERFACE_(ISubdevice, IUnknown)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(NTSTATUS, NewIrpTarget)(THIS_
        OUT struct IIrpTarget **OutTarget,
        IN WCHAR * Name,
        IN PUNKNOWN Unknown,
        IN POOL_TYPE PoolType,
        IN PDEVICE_OBJECT * DeviceObject,
        IN PIRP Irp, 
        IN KSOBJECT_CREATE *CreateObject) PURE;

    STDMETHOD_(NTSTATUS, ReleaseChildren)(THIS) PURE;

    STDMETHOD_(NTSTATUS, GetDescriptor)(THIS_
        IN struct SUBDEVICE_DESCRIPTOR **) PURE;

    STDMETHOD_(NTSTATUS, DataRangeIntersection)(THIS_
        IN  ULONG PinId,
        IN  PKSDATARANGE DataRange,
        IN  PKSDATARANGE MatchingDataRange,
        IN  ULONG OutputBufferLength,
        OUT PVOID ResultantFormat OPTIONAL,
        OUT PULONG ResultantFormatLength) PURE;

    STDMETHOD_(NTSTATUS, PowerChangeNotify)(THIS_
        IN POWER_STATE PowerState) PURE;

    STDMETHOD_(NTSTATUS, PinCount)(THIS_
        IN ULONG  PinId,
        IN OUT PULONG  FilterNecessary,
        IN OUT PULONG  FilterCurrent,
        IN OUT PULONG  FilterPossible,
        IN OUT PULONG  GlobalCurrent,
        IN OUT PULONG  GlobalPossible)PURE;


};
#undef INTERFACE


#endif
