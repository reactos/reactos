#ifndef __APPMGR_H_
#define __APPMGR_H_

#define REGSTR_PATH_APPPUBLISHER    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Management\\Publishers")

// Structure to contain the list of GUIDs for a category
typedef struct _GuidList
{
    GUID CatGuid;
    IAppPublisher * papSupport;
    struct _GuidList * pNextGuid;
} GUIDLIST;

/////////////////////////////////////////////////////////////////////////////
// CShellAppManager
class CShellAppManager : public IShellAppManager
{
public:
    CShellAppManager();
    
    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellAppManager
    STDMETHODIMP GetNumberofInstalledApps(DWORD * pdwResult);
    STDMETHODIMP EnumInstalledApps(IEnumInstalledApps ** ppeia);
    STDMETHODIMP GetPublishedAppCategories(PSHELLAPPCATEGORYLIST psacl);
    STDMETHODIMP EnumPublishedApps(LPCWSTR pszCategory, IEnumPublishedApps ** ppepa);
    STDMETHODIMP InstallFromFloppyOrCDROM(HWND hwndParent);

protected:

    virtual ~CShellAppManager();

    LONG _cRef;

    // App Publisher List
    HDPA _hdpaPub;

    // Item of the Internal Category List 
#define CATEGORYLIST_GROW 16
    typedef struct _CategoryItem
    {
        LPWSTR pszDescription;
        GUIDLIST * pGuidList;
    } CATEGORYITEM;

    // Category List
    HDSA _hdsaCategoryList;

    void    _Lock(void);
    void    _Unlock(void);
    
    CRITICAL_SECTION _cs;
    DEBUG_CODE( LONG _cRefLock; )

    // Internal structure funcitons
    void       _DestroyGuidList(GUIDLIST * pGuidList);
    
#ifndef DOWNLEVEL_PLATFORM
    HRESULT    _AddCategoryToList(APPCATEGORYINFO * pai, IAppPublisher * pap);
    HRESULT    _BuildInternalCategoryList(void);
    HRESULT    _CompileCategoryList(PSHELLAPPCATEGORYLIST pascl);
#endif //DOWNLEVEL_PLATFORM

    void       _DestroyCategoryItem(CATEGORYITEM * pci);
    void       _DestroyInternalCategoryList(void);
    void       _DestroyAppPublisherList(void);

#ifndef DOWNLEVEL_PLATFORM
    GUIDLIST * _FindGuidListForCategory(LPCWSTR pszDescription);
#endif //DOWNLEVEL_PLATFORM

    BOOL       _bCreatedTSMsiHack; // The "EnableAdminRemote" value for MSI to work on TS
};

#endif //__APPMGR_H_
