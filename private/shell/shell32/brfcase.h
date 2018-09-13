#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// This structure is for the cache of brfinfohdr's
typedef struct _BrfExpensiveList
    {
    HWND                hwndMain;
    LPSHELLFOLDER       psf;
    LPBRIEFCASESTG      pbrfstg;
    HDPA                hdpa;           // Cache of expensive data
    int                 idpaStaleCur;
    int                 idpaUndeterminedCur;
    int                 idpaDeletedCur;
    HANDLE              hSemPending;    // Pending semaphore
    CRITICAL_SECTION    cs;
    HANDLE              hEventDie;
    HANDLE              hThreadPaint;
    HANDLE              hMutexDelay;    // Not owned by BrfExp
    BOOL                bFreePending;
#ifdef DEBUG
    UINT                cUndetermined;
    UINT                cStale;
    UINT                cDeleted;
    UINT                cCSRef;
#endif

    } BRFEXP, * PBRFEXP;


typedef struct _BrfView
    {
    LPBRIEFCASESTG      pbrfstg;
    LPITEMIDLIST        pidlRoot;       // Root of briefcase
    LPCITEMIDLIST       pidl;
    PBRFEXP             pbrfexp;
    HANDLE              hMutexDelay;
    ULONG               uSCNRExtra;     // Extra SHChangeNotifyRegister for our pidl...
    TCHAR               szDBName[MAX_PATH];

    } BrfView, * PBRFVIEW;


void BrfView_OnCreate(
    PBRFVIEW that,
    HWND hwndMain,
    HWND hwndView);
void BrfView_OnDestroy(
    PBRFVIEW that,
    HWND hwndView);
HRESULT BrfView_MergeMenu(
    PBRFVIEW that,
    LPQCMINFO pinfo);
HRESULT BrfView_Command(
    PBRFVIEW that,
    LPSHELLFOLDER psf,
    HWND hwnd,
    UINT uID);
HRESULT BrfView_InitMenuPopup(
    PBRFVIEW that,
    HWND hwnd,
    UINT idCmdFirst,
    int nIndex,
    HMENU hmenu);
void BrfView_OnGetButtons(
    PBRFVIEW that,
    HWND hwndMain,
    UINT idCmdFirst,
    LPTBBUTTON ptbbutton);
HRESULT BrfView_OnSelChange(
    PBRFVIEW that,
    HWND hwndMain,
    UINT idCmdFirst);
HRESULT BrfView_OnQueryFSNotify(
    PBRFVIEW that,
    SHChangeNotifyEntry * pfsne);
HRESULT BrfView_OnFSNotify(
    PBRFVIEW that,
    HWND hwndMain,
    LONG lEvent,
    LPCITEMIDLIST * ppidl);
HRESULT BrfView_OnGetDetailsOf(
    PBRFVIEW that,
    HWND hwndMain,
    UINT iColumn,
    PDETAILSINFO lpDetails);
HRESULT BrfView_OnNotifyCopyHook(
    PBRFVIEW that,
    HWND hwndMain,
    const COPYHOOKINFO * pchi);
HRESULT BrfView_OnInsertItem(
    PBRFVIEW that,
    HWND hwndMain,
    LPCITEMIDLIST pidl);

IShellFolderViewCB* BrfView_CreateSFVCB(IShellFolder* psf, PBRFVIEW pBVFolder);

#ifdef __cplusplus
}
#endif // __cplusplus
