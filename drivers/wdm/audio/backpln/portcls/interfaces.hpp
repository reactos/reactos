#ifndef INTERFACES_H__
#define INTERFACES_H__

DEFINE_GUID(IID_IIrpTarget,        0xB4C90A60, 0x5791, 0x11D0, 0xF9, 0x86, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);
DEFINE_GUID(IID_ISubdevice,        0xB4C90A61, 0x5791, 0x11D0, 0xF9, 0x86, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);
DEFINE_GUID(IID_IIrpTargetFactory, 0xB4C90A62, 0x5791, 0x11D0, 0xF9, 0x86, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);


/*****************************************************************************
 * IIrpTarget
 *****************************************************************************
 */

#define IMP_IIrpTarget                                     \
    STDMETHODIMP_(NTSTATUS) NewIrpTarget(THIS_             \
        OUT struct IIrpTarget **OutTarget,                 \
        IN PCWSTR Name,                                    \
        IN PUNKNOWN Unknown,                               \
        IN POOL_TYPE PoolType,                             \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp,                                       \
        IN KSOBJECT_CREATE *CreateObject);                 \
                                                           \
    STDMETHODIMP_(NTSTATUS) DeviceIoControl(THIS_          \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp);                                      \
                                                           \
    STDMETHODIMP_(NTSTATUS) Read(THIS_                     \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp);                                      \
                                                           \
    STDMETHODIMP_(NTSTATUS) Write(THIS_                    \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp);                                      \
                                                           \
    STDMETHODIMP_(NTSTATUS) Flush(THIS_                    \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp);                                      \
                                                           \
    STDMETHODIMP_(NTSTATUS) Close(THIS_                    \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp);                                      \
                                                           \
    STDMETHODIMP_(NTSTATUS) QuerySecurity(THIS_            \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp);                                      \
                                                           \
    STDMETHODIMP_(NTSTATUS) SetSecurity(THIS_              \
        IN PDEVICE_OBJECT DeviceObject,                    \
        IN PIRP Irp);                                      \
                                                           \
    STDMETHODIMP_(BOOLEAN) FastDeviceIoControl(THIS_       \
        IN PFILE_OBJECT FileObject,                        \
        IN BOOLEAN Wait,                                   \
        IN PVOID InputBuffer,                              \
        IN ULONG InputBufferLength,                        \
        OUT PVOID OutputBuffer,                            \
        IN ULONG OutputBufferLength,                       \
        IN ULONG IoControlCode,                            \
        OUT PIO_STATUS_BLOCK StatusBlock,                  \
        IN PDEVICE_OBJECT DeviceObject);                   \
                                                           \
    STDMETHODIMP_(BOOLEAN) FastRead(THIS_                  \
        IN PFILE_OBJECT FileObject,                        \
        IN PLARGE_INTEGER FileOffset,                      \
        IN ULONG Length,                                   \
        IN BOOLEAN Wait,                                   \
        IN ULONG LockKey,                                  \
        IN PVOID Buffer,                                   \
        OUT PIO_STATUS_BLOCK StatusBlock,                  \
        IN PDEVICE_OBJECT DeviceObject);                   \
                                                           \
    STDMETHODIMP_(BOOLEAN) FastWrite(THIS_                 \
        IN PFILE_OBJECT FileObject,                        \
        IN PLARGE_INTEGER FileOffset,                      \
        IN ULONG Length,                                   \
        IN BOOLEAN Wait,                                   \
        IN ULONG LockKey,                                  \
        IN PVOID Buffer,                                   \
        OUT PIO_STATUS_BLOCK StatusBlock,                  \
        IN PDEVICE_OBJECT DeviceObject)

