// Appmgr.cpp : Implementation of CShellAppManager
#include "priv.h"

#include "appmgr.h"
#include "instenum.h"
#include "util.h"
#ifndef DOWNLEVEL_PLATFORM
#include "pubenum.h"
#endif //DOWNLEVEL_PLATFORM
#include "sccls.h"

#ifndef DOWNLEVEL_PLATFORM
#pragma message("DOWNLEVEL_PLATFORM is NOT defined")
#endif //DOWNLEVEL_PLATFORM

const TCHAR c_szTSMsiHackKey[] = TEXT("Software\\Policies\\Microsoft\\Windows\\Installer\\Terminal Server");
const TCHAR c_szTSMsiHackValue[] = TEXT("EnableAdminRemote");

/////////////////////////////////////////////////////////////////////////////
// CShellAppManager

// constructor
CShellAppManager::CShellAppManager() : _cRef(1)
{
    DllAddRef();
    TraceAddRef(CShellAppManager, _cRef);
    
    ASSERT(_hdpaPub == NULL);
#ifndef DOWNLEVEL_PLATFORM
    InitializeCriticalSection(&_cs);
    
    HDCA hdca = DCA_Create();
    if (hdca)
    {
        // Enumerate all of the Application Publishers
        DCA_AddItemsFromKey(hdca, HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPUBLISHER);
        if (DCA_GetItemCount(hdca) > 0)
        {
            _Lock();
            // Create our internal list of IAppPublisher *  
            _hdpaPub = DPA_Create(4);

            if(_hdpaPub)
            {
                int idca;
                for (idca = 0; idca < DCA_GetItemCount(hdca); idca++)
                {
                    IAppPublisher * pap;
                    if (FAILED(DCA_CreateInstance(hdca, idca, IID_IAppPublisher, (LPVOID *) &pap)))
                        continue;

                    ASSERT(IS_VALID_CODE_PTR(pap, IAppPublisher));

                    if (DPA_AppendPtr(_hdpaPub, pap) == DPA_ERR)
                    {
                        pap->Release();
                        break;
                    }
                }

                // if we have no pointers in this array, don't bother create one
                if (DPA_GetPtrCount(_hdpaPub) == 0)
                {
                    DPA_Destroy(_hdpaPub);
                    _hdpaPub = NULL;
                }
            }
            _Unlock();
        }

        DCA_Destroy(hdca);
    }

    
    if (IsTerminalServicesRunning())
    {
        // Hack for MSI work on Terminal Server
        HKEY hkeyMsiHack = NULL; 
        DWORD lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szTSMsiHackKey, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyMsiHack);
        if (lRet == ERROR_FILE_NOT_FOUND)
        {
            lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szTSMsiHackKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE,
                                  NULL, &hkeyMsiHack, NULL);
        }

        if (lRet == ERROR_SUCCESS)
        {
            DWORD dwType = 0;
            DWORD dwTSMsiHack = 0;
            DWORD cbSize = SIZEOF(dwTSMsiHack);


            if ((ERROR_SUCCESS != RegQueryValueEx(hkeyMsiHack, c_szTSMsiHackValue, 0, &dwType, (LPBYTE)&dwTSMsiHack, &cbSize))
                || (dwType != REG_DWORD) || (dwTSMsiHack != 1))
            {
                dwTSMsiHack = 1;
                if (RegSetValueEx(hkeyMsiHack, c_szTSMsiHackValue, 0, REG_DWORD, (LPBYTE)&dwTSMsiHack, SIZEOF(dwTSMsiHack)) == ERROR_SUCCESS)
                    _bCreatedTSMsiHack = TRUE;
            }

            RegCloseKey(hkeyMsiHack);
        }
    }
#endif //DOWNLEVEL_PLATFORM
}


// destructor
CShellAppManager::~CShellAppManager()
{
    if (_bCreatedTSMsiHack)
    {
        ASSERT(IsTerminalServicesRunning());
        HKEY hkeyMsiHack;
        DWORD lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szTSMsiHackKey, 0, KEY_SET_VALUE, &hkeyMsiHack);
        if (ERROR_SUCCESS == lRet)
        {
            RegDeleteValue(hkeyMsiHack, c_szTSMsiHackValue);
            RegCloseKey(hkeyMsiHack);
        }
    }

    _Lock();
    // The order below is important
    if (_hdsaCategoryList)
        _DestroyInternalCategoryList();

    if (_hdpaPub)
        _DestroyAppPublisherList();

    _Unlock();

    DeleteCriticalSection(&_cs);
    DllRelease();
}


// Recursively destroys a GUIDLIST
void CShellAppManager::_DestroyGuidList(GUIDLIST * pGuidList)
{
    ASSERT(IS_VALID_WRITE_PTR(pGuidList, GUIDLIST));
    if (pGuidList->pNextGuid)
        _DestroyGuidList(pGuidList->pNextGuid);

    LocalFree(pGuidList);
}

