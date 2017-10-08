#ifndef _DMUSICKS_
#define _DMUSICKS_

#include <dmusprop.h>

#define DONT_HOLD_FOR_SEQUENCING 0x8000000000000000

#ifndef REFERENCE_TIME
typedef LONGLONG REFERENCE_TIME;
#endif

typedef struct _DMUS_KERNEL_EVENT {
  BYTE bReserved;
  BYTE cbStruct;
  USHORT cbEvent;
  USHORT usChannelGroup;
  USHORT usFlags;
  REFERENCE_TIME ullPresTime100ns;
  ULONGLONG ullBytePosition;
  struct _DMUS_KERNEL_EVENT *pNextEvt;
  union {
    BYTE abData[sizeof(PBYTE)];
    PBYTE pbData;
    struct _DMUS_KERNEL_EVENT *pPackageEvt;
  } uData;
} DMUS_KERNEL_EVENT, *PDMUS_KERNEL_EVENT;

typedef enum {
  DMUS_STREAM_MIDI_INVALID = -1,
  DMUS_STREAM_MIDI_RENDER = 0,
  DMUS_STREAM_MIDI_CAPTURE,
  DMUS_STREAM_WAVE_SINK
} DMUS_STREAM_TYPE;

DEFINE_GUID(CLSID_MiniportDriverDMusUART,        0xd3f0ce1c, 0xFFFC, 0x11D1, 0x81, 0xB0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xC1);
DEFINE_GUID(CLSID_MiniportDriverDMusUARTCapture, 0xD3F0CE1D, 0xFFFC, 0x11D1, 0x81, 0xB0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xC1);

/* ===============================================================
    IMasterClock Interface
*/

#undef INTERFACE
#define INTERFACE IMasterClock

DECLARE_INTERFACE_(IMasterClock,IUnknown) {
  DEFINE_ABSTRACT_UNKNOWN()

  STDMETHOD_(NTSTATUS,GetTime)( THIS_
    _Out_ REFERENCE_TIME *pTime
  ) PURE;
};

typedef IMasterClock *PMASTERCLOCK;

#define IMP_IMasterClock             \
  STDMETHODIMP_(NTSTATUS) GetTime(   \
    _Out_ REFERENCE_TIME *pTime      \
  )

/* ===============================================================
    IMXF Interface
*/

#undef INTERFACE
#define INTERFACE IMXF

struct IMXF;
typedef struct IMXF *PMXF;

#define DEFINE_ABSTRACT_IMXF()                    \
  STDMETHOD_(NTSTATUS,SetState)(THIS_             \
    _In_ KSSTATE State                            \
  ) PURE;                                         \
  STDMETHOD_(NTSTATUS,PutMessage)(THIS_           \
    _In_ PDMUS_KERNEL_EVENT pDMKEvt               \
  ) PURE;                                         \
  STDMETHOD_(NTSTATUS,ConnectOutput)(THIS_        \
    _In_ PMXF sinkMXF                             \
  ) PURE;                                         \
  STDMETHOD_(NTSTATUS,DisconnectOutput)(THIS_     \
    _In_ PMXF sinkMXF                             \
  ) PURE;

#define IMP_IMXF                                  \
  STDMETHODIMP_(NTSTATUS) SetState (              \
    _In_ KSSTATE State);                          \
  STDMETHODIMP_(NTSTATUS) PutMessage (THIS_       \
    _In_ PDMUS_KERNEL_EVENT pDMKEvt);             \
  STDMETHODIMP_(NTSTATUS) ConnectOutput (THIS_    \
    _In_ PMXF sinkMXF);                           \
  STDMETHODIMP_(NTSTATUS) DisconnectOutput (THIS_ \
    _In_ PMXF sinkMXF)

DECLARE_INTERFACE_(IMXF,IUnknown) {
  DEFINE_ABSTRACT_UNKNOWN()
  DEFINE_ABSTRACT_IMXF()
};

/* ===============================================================
    IAllocatorMXF Interface
*/

#undef INTERFACE
#define INTERFACE IAllocatorMXF

struct  IAllocatorMXF;
typedef struct IAllocatorMXF *PAllocatorMXF;

#define STATIC_IID_IAllocatorMXF\
  0xa5f0d62c, 0xb30f, 0x11d2, {0xb7, 0xa3, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1}
