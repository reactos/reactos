//
//  APITHK.H
//


#ifndef _APITHK_H_
#define _APITHK_H_

STDAPI_(BOOL) NT5_CreateAndWaitForProcess(LPTSTR pszExeName);


// Darwin API's 
STDAPI_(UINT) MSI_MsiEnumProducts(DWORD iProductIndex, LPTSTR lpProductBuf);
STDAPI_(UINT) MSI_MsiGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf); 
STDAPI_(UINT) MSI_MsiConfigureProduct(LPCTSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState);
STDAPI_(INSTALLUILEVEL) MSI_MsiSetInternalUI(INSTALLUILEVEL dwUILevel, HWND * phwnd);

STDAPI_(UINT) MSI_MsiReinstallProduct(LPCTSTR szProduct, DWORD szReinstallMode); 
STDAPI_(UINT) MSI_MsiOpenPackage(LPCTSTR szPackagePath, MSIHANDLE *hProduct);
STDAPI_(UINT) MSI_MsiEnumFeatures(LPCTSTR  szProduct, DWORD iFeatureIndex, LPTSTR   lpFeatureBuf, LPTSTR   lpParentBuf);

STDAPI_(INSTALLSTATE) MSI_MsiQueryProductState(LPCTSTR szProductID);
STDAPI_(INSTALLSTATE) MSI_MsiQueryFeatureState(LPCTSTR szProduct, LPCTSTR  szFeature);

// declared in msiquery.h
STDAPI_(UINT) MSI_MsiDoAction(MSIHANDLE hInstall, LPCTSTR szAction);
STDAPI_(UINT) MSI_MsiCloseHandle(MSIHANDLE hAny);
STDAPI_(UINT) MSI_MsiSetFeatureState(MSIHANDLE hInstall, LPCTSTR szFeature, INSTALLSTATE iState);
STDAPI_(UINT) MSI_MsiGetFeatureCost(MSIHANDLE hInstall, LPCTSTR szFeature, MSICOSTTREE  iCostTree, INSTALLSTATE iState, INT *piCost);

// Class store APIs
STDAPI  NT5_CsGetAppCategories(APPCATEGORYINFOLIST *pAppCategoryList);
STDAPI  NT5_ReleaseAppCategoryInfoList(APPCATEGORYINFOLIST *pAppCategoryList);

// Advapi APIs
STDAPI_(DWORD) NT5_InstallApplication(PINSTALLDATA pInstallInfo);
STDAPI_(DWORD) NT5_UninstallApplication(WCHAR * ProductCode);
STDAPI_(DWORD) NT5_GetApplicationState(WCHAR * ProductCode, APPSTATE * pAppState);
STDAPI_(DWORD) NT5_CommandLineFromMsiDescriptor(WCHAR * Descriptor, WCHAR * CommandLine, DWORD * CommandLineLength);
STDAPI_(DWORD) NT5_GetManagedApplications(GUID * pCategory, DWORD dwQueryFlags, DWORD dwInfoLevel, LPDWORD pdwApps, PMANAGEDAPPLICATION* prgManagedApps);

// Kernel APIs
STDAPI_(DWORD) NT5_GetLongPathName(LPCTSTR pszShortPath, LPTSTR pszLongBuf, DWORD cchBuf);
STDAPI_(ULONGLONG) NT5_VerSetConditionMask(ULONGLONG ConditionMask, DWORD TypeMask, BYTE Condition);

// User32 APIs
STDAPI_(BOOL) NT5_AllowSetForegroundWindow( DWORD dwProcessID );

// NetApi32
STDAPI_(NET_API_STATUS) NT5_NetGetJoinInformation(LPCWSTR lpServer, LPWSTR *lpNameBuffer, PNETSETUP_JOIN_STATUS  BufferType);
STDAPI_(NET_API_STATUS) NT5_NetApiBufferFree(LPVOID lpBuffer);


#define AllowSetForegroundWindow  NT5_AllowSetForegroundWindow

#define CsGetAppCategories      NT5_CsGetAppCategories
#define ReleaseAppCategoryInfoList  NT5_ReleaseAppCategoryInfoList

#undef  MsiEnumProducts
#define MsiEnumProducts         MSI_MsiEnumProducts

#undef  MsiGetProductInfo
#define MsiGetProductInfo       MSI_MsiGetProductInfo

#undef  MsiSetInternalUI
#define MsiSetInternalUI        MSI_MsiSetInternalUI

#undef  MsiConfigureProduct
#define MsiConfigureProduct     MSI_MsiConfigureProduct

#undef  MsiReinstallProduct
#define MsiReinstallProduct     MSI_MsiReinstallProduct

#undef  MsiQueryProductState
#define MsiQueryProductState    MSI_MsiQueryProductState

#undef  MsiQueryFeatureState
#define MsiQueryFeatureState    MSI_MsiQueryFeatureState

#undef  MsiOpenPackage
#define MsiOpenPackage          MSI_MsiOpenPackage

#undef  MsiEnumFeatures
#define MsiEnumFeatures         MSI_MsiEnumFeatures

#undef  MsiCloseHandle
#define MsiCloseHandle          MSI_MsiCloseHandle

#undef  MsiGetFeatureCost
#define MsiGetFeatureCost       MSI_MsiGetFeatureCost

#undef  MsiDoAction
#define MsiDoAction             MSI_MsiDoAction

#undef  MsiSetFeatureState
#define MsiSetFeatureState      MSI_MsiSetFeatureState

#undef  GetLongPathName
#define GetLongPathName         NT5_GetLongPathName

#define VerSetConditionMask     NT5_VerSetConditionMask

#define InstallApplication              NT5_InstallApplication
#define UninstallApplication            NT5_UninstallApplication
#define GetApplicationState             NT5_GetApplicationState
#define CommandLineFromMsiDescriptor    NT5_CommandLineFromMsiDescriptor
#define GetManagedApplications          NT5_GetManagedApplications
#define NetGetJoinInformation           NT5_NetGetJoinInformation
#define NetApiBufferFree                NT5_NetApiBufferFree
#endif // _APITHK_H_

