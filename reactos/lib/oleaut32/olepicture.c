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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Must be before wine includes, the header has things conflicting with
 * WINE headers.
 */
#ifdef HAVE_GIF_LIB_H
# include <gif_lib.h>
#endif

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
#include "wine/debug.h"

#include "wine/wingdi16.h"
#include "cursoricon.h"

#ifdef HAVE_LIBJPEG
/* This is a hack, so jpeglib.h does not redefine INT32 and the like*/
#define XMD_H
#define UINT8 JPEG_UINT8
#define UINT16 JPEG_UINT16
#undef FAR
#ifdef HAVE_JPEGLIB_H
# include <jpeglib.h>
#endif
#undef UINT16
#endif

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/*************************************************************************
 *  Declaration of implementation class
 */

typedef struct OLEPictureImpl {

  /*
   * IPicture handles IUnknown
   */

    ICOM_VTABLE(IPicture)       *lpvtbl1;
    ICOM_VTABLE(IDispatch)      *lpvtbl2;
    ICOM_VTABLE(IPersistStream) *lpvtbl3;
    ICOM_VTABLE(IConnectionPointContainer) *lpvtbl4;

  /* Object referenece count */
    DWORD ref;

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

  /* data */
    void* data;
    int datalen;
} OLEPictureImpl;

/*
 * Macros to retrieve pointer to IUnknown (IPicture) from the other VTables.
 */
#define ICOM_THIS_From_IDispatch(impl, name) \
    impl *This = (impl*)(((char*)name)-sizeof(void*));
#define ICOM_THIS_From_IPersistStream(impl, name) \
    impl *This = (impl*)(((char*)name)-2*sizeof(void*));
#define ICOM_THIS_From_IConnectionPointContainer(impl, name) \
    impl *This = (impl*)(((char*)name)-3*sizeof(void*));

/*
 * Predeclare VTables.  They get initialized at the end.
 */
static ICOM_VTABLE(IPicture) OLEPictureImpl_VTable;
static ICOM_VTABLE(IDispatch) OLEPictureImpl_IDispatch_VTable;
static ICOM_VTABLE(IPersistStream) OLEPictureImpl_IPersistStream_VTable;
static ICOM_VTABLE(IConnectionPointContainer) OLEPictureImpl_IConnectionPointContainer_VTable;

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
  newObject->lpvtbl1 = &OLEPictureImpl_VTable;
  newObject->lpvtbl2 = &OLEPictureImpl_IDispatch_VTable;
  newObject->lpvtbl3 = &OLEPictureImpl_IPersistStream_VTable;
  newObject->lpvtbl4 = &OLEPictureImpl_IConnectionPointContainer_VTable;

  CreateConnectionPoint((IUnknown*)newObject,&IID_IPropertyNotifySink,&newObject->pCP);

  /*
   * Start with one reference count. The caller of this function
   * must release the interface pointer when it is done.
   */
  newObject->ref	= 1;
  newObject->hDCCur	= 0;

  newObject->fOwn	= fOwn;

  /* dunno about original value */
  newObject->keepOrigFormat = TRUE;

  if (pictDesc) {
      if(pictDesc->cbSizeofstruct != sizeof(PICTDESC)) {
	  FIXME("struct size = %d\n", pictDesc->cbSizeofstruct);
      }
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

  if(Obj->fOwn) { /* We need to destroy the picture */
    switch(Obj->desc.picType) {
    case PICTYPE_BITMAP:
      DeleteObject(Obj->desc.u.bmp.hbitmap);
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
    default:
      FIXME("Unsupported type %d - unable to delete\n", Obj->desc.picType);
      break;
    }
  }
  if (Obj->data) HeapFree(GetProcessHeap(), 0, Obj->data);
  HeapFree(GetProcessHeap(), 0, Obj);
}

static ULONG WINAPI OLEPictureImpl_AddRef(IPicture* iface);

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
  ICOM_THIS(OLEPictureImpl, iface);
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
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
  {
    *ppvObject = (IPicture*)This;
  }
  else if (memcmp(&IID_IPicture, riid, sizeof(IID_IPicture)) == 0)
  {
    *ppvObject = (IPicture*)This;
  }
  else if (memcmp(&IID_IDispatch, riid, sizeof(IID_IDispatch)) == 0)
  {
    *ppvObject = (IDispatch*)&(This->lpvtbl2);
  }
  else if (memcmp(&IID_IPictureDisp, riid, sizeof(IID_IPictureDisp)) == 0)
  {
    *ppvObject = (IDispatch*)&(This->lpvtbl2);
  }
  else if (memcmp(&IID_IPersistStream, riid, sizeof(IID_IPersistStream)) == 0)
  {
  *ppvObject = (IPersistStream*)&(This->lpvtbl3);
  }
  else if (memcmp(&IID_IConnectionPointContainer, riid, sizeof(IID_IConnectionPointContainer)) == 0)
  {
  *ppvObject = (IConnectionPointContainer*)&(This->lpvtbl4);
  }
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
 * OLEPictureImpl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_AddRef(
  IPicture* iface)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);
  This->ref++;

  return This->ref;
}

