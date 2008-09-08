#ifndef __NETCON_H__
#define __NETCON_H__

typedef enum
{
    NCME_DEFAULT                = 0
}NETCONMGR_ENUM_FLAGS;

typedef enum
{
    NCCF_NONE                   = 0,
    NCCF_ALL_USERS              = 0x1,
    NCCF_ALLOW_DUPLICATION      = 0x2,
    NCCF_ALLOW_REMOVAL          = 0x4,
    NCCF_ALLOW_RENAME           = 0x8,
    NCCF_SHOW_ICON              = 0x10,
    NCCF_INCOMING_ONLY          = 0x20,
    NCCF_OUTGOING_ONLY          = 0x40,
    NCCF_BRANDED                = 0x80,
    NCCF_SHARED                 = 0x100,
    NCCF_BRIDGED                = 0x200,
    NCCF_FIREWALLED             = 0x400,
    NCCF_DEFAULT                = 0x800,
    NCCF_HOMENET_CAPABLE        = 0x1000,
    NCCF_SHARED_PRIVATE         = 0x2000,
    NCCF_QUARANTINED            = 0x4000,
    NCCF_RESERVED               = 0x8000,
    NCCF_BLUETOOTH_MASK         = 0xf0000,
    NCCF_LAN_MASK               = 0xf00000
}NETCON_CHARACTERISTIC_FLAGS;


typedef enum
{
    NCS_DISCONNECTED            = 0,
    NCS_CONNECTING              = NCS_DISCONNECTED + 1,
    NCS_CONNECTED               = NCS_CONNECTING + 1,
    NCS_DISCONNECTING           = NCS_CONNECTED + 1,
    NCS_HARDWARE_NOT_PRESENT    = NCS_DISCONNECTING + 1,
    NCS_HARDWARE_DISABLED       = NCS_HARDWARE_NOT_PRESENT + 1,
    NCS_HARDWARE_MALFUNCTION    = NCS_HARDWARE_DISABLED + 1,
    NCS_MEDIA_DISCONNECTED      = NCS_HARDWARE_MALFUNCTION + 1,
    NCS_AUTHENTICATING          = NCS_MEDIA_DISCONNECTED + 1,
    NCS_AUTHENTICATION_SUCCEEDED= NCS_AUTHENTICATING + 1,
    NCS_AUTHENTICATION_FAILED   = NCS_AUTHENTICATION_SUCCEEDED + 1,
    NCS_INVALID_ADDRESS         = NCS_AUTHENTICATION_FAILED + 1,
    NCS_CREDENTIALS_REQUIRED    = NCS_INVALID_ADDRESS + 1
}NETCON_STATUS;

typedef enum
{
    NCT_DIRECT_CONNECT          = 0,
    NCT_INBOUND                 = NCT_DIRECT_CONNECT + 1,
    NCT_INTERNET                = NCT_INBOUND + 1,
    NCT_LAN                     = NCT_INTERNET + 1,
    NCT_PHONE                   = NCT_LAN + 1,
    NCT_TUNNEL                  = NCT_PHONE + 1,
    NCT_BRIDGE                  = NCT_TUNNEL + 1
}NETCON_TYPE;


typedef enum
{
    NCM_NONE                    = 0,
    NCM_DIRECT                  = NCM_NONE + 1,
    NCM_ISDN                    = NCM_DIRECT + 1,
    NCM_LAN                     = NCM_ISDN + 1,
    NCM_PHONE                   = NCM_LAN + 1,
    NCM_TUNNEL                  = NCM_PHONE + 1,
    NCM_PPPOE                   = NCM_TUNNEL + 1,
    NCM_BRIDGE                  = NCM_PPPOE + 1,
    NCM_SHAREDACCESSHOST_LAN    = NCM_BRIDGE + 1,
    NCM_SHAREDACCESSHOST_RAS    = NCM_SHAREDACCESSHOST_LAN + 1
}NETCON_MEDIATYPE;


typedef struct tagNETCON_PROPERTIES
{
    GUID guidId;
    LPWSTR pszwName;
    LPWSTR pszwDeviceName;
    NETCON_STATUS Status;
    NETCON_MEDIATYPE MediaType;
    DWORD dwCharacter;
    CLSID clsidThisObject;
    CLSID clsidUiObject;
}NETCON_PROPERTIES;


#undef  INTERFACE
#define INTERFACE   INetConnection
DECLARE_INTERFACE_(INetConnection, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,Connect) (THIS) PURE;
    STDMETHOD_(HRESULT,Disconnect) (THIS) PURE;
    STDMETHOD_(HRESULT,Delete) (THIS) PURE;
    STDMETHOD_(HRESULT,Duplicate)(THIS_ LPCWSTR pszwDuplicateName, INetConnection **ppCon) PURE;
    STDMETHOD_(HRESULT,GetProperties) (THIS_ NETCON_PROPERTIES **ppProps) PURE;
    STDMETHOD_(HRESULT,GetUiObjectClassId) (THIS_ CLSID *pclsid) PURE;
    STDMETHOD_(HRESULT,Rename) (THIS_ LPCWSTR pszwNewName) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define INetConnection_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetConnection_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetConnection_Release(p)                 (p)->lpVtbl->Release(p)
