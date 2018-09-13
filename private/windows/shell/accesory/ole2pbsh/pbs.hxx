//+---------------------------------------------------------------------
//
//   File:      pbs.hxx
//
//   Contents:  Paintbrush Class Definitions
//
//   Classes:
//              PBFactory
//              PBCtrl
//              PBInPlace
//              PBDV
//
//              CXBag
//
//              Tray
//              ToolTray
//              ColorTray
//
// Functions:
//              ObjectOffersAcceptableFormats
//                              GetTypedHGlobalFromObject
//
//------------------------------------------------------------------------

#ifndef __PBS_HXX
#define __PBS_HXX

//
// Resource IDs:
//
//  Our base id is 0, so our Class Descriptor resource IDs
//  are identical to their offsets
//
#define IDS_CLASSID             IDOFF_CLASSID
#define IDS_USERTYPEFULL        IDOFF_USERTYPEFULL
#define IDS_USERTYPESHORT       IDOFF_USERTYPESHORT
#define IDS_USERTYPEAPP         IDOFF_USERTYPEAPP
#define IDS_DOCFEXT             IDOFF_DOCFEXT
#define IDR_ICON                IDOFF_ICON
#define IDR_ACCELS              IDOFF_ACCELS
#define IDR_MENU                IDOFF_MENU
#define IDR_MGW                 IDOFF_MGW
#define IDR_MISCSTATUS          IDOFF_MISCSTATUS

//
// resource compiler not interested in the rest...
//
#ifndef RC_INVOKED

BOOL ObjectOffersAcceptableFormats(LPDATAOBJECT pDataObj, CLIPFORMAT FAR* lpcf);
HRESULT GetTypedHGlobalFromObject(LPDATAOBJECT pDataObj, CLIPFORMAT FAR* lpcf, HGLOBAL FAR* lphGlobal);

//
// The following value must be greater than the largest legitimate
// CF_WHATEVER value, but smaller than 2^15.
// We use this constant to distinguish values in static tables of
// cliboard-formats which are really indexes into other tables
// (e.g., OleClipFormat) who's actual values are runtime determined...
#define MAX_CF_VAL  10000

//
// ToolTray Classes
//=====================
//
#define CLASS_NAME_PBTRAY L"PBToolTray"

extern "C" LRESULT CALLBACK TrayWndProc(HWND, UINT, WPARAM, LPARAM);

class Tray;
typedef Tray FAR* LPTRAY;

class Tray
{
    friend LRESULT CALLBACK TrayWndProc(HWND, UINT, WPARAM, LPARAM);

public:
    static LPTRAY Create(HINSTANCE hinst,
                        HWND hwndParent,
                        HWND hwndNotify,
                        int iChild);

    HWND WindowHandle(void)
        { return _hwnd; };

    virtual void GetToolRect(LPRECT lprc);
    virtual void Position(int x, int y);
    virtual ~Tray(void);

protected:
    Tray(HWND hwndParent, HWND hwndNotify, int iChild);
    HWND CreateToolWindow(HINSTANCE hinst, HWND hwndParent)
        {
            DWORD dwStyle = hwndParent != NULL
                        ? WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS
                        : WS_BORDER;
            HWND hwndOwner = hwndParent ? hwndParent : _hwndNotify;
            HMENU hmenuID = hwndParent ? (HMENU)_iChild : NULL;
            _hwnd = CreateWindow(CLASS_NAME_PBTRAY,
                                        NULL,               //szWindowName
                                        dwStyle,
                                        0, 0, 0, 0,         //will be resized
                                        hwndOwner,
                                        hmenuID,
                                        hinst,
                                        this);
            return _hwnd;
        };

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
    virtual void OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos);

    SHORT _dxMargin;            // horizontal margin between tray border and contents
    SHORT _dyMargin;            // vertical margin between tray border and contens
    SHORT _cxFrame;             // thickness of nc frame
    SHORT _cyFrame;             // thickness of nc frame
    int _xWidth;                // tray width in pixels
    int _yHeight;               // tray Height in pixels

    HWND _hwnd;                 // our tray window
    HWND _hwndNotify;           // tray notification window
    int _iChild;                // notification ID
};


class ToolTray;
typedef ToolTray FAR* LPTOOLTRAY;

class ToolTray: public Tray
{
public:
    static LPTOOLTRAY Create(HINSTANCE hinst,
                        HWND hwndParent,
                        HWND hwndNotify,
                        int iChild);
    void GetToolRect(LPRECT lprc);
    void GetLSizeRect(LPRECT lprc);
    virtual void Position(int x, int y);
protected:
    virtual void OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos);
