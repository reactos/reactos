//
//  APITHK.C
//
//  This file has API thunks that allow shell32 to load and run on
//  multiple versions of NT or Win95.  Since this component needs
//  to load on the base-level NT 4.0 and Win95, any calls to system
//  APIs introduced in later OS versions must be done via GetProcAddress.
// 
//  Also, any code that may need to access data structures that are
//  post-4.0 specific can be added here.
//
//  NOTE:  this file does *not* use the standard precompiled header,
//         so it can set _WIN32_WINNT to a later version.
//


#include "priv.h"       // Don't use precompiled header here
#include "appwiz.h"

#define c_szARPJob  TEXT("ARP Job")


// Return: hIOPort for the CompletionPort
HANDLE _SetJobCompletionPort(HANDLE hJob)
{
    HANDLE hRet = NULL;

    HANDLE hIOPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)hJob, 1 );
    if ( hIOPort != NULL )
    {
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT CompletionPort;
        CompletionPort.CompletionKey = hJob ;

        CompletionPort.CompletionPort = hIOPort;

        if (SetInformationJobObject( hJob,JobObjectAssociateCompletionPortInformation,
                                     &CompletionPort, sizeof(CompletionPort) ) )
        {   
            hRet = hIOPort;
        }
    }
    return hRet;
}


STDAPI_(DWORD) WaitingThreadProc(void *pv)
{
    HANDLE hIOPort = (HANDLE)pv;

    // RIP(hIOPort);
    
    DWORD dwCompletionCode;
    PVOID pCompletionKey;
    LPOVERLAPPED lpOverlapped;
    
    while (TRUE)
    {
        // Wait for all the processes to finish...
        if (!GetQueuedCompletionStatus( hIOPort, &dwCompletionCode, (PULONG_PTR) &pCompletionKey,
                                        &lpOverlapped, INFINITE ) || (dwCompletionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO))
        {
            break;
        }
    }
    
    return 0;
}


EXTERN_C void WaitForThreadDeath(HWND hDlg, HANDLE FAR * phThread, DWORD dwMilliseconds);


/*-------------------------------------------------------------------------
Purpose: Creates a process and waits for it to finish
*/
STDAPI_(BOOL) NT5_CreateAndWaitForProcess(LPTSTR pszExeName)
{
    PROCESS_INFORMATION pi = {0};
    STARTUPINFO si = {0};
    BOOL fWorked = FALSE;
#ifdef WX86
    DWORD  cchArch;
    WCHAR  szArchValue[32];
#endif    

    HANDLE hJob = CreateJobObject(NULL, c_szARPJob);
    
    if (hJob)
    {
        HANDLE hIOPort = _SetJobCompletionPort(hJob);
        if (hIOPort)
        {
            DWORD dwCreationFlags = 0;
            // Create the install process
            si.cb = sizeof(si);

#ifdef WX86
            if (bWx86Enabled && bForceX86Env) {
                cchArch = GetEnvironmentVariableW(ProcArchName,
                    szArchValue,
                    sizeof(szArchValue)
                    );

                if (!cchArch || cchArch >= sizeof(szArchValue)) {
                    szArchValue[0]=L'\0';
                }

                SetEnvironmentVariableW(ProcArchName, L"x86");
            }
#endif

            dwCreationFlags = CREATE_SUSPENDED | CREATE_SEPARATE_WOW_VDM;
            
            // Create the process
            fWorked = CreateProcess(NULL, pszExeName, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL,
                                    &si, &pi);
            if (fWorked)
            {
                BOOL bAssigned = FALSE;
            
                if (AssignProcessToJobObject(hJob, pi.hProcess))
                {                    
                    ResumeThread(pi.hThread);
                    bAssigned = TRUE;
                }
                else
                    TerminateProcess( pi.hProcess, ERROR_ACCESS_DENIED);

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
#ifdef WX86
                if (bWx86Enabled && bForceX86Env) 
                    SetEnvironmentVariableW(ProcArchName, szArchValue);
#endif

                // Now wait for the processes to finish
                if (bAssigned)
                {
                    HANDLE hThread;
                    DWORD idThread;

                    // Create the waiting thread.
                    hThread = CreateThread(NULL, 0, WaitingThreadProc,
                                 (LPVOID)hIOPort, 0, &idThread);

                    if (hThread)
                    {
                        WaitForThreadDeath(NULL, &hThread, INFINITE);
                        CloseHandle(hThread);
                    }
                }
            }
            CloseHandle(hIOPort);
        }
        CloseHandle(hJob);
    }
    return fWorked;
}





