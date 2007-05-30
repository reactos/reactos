

#if !defined(_KS_)
#error KS.H must be included before KCOM.H
#endif

#if !defined(_KCOM_)
#define _KCOM_

#if defined(__cplusplus)
extern "C" {
#endif 

#define STATIC_KoCreateObject 0x72CF721CL, 0x525A, 0x11D1, 0x9A, 0xA1, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
DEFINE_GUIDSTRUCT("72CF721C-525A-11D1-9AA1-00A0C9223196", KoCreateObject);
#define KOSTRING_CreateObject L"{72CF721C-525A-11D1-9AA1-00A0C9223196}"

#ifndef CLSCTX_KERNEL_SERVER
#define CLSCTX_KERNEL_SERVER    0x00000200
#endif

#if !defined(__cplusplus) || _MSC_VER < 1100
#define STATIC_IID_IKoInitializeParentDeviceObject 0x21B36996, 0x8DE3, 0x11D1, 0x8A, 0xE0, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
DEFINE_GUIDEX(IID_IKoInitializeParentDeviceObject);
#else
interface __declspec(uuid("21B36996-8DE3-11D1-8AE0-00A0C9223196")) IKoInitializeParentDeviceObject;
#endif

#ifndef COMDDKMETHOD
#ifdef _COMDDK_
#define COMDDKMETHOD
#else 
#define COMDDKMETHOD DECLSPEC_IMPORT
#endif
#endif

#ifdef _COMDDK_
#define COMDDKAPI
#else 
#define COMDDKAPI DECLSPEC_IMPORT
#endif

typedef
NTSTATUS
(*KoCreateObjectHandler)( IN REFCLSID ClassId,
                          IN IUnknown* UnkOuter OPTIONAL,
                          IN REFIID InterfaceId,
                          OUT PVOID* Interface);

#undef INTERFACE
#define INTERFACE INonDelegatedUnknown
DECLARE_INTERFACE(INonDelegatedUnknown)
{
  STDMETHOD(NonDelegatedQueryInterface)
  (
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
  ) PURE;

  STDMETHOD_(ULONG,NonDelegatedAddRef)
  (
    THIS
  ) PURE;

  STDMETHOD_(ULONG,NonDelegatedRelease)
  (
    THIS
  ) PURE;
};

#undef INTERFACE
#define INTERFACE IIndirectedUnknown
DECLARE_INTERFACE(IIndirectedUnknown)
{
  STDMETHOD(IndirectedQueryInterface)
  (
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
  ) PURE;

  STDMETHOD_(ULONG,IndirectedAddRef)
  (
    THIS
  ) PURE;

  STDMETHOD_(ULONG,IndirectedRelease)
  (
    THIS
  ) PURE;
};


#undef INTERFACE
#define INTERFACE IKoInitializeParentDeviceObject
DECLARE_INTERFACE_(IKoInitializeParentDeviceObject, IUnknown)
{
  STDMETHOD(SetParentDeviceObject)
  (
    THIS_
    IN PDEVICE_OBJECT ParentDeviceObject
  ) PURE;
};



#if defined(__cplusplus)


class CBaseUnknown : public INonDelegatedUnknown, public IIndirectedUnknown
{
    protected:
        LONG m_RefCount;
    private:
        BOOLEAN m_UsingClassId;
        CLSID m_ClassId;
    protected:
        IUnknown* m_UnknownOuter;
    public:
        COMDDKMETHOD CBaseUnknown (IN REFCLSID ClassId, IN IUnknown* UnknownOuter OPTIONAL = NULL);
        COMDDKMETHOD CBaseUnknown(IN IUnknown* UnknownOuter OPTIONAL = NULL);
        COMDDKMETHOD virtual ~CBaseUnknown();
        COMDDKMETHOD STDMETHODIMP_(ULONG) NonDelegatedAddRef();
        COMDDKMETHOD STDMETHODIMP_(ULONG) NonDelegatedRelease();
        COMDDKMETHOD STDMETHODIMP NonDelegatedQueryInterface(IN REFIID InterfaceId,OUT PVOID* Interface);
        COMDDKMETHOD STDMETHODIMP_(ULONG) IndirectedAddRef();
        COMDDKMETHOD STDMETHODIMP_(ULONG) IndirectedRelease();
        COMDDKMETHOD STDMETHODIMP IndirectedQueryInterface(IN REFIID InterfaceId, OUT PVOID* Interface);
};


#if !defined(DEFINE_ABSTRACT_UNKNOWN)

#define DEFINE_ABSTRACT_UNKNOWN() \
  STDMETHOD(QueryInterface)(THIS_ REFIID InterfaceId, PVOID* Interface) PURE; \
  STDMETHOD_(ULONG,AddRef)(THIS) PURE; \
  STDMETHOD_(ULONG,Release)(THIS) PURE;

#endif

#define DEFINE_STD_UNKNOWN() \
  STDMETHODIMP NonDelegatedQueryInterface( REFIID InterfaceId, PVOID* Interface); \
  STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface); \
  STDMETHODIMP_(ULONG) AddRef(); \
  STDMETHODIMP_(ULONG) Release();

