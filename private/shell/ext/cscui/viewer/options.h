//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       options.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_OPTIONS_H
#define _INC_CSCUI_OPTIONS_H
///////////////////////////////////////////////////////////////////////////////
/*  File: options.h

    Description: 
            

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    04/15/98    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

#ifndef _INC_CSCVIEW_CONFIG_H
#   include "config.h"
#endif

#ifndef _INC_CSCUI_UIHELP_H
#   include "uihelp.h"
#endif

#ifndef _INC_MATH
#   include <math.h>
#endif

#ifndef _INC_CSCUI_PURGE_H
#   include "purge.h"
#endif

#include "resource.h"

//
// The "Advanced" dialog invoked from the "Advanced" button on the 
// "Offline Files" prop page.
//
class CAdvOptDlg
{
    public:
        enum 
        {
            //
            // Listview subitem IDs.
            // 
            iLVSUBITEM_SERVER = 0,
            iLVSUBITEM_ACTION = 1
        };

        CAdvOptDlg(HINSTANCE hInstance, 
                   HWND hwndParent)
            : m_hInstance(hInstance),
              m_hwndParent(hwndParent),
              m_hwndDlg(NULL),
              m_hwndLV(NULL),
              m_iLastColSorted(-1),
              m_bSortAscending(true),
              m_bNoConfigGoOfflineAction(false),
              m_bNoCustomizeGoOfflineAction(false) { }

        int Run(void);

    protected:
        BOOL OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lInitParam);
        BOOL OnNotify(HWND hDlg, int idCtl, LPNMHDR pnmhdr);
        BOOL OnCommand(HWND hDlg, WORD wNotifyCode, WORD wID, HWND hwndCtl);
        BOOL OnDestroy(HWND hDlg);
        BOOL OnContextMenu(WPARAM wParam, LPARAM lParam);
        BOOL OnHelp(HWND hDlg, LPHELPINFO pHelpInfo);

    private:
        //
        // Structure to associate a radio button with an offline action code.
        //
        struct CtlActions
        {
            UINT                    idRbn;
            CConfig::OfflineAction action;
        };

        //
        // Class to describe the state of controls in the dialog.  Used to 
        // determine if user has changed anything.
        //
        class PgState
        {
            public:
                PgState(void)
                    : m_DefaultGoOfflineAction(CConfig::eGoOfflineSilent) { }

                void SetDefGoOfflineAction(CConfig::OfflineAction action)
                    { m_DefaultGoOfflineAction = action; }

                CConfig::OfflineAction GetDefGoOfflineAction(void) const
                    { return m_DefaultGoOfflineAction; }

                void SetCustomGoOfflineActions(HWND hwndLV);

                const CArray<CConfig::CustomGOA>& GetCustomGoOfflineActions(void) const
                    { return m_rgCustomGoOfflineActions; }

                bool operator == (const PgState& rhs) const;

                bool operator != (const PgState& rhs) const
                    { return !(*this == rhs); }

            private:
                CConfig::OfflineAction     m_DefaultGoOfflineAction;
                CArray<CConfig::CustomGOA> m_rgCustomGoOfflineActions;
        };

        HINSTANCE    m_hInstance;
        HWND         m_hwndParent;
        HWND         m_hwndDlg;
        HWND         m_hwndLV;
        int          m_iLastColSorted;
        PgState      m_state;         // State on creation.
        bool         m_bSortAscending;
        bool         m_bNoConfigGoOfflineAction;
        bool         m_bNoCustomizeGoOfflineAction;

        static const CtlActions m_rgCtlActions[CConfig::eNumOfflineActions];
        static const DWORD m_rgHelpIDs[];

        static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        void ApplySettings(void);
        void EnableCtls(HWND hwnd);
        void CreateListColumns(HWND hwndList);
        void OnLVN_GetDispInfo(LV_DISPINFO *plvdi);
        void OnLVN_ColumnClick(NM_LISTVIEW *pnmlv);
        void OnLVN_ItemChanged(NM_LISTVIEW *pnmlv);
        void OnLVN_KeyDown(NMLVKEYDOWN *plvkd);
        void OnContextMenuItemSelected(int idMenuItem);
        static int CALLBACK CompareLVItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
        void GetPageState(PgState *pps);
        CConfig::OfflineAction GetCurrentGoOfflineAction(void) const;
        void AddCustomGoOfflineAction(LPCTSTR pszServer, CConfig::OfflineAction action);
        void ReplaceCustomGoOfflineAction(CConfig::CustomGOA *pGOA, int iItem, CConfig::OfflineAction action);
        void OnAddCustomGoOfflineAction(void);
        void OnEditCustomGoOfflineAction(void);
        void OnDeleteCustomGoOfflineAction(void);
        int CountSelectedListviewItems(int *pcSetByPolicy);
        void DeleteSelectedListviewItems(void);
        void SetSelectedListviewItemsAction(CConfig::OfflineAction action);
        void FocusOnSomethingInListview(void);
        int GetFirstSelectedLVItemRect(RECT *prc);
        bool IsCustomActionListviewEnabled(void) const
            { return boolify(IsWindowEnabled(GetDlgItem(m_hwndDlg, IDC_GRP_CUSTGOOFFLINE))); }
        static DWORD CheckNetServer(LPCTSTR pszServer);
        static int AddGOAToListView(HWND hwndLV, int iItem, const CConfig::CustomGOA& goa);
        static CConfig::CustomGOA *FindGOAInListView(HWND hwndLV, LPCTSTR pszServer, int *piItem);
        static CConfig::CustomGOA *GetListviewObject(HWND hwndLV, int iItem);

        //
        // PgState calls GetListviewObject.
        //
        friend void PgState::SetCustomGoOfflineActions(HWND);
};

//
// The "Offline Files" property sheet page.
//
class COfflineFilesPage
{
    public:
        COfflineFilesPage(HINSTANCE hInstance, LPUNKNOWN pUnkOuter)
            : m_hInstance(hInstance),
              m_hwndDlg(NULL),
              m_pUnkOuter(pUnkOuter),
              m_hwndSlider(NULL),
              m_iSliderMax(0),
              m_llAvailableDiskSpace(0) { }

        UINT GetDlgTemplateID(void) const
            { return IDD_CSC_OPTIONS; }

        LPFNPSPCALLBACK GetCallbackFuncPtr(void) const
            { return PageCallback; }

        DLGPROC GetDlgProcPtr(void) const
            { return DlgProc; }

        //
        // This is called by the "Advanced" page to determine if controls can
        // be enabled or not.
        //
        bool IsCscEnabledChecked(void) const
            { return m_hwndDlg && BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_ENABLE_CSC); }

    protected:
        BOOL OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lInitParam);
        BOOL OnNotify(HWND hDlg, int idCtl, LPNMHDR pnmhdr);
        BOOL OnCommand(HWND hDlg, WORD wNotifyCode, WORD wID, HWND hwndCtl);
        BOOL OnDestroy(HWND hDlg) { return FALSE; };
        BOOL OnContextMenu(HWND hwndItem, int xPos, int yPos);
        BOOL ApplySettings(HWND hDlg);
        BOOL OnHelp(HWND hDlg, LPHELPINFO pHelpInfo);
        BOOL OnSettingChange(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        class PgState
        {
            public:
                PgState(void)
                    : m_bCscEnabled(false),
                      m_bFullSync(false),
                      m_bLinkOnDesktop(false),
                      m_bReminders(false),
                      m_iReminderFreq(0),
                      m_iSliderPos(0) { }

                void SetCscEnabled(bool bCscEnabled)
                    { m_bCscEnabled = bCscEnabled; }

                bool GetCscEnabled(void) const
                    { return m_bCscEnabled; }

                void SetFullSync(bool bFullSync)
                    { m_bFullSync = bFullSync; }

                bool GetFullSync(void) const
                    { return m_bFullSync; }

                void SetSliderPos(int iSliderPos)
                    { m_iSliderPos = iSliderPos; }

                int GetSliderPos(void) const
                    { return m_iSliderPos; }

                void SetRemindersEnabled(bool bEnabled)
                    { m_bReminders = bEnabled; }

                bool GetRemindersEnabled(void) const
                    { return m_bReminders; }

                void SetReminderFreq(int iMinutes)
                    { m_iReminderFreq = iMinutes; }

                int GetReminderFreq(void) const
                    { return m_iReminderFreq; }

                void SetLinkOnDesktop(bool bEnabled)
                    { m_bLinkOnDesktop = bEnabled; }

                bool GetLinkOnDesktop(void) const
                    { return m_bLinkOnDesktop; }

                bool operator == (const PgState& rhs) const
                    { return (m_bCscEnabled    == rhs.m_bCscEnabled &&
                              m_bFullSync      == rhs.m_bFullSync &&
                              m_bLinkOnDesktop == rhs.m_bLinkOnDesktop && 
                              m_iSliderPos     == rhs.m_iSliderPos &&
                              m_bReminders     == rhs.m_bReminders &&
                              m_iReminderFreq  == rhs.m_iReminderFreq); }

                bool operator != (const PgState& rhs) const
                    { return !(*this == rhs); }

            private:
                bool m_bCscEnabled;
                bool m_bFullSync;
                bool m_bLinkOnDesktop;
                bool m_bReminders;
                int  m_iSliderPos;
                int  m_iReminderFreq;
        };

        class CConfigItems
        {
            public:
                CConfigItems(void) { ZeroMemory(m_rgItems, sizeof(m_rgItems)); }

                void Load(void);

                CConfig::SyncAction SyncAtLogoff(void) const
                    { return CConfig::SyncAction(m_rgItems[iCFG_SYNCATLOGOFF].dwValue); }
                bool NoConfigCache(void) const
                    { return boolify(m_rgItems[iCFG_NOCONFIGCACHE].dwValue); }
                bool NoConfigSyncAtLogoff(void) const
                    { return m_rgItems[iCFG_SYNCATLOGOFF].bSetByPolicy; }
                bool NoReminders(void) const
                    { return boolify(m_rgItems[iCFG_NOREMINDERS].dwValue); }
                bool NoConfigReminders(void) const
                    { return m_rgItems[iCFG_NOREMINDERS].bSetByPolicy; }
                bool NoConfigCacheSize(void) const
                    { return m_rgItems[iCFG_DEFCACHESIZE].bSetByPolicy; }
                bool NoCacheViewer(void) const
                    { return boolify(m_rgItems[iCFG_NOCACHEVIEWER].dwValue); }
                bool NoConfigCscEnabled(void) const
                    { return boolify(m_rgItems[iCFG_CSCENABLED].bSetByPolicy); }
                bool NoConfigReminderFreqMinutes(void) const
                    { return boolify(m_rgItems[iCFG_REMINDERFREQMINUTES].bSetByPolicy); }
                int ReminderFreqMinutes(void) const
                    { return int(m_rgItems[iCFG_REMINDERFREQMINUTES].dwValue); }

            private:
                struct ConfigItem
                {
                    DWORD dwValue;
                    bool bSetByPolicy;
                };

                enum eConfigItems
                {
                    iCFG_NOCONFIGCACHE,
                    iCFG_SYNCATLOGOFF,
                    iCFG_NOREMINDERS,
                    iCFG_DEFCACHESIZE,
                    iCFG_NOCACHEVIEWER,
                    iCFG_CSCENABLED,
                    iCFG_REMINDERFREQMINUTES,
                    MAX_CONFIG_ITEMS
                };

                ConfigItem m_rgItems[MAX_CONFIG_ITEMS];
        };


        HINSTANCE m_hInstance;
        HWND      m_hwndDlg;
        LPUNKNOWN m_pUnkOuter;
        HWND      m_hwndSlider;
        int       m_iSliderMax;
        LONGLONG  m_llAvailableDiskSpace;
        PgState   m_state;
        CConfigItems m_config;
        static const DWORD m_rgHelpIDs[];

        static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        static UINT CALLBACK PageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
        static BOOL CALLBACK PurgeCacheCallback(CCachePurger *pPurger);
        void OnDeleteCache(void);
        void OnFormatCache(void);
        void EnableCtls(HWND hwnd);
        void InitSlider(HWND hwndDlg, LONGLONG llMaxDiskSpace, LONGLONG llUsedDiskSpace);
        void OnHScroll(HWND hwndDlg, HWND hwndCtl, int iCode, int iPos);
        void SetCacheSizeDisplay(HWND hwndCtl, int iThumbPos);
        double Fx(double x);
        double Fy(double y);
        double Rx(double x);
        LONGLONG DiskSpaceAtThumb(int t);
        int ThumbAtPctDiskSpace(double pct);
        void GetPageState(PgState *pps);
        void HandlePageStateChange(void);
        bool IsLinkOnDesktop(LPTSTR pszPathOut = NULL, UINT cchPathOut = 0);
        bool EnableOrDisableCsc(bool bEnable, bool *pbReboot, DWORD *pdwError);
};


class COfflineFilesSheet
{
    public:
        static DWORD CreateAndRun(HINSTANCE hInstance,
                                  HWND hwndParent,
                                  LONG *pDllRefCount);

    private:
        //
        // Increase this if more pages are required.
        // Currently, we only need the "Offline Files" page.
        //
        enum { MAXPAGES = 1 };

        HINSTANCE m_hInstance;
        HWND      m_hwndParent;
        LONG     *m_pDllRefCount;

        //
        // Trivial class for passing parameters to share dialog thread proc.
        //
        class ThreadParams
        {
            public:
                ThreadParams(HWND hwndParent, LONG *pDllRefCount)
                    : m_hwndParent(hwndParent),
                      m_hInstance(NULL),
                      m_pDllRefCount(pDllRefCount) { }
    
                HWND      m_hwndParent;
                HINSTANCE m_hInstance;
                LONG     *m_pDllRefCount;
  
                void SetModuleHandle(HINSTANCE hInstance)
                    { m_hInstance = hInstance; }
        };

        COfflineFilesSheet(HINSTANCE hInstance,
                           LONG *pDllRefCount,
                           HWND hwndParent);

        ~COfflineFilesSheet(void);

        DWORD Run(void);
        static UINT WINAPI ThreadProc(LPVOID pvParam);
        static BOOL CALLBACK AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);
};


class CscOptPropSheetExt : public IShellExtInit, IShellPropSheetExt
{
    public:
        CscOptPropSheetExt(HINSTANCE hInstance, LONG *pDllRefCnt);
        ~CscOptPropSheetExt(void);

        //
        // IUnknown
        //
        STDMETHODIMP         QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        //
        // IShellExtInit method
        //
        STDMETHODIMP Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY);
        //
        // IShellPropSheetExt
        //
        STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
        STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
            { return E_NOTIMPL; }

        //
        // Change this if you add or remove any prop pages from this prop sheet ext.
        //
        enum { NUMPAGES = 2 };

    private:
        LONG               m_cRef;
        HINSTANCE          m_hInstance;
        LONG              *m_pDllRefCnt;
        COfflineFilesPage *m_pOfflineFoldersPg;

        HRESULT AddPage(LPFNADDPROPSHEETPAGE lpfnAddPage, 
                        LPARAM lParam, 
                        const COfflineFilesPage& pg,
                        HPROPSHEETPAGE *phPage);
};


class CustomGOAAddDlg
{
    public:
        CustomGOAAddDlg(HINSTANCE hInstance, 
                        HWND hwndParent, 
                        CString *pstrServer, 
                        CConfig::OfflineAction *pAction);
        int Run(void);

    private:
        //
        // Structure to associate a radio button with an offline action code.
        //
        struct CtlActions
        {
            UINT                   idRbn;
            CConfig::OfflineAction action;
        };
        HINSTANCE m_hInstance;
        HWND      m_hwndParent;
        HWND      m_hwndDlg;
        HWND      m_hwndEdit;
        CString  *m_pstrServer; // We don't own this CString.
        CConfig::OfflineAction *m_pAction;
        static const CtlActions m_rgCtlActions[CConfig::eNumOfflineActions];
        static const DWORD m_rgHelpIDs[];

        static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        BOOL OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lInitParam);
        BOOL OnCommand(HWND hDlg, WORD wNotifyCode, WORD wID, HWND hwndCtl);
        BOOL OnDestroy(HWND hDlg);
        BOOL OnHelp(HWND hDlg, LPHELPINFO pHelpInfo);
        BOOL OnContextMenu(HWND hwndItem, int xPos, int yPos);
        void BrowseForServer(HWND hDlg, CString *pstrServer);
        void GetActionInfo(CString *pstrServer, CConfig::OfflineAction *pAction);
        bool CheckServerNameEntered(void);
        void GetEnteredServerName(CString *pstrServer, bool bTrimLeadingJunk);
};

class CustomGOAEditDlg
{
    public:
        CustomGOAEditDlg(HINSTANCE hInstance, 
                         HWND hwndParent, 
                         LPCTSTR pszServer, 
                         CConfig::OfflineAction *pAction);
        int Run(void);

    private:
        //
        // Structure to associate a radio button with an offline action code.
        //
        struct CtlActions
        {
            UINT                   idRbn;
            CConfig::OfflineAction action;
        };
        HINSTANCE m_hInstance;
        HWND      m_hwndParent;
        HWND      m_hwndDlg;
        CString   m_strServer;
        CConfig::OfflineAction *m_pAction;
        static const CtlActions m_rgCtlActions[CConfig::eNumOfflineActions];
        static const DWORD m_rgHelpIDs[];

        static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        BOOL OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lInitParam);
        BOOL OnCommand(HWND hDlg, WORD wNotifyCode, WORD wID, HWND hwndCtl);
        BOOL OnDestroy(HWND hDlg);
        BOOL OnHelp(HWND hDlg, LPHELPINFO pHelpInfo);
        BOOL OnContextMenu(HWND hwndItem, int xPos, int yPos);
        void GetActionInfo(CConfig::OfflineAction *pAction);
};



//-----------------------------------------------------------------------------
// Inline functions.
//-----------------------------------------------------------------------------
//
// This is the "gain" function for the slider and resulting value.
// Pass in a true position of the slider thumb and you get back 
// a scaled value for the thumb.  To change the gain, change
// this function.  Remember to change Fy() also.
//
inline double
COfflineFilesPage::Fx(
    double x
    )
{
    return (x * x) / 2.0;
}


//
// This is the "gain" function for the slider solved for 'y'.
// Pass in a virtual position for the thumb and you get back
// a true thumb position.
//
inline double
COfflineFilesPage::Fy(
    double y
    )
{
    return sqrt(2.0 * y);
}

//
// Ratio used to calculate the disk space value for a given
// true thumb position.  Give it a true thumb position between
// 0 and 100 and it will return a number between 0.0 and 1.0 that
// can be used to find the disk space.
// 
// DiskSpace = DiskSpaceMax * Rx(thumb)
//
inline double
COfflineFilesPage::Rx(
    double x
    )
{
    double denominator = Fx(m_iSliderMax);
    if (0.00001 < denominator)
        return Fx(x) / denominator;
    else
        return 1.0;
}

//
// Calculates the disk space value at a particular position of the
// slider thumb for values of 't' between 0 and 100.
//
inline LONGLONG
COfflineFilesPage::DiskSpaceAtThumb(
    int t
    )
{
    return LONGLONG(double(m_llAvailableDiskSpace) * Rx(t));
}

//
// Calculates the true thumb position for a given disk space
// percent value between 0.0 and 1.0.
// The expression breaks down as follows:
// 
//      double MaxVirtualThumb = Fx(m_iSliderMax);
//      double VirtualThumb    = MaxVirtualThumb * pct;
//      double TrueThumb       = Fy(VirtualThumb);
//
//      return round(TrueThumb);  // "round" used for illustration only.
//
inline int
COfflineFilesPage::ThumbAtPctDiskSpace(
    double pct
    )
{
    double t  = Fy(Fx(m_iSliderMax) * pct);
    double ft = floor(t);
    if (0.5 < t - ft)
    {
        //
        // Since the thumb position must be a whole number,
        // round up if necessary.  Typecast from double to int
        // merely truncates at the decimal point.
        //
        ft += 1.0;
    }
    return int(ft);
}


#endif // _INC_CSCUI_OPTIONS_H
