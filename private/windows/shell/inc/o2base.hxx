//+---------------------------------------------------------------------------
//
//  File:       o2base.hxx
//
//  Contents:   Compound Document Object helper library definitions
//
//  Classes:
//              OLEBorder
//              InPlaceBorder
//              StdClassFactory
//              ViewAdviseHolder
//
//              ---
//
//              StatusBar     // Included only if INC_STATUS_BAR defined
//
//  Functions:
//              HimetricFromHPix
//              HimetricFromVPix
//              HPixFromHimetric
//              VPixFromHimetric
//
//              WatchInterface
//
//              OleAllocMem
//              OleFreeMem
//              OleAllocString
//              OleFreeString
//
//              IsCompatibleOleVersion
//              IsDraggingDistance
//
//              TraceIID
//              TraceHRESULT
//
//              RECTtoRECTL
//              RECTLtoRECT
//              ProportionateRectl
//              RectToScreen
//              RectToClient
//              IsRectInRect
//              lMulDiv
//
//              LeadResourceData
//              GetChildWindowRect
//              SizeClientRect
//
//              RegisterOleClipFormats
//              IsCompatibleDevice
//              IsCompatibleFormat
//              FindCompatibleFormat
//              GetObjectDescriptor
//              UpdateObjectDescriptor
//              DrawMetafile
//
//              InsertServerMenus
//              RemoteServerMenus
//
//              RegisterAsRunning
//              RevokeAsRunning
//
//              CreateStreamOnFile
//
//              CreateOLEVERBEnum
//              CreateFORMATETCEnum
//              CreateStaticEnum
//
//              GetMonikerDisplayName
//              CreateStorageOnHGlobal
//              ConvertToMemoryStream
//              StmReadString
//              StmWriteString
//
//              CreateViewAdviseHolder
//
//            ---
//
//  Macros:     WATCHINTERFACE
//              IsSameIID
//
//              TaskAllocMem
//              TaskFreeMem
//              TaskAllocString
//              TaskFreeString
//
//              OK
//              NOTOK
//
//              DECLARE_IUNKNOWN_METHODS
//              DECLARE_PURE_IUNKNOWN_METHODS
//              DECLARE_STANDARD_IUNKNOWN
//              IMPLEMENT_STANDARD_IUNKNOWN
//              DECLARE_DELEGATING_IUNKNOWN
//              IMPLEMENT_DELEGATING_IUNKNOWN
//              DECLARE_PRIVATE_IUNKNOWN
//              IMPLEMENT_PRIVATE_IUNKNOWN
//
//              DECLARE_CODE_TIMER
//              IMPLEMENT_CODE_TIMER
//              START_CODE_TIMER
//              STOP_CODE_TIMER
//
//  Enums:
//              OLE_SERVER_STATE
//
//              OBPARTS
//              OLECLIPFORMAT
//
//                ---
//
//  Notes:      Define INC_STATUS_BAR to include the status bar
//              class definition.
//
//----------------------------------------------------------------------------

#ifndef _O2BASE_HXX_
#define _O2BASE_HXX_

//  resource ID offsets for class descriptor information
#define IDOFF_CLASSID         0
#define IDOFF_USERTYPEFULL    1
#define IDOFF_USERTYPESHORT   2
#define IDOFF_USERTYPEAPP     3
#define IDOFF_DOCFEXT         5
#define IDOFF_ICON            10
#define IDOFF_ACCELS          11
#define IDOFF_MENU            12
#define IDOFF_MGW             13
#define IDOFF_MISCSTATUS      14

#ifndef RC_INVOKED     // the resource compiler is not interested in the rest


#if DBG

extern "C" void FAR PASCAL AssertSFL(
        LPSTR lpszClause,
        LPSTR lpszFileName,
        int nLine);
#define Assert(f) ((f)? (void)0 : AssertSFL(#f, __FILE__, __LINE__))

#else   // !DBG

#define Assert(x)

#endif // DBG


//+---------------------------------------------------------------------
//
//  Windows helper functions
//
//----------------------------------------------------------------------

LPVOID LoadResourceData(HINSTANCE hinst,
        LPCWSTR lpstrId,
        LPVOID lpvBuf,
        int cbBuf);
void GetChildWindowRect(HWND hwndChild, LPRECT lprect);
void SizeClientRect(HWND hwnd, RECT& rc, BOOL fMove);


//+---------------------------------------------------------------------
//
//   Generally useful #defines and inline functions for OLE2.
//
//------------------------------------------------------------------------

// this macro can be used to put string constants in a read-only code segment
// usage:   char CODE_BASED szFoo[] = "Bar";
#define CODE_BASED __based(__segname("_CODE"))

// These are the major and minor version returned by OleBuildVersion
#define OLE_MAJ_VER 0x0003
#define OLE_MIN_VER 0x003A


//---------------------------------------------------------------
//  SCODE and HRESULT macros
//---------------------------------------------------------------

#define OK(r)       (SUCCEEDED(r))
#define NOTOK(r)    (FAILED(r))

//---------------------------------------------------------------
//  GUIDs, CLSIDs, IIDs
//---------------------------------------------------------------

#define IsSameIID(iid1, iid2)   ((iid1)==(iid2))

//---------------------------------------------------------------
//  IUnknown
//---------------------------------------------------------------

//
// This declares the set of IUnknown methods and is for general-purpose
// use inside classes that inherit from IUnknown
#define DECLARE_IUNKNOWN_METHODS        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj); \
    STDMETHOD_(ULONG,AddRef) (void);    \
    STDMETHOD_(ULONG,Release) (void)

//
// This declares the set of IUnknown methods as pure virtual methods
//
#define DECLARE_PURE_IUNKNOWN_METHODS        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj) = 0; \
    STDMETHOD_(ULONG,AddRef) (void) = 0;    \
    STDMETHOD_(ULONG,Release) (void) = 0

//
// This is for use in declaring non-aggregatable objects.  It declares the
// IUnknown methods and reference counter, _ulRefs.
// _ulRefs should be initialized to 1 in the constructor of the object
#define DECLARE_STANDARD_IUNKNOWN(cls)    \
    DECLARE_IUNKNOWN_METHODS;               \
    ULONG _ulRefs

// note:    this does NOT implement QueryInterface, which must be
//          implemented by each object
#define IMPLEMENT_STANDARD_IUNKNOWN(cls) \
    STDMETHODIMP_(ULONG) cls##::AddRef(void)                \
    { ++_ulRefs;                                            \
      return _ulRefs;}                                      \
    STDMETHODIMP_(ULONG) cls##::Release(void)               \
    { ULONG ulRet = --_ulRefs;                              \
      if (ulRet == 0) delete this;                          \
      return ulRet; }

// This is for use in declaring aggregatable objects.  It declares the IUnknown
// methods and a member pointer to the aggregate controlling unknown, _pUnkOuter,
// that all IUnknowns delegate to except the controlling unknown of the object
// itself.
// _pUnkOuter must be initialized to point to either an external controlling
// unknown or the object's own controlling unknown, depending on whether the
// object is being created as part of an aggregate or not.
#define DECLARE_DELEGATING_IUNKNOWN(cls)    \
    DECLARE_IUNKNOWN_METHODS;               \
    LPUNKNOWN _pUnkOuter

// This, correspondingly, is for use in implementing aggregatable objects.
// It implements the IUnknown methods by trivially delegating to the controlling
// unknown described by _pUnkOuter.
#define IMPLEMENT_DELEGATING_IUNKNOWN(cls)  \
    STDMETHODIMP cls##::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)    \
    { return _pUnkOuter->QueryInterface(riid, ppvObj); }                    \
    STDMETHODIMP_(ULONG) cls##::AddRef (void)                               \
    { return _pUnkOuter->AddRef(); }                                        \
    STDMETHODIMP_(ULONG) cls##::Release (void)                              \
    { return _pUnkOuter->Release(); }

