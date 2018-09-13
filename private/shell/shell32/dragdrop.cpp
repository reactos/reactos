#include "shellprv.h"
#include "defview.h"
#include "lvutil.h"
#include "ids.h"
#include "idlcomm.h"
#pragma hdrstop

#include "datautil.h"
#include "apithk.h"
#include "multimon.h"

#define MONITORS_MAX    16  // Is this really the max?

#define TF_DRAGIMAGES 0x02000000
#define DRAGDROP_ALPHA 120
#define MAX_WIDTH_ALPHA 200
#define MAX_HEIGHT_ALPHA 200

#define CIRCULAR_ALPHA   // Circular Alpha Blending Centered on Center of image

class CDragImages : public IDragSourceHelper, IDropTargetHelper
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() { return 1; };      // One global Com object per process
    STDMETHODIMP_(ULONG) Release() { return 1; };     // One global Com object per process

    // IDragSourceHelper methods
    STDMETHODIMP InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject* pDataObject);
    STDMETHODIMP InitializeFromWindow(HWND hwnd, POINT* ppt, IDataObject* pDataObject);

    // IDropTargetHelper methods
    STDMETHODIMP DragEnter(HWND hwndTarget, IDataObject* pDataObject, POINT* ppt, DWORD dwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP DragOver(POINT* ppt, DWORD dwEffect);
    STDMETHODIMP Drop(IDataObject* pDataObject, POINT* ppt, DWORD dwEffect);
    STDMETHODIMP Show(BOOL fShow);

    // These are public so the DAD_* routines can access.
    BOOL IsDragging()           { return (IsValid() && _Single.bDragging);              };
    BOOL IsDraggingImage()      { return (IsValid() && _fImage && _Single.bDragging);   };
    BOOL IsDraggingLayeredWindow() { return _shdi.hbmpDragImage != NULL; };
    BOOL SetDragImage(HIMAGELIST himl, int index, POINT * pptOffset);
    void SetDragCursor(int idCursor);

    BOOL IsValid();
    void Validate();
    void Invalidate();
    BOOL IsLayeredSupported();
    inline DWORD GetThread() { return _idThread; };

    BOOL DehydrateToDataObject(IDataObject* pDataObject);
    BOOL RehydrateFromDataObject(IDataObject* pDataObject);

    BOOL ReadBitmapBits(HGLOBAL hGlobal);
    BOOL WriteBitmapBits(HGLOBAL* phGlobal);

    void ThreadDetach();
    void ProcessDetach();

    BOOL ShowDragImageInterThread(HWND hwndLock, BOOL * pfShow);

    // MultiRectDragging
    void _MultipleDragShow(BOOL bShow);
    void _MultipleDragStart(HWND hwndLock, LPRECT aRect, int nRects, POINT ptStart, POINT ptOffset);
    void _MultipleDragMove(POINT ptNew);
    BOOL _SetMultiItemDragging(HWND hwndLV, int cItems, LPPOINT pptOffset);
    BOOL _SetMultiRectDragging(int cItems, LPRECT prect, LPPOINT pptOffset);

    // Merged Cursors
    HBITMAP CreateColorBitmap(int cx, int cy);
    void _DestroyCachedCursors();
    void _GetCursorLowerRight(HCURSOR hcursor, int * px, int * py, POINT *pptHotSpot);
    int _MapCursorIDToImageListIndex(int idCur);
    int _AddCursorToImageList(HCURSOR hcur, LPCTSTR idMerge, POINT *pptHotSpot);
    BOOL _MergeIcons(HCURSOR hcursor, LPCTSTR idMerge, HBITMAP *phbmImage, HBITMAP *phbmMask, POINT* pptHotSpot);
    void _SetDropEffectCursor(int idCur);
    HCURSOR SetCursorHotspot(HCURSOR hcur, POINT *ptHot);

    CDragImages() {};
    // Helper Routines
    BOOL _CreateDragWindow();
    BOOL _PreProcessDragBitmap(void** ppvBits);
    BOOL _IsTooBigForAlpha();

    // Member Variables
    SHDRAGIMAGE     _shdi;
    HWND            _hwndTarget;
    HWND            _hwndFrom;      // The HWND that is sourcing this drag
    HWND            _hwnd;          // The HWND of the Layered Window
    HDC             _hdcDragImage;
    HBITMAP         _hbmpOld;

    BITBOOL         _fInitializedFromBitmap: 1;
    BITBOOL         _fLayeredSupported: 1;

    // Legacy drag support
    BOOL        _fImage;
    POINT       _ptOffset;
    DWORD       _idThread;
    HIMAGELIST  _himlCursors;
    UINT        _cRev;
    int         _aindex[DCID_MAX]; // will be initialized.
    HCURSOR     _ahcur[DCID_MAX];
    POINT       _aptHotSpot[DCID_MAX];
    int         _idCursor;

    BITBOOL         _fValid;

    // _Single struct is used between DAD_Enter and DAD_Leave
    struct
    {
        // Common part
        BOOL    bDragging;
        BOOL    bLocked;
        HWND    hwndLock;
        BOOL    bSingle;    // Single imagelist dragging.
        DWORD   idThreadEntered;

        // Multi-rect dragging specific part
        struct 
        {
            BOOL bShown;
            LPRECT pRect;
            int nRects;
            POINT ptOffset;
            POINT ptNow;
        } _Multi;
    } _Single;

    // following fields are used only when fImage==FALSE
    RECT*       _parc;         // cItems
    UINT        _cItems;         // This is a sentinal. Needs to be the last item.
};

//
// Read 'Notes' in CDropSource_GiveFeedback for detail about this
// g_fDraggingOverSource flag, which is TRUE only if we are dragging
// over the source window itself with left mouse button
// (background and large/small icon mode only).
//
UINT g_cRev = 0;
CDragImages* g_pdiDragImages = NULL;
int g_iGetDragImage = 0;
BOOL g_fDraggingOverSource = FALSE;

STDAPI CDragImages_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut)
{
    ASSERT(pUnkOuter == NULL);  //Who's trying to aggregate us?
    if (!g_pdiDragImages)
        g_pdiDragImages = new CDragImages();

    if (g_pdiDragImages && ppvOut)
        return g_pdiDragImages->QueryInterface(riid, ppvOut);

    return E_OUTOFMEMORY;
}

STDMETHODIMP CDragImages::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDragImages, IDragSourceHelper),
        QITABENT(CDragImages, IDropTargetHelper),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

BOOL CDragImages::_CreateDragWindow()
{
#ifdef WINNT
    if (_hwnd == NULL)
    {
        WNDCLASS wc = {0};

        wc.hInstance       = g_hinst;
        wc.lpfnWndProc     = DefWindowProc;
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName   = TEXT("SysDragImage");
        wc.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1); // NULL;
        if (!SHRegisterClass(&wc))
            return FALSE;

        _hwnd = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, 
            TEXT("SysDragImage"), TEXT("Drag"), WS_POPUPWINDOW,
            0, 0, 50, 50, NULL, NULL, g_hinst, NULL);

        if (!_hwnd)
            return FALSE;

        //
        // This window should not be mirrored so that the image contents won't be flipped. [samera]
        //
        SetWindowBits(_hwnd, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);
    }

    return TRUE;
#else
    return FALSE;
#endif
}

BOOL CDragImages::IsValid()
{ 
    return _fValid; 
}

void CDragImages::Invalidate()
{
    _fValid = FALSE;

    // Make sure we destroy the cursors on an invalidate.
    if (_himlCursors)
        _DestroyCachedCursors();

    // Do we have an array?
    if (_parc)
    {
        delete _parc;
        _parc = NULL;
    }

    if (_fImage)
        ImageList_EndDrag();


    if (_hbmpOld)
    {
        SelectObject(_hdcDragImage, _hbmpOld);
        _hbmpOld = NULL;
    }

    if (_hdcDragImage)
    {
        DeleteDC(_hdcDragImage);
        _hdcDragImage = NULL;
    }

    if (_shdi.hbmpDragImage)
        DeleteObject(_shdi.hbmpDragImage);

    ZeroMemory(&_Single, sizeof(_Single));
    ZeroMemory(&_shdi, sizeof(_shdi));

    _ptOffset.x = 0;
    _ptOffset.y = 0;

    _hwndTarget = _hwndFrom = _hwnd = NULL;
    _fValid = _fInitializedFromBitmap = _fLayeredSupported = FALSE;
    _fImage = FALSE;
    _idThread = 0;
    _himlCursors = NULL;
    _cRev = 0;
    _idCursor = 0;
}

void CDragImages::Validate()
{
    _fValid = TRUE;
    _idThread  = GetCurrentThreadId();
    if (_himlCursors && _cRev != g_cRev)
        _DestroyCachedCursors();

    if (_himlCursors == NULL)
    {
        UINT uFlags = ILC_MASK | ILC_SHARED;
        if(IS_BIDI_LOCALIZED_SYSTEM())
        {
            uFlags |= ILC_MIRROR;
        }
        HDC hdc;

        //
        // if this is not a palette device, use a DDB for the imagelist
        // this is important when displaying high-color cursors
        //
        hdc = GetDC(NULL);
        if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
        {
            uFlags |= ILC_COLORDDB;
        }
        ReleaseDC(NULL, hdc);

        _himlCursors = ImageList_Create(GetSystemMetrics(SM_CXCURSOR),
                                        GetSystemMetrics(SM_CYCURSOR),
                                        uFlags, 1, 0);

        _cRev = g_cRev;

        // We need to initialize s_cursors._aindex[*]
        _MapCursorIDToImageListIndex(DCID_INVALID);
    }
}

