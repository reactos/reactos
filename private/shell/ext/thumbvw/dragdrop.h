/* sample source code for extension view mechanism for IE40
 * Copyright Microsoft Corporation 1996
 * 
 * this file contains a tear off interface class for supporting Shell drag-drop
 * behaviour for a generic listview control. */

#ifndef _DRAGDROP_H
#define _DRAGDROP_H

// the scroll direction flags that are passed to PreScrolling()
#define VSCROLLDIR_NONE     0xffff
#define VSCROLLDIR_UP       SB_LINEUP
#define VSCROLLDIR_DOWN     SB_LINEDOWN

#define HSCROLLDIR_NONE     0xffff
#define HSCROLLDIR_LEFT     SB_LINELEFT
#define HSCROLLDIR_RIGHT    SB_LINERIGHT

/////////////////////////////////////////////////////////////////////////////////////
// base class for object that requires used of the CViewDropTarget object...
class CDropTargetClient
{
    public:
        // the dropTarget will notify the client before it starts scrolling
        // in any direction. The two flags determine Up/Down, Right/Left
        virtual void PreScrolling( WORD wVertical, WORD wHorizontal ) = 0;

        virtual void GetOrigin( POINT * prgOrigin ) = 0;

        // this returns the HWND of the listview.
        virtual HWND GetWindow() = 0;

        virtual BOOL WasDragStartedHere() = 0;

        virtual HRESULT MoveSelectedItems( int iDx, int iDy ) = 0;
};

// tear off interface for the window background drop target
class CViewDropTarget : public CComObjectRoot,
                        public IDropTarget
{
    public:
        CViewDropTarget();  
        virtual ~CViewDropTarget();

        HRESULT Init( CDropTargetClient *    pParent,
                      LPSHELLFOLDER          pFolder,
                      HWND                   hwnd );

        BEGIN_COM_MAP( CViewDropTarget )
            COM_INTERFACE_ENTRY( IDropTarget )
        END_COM_MAP( )

        DECLARE_NOT_AGGREGATABLE( CViewDropTarget )

        // IDropTarget methods 
        STDMETHOD( DragEnter )( IDataObject *pDataObj, DWORD grfKeyState, 
            POINTL pt, DWORD *pdwEffect );
        STDMETHOD( DragOver )( DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
        STDMETHOD( DragLeave )( void );
        STDMETHOD( Drop )( IDataObject *pDataObj, DWORD grfKeyState,
            POINTL pt, DWORD *pdwEffect );

        // General support methods ...
        LPDROPTARGET GetBackgrndDT( void );
        void DragStartHere( const POINT * prgStart );
        void DropPoint( POINT * pDrop );
        BOOL DropOnBackGrnd( void );
        
    protected:
        void GetItemUnder( LV_HITTESTINFO * pInfo );
        void CreateDTForItemUnder( LV_HITTESTINFO *pInfo );
        void FocusItem( int iItem, BOOL fFocus );
        DWORD DragDirection( const POINTL * pt );
        BOOL CanScroll(HWND hWnd, int code, BOOL bDown);
        HRESULT MoveSelectedItems( int iDx, int iDy );
        
        LPDROPTARGET m_pCurDT;
        LPDROPTARGET m_pBkgrndDT;
        LPDATAOBJECT m_pDataObj;
        IDropTargetHelper*  m_pDragImages;
        int m_iCurItem;
        int m_iFlags;
        DWORD m_dwDragDropDelay;
        DWORD m_dwScrollFlags;
        DWORD m_dwStartTickCount;
        DWORD m_dwDragDropInset;
        DWORD m_dwDragDropScrollDelay;
        DWORD m_grfKeyState;

        HWND m_hWnd;
        HWND m_hWndListView;
        HWND m_hwndDD;          // draw drag cursors here.
        
        LPSHELLFOLDER m_pFolder;

        // callback needed to be able to deal with the player .....
        CDropTargetClient * m_pParent;

        POINT m_ptDragStart;        // the drag-drop start point (if in this HWND)
        POINT m_ptDragEnd;          // the drag-drop end point (if in this HWND)

        // did the drop occur on the background ?
        BOOL m_fDropOnBack : 1;
};


//////////////////////////////////////////////////////////////////////////////////////
// a tear off interface for implementing the Drag source
class CViewDropSource : public CComObjectRoot,
                        public IDropSource
{
    public:
        CViewDropSource();
        ~CViewDropSource();

        BEGIN_COM_MAP( CViewDropSource )
            COM_INTERFACE_ENTRY( IDropSource )
        END_COM_MAP( )

        DECLARE_NOT_AGGREGATABLE( CViewDropSource )

        STDMETHOD( QueryContinueDrag ) ( BOOL fEscapePressed, DWORD grfKeyState );
        STDMETHOD( GiveFeedback )( DWORD dwEffect );
        
        // member data for remembering the initial key state ...
    protected:
        DWORD m_grfInitialKeyState;
};

#endif