// This declares a nested class that is the private unknown of the object
#define DECLARE_PRIVATE_IUNKNOWN(cls)   \
    class PrivateUnknown: public IUnknown { \
    public:                                     \
        PrivateUnknown(cls* p##cls);        \
        DECLARE_IUNKNOWN_METHODS;               \
    private:                                    \
        ULONG _ulRefs;                          \
        cls* _p##cls; };                        \
    friend class PrivateUnknown;            \
    PrivateUnknown _PrivUnk


//
// note:    this does NOT implement QueryInterface, which must be
//          implemented by each object
#define IMPLEMENT_PRIVATE_IUNKNOWN(cls) \
    cls##::PrivateUnknown::PrivateUnknown(cls* p##cls)      \
    { _p##cls = p##cls; _ulRefs = 1; }                              \
    STDMETHODIMP_(ULONG) cls##::PrivateUnknown::AddRef(void)    \
    { ++_ulRefs;                                                    \
      return _ulRefs; }                                             \
    STDMETHODIMP_(ULONG) cls##::PrivateUnknown::Release(void)   \
    { ULONG ulRet = --_ulRefs;                                      \
      if (ulRet == 0) delete _p##cls;                               \
      return ulRet; }



//+---------------------------------------------------------------------
//
//  Miscellaneous useful OLE helper and debugging functions
//
//----------------------------------------------------------------------

//
//  Some convenient OLE-related definitions and declarations
//

typedef  unsigned short far * LPUSHORT;

//REVIEW we are experimenting with a non-standard OLEMISC flag.
#define OLEMISC_STREAMABLE 1024

IsCompatibleOleVersion(WORD wMaj, WORD wMin);


inline BOOL IsDraggingDistance(POINT pt1, POINT pt2)
{
#define MIN_DRAG_DIST 12
    return (abs(pt1.x - pt2.x) >= MIN_DRAG_DIST ||
            abs(pt1.y - pt2.y) >= MIN_DRAG_DIST);
#undef MIN_DRAG_DIST
}

#if ENABLED_DBG == 1

void TraceIID(REFIID riid);
HRESULT TraceHRESULT(HRESULT r);

#define TRACEIID(iid) TraceIID(iid)
#define TRACEHRESULT(r) TraceHRESULT(r)


#else   // DBG == 0

#define TRACEIID(iid)
#define TRACEHRESULT(r)

#endif  // DBG

//+---------------------------------------------------------------------
//
//  Routines to convert Pixels to Himetric and vice versa
//
//----------------------------------------------------------------------

long HimetricFromHPix(int iPix);
long HimetricFromVPix(int iPix);
int HPixFromHimetric(long lHi);
int VPixFromHimetric(long lHi);


//+---------------------------------------------------------------------
//
//   Timing helpers
//
//------------------------------------------------------------------------

#ifdef _TIMING

#define DECLARE_CODE_TIMER(t)   extern CTimer t
#define IMPLEMENT_CODE_TIMER(t,s) CTimer t(s)
#define START_CODE_TIMER(t)     t.Start()
#define STOP_CODE_TIMER(t)      t.Stop()

#else   // !_TIMING

#define DECLARE_CODE_TIMER(t)
#define IMPLEMENT_CODE_TIMER(t,s)
#define START_CODE_TIMER(t)
#define STOP_CODE_TIMER(t)

#endif  // _TIMING

//+---------------------------------------------------------------------
//
//  Rectangle helper functions
//
//----------------------------------------------------------------------

//+---------------------------------------------------------------
//
//  Function:   RECTtoRECTL
//
//  Synopsis:   Converts a RECT structure to a RECTL
//
//----------------------------------------------------------------

inline void RECTtoRECTL(RECT& rc, LPRECTL lprcl)
{
    lprcl->left = (long)rc.left;
    lprcl->top = (long)rc.top;
    lprcl->bottom = (long)rc.bottom;
    lprcl->right = (long)rc.right;
}

//+---------------------------------------------------------------
//
//  Function:   RECTLtoRECT
//
//  Synopsis:   Converts a RECTL structure to a RECT
//
//----------------------------------------------------------------

inline void RECTLtoRECT(RECTL& rcl, LPRECT lprc)
{
    lprc->left = (int)rcl.left;
    lprc->top = (int)rcl.top;
    lprc->bottom = (int)rcl.bottom;
    lprc->right = (int)rcl.right;
}

//+---------------------------------------------------------------
//
//  Function:   lMulDiv, private
//
//  Synopsis:   Does a long MulDiv operation
//
//----------------------------------------------------------------

inline long lMulDiv(long lMultiplicand, long lMultiplier, long lDivisor)
{
    return (lMultiplicand * lMultiplier)/lDivisor;  //BUGBUG 64bit intermediate?
}

//+---------------------------------------------------------------
//
//  Function:   ProportionateRectl
//
//  Synopsis:   Calculates rectangle A that is proportionate to rectangle B
//              as rectangle C is to rectangle D,
//              i.e. determine A such that A's relation to B is the same as
//              C's relation to D.
//
//  Arguments:  [lprclA] -- rectangle A to be computed
//              [rclB] -- rectangle B
//              [rclC] -- rectangle C
//              [rclD] -- rectangle D
//
//----------------------------------------------------------------

inline void ProportionateRectl(LPRECTL lprclA, RECTL& rclB,
                                            RECTL& rclC, RECTL& rclD)
{
    // start with rectangle C
    *lprclA = rclC;

    // translate it so the UL corner of D is at the origin
    lprclA->left -= rclD.left;
    lprclA->top -= rclD.top;
    lprclA->right -= rclD.left;
    lprclA->bottom -= rclD.top;

    // scale it by the ratio of the size of B to D (each axis independently)
    SIZEL sizelB = { rclB.right - rclB.left, rclB.bottom - rclB.top };
    SIZEL sizelD = { rclD.right - rclD.left, rclD.bottom - rclD.top };
    lprclA->left = lMulDiv(lprclA->left, sizelB.cx, sizelD.cx);
    lprclA->top = lMulDiv(lprclA->top, sizelB.cy, sizelD.cy);
    lprclA->right = lMulDiv(lprclA->right, sizelB.cx, sizelD.cx);
    lprclA->bottom = lMulDiv(lprclA->bottom, sizelB.cy, sizelD.cy);

    // translate it to the coordinates represented by B
    lprclA->left += rclB.left;
    lprclA->top += rclB.top;
    lprclA->right += rclB.left;
    lprclA->bottom += rclB.top;
}

//+---------------------------------------------------------------
//
//  Function:   RectToScreen
//
//  Synopsis:   Converts a rectangle in client coordinates of a window
//              to screen coordinates.
//
//  Arguments:  [hwnd] -- the window defining the client coordinate space
//              [lprect] -- the rectangle to be converted
//
//----------------------------------------------------------------

inline void RectToScreen(HWND hwnd, LPRECT lprect)
{
    POINT ptUL = { lprect->left, lprect->top };
    ClientToScreen(hwnd,&ptUL);
    POINT ptLR = { lprect->right, lprect->bottom };
    ClientToScreen(hwnd,&ptLR);
    lprect->left = ptUL.x;
    lprect->top = ptUL.y;
    lprect->right = ptLR.x;
    lprect->bottom = ptLR.y;
}

//+---------------------------------------------------------------
//
//  Function:   RectToClient
//
//  Synopsis:   Converts a rectangle in screen coordinates to client
//              coordinates of a window.
//
//  Arguments:  [hwnd] -- the window defining the client coordinate space
//              [lprect] -- the rectangle to be converted
//
//----------------------------------------------------------------

inline void RectToClient(HWND hwnd, LPRECT lprect)
{
    POINT ptUL = { lprect->left, lprect->top };
    ScreenToClient(hwnd,&ptUL);
    POINT ptLR = { lprect->right, lprect->bottom };
    ScreenToClient(hwnd,&ptLR);
    lprect->left = ptUL.x;
    lprect->top = ptUL.y;
    lprect->right = ptLR.x;
    lprect->bottom = ptLR.y;
}

//+---------------------------------------------------------------
//
//  Function:   IsRectInRect
//
//  Synopsis:   Determines whether one rectangle is wholly contained within
//              another rectangle.
//
//  Arguments:  [rcOuter] -- the containing rectangle
//              [lprect] -- the contained rectangle
//
//----------------------------------------------------------------

inline BOOL IsRectInRect(RECT& rcOuter, RECT& rcInner)
{
    POINT pt1 = { rcInner.left, rcInner.top };
    POINT pt2 = { rcInner.right, rcInner.bottom };
    return PtInRect(&rcOuter, pt1) && PtInRect(&rcOuter, pt2);
}


//+---------------------------------------------------------------------
//
//  IMalloc-related helpers
//
//----------------------------------------------------------------------


//REVIEW: We may want to cache the IMalloc pointer for efficiency
#ifdef WIN16
//
//  C++ new/delete replacements that use OLE's allocators
//

void FAR* operator new(size_t size);
void FAR* operator new(size_t size, MEMCTX memctx);
void operator delete(void FAR* lpv);

#endif //WIN16

//
//  inline IMalloc memory allocation functions
//

HRESULT OleAllocMem(MEMCTX ctx, ULONG cb, LPVOID FAR* ppv);
void OleFreeMem(MEMCTX ctx, LPVOID pv);
HRESULT OleAllocString(MEMCTX ctx, LPCWSTR lpstrSrc, LPWSTR FAR* ppstr);
void OleFreeString(MEMCTX ctx, LPWSTR lpstr);

#define TaskAllocMem(cb, ppv)    OleAllocMem(MEMCTX_TASK, cb, ppv)
#define TaskFreeMem(pv)     OleFreeMem(MEMCTX_TASK, pv)
#define TaskAllocString(lpstr, ppstr) OleAllocString(MEMCTX_TASK, lpstr, ppstr)
#define TaskFreeString(lpstr) OleFreeString(MEMCTX_TASK, lpstr)



//+---------------------------------------------------------------------
//
//   Border definitions and helper class
//
//------------------------------------------------------------------------


// Default value for border thickness unless over-ridden via SetThickness
#define FBORDER_THICKNESS       4

// Default values for border minimums (customize via SetMinimums)
#define FBORDER_MINHEIGHT   (FBORDER_THICKNESS*2 + 8);
#define FBORDER_MINWIDTH    (FBORDER_THICKNESS*2 + 8);

#define OBSTYLE_MODMASK         0xff00  /* Mask style modifier bits */
#define OBSTYLE_TYPEMASK        0x00ff  /* Mask basic type definition bits */

#define OBSTYLE_RESERVED        0x8000  /* bit reserved for internal use */
#define OBSTYLE_INSIDE          0x4000  /* Inside or Outside rect? */
#define OBSTYLE_HANDLED         0x2000  /* Size Handles Drawn? */
#define OBSTYLE_ACTIVE          0x1000  /* Active Border Shading? */
#define OBSTYLE_XOR             0x0800  /* Draw with XOR? */
#define OBSTYLE_THICK           0x0400  /* double up lines? */

#define OBSTYLE_DIAGONAL_FILL   0x0001  /* Open editing */
#define OBSTYLE_SOLID_PEN       0x0002  /* Simple Outline */

#define FB_HANDLED  (OBSTYLE_HANDLED | OBSTYLE_SOLID_PEN)
#define FB_OPEN OBSTYLE_DIAGONAL_FILL
#define FB_OUTLINED OBSTYLE_SOLID_PEN
#define FB_HIDDEN 0

#define MAX_OBPART 13

    enum OBPARTS
    {
        BP_NOWHERE,
        BP_TOP, BP_RIGHT, BP_BOTTOM, BP_LEFT,
        BP_TOPRIGHT, BP_BOTTOMRIGHT, BP_BOTTOMLEFT, BP_TOPLEFT,
        BP_TOPHAND, BP_RIGHTHAND, BP_BOTTOMHAND, BP_LEFTHAND,
        BP_INSIDE
    };


class OLEBorder
{
public:
    OLEBorder(RECT& r);
    OLEBorder(void);
    ~OLEBorder(void);

    void SetMinimums( SHORT sMinHeight, SHORT sMinWidth );
    int SetThickness(int sBorderThickness);
    int GetThickness(void);

    USHORT SetState(HDC hdc, HWND hwnd, USHORT usBorderState);
    USHORT GetState(void);

    void Draw(HDC hdc, HWND hwnd);
    void Erase(HWND hwnd);

    USHORT QueryHit(POINT point);

    HCURSOR MapPartToCursor(USHORT usPart);

    HCURSOR QueryMoveCursor(POINT ptCurrent, BOOL fMustMove);
    HCURSOR BeginMove(HDC hdc, HWND hwnd, POINT ptStart, BOOL fMustMove);
    RECT& UpdateMove(HDC hdc, HWND hwnd, POINT ptCurrent, BOOL fNewRegion);
    RECT& EndMove(HDC hdc, HWND hwnd, POINT ptCurrent, USHORT usBorderState);

    void SwitchCoords( HWND hwndFrom, HWND hwndTo );

    //
    //OLEBorder exposes it's RECT as a public data member
    //
    RECT rect;

private:
    enum ICURS
    {
        ICURS_STD,
        ICURS_NWSE,
        ICURS_NESW,
        ICURS_NS,
        ICURS_WE
    };
    static BOOL fInit;
    static HCURSOR ahc[5];
    static int iPartMap[14];

    void InitClass(void);
    void GetBorderRect(RECT& rDest, int iEdge);
    void GetInsideBorder(RECT& rDest, int iEdge);
    void GetOutsideBorder(RECT& rDest, int iEdge);

    USHORT _state;
    int _sMinHeight;
    int _sMinWidth;
    int _sThickness;
    USHORT _usPart;         //which portion of border was hit?
    POINT _pt;              //last known point
    BOOL _fErased;
};


inline void OLEBorder::SetMinimums( SHORT sMinHeight, SHORT sMinWidth )
{
    _sMinHeight = sMinHeight;
    _sMinWidth = sMinWidth;
}

inline int OLEBorder::GetThickness(void)
{
    return(_sThickness);
}

inline int OLEBorder::SetThickness(int sBorderThickness)
{
    if(sBorderThickness > 0)
    {
        _sThickness = sBorderThickness;
    }
    return(_sThickness);
}

inline USHORT OLEBorder::GetState(void)
{
    return(_state);
}

//+---------------------------------------------------------------------
//
//  Helper functions for implementing IDataObject and IViewObject
//
//----------------------------------------------------------------------

//
//  Useful #defines
//

#define DVASPECT_ALL     \
        DVASPECT_CONTENT|DVASPECT_THUMBNAIL|DVASPECT_ICON|DVASPECT_DOCPRINT


//
//  Standard OLE Clipboard formats
//

enum OLECLIPFORMAT
{
    OCF_OBJECTLINK,
    OCF_OWNERLINK,
    OCF_NATIVE,
    OCF_FILENAME,
    OCF_NETWORKNAME,
    OCF_DATAOBJECT,
    OCF_EMBEDDEDOBJECT,
    OCF_EMBEDSOURCE,
    OCF_LINKSOURCE,
    OCF_LINKSRCDESCRIPTOR,
    OCF_OBJECTDESCRIPTOR,
    OCF_OLEDRAW,            OCF_LAST = OCF_OLEDRAW
};

extern UINT OleClipFormat[OCF_LAST+1];  // array of OLE standard clipboard formats
                                        // indexed by OLECLIPFORMAT enumeration.

void RegisterOleClipFormats(void);      // initializes OleClipFormat table.

//
//  FORMATETC helpers
//

BOOL IsCompatibleDevice(DVTARGETDEVICE FAR* ptdLeft, DVTARGETDEVICE FAR* ptdRight);
BOOL IsCompatibleFormat(FORMATETC& f1, FORMATETC& f2);
int FindCompatibleFormat(FORMATETC FmtTable[], int iSize, FORMATETC& formatetc);

//
//  OBJECTDESCRIPTOR clipboard format helpers
//

HRESULT GetObjectDescriptor(LPDATAOBJECT pDataObj, LPOBJECTDESCRIPTOR pDescOut);
HRESULT UpdateObjectDescriptor(LPDATAOBJECT pDataObj, POINTL& ptl, DWORD dwAspect);

//
//  Other helper functions
//

HRESULT DrawMetafile(LPVIEWOBJECT pVwObj, RECT& rc, DWORD dwAspect,
                                                    HMETAFILE FAR* pHMF);
//+---------------------------------------------------------------------
//
//  IStream on top of a DOS (non-docfile) file
//
//----------------------------------------------------------------------

HRESULT CreateStreamOnFile(LPCSTR lpstrFile, DWORD stgm, LPSTREAM FAR* ppstrm);

//+---------------------------------------------------------------------
//
//  Class:      InPlaceBorder Class (IP)
//
//  Synopsis:   Helper Class to draw inplace activation borders around an
//              object.
//
//  Notes:      Use of this class limits windows to the use of the
//              non-client region for UIAtive borders only: Standard
//              (non-control window) scroll bars are specifically NOT
//              supported.
//
//  History:    14-May-93   CliffG        Created.
//
//------------------------------------------------------------------------


#define IPBORDER_THICKNESS  6


class InPlaceBorder
{
public:
    InPlaceBorder(void);
    ~InPlaceBorder(void);

    //
    //Substitute for standard windows API: identical signature & semantics
    //
    LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    //
    //Change the server state: reflected in the nonclient window border
    //
    void SetUIActive(BOOL fUIActive);
    BOOL GetUIActive(void);
    //
    //Force border state to OS_LOADED, managing the cooresponding change
    //in border appearance: guaranteed redraw
    //
    void Erase(void);

    void SetBorderSize( int cx, int cy );
    void GetBorderSize( LPINT pcx, LPINT pcy );
    void Bind(LPOLEINPLACESITE pSite, HWND hwnd, BOOL fUIActive );
    void Attach( HWND hwnd, BOOL fUIActive );
    void Detach(void);
    void SetSize(HWND hwnd, RECT& rc);
    void SetParentActive( BOOL fActive );

private:
    BOOL _fUIActive;                //current state of border
    BOOL _fParentActive;            //shade as active border?
    HWND _hwnd;                     //attached window (if any)
    LPOLEINPLACESITE _pSite;        //InPlace site we are bound to
    int _cxFrame;                   //border horizontal thickness
    int _cyFrame;                   //border vertical thickness
    int _cResizing;                 //reentrancy control flag

    static WORD _cUsage;            //refcount for static resources
    static HBRUSH _hbrActiveCaption;
    static HBRUSH _hbrInActiveCaption;

    void DrawFrame(HWND hwnd);
    void CalcClientRect(HWND hwnd, LPRECT lprc);
    LONG HitTest(HWND hwnd, int x, int y);
    void InvalidateFrame(void);
    void RedrawFrame(void);
};

inline void
InPlaceBorder::Detach(void)
{
    _pSite = NULL;
    _hwnd = NULL;
}

inline void
InPlaceBorder::InvalidateFrame(void)
{
    //cause a WM_NCCALCRECT to be generated
    if(_hwnd != NULL)
    {
        ++_cResizing;
        SetWindowPos( _hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED |
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
        _cResizing--;
    }
}

inline void
InPlaceBorder::RedrawFrame(void)
{
    if(_hwnd != NULL)
    {
        ++_cResizing;
        UINT afuRedraw = RDW_INVALIDATE | RDW_UPDATENOW;
        RedrawWindow(_hwnd, NULL, NULL, afuRedraw);
        _cResizing--;
    }
}

inline void
InPlaceBorder::SetParentActive( BOOL fActive )
{
    _fParentActive = fActive;
    RedrawFrame();
}

inline void
InPlaceBorder::CalcClientRect(HWND hwnd, LPRECT lprc)
{
    if(_fUIActive)
        InflateRect(lprc, -_cxFrame, -_cyFrame);
}

inline BOOL
InPlaceBorder::GetUIActive(void)
{
    return _fUIActive;
}

//+---------------------------------------------------------------------
//
//  Helper functions for in-place activation
//
//----------------------------------------------------------------------

HRESULT InsertServerMenus(HMENU hmenuShared, HMENU hmenuObject,
                        LPOLEMENUGROUPWIDTHS lpmgw);

void RemoveServerMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpmgw);



//---------------------------------------------------------------
//  IStorage
//---------------------------------------------------------------

#define STGM_SHARE 0x000000F0


//+---------------------------------------------------------------------
//
//  Running Object Table helper functions
//
//----------------------------------------------------------------------

void RegisterAsRunning(LPUNKNOWN lpUnk, LPMONIKER lpmkFull,
                                            DWORD FAR* lpdwRegister);
void RevokeAsRunning(DWORD FAR* lpdwRegister);


//+---------------------------------------------------------------------
//
//  Standard implementations of common enumerators
//
//----------------------------------------------------------------------

HRESULT CreateOLEVERBEnum(LPOLEVERB pVerbs, ULONG cVerbs,
                                                      LPENUMOLEVERB FAR* ppenum);

HRESULT CreateFORMATETCEnum(LPFORMATETC pFormats, ULONG cFormats,
                                                      LPENUMFORMATETC FAR* ppenum);

#if 0   // currently not used but useful in the future.
        // we ifdef it out so it doesn't take up space
        // in our servers.
HRESULT CreateStaticEnum(REFIID riid, LPVOID pStart, ULONG cSize, ULONG cCount,
                                                            LPVOID FAR* ppenum);
#endif  //0

//+---------------------------------------------------------------------
//
//  Standard IClassFactory implementation
//
//----------------------------------------------------------------------

//+---------------------------------------------------------------
//
//  Class:      StdClassFactory
//
//  Purpose:    Standard implementation of a class factory object
//
//  Notes:      *
//
//---------------------------------------------------------------

class StdClassFactory: public IClassFactory
{
public:
    StdClassFactory(void);
    DECLARE_IUNKNOWN_METHODS;
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID iid,
                              LPVOID FAR* ppv) PURE;
    STDMETHOD(LockServer) (BOOL fLock);

    BOOL CanUnload(void);
private:
    ULONG _ulRefs;
    ULONG _ulLocks;
};

//+---------------------------------------------------------------------
//
//  IStorage and IStream Helper functions
//
//----------------------------------------------------------------------

#define STGM_DFRALL (STGM_READWRITE|STGM_TRANSACTED|STGM_SHARE_DENY_WRITE)
#define STGM_DFALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE)
#define STGM_SALL  (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)
#define STGM_SRO  (STGM_READ | STGM_SHARE_EXCLUSIVE)

HRESULT GetMonikerDisplayName(LPMONIKER pmk, LPWSTR FAR* ppstr);
HRESULT CreateStorageOnHGlobal(HGLOBAL hgbl, LPSTORAGE FAR* ppStg);
LPSTREAM ConvertToMemoryStream(LPSTREAM pStrmFrom);

//+---------------------------------------------------------------
//
//  Function:   StmReadString
//
//  Synopsis:   Reads a string from a stream
//
//  Arguments:  [pStrm] -- the stream to read from
//              [ppstr] -- where the string read is returned
//
//  Returns:    Success if the string was read successfully
//
//  Notes:      The string is allocated with the task allocator
//              and needs to be freed by the same.
//              This is an inline function.
//
//----------------------------------------------------------------

inline HRESULT StmReadString(LPSTREAM pStrm, LPSTR FAR *ppstr)
{
    HRESULT r;
    USHORT cb;
    LPSTR lpstr = NULL;
    if (OK(r = pStrm->Read(&cb, sizeof(cb), NULL)))
    {
        if (OK(r = TaskAllocMem(cb+1, (LPVOID FAR*)&lpstr)))
        {
            r = pStrm->Read(lpstr, cb, NULL);
            *(lpstr+cb) = '\0';
        }
    }
    *ppstr = lpstr;
    return r;
}

//+---------------------------------------------------------------
//
//  Function:   StmWriteString
//
//  Synopsis:   Writes a string to a stream
//
//  Arguments:  [pStrm] -- the stream to write to
//              [lpstr] -- the string to write
//
//  Returns:    Success iff the string was written successfully
//
//  Notes:      This is an inline function.
//
//----------------------------------------------------------------

inline HRESULT StmWriteString(LPSTREAM pStrm, LPSTR lpstr)
{
    HRESULT r;
    USHORT cb = strlen(lpstr);
    if (OK(r = pStrm->Write(&cb, sizeof(cb), NULL)))
        r = pStrm->Write(lpstr, cb, NULL);
    return r;
}

//+---------------------------------------------------------------------
//
//  View advise holder
//
//----------------------------------------------------------------------

//
//  forward declaration
//

class ViewAdviseHolder;
typedef ViewAdviseHolder FAR* LPVIEWADVISEHOLDER;

//+---------------------------------------------------------------
//
//  Class:      ViewAdviseHolder
//
//  Purpose:    Manages the view advises on behalf of a IViewObject object
//
//  Notes:      This is analogous to the standard DataAdviseHolder provided
//              by OLE.  c.f. CreateViewAdviseHolder.
//
//---------------------------------------------------------------

class ViewAdviseHolder: public IUnknown
{
    friend HRESULT CreateViewAdviseHolder(LPVIEWADVISEHOLDER FAR*);
public:
    //*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    //*** ViewAdviseHolder methods
    STDMETHOD(SetAdvise) (DWORD aspects, DWORD advf, LPADVISESINK pAdvSink);
    STDMETHOD(GetAdvise) (DWORD FAR* pAspects, DWORD FAR* pAdvf,
                                                LPADVISESINK FAR* ppAdvSink);
    void SendOnViewChange(DWORD dwAspect);

private:
    ViewAdviseHolder();
    ~ViewAdviseHolder();

    ULONG _refs;
    LPADVISESINK _pAdvSink;     // THE view advise sink
    DWORD _dwAdviseAspects;     // view aspects of interest to advise sink
    DWORD _dwAdviseFlags;       // view advise flags

};

HRESULT CreateViewAdviseHolder(LPVIEWADVISEHOLDER FAR* ppViewHolder);

#ifdef INC_STATUS_BAR

//
//  forward declarations
//

extern "C" LRESULT CALLBACK
StatusWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class StatusBar;
typedef StatusBar FAR* LPSTATUSBAR;

//+-------------------------------------------------------------------
//
//  Class:      StatusBar
//
//  Purpose:    A status bar.
//
//  Notes:      Of the guidelines laid down by the UI people, the
//              current implementation violates the following::
//              - height of status bar should equal the height of
//                the window's title bar.
//
//--------------------------------------------------------------------

class StatusBar
{
    friend LRESULT CALLBACK StatusWndProc(HWND, UINT, WPARAM, LPARAM);

public:
    static BOOL ClassInit(HINSTANCE hinst); // registers window class

    StatusBar();
    HWND Init(HINSTANCE hinst, HWND hwndParent);
    ~StatusBar();

    void OnSize(LPRECT lprc);
    void DisplayMessage(LPCWSTR);
    WORD GetHeight(void);

private:
    void OnPaint(HWND hwnd);            // handles WM_PAINT message

    HWND _hwnd;                         // our status bar window
    HFONT _hfont;                       // font used to display messages.
    WCHAR _lpstrString[128];            // string currently being displayed.

    // various metrics:
    WORD _wHeight;                      // current height, once computed.
    WORD _wUnitBorder;                  // border, based on _wHeight;
    WORD _wSpace;                       // space between controls on bar.
    WORD _wAboveBelow;                  // space above and below
};


//+---------------------------------------------------------------
//
//  Member:     StatusBar::GetHeight, public
//
//  Synopsis:   Returns the height of the status bar in pixels
//
//----------------------------------------------------------------

inline WORD StatusBar::GetHeight(void)
{
    return _wHeight;
}

#endif // INC_STATUS_BAR

//+------------------------------------------------------------------------
//
//  Macro that calculates the number of elements in a statically-defined
//  array.
//
//-------------------------------------------------------------------------
#define ARRAY_SIZE(_a)  (sizeof(_a) / sizeof(_a[0]))



//+------------------------------------------------------------------------
//
//  Class:      CEnumGeneric (enum)
//
//  Purpose:    Implements an OLE2 enumerator for arrays of data. The CAry
//              class uses this class for its enumerator.
//
//  Interface:  Next        Returns the next element[s] in the array
//              Skip        Skips the next element[s] in the array
//              Reset       Restarts at the beginning of the array
//              Clone       Creates a copy of theis enumerator
//
//              Create      Creates a new instance of this enumerator, given
//                          an interface ID, element size, count, and pointer
//
//  Members:    _refs      Ref count
//              _iid       Interface implemented by this enumerator
//              _cb        Size of each element
//              _c         Number of elements
//              _i         Current index in the array
//              _pv        Pointer to array data
//
//-------------------------------------------------------------------------
class CEnumGeneric : public IUnknown
{
public:
    //  IUnknown methods

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    //  IEnum methods

    STDMETHOD(Next) (ULONG celt, void FAR* reelt, ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) ();
    STDMETHOD(Clone) (CEnumGeneric FAR* FAR* ppenm);

    //  CEnumGeneric methods

    static CEnumGeneric FAR* Create(REFIID, int cb, int c, void FAR* pv, BOOL fAddRef);

protected:
    CEnumGeneric( REFIID, int cb, int c, void FAR* pv, BOOL fAddRef );
    CEnumGeneric( CEnumGeneric * );

    ULONG           _refs;
    IID             _iid;

    BOOL            _fAddRef;
    int             _cb;
    int             _c;
    int             _i;
    void FAR*       _pv;
};

//+------------------------------------------------------------------------
//
//  Class:      CAry (ary)
//
//  Purpose:    Generic resizeable array class
//
//  Interface:  CAry, ~CAry
//              EnsureSize      Ensures that the array is at least a certain
//                              size, allocating more memory if necessary.
//                              Note that the array size is measured in elements,
//                              rather than in bytes.
//              Size            Returns the current size of the array.
//              SetSize         Sets the array size; EnsureSize must be called
//                              first to reserve space if the array is growing
//
//              operator LPVOID Allow the CAry class to be cast to a (void *)
//
//              Append          Adds a new pointer to the end of the array,
//                              growing the array if necessary.  Only works
//                              for arrays of pointers.
//              AppendIndirect  As Append, for non-pointer arrays
//
//              Insert          Inserts a new pointer at the given index in
//                              the array, growing the array if necessary. Any
//                              elements at or following the index are moved
//                              out of the way.
//              InsertIndirect  As Insert, for non-pointer arrays
//
//              Delete          Deletes an element of the array, moving any
//                              elements that follow it to fill
//              DeleteMultiple  Deletes a range of elements from the array,
//                              moving to fill
//              BringToFront    Moves an element of the array to index 0,
//                              shuffling elements to make room
//              SendToBack      Moves an element to the end of the array,
//                              shuffling elements to make room
//
//              Find            Returns the index at which a given pointer
//                              is found
//              FindIndirect    As Find, for non-pointer arrays
//
//              EnumElements    Create an enumerator which supports the given
//                              interface ID for the contents of the array
//
//              Deref [prot]    Returns a pointer to an element of the array;
//                              normally used by type-safe methods in derived
//                              classes
//
//  Members:    _c     Current size of the array
//              _cMac  Current number of elements allocated
//              _cb    Size of each element
//              _pv    Buffer storing the elements
//
//  Note:       The CAry class only supports arrays of elements whose size is
//              less than CARY_MAXELEMSIZE (currently 32).
//
//-------------------------------------------------------------------------
class CAry
{
public:
    CAry(size_t cb);
    ~CAry();

    HRESULT     EnsureSize(int c);
    int         Size(void)
                    { return _c; }
    void        SetSize(int c)
                    { _c = c; }

    operator LPVOID(void)
                    { return _pv; }

    HRESULT     Append(void *);
    HRESULT     AppendIndirect(void *);
    HRESULT     Insert(int, void *);
    HRESULT     InsertIndirect(int, void *);

    void        Delete(int);
    void        DeleteAll(void);
    void        DeleteMultiple(int, int);
    void        BringToFront(int);
    void        SendToBack(int);

    int         Find(void *);
    int         FindIndirect(void **);

    HRESULT     EnumElements(REFIID iid, LPVOID * ppv, BOOL fAddRef);

protected:
    int     _c;
    int     _cMac;
    size_t  _cb;
    void *  _pv;

    void *      Deref(int i);
};
#define CARY_MAXELEMSIZE    32


//+------------------------------------------------------------------------
//
//  Macro:      DECLARE_ARY
//
//  Purpose:    Declares a type-safe class derived from CAry.
//
//  Arguments:  _Cls -- Name of new array class
//              _Ty  -- Type of array element (e.g. FOO)
//              _pTy -- Type which is a pointer to _Ty (e.g. LPFOO)
//
//  Interface:  operator []     Provides a type-safe pointer to an element
//                              of the array
//              operator LPFOO  Allows the array to be cast to a type-safe
//                              pointer to its first element
//
//-------------------------------------------------------------------------
#define DECLARE_ARY(_Cls, _Ty, _pTy) \
    class _Cls : public CAry { public: \
        _Cls(void) : CAry(sizeof(_Ty)) { ; } \
        _Ty& operator[] (int i) { return * (_pTy) Deref(i); } \
        operator _pTy(void) { return (_pTy) _pv; } };


//****************************************************************************
//
// IconBar Class
//
//****************************************************************************

// WM_COMMAND notification codes
#define IBI_NEWSELECTION 1
#define IBI_STATUSAVAIL  2

#define IBP_LEFTBELOW       0
#define IBP_RIGHTABOVE      1
#define IBP_RIGHTBELOW      2
#define IBP_LEFTABOVE       3


extern "C" LRESULT CALLBACK IconBarWndProc(HWND,UINT,WPARAM,LPARAM);

class IconBar;
typedef IconBar FAR* LPICONBAR;

class IconBar
{
    friend LRESULT CALLBACK IconBarWndProc(HWND,UINT,WPARAM,LPARAM);

public:
    static LPICONBAR Create(HINSTANCE hinst, HWND hwndParent, HWND hwndNotify,
            SHORT sIdChild);
    ~IconBar(void);

    HWND GetHwnd(void);

    void Position(int sTop, int sLeft, int sWhere);
    HRESULT AddCellsFromRegDB(LPWSTR pszFilter);
    void AddCell(CLSID& clsid, HICON hIcon, LPWSTR szTitle);
    void SetCellAspect( int cWide, int cHigh );
    void SelectCell(int iCell, BOOL fStick);
    int GetSelectedCell(void);
    CLSID& GetSelectedClassId(void);
    LPWSTR GetStatusMessage(void);
    BOOL IsStuck(void);

private:
#define MAX_CELLS       24
#define MAX_ICON_TITLE  64

    struct IconCell {
        CLSID clsid;
        HICON hIcon;
        HBITMAP hBmp;
        WCHAR achTitle[MAX_ICON_TITLE];
    };
    typedef IconCell FAR *LPICONCELL;

    IconBar(HWND hwndParent, HWND hwndNotify, SHORT sIDChild);

    int AddCell(LPICONCELL pCells, int i,
            CLSID& clsid, HICON hIcon, LPWSTR szTitle);
    void AddCache(void);
    void Draw(HDC hdc, POINT pt, BOOL fDown, int iCell);

    void ncCalcRect(HWND hwnd, LPRECT lprc);
    void ncDrawFrame(HWND hwnd);
    LONG ncHitTest(HWND hwnd, POINT pt);
    LRESULT ncMsgFilter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static ATOM InitClass(HINSTANCE hinst);
    static BOOL _fInit;
    static ATOM _atomClass;
    static HBRUSH hbrActiveCaption;
    static HBRUSH hbrInActiveCaption;
    static HBRUSH hbrWindowFrame;
    static HBRUSH hbrSysBox;
    static WORD wCnt;
    static COLORREF crLtGray;
    static COLORREF crGray;
    static COLORREF crDkGray;
    static COLORREF crBlack;

    //windows message handling
    BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct);
    void OnDestroy(HWND hwnd);
    void OnPaint(HWND hwnd);
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);
    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);

    SHORT _dxMargin;            // horizontal margin between tray border and icon
    SHORT _dyMargin;            // vertical margin between tray border and icon
    SHORT _dxCell;              // horizontal size of icon
    SHORT _dyCell;              // vertical size of icon
    SHORT _cyCaption;           // title bar height
    SHORT _cxFrame;             // thickness of nc frame
    SHORT _cyFrame;
    SHORT _cxSize;              // size of sizing corner
    SHORT _cySize;

    BOOL _fFloating;            // free-floating window?
    HWND _hwnd;                 // the icon toolbar window
    HWND _hwndNotify;           // the window to notify via WM_COMMAND
    SHORT _sIdChild;            // child identifier for WM_COMMAND

    int _numCells;              // the number of icon "cells" that have been loaded
    IconCell _cell[MAX_CELLS];  // the array of icon cells
    static int _cUsed;                  // outstanding users of cached cells
    static int _cCache;                 // total cached cells
    static IconCell _cache[MAX_CELLS];  // a cache array of icon cells

    int _cellsPerRow;
    int _selectedCell;
    int _iCellAtCursor;
    BOOL _fStuck;

    // other private helper members
    void GetCellPoint(LPPOINT lppt, int i);
    void GetCellRect(LPRECT lprc, int i);
    void InvalidateCell(HWND hwnd, int i);
    int CellIndexFromPoint(POINT pt);
};

