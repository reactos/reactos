#include "priv.h"
#include "srvwiz.h"
#include <dsrole.h>
#include <clusapi.h>
#include <winsvc.h>
#include "dspsprt.h"
#include "resource.h"

#define SZ_REGKEY_NTCV              TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion")

#define SZ_REGKEY_SRVWIZ            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SrvWiz")
#define SZ_REGKEY_TODOLIST          TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\OCManager\\ToDoList")
#define SZ_REGKEY_WELCOME           TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Tips")

#define SZ_REGKEY_MSMQ              TEXT("SOFTWARE\\Microsoft\\MSMQ\\Parameters")
#define SZ_REGKEY_RRAS              TEXT("System\\CurrentControlSet\\Services\\RemoteAccess")
#define SZ_REGKEY_NETSHOW           TEXT("SOFTWARE\\Microsoft\\NetShow")

//
// HOME_STATE decides the first page to show at start up
//

typedef enum _HOME_STATE {
    HOME_FINISH     = 0x00000000,   // finish OC setup
    HOME_INTRO1,                    // server choice page
    HOME_INTRO2,
    HOME_DETAILS,
    HOME_INTRO3,
    HOME_NEXT,
    HOME_TMP,                       // my first server middle state 
    HOME_CONFIG,                    // either the machine is a DC, or has run through my first server steps
    HOME_MULTISERVER                // user selected multiple server in intro1.htm
} HOME_STATE;

// utility functions declaration

BOOL ValidateDomainDNSName(LPWSTR pszDomainDNSName);
BOOL ValidateDomainNetBiosName(LPWSTR pszDomainNetBiosName);

BOOL IsTerminalServicesRunning(void);
BOOL VerifyClusterOS(void);
BOOL IsTerminalServicesInstalled(void);

LPTSTR WINAPI MyFormatString(UINT resId, ...);
LPSTR WINAPI MyFormatStringAnsi(UINT resId, ...);
HRESULT CreateTempFile(LPCTSTR szPath, LPSTR szText);

BOOL InstallDNS(BOOL bWait);
BOOL InstallDHCP(BOOL bWait);
BOOL InstallAD(BOOL bWait);
VOID SetStaticIPAddressAndSubnetMask();

EXTERN_C void RegisterWLNotifyEvent(void);
EXTERN_C BOOL IsDhcpServerAround(VOID);

class CSrvWiz : public ISrvWiz
              , public CImpIDispatch
{
public:
    // *** IUnknown Methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) ;
    STDMETHOD_(ULONG, Release)(void);

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT * pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // *** ISrvWiz
    STDMETHOD(get_Home)(/*[out, retval]*/ DWORD *pdwHome);
    STDMETHOD(put_Home)(/*[in]*/ DWORD dwHome);
    STDMETHOD(get_ShowAtStartup)(/*[out, retval]*/ BOOL *pbShow);
    STDMETHOD(put_ShowAtStartup)(/*[in]*/ BOOL bShow);
    STDMETHOD(put_DomainDNSName)(/*[in]*/ BSTR bstrName);
    STDMETHOD(put_DomainNetBiosName)(/*[in]*/ BSTR bstrName);
    STDMETHOD(get_ProductRegistered)(/*[out, retval]*/ BOOL *pbRet);
    STDMETHOD(DsRole)(/*[in*/ DWORD dwInfoLevel, /*[out, retval]*/ DWORD *pdwRole);
    
    STDMETHOD(ServiceState)(/*[in]*/ BSTR bstrService, /*[out, retval]*/ DWORD *pdwState);
    STDMETHOD(InstallService)(/*[in]*/ BSTR bstrService, /*[out, retval]*/ BOOL *pbRet);
    STDMETHOD(ValidateName)(/*[in]*/ BSTR bstrType, /*[in]*/ BSTR bstrName, /*[out, retval]*/ BOOL *pbRet);
    STDMETHOD(SetupFirstServer)(/*[out, retval]*/ BOOL *pbRet);
    STDMETHOD(CheckDHCPServer)(/*[out, retval]*/ BOOL *pbRet);

    CSrvWiz();
    ~CSrvWiz();

    // friend function
    friend HRESULT CSrvWiz_CreateInstance(REFIID riid, void **ppvObj);

private:
    UINT _cRef;
} ;

