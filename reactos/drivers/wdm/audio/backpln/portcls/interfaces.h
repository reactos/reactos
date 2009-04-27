#ifndef INTERFACES_H__
#define INTERFACES_H__

DEFINE_GUID(IID_IIrpTarget,        0xB4C90A60, 0x5791, 0x11D0, 0xF9, 0x86, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);
DEFINE_GUID(IID_ISubdevice,        0xB4C90A61, 0x5791, 0x11D0, 0xF9, 0x86, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);
DEFINE_GUID(IID_IIrpTargetFactory, 0xB4C90A62, 0x5791, 0x11D0, 0xF9, 0x86, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);


/*****************************************************************************
 * IIrpTarget
 *****************************************************************************
 */

#define DEFINE_ABSTRACT_IRPTARGET()                        \
    STDMETHOD_(NTSTATUS, NewIrpTarget)(THIS_               \
        OUT struct IIrpTarget **OutTarget,                 \
        IN WCHAR * Name,                                   \
        IN PUNKNOWN Unknown,                               \
        IN POOL_TYPE PoolType,                             \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp,                                       \
        IN KSOBJECT_CREATE *CreateObject) PURE;            \
                                                           \
    STDMETHOD_(NTSTATUS, DeviceIoControl)(THIS_            \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp)PURE;                                  \
                                                           \
    STDMETHOD_(NTSTATUS, Read)(THIS_                       \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp)PURE;                                  \
                                                           \
    STDMETHOD_(NTSTATUS, Write)(THIS_                      \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp)PURE;                                  \
                                                           \
    STDMETHOD_(NTSTATUS, Flush)(THIS_                      \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp)PURE;                                  \
                                                           \
    STDMETHOD_(NTSTATUS, Close)(THIS_                      \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp)PURE;                                  \
                                                           \
    STDMETHOD_(NTSTATUS, QuerySecurity)(THIS_              \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp)PURE;                                  \
                                                           \
    STDMETHOD_(NTSTATUS, SetSecurity)(THIS_                \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp)PURE;                                  \
                                                           \
    STDMETHOD_(BOOLEAN, FastDeviceIoControl)(THIS_        \
        IN PFILE_OBJECT FileObject,                        \
        IN BOOLEAN Wait,                                   \
        IN PVOID InputBuffer,                              \
        IN ULONG InputBufferLength,                        \
        OUT PVOID OutputBuffer,                            \
        IN ULONG OutputBufferLength,                       \
        IN ULONG IoControlCode,                            \
        OUT PIO_STATUS_BLOCK StatusBlock,                  \
        IN PDEVICE_OBJECT DeviceObject)PURE;               \
                                                           \
    STDMETHOD_(BOOLEAN, FastRead)(THIS_                    \
        IN PFILE_OBJECT FileObject,                        \
        IN PLARGE_INTEGER FileOffset,                      \
        IN ULONG Length,                                   \
        IN BOOLEAN Wait,                                   \
        IN ULONG LockKey,                                  \
        IN PVOID Buffer,                                   \
        OUT PIO_STATUS_BLOCK StatusBlock,                  \
        IN PDEVICE_OBJECT DeviceObject)PURE;               \
                                                           \
    STDMETHOD_(BOOLEAN, FastWrite)(THIS_                   \
        IN PFILE_OBJECT FileObject,                        \
        IN PLARGE_INTEGER FileOffset,                      \
        IN ULONG Length,                                   \
        IN BOOLEAN Wait,                                   \
        IN ULONG LockKey,                                  \
        IN PVOID Buffer,                                   \
        OUT PIO_STATUS_BLOCK StatusBlock,                  \
        IN PDEVICE_OBJECT DeviceObject)PURE;

#undef INTERFACE
#define INTERFACE IIrpTarget

DECLARE_INTERFACE_(IIrpTarget, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()
};

/*****************************************************************************
 * ISubdevice
 *****************************************************************************
 */

struct IIrpTargetFactory;

typedef struct
{
    ULONG MaxGlobalInstanceCount;
    ULONG MaxFilterInstanceCount;
    ULONG MinFilterInstanceCount;
    ULONG CurrentPinInstanceCount;

}PIN_INSTANCE_INFO, *PPIN_INSTANCE_INFO;


typedef struct
{
    ULONG PinDescriptorCount;
    ULONG PinDescriptorSize;
    KSPIN_DESCRIPTOR * KsPinDescriptor;
    PIN_INSTANCE_INFO * Instances;
}KSPIN_FACTORY;

