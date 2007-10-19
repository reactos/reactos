/*
 * OLE Picture object
 *
 * Implementation of OLE IPicture and related interfaces
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers.
 * Copyright 2001 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * BUGS
 *
 * Support PICTYPE_BITMAP and PICTYPE_ICON, altough only bitmaps very well..
 * Lots of methods are just stubs.
 *
 *
 * NOTES (or things that msdn doesn't tell you)
 *
 * The width and height properties are returned in HIMETRIC units (0.01mm)
 * IPicture::Render also uses these to select a region of the src picture.
 * A bitmap's size is converted into these units by using the screen resolution
 * thus an 8x8 bitmap on a 96dpi screen has a size of 212x212 (8/96 * 2540).
 *
 */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Must be before wine includes, the header has things conflicting with
 * WINE headers.
 */
#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"
#include "connpt.h"
#include "urlmon.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "wine/wingdi16.h"

#ifdef SONAME_LIBJPEG
/* This is a hack, so jpeglib.h does not redefine INT32 and the like*/
#define XMD_H
#define UINT8 JPEG_UINT8
#define UINT16 JPEG_UINT16
#undef FAR
#define boolean jpeg_boolean
# include <jpeglib.h>
#undef jpeg_boolean
#undef UINT16
#endif

#ifdef HAVE_PNG_H
#undef FAR
#include <png.h>
#endif

#include "ungif.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#include "pshpack1.h"

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;

#include "poppack.h"

/*************************************************************************
 *  Declaration of implementation class
 */

typedef struct OLEPictureImpl {

  /*
   * IPicture handles IUnknown
   */

    const IPictureVtbl       *lpVtbl;
    const IDispatchVtbl      *lpvtblIDispatch;
    const IPersistStreamVtbl *lpvtblIPersistStream;
    const IConnectionPointContainerVtbl *lpvtblIConnectionPointContainer;

  /* Object reference count */
    LONG ref;

  /* We own the object and must destroy it ourselves */
    BOOL fOwn;

  /* Picture description */
    PICTDESC desc;

  /* These are the pixel size of a bitmap */
    DWORD origWidth;
    DWORD origHeight;

  /* And these are the size of the picture converted into HIMETRIC units */
    OLE_XSIZE_HIMETRIC himetricWidth;
    OLE_YSIZE_HIMETRIC himetricHeight;

    IConnectionPoint *pCP;

    BOOL keepOrigFormat;
    HDC	hDCCur;

  /* Bitmap transparency mask */
    HBITMAP hbmMask;
    HBITMAP hbmXor;
    COLORREF rgbTrans;

  /* data */
    void* data;
    int datalen;
    BOOL bIsDirty;                  /* Set to TRUE if picture has changed */
    unsigned int loadtime_magic;    /* If a length header was found, saves value */
    unsigned int loadtime_format;   /* for PICTYPE_BITMAP only, keeps track of image format (GIF/BMP/JPEG) */
} OLEPictureImpl;

/*
 * Macros to retrieve pointer to IUnknown (IPicture) from the other VTables.
 */

static inline OLEPictureImpl *impl_from_IDispatch( IDispatch *iface )
{
    return (OLEPictureImpl *)((char*)iface - FIELD_OFFSET(OLEPictureImpl, lpvtblIDispatch));
}

static inline OLEPictureImpl *impl_from_IPersistStream( IPersistStream *iface )
{
    return (OLEPictureImpl *)((char*)iface - FIELD_OFFSET(OLEPictureImpl, lpvtblIPersistStream));
}

static inline OLEPictureImpl *impl_from_IConnectionPointContainer( IConnectionPointContainer *iface )
{
    return (OLEPictureImpl *)((char*)iface - FIELD_OFFSET(OLEPictureImpl, lpvtblIConnectionPointContainer));
}

/*
 * Predeclare VTables.  They get initialized at the end.
 */
static const IPictureVtbl OLEPictureImpl_VTable;
static const IDispatchVtbl OLEPictureImpl_IDispatch_VTable;
static const IPersistStreamVtbl OLEPictureImpl_IPersistStream_VTable;
static const IConnectionPointContainerVtbl OLEPictureImpl_IConnectionPointContainer_VTable;

/***********************************************************************
 * Implementation of the OLEPictureImpl class.
 */

static void OLEPictureImpl_SetBitmap(OLEPictureImpl*This) {
  BITMAP bm;
  HDC hdcRef;

  TRACE("bitmap handle %p\n", This->desc.u.bmp.hbitmap);
  if(GetObjectA(This->desc.u.bmp.hbitmap, sizeof(bm), &bm) != sizeof(bm)) {
    ERR("GetObject fails\n");
    return;
  }
  This->origWidth = bm.bmWidth;
  This->origHeight = bm.bmHeight;
  /* The width and height are stored in HIMETRIC units (0.01 mm),
     so we take our pixel width divide by pixels per inch and
     multiply by 25.4 * 100 */
  /* Should we use GetBitmapDimension if available? */
  hdcRef = CreateCompatibleDC(0);
  This->himetricWidth =(bm.bmWidth *2540)/GetDeviceCaps(hdcRef, LOGPIXELSX);
  This->himetricHeight=(bm.bmHeight*2540)/GetDeviceCaps(hdcRef, LOGPIXELSY);
  DeleteDC(hdcRef);
}

static void OLEPictureImpl_SetIcon(OLEPictureImpl * This)
{
    ICONINFO infoIcon;

    TRACE("icon handle %p\n", This->desc.u.icon.hicon);
    if (GetIconInfo(This->desc.u.icon.hicon, &infoIcon)) {
        HDC hdcRef;
        BITMAP bm;

        TRACE("bitmap handle for icon is %p\n", infoIcon.hbmColor);
        if(GetObjectA(infoIcon.hbmColor ? infoIcon.hbmColor : infoIcon.hbmMask, sizeof(bm), &bm) != sizeof(bm)) {
            ERR("GetObject fails on icon bitmap\n");
            return;
        }

        This->origWidth = bm.bmWidth;
        This->origHeight = infoIcon.hbmColor ? bm.bmHeight : bm.bmHeight / 2;
        /* see comment on HIMETRIC on OLEPictureImpl_SetBitmap() */
        hdcRef = GetDC(0);
        This->himetricWidth = (This->origWidth *2540)/GetDeviceCaps(hdcRef, LOGPIXELSX);
        This->himetricHeight= (This->origHeight *2540)/GetDeviceCaps(hdcRef, LOGPIXELSY);
        ReleaseDC(0, hdcRef);

        DeleteObject(infoIcon.hbmMask);
        if (infoIcon.hbmColor) DeleteObject(infoIcon.hbmColor);
    } else {
        ERR("GetIconInfo() fails on icon %p\n", This->desc.u.icon.hicon);
    }
}

/************************************************************************
 * OLEPictureImpl_Construct
 *
 * This method will construct a new instance of the OLEPictureImpl
 * class.
 *
 * The caller of this method must release the object when it's
 * done with it.
 */
static OLEPictureImpl* OLEPictureImpl_Construct(LPPICTDESC pictDesc, BOOL fOwn)
{
  OLEPictureImpl* newObject = 0;

  if (pictDesc)
      TRACE("(%p) type = %d\n", pictDesc, pictDesc->picType);

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OLEPictureImpl));

  if (newObject==0)
    return newObject;

  /*
   * Initialize the virtual function table.
   */
  newObject->lpVtbl = &OLEPictureImpl_VTable;
  newObject->lpvtblIDispatch = &OLEPictureImpl_IDispatch_VTable;
  newObject->lpvtblIPersistStream = &OLEPictureImpl_IPersistStream_VTable;
  newObject->lpvtblIConnectionPointContainer = &OLEPictureImpl_IConnectionPointContainer_VTable;

  newObject->pCP = NULL;
  CreateConnectionPoint((IUnknown*)newObject,&IID_IPropertyNotifySink,&newObject->pCP);
  if (!newObject->pCP)
  {
    HeapFree(GetProcessHeap(), 0, newObject);
    return NULL;
  }

  /*
   * Start with one reference count. The caller of this function
   * must release the interface pointer when it is done.
   */
  newObject->ref	= 1;
  newObject->hDCCur	= 0;

  newObject->fOwn	= fOwn;

  /* dunno about original value */
  newObject->keepOrigFormat = TRUE;

  newObject->hbmMask = NULL;
  newObject->hbmXor = NULL;
  newObject->loadtime_magic = 0xdeadbeef;
  newObject->loadtime_format = 0;
  newObject->bIsDirty = FALSE;

  if (pictDesc) {
      memcpy(&newObject->desc, pictDesc, sizeof(PICTDESC));

      switch(pictDesc->picType) {
      case PICTYPE_BITMAP:
	OLEPictureImpl_SetBitmap(newObject);
	break;

      case PICTYPE_METAFILE:
	TRACE("metafile handle %p\n", pictDesc->u.wmf.hmeta);
	newObject->himetricWidth = pictDesc->u.wmf.xExt;
	newObject->himetricHeight = pictDesc->u.wmf.yExt;
	break;

      case PICTYPE_NONE:
	/* not sure what to do here */
	newObject->himetricWidth = newObject->himetricHeight = 0;
	break;

      case PICTYPE_ICON:
        OLEPictureImpl_SetIcon(newObject);
        break;
      case PICTYPE_ENHMETAFILE:
      default:
	FIXME("Unsupported type %d\n", pictDesc->picType);
	newObject->himetricWidth = newObject->himetricHeight = 0;
	break;
      }
  } else {
      newObject->desc.picType = PICTYPE_UNINITIALIZED;
  }

  TRACE("returning %p\n", newObject);
  return newObject;
}

/************************************************************************
 * OLEPictureImpl_Destroy
 *
 * This method is called by the Release method when the reference
 * count goes down to 0. It will free all resources used by
 * this object.  */
static void OLEPictureImpl_Destroy(OLEPictureImpl* Obj)
{
  TRACE("(%p)\n", Obj);

  if (Obj->pCP)
    IConnectionPoint_Release(Obj->pCP);

  if(Obj->fOwn) { /* We need to destroy the picture */
    switch(Obj->desc.picType) {
    case PICTYPE_BITMAP:
      DeleteObject(Obj->desc.u.bmp.hbitmap);
      if (Obj->hbmMask != NULL) DeleteObject(Obj->hbmMask);
      if (Obj->hbmXor != NULL) DeleteObject(Obj->hbmXor);
      break;
    case PICTYPE_METAFILE:
      DeleteMetaFile(Obj->desc.u.wmf.hmeta);
      break;
    case PICTYPE_ICON:
      DestroyIcon(Obj->desc.u.icon.hicon);
      break;
    case PICTYPE_ENHMETAFILE:
      DeleteEnhMetaFile(Obj->desc.u.emf.hemf);
      break;
    case PICTYPE_NONE:
    case PICTYPE_UNINITIALIZED:
      /* Nothing to do */
      break;
    default:
      FIXME("Unsupported type %d - unable to delete\n", Obj->desc.picType);
      break;
    }
  }
  HeapFree(GetProcessHeap(), 0, Obj->data);
  HeapFree(GetProcessHeap(), 0, Obj);
}


