//
// OneStop Offline Synch handler
//

#ifndef _OFFSYNC_H
#define _OFFSYNC_H

class CThrottler;

class COfflineSync : public ISyncMgrSynchronize,
                     public ISubscriptionAgentEvents
{
private:
    ~COfflineSync();
    ULONG           m_cRef;

    ISyncMgrSynchronizeCallback *m_pSyncCallback;
    ISubscriptionMgr2           *m_pSubsMgr2;

    DWORD               m_dwSyncFlags;
    HWND                m_hWndParent;
    CThrottler          *m_pThrottler;
    HRESULT             m_hrResult;

    ULONG               m_nItemsToRun;
    ULONG               m_nItemsCompleted;

    BOOL                m_fCookiesSpecified;
    
    SUBSCRIPTIONCOOKIE  *m_pItems;

    void Cleanup();
    BOOL AreWeDoneYet();
    HRESULT GetSubsMgr2();
    int FindCookie(const SUBSCRIPTIONCOOKIE *pCookie);
    HRESULT DupItems(ULONG cbNumItems, SUBSCRIPTIONCOOKIE *pItemIDs);

    HRESULT CallSyncMgrProgress(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
        const WCHAR *lpcStatusText, DWORD dwStatusType, INT iProgValue, INT iMaxValue);

public:
    COfflineSync();

    HWND GetParentWindow() { return m_hWndParent; }

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ISyncMgrSynchronize members
    STDMETHODIMP Initialize( 
        /* [in] */ DWORD dwReserved,
        /* [in] */ DWORD dwSyncMgrFlags,
        /* [in] */ DWORD cbCookie,
        /* [in] */ const BYTE *lpCookie);
    
    STDMETHODIMP GetHandlerInfo( 
        /* [out] */ LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
    
    STDMETHODIMP EnumSyncMgrItems( 
        /* [out] */ ISyncMgrEnumItems **ppSyncMgrEnumItems);
    
    STDMETHODIMP GetItemObject( 
        /* [in] */ REFSYNCMGRITEMID ItemID,
        /* [in] */ REFIID riid,
        /* [out] */ void **ppv);
    
    STDMETHODIMP ShowProperties( 
        /* [in] */ HWND hWndParent,
        /* [in] */ REFSYNCMGRITEMID ItemID);
    
    STDMETHODIMP SetProgressCallback( 
        /* [in] */ ISyncMgrSynchronizeCallback *lpCallBack);
    
    STDMETHODIMP PrepareForSync( 
        /* [in] */ ULONG cbNumItems,
        /* [in] */ SYNCMGRITEMID *pItemIDs,
        /* [in] */ HWND hWndParent,
        /* [in] */ DWORD dwReserved);
    
    STDMETHODIMP Synchronize( 
        /* [in] */ HWND hWndParent);
    
    STDMETHODIMP SetItemStatus( 
        /* [in] */ REFSYNCMGRITEMID ItemID,
        /* [in] */ DWORD dwSyncMgrStatus);
    
    STDMETHODIMP ShowError( 
        /* [in]  */ HWND hWndParent,
        /* [in]  */ REFSYNCMGRERRORID ErrorID);

    // ISubscriptionAgentEvents members
    STDMETHODIMP UpdateBegin(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie);

    STDMETHODIMP UpdateProgress(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
        long lSizeDownloaded,
        long lProgressCurrent,
        long lProgressMax,
        HRESULT hrStatus,
        LPCWSTR wszStatus);

    STDMETHODIMP UpdateEnd(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
        long lSizeDownloaded,
        HRESULT hrResult,
        LPCWSTR wszResult);

    STDMETHODIMP ReportError(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
        HRESULT hrError, 
        LPCWSTR wszError);

    HRESULT UpdateSyncMgrStatus(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
        LPCWSTR wszStatusMsg, DWORD dwStatus);

};

class COfflineEnum : public ISyncMgrEnumItems
{
private:
    ~COfflineEnum();
    ULONG           m_cRef;

    SYNCMGRITEM    *m_pItems;
    ULONG           m_iNumItems;
    ULONG           m_iEnumPtr;

    HRESULT LoadItem(ISubscriptionMgr2 *pSubsMgr2, 
        const SUBSCRIPTIONCOOKIE *pCookie, SYNCMGRITEM *pItem, DWORD dwItemState);

public:
    COfflineEnum();

    HRESULT Init(ISubscriptionMgr2 *pSubsMgr2, ULONG nItems, 
            SUBSCRIPTIONCOOKIE *pInitCookies, ISyncMgrEnumItems **ppenum, DWORD dwSyncFlags);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumOfflineItems members
    STDMETHODIMP Next( 
        /* [in] */ ULONG celt,
        /* [length_is][size_is][out] */ LPSYNCMGRITEM rgelt,
        /* [out] */ ULONG *pceltFetched);
    
    STDMETHODIMP Skip( 
        /* [in] */ ULONG celt);
    
    STDMETHODIMP Reset( void);
    
    STDMETHODIMP Clone( 
        /* [out] */ ISyncMgrEnumItems **ppenum);
};

#include "throttle.h"

#endif