/************************************************************************
 * OLEPictureImpl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_Release(
      IPicture* iface)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);

  /*
   * Decrease the reference count on this object.
   */
  This->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (This->ref==0)
  {
    OLEPictureImpl_Destroy(This);

    return 0;
  }

  return This->ref;
}


/************************************************************************
 * OLEPictureImpl_get_Handle
 */
static HRESULT WINAPI OLEPictureImpl_get_Handle(IPicture *iface,
						OLE_HANDLE *phandle)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p)\n", This, phandle);
  switch(This->desc.picType) {
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
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%p): stub\n", This, phandle);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_get_Type
 */
static HRESULT WINAPI OLEPictureImpl_get_Type(IPicture *iface,
					      short *ptype)
{
  ICOM_THIS(OLEPictureImpl, iface);
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
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p): width is %ld\n", This, pwidth, This->himetricWidth);
  *pwidth = This->himetricWidth;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_get_Height
 */
static HRESULT WINAPI OLEPictureImpl_get_Height(IPicture *iface,
						OLE_YSIZE_HIMETRIC *pheight)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p): height is %ld\n", This, pheight, This->himetricHeight);
  *pheight = This->himetricHeight;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_Render
 */
static HRESULT WINAPI OLEPictureImpl_Render(IPicture *iface, HDC hdc,
					    long x, long y, long cx, long cy,
					    OLE_XPOS_HIMETRIC xSrc,
					    OLE_YPOS_HIMETRIC ySrc,
					    OLE_XSIZE_HIMETRIC cxSrc,
					    OLE_YSIZE_HIMETRIC cySrc,
					    LPCRECT prcWBounds)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p, (%ld,%ld), (%ld,%ld) <- (%ld,%ld), (%ld,%ld), %p)\n",
	This, hdc, x, y, cx, cy, xSrc, ySrc, cxSrc, cySrc, prcWBounds);
  if(prcWBounds)
    TRACE("prcWBounds (%ld,%ld) - (%ld,%ld)\n", prcWBounds->left, prcWBounds->top,
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

      hbmpOld = SelectObject(hdcBmp, This->desc.u.bmp.hbitmap);

      StretchBlt(hdc, x, y, cx, cy, hdcBmp, xSrc, ySrc, cxSrc, cySrc, SRCCOPY);

      SelectObject(hdcBmp, hbmpOld);
      DeleteDC(hdcBmp);
    }
    break;
  case PICTYPE_ICON:
    FIXME("Not quite correct implementation of rendering icons...\n");
    DrawIcon(hdc,x,y,This->desc.u.icon.hicon);
    break;

  case PICTYPE_METAFILE:
  case PICTYPE_ENHMETAFILE:
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
  ICOM_THIS(OLEPictureImpl, iface);
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
  ICOM_THIS(OLEPictureImpl, iface);
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
  ICOM_THIS(OLEPictureImpl, iface);
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
  ICOM_THIS(OLEPictureImpl, iface);
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
  ICOM_THIS(OLEPictureImpl, iface);
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
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->()\n", This);
  OLEPicture_SendNotify(This,DISPID_PICT_HANDLE);
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
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%p, %d, %p), hacked stub.\n", This, pstream, SaveMemCopy, pcbSize);
  return IStream_Write(pstream,This->data,This->datalen,(ULONG*)pcbSize);
}

/************************************************************************
 * OLEPictureImpl_get_Attributes
 */
static HRESULT WINAPI OLEPictureImpl_get_Attributes(IPicture *iface,
						    DWORD *pdwAttr)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p).\n", This, pdwAttr);
  *pdwAttr = 0;
  switch (This->desc.picType) {
  case PICTYPE_BITMAP: 	break;	/* not 'truely' scalable, see MSDN. */
  case PICTYPE_ICON: *pdwAttr     = PICTURE_TRANSPARENT;break;
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
  VOID** ppvoid
) {
  ICOM_THIS_From_IConnectionPointContainer(IPicture,iface);

  return IPicture_QueryInterface(This,riid,ppvoid);
}