// constructor
CSrvWiz::CSrvWiz() : CImpIDispatch(&LIBID_SrvWizLib, 1, 0, &IID_ISrvWiz)
{
    _cRef = 1;

    DllAddRef();
}

// destructor
CSrvWiz::~CSrvWiz()
{
    DllRelease();
}

// ISrvWiz::QueryInterface
HRESULT CSrvWiz::QueryInterface(REFIID riid, LPVOID * ppvOut)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISrvWiz))
    {
        *ppvOut = SAFECAST(this, ISrvWiz *);
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvOut = SAFECAST(this, IDispatch *);
    }
    else
    {
        *ppvOut = NULL;
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

// ISrvWiz::AddRef
ULONG CSrvWiz::AddRef()
{
    _cRef++;

    return _cRef;
}

// ISrvWiz::Release
ULONG CSrvWiz::Release()
{
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;

    return 0;
}



//
// CSrvWiz IDispatch implementation
//

STDMETHODIMP CSrvWiz::GetTypeInfoCount(UINT * pctinfo)
{ 
    return CImpIDispatch::GetTypeInfoCount(pctinfo); 
}

STDMETHODIMP CSrvWiz::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{ 
    return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); 
}

STDMETHODIMP CSrvWiz::GetIDsOfNames(REFIID riid, OLECHAR ** rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{ 
    return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP CSrvWiz::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


//
// CSrvWiz ISrvWiz implementation
//

STDMETHODIMP CSrvWiz::get_Home(DWORD *pdwHome)
{
    HKEY hkey;
    DWORD cSubKeys = 0; // number of subkeys under ToDoList

    if (!pdwHome)
        return E_INVALIDARG;
    
    // Check ToDoList first. If there is anything under it, show finish.htm to finish setup

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, SZ_REGKEY_TODOLIST, &hkey))
    {
        RegQueryInfoKey(hkey, NULL, NULL, NULL, &cSubKeys, NULL, 
                        NULL, NULL, NULL, NULL, NULL, NULL
                       );
        RegCloseKey(hkey);
    }

    if (cSubKeys)
    {
        *pdwHome = HOME_FINISH;
    }
    else 
    {
        // Then check whether this machine is a DC. If so, go directly to config.htm

        DWORD dwDsRole = 0;

        DsRole(0, &dwDsRole);

        if (dwDsRole)
        {
            // remember state in registry

            DWORD cbSize = sizeof(*pdwHome);
            DWORD dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_SRVWIZ, TEXT("home"), 
                                     NULL, (LPVOID)pdwHome, &cbSize);

            if ((NO_ERROR != dwRet) || (HOME_CONFIG != *pdwHome))
            {
                put_Home(HOME_CONFIG);
            }

            *pdwHome = HOME_CONFIG;
        }
        else
        {
            // Last, check registry for state in My First Server steps

            DWORD cbSize = sizeof(*pdwHome);
            DWORD dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_SRVWIZ, TEXT("home"), 
                                     NULL, (LPVOID)pdwHome, &cbSize
                                    );

            if ((NO_ERROR != dwRet) ||          // First time run CYS
                (HOME_MULTISERVER < *pdwHome) || (HOME_FINISH > *pdwHome) ||    // corrupted registry
                (HOME_CONFIG == *pdwHome) ||    // the machine is no longer a DC
                (HOME_TMP == *pdwHome )         // my first server setup failed
               )
            {
                *pdwHome = HOME_INTRO1;
                put_Home(HOME_INTRO1);  // repair registry and start My First Server steps
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CSrvWiz::put_Home(DWORD dwHome)
{
    if ((HOME_MULTISERVER < dwHome) || (HOME_FINISH > dwHome))
        return E_INVALIDARG;

    SHSetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_SRVWIZ, TEXT("home"), 
               REG_DWORD, (LPVOID)&dwHome, sizeof(dwHome));

    return S_OK;
}

STDMETHODIMP CSrvWiz::get_ShowAtStartup(BOOL *pbShow)
{
    DWORD dwShow;
    DWORD cbShow = sizeof(dwShow);

    if (!pbShow)
        return E_INVALIDARG;

    *pbShow = TRUE; // always show it by default

    if (NO_ERROR == SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_WELCOME, TEXT("show"), 
                               NULL, &dwShow, &cbShow)
       )
    {
        *pbShow = (dwShow != 0);
    }

    return S_OK;
}