typedef struct
{
    ULONG MaxKsPropertySetCount;
    ULONG FreeKsPropertySetOffset;
    PKSPROPERTY_SET Properties;
}KSPROPERTY_SET_LIST;

typedef struct
{
    ULONG InterfaceCount;
    GUID *Interfaces;
    KSPIN_FACTORY Factory;
    KSPROPERTY_SET_LIST FilterPropertySet;

    PPCFILTER_DESCRIPTOR DeviceDescriptor;
}SUBDEVICE_DESCRIPTOR, *PSUBDEVICE_DESCRIPTOR;

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
        OUT IIrpTarget **OutTarget,
        IN WCHAR * Name,
        IN PUNKNOWN Unknown,
        IN POOL_TYPE PoolType,
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp, 
        IN KSOBJECT_CREATE *CreateObject) PURE;

    STDMETHOD_(NTSTATUS, ReleaseChildren)(THIS) PURE;

    STDMETHOD_(NTSTATUS, GetDescriptor)(THIS_
        IN SUBDEVICE_DESCRIPTOR **) PURE;

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

/*****************************************************************************
 * IIrpQueue
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IIrpQueue

DECLARE_INTERFACE_(IIrpQueue, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN KSPIN_CONNECT *ConnectDetails,
        IN PKSDATAFORMAT DataFormat,
        IN PDEVICE_OBJECT DeviceObject,
        IN ULONG FrameSize);

    STDMETHOD_(NTSTATUS, AddMapping)(THIS_
        IN PUCHAR Buffer,
        IN ULONG BufferSize,
        IN PIRP Irp);

    STDMETHOD_(NTSTATUS, GetMapping)(THIS_
        OUT PUCHAR * Buffer,
        OUT PULONG BufferSize);

    STDMETHOD_(VOID, UpdateMapping)(THIS_
        IN ULONG BytesWritten);

    STDMETHOD_(ULONG, NumMappings)(THIS);

    STDMETHOD_(ULONG, MinMappings)(THIS);

    STDMETHOD_(BOOL, MinimumDataAvailable)(THIS);

    STDMETHOD_(BOOL, CancelBuffers)(THIS);

    STDMETHOD_(VOID, UpdateFormat)(THIS_
        IN PKSDATAFORMAT DataFormat);

    STDMETHOD_(NTSTATUS, GetMappingWithTag)(THIS_
        IN PVOID Tag,
        OUT PPHYSICAL_ADDRESS  PhysicalAddress,
        OUT PVOID  *VirtualAddress,
        OUT PULONG  ByteCount,
        OUT PULONG  Flags);

    STDMETHOD_(VOID, ReleaseMappingWithTag)(THIS_
        IN PVOID Tag);

    STDMETHOD_(BOOL, HasLastMappingFailed)(THIS);
};


/*****************************************************************************
 * IKsWorkSink
 *****************************************************************************
 */
#undef INTERFACE
#define INTERFACE IKsWorkSink

DECLARE_INTERFACE_(IKsWorkSink, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, Work)(THIS);
};

/*****************************************************************************
 * IIrpStreamNotify
 *****************************************************************************
 */
#undef INTERFACE
#define INTERFACE IIrpStreamNotify

struct IRPSTREAMPOSITION;

DECLARE_INTERFACE_(IIrpStreamNotify, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, IrpSubmitted)(THIS_
        IN PIRP Irp,
        IN BOOLEAN WAIT)PURE;

    STDMETHOD_(NTSTATUS, GetPosition)(THIS_
        OUT struct IRPSTREAMPOSITION * Position)PURE;
};

/*****************************************************************************
 * IKsShellTransport
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsShellTransport

#define DEFINE_ABSTRACT_IKSSHELLTRANSPORT()                     \
    STDMETHOD_(NTSTATUS, TransferKsIrp)(THIS_                   \
        IN PIRP Irp,                                            \
        OUT IKsShellTransport ** Transport) PURE;               \
                                                                \
    STDMETHOD_(NTSTATUS, Connect)(THIS_                         \
        IN IKsShellTransport * StartTransport,                  \
        OUT IKsShellTransport ** EndTransport,                  \
        IN KSPIN_DATAFLOW DataFlow)PURE;                        \
                                                                \
    STDMETHOD_(NTSTATUS, SetDeviceState)(THIS_                  \
        IN KSSTATE State1,                                      \
        IN KSSTATE State2,                                      \
        OUT IKsShellTransport ** EndTransport)PURE;             \
                                                                \
    STDMETHOD_(NTSTATUS, SetResetState)(THIS_                   \
        IN KSRESET State1,                                      \
        OUT IKsShellTransport ** EndTransport)PURE;


DECLARE_INTERFACE_(IKsShellTransport, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IKSSHELLTRANSPORT()
};

/*****************************************************************************
 * IIrpStream
 *****************************************************************************
 */