static ULONG WINAPI OLEPictureImpl_IConnectionPointContainer_AddRef(
  IConnectionPointContainer* iface)
{
  ICOM_THIS_From_IConnectionPointContainer(IPicture, iface);

  return IPicture_AddRef(This);
}

static ULONG WINAPI OLEPictureImpl_IConnectionPointContainer_Release(
  IConnectionPointContainer* iface)
{
  ICOM_THIS_From_IConnectionPointContainer(IPicture, iface);

  return IPicture_Release(This);
}

static HRESULT WINAPI OLEPictureImpl_EnumConnectionPoints(
  IConnectionPointContainer* iface,
  IEnumConnectionPoints** ppEnum
) {
  ICOM_THIS_From_IConnectionPointContainer(IPicture, iface);

  FIXME("(%p,%p), stub!\n",This,ppEnum);
  return E_NOTIMPL;
}

static HRESULT WINAPI OLEPictureImpl_FindConnectionPoint(
  IConnectionPointContainer* iface,
  REFIID riid,
  IConnectionPoint **ppCP
) {
  ICOM_THIS_From_IConnectionPointContainer(OLEPictureImpl, iface);
  TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppCP);
  if (!ppCP)
      return E_POINTER;
  *ppCP = NULL;
  if (IsEqualGUID(riid,&IID_IPropertyNotifySink))
      return IConnectionPoint_QueryInterface(This->pCP,&IID_IConnectionPoint,(LPVOID)ppCP);
  FIXME("tried to find connection point on %s?\n",debugstr_guid(riid));
  return 0x80040200;
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
  ICOM_THIS_From_IPersistStream(IPicture, iface);

  return IPicture_QueryInterface(This, riid, ppvoid);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IPersistStream_AddRef(
  IPersistStream* iface)
{
  ICOM_THIS_From_IPersistStream(IPicture, iface);

  return IPicture_AddRef(This);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IPersistStream_Release(
  IPersistStream* iface)
{
  ICOM_THIS_From_IPersistStream(IPicture, iface);

  return IPicture_Release(This);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_GetClassID
 */
static HRESULT WINAPI OLEPictureImpl_GetClassID(
  IPersistStream* iface,CLSID* pClassID)
{
  ICOM_THIS_From_IPersistStream(IPicture, iface);
  FIXME("(%p),stub!\n",This);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_IsDirty
 */
static HRESULT WINAPI OLEPictureImpl_IsDirty(
  IPersistStream* iface)
{
  ICOM_THIS_From_IPersistStream(IPicture, iface);
  FIXME("(%p),stub!\n",This);
  return E_NOTIMPL;
}

#ifdef HAVE_LIBJPEG
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
#endif /* HAVE_LIBJPEG */

#ifdef HAVE_LIBGIF
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
#endif

/************************************************************************
 * OLEPictureImpl_IPersistStream_Load (IUnknown)
 *
 * Loads the binary data from the IStream. Starts at current position.
 * There appears to be an 2 DWORD header:
 * 	DWORD magic;
 * 	DWORD len;
 *
 * Currently implemented: BITMAP, ICON, JPEG.
 */
static HRESULT WINAPI OLEPictureImpl_Load(IPersistStream* iface,IStream*pStm) {
  HRESULT	hr = E_FAIL;
  ULONG		xread;
  BYTE 		*xbuf;
  DWORD		header[2];
  WORD		magic;
  STATSTG       statstg;
  ICOM_THIS_From_IPersistStream(OLEPictureImpl, iface);
  
  TRACE("(%p,%p)\n",This,pStm);

  /* Sometimes we have a header, sometimes we don't. Apply some guesses to find
   * out whether we do.
   */
  hr=IStream_Stat(pStm,&statstg,STATFLAG_NONAME);
  if (hr)
    FIXME("Stat failed with hres %lx\n",hr);
  hr=IStream_Read(pStm,header,8,&xread);
  if (hr || xread!=8) {
      FIXME("Failure while reading picture header (hr is %lx, nread is %ld).\n",hr,xread);
      return hr;
  }
  if (header[1] > statstg.cbSize.QuadPart || (header[1]==0)) {/* Incorrect header, assume none. */
    xread = 8;
    xbuf = This->data = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,statstg.cbSize.QuadPart);
    memcpy(xbuf,&header,8);
    This->datalen = statstg.cbSize.QuadPart;
    while (xread < This->datalen) {
      ULONG nread;
      hr = IStream_Read(pStm,xbuf+xread,This->datalen-xread,&nread);
      xread+=nread;
      if (hr || !nread)
	break;
    }
    if (xread != This->datalen)
      FIXME("Could only read %ld of %d bytes in no-header case?\n",xread,This->datalen);
  } else {
    xread = 0;
    xbuf = This->data = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,header[1]);
    This->datalen = header[1];
    while (xread < header[1]) {
      ULONG nread;
      hr = IStream_Read(pStm,xbuf+xread,header[1]-xread,&nread);
      xread+=nread;
      if (hr || !nread)
	break;
    }
    if (xread != header[1])
      FIXME("Could only read %ld of %ld bytes?\n",xread,header[1]);
  }
  magic = xbuf[0] + (xbuf[1]<<8);
  switch (magic) {
  case 0x4947: { /* GIF */
#ifdef HAVE_LIBGIF
    struct gifdata 	gd;
    GifFileType 	*gif;
    BITMAPINFO		*bmi;
    HDC			hdcref;
    LPBYTE              bytes;
    int                 i,j,ret;
    GifImageDesc        *gid;
    SavedImage          *si;
    ColorMapObject      *cm;
    
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
    bmi  = HeapAlloc(GetProcessHeap(),0,sizeof(BITMAPINFOHEADER)+(1<<gif->SColorResolution)*sizeof(RGBQUAD));
    bytes= HeapAlloc(GetProcessHeap(),0,gif->SWidth*gif->SHeight);
    si   = gif->SavedImages+0;
    gid  = &(si->ImageDesc);
    cm   = gid->ColorMap;
    if (!cm) cm = gif->SColorMap;
    for (i=0;i<(1<<gif->SColorResolution);i++) {
      bmi->bmiColors[i].rgbRed = cm->Colors[i].Red;
      bmi->bmiColors[i].rgbGreen = cm->Colors[i].Green;
      bmi->bmiColors[i].rgbBlue = cm->Colors[i].Blue;
    }
    /* Map to in picture coordinates */
    for (i=0;i<gid->Height;i++)
      for (j=0;j<gid->Width;j++)
        bytes[(gid->Top+i)*gif->SWidth+gid->Left+j]=si->RasterBits[i*gid->Width+j];
    bmi->bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth		= gif->SWidth;
    bmi->bmiHeader.biHeight		= gif->SHeight;
    bmi->bmiHeader.biPlanes		= 1;
    bmi->bmiHeader.biBitCount		= 8;
    bmi->bmiHeader.biCompression	= BI_RGB;
    bmi->bmiHeader.biSizeImage		= gif->SWidth*gif->SHeight;
    bmi->bmiHeader.biXPelsPerMeter	= 0;
    bmi->bmiHeader.biYPelsPerMeter	= 0;
    bmi->bmiHeader.biClrUsed		= 1 << gif->SColorResolution;
    bmi->bmiHeader.biClrImportant	= 0;

    hdcref = GetDC(0);
    This->desc.u.bmp.hbitmap=CreateDIBitmap(
	    hdcref,
	    &bmi->bmiHeader,
	    CBM_INIT,
	    bytes,
	    bmi,
	    DIB_PAL_COLORS
    );
    DeleteDC(hdcref);
    This->desc.picType = PICTYPE_BITMAP;
    OLEPictureImpl_SetBitmap(This);
    DGifCloseFile(gif);
    HeapFree(GetProcessHeap(),0,bytes);
    return S_OK;
#else
    FIXME("Trying to load GIF, but no support for libgif/libungif compiled in.\n");
    return E_FAIL;
#endif
    break;
  }
  case 0xd8ff: { /* JPEG */
#ifdef HAVE_LIBJPEG
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
    int i;

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

    jd.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&jd);
    jd.src = &xjsm;
    ret=jpeg_read_header(&jd,TRUE);
    jd.out_color_space = JCS_RGB;
    jpeg_start_decompress(&jd);
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
      x = jpeg_read_scanlines(&jd,&samprow,1);
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
    jpeg_finish_decompress(&jd);
    jpeg_destroy_decompress(&jd);
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
    hr = S_OK;
    HeapFree(GetProcessHeap(),0,bits);
#else
    ERR("Trying to load JPEG picture, but JPEG supported not compiled in.\n");
    hr = E_FAIL;
#endif
    break;
  }
  case 0x4d42: { /* Bitmap */
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
	(bi->bmiHeader.biBitCount<=8)?DIB_PAL_COLORS:DIB_RGB_COLORS
    );
    DeleteDC(hdcref);
    This->desc.picType = PICTYPE_BITMAP;
    OLEPictureImpl_SetBitmap(This);
    hr = S_OK;
    break;
  }
  case 0x0000: { /* ICON , first word is dwReserved */
    HICON hicon;
    CURSORICONFILEDIR	*cifd = (CURSORICONFILEDIR*)xbuf;
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
	hr = E_FAIL;
    } else {
	This->desc.picType = PICTYPE_ICON;
	This->desc.u.icon.hicon = hicon;
	This->himetricWidth = cifd->idEntries[i].bWidth;
	This->himetricHeight = cifd->idEntries[i].bHeight;
	hr = S_OK;
    }
    break;
  }
  default:
  {
    int i;
    FIXME("Unknown magic %04x, %ld read bytes:\n",magic,xread);
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

  /* FIXME: this notify is not really documented */
  if (hr==S_OK)
      OLEPicture_SendNotify(This,DISPID_PICT_TYPE);
  return hr;
}