void CShellAppManager::_DestroyCategoryItem(CATEGORYITEM * pci)
{
    ASSERT(IS_VALID_WRITE_PTR(pci, CATEGORYITEM));
    if (pci->pszDescription)
        LocalFree(pci->pszDescription);
    if (pci->pGuidList)
        _DestroyGuidList(pci->pGuidList);
}

// Destroys our Category List Table
void CShellAppManager::_DestroyInternalCategoryList()
{
    // The caller must enter the lock first
    ASSERT(0 < _cRefLock);

    ASSERT(IS_VALID_HANDLE(_hdsaCategoryList, DSA));
    int idsa;
    for (idsa = 0; idsa < DSA_GetItemCount(_hdsaCategoryList); idsa++)
    {
        CATEGORYITEM * pci = (CATEGORYITEM *)DSA_GetItemPtr(_hdsaCategoryList, idsa);
        if (pci)
            _DestroyCategoryItem(pci);
    }    
    DSA_Destroy(_hdsaCategoryList);
}

// Destroys our list of IAppPublisher *
void CShellAppManager::_DestroyAppPublisherList()
{
    // The caller must enter the lock first
    ASSERT(0 < _cRefLock);
    ASSERT(IS_VALID_HANDLE(_hdpaPub, DPA));
    int idpa;
    for (idpa = 0; idpa < DPA_GetPtrCount(_hdpaPub); idpa++)
    {
        IAppPublisher * pap = (IAppPublisher *)DPA_GetPtr(_hdpaPub, idpa);
        if (EVAL(pap))
            pap->Release();
    }

    DPA_Destroy(_hdpaPub);
    _hdpaPub = NULL;
}

// IShellAppManager::QueryInterface
HRESULT CShellAppManager::QueryInterface(REFIID riid, LPVOID * ppvOut)
{
    static const QITAB qit[] = {
        QITABENT(CShellAppManager, IShellAppManager),                  // IID_IShellAppManager
        { 0 },
    };

    return QISearch(this, qit, riid, ppvOut);
}

// IShellAppManager::AddRef
ULONG CShellAppManager::AddRef()
{
    InterlockedIncrement(&_cRef);
    TraceAddRef(CShellAppManager, _cRef);
    return _cRef;
}