struct IRPSTREAM_POSITION;
struct IRPSTREAMPACKETINFO;

#define DEFINE_ABSTRACT_IRPSTREAM()                             \
    STDMETHOD_(NTSTATUS, TransferKsIrp)(THIS_                   \
        IN PIRP Irp,                                            \
        OUT IKsShellTransport ** Transport) PURE;               \
                                                                \
    STDMETHOD_(NTSTATUS, Connect)(THIS_                         \
        IN IKsShellTransport * StartTransport,                  \
        OUT IKsShellTransport ** EndTransport,                  \
        IN KSPIN_DATAFLOW DataFlow)PURE;                        \
                                                                \
    STDMETHOD_(NTSTATUS, SetDeviceState)(THIS_                  \
        IN KSSTATE State1,                                      \
        IN KSSTATE State2,                                      \
        OUT IKsShellTransport ** EndTransport)PURE;             \
                                                                \
    STDMETHOD_(NTSTATUS, SetResetState)(THIS_                   \
        IN KSRESET State1,                                      \
        OUT IKsShellTransport ** EndTransport)PURE;             \
                                                                \
    STDMETHOD_(NTSTATUS, GetPosition)(THIS_                     \
        IN OUT struct IRPSTREAM_POSITION * Position) PURE;      \
                                                                \
    STDMETHOD_(NTSTATUS, Init)(THIS_                            \
        IN BOOLEAN Wait,                                        \
        KSPIN_CONNECT *ConnectDetails,                          \
        PDEVICE_OBJECT DeviceObject,                            \
        PDMA_ADAPTER DmaAdapter) PURE;                          \
                                                                \
    STDMETHOD_(NTSTATUS, CancelAllIrps)(THIS_                   \
        ULONG Wait)PURE;                                        \
                                                                \
    STDMETHOD_(VOID, TerminatePacket)(THIS);                    \
                                                                \
    STDMETHOD_(NTSTATUS, ChangeOptionsFlag)(THIS_               \
       ULONG Unknown1,                                          \
       ULONG Unknown2,                                          \
       ULONG Unknown3,                                          \
       ULONG Unknown4)PURE;                                     \
                                                                \
    STDMETHOD_(NTSTATUS, GetPacketInfo)(THIS_                   \
       struct IRPSTREAMPACKETINFO * Info1,                      \
       struct IRPSTREAMPACKETINFO * Info2)PURE;                 \
                                                                \
    STDMETHOD_(NTSTATUS, SetPacketOffsets)(THIS_                \
       ULONG Unknown1,                                          \
       ULONG Unknown2)PURE;                                     \
                                                                \
    STDMETHOD_(NTSTATUS, RegisterNotifySink)(THIS_              \
       IN IIrpStreamNotify * NotifyStream)PURE;



#undef INTERFACE
#define INTERFACE IIrpStream

DECLARE_INTERFACE_(IIrpStream, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPSTREAM()
};


/*****************************************************************************
 * IIrpStreamPhysical
 *****************************************************************************
 */
#undef INTERFACE
#define INTERFACE IIrpStreamPhysical

DECLARE_INTERFACE_(IIrpStreamPhysical, IIrpStream)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPSTREAM()

    STDMETHOD_(NTSTATUS, GetMapping)(THIS_
       IN PVOID Tag,
       OUT PPHYSICAL_ADDRESS PhysicalAddress,
       OUT PVOID * VirtualAddress,
       OUT PULONG ByteCount,
       OUT PULONG Flags)PURE;

};

/*****************************************************************************
 * IIrpStreamVirtual
 *****************************************************************************
 */
#undef INTERFACE
#define INTERFACE IIrpStreamVirtual