STDMETHODIMP CSrvWiz::put_ShowAtStartup(BOOL bShow)
{
    DWORD dwShow = bShow ? 1 : 0;
    
    SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_WELCOME, TEXT("show"), 
               REG_DWORD, (LPVOID)&dwShow, sizeof(dwShow));

    return S_OK;
}

STDMETHODIMP CSrvWiz::put_DomainDNSName(BSTR bstrDomainDNSName)
{
    if (NULL == bstrDomainDNSName || !*bstrDomainDNSName)
        return E_INVALIDARG;

    SHSetValueW(HKEY_LOCAL_MACHINE, SZ_REGKEY_SRVWIZ, L"DomainDNSName", 
                REG_SZ, bstrDomainDNSName, 
                (lstrlenW(bstrDomainDNSName)+1)*sizeof(WCHAR));

    return S_OK;
}

STDMETHODIMP CSrvWiz::put_DomainNetBiosName(BSTR bstrDomainNetBiosName)
{
    if (NULL == bstrDomainNetBiosName || !*bstrDomainNetBiosName)
        return E_INVALIDARG;

    SHSetValueW(HKEY_LOCAL_MACHINE, SZ_REGKEY_SRVWIZ, L"DomainNetBiosName", 
                REG_SZ, bstrDomainNetBiosName, 
                (lstrlenW(bstrDomainNetBiosName)+1)*sizeof(WCHAR));

    return S_OK;
}

STDMETHODIMP CSrvWiz::get_ProductRegistered(BOOL *pbReg)
{
    TCHAR szData[16];
    DWORD cbSize = sizeof(szData);
    DWORD dwRet;

    if (!pbReg)
        return E_INVALIDARG;

    *pbReg = FALSE; // Assume it is not registered
    
    dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_NTCV, TEXT("Regdone"), 
                       NULL, (LPVOID)szData, &cbSize);

    if ((NO_ERROR == dwRet) && !StrCmpI(TEXT("1"), szData))
        *pbReg = TRUE;

    return S_OK;
}

STDMETHODIMP CSrvWiz::DsRole(DWORD dwInfoLevel, DWORD *pdwRole)
{
    DWORD dwRet;

    // parameter checking
    if (!pdwRole )
    {
        return E_INVALIDARG;
    }

    // by default, assume the machine is not a DC, and not in the middle of upgrade
    *pdwRole = 0;

    if (dwInfoLevel == 0)
    {
        // check whetehr this machine is a DC
        
        PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pdib = NULL;

        dwRet = DsRoleGetPrimaryDomainInformation(
                    NULL, 
                    DsRolePrimaryDomainInfoBasic, 
                    (PBYTE *)&pdib
                    );
                    
        if ((ERROR_SUCCESS == dwRet) && pdib)
        {
            if ((DsRole_RoleBackupDomainController == pdib->MachineRole) ||
                (DsRole_RolePrimaryDomainController == pdib->MachineRole)
               )
            {
                *pdwRole = 1;   // this is a DC
            }
            else
            {
                *pdwRole = 0;   // this is not a DC
            }
            
            DsRoleFreeMemory(pdib);
        }
    } 
    else if (dwInfoLevel == 1)
    {
        // check whether this machine is in the middle of upgrade
        
        PDSROLE_UPGRADE_STATUS_INFO pusi = NULL;

        dwRet = DsRoleGetPrimaryDomainInformation(
                    NULL, 
                    DsRoleUpgradeStatus,
                    (PBYTE *)&pusi
                    );
                    
        if ((ERROR_SUCCESS == dwRet) && pusi)
        {
            if (DSROLE_UPGRADE_IN_PROGRESS == pusi->OperationState)
            {
                *pdwRole = 1;
            }
            
            DsRoleFreeMemory(pusi);
        }

    }

    return S_OK;
}

