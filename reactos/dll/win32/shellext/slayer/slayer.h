#ifndef __SLAYER_H
#define __SLAYER_H

extern HINSTANCE hInstance;

typedef struct _CITEM
{
  struct _CITEM *next;
  TCHAR szName[256];
  TCHAR szKeyName[256];
  DWORD MajorVersion;
  DWORD MinorVersion;
  DWORD BuildNumber;
  DWORD PlatformId;
  DWORD SPMajorVersion;
  DWORD SPMinorVersion;
} CITEM, *PCITEM;

/******************************************************************************
   ICompatibilityPage
 ******************************************************************************/

static const GUID CLSID_ICompatibilityPage = {0x513D916F,0x2A8E,0x4F51,{0xAE,0xAB,0x0C,0xBC,0x76,0xFB,0x1A,0xF9}}; /* F8 on XP! */

typedef struct ICompatibilityPage *LPCOMPATIBILITYPAGE;

/* IShellPropSheetExt */
typedef struct ifaceIShellPropSheetExtVbtl ifaceIShellPropSheetExtVbtl;
struct ifaceIShellPropSheetExtVbtl
{
  HRESULT (STDMETHODCALLTYPE *AddPages)(LPCOMPATIBILITYPAGE this,
                                        LPFNADDPROPSHEETPAGE lpfnAddPage,
                                        LPARAM lParam);
  HRESULT (STDMETHODCALLTYPE *ReplacePage)(LPCOMPATIBILITYPAGE this,
                                           UINT uPageID,
                                           LPFNADDPROPSHEETPAGE lpfnReplacePage,
                                           LPARAM lParam);
};

/* IShellExtInit */
typedef struct ifaceIShellExtInitVbtl ifaceIShellExtInitVbtl;
struct ifaceIShellExtInitVbtl
{
  HRESULT (STDMETHODCALLTYPE *Initialize)(LPCOMPATIBILITYPAGE this,
                                          LPCITEMIDLIST pidlFolder,
                                          IDataObject *pdtobj,
                                          HKEY hkeyProgID);
};

/* IClassFactory */
typedef struct ifaceIClassFactoryVbtl ifaceIClassFactoryVbtl;
struct ifaceIClassFactoryVbtl
{
  HRESULT (STDMETHODCALLTYPE *CreateInstance)(LPCOMPATIBILITYPAGE this,
                                              LPUNKNOWN pUnkOuter,
                                              REFIID riid,
                                              PVOID *ppvObject);
  HRESULT (STDMETHODCALLTYPE *LockServer)(LPCOMPATIBILITYPAGE this,
                                          BOOL fLock);
};

/* ICompatibilityPage */
typedef struct ifaceICompatibilityPageVbtl ifaceICompatibilityPageVbtl;
struct ifaceICompatibilityPageVbtl
{
  /* IUnknown */
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(LPCOMPATIBILITYPAGE this,
                                              REFIID iid,
					      PVOID *pvObject);
  ULONG (STDMETHODCALLTYPE *AddRef)(LPCOMPATIBILITYPAGE this);
  ULONG (STDMETHODCALLTYPE *Release)(LPCOMPATIBILITYPAGE this);

  union
  {
    ifaceIShellPropSheetExtVbtl IShellPropSheetExt;
    ifaceIShellExtInitVbtl IShellExtInit;
    ifaceIClassFactoryVbtl IClassFactory;
  } fn;
};

typedef struct ICompatibilityPage
{
  /* IUnknown fields */
  ifaceICompatibilityPageVbtl* lpVtbl;
  LONG ref;
  /* ICompatibilityPage fields */
  TCHAR szFile[MAX_PATH + 1];
  BOOL Changed;

  PCITEM CItems;
  PCITEM CSelectedItem;
  UINT nItems;
} COMPATIBILITYPAGE;

/* IUnknown */
HRESULT STDMETHODCALLTYPE ICompatibilityPage_fnQueryInterface(LPCOMPATIBILITYPAGE this,
                                                              REFIID iid,
                                                              PVOID *pvObject);
ULONG STDMETHODCALLTYPE ICompatibilityPage_fnAddRef(LPCOMPATIBILITYPAGE this);
ULONG STDMETHODCALLTYPE ICompatibilityPage_fnRelease(LPCOMPATIBILITYPAGE this);

/* IShellPropSheetExt */
HRESULT STDMETHODCALLTYPE ICompatibilityPage_fnAddPages(LPCOMPATIBILITYPAGE this,
                                                        LPFNADDPROPSHEETPAGE lpfnAddPage,
                                                        LPARAM lParam);
HRESULT STDMETHODCALLTYPE ICompatibilityPage_fnReplacePage(LPCOMPATIBILITYPAGE this,
                                                           UINT uPageID,
                                                           LPFNADDPROPSHEETPAGE lpfnReplacePage,
                                                           LPARAM lParam);
/* IShellExtInit */
HRESULT STDMETHODCALLTYPE ICompatibilityPage_fnInitialize(LPCOMPATIBILITYPAGE this,
                                                          LPCITEMIDLIST pidlFolder,
                                                          IDataObject *pdtobj,
                                                          HKEY hkeyProgID);
/* IClassFactory */
HRESULT STDMETHODCALLTYPE ICompatibilityPage_fnCreateInstance(LPCOMPATIBILITYPAGE this,
                                                              LPUNKNOWN pUnkOuter,
                                                              REFIID riid,
                                                              PVOID *ppvObject);
HRESULT STDMETHODCALLTYPE ICompatibilityPage_fnLockServer(LPCOMPATIBILITYPAGE this,
                                                          BOOL fLock);


#endif /* __SLAYER_H */

/* EOF */
