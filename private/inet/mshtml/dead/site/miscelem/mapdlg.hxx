//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mapdlg.hxx
//
//  Contents:   Definition of the Map Edit Dialog class and all the image
//              map editing code
//
//  Classes:    CMapEditDialog
//
//----------------------------------------------------------------------------

#include "element.hxx"
#include "formkrnl.hxx"
#include "earea.hxx"
#include "emap.hxx"
#include "imgelem.hxx"
#include "csimutil.hxx"



class CMapEditDialog
{
public:

    // Creation/Destruction/Initialization
    CMapEditDialog() {}
    ~CMapEditDialog() { OnClose(); }
    HRESULT Init(IImgElement *pImg);
    HRESULT InitDialog();
    HRESULT InitSize();

    // Parsing/Unparsing through object model
    HRESULT Parse(BSTR bstrCOORDS, BSTR bstrSHAPE, BSTR bstrHREF);
    HRESULT ParseArea(IAreaElement *pArea);
    HRESULT UnParse();

    // Containment/Border checking
    HRESULT CheckSelect(POINT pt);
    HRESULT GetAreaContaining(POINT pt, IAreaElement **ppArea);
    int     PointOnRect(POINT pt);
    int     PointOnCircle(POINT pt);
    int     PointOnPoly(POINT pt);
    int     PointOnBorder(POINT pt);

    // Stuff to be made static

    // Misc. window code
    HRESULT InitScrollBars();
    HDC     GetSetDC();
    HRESULT SetCaption();
    HRESULT DrawCurrent(HDC hDC, RECT rc);      // Make static
    HRESULT DrawGrabHandles(HDC hDC);
    HRESULT UpdateToolbar();
    HRESULT GetViewRect(RECT *prc);

    // Misc. Editing code
    int     StartSize(LONG lx, LONG ly);            // Sets initial size
    HRESULT SizeTo(int nBorder, LONG lx, LONG ly, BOOL fShift);  
    HRESULT MoveTo(LONG lXOff, LONG lYOff);         // Moves area by offset
    HRESULT UpdatePolygon();                        // Updates polygon region
    HRESULT UpdateRectangle();                      // Corrects rectangle 
    HRESULT GetCursorClipRect(RECT *prcClip);
    HRESULT AddCurrent();


    // Message handlers
    HRESULT OnClose();
    HRESULT OnCommand(WORD wNofifyCode, WORD idiCtrl, HWND hwndCtrl);
    HRESULT OnContextMenu(CMessage *pmsg);
    HRESULT OnDelete();
    HRESULT OnDeleteAll();
    HRESULT OnInitMenuPopup(CMessage *pmsg);
    HRESULT OnLButtonDblClk(CMessage *pmsg);
    HRESULT OnLButtonDown(CMessage *pmsg);
    HRESULT OnLButtonUp(CMessage *pmsg);
    HRESULT OnMouseMove(CMessage *pmsg);
    HRESULT OnPaint();
    HRESULT OnRButtonDown(CMessage *pmsg);
    HRESULT OnScroll(CMessage *pmsg, long lPos);
    HRESULT OnSetCursor(CMessage *pmsg);
    HRESULT OnShowProperties();
    HRESULT OnSize(CMessage *pmsg);
    HRESULT OnGetMinMaxInfo(CMessage *pmsg);
    HRESULT OnSysCommand(CMessage *pmsg);

 
    // Dialog procedure
    static BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


    // Editing flags/info
    union CoordinateUnion _coords;
    CPointAry _ptList;
    unsigned  _fCreating:1;
    unsigned  _fhref:1;
    unsigned  _fMoving:1;
    unsigned  _nDrawMode:2;
    unsigned  _nSizing:8;    // Max of 256 points/vertices
    unsigned  _fSelected:1;
    unsigned  _fCookie:1;    // Polygon drawing
    unsigned  _nShapeType:2;
    unsigned  _fNoShape:1;
    // BUGBUG(t-johnha): This should be in the options object
    unsigned  _fAutoURL:1;
    unsigned  _fSBHorz:1;
    unsigned  _fSBVert:1;
    unsigned  _fToolLock:1;

    // Space for frame, caption, toolbar, etc, cached
    // from GetSystemMetrics
    int       _nYFrame;
    int       _nYSpace;
    int       _nXFrame;

    // Important objects to keep track of
    HWND         _hwndDialog;
    RECT         _rcImg;
    RECT         _rcClient;
    RECT         _rcToolbar;
    CImgCtx     *_pImgCtx;
    CDoc        *_pDoc;
    IMapElement *_pMap;
    HRESULT      _hr;

    // Previous window width/height
    int      _nOldWidth;
    int      _nOldHeight;

    // Cached GDI objects
    HPEN    _hpenXOR;   
    HBRUSH  _hbrHollow;
    HBRUSH  _hbrWhite;

    // Menus/toolbar
    HMENU   _hmenuCtx;
    HMENU   _hmenuBar;
    HWND    _hwndToolbar;
    HWND    _hwndHScroll;
    HWND    _hwndVScroll;
    HWND    _hwndSizeGrip;

    // Scrollbar positioning
    SCROLLINFO _siHorz, _siVert;

    // Misc info/pointers
    POINT         _ptMovePoint;
    IAreaElement *_pareaCurrent;
    CAreaElement *_pareaElement;
    int           _nIndex;

};