inline void
IconBar::OnDestroy(HWND hwnd)
{
    _hwnd = NULL;
}

inline BOOL
IconBar::OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
    return TRUE;
}

inline BOOL
IconBar::IsStuck(void)
{
    return _fStuck;
}

inline int
IconBar::GetSelectedCell(void)
{
    return _selectedCell;
}

inline HWND
IconBar::GetHwnd(void)
{
    return _hwnd;
}


//****************************************************************************
//
// Macros
//
//****************************************************************************

//+------------------------------------------------------------------------
//
//  Macro:      DECLARE_ARY
//
//  Purpose:    Declares a type-safe class derived from CAry.
//
//  Arguments:  _Cls -- Name of new array class
//              _Ty  -- Type of array element (e.g. FOO)
//              _pTy -- Type which is a pointer to _Ty (e.g. LPFOO)
//
//  Interface:  operator []     Provides a type-safe pointer to an element
//                              of the array
//              operator LPFOO  Allows the array to be cast to a type-safe
//                              pointer to its first element
//
//-------------------------------------------------------------------------
#define DECLARE_ARY(_Cls, _Ty, _pTy) \
    class _Cls : public CAry { public: \
        _Cls(void) : CAry(sizeof(_Ty)) { ; } \
        _Ty& operator[] (int i) { return * (_pTy) Deref(i); } \
        operator _pTy(void) { return (_pTy) _pv; } };