#define DEFINE_ABSTRACT_IRPTARGET()                        \
    STDMETHOD_(NTSTATUS, NewIrpTarget)(THIS_               \
        OUT struct IIrpTarget **OutTarget,                 \
        IN PCWSTR Name,                                    \
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
    STDMETHOD_(NTSTATUS, Close)(                           \
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
    STDMETHOD_(BOOLEAN, FastDeviceIoControl)(THIS_         \
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

typedef IIrpTarget *PIRPTARGET;

/*****************************************************************************
 * ISubdevice
 *****************************************************************************
 */

struct IIrpTargetFactory;

typedef struct
{
    LIST_ENTRY Entry;
    UNICODE_STRING SymbolicLink;
}SYMBOLICLINK_ENTRY, *PSYMBOLICLINK_ENTRY;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG FromPin;
    KSPIN_PHYSICALCONNECTION Connection;
}PHYSICAL_CONNECTION_ENTRY, *PPHYSICAL_CONNECTION_ENTRY;

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
    ULONG InterfaceCount;
    GUID *Interfaces;
    KSPIN_FACTORY Factory;
    ULONG FilterPropertySetCount;
    PKSPROPERTY_SET FilterPropertySet;

    ULONG EventSetCount;
    PKSEVENT_SET EventSet;
    PLIST_ENTRY EventList;
    PKSPIN_LOCK EventListLock;

    PPCFILTER_DESCRIPTOR DeviceDescriptor;
    KSTOPOLOGY*  Topology;
    LIST_ENTRY SymbolicLinkList;
    LIST_ENTRY PhysicalConnectionList;
    UNICODE_STRING RefString;
    PUNKNOWN UnknownMiniport;
    PUNKNOWN UnknownStream;
    PVOID PortPin;
}SUBDEVICE_DESCRIPTOR, *PSUBDEVICE_DESCRIPTOR;

#undef INTERFACE
#define INTERFACE ISubdevice

#define DEFINE_ABSTRACT_ISUBDEVICE()                          \
    STDMETHOD_(NTSTATUS, NewIrpTarget)(THIS_                  \
        OUT IIrpTarget **OutTarget,                           \
        IN PCWSTR Name,                                       \
        IN PUNKNOWN Unknown,                                  \
        IN POOL_TYPE PoolType,                                \
        IN PDEVICE_OBJECT DeviceObject,                       \
        IN PIRP Irp,                                          \
        IN KSOBJECT_CREATE *CreateObject) PURE;               \
                                                              \
    STDMETHOD_(NTSTATUS, ReleaseChildren)(THIS) PURE;         \
                                                              \
    STDMETHOD_(NTSTATUS, GetDescriptor)(THIS_                 \
        IN SUBDEVICE_DESCRIPTOR **) PURE;                     \
                                                              \
    STDMETHOD_(NTSTATUS, DataRangeIntersection)(THIS_         \
        IN  ULONG PinId,                                      \
        IN  PKSDATARANGE DataRange,                           \
        IN  PKSDATARANGE MatchingDataRange,                   \
        IN  ULONG OutputBufferLength,                         \
        OUT PVOID ResultantFormat OPTIONAL,                   \
        OUT PULONG ResultantFormatLength) PURE;               \
                                                              \
    STDMETHOD_(NTSTATUS, PowerChangeNotify)(THIS_             \
        IN POWER_STATE PowerState) PURE;                      \
                                                              \
    STDMETHOD_(NTSTATUS, PinCount)(THIS_                      \
        IN ULONG  PinId,                                      \
        IN OUT PULONG  FilterNecessary,                       \
        IN OUT PULONG  FilterCurrent,                         \
        IN OUT PULONG  FilterPossible,                        \
        IN OUT PULONG  GlobalCurrent,                         \
        IN OUT PULONG  GlobalPossible)PURE;



#define IMP_ISubdevice                                        \
    STDMETHODIMP_(NTSTATUS) NewIrpTarget(                     \
        OUT IIrpTarget **OutTarget,                           \
        IN PCWSTR Name,                                       \
        IN PUNKNOWN Unknown,                                  \
        IN POOL_TYPE PoolType,                                \
        IN PDEVICE_OBJECT DeviceObject,                       \
        IN PIRP Irp,                                          \
        IN KSOBJECT_CREATE *CreateObject);                    \
                                                              \
    STDMETHODIMP_(NTSTATUS) ReleaseChildren(THIS);            \
                                                              \
    STDMETHODIMP_(NTSTATUS) GetDescriptor(THIS_               \
        IN SUBDEVICE_DESCRIPTOR **);                          \
                                                              \
    STDMETHODIMP_(NTSTATUS) DataRangeIntersection(            \
        IN  ULONG PinId,                                      \
        IN  PKSDATARANGE DataRange,                           \
        IN  PKSDATARANGE MatchingDataRange,                   \
        IN  ULONG OutputBufferLength,                         \
        OUT PVOID ResultantFormat OPTIONAL,                   \
        OUT PULONG ResultantFormatLength);                    \
                                                              \
    STDMETHODIMP_(NTSTATUS) PowerChangeNotify(                \
        IN POWER_STATE PowerState);                           \
                                                              \
    STDMETHODIMP_(NTSTATUS) PinCount(                         \
        IN ULONG  PinId,                                      \
        IN OUT PULONG  FilterNecessary,                       \
        IN OUT PULONG  FilterCurrent,                         \
        IN OUT PULONG  FilterPossible,                        \
        IN OUT PULONG  GlobalCurrent,                         \
        IN OUT PULONG  GlobalPossible)


DECLARE_INTERFACE_(ISubdevice, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_ISUBDEVICE()
};

typedef ISubdevice *PSUBDEVICE;

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
        IN PKSPIN_CONNECT ConnectDetails,
        IN PKSPIN_DESCRIPTOR Descriptor,
        IN ULONG FrameSize,
        IN ULONG Alignment,
        IN ULONG TagSupportEnabled) PURE;

    STDMETHOD_(NTSTATUS, AddMapping)(THIS_
        IN PIRP Irp,
        OUT PULONG Data) PURE;

    STDMETHOD_(NTSTATUS, GetMapping)(THIS_
        OUT PUCHAR * Buffer,
        OUT PULONG BufferSize) PURE;

    STDMETHOD_(VOID, UpdateMapping)(THIS_
        IN ULONG BytesWritten) PURE;

    STDMETHOD_(ULONG, NumData)(THIS) PURE;

    STDMETHOD_(BOOL, CancelBuffers)(THIS) PURE;

    STDMETHOD_(NTSTATUS, GetMappingWithTag)(THIS_
        IN PVOID Tag,
        OUT PPHYSICAL_ADDRESS  PhysicalAddress,
        OUT PVOID  *VirtualAddress,
        OUT PULONG  ByteCount,
        OUT PULONG  Flags) PURE;

    STDMETHOD_(NTSTATUS, ReleaseMappingWithTag)(THIS_
        IN PVOID Tag) PURE;

    STDMETHOD_(ULONG, GetCurrentIrpOffset)(THIS) PURE;

    STDMETHOD_(BOOLEAN, GetAcquiredTagRange)(THIS_
        IN PVOID * FirstTag,
        IN PVOID * LastTag) PURE;

};