/************************************************************************
 * OLEPictureImpl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_AddRef(
  IPicture* iface)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  ULONG refCount = InterlockedIncrement(&This->ref);

  TRACE("(%p)->(ref before=%d)\n", This, refCount - 1);

  return refCount;
}

/************************************************************************
 * OLEPictureImpl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_Release(
      IPicture* iface)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  ULONG refCount = InterlockedDecrement(&This->ref);

  TRACE("(%p)->(ref before=%d)\n", This, refCount + 1);

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (!refCount) OLEPictureImpl_Destroy(This);

  return refCount;
}

/************************************************************************
 * OLEPictureImpl_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEPictureImpl_QueryInterface(
  IPicture*  iface,
  REFIID  riid,
  void**  ppvObject)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;

  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IPicture, riid))
    *ppvObject = (IPicture*)This;
  else if (IsEqualIID(&IID_IDispatch, riid))
    *ppvObject = (IDispatch*)&(This->lpvtblIDispatch);
  else if (IsEqualIID(&IID_IPictureDisp, riid))
    *ppvObject = (IDispatch*)&(This->lpvtblIDispatch);
  else if (IsEqualIID(&IID_IPersist, riid) || IsEqualIID(&IID_IPersistStream, riid))
    *ppvObject = (IPersistStream*)&(This->lpvtblIPersistStream);
  else if (IsEqualIID(&IID_IConnectionPointContainer, riid))
    *ppvObject = (IConnectionPointContainer*)&(This->lpvtblIConnectionPointContainer);

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    FIXME("() : asking for un supported interface %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  OLEPictureImpl_AddRef((IPicture*)This);

  return S_OK;
}

/***********************************************************************
 *    OLEPicture_SendNotify (internal)
 *
 * Sends notification messages of changed properties to any interested
 * connections.
 */
static void OLEPicture_SendNotify(OLEPictureImpl* this, DISPID dispID)
{
  IEnumConnections *pEnum;
  CONNECTDATA CD;

  if (IConnectionPoint_EnumConnections(this->pCP, &pEnum))
      return;
  while(IEnumConnections_Next(pEnum, 1, &CD, NULL) == S_OK) {
    IPropertyNotifySink *sink;

    IUnknown_QueryInterface(CD.pUnk, &IID_IPropertyNotifySink, (LPVOID)&sink);
    IPropertyNotifySink_OnChanged(sink, dispID);
    IPropertyNotifySink_Release(sink);
    IUnknown_Release(CD.pUnk);
  }
  IEnumConnections_Release(pEnum);
  return;
}

/************************************************************************
 * OLEPictureImpl_get_Handle
 */
static HRESULT WINAPI OLEPictureImpl_get_Handle(IPicture *iface,
						OLE_HANDLE *phandle)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%p)\n", This, phandle);
  switch(This->desc.picType) {
  case PICTYPE_NONE:
  case PICTYPE_UNINITIALIZED:
    *phandle = 0;
    break;
  case PICTYPE_BITMAP:
    *phandle = (OLE_HANDLE)This->desc.u.bmp.hbitmap;
    break;
  case PICTYPE_METAFILE:
    *phandle = (OLE_HANDLE)This->desc.u.wmf.hmeta;
    break;
  case PICTYPE_ICON:
    *phandle = (OLE_HANDLE)This->desc.u.icon.hicon;
    break;
  case PICTYPE_ENHMETAFILE:
    *phandle = (OLE_HANDLE)This->desc.u.emf.hemf;
    break;
  default:
    FIXME("Unimplemented type %d\n", This->desc.picType);
    return E_NOTIMPL;
  }
  TRACE("returning handle %08x\n", *phandle);
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_get_hPal
 */
static HRESULT WINAPI OLEPictureImpl_get_hPal(IPicture *iface,
					      OLE_HANDLE *phandle)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  HRESULT hres;
  TRACE("(%p)->(%p)\n", This, phandle);

  if (!phandle)
    return E_POINTER;

  switch (This->desc.picType) {
    case PICTYPE_UNINITIALIZED:
    case PICTYPE_NONE:
      *phandle = 0;
      hres = S_FALSE;
      break;
    case PICTYPE_BITMAP:
      *phandle = (OLE_HANDLE)This->desc.u.bmp.hpal;
      hres = S_OK;
      break;
    case PICTYPE_ICON:
    case PICTYPE_METAFILE:
    case PICTYPE_ENHMETAFILE:
    default:
      FIXME("unimplemented for type %d. Returning 0 palette.\n",
           This->desc.picType);
      *phandle = 0;
      hres = S_OK;
  }

  TRACE("returning 0x%08x, palette handle %08x\n", hres, *phandle);
  return hres;
}

/************************************************************************
 * OLEPictureImpl_get_Type
 */
static HRESULT WINAPI OLEPictureImpl_get_Type(IPicture *iface,
					      short *ptype)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%p): type is %d\n", This, ptype, This->desc.picType);
  *ptype = This->desc.picType;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_get_Width
 */
static HRESULT WINAPI OLEPictureImpl_get_Width(IPicture *iface,
					       OLE_XSIZE_HIMETRIC *pwidth)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%p): width is %d\n", This, pwidth, This->himetricWidth);
  *pwidth = This->himetricWidth;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_get_Height
 */
static HRESULT WINAPI OLEPictureImpl_get_Height(IPicture *iface,
						OLE_YSIZE_HIMETRIC *pheight)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%p): height is %d\n", This, pheight, This->himetricHeight);
  *pheight = This->himetricHeight;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_Render
 */
static HRESULT WINAPI OLEPictureImpl_Render(IPicture *iface, HDC hdc,
					    LONG x, LONG y, LONG cx, LONG cy,
					    OLE_XPOS_HIMETRIC xSrc,
					    OLE_YPOS_HIMETRIC ySrc,
					    OLE_XSIZE_HIMETRIC cxSrc,
					    OLE_YSIZE_HIMETRIC cySrc,
					    LPCRECT prcWBounds)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%p, (%d,%d), (%d,%d) <- (%d,%d), (%d,%d), %p)\n",
	This, hdc, x, y, cx, cy, xSrc, ySrc, cxSrc, cySrc, prcWBounds);
  if(prcWBounds)
    TRACE("prcWBounds (%d,%d) - (%d,%d)\n", prcWBounds->left, prcWBounds->top,
	  prcWBounds->right, prcWBounds->bottom);

  /*
   * While the documentation suggests this to be here (or after rendering?)
   * it does cause an endless recursion in my sample app. -MM 20010804
  OLEPicture_SendNotify(This,DISPID_PICT_RENDER);
   */

  switch(This->desc.picType) {
  case PICTYPE_BITMAP:
    {
      HBITMAP hbmpOld;
      HDC hdcBmp;

      /* Set a mapping mode that maps bitmap pixels into HIMETRIC units.
         NB y-axis gets flipped */

      hdcBmp = CreateCompatibleDC(0);
      SetMapMode(hdcBmp, MM_ANISOTROPIC);
      SetWindowOrgEx(hdcBmp, 0, 0, NULL);
      SetWindowExtEx(hdcBmp, This->himetricWidth, This->himetricHeight, NULL);
      SetViewportOrgEx(hdcBmp, 0, This->origHeight, NULL);
      SetViewportExtEx(hdcBmp, This->origWidth, -This->origHeight, NULL);

      if (This->hbmMask) {
	  HDC hdcMask = CreateCompatibleDC(0);
	  HBITMAP hOldbm = SelectObject(hdcMask, This->hbmMask);

          hbmpOld = SelectObject(hdcBmp, This->hbmXor);

	  SetMapMode(hdcMask, MM_ANISOTROPIC);
	  SetWindowOrgEx(hdcMask, 0, 0, NULL);
	  SetWindowExtEx(hdcMask, This->himetricWidth, This->himetricHeight, NULL);
	  SetViewportOrgEx(hdcMask, 0, This->origHeight, NULL);
	  SetViewportExtEx(hdcMask, This->origWidth, -This->origHeight, NULL);

	  SetBkColor(hdc, RGB(255, 255, 255));
	  SetTextColor(hdc, RGB(0, 0, 0));
	  StretchBlt(hdc, x, y, cx, cy, hdcMask, xSrc, ySrc, cxSrc, cySrc, SRCAND);
	  StretchBlt(hdc, x, y, cx, cy, hdcBmp, xSrc, ySrc, cxSrc, cySrc, SRCPAINT);

	  SelectObject(hdcMask, hOldbm);
	  DeleteDC(hdcMask);
      } else {
          hbmpOld = SelectObject(hdcBmp, This->desc.u.bmp.hbitmap);
	  StretchBlt(hdc, x, y, cx, cy, hdcBmp, xSrc, ySrc, cxSrc, cySrc, SRCCOPY);
      }

      SelectObject(hdcBmp, hbmpOld);
      DeleteDC(hdcBmp);
    }
    break;
  case PICTYPE_ICON:
    FIXME("Not quite correct implementation of rendering icons...\n");
    DrawIcon(hdc,x,y,This->desc.u.icon.hicon);
    break;

  case PICTYPE_METAFILE:
    PlayMetaFile(hdc, This->desc.u.wmf.hmeta);
    break;

  case PICTYPE_ENHMETAFILE:
  {
    RECT rc = { x, y, cx, cy };
    PlayEnhMetaFile(hdc, This->desc.u.emf.hemf, &rc);
    break;
  }

  default:
    FIXME("type %d not implemented\n", This->desc.picType);
    return E_NOTIMPL;
  }
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_set_hPal
 */
static HRESULT WINAPI OLEPictureImpl_set_hPal(IPicture *iface,
					      OLE_HANDLE hpal)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  FIXME("(%p)->(%08x): stub\n", This, hpal);
  OLEPicture_SendNotify(This,DISPID_PICT_HPAL);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_get_CurDC
 */
static HRESULT WINAPI OLEPictureImpl_get_CurDC(IPicture *iface,
					       HDC *phdc)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p), returning %p\n", This, This->hDCCur);
  if (phdc) *phdc = This->hDCCur;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_SelectPicture
 */
static HRESULT WINAPI OLEPictureImpl_SelectPicture(IPicture *iface,
						   HDC hdcIn,
						   HDC *phdcOut,
						   OLE_HANDLE *phbmpOut)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%p, %p, %p)\n", This, hdcIn, phdcOut, phbmpOut);
  if (This->desc.picType == PICTYPE_BITMAP) {
      SelectObject(hdcIn,This->desc.u.bmp.hbitmap);

      if (phdcOut)
	  *phdcOut = This->hDCCur;
      This->hDCCur = hdcIn;
      if (phbmpOut)
	  *phbmpOut = (OLE_HANDLE)This->desc.u.bmp.hbitmap;
      return S_OK;
  } else {
      FIXME("Don't know how to select picture type %d\n",This->desc.picType);
      return E_FAIL;
  }
}

/************************************************************************
 * OLEPictureImpl_get_KeepOriginalFormat
 */
static HRESULT WINAPI OLEPictureImpl_get_KeepOriginalFormat(IPicture *iface,
							    BOOL *pfKeep)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%p)\n", This, pfKeep);
  if (!pfKeep)
      return E_POINTER;
  *pfKeep = This->keepOrigFormat;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_put_KeepOriginalFormat
 */
static HRESULT WINAPI OLEPictureImpl_put_KeepOriginalFormat(IPicture *iface,
							    BOOL keep)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%d)\n", This, keep);
  This->keepOrigFormat = keep;
  /* FIXME: what DISPID notification here? */
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_PictureChanged
 */
static HRESULT WINAPI OLEPictureImpl_PictureChanged(IPicture *iface)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->()\n", This);
  OLEPicture_SendNotify(This,DISPID_PICT_HANDLE);
  This->bIsDirty = TRUE;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_SaveAsFile
 */