#define PFN_FIRSTTIME   ((void *)0xFFFFFFFF)


// GetLongPathName
typedef UINT (WINAPI * PFNGETLONGPATHNAME)(LPCTSTR pszShortPath, LPTSTR pszLongBuf, DWORD cchBuf); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's GetLongPathName
*/
DWORD NT5_GetLongPathName(LPCTSTR pszShortPath, LPTSTR pszLongBuf, DWORD cchBuf)
{
    static PFNGETLONGPATHNAME s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle KERNEL32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("KERNEL32.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNGETLONGPATHNAME)GetProcAddress(hinst, "GetLongPathNameW");
#else
            s_pfn = (PFNGETLONGPATHNAME)GetProcAddress(hinst, "GetLongPathNameA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pszShortPath, pszLongBuf, cchBuf);

    if (0 < cchBuf && pszLongBuf)
        *pszLongBuf = 0;
        
    return 0;       // failure
}


// VerSetConditionMask
typedef ULONGLONG (WINAPI * PFNVERSETCONDITIONMASK)(ULONGLONG conditionMask, DWORD dwTypeMask, BYTE condition); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's VerSetConditionMask
*/
ULONGLONG NT5_VerSetConditionMask(ULONGLONG conditionMask, DWORD dwTypeMask, BYTE condition)
{
    static PFNVERSETCONDITIONMASK s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle KERNEL32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("KERNEL32.DLL");

        if (hinst)
            s_pfn = (PFNVERSETCONDITIONMASK)GetProcAddress(hinst, "VerSetConditionMask");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(conditionMask, dwTypeMask, condition);

    return 0;       // failure
}



// MsiReinstallProduct
typedef UINT (WINAPI * PFNMSIREINSTALLPRODUCT) (LPCTSTR szProduct, DWORD dwReinstallMode); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiReinstallProduct
*/
UINT MSI_MsiReinstallProduct(LPCTSTR szProduct, DWORD dwReinstallMode)
{
    static PFNMSIREINSTALLPRODUCT s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIREINSTALLPRODUCT)GetProcAddress(hinst, "MsiReinstallProductW");
#else
            s_pfn = (PFNMSIREINSTALLPRODUCT)GetProcAddress(hinst, "MsiReinstallProductA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(szProduct, dwReinstallMode);

    return ERROR_CALL_NOT_IMPLEMENTED;
}


// MsiEnumProducts
typedef UINT (WINAPI * PFNMSIENUMPRODUCTS) (DWORD iProductIndex, LPTSTR lpProductBuf);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiEnumProducts
*/
UINT MSI_MsiEnumProducts(DWORD iProductIndex, LPTSTR lpProductBuf)
{
    static PFNMSIENUMPRODUCTS s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");
        
        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIENUMPRODUCTS)GetProcAddress(hinst, "MsiEnumProductsW");
#else
            s_pfn = (PFNMSIENUMPRODUCTS)GetProcAddress(hinst, "MsiEnumProductsA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(iProductIndex, lpProductBuf);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiEnumFeatures
typedef UINT (WINAPI * PFNMSIENUMFEATURES) (LPCTSTR  szProduct, DWORD iFeatureIndex, LPTSTR   lpFeatureBuf, LPTSTR   lpParentBuf);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiEnumFeatures
*/
UINT MSI_MsiEnumFeatures(LPCTSTR szProduct, DWORD iFeatureIndex, LPTSTR lpFeatureBuf, LPTSTR lpParentBuf)
{
    static PFNMSIENUMFEATURES s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIENUMFEATURES)GetProcAddress(hinst, "MsiEnumFeaturesW");
#else
            s_pfn = (PFNMSIENUMFEATURES)GetProcAddress(hinst, "MsiEnumFeaturesA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(szProduct, iFeatureIndex, lpFeatureBuf, lpParentBuf);

    return ERROR_CALL_NOT_IMPLEMENTED;
}


// MsiGetProductInfo
typedef UINT (WINAPI * PFNMSIGETPRODUCTINFO) (LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiGetProductInfo
*/
UINT MSI_MsiGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf)
{
    static PFNMSIGETPRODUCTINFO s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIGETPRODUCTINFO)GetProcAddress(hinst, "MsiGetProductInfoW");
#else
            s_pfn = (PFNMSIGETPRODUCTINFO)GetProcAddress(hinst, "MsiGetProductInfoA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(szProduct, szAttribute, lpValueBuf, pcchValueBuf);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiSetInternalUI
typedef INSTALLUILEVEL (WINAPI * PFNMSISETINTERNALUI) (INSTALLUILEVEL dwUILevel, HWND * phwnd);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiSetInternalUI
*/
INSTALLUILEVEL MSI_MsiSetInternalUI(INSTALLUILEVEL dwUILevel, HWND * phwnd)
{
    static PFNMSISETINTERNALUI s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            s_pfn = (PFNMSISETINTERNALUI)GetProcAddress(hinst, "MsiSetInternalUI");
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(dwUILevel, phwnd);

    return INSTALLUILEVEL_NOCHANGE;
}

// MsiConfigureProduct
typedef UINT (WINAPI * PFNMSICONFIGUREPRODUCT) (LPCTSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState);  

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiConfigureProduct
*/
UINT MSI_MsiConfigureProduct(LPCTSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState)
{
    static PFNMSICONFIGUREPRODUCT s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSICONFIGUREPRODUCT)GetProcAddress(hinst, "MsiConfigureProductW");
#else
            s_pfn = (PFNMSICONFIGUREPRODUCT)GetProcAddress(hinst, "MsiConfigureProductA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(szProduct, iInstallLevel, eInstallState);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiQueryProductState
typedef INSTALLSTATE (WINAPI * PFNMSIQUERYPRODUCTSTATE) (LPCTSTR szProductID);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiQueryProductState
*/
INSTALLSTATE MSI_MsiQueryProductState(LPCTSTR szProductID)
{
    static PFNMSIQUERYPRODUCTSTATE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIQUERYPRODUCTSTATE)GetProcAddress(hinst, "MsiQueryProductStateW");
#else
            s_pfn = (PFNMSIQUERYPRODUCTSTATE)GetProcAddress(hinst, "MsiQueryProductStateA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(szProductID);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiQueryFeatureState
typedef INSTALLSTATE (WINAPI * PFNMSIQUERYFEATURESTATE) (LPCTSTR szProductID, LPCTSTR szFeature);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiQueryFeatureState
*/
INSTALLSTATE MSI_MsiQueryFeatureState(LPCTSTR szProductID, LPCTSTR szFeature)
{
    static PFNMSIQUERYFEATURESTATE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIQUERYFEATURESTATE)GetProcAddress(hinst, "MsiQueryFeatureStateW");
#else
            s_pfn = (PFNMSIQUERYFEATURESTATE)GetProcAddress(hinst, "MsiQueryFeatureStateA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(szProductID, szFeature);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiOpenPackage
typedef UINT (WINAPI * PFNMSIOPENPACKAGE) (LPCTSTR szPackagePath, MSIHANDLE * hProduct);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiOpenPackage
*/
UINT MSI_MsiOpenPackage(LPCTSTR szPackagePath, MSIHANDLE * hProduct)
{
    static PFNMSIOPENPACKAGE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIOPENPACKAGE)GetProcAddress(hinst, "MsiOpenPackageW");
#else
            s_pfn = (PFNMSIOPENPACKAGE)GetProcAddress(hinst, "MsiOpenPackageA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(szPackagePath, hProduct);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiCloseHandle
typedef UINT (WINAPI * PFNMSICLOSEHANDLE) (MSIHANDLE hAny);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiCloseHandle
*/
UINT MSI_MsiCloseHandle(MSIHANDLE hAny)
{
    static PFNMSICLOSEHANDLE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            s_pfn = (PFNMSICLOSEHANDLE)GetProcAddress(hinst, "MsiCloseHandle");
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(hAny);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiDoAction
typedef UINT (WINAPI * PFNMSIDOACTION) (MSIHANDLE hAny, LPCTSTR szAction);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiDoAction
*/
UINT MSI_MsiDoAction(MSIHANDLE hAny, LPCTSTR szAction)
{
    static PFNMSIDOACTION s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIDOACTION)GetProcAddress(hinst, "MsiDoActionW");
#else
            s_pfn = (PFNMSIDOACTION)GetProcAddress(hinst, "MsiDoActionA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(hAny, szAction);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiGetFeatureCost
typedef UINT (WINAPI * PFNMSIGETFEATURECOST) (MSIHANDLE hInstall, LPCTSTR szFeature, MSICOSTTREE  iCostTree, INSTALLSTATE iState, INT *piCost);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiGetFeatureCost
*/
UINT MSI_MsiGetFeatureCost(MSIHANDLE hInstall, LPCTSTR szFeature, MSICOSTTREE  iCostTree, INSTALLSTATE iState, INT *piCost)
{
    static PFNMSIGETFEATURECOST s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIGETFEATURECOST)GetProcAddress(hinst, "MsiGetFeatureCostW");
#else
            s_pfn = (PFNMSIGETFEATURECOST)GetProcAddress(hinst, "MsiGetFeatureCostA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(hInstall, szFeature, iCostTree, iState, piCost);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

// MsiSetFeatureState
typedef UINT (WINAPI * PFNMSISETFEATURESTATE) (MSIHANDLE hInstall, LPCTSTR szFeature, INSTALLSTATE iState);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiSetFeatureState
*/
UINT MSI_MsiSetFeatureState(MSIHANDLE hInstall, LPCTSTR szFeature, INSTALLSTATE iState)
{
    static PFNMSISETFEATURESTATE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("MSI.DLL");

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSISETFEATURESTATE)GetProcAddress(hinst, "MsiSetFeatureStateW");
#else
            s_pfn = (PFNMSISETFEATURESTATE)GetProcAddress(hinst, "MsiSetFeatureStateA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(hInstall, szFeature, iState);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

typedef HRESULT (__stdcall * PFNCSGETAPPCATEGORIES)(APPCATEGORYINFOLIST *pAppCategoryList);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's CsGetAppCategories
*/
HRESULT NT5_CsGetAppCategories(APPCATEGORYINFOLIST *pAppCategoryList)
{
    static PFNCSGETAPPCATEGORIES s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("APPMGMTS.DLL");

        if (hinst)
            s_pfn = (PFNCSGETAPPCATEGORIES)GetProcAddress(hinst, "CsGetAppCategories");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pAppCategoryList);

    return E_NOTIMPL;    
}


typedef HRESULT (__stdcall * PFNRELEASEAPPCATEGORYINFOLIST)(APPCATEGORYINFOLIST *pAppCategoryList);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's ReleaseAppCategoryInfoList
*/
HRESULT NT5_ReleaseAppCategoryInfoList(APPCATEGORYINFOLIST *pAppCategoryList)
{
    static PFNRELEASEAPPCATEGORYINFOLIST s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("APPMGMTS.DLL");

        if (hinst)
            s_pfn = (PFNRELEASEAPPCATEGORYINFOLIST)GetProcAddress(hinst, "ReleaseAppCategoryInfoList");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pAppCategoryList);

    return E_NOTIMPL;    
}

/*----------------------------------------------------------
Purpose: Thunk for NT 5's AllowSetForegroundWindow
*/
typedef UINT (WINAPI * PFNALLOWSFW) (DWORD dwPRocessID);  

BOOL NT5_AllowSetForegroundWindow(DWORD dwProcessID)
{
    static PFNALLOWSFW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("USER32.DLL");

        if (hinst)
        {
            s_pfn = (PFNALLOWSFW)GetProcAddress(hinst, "AllowSetForegroundWindow");
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(dwProcessID);

    return FALSE;
}




// InstallApplication
typedef DWORD (WINAPI * PFNINSTALLAPP)(PINSTALLDATA pInstallInfo); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's InstallApplication
*/
DWORD NT5_InstallApplication(PINSTALLDATA pInstallInfo)
{
    static PFNINSTALLAPP s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle ADVAPI32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNINSTALLAPP)GetProcAddress(hinst, "InstallApplication");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pInstallInfo);

    return ERROR_INVALID_FUNCTION;       // failure
}


// UninstallApplication
typedef DWORD (WINAPI * PFNUNINSTALLAPP)(WCHAR * pszProductCode); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's UninstallApplication
*/
DWORD NT5_UninstallApplication(WCHAR * pszProductCode)
{
    static PFNUNINSTALLAPP s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle ADVAPI32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNUNINSTALLAPP)GetProcAddress(hinst, "UninstallApplication");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pszProductCode);

    return ERROR_INVALID_FUNCTION;       // failure
}


// GetApplicationState
typedef DWORD (WINAPI * PFNGETAPPSTATE)(WCHAR * ProductCode, APPSTATE * pAppState); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's GetApplicationState
*/
DWORD NT5_GetApplicationState(WCHAR * pszProductCode, APPSTATE * pAppState)
{
    static PFNGETAPPSTATE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle ADVAPI32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNGETAPPSTATE)GetProcAddress(hinst, "GetApplicationState");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pszProductCode, pAppState);

    return ERROR_INVALID_FUNCTION;       // failure
}


// CommandLineFromMsiDescriptor
typedef DWORD (WINAPI * PFNCMDLINE)(WCHAR * Descriptor, WCHAR * CommandLine, DWORD * CommandLineLength); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's CommandLineFromMsiDescriptor
*/
DWORD NT5_CommandLineFromMsiDescriptor(WCHAR * pszDescriptor, WCHAR * pszCommandLine, DWORD * pcchCommandLine)
{
    static PFNCMDLINE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle ADVAPI32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNCMDLINE)GetProcAddress(hinst, "CommandLineFromMsiDescriptor");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pszDescriptor, pszCommandLine, pcchCommandLine);

    return ERROR_INVALID_FUNCTION;       // failure
}


// GetManagedApplications
typedef DWORD (WINAPI * PFNGETAPPS)(GUID * pCategory, DWORD dwQueryFlags, DWORD dwInfoLevel, LPDWORD pdwApps, PMANAGEDAPPLICATION* prgManagedApps); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's GetManagedApplications
*/
DWORD NT5_GetManagedApplications(GUID * pCategory, DWORD dwQueryFlags, DWORD dwInfoLevel, LPDWORD pdwApps, PMANAGEDAPPLICATION* prgManagedApps)
{
    static PFNGETAPPS s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle ADVAPI32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNGETAPPS)GetProcAddress(hinst, "GetManagedApplications");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pCategory, dwQueryFlags, dwInfoLevel, pdwApps, prgManagedApps);

    return ERROR_INVALID_FUNCTION;       // failure
}


// NetGetJoinInformation
typedef NET_API_STATUS (WINAPI * PFNGETJOININFO)(LPCWSTR lpServer, LPWSTR *lpNameBuffer, PNETSETUP_JOIN_STATUS  BufferType); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's NetGetJoinInformation
*/
NET_API_STATUS NT5_NetGetJoinInformation(LPCWSTR lpServer, LPWSTR *lpNameBuffer, PNETSETUP_JOIN_STATUS  BufferType)
{
    static PFNGETJOININFO s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("NETAPI32.DLL");
        GetLastError();

        if (hinst)
            s_pfn = (PFNGETJOININFO)GetProcAddress(hinst, "NetGetJoinInformation");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(lpServer, lpNameBuffer, BufferType);

    return NERR_NetNotStarted;       // failure
}

// NetApiBufferFree
typedef NET_API_STATUS (WINAPI * PFNNETFREEBUFFER)(LPVOID lpBuffer); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's NetApiBufferFree
*/
NET_API_STATUS NT5_NetApiBufferFree(LPVOID lpBuffer)
{
    static PFNNETFREEBUFFER s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("NETAPI32.DLL");

        if (hinst)
            s_pfn = (PFNNETFREEBUFFER)GetProcAddress(hinst, "NetApiBufferFree");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(lpBuffer);

    return NERR_NetNotStarted;       // failure
}