DECLARE_INTERFACE_(IIrpStreamVirtual, IIrpStream)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPSTREAM()

    STDMETHOD_(NTSTATUS, GetLockedRegion)(THIS_
       OUT PULONG OutSize,
       OUT PVOID * OutBuffer)PURE;

    STDMETHOD_(NTSTATUS, Copy)(THIS_
       IN BOOLEAN Wait,
       OUT ULONG Size,
       IN PULONG Buffer,
       OUT PVOID Result)PURE;

    STDMETHOD_(NTSTATUS, Complete)(THIS_
       IN ULONG Unknown1,
       IN PULONG Data)PURE;

    STDMETHOD_(ULONG, GetIrpStreamPositionLock)(THIS);
};

/*****************************************************************************
 * IPortFilterWavePci
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortFilterWavePci

DECLARE_INTERFACE_(IPortFilterWavePci, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTWAVEPCI Port)PURE;
};

typedef IPortFilterWavePci *PPORTFILTERWAVEPCI;


/*****************************************************************************
 * IPortPinWavePci
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortPinWavePci

DECLARE_INTERFACE_(IPortPinWavePci, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTWAVEPCI Port,
        IN PPORTFILTERWAVEPCI Filter,
        IN KSPIN_CONNECT * ConnectDetails,
        IN KSPIN_DESCRIPTOR * PinDescriptor,
        IN PDEVICE_OBJECT DeviceObject) PURE;

    STDMETHOD_(PVOID, GetIrpStream)(THIS);
    STDMETHOD_(PMINIPORT, GetMiniport)(THIS);
};

typedef IPortPinWavePci *PPORTPINWAVEPCI;

/*****************************************************************************
 * IPortFilterWaveCyclic
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortFilterWaveCyclic

DECLARE_INTERFACE_(IPortFilterWaveCyclic, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTWAVECYCLIC Port)PURE;
};

typedef IPortFilterWaveCyclic *PPORTFILTERWAVECYCLIC;

/*****************************************************************************
 * IPortPinWaveCyclic
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortPinWaveCyclic

DECLARE_INTERFACE_(IPortPinWaveCyclic, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTWAVECYCLIC Port,
        IN PPORTFILTERWAVECYCLIC Filter,
        IN KSPIN_CONNECT * ConnectDetails,
        IN KSPIN_DESCRIPTOR * PinDescriptor) PURE;

    STDMETHOD_(ULONG, GetCompletedPosition)(THIS);
    STDMETHOD_(ULONG, GetCycleCount)(THIS);
    STDMETHOD_(ULONG, GetDeviceBufferSize)(THIS);
    STDMETHOD_(PVOID, GetIrpStream)(THIS);
    STDMETHOD_(PMINIPORT, GetMiniport)(THIS);
};

/*****************************************************************************
 * IDmaChannelInit
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IDmaChannelInit

DECLARE_INTERFACE_(IDmaChannelInit, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS,AllocateBuffer)(THIS_
        IN  ULONG BufferSize,
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL);

    STDMETHOD_(VOID, FreeBuffer)(THIS);
    STDMETHOD_(ULONG, TransferCount)(THIS);
    STDMETHOD_(ULONG, MaximumBufferSize)(THIS);
    STDMETHOD_(ULONG, AllocatedBufferSize)(THIS);
    STDMETHOD_(ULONG, BufferSize)(THIS);

    STDMETHOD_(VOID, SetBufferSize)(THIS_ 
        IN  ULONG BufferSize);

    STDMETHOD_(PVOID, SystemAddress)(THIS);
    STDMETHOD_(PHYSICAL_ADDRESS, PhysicalAddress)(THIS_
        IN PPHYSICAL_ADDRESS Address);

    STDMETHOD_(PADAPTER_OBJECT, GetAdapterObject)(THIS);

    STDMETHOD_(VOID, CopyTo)(THIS_
        IN  PVOID Destination,
        IN  PVOID Source,
        IN  ULONG ByteCount);

    STDMETHOD_(VOID, CopyFrom)(THIS_
        IN  PVOID Destination,
        IN  PVOID Source,
        IN  ULONG ByteCount);

    STDMETHOD_(NTSTATUS, Start)( THIS_ 
        IN  ULONG MapSize,
        IN  BOOLEAN WriteToDevice) PURE;

    STDMETHOD_(NTSTATUS, Stop)( THIS ) PURE;
    STDMETHOD_(ULONG, ReadCounter)( THIS ) PURE;

    STDMETHOD_(NTSTATUS, WaitForTC)( THIS_
        ULONG Timeout) PURE;

    STDMETHOD_(NTSTATUS, Init)( THIS_
        IN PDEVICE_DESCRIPTION DeviceDescription,
        IN PDEVICE_OBJECT DeviceObject) PURE;
};

#endif