static HRESULT WINAPI OLEPictureImpl_SaveAsFile(IPicture *iface,
						IStream *pstream,
						BOOL SaveMemCopy,
						LONG *pcbSize)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  FIXME("(%p)->(%p, %d, %p), hacked stub.\n", This, pstream, SaveMemCopy, pcbSize);
  return IStream_Write(pstream,This->data,This->datalen,(ULONG*)pcbSize);
}

/************************************************************************
 * OLEPictureImpl_get_Attributes
 */
static HRESULT WINAPI OLEPictureImpl_get_Attributes(IPicture *iface,
						    DWORD *pdwAttr)
{
  OLEPictureImpl *This = (OLEPictureImpl *)iface;
  TRACE("(%p)->(%p).\n", This, pdwAttr);
  *pdwAttr = 0;
  switch (This->desc.picType) {
  case PICTYPE_BITMAP: 	if (This->hbmMask) *pdwAttr = PICTURE_TRANSPARENT; break;	/* not 'truly' scalable, see MSDN. */
  case PICTYPE_ICON: *pdwAttr     = PICTURE_TRANSPARENT;break;
  case PICTYPE_ENHMETAFILE: /* fall through */
  case PICTYPE_METAFILE: *pdwAttr = PICTURE_TRANSPARENT|PICTURE_SCALABLE;break;
  default:FIXME("Unknown pictype %d\n",This->desc.picType);break;
  }
  return S_OK;
}


/************************************************************************
 *    IConnectionPointContainer
 */
static HRESULT WINAPI OLEPictureImpl_IConnectionPointContainer_QueryInterface(
  IConnectionPointContainer* iface,
  REFIID riid,
  VOID** ppvoid)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);

  return IPicture_QueryInterface((IPicture *)This,riid,ppvoid);
}

static ULONG WINAPI OLEPictureImpl_IConnectionPointContainer_AddRef(
  IConnectionPointContainer* iface)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);

  return IPicture_AddRef((IPicture *)This);
}

static ULONG WINAPI OLEPictureImpl_IConnectionPointContainer_Release(
  IConnectionPointContainer* iface)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);

  return IPicture_Release((IPicture *)This);
}

static HRESULT WINAPI OLEPictureImpl_EnumConnectionPoints(
  IConnectionPointContainer* iface,
  IEnumConnectionPoints** ppEnum)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);

  FIXME("(%p,%p), stub!\n",This,ppEnum);
  return E_NOTIMPL;
}

static HRESULT WINAPI OLEPictureImpl_FindConnectionPoint(
  IConnectionPointContainer* iface,
  REFIID riid,
  IConnectionPoint **ppCP)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);
  TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppCP);
  if (!ppCP)
      return E_POINTER;
  *ppCP = NULL;
  if (IsEqualGUID(riid,&IID_IPropertyNotifySink))
      return IConnectionPoint_QueryInterface(This->pCP,&IID_IConnectionPoint,(LPVOID)ppCP);
  FIXME("no connection point for %s\n",debugstr_guid(riid));
  return CONNECT_E_NOCONNECTION;
}


/************************************************************************
 *    IPersistStream
 */

/************************************************************************
 * OLEPictureImpl_IPersistStream_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEPictureImpl_IPersistStream_QueryInterface(
  IPersistStream* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);

  return IPicture_QueryInterface((IPicture *)This, riid, ppvoid);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IPersistStream_AddRef(
  IPersistStream* iface)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);

  return IPicture_AddRef((IPicture *)This);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IPersistStream_Release(
  IPersistStream* iface)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);

  return IPicture_Release((IPicture *)This);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_GetClassID
 */
static HRESULT WINAPI OLEPictureImpl_GetClassID(
  IPersistStream* iface,CLSID* pClassID)
{
  TRACE("(%p)\n", pClassID);
  memcpy(pClassID, &CLSID_StdPicture, sizeof(*pClassID));
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_IsDirty
 */
static HRESULT WINAPI OLEPictureImpl_IsDirty(
  IPersistStream* iface)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);
  FIXME("(%p),stub!\n",This);
  return E_NOTIMPL;
}

#ifdef SONAME_LIBJPEG

static void *libjpeg_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(jpeg_std_error);
MAKE_FUNCPTR(jpeg_CreateDecompress);
MAKE_FUNCPTR(jpeg_read_header);
MAKE_FUNCPTR(jpeg_start_decompress);
MAKE_FUNCPTR(jpeg_read_scanlines);
MAKE_FUNCPTR(jpeg_finish_decompress);
MAKE_FUNCPTR(jpeg_destroy_decompress);
#undef MAKE_FUNCPTR

static void *load_libjpeg(void)
{
    if((libjpeg_handle = wine_dlopen(SONAME_LIBJPEG, RTLD_NOW, NULL, 0)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libjpeg_handle, #f, NULL, 0)) == NULL) { \
        libjpeg_handle = NULL; \
        return NULL; \
    }

        LOAD_FUNCPTR(jpeg_std_error);
        LOAD_FUNCPTR(jpeg_CreateDecompress);
        LOAD_FUNCPTR(jpeg_read_header);
        LOAD_FUNCPTR(jpeg_start_decompress);
        LOAD_FUNCPTR(jpeg_read_scanlines);
        LOAD_FUNCPTR(jpeg_finish_decompress);
        LOAD_FUNCPTR(jpeg_destroy_decompress);
#undef LOAD_FUNCPTR
    }
    return libjpeg_handle;
}

/* for the jpeg decompressor source manager. */
static void _jpeg_init_source(j_decompress_ptr cinfo) { }

static boolean _jpeg_fill_input_buffer(j_decompress_ptr cinfo) {
    ERR("(), should not get here.\n");
    return FALSE;
}

static void _jpeg_skip_input_data(j_decompress_ptr cinfo,long num_bytes) {
    TRACE("Skipping %ld bytes...\n", num_bytes);
    cinfo->src->next_input_byte += num_bytes;
    cinfo->src->bytes_in_buffer -= num_bytes;
}

static boolean _jpeg_resync_to_restart(j_decompress_ptr cinfo, int desired) {
    ERR("(desired=%d), should not get here.\n",desired);
    return FALSE;
}
static void _jpeg_term_source(j_decompress_ptr cinfo) { }
#endif /* SONAME_LIBJPEG */

struct gifdata {
    unsigned char *data;
    unsigned int curoff;
    unsigned int len;
};

static int _gif_inputfunc(GifFileType *gif, GifByteType *data, int len) {
    struct gifdata *gd = (struct gifdata*)gif->UserData;

    if (len+gd->curoff > gd->len) {
        FIXME("Trying to read %d bytes, but only %d available.\n",len, gd->len-gd->curoff);
        len = gd->len - gd->curoff;
    }
    memcpy(data, gd->data+gd->curoff, len);
    gd->curoff += len;
    return len;
}


static HRESULT OLEPictureImpl_LoadGif(OLEPictureImpl *This, BYTE *xbuf, ULONG xread)
{
    struct gifdata 	gd;
    GifFileType 	*gif;
    BITMAPINFO		*bmi;
    HDC			hdcref;
    LPBYTE              bytes;
    int                 i,j,ret;
    GifImageDesc        *gid;
    SavedImage          *si;
    ColorMapObject      *cm;
    int                 transparent = -1;
    ExtensionBlock      *eb;
    int                 padding;

    gd.data   = xbuf;
    gd.curoff = 0;
    gd.len    = xread;
    gif = DGifOpen((void*)&gd, _gif_inputfunc);
    ret = DGifSlurp(gif);
    if (ret == GIF_ERROR) {
      FIXME("Failed reading GIF using libgif.\n");
      return E_FAIL;
    }
    TRACE("screen height %d, width %d\n", gif->SWidth, gif->SHeight);
    TRACE("color res %d, backgcolor %d\n", gif->SColorResolution, gif->SBackGroundColor);
    TRACE("imgcnt %d\n", gif->ImageCount);
    if (gif->ImageCount<1) {
      FIXME("GIF stream does not have images inside?\n");
      return E_FAIL;
    }
    TRACE("curimage: %d x %d, on %dx%d, interlace %d\n",
      gif->Image.Width, gif->Image.Height,
      gif->Image.Left, gif->Image.Top,
      gif->Image.Interlace
    );
    /* */
    padding = (gif->SWidth+3) & ~3;
    si   = gif->SavedImages+0;
    gid  = &(si->ImageDesc);
    cm   = gid->ColorMap;
    if (!cm) cm = gif->SColorMap;
    bmi  = HeapAlloc(GetProcessHeap(),0,sizeof(BITMAPINFOHEADER)+(cm->ColorCount)*sizeof(RGBQUAD));
    bytes= HeapAlloc(GetProcessHeap(),0,padding*gif->SHeight);

    /* look for the transparent color extension */
    for (i = 0; i < si->ExtensionBlockCount; ++i) {
	eb = si->ExtensionBlocks + i;
	if (eb->Function == 0xF9 && eb->ByteCount == 4) {
	    if ((eb->Bytes[0] & 1) == 1) {
		transparent = (unsigned char)eb->Bytes[3];
	    }
	}
    }

    for (i = 0; i < cm->ColorCount; i++) {
      bmi->bmiColors[i].rgbRed = cm->Colors[i].Red;
      bmi->bmiColors[i].rgbGreen = cm->Colors[i].Green;
      bmi->bmiColors[i].rgbBlue = cm->Colors[i].Blue;
      if (i == transparent) {
	  This->rgbTrans = RGB(bmi->bmiColors[i].rgbRed,
			       bmi->bmiColors[i].rgbGreen,
			       bmi->bmiColors[i].rgbBlue);
      }
    }

    /* Map to in picture coordinates */
    for (i = 0, j = 0; i < gid->Height; i++) {
        if (gif->Image.Interlace) {
            memcpy(
                bytes + (gid->Top + j) * padding + gid->Left,
                si->RasterBits + i * gid->Width,
                gid->Width);

            /* Lower bits of interlaced counter encode current interlace */
            if (j & 1) j += 2;      /* Currently filling odd rows */
            else if (j & 2) j += 4; /* Currently filling even rows not multiples of 4 */
            else j += 8;            /* Currently filling every 8th row or 4th row in-between */

            if (j >= gid->Height && i < gid->Height && (j & 1) == 0) {
                /* End of current interlace, go to next interlace */
                if (j & 2) j = 1;       /* Next iteration fills odd rows */
                else if (j & 4) j = 2;  /* Next iteration fills even rows not mod 4 and not mod 8 */
                else j = 4;             /* Next iteration fills rows in-between rows mod 6 */
            }
        } else {
            memcpy(
                bytes + (gid->Top + i) * padding + gid->Left,
                si->RasterBits + i * gid->Width,
                gid->Width);
        }
    }

    bmi->bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth		= gif->SWidth;
    bmi->bmiHeader.biHeight		= -gif->SHeight;
    bmi->bmiHeader.biPlanes		= 1;
    bmi->bmiHeader.biBitCount		= 8;
    bmi->bmiHeader.biCompression	= BI_RGB;
    bmi->bmiHeader.biSizeImage		= padding*gif->SHeight;
    bmi->bmiHeader.biXPelsPerMeter	= 0;
    bmi->bmiHeader.biYPelsPerMeter	= 0;
    bmi->bmiHeader.biClrUsed		= cm->ColorCount;
    bmi->bmiHeader.biClrImportant	= 0;

    hdcref = GetDC(0);
    This->desc.u.bmp.hbitmap=CreateDIBitmap(
	    hdcref,
	    &bmi->bmiHeader,
	    CBM_INIT,
	    bytes,
	    bmi,
	    DIB_RGB_COLORS
    );

    if (transparent > -1) {
	/* Create the Mask */
	HDC hdc = CreateCompatibleDC(0);
	HDC hdcMask = CreateCompatibleDC(0);
	HBITMAP hOldbitmap;
	HBITMAP hOldbitmapmask;

        unsigned int monopadding = (((unsigned)(gif->SWidth + 31)) >> 5) << 2;
        HBITMAP hTempMask;

        This->hbmXor = CreateDIBitmap(
            hdcref,
            &bmi->bmiHeader,
            CBM_INIT,
            bytes,
            bmi,
            DIB_RGB_COLORS
        );

        bmi->bmiColors[0].rgbRed = 0;
        bmi->bmiColors[0].rgbGreen = 0;
        bmi->bmiColors[0].rgbBlue = 0;
        bmi->bmiColors[1].rgbRed = 255;
        bmi->bmiColors[1].rgbGreen = 255;
        bmi->bmiColors[1].rgbBlue = 255;

        bmi->bmiHeader.biBitCount		= 1;
        bmi->bmiHeader.biSizeImage		= monopadding*gif->SHeight;
        bmi->bmiHeader.biClrUsed		= 2;

        for (i = 0; i < gif->SHeight; i++) {
            unsigned char * colorPointer = bytes + padding * i;
            unsigned char * monoPointer = bytes + monopadding * i;
            for (j = 0; j < gif->SWidth; j++) {
                unsigned char pixel = colorPointer[j];
                if ((j & 7) == 0) monoPointer[j >> 3] = 0;
                if (pixel == (transparent & 0x000000FFU)) monoPointer[j >> 3] |= 1 << (7 - (j & 7));
            }
        }
        hdcref = GetDC(0);
        hTempMask = CreateDIBitmap(
                hdcref,
                &bmi->bmiHeader,
                CBM_INIT,
                bytes,
                bmi,
                DIB_RGB_COLORS
        );
        DeleteDC(hdcref);

        bmi->bmiHeader.biHeight = -bmi->bmiHeader.biHeight;
        This->hbmMask = CreateBitmap(bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight, 1, 1, NULL);
	hOldbitmap = SelectObject(hdc, hTempMask);
	hOldbitmapmask = SelectObject(hdcMask, This->hbmMask);

        SetBkColor(hdc, RGB(255, 255, 255));
	BitBlt(hdcMask, 0, 0, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight, hdc, 0, 0, SRCCOPY);

	/* We no longer need the original bitmap, so we apply the first
	   transformation with the mask to speed up the rendering */
        SelectObject(hdc, This->hbmXor);
	SetBkColor(hdc, RGB(0,0,0));
	SetTextColor(hdc, RGB(255,255,255));
	BitBlt(hdc, 0, 0, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight,
		 hdcMask, 0, 0,  SRCAND);

	SelectObject(hdc, hOldbitmap);
	SelectObject(hdcMask, hOldbitmapmask);
	DeleteDC(hdcMask);
	DeleteDC(hdc);
        DeleteObject(hTempMask);
    }

    DeleteDC(hdcref);
    This->desc.picType = PICTYPE_BITMAP;
    OLEPictureImpl_SetBitmap(This);
    DGifCloseFile(gif);
    HeapFree(GetProcessHeap(),0,bytes);
    return S_OK;
}

static HRESULT OLEPictureImpl_LoadJpeg(OLEPictureImpl *This, BYTE *xbuf, ULONG xread)
{
#ifdef SONAME_LIBJPEG
    struct jpeg_decompress_struct	jd;
    struct jpeg_error_mgr		jerr;
    int					ret;
    JDIMENSION				x;
    JSAMPROW				samprow,oldsamprow;
    BITMAPINFOHEADER			bmi;
    LPBYTE				bits;
    HDC					hdcref;
    struct jpeg_source_mgr		xjsm;
    LPBYTE                              oldbits;
    unsigned int i;

    if(!libjpeg_handle) {
        if(!load_libjpeg()) {
            FIXME("Failed reading JPEG because unable to find %s\n", SONAME_LIBJPEG);
            return E_FAIL;
        }
    }

    /* This is basically so we can use in-memory data for jpeg decompression.
     * We need to have all the functions.
     */
    xjsm.next_input_byte	= xbuf;
    xjsm.bytes_in_buffer	= xread;
    xjsm.init_source		= _jpeg_init_source;
    xjsm.fill_input_buffer	= _jpeg_fill_input_buffer;
    xjsm.skip_input_data	= _jpeg_skip_input_data;
    xjsm.resync_to_restart	= _jpeg_resync_to_restart;
    xjsm.term_source		= _jpeg_term_source;

    jd.err = pjpeg_std_error(&jerr);
    /* jpeg_create_decompress is a macro that expands to jpeg_CreateDecompress - see jpeglib.h
     * jpeg_create_decompress(&jd); */
    pjpeg_CreateDecompress(&jd, JPEG_LIB_VERSION, (size_t) sizeof(struct jpeg_decompress_struct));
    jd.src = &xjsm;
    ret=pjpeg_read_header(&jd,TRUE);
    jd.out_color_space = JCS_RGB;
    pjpeg_start_decompress(&jd);
    if (ret != JPEG_HEADER_OK) {
	ERR("Jpeg image in stream has bad format, read header returned %d.\n",ret);
	HeapFree(GetProcessHeap(),0,xbuf);
	return E_FAIL;
    }

    bits = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                     (jd.output_height+1) * ((jd.output_width*jd.output_components + 3) & ~3) );
    samprow=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,jd.output_width*jd.output_components);

    oldbits = bits;
    oldsamprow = samprow;
    while ( jd.output_scanline<jd.output_height ) {
      x = pjpeg_read_scanlines(&jd,&samprow,1);
      if (x != 1) {
	FIXME("failed to read current scanline?\n");
	break;
      }
      /* We have to convert from RGB to BGR, see MSDN/ BITMAPINFOHEADER */
      for(i=0;i<jd.output_width;i++,samprow+=jd.output_components) {
	*(bits++) = *(samprow+2);
	*(bits++) = *(samprow+1);
	*(bits++) = *(samprow);
      }
      bits = (LPBYTE)(((UINT_PTR)bits + 3) & ~3);
      samprow = oldsamprow;
    }
    bits = oldbits;

    bmi.biSize		= sizeof(bmi);
    bmi.biWidth		=  jd.output_width;
    bmi.biHeight	= -jd.output_height;
    bmi.biPlanes	= 1;
    bmi.biBitCount	= jd.output_components<<3;
    bmi.biCompression	= BI_RGB;
    bmi.biSizeImage	= jd.output_height*jd.output_width*jd.output_components;
    bmi.biXPelsPerMeter	= 0;
    bmi.biYPelsPerMeter	= 0;
    bmi.biClrUsed	= 0;
    bmi.biClrImportant	= 0;

    HeapFree(GetProcessHeap(),0,samprow);
    pjpeg_finish_decompress(&jd);
    pjpeg_destroy_decompress(&jd);
    hdcref = GetDC(0);
    This->desc.u.bmp.hbitmap=CreateDIBitmap(
	    hdcref,
	    &bmi,
	    CBM_INIT,
	    bits,
	    (BITMAPINFO*)&bmi,
	    DIB_RGB_COLORS
    );
    DeleteDC(hdcref);
    This->desc.picType = PICTYPE_BITMAP;
    OLEPictureImpl_SetBitmap(This);
    HeapFree(GetProcessHeap(),0,bits);
    return S_OK;
#else
    ERR("Trying to load JPEG picture, but JPEG supported not compiled in.\n");
    return E_FAIL;
#endif
}

static HRESULT OLEPictureImpl_LoadDIB(OLEPictureImpl *This, BYTE *xbuf, ULONG xread)
{
    BITMAPFILEHEADER	*bfh = (BITMAPFILEHEADER*)xbuf;
    BITMAPINFO		*bi = (BITMAPINFO*)(bfh+1);
    HDC			hdcref;

    /* Does not matter whether this is a coreheader or not, we only use
     * components which are in both
     */
    hdcref = GetDC(0);
    This->desc.u.bmp.hbitmap = CreateDIBitmap(
	hdcref,
	&(bi->bmiHeader),
	CBM_INIT,
	xbuf+bfh->bfOffBits,
	bi,
       DIB_RGB_COLORS
    );
    DeleteDC(hdcref);
    This->desc.picType = PICTYPE_BITMAP;
    OLEPictureImpl_SetBitmap(This);
    return S_OK;
}

/*****************************************************
*   start of PNG-specific code
*   currently only supports colortype PNG_COLOR_TYPE_RGB
*/
#ifdef SONAME_LIBPNG
typedef struct{
    ULONG position;
    ULONG size;
    BYTE * buff;
} png_io;

static void png_stream_read_data(png_structp png_ptr, png_bytep data,
    png_size_t length)
{
    png_io * io_ptr = png_ptr->io_ptr;

    if(length + io_ptr->position > io_ptr->size){
        length = io_ptr->size - io_ptr->position;
    }

    memcpy(data, io_ptr->buff + io_ptr->position, length);

    io_ptr->position += length;
}

static void *libpng_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(png_create_read_struct);
MAKE_FUNCPTR(png_create_info_struct);
MAKE_FUNCPTR(png_set_read_fn);
MAKE_FUNCPTR(png_read_info);
MAKE_FUNCPTR(png_read_image);
MAKE_FUNCPTR(png_get_rowbytes);
MAKE_FUNCPTR(png_set_bgr);
MAKE_FUNCPTR(png_destroy_read_struct);
MAKE_FUNCPTR(png_set_palette_to_rgb);
MAKE_FUNCPTR(png_read_update_info);
#undef MAKE_FUNCPTR

static void *load_libpng(void)
{
    if((libpng_handle = wine_dlopen(SONAME_LIBPNG, RTLD_NOW, NULL, 0)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libpng_handle, #f, NULL, 0)) == NULL) { \
        libpng_handle = NULL; \
        return NULL; \
    }
        LOAD_FUNCPTR(png_create_read_struct);
        LOAD_FUNCPTR(png_create_info_struct);
        LOAD_FUNCPTR(png_set_read_fn);
        LOAD_FUNCPTR(png_read_info);
        LOAD_FUNCPTR(png_read_image);
        LOAD_FUNCPTR(png_get_rowbytes);
        LOAD_FUNCPTR(png_set_bgr);
        LOAD_FUNCPTR(png_destroy_read_struct);
        LOAD_FUNCPTR(png_set_palette_to_rgb);
        LOAD_FUNCPTR(png_read_update_info);

#undef LOAD_FUNCPTR
    }
    return libpng_handle;
}
#endif /* SONAME_LIBPNG */