#define IMP_IIrpQueue                                  \
    STDMETHODIMP_(NTSTATUS) Init(THIS_                 \
        IN PKSPIN_CONNECT ConnectDetails,              \
        IN PKSPIN_DESCRIPTOR Descriptor,               \
        IN ULONG FrameSize,                            \
        IN ULONG Alignment,                            \
        IN ULONG TagSupportEnabled);                   \
                                                       \
    STDMETHODIMP_(NTSTATUS) AddMapping(THIS_           \
        IN PIRP Irp,                                   \
        OUT PULONG Data);                              \
                                                       \
    STDMETHODIMP_(NTSTATUS) GetMapping(THIS_           \
        OUT PUCHAR * Buffer,                           \
        OUT PULONG BufferSize);                        \
                                                       \
    STDMETHODIMP_(VOID) UpdateMapping(THIS_            \
        IN ULONG BytesWritten);                        \
                                                       \
    STDMETHODIMP_(ULONG) NumData(THIS);                \
                                                       \
    STDMETHODIMP_(BOOL) CancelBuffers(THIS);           \
                                                       \
    STDMETHODIMP_(NTSTATUS) GetMappingWithTag(THIS_    \
        IN PVOID Tag,                                  \
        OUT PPHYSICAL_ADDRESS  PhysicalAddress,        \
        OUT PVOID  *VirtualAddress,                    \
        OUT PULONG  ByteCount,                         \
        OUT PULONG  Flags);                            \
                                                       \
    STDMETHODIMP_(NTSTATUS) ReleaseMappingWithTag(     \
        IN PVOID Tag);                                 \
                                                       \
    STDMETHODIMP_(BOOLEAN) HasLastMappingFailed(THIS); \
    STDMETHODIMP_(ULONG) GetCurrentIrpOffset(THIS);    \
    STDMETHODIMP_(BOOLEAN) GetAcquiredTagRange(THIS_      \
        IN PVOID * FirstTag,                           \
        IN PVOID * LastTag);



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