BOOL AreAllMonitorsAtLeast(int iBpp)
{
    DISPLAY_DEVICE DisplayDevice;
    BOOL fAreAllMonitorsAtLeast = TRUE;


    for (int iEnum = 0; fAreAllMonitorsAtLeast && iEnum < MONITORS_MAX; iEnum++)
    {
        ZeroMemory(&DisplayDevice, sizeof(DISPLAY_DEVICE));
        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);

        if (EnumDisplayDevices(NULL, iEnum, &DisplayDevice, 0) &&
            (DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {

            HDC hdc = CreateDC(NULL, (LPTSTR)DisplayDevice.DeviceName, NULL, NULL);
            if (hdc)
            {
                int iBits = GetDeviceCaps(hdc, BITSPIXEL);

                if (iBits < iBpp)
                    fAreAllMonitorsAtLeast = FALSE;

                DeleteDC(hdc);
            }
        }
    }


    return fAreAllMonitorsAtLeast;
}

BOOL CDragImages::IsLayeredSupported()
{
    // For the first rev, we will only support Layered drag images
    // when the Color depth is greater than 65k colors.

    // We should ask everytime....
    _fLayeredSupported = FALSE;
#ifdef WINNT
    if (g_bRunOnNT5)
    {
        _fLayeredSupported = AreAllMonitorsAtLeast(16);
        
        if (_fLayeredSupported)
        {
            BOOL bDrag;
            if (SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &bDrag, 0))
            {
                _fLayeredSupported = BOOLIFY(bDrag);
            }


            if (_fLayeredSupported)
                _fLayeredSupported = SHRegGetBoolUSValue(REGSTR_PATH_EXPLORER TEXT("\\Advanced"), TEXT("NewDragImages"), FALSE, TRUE);
        }
    }
#endif
    return _fLayeredSupported;
}


//////////////////////////////////////
//
//  IDragImages::InitializeFromBitmap
//
//  Purpose: To initialize the static drag image manager from a structure
//           this is implemented for WindowLess controls that can act as a
//           drag source.
//
STDMETHODIMP CDragImages::InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject* pDataObject)
{
    TraceMsg(TF_DRAGIMAGES, "CDragImages::InitializeFromBitmap");
    // We don't support being initialized from a bitmap when Layered Windows are not supported
    if (!IsLayeredSupported())
        return E_FAIL;

    RIP(pshdi);
    RIP(IsValidHANDLE(pshdi->hbmpDragImage));
    HRESULT hres = E_FAIL;

    _shdi = *pshdi;     // Keep a copy of this.

    _idCursor = -1;     // Initialize this... This is an arbitraty place and can be put 
                        // anywhere before the first Setcursor call
    Validate();

    if (DehydrateToDataObject(pDataObject))
    {
        // We only want to initialize once between the DragSource and here...
        _fInitializedFromBitmap = TRUE;
        hres = S_OK;
    }

    if (FAILED(hres))
        Invalidate();
    return hres;
}

//////////////////////////////////////
//
//  IDragImages::InitializeFromWindow
//
//  Purpose: To initialize the static drag image manager from an HWND that
//           can process the RegisteredWindowMessage(DI_GETDRAGIMAGE)
//
STDMETHODIMP CDragImages::InitializeFromWindow(HWND hwnd, POINT* ppt, IDataObject* pDataObject)
{
    TraceMsg(TF_DRAGIMAGES, "CDragImages::InitializeFromWindow HWND 0x%x", hwnd);

    RIP(IsValidHWND(hwnd));
    HRESULT hres = E_FAIL;

    if (IsLayeredSupported())
    {
        // Register the message that gets us the Bitmap from the control.
        if (g_iGetDragImage == 0)
        {
            g_iGetDragImage = RegisterWindowMessage(DI_GETDRAGIMAGE);

            if (g_iGetDragImage == 0)
                goto DoOldMethod;
        }

        _hwndFrom = hwnd;

        // Can this HWND generate a drag image for me?
        if (SendMessage(hwnd, g_iGetDragImage, 0, (LPARAM)&(_shdi)))
        {
            // Yes; Now we select that into the window 
            if (FAILED(InitializeFromBitmap(&(_shdi), pDataObject)))
                goto DoOldMethod;

            hres = S_OK;
        }
        else
            goto DoOldMethod;
    }
    else
    {
DoOldMethod:
        TCHAR szClassName[50];
        TraceMsg(TF_DRAGIMAGES, "CDragImages::InitializeFromWindow :: Layering Not supported");

        if (GetClassName(hwnd, szClassName, SIZEOF(szClassName))) 
        {
            if (lstrcmpi(szClassName, TEXT("SysListView32")) == 0)
            {
                //
                // Count the number of selected items.
                //
                POINT ptTemp;
                HIMAGELIST himl;
                POINT ptOffset;

                if (ppt)
                    ptOffset = *ppt;

                int cItems = ListView_GetSelectedCount(hwnd);

                switch (cItems)
                {
                case 0:
                    // There's nothing to drag
                    break;

                case 1:
                    if (NULL != (himl = ListView_CreateDragImage(hwnd,
                            ListView_GetNextItem(hwnd, -1, LVNI_SELECTED), &ptTemp)))
                    {
                        TraceMsg(TF_DRAGIMAGES, "CDragImages::InitializeFromWindow :: Created single Drag Image");
                        ClientToScreen(hwnd, &ptTemp);
                        ptOffset.x -= ptTemp.x;

                        //
                        // Since the listview is mirrored, then mirror the selected
                        // icon coord. This would result in negative offset so let's
                        // compensate. [samera]
                        //
                        if (IS_WINDOW_RTL_MIRRORED(hwnd))
                            ptOffset.x *= -1;

                        ptOffset.y -= ptTemp.y;
                        SetDragImage(himl, 0, &ptOffset);
                        ImageList_Destroy(himl);
                        hres = S_OK;
                    }
                    break;

                default:
                    {
                        TraceMsg(TF_DRAGIMAGES, "CDragImages::InitializeFromWindow :: Created Multiple Drag Rects");

                        hres = _SetMultiItemDragging(hwnd, cItems, &ptOffset)? S_OK: E_FAIL;
                    }
                }
            }
            else if (lstrcmpi(szClassName, TEXT("SysTreeView32")) == 0)
            {

                HIMAGELIST himlDrag = TreeView_CreateDragImage(hwnd, NULL);
                if (himlDrag) 
                {
                    SetDragImage(himlDrag, 0, NULL);
                    ImageList_Destroy(himlDrag);
                    hres = S_OK;
                }
            }
        }
    }

    return hres;
}

//////////////////////////////////////
//
//  IDragImages::DragEnter
//
//  Purpose: To create the drag window in the layered window case, or to begin drawing the 
//           Multi Rect or icon drag images.
//
STDMETHODIMP CDragImages::DragEnter(HWND hwndTarget, IDataObject* pDataObject, POINT* ppt, DWORD dwEffect)
{
    HRESULT hres = E_FAIL;
    TraceMsg(TF_DRAGIMAGES, "CDragImages::DragEnter");

    if (RehydrateFromDataObject(pDataObject))
    {
        _hwndTarget = hwndTarget ? hwndTarget : GetDesktopWindow();
        SetDragCursor(DCID_INVALID);
        _Single.bDragging = TRUE;
        _Single.bSingle = _fImage;
        _Single.hwndLock = _hwndTarget;
        _Single.bLocked = FALSE;
        _Single.idThreadEntered = GetCurrentThreadId();

        if (_shdi.hbmpDragImage)
        {
#ifdef WINNT
            TraceMsg(TF_DRAGIMAGES, "CDragImages::DragEnter : Creating Drag Window");
            // At this point the information has been read from the data object. 
            // Reconstruct the HWND if necessary
            if (_CreateDragWindow() && _hdcDragImage)
            {
                POINT ptSrc = {0, 0};
                POINT pt;


                SetWindowPos(_hwnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | 
                    SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

                GetMsgPos(&pt);
            

                pt.x -= _shdi.ptOffset.x;
                pt.y -= _shdi.ptOffset.y;

                BLENDFUNCTION blend;
                blend.BlendOp = AC_SRC_OVER;
                blend.BlendFlags = 0;
                blend.AlphaFormat = AC_SRC_ALPHA;
                blend.SourceConstantAlpha = 0xFF /*DRAGDROP_ALPHA*/;

                HDC hdc = GetDC(_hwnd);
                if (hdc)
                {
                    DWORD fULWType = ULW_ALPHA;

#ifndef CIRCULAR_ALPHA   // Circular Alpha Blending Centered on Center of image
                    if (_IsTooBigForAlpha())
                        fULWType = ULW_COLORKEY;
#endif
                   
                    // Should have been preprocess already
                    UpdateLayeredWindow(_hwnd, hdc, &pt, &(_shdi.sizeDragImage), 
                                        _hdcDragImage, &ptSrc, _shdi.crColorKey,
                                        &blend, fULWType);

                    ReleaseDC(_hwnd, hdc);
                }
                hres = S_OK;
            }
#endif
        }
        else
        {
            // These are in Client Cordinates, not screen coords. Translate:
            POINT pt = *ppt;
            RECT rc;
            GetWindowRect(_hwndTarget, &rc);
            pt.x -= rc.left;
            pt.y -= rc.top;
            if (_fImage)
            {

                TraceMsg(TF_DRAGIMAGES, "CDragImages::DragEnter : Dragging an image");
                // Avoid the flicker by always pass even coords
                ImageList_DragEnter(hwndTarget, pt.x & ~1, pt.y & ~1);
                hres = S_OK;
            }
            else
            {
                TraceMsg(TF_DRAGIMAGES, "CDragImages::DragEnter : Dragging mutli rects");
                _MultipleDragStart(hwndTarget, _parc, _cItems, pt, _ptOffset);
                hres = S_OK;
            }
        }

        //
        // We should always show the image whenever this function is called.
        //
        Show(TRUE);
    }
    return hres;
}

//////////////////////////////////////
//
//  IDragImages::DragLeave
//
//  Purpose: To kill the Layered Window, or to stop painting the icon or rect drag images
//
STDMETHODIMP CDragImages::DragLeave()
{
    TraceMsg(TF_DRAGIMAGES, "CDragImages::DragLeave");
    if (IsValid())
    {
        if (_hwnd)
        {
            // If we're leaving, Destroy the Window
            DestroyWindow(_hwnd);
            _hwnd = NULL;
            Invalidate();
        }
        else if (_Single.bDragging &&
             _Single.idThreadEntered == GetCurrentThreadId())
        {
            Show(FALSE);

            if (_fImage)
            {
                ImageList_DragLeave(_Single.hwndLock);
            }

            _Single.bDragging = FALSE;

            DAD_SetDragImage((HIMAGELIST)-1, NULL);
        }
    }

    return S_OK;
}

//////////////////////////////////////
//
//  IDragImages::DragOver
//
//  Purpose: To move the Layered window or to rerender the icon or rect images within
//           the Window they are over.
//
STDMETHODIMP CDragImages::DragOver(POINT* ppt, DWORD dwEffect)
{
    if (IsValid())
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::DragOver pt {%d, %d}", ppt->x, ppt->y);
        // Avoid the flicker by always pass even coords
        ppt->x &= ~1;
        ppt->y &= ~1;

        if (IsDraggingLayeredWindow())
        {
#ifdef WINNT
            POINT pt;
            GetCursorPos(&pt);
            pt.x -= _shdi.ptOffset.x;
            pt.y -= _shdi.ptOffset.y;

            SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | 
                SWP_NOSIZE | SWP_SHOWWINDOW);

            UpdateLayeredWindow(_hwnd, NULL, &pt, NULL, NULL, NULL, 0,
                NULL, 0);
#endif
        }
        else
        {
            // These are in Client Cordinates, not screen coords. Translate:
            POINT pt = *ppt;
            RECT rc;
            GetWindowRect(_hwndTarget, &rc);
            pt.x -= rc.left;
            pt.y -= rc.top;
            if (_fImage)
            {
                ImageList_DragMove(pt.x, pt.y);
            }
            else
            {
                _MultipleDragMove(pt);
            }
        }
    }

    return S_OK;
}