static HRESULT OLEPictureImpl_LoadPNG(OLEPictureImpl *This, BYTE *xbuf, ULONG xread)
{
#ifdef SONAME_LIBPNG
    png_io              io;
    png_structp         png_ptr = NULL;
    png_infop           info_ptr = NULL;
    INT                 row, rowsize, height, width;
    png_bytep*          row_pointers = NULL;
    png_bytep           pngdata = NULL;
    BITMAPINFOHEADER    bmi;
    HDC                 hdcref = NULL;
    HRESULT             ret;
    BOOL                set_bgr = FALSE;

    if(!libpng_handle) {
        if(!load_libpng()) {
            ERR("Failed reading PNG because unable to find %s\n",SONAME_LIBPNG);
            return E_FAIL;
        }
    }

    io.size     = xread;
    io.position = 0;
    io.buff     = xbuf;

    png_ptr = ppng_create_read_struct(PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL);

    if(setjmp(png_jmpbuf(png_ptr))){
        TRACE("Error in libpng\n");
        ret = E_FAIL;
        goto pngend;
    }

    info_ptr = ppng_create_info_struct(png_ptr);
    ppng_set_read_fn(png_ptr, &io, png_stream_read_data);
    ppng_read_info(png_ptr, info_ptr);

    if(!(png_ptr->color_type == PNG_COLOR_TYPE_RGB ||
         png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)){
        FIXME("Unsupported .PNG type: %d\n", png_ptr->color_type);
        ret = E_FAIL;
        goto pngend;
    }

    if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE){
        ppng_set_palette_to_rgb(png_ptr);
        set_bgr = TRUE;
    }

    if (png_ptr->color_type == PNG_COLOR_TYPE_RGB ||
        png_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
        set_bgr){
        ppng_set_bgr(png_ptr);
    }

    ppng_read_update_info(png_ptr, info_ptr);

    rowsize = ppng_get_rowbytes(png_ptr, info_ptr);
    /* align rowsize to 4-byte boundary */
    rowsize = (rowsize + 3) & ~3;
    height = info_ptr->height;
    width = info_ptr->width;

    pngdata = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, height * rowsize);
    row_pointers = HeapAlloc(GetProcessHeap(), 0, height * (sizeof(VOID *)));

    if(!pngdata || !row_pointers){
        ret = E_FAIL;
        goto pngend;
    }

    for (row = 0; row < height; row++){
        row_pointers[row] = pngdata + row * rowsize;
    }

    ppng_read_image(png_ptr, row_pointers);

    bmi.biSize          = sizeof(bmi);
    bmi.biWidth         = width;
    bmi.biHeight        = -height;
    bmi.biPlanes        = 1;
    bmi.biBitCount      = info_ptr->channels * 8;
    bmi.biCompression   = BI_RGB;
    bmi.biSizeImage     = height * rowsize;
    bmi.biXPelsPerMeter = 0;
    bmi.biYPelsPerMeter = 0;
    bmi.biClrUsed       = 0;
    bmi.biClrImportant  = 0;

    hdcref = GetDC(0);
    This->desc.u.bmp.hbitmap = CreateDIBitmap(
        hdcref,
        &bmi,
        CBM_INIT,
        pngdata,
        (BITMAPINFO*)&bmi,
        DIB_RGB_COLORS
    );
    ReleaseDC(0, hdcref);
    This->desc.picType = PICTYPE_BITMAP;
    OLEPictureImpl_SetBitmap(This);
    ret = S_OK;

pngend:
    if(png_ptr)
        ppng_destroy_read_struct(&png_ptr,
                                (info_ptr ? &info_ptr : (png_infopp) NULL),
                                (png_infopp)NULL);
    HeapFree(GetProcessHeap(), 0, row_pointers);
    HeapFree(GetProcessHeap(), 0, pngdata);
    return ret;
#else /* SONAME_LIBPNG */
    ERR("Trying to load PNG picture, but PNG supported not compiled in.\n");
    return E_FAIL;
#endif
}

/*****************************************************
*   start of Icon-specific code
*/

static HRESULT OLEPictureImpl_LoadIcon(OLEPictureImpl *This, BYTE *xbuf, ULONG xread)
{
    HICON hicon;
    CURSORICONFILEDIR	*cifd = (CURSORICONFILEDIR*)xbuf;
    HDC hdcRef;
    int	i;

    /*
    FIXME("icon.idReserved=%d\n",cifd->idReserved);
    FIXME("icon.idType=%d\n",cifd->idType);
    FIXME("icon.idCount=%d\n",cifd->idCount);

    for (i=0;i<cifd->idCount;i++) {
	FIXME("[%d] width %d\n",i,cifd->idEntries[i].bWidth);
	FIXME("[%d] height %d\n",i,cifd->idEntries[i].bHeight);
	FIXME("[%d] bColorCount %d\n",i,cifd->idEntries[i].bColorCount);
	FIXME("[%d] bReserved %d\n",i,cifd->idEntries[i].bReserved);
	FIXME("[%d] xHotspot %d\n",i,cifd->idEntries[i].xHotspot);
	FIXME("[%d] yHotspot %d\n",i,cifd->idEntries[i].yHotspot);
	FIXME("[%d] dwDIBSize %d\n",i,cifd->idEntries[i].dwDIBSize);
	FIXME("[%d] dwDIBOffset %d\n",i,cifd->idEntries[i].dwDIBOffset);
    }
    */
    i=0;
    /* If we have more than one icon, try to find the best.
     * this currently means '32 pixel wide'.
     */
    if (cifd->idCount!=1) {
	for (i=0;i<cifd->idCount;i++) {
	    if (cifd->idEntries[i].bWidth == 32)
		break;
	}
	if (i==cifd->idCount) i=0;
    }

    hicon = CreateIconFromResourceEx(
		xbuf+cifd->idEntries[i].dwDIBOffset,
		cifd->idEntries[i].dwDIBSize,
		TRUE, /* is icon */
		0x00030000,
		cifd->idEntries[i].bWidth,
		cifd->idEntries[i].bHeight,
		0
    );
    if (!hicon) {
	FIXME("CreateIcon failed.\n");
	return E_FAIL;
    } else {
	This->desc.picType = PICTYPE_ICON;
	This->desc.u.icon.hicon = hicon;
	This->origWidth = cifd->idEntries[i].bWidth;
	This->origHeight = cifd->idEntries[i].bHeight;
	hdcRef = CreateCompatibleDC(0);
	This->himetricWidth =(cifd->idEntries[i].bWidth *2540)/GetDeviceCaps(hdcRef, LOGPIXELSX);
	This->himetricHeight=(cifd->idEntries[i].bHeight*2540)/GetDeviceCaps(hdcRef, LOGPIXELSY);
	DeleteDC(hdcRef);
	return S_OK;
    }
}

static HRESULT OLEPictureImpl_LoadMetafile(OLEPictureImpl *This,
                                           const BYTE *data, ULONG size)
{
    HMETAFILE hmf;
    HENHMETAFILE hemf;

    /* SetMetaFileBitsEx performs data check on its own */
    hmf = SetMetaFileBitsEx(size, data);
    if (hmf)
    {
        This->desc.picType = PICTYPE_METAFILE;
        This->desc.u.wmf.hmeta = hmf;
        This->desc.u.wmf.xExt = 0;
        This->desc.u.wmf.yExt = 0;

        This->origWidth = 0;
        This->origHeight = 0;
        This->himetricWidth = 0;
        This->himetricHeight = 0;

        return S_OK;
    }

    hemf = SetEnhMetaFileBits(size, data);
    if (!hemf) return E_FAIL;

    This->desc.picType = PICTYPE_ENHMETAFILE;
    This->desc.u.emf.hemf = hemf;

    This->origWidth = 0;
    This->origHeight = 0;
    This->himetricWidth = 0;
    This->himetricHeight = 0;

    return S_OK;
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_Load (IUnknown)
 *
 * Loads the binary data from the IStream. Starts at current position.
 * There appears to be an 2 DWORD header:
 * 	DWORD magic;
 * 	DWORD len;
 *
 * Currently implemented: BITMAP, ICON, JPEG, GIF, WMF, EMF
 */
static HRESULT WINAPI OLEPictureImpl_Load(IPersistStream* iface,IStream*pStm) {
  HRESULT	hr = E_FAIL;
  BOOL		headerisdata = FALSE;
  BOOL		statfailed = FALSE;
  ULONG		xread, toread;
  ULONG 	headerread;
  BYTE 		*xbuf;
  DWORD		header[2];
  WORD		magic;
  STATSTG       statstg;
  OLEPictureImpl *This = impl_from_IPersistStream(iface);

  TRACE("(%p,%p)\n",This,pStm);

  /****************************************************************************************
   * Part 1: Load the data
   */
  /* Sometimes we have a header, sometimes we don't. Apply some guesses to find
   * out whether we do.
   *
   * UPDATE: the IStream can be mapped to a plain file instead of a stream in a
   * compound file. This may explain most, if not all, of the cases of "no
   * header", and the header validation should take this into account.
   * At least in Visual Basic 6, resource streams, valid headers are
   *    header[0] == "lt\0\0",
   *    header[1] == length_of_stream.
   *
   * Also handle streams where we do not have a working "Stat" method by
   * reading all data until the end of the stream.
   */
  hr=IStream_Stat(pStm,&statstg,STATFLAG_NONAME);
  if (hr) {
      TRACE("stat failed with hres %x, proceeding to read all data.\n",hr);
      statfailed = TRUE;
      /* we will read at least 8 byte ... just right below */
      statstg.cbSize.QuadPart = 8;
  }

  toread = 0;
  headerread = 0;
  headerisdata = FALSE;
  do {
      hr=IStream_Read(pStm,header,8,&xread);
      if (hr || xread!=8) {
          FIXME("Failure while reading picture header (hr is %x, nread is %d).\n",hr,xread);
          return hr;
      }
      headerread += xread;
      xread = 0;

      if (!memcmp(&(header[0]),"lt\0\0", 4) && (statfailed || (header[1] + headerread <= statstg.cbSize.QuadPart))) {
          if (toread != 0 && toread != header[1])
              FIXME("varying lengths of image data (prev=%u curr=%u), only last one will be used\n",
                  toread, header[1]);
          toread = header[1];
          if (toread == 0) break;
      } else {
          if (!memcmp(&(header[0]), "GIF8",     4) ||   /* GIF header */
              !memcmp(&(header[0]), "BM",       2) ||   /* BMP header */
              !memcmp(&(header[0]), "\xff\xd8", 2) ||   /* JPEG header */
              (header[1] > statstg.cbSize.QuadPart)||   /* invalid size */
              (header[1]==0)
          ) {/* Found start of bitmap data */
              headerisdata = TRUE;
              if (toread == 0)
              	  toread = statstg.cbSize.QuadPart-8;
              else toread -= 8;
              xread = 8;
          } else {
              FIXME("Unknown stream header magic: %08x\n", header[0]);
              toread = header[1];
          }
      }
  } while (!headerisdata);

  if (statfailed) { /* we don't know the size ... read all we get */
      int sizeinc = 4096;
      int origsize = sizeinc;
      ULONG nread = 42;

      TRACE("Reading all data from stream.\n");
      xbuf = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, origsize);
      if (headerisdata)
          memcpy (xbuf, &header, 8);
      while (1) {
          while (xread < origsize) {
              hr = IStream_Read(pStm,xbuf+xread,origsize-xread,&nread);
              xread+=nread;
              if (hr || !nread)
                  break;
          }
          if (!nread || hr) /* done, or error */
              break;
          if (xread == origsize) {
              origsize += sizeinc;
              sizeinc = 2*sizeinc; /* exponential increase */
              xbuf = HeapReAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, xbuf, origsize);
          }
      }
      if (hr)
          TRACE("hr in no-stat loader case is %08x\n", hr);
      TRACE("loaded %d bytes.\n", xread);
      This->datalen = xread;
      This->data    = xbuf;
  } else {
      This->datalen = toread+(headerisdata?8:0);
      xbuf = This->data = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, This->datalen);

      if (headerisdata)
          memcpy (xbuf, &header, 8);

      while (xread < This->datalen) {
          ULONG nread;
          hr = IStream_Read(pStm,xbuf+xread,This->datalen-xread,&nread);
          xread+=nread;
          if (hr || !nread)
              break;
      }
      if (xread != This->datalen)
          FIXME("Could only read %d of %d bytes out of stream?\n",xread,This->datalen);
  }
  if (This->datalen == 0) { /* Marks the "NONE" picture */
      This->desc.picType = PICTYPE_NONE;
      return S_OK;
  }


  /****************************************************************************************
   * Part 2: Process the loaded data
   */

  magic = xbuf[0] + (xbuf[1]<<8);
  This->loadtime_format = magic;

  switch (magic) {
  case 0x4947: /* GIF */
    hr = OLEPictureImpl_LoadGif(This, xbuf, xread);
    break;
  case 0xd8ff: /* JPEG */
    hr = OLEPictureImpl_LoadJpeg(This, xbuf, xread);
    break;
  case 0x4d42: /* Bitmap */
    hr = OLEPictureImpl_LoadDIB(This, xbuf, xread);
    break;
  case 0x5089: /* PNG */
    hr = OLEPictureImpl_LoadPNG(This, xbuf, xread);
    break;
  case 0x0000: { /* ICON , first word is dwReserved */
    hr = OLEPictureImpl_LoadIcon(This, xbuf, xread);
    break;
  }
  default:
  {
    unsigned int i;

    /* let's see if it's a metafile */
    hr = OLEPictureImpl_LoadMetafile(This, xbuf, xread);
    if (hr == S_OK) break;

    FIXME("Unknown magic %04x, %d read bytes:\n",magic,xread);
    hr=E_FAIL;
    for (i=0;i<xread+8;i++) {
	if (i<8) MESSAGE("%02x ",((unsigned char*)&header)[i]);
	else MESSAGE("%02x ",xbuf[i-8]);
        if (i % 10 == 9) MESSAGE("\n");
    }
    MESSAGE("\n");
    break;
  }
  }
  This->bIsDirty = FALSE;

  /* FIXME: this notify is not really documented */
  if (hr==S_OK)
      OLEPicture_SendNotify(This,DISPID_PICT_TYPE);
  return hr;
}