// For most services:
//     return -1 if the service should not be installed, 
//     return 0 if not installed, 
//     return 1 if installed
// For others that have more states, it is up to the caller to interprete the return value

STDMETHODIMP CSrvWiz::ServiceState(BSTR bstrService, DWORD *pdwState)
{
    if (!bstrService || !pdwState)
        return E_INVALIDARG;

    *pdwState = 0;

    if (!StrCmpIW(bstrService, L"TerminalServices"))
    {
        if (IsTerminalServicesRunning())
            *pdwState = 1;
    }
    else if (!StrCmpIW(bstrService, L"MessageQueue"))
    {
        // If reg value [HKLM\Software\Microsoft\MSMQ\Parameters,MaxSysQueue] exists,
        // we assume Message Queue is installed.
        
        DWORD dwRet;
        DWORD dwMaxSysQueue;
        DWORD cbSize = sizeof(dwMaxSysQueue);

        dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_MSMQ, TEXT("MaxSysQueue"), 
                           NULL, (LPVOID)&dwMaxSysQueue, &cbSize);

        if (NO_ERROR == dwRet)
        {
            *pdwState = 1;
        }
    }
    else if (!StrCmpIW(bstrService, L"RemoteAccess") || 
             !StrCmpIW(bstrService, L"Routing"))
    {   
        // If [HKLM\System\CurrentControlSet\Services\RemoteAccess, ConfigurationFlags(REG_DWORD)] == 1, 
        // Ras and Routing is configured. RRAS(Routing & Remote Access) is always installed.

        DWORD dwRet;
        DWORD dwCfgFlags;
        DWORD cbSize = sizeof(dwCfgFlags);

        dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_RRAS, TEXT("ConfigurationFlags"), 
                           NULL, (LPVOID)&dwCfgFlags, &cbSize);

        if ((NO_ERROR == dwRet) && (1 == dwCfgFlags))
        {
            *pdwState = 1;
        }
    } 
    else if (!StrCmpIW(bstrService, L"Clustering"))
    {   
        /* 
          public\sdk\inc\clusapi.h:

          #define CLUSTER_INSTALLED   0x00000001
          #define CLUSTER_CONFIGURED  0x00000002
          #define CLUSTER_RUNNING     0x00000010

          typedef enum NODE_CLUSTER_STATE {
          ClusterStateNotInstalled                = 0x00000000,
          ClusterStateNotConfigured               = CLUSTER_INSTALLED,
          ClusterStateNotRunning                  = CLUSTER_INSTALLED | CLUSTER_CONFIGURED,
          ClusterStateRunning                     = CLUSTER_INSTALLED | CLUSTER_CONFIGURED | CLUSTER_RUNNING
          } NODE_CLUSTER_STATE;
        */
        if (VerifyClusterOS() && !IsTerminalServicesInstalled())
        {
            GetNodeClusterState(NULL, pdwState);
        }
        else
        {
            // Clustering services should not be available on this platform.
            *pdwState = -1;
        }
    } 
    else if (!StrCmpIW(bstrService, L"NetShow"))
    {   
        // If we can find nsadmin.exe, we assume netshow is installed

        DWORD dwRet;
        TCHAR szPath[MAX_PATH];
        DWORD cbSize = sizeof(szPath);

        dwRet = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_NETSHOW, TEXT("InstallDir"), 
                           NULL, (LPVOID)szPath, &cbSize);

        if ((NO_ERROR == dwRet) && *szPath)
        {
            if (PathAppend(szPath, TEXT("Server\\nsadmin.exe")) && PathFileExists(szPath))
            {
                *pdwState = 1;
            }
        }
    } 
    else
    {
        // For all other services, if we can open the service, we assume it is installed

        SC_HANDLE hsc = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_READ);

        if (hsc)
        {
            SC_HANDLE hs = OpenServiceW(hsc, bstrService, GENERIC_READ);

            if (hs)
            {
                CloseServiceHandle(hs);
                *pdwState = 1;
            }

            CloseServiceHandle(hsc);
        }
    }

    return S_OK;
}

