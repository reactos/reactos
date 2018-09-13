#ifndef _INC_CSCVIEW_CACHVIEW_H
#define _INC_CSCVIEW_CACHVIEW_H
///////////////////////////////////////////////////////////////////////////////
/*  File: cachview.h

    Description: Declaration for classes:
    
            CacheWindow
            CacheView
                ShareView
                DetailsView
                StaleView
            ViewState

        The following comments describe some of the behaviors and issues that
        are relavent to this design. While it exists here in the cachview 
        module, some of the information is applicable to the non-UI portions
        of the design (i.e. object tree).

        General operation -----------------------------------------------------

        A client instantiates a CacheWindow object then calls the Run() member
        passing an identifier for the initial view type and a string 
        identifying the intial network share to view from the CSC cache.  
        The window creates the appropriate view object but deals with the 
        view through the abstract base class CacheView.  The window includes 
        the non-client areas (menu, status bar, toolbar) while the view 
        includes the header and listview controls.  Whenever a new view type 
        is selected from within the UI, the existing view object is destroyed 
        and a new one of the proper type is created.  
        
        The source of data for the various views is supplied by a CscObjTree
        object maintained in the CacheWindow.  By associating the object data
        with the window object, this enforces the concept that the various
        views are merely "views" onto the same underlying data.  It also 
        ensures that the data is loaded only once when the window is
        first created then made available for any subsequently created views.

        Performance -----------------------------------------------------------

        Several pieces of this implementation are in place to prevent undue
        latencies in the UI while CSC cache information is being obtained.  

        1.  Object tree loads on background thread(s).  The loading of each
            share's files into the object tree is handled on a separate thread.
            While each share has it's own thread, only one thread is loading
            at any one point in time.  Tree population for a share only occurs 
            if a user is requesting information for a share in the UI.
            If a share subtree is currently being populated and the user 
            selects another share in the UI, the 1st share's thread is suspended
            at a controlled point in the process and the newly-selected share
            begins or resumes population.  The suspended share's thread
            maintains context information about the current point in the 
            cache enumeration.  If the user again selects the suspended share
            for viewing, tree population for that share resumes where it
            previously left off.

        2.  UI uses "pull" model.  Rather than have the loading process "push"
            data to the UI, the UI requests data from the object tree whenever
            it has time to do so.  This allows the user's interaction with
            the UI to take priority.  It also eliminates many thread 
            synchronization issues since object loading and UI loading run
            on different threads.  This also means that the UI may display 
            items before a share is fully populated in the object tree.  
            UI population continues until either the share subtree is fully
            populated or the user selects a different view.  

        3.  Object tree is a hierarchy.  While data is displayed in the UI
            as a "flat" list with each entry containing full path information,
            the information source (object tree) maintains this information 
            in a hierarchy parallel to that found on disk.  This minimizes
            any duplication of data storage, reducing memory requirements.

        4.  Reference-counted string class.  The entire viewer implementation
            relies heavily on the class CString.  This is a reference-counted
            string class which greatly reduces the amount of duplication of
            string information when caching and passing strings through the
            system.

        5.  UI display information cached.  Storing object tree data as a
            hierarchy, requires that the UI display code traverse tree
            entries whenever path information is required for a given item
            in the CSC cache.  To minimize the effect of this processing 
            during common UI repaint situations (window exposed, scrolled etc),
            the window object maintains a small cache of display information
            for objects in the object tree.  The information is keyed on 
            the address of an object in the object tree.  A hash table is 
            used for fast retrieval.  When the cache is full, item's are 
            expelled from the cache on a least-recently-used basis.  

        6.  Icon loading deferred.  In order to display proper icon information
            for cache entries, the UI must go to the shell for this
            information using SHGetFileInformation().  Because this function
            must go out to the network when CSC is in online mode, it can
            take a considerable amount of time to obtain icon information.
            For this reason, gathering of icon information is placed on a
            background thread and processed as the UI has time.  Once an
            icon is located by the shell's code, a copy of it is placed in
            the cache window's image list and it's image list index is 
            stored in the object tree for that item.  Subsequent requests
            for an icon are immediately satisfied by the cached image index
            and imagelist image.


        State Information -----------------------------------------------------

        Both window and view state information are persisted to the registry
        on a per-user basis.  Window state information includes window size
        and display mode (maximized or not).  View information includes 
        column order and column width.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_CSCVIEW_OBJTREE_H
#   include "objtree.h"
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

#ifndef _INC_CSCVIEW_CARRAY_H
#   include "carray.h"
#endif

#ifndef _INC_CSCVIEW_VIEWCOLS_H
#   include "viewcols.h"
#endif

#ifndef _INC_CSCVIEW_REGSTR_H
#   include "regstr.h"
#endif

#ifndef _INC_CSCVIEW_CLIST_H
#   include "clist.h"
#endif

#ifndef _INC_CSCVIEW_SUBCLASS_H
#   include "subclass.h"
#endif

#ifndef _INC_CSCVIEW_PROGRESS_H
#   include "progress.h"
#endif

#ifndef _INC_CSCVIEW_CCINLINE_H
#   include "ccinline.h"
#endif

#ifndef _INC_CSCVIEW_BITSET_H
#   include "bitset.h"
#endif

#ifndef _INC_CSCVIEW_PATHSTR_H
#   include "pathstr.h"
#endif

#ifndef _INC_CSCVIEW_COPYDLGS_H
#   include "copydlgs.h"
#endif

class ViewState
{
    public:
        ViewState(void) throw() { }

        ViewState(const ViewState& rhs);
        ViewState& operator = (const ViewState& rhs);

        int ColumnCount(void) const throw()
            { return m_rgColIndices.Count(); }

        void AddColumnInfo(int iIndex, int iWidth);

        int ColumnWidth(int iCol) const
            { return m_rgColWidths[iCol]; }

        int ColumnIndex(int iCol) const
            { return m_rgColIndices[iCol]; }

        void Clear(void) throw();

        int PersistentBufferSize(void) const throw();
        bool WriteToBuffer(LPBYTE pbBuffer, int cbBuffer) const;
        bool LoadFromBuffer(LPBYTE pbBuffer, int cbBuffer);

    private:
        CArray<int> m_rgColWidths;
        CArray<int> m_rgColIndices;
        static const short VIEW_STATE_VERSION;
};


class CacheView
{
    public:
        CacheView(HINSTANCE hInstance, 
                  HWND hwndParent, 
                  const CscObjTree& tree,
                  const CString& strShare);

        virtual ~CacheView(void);

        int ColumnCount(void) const throw()
            { return m_rgpColumns.Count(); }

        void AddColumn(int iSubItem, const LVColumn& col);

        void Move(int x, int y, int w, int h, bool bRepaint) throw()
            { MoveWindow(m_hwndLV, x, y, w, h, bRepaint); }

        HWND GetViewWindow(void) const throw()
            { return m_hwndLV; }

        HIMAGELIST GetImageList(void) const throw()
            { return m_himl; }

        HFONT GetListViewFont(void) throw();
        
        void SetRedraw(bool bRedraw) throw()
            { SendMessage(m_hwndLV, WM_SETREDRAW, (WPARAM)bRedraw, 0); }

        void DestroyImageList(void) throw();
        void DestroyIconHound(void) throw();
        bool EndIconHoundThread(DWORD dwWait = INFINITE);
        bool IsIconHoundThreadRunning(DWORD dwWait = 0);

        void GetDispInfo(LV_DISPINFO *pdi);

        int ObjectCount(void) const throw()
            { return NULL != m_hwndLV ? ListView_GetItemCount(m_hwndLV) : 0; }
        int SelectedObjectCount(void) const throw()
            { return NULL != m_hwndLV ? ListView_GetSelectedCount(m_hwndLV) : 0; }
        int ConsideredObjectCount(void) const throw()
            { return m_cObjConsidered; }

        LRESULT SortObjects(DWORD idColumn,  bool bSortAscending) throw();
        static CscObject *GetItemObject(HWND hwndLV, int iItem) throw();

        bool SaveViewState(void) const;
        bool RestoreViewState(void);

        void SelectAllItems(void);
        void InvertSelectedItems(void);
        void SetFocus(void) const throw();

        //
        // These virtual functions determine the sensitivity of the 
        // toolbar buttons and menu items in a view.  By default,
        // all functions are available.  To disable a toolbar button
        // or menu option, subclasses override the appropriate function
        // and have it return false.
        //
        virtual bool CanOpen(void) const throw();
        virtual bool CanSaveAs(void) const throw()
            { return 0 < SelectedObjectCount(); }
        virtual bool CanDelete(void) const throw()
            { return 0 < SelectedObjectCount(); }
        virtual bool CanPin(void) const throw()
            { return 0 < SelectedObjectCount(); }
        virtual bool CanUnpin(void) const throw()
            { return 0 < SelectedObjectCount(); }
        virtual bool CanUpdate(void) const throw()
            { return 0 < SelectedObjectCount() && !AnyInSelectionDisconnected(); }

        bool LoadNextCscObject(bool *pbObjEnum, int *pcObjAdded);

        void ObjectChanged(const CscObject *pObject) const throw();
        void DeleteObject(const CscObject *pObject) const;

        virtual bool ExcludeCscObject(const CscObject& object) const throw()
            { return false; }

        //
        // Iterate through selected items in a listview.
        //
        class ItemIterator
        {
            public:
                explicit ItemIterator(HWND hwndLV, bool bSelected = false) throw()
                    : m_hwndLV(hwndLV),
                      m_iItem(-1),
                      m_bSelected(bSelected) { }

                ItemIterator(const ItemIterator& rhs) throw();

                ItemIterator& ItemIterator::operator = (const ItemIterator& rhs) throw();

                bool Next(CscObject **pObject) throw();

                void Reset(void) throw()
                    { m_iItem = -1; }

            private:
                HWND m_hwndLV;
                int  m_iItem;
                bool m_bSelected;
        };

        ItemIterator CreateItemIterator(bool bSelected) throw()
            { return ItemIterator(m_hwndLV, bSelected); }

    protected:
        HINSTANCE   m_hInstance;
        HWND        m_hwndParent;   // Parent window.
        HWND        m_hwndLV;       // Listview window handle.
        HWND        m_hwndHeader;   // Listview header window handle.
        static HIMAGELIST  m_himl;  // Images used in listview & header.

        CString             m_strShare;         // Name of share in view.  "" = all
        CString             m_strRegKey;        // Reg key for persistent data.
        const CscObjTree&   m_CscObjTree;       // Reference to object tree.
        CscObjIterator     *m_pCscObjIter;      // UI object iterator.
        CArray<LVColumn *>  m_rgpColumns;       // Array of view columns.
        CscObject          *m_rgLoadBuffer[25]; // Temp cache for loading objects.
        int                 m_cLoadBuffer;      // Item count in temp cache.
        int                 m_cObjConsidered;   // Obj's considered for loading.

        bool CreateTheView(const RECT& rc);
        bool CreateListView(const RECT& rc);
        virtual void CreateListViewColumns(void) { }
        HIMAGELIST CreateImageList(VOID);
        void FlushLoadBuffer(void);
        int FindObject(const CscObject *pObject) const throw();

        virtual CscObjIterator *CreateCscObjIterator(const CString& strShare);

        static int CALLBACK SortCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

        bool LoadViewStateFromRegistry(ViewState *pvs);
        bool SaveViewStateToRegistry(const ViewState& vs) const;
        virtual void GetViewStateRegValueName(CString *pstrName) const = 0;

    private:
        //
        // Runs on a separate thread to obtain icons for objects in the
        // CSC object tree.
        //
        class IconHound
        {
            public:
                IconHound(void);
                ~IconHound(void);

                void AddObject(CscObject *pObject);
                void Run(HWND hwndNotify) throw();
                bool EndThread(DWORD dwWait = INFINITE);
                bool WaitForThreadExit(DWORD dwWait = INFINITE);

            private:
                HWND m_hwndNotify;               // Wnd to post CVM_UPDATEOBJECT
                CQueueAsList<CscObject *> m_oq;  // Queue of CscObject ptrs.
                HANDLE m_hThread;
                CCriticalSection m_cs;
                CSemaphore m_sem;
                static CArray<unsigned short> m_rgSysImageXref;
                
                static UINT WINAPI ThreadProc(LPVOID pvParam) throw();

                int GetCachedImageIndex(int iSysImage) const throw();
                void CacheSysImageIndex(int iSysImage, int iImage);

                //
                // Prevent copy.
                //
                IconHound(const IconHound& rhs);
                IconHound& operator = (const IconHound& rhs);

        };
        
        static IconHound *m_pIconHound;

        void FocusOnSomething(void) const throw();
        bool AnyInSelectionDisconnected(void) const throw();
        //
        // Prevent copy.
        //
        CacheView(const CacheView& rhs);
        CacheView& operator = (const CacheView& rhs);

        //
        // IconHound::ThreadProc needs to declare private type IconHound.
        //
        friend UINT IconHound::ThreadProc(LPVOID pvParam);
};



class ShareView : public CacheView
{
    public:
        ShareView(HINSTANCE hInstance, 
                  HWND hwndParent, 
                  const RECT& rc, 
                  const CscObjTree& tree, 
                  const CString& strShare)
            : CacheView(hInstance, hwndParent, tree, strShare) {  CreateTheView(rc); }

        virtual void GetViewStateRegValueName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = REGSTR_VAL_VIEWSTATE_SHARE; }

        virtual bool CanOpen(void) const throw()
            { return false; }
        virtual bool CanSaveAs(void) const throw()
            { return false; }
        virtual bool CanPin(void) const throw() 
            { return false; }
        virtual bool CanUnpin(void) const throw()
            { return false; }

    private:
        virtual void CreateListViewColumns(void);
        virtual CscObjIterator *CreateCscObjIterator(const CString& strShare);
};




class DetailsView : public CacheView
{
    public:
        DetailsView(HINSTANCE hInstance, 
                    HWND hwndParent, 
                    const RECT& rc, 
                    const CscObjTree& tree,
                    const CString& strShare)
            : CacheView(hInstance, hwndParent, tree, strShare) {  CreateTheView(rc); }

        virtual void GetViewStateRegValueName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = REGSTR_VAL_VIEWSTATE_DETAILS; }

        virtual bool ExcludeCscObject(const CscObject& object) const throw()
            { return object.IsShare(); }

    private:
        virtual void CreateListViewColumns(void);

};


class StaleView : public CacheView
{
    public:
        StaleView(HINSTANCE hInstance, 
                  HWND hwndParent, 
                  const RECT& rc, 
                  const CscObjTree& tree,
                  const CString& strShare)
            : CacheView(hInstance, hwndParent, tree, strShare) {  CreateTheView(rc); }

        virtual void GetViewStateRegValueName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = REGSTR_VAL_VIEWSTATE_STALE; }

    private:
        virtual void CreateListViewColumns(void);

        virtual bool ExcludeCscObject(const CscObject& object) const throw();
};


class CacheWindow;  // fwd decl.

class CacheWindow
{
    public:
        enum ViewType { eUnknownView = 0, eShareView, eDetailsView, eStaleView };

        explicit CacheWindow(HINSTANCE hInstance);
        ~CacheWindow(VOID);

        bool Run(ViewType eInitialView,  const CString& strInitialShare);

    private:
        class ShareDlg
        {
            public:
                ShareDlg(HINSTANCE hInstance,
                         HWND hwndParent,
                         const CArray<CscShare *>& rgpShares,
                         CString *pstrShare,
                         CString *pstrDisplayName);

                bool Run(void) throw();

            private:
                HINSTANCE m_hInstance;
                HWND      m_hwndParent;
                HWND      m_hwndList;
                CString  *m_pstrShare;
                CString  *m_pstrDisplayName;
                const CArray<CscShare *>& m_rgpShares;

                BOOL OnInitDialog(HWND hwnd);
                void OnOk(void);
                static BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
                static BOOL CALLBACK DlgProcInternal(HWND, UINT, WPARAM, LPARAM);

                //
                // Prevent copy.
                //
                ShareDlg(const ShareDlg& rhs);
                ShareDlg& operator = (const ShareDlg& rhs);
        };

        //
        // Subclass the toolbar window so we can intercept mouse messages
        // and display button help text in the status bar.
        //
        class TBSubclass : public WindowSubclass
        {
            public:
                TBSubclass(CacheWindow *pCacheWindow) throw()
                    : m_iLastBtnHit(-1),
                      m_pCacheWindow(pCacheWindow) { }

                virtual LRESULT HandleMessages(HWND, UINT, WPARAM, LPARAM);

            private:
                CacheWindow *m_pCacheWindow;
                int          m_iLastBtnHit;     // Last toolbar button hit.
                static const UINT ID_TIMER_TBHITTEST;

                int ButtonHitTest(HWND hwnd, const POINT& pt, bool bSetTimer);
        };

        HINSTANCE           m_hInstance;        // Module instance handle.
        CscObjTree         *m_pCscObjTree;      // THE object tree.
        CacheView          *m_pView;            // The current view.
        TBSubclass          m_TBSubclass;       // Subclass the toolbar.
        ProgressBar         m_ProgressBar;      // Progress bar controller.
        HWND                m_hwndMain;         // Main window.
        HWND                m_hwndStatusbar;    // Status bar.
        HWND                m_hwndToolbar;      // Tool bar.
        HWND                m_hwndProgressbar;  // Progress bar contro.
        HWND                m_hwndCombo;        // "Find User" combo box.
        HWND                m_hwndComboLabel;   // "List Shares:" static text.
        HMODULE             m_hmodHtmlHelp;     // Loaded for html help.
        HMENU               m_hmenuMain;        // Window main menu.
        HMENU               m_hmenuShares;      // "Shares" dynamic menu.
        HACCEL              m_hKbdAccel;        // Accelerator table.
        CString             m_strRegKey;        // Name of reg key for saved state.
        CString             m_strCurrentShare;  // Name of current share.
        CString             m_strToolTip;       // Tooltip text string.
        ViewType            m_eCurrentView;     // View to initially display.
        CCriticalSection    m_ViewCs;           // Sync access to view.
        int                 m_iLastColSorted;   // Last column sorted.
        bool                m_bViewPopulated;   // View is fully populated?
        bool                m_bSortAscending;   // Sort ascending?
        bool                m_bMenuActive;
        bool                m_bToolbarVisible;
        bool                m_bStatusbarVisible;
        bool                m_bStatusbarDisabled;
        static const DWORD  WINDOW_STATE_VERSION;
        static const UINT   ID_TIMER_GETNEXTCSCOBJECT;
        static const UINT   ID_TIMER_PROGRESS;
        //
        // Structure of saved window state info.
        //
        struct StateInfo
        {
            int cbSize;
            DWORD dwVersion;
            int cxWindow;
            int cyWindow;
            bool bToolbar;
            bool bStatusbar;
            bool bMaximized;
        };

        struct BrowseForFolderCbkInfo
        {
            const CacheWindow *pThis;  // Ptr to Cache window object.
            CPath *pstrFolder;         // Fill in if entered path doesn't exist.
        };

        bool CreateMainWindow(ViewType eInitialView, const CString& strInitialShare);
        CacheView *CreateView(ViewType eViewType, const CString& strShare);
        bool CreateToolbar(void);
        bool CreateStatusbar(void);
        bool IsValidViewType(ViewType eViewType) const throw();
        void ShowItemCountInStatusbar(void);
        void OnLVItemCountChanged(int cItems);
        void ShowMenuTextInStatusbar(DWORD idMenuOption);
        void ShowToolbarButtonTextInStatusbar(int iBtn);
        void SetStatusbarText(int iPart, const CString &strText) throw();
        void UpdateViewButtons(void) throw();
        void UpdateButtonsAndMenuItems(void);
        void UpdateWindowTitle(void);
        bool SaveWindowState(void) const;
        bool RestoreWindowState(int *pnCmdShow);
        void ConfigureWindowForView(void);
        bool InsertOrRemoveSharesMenu(void);
        bool IsStatusbarVisible(void) const throw();
        void ShowStatusbar(bool bShow);
        bool IsToolbarVisible(void) const throw();
        void ShowToolbar(bool bShow);
        void UpdateWindowLayout(void);
        void FillShareComboAndMenu(bool bFillCombo, bool bFillMenu);
        void ClearSharesMenu(void) throw();
        void ClearSharesCombo(void) throw();
        void SetShareComboSelection(const CString& strShare) throw();
        void SetShareMenuSelection(const CString& strShare);
        void ShareComboSelectionChanged(void);
        void ShareMenuSelectionChanged(int idCmdShare);
        void ShowShareCombo(bool bShow);
        void ShowDebugInfo(void) const;
        int GetShareComboSelection(CString *pstrShare, CString *pstrDisplayName) const;
        bool OnCreate(ViewType eViewType, const CString& strShare);
        LRESULT OnDestroy(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnSize(HWND, UINT, WPARAM, LPARAM) throw();
        LRESULT OnCommand(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnNotify(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnMenuSelect(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnSetFocus(HWND, UINT, WPARAM, LPARAM) throw();
        LRESULT OnContextMenu(HWND, UINT, WPARAM, LPARAM);
        LRESULT OnQueryEndSession(HWND hWnd, WPARAM wParam, LPARAM lParam);
        LRESULT OnLVN_GetDispInfo(LV_DISPINFO *pdi);
        LRESULT OnLVN_ColumnClick(NM_LISTVIEW *pnm);
        LRESULT OnTTN_NeedText(TOOLTIPTEXT *pttt);
        LRESULT OnLVN_ItemChanged(NM_LISTVIEW *pnm);
        LRESULT OnTestGetViewerStatus(void) throw();
        LRESULT OnTestGetItemStatus(int iItem) throw();
        LRESULT Refresh(VOID);
        bool EndIconHoundThread(DWORD dwWait = INFINITE);
        int  GetFirstSelectedItemRect(RECT *prc);
        void InitProgressBarForView(int cUpdateIntervalMs);
        void OnPBN_100Pct(void) throw();
        void SelectAllItems(void) throw();
        void InvertSelectedItems(void) throw();
        void OpenSelectedItem(void);
        void OnGetNextCscObject(void);
        void OnUpdateObject(CscObject *pObject);
        void ViewSelectionChanged(ViewType eViewType, bool bUserSelectedViewItem);
        void ChangeView(ViewType eViewType, const CString& strShare, bool bConfigWindow = true, bool bBeginLoading = true);
        void PinSelectedItems(bool bPin);
        void UpdateSelectedItems(void);
        void DeleteSelectedItems(void);
        void DeleteSelectedItems2(void);
        HRESULT DeleteObject(CscObject *pObject, CProgressDialog& dlgProgress);
        HRESULT DeleteShareObject(CscShare *pShare, CProgressDialog& dlgProgress);
        void EnsureShareIsPopulated(CscShare *pShare);
        void EnsureSharesArePopulated(const CArray<CscShare *>& rgpShares);
        bool ConfirmDeleteItems(void) const;
        void ApplyPolicy(void);
        void ApplyPolicyToFileMenu(HMENU hmenu);
        void ShowHelp(void);
        bool SynchronizeObjects(const CArray<CscObject *>& rgpObj,
                                bool bSyncStaleOnly,
                                BitSet *pbsetStale);

        bool CopySelectedItemsToFolder(void);
        void CopySelectedObject(const CscObject *pObj, 
                                const CString& strPathTo,
                                CopyProgressDialog& dlgProgress,
                                bool *pbYesToAll);
        bool PrepareCopyList(CArray<CscObject *> *prgpObj) const;
        bool ObjectCopyError(HWND hwndParent, 
                             const CString& strName, 
                             DWORD dwWin32Err, 
                             bool bIsFolder);
        bool ConfirmReplaceOnCopy(HWND hwndParent, LPCTSTR pszTo, bool *pbYesToAll);
        bool BrowseForFolder(CPath *pstrFolder) const;
        int OnBrowseForFolderValidateFailed(HWND hwndParent, CPath *pstrFolder, LPCTSTR pszPath) const;
        static bool CreateDirectoryPath(const CPath& path);
        static int CALLBACK BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) throw();

        static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM) throw();
        static LRESULT CALLBACK WndProcInternal(CacheWindow *, HWND, UINT, WPARAM, LPARAM);
        static const int CX_SHARE_COMBO;
        static const int CY_SHARE_COMBO;
        static const int MAX_SHARE_MENU_ITEMS;
        static const int PROGRESSBAR_UPDATE_MS;

        //
        // Prevent copying.
        //
        CacheWindow(const CacheWindow&);
        void operator = (const CacheWindow&);

        friend class TBSubclass;
};


inline void 
CacheWindow::SetStatusbarText(
    int iPart, 
    const CString &strText
    ) throw()
{
    DBGASSERT((NULL != m_hwndStatusbar));
    StatusBar_SetText(m_hwndStatusbar, iPart, 0, (LPCTSTR)strText);
}



#endif // _INC_CSCVIEW_CACHVIEW_H


