

ifndef _DMKSCTRL_
#define _DMKSCTRL_

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#include <pshpack8.h>
#include <objbase.h>

DEFINE_GUID(IID_IKsControl,                   0x28F54685, 0x06FD, 0x11D2, 0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);
#ifndef _KSMEDIA_
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_MIDI,        0x1D262760, 0xE957, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00);
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_DIRECTMUSIC, 0x1A82F8BC, 0x3F8B, 0x11D2, 0xB7, 0x74, 0x00, 0x60, 0x08, 0x33, 0x16, 0xC1);
#endif

#ifndef STATIC_IID_IKsControl
  #define STATIC_IID_IKsControl 0x28F54685L, 0x06FD, 0x11D2, 0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
#endif

#if !defined(_NTRTL_)
  #ifndef STATICGUIDOF
    #define STATICGUIDOF(guid) STATIC_##guid
  #endif
  #ifndef DEFINE_GUIDEX
    #define DEFINE_GUIDEX(name) EXTERN_C const CDECL GUID name
  #endif
#endif

#ifndef _KS_
#define _KS_
#define KSMETHOD_TYPE_NONE                      0x00000000
#define KSMETHOD_TYPE_READ                      0x00000001
#define KSMETHOD_TYPE_WRITE                     0x00000002
#define KSMETHOD_TYPE_MODIFY                    0x00000003
#define KSMETHOD_TYPE_SOURCE                    0x00000004
#define KSMETHOD_TYPE_SEND                      0x00000001
#define KSMETHOD_TYPE_SETSUPPORT                0x00000100
#define KSMETHOD_TYPE_BASICSUPPORT              0x00000200
#define KSPROPERTY_TYPE_GET                     0x00000001
#define KSPROPERTY_TYPE_SET                     0x00000002
#define KSPROPERTY_TYPE_SETSUPPORT              0x00000100
#define KSPROPERTY_TYPE_BASICSUPPORT            0x00000200
#define KSPROPERTY_TYPE_RELATIONS               0x00000400
#define KSPROPERTY_TYPE_SERIALIZESET            0x00000800
#define KSPROPERTY_TYPE_UNSERIALIZESET          0x00001000
#define KSPROPERTY_TYPE_SERIALIZERAW            0x00002000
#define KSPROPERTY_TYPE_UNSERIALIZERAW          0x00004000
#define KSPROPERTY_TYPE_SERIALIZESIZE           0x00008000
#define KSPROPERTY_TYPE_DEFAULTVALUES           0x00010000
#define KSPROPERTY_TYPE_TOPOLOGY                0x10000000

#if (defined(_MSC_EXTENSIONS) || defined(__cplusplus)) && !defined(CINTERFACE)
typedef struct
{
  union
  {
    struct
    {
      GUID Set;
      ULONG Id;
      ULONG Flags;
    };
  LONGLONG Alignment;
  };
} KSIDENTIFIER, *PKSIDENTIFIER,KSPROPERTY, *PKSPROPERTY, KSMETHOD, *PKSMETHOD, KSEVENT, *PKSEVENT;
#else
typedef struct
{
  union
  {
    struct
    {
      GUID Set;
      ULONG Id;
      ULONG Flags;
    } Data;
    LONGLONG Alignment;
  };
} KSIDENTIFIER, *PKSIDENTIFIER,KSPROPERTY, *PKSPROPERTY, KSMETHOD, *PKSMETHOD, KSEVENT, *PKSEVENT;
#endif
#endif

#ifndef _IKsControl_
#define _IKsControl_

#ifdef DECLARE_INTERFACE_


#undef INTERFACE
#define INTERFACE IKsControl
DECLARE_INTERFACE_(IKsControl, IUnknown)
{
  STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID FAR *) PURE;
  STDMETHOD_(ULONG,AddRef) (THIS) PURE;
  STDMETHOD_(ULONG,Release) (THIS) PURE;
  STDMETHOD(KsProperty)(THIS_ IN PKSPROPERTY Property, IN ULONG PropertyLength, IN OUT LPVOID PropertyData,
                              IN ULONG DataLength, OUT ULONG* BytesReturned) PURE;
  STDMETHOD(KsMethod)(THIS_ IN PKSMETHOD Method, IN ULONG MethodLength, IN OUT LPVOID MethodData,
                            IN ULONG DataLength, OUT ULONG* BytesReturned) PURE;
  STDMETHOD(KsEvent)(THIS_ IN PKSEVENT Event OPTIONAL, IN ULONG EventLength, IN OUT LPVOID EventData,
                           IN ULONG DataLength, OUT ULONG* BytesReturned) PURE;
};
#endif
#endif

#include <poppack.h>

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#endif