// BUGBUG: we can't wait for sysocmgr or dcpromo to finish
// and then check its return value for sucess because it
// can cause system reboot, so here we blindly assume success.
STDMETHODIMP CSrvWiz::InstallService(BSTR bstrService, BOOL *pbRet)
{
    if (!bstrService)
        return E_INVALIDARG;

    if (!StrCmpIW(bstrService, L"DNS"))
    {
        *pbRet = InstallDNS(FALSE);
    }
    else if (!StrCmpIW(bstrService, L"DHCPServer"))
    {
        *pbRet = InstallDHCP(FALSE);
    }
    else if (!StrCmpIW(bstrService, L"AD"))
    {
        *pbRet = InstallAD(FALSE);
    }
    else
        return E_INVALIDARG;

    return S_OK;
}

STDMETHODIMP CSrvWiz::ValidateName(BSTR bstrType, BSTR bstrName, BOOL *pbRet)
{
    HRESULT hr = S_OK;
    
    if (!bstrType || !bstrName || !pbRet)
        return E_INVALIDARG;

    // this is a lengthy operation
    HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    if (!StrCmpIW(bstrType, L"DNS"))
    {
        *pbRet = ValidateDomainDNSName(bstrName);
    }
    else if (!StrCmpIW(bstrType, L"NetBios"))
    {
        *pbRet = ValidateDomainNetBiosName(bstrName);
    }
    else
    {
        *pbRet = FALSE;
        hr = E_INVALIDARG;
    }

    SetCursor(hcurOld);
    
    return hr;
}

// BUGBUG: We can't wait for sysocmgr or dcpromo to finish
// and check the return value because it can cause a reboot.
// So here we register winlogon notify event first, 
// launch both sysocmgr and dcpromo without wait and 
// blindly assume success.
STDMETHODIMP CSrvWiz::SetupFirstServer(BOOL *pbRet)
{
    if (!pbRet)
        return E_INVALIDARG;

    *pbRet = TRUE;  // assume success

    SetStaticIPAddressAndSubnetMask();

    RegisterWLNotifyEvent();
    
    if (!InstallDHCP(TRUE))
    {
        goto Quit;
    }
        
    if (!InstallAD(FALSE))
    {
        goto Quit;
    }
    
    put_Home(HOME_TMP);

    return S_OK;

Quit:
    *pbRet = FALSE;
    return S_OK;
}

STDMETHODIMP CSrvWiz::CheckDHCPServer(BOOL *pbRet)
{
    WSADATA WsaData;
    DWORD Error;

    if (!pbRet)
        return E_INVALIDARG;

    *pbRet = TRUE;
    
    Error = WSAStartup(MAKEWORD(2,0), &WsaData );
    if( NO_ERROR == Error )
    {
        if( !IsDhcpServerAround() )
            *pbRet = FALSE;

        WSACleanup();
    }
    
    return S_OK;
}

/*----------------------------------------------------------
Purpose: Create-instance function for class factory

*/
HRESULT CSrvWiz_CreateInstance(REFIID riid, LPVOID * ppvObj)
{
    // aggregation checking is handled in class factory

    HRESULT hres = E_OUTOFMEMORY;

    CSrvWiz* pObj = new CSrvWiz();

    if (pObj)
    {
        hres = pObj->QueryInterface(riid, ppvObj);
        pObj->Release();
    }

    return hres;
}