//////////////////////////////////////
//
//  IDragImages::Drop
//
//  Purpose: To do any cleanup after a drop (Currently calls DragLeave)
//
STDMETHODIMP CDragImages::Drop(IDataObject* pDataObject, POINT* ppt, DWORD dwEffect)
{
    return DragLeave();
}

//////////////////////////////////////
//
//  IDragImages::InitializeFromBitmap
//
//  Purpose: To initialize the static drag image manager from a structure
//           this is implemented for WindowLess controls that can act as a
//           drag source.
//
void CDragImages::SetDragCursor(int idCursor)
{
    //
    // Ignore if we are dragging over ourselves.
    //
    if (IsDraggingImage())
    {
        POINT ptHotSpot;

        if (_himlCursors && (idCursor != DCID_INVALID))
        {
            int iIndex = _MapCursorIDToImageListIndex(idCursor);
            if (iIndex != -1) 
            {
                ImageList_GetDragImage(NULL, &ptHotSpot);
                ptHotSpot.x -= _aptHotSpot[idCursor].x;
                ptHotSpot.y -= _aptHotSpot[idCursor].y;
                if (ptHotSpot.x < 0)
                {
                    ptHotSpot.x = 0;
                }

                if (ptHotSpot.y < 0)
                {
                    ptHotSpot.y = 0;
                }

                ImageList_SetDragCursorImage(_himlCursors, iIndex, ptHotSpot.x, ptHotSpot.y);
            } 
            else 
            {
                // You passed a bad Cursor ID.
                ASSERT(0);
            }
        }

        _idCursor = idCursor;
    }
}

// Reads the written information to recreate the drag image
BOOL CDragImages::ReadBitmapBits(HGLOBAL hGlobal)
{
    BITMAPINFO      bmi = {0};
    void*           pvDragStuff = (void*)GlobalLock(hGlobal);
    BOOL            fRet = FALSE;
    HDC             hdcScreen;

    if (IsValid())
        return FALSE;

    ASSERT(_shdi.hbmpDragImage == NULL);
    ASSERT(_hdcDragImage == NULL);

    hdcScreen = GetDC(NULL);

    if (hdcScreen)
    {
        void* pvBits;
        CopyMemory(&_shdi, pvDragStuff, sizeof(SHDRAGIMAGE));

        // Create a buffer to read the bits into
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = _shdi.sizeDragImage.cx;
        bmi.bmiHeader.biHeight      = _shdi.sizeDragImage.cy;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        // Next create a DC and an HBITMAP.
        _hdcDragImage = CreateCompatibleDC(hdcScreen);

        if (!_hdcDragImage)
            goto Out;

        _shdi.hbmpDragImage = CreateDIBSection(_hdcDragImage, &bmi, DIB_RGB_COLORS, &pvBits, NULL, NULL);

        if (!_shdi.hbmpDragImage)
            goto Out;

        _hbmpOld = (HBITMAP)SelectObject(_hdcDragImage, _shdi.hbmpDragImage);

        // then Set the bits into the Bitmap
        RGBQUAD* pvStart = (RGBQUAD*)((BYTE*)pvDragStuff + sizeof(SHDRAGIMAGE));
        DWORD dwCount = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
        CopyMemory((RGBQUAD*)pvBits, (RGBQUAD*)pvStart, dwCount);
        fRet = TRUE;
    }

Out:
    GlobalUnlock(hGlobal);
    if (hdcScreen)
        ReleaseDC(NULL, hdcScreen);
    return fRet;

}

// Writes the written information to recreate the drag image
BOOL CDragImages::WriteBitmapBits(HGLOBAL* phGlobal)
{
    BITMAPINFO      bmi = {0};
    void*           pvDragStuff;
    BOOL            fRet = FALSE;
    DWORD           cbImageSize;

    if (!IsValid())
        return FALSE;

    ASSERT(_shdi.hbmpDragImage);

    cbImageSize = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);

    *phGlobal = GlobalAlloc(GPTR, cbImageSize + sizeof(SHDRAGIMAGE));

    if (*phGlobal)
    {
        void* pvBits;
        pvDragStuff = GlobalLock(*phGlobal);
        CopyMemory(pvDragStuff, &_shdi, sizeof(SHDRAGIMAGE));

        fRet = _PreProcessDragBitmap(&pvBits);
        
        if (fRet)
        {
            RGBQUAD* pvStart = (RGBQUAD*)((BYTE*)pvDragStuff + sizeof(SHDRAGIMAGE));
            DWORD dwCount = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
            CopyMemory((RGBQUAD*)pvStart, (RGBQUAD*)pvBits, dwCount);
        }
        GlobalUnlock(*phGlobal);
    }
    return fRet;
}

BOOL CDragImages::_IsTooBigForAlpha()
{
    BOOL fTooBig = FALSE;
    int dSelectionArea = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy;

    // The number here is "It just feels right" or 
    // about 3 Thumbnail icons linned up next to each other.
    if ( dSelectionArea > 0x10000 )
        fTooBig = TRUE;

    return fTooBig;
}


BOOL IsColorKey(RGBQUAD rgbPixel, COLORREF crKey)
{
    // COLORREF is backwards to RGBQUAD
    return InRange( rgbPixel.rgbBlue,  ((crKey & 0xFF0000) >> 16) - 5, ((crKey & 0xFF0000) >> 16) + 5) &&
           InRange( rgbPixel.rgbGreen, ((crKey & 0x00FF00) >>  8) - 5, ((crKey & 0x00FF00) >>  8) + 5) &&
           InRange( rgbPixel.rgbRed,   ((crKey & 0x0000FF) >>  0) - 5, ((crKey & 0x0000FF) >>  0) + 5);
}

#ifdef RADIAL
int QuickRoot(int n, int iNum)
{

    int iRoot = iNum;
    for (int i=10; i > 0; i--)
    {
        int iOld = iRoot;
        iRoot = (iRoot + iNum/iRoot)/2;
        if (iRoot == iOld)
            break;
    }

    return iRoot;
}

#endif