static int serializeBMP(HBITMAP hBitmap, void ** ppBuffer, unsigned int * pLength)
{
    int iSuccess = 0;
    HDC hDC;
    BITMAPINFO * pInfoBitmap;
    int iNumPaletteEntries;
    unsigned char * pPixelData;
    BITMAPFILEHEADER * pFileHeader;
    BITMAPINFO * pInfoHeader;

    pInfoBitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

    /* Find out bitmap size and padded length */
    hDC = GetDC(0);
    pInfoBitmap->bmiHeader.biSize = sizeof(pInfoBitmap->bmiHeader);
    GetDIBits(hDC, hBitmap, 0, 0, NULL, pInfoBitmap, DIB_RGB_COLORS);

    /* Fetch bitmap palette & pixel data */

    pPixelData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pInfoBitmap->bmiHeader.biSizeImage);
    GetDIBits(hDC, hBitmap, 0, pInfoBitmap->bmiHeader.biHeight, pPixelData, pInfoBitmap, DIB_RGB_COLORS);

    /* Calculate the total length required for the BMP data */
    if (pInfoBitmap->bmiHeader.biClrUsed != 0) {
	iNumPaletteEntries = pInfoBitmap->bmiHeader.biClrUsed;
	if (iNumPaletteEntries > 256) iNumPaletteEntries = 256;
    } else {
	if (pInfoBitmap->bmiHeader.biBitCount <= 8)
	    iNumPaletteEntries = 1 << pInfoBitmap->bmiHeader.biBitCount;
	else
    	    iNumPaletteEntries = 0;
    }
    *pLength =
        sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPINFOHEADER) +
        iNumPaletteEntries * sizeof(RGBQUAD) +
        pInfoBitmap->bmiHeader.biSizeImage;
    *ppBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *pLength);

    /* Fill the BITMAPFILEHEADER */
    pFileHeader = (BITMAPFILEHEADER *)(*ppBuffer);
    pFileHeader->bfType = 0x4d42;
    pFileHeader->bfSize = *pLength;
    pFileHeader->bfOffBits =
        sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPINFOHEADER) +
        iNumPaletteEntries * sizeof(RGBQUAD);

    /* Fill the BITMAPINFOHEADER and the palette data */
    pInfoHeader = (BITMAPINFO *)((unsigned char *)(*ppBuffer) + sizeof(BITMAPFILEHEADER));
    memcpy(pInfoHeader, pInfoBitmap, sizeof(BITMAPINFOHEADER) + iNumPaletteEntries * sizeof(RGBQUAD));
    memcpy(
        (unsigned char *)(*ppBuffer) +
            sizeof(BITMAPFILEHEADER) +
            sizeof(BITMAPINFOHEADER) +
            iNumPaletteEntries * sizeof(RGBQUAD),
        pPixelData, pInfoBitmap->bmiHeader.biSizeImage);
    iSuccess = 1;

    HeapFree(GetProcessHeap(), 0, pPixelData);
    HeapFree(GetProcessHeap(), 0, pInfoBitmap);
    return iSuccess;
}

static int serializeIcon(HICON hIcon, void ** ppBuffer, unsigned int * pLength)
{
	ICONINFO infoIcon;
	int iSuccess = 0;

	*ppBuffer = NULL; *pLength = 0;
	if (GetIconInfo(hIcon, &infoIcon)) {
		HDC hDC;
		BITMAPINFO * pInfoBitmap;
		unsigned char * pIconData = NULL;
		unsigned int iDataSize = 0;

        pInfoBitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

		/* Find out icon size */
		hDC = GetDC(0);
		pInfoBitmap->bmiHeader.biSize = sizeof(pInfoBitmap->bmiHeader);
		GetDIBits(hDC, infoIcon.hbmColor, 0, 0, NULL, pInfoBitmap, DIB_RGB_COLORS);
		if (1) {
			/* Auxiliary pointers */
			CURSORICONFILEDIR * pIconDir;
			CURSORICONFILEDIRENTRY * pIconEntry;
			BITMAPINFOHEADER * pIconBitmapHeader;
			unsigned int iOffsetPalette;
			unsigned int iOffsetColorData;
			unsigned int iOffsetMaskData;

			unsigned int iLengthScanLineColor;
			unsigned int iLengthScanLineMask;
			unsigned int iNumEntriesPalette;

			iLengthScanLineMask = ((pInfoBitmap->bmiHeader.biWidth + 31) >> 5) << 2;
			iLengthScanLineColor = ((pInfoBitmap->bmiHeader.biWidth * pInfoBitmap->bmiHeader.biBitCount + 31) >> 5) << 2;
/*
			FIXME("DEBUG: bitmap size is %d x %d\n",
				pInfoBitmap->bmiHeader.biWidth,
				pInfoBitmap->bmiHeader.biHeight);
			FIXME("DEBUG: bitmap bpp is %d\n",
				pInfoBitmap->bmiHeader.biBitCount);
			FIXME("DEBUG: bitmap nplanes is %d\n",
				pInfoBitmap->bmiHeader.biPlanes);
			FIXME("DEBUG: bitmap biSizeImage is %u\n",
				pInfoBitmap->bmiHeader.biSizeImage);
*/
			/* Let's start with one CURSORICONFILEDIR and one CURSORICONFILEDIRENTRY */
			iDataSize += 3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY) + sizeof(BITMAPINFOHEADER);
			pIconData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, iDataSize);

			/* Fill out the CURSORICONFILEDIR */
			pIconDir = (CURSORICONFILEDIR *)pIconData;
			pIconDir->idType = 1;
			pIconDir->idCount = 1;

			/* Fill out the CURSORICONFILEDIRENTRY */
			pIconEntry = (CURSORICONFILEDIRENTRY *)(pIconData + 3 * sizeof(WORD));
			pIconEntry->bWidth = (unsigned char)pInfoBitmap->bmiHeader.biWidth;
			pIconEntry->bHeight = (unsigned char)pInfoBitmap->bmiHeader.biHeight;
			pIconEntry->bColorCount =
				(pInfoBitmap->bmiHeader.biBitCount < 8)
				? 1 << pInfoBitmap->bmiHeader.biBitCount
				: 0;
			pIconEntry->xHotspot = pInfoBitmap->bmiHeader.biPlanes;
			pIconEntry->yHotspot = pInfoBitmap->bmiHeader.biBitCount;
			pIconEntry->dwDIBSize = 0;
			pIconEntry->dwDIBOffset = 3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY);

			/* Fill out the BITMAPINFOHEADER */
			pIconBitmapHeader = (BITMAPINFOHEADER *)(pIconData + 3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY));
			memcpy(pIconBitmapHeader, &pInfoBitmap->bmiHeader, sizeof(BITMAPINFOHEADER));

			/*	Find out whether a palette exists for the bitmap */
			if (	(pInfoBitmap->bmiHeader.biBitCount == 16 && pInfoBitmap->bmiHeader.biCompression == BI_RGB)
				||	(pInfoBitmap->bmiHeader.biBitCount == 24)
				||	(pInfoBitmap->bmiHeader.biBitCount == 32 && pInfoBitmap->bmiHeader.biCompression == BI_RGB)) {
				iNumEntriesPalette = pInfoBitmap->bmiHeader.biClrUsed;
				if (iNumEntriesPalette > 256) iNumEntriesPalette = 256;
			} else if ((pInfoBitmap->bmiHeader.biBitCount == 16 || pInfoBitmap->bmiHeader.biBitCount == 32)
				&& pInfoBitmap->bmiHeader.biCompression == BI_BITFIELDS) {
				iNumEntriesPalette = 3;
			} else if (pInfoBitmap->bmiHeader.biBitCount <= 8) {
				iNumEntriesPalette = 1 << pInfoBitmap->bmiHeader.biBitCount;
			} else {
				iNumEntriesPalette = 0;
			}

			/*  Add bitmap size and header size to icon data size. */
			iOffsetPalette = iDataSize;
			iDataSize += iNumEntriesPalette * sizeof(DWORD);
			iOffsetColorData = iDataSize;
			iDataSize += pIconBitmapHeader->biSizeImage;
			iOffsetMaskData = iDataSize;
			iDataSize += pIconBitmapHeader->biHeight * iLengthScanLineMask;
			pIconBitmapHeader->biSizeImage += pIconBitmapHeader->biHeight * iLengthScanLineMask;
			pIconBitmapHeader->biHeight *= 2;
			pIconData = HeapReAlloc(GetProcessHeap(), 0, pIconData, iDataSize);
			pIconEntry = (CURSORICONFILEDIRENTRY *)(pIconData + 3 * sizeof(WORD));
			pIconBitmapHeader = (BITMAPINFOHEADER *)(pIconData + 3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY));
			pIconEntry->dwDIBSize = iDataSize - (3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY));

			/* Get the actual bitmap data from the icon bitmap */
			GetDIBits(hDC, infoIcon.hbmColor, 0, pInfoBitmap->bmiHeader.biHeight,
				pIconData + iOffsetColorData, pInfoBitmap, DIB_RGB_COLORS);
			if (iNumEntriesPalette > 0) {
				memcpy(pIconData + iOffsetPalette, pInfoBitmap->bmiColors,
					iNumEntriesPalette * sizeof(RGBQUAD));
			}

			/* Reset all values so that GetDIBits call succeeds */
			memset(pIconData + iOffsetMaskData, 0, iDataSize - iOffsetMaskData);
			memset(pInfoBitmap, 0, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
			pInfoBitmap->bmiHeader.biSize = sizeof(pInfoBitmap->bmiHeader);
/*
            if (!(GetDIBits(hDC, infoIcon.hbmMask, 0, 0, NULL, pInfoBitmap, DIB_RGB_COLORS)
				&& GetDIBits(hDC, infoIcon.hbmMask, 0, pIconEntry->bHeight,
					pIconData + iOffsetMaskData, pInfoBitmap, DIB_RGB_COLORS))) {

                printf("ERROR: unable to get bitmap mask (error %u)\n",
					GetLastError());

			}
*/
            GetDIBits(hDC, infoIcon.hbmMask, 0, 0, NULL, pInfoBitmap, DIB_RGB_COLORS);
            GetDIBits(hDC, infoIcon.hbmMask, 0, pIconEntry->bHeight, pIconData + iOffsetMaskData, pInfoBitmap, DIB_RGB_COLORS);

			/* Write out everything produced so far to the stream */
			*ppBuffer = pIconData; *pLength = iDataSize;
			iSuccess = 1;
		} else {
/*
			printf("ERROR: unable to get bitmap information via GetDIBits() (error %u)\n",
				GetLastError());
*/
		}
		/*
			Remarks (from MSDN entry on GetIconInfo):

			GetIconInfo creates bitmaps for the hbmMask and hbmColor
			members of ICONINFO. The calling application must manage
			these bitmaps and delete them when they are no longer
			necessary.
		 */
		if (hDC) ReleaseDC(0, hDC);
		DeleteObject(infoIcon.hbmMask);
		if (infoIcon.hbmColor) DeleteObject(infoIcon.hbmColor);
		HeapFree(GetProcessHeap(), 0, pInfoBitmap);
	} else {
		printf("ERROR: Unable to get icon information (error %u)\n",
			GetLastError());
	}
	return iSuccess;
}

