//
// twin.h: Declares data, defines and struct types for twin handling
//          module.
//
//

#ifndef __TWIN_H__
#define __TWIN_H__


/////////////////////////////////////////////////////  INCLUDES

/////////////////////////////////////////////////////  DEFINES

#define CMP_RECNODES        1L
#define CMP_FOLDERTWINS     2L

#define OBJECT_TWIN_ATTRIBUTES   (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY)

/////////////////////////////////////////////////////  MACROS

#ifdef DEBUG

#define Sync_Dump(lpvBuf, type)     Sync_FnDump((LPVOID)lpvBuf, sizeof(type))

extern CONST LPCTSTR rgcpcszTwinResult[];
#define SzFromTR(tr)                (rgcpcszTwinResult[(tr)])

#else

#define Sync_Dump(lpvBuf, type)
#define Sync_DumpRecItem(tr, lpri, pszMsg)
#define Sync_DumpRecNode(tr, lprn)
#define Sync_DumpRecList(tr, lprl, pszMsg)
#define SzFromTR(tr)                (tr)

#endif

// Global v-table to the engine
//

typedef struct 
    {
    HINSTANCE   hinst;

    OPENBRIEFCASEINDIRECT           OpenBriefcase;
    SAVEBRIEFCASEINDIRECT           SaveBriefcase;
    CLOSEBRIEFCASEINDIRECT          CloseBriefcase;
    DELETEBRIEFCASEINDIRECT         DeleteBriefcase;
    GETOPENBRIEFCASEINFOINDIRECT    GetOpenBriefcaseInfo;
    CLEARBRIEFCASECACHEINDIRECT     ClearBriefcaseCache;
    FINDFIRSTBRIEFCASEINDIRECT      FindFirstBriefcase;
    FINDNEXTBRIEFCASEINDIRECT       FindNextBriefcase;
    FINDBRIEFCASECLOSEINDIRECT      FindBriefcaseClose;

    ADDOBJECTTWININDIRECT           AddObjectTwin;
    ADDFOLDERTWININDIRECT           AddFolderTwin;
    RELEASETWINHANDLEINDIRECT       ReleaseTwinHandle;
    DELETETWININDIRECT              DeleteTwin;
    GETOBJECTTWINHANDLEINDIRECT     GetObjectTwinHandle;
    ISFOLDERTWININDIRECT            IsFolderTwin;
    CREATEFOLDERTWINLISTINDIRECT    CreateFolderTwinList;
    DESTROYFOLDERTWINLISTINDIRECT   DestroyFolderTwinList;
    GETFOLDERTWINSTATUSINDIRECT     GetFolderTwinStatus;
    ISORPHANOBJECTTWININDIRECT      IsOrphanObjectTwin;
    COUNTSOURCEFOLDERTWINSINDIRECT  CountSourceFolderTwins;
    ANYTWINSINDIRECT                AnyTwins;

    CREATETWINLISTINDIRECT          CreateTwinList;
    DESTROYTWINLISTINDIRECT         DestroyTwinList;
    ADDTWINTOTWINLISTINDIRECT       AddTwinToTwinList;
    ADDALLTWINSTOTWINLISTINDIRECT   AddAllTwinsToTwinList;
    REMOVETWINFROMTWINLISTINDIRECT  RemoveTwinFromTwinList;
    REMOVEALLTWINSFROMTWINLISTINDIRECT RemoveAllTwinsFromTwinList;

    CREATERECLISTINDIRECT           CreateRecList;
    DESTROYRECLISTINDIRECT          DestroyRecList;
    RECONCILEITEMINDIRECT           ReconcileItem;
    BEGINRECONCILIATIONINDIRECT     BeginReconciliation;
    ENDRECONCILIATIONINDIRECT       EndReconciliation;

    ISPATHONVOLUMEINDIRECT          IsPathOnVolume;
    GETVOLUMEDESCRIPTIONINDIRECT    GetVolumeDescription;

    } VTBLENGINE, * PVTBLENGINE;