// IShellAppManager::Release
ULONG CShellAppManager::Release()
{
    InterlockedDecrement(&_cRef);
    TraceRelease(CShellAppManager, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

void CShellAppManager::_Lock(void)
{
    EnterCriticalSection(&_cs);
    DEBUG_CODE( _cRefLock++; )
}

void CShellAppManager::_Unlock(void)
{
    DEBUG_CODE( _cRefLock--; )
    LeaveCriticalSection(&_cs);
}


STDMETHODIMP CShellAppManager::GetNumberofInstalledApps(DWORD * pdwResult)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellAppManager::EnumInstalledApps(IEnumInstalledApps ** ppeia)
{
    HRESULT hres = E_FAIL;
    CEnumInstalledApps * peia = new CEnumInstalledApps();
    if (peia)
    {
        *ppeia = SAFECAST(peia, IEnumInstalledApps *);
        hres = S_OK;
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}


#ifndef DOWNLEVEL_PLATFORM
HRESULT CShellAppManager::_AddCategoryToList(APPCATEGORYINFO * pai, IAppPublisher * pap)
{
    // The caller must enter the lock first
    ASSERT(0 < _cRefLock);

    if (pai == NULL || _hdsaCategoryList == NULL)
        return E_FAIL;

    ASSERT(IS_VALID_CODE_PTR(pap, IAppPublisher));
    
    // Allocate a GUIDLIST item first
    GUIDLIST * pgl  = (GUIDLIST *)LocalAlloc(LPTR, SIZEOF(GUIDLIST));
    if (!pgl)
        return E_OUTOFMEMORY;
    
    pgl->CatGuid    = pai->AppCategoryId;
    pgl->papSupport = pap;

    // Search in the CategoryList
    int idsa;
    for (idsa = 0; idsa < DSA_GetItemCount(_hdsaCategoryList); idsa++)
    {
        CATEGORYITEM * pci = (CATEGORYITEM *)DSA_GetItemPtr(_hdsaCategoryList, idsa);

        if (pci)
        {
            // If we find an empty spot, fill it
            if (pci->pszDescription == NULL)
            {
                CATEGORYITEM ci = {0};
                ci.pszDescription = StrDupW(pai->pszDescription);
                ci.pGuidList    = pgl;

                pgl->pNextGuid  = NULL;

                if (DSA_InsertItem(_hdsaCategoryList, idsa, &ci) == -1)
                {
                    LocalFree(ci.pszDescription);
                    break;
                }
            }
            // If we find an entry with the same description text, add our GUID to the GuidList
            else if(!lstrcmpi(pai->pszDescription, pci->pszDescription))
            {
                pgl->pNextGuid  = pci->pGuidList;
                pci->pGuidList  = pgl;
                break;
            }
        }
        else
            ASSERT(0);
    }

    // We ran out of items in the list, and didn't run into an identical category string
    if (idsa == DSA_GetItemCount(_hdsaCategoryList))
    {
        CATEGORYITEM ci = {0};
        ci.pszDescription = StrDupW(pai->pszDescription);
        ci.pGuidList    = pgl;

        pgl->pNextGuid  = NULL;
        if (DSA_AppendItem(_hdsaCategoryList, &ci) == -1)
            LocalFree(ci.pszDescription);
    }
    
    return S_OK;
}
#endif //DOWNLEVEL_PLATFORM

#ifndef DOWNLEVEL_PLATFORM
HRESULT CShellAppManager::_BuildInternalCategoryList()
{
    HRESULT hres = E_OUTOFMEMORY;
    // The caller must enter the lock first
    ASSERT(0 < _cRefLock);

    ASSERT(IsValidHDPA(_hdpaPub));

    // We have just one valid version of _hdsaCategoryList, so we should never call this function
    // again once _hdsaCategoryList is created.
    ASSERT(_hdsaCategoryList == NULL);

    // Is the structure automatically filled with zero?
    _hdsaCategoryList =  DSA_Create(SIZEOF(CATEGORYITEM), CATEGORYLIST_GROW);

    if (_hdsaCategoryList)
    {
        int idpa;
        for (idpa = 0; idpa < DPA_GetPtrCount(_hdpaPub); idpa++)
        {
            IAppPublisher * pap = (IAppPublisher *)DPA_GetPtr(_hdpaPub, idpa);
            ASSERT(pap);

            if (pap)
            {
                UINT i;
                APPCATEGORYINFOLIST AppCatList;
                if (SUCCEEDED(pap->GetCategories(&AppCatList)))
                {
                    if ((AppCatList.cCategory > 0) && AppCatList.pCategoryInfo)
                    {
                        for (i = 0; i < AppCatList.cCategory; i++)
                            _AddCategoryToList(&AppCatList.pCategoryInfo[i], pap);

                        _DestroyCategoryList(&AppCatList);
                    }
                }
            }

            hres = S_OK;
        }
    }

    return hres;
}
#endif //DOWNLEVEL_PLATFORM

#ifndef DOWNLEVEL_PLATFORM
// Compile a multi string of categories and return it to the caller
HRESULT CShellAppManager::_CompileCategoryList(PSHELLAPPCATEGORYLIST psacl)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_READ_PTR(psacl, SHELLAPPCATEGORYLIST));
    
    // Don't do anything if we have an empty list
    if (_hdsaCategoryList && (DSA_GetItemCount(_hdsaCategoryList) > 0))
    {
        psacl->pCategory = (PSHELLAPPCATEGORY) SHAlloc(DSA_GetItemCount(_hdsaCategoryList) * SIZEOF(SHELLAPPCATEGORY));
        if (psacl->pCategory)
        {
            int idsa;
            for (idsa = 0; idsa < DSA_GetItemCount(_hdsaCategoryList); idsa++)
            {
                CATEGORYITEM * pci = (CATEGORYITEM *)DSA_GetItemPtr(_hdsaCategoryList, idsa);
                if (pci && pci->pszDescription)
                {
                    if (SUCCEEDED(SHStrDup(pci->pszDescription, &psacl->pCategory[idsa].pszCategory)))
                    {
                        ASSERT(IS_VALID_STRING_PTR(psacl->pCategory[idsa].pszCategory, -1));
                        psacl->cCategories++;
                    }
                    else
                        break;
                }
            }
            hres = S_OK;
        }
        else          
            hres = E_OUTOFMEMORY;
    }

    return hres;
}
#endif //DOWNLEVEL_PLATFORM

// IShellAppManager::GetPublishedAppCategories
STDMETHODIMP CShellAppManager::GetPublishedAppCategories(PSHELLAPPCATEGORYLIST psacl)
{
#ifndef DOWNLEVEL_PLATFORM
    HRESULT hres = E_INVALIDARG;
    if (psacl)
    {
        ASSERT(IS_VALID_READ_PTR(psacl, SHELLAPPCATEGORYLIST));
        ZeroMemory(psacl, SIZEOF(SHELLAPPCATEGORYLIST));

        // NOTE: keep the check inside the lock! So that only one thread
        // is allowed in.
        
        _Lock();
        // Do we have any app publishers in our list at all
        if (_hdpaPub)
        {
            if (_hdsaCategoryList == NULL)
                _BuildInternalCategoryList();

            hres = _CompileCategoryList(psacl);
        }
        _Unlock();
    }
#else
    HRESULT hres = E_NOTIMPL;
#endif //DOWNLEVEL_PLATFORM
    return hres;
}

#ifndef DOWNLEVEL_PLATFORM
GUIDLIST * CShellAppManager::_FindGuidListForCategory(LPCWSTR pszDescription)
{
    // The caller must enter the lock first
    ASSERT(0 < _cRefLock);

    if (_hdsaCategoryList)
    {
        int idsa;
        for (idsa = 0; idsa < DSA_GetItemCount(_hdsaCategoryList); idsa++)
        {
            CATEGORYITEM * pci = (CATEGORYITEM *)DSA_GetItemPtr(_hdsaCategoryList, idsa);
            if (pci && pci->pszDescription && !lstrcmpi(pszDescription, pci->pszDescription))
                return pci->pGuidList;
        }
    }
    return NULL;
}
#endif //DOWNLEVEL_PLATFORM

extern void _DestroyHdpaEnum(HDPA hdpaEnum);

// IShellAppManager::EnumPublishedApps
STDMETHODIMP CShellAppManager::EnumPublishedApps(LPCWSTR pszCategory, IEnumPublishedApps ** ppepa)
{
#ifndef DOWNLEVEL_PLATFORM
    HRESULT hres = E_OUTOFMEMORY;

    ASSERT(pszCategory == NULL || IS_VALID_STRING_PTRW(pszCategory, -1));

    // hdpaEnum is the list of IEnumPublishedApp * we pass to the constructor of CShellEnumPublishedApps
    HDPA hdpaEnum = DPA_Create(4);

    if (hdpaEnum)
    {
        // If no category is required, we enumerate all 
        if (pszCategory == NULL)
        {
            _Lock();
            if (_hdpaPub)
            {
                int idpa;
                for (idpa = 0; idpa < DPA_GetPtrCount(_hdpaPub); idpa++)
                {
                    IAppPublisher * pap = (IAppPublisher *)DPA_GetPtr(_hdpaPub, idpa);
                    IEnumPublishedApps * pepa; 
                    if (pap && SUCCEEDED(pap->EnumApps(NULL, &pepa)))
                    {
                        ASSERT(IS_VALID_CODE_PTR(pepa, IEnumPublishedApps));
                        if (DPA_AppendPtr(hdpaEnum, pepa) == DPA_ERR)
                        {
                            pepa->Release();
                            break;
                        }
                    }
                }
            }
            _Unlock();
        }
        else
        {
            _Lock();
            // If there is no Category list, let's build one
            if (_hdsaCategoryList == NULL)
                _BuildInternalCategoryList();

            // Otherwise we find the GuidList and enumerate all the guys in the list
            GUIDLIST * pgl = _FindGuidListForCategory(pszCategory);
            
            while (pgl && pgl->papSupport)
            {
                ASSERT(IS_VALID_READ_PTR(pgl, GUIDLIST) && IS_VALID_CODE_PTR(pgl->papSupport, IAppPulisher));
                IEnumPublishedApps * pepa; 
                if (SUCCEEDED(pgl->papSupport->EnumApps(&pgl->CatGuid, &pepa)))
                {
                    ASSERT(IS_VALID_CODE_PTR(pepa, IEnumPublishedApps));
                    if (DPA_AppendPtr(hdpaEnum, pepa) == DPA_ERR)
                    {
                        pepa->Release();
                        break;
                    }
                }
                pgl = pgl->pNextGuid;
            }

            _Unlock();
        }
                
    }

    // Even if we have no enumerators we return success and pass back an empty enumerator
        
    CShellEnumPublishedApps * psepa = new CShellEnumPublishedApps(hdpaEnum);
    if (psepa)
    {
        *ppepa = SAFECAST(psepa, IEnumPublishedApps *);
        hres = S_OK;
    }
    else
    {
        hres = E_OUTOFMEMORY;
        if (hdpaEnum)
            _DestroyHdpaEnum(hdpaEnum);
    }
#else
    HRESULT hres = E_NOTIMPL;
#endif //DOWNLEVEL_PLATFORM
    return hres;
}

EXTERN_C STDAPI InstallAppFromFloppyOrCDROM(HWND hwnd);
STDMETHODIMP CShellAppManager::InstallFromFloppyOrCDROM(HWND hwndParent)
{
    return InstallAppFromFloppyOrCDROM(hwndParent);
}



/*----------------------------------------------------------
Purpose: Create-instance function for class factory

*/
STDAPI CShellAppManager_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    HRESULT hres = E_OUTOFMEMORY;
    CShellAppManager* pObj = new CShellAppManager();
    if (pObj)
    {
        *ppunk = SAFECAST(pObj, IShellAppManager *);
        hres = S_OK;
    }

    return hres;
}