static HRESULT WINAPI OLEPictureImpl_Save(
  IPersistStream* iface,IStream*pStm,BOOL fClearDirty)
{
    HRESULT hResult = E_NOTIMPL;
    void * pIconData;
    unsigned int iDataSize;
    ULONG dummy;
    int iSerializeResult = 0;
    OLEPictureImpl *This = impl_from_IPersistStream(iface);

    TRACE("%p %p %d\n", This, pStm, fClearDirty);

    switch (This->desc.picType) {
    case PICTYPE_ICON:
        if (This->bIsDirty || !This->data) {
            if (!serializeIcon(This->desc.u.icon.hicon, &pIconData, &iDataSize)) {
                ERR("(%p,%p,%d), serializeIcon() failed\n", This, pStm, fClearDirty);
                hResult = E_FAIL;
                break;
            }
            HeapFree(GetProcessHeap(), 0, This->data);
            This->data = pIconData;
            This->datalen = iDataSize;
        }
        if (This->loadtime_magic != 0xdeadbeef) {
            DWORD header[2];

            header[0] = This->loadtime_magic;
            header[1] = This->datalen;
            IStream_Write(pStm, header, 2 * sizeof(DWORD), &dummy);
        }
        IStream_Write(pStm, This->data, This->datalen, &dummy);

        hResult = S_OK;
        break;
    case PICTYPE_BITMAP:
        if (This->bIsDirty) {
            switch (This->keepOrigFormat ? This->loadtime_format : 0x4d42) {
            case 0x4d42:
                iSerializeResult = serializeBMP(This->desc.u.bmp.hbitmap, &pIconData, &iDataSize);
                break;
            case 0xd8ff:
                FIXME("(%p,%p,%d), PICTYPE_BITMAP (format JPEG) not implemented!\n",This,pStm,fClearDirty);
                break;
            case 0x4947:
                FIXME("(%p,%p,%d), PICTYPE_BITMAP (format GIF) not implemented!\n",This,pStm,fClearDirty);
                break;
            case 0x5089:
                FIXME("(%p,%p,%d), PICTYPE_BITMAP (format PNG) not implemented!\n",This,pStm,fClearDirty);
                break;
            default:
                FIXME("(%p,%p,%d), PICTYPE_BITMAP (format UNKNOWN, using BMP?) not implemented!\n",This,pStm,fClearDirty);
                break;
            }
            if (iSerializeResult) {
                /*
                if (This->loadtime_magic != 0xdeadbeef) {
                */
                if (1) {
                    DWORD header[2];

                    header[0] = (This->loadtime_magic != 0xdeadbeef) ? This->loadtime_magic : 0x0000746c;
                    header[1] = iDataSize;
                    IStream_Write(pStm, header, 2 * sizeof(DWORD), &dummy);
                }
                IStream_Write(pStm, pIconData, iDataSize, &dummy);

                HeapFree(GetProcessHeap(), 0, This->data);
                This->data = pIconData;
                This->datalen = iDataSize;
                hResult = S_OK;
            }
        } else {
            /*
            if (This->loadtime_magic != 0xdeadbeef) {
            */
            if (1) {
                DWORD header[2];

                header[0] = (This->loadtime_magic != 0xdeadbeef) ? This->loadtime_magic : 0x0000746c;
                header[1] = This->datalen;
                IStream_Write(pStm, header, 2 * sizeof(DWORD), &dummy);
            }
            IStream_Write(pStm, This->data, This->datalen, &dummy);
            hResult = S_OK;
        }
        break;
    case PICTYPE_METAFILE:
        FIXME("(%p,%p,%d), PICTYPE_METAFILE not implemented!\n",This,pStm,fClearDirty);
        break;
    case PICTYPE_ENHMETAFILE:
        FIXME("(%p,%p,%d),PICTYPE_ENHMETAFILE not implemented!\n",This,pStm,fClearDirty);
        break;
    default:
        FIXME("(%p,%p,%d), [unknown type] not implemented!\n",This,pStm,fClearDirty);
        break;
    }
    if (hResult == S_OK && fClearDirty) This->bIsDirty = FALSE;
    return hResult;
}

static HRESULT WINAPI OLEPictureImpl_GetSizeMax(
  IPersistStream* iface,ULARGE_INTEGER*pcbSize)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);
  FIXME("(%p,%p),stub!\n",This,pcbSize);
  return E_NOTIMPL;
}


/************************************************************************
 *    IDispatch
 */

/************************************************************************
 * OLEPictureImpl_IDispatch_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEPictureImpl_IDispatch_QueryInterface(
  IDispatch* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  OLEPictureImpl *This = impl_from_IDispatch(iface);

  return IPicture_QueryInterface((IPicture *)This, riid, ppvoid);
}

/************************************************************************
 * OLEPictureImpl_IDispatch_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IDispatch_AddRef(
  IDispatch* iface)
{
  OLEPictureImpl *This = impl_from_IDispatch(iface);

  return IPicture_AddRef((IPicture *)This);
}

/************************************************************************
 * OLEPictureImpl_IDispatch_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IDispatch_Release(
  IDispatch* iface)
{
  OLEPictureImpl *This = impl_from_IDispatch(iface);

  return IPicture_Release((IPicture *)This);
}

/************************************************************************
 * OLEPictureImpl_GetTypeInfoCount (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEPictureImpl_GetTypeInfoCount(
  IDispatch*    iface,
  unsigned int* pctinfo)
{
  TRACE("(%p)\n", pctinfo);

  *pctinfo = 1;

  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_GetTypeInfo (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEPictureImpl_GetTypeInfo(
  IDispatch*  iface,
  UINT      iTInfo,
  LCID        lcid,
  ITypeInfo** ppTInfo)
{
  static const WCHAR stdole2tlb[] = {'s','t','d','o','l','e','2','.','t','l','b',0};
  ITypeLib *tl;
  HRESULT hres;

  TRACE("(iTInfo=%d, lcid=%04x, %p)\n", iTInfo, (int)lcid, ppTInfo);

  if (iTInfo != 0)
    return E_FAIL;

  hres = LoadTypeLib(stdole2tlb, &tl);
  if (FAILED(hres))
  {
    ERR("Could not load stdole2.tlb\n");
    return hres;
  }

  hres = ITypeLib_GetTypeInfoOfGuid(tl, &IID_IPictureDisp, ppTInfo);
  if (FAILED(hres))
    ERR("Did not get IPictureDisp typeinfo from typelib, hres %x\n", hres);

  return hres;
}

/************************************************************************
 * OLEPictureImpl_GetIDsOfNames (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEPictureImpl_GetIDsOfNames(
  IDispatch*  iface,
  REFIID      riid,
  LPOLESTR* rgszNames,
  UINT      cNames,
  LCID        lcid,
  DISPID*     rgDispId)
{
  FIXME("():Stub\n");

  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_Invoke (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEPictureImpl_Invoke(
  IDispatch*  iface,
  DISPID      dispIdMember,
  REFIID      riid,
  LCID        lcid,
  WORD        wFlags,
  DISPPARAMS* pDispParams,
  VARIANT*    pVarResult,
  EXCEPINFO*  pExepInfo,
  UINT*     puArgErr)
{
  OLEPictureImpl *This = impl_from_IDispatch(iface);

  /* validate parameters */

  if (!IsEqualIID(riid, &IID_NULL))
  {
    ERR("riid was %s instead of IID_NULL\n", debugstr_guid(riid));
    return DISP_E_UNKNOWNNAME;
  }

  if (!pDispParams)
  {
    ERR("null pDispParams not allowed\n");
    return DISP_E_PARAMNOTOPTIONAL;
  }

  if (wFlags & DISPATCH_PROPERTYGET)
  {
    if (pDispParams->cArgs != 0)
    {
      ERR("param count for DISPATCH_PROPERTYGET was %d instead of 0\n", pDispParams->cArgs);
      return DISP_E_BADPARAMCOUNT;
    }
    if (!pVarResult)
    {
      ERR("null pVarResult not allowed when DISPATCH_PROPERTYGET specified\n");
      return DISP_E_PARAMNOTOPTIONAL;
    }
  }
  else if (wFlags & DISPATCH_PROPERTYPUT)
  {
    if (pDispParams->cArgs != 1)
    {
      ERR("param count for DISPATCH_PROPERTYPUT was %d instead of 1\n", pDispParams->cArgs);
      return DISP_E_BADPARAMCOUNT;
    }
  }

  switch (dispIdMember)
  {
  case DISPID_PICT_HANDLE:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_HANDLE\n");
      V_VT(pVarResult) = VT_I4;
      return IPicture_get_Handle((IPicture *)&This->lpVtbl, &V_UINT(pVarResult));
    }
    break;
  case DISPID_PICT_HPAL:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_HPAL\n");
      V_VT(pVarResult) = VT_I4;
      return IPicture_get_hPal((IPicture *)&This->lpVtbl, &V_UINT(pVarResult));
    }
    else if (wFlags & DISPATCH_PROPERTYPUT)
    {
      VARIANTARG vararg;
      HRESULT hr;
      TRACE("DISPID_PICT_HPAL\n");

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_I4);
      if (FAILED(hr))
        return hr;

      hr = IPicture_set_hPal((IPicture *)&This->lpVtbl, V_I4(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_PICT_TYPE:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_TYPE\n");
      V_VT(pVarResult) = VT_I2;
      return OLEPictureImpl_get_Type((IPicture *)&This->lpVtbl, &V_I2(pVarResult));
    }
    break;
  case DISPID_PICT_WIDTH:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_WIDTH\n");
      V_VT(pVarResult) = VT_I4;
      return IPicture_get_Width((IPicture *)&This->lpVtbl, &V_I4(pVarResult));
    }
    break;
  case DISPID_PICT_HEIGHT:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_HEIGHT\n");
      V_VT(pVarResult) = VT_I4;
      return IPicture_get_Height((IPicture *)&This->lpVtbl, &V_I4(pVarResult));
    }
    break;
  }

  ERR("invalid dispid 0x%x or wFlags 0x%x\n", dispIdMember, wFlags);
  return DISP_E_MEMBERNOTFOUND;
}