private:
    ToolTray(HWND hwndParent, HWND hwndNotify, int iChild);
    int _yOffset;               // size of Line-Size tool space
};

class ColorTray;
typedef ColorTray FAR* LPCOLORTRAY;

class ColorTray: public Tray
{
public:
    static LPCOLORTRAY Create(HINSTANCE hinst,
                        HWND hwndParent,
                        HWND hwndNotify,
                        int iChild);
    void GetColorRect(LPRECT lprc);
    virtual void Position(int x, int y);
protected:
    virtual void OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos);
private:
    ColorTray(HWND hwndParent, HWND hwndNotify, int iChild);
};



#include "oleglue.h"    // functions exposed to pbrush C code

DEFINE_OLEGUID(CLSID_PBRUSH32, 0x00020C00, 0, 0);



//+---------------------------------------------------------------
//
//  Class:      PBFactory
//
//  Purpose:    Creates new Paintbrush objects
//
//  Notes:      This factory creates PBCtrl objects, which in turn
//              create the PBDV and PBInPlace subobjects.
//
//---------------------------------------------------------------

class PBFactory: public StdClassFactory
{
public:
    STDMETHOD(CreateInstance) (LPUNKNOWN, REFIID, LPVOID FAR*);
    STDMETHOD(LockServer) (BOOL fLock);
    static BOOL Create(HINSTANCE hinst);
    BOOL Init(HINSTANCE hinst);
    ~PBFactory() { delete _pClass; }
    LPCLASSDESCRIPTOR _pClass;
};

//
// forward declaration of classes
//

class PBCtrl;
typedef PBCtrl FAR* LPPBCTRL;

class PBInPlace;
typedef PBInPlace FAR* LPPBINPLACE;

class PBDV;
typedef PBDV FAR* LPPBDV;

class CXBag;
typedef CXBag FAR* LPXBAG;

//+---------------------------------------------------------------
//
//  Class:      CXBag
//
//  Purpose:    Plumbing for data transfer
//
//---------------------------------------------------------------

class CXBag: public IDataObject
{
public:
    static HRESULT Create(LPXBAG *ppXBag, LPPBCTRL pHost, LPPOINT pptSelect);

    DECLARE_STANDARD_IUNKNOWN(CXBag);

    //
    //IDataObject
    //

    // DAdvise does not need to be suported by transfer objects
    STDMETHODIMP DAdvise( FORMATETC FAR* pFormatetc,
                            DWORD advf,
                            LPADVISESINK pAdvSink,
                            DWORD FAR* pdwConnection);

    // DUnadvise does not need to be suported by transfer objects
    STDMETHODIMP DUnadvise( DWORD dwConnection);

    // EnumDAdvise does not need to be suported by transfer objects
    STDMETHODIMP EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise);

    STDMETHODIMP EnumFormatEtc( DWORD dwDirection,
                                LPENUMFORMATETC FAR* ppenumFormatEtc);

    // GetCanonicalFormatEtc does not need to be suported by transfer objects
    STDMETHODIMP GetCanonicalFormatEtc( LPFORMATETC pformatetc,
                                        LPFORMATETC pformatetcOut);

    STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium );
    STDMETHODIMP GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pformatetc );

    // SetData does not need to be suported by transfer objects
    STDMETHODIMP SetData(LPFORMATETC pformatetc,
                            STGMEDIUM FAR * pmedium,
                            BOOL fRelease);

    //
    //Public Helpers
    //
    HRESULT SnapShotAndDetach(void);
    void Detach(void)
    {
        _pHost = NULL;
    };

private:
    CXBag(LPPBCTRL pHost);
    ~CXBag();

    HRESULT BagItInStorage(LPSTGMEDIUM pmedium, BOOL fStgProvided);

    LPPBCTRL _pHost;        // ptr back to host
    LPSTORAGE _pStgBag;     // snapshot storage (or NULL)
        RECT _rcSelection;              // what was selected at copy/cut time
};

//+---------------------------------------------------------------
//
//  Class:      PBInPlace
//
//  Purpose:    InPlace subobject of our CD object
//
//  Notes:      This class supports SrvrInPlace
//
//---------------------------------------------------------------

#define UIBORDER_WIDTH  4   /* per UI guidlines */
#define UIBORDER_HEIGHT 4   /* per UI guidlines */

extern "C" LRESULT CALLBACK IPWndProc(HWND,UINT,WPARAM,LPARAM);

