//+-----------------------------------------------------------------------
//
//  Matrix Array
//
//------------------------------------------------------------------------


#ifndef _MTXARRAY_H_
#define _MTXARRAY_H_

#include "iface.h"          // for APPCMD_*
#include "worker.h"
#include "setupenum.h"      // for COCSetup* classes
#include "simpdata.h"       // for OLEDB

// Areas that this control can enumerate
// Warning: Please do not change these numbers, they are also used
// as array indexes. 
#define ENUM_INSTALLED  0       // installed apps
#define ENUM_PUBLISHED  1       // published apps
#define ENUM_CATEGORIES 2       // published categories
#define ENUM_OCSETUP    3       // Optional components selected during setup
#define ENUM_UNKNOWN    0xffffffff

// Sort indexes
#define SORT_NA         (-1)    // n/a
#define SORT_NAME       0       // sort by name
#define SORT_SIZE       1       // sort by size
#define SORT_TIMESUSED  2       // sort by frequency of use
#define SORT_LASTUSED   3       // sort by last used date


typedef struct tagAPPFIELDS
{
    LPWSTR pszLabel;
    DWORD  dwSort;          // can be SORT_*
    DWORD  dwStruct;        // IFS_*
    VARTYPE vt;             // field type
    DWORD  ibOffset;        // offset into structure designated by dwStruct
} APPFIELDS;


// BUGBUG: (dli) We should have just one base CAppData class and installed, published,
// categories and OCSetup should derives from it. 
// CAppData

class CAppData : public IAppData
{
public:
    // *** IUnknown ***
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);
    STDMETHOD(QueryInterface)   (REFIID riid, LPVOID * ppvObj);

    // *** IAppData ***
    STDMETHOD(DoCommand)        (HWND hwndParent, APPCMD appcmd);
    STDMETHOD(ReadSlowData)     (void);
    STDMETHOD(GetVariant)       (DB_LORDINAL iField, VARIANT * pvar);
    STDMETHOD(SetMtxParent)     (IMtxArray * pmtxParent);
    STDMETHOD_(APPINFODATA *, GetDataPtr)(void);
    STDMETHOD_(SLOWAPPINFO *, GetSlowDataPtr)(void);
    STDMETHOD(GetFrequencyOfUse)(LPWSTR pszBuf, int cchBuf);
    STDMETHOD(SetNameDupe)      (BOOL bDupe) {_bNameDupe = bDupe; return S_OK;};
            
    // Overloaded constructors
    CAppData(IInstalledApp* pia, APPINFODATA* paid, PSLOWAPPINFO psai);
    CAppData(IPublishedApp* ppa, APPINFODATA* paid, PUBAPPINFO * ppai);
    CAppData(SHELLAPPCATEGORY * psac);
    CAppData(COCSetupApp * pocsa, APPINFODATA* paid);

    ~CAppData();

    
private:
    HRESULT _GetInstField(DB_LORDINAL iField, VARIANT * pvar);
    HRESULT _GetPubField(DB_LORDINAL iField, VARIANT * pvar);
    HRESULT _GetSetupField(DB_LORDINAL iField, VARIANT * pvar);
    HRESULT _GetCatField(DB_LORDINAL iField, VARIANT * pvar);

    LPTSTR _BuildSupportInfo(void);
    LPTSTR _BuildPropertiesHTML(void);
    void   _GetIconHTML(LPTSTR pszIconHTML, UINT cch);
                        
    HRESULT _VariantFromData(const APPFIELDS * pfield, LPVOID pvData, VARIANT * pvar);
    DWORD   _GetCapability();
    HRESULT _GetDiskSize(LPTSTR pszBuf, int cchBuf);
    HRESULT _GetLastUsed(LPTSTR pszBuf, int cchBuf);
    DWORD   _GetSortIndex(void);

    void    _MassageSlowAppInfo(void);
    
    LONG _cRef;
    union
    {
        IInstalledApp* _pia;
        IPublishedApp* _ppa;
        SHELLAPPCATEGORY * _psac;
        COCSetupApp * _pocsa;
    };
    APPINFODATA _ai;
    PUBAPPINFO _pai;        // Published app specific info
    SLOWAPPINFO _aiSlow;
    IMtxArray * _pmtxParent;
    DWORD _dwEnum;          // Can be ENUM_*
    BOOL  _bNameDupe;       // Has a duplicate name with other apps
    CRITICAL_SECTION _cs;   // NOTE: only used for Installed Apps. 
    BOOL  _fCsInitialized;  // Was the critical section initialized 
};


// EnumAppItems callback
typedef void (CALLBACK *ENUMAPPITEMSCB)(IAppData * pappdata, LPARAM lParam);

HRESULT EnumAppItems(DWORD dwEnum, LPCWSTR pszCategory, IShellAppManager * pam, ENUMAPPITEMSCB pfnCallback, LPARAM lpContext);



//------------------------------------------------------------------------
//
//  Matrix Array
//
//  This object maintains the array of data.  It knows how to map
//  ordinal row/column references to the actual records and fields
//  kept in the array.  The base class supplies all the necessary
//  methods as virtuals, and derived classes allow for arrays of:
//
//   - installed apps
//   - published apps
//   - publishing categories
//
//------------------------------------------------------------------------

class CMtxArray2 : public IMtxArray,
                   public CWorkerThread
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef   (void) {return CWorkerThread::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release  (void) {return CWorkerThread::Release();};
    virtual STDMETHODIMP QueryInterface   (REFIID riid, LPVOID * ppvObj);
    
    // *** IMtxArray ***
    STDMETHOD(Initialize)       (DWORD dwEnum);
    STDMETHOD(AddItem)          (IAppData * pappdata, DBROWCOUNT * piRow);
    STDMETHOD(DeleteItems)      (DBROWCOUNT iRow, DBROWCOUNT cRows);
    STDMETHOD(GetAppData)       (DBROWCOUNT iRow, IAppData ** ppappdata);
    STDMETHOD(GetItemCount)     (DBROWCOUNT * pcItems);
    STDMETHOD(GetFieldCount)    (DB_LORDINAL * pcFields);
    STDMETHOD(GetFieldName)     (DB_LORDINAL iField, VARIANT * pvar);
    STDMETHOD(GetSortIndex)     (DWORD * pdwSort);
    STDMETHOD(SortItems)        (void);
    STDMETHOD(SetSortCriteria)  (LPCWSTR pszSortField);
    STDMETHOD_(int,CompareItems)(IAppData * pappdata1, IAppData * pappdata2);
    STDMETHOD(MarkDupEntries)   (void);
    
    // *** IARPWorker methods *** (override)
    STDMETHOD(KillWT)           (void);

    CMtxArray2();
    ~CMtxArray2();

    static int s_SortItemsCallbackWrapper(LPVOID pv1, LPVOID pv2, LPARAM lParam);
    
protected:
    HRESULT _DeleteItem(DBROWCOUNT idpa);
    HRESULT _CreateArray(void);
    DWORD   _ThreadStartProc();

    void    _Lock(void);
    void    _Unlock(void);

    CRITICAL_SECTION _cs;
    DEBUG_CODE( LONG _cRefLock; )

            

    HDPA        _hdpa;          // the data is here
    DWORD       _dwEnum;
    DWORD       _dwSort;        // can be SORT_*
};


HRESULT CMtxArray_CreateInstance(REFIID riid, LPVOID * ppvObj);


#endif // _MTXARRAY_H_