//======================================================================
//
// The base class stuff...
//
//
//  Classes:    ClassDescriptor
//              SrvrCtrl
//              SrvrDV
//              SrvrInPlace
//
//----------------------------------------------------------------------------


enum OLE_SERVER_STATE
{
    OS_PASSIVE,
    OS_LOADED,                          // handler but no server
    OS_RUNNING,                         // server running, invisible
    OS_INPLACE,                         // server running, inplace-active, no U.I.
    OS_UIACTIVE,                        // server running, inplace-active, w/ U.I.
    OS_OPEN                             // server running, open-edited
};

//  forward declarations of classes

class ClassDescriptor;
typedef ClassDescriptor FAR* LPCLASSDESCRIPTOR;

class SrvrCtrl;
typedef SrvrCtrl FAR* LPSRVRCTRL;

class SrvrDV;
typedef SrvrDV FAR* LPSRVRDV;

class SrvrInPlace;
typedef SrvrInPlace FAR* LPSRVRINPLACE;


//+---------------------------------------------------------------
//
//  Class:      ClassDescriptor
//
//  Purpose:    Global, static information about a server class
//
//  Notes:      This allows the base classes to implement a lot of
//              OLE functionality with requiring additional virtual
//              method calls on the derived classes.
//
//---------------------------------------------------------------

