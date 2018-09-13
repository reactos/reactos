/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    factory.cpp

Abstract:

    This module implements CClassFactory class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "factory.h"
#include "about.h"



const TCHAR* const REG_MMC_SNAPINS  =  TEXT("Software\\Microsoft\\MMC\\Snapins");
const TCHAR* const REG_MMC_NODETYPE =  TEXT("Software\\Microsoft\\MMC\\NodeTypes");
const TCHAR* const MMC_NAMESTRING   =  TEXT("NameString");
const TCHAR* const MMC_PROVIDER     =   TEXT("Provider");
const TCHAR* const MMC_VERSION   =  TEXT("Version");
const TCHAR* const MMC_NODETYPES =  TEXT("NodeTypes");
const TCHAR* const MMC_STANDALONE    =  TEXT("StandAlone");
const TCHAR* const MMC_EXTENSIONS    =  TEXT("Extensions");
const TCHAR* const MMC_NAMESPACE     =  TEXT("NameSpace");
const TCHAR* const MMC_ABOUT         = TEXT("About");
const TCHAR* const REG_INPROCSERVER32    = TEXT("InprocServer32");
const TCHAR* const REG_THREADINGMODEL    = TEXT("ThreadingModel");
const TCHAR* const REG_CLSID         = TEXT("CLSID");
const TCHAR* const REG_PROGID        = TEXT("ProgId");
const TCHAR* const REG_VERSIONINDEPENDENTPROGID = TEXT("VersionIndependentProgId");
const TCHAR* const APARTMENT         = TEXT("Apartment");
//
// CClassFactory implmentation
//


LONG CClassFactory::s_Locks = 0;
LONG CClassFactory::s_Objects = 0;

ULONG
CClassFactory::AddRef()
{
    ::InterlockedIncrement((LONG*)&m_Ref);
    return m_Ref;
}
ULONG
CClassFactory::Release()
{
    ::InterlockedDecrement((LONG*)&m_Ref);
    if (!m_Ref)
    {
    delete this;
    return 0;
    }
    return m_Ref;
}

STDMETHODIMP
CClassFactory::QueryInterface(
    REFIID riid,
    LPVOID*  ppv
    )
{

    if (!ppv)
    {
    return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown))
    {
    *ppv = (IUnknown *)(IClassFactory *)this;
    }
    else if (IsEqualIID(riid, IID_IClassFactory))
    {
    *ppv = (IUnknown *)(IClassFactory *)this;
    }
    else
    {
    hr = E_NOINTERFACE;
    }
    if (SUCCEEDED(hr))
    AddRef();
    else
    *ppv = NULL;
    return hr;
}


STDMETHODIMP
CClassFactory::CreateInstance(
    IUnknown    *pUnkOuter,
    REFIID       riid,
    LPVOID  *ppv
    )
{

    if (!ppv)
    return E_INVALIDARG;

    HRESULT hr = S_OK;
    *ppv = NULL;

    if (pUnkOuter != NULL)
    hr = CLASS_E_NOAGGREGATION;
    try
    {
    switch (m_ClassType)
    {
        case DM_CLASS_TYPE_SNAPIN:
        {
            // create the factory with the request class(class type).
            // When a new OLE object is created, it initializes its
            // ref count to 1. We do a release right after the QI
            // so that if the QI failed, the object will be self-destructed
            CComponentData* pCompData = new CComponentDataPrimary();
            hr = pCompData->QueryInterface(riid, ppv);
            pCompData->Release();
            break;
        }
        case DM_CLASS_TYPE_SNAPIN_EXTENSION:
        {
            // create the factory with the request class(class type).
            // When a new OLE object is created, it initializes its
            // ref count to 1. We do a release right after the QI
            // so that if the QI failed, the object will be self-destructed
            CComponentData* pCompData = new CComponentDataExtension();
            hr = pCompData->QueryInterface(riid, ppv);
            pCompData->Release();
            break;
        }
        case DM_CLASS_TYPE_SNAPIN_ABOUT:
        {
            // create the factory with the request class(class type).
            // When a new OLE object is created, it initializes its
            // ref count to 1. We do a release right after the QI
            // so that if the QI failed, the object will be self-destructed
            CDevMgrAbout* pAbout = new CDevMgrAbout;
            hr = pAbout->QueryInterface(riid, ppv);
            pAbout->Release();
            break;
        }
        default:
        {
            hr = E_NOINTERFACE;
        }
    }
    }
    catch (CMemoryException* e)
    {
    hr = E_OUTOFMEMORY;
    e->Delete();
    }
    return hr;
}