class PBInPlace: public SrvrInPlace,
                 public IDropTarget
{
friend LRESULT CALLBACK IPWndProc(HWND,UINT,WPARAM,LPARAM);

public:
    static BOOL ClassInit(LPCLASSDESCRIPTOR pClass);

    static HRESULT Create(LPPBCTRL pPBCtrl,
            LPCLASSDESCRIPTOR pClass,
            LPUNKNOWN FAR* ppUnkCtrl,
            LPPBINPLACE FAR* ppObj);


    DECLARE_DELEGATING_IUNKNOWN(PBInPlace);

    //
    //IDropTarget methods
    //
    STDMETHOD(DragEnter) (LPDATAOBJECT pDataObj,
                            DWORD grfKeyState,
                            POINTL pt,
                            LPDWORD pdwEffect);
    STDMETHOD(DragOver) (DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragLeave) (void);
    STDMETHOD(Drop) (LPDATAOBJECT pDataObj,
                    DWORD grfKeyState,
                    POINTL pt,
                    LPDWORD pdwEffect);

    //
    // Public helper methods for glue code...
    //
    STDMETHOD(SetObjectRects) (LPCRECT lprcPosRect, LPCRECT lprcClipRect);
    STDMETHOD(OnFrameWindowActivate) (BOOL fActivate);

    HWND GetFrameWindow(void)
    {
        HWND hwnd = NULL;
        if(_pFrame != NULL)
            _pFrame->GetWindow(&hwnd);
        return hwnd;
    };
    int CalcMenuPos(int iMenu);
    void GetInPlaceInfo(LPOLEINPLACEFRAME *ppFrame, OLEINPLACEFRAMEINFO **ppInfo)
    {
        *ppFrame = _pFrame;
        *ppInfo = &_frameInfo;
    };
    void RegisterAsDropTarget(HWND hwnd)
    {
        HRESULT hr = RegisterDragDrop(hwnd, (LPDROPTARGET)this);
        if(hr == NOERROR)
        {
            _hwndDropTarget = hwnd;
        }
        else
        {
            DOUT(L"......PBInPlace::RegisterAsDropTarget ERROR!\r\n");
            _hwndDropTarget = NULL;
        }
    }
    void RevokeOurDropTarget(void)
    {
                if(_hwndDropTarget)
                        RevokeDragDrop(_hwndDropTarget);
        _hwndDropTarget = NULL;
    };
    void SetWindowHandle(HWND hwnd)
    {
        _hwnd = hwnd;
    };

protected:
    PBInPlace(LPUNKNOWN pUnkOuter);
    HRESULT Init(LPPBCTRL pPBCtrl, LPCLASSDESCRIPTOR pClass);
    ~PBInPlace(void);

    DECLARE_PRIVATE_IUNKNOWN(PBInPlace);

    //
    // helpers used by base classes
    //
    virtual HWND AttachWin(HWND hwndParent);
    virtual void CreateUI(void);
    virtual void DestroyUI(void);
    virtual void InstallFrameUI(void);
    virtual void RemoveFrameUI(void);

    virtual void SetFocus(HWND hwnd);

    //
    // windows message handling
    //
    BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct);
    void OnDestroy(HWND hwnd);
    void OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos);
    void OnPaint(HWND hwnd);
    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc);

    HMENU LoadIPServerMenu(void);
    void SwitchIPContext(BOOL fBeInPlace);
    void SetPaintWindowPos(void);

    HWND _hwndPBrush;           // used during context-switches
    HMENU _hmenuPBrush;         // used during context-switches
    RECT _rcPBrush;             // used during context-switches
    LPTOOLTRAY _trayTool;       // drawing  and LSize tool container
    LPCOLORTRAY _trayColor;     // color-picker tool container
    RECT _rcVis;                // from last SetObjectRects call

    //
    // drag-drop stuff
    //
    HWND _hwndDropTarget;           // handle of the window we work with
    BOOL _fCanDrop;                 // TRUE if they have a format we know
    SIZE _sizeObj;                  // size of object to drop
    POINT _ptOffset;                // point on object where mouse went down
    POINT _ptLast;                  // last known point
    RECT _rcLastFeedback;           // last feedback rectangle drawn
};

#define MAX_VERBNAME_LEN    64

//+---------------------------------------------------------------
//
//  Class:      PBCtrl
//
//  Purpose:    Manages the control aspect of server
//
//  Notes:      Our objects are composed of three subobjects:
//              a PBCtrl subobject, a PBDV subobject, and an
//              PBInPlace subobject.  Each of these is derived from
//              a corresponding Srvr base class.
//
//---------------------------------------------------------------