BOOL CDragImages::_PreProcessDragBitmap(void** ppvBits)
{
    BOOL            fRet = FALSE;

    TraceMsg(TF_DRAGIMAGES, "CDragImages::_PreProcessDragBitmap");

    ASSERT(_hdcDragImage == NULL);
    _hdcDragImage = CreateCompatibleDC(NULL);

    if (_hdcDragImage)
    {
        ULONG*          pul;
        HBITMAP         hbmpResult = NULL;
        HBITMAP         hbmpOld;
        HDC             hdcSource = NULL;
        BITMAPINFO      bmi = {0};
        HBITMAP         hbmp = _shdi.hbmpDragImage;

        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = _shdi.sizeDragImage.cx;
        bmi.bmiHeader.biHeight      = _shdi.sizeDragImage.cy;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        hbmpResult = CreateDIBSection(_hdcDragImage,
                                   &bmi,
                                   DIB_RGB_COLORS,
                                   ppvBits,
                                   NULL,
                                   0);

        hdcSource = CreateCompatibleDC(_hdcDragImage);

        if (hdcSource && hbmpResult)
        {
            _hbmpOld = (HBITMAP)SelectObject(_hdcDragImage, hbmpResult);
            hbmpOld = (HBITMAP)SelectObject(hdcSource, hbmp);

            BitBlt(_hdcDragImage, 0, 0, _shdi.sizeDragImage.cx, _shdi.sizeDragImage.cy,
                   hdcSource, 0, 0, SRCCOPY);

#ifdef CIRCULAR_ALPHA   // Circular Alpha Blending Centered on Center of image
            pul = (ULONG*)*ppvBits;

            int iOffsetX = _shdi.ptOffset.x;
            int iOffsetY = _shdi.ptOffset.y;
            int iDenomX = max(_shdi.sizeDragImage.cx - iOffsetX, iOffsetX);
            int iDenomY = max(_shdi.sizeDragImage.cy - iOffsetY, iOffsetY);
#ifdef RADIAL
            int iRadius = min(_shdi.sizeDragImage.cy, _shdi.sizeDragImage.cx);
#endif
            BOOL fRadialFade = TRUE;
            // If both are less than the max, then no radial fade.
            if (_shdi.sizeDragImage.cy <= MAX_HEIGHT_ALPHA && _shdi.sizeDragImage.cx <= MAX_WIDTH_ALPHA)
                fRadialFade = FALSE;

            for (int Y = 0; Y < _shdi.sizeDragImage.cy; Y++)
            {
                int y = _shdi.sizeDragImage.cy - Y; // Bottom up DIB.
                for (int x = 0; x < _shdi.sizeDragImage.cx; x++)
                {
                    RGBQUAD* prgb = (RGBQUAD*)&pul[Y * _shdi.sizeDragImage.cx + x];

                    if (IsColorKey(*prgb, _shdi.crColorKey))
                    {
                        // Write a pre-multiplied value of 0:

                        *((DWORD*)prgb) = 0;
                    }
                    else
                    {
                        int Alpha = DRAGDROP_ALPHA;
                        if (fRadialFade)
                        {
#ifdef RADIAL
                            // This generates a Nice smooth curve, but is computationally expensive (Square root per pixel)
                            // that's 10*2+1 divides plus a left shift per pixel
                            __int64 iRad = QuickRoot(1, (x * x) + (y * y));

                            // BUGBUG(lamadio): if iRad is > iRadius, Alpha should be zero.

                            int Alpha = 0xF0 - (int)(((0xF0 * iRad << 10)/ iRadius) >> 10);
#else
                            // This is also expensive, but not as. It does not generate a smooth curve, but this is just
                            // an effect, not trying to be accurate here.

                            // 3 devides per pixel
                            int ddx = (x < iOffsetX)? iOffsetX - x : x - iOffsetX;
                            int ddy = (y < iOffsetY)? iOffsetY - y : y - iOffsetY;

                            __int64 iAlphaX = (100000l - (((__int64)ddx * 100000l) / (iDenomX )));
                            __int64 iAlphaY = (100000l - (((__int64)ddy * 100000l) / (iDenomY )));

                            ASSERT (iAlphaX >= 0);
                            ASSERT (iAlphaY >= 0);

                            __int64 iDenom = 100000;
                            iDenom *= 100000;

                            Alpha = (int) ((250 * iAlphaX * iAlphaY * 100000) / (iDenom* 141428));
#endif
                        }

                        ASSERT(Alpha <= 0xFF);
                        prgb->rgbReserved = (BYTE)Alpha;
                        prgb->rgbRed      = ((prgb->rgbRed   * Alpha) + 128) / 255;
                        prgb->rgbGreen    = ((prgb->rgbGreen * Alpha) + 128) / 255;
                        prgb->rgbBlue     = ((prgb->rgbBlue  * Alpha) + 128) / 255;
                    }
                }
            }

#else
            if ( !_IsTooBigForAlpha())
            {
                pul = (ULONG*)*ppvBits;

                for (int i = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy; 
                     i != 0; 
                     --i)
                {
                    if (IsColorKey(*(RGBQUAD*)pul, _shdi.crColorKey))
                    {
                        // Write a pre-multiplied value of 0:

                        *pul = 0;
                    }
                    else
                    {
                        // Where the bitmap is not the transparent color, change the
                        // alpha value to opaque:
                        RGBQUAD* prgb = (RGBQUAD*) pul;
                        prgb->rgbReserved = DRAGDROP_ALPHA;
                        prgb->rgbRed      = ((prgb->rgbRed   * DRAGDROP_ALPHA) + 128) / 255;
                        prgb->rgbGreen    = ((prgb->rgbGreen * DRAGDROP_ALPHA) + 128) / 255;
                        prgb->rgbBlue     = ((prgb->rgbBlue  * DRAGDROP_ALPHA) + 128) / 255;
                    }

                    pul++;
                }
            }
#endif

            DeleteObject(hbmp);
            _shdi.hbmpDragImage = hbmpResult;

            fRet = TRUE;
        }

        if (hbmpOld && hdcSource)
            SelectObject(hdcSource, hbmpOld);
        if (hdcSource)
            DeleteObject(hdcSource);
    }

    return fRet;
}

//=====================================================================
// Shared Code
//=====================================================================

// Streams the required information to the data object so that it can be requeried in
// another context.
BOOL CDragImages::DehydrateToDataObject(IDataObject *pdtobj)
{
    IStream *pstm;

    //Check if we have a drag context 
    // and that we have not already been dehydrated into the data object.
    if (!IsValid() || _fInitializedFromBitmap || pdtobj == NULL)
    {
        //No, We Can't do much
        return FALSE;
    }

    //Create a OLE defined Memory Stream Object.
    if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pstm)))
    {
        ULONG ulWritten;
        FORMATETC fmte;
        STGMEDIUM medium;
        DragContextHeader hdr;

        //Set the header .
        hdr.fImage   = _fImage;
        hdr.ptOffset = _ptOffset;
        hdr.fLayered = IsDraggingLayeredWindow();
       
        //First Write the drag context header
        pstm->Write(&hdr, sizeof(hdr), &ulWritten);

        //Check for success
        if (ulWritten != sizeof(hdr))
            goto WriteFailed;

        if (hdr.fLayered)
        {
            STGMEDIUM mediumBits;
            //Set the medium.
            mediumBits.tymed = TYMED_HGLOBAL;
            mediumBits.pUnkForRelease = NULL;

            //Set the Formatetc
            fmte.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(TEXT("DragImageBits"));
            fmte.ptd = NULL;
            fmte.dwAspect = DVASPECT_CONTENT;
            fmte.lindex = -1;
            fmte.tymed = TYMED_HGLOBAL;

            // Write out layered window information
            if (!WriteBitmapBits(&mediumBits.hGlobal))
                goto WriteFailed;

            //Set the medium in the data.
            if (FAILED(pdtobj->SetData(&fmte, &mediumBits, TRUE)))
            {
                ReleaseStgMedium(&mediumBits);
                goto WriteFailed;
            }
        }
        else if (_fImage)
        {
            //We need to write  an image
        
            //Get the Current Drag Image List.
            HIMAGELIST himl = ImageList_GetDragImage(NULL, NULL);
            if (!ImageList_Write(himl, pstm))
                goto WriteFailed;
        }
        else
        {
            // We need to write  multi rect stuff
            
            //First Write the number of rects.
            pstm->Write(&_cItems, sizeof(_cItems), &ulWritten);
            
            //Check for success
            if (ulWritten != sizeof(_cItems))
                goto WriteFailed;

            // Write the  rects into the stream
            pstm->Write(_parc, sizeof(RECT) * _cItems, &ulWritten);

            //Check for success
            if (ulWritten != sizeof(RECT) * _cItems)
                goto WriteFailed;
        }

        //Set the seek pointer at the beginning.
        pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

        //Set the medium.
        medium.tymed = TYMED_ISTREAM;
        medium.pstm = pstm;
        medium.pUnkForRelease = NULL;

        //Set the Formatetc
        fmte.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_DRAGCONTEXT);
        fmte.ptd = NULL;
        fmte.dwAspect = DVASPECT_CONTENT;
        fmte.lindex = -1;
        fmte.tymed = TYMED_ISTREAM;

        //Set the medium in the data.
        if (FAILED(pdtobj->SetData(&fmte, &medium, TRUE)))
        {
            ReleaseStgMedium(&medium);
            return FALSE;
        }
    }
    return TRUE;

WriteFailed:

    //Failed. Release the  stream.
    if (pstm)
        pstm->Release();
    return FALSE;
}