STDMETHODIMP
CClassFactory::LockServer(
    BOOL fLock
    )
{
    if (fLock)
    ::InterlockedIncrement((LONG*)&s_Locks);
    else
    ::InterlockedDecrement((LONG*)&s_Locks);
    return S_OK;
}

HRESULT
CClassFactory::CanUnloadNow()
{
    return (s_Objects || s_Locks) ? S_FALSE : S_OK;
}


//
// This function create a CClassFactory. It is mainly called
// by DllGetClassObject API
// INPUT:
//  rclsid  -- reference to the CLSID
//  riid    -- reference to the interface IID
//  ppv -- interface pointer holder
//
// OUTPUT:
//  S_OK if succeeded else standard OLE error code
//
//
HRESULT
CClassFactory::GetClassObject(
    REFCLSID rclsid,
    REFIID   riid,
    void**   ppv
    )
{
    if (!ppv)
    return E_INVALIDARG;
    *ppv = NULL;
    HRESULT hr = S_OK;
    DM_CLASS_TYPE ClassType;
    //
    // determine the class type so that CreateInstance will be
    // creating the right object. We use a single class factory
    // to create all the object types.
    //
    if (IsEqualCLSID(rclsid, CLSID_DEVMGR))
    ClassType = DM_CLASS_TYPE_SNAPIN;
    else if (IsEqualCLSID(rclsid, CLSID_DEVMGR_EXTENSION))
    ClassType = DM_CLASS_TYPE_SNAPIN_EXTENSION;
    else if (IsEqualCLSID(rclsid, CLSID_DEVMGR_ABOUT))
    ClassType = DM_CLASS_TYPE_SNAPIN_ABOUT;
    else
    {
    ClassType = DM_CLASS_TYPE_UNKNOWN;
    hr = E_NOINTERFACE;
    }
    if (SUCCEEDED(hr))
    {
    CClassFactory* pUnk;
    // guard memory allocation error because we do not want
    // to cause an execption here.
    try
    {
        // create the factory with the request class(class type).
        // When a new OLE object is created, it initializes its
        // ref count to 1. We do a release right after the QI
        // so that if the QI failed, the object will be self-destructed

        pUnk = new CClassFactory(ClassType);
        hr = pUnk->QueryInterface(riid, ppv);
        pUnk->Release();
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        hr = E_OUTOFMEMORY;
    }
    }
    return hr;
}