#define IMPLEMENT_STD_UNKNOWN(Class) \
  STDMETHODIMP Class::QueryInterface( REFIID InterfaceId, PVOID* Interface) \
  { \
    return m_UnknownOuter->QueryInterface(InterfaceId, Interface);\
  } \
  STDMETHODIMP_(ULONG) Class::AddRef() \
  { \
    return m_UnknownOuter->AddRef(); \
  } \
  STDMETHODIMP_(ULONG) Class::Release() \
  { \
    return m_UnknownOuter->Release(); \
  }
#else

COMDDKAPI
void
NTAPI
KoRelease( IN REFCLSID ClassId);
#endif // !__cplusplus

COMDDKAPI
NTSTATUS
NTAPI
KoCreateInstance( IN REFCLSID ClassId,
                  IN IUnknown* UnkOuter OPTIONAL,
                  IN ULONG ClsContext,
                  IN REFIID InterfaceId,
                  OUT PVOID* Interface);

COMDDKAPI
NTSTATUS
NTAPI
KoDeviceInitialize( IN PDEVICE_OBJECT DeviceObject);

COMDDKAPI
NTSTATUS
NTAPI
KoDriverInitialize( IN PDRIVER_OBJECT DriverObject,
                    IN PUNICODE_STRING RegistryPathName,
                    IN KoCreateObjectHandler CreateObjectHandler);


#if defined(__cplusplus)
}
#endif

#ifdef __cplusplus

#ifndef _NEW_DELETE_OPERATORS_
#define _NEW_DELETE_OPERATORS_

inline PVOID operator new
(
  size_t iSize,
  POOL_TYPE poolType
)
{
  PVOID result = ExAllocatePoolWithTag(poolType,iSize,'wNCK');
  if (result)
  {
    RtlZeroMemory(result,iSize);
  }
  return result;
}

inline PVOID operator new
(
    size_t          iSize,
    POOL_TYPE       poolType,
    ULONG           tag
)
{
  PVOID result = ExAllocatePoolWithTag(poolType,iSize,tag);
  if (result)
  {
    RtlZeroMemory(result,iSize);
  }
  return result;
}

inline void __cdecl operator delete
(
  PVOID pVoid
)
{
  ExFreePool(pVoid);
}

#endif

#if defined(_SYS_GUID_OPERATOR_EQ_)
#define _GUID_OPERATORS_
#pragma message("WARNING: Using system operator==/!= for GUIDs")
#endif

#ifndef _GUID_OPERATORS_
#define _GUID_OPERATORS_

__inline BOOL operator==(const GUID& guidOne, const GUID& guidOther)
{
  return IsEqualGUIDAligned(guidOne,guidOther);
}
__inline BOOL operator!=(const GUID& guidOne, const GUID& guidOther)
{
  return !(guidOne == guidOther);
}

#endif

#endif

#endif