// Gets the information to rebuild the drag images from the data object
BOOL CDragImages::RehydrateFromDataObject(IDataObject *pdtObject)
{
    FORMATETC  fmte;
    HRESULT hr;
    STGMEDIUM medium;
    BOOL fRet = FALSE;
    TraceMsg(TF_DRAGIMAGES, "CDragImages::RehydrateFromDataObject(0x%x)", pdtObject);

    // Check if we have a drag context
    if (IsValid() || !pdtObject)
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::RehydrateFromDataObject - Already have context");
        // We already have a drag context
        return TRUE;
    }
    
    //Set the format we are interested in
    fmte.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_DRAGCONTEXT);
    fmte.ptd = NULL;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.lindex = -1;
    fmte.tymed = TYMED_ISTREAM;
    medium.pstm = NULL;
    
    //if the data object has the format we are interested in
    // then Get the data
    hr = pdtObject->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))   // if no pstm, bag out.
    {
        ULONG ulRead;
        DragContextHeader hdr;

        //Set the seek pointer at the beginning. PARANOIA: This is for people
        // Who don't set the seek for me.
        medium.pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

        //First Read the drag context header
        medium.pstm->Read(&hdr, sizeof(hdr),&ulRead);

        //Check for success
        if (ulRead != sizeof(hdr))
            goto ReadFailed;

        if (hdr.fLayered)
        {
            STGMEDIUM mediumBits;
            //Set the medium.
            mediumBits.tymed = TYMED_HGLOBAL;
            mediumBits.pUnkForRelease = NULL;

            //Set the Formatetc
            fmte.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(TEXT("DragImageBits"));
            fmte.ptd = NULL;
            fmte.dwAspect = DVASPECT_CONTENT;
            fmte.lindex = -1;
            fmte.tymed = TYMED_HGLOBAL;

            hr = pdtObject->GetData(&fmte, &mediumBits);
            if (FAILED(hr))
                goto ReadFailed;

            fRet = ReadBitmapBits(mediumBits.hGlobal);
            ReleaseStgMedium(&mediumBits);
        }
        else if (hdr.fImage)
        {
            //Read the image list in the data.
            HIMAGELIST himl = ImageList_Read(medium.pstm);
            if (!himl)
                goto ReadFailed;

            DAD_SetDragImage(himl, &(hdr.ptOffset));
            ImageList_Destroy(himl);
            fRet = TRUE;
        }
        else
        {
            //Muli Rect  Dragging
            int cItems;
            LPRECT lprect;

            //Read the number of Rects
            medium.pstm->Read(&cItems, sizeof(cItems), &ulRead);

            //Check for success
            if (ulRead != sizeof(cItems))
                goto ReadFailed;

            lprect = (LPRECT)LocalAlloc(LPTR, SIZEOF(RECT) * cItems);

            if (!lprect)
                goto ReadFailed;

            medium.pstm->Read(lprect, sizeof(RECT) * cItems, &ulRead);
            
            //Check for success
            if (ulRead != sizeof(RECT) * cItems)
                goto ReadFailed;

            _SetMultiRectDragging(cItems, lprect, &(hdr.ptOffset));
            LocalFree(lprect);

            fRet = TRUE;

        }

        Validate();
ReadFailed:
        //Set the seek pointer at the beginning. Just cleaning up...
        medium.pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

        //Release the stg medium.
        ReleaseStgMedium(&medium);
    }
    return fRet;

}


// Shows or hides the drag images. NOTE: Doesn't do anything in the layered window case.
// We don't need to because this function is specifically for drawing to a locked window.
STDMETHODIMP CDragImages::Show(BOOL bShow)
{
    BOOL fOld = bShow;
    TraceMsg(TF_DRAGIMAGES, "CDragImages::Show(%s)", bShow? TEXT("true") : TEXT("false"));

    if (!IsValid() || !_Single.bDragging)
    {
        return S_FALSE;
    }

    // No point in showing and hiding a Window. This causes unnecessary flicker.
    if (_hwnd)
    {
        return S_OK;
    }


    // If we're going across thread boundaries we have to try a context switch
    if (GetCurrentThreadId() != GetWindowThreadProcessId(_Single.hwndLock, NULL) &&
        ShowDragImageInterThread(_Single.hwndLock, &fOld))
        return fOld;

    fOld = _Single.bLocked;

    //
    // If we are going to show the drag image, lock the target window.
    //
    if (bShow && !_Single.bLocked)
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::Show : Shown and not locked");
        UpdateWindow(_Single.hwndLock);
        LockWindowUpdate(_Single.hwndLock);
        _Single.bLocked = TRUE;
    }

    if (_Single.bSingle)
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::Show : Calling ImageList_DragShowNoLock");
        ImageList_DragShowNolock(bShow);
    }
    else
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::Show : MultiDragShow");
        _MultipleDragShow(bShow);
    }

    //
    // If we have just hide the drag image, unlock the target window.
    //
    if (!bShow && _Single.bLocked)
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::Show : hiding image, unlocking");
        LockWindowUpdate(NULL);
        _Single.bLocked = FALSE;
    }

    return fOld? S_OK : S_FALSE;
}

// tell the drag source to hide or unhide the drag image to allow
// the destination to do drawing (unlock the screen)
//
// in:
//      bShow   FALSE   - hide the drag image, allow drawing
//              TRUE    - show the drag image, no drawing allowed after this

// Helper function for DAD_ShowDragImage - handles the inter-thread case.
// We need to handle this case differently because LockWindowUpdate calls fail
// if they are on the wrong thread.
extern "C" TCHAR c_szDefViewClass[];
BOOL CDragImages::ShowDragImageInterThread(HWND hwndLock, BOOL * pfShow)
{
    TCHAR szClassName[50];

    if (GetClassName(hwndLock, szClassName, SIZEOF(szClassName))) 
    {
        UINT uMsg = 0;
        ULONG_PTR dw = 0;

        if (lstrcmpi(szClassName, c_szDefViewClass) == 0)
            uMsg = WM_DSV_SHOWDRAGIMAGE;
        if (lstrcmpi(szClassName, TEXT("CabinetWClass")) == 0)
            uMsg = CWM_SHOWDRAGIMAGE;

        if (uMsg) 
        {
            SendMessageTimeout(hwndLock, uMsg, 0, *pfShow, SMTO_ABORTIFHUNG, 1000, &dw);
            *pfShow = (dw != 0);
            return TRUE;
        }
    }

    return FALSE;
}


void CDragImages::ThreadDetach()
{
    if (_idThread == GetCurrentThreadId())
        SetDragImage(NULL, 0, NULL);
}

void CDragImages::ProcessDetach()
{
    SetDragImage(NULL, 0, NULL);
    _DestroyCachedCursors();
}

BOOL CDragImages::SetDragImage(HIMAGELIST himl, int index, POINT * pptOffset)
{
    if (himl)
    {
        // We are setting
        if (IsValid())
            return FALSE;

        _fImage = TRUE;
        if (pptOffset) 
        {
            // Avoid the flicker by always pass even coords
            _ptOffset.x = (pptOffset->x & ~1);
            _ptOffset.y = (pptOffset->y & ~1);
        }

        ImageList_BeginDrag(himl, index, _ptOffset.x, _ptOffset.y);
        Validate();
    }
    else
    {
        Invalidate();
    }
    return TRUE;
}

//=====================================================================
// Multile Drag show
//=====================================================================

void CDragImages::_MultipleDragShow(BOOL bShow)
{
    HDC hDC;
    int nRect;
    RECT rc, rcClip;

    if ((bShow && _Single._Multi.bShown) || (!bShow && !_Single._Multi.bShown))
        return;

    _Single._Multi.bShown = bShow;

    // clip to window, NOT SM_CXSCREEN/SM_CYSCREEN (multiple monitors)
    GetWindowRect(_Single.hwndLock, &rcClip);
    rcClip.right -= rcClip.left;
    rcClip.bottom -= rcClip.top;

    hDC = GetDCEx(_Single.hwndLock, NULL, DCX_WINDOW | DCX_CACHE |
        DCX_LOCKWINDOWUPDATE | DCX_CLIPSIBLINGS);


    for (nRect = _Single._Multi.nRects - 1; nRect >= 0; --nRect)
    {
        rc = _Single._Multi.pRect[nRect];
        OffsetRect(&rc, _Single._Multi.ptNow.x - _Single._Multi.ptOffset.x,
            _Single._Multi.ptNow.y - _Single._Multi.ptOffset.y);

        if ((rc.top < rcClip.bottom) && (rc.bottom > 0) &&
            (rc.left < rcClip.right) && (rc.right > 0))
        {
            DrawFocusRect(hDC, &rc);
        }
    }
    ReleaseDC(_Single.hwndLock, hDC);
}


void CDragImages::_MultipleDragStart(HWND hwndLock, LPRECT aRect, int nRects, POINT ptStart, POINT ptOffset)
{
    _Single._Multi.bShown = FALSE;
    _Single._Multi.pRect = aRect;
    _Single._Multi.nRects = nRects;
    _Single._Multi.ptOffset = ptOffset;
    _Single._Multi.ptNow = ptStart;
}


void CDragImages::_MultipleDragMove(POINT ptNew)
{
    if ((_Single._Multi.ptNow.x == ptNew.x) &&
        (_Single._Multi.ptNow.y == ptNew.y))
    {
        // nothing has changed.  bail
        return;
    }

    if (_Single._Multi.bShown)
    {
        HDC hDC;
        int nRect;
        RECT rc, rcClip;
        int dx1 = _Single._Multi.ptNow.x - _Single._Multi.ptOffset.x;
        int dy1 = _Single._Multi.ptNow.y - _Single._Multi.ptOffset.y;
        int dx2 = ptNew.x - _Single._Multi.ptNow.x;
        int dy2 = ptNew.y - _Single._Multi.ptNow.y;

        // clip to window, NOT SM_CXSCREEN/SM_CYSCREEN (multiple monitors)
        GetWindowRect(_Single.hwndLock, &rcClip);
        rcClip.right -= rcClip.left;
        rcClip.bottom -= rcClip.top;



        hDC = GetDCEx(_Single.hwndLock, NULL, DCX_WINDOW | DCX_CACHE |
            DCX_LOCKWINDOWUPDATE | DCX_CLIPSIBLINGS);

        for (nRect = _Single._Multi.nRects - 1; nRect >= 0; --nRect)
        {
            rc = _Single._Multi.pRect[nRect];
            // hide pass
            OffsetRect(&rc, dx1, dy1);
            if ((rc.top < rcClip.bottom) && (rc.bottom > 0) &&
                (rc.left < rcClip.right) && (rc.right > 0))
            {

                DrawFocusRect(hDC, &rc);
            }
            // show pass
            OffsetRect(&rc, dx2, dy2);
            if ((rc.top < rcClip.bottom) && (rc.bottom > 0) &&
                (rc.left < rcClip.right) && (rc.right > 0))
            {
                DrawFocusRect(hDC, &rc);
            }
        }
        ReleaseDC(_Single.hwndLock, hDC);
    }

    _Single._Multi.ptNow = ptNew;
}

