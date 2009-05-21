#ifndef _DMUSICKS_
#define _DMUSICKS_

#define DONT_HOLD_FOR_SEQUENCING 0x8000000000000000

#ifndef REFERENCE_TIME
typedef LONGLONG REFERENCE_TIME;
#endif

typedef struct _DMUS_KERNEL_EVENT
{
    BYTE bReserved;
    BYTE cbStruct;
    USHORT cbEvent;
    USHORT usChannelGroup;
    USHORT usFlags;
    REFERENCE_TIME ullPresTime100ns;
    ULONGLONG ullBytePosition;
    struct _DMUS_KERNEL_EVENT *pNextEvt;
    union
    {
        BYTE abData[sizeof(PBYTE)];
        PBYTE pbData;
        struct _DMUS_KERNEL_EVENT *pPackageEvt;
    }uData;
}DMUS_KERNEL_EVENT, *PDMUS_KERNEL_EVENT;

typedef enum
{
    DMUS_STREAM_MIDI_INVALID = -1,
    DMUS_STREAM_MIDI_RENDER = 0,
    DMUS_STREAM_MIDI_CAPTURE,
    DMUS_STREAM_WAVE_SINK
}DMUS_STREAM_TYPE;

DEFINE_GUID(CLSID_MiniportDriverDMusUART, 0xd3f0ce1c, 0xFFFC, 0x11D1, 0x81, 0xB0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xC1);
DEFINE_GUID(CLSID_MiniportDriverDMusUARTCapture, 0xD3F0CE1D, 0xFFFC, 0x11D1, 0x81, 0xB0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xC1);

/* ===============================================================
    IMasterClock Interface
*/

#undef INTERFACE
#define INTERFACE IMasterClock

DECLARE_INTERFACE_(IMasterClock,IUnknown)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(NTSTATUS,GetTime)( THIS_
        OUT     REFERENCE_TIME  * pTime
    ) PURE;
};

typedef IMasterClock *PMASTERCLOCK;

/* ===============================================================
    IMXF Interface
*/

#undef INTERFACE
#define INTERFACE IMXF

struct IMXF;
typedef struct IMXF *PMXF;

DECLARE_INTERFACE_(IMXF,IUnknown)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(NTSTATUS,SetState)(THIS_
        IN      KSSTATE State
    ) PURE;
    STDMETHOD_(NTSTATUS,PutMessage)
    (   THIS_
        IN      PDMUS_KERNEL_EVENT  pDMKEvt
    ) PURE;
    STDMETHOD_(NTSTATUS,ConnectOutput)
    (   THIS_
        IN      PMXF    sinkMXF
    ) PURE;
    STDMETHOD_(NTSTATUS,DisconnectOutput)
    (   THIS_
        IN      PMXF    sinkMXF
    ) PURE;
};

/* ===============================================================
    IAllocatorMXF Interface
*/

#undef INTERFACE
#define INTERFACE IAllocatorMXF

struct  IAllocatorMXF;
typedef struct IAllocatorMXF *PAllocatorMXF;

DECLARE_INTERFACE_(IAllocatorMXF,IMXF)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(NTSTATUS,SetState)(THIS_
        IN      KSSTATE State
    ) PURE;
    STDMETHOD_(NTSTATUS,PutMessage)
    (   THIS_
        IN      PDMUS_KERNEL_EVENT  pDMKEvt
    ) PURE;
    STDMETHOD_(NTSTATUS,ConnectOutput)
    (   THIS_
        IN      PMXF    sinkMXF
    ) PURE;
    STDMETHOD_(NTSTATUS,DisconnectOutput)
    (   THIS_
        IN      PMXF    sinkMXF
    ) PURE;

    STDMETHOD_(NTSTATUS,GetMessage)(THIS_
        OUT     PDMUS_KERNEL_EVENT * ppDMKEvt
    ) PURE;

    STDMETHOD_(USHORT,GetBufferSize)(THIS) PURE;

    STDMETHOD_(NTSTATUS,GetBuffer)(THIS_
        OUT     PBYTE * ppBuffer
    )PURE;

    STDMETHOD_(NTSTATUS,PutBuffer)(THIS_
        IN      PBYTE   pBuffer
    )   PURE;
};


