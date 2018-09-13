//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       update.h
//
//--------------------------------------------------------------------------

#ifndef _UPDATE_H_
#define _UPDATE_H_

#include <mobsync.h>
#include "cscentry.h"
#include "util.h"       // ENUM_REASON


//
// Flags used in CscUpdateCache
//
#define CSC_UPDATE_STARTNOW       0x00000002  // Don't wait for user confirmation to start update
#define CSC_UPDATE_SELECTION      0x00000004  // Update current selection (CSC_NAMELIST_HDR buffer)
#define CSC_UPDATE_PINFILES       0x00000008  // Pin files while updating them
#define CSC_UPDATE_PIN_RECURSE    0x00000010  // Recurse into subfolders when pinning
#define CSC_UPDATE_REINT          0x00000020  // Perform outward reintegration
#define CSC_UPDATE_FILL_QUICK     0x00000040  // Perform quick inward sync (fill sparse files)
#define CSC_UPDATE_FILL_ALL       0x00000080  // Perform full inward sync (overrides CSC_UPDATE_FILL_QUICK)
#define CSC_UPDATE_NOTIFY_DONE    0x00000100  // Send CSCWM_DONESYNCING to notify window when done.
#define CSC_UPDATE_SHOWUI_ALWAYS  0x00000200  // Nothing to sync but show SyncMgr UI anyway.
#define CSC_UPDATE_IGNORE_ACCESS  0x00000400  // Default is to sync files with USER and/or GUEST access.
#define CSC_UPDATE_RECONNECT      0x00000800  // Transition all servers online after syncing

HRESULT CscRegisterHandler(BOOL bRegister=TRUE, LPUNKNOWN punkSyncMgr=NULL);
HRESULT CscUpdateCache(DWORD dwUpdateFlags, CscFilenameList *pfnl=NULL);

void BuildSilentFolderList(CscFilenameList *pfnlSilentFolders,
                           CscFilenameList *pfnlSpecialFolders);


class CCscUpdate;
typedef CCscUpdate *PCSCUPDATE;

typedef struct
{
    PCSCUPDATE      pThis;
    SYNCMGRITEMID   ItemID;
    HANDLE          hThread;
    LPTSTR          pszShareName;
    TCHAR           szDrive[4];
    DWORD           dwSyncStatus;
    LONG            cFilesToSync;
    LONG            cFilesDone;
    CscFilenameList *pUndoExclusionList;
    DWORD           dwCscContext;
} SYNCTHREADDATA, *PSYNCTHREADDATA;


class CCscUpdate : ISyncMgrSynchronize
{
private:
    LONG                            m_cRef;
    CscFilenameList                *m_pFileList;
    DWORD                           m_dwSyncFlags;
    HDSA                            m_hSyncItems;
    CSCEntryLog                     m_ShareLog;
    LPSYNCMGRSYNCHRONIZECALLBACK    m_pSyncMgrCB;
    HDPA                            m_hSyncThreads;
    CRITICAL_SECTION                m_csThreadList;
    HWND                            m_hwndDlgParent;
    HANDLE                          m_hSyncMutex;
    HANDLE                          m_hSyncInProgMutex;
    CscFilenameList                *m_pConflictPinList;
    CscFilenameList                *m_pSilentFolderList;
    CscFilenameList                *m_pSpecialFolderList;
    CscFilenameList                 m_ReconnectList;

public:
    CCscUpdate();
    ~CCscUpdate();

    static HRESULT WINAPI CreateInstance(REFIID riid, LPVOID *ppv);

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ISyncMgrSynchronize methods
    STDMETHODIMP Initialize(DWORD dwReserved,
                            DWORD dwSyncFlags,
                            DWORD cbCookie,
                            const BYTE *lpCookie);
    STDMETHODIMP GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
    STDMETHODIMP EnumSyncMgrItems(LPSYNCMGRENUMITEMS *ppenum);
    STDMETHODIMP GetItemObject(REFSYNCMGRITEMID rItemID, REFIID riid, LPVOID *ppv);
    STDMETHODIMP ShowProperties(HWND hWndParent, REFSYNCMGRITEMID rItemID);
    STDMETHODIMP SetProgressCallback(LPSYNCMGRSYNCHRONIZECALLBACK pCallback);
    STDMETHODIMP PrepareForSync(ULONG cbNumItems,
                                SYNCMGRITEMID *pItemIDs,
                                HWND hWndParent,
                                DWORD dwReserved);
    STDMETHODIMP Synchronize(HWND hWndParent);
    STDMETHODIMP SetItemStatus(REFSYNCMGRITEMID pItemID,
                               DWORD dwSyncMgrStatus);
    STDMETHODIMP ShowError(HWND hWndParent,
                            REFSYNCMGRERRORID ErrorID);
private:
    HRESULT LogError(REFSYNCMGRITEMID rItemID,
                     LPCTSTR pszText,
                     DWORD dwLogLevel = SYNCMGRLOGLEVEL_ERROR,
                     REFSYNCMGRERRORID ErrorID = GUID_NULL);
    DWORD LogError(REFSYNCMGRITEMID rItemID,
                   DWORD dwLogLevel,
                   UINT nFormatID,
                   ...);
    DWORD LogError(REFSYNCMGRITEMID rItemID,
                   UINT nFormatID,
                   LPCTSTR pszName,
                   DWORD dwErr,
                   DWORD dwLogLevel = SYNCMGRLOGLEVEL_ERROR);
    HRESULT SynchronizeShare(SYNCMGRITEMID *pItemID,
                             LPCTSTR pszShareName,
                             BOOL bRasConnected);
    void  SetLastSyncTime(LPCTSTR pszShareName);
    DWORD GetLastSyncTime(LPCTSTR pszShareName, LPFILETIME pft);
    void  SyncThreadCompleted(PSYNCTHREADDATA pSyncData);
    void  SyncCompleted(void);
    DWORD CopyLocalFileWithDriveMapping(LPCTSTR pszSrc,
                                        LPCTSTR pszDst,
                                        LPCTSTR pszShare,
                                        LPCTSTR pszDrive,
                                        BOOL    bDirectory = FALSE);
    DWORD HandleFileConflict(PSYNCTHREADDATA    pSyncData,
                             LPCTSTR            pszName,
                             DWORD              dwStatus,
                             DWORD              dwHintFlags,
                             LPWIN32_FIND_DATA  pFind32);
    DWORD HandleDeleteConflict(PSYNCTHREADDATA   pSyncData,
                               LPCTSTR           pszName,
                               DWORD             dwStatus,
                               DWORD             dwHintFlags,
                               LPWIN32_FIND_DATA pFind32);
    DWORD CscCallback(PSYNCTHREADDATA     pSyncData,
                      LPCTSTR             pszName,
                      DWORD               dwStatus,
                      DWORD               dwHintFlags,
                      DWORD               dwPinCount,
                      LPWIN32_FIND_DATA   pFind32,
                      DWORD               dwReason,
                      DWORD               dwParam1,
                      DWORD               dwParam2);