static const IPictureVtbl OLEPictureImpl_VTable =
{
  OLEPictureImpl_QueryInterface,
  OLEPictureImpl_AddRef,
  OLEPictureImpl_Release,
  OLEPictureImpl_get_Handle,
  OLEPictureImpl_get_hPal,
  OLEPictureImpl_get_Type,
  OLEPictureImpl_get_Width,
  OLEPictureImpl_get_Height,
  OLEPictureImpl_Render,
  OLEPictureImpl_set_hPal,
  OLEPictureImpl_get_CurDC,
  OLEPictureImpl_SelectPicture,
  OLEPictureImpl_get_KeepOriginalFormat,
  OLEPictureImpl_put_KeepOriginalFormat,
  OLEPictureImpl_PictureChanged,
  OLEPictureImpl_SaveAsFile,
  OLEPictureImpl_get_Attributes
};

static const IDispatchVtbl OLEPictureImpl_IDispatch_VTable =
{
  OLEPictureImpl_IDispatch_QueryInterface,
  OLEPictureImpl_IDispatch_AddRef,
  OLEPictureImpl_IDispatch_Release,
  OLEPictureImpl_GetTypeInfoCount,
  OLEPictureImpl_GetTypeInfo,
  OLEPictureImpl_GetIDsOfNames,
  OLEPictureImpl_Invoke
};

static const IPersistStreamVtbl OLEPictureImpl_IPersistStream_VTable =
{
  OLEPictureImpl_IPersistStream_QueryInterface,
  OLEPictureImpl_IPersistStream_AddRef,
  OLEPictureImpl_IPersistStream_Release,
  OLEPictureImpl_GetClassID,
  OLEPictureImpl_IsDirty,
  OLEPictureImpl_Load,
  OLEPictureImpl_Save,
  OLEPictureImpl_GetSizeMax
};

static const IConnectionPointContainerVtbl OLEPictureImpl_IConnectionPointContainer_VTable =
{
  OLEPictureImpl_IConnectionPointContainer_QueryInterface,
  OLEPictureImpl_IConnectionPointContainer_AddRef,
  OLEPictureImpl_IConnectionPointContainer_Release,
  OLEPictureImpl_EnumConnectionPoints,
  OLEPictureImpl_FindConnectionPoint
};

/***********************************************************************
 * OleCreatePictureIndirect (OLEAUT32.419)
 */
HRESULT WINAPI OleCreatePictureIndirect(LPPICTDESC lpPictDesc, REFIID riid,
		            BOOL fOwn, LPVOID *ppvObj )
{
  OLEPictureImpl* newPict = NULL;
  HRESULT      hr         = S_OK;

  TRACE("(%p,%s,%d,%p)\n", lpPictDesc, debugstr_guid(riid), fOwn, ppvObj);

  /*
   * Sanity check
   */
  if (ppvObj==0)
    return E_POINTER;

  *ppvObj = NULL;

  /*
   * Try to construct a new instance of the class.
   */
  newPict = OLEPictureImpl_Construct(lpPictDesc, fOwn);

  if (newPict == NULL)
    return E_OUTOFMEMORY;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IPicture_QueryInterface((IPicture*)newPict, riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IPicture_Release((IPicture*)newPict);

  return hr;
}


/***********************************************************************
 * OleLoadPicture (OLEAUT32.418)
 */
HRESULT WINAPI OleLoadPicture( LPSTREAM lpstream, LONG lSize, BOOL fRunmode,
		            REFIID riid, LPVOID *ppvObj )
{
  LPPERSISTSTREAM ps;
  IPicture	*newpic;
  HRESULT hr;

  TRACE("(%p,%d,%d,%s,%p), partially implemented.\n",
	lpstream, lSize, fRunmode, debugstr_guid(riid), ppvObj);

  hr = OleCreatePictureIndirect(NULL,riid,!fRunmode,(LPVOID*)&newpic);
  if (hr)
    return hr;
  hr = IPicture_QueryInterface(newpic,&IID_IPersistStream, (LPVOID*)&ps);
  if (hr) {
      FIXME("Could not get IPersistStream iface from Ole Picture?\n");
      IPicture_Release(newpic);
      *ppvObj = NULL;
      return hr;
  }
  IPersistStream_Load(ps,lpstream);
  IPersistStream_Release(ps);
  hr = IPicture_QueryInterface(newpic,riid,ppvObj);
  if (hr)
      FIXME("Failed to get interface %s from IPicture.\n",debugstr_guid(riid));
  IPicture_Release(newpic);
  return hr;
}

/***********************************************************************
 * OleLoadPictureEx (OLEAUT32.401)
 */
HRESULT WINAPI OleLoadPictureEx( LPSTREAM lpstream, LONG lSize, BOOL fRunmode,
		            REFIID riid, DWORD xsiz, DWORD ysiz, DWORD flags, LPVOID *ppvObj )
{
  LPPERSISTSTREAM ps;
  IPicture	*newpic;
  HRESULT hr;

  FIXME("(%p,%d,%d,%s,x=%d,y=%d,f=%x,%p), partially implemented.\n",
	lpstream, lSize, fRunmode, debugstr_guid(riid), xsiz, ysiz, flags, ppvObj);

  hr = OleCreatePictureIndirect(NULL,riid,!fRunmode,(LPVOID*)&newpic);
  if (hr)
    return hr;
  hr = IPicture_QueryInterface(newpic,&IID_IPersistStream, (LPVOID*)&ps);
  if (hr) {
      FIXME("Could not get IPersistStream iface from Ole Picture?\n");
      IPicture_Release(newpic);
      *ppvObj = NULL;
      return hr;
  }
  IPersistStream_Load(ps,lpstream);
  IPersistStream_Release(ps);
  hr = IPicture_QueryInterface(newpic,riid,ppvObj);
  if (hr)
      FIXME("Failed to get interface %s from IPicture.\n",debugstr_guid(riid));
  IPicture_Release(newpic);
  return hr;
}

/***********************************************************************
 * OleLoadPicturePath (OLEAUT32.424)
 */
HRESULT WINAPI OleLoadPicturePath( LPOLESTR szURLorPath, LPUNKNOWN punkCaller,
		DWORD dwReserved, OLE_COLOR clrReserved, REFIID riid,
		LPVOID *ppvRet )
{
  static const WCHAR file[] = { 'f','i','l','e',':','/','/',0 };
  IPicture *ipicture;
  HANDLE hFile;
  DWORD dwFileSize;
  HGLOBAL hGlobal = NULL;
  DWORD dwBytesRead = 0;
  IStream *stream;
  BOOL bRead;
  IPersistStream *pStream;
  HRESULT hRes;

  TRACE("(%s,%p,%d,%08x,%s,%p): stub\n",
        debugstr_w(szURLorPath), punkCaller, dwReserved, clrReserved,
        debugstr_guid(riid), ppvRet);

  if (!ppvRet) return E_POINTER;

  if (strncmpW(szURLorPath, file, 7) == 0) {
      szURLorPath += 7;

      hFile = CreateFileW(szURLorPath, GENERIC_READ, 0, NULL, OPEN_EXISTING,
				   0, NULL);
      if (hFile == INVALID_HANDLE_VALUE)
	  return E_UNEXPECTED;

      dwFileSize = GetFileSize(hFile, NULL);
      if (dwFileSize != INVALID_FILE_SIZE )
      {
	  hGlobal = GlobalAlloc(GMEM_FIXED,dwFileSize);
	  if ( hGlobal)
	  {
	      bRead = ReadFile(hFile, hGlobal, dwFileSize, &dwBytesRead, NULL);
	      if (!bRead)
	      {
		  GlobalFree(hGlobal);
		  hGlobal = 0;
	      }
	  }
      }
      CloseHandle(hFile);

      if (!hGlobal)
	  return E_UNEXPECTED;

      hRes = CreateStreamOnHGlobal(hGlobal, TRUE, &stream);
      if (FAILED(hRes))
      {
	  GlobalFree(hGlobal);
	  return hRes;
      }
  } else {
      IMoniker *pmnk;
      IBindCtx *pbc;

      hRes = CreateBindCtx(0, &pbc);
      if (SUCCEEDED(hRes))
      {
	  hRes = CreateURLMoniker(NULL, szURLorPath, &pmnk);
	  if (SUCCEEDED(hRes))
	  {
	      hRes = IMoniker_BindToStorage(pmnk, pbc, NULL, &IID_IStream, (LPVOID*)&stream);
	      IMoniker_Release(pmnk);
	  }
	  IBindCtx_Release(pbc);
      }
      if (FAILED(hRes))
	  return hRes;
  }

  hRes = CoCreateInstance(&CLSID_StdPicture, punkCaller, CLSCTX_INPROC_SERVER,
		   &IID_IPicture, (LPVOID*)&ipicture);
  if (hRes != S_OK) {
      IStream_Release(stream);
      return hRes;
  }

  hRes = IPicture_QueryInterface(ipicture, &IID_IPersistStream, (LPVOID*)&pStream);
  if (hRes) {
      IStream_Release(stream);
      IPicture_Release(ipicture);
      return hRes;
  }

  hRes = IPersistStream_Load(pStream, stream);
  IPersistStream_Release(pStream);
  IStream_Release(stream);

  if (hRes) {
      IPicture_Release(ipicture);
      return hRes;
  }

  hRes = IPicture_QueryInterface(ipicture,riid,ppvRet);
  if (hRes)
      FIXME("Failed to get interface %s from IPicture.\n",debugstr_guid(riid));

  IPicture_Release(ipicture);
  return hRes;
}

/*******************************************************************************
 * StdPic ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl    *lpVtbl;
    LONG                        ref;
} IClassFactoryImpl;

static HRESULT WINAPI
SPCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
SPCF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SPCF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	/* static class, won't be  freed */
	return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI SPCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
    /* Creates an uninitialized picture */
    return OleCreatePictureIndirect(NULL,riid,TRUE,ppobj);

}

static HRESULT WINAPI SPCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static const IClassFactoryVtbl SPCF_Vtbl = {
	SPCF_QueryInterface,
	SPCF_AddRef,
	SPCF_Release,
	SPCF_CreateInstance,
	SPCF_LockServer
};
static IClassFactoryImpl STDPIC_CF = {&SPCF_Vtbl, 1 };

void _get_STDPIC_CF(LPVOID *ppv) { *ppv = (LPVOID)&STDPIC_CF; }