BOOL CDragImages::_SetMultiRectDragging(int cItems, LPRECT prect, LPPOINT pptOffset)
{
    BOOL fRet = FALSE;

    if (IsValid())
        return FALSE;

    // Multiple item drag
    int i;
    _cItems = cItems;
    _parc = new RECT[2 * _cItems];
    if (_parc)
    {
        for (i = 0;  i < cItems; i++)
            _parc[i] = prect[i];

        // Avoid the flicker by always pass even coords
        _ptOffset.x = (pptOffset->x & ~1);
        _ptOffset.y = (pptOffset->y & ~1);
        Validate();
        fRet = TRUE;
    }
    return fRet;

}
#define ListView_IsIconView(hwndLV)    ((GetWindowLong(hwndLV, GWL_STYLE) & (UINT)LVS_TYPEMASK) == (UINT)LVS_ICON)

BOOL CDragImages::_SetMultiItemDragging(HWND hwndLV, int cItems, LPPOINT pptOffset)
{
    BOOL fRet = FALSE;

    if (IsValid())
        return FALSE;

    // Multiple item drag
    _parc = new RECT[2 * cItems];
    if (_parc)
    {
        POINT ptTemp;
        int iLast, iNext;
        int cxScreens, cyScreens;
        LPRECT prcNext;
        RECT rc;

        _cItems = 0;
        ASSERT(_fImage == FALSE);

        //
        // If this is a mirrored Window, then lead edge is going
        // to be the far end in screen coord. So let's compute
        // as the original code, and later in _MultipleDragMove
        // we will compensate.
        //
        //
        
        GetWindowRect( hwndLV , &rc );
        ptTemp.x = rc.left;
        ptTemp.y = rc.top;

        //
        // Reflect the shift the if the window is RTL mirrored.
        //
        if (IS_WINDOW_RTL_MIRRORED(hwndLV))
        {
            ptTemp.x = -ptTemp.x;
            pptOffset->x = ((rc.right-rc.left)-pptOffset->x);
        }

        cxScreens = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        cyScreens = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        // for pre-Nashville platforms
        if (!cxScreens || !cyScreens)
        {
            cxScreens = GetSystemMetrics(SM_CXSCREEN);
            cyScreens = GetSystemMetrics(SM_CYSCREEN);
        }

        for (iNext = cItems-1, iLast = -1, prcNext = _parc; iNext >= 0; --iNext)
        {
            iLast = ListView_GetNextItem(hwndLV, iLast, LVNI_SELECTED);
            if (iLast != -1) 
            {

                ListView_GetItemRect(hwndLV, iLast, &prcNext[0], LVIR_ICON);
                OffsetRect(&prcNext[0], ptTemp.x, ptTemp.y);

                if (((prcNext[0].left - pptOffset->x) < cxScreens) &&
                    ((pptOffset->x - prcNext[0].right) < cxScreens) &&
                    ((prcNext[0].top - pptOffset->y) < cyScreens)) 
                {

                    ListView_GetItemRect(hwndLV, iLast, &prcNext[1], LVIR_LABEL);
                    OffsetRect(&prcNext[1], ptTemp.x, ptTemp.y);
                    if ((pptOffset->y - prcNext[1].bottom) < cxScreens) 
                    {

                        //
                        // Fix 24857: Ask JoeB why we are drawing a bar instead of
                        //  a text rectangle.
                        //
                        prcNext[1].top = (prcNext[1].top + prcNext[1].bottom)/2;
                        prcNext[1].bottom = prcNext[1].top + 2;
                        prcNext += 2;
                        _cItems += 2;
                    }
                }
            }
        }

        // Avoid the flicker by always pass even coords
        _ptOffset.x = (pptOffset->x & ~1);
        _ptOffset.y = (pptOffset->y & ~1);
        _hwndFrom = hwndLV;
        Validate();
        fRet = TRUE;
    }
    return fRet;
}


//=====================================================================
// Cursor Merging
//=====================================================================
void CDragImages::_DestroyCachedCursors()
{
    int i;

    if (_himlCursors) 
    {
        ImageList_Destroy(_himlCursors);
        _himlCursors = NULL;
    }

    for (i=0 ; i < DCID_MAX ; i++) 
    {
        if (_ahcur[i])
        {
            DestroyCursor(_ahcur[i]);
            _ahcur[i] = NULL;
        }
    }
}

HBITMAP CDragImages::CreateColorBitmap(int cx, int cy)
{
    HDC hdc;
    HBITMAP hbm;

    hdc = GetDC(NULL);
    hbm = CreateCompatibleBitmap(hdc, cx, cy);
    ReleaseDC(NULL, hdc);

    return hbm;
}

#define CreateMonoBitmap( cx,  cy) CreateBitmap(cx, cy, 1, 1, NULL)
typedef WORD CURMASK;
#define _BitSizeOf(x) (SIZEOF(x)*8)

void CDragImages::_GetCursorLowerRight(HCURSOR hcursor, int * px, int * py, POINT *pptHotSpot)
{
    ICONINFO iconinfo;
    CURMASK CurMask[16*8];
    BITMAP bm;
    int i;
    int xFine = 16;

    GetIconInfo(hcursor, &iconinfo);
    GetObject(iconinfo.hbmMask, SIZEOF(bm), (LPTSTR)&bm);
    GetBitmapBits(iconinfo.hbmMask, SIZEOF(CurMask), CurMask);
    pptHotSpot->x = iconinfo.xHotspot;
    pptHotSpot->y = iconinfo.yHotspot;
    if (iconinfo.hbmColor) 
    {
        i = (int)(bm.bmWidth * bm.bmHeight / _BitSizeOf(CURMASK) - 1);
    } 
    else 
    {
        i = (int)(bm.bmWidth * (bm.bmHeight/2) / _BitSizeOf(CURMASK) - 1);
    }

    if ( i >= SIZEOF(CurMask)) 
    {
        i = SIZEOF(CurMask) -1;
    }

    // BUGBUG: this assumes that the first pixel encountered on this bottom
    // up/right to left search will be reasonably close to the rightmost pixel
    // which for all of our cursors is correct, but it not necessarly correct.

    // also, it assumes the cursor has a good mask... not like the IBeam XOR only
    // cursor
    for (; i >= 0; i--)   
    {
        if (CurMask[i] != 0xFFFF) 
        {
            // this is only accurate to 16 pixels... which is a big gap..
            // so let's try to be a bit more accurate.
            int j;
            DWORD dwMask;

            for (j = 0; j < 16; j++, xFine--) 
            {
                if (j < 8) 
                {
                    dwMask = (1 << (8 + j));
                } 
                else 
                {
                    dwMask = (1 << (j - 8));
                }

                if (!(CurMask[i] & dwMask))
                    break;
            }
            ASSERT(j < 16);
            break;
        }
    }

    if (iconinfo.hbmColor) 
    {
        DeleteObject(iconinfo.hbmColor);
    }

    if (iconinfo.hbmMask) 
    {
        DeleteObject(iconinfo.hbmMask);
    }

    // Compute the pointer height
    // use width in both directions because the cursor is square, but the
    // height might be doubleheight if it's mono
    *py = ((i + 1) * _BitSizeOf(CURMASK)) / (int)bm.bmWidth;
    *px = ((i * _BitSizeOf(CURMASK)) % (int)bm.bmWidth) + xFine + 2; // hang it off a little
}

// this will draw iiMerge's image over iiMain on main's lower right.
BOOL CDragImages::_MergeIcons(HCURSOR hcursor, LPCTSTR idMerge, HBITMAP *phbmImage, HBITMAP *phbmMask, POINT* pptHotSpot)
{
    BITMAP bm;
    int xBitmap;
    int yBitmap;
    int xDraw;
    int yDraw;
    HDC hdcCursor, hdcBitmap;
    HBITMAP hbmTemp;
    HBITMAP hbmImage;
    HBITMAP hbmMask;
    int xCursor = GetSystemMetrics(SM_CXCURSOR);
    int yCursor = GetSystemMetrics(SM_CYCURSOR);
    HBITMAP hbmp;

    // find the lower corner of the cursor and put it there.
    // do this whether or not we have an idMerge because it will set the hotspot
    _GetCursorLowerRight(hcursor, &xDraw, &yDraw, pptHotSpot);
    if (idMerge != (LPCTSTR)-1) 
    {
        hbmp = (HBITMAP)LoadImage(HINST_THISDLL, idMerge, IMAGE_BITMAP, 0, 0, 0);
        if (hbmp) 
        {
            GetObject(hbmp, SIZEOF(bm), &bm);
            xBitmap = bm.bmWidth;
            yBitmap = bm.bmHeight/2;

            if (xDraw + xBitmap > xCursor)
                xDraw = xCursor - xBitmap;
            if (yDraw + yBitmap > yCursor)
                yDraw = yCursor - yBitmap;
        }
    } 
    else
        hbmp = NULL;


    hdcCursor = CreateCompatibleDC(NULL);

    hbmMask = CreateMonoBitmap(xCursor, yCursor);
    hbmImage = CreateColorBitmap(xCursor, yCursor);

    if (hdcCursor && hbmMask && hbmImage) 
    {

        hbmTemp = (HBITMAP)SelectObject(hdcCursor, hbmImage);
        DrawIconEx(hdcCursor, 0, 0, hcursor, 0, 0, 0, NULL, DI_NORMAL);

        if (hbmp) 
        {
            hdcBitmap = CreateCompatibleDC(NULL);
            SelectObject(hdcBitmap, hbmp);

            //blt the two bitmaps onto the color and mask bitmaps for the cursor
            BitBlt(hdcCursor, xDraw, yDraw, xBitmap, yBitmap, hdcBitmap, 0, 0, SRCCOPY);
        }

        SelectObject(hdcCursor, hbmMask);

        DrawIconEx(hdcCursor, 0, 0, hcursor, 0, 0, 0, NULL, DI_MASK);

        if (hbmp) 
        {
            BitBlt(hdcCursor, xDraw, yDraw, xBitmap, yBitmap, hdcBitmap, 0, yBitmap, SRCCOPY);

            // select back in the old bitmaps
            SelectObject(hdcBitmap, hbmTemp);
            DeleteDC(hdcBitmap);
            DeleteObject(hbmp);
        }

        // select back in the old bitmaps
        SelectObject(hdcCursor, hbmTemp);
    }

    if (hdcCursor)
        DeleteDC(hdcCursor);

    *phbmImage = hbmImage;
    *phbmMask = hbmMask;
    return (hbmImage && hbmMask);
}