struct IPortPinWavePci;

DECLARE_INTERFACE_(IPortFilterWavePci, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTWAVEPCI Port)PURE;

    STDMETHOD_(NTSTATUS, FreePin)(THIS_
        IN struct IPortPinWavePci* Pin)PURE;
};

typedef IPortFilterWavePci *PPORTFILTERWAVEPCI;

#define IMP_IPortFilterPci           \
    IMP_IIrpTarget;                         \
    STDMETHODIMP_(NTSTATUS) Init(THIS_      \
        IN PPORTWAVEPCI Port);              \
    STDMETHODIMP_(NTSTATUS) FreePin(THIS_   \
        IN struct IPortPinWavePci* Pin)


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

    STDMETHOD_(PVOID, GetIrpStream)(THIS) PURE;
    STDMETHOD_(PMINIPORT, GetMiniport)(THIS) PURE;
};

#define IMP_IPortPinWavePci                        \
    IMP_IIrpTarget;                                \
    STDMETHODIMP_(NTSTATUS) Init(THIS_             \
        IN PPORTWAVEPCI Port,                      \
        IN PPORTFILTERWAVEPCI Filter,              \
        IN KSPIN_CONNECT * ConnectDetails,         \
        IN KSPIN_DESCRIPTOR * PinDescriptor,       \
        IN PDEVICE_OBJECT DeviceObject);           \
                                                   \
    STDMETHODIMP_(PVOID) GetIrpStream();           \
    STDMETHODIMP_(PMINIPORT) GetMiniport(THIS)



typedef IPortPinWavePci *PPORTPINWAVEPCI;


#if (NTDDI_VERSION >= NTDDI_VISTA)