static HRESULT WINAPI OLEPictureImpl_Save(
  IPersistStream* iface,IStream*pStm,BOOL fClearDirty)
{
  ICOM_THIS_From_IPersistStream(IPicture, iface);
  FIXME("(%p,%p,%d),stub!\n",This,pStm,fClearDirty);
  return E_NOTIMPL;
}

static HRESULT WINAPI OLEPictureImpl_GetSizeMax(
  IPersistStream* iface,ULARGE_INTEGER*pcbSize)
{
  ICOM_THIS_From_IPersistStream(IPicture, iface);
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
  ICOM_THIS_From_IDispatch(IPicture, iface);

  return IPicture_QueryInterface(This, riid, ppvoid);
}

/************************************************************************
 * OLEPictureImpl_IDispatch_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IDispatch_AddRef(
  IDispatch* iface)
{
  ICOM_THIS_From_IDispatch(IPicture, iface);

  return IPicture_AddRef(This);
}

/************************************************************************
 * OLEPictureImpl_IDispatch_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IDispatch_Release(
  IDispatch* iface)
{
  ICOM_THIS_From_IDispatch(IPicture, iface);

  return IPicture_Release(This);
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
  FIXME("():Stub\n");

  return E_NOTIMPL;
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
  FIXME("():Stub\n");

  return E_NOTIMPL;
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
  FIXME("(dispid: %ld):Stub\n",dispIdMember);

  VariantInit(pVarResult);
  V_VT(pVarResult) = VT_BOOL;
  V_UNION(pVarResult,boolVal) = FALSE;
  return S_OK;
}


static ICOM_VTABLE(IPicture) OLEPictureImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

static ICOM_VTABLE(IDispatch) OLEPictureImpl_IDispatch_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEPictureImpl_IDispatch_QueryInterface,
  OLEPictureImpl_IDispatch_AddRef,
  OLEPictureImpl_IDispatch_Release,
  OLEPictureImpl_GetTypeInfoCount,
  OLEPictureImpl_GetTypeInfo,
  OLEPictureImpl_GetIDsOfNames,
  OLEPictureImpl_Invoke
};

static ICOM_VTABLE(IPersistStream) OLEPictureImpl_IPersistStream_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEPictureImpl_IPersistStream_QueryInterface,
  OLEPictureImpl_IPersistStream_AddRef,
  OLEPictureImpl_IPersistStream_Release,
  OLEPictureImpl_GetClassID,
  OLEPictureImpl_IsDirty,
  OLEPictureImpl_Load,
  OLEPictureImpl_Save,
  OLEPictureImpl_GetSizeMax
};

static ICOM_VTABLE(IConnectionPointContainer) OLEPictureImpl_IConnectionPointContainer_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

  TRACE("(%p,%p,%d,%p)\n", lpPictDesc, riid, fOwn, ppvObj);

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

  TRACE("(%p,%ld,%d,%s,%p), partially implemented.\n",
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

  FIXME("(%p,%ld,%d,%s,x=%ld,y=%ld,f=%lx,%p), partially implemented.\n",
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

/*******************************************************************************
 * StdPic ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI
SPCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
SPCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI SPCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI SPCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	PICTDESC	pd;

	FIXME("(%p,%p,%s,%p), creating stdpic with PICTYPE_NONE.\n",iface,pOuter,debugstr_guid(riid),ppobj);
	pd.cbSizeofstruct = sizeof(pd);
	pd.picType = PICTYPE_NONE;
	return OleCreatePictureIndirect(&pd,riid,TRUE,ppobj);

}

static HRESULT WINAPI SPCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) SPCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	SPCF_QueryInterface,
	SPCF_AddRef,
	SPCF_Release,
	SPCF_CreateInstance,
	SPCF_LockServer
};
static IClassFactoryImpl STDPIC_CF = {&SPCF_Vtbl, 1 };

void _get_STDPIC_CF(LPVOID *ppv) { *ppv = (LPVOID)&STDPIC_CF; }