//
// This function registers the dll to MMC
//
HRESULT
CClassFactory::RegisterAll()
{
    BOOL Result;
    TCHAR szText[MAX_PATH];
    {
    TCHAR ModuleName[MAX_PATH];
    GetModuleFileName(g_hInstance, ModuleName, ARRAYLEN(ModuleName));

    Result = FALSE;
    // first register standalone snapin CLSID
    CSafeRegistry regRootCLSID;
    if (regRootCLSID.Open(HKEY_CLASSES_ROOT, REG_CLSID))
    {
        CSafeRegistry regCLSID;
        // register our CLSID to HKEY_CLASS_ROOT\CLSID
        if (regCLSID.Create(regRootCLSID, CLSID_STRING_DEVMGR))
        {
        // write the description
        ::LoadString(g_hInstance, IDS_DESC_DEVMGR, szText, ARRAYLEN(szText));
        if (regCLSID.SetValue(NULL, szText))
        {
            CSafeRegistry regServer;
            if (regServer.Create(regCLSID, REG_INPROCSERVER32)&&
            regServer.SetValue(NULL, ModuleName) &&
            regServer.SetValue(REG_THREADINGMODEL, APARTMENT))
            {
            CSafeRegistry regProgId;
            if (regProgId.Create(regCLSID, REG_PROGID) &&
                regProgId.SetValue(NULL, PROGID_DEVMGR))
            {
                CSafeRegistry regVerIndProgId;
                if (regVerIndProgId.Create(regCLSID, REG_VERSIONINDEPENDENTPROGID))
                {
                Result = regVerIndProgId.SetValue(NULL, PROGID_DEVMGR);
                }
            }
            }
        }
        }
        if (Result)
        {
        regCLSID.Close();
        Result = FALSE;
        // register extension snapin CLSID
        if (regCLSID.Create(regRootCLSID, CLSID_STRING_DEVMGR_EXTENSION))
        {
            ::LoadString(g_hInstance, IDS_EXTENSION_DESC, szText, ARRAYLEN(szText));
            if (regCLSID.SetValue(NULL, szText))
            {
            CSafeRegistry regServer;
            if (regServer.Create(regCLSID, REG_INPROCSERVER32)&&
                regServer.SetValue(NULL, ModuleName) &&
                regServer.SetValue(REG_THREADINGMODEL, APARTMENT))
            {
                CSafeRegistry regProgId;
                if (regProgId.Create(regCLSID, REG_PROGID) &&
                regProgId.SetValue(NULL, PROGID_DEVMGREXT))
                {
                CSafeRegistry regVerIndProgId;
                if (regVerIndProgId.Create(regCLSID, REG_VERSIONINDEPENDENTPROGID))
                {
                    Result = regVerIndProgId.SetValue(NULL, PROGID_DEVMGREXT);
                }
                }
            }
            }
        }
        }
        if (Result)
        {
        regCLSID.Close();
        Result = FALSE;
        // register snapin about CLSID
        if (regCLSID.Create(regRootCLSID, CLSID_STRING_DEVMGR_ABOUT))
        {
            ::LoadString(g_hInstance, IDS_ABOUT_DEVMGR, szText, ARRAYLEN(szText));
            if (regCLSID.SetValue(NULL, szText))
            {
            CSafeRegistry regServer;
            if (regServer.Create(regCLSID, REG_INPROCSERVER32)&&
                regServer.SetValue(NULL, ModuleName) &&
                regServer.SetValue(REG_THREADINGMODEL, APARTMENT))
            {
                CSafeRegistry regProgId;
                if (regProgId.Create(regCLSID, REG_PROGID) &&
                regProgId.SetValue(NULL, PROGID_DEVMGR_ABOUT))
                {
                CSafeRegistry regVerIndProgId;
                if (regVerIndProgId.Create(regCLSID, REG_VERSIONINDEPENDENTPROGID))
                {
                    Result = regVerIndProgId.SetValue(NULL, PROGID_DEVMGR_ABOUT);
                }
                }
            }
            }
        }
        }
    }
    }
    if (Result)
    {
    Result = FALSE;
    CSafeRegistry regSnapins;
    //
    // open mmc snapin subkey
    //
    if (regSnapins.Open(HKEY_LOCAL_MACHINE, REG_MMC_SNAPINS))
    {
        PNODEINFO pni = (PNODEINFO)&NodeInfo[COOKIE_TYPE_SCOPEITEM_DEVMGR];
        CSafeRegistry regDevMgr;
        if (regDevMgr.Create(regSnapins, CLSID_STRING_DEVMGR))
        {
        ::LoadString(g_hInstance, pni->idsName, szText, ARRAYLEN(szText));
        if (regDevMgr.SetValue(MMC_NAMESTRING, szText))
        {
            ::LoadString(g_hInstance, IDS_PROGRAM_PROVIDER, szText, ARRAYLEN(szText));
            if (regDevMgr.SetValue(MMC_PROVIDER, szText))
            {
            ::LoadString(g_hInstance, IDS_PROGRAM_VERSION, szText, ARRAYLEN(szText));
            if (regDevMgr.SetValue(MMC_VERSION, szText) &&
                regDevMgr.SetValue(MMC_ABOUT, CLSID_STRING_DEVMGR_ABOUT))
            {
                // let MMC knows that we are a standalone snapin --
                // meaning we do not need any extension snapins for us
                // to run.
                CSafeRegistry regStandAlone;
                Result = regStandAlone.Create(regDevMgr, MMC_STANDALONE);
            }
            }
        }
        }
        CSafeRegistry regMMCNodeTypes;
        if (Result)
        {
        // populate our nodes
        Result = regMMCNodeTypes.Open(HKEY_LOCAL_MACHINE, REG_MMC_NODETYPE);
        if (Result)
        {
            CSafeRegistry regTheNode;
            int i = NODETYPE_FIRST;
            do
            {
            PNODEINFO pni = (PNODEINFO) &NodeInfo[i];
            Result = regTheNode.Create(regMMCNodeTypes, pni->GuidString);
            regTheNode.Close();
            } while (Result && ++i <= NODETYPE_LAST);
        }
        }
        if (Result)
        {
        // register as an extension to Computer management snapin
        CSafeRegistry regDevMgrExt;
        if (regDevMgrExt.Create(regSnapins, CLSID_STRING_DEVMGR_EXTENSION))
        {
            ::LoadString(g_hInstance, IDS_EXTENSION_DESC, szText, ARRAYLEN(szText));
            if (regDevMgrExt.SetValue(MMC_NAMESTRING, szText))
            {
            ::LoadString(g_hInstance, IDS_PROGRAM_PROVIDER, szText, ARRAYLEN(szText));
            if (regDevMgrExt.SetValue(MMC_PROVIDER, szText))
            {
                ::LoadString(g_hInstance, IDS_PROGRAM_VERSION, szText, ARRAYLEN(szText));
                if (regDevMgrExt.SetValue(MMC_VERSION, szText) &&
                regDevMgrExt.SetValue(MMC_ABOUT, CLSID_STRING_DEVMGR_ABOUT))
                {
                CSafeRegistry regSysTools;
                if (regSysTools.Open(regMMCNodeTypes, CLSID_STRING_SYSTOOLS))
                {
                    CSafeRegistry regExtensions;
                    if (regExtensions.Open(regSysTools,MMC_EXTENSIONS))
                    {
                    CSafeRegistry regNameSpace;
                    if (regNameSpace.Open(regExtensions, MMC_NAMESPACE))
                    {
                        // add our guid as a value of Name space
                        Result = regNameSpace.SetValue(CLSID_STRING_DEVMGR_EXTENSION, szText);
                    }
                    }
                }
                }
            }
            }
        }
        }
    }
    }
    if (!Result)
    {
    HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
    UnregisterAll();
    return hr;
    }
    return S_OK;
}
//
// This function unregisters the dll from MMC
//
HRESULT
CClassFactory::UnregisterAll()
{

    HRESULT hr = S_OK;
    CSafeRegistry regSnapins;
    //
    // open mmc snapin subkey
    //
    if (regSnapins.Open(HKEY_LOCAL_MACHINE, REG_MMC_SNAPINS))
    {
    // remove devmgr subkey from MMC snapins main key
    // both primary and extension
    regSnapins.DeleteSubkey(CLSID_STRING_DEVMGR);
    regSnapins.DeleteSubkey(CLSID_STRING_DEVMGR_EXTENSION);

    // removed populated node types
    CSafeRegistry regMMCNodeTypes;
    if (regMMCNodeTypes.Open(HKEY_LOCAL_MACHINE, REG_MMC_NODETYPE))
    {
        for (int i = NODETYPE_FIRST; i <= NODETYPE_LAST; i++)
        {
        PNODEINFO pni = (PNODEINFO) &NodeInfo[i];
        regMMCNodeTypes.DeleteValue(pni->GuidString);
        }
        // remove from system tools
        CSafeRegistry regSysTools;
        if (regSysTools.Open(regMMCNodeTypes, CLSID_STRING_SYSTOOLS))
        {
        CSafeRegistry regExtensions;
        if (regExtensions.Open(regSysTools, MMC_EXTENSIONS))
        {
            CSafeRegistry regNameSpace;
            if (regNameSpace.Open(regExtensions, MMC_NAMESPACE))
            regNameSpace.DeleteValue(CLSID_STRING_DEVMGR_EXTENSION);
        }
        }
    }
    }
    // unregister from OLE
    CSafeRegistry regRootCLSID;
    if (regRootCLSID.Open(HKEY_CLASSES_ROOT, REG_CLSID))
    {
    regRootCLSID.DeleteSubkey(CLSID_STRING_DEVMGR);
    regRootCLSID.DeleteSubkey(CLSID_STRING_DEVMGR_EXTENSION);
    regRootCLSID.DeleteSubkey(CLSID_STRING_DEVMGR_ABOUT);
    }
    return S_OK;
}