/*****************************************************************************
 * IPortFilterWaveRT
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortFilterWaveRT

#ifndef PPORTWAVERT
typedef IPortWaveRT *PPORTWAVERT;
#endif

DECLARE_INTERFACE_(IPortFilterWaveRT, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTWAVERT Port)PURE;
};

typedef IPortFilterWaveRT *PPORTFILTERWAVERT;

#define IMP_IPortFilterWaveRT               \
    IMP_IIrpTarget;                         \
    STDMETHODIMP_(NTSTATUS) Init(THIS_      \
        IN PPORTWAVERT Port)


/*****************************************************************************
 * IPortPinWaveRT
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortPinWaveRT

DECLARE_INTERFACE_(IPortPinWaveRT, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTWAVERT Port,
        IN PPORTFILTERWAVERT Filter,
        IN KSPIN_CONNECT * ConnectDetails,
        IN KSPIN_DESCRIPTOR * PinDescriptor,
        IN PDEVICE_OBJECT DeviceObject) PURE;
};

typedef IPortPinWaveRT *PPORTPINWAVERT;

#define IMP_IPortPinWaveRT                       \
    IMP_IIrpTarget;                              \
    STDMETHODIMP_(NTSTATUS) Init(THIS_           \
        IN PPORTWAVERT Port,                     \
        IN PPORTFILTERWAVERT Filter,             \
        IN KSPIN_CONNECT * ConnectDetails,       \
        IN KSPIN_DESCRIPTOR * PinDescriptor,     \
        IN PDEVICE_OBJECT DeviceObject)


#endif

/*****************************************************************************
 * IPortFilterWaveCyclic
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortFilterWaveCyclic

struct IPortPinWaveCyclic;

DECLARE_INTERFACE_(IPortFilterWaveCyclic, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTWAVECYCLIC Port)PURE;

    STDMETHOD_(NTSTATUS, FreePin)(THIS_
        IN struct IPortPinWaveCyclic* Pin)PURE;
};

typedef IPortFilterWaveCyclic *PPORTFILTERWAVECYCLIC;

#define IMP_IPortFilterWaveCyclic           \
    IMP_IIrpTarget;                         \
    STDMETHODIMP_(NTSTATUS) Init(THIS_      \
        IN PPORTWAVECYCLIC Port);           \
    STDMETHODIMP_(NTSTATUS) FreePin(THIS_   \
        IN struct IPortPinWaveCyclic* Pin)


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

    STDMETHOD_(ULONG, GetCompletedPosition)(THIS) PURE;
    STDMETHOD_(ULONG, GetCycleCount)(THIS) PURE;
    STDMETHOD_(ULONG, GetDeviceBufferSize)(THIS) PURE;
    STDMETHOD_(PVOID, GetIrpStream)(THIS) PURE;
    STDMETHOD_(PMINIPORT, GetMiniport)(THIS) PURE;
};

typedef IPortPinWaveCyclic *PPORTPINWAVECYCLIC;

#define IMP_IPortPinWaveCyclic                           \
    IMP_IIrpTarget;                                      \
    STDMETHODIMP_(NTSTATUS) Init(THIS_                   \
        IN PPORTWAVECYCLIC Port,                         \
        IN PPORTFILTERWAVECYCLIC Filter,                 \
        IN KSPIN_CONNECT * ConnectDetails,               \
        IN KSPIN_DESCRIPTOR * PinDescriptor);            \
    STDMETHODIMP_(ULONG) GetCompletedPosition(THIS);     \
    STDMETHODIMP_(ULONG) GetCycleCount(THIS);            \
    STDMETHODIMP_(ULONG) GetDeviceBufferSize(THIS);      \
    STDMETHODIMP_(PVOID) GetIrpStream(THIS);             \
    STDMETHODIMP_(PMINIPORT) GetMiniport(THIS)


/*****************************************************************************
 * IPortFilterDMus
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortFilterDMus

struct IPortPinDMus;

DECLARE_INTERFACE_(IPortFilterDMus, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTDMUS Port)PURE;

    STDMETHOD_(NTSTATUS, FreePin)(THIS_
        IN struct IPortPinDMus* Pin)PURE;

    STDMETHOD_(VOID, NotifyPins)(THIS) PURE;
};

typedef IPortFilterDMus *PPORTFILTERDMUS;

#define IMP_IPortFilterDMus                 \
    IMP_IIrpTarget;                         \
    STDMETHODIMP_(NTSTATUS) Init(THIS_      \
        IN PPORTDMUS Port);                 \
    STDMETHODIMP_(NTSTATUS) FreePin(THIS_   \
        IN struct IPortPinDMus* Pin);       \
    STDMETHODIMP_(VOID) NotifyPins(THIS)

/*****************************************************************************
 * IPortPinDMus
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortPinDMus

DECLARE_INTERFACE_(IPortPinDMus, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTDMUS Port,
        IN PPORTFILTERDMUS Filter,
        IN KSPIN_CONNECT * ConnectDetails,
        IN KSPIN_DESCRIPTOR * PinDescriptor,
        IN PDEVICE_OBJECT DeviceObject) PURE;

    STDMETHOD_(VOID, Notify)(THIS) PURE;
};

#define IMP_IPortPinDMus                       \
    IMP_IIrpTarget;                            \
    STDMETHODIMP_(NTSTATUS) Init(THIS_         \
        IN PPORTDMUS Port,                     \
        IN PPORTFILTERDMUS Filter,             \
        IN KSPIN_CONNECT * ConnectDetails,     \
        IN KSPIN_DESCRIPTOR * PinDescriptor,   \
        IN PDEVICE_OBJECT DeviceObject);       \
    STDMETHODIMP_(VOID) Notify(THIS)

typedef IPortPinDMus *PPORTPINDMUS;

/*****************************************************************************
 * IDmaChannelInit
 *****************************************************************************
 */