    static void NotifySyncMgr(PSYNCTHREADDATA pSyncData,
                              LPSYNCMGRPROGRESSITEM pspi);
    static DWORD WINAPI _CscCallback(LPCTSTR             pszName,
                                     DWORD               dwStatus,
                                     DWORD               dwHintFlags,
                                     DWORD               dwPinCount,
                                     LPWIN32_FIND_DATA   pFind32,
                                     DWORD               dwReason,
                                     DWORD               dwParam1,
                                     DWORD               dwParam2,
                                     DWORD_PTR           dwContext);

    BOOL PinLinkTarget(LPCTSTR pszName, PSYNCTHREADDATA pSyncData);
    static DWORD WINAPI _PinNewFilesW32Callback(LPCTSTR             pszName,
                                                ENUM_REASON         eReason,
                                                LPWIN32_FIND_DATA   pFind32,
                                                LPARAM              lpContext);
    static DWORD WINAPI _PinNewFilesCSCCallback(LPCTSTR             pszName,
                                                ENUM_REASON         eReason,
                                                DWORD               dwStatus,
                                                DWORD               dwHintFlags,
                                                DWORD               dwPinCount,
                                                LPWIN32_FIND_DATA   pFind32,
                                                LPARAM              lpContext);
    static DWORD WINAPI _SyncThread(LPVOID pThreadData);

    DWORD MergeShare(PSYNCTHREADDATA pSyncData);
    DWORD FillShare(PSYNCTHREADDATA pSyncData, int cPinned, DWORD dwConnectionSpeed);

    void PinFiles(PSYNCTHREADDATA pSyncData, BOOL bConflictPinList=FALSE);
    void NotifyUndo(PSYNCTHREADDATA pSyncData, LPCTSTR pszName);
    void UndoPinFiles(PSYNCTHREADDATA pSyncData);
    static DWORD WINAPI _UndoProgress(LPCTSTR pszItem, LPARAM lpContext);

    BOOL SkipEFSPin(PSYNCTHREADDATA pSyncData, LPCTSTR pszItem);

    typedef enum
    {
        SyncStop = 0,
        SyncPause,
        SyncResume
    } eSetSyncStatus;

    HRESULT SetSyncThreadStatus(eSetSyncStatus status, REFGUID rItemID);

    void GetSilentFolderList(void);
    BOOL IsSilentFolder(LPCTSTR pszName)
    { return (m_pSilentFolderList && m_pSilentFolderList->FileExists(pszName, false)); }
    BOOL IsSilentShare(LPCTSTR pszShare)
    { return (m_pSilentFolderList && m_pSilentFolderList->ShareExists(pszShare)); }
    BOOL IsSpecialFolder(LPCTSTR pszName)
    { return ((m_pSpecialFolderList && m_pSpecialFolderList->FileExists(pszName, false)) || IsSilentFolder(pszName)); }
    BOOL IsSpecialFolderShare(LPCTSTR pszShare)
    { return ((m_pSpecialFolderList && m_pSpecialFolderList->ShareExists(pszShare)) || IsSilentShare(pszShare)); }

    friend class CUpdateEnumerator;
};

class CUpdateEnumerator : ISyncMgrEnumItems
{
private:
    LONG                        m_cRef;
    PCSCUPDATE                  m_pUpdate;
    HANDLE                      m_hFind;
    BOOL                        m_bEnumFileSelection;
    INT                         m_cCheckedItemsEnumerated;
    CscFilenameList::ShareIter  m_SelectionIterator;

public:
    CUpdateEnumerator(PCSCUPDATE pUpdate);
    ~CUpdateEnumerator();

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ISyncMgrEnumItems methods
    STDMETHODIMP Next(ULONG celt, LPSYNCMGRITEM rgelt, PULONG pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(LPSYNCMGRENUMITEMS *ppenum);
};
typedef CUpdateEnumerator *PUPDATEENUM;

#endif  // _UPDATE_H_