DEFINE_GUIDSTRUCT("a5f0d62c-b30f-11d2-b7a3-0060083316c1", IID_IAllocatorMXF);
#define IID_IAllocatorMXF DEFINE_GUIDNAMED(IID_IAllocatorMXF)


DECLARE_INTERFACE_(IAllocatorMXF, IMXF) {
  DEFINE_ABSTRACT_UNKNOWN()

  DEFINE_ABSTRACT_IMXF()

  STDMETHOD_(NTSTATUS,GetMessage)(THIS_
    _Out_ PDMUS_KERNEL_EVENT *ppDMKEvt
  ) PURE;

  STDMETHOD_(USHORT,GetBufferSize)(THIS) PURE;

  STDMETHOD_(NTSTATUS,GetBuffer)(THIS_
    _Outptr_result_bytebuffer_(_Inexpressible_(GetBufferSize bytes)) PBYTE *ppBuffer
  )PURE;

  STDMETHOD_(NTSTATUS,PutBuffer)(THIS_
    _In_ PBYTE pBuffer
  ) PURE;
};

#define IMP_IAllocatorMXF                                            \
  IMP_IMXF;                                                          \
  STDMETHODIMP_(NTSTATUS) GetMessage(                                \
    _Out_ PDMUS_KERNEL_EVENT *ppDMKEvt);                             \
                                                                     \
  STDMETHODIMP_(USHORT) GetBufferSize(void);                         \
                                                                     \
  STDMETHODIMP_(NTSTATUS) GetBuffer(                                 \
    _Outptr_result_bytebuffer_(_Inexpressible_(GetBufferSize bytes)) \
      PBYTE *ppBuffer);                                              \
                                                                     \
  STDMETHODIMP_(NTSTATUS) PutBuffer(                                 \
    _In_ PBYTE pBuffer)

#undef INTERFACE
#define INTERFACE IPortDMus

DEFINE_GUID(IID_IPortDMus, 0xc096df9c, 0xfb09, 0x11d1, 0x81, 0xb0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);
DEFINE_GUID(CLSID_PortDMus, 0xb7902fe9, 0xfb0a, 0x11d1, 0x81, 0xb0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);

DECLARE_INTERFACE_(IPortDMus, IPort) {
  DEFINE_ABSTRACT_UNKNOWN()

  DEFINE_ABSTRACT_PORT()

  STDMETHOD_(void,Notify)(THIS_
    _In_opt_ PSERVICEGROUP ServiceGroup
  ) PURE;

  STDMETHOD_(void,RegisterServiceGroup)(THIS_
    _In_ PSERVICEGROUP ServiceGroup
  ) PURE;
};
typedef IPortDMus *PPORTDMUS;

#define IMP_IPortDMus                         \
  IMP_IPort;                                  \
  STDMETHODIMP_(void) Notify(                 \
    _In_opt_ PSERVICEGROUP ServiceGroup);  \
                                              \
  STDMETHODIMP_(void) RegisterServiceGroup(   \
    _In_ PSERVICEGROUP ServiceGroup)

#undef INTERFACE
#define INTERFACE IMiniportDMus

DEFINE_GUID(IID_IMiniportDMus, 0xc096df9d, 0xfb09, 0x11d1, 0x81, 0xb0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);

DECLARE_INTERFACE_(IMiniportDMus, IMiniport) {
  DEFINE_ABSTRACT_UNKNOWN()

  DEFINE_ABSTRACT_MINIPORT()

  STDMETHOD_(NTSTATUS,Init)(THIS_
    _In_opt_ PUNKNOWN UnknownAdapter,
    _In_ PRESOURCELIST ResourceList,
    _In_ PPORTDMUS Port,
    _Out_ PSERVICEGROUP *ServiceGroup
  ) PURE;

  STDMETHOD_(void,Service)(THIS) PURE;

  STDMETHOD_(NTSTATUS,NewStream)(THIS_
    _Out_ PMXF *MXF,
    _In_opt_ PUNKNOWN OuterUnknown,
    _In_ POOL_TYPE PoolType,
    _In_ ULONG PinID,
    _In_ DMUS_STREAM_TYPE StreamType,
    _In_ PKSDATAFORMAT DataFormat,
    _Out_ PSERVICEGROUP *ServiceGroup,
    _In_ PAllocatorMXF AllocatorMXF,
    _In_ PMASTERCLOCK MasterClock,
    _Out_ PULONGLONG SchedulePreFetch
  ) PURE;
};