#ifdef _MSC_VER

#define IMP_IDmaChannelEx                                                 \
    STDMETHODIMP_(NTSTATUS) AllocateBuffer(                               \
        IN  ULONG BufferSize,                                             \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL);        \
                                                                          \
    STDMETHODIMP_(void) FreeBuffer(void);                                 \
    STDMETHODIMP_(ULONG) TransferCount(void);                             \
    STDMETHODIMP_(ULONG) MaximumBufferSize(void);                         \
    STDMETHODIMP_(ULONG) AllocatedBufferSize(void);                       \
    STDMETHODIMP_(ULONG) BufferSize(void);                                \
                                                                          \
    STDMETHODIMP_(void) SetBufferSize(                                    \
        IN  ULONG BufferSize);                                            \
                                                                          \
    STDMETHODIMP_(PVOID) SystemAddress(void);                             \
    STDMETHODIMP_(PHYSICAL_ADDRESS) PhysicalAddress();                    \
    STDMETHODIMP_(PADAPTER_OBJECT) GetAdapterObject(void);                \
                                                                          \
    STDMETHODIMP_(void) CopyTo(                                           \
        IN  PVOID Destination,                                            \
        IN  PVOID Source,                                                 \
        IN  ULONG ByteCount);                                             \
                                                                          \
    STDMETHODIMP_(void) CopyFrom(                                         \
        IN  PVOID Destination,                                            \
        IN  PVOID Source,                                                 \
        IN  ULONG ByteCount)

#else

#define IMP_IDmaChannelEx                                                 \
    STDMETHODIMP_(NTSTATUS) AllocateBuffer(                               \
        IN  ULONG BufferSize,                                             \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL);        \
                                                                          \
    STDMETHODIMP_(void) FreeBuffer(void);                                 \
    STDMETHODIMP_(ULONG) TransferCount(void);                             \
    STDMETHODIMP_(ULONG) MaximumBufferSize(void);                         \
    STDMETHODIMP_(ULONG) AllocatedBufferSize(void);                       \
    STDMETHODIMP_(ULONG) BufferSize(void);                                \
                                                                          \
    STDMETHODIMP_(void) SetBufferSize(                                    \
        IN  ULONG BufferSize);                                            \
                                                                          \
    STDMETHODIMP_(PVOID) SystemAddress(void);                             \
    STDMETHODIMP_(PHYSICAL_ADDRESS) PhysicalAddress(                      \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL);        \
    STDMETHODIMP_(PADAPTER_OBJECT) GetAdapterObject(void);                \
                                                                          \
    STDMETHODIMP_(void) CopyTo(                                           \
        IN  PVOID Destination,                                            \
        IN  PVOID Source,                                                 \
        IN  ULONG ByteCount);                                             \
                                                                          \
    STDMETHODIMP_(void) CopyFrom(                                         \
        IN  PVOID Destination,                                            \
        IN  PVOID Source,                                                 \
        IN  ULONG ByteCount)



#endif


#define IMP_IDmaChannelSlaveEx                 \
    IMP_IDmaChannelEx;                         \
    STDMETHODIMP_(NTSTATUS) Start(             \
        IN  ULONG MapSize,                     \
        IN  BOOLEAN WriteToDevice);            \
                                               \
    STDMETHODIMP_(NTSTATUS) Stop(void);        \
    STDMETHODIMP_(ULONG) ReadCounter(void);    \
                                               \
    STDMETHODIMP_(NTSTATUS) WaitForTC(         \
        ULONG Timeout)