#undef INTERFACE
#define INTERFACE IPortDMus

DEFINE_GUID(IID_IPortDMus, 0xc096df9c, 0xfb09, 0x11d1, 0x81, 0xb0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);
DEFINE_GUID(CLSID_PortDMus, 0xb7902fe9, 0xfb0a, 0x11d1, 0x81, 0xb0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);

DECLARE_INTERFACE_(IPortDMus, IPort)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(NTSTATUS,Init)(THIS_
        IN      PDEVICE_OBJECT  DeviceObject,
        IN      PIRP            Irp,
        IN      PUNKNOWN        UnknownMiniport,
        IN      PUNKNOWN        UnknownAdapter      OPTIONAL,
        IN      PRESOURCELIST   ResourceList 
    )PURE;
    STDMETHOD_(NTSTATUS,GetDeviceProperty)(THIS_
        IN      DEVICE_REGISTRY_PROPERTY    DeviceProperty,
        IN      ULONG                       BufferLength,
        OUT     PVOID                       PropertyBuffer,
        OUT     PULONG                      ResultLength
    )PURE;
    STDMETHOD_(NTSTATUS,NewRegistryKey)(THIS_
        OUT     PREGISTRYKEY *      OutRegistryKey,
        IN      PUNKNOWN            OuterUnknown,
        IN      ULONG               RegistryKeyType,
        IN      ACCESS_MASK         DesiredAccess,
        IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,
        IN      ULONG               CreateOptions       OPTIONAL,
        OUT     PULONG              Disposition         OPTIONAL 
    )PURE;
    STDMETHOD_(void,Notify)(THIS_
        IN PSERVICEGROUP ServiceGroup OPTIONAL
    )PURE;

    STDMETHOD_(void,RegisterServiceGroup)(THIS_
        IN PSERVICEGROUP ServiceGroup
    ) PURE;
};
typedef IPortDMus *PPORTDMUS;



#undef INTERFACE
#define INTERFACE IMiniportDMus

DEFINE_GUID(IID_IMiniportDMus, 0xc096df9d, 0xfb09, 0x11d1, 0x81, 0xb0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);
DECLARE_INTERFACE_(IMiniportDMus, IMiniport)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
    ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(NTSTATUS,GetDescription)(THIS_
        OUT     PPCFILTER_DESCRIPTOR *  Description
    ) PURE;
    STDMETHOD_(NTSTATUS,DataRangeIntersection)(THIS_
        IN      ULONG           PinId,
        IN      PKSDATARANGE    DataRange,
        IN      PKSDATARANGE    MatchingDataRange,
        IN      ULONG           OutputBufferLength,
        OUT     PVOID           ResultantFormat     OPTIONAL,
        OUT     PULONG          ResultantFormatLength
    ) PURE;

    STDMETHOD_(NTSTATUS,Init)(THIS_
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTDMUS       Port,
        OUT     PSERVICEGROUP * ServiceGroup
    )   PURE;

    STDMETHOD_(void,Service)(THIS) PURE;

    STDMETHOD_(NTSTATUS,NewStream)(THIS_
        OUT     PMXF                  * MXF,
        IN      PUNKNOWN                OuterUnknown    OPTIONAL,
        IN      POOL_TYPE               PoolType,
        IN      ULONG                   PinID,
        IN      DMUS_STREAM_TYPE        StreamType,
        IN      PKSDATAFORMAT           DataFormat,
        OUT     PSERVICEGROUP         * ServiceGroup,
        IN      PAllocatorMXF           AllocatorMXF,
        IN      PMASTERCLOCK            MasterClock,
        OUT     PULONGLONG              SchedulePreFetch
    ) PURE;
};

typedef IMiniportDMus *PMINIPORTDMUS;
#undef INTERFACE

#define STATIC_IID_IAllocatorMXF\
    0xa5f0d62c, 0xb30f, 0x11d2, 0xb7, 0xa3, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1
DEFINE_GUIDSTRUCT("a5f0d62c-b30f-11d2-b7a3-0060083316c1", IID_IAllocatorMXF);
#define IID_IAllocatorMXF DEFINE_GUIDNAMED(IID_IAllocatorMXF)

#endif
