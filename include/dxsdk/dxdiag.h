#ifndef _DXDIAG_H_
#define _DXDIAG_H_

#include <ole2.h>

#define DXDIAG_DX9_SDK_VERSION 111

#ifdef __cplusplus
extern "C" {
#endif


DEFINE_GUID(CLSID_DxDiagProvider,    0xA65B8071, 0x3BFE, 0x4213, 0x9A, 0x5B, 0x49, 0x1D, 0xA4, 0x46, 0x1C, 0xA7);
DEFINE_GUID(IID_IDxDiagProvider,     0x9C6B4CB0, 0x23F8, 0x49CC, 0xA3, 0xED, 0x45, 0xA5, 0x50, 0x00, 0xA6, 0xD2);
DEFINE_GUID(IID_IDxDiagContainer,    0x7D0F462F, 0x4064, 0x4862, 0xBC, 0x7F, 0x93, 0x3E, 0x50, 0x58, 0xC1, 0x0F);

typedef struct _DXDIAG_INIT_PARAMS
{
  DWORD dwSize;
  DWORD dwDxDiagHeaderVersion;
  BOOL bAllowWHQLChecks;
  VOID* pReserved;
} DXDIAG_INIT_PARAMS;



typedef struct IDxDiagContainer *LPDXDIAGCONTAINER, *PDXDIAGCONTAINER;
#undef INTERFACE
#define INTERFACE IDxDiagContainer
DECLARE_INTERFACE_(IDxDiagContainer,IUnknown)
{

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(GetNumberOfChildContainers) (THIS_ DWORD *pdwCount) PURE;
    STDMETHOD(EnumChildContainerNames) (THIS_ DWORD dwIndex, LPWSTR pwszContainer, DWORD cchContainer) PURE;
    STDMETHOD(GetChildContainer) (THIS_ LPCWSTR pwszContainer, IDxDiagContainer **ppInstance) PURE;  
    STDMETHOD(GetNumberOfProps) (THIS_ DWORD *pdwCount) PURE;
    STDMETHOD(EnumPropNames) (THIS_ DWORD dwIndex, LPWSTR pwszPropName, DWORD cchPropName) PURE;
    STDMETHOD(GetProp) (THIS_ LPCWSTR pwszPropName, VARIANT *pvarProp) PURE;
};

 typedef struct IDxDiagProvider *LPDXDIAGPROVIDER, *PDXDIAGPROVIDER;
#undef INTERFACE
#define INTERFACE IDxDiagProvider
DECLARE_INTERFACE_(IDxDiagProvider,IUnknown)
{
  STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef) (THIS) PURE;
  STDMETHOD_(ULONG,Release) (THIS) PURE;
  STDMETHOD(Initialize) (THIS_ DXDIAG_INIT_PARAMS* pParams) PURE; 
  STDMETHOD(GetRootContainer) (THIS_ IDxDiagContainer **ppInstance) PURE;
};

#define DXDIAG_E_INSUFFICIENT_BUFFER ((HRESULT)0x8007007AL)
#if !defined(__cplusplus) || defined(CINTERFACE)
  #define IDxDiagProvider_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
  #define IDxDiagProvider_AddRef(p) (p)->lpVtbl->AddRef(p)
  #define IDxDiagProvider_Release(p) (p)->lpVtbl->Release(p)
  #define IDxDiagProvider_Initialize(p,a,b) (p)->lpVtbl->Initialize(p,a,b)
  #define IDxDiagProvider_GetRootContainer(p,a) (p)->lpVtbl->GetRootContainer(p,a)
  #define IDxDiagContainer_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
  #define IDxDiagContainer_AddRef(p) (p)->lpVtbl->AddRef(p)
  #define IDxDiagContainer_Release(p) (p)->lpVtbl->Release(p)
  #define IDxDiagContainer_GetNumberOfChildContainers(p,a) (p)->lpVtbl->GetNumberOfChildContainers(p,a)
  #define IDxDiagContainer_EnumChildContainerNames(p,a,b,c) (p)->lpVtbl->EnumChildContainerNames(p,a,b,c)
  #define IDxDiagContainer_GetChildContainer(p,a,b) (p)->lpVtbl->GetChildContainer(p,a,b)
  #define IDxDiagContainer_GetNumberOfProps(p,a) (p)->lpVtbl->GetNumberOfProps(p,a)
  #define IDxDiagContainer_EnumProps(p,a,b) (p)->lpVtbl->EnumProps(p,a,b,c)
  #define IDxDiagContainer_GetProp(p,a,b) (p)->lpVtbl->GetProp(p,a,b)
#else
  #define IDxDiagProvider_QueryInterface(p,a,b) (p)->QueryInterface(p,a,b)
  #define IDxDiagProvider_AddRef(p) (p)->AddRef(p)
  #define IDxDiagProvider_Release(p) (p)->Release(p)
  #define IDxDiagProvider_Initialize(p,a,b) (p)->Initialize(p,a,b)
  #define IDxDiagProvider_GetRootContainer(p,a) (p)->GetRootContainer(p,a)
  #define IDxDiagContainer_QueryInterface(p,a,b) (p)->QueryInterface(p,a,b)
  #define IDxDiagContainer_AddRef(p) (p)->AddRef(p)
  #define IDxDiagContainer_Release(p) (p)->Release(p)
  #define IDxDiagContainer_GetNumberOfChildContainers(p,a) (p)->GetNumberOfChildContainers(p,a)
  #define IDxDiagContainer_EnumChildContainerNames(p,a,b,c) (p)->EnumChildContainerNames(p,a,b,c)
  #define IDxDiagContainer_GetChildContainer(p,a,b) (p)->GetChildContainer(p,a,b)
  #define IDxDiagContainer_GetNumberOfProps(p,a) (p)->GetNumberOfProps(p,a)
  #define IDxDiagContainer_EnumProps(p,a,b) (p)->EnumProps(p,a,b,c)
  #define IDxDiagContainer_GetProp(p,a,b) (p)->GetProp(p,a,b)
#endif

#ifdef __cplusplus
}
#endif

#endif