class ClassDescriptor
{
public:
    ClassDescriptor(void);
    BOOL Init(HINSTANCE hinst, WORD wBaseID);
    HMENU LoadMenu(void);

    HINSTANCE _hinst;           // instance handle of module serving this class
    WORD _wBaseResID;           // base resource identifier (see IDOFF_ )
    CLSID _clsid;               // class's unique identifier

    HICON _hicon;               // iconic representation of class
    HACCEL _haccel;             // accelerators for those menus
    OLEMENUGROUPWIDTHS _mgw;    // the number of Edit, Object, and Help menus

    WCHAR _szUserClassType[4][64];// [0] unused
                                // [1] the string assigned to classid key in reg db.
                                // [2] reg db: \CLSID\<clsid>\AuxUserType\2
                                // [3] reg db: \CLSID\<clsid>\AuxUserType\3
    WCHAR _szDocfileExt[8];     // extension for docfile files

    DWORD _dwMiscStatus;        // reg db: \CLSID\<clsid>\MiscStatus

    //DERIVED:  The derived class must supply these tables.
    //REVIEW:  These could be loaded from resources, too!
    LPOLEVERB _pVerbTable;      // pointer to list of verbs available
    int _cVerbTable;            // number of entries in the verb table

    LPFORMATETC _pGetFmtTable;  // format table for IDataObject::GetData[Here]
    int _cGetFmtTable;          // number of entries in the table