typedef IMiniportDMus *PMINIPORTDMUS;
#undef INTERFACE

#define IMP_IMiniportDMus               \
  IMP_IMiniport;                        \
  STDMETHODIMP_(NTSTATUS) Init(         \
    _In_opt_ PUNKNOWN UnknownAdapter,   \
    _In_ PRESOURCELIST ResourceList,    \
    _In_ PPORTDMUS Port,                \
    _Out_ PSERVICEGROUP *ServiceGroup); \
                                        \
  STDMETHODIMP_(void) Service(THIS);    \
                                        \
  STDMETHODIMP_(NTSTATUS) NewStream(    \
    _Out_ PMXF *MXF,                    \
    _In_opt_ PUNKNOWN OuterUnknown,     \
    _In_ POOL_TYPE PoolType,            \
    _In_ ULONG PinID,                   \
    _In_ DMUS_STREAM_TYPE StreamType,   \
    _In_ PKSDATAFORMAT DataFormat,      \
    _Out_ PSERVICEGROUP *ServiceGroup,  \
    _In_ PAllocatorMXF AllocatorMXF,    \
    _In_ PMASTERCLOCK MasterClock,      \
    _Out_ PULONGLONG SchedulePreFetch)


#define STATIC_KSAUDFNAME_DMUSIC_MPU_OUT\
    0xA4DF0EB5, 0xBAC9, 0x11d2, {0xB7, 0xA8, 0x00, 0x60, 0x08, 0x33, 0x16, 0xC1}
DEFINE_GUIDSTRUCT("A4DF0EB5-BAC9-11d2-B7A8-0060083316C1", KSAUDFNAME_DMUSIC_MPU_OUT);
#define KSAUDFNAME_DMUSIC_MPU_OUT DEFINE_GUIDNAMED(KSAUDFNAME_DMUSIC_MPU_OUT)

#define STATIC_KSAUDFNAME_DMUSIC_MPU_IN\
    0xB2EC0A7D, 0xBAC9, 0x11d2, {0xB7, 0xA8, 0x00, 0x60, 0x08, 0x33, 0x16, 0xC1}
DEFINE_GUIDSTRUCT("B2EC0A7D-BAC9-11d2-B7A8-0060083316C1", KSAUDFNAME_DMUSIC_MPU_IN);
#define KSAUDFNAME_DMUSIC_MPU_IN DEFINE_GUIDNAMED(KSAUDFNAME_DMUSIC_MPU_IN)

#define STATIC_IID_IMXF\
    0xc096df9e, 0xfb09, 0x11d1, {0x81, 0xb0, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1}
DEFINE_GUIDSTRUCT("c096df9e-fb09-11d1-81b0-0060083316c1", IID_IMXF);
#define IID_IMXF DEFINE_GUIDNAMED(IID_IMXF)

#define DMUS_KEF_EVENT_COMPLETE     0x0000
#define DMUS_KEF_EVENT_INCOMPLETE   0x0001
#define DMUS_KEF_PACKAGE_EVENT      0x0002
#define kBytePositionNone   (~(ULONGLONG)0)

#define SHORT_EVT(evt)       ((evt)->cbEvent <= sizeof(PBYTE))
#define PACKAGE_EVT(evt)     ((evt)->usFlags & DMUS_KEF_PACKAGE_EVENT)
#define INCOMPLETE_EVT(evt)  ((evt)->usFlags & DMUS_KEF_EVENT_INCOMPLETE)
#define COMPLETE_EVT(evt)    (((evt)->usFlags & DMUS_KEF_EVENT_INCOMPLETE) == 0)

#define SET_INCOMPLETE_EVT(evt) ((evt)->usFlags |= DMUS_KEF_EVENT_INCOMPLETE)
#define SET_COMPLETE_EVT(evt)   ((evt)->usFlags &= (~DMUS_KEF_EVENT_INCOMPLETE))
#define SET_PACKAGE_EVT(evt)    ((evt)->usFlags |= DMUS_KEF_PACKAGE_EVENT)
#define CLEAR_PACKAGE_EVT(evt)  ((evt)->usFlags &= (~DMUS_KEF_PACKAGE_EVENT))

#endif /* _DMUSICKS_ */