#define IMP_IDmaChannelInit\
    IMP_IDmaChannelSlaveEx;\
    STDMETHODIMP_(NTSTATUS) Init( \
        IN  PDEVICE_DESCRIPTION DeviceDescription, \
        IN  PDEVICE_OBJECT DeviceObject)

#ifdef _MSC_VER

#define DEFINE_ABSTRACT_DMACHANNEL_EX() \
    STDMETHOD_(NTSTATUS, AllocateBuffer)( THIS_ \
        IN  ULONG BufferSize, \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL) PURE; \
\
    STDMETHOD_(void, FreeBuffer)( THIS ) PURE; \
    STDMETHOD_(ULONG, TransferCount)( THIS ) PURE; \
    STDMETHOD_(ULONG, MaximumBufferSize)( THIS ) PURE; \
    STDMETHOD_(ULONG, AllocatedBufferSize)( THIS ) PURE; \
    STDMETHOD_(ULONG, BufferSize)( THIS ) PURE; \
\
    STDMETHOD_(void, SetBufferSize)( THIS_ \
        IN  ULONG BufferSize) PURE; \
\
    STDMETHOD_(PVOID, SystemAddress)( THIS ) PURE; \
    STDMETHOD_(PHYSICAL_ADDRESS, PhysicalAddress)( THIS) PURE;  \
    STDMETHOD_(PADAPTER_OBJECT, GetAdapterObject)( THIS ) PURE; \
\
    STDMETHOD_(void, CopyTo)( THIS_ \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount) PURE; \
\
    STDMETHOD_(void, CopyFrom)( THIS_ \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount) PURE;
#else

#define DEFINE_ABSTRACT_DMACHANNEL_EX() \
    STDMETHOD_(NTSTATUS, AllocateBuffer)( THIS_ \
        IN  ULONG BufferSize, \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL) PURE; \
\
    STDMETHOD_(void, FreeBuffer)( THIS ) PURE; \
    STDMETHOD_(ULONG, TransferCount)( THIS ) PURE; \
    STDMETHOD_(ULONG, MaximumBufferSize)( THIS ) PURE; \
    STDMETHOD_(ULONG, AllocatedBufferSize)( THIS ) PURE; \
    STDMETHOD_(ULONG, BufferSize)( THIS ) PURE; \
\
    STDMETHOD_(void, SetBufferSize)( THIS_ \
        IN  ULONG BufferSize) PURE; \
\
    STDMETHOD_(PVOID, SystemAddress)( THIS ) PURE; \
    STDMETHOD_(PHYSICAL_ADDRESS, PhysicalAddress)( THIS_       \
        IN PPHYSICAL_ADDRESS Address) PURE; \
    STDMETHOD_(PADAPTER_OBJECT, GetAdapterObject)( THIS ) PURE; \
\
    STDMETHOD_(void, CopyTo)( THIS_ \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount) PURE; \
\
    STDMETHOD_(void, CopyFrom)( THIS_ \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount) PURE;

#endif
#undef INTERFACE
#define INTERFACE IDmaChannelInit

DECLARE_INTERFACE_(IDmaChannelInit, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_DMACHANNEL_EX()
    DEFINE_ABSTRACT_DMACHANNELSLAVE()

    STDMETHOD_(NTSTATUS, Init)( THIS_
        IN PDEVICE_DESCRIPTION DeviceDescription,
        IN PDEVICE_OBJECT DeviceObject) PURE;
};

#undef INTERFACE