    LPFORMATETC _pSetFmtTable;  // format table for IDataObject::SetData
    int _cSetFmtTable;          // number of entries in the table
};

//+---------------------------------------------------------------
//
//  Class:      SrvrCtrl
//
//  Purpose:    Control subobject of OLE compound document server
//
//  Notes:      This class supports the IOleObject interface.
//
//---------------------------------------------------------------

class SrvrCtrl : public IOleObject
{
public:
    // standard verb implementations
    typedef HRESULT (*LPFNDOVERB) (LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoShow(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoOpen(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoHide(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoUIActivate(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoInPlaceActivate(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);

    //DERIVED: IUnknown methods are left pure virtual and must be implemented
    //  by the derived class

    // IOleObject interface methods
    STDMETHOD(SetClientSite) (LPOLECLIENTSITE pClientSite);
    STDMETHOD(GetClientSite) (LPOLECLIENTSITE FAR* ppClientSite);
    STDMETHOD(SetHostNames) (LPCWSTR szContainerApp, LPCWSTR szContainerObj);
    STDMETHOD(Close) (DWORD dwSaveOption);
    STDMETHOD(SetMoniker) (DWORD dwWhichMoniker, LPMONIKER pmk);
    STDMETHOD(GetMoniker) (DWORD dwAssign, DWORD dwWhichMoniker,
                                                    LPMONIKER FAR* ppmk);
    STDMETHOD(InitFromData) (LPDATAOBJECT pDataObject,BOOL fCreation,
                                                        DWORD dwReserved);
    STDMETHOD(GetClipboardData) (DWORD dwReserved, LPDATAOBJECT FAR* ppDataObject);
    STDMETHOD(DoVerb) (LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite,
                            LONG lindex, HWND hwndParent, LPCRECT lprcPosRect);
    STDMETHOD(EnumVerbs) (LPENUMOLEVERB FAR* ppenumOleVerb);
    STDMETHOD(Update) (void);
    STDMETHOD(IsUpToDate) (void);
    STDMETHOD(GetUserClassID) (CLSID FAR* pClsid);
    STDMETHOD(GetUserType) (DWORD dwFormOfType, LPWSTR FAR* pszUserType);
    STDMETHOD(SetExtent) (DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHOD(GetExtent) (DWORD dwDrawAspect, LPSIZEL lpsizel);

    STDMETHOD(Advise)(IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection);
    STDMETHOD(Unadvise)(DWORD dwConnection);
    STDMETHOD(EnumAdvise) (LPENUMSTATDATA FAR* ppenumAdvise);
    STDMETHOD(GetMiscStatus) (DWORD dwAspect, DWORD FAR* pdwStatus);
    STDMETHOD(SetColorScheme) (LPLOGPALETTE lpLogpal);

    // pointers to our data/view and inplace subobjects
    LPSRVRDV _pDV;                      // our persistent data/view subobject
    LPSRVRINPLACE _pInPlace;            // our inplace-active subobject
    // pointers to those objects' private unknowns
    LPUNKNOWN _pPrivUnkDV;
    LPUNKNOWN _pPrivUnkIP;

    // methods and members required by our data/view and inplace subobjects.
    OLE_SERVER_STATE State(void);
    void SetState(OLE_SERVER_STATE state)
        {
            _state = state;
        };
    HRESULT TransitionTo(OLE_SERVER_STATE state);
    LPOLECLIENTSITE _pClientSite;
    void OnSave(void);

    void EnableIPB(BOOL fEnabled);
    BOOL IsIPBEnabled(void);

    virtual HRESULT Init(LPCLASSDESCRIPTOR pClass);

    // IOleObject-related members
    DWORD _dwRegROT;                    // our R.O.T. registration value
    LPOLEADVISEHOLDER _pOleAdviseHolder;// for collection our advises
    LPWSTR _lpstrCntrApp;               // top-level container application
    LPWSTR _lpstrCntrObj;               // and object names


protected:

    //
    // DERIVED:  Each of these correspond to a unique state transition.
    //  The derived class may want to override to do additional processing.
    //
    virtual HRESULT PassiveToLoaded();
    virtual HRESULT LoadedToPassive();

    virtual HRESULT LoadedToRunning();
    virtual HRESULT RunningToLoaded();

    virtual HRESULT RunningToInPlace();
    virtual HRESULT InPlaceToRunning();

    virtual HRESULT InPlaceToUIActive();
    virtual HRESULT UIActiveToInPlace();

    virtual HRESULT RunningToOpened();
    virtual HRESULT OpenedToRunning();

    // constructors, initializers, and destructors
    SrvrCtrl(void);
    virtual ~SrvrCtrl(void);

    LPCLASSDESCRIPTOR _pClass;          // global info about our OLE server

    OLE_SERVER_STATE _state;            // our current state

    BOOL _fEnableIPB;                   // FALSE turns off built-in border

    //DERIVED:  The derived class must supply table of verb functions
    //  parallel to the table of verbs in the class descriptor
    LPFNDOVERB FAR* _pVerbFuncs;        // verb function table
};

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::State, public
//
//  Synopsis:   Returns the current state of the object
//
//  Notes:      The valid object states are closed, loaded, inplace,
//              U.I. active, and open.  These states are defined
//              by the OLE_SERVER_STATE enumeration.
//
//---------------------------------------------------------------

inline OLE_SERVER_STATE
SrvrCtrl::State(void)
{
    return _state;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::EnableIPB, public
//
//  Synopsis:   Enables/Disables built in InPlace border
//
//---------------------------------------------------------------
inline void
SrvrCtrl::EnableIPB(BOOL fEnabled)
{
    _fEnableIPB = fEnabled;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::IsIPBEnabled, public
//
//  Synopsis:   Answers whether built-in InPlace border is enabled
//
//---------------------------------------------------------------
inline BOOL
SrvrCtrl::IsIPBEnabled(void)
{
    return _fEnableIPB;
}

//+---------------------------------------------------------------
//
//  Class:      SrvrDV
//
//  Purpose:    Data/View subobject of OLE compound document server
//
//  Notes:      This class supports the IDataObject and IViewObject interfaces.
//              It also supports the IPersist-derived interfaces.
//              Objects of this class can operate as part of a complete
//              server aggregation or independently as a transfer data
//              object.
//
//---------------------------------------------------------------

class SrvrDV: public IDataObject,
              public IViewObject,
              public IPersistStorage,
              public IPersistStream,
              public IPersistFile
{
public:

typedef HRESULT (*LPFNGETDATA) (LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);
typedef HRESULT (*LPFNSETDATA) (LPSRVRDV, LPFORMATETC, LPSTGMEDIUM);

    // standard format Get/Set implementations
    static HRESULT GetEMBEDDEDOBJECT(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetMETAFILEPICT(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetOBJECTDESCRIPTOR(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetLINKSOURCE(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);

    //
    //DERIVED: IUnknown methods are left pure virtual and must be implemented
    //  by the derived class

    //
    // IDataObject interface methods
    //
    STDMETHOD(GetData) (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHOD(GetDataHere) (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHOD(QueryGetData) (LPFORMATETC pformatetc);
    STDMETHOD(GetCanonicalFormatEtc) (LPFORMATETC pformatetc,
                    LPFORMATETC pformatetcOut);
    STDMETHOD(SetData) (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium,
                    BOOL fRelease);
    STDMETHOD(EnumFormatEtc) (DWORD dwDirection, LPENUMFORMATETC FAR* ppenum);
    STDMETHOD(DAdvise) (FORMATETC FAR* pFormatetc, DWORD advf,
                    LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
    STDMETHOD(DUnadvise) (DWORD dwConnection);
    STDMETHOD(EnumDAdvise) (LPENUMSTATDATA FAR* ppenumAdvise);

    //
    // IViewObject interface methods
    //
    STDMETHOD(Draw) (DWORD dwDrawAspect, LONG lindex,
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
                    HDC hicTargetDev,
                    HDC hdcDraw,
                    LPCRECTL lprcBounds,
                    LPCRECTL lprcWBounds,
                    BOOL (CALLBACK * pfnContinue) (DWORD),
                    DWORD dwContinue);
    STDMETHOD(GetColorSet) (DWORD dwDrawAspect, LONG lindex,
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
                    HDC hicTargetDev,
                    LPLOGPALETTE FAR* ppColorSet);
    STDMETHOD(Freeze)(DWORD dwDrawAspect, LONG lindex, void FAR* pvAspect,
                    DWORD FAR* pdwFreeze);
    STDMETHOD(Unfreeze) (DWORD dwFreeze);
    STDMETHOD(SetAdvise) (DWORD aspects, DWORD advf, LPADVISESINK pAdvSink);
    STDMETHOD(GetAdvise) (DWORD FAR* pAspects, DWORD FAR* pAdvf,
                    LPADVISESINK FAR* ppAdvSink);

    //
    // IPersist interface methods
    //
    STDMETHOD(GetClassID) (LPCLSID lpClassID);

    STDMETHOD(IsDirty) (void);

    //
    // IPersistStream interface methods
    //
    STDMETHOD(Load) (LPSTREAM pStm);
    STDMETHOD(Save) (LPSTREAM pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax) (ULARGE_INTEGER FAR * pcbSize);

    //
    // IPersistStorage interface methods
    //
    STDMETHOD(InitNew) (LPSTORAGE pStg);
    STDMETHOD(Load) (LPSTORAGE pStg);
    STDMETHOD(Save) (LPSTORAGE pStgSave, BOOL fSameAsLoad);
    STDMETHOD(SaveCompleted) (LPSTORAGE pStgNew);
    STDMETHOD(HandsOffStorage) (void);

    //
    // IPersistFile interface methods
    //
    STDMETHOD(Load) (LPCOLESTR lpszFileName, DWORD grfMode);
    STDMETHOD(Save) (LPCOLESTR lpszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted) (LPCOLESTR lpszFileName);
    STDMETHOD(GetCurFile) (LPOLESTR FAR * lplpszFileName);

    //
    // DERIVED: methods required by the control
    //
    virtual HRESULT GetClipboardCopy(LPSRVRDV FAR* ppDV) = 0;
    virtual HRESULT GetExtent(DWORD dwAspect, LPSIZEL lpsizel);
    virtual HRESULT SetExtent(DWORD dwAspect, SIZEL& sizel);
    virtual void SetMoniker(LPMONIKER pmk);
    HRESULT GetMoniker(DWORD dwAssign, LPMONIKER FAR* ppmk);
    LPWSTR GetMonikerDisplayName(DWORD dwAssign = OLEGETMONIKER_ONLYIFTHERE);

    //
    //DERIVED:  The derived class should call this base class method whenever
    //  the data changes.  This launches all appropriate advises.
    //
    void OnDataChange(DWORD dwAdvf = 0);

    //
    //DERIVED:  The derived class should override these methods to perform
    //  rendering of its native data.  These are used in the implementation of
    //  the IViewObject interface
    //
    virtual HRESULT RenderContent(DWORD dwDrawAspect,
                LONG lindex,
                void FAR* pvAspect,
                DVTARGETDEVICE FAR * ptd,
                HDC hicTargetDev,
                HDC hdcDraw,
                LPCRECTL lprectl,
                LPCRECTL lprcWBounds,
                BOOL (CALLBACK * pfnContinue) (DWORD),
                DWORD dwContinue);
    virtual HRESULT RenderPrint(DWORD dwDrawAspect,
                LONG lindex,
                void FAR* pvAspect,
                DVTARGETDEVICE FAR * ptd,
                HDC hicTargetDev,
                HDC hdcDraw,
                LPCRECTL lprectl,
                LPCRECTL lprcWBounds,
                BOOL (CALLBACK * pfnContinue) (DWORD),
                DWORD dwContinue);
    virtual HRESULT RenderThumbnail(DWORD dwDrawAspect,
                LONG lindex,
                void FAR* pvAspect,
                DVTARGETDEVICE FAR * ptd,
                HDC hicTargetDev,
                HDC hdcDraw,
                LPCRECTL lprectl,
                LPCRECTL lprcWBounds,
                BOOL (CALLBACK * pfnContinue) (DWORD),
                DWORD dwContinue);

    BOOL IsInNoScrible(void);

    virtual HRESULT Init(LPCLASSDESCRIPTOR pClass, LPSRVRCTRL pCtrl);
    virtual HRESULT Init(LPCLASSDESCRIPTOR pClass, LPSRVRDV pDV);

protected:

    //
    //DERIVED:  The derived class should override these methods to perform
    //  persistent serialization and deserialization.  These are used in the
    //  implementation of IPersistStream/Storage/File
    //
    virtual HRESULT LoadFromStream(LPSTREAM pStrm);
    virtual HRESULT SaveToStream(LPSTREAM pStrm);
    virtual DWORD GetStreamSizeMax(void);

    virtual HRESULT LoadFromStorage(LPSTORAGE pStg);
    virtual HRESULT SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad);

    //
    // constructors, initializers, and destructors
    //
    SrvrDV(void);
    virtual ~SrvrDV(void);

    LPCLASSDESCRIPTOR _pClass;          // global info about our OLE server
    LPSRVRCTRL _pCtrl;                  // control of server aggregate or
                                        // NULL if transfer object
    //
    // IDataObject-related members
    //
    LPMONIKER _pmk;                     // moniker used for LINKSOURCE
    LPWSTR _lpstrDisplayName;           // cached display name of moniker
    SIZEL _sizel;                       // used for OBJECTDESCRIPTOR and Extent
    LPDATAADVISEHOLDER _pDataAdviseHolder;

    //
    //DERIVED:  The derived class must supply table of Get functions
    //  and a table of Set functions parallel to the FORMATETC tables
    //  in the class descriptor.
    //
    LPFNGETDATA FAR* _pGetFuncs;        // GetData(Here) function table
    LPFNSETDATA FAR* _pSetFuncs;        // SetData function table

    LPVIEWADVISEHOLDER _pViewAdviseHolder;

    unsigned _fFrozen: 1;               // blocked from updating
    unsigned _fDirty: 1;                // TRUE iff persistent data has changed
    unsigned _fNoScribble: 1;           // between save and save completed

    // IPersistStorage-related members
    LPSTORAGE _pStg;                    // our home IStorage instance
};

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::IsInNoScrible, public
//
//  Synopsis:   Answers wether we are currently in no-scribble mode
//
//---------------------------------------------------------------

inline BOOL
SrvrDV::IsInNoScrible(void)
{
    return _fNoScribble;
}

//+---------------------------------------------------------------
//
//  Class:      SrvrInPlace
//
//  Purpose:    Inplace subobject of OLE compound document server
//
//  Notes:      This class supports the IOleInPlaceObject and
//              IOleInPlaceActiveObject interfaces.
//
//---------------------------------------------------------------

class SrvrInPlace: public IOleInPlaceObject,
                   public IOleInPlaceActiveObject
{
public:
    //DERIVED: IUnknown methods are left pure virtual and must be implemented
    //  by the derived class

    // IOleWindow interface methods
    STDMETHOD(GetWindow) (HWND FAR* lphwnd);
    STDMETHOD(ContextSensitiveHelp) (BOOL fEnterMode);

    // IOleInPlaceObject interface methods
    STDMETHOD(InPlaceDeactivate) (void);
    STDMETHOD(UIDeactivate) (void);
    STDMETHOD(SetObjectRects) (LPCRECT lprcPosRect, LPCRECT lprcClipRect);
    STDMETHOD(ReactivateAndUndo) (void);

    // IOleInPlaceActiveObject methods
    STDMETHOD(TranslateAccelerator) (LPMSG lpmsg);
    STDMETHOD(OnFrameWindowActivate) (BOOL fActivate);
    STDMETHOD(OnDocWindowActivate) (BOOL fActivate);
    STDMETHOD(ResizeBorder) (LPCRECT lprectBorder,
            LPOLEINPLACEUIWINDOW lpUIWindow,
            BOOL fFrameWindow);
    STDMETHOD(EnableModeless) (BOOL fEnable);

    // methods and members required by the other subobjects.
    HWND WindowHandle(void);
    void SetChildActivating(BOOL fGoingActive);
    BOOL GetChildActivating(void);
    BOOL IsDeactivating(void);
    void ReflectState(BOOL fUIActive);

    LPOLEINPLACEFRAME _pFrame;          // our in-place active frame
    LPOLEINPLACEUIWINDOW _pDoc;         // our in-place active document

    //DERIVED:  These methods are called by the control to effect a
    //  state transition.  The derived class can override these methods if
    //  it requires additional processing.
    virtual HRESULT ActivateInPlace(IOleClientSite *pClientSite);
    virtual HRESULT DeactivateInPlace(void);
    virtual HRESULT ActivateUI(void);
    virtual HRESULT DeactivateUI(void);

    //DERIVED:  These methods are related to U.I. activation.  The derived
    //  class should override these to perform additional processing for
    //  any frame, document, or floating toolbars or palettes.
    virtual void InstallUI(void);
    virtual void RemoveUI(void);

    LPOLEINPLACESITE _pInPlaceSite; // our in-place client site

    virtual HRESULT Init(LPCLASSDESCRIPTOR pClass, LPSRVRCTRL pCtrl);

protected:

    //DERIVED:  More U.I. activation-related methods.
    virtual void CreateUI(void);
    virtual void DestroyUI(void);
    virtual void InstallFrameUI(void);
    virtual void RemoveFrameUI(void);
    virtual void InstallDocUI(void);
    virtual void RemoveDocUI(void);
    virtual void ClearSelection(void);
    virtual void SetFocus(HWND hwnd);

    //DERIVED:  The derived class must override this function to
    //          attach the servers in-place active window.
    virtual HWND AttachWin(HWND hwndParent) = 0;
    virtual void DetachWin(void);

    SrvrInPlace(void);
    virtual ~SrvrInPlace(void);

    LPCLASSDESCRIPTOR _pClass;      // global info about our class

    LPSRVRCTRL _pCtrl;              // the control we are part of.

    // IOleInPlaceObject-related members
    unsigned _fUIDown: 1;           // menu/tools integrated with container?
    unsigned _fChildActivating: 1;  // site going UIActive?
    unsigned _fDeactivating: 1;     // being deactivated from the outside?
    unsigned _fCSHelpMode: 1;       // in context-sensitive help state?

    OLEINPLACEFRAMEINFO _frameInfo; // accelerator information from our container

    InPlaceBorder _IPB;             // our In-Place border when UIActive
    RECT _rcFrame;                  // our frame rect

    HWND _hwnd;                     // our InPlace window
    HMENU _hmenu;
    HOLEMENU _hOleMenu;             // menu registered w/ OLE
    HMENU _hmenuShared;             // the shared menu when we are UI active
    OLEMENUGROUPWIDTHS _mgw;        // menu interleaving information

    BOOL _fClientResize;            // TRUE during calls to SetObjectRects
};


//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::IsDeactivating, public
//
//  Synopsis:   Gets value of a flag indicating deactivation
//              (from the outside) in progress
//
//---------------------------------------------------------------

inline BOOL
SrvrInPlace::IsDeactivating(void)
{
    return _fDeactivating;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::GetChildActivating, public
//
//  Synopsis:   Gets value of a flag indicating that a child is
//              activating to prevent menu flashing
//
//---------------------------------------------------------------

inline BOOL
SrvrInPlace::GetChildActivating(void)
{
    return _fChildActivating;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::SetChildActivating, public
//
//  Synopsis:   Sets or clears a flag indicating that a child is
//              activating to prevent menu flashing
//
//---------------------------------------------------------------

inline void
SrvrInPlace::SetChildActivating(BOOL fGoingActive)
{
    _fChildActivating = fGoingActive;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::WindowHandle, public
//
//  Synopsis:   Returns object window handle
//
//---------------------------------------------------------------

inline HWND
SrvrInPlace::WindowHandle(void)
{
    return _hwnd;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrInPlace::ReflectState, public
//
//  Synopsis:   TBD
//
//---------------------------------------------------------------

inline void
SrvrInPlace::ReflectState(BOOL fActive)
{
    if(_pCtrl->IsIPBEnabled())
    {
        _IPB.SetUIActive(fActive);
    }
}

#endif  // !RC_INVOKED


#endif //_O2BASE_HXX_
