#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <objbase.h>
#include <netcfgx.h>
#include <setupapi.h>
#include <netcfgn.h>
#include <devguid.h>
#include <commctrl.h>

#include "resource.h"

typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);
typedef struct {
    REFIID riid;
    LPFNCREATEINSTANCE lpfnCI;
} INTERFACE_TABLE;

typedef struct tagNetCfgComponentItem
{
    LPWSTR szDisplayName;       //Y
    LPWSTR szHelpText;          //Y
    LPWSTR szId;                //Y
    LPWSTR szBindName;          //Y
    LPWSTR szNodeId;            //Y
    CLSID InstanceId;           //Y
    CLSID ClassGUID;            //Y
    DWORD dwCharacteristics;    //Y
    ULONG Status;               //Y
    BOOL bChanged;              //Y
    struct tagNetCfgComponentItem * pNext;
    INetCfgComponentControl * pNCCC;
}NetCfgComponentItem;

/* netcfg_iface.c */
HRESULT WINAPI INetCfg_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

/* classfactory.c */
IClassFactory * IClassFactory_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst);

/* globals */
extern HINSTANCE netcfgx_hInstance;

/* inetcfgcomp_iface.c */
HRESULT WINAPI INetCfgComponent_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv, NetCfgComponentItem * pItem,INetCfg * iface);
HRESULT WINAPI IEnumNetCfgComponent_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv, NetCfgComponentItem * pItem, INetCfg * iface);

/* tcpipconf_notify.c */
HRESULT WINAPI TcpipConfigNotify_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

extern const GUID CLSID_TcpipConfigNotifyObject;

#endif /* _PRECOMP_H__ */