// Function wrappers
//
//      TWINRESULT Sync_Verify(fn, args)
//          Returns the twinresult of the function that is called
//
#ifdef DEBUG
#define Sync_Verify(fn, args)  \
    (Sync_SetLastError(g_vtblEngine[0].##fn args), VERIFYSZ2(TR_SUCCESS == Sync_GetLastError(), TEXT("Assert  ") TEXT(#fn) TEXT(": %s (LastError=%ldL)"), SzFromTR(Sync_GetLastError()), GetLastError()), Sync_GetLastError())
#else
// Optimized version for retail
//
#define Sync_Verify(fn, args)  \
    Sync_SetLastError(g_vtblEngine[0].##fn args)
#endif

//      BOOL Sync_IsEngineLoaded();
//          Returns TRUE if the engine is loaded.
//
BOOL PUBLIC Sync_IsEngineLoaded();

//      TWINRESULT Sync_SetLastError(TWINRESULT tr);
//
TWINRESULT PUBLIC Sync_SetLastError(TWINRESULT tr);

//      TWINRESULT Sync_GetLastError();
//
TWINRESULT PUBLIC Sync_GetLastError(void);

//      TWINRESULT Sync_OpenBriefcase(LPCSTR pszPath, DWORD dwFlags, HWND hwndOwner, HBRFCASE FAR * lphbrf);
//
#define Sync_OpenBriefcase(lpcsz, dwflags, hwnd, lphbrf)           \
    Sync_Verify(OpenBriefcase, (lpcsz, dwflags, hwnd, lphbrf))

//      TWINRESULT Sync_SaveBriefcase(HBRFCASE hbrf);
//
#define Sync_SaveBriefcase(hbrf)             \
    Sync_Verify(SaveBriefcase, (hbrf))

//      TWINRESULT Sync_DeleteBriefcase(LPCSTR pszPath);
//
#define Sync_DeleteBriefcase(lpcsz)                 \
    Sync_Verify(DeleteBriefcase, (lpcsz))

//      TWINRESULT Sync_CloseBriefcase(HBRFCASE hbrf);
//
#define Sync_CloseBriefcase(hbrf)                   \
    Sync_Verify(CloseBriefcase, (hbrf))

//      TWINRESULT Sync_GetOpenBriefcaseInfo(HBRFCASE hbrf, POPENBRFCASEINFO pinfo);
//
#define Sync_GetOpenBriefcaseInfo(hbrf, pinfo)          \
    Sync_Verify(GetOpenBriefcaseInfo, (hbrf, pinfo))

//      TWINRESULT Sync_ClearBriefcaseCache(HBRFCASE hbrf);
//
#define Sync_ClearBriefcaseCache(hbrf)                   \
    Sync_Verify(ClearBriefcaseCache, (hbrf))

//      TWINRESULT Sync_FindFirst(PHBRFCASEITER phbrfiter, PBRFCASEINFO pinfo);
//
#define Sync_FindFirst(phbrfiter, pinfo)            \
    Sync_Verify(FindFirstBriefcase, (phbrfiter, pinfo))

//      TWINRESULT Sync_FindNext(HBRFCASEITER hbrfiter, PBRFCASEINFO pinfo);
//
#define Sync_FindNext(hbrfiter, pinfo)            \
    Sync_Verify(FindNextBriefcase, (hbrfiter, pinfo))

//      TWINRESULT Sync_FindClose(HBRFCASEITER hbrfiter);
//
#define Sync_FindClose(hbrfiter)            \
    Sync_Verify(FindBriefcaseClose, (hbrfiter))

//      TWINRESULT Sync_AddObject(HBRFCASE hbrf, LPNEWOBJECTTWIN lpnot, HTWIN FAR * lphtfam);
//
#define Sync_AddObject(hbrf, lpnot, lphtfam)        \
    Sync_Verify(AddObjectTwin, (hbrf, lpnot, lphtfam))

//      TWINRESULT Sync_AddFolder(HBRFCASE hbrf, LPNEWFOLDERTWIN lpnft, HTWIN FAR * lphft);
//
#define Sync_AddFolder(hbrf, lpnft, lphft)          \
    Sync_Verify(AddFolderTwin, (hbrf, lpnft, lphft))

//      TWINRESULT Sync_ReleaseTwin(HTWIN htwin);
//
#define Sync_ReleaseTwin(htwin)                     \
    Sync_Verify(ReleaseTwinHandle, ((HTWIN)htwin))

//      TWINRESULT Sync_DeleteTwin(HTWIN htwin);
//
#define Sync_DeleteTwin(htwin)                      \
    Sync_Verify(DeleteTwin, ((HTWIN)htwin))

//      TWINRESULT Sync_AnyTwins(HBRFCASE hbrf, BOOL FAR * lpb);
//
#define Sync_AnyTwins(hbrf, lpb)                    \
    Sync_Verify(AnyTwins, (hbrf, lpb))

//      TWINRESULT Sync_CountSourceFolders(HOBJECTTWIN hot, ULONG FAR * lpulcSource);
//
#define Sync_CountSourceFolders(hot, lpulcSource)   \
    Sync_Verify(CountSourceFolderTwins, (hot, lpulcSource))

//      TWINRESULT Sync_IsExplicitObject(HOBJECTTWIN hot, BOOL FAR * lpb);
//
#define Sync_IsExplicitObject(hot, lpb)               \
    Sync_Verify(IsOrphanObjectTwin, (hot, lpb))

//      TWINRESULT Sync_GetObject(HBRFCASE hbrf, LPCSTR pszDir, LPCSTR pszName, HOBJECTTWIN FAR * lphot);
//
#define Sync_GetObject(hbrf, lpszFolder, lpszName, lphot)   \
    Sync_Verify(GetObjectTwinHandle, (hbrf, lpszFolder, lpszName, lphot))

//      TWINRESULT Sync_IsFolder(HBRFCASE hbrf, LPCSTR pszFolder, BOOL FAR * lpb);
//
#define Sync_IsFolder(hbrf, lpszFolder, lpb)        \
    Sync_Verify(IsFolderTwin, (hbrf, lpszFolder, lpb))

//      TWINRESULT Sync_GetFolderTwinStatus(HFOLDERTWIN hft, CREATERECLISTPROC crlp, LPARAM lpCallbackData, PFOLDERTWINSTATUS pfts);
//
#ifdef NEW_REC

#define Sync_GetFolderTwinStatus(hft, crlp, lpcb, pfts)        \
    Sync_Verify(GetFolderTwinStatus, (hft, crlp, lpcb, pfts))

#else

#define Sync_GetFolderTwinStatus(hft, crlp, lpcb, pfts)        \
    Sync_Verify(GetFolderTwinStatus, (hft, crlp, lpcb, CRL_FLAGS, pfts))

#endif

//      TWINRESULT Sync_CreateFolderList(HBRFCASE hbrf, LPCSTR pszDir, LPFOLDERTWINLIST FAR * lplpftl);
//
#define Sync_CreateFolderList(hbrf, lpszFolder, lplpftl)    \
    Sync_Verify(CreateFolderTwinList, (hbrf, lpszFolder, lplpftl))

//      TWINRESULT Sync_DestroyFolderList(LPFOLDERTWINLIST lpftl);
//
#define Sync_DestroyFolderList(lpftl)               \
    Sync_Verify(DestroyFolderTwinList, (lpftl))

//      TWINRESULT Sync_CreateTwinList(HBRFCASE hbrf, LPHTWINLIST lphtl);
//
#define Sync_CreateTwinList(hbrf, lphtl)            \
    Sync_Verify(CreateTwinList, (hbrf, lphtl))

//      TWINRESULT Sync_DestroyTwinList(HTWINLIST htl);
//
#define Sync_DestroyTwinList(htl)                   \
    Sync_Verify(DestroyTwinList, (htl))

//      TWINRESULT Sync_AddToTwinList(HTWINLIST htl, HTWIN htwin);
//
#define Sync_AddToTwinList(htl, htwin)              \
    Sync_Verify(AddTwinToTwinList, (htl, (HTWIN)htwin))

//      TWINRESULT Sync_AddAllToTwinList(HTWINLIST htl);
//
#define Sync_AddAllToTwinList(htl)                  \
    Sync_Verify(AddAllTwinsToTwinList, (htl))

//      TWINRESULT Sync_RemoveFromTwinList(HTWINLIST htl, HTWIN htwin);
//
#define Sync_RemoveFromTwinList(htl, htwin)         \
    Sync_Verify(RemoveTwinFromTwinList, (htl, (HTWIN)htwin))

//      TWINRESULT Sync_RemoveAllFromTwinList(HTWINLIST htl);
//
#define Sync_RemoveAllFromTwinList(htl)             \
    Sync_Verify(RemoveAllTwinsFromTwinList, (htl))

//      TWINRESULT Sync_CreateRecList(HTWINLIST htl, CREATERECLISTPROC crlp, LPARAM lpcb, LPRECLIST FAR * lplprl);
//
#ifdef NEW_REC

#define Sync_CreateRecList(htl, crlp, lpcb, lplprl)            \
    Sync_Verify(CreateRecList, (htl, crlp, lpcb, lplprl))

#else

#define Sync_CreateRecList(htl, crlp, lpcb, lplprl)            \
    Sync_Verify(CreateRecList, (htl, crlp, lpcb, CRL_FLAGS, lplprl))

#endif

//      TWINRESULT Sync_DestroyRecList(LPRECLIST lprl);
//
#define Sync_DestroyRecList(lprl)                   \
    Sync_Verify(DestroyRecList, (lprl))

//      TWINRESULT Sync_BeginRec(HBRFCASE hbrf);
//
#define Sync_BeginRec(hbrf)                         \
    Sync_Verify(BeginReconciliation, (hbrf))

//      TWINRESULT Sync_EndRec(HBRFCASE hbrf);
//
#define Sync_EndRec(hbrf)                           \
    Sync_Verify(EndReconciliation, (hbrf))

//      TWINRESULT Sync_ReconcileItem(LPRECITEM lpri, RECSTATUSPROC rsp, LPARAM lpCallbackData, DWORD dwFlags, HWND hwndOwner, HWND hwndStatusText);
//
#define Sync_ReconcileItem(lpri, rsp, lpcb, flags, hwnd, hwndStatusText)    \
    Sync_Verify(ReconcileItem, (lpri, rsp, lpcb, flags, hwnd, hwndStatusText))

//      BOOL Sync_IsPathOnVolume(LPCSTR pszPath, HVOLUMEID hvid);
//
#define Sync_IsPathOnVolume(pszPath, hvid, pbool)      \
    Sync_Verify(IsPathOnVolume, (pszPath, hvid, pbool))

//      BOOL Sync_GetVolumeDecription(HVOLUMEID hvid, PVOLUMEDESC pvd);
//
#define Sync_GetVolumeDescription(hvid, pvd)      \
    Sync_Verify(GetVolumeDescription, (hvid, pvd))


#ifdef NEW_REC

// Returns TRUE if the recitem is referring to a file (not a folder)
#define IsFileRecItem(pri)    (0 != *(pri)->pcszName)

#else

#define IsFileRecItem(pri)    TRUE

#endif


extern const TCHAR szAll[];

extern VTBLENGINE g_vtblEngine[1];

int CALLBACK _export NCompareFolders (LPVOID lpv1, LPVOID lpv2, LPARAM lParam);

// Structure for the ChooseSide functions
typedef struct tagCHOOSESIDE
    {
    DWORD       dwFlags;        // CSF_* flags
    int         nRank;          // Higher means better choice

    HTWIN       htwin;
    HVOLUMEID   hvid;
    LPCTSTR      pszFolder;
    PRECNODE    prn;            // Only if CSF_FOLDER is clear
    } CHOOSESIDE, * PCHOOSESIDE;

#define CSF_FOLDER      0x0001
#define CSF_INSIDE      0x0002      // Ranking for inside

#ifdef DEBUG
void    PUBLIC ChooseSide_DumpList(HDSA hdsa);
#endif

void    PUBLIC ChooseSide_InitAsFile(HDSA hdsa, PRECITEM pri);
HRESULT PUBLIC ChooseSide_CreateEmpty(HDSA * phdsa);
HRESULT PUBLIC ChooseSide_CreateAsFile(HDSA * phdsa, PRECITEM pri);
HRESULT PUBLIC ChooseSide_CreateAsFolder(HDSA * phdsa, PFOLDERTWINLIST pftl);
BOOL    PUBLIC ChooseSide_GetBest(HDSA hdsa, LPCTSTR pszBrfPath, LPCTSTR pszFolder, PCHOOSESIDE * ppchside);
BOOL    PUBLIC ChooseSide_GetNextBest(HDSA hdsa, PCHOOSESIDE * ppchside);
void    PUBLIC ChooseSide_Free(HDSA hdsa);


HRESULT PUBLIC Sync_GetNodePair(PRECITEM pri, LPCTSTR pszBrfPath, LPCTSTR pszFolder, PRECNODE  * pprnInside, PRECNODE  * pprnOutside);
void    PUBLIC Sync_ChangeRecItemAction(PRECITEM pri, LPCTSTR pszBrfPath, LPCTSTR pszInsideDir, UINT uAction);

BOOL PUBLIC Sync_QueryVTable();
void PUBLIC Sync_ReleaseVTable();

BOOL PUBLIC Sync_AddPathToTwinList(HBRFCASE hbrf, HTWINLIST htl, LPCTSTR lpcszPath, PFOLDERTWINLIST  * lplpftl);

HRESULT PUBLIC Sync_CreateCompleteRecList(HBRFCASE hbrf, PABORTEVT pabortevt, PRECLIST * pprl);
HRESULT PUBLIC Sync_CreateRecListEx(HTWINLIST htl, PABORTEVT pabortevt, PRECLIST * pprl);

// Flags for Sync_ReconcileRecList
#define RF_DEFAULT      0x0000
#define RF_ONADD        0x0001

HRESULT PUBLIC Sync_ReconcileRecList (PRECLIST lprl, LPCTSTR pszPathBrf, HWND hwndParent, UINT uFlags);

// Flags for Sync_Split
#define SF_NOCONFIRM    0x0001
#define SF_QUIET        0x0002

// Flags for Sync_IsTwin and Sync_Split
#define SF_ISFOLDER     0x1000
#define SF_ISFILE       0x2000
#define SF_ISTWIN       0x4000
#define SF_ISNOTTWIN    0x8000

HRESULT PUBLIC Sync_IsTwin (HBRFCASE hbrf, LPCTSTR lpcszPath, UINT uFlags);
HRESULT PUBLIC Sync_Split (HBRFCASE hbrf, LPCTSTR pszList, UINT cFiles, HWND hwndOwner, UINT uFlags);

ULONG   PUBLIC CountActionItems(PRECLIST prl);

#ifdef DEBUG
void PUBLIC Sync_FnDump (LPVOID lpvBuf, UINT cbBuf);
void PUBLIC Sync_DumpRecItem (TWINRESULT tr, PRECITEM lpri, LPCTSTR pszMsg);
void PUBLIC Sync_DumpRecNode (TWINRESULT tr, PRECNODE lprn);
void PUBLIC Sync_DumpRecList(TWINRESULT tr, PRECLIST lprl, LPCTSTR pszMsg);
void PUBLIC Sync_DumpFolderTwin(PCFOLDERTWIN pft);
void PUBLIC Sync_DumpFolderTwinList(PFOLDERTWINLIST pftl, LPCTSTR pszMsg);
#endif

#endif // __TWIN_H__