// this will take a cursor index and load
int CDragImages::_AddCursorToImageList(HCURSOR hcur, LPCTSTR idMerge, POINT *pptHotSpot)
{
    int iIndex;
    HBITMAP hbmImage, hbmMask;

    // merge in the plus or link arrow if it's specified
    if (_MergeIcons(hcur, idMerge, &hbmImage, &hbmMask, pptHotSpot)) 
    {
        iIndex = ImageList_Add(_himlCursors, hbmImage, hbmMask);
    } 
    else 
    {
        iIndex = -1;
    }

    if (hbmImage)
        DeleteObject(hbmImage);

    if (hbmMask)
        DeleteObject(hbmMask);

    return iIndex;
}

int _MapEffectToId(DWORD dwEffect)
{
    int idCursor;

    // DebugMsg(DM_TRACE, "sh TR - DAD_GiveFeedBack dwEffect=%x", dwEffect);

    switch (dwEffect & (DROPEFFECT_COPY|DROPEFFECT_LINK|DROPEFFECT_MOVE))
    {
    case 0:
        idCursor = DCID_NO;
        break;

    case DROPEFFECT_COPY:
        idCursor = DCID_COPY;
        break;

    case DROPEFFECT_LINK:
        idCursor = DCID_LINK;
        break;

    case DROPEFFECT_MOVE:
        idCursor = DCID_MOVE;
        break;

    default:
        // if it's a right drag, we can have any effect... we'll
        // default to the arrow without merging in anything
//
// REVIEW: Our Defview's DragEnter code is lazy and does not pick the default.
// We'll fix it only if it causes some problem with OLE-apps.
//
#if 0
        if (GetKeyState(VK_LBUTTON) < 0)
        {
            // if the left button is down we should always have
            // one of the above
            ASSERT(0);
        }
#endif
        idCursor = DCID_MOVE;
        break;
    }

    return idCursor;
}

int CDragImages::_MapCursorIDToImageListIndex(int idCur)
{
    const static struct 
    {
        BOOL   fSystem;
        LPCTSTR idRes;
        LPCTSTR idMerge;
    } c_acurmap[DCID_MAX] = 
    {
        { FALSE, MAKEINTRESOURCE(IDC_NULL), (LPCTSTR)-1},
        { TRUE, IDC_NO, (LPCTSTR)-1 },
        { TRUE, IDC_ARROW, (LPCTSTR)-1 },
        { TRUE, IDC_ARROW, MAKEINTRESOURCE(IDB_PLUS_MERGE) },
        { TRUE, IDC_ARROW, MAKEINTRESOURCE(IDB_LINK_MERGE) },
    };

    ASSERT(idCur >= DCID_INVALID && idCur < DCID_MAX);

    //
    // idCur==DCID_INVALID means "Initialize the image list index array".
    //
    if (idCur == DCID_INVALID)
    {
        int i;
        for (i=0 ; i<DCID_MAX ; i++) 
        {
            _aindex[i] = -1;
        }
        return -1;
    }

    if (_aindex[idCur] == -1)
    {
        HINSTANCE hinst = c_acurmap[idCur].fSystem ? NULL : HINST_THISDLL;
        HCURSOR   hcur = LoadCursor(hinst, c_acurmap[idCur].idRes);

        _aindex[idCur] = _AddCursorToImageList(hcur, c_acurmap[idCur].idMerge,
                                                         &_aptHotSpot[idCur]);
    }

    return _aindex[idCur];
}

HCURSOR CDragImages::SetCursorHotspot(HCURSOR hcur, POINT *ptHot)
{
    ICONINFO iconinfo;
    HCURSOR hcurHotspot;

    GetIconInfo(hcur, &iconinfo);
    iconinfo.xHotspot = ptHot->x;
    iconinfo.yHotspot = ptHot->y;
    iconinfo.fIcon = FALSE;
    hcurHotspot = (HCURSOR)CreateIconIndirect(&iconinfo);
    if (iconinfo.hbmColor) 
    {
        DeleteObject(iconinfo.hbmColor);
    }

    if (iconinfo.hbmMask) 
    {
        DeleteObject(iconinfo.hbmMask);
    }
    return hcurHotspot;
}

void CDragImages::_SetDropEffectCursor(int idCur)
{
    if (_himlCursors && (idCur != DCID_INVALID))
    {
        if (!_ahcur[idCur])
        {
            int iIndex = _MapCursorIDToImageListIndex(idCur);
            if (iIndex != -1)
            {
                HCURSOR hcurColor = ImageList_GetIcon(_himlCursors, iIndex, 0);
                //
                // On non C1_COLORCURSOR displays, CopyImage() will enforce
                // monochrome.  So on color cursor displays, we'll get colored
                // dragdrop pix.
                //
                HCURSOR hcurScreen = (HCURSOR)CopyImage(hcurColor, IMAGE_CURSOR,
                    0, 0, LR_COPYRETURNORG | LR_DEFAULTSIZE);

                HCURSOR hcurFinal = SetCursorHotspot(hcurScreen, &_aptHotSpot[idCur]);

                if (hcurScreen != hcurColor) 
                {
                    DestroyCursor(hcurColor);
                }

                if (hcurFinal)
                    DestroyCursor(hcurScreen);
                else
                    hcurFinal = hcurScreen;

                _ahcur[idCur] = hcurFinal;
            }
        }

        if (_ahcur[idCur]) 
        {
            //
            // This code assumes that SetCursor is pretty quick if it is
            // already set.
            //
            SetCursor(_ahcur[idCur]);
        }
    }
}


//=====================================================================
// CDropSource
//=====================================================================

class CDropSource : public IDropSource
{
private:
    LONG            _cRef;
    DWORD           _grfInitialKeyState;
    IDataObject*    _pdtobj;

public:
    explicit CDropSource(IDataObject *pdtobj);
    virtual ~CDropSource();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDropSource methods
    STDMETHODIMP GiveFeedback(DWORD dwEffect);
    STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
};


void DAD_ShowCursor(BOOL fShow)
{
    static BOOL s_fCursorHidden = FALSE;

    if (fShow) 
    {
        if (s_fCursorHidden)
        {
            ShowCursor(TRUE);
            s_fCursorHidden = FALSE;
        }
    } 
    else 
    {
        if (!s_fCursorHidden)
        {
            ShowCursor(FALSE);
            s_fCursorHidden = TRUE;
        }
    }
}



CDropSource::CDropSource(IDataObject *pdtobj) 
    : _cRef(1),
      _pdtobj(pdtobj),
      _grfInitialKeyState(0)
{
    if (NULL != _pdtobj)
    {
        _pdtobj->AddRef();

       //
       // Tell the data object that we're entering the drag loop.
       //
       DataObj_SetDWORD (_pdtobj, g_cfInDragLoop, 1);
    }
}

CDropSource::~CDropSource()
{
    DAD_ShowCursor(TRUE); // just in case
    ATOMICRELEASE(_pdtobj);
}

//
// Create an instance of CDropSource
//
STDMETHODIMP CDropSource_CreateInstance(IDropSource **ppdsrc, IDataObject *pdtobj)
{
    CDropSource *pDropSource = new CDropSource(pdtobj);
    if (pDropSource)
    {
        if (pdtobj)
        {
           //Set the Drage context as  part of the data object
           if (g_pdiDragImages)
               g_pdiDragImages->DehydrateToDataObject(pdtobj);
        }

        *ppdsrc = pDropSource;
        return NOERROR;
    }
    else
    {
        *ppdsrc = NULL;
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CDropSource::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDropSource, IDropSource),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDropSource::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDropSource::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    HRESULT hres = S_OK;

    if (fEscapePressed)
    {
        hres = DRAGDROP_S_CANCEL;
    }
    else
    {
        // initialize ourself with the drag begin button
        if (_grfInitialKeyState == 0)
            _grfInitialKeyState = (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));

        // If the window is hung for a while, the drag operation can happen before
        // the first call to this function, so grfInitialKeyState will be 0. If this
        // happened, then we did a drop. No need to assert...
        //ASSERT(this->grfInitialKeyState);

        if (!(grfKeyState & _grfInitialKeyState))
        {
            //
            // A button is released.
            //
            hres = DRAGDROP_S_DROP;
        }
        else if (_grfInitialKeyState != (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
        {
            //
            //  If the button state is changed (except the drop case, which we handle
            // above, cancel the drag&drop.
            //
            hres = DRAGDROP_S_CANCEL;
        }
    }

    if (hres != S_OK)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        DAD_ShowCursor(TRUE);
        DAD_SetDragCursor(DCID_NULL);

        //
        // Tell the data object that we're leaving the drag loop.
        //
        if (_pdtobj)
        {
           DataObj_SetDWORD(_pdtobj, g_cfInDragLoop, 0);
        }
    }

    return hres;
}

STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect)
{
    int idCursor = _MapEffectToId(dwEffect);

    //
    // Notes:
    //
    //  OLE does not give us DROPEFFECT_MOVE even though our IDT::DragOver
    // returns it, if we haven't set that bit when we have called DoDragDrop.
    // Instead of arguing whether or not this is a bug or by-design of OLE,
    // we work around it. It is important to note that this hack around
    // g_fDraggingOverSource is purely visual hack. It won't affect the
    // actual drag&drop operations at all (DV_AlterEffect does it all).
    //
    // - SatoNa
    //
    if (idCursor == DCID_NO && g_fDraggingOverSource)
    {
        idCursor = DCID_MOVE;
    }

    
    //
    //  No need to merge the cursor, if we are not dragging over to
    // one of shell windows.
    //
    if (DAD_IsDraggingImage())
    {
        // Feedback for single (image) dragging
        DAD_ShowCursor(FALSE);
        DAD_SetDragCursor(idCursor);
    }
    else if (DAD_IsDragging() && g_pdiDragImages)
    {
        // Feedback for multiple (rectangles) dragging
        g_pdiDragImages->_SetDropEffectCursor(idCursor);
        DAD_ShowCursor(TRUE);
        return NOERROR;
    }
    else
    {
        DAD_ShowCursor(TRUE);
    }

    return DRAGDROP_S_USEDEFAULTCURSORS;
}

//=====================================================================
// DAD
//=====================================================================

void FixupDragPoint(HWND hwnd, POINT* ppt)
{
    if (hwnd)
    {
        RECT rc = {0};
        GetWindowRect(hwnd, &rc);
        ppt->x += rc.left;
        ppt->y += rc.top;
    }
}

BOOL DAD_InitDragImages()
{
    if (!g_pdiDragImages)
        CDragImages_CreateInstance(NULL, IID_IDragSourceHelper, NULL);

    return g_pdiDragImages != NULL;
}


STDAPI_(BOOL) DAD_ShowDragImage(BOOL bShow)
{

    if (DAD_InitDragImages())
        return g_pdiDragImages->Show(bShow) == S_OK? TRUE : FALSE;
    return FALSE;
}

STDAPI_(BOOL) DAD_IsDragging()
{
    if (DAD_InitDragImages())
        return g_pdiDragImages->IsDragging();
    return FALSE;
}

void DAD_SetDragCursor(int idCursor)
{
    if (DAD_InitDragImages())
        g_pdiDragImages->SetDragCursor(idCursor);
}

STDAPI_(BOOL) DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject)
{
    if (DAD_InitDragImages())
    {
        POINT pt = ptStart;
        FixupDragPoint(hwndTarget, &pt);
        return g_pdiDragImages->DragEnter(hwndTarget, pdtObject, (POINT*)&pt, NULL);
    }
    return FALSE;
}

STDAPI_(BOOL) DAD_DragEnterEx(HWND hwndTarget, const POINT ptStart)
{
    if (DAD_InitDragImages())
    {
        POINT pt = ptStart;
        FixupDragPoint(hwndTarget, &pt);
        return g_pdiDragImages->DragEnter(hwndTarget, NULL, (POINT*)&pt, NULL);
    }
    return FALSE;
}

STDAPI_(BOOL) DAD_DragEnter(HWND hwndTarget)
{
    POINT ptStart;

    GetCursorPos(&ptStart);
    if (hwndTarget) 
    {
        ScreenToClient(hwndTarget, &ptStart);
    }

    return DAD_DragEnterEx(hwndTarget, ptStart);
}

STDAPI_(BOOL) DAD_DragMove(POINT pt)
{
    if (DAD_InitDragImages())
    {
        FixupDragPoint(g_pdiDragImages->_hwndTarget, &pt);
        return g_pdiDragImages->DragOver(&pt, 0);
    }
    return FALSE;
}

STDAPI_(BOOL) DAD_SetDragImage(HIMAGELIST him, POINT * pptOffset)
{
    if (DAD_InitDragImages() && !g_pdiDragImages->IsDraggingLayeredWindow())
    {
        //
        // DAD_SetDragImage(-1, NULL) means "clear the drag image only
        //  if the image is set by this thread"
        //
        if (him == (HIMAGELIST)-1)
        {
            BOOL fThisThreadHasImage = FALSE;
            ENTERCRITICAL;
            if (g_pdiDragImages->IsValid() && g_pdiDragImages->GetThread() == GetCurrentThreadId())
            {
                fThisThreadHasImage = TRUE;
            }
            LEAVECRITICAL;

            if (fThisThreadHasImage)
            {
                return g_pdiDragImages->SetDragImage(NULL, 0, NULL);
            }
            return FALSE;
        }

        return g_pdiDragImages->SetDragImage(him, 0, pptOffset);
    }

    return TRUE;
}

//
//  This function returns TRUE, if we are dragging an image. It means
// you have called either DAD_SetDragImage (with him != NULL) or
// DAD_SetDragImageFromListview.
//
BOOL DAD_IsDraggingImage(void)
{
    if (DAD_InitDragImages())
        return g_pdiDragImages->IsDraggingImage();
    return FALSE;
}


STDAPI_(BOOL) DAD_DragLeave()
{
    if (DAD_InitDragImages())
        return g_pdiDragImages->DragLeave();
    return FALSE;
}

STDAPI_(void) DAD_ProcessDetach(void)
{
    if (g_pdiDragImages)
    {
        g_pdiDragImages->ProcessDetach();
        delete g_pdiDragImages;
    }
}

STDAPI_(void) DAD_ThreadDetach(void)
{
    if (g_pdiDragImages)
        g_pdiDragImages->ThreadDetach();
}

//
//  We don't want to destroy the cached cursors now. We simply increment
// g_cRef (global) to make it different from s_cursors._cRef.
//
void DAD_InvalidateCursors(void)
{
    g_cRev++;
}


STDAPI_(BOOL) DAD_SetDragImageFromWindow(HWND hwnd, POINT* ppt, IDataObject* pDataObject)
{
    if (DAD_InitDragImages())
        return S_OK == g_pdiDragImages->InitializeFromWindow(hwnd, ppt, pDataObject);
    return FALSE;
}

//
//  This function allocate a shared memory block which contains either
// a set of images (currently always one) or a set of rectangles.
//
// Notes: NEVER think about making this function public!
//
STDAPI_(BOOL) DAD_SetDragImageFromListView(HWND hwndLV, POINT ptOffset)
{
    if (DAD_InitDragImages())
        return S_OK == g_pdiDragImages->InitializeFromWindow(hwndLV, 0, NULL);

    return FALSE;
}


//=====================================================================
// Other exports
//=====================================================================
STDAPI SHDoDragDrop(HWND hwnd, IDataObject *pdata, IDropSource *pdsrc, DWORD dwEffect, DWORD *pdwEffect)
{
    HRESULT hres;
    IDropSource *pdsrcRelease = NULL;

    if (pdsrc == NULL)
    {
        CDropSource_CreateInstance(&pdsrcRelease, pdata);
        pdsrc = pdsrcRelease;
    }
    else if (DAD_InitDragImages())
       g_pdiDragImages->DehydrateToDataObject(pdata);    // CDropSource_CreateInstance above didn't do it so do it here.

    hres = DoDragDrop(pdata, pdsrc, dwEffect, pdwEffect);

    if (pdsrcRelease)
        pdsrcRelease->Release();

    return hres;
}


// move to commctrl\cutils.c

/*
 *  QueryDropObject() -
 *
 *  Determines where in the window heirarchy the "drop" takes place, and
 *  sends a message to the deepest child window first.  If that window does
 *  not respond, we go up the heirarchy (recursively, for the moment) until
 *  we either get a window that does respond or the parent doesn't respond.
 *
 *  in:
 *
 *  out:
 *      lpds->ptDrop    set to the point of the query (window coordinates)
 *      lpds->hwndSink  the window that answered the query
 *
 *  returns:
 *      value from WM_QUERYDROPOBJECT (0, 1, or hCursor)
 */

HCURSOR QueryDropObject(HWND hwnd, LPDROPSTRUCT lpds)
{
    HWND hwndT;
    HCURSOR hCurT = 0;
    POINT pt;
    BOOL fNC;
    RECT rc;

    pt = lpds->ptDrop;          /* pt is in screen coordinates */

    GetWindowRect(hwnd, &rc);

    /* reject points outside this window or if the window is disabled */
    if (!PtInRect(&rc, pt) || !IsWindowEnabled(hwnd))
        return NULL;

    /* are we dropping in the nonclient area of the window or on an iconic
     * window? */
    GetClientRect(hwnd, &rc);
    MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);
    if (IsMinimized(hwnd) || !PtInRect(&rc, pt)) {
        fNC = TRUE;
        ScreenToClient(hwnd, &lpds->ptDrop);
        goto SendQueryDrop;
    }

    fNC = FALSE;                /* dropping in client area */

    for (hwndT = GetWindow(hwnd, GW_CHILD); hwndT && !hCurT; hwndT = GetWindow(hwndT, GW_HWNDNEXT)) {

        if (!IsWindowVisible(hwndT))    /* Ignore invisible windows */
            continue;

        GetWindowRect(hwndT, &rc);
        if (!PtInRect(&rc, pt))         /* not in window? skip it*/
            continue;

        if (!IsWindowEnabled(hwndT))
            /* If point is in a disabled, visible window, get the heck out. No
             * need to check further since no drops allowed here. */
            break;

        /* recursively search child windows for the drop place */
        hCurT = QueryDropObject(hwndT, lpds);

        /* don't look at windows below this one in the zorder
         */
        break;
    }

    if (!hCurT) {
        /* there are no children who are in the right place or who want
         * drops... convert the point into client coordinates of the
         * current window.  Because of the recursion, this is already
         * done if a child window grabbed the drop. */
        ScreenToClient(hwnd, &lpds->ptDrop);

SendQueryDrop:
        lpds->hwndSink = hwnd;
        hCurT = (HCURSOR)SendMessage(hwnd, WM_QUERYDROPOBJECT, fNC, (LPARAM)lpds);

        /* restore drop point to screen coordinates if this window won't take
         * drops */
        if (!hCurT)
            lpds->ptDrop = pt;
    }

    return hCurT;
}
