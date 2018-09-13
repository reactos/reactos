/* sample source code for extension view mechanism for IE40
 * Copyright Microsoft Corporation 1996
 * 
 * This source code demonstrates the mechanism by which an extension view
 * can be added to the IE40 shell so that it acts as a normal view.
 */



#ifndef _THUMBS_H
#define _THUMBS_H

#include <runtask.h>

// index for the first/default column in the details view...
#define DETAILSCOL_DEFAULT     0
#define SZ_DEBUGINI         "ccshell.ini"

#define SZ_DEBUGSECTION     "ThumbVw"
#define SZ_MODULE           "THUMBVW"

#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define INTERNET_MAX_URL_LENGTH         (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)

#define VIEWCLASSNAME           L"ThumbnailVwExtWnd32"
#define REGSTR_THUMBNAILVIEW    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Thumbnail View"
extern HINSTANCE g_hinstDll;
extern UINT g_uiShell32;
extern UINT g_msgMSWheel;

class CThumbnailView: public IShellView2,
                      public IShellFolderView,
                      public IDefViewExtInit2,
                      public IShellChangeNotify,
                      public CDropTargetClient,
                      public CComObjectRoot,
                      public CComCoClass< CThumbnailView, &CLSID_ThumbnailViewExt >

{
    public:
        BEGIN_COM_MAP( CThumbnailView )
            COM_INTERFACE_ENTRY( IShellView2 )
            COM_INTERFACE_ENTRY2( IShellView, IShellView2 )
            COM_INTERFACE_ENTRY( IShellFolderView )
            COM_INTERFACE_ENTRY2( IDefViewExtInit, IDefViewExtInit2 )
            COM_INTERFACE_ENTRY( IDefViewExtInit2 )
            COM_INTERFACE_ENTRY( IShellChangeNotify )
        END_COM_MAP( )

        DECLARE_REGISTRY( CThumbnailView,
                          _T("Shell.ThumbnailView.1"),
                          _T("Shell.ThumbnailView.1"),
                          IDS_THUMBNAILVIEW_DESC,
                          THREADFLAGS_APARTMENT)

        DECLARE_NOT_AGGREGATABLE( CThumbnailView )

        CThumbnailView( void );
        virtual ~CThumbnailView( void );

        ///////////////////CDropTargetClient
        virtual void PreScrolling( WORD wVertical, WORD wHorizontal );
        virtual void GetOrigin( POINT * prgOrigin );
        virtual HWND GetWindow();
        virtual BOOL WasDragStartedHere();
        virtual HRESULT MoveSelectedItems( int iDx, int iDy );

        ////////////////////////// IShellFolderView methods ....
        STDMETHOD(Rearrange) ( LPARAM lParamSort);
        STDMETHOD(GetArrangeParam) ( LPARAM *plParamSort);
        STDMETHOD(ArrangeGrid) (THIS);
        STDMETHOD(AutoArrange) (THIS);
        STDMETHOD(GetAutoArrange) (THIS);
        STDMETHOD(AddObject) ( LPITEMIDLIST pidl, UINT *puItem);
        STDMETHOD(GetObject) ( LPITEMIDLIST *ppidl, UINT uItem);
        STDMETHOD(RemoveObject) ( LPITEMIDLIST pidl, UINT *puItem);
        STDMETHOD(GetObjectCount) ( UINT *puCount);
        STDMETHOD(SetObjectCount) ( UINT uCount, UINT dwFlags);
        STDMETHOD(UpdateObject) ( LPITEMIDLIST pidlOld, LPITEMIDLIST pidlNew,
            UINT *puItem);
        STDMETHOD(RefreshObject) ( LPITEMIDLIST pidl, UINT *puItem);
        STDMETHOD(SetRedraw) ( BOOL bRedraw);
        STDMETHOD(GetSelectedCount) ( UINT *puSelected);
        STDMETHOD(GetSelectedObjects) ( LPCITEMIDLIST **pppidl, UINT *puItems);
        STDMETHOD(IsDropOnSource) ( IDropTarget *pDropTarget);
        STDMETHOD(GetDragPoint) ( POINT *ppt);
        STDMETHOD(GetDropPoint) ( POINT *ppt);
        STDMETHOD(MoveIcons) ( IDataObject *pDataObject);
        STDMETHOD(SetItemPos) ( LPCITEMIDLIST pidl, POINT *ppt);
        STDMETHOD(IsBkDropTarget) ( IDropTarget *pDropTarget);
        STDMETHOD(SetClipboard) ( BOOL bMove);
        STDMETHOD(SetPoints) ( IDataObject *pDataObject);
        STDMETHOD(GetItemSpacing) ( ITEMSPACING *pSpacing);
        STDMETHOD(SetCallback) ( IShellFolderViewCB* pNewCB,
            IShellFolderViewCB** ppOldCB);
        STDMETHOD(Select) ( UINT dwFlags );
        STDMETHOD(QuerySupport) (UINT * pdwSupport );
        STDMETHOD(SetAutomationObject)(IDispatch* pdisp);

        ////////////////////////////// IShellView2
        STDMETHOD(GetWindow) ( HWND * lphwnd);
        STDMETHOD(ContextSensitiveHelp) ( BOOL fEnterMode);
        STDMETHOD(TranslateAccelerator) ( LPMSG lpmsg);
        STDMETHOD(EnableModeless) ( BOOL fEnable);
        STDMETHOD(UIActivate) ( UINT uState);
        STDMETHOD(Refresh) ();
        STDMETHOD(CreateViewWindow)( IShellView  *lpPrevView,
                        LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,
                        RECT * prcView, HWND  *phWnd);
        STDMETHOD(DestroyViewWindow)();
        STDMETHOD(GetCurrentInfo)( LPFOLDERSETTINGS lpfs);
        STDMETHOD(AddPropertySheetPages)( DWORD dwReserved,
                        LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam);
        STDMETHOD(SaveViewState)();
        STDMETHOD(SelectItem)( LPCITEMIDLIST pidlItem, UINT uFlags);
        STDMETHOD(GetItemObject)( UINT uItem, REFIID riid,
                        LPVOID *ppv);
        STDMETHOD(GetView)( SHELLVIEWID* pvid, ULONG uView);
        STDMETHOD(CreateViewWindow2)( LPSV2CVW2_PARAMS lpParams);
        STDMETHOD(HandleRename)( LPCITEMIDLIST pidlNew);
        STDMETHOD(SelectAndPositionItem) (THIS_ LPCITEMIDLIST pidlItem,
            UINT uFlags,POINT* point); 

        ///////////////////////////////// IDefViewExtInit
        STDMETHOD( SetOwnerDetails )( IShellFolder * pSF, DWORD lParam );

        ///////////////////////////////// IDefViewExtInit2
        STDMETHOD( SetViewWindowStyle )( DWORD dwBits, DWORD dwVal);
        STDMETHOD( SetViewWindowBkImage )(LPCWSTR pszImage);
        STDMETHOD( SetViewWindowColors )(COLORREF clrText, COLORREF clrTextBk, COLORREF clrWindow);
        STDMETHOD( IsModal ) ();
        STDMETHOD( AutoAutoArrange ) ( DWORD dwReserved);
        STDMETHOD( SetStatusText )(LPCWSTR pwszStatusText);

        ///////////////////////////////// *** IShellChangeNotify methods ***
        STDMETHOD(OnChange) (LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

        DWORD GetOverlayMask( LPCITEMIDLIST pidl );
        int ViewGetIconIndex( LPCITEMIDLIST pidl );
        void StampIconOnThumbnail( LPCITEMIDLIST pidl, HBITMAP hBmpThumb, BOOL fBorder = FALSE );
        HRESULT CreateDefaultThumbnail( int iIndex, HBITMAP * phBmpThumbnail, BOOL fDrawBorder);
        void DrawShadowBorder(HDC hdc, int x, int y, int dx, int dy);
        int FindItem( LPCITEMIDLIST pidl );
        HRESULT ExtractItem( UINT * puIndex, int iItem, LPCITEMIDLIST pidl, BOOL fBackground, BOOL fForce );
        
    protected:
        //////////////////////////////// private helper methods 

        // empty the view       
        HRESULT CreateBackgroundMenu( LPCONTEXTMENU * ppMenu );
        void SortBy( LPARAM dwArrange, BOOL fAscend = TRUE );
        HRESULT EnumFolder( void );
        void FocusOnSomething( void );
        int AddItem( LPCITEMIDLIST pidl );
        void RegisterWindowClass( void );

        friend LRESULT CALLBACK CThumbnailView_WndProc( HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam );
        LRESULT OnWmCreate( HWND hWnd, LPCREATESTRUCT pCreate );
        LRESULT OnWmDestroy( HWND hWnd );
        LRESULT OnWmSize( HWND hwnd, UINT fFlags, int iWidth, int iHeight );
        LRESULT OnWmClipboardUpdate( void );
        LRESULT OnWmNotify( HWND hWnd, int iID, NMHDR * pHdr );
        LRESULT OnWmActivate( HWND hWnd, int fActive, HWND hWndPrev,BOOL fMinimized );
        LRESULT OnWmContextMenu( HWND hWnd, int iX, int iY );
        LRESULT OnWmMenuSelect( HWND hWnd, WPARAM wParam, LPARAM lParam );
        void UpdateWithoutRefresh();
        void UpdateStatusBar( LPCWSTR pszText = NULL, LPCWSTR pszText2 = NULL );

        // used from other threads 
        void ThreadUpdateStatusBar( UINT idMsg = 0, int idItem = -1 );
        
        LRESULT FetchIcon( LV_DISPINFO * pInfo );
        LRESULT OnLVNEndLabelEdit( NMHDR * pHdr );
        LRESULT OnDefaultAction( BOOL fDoubleClick );
        HRESULT StartDragDrop( NM_LISTVIEW * pNMHdr );
        LRESULT OnInfoTipText(NMLVGETINFOTIPW *plvn);

        LRESULT OnViewGetThumbnail( NMHDR * pHdr );

        HRESULT ViewUpdateThumbnail( LPCITEMIDLIST pidl, LPCWSTR pszPath, HBITMAP hBitmap );
        HRESULT TaskUpdateItem( LPCITEMIDLIST pidl,
                                int iItem,
                                DWORD dwMask,
                                LPCWSTR pszPath,
                                const FILETIME * pftDateStamp,
                                int iThumb,
                                HBITMAP hBitmap );
        void CheckViewOptions( void );

        // this will update the display and optionally add it to the disk cache.
        HRESULT UpdateImageForItem( HBITMAP hImage,
                                    int iItem,
                                    LPCITEMIDLIST pidl,
                                    LPCWSTR pszCache,
                                    LPCWSTR pszFullPath,
                                    const FILETIME * pftDateStamp,
                                    BOOL fCache );

        LRESULT OnCustomDraw( NMCUSTOMDRAW * pNM );

        HRESULT ThreadAddItem( LPCITEMIDLIST pidl );
        HRESULT ThreadDeleteItem( LPCITEMIDLIST pidl );
        HRESULT ThreadEmptyUpdateLists( );
        HRESULT OnWmProcessItems( );

        HRESULT SetPointData( IDataObject * ptdObj, LPCITEMIDLIST * ppPidl, int cidl );
        
        friend class CThumbnailMenu;
        friend class CHandlerTask;
        friend class CDiskCacheTask;
        friend class CExtractTask;
        friend class CExtractImageTask;
        friend class CUpdateDirTask;
        friend class CTestCacheTask;

    protected:
        IShellBrowser * m_pBrowser;
        ICommDlgBrowser * m_pCommDlg;
        IShellView * m_pDefView;
        IShellFolder * m_pFolder;
        IShellFolderViewCB * m_pFolderCB;
        CViewDropTarget * m_pDropTarget;
        IDragSourceHelper*    m_pDragImages;
        IShellIcon * m_pIcon;
        IShellImageStore * m_pDiskCache;
        IImageCache2 * m_pImageCache;
        
        HIMAGELIST m_himlThumbs;

        HPALETTE m_hpal;

        LPSHELLTASKSCHEDULER m_pScheduler;
        
        LPCONTEXTMENU m_pCMCache;         // cached context-menu, used for statusbar
        int m_idCMStartOffset;            // start ID of the cached menu

        HWND m_hWndParent;
        HWND m_hWnd;
        HWND m_hWndListView;

        LPITEMIDLIST m_pidl;
        
        // Keyboard accelerators that are special to this view...
        HACCEL m_hAccel;

        DWORD m_dwRecClrDepth;
        
        // Handle for registering for shell notifications
        ULONG m_ulShellRegId;

        FOLDERSETTINGS m_rgSettings;
        
        // a flag used to determine if we are in "EditLabel" mode, if so, turn off the
        // normal Translate Accelerator behavior (i.e. let the edit Window handle them)
        BOOL m_fTranslateAccel : 1;

        // does the explorer have the tree view on the left ?
        BOOL m_fExploreMode : 1;

        // was drag started here ?
        BOOL m_fDragStarted : 1;
        BOOL m_fItemsMoved : 1;

        BOOL m_fAutoExtract : 1;
        BOOL m_fDrawBorder : 1;
        BOOL m_fIconStamp : 1;

        BOOL m_fDestroying : 1;

        BOOL m_fShowAllObjects : 1;

        BOOL m_fOffline : 1;              // is the browser offline ?

        BOOL m_fTimerActive : 1;

        BOOL m_fUpdateDir : 1;

        BOOL m_fShowCompColor : 1;

       	int m_fSelChanges;
       	
        // start point for drag-drop
        POINT m_ptDragStart;

        // current sorting column....
        int m_iSortBy;

        HIMAGELIST m_hSysLargeImgLst;     // the system iage list we will use to stamp the icons
                                          // onto the thumbnails...
        HIMAGELIST m_hSysSmallImgLst;
        
        int m_iXSizeThumbnail;
        int m_iYSizeThumbnail;

        int m_iMaxCacheSize;
        int m_iLastCtr;

        CRITICAL_SECTION m_csAddLock;
        HDPA m_hAddList;
        HDPA m_hDeleteList;
        LPITEMIDLIST m_pidlRename;

        DWORD m_dwCacheTickCount;
        IDispatch   * m_pAuto;      // to dispatch events for those who need them
};

// start of range is out of the way of any WM_USER messages that the explore might pass to 
// us
#define WM_BASE (WM_USER + 800)

// the window message that is passed to notify us that a SHCHangeNotify() event
// has occurred..
#define WM_CLIPBOARD_CUTCOPY    WM_BASE + 2

// a message that gets sent to the view to get it to do a CustomExtract on 
// an item... wParam is the ListView item to re-extract
#define WM_EXTRACTITEM   WM_BASE + 0x100

// the wPARAM is the listview item number the lParam is the new image index...
#define WM_UPDATEITEMIMAGE  WM_BASE + 0x101

// wParam is the string resource and the lParam is the item number (or -1) 
#define WM_STATUSBARUPDATE  WM_BASE + 0x102

#define WM_VIEWREFRESH      WM_BASE + 0x103

#define WM_PROCESSITEMS     WM_BASE + 0x104

#define WM_HANDLESELCHANGE	WM_BASE + 0x105

#define TIMER_DISKCACHE     0x0001
////////////////////////////////////////////////////////////////////////////////////////////
// IContextMenu wrapper for 
class CThumbnailMenu : public IContextMenu3,
                       public CComObjectRoot
{
    public:
        BEGIN_COM_MAP( CThumbnailMenu )
            COM_INTERFACE_ENTRY( IContextMenu3 )
            COM_INTERFACE_ENTRY( IContextMenu2 )
            COM_INTERFACE_ENTRY( IContextMenu )
        END_COM_MAP( )

        DECLARE_NOT_AGGREGATABLE( CThumbnailMenu )
        
        CThumbnailMenu();
        ~CThumbnailMenu();

        HRESULT Init( CThumbnailView * pView,
                      UINT * prgfFlags,
                      LPCITEMIDLIST * apidl,
                      UINT cidl );
        
        STDMETHOD( QueryContextMenu )( HMENU hmenu,
                                       UINT indexMenu,
                                       UINT idCmdFirst,
                                       UINT idCmdLast,
                                       UINT uFlags );


        STDMETHOD(InvokeCommand)( LPCMINVOKECOMMANDINFO lpici );

        STDMETHOD(GetCommandString)( UINT_PTR idCmd,
                                     UINT uType,
                                     UINT * pwReserved,
                                     LPSTR pszName,
                                     UINT cchMax);
                                     
        STDMETHOD(HandleMenuMsg)( UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam);

        STDMETHOD(HandleMenuMsg2)( UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  LRESULT* plRes);
                                  
    protected:
        LPCITEMIDLIST * m_apidl;
        UINT m_cidl;
        LPCONTEXTMENU m_pMenu;
        LPCONTEXTMENU2 m_pMenu2;
        BOOL m_fCaptureAvail;
        UINT m_wID;
        CThumbnailView * m_pView;
};

/////////////////////////////////////////////////////////////////////////////////
// an object that can be added to the background task scheduler for 
// extracting the thumbnail from cache... (read from disk is slow-ish)
HRESULT CDiskCacheTask_Create( CThumbnailView * pView, 
                               int iItem,
                               LPCITEMIDLIST pidl,
                               LPCWSTR pszCache,
                               LPCWSTR pszPath,
                               const FILETIME * pftDateStamp,
                               LPRUNNABLETASK * ppTask );

class CDiskCacheTask : public IRunnableTask,
                       public CComObjectRoot
{
    public:
        BEGIN_COM_MAP( CDiskCacheTask )
            COM_INTERFACE_ENTRY( IRunnableTask )
        END_COM_MAP( )

        DECLARE_NOT_AGGREGATABLE( CDiskCacheTask )
        
        STDMETHOD (Run)();
        STDMETHOD (Suspend)( void );
        STDMETHOD (Resume)( void );
        STDMETHOD (Kill)( BOOL fWait );
        STDMETHOD_(ULONG, IsRunning)();

    protected:
        CDiskCacheTask();
        ~CDiskCacheTask();

        friend HRESULT CDiskCacheTask_Create( CThumbnailView * pView, 
                                              int iItem,
                                              LPCITEMIDLIST pidl,
                                              LPCWSTR pszCache,
                                              LPCWSTR pszPath,
                                              const FILETIME * pftDateStamp,
                                              LPRUNNABLETASK * ppTask );
        int m_iItem;
        LPCITEMIDLIST m_pidl;
        LONG m_lState;
        CThumbnailView * m_pView;
        WCHAR m_szPath[MAX_PATH];
        WCHAR m_szCache[MAX_PATH];
        FILETIME m_ftDateStamp;
        BOOL m_fNoDateStamp;
};

/////////////////////////////////////////////////////////////////////////////////
// an object that can be added to the background task scheduler for 
// handling SHCNE_UPDATEDIR changes...

HRESULT CUpdateDirTask_Create( CThumbnailView * pView, 
                               LPRUNNABLETASK * ppTask );

class CUpdateDirTask : public IRunnableTask,
                       public CComObjectRoot
{
    public:
        BEGIN_COM_MAP( CUpdateDirTask )
            COM_INTERFACE_ENTRY( IRunnableTask )
        END_COM_MAP( )

        DECLARE_NOT_AGGREGATABLE( CUpdateDirTask )
        
        STDMETHOD (Run)();
        STDMETHOD (Suspend)( void );
        STDMETHOD (Resume)( void );
        STDMETHOD (Kill)( BOOL fWait );
        STDMETHOD_(ULONG, IsRunning)();

    protected:
        CUpdateDirTask();
        ~CUpdateDirTask();
        HRESULT InternalResume();

        friend HRESULT CUpdateDirTask_Create( CThumbnailView * pView, 
                                              LPRUNNABLETASK * ppTask );
        LONG m_lState;
        CThumbnailView * m_pView;
        HANDLE m_hEvent;
        LPITEMIDLIST * m_apElems;
        LPITEMIDLIST * m_apCurBlock;
        ULONG m_celtThisBlock;
        LPENUMIDLIST m_pEnum;
        int m_iIndex;
        BOOL m_fEnumDone;
        BOOL m_fViewPassDone;
        
};

/////////////////////////////////////////////////////////////////////////////////
// an object that is added to figure out what we need to do to extract an item 

HRESULT CTestCacheTask_Create( CThumbnailView * pView,
                               IExtractImage * pExtract,
                               LPCWSTR pszCache,
                               LPCWSTR pszFullPath,
                               const FILETIME * pftTimeStamp,
                               LPCITEMIDLIST pidl,
                               int iItem,
                               DWORD dwFlags,
                               DWORD dwPriority,
                               BOOL fAsync,
                               BOOL fBackground,
                               BOOL fForce,
                               LPRUNNABLETASK * ppTask );

class CTestCacheTask : public CRunnableTask
{
    public:
        STDMETHOD (RunInitRT)();

    protected:
        CTestCacheTask();
        ~CTestCacheTask();

        friend HRESULT CTestCacheTask_Create( CThumbnailView * pView,
                                              IExtractImage * pExtract,
                                              LPCWSTR pszCache,
                                              LPCWSTR pszFullPath,
                                              const FILETIME * pftTimeStamp,
                                              LPCITEMIDLIST pidl,
                                              int iItem,
                                              DWORD dwFlags,
                                              DWORD dwPriority,
                                              BOOL fAsync,
                                              BOOL fBackground,
                                              BOOL fForce,
                                              LPRUNNABLETASK * ppTask );
        LONG m_lState;
        CThumbnailView * m_pView;
        IExtractImage * m_pExtract;
        WCHAR m_szCache[MAX_PATH];
        WCHAR m_szFullPath[MAX_PATH];
        FILETIME m_ftDateStamp;
        BOOL m_fDateStamp;
        LPCITEMIDLIST m_pidl;
        int m_iItem;
        DWORD m_dwFlags;
        DWORD m_dwPriority;
        BOOL m_fAsync;
        BOOL m_fBackground;
        BOOL m_fForce;
};

// PRIORITIES
#define PRIORITY_LOW        ITSAT_DEFAULT_PRIORITY
#define PRIORITY_NORMAL     (PRIORITY_LOW + 1)
#define PRIORITY_HIGH       (PRIORITY_NORMAL + 1)

#define TV_NOBORDER         0x80000000          // no border drawn around thumbvw.
#define TV_NOICONSTAMP      0x40000000          // no app icon stamped on the thumbvw.

// Color stuff
DWORD GetCurrentColorFlags( UINT * puBytesPerPixel );
UINT CalcCacheMaxSize( const SIZE * prgSize, UINT uBytesPerPix );


#define ListView_GetItemWrapW(hwnd, pitem) \
    (BOOL)SendMessageA((hwnd), LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW *)(pitem))

#define ListView_SetItemWrapW(hwnd, pitem) \
    (BOOL)SendMessageA((hwnd), LVM_SETITEMW, 0, (LPARAM)(const LV_ITEMW *)(pitem))

#define ListView_InsertItemWrapW(hwnd, pitem)   \
    (int)SendMessageA((hwnd), LVM_INSERTITEMW, 0, (LPARAM)(const LV_ITEMW *)(pitem))

#define ListView_SetBkImageWrapW(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_SETBKIMAGEW, 0, (LPARAM)plvbki)
   
#endif