class PBCtrl: public SrvrCtrl
{
public:
    static BOOL ClassInit(LPCLASSDESCRIPTOR pClass);
    static HRESULT Create(LPUNKNOWN pUnkOuter, LPCLASSDESCRIPTOR pClass,
                            LPUNKNOWN FAR* ppUnkCtrl, LPPBCTRL FAR* ppObj);

    // we are an aggregatable object so we use a delegating IUnknown
    DECLARE_DELEGATING_IUNKNOWN(PBCtrl);

    STDMETHOD(GetMoniker) (DWORD dwAssign,
                            DWORD dwWhichMoniker,
                            LPMONIKER FAR* ppmk);

    STDMETHOD(IsUpToDate) (void);

    void GetHostNames(LPTSTR FAR* plpstrCntrApp, LPTSTR FAR* plpstrCntrObj);
    void Lock(void);
    void UnLock(void);

    //
    // base-class virtuals overridden to do additional,
    // server-specific processing
    //
    virtual HRESULT PassiveToLoaded();
    virtual HRESULT LoadedToPassive();

    virtual HRESULT LoadedToRunning();
    virtual HRESULT RunningToLoaded();

    virtual HRESULT RunningToOpened();
    virtual HRESULT OpenedToRunning();

    virtual HRESULT RunningToInPlace();
    virtual HRESULT InPlaceToRunning();
    virtual HRESULT InPlaceToUIActive();
    virtual HRESULT UIActiveToInPlace();

    HWND GetFrameWindow(void)
    {
        return _pInPlace != NULL ?
                ((LPPBINPLACE)_pInPlace)->GetFrameWindow() : NULL;
    };

    static OLECHAR gachEditVerb[MAX_VERBNAME_LEN];
    static OLECHAR gachOpenVerb[MAX_VERBNAME_LEN];

protected:
    PBCtrl(LPUNKNOWN pUnkOuter);
    HRESULT Init(LPCLASSDESCRIPTOR pClass);
    virtual ~PBCtrl(void);

    DECLARE_PRIVATE_IUNKNOWN(PBCtrl);

    LPUNKNOWN _pDVCtrlUnk;          // controlling unknown for DV subobj
    LPUNKNOWN _pIPCtrlUnk;          // controlling unknown for InPlace subobj
    int _cLock;
};

//+---------------------------------------------------------------
//
//  Class:      PBHeader
//
//  Purpose:    Document information placed at the head of our
//              contents stream
//
//---------------------------------------------------------------

class PBHeader
{
public:
    PBHeader();
    HRESULT Read(LPSTREAM pStrm);
    HRESULT Write(LPSTREAM pStrm);

    SIZEL _sizel;                       // our presentation size (HIMETRIC)
    DWORD _dwNative;                    // size of native data
};


//+---------------------------------------------------------------
//
//  Class:      PBDV
//
//  Purpose:    The data/view subobject of our CD object
//
//---------------------------------------------------------------

class PBDV: public SrvrDV
{
public:
    static BOOL ClassInit(LPCLASSDESCRIPTOR pClass);

    static HRESULT Create(LPPBCTRL pCtrl,
            LPCLASSDESCRIPTOR pClass,
            LPUNKNOWN FAR* ppUnkCtrl,
            LPPBDV FAR* ppObj);

    static HRESULT Create(LPPBDV pPBDV,
            LPCLASSDESCRIPTOR pClass,
            LPUNKNOWN FAR* ppUnkCtrl,
            LPPBDV FAR* ppObj);

    // we use standard aggregation for delegation to the control subobject
    DECLARE_DELEGATING_IUNKNOWN(PBDV);

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
    virtual HRESULT GetClipboardCopy(LPSRVRDV FAR* ppDV);

    void SetNativeExtents( int cx, int cy)
    {
        _sizel.cx =  HimetricFromHPix(cx);
        _sizel.cy =  HimetricFromVPix(cy);
    }

protected:
    virtual HRESULT LoadFromStorage(LPSTORAGE pStg);
    virtual HRESULT SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad);

    // constructors, initializers, and destructors
    PBDV(LPUNKNOWN pUnkOuter);
    HRESULT Init(LPPBCTRL pCtrl, LPCLASSDESCRIPTOR pClass);
    HRESULT Init(LPPBDV pDV, LPCLASSDESCRIPTOR pClass);
    virtual ~PBDV(void);

    DECLARE_PRIVATE_IUNKNOWN(PBDV);

    //
    // native data
    //
    PBHeader _header;           // global properties for the document
};


#endif  //!RC_INVOKED

#endif  //__PBS_HXX