#define INetConnection_Connect(p)                 (p)->lpVtbl->Connect(p)
#define INetConnection_Disconnect(p)              (p)->lpVtbl->Disconnect(p)
#define INetConnection_Delete(p)                  (p)->lpVtbl->Delete(p)
#define INetConnection_Duplicate(p,a,b)           (p)->lpVtbl->Duplicate(p,a,b)
#define INetConnection_GetProperties(p,a)         (p)->lpVtbl->GetProperties(p,a)
#define INetConnection_GetUiObjectClassId(p,a)    (p)->lpVtbl->GetUiObjectClassId(p,a)
#define INetConnection_Rename(p,a)                (p)->lpVtbl->Rename(p,a)
#endif

EXTERN_C const IID IID_INetConnection;


#undef  INTERFACE
#define INTERFACE   IEnumNetConnection
DECLARE_INTERFACE_(IEnumNetConnection, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(Next)(THIS_ ULONG celt, INetConnection **rgelt, ULONG *pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumNetConnection **ppenum) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IEnumNetConnection;

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IEnumNetConnection_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumNetConnection_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IEnumNetConnection_Release(p)                 (p)->lpVtbl->Release(p)
#define IEnumNetConnection_Next(p,a,b,c)              (p)->lpVtbl->Next(p,a,b,c)
#define IEnumNetConnection_Skip(p,a)                  (p)->lpVtbl->Skip(p,a)
#define IEnumNetConnection_Reset(p)                   (p)->lpVtbl->Reset(p)
#define IEnumNetConnection_Clone(p,a)                 (p)->lpVtbl->Clone(p,a)
#endif

#undef  INTERFACE
#define INTERFACE INetConnectionManager
DECLARE_INTERFACE_(INetConnectionManager, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT,EnumConnections) (THIS_ NETCONMGR_ENUM_FLAGS Flags, IEnumNetConnection **ppEnum) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define INetConnectionManager_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetConnectionManager_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetConnectionManager_Release(p)                 (p)->lpVtbl->Release(p)
#define INetConnectionManager_EnumConnections(p,a,b)     (p)->lpVtbl->EnumConnections(p,a,b)
#endif

EXTERN_C const CLSID CLSID_ConnectionManager;
EXTERN_C const IID IID_INetConnectionManager;

#undef  INTERFACE
#define INTERFACE INetConnectionPropertyUi
DECLARE_INTERFACE_(INetConnectionPropertyUi, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT, SetConnection) (THIS_ INetConnection *pCon) PURE;
    STDMETHOD_(HRESULT, AddPages) (THIS_ HWND hwndParent, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) PURE;


};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define INetConnectionPropertyUi_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetConnectionPropertyUi_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetConnectionPropertyUi_Release(p)                 (p)->lpVtbl->Release(p)
#define INetConnectionPropertyUi_SetConnection(p,a)         (p)->lpVtbl->SetConnection(p,a)
#define INetConnectionPropertyUi_AddPages(p,a,b,c)          (p)->lpVtbl->AddPages(p,a,b,c)
#endif

EXTERN_C const IID IID_INetConnectionPropertyUi;

#undef  INTERFACE
#define INTERFACE INetConnectionPropertyUi2
DECLARE_INTERFACE_(INetConnectionPropertyUi2, IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD_(HRESULT, SetConnection) (THIS_ INetConnection *pCon) PURE;
    STDMETHOD_(HRESULT, AddPages) (THIS_ HWND hwndParent, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) PURE;
    STDMETHOD_(HRESULT, GetIcon) (THIS_ DWORD dwSize, HICON *phIcon) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define INetConnectionPropertyUi2_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetConnectionPropertyUi2_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetConnectionPropertyUi2_Release(p)                 (p)->lpVtbl->Release(p)
#define INetConnectionPropertyUi2_SetConnection(p,a)         (p)->lpVtbl->SetConnection(p,a)
#define INetConnectionPropertyUi2_AddPages(p,a,b,c)          (p)->lpVtbl->AddPages(p,a,b,c)
#define INetConnectionPropertyUi2_GetIcon(p,a,b)             (p)->lpVtbl->GetIcon(p,a,b)
#endif


EXTERN_C const IID IID_INetConnectionPropertyUi2;


VOID STDCALL NcFreeNetconProperties (NETCON_PROPERTIES* pProps);

#endif