/*****************************************************************************
 * IPortFilterTopology
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortFilterTopology

DECLARE_INTERFACE_(IPortFilterTopology, IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_IRPTARGET()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PPORTTOPOLOGY Port)PURE;
};

typedef IPortFilterTopology *PPORTFILTERTOPOLOGY;

#define IMP_IPortFilterTopology        \
    IMP_IIrpTarget;                    \
    STDMETHODIMP_(NTSTATUS) Init(THIS_ \
        IN PPORTTOPOLOGY Port)

#undef INTERFACE

/*****************************************************************************
 * IPortWaveRTStreamInit
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IPortWaveRTStreamInit


DECLARE_INTERFACE_(IPortWaveRTStreamInit, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(PMDL, AllocatePagesForMdl)
    (   THIS_
        IN      PHYSICAL_ADDRESS    HighAddress,
        IN      SIZE_T              TotalBytes
    )   PURE;

    STDMETHOD_(PMDL, AllocateContiguousPagesForMdl)
    (   THIS_
        IN      PHYSICAL_ADDRESS    LowAddress,
        IN      PHYSICAL_ADDRESS    HighAddress,
        IN      SIZE_T              TotalBytes
    )   PURE;

    STDMETHOD_(PVOID, MapAllocatedPages)
    (   THIS_
        IN      PMDL                    MemoryDescriptorList,
        IN      MEMORY_CACHING_TYPE     CacheType
    )   PURE;

    STDMETHOD_(VOID, UnmapAllocatedPages)
    (   THIS_
        IN      PVOID   BaseAddress,
        IN      PMDL    MemoryDescriptorList
    )   PURE;

    STDMETHOD_(VOID, FreePagesFromMdl)
    (   THIS_
        IN      PMDL    MemoryDescriptorList
    )   PURE;

    STDMETHOD_(ULONG, GetPhysicalPagesCount)
    (   THIS_
        IN      PMDL    MemoryDescriptorList
    )   PURE;

    STDMETHOD_(PHYSICAL_ADDRESS, GetPhysicalPageAddress)
    (   THIS_
        IN      PPHYSICAL_ADDRESS Address,
        IN      PMDL              MemoryDescriptorList,
        IN      ULONG             Index
    )   PURE;
};

#undef INTERFACE

#define IMP_IPortWaveRTStreamInit                                         \
    STDMETHODIMP_(PMDL) AllocatePagesForMdl                               \
    (   THIS_                                                             \
        IN      PHYSICAL_ADDRESS    HighAddress,                          \
        IN      SIZE_T              TotalBytes                            \
    );                                                                    \
                                                                          \
    STDMETHODIMP_(PMDL) AllocateContiguousPagesForMdl                     \
    (   THIS_                                                             \
        IN      PHYSICAL_ADDRESS    LowAddress,                           \
        IN      PHYSICAL_ADDRESS    HighAddress,                          \
        IN      SIZE_T              TotalBytes                            \
    );                                                                    \
                                                                          \
    STDMETHODIMP_(PVOID) MapAllocatedPages                                \
    (   THIS_                                                             \
        IN      PMDL                    MemoryDescriptorList,             \
        IN      MEMORY_CACHING_TYPE     CacheType                         \
    );                                                                    \
                                                                          \
    STDMETHODIMP_(VOID) UnmapAllocatedPages                               \
    (   THIS_                                                             \
        IN      PVOID   BaseAddress,                                      \
        IN      PMDL    MemoryDescriptorList                              \
    );                                                                    \
                                                                          \
    STDMETHODIMP_(VOID) FreePagesFromMdl                                  \
    (   THIS_                                                             \
        IN      PMDL    MemoryDescriptorList                              \
    );                                                                    \
                                                                          \
    STDMETHODIMP_(ULONG) GetPhysicalPagesCount                            \
    (   THIS_                                                             \
        IN      PMDL    MemoryDescriptorList                              \
    );                                                                    \
                                                                          \
    STDMETHODIMP_(PHYSICAL_ADDRESS) GetPhysicalPageAddress                \
    (   THIS_                                                             \
        IN      PPHYSICAL_ADDRESS Address,                                \
        IN      PMDL              MemoryDescriptorList,                   \
        IN      ULONG             Index                                   \
    )

#ifndef IMP_IPortClsVersion

#define IMP_IPortClsVersion \
    STDMETHODIMP_(DWORD) GetVersion(void);

#endif

#ifdef IMP_IPortWaveRT
#define IMP_IPortWaveRT IMP_IPort
#endif

#endif
