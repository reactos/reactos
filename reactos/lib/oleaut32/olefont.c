/*
 * OLE Font encapsulation implementation
 *
 * This file contains an implementation of the IFont
 * interface and the OleCreateFontIndirect API call.
 *
 * Copyright 1999 Francis Beaudet
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
 */
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "oleauto.h"    /* for SysAllocString(....) */
#include "objbase.h"
#include "ole2.h"
#include "olectl.h"
#include "wine/debug.h"
#include "connpt.h" /* for CreateConnectionPoint */

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 * Declaration of constants used when serializing the font object.
 */
#define FONTPERSIST_ITALIC        0x02
#define FONTPERSIST_UNDERLINE     0x04
#define FONTPERSIST_STRIKETHROUGH 0x08

/***********************************************************************
 * Declaration of the implementation class for the IFont interface
 */
typedef struct OLEFontImpl OLEFontImpl;

struct OLEFontImpl
{
  /*
   * This class supports many interfaces. IUnknown, IFont,
   * IDispatch, IDispFont IPersistStream and IConnectionPointContainer.
   * The first two are supported by the first vtable, the next two are
   * supported by the second table and the last two have their own.
   */
  ICOM_VTABLE(IFont)*				lpvtbl1;
  ICOM_VTABLE(IDispatch)*			lpvtbl2;
  ICOM_VTABLE(IPersistStream)*			lpvtbl3;
  ICOM_VTABLE(IConnectionPointContainer)*	lpvtbl4;
  ICOM_VTABLE(IPersistPropertyBag)*		lpvtbl5;
  ICOM_VTABLE(IPersistStreamInit)*		lpvtbl6;
  /*
   * Reference count for that instance of the class.
   */
  ULONG ref;

  /*
   * This structure contains the description of the class.
   */
  FONTDESC description;

  /*
   * Contain the font associated with this object.
   */
  HFONT gdiFont;

  /*
   * Font lock count.
   */
  DWORD fontLock;

  /*
   * Size ratio
   */
  long cyLogical;
  long cyHimetric;

  IConnectionPoint *pCP;
};

/*
 * Here, I define utility macros to help with the casting of the
 * "this" parameter.
 * There is a version to accomodate all of the VTables implemented
 * by this object.
 */
#define _ICOM_THIS(class,name) class* this = (class*)name
#define _ICOM_THIS_From_IDispatch(class, name) class* this = (class*)(((char*)name)-sizeof(void*))
#define _ICOM_THIS_From_IPersistStream(class, name) class* this = (class*)(((char*)name)-2*sizeof(void*))
#define _ICOM_THIS_From_IConnectionPointContainer(class, name) class* this = (class*)(((char*)name)-3*sizeof(void*))
#define _ICOM_THIS_From_IPersistPropertyBag(class, name) class* this = (class*)(((char*)name)-4*sizeof(void*))
#define _ICOM_THIS_From_IPersistStreamInit(class, name) class* this = (class*)(((char*)name)-5*sizeof(void*))


/***********************************************************************
 * Prototypes for the implementation functions for the IFont
 * interface
 */
static OLEFontImpl* OLEFontImpl_Construct(LPFONTDESC fontDesc);
static void         OLEFontImpl_Destroy(OLEFontImpl* fontDesc);
static HRESULT      WINAPI OLEFontImpl_QueryInterface(IFont* iface, REFIID riid, VOID** ppvoid);
static ULONG        WINAPI OLEFontImpl_AddRef(IFont* iface);
static ULONG        WINAPI OLEFontImpl_Release(IFont* iface);
static HRESULT      WINAPI OLEFontImpl_get_Name(IFont* iface, BSTR* pname);
static HRESULT      WINAPI OLEFontImpl_put_Name(IFont* iface, BSTR name);
static HRESULT      WINAPI OLEFontImpl_get_Size(IFont* iface, CY* psize);
static HRESULT      WINAPI OLEFontImpl_put_Size(IFont* iface, CY size);
static HRESULT      WINAPI OLEFontImpl_get_Bold(IFont* iface, BOOL* pbold);
static HRESULT      WINAPI OLEFontImpl_put_Bold(IFont* iface, BOOL bold);
static HRESULT      WINAPI OLEFontImpl_get_Italic(IFont* iface, BOOL* pitalic);
static HRESULT      WINAPI OLEFontImpl_put_Italic(IFont* iface, BOOL italic);
static HRESULT      WINAPI OLEFontImpl_get_Underline(IFont* iface, BOOL* punderline);
static HRESULT      WINAPI OLEFontImpl_put_Underline(IFont* iface, BOOL underline);
static HRESULT      WINAPI OLEFontImpl_get_Strikethrough(IFont* iface, BOOL* pstrikethrough);
static HRESULT      WINAPI OLEFontImpl_put_Strikethrough(IFont* iface, BOOL strikethrough);
static HRESULT      WINAPI OLEFontImpl_get_Weight(IFont* iface, short* pweight);
static HRESULT      WINAPI OLEFontImpl_put_Weight(IFont* iface, short weight);
static HRESULT      WINAPI OLEFontImpl_get_Charset(IFont* iface, short* pcharset);
static HRESULT      WINAPI OLEFontImpl_put_Charset(IFont* iface, short charset);
static HRESULT      WINAPI OLEFontImpl_get_hFont(IFont* iface, HFONT* phfont);
static HRESULT      WINAPI OLEFontImpl_Clone(IFont* iface, IFont** ppfont);
static HRESULT      WINAPI OLEFontImpl_IsEqual(IFont* iface, IFont* pFontOther);
static HRESULT      WINAPI OLEFontImpl_SetRatio(IFont* iface, long cyLogical, long cyHimetric);
static HRESULT      WINAPI OLEFontImpl_QueryTextMetrics(IFont* iface, TEXTMETRICOLE* ptm);
static HRESULT      WINAPI OLEFontImpl_AddRefHfont(IFont* iface, HFONT hfont);
static HRESULT      WINAPI OLEFontImpl_ReleaseHfont(IFont* iface, HFONT hfont);
static HRESULT      WINAPI OLEFontImpl_SetHdc(IFont* iface, HDC hdc);

/***********************************************************************
 * Prototypes for the implementation functions for the IDispatch
 * interface
 */
static HRESULT WINAPI OLEFontImpl_IDispatch_QueryInterface(IDispatch* iface,
						    REFIID     riid,
						    VOID**     ppvoid);
static ULONG   WINAPI OLEFontImpl_IDispatch_AddRef(IDispatch* iface);
static ULONG   WINAPI OLEFontImpl_IDispatch_Release(IDispatch* iface);
static HRESULT WINAPI OLEFontImpl_GetTypeInfoCount(IDispatch*    iface,
					           unsigned int* pctinfo);
static HRESULT WINAPI OLEFontImpl_GetTypeInfo(IDispatch*  iface,
				       	      UINT      iTInfo,
				              LCID        lcid,
				              ITypeInfo** ppTInfo);
static HRESULT WINAPI OLEFontImpl_GetIDsOfNames(IDispatch*  iface,
					        REFIID      riid,
					        LPOLESTR* rgszNames,
					        UINT      cNames,
					        LCID        lcid,
					        DISPID*     rgDispId);
static HRESULT WINAPI OLEFontImpl_Invoke(IDispatch*  iface,
				         DISPID      dispIdMember,
				         REFIID      riid,
				         LCID        lcid,
				         WORD        wFlags,
				         DISPPARAMS* pDispParams,
				         VARIANT*    pVarResult,
				         EXCEPINFO*  pExepInfo,
				         UINT*     puArgErr);

/***********************************************************************
 * Prototypes for the implementation functions for the IPersistStream
 * interface
 */
static HRESULT WINAPI OLEFontImpl_IPersistStream_QueryInterface(IPersistStream* iface,
						    REFIID     riid,
						    VOID**     ppvoid);
static ULONG   WINAPI OLEFontImpl_IPersistStream_AddRef(IPersistStream* iface);
static ULONG   WINAPI OLEFontImpl_IPersistStream_Release(IPersistStream* iface);
static HRESULT WINAPI OLEFontImpl_GetClassID(IPersistStream* iface,
					     CLSID*                pClassID);
static HRESULT WINAPI OLEFontImpl_IsDirty(IPersistStream*  iface);
static HRESULT WINAPI OLEFontImpl_Load(IPersistStream*  iface,
				       IStream*         pLoadStream);
static HRESULT WINAPI OLEFontImpl_Save(IPersistStream*  iface,
				       IStream*         pOutStream,
				       BOOL             fClearDirty);
static HRESULT WINAPI OLEFontImpl_GetSizeMax(IPersistStream*  iface,
					     ULARGE_INTEGER*  pcbSize);

/***********************************************************************
 * Prototypes for the implementation functions for the
 * IConnectionPointContainer interface
 */
static HRESULT WINAPI OLEFontImpl_IConnectionPointContainer_QueryInterface(
					    IConnectionPointContainer* iface,
					    REFIID     riid,
					    VOID**     ppvoid);
static ULONG   WINAPI OLEFontImpl_IConnectionPointContainer_AddRef(
					    IConnectionPointContainer* iface);
static ULONG   WINAPI OLEFontImpl_IConnectionPointContainer_Release(
					    IConnectionPointContainer* iface);
static HRESULT WINAPI OLEFontImpl_EnumConnectionPoints(
					    IConnectionPointContainer* iface,
					    IEnumConnectionPoints **ppEnum);
static HRESULT WINAPI OLEFontImpl_FindConnectionPoint(
					    IConnectionPointContainer* iface,
					    REFIID riid,
					    IConnectionPoint **ppCp);

/*
 * Virtual function tables for the OLEFontImpl class.
 */
static ICOM_VTABLE(IFont) OLEFontImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEFontImpl_QueryInterface,
  OLEFontImpl_AddRef,
  OLEFontImpl_Release,
  OLEFontImpl_get_Name,
  OLEFontImpl_put_Name,
  OLEFontImpl_get_Size,
  OLEFontImpl_put_Size,
  OLEFontImpl_get_Bold,
  OLEFontImpl_put_Bold,
  OLEFontImpl_get_Italic,
  OLEFontImpl_put_Italic,
  OLEFontImpl_get_Underline,
  OLEFontImpl_put_Underline,
  OLEFontImpl_get_Strikethrough,
  OLEFontImpl_put_Strikethrough,
  OLEFontImpl_get_Weight,
  OLEFontImpl_put_Weight,
  OLEFontImpl_get_Charset,
  OLEFontImpl_put_Charset,
  OLEFontImpl_get_hFont,
  OLEFontImpl_Clone,
  OLEFontImpl_IsEqual,
  OLEFontImpl_SetRatio,
  OLEFontImpl_QueryTextMetrics,
  OLEFontImpl_AddRefHfont,
  OLEFontImpl_ReleaseHfont,
  OLEFontImpl_SetHdc
};

static ICOM_VTABLE(IDispatch) OLEFontImpl_IDispatch_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEFontImpl_IDispatch_QueryInterface,
  OLEFontImpl_IDispatch_AddRef,
  OLEFontImpl_IDispatch_Release,
  OLEFontImpl_GetTypeInfoCount,
  OLEFontImpl_GetTypeInfo,
  OLEFontImpl_GetIDsOfNames,
  OLEFontImpl_Invoke
};

static ICOM_VTABLE(IPersistStream) OLEFontImpl_IPersistStream_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEFontImpl_IPersistStream_QueryInterface,
  OLEFontImpl_IPersistStream_AddRef,
  OLEFontImpl_IPersistStream_Release,
  OLEFontImpl_GetClassID,
  OLEFontImpl_IsDirty,
  OLEFontImpl_Load,
  OLEFontImpl_Save,
  OLEFontImpl_GetSizeMax
};

static ICOM_VTABLE(IConnectionPointContainer)
     OLEFontImpl_IConnectionPointContainer_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEFontImpl_IConnectionPointContainer_QueryInterface,
  OLEFontImpl_IConnectionPointContainer_AddRef,
  OLEFontImpl_IConnectionPointContainer_Release,
  OLEFontImpl_EnumConnectionPoints,
  OLEFontImpl_FindConnectionPoint
};

static ICOM_VTABLE(IPersistPropertyBag) OLEFontImpl_IPersistPropertyBag_VTable;
static ICOM_VTABLE(IPersistStreamInit) OLEFontImpl_IPersistStreamInit_VTable;
/******************************************************************************
 *		OleCreateFontIndirect	[OLEAUT32.420]
 */
HRESULT WINAPI OleCreateFontIndirect(
  LPFONTDESC lpFontDesc,
  REFIID     riid,
  LPVOID*     ppvObj)
{
  OLEFontImpl* newFont = 0;
  HRESULT      hr      = S_OK;

  TRACE("(%p, %s, %p)\n", lpFontDesc, debugstr_guid(riid), ppvObj);
  /*
   * Sanity check
   */
  if (ppvObj==0)
    return E_POINTER;

  *ppvObj = 0;

  if (!lpFontDesc) {
    FONTDESC fd;

    WCHAR fname[] = { 'S','y','s','t','e','m',0 };

    fd.cbSizeofstruct = sizeof(fd);
    fd.lpstrName      = fname;
    fd.cySize.s.Lo    = 80000;
    fd.cySize.s.Hi    = 0;
    fd.sWeight 	      = 0;
    fd.sCharset       = 0;
    fd.fItalic	      = 0;
    fd.fUnderline     = 0;
    fd.fStrikethrough = 0;
    lpFontDesc = &fd;
  }

  /*
   * Try to construct a new instance of the class.
   */
  newFont = OLEFontImpl_Construct(lpFontDesc);

  if (newFont == 0)
    return E_OUTOFMEMORY;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IFont_QueryInterface((IFont*)newFont, riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IFont_Release((IFont*)newFont);

  return hr;
}


/***********************************************************************
 * Implementation of the OLEFontImpl class.
 */

/***********************************************************************
 *    OLEFont_SendNotify (internal)
 *
 * Sends notification messages of changed properties to any interested
 * connections.
 */
static void OLEFont_SendNotify(OLEFontImpl* this, DISPID dispID)
{
  IEnumConnections *pEnum;
  CONNECTDATA CD;
  HRESULT hres;

  hres = IConnectionPoint_EnumConnections(this->pCP, &pEnum);
  if (FAILED(hres)) /* When we have 0 connections. */
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
 * OLEFontImpl_Construct
 *
 * This method will construct a new instance of the OLEFontImpl
 * class.
 *
 * The caller of this method must release the object when it's
 * done with it.
 */
static OLEFontImpl* OLEFontImpl_Construct(LPFONTDESC fontDesc)
{
  OLEFontImpl* newObject = 0;

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(OLEFontImpl));

  if (newObject==0)
    return newObject;

  /*
   * Initialize the virtual function table.
   */
  newObject->lpvtbl1 = &OLEFontImpl_VTable;
  newObject->lpvtbl2 = &OLEFontImpl_IDispatch_VTable;
  newObject->lpvtbl3 = &OLEFontImpl_IPersistStream_VTable;
  newObject->lpvtbl4 = &OLEFontImpl_IConnectionPointContainer_VTable;
  newObject->lpvtbl5 = &OLEFontImpl_IPersistPropertyBag_VTable;
  newObject->lpvtbl6 = &OLEFontImpl_IPersistStreamInit_VTable;

  /*
   * Start with one reference count. The caller of this function
   * must release the interface pointer when it is done.
   */
  newObject->ref = 1;

  /*
   * Copy the description of the font in the object.
   */
  assert(fontDesc->cbSizeofstruct >= sizeof(FONTDESC));

  newObject->description.cbSizeofstruct = sizeof(FONTDESC);
  newObject->description.lpstrName = HeapAlloc(GetProcessHeap(),
					       0,
					       (lstrlenW(fontDesc->lpstrName)+1) * sizeof(WCHAR));
  strcpyW(newObject->description.lpstrName, fontDesc->lpstrName);
  newObject->description.cySize         = fontDesc->cySize;
  newObject->description.sWeight        = fontDesc->sWeight;
  newObject->description.sCharset       = fontDesc->sCharset;
  newObject->description.fItalic        = fontDesc->fItalic;
  newObject->description.fUnderline     = fontDesc->fUnderline;
  newObject->description.fStrikethrough = fontDesc->fStrikethrough;

  /*
   * Initializing all the other members.
   */
  newObject->gdiFont  = 0;
  newObject->fontLock = 0;
  newObject->cyLogical  = 72L;
  newObject->cyHimetric = 2540L;
  CreateConnectionPoint((IUnknown*)newObject, &IID_IPropertyNotifySink, &newObject->pCP);
  TRACE("returning %p\n", newObject);
  return newObject;
}

/************************************************************************
 * OLEFontImpl_Destroy
 *
 * This method is called by the Release method when the reference
 * count goes down to 0. It will free all resources used by
 * this object.
 */
static void OLEFontImpl_Destroy(OLEFontImpl* fontDesc)
{
  TRACE("(%p)\n", fontDesc);

  if (fontDesc->description.lpstrName!=0)
    HeapFree(GetProcessHeap(), 0, fontDesc->description.lpstrName);

  if (fontDesc->gdiFont!=0)
    DeleteObject(fontDesc->gdiFont);

  HeapFree(GetProcessHeap(), 0, fontDesc);
}

/************************************************************************
 * OLEFontImpl_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
HRESULT WINAPI OLEFontImpl_QueryInterface(
  IFont*  iface,
  REFIID  riid,
  void**  ppvObject)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%s, %p)\n", this, debugstr_guid(riid), ppvObject);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (this==0) || (ppvObject==0) )
    return E_INVALIDARG;

  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (IsEqualGUID(&IID_IUnknown, riid))
    *ppvObject = (IFont*)this;
  if (IsEqualGUID(&IID_IFont, riid))
    *ppvObject = (IFont*)this;
  if (IsEqualGUID(&IID_IDispatch, riid))
    *ppvObject = (IDispatch*)&(this->lpvtbl2);
  if (IsEqualGUID(&IID_IFontDisp, riid))
    *ppvObject = (IDispatch*)&(this->lpvtbl2);
  if (IsEqualGUID(&IID_IPersistStream, riid))
    *ppvObject = (IPersistStream*)&(this->lpvtbl3);
  if (IsEqualGUID(&IID_IConnectionPointContainer, riid))
    *ppvObject = (IConnectionPointContainer*)&(this->lpvtbl4);
  if (IsEqualGUID(&IID_IPersistPropertyBag, riid))
    *ppvObject = (IPersistPropertyBag*)&(this->lpvtbl5);
  if (IsEqualGUID(&IID_IPersistStreamInit, riid))
    *ppvObject = (IPersistStreamInit*)&(this->lpvtbl6);

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    FIXME("() : asking for unsupported interface %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
  }
  OLEFontImpl_AddRef((IFont*)this);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
ULONG WINAPI OLEFontImpl_AddRef(
  IFont* iface)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", this, this->ref);
  this->ref++;

  return this->ref;
}

/************************************************************************
 * OLEFontImpl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
ULONG WINAPI OLEFontImpl_Release(
      IFont* iface)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", this, this->ref);

  /*
   * Decrease the reference count on this object.
   */
  this->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (this->ref==0)
  {
    OLEFontImpl_Destroy(this);

    return 0;
  }

  return this->ref;
}

/************************************************************************
 * OLEFontImpl_get_Name (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Name(
  IFont*  iface,
  BSTR* pname)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, pname);
  /*
   * Sanity check.
   */
  if (pname==0)
    return E_POINTER;

  if (this->description.lpstrName!=0)
    *pname = SysAllocString(this->description.lpstrName);
  else
    *pname = 0;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Name (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Name(
  IFont* iface,
  BSTR name)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, name);

  if (this->description.lpstrName==0)
  {
    this->description.lpstrName = HeapAlloc(GetProcessHeap(),
					    0,
					    (lstrlenW(name)+1) * sizeof(WCHAR));
  }
  else
  {
    this->description.lpstrName = HeapReAlloc(GetProcessHeap(),
					      0,
					      this->description.lpstrName,
					      (lstrlenW(name)+1) * sizeof(WCHAR));
  }

  if (this->description.lpstrName==0)
    return E_OUTOFMEMORY;

  strcpyW(this->description.lpstrName, name);
  TRACE("new name %s\n", debugstr_w(this->description.lpstrName));
  OLEFont_SendNotify(this, DISPID_FONT_NAME);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Size (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Size(
  IFont* iface,
  CY*    psize)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, psize);

  /*
   * Sanity check
   */
  if (psize==0)
    return E_POINTER;

  psize->s.Hi = 0;
  psize->s.Lo = this->description.cySize.s.Lo;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Size (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Size(
  IFont* iface,
  CY     size)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%ld)\n", this, size.s.Lo);
  this->description.cySize.s.Hi = 0;
  this->description.cySize.s.Lo = size.s.Lo;
  OLEFont_SendNotify(this, DISPID_FONT_SIZE);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Bold (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Bold(
  IFont*  iface,
  BOOL* pbold)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, pbold);
  /*
   * Sanity check
   */
  if (pbold==0)
    return E_POINTER;

  *pbold = this->description.sWeight > 550;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Bold (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Bold(
  IFont* iface,
  BOOL bold)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%d)\n", this, bold);
  this->description.sWeight = bold ? FW_BOLD : FW_NORMAL;
  OLEFont_SendNotify(this, DISPID_FONT_BOLD);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Italic (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Italic(
  IFont*  iface,
  BOOL* pitalic)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, pitalic);
  /*
   * Sanity check
   */
  if (pitalic==0)
    return E_POINTER;

  *pitalic = this->description.fItalic;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Italic (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Italic(
  IFont* iface,
  BOOL italic)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%d)\n", this, italic);

  this->description.fItalic = italic;

  OLEFont_SendNotify(this, DISPID_FONT_ITALIC);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Underline (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Underline(
  IFont*  iface,
  BOOL* punderline)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, punderline);

  /*
   * Sanity check
   */
  if (punderline==0)
    return E_POINTER;

  *punderline = this->description.fUnderline;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Underline (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Underline(
  IFont* iface,
  BOOL underline)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%d)\n", this, underline);

  this->description.fUnderline = underline;

  OLEFont_SendNotify(this, DISPID_FONT_UNDER);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Strikethrough (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Strikethrough(
  IFont*  iface,
  BOOL* pstrikethrough)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, pstrikethrough);

  /*
   * Sanity check
   */
  if (pstrikethrough==0)
    return E_POINTER;

  *pstrikethrough = this->description.fStrikethrough;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Strikethrough (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Strikethrough(
 IFont* iface,
 BOOL strikethrough)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%d)\n", this, strikethrough);

  this->description.fStrikethrough = strikethrough;
  OLEFont_SendNotify(this, DISPID_FONT_STRIKE);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Weight (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Weight(
  IFont* iface,
  short* pweight)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, pweight);

  /*
   * Sanity check
   */
  if (pweight==0)
    return E_POINTER;

  *pweight = this->description.sWeight;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Weight (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Weight(
  IFont* iface,
  short  weight)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%d)\n", this, weight);

  this->description.sWeight = weight;

  OLEFont_SendNotify(this, DISPID_FONT_WEIGHT);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Charset (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Charset(
  IFont* iface,
  short* pcharset)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, pcharset);

  /*
   * Sanity check
   */
  if (pcharset==0)
    return E_POINTER;

  *pcharset = this->description.sCharset;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Charset (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Charset(
  IFont* iface,
  short charset)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%d)\n", this, charset);

  this->description.sCharset = charset;
  OLEFont_SendNotify(this, DISPID_FONT_CHARSET);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_hFont (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_hFont(
  IFont*   iface,
  HFONT* phfont)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, phfont);
  if (phfont==NULL)
    return E_POINTER;

  /*
   * Realize the font if necessary
 */
  if (this->gdiFont==0)
{
    LOGFONTW logFont;
    INT      fontHeight;
    CY       cySize;

    /*
     * The height of the font returned by the get_Size property is the
     * height of the font in points multiplied by 10000... Using some
     * simple conversions and the ratio given by the application, it can
     * be converted to a height in pixels.
     */
    IFont_get_Size(iface, &cySize);

    fontHeight = MulDiv( cySize.s.Lo, this->cyLogical, this->cyHimetric );

    memset(&logFont, 0, sizeof(LOGFONTW));

    logFont.lfHeight          = ((fontHeight%10000L)>5000L) ?	(-fontHeight/10000L)-1 :
 						    		(-fontHeight/10000L);
    logFont.lfItalic          = this->description.fItalic;
    logFont.lfUnderline       = this->description.fUnderline;
    logFont.lfStrikeOut       = this->description.fStrikethrough;
    logFont.lfWeight          = this->description.sWeight;
    logFont.lfCharSet         = this->description.sCharset;
    logFont.lfOutPrecision    = OUT_CHARACTER_PRECIS;
    logFont.lfClipPrecision   = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality         = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily  = DEFAULT_PITCH;
    strcpyW(logFont.lfFaceName,this->description.lpstrName);

    this->gdiFont = CreateFontIndirectW(&logFont);
  }

  *phfont = this->gdiFont;
  TRACE("Returning %p\n", *phfont);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_Clone (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_Clone(
  IFont*  iface,
  IFont** ppfont)
{
  OLEFontImpl* newObject = 0;
  LOGFONTW logFont;
  INT      fontHeight;
  CY       cySize;
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p)\n", this, ppfont);

  if (ppfont == NULL)
    return E_POINTER;

  *ppfont = NULL;

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(OLEFontImpl));

  if (newObject==NULL)
    return E_OUTOFMEMORY;

  *newObject = *this;

  /* We need to alloc new memory for the string, otherwise
   * we free memory twice.
   */
  newObject->description.lpstrName = HeapAlloc(
	GetProcessHeap(),0,
	(1+strlenW(this->description.lpstrName))*2
  );
  strcpyW(newObject->description.lpstrName, this->description.lpstrName);
  /* We need to clone the HFONT too. This is just cut & paste from above */
  IFont_get_Size(iface, &cySize);

  fontHeight = MulDiv(cySize.s.Lo, this->cyLogical,this->cyHimetric);

  memset(&logFont, 0, sizeof(LOGFONTW));

  logFont.lfHeight          = ((fontHeight%10000L)>5000L) ? (-fontHeight/10000L)-1 :
							    (-fontHeight/10000L);
  logFont.lfItalic          = this->description.fItalic;
  logFont.lfUnderline       = this->description.fUnderline;
  logFont.lfStrikeOut       = this->description.fStrikethrough;
  logFont.lfWeight          = this->description.sWeight;
  logFont.lfCharSet         = this->description.sCharset;
  logFont.lfOutPrecision    = OUT_CHARACTER_PRECIS;
  logFont.lfClipPrecision   = CLIP_DEFAULT_PRECIS;
  logFont.lfQuality         = DEFAULT_QUALITY;
  logFont.lfPitchAndFamily  = DEFAULT_PITCH;
  strcpyW(logFont.lfFaceName,this->description.lpstrName);

  newObject->gdiFont = CreateFontIndirectW(&logFont);


  /* The cloned object starts with a reference count of 1 */
  newObject->ref          = 1;

  *ppfont = (IFont*)newObject;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_IsEqual (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_IsEqual(
  IFont* iface,
  IFont* pFontOther)
{
  FIXME("(%p, %p), stub!\n",iface,pFontOther);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_SetRatio (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_SetRatio(
  IFont* iface,
  long   cyLogical,
  long   cyHimetric)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%ld, %ld)\n", this, cyLogical, cyHimetric);

  this->cyLogical  = cyLogical;
  this->cyHimetric = cyHimetric;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_QueryTextMetrics (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT      WINAPI OLEFontImpl_QueryTextMetrics(
  IFont*         iface,
  TEXTMETRICOLE* ptm)
{
  FIXME("(%p, %p), stub!\n",iface,ptm);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_AddRefHfont (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_AddRefHfont(
  IFont*  iface,
  HFONT hfont)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p) (lock=%ld)\n", this, hfont, this->fontLock);

  if ( (hfont == 0) ||
       (hfont != this->gdiFont) )
    return E_INVALIDARG;

  this->fontLock++;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_ReleaseHfont (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_ReleaseHfont(
  IFont*  iface,
  HFONT hfont)
{
  _ICOM_THIS(OLEFontImpl, iface);
  TRACE("(%p)->(%p) (lock=%ld)\n", this, hfont, this->fontLock);

  if ( (hfont == 0) ||
       (hfont != this->gdiFont) )
    return E_INVALIDARG;

  this->fontLock--;

  /*
   * If we just released our last font reference, destroy it.
   */
  if (this->fontLock==0)
  {
    DeleteObject(this->gdiFont);
    this->gdiFont = 0;
  }

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_SetHdc (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_SetHdc(
  IFont* iface,
  HDC  hdc)
{
  _ICOM_THIS(OLEFontImpl, iface);
  FIXME("(%p)->(%p): Stub\n", this, hdc);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_IDispatch_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEFontImpl_IDispatch_QueryInterface(
  IDispatch* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  _ICOM_THIS_From_IDispatch(IFont, iface);

  return IFont_QueryInterface(this, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IDispatch_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IDispatch_Release(
  IDispatch* iface)
{
  _ICOM_THIS_From_IDispatch(IFont, iface);

  return IFont_Release(this);
}

/************************************************************************
 * OLEFontImpl_IDispatch_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IDispatch_AddRef(
  IDispatch* iface)
{
  _ICOM_THIS_From_IDispatch(IFont, iface);

  return IFont_AddRef(this);
}

/************************************************************************
 * OLEFontImpl_GetTypeInfoCount (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEFontImpl_GetTypeInfoCount(
  IDispatch*    iface,
  unsigned int* pctinfo)
{
  _ICOM_THIS_From_IDispatch(IFont, iface);
  FIXME("(%p)->(%p): Stub\n", this, pctinfo);

  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_GetTypeInfo (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEFontImpl_GetTypeInfo(
  IDispatch*  iface,
  UINT      iTInfo,
  LCID        lcid,
  ITypeInfo** ppTInfo)
{
  WCHAR stdole32tlb[] = {'s','t','d','o','l','e','3','2','.','t','l','b',0};
  ITypeLib *tl;
  HRESULT hres;

  _ICOM_THIS_From_IDispatch(OLEFontImpl, iface);
  TRACE("(%p, iTInfo=%d, lcid=%04x, %p), unimplemented stub!\n", this, iTInfo, (int)lcid, ppTInfo);
  if (iTInfo != 0)
    return E_FAIL;
  hres = LoadTypeLib(stdole32tlb, &tl);
  if (FAILED(hres)) {
    FIXME("Could not load the stdole32.tlb?\n");
    return hres;
  }
  hres = ITypeLib_GetTypeInfoOfGuid(tl, &IID_IDispatch, ppTInfo);
  if (FAILED(hres)) {
    FIXME("Did not IDispatch typeinfo from typelib, hres %lx\n",hres);
  }
  return hres;
}

/************************************************************************
 * OLEFontImpl_GetIDsOfNames (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEFontImpl_GetIDsOfNames(
  IDispatch*  iface,
  REFIID      riid,
  LPOLESTR* rgszNames,
  UINT      cNames,
  LCID        lcid,
  DISPID*     rgDispId)
{
  _ICOM_THIS_From_IDispatch(IFont, iface);
  FIXME("(%p,%s,%p,%d,%04x,%p), stub!\n", this, debugstr_guid(riid), rgszNames,
	cNames, (int)lcid, rgDispId
  );
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_Invoke (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 * 
 * Note: Do not call _put_Xxx methods, since setting things here
 * should not call notify functions as I found out debugging the generic
 * MS VB5 installer.
 */
static HRESULT WINAPI OLEFontImpl_Invoke(
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
  _ICOM_THIS_From_IDispatch(IFont, iface);
  OLEFontImpl *xthis = (OLEFontImpl*)this;

  switch (dispIdMember) {
  case DISPID_FONT_NAME:
    switch (wFlags) {
    case DISPATCH_PROPERTYGET:
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
      V_VT(pVarResult) = VT_BSTR;
      return OLEFontImpl_get_Name(this, &V_BSTR(pVarResult));
    case DISPATCH_PROPERTYPUT: {
      BSTR name = V_BSTR(&pDispParams->rgvarg[0]);
      if (V_VT(&pDispParams->rgvarg[0])!=VT_BSTR) {
        FIXME("property put of Name, vt is not VT_BSTR but %d\n",V_VT(&pDispParams->rgvarg[0]));
	return E_FAIL;
      }
      if (!xthis->description.lpstrName)
	xthis->description.lpstrName = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(name)+1) * sizeof(WCHAR));
      else
	xthis->description.lpstrName = HeapReAlloc(GetProcessHeap(), 0, xthis->description.lpstrName, (lstrlenW(name)+1) * sizeof(WCHAR));

      if (xthis->description.lpstrName==0)
	return E_OUTOFMEMORY;
      strcpyW(xthis->description.lpstrName, name);
      return S_OK;
    }
    }
    break;
  case DISPID_FONT_BOLD:
    switch (wFlags) {
    case DISPATCH_PROPERTYGET:
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
      V_VT(pVarResult) = VT_BOOL;
      return OLEFontImpl_get_Bold(this, (BOOL*)&V_BOOL(pVarResult));
    case DISPATCH_PROPERTYPUT:
      if (V_VT(&pDispParams->rgvarg[0]) != VT_BOOL) {
	FIXME("DISPID_FONT_BOLD/put, vt is %d, not VT_BOOL.\n",V_VT(&pDispParams->rgvarg[0]));
	return E_FAIL;
      } else {
        xthis->description.sWeight = V_BOOL(&pDispParams->rgvarg[0]) ? FW_BOLD : FW_NORMAL;
	return S_OK;
      }
    }
    break;
  case DISPID_FONT_ITALIC:
    switch (wFlags) {
    case DISPATCH_PROPERTYGET:
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
      V_VT(pVarResult) = VT_BOOL;
      return OLEFontImpl_get_Italic(this, (BOOL*)&V_BOOL(pVarResult));
    case DISPATCH_PROPERTYPUT:
      if (V_VT(&pDispParams->rgvarg[0]) != VT_BOOL) {
	FIXME("DISPID_FONT_ITALIC/put, vt is %d, not VT_BOOL.\n",V_VT(&pDispParams->rgvarg[0]));
	return E_FAIL;
      } else {
        xthis->description.fItalic = V_BOOL(&pDispParams->rgvarg[0]);
	return S_OK;
      }
    }
    break;
  case DISPID_FONT_UNDER:
    switch (wFlags) {
    case DISPATCH_PROPERTYGET:
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
      V_VT(pVarResult) = VT_BOOL;
      return OLEFontImpl_get_Underline(this, (BOOL*)&V_BOOL(pVarResult));
    case DISPATCH_PROPERTYPUT:
      if (V_VT(&pDispParams->rgvarg[0]) != VT_BOOL) {
	FIXME("DISPID_FONT_UNDER/put, vt is %d, not VT_BOOL.\n",V_VT(&pDispParams->rgvarg[0]));
	return E_FAIL;
      } else {
        xthis->description.fUnderline = V_BOOL(&pDispParams->rgvarg[0]);
	return S_OK;
      }
    }
    break;
  case DISPID_FONT_STRIKE:
    switch (wFlags) {
    case DISPATCH_PROPERTYGET:
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
      V_VT(pVarResult) = VT_BOOL;
      return OLEFontImpl_get_Strikethrough(this, (BOOL*)&V_BOOL(pVarResult));
    case DISPATCH_PROPERTYPUT:
      if (V_VT(&pDispParams->rgvarg[0]) != VT_BOOL) {
	FIXME("DISPID_FONT_STRIKE/put, vt is %d, not VT_BOOL.\n",V_VT(&pDispParams->rgvarg[0]));
	return E_FAIL;
      } else {
        xthis->description.fStrikethrough = V_BOOL(&pDispParams->rgvarg[0]);
	return S_OK;
      }
    }
    break;
  case DISPID_FONT_SIZE:
    switch (wFlags) {
    case DISPATCH_PROPERTYPUT: {
      assert (pDispParams->cArgs == 1);
      xthis->description.cySize.s.Hi = 0;
      if (V_VT(&pDispParams->rgvarg[0]) != VT_CY) {
        if (V_VT(&pDispParams->rgvarg[0]) == VT_I2) {
	  xthis->description.cySize.s.Lo = V_I2(&pDispParams->rgvarg[0]) * 10000;
	} else {
	  FIXME("property put for Size with vt %d unsupported!\n",V_VT(&pDispParams->rgvarg[0]));
	}
      } else {
        xthis->description.cySize.s.Lo = V_CY(&pDispParams->rgvarg[0]).s.Lo;
      }
      return S_OK;
    }
    case DISPATCH_PROPERTYGET:
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
      V_VT(pVarResult) = VT_CY;
      return OLEFontImpl_get_Size(this, &V_CY(pVarResult));
    }
    break;
  case DISPID_FONT_CHARSET:
    switch (wFlags) {
    case DISPATCH_PROPERTYPUT:
      assert (pDispParams->cArgs == 1);
      if (V_VT(&pDispParams->rgvarg[0]) != VT_I2)
	FIXME("varg of first disparg is not VT_I2, but %d\n",V_VT(&pDispParams->rgvarg[0]));
      xthis->description.sCharset = V_I2(&pDispParams->rgvarg[0]);
      return S_OK;
    case DISPATCH_PROPERTYGET:
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
      V_VT(pVarResult) = VT_I2;
      return OLEFontImpl_get_Charset(this, &V_I2(pVarResult));
    }
    break;
  }
  FIXME("%p->(%ld,%s,%lx,%x,%p,%p,%p,%p), unhandled dispid/flag!\n",
    this,dispIdMember,debugstr_guid(riid),lcid,
    wFlags,pDispParams,pVarResult,pExepInfo,puArgErr
  );
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_IPersistStream_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEFontImpl_IPersistStream_QueryInterface(
  IPersistStream* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  _ICOM_THIS_From_IPersistStream(IFont, iface);

  return IFont_QueryInterface(this, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IPersistStream_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IPersistStream_Release(
  IPersistStream* iface)
{
  _ICOM_THIS_From_IPersistStream(IFont, iface);

  return IFont_Release(this);
}

/************************************************************************
 * OLEFontImpl_IPersistStream_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IPersistStream_AddRef(
  IPersistStream* iface)
{
  _ICOM_THIS_From_IPersistStream(IFont, iface);

  return IFont_AddRef(this);
}

/************************************************************************
 * OLEFontImpl_GetClassID (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 */
static HRESULT WINAPI OLEFontImpl_GetClassID(
  IPersistStream* iface,
  CLSID*                pClassID)
{
  TRACE("(%p,%p)\n",iface,pClassID);
  if (pClassID==0)
    return E_POINTER;

  memcpy(pClassID, &CLSID_StdFont, sizeof(CLSID_StdFont));

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_IsDirty (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 */
static HRESULT WINAPI OLEFontImpl_IsDirty(
  IPersistStream*  iface)
{
  TRACE("(%p)\n",iface);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_Load (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 *
 * This is the format of the standard font serialization as far as I
 * know
 *
 * Offset   Type   Value           Comment
 * 0x0000   Byte   Unknown         Probably a version number, contains 0x01
 * 0x0001   Short  Charset         Charset value from the FONTDESC structure
 * 0x0003   Byte   Attributes      Flags defined as follows:
 *                                     00000010 - Italic
 *                                     00000100 - Underline
 *                                     00001000 - Strikethrough
 * 0x0004   Short  Weight          Weight value from FONTDESC structure
 * 0x0006   DWORD  size            "Low" portion of the cySize member of the FONTDESC
 *                                 structure/
 * 0x000A   Byte   name length     Length of the font name string (no null character)
 * 0x000B   String name            Name of the font (ASCII, no nul character)
 */
static HRESULT WINAPI OLEFontImpl_Load(
  IPersistStream*  iface,
  IStream*         pLoadStream)
{
  char  readBuffer[0x100];
  ULONG cbRead;
  BYTE  bVersion;
  BYTE  bAttributes;
  BYTE  bStringSize;
  INT len;

  _ICOM_THIS_From_IPersistStream(OLEFontImpl, iface);

  /*
   * Read the version byte
   */
  IStream_Read(pLoadStream, &bVersion, 1, &cbRead);

  if ( (cbRead!=1) ||
       (bVersion!=0x01) )
    return E_FAIL;

  /*
   * Charset
   */
  IStream_Read(pLoadStream, &this->description.sCharset, 2, &cbRead);

  if (cbRead!=2)
    return E_FAIL;

  /*
   * Attributes
   */
  IStream_Read(pLoadStream, &bAttributes, 1, &cbRead);

  if (cbRead!=1)
    return E_FAIL;

  this->description.fItalic        = (bAttributes & FONTPERSIST_ITALIC) != 0;
  this->description.fStrikethrough = (bAttributes & FONTPERSIST_STRIKETHROUGH) != 0;
  this->description.fUnderline     = (bAttributes & FONTPERSIST_UNDERLINE) != 0;

  /*
   * Weight
   */
  IStream_Read(pLoadStream, &this->description.sWeight, 2, &cbRead);

  if (cbRead!=2)
    return E_FAIL;

  /*
   * Size
   */
  IStream_Read(pLoadStream, &this->description.cySize.s.Lo, 4, &cbRead);

  if (cbRead!=4)
    return E_FAIL;

  this->description.cySize.s.Hi = 0;

  /*
   * FontName
   */
  IStream_Read(pLoadStream, &bStringSize, 1, &cbRead);

  if (cbRead!=1)
    return E_FAIL;

  IStream_Read(pLoadStream, readBuffer, bStringSize, &cbRead);

  if (cbRead!=bStringSize)
    return E_FAIL;

  if (this->description.lpstrName!=0)
    HeapFree(GetProcessHeap(), 0, this->description.lpstrName);

  len = MultiByteToWideChar( CP_ACP, 0, readBuffer, bStringSize, NULL, 0 );
  this->description.lpstrName = HeapAlloc( GetProcessHeap(), 0, (len+1) * sizeof(WCHAR) );
  MultiByteToWideChar( CP_ACP, 0, readBuffer, bStringSize, this->description.lpstrName, len );
  this->description.lpstrName[len] = 0;

  /* Ensure use of this font causes a new one to be created @@@@ */
  DeleteObject(this->gdiFont);
  this->gdiFont = 0;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_Save (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 */
static HRESULT WINAPI OLEFontImpl_Save(
  IPersistStream*  iface,
  IStream*         pOutStream,
  BOOL             fClearDirty)
{
  char* writeBuffer = NULL;
  ULONG cbWritten;
  BYTE  bVersion = 0x01;
  BYTE  bAttributes;
  BYTE  bStringSize;

  _ICOM_THIS_From_IPersistStream(OLEFontImpl, iface);

  /*
   * Read the version byte
   */
  IStream_Write(pOutStream, &bVersion, 1, &cbWritten);

  if (cbWritten!=1)
    return E_FAIL;

  /*
   * Charset
   */
  IStream_Write(pOutStream, &this->description.sCharset, 2, &cbWritten);

  if (cbWritten!=2)
    return E_FAIL;

  /*
   * Attributes
   */
  bAttributes = 0;

  if (this->description.fItalic)
    bAttributes |= FONTPERSIST_ITALIC;

  if (this->description.fStrikethrough)
    bAttributes |= FONTPERSIST_STRIKETHROUGH;

  if (this->description.fUnderline)
    bAttributes |= FONTPERSIST_UNDERLINE;

  IStream_Write(pOutStream, &bAttributes, 1, &cbWritten);

  if (cbWritten!=1)
    return E_FAIL;

  /*
   * Weight
   */
  IStream_Write(pOutStream, &this->description.sWeight, 2, &cbWritten);

  if (cbWritten!=2)
    return E_FAIL;

  /*
   * Size
   */
  IStream_Write(pOutStream, &this->description.cySize.s.Lo, 4, &cbWritten);

  if (cbWritten!=4)
    return E_FAIL;

  /*
   * FontName
   */
  if (this->description.lpstrName!=0)
    bStringSize = WideCharToMultiByte( CP_ACP, 0, this->description.lpstrName,
                                       strlenW(this->description.lpstrName), NULL, 0, NULL, NULL );
  else
    bStringSize = 0;

  IStream_Write(pOutStream, &bStringSize, 1, &cbWritten);

  if (cbWritten!=1)
    return E_FAIL;

  if (bStringSize!=0)
  {
      if (!(writeBuffer = HeapAlloc( GetProcessHeap(), 0, bStringSize ))) return E_OUTOFMEMORY;
      WideCharToMultiByte( CP_ACP, 0, this->description.lpstrName,
                           strlenW(this->description.lpstrName),
                           writeBuffer, bStringSize, NULL, NULL );

    IStream_Write(pOutStream, writeBuffer, bStringSize, &cbWritten);
    HeapFree(GetProcessHeap(), 0, writeBuffer);

    if (cbWritten!=bStringSize)
      return E_FAIL;
  }

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_GetSizeMax (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 */
static HRESULT WINAPI OLEFontImpl_GetSizeMax(
  IPersistStream*  iface,
  ULARGE_INTEGER*  pcbSize)
{
  _ICOM_THIS_From_IPersistStream(OLEFontImpl, iface);

  if (pcbSize==NULL)
    return E_POINTER;

  pcbSize->u.HighPart = 0;
  pcbSize->u.LowPart = 0;

  pcbSize->u.LowPart += sizeof(BYTE);  /* Version */
  pcbSize->u.LowPart += sizeof(WORD);  /* Lang code */
  pcbSize->u.LowPart += sizeof(BYTE);  /* Flags */
  pcbSize->u.LowPart += sizeof(WORD);  /* Weight */
  pcbSize->u.LowPart += sizeof(DWORD); /* Size */
  pcbSize->u.LowPart += sizeof(BYTE);  /* StrLength */

  if (this->description.lpstrName!=0)
    pcbSize->u.LowPart += lstrlenW(this->description.lpstrName);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_IConnectionPointContainer_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEFontImpl_IConnectionPointContainer_QueryInterface(
  IConnectionPointContainer* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  _ICOM_THIS_From_IConnectionPointContainer(OLEFontImpl, iface);

  return IFont_QueryInterface((IFont*)this, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IConnectionPointContainer_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IConnectionPointContainer_Release(
  IConnectionPointContainer* iface)
{
  _ICOM_THIS_From_IConnectionPointContainer(OLEFontImpl, iface);

  return IFont_Release((IFont*)this);
}

/************************************************************************
 * OLEFontImpl_IConnectionPointContainer_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IConnectionPointContainer_AddRef(
  IConnectionPointContainer* iface)
{
  _ICOM_THIS_From_IConnectionPointContainer(OLEFontImpl, iface);

  return IFont_AddRef((IFont*)this);
}

/************************************************************************
 * OLEFontImpl_EnumConnectionPoints (IConnectionPointContainer)
 *
 * See Windows documentation for more details on IConnectionPointContainer
 * methods.
 */
static HRESULT WINAPI OLEFontImpl_EnumConnectionPoints(
  IConnectionPointContainer* iface,
  IEnumConnectionPoints **ppEnum)
{
  _ICOM_THIS_From_IConnectionPointContainer(OLEFontImpl, iface);

  FIXME("(%p)->(%p): stub\n", this, ppEnum);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_FindConnectionPoint (IConnectionPointContainer)
 *
 * See Windows documentation for more details on IConnectionPointContainer
 * methods.
 */
static HRESULT WINAPI OLEFontImpl_FindConnectionPoint(
   IConnectionPointContainer* iface,
   REFIID riid,
   IConnectionPoint **ppCp)
{
  _ICOM_THIS_From_IConnectionPointContainer(OLEFontImpl, iface);
  TRACE("(%p)->(%s, %p): stub\n", this, debugstr_guid(riid), ppCp);

  if(memcmp(riid, &IID_IPropertyNotifySink, sizeof(IID_IPropertyNotifySink)) == 0) {
    return IConnectionPoint_QueryInterface(this->pCP, &IID_IConnectionPoint,
					   (LPVOID)ppCp);
  } else {
    FIXME("Tried to find connection point on %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
  }
}

/************************************************************************
 * OLEFontImpl implementation of IPersistPropertyBag.
 */
static HRESULT WINAPI OLEFontImpl_IPersistPropertyBag_QueryInterface(
   IPersistPropertyBag *iface, REFIID riid, LPVOID *ppvObj
) {
  _ICOM_THIS_From_IPersistPropertyBag(IFont, iface);
  return IFont_QueryInterface(this,riid,ppvObj);
}

static ULONG WINAPI OLEFontImpl_IPersistPropertyBag_AddRef(
   IPersistPropertyBag *iface
) {
  _ICOM_THIS_From_IPersistPropertyBag(IFont, iface);
  return IFont_AddRef(this);
}

static ULONG WINAPI OLEFontImpl_IPersistPropertyBag_Release(
   IPersistPropertyBag *iface
) {
  _ICOM_THIS_From_IPersistPropertyBag(IFont, iface);
  return IFont_Release(this);
}

static HRESULT WINAPI OLEFontImpl_IPersistPropertyBag_GetClassID(
   IPersistPropertyBag *iface, CLSID *classid
) {
  FIXME("(%p,%p), stub!\n", iface, classid);
  return E_FAIL;
}

static HRESULT WINAPI OLEFontImpl_IPersistPropertyBag_InitNew(
   IPersistPropertyBag *iface
) {
  FIXME("(%p), stub!\n", iface);
  return S_OK;
}

static HRESULT WINAPI OLEFontImpl_IPersistPropertyBag_Load(
   IPersistPropertyBag *iface, IPropertyBag* pPropBag, IErrorLog* pErrorLog
) {
  FIXME("(%p,%p,%p), stub!\n", iface, pPropBag, pErrorLog);
  return E_FAIL;
}

static HRESULT WINAPI OLEFontImpl_IPersistPropertyBag_Save(
   IPersistPropertyBag *iface, IPropertyBag* pPropBag, BOOL fClearDirty,
   BOOL fSaveAllProperties
) {
  FIXME("(%p,%p,%d,%d), stub!\n", iface, pPropBag, fClearDirty, fSaveAllProperties);
  return E_FAIL;
}

static ICOM_VTABLE(IPersistPropertyBag) OLEFontImpl_IPersistPropertyBag_VTable = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEFontImpl_IPersistPropertyBag_QueryInterface,
  OLEFontImpl_IPersistPropertyBag_AddRef,
  OLEFontImpl_IPersistPropertyBag_Release,

  OLEFontImpl_IPersistPropertyBag_GetClassID,
  OLEFontImpl_IPersistPropertyBag_InitNew,
  OLEFontImpl_IPersistPropertyBag_Load,
  OLEFontImpl_IPersistPropertyBag_Save
};

/************************************************************************
 * OLEFontImpl implementation of IPersistStreamInit.
 */
static HRESULT WINAPI OLEFontImpl_IPersistStreamInit_QueryInterface(
   IPersistStreamInit *iface, REFIID riid, LPVOID *ppvObj
) {
  _ICOM_THIS_From_IPersistStreamInit(IFont, iface);
  return IFont_QueryInterface(this,riid,ppvObj);
}

static ULONG WINAPI OLEFontImpl_IPersistStreamInit_AddRef(
   IPersistStreamInit *iface
) {
  _ICOM_THIS_From_IPersistStreamInit(IFont, iface);
  return IFont_AddRef(this);
}

static ULONG WINAPI OLEFontImpl_IPersistStreamInit_Release(
   IPersistStreamInit *iface
) {
  _ICOM_THIS_From_IPersistStreamInit(IFont, iface);
  return IFont_Release(this);
}

static HRESULT WINAPI OLEFontImpl_IPersistStreamInit_GetClassID(
   IPersistStreamInit *iface, CLSID *classid
) {
  FIXME("(%p,%p), stub!\n", iface, classid);
  return E_FAIL;
}

static HRESULT WINAPI OLEFontImpl_IPersistStreamInit_IsDirty(
   IPersistStreamInit *iface
) {
  FIXME("(%p), stub!\n", iface);
  return E_FAIL;
}

static HRESULT WINAPI OLEFontImpl_IPersistStreamInit_Load(
   IPersistStreamInit *iface, LPSTREAM pStm
) {
  FIXME("(%p,%p), stub!\n", iface, pStm);
  return E_FAIL;
}

static HRESULT WINAPI OLEFontImpl_IPersistStreamInit_Save(
   IPersistStreamInit *iface, LPSTREAM pStm, BOOL fClearDirty
) {
  FIXME("(%p,%p,%d), stub!\n", iface, pStm, fClearDirty);
  return E_FAIL;
}

static HRESULT WINAPI OLEFontImpl_IPersistStreamInit_GetSizeMax(
   IPersistStreamInit *iface, ULARGE_INTEGER *pcbSize
) {
  FIXME("(%p,%p), stub!\n", iface, pcbSize);
  return E_FAIL;
}

static HRESULT WINAPI OLEFontImpl_IPersistStreamInit_InitNew(
   IPersistStreamInit *iface
) {
  FIXME("(%p), stub!\n", iface);
  return S_OK;
}

static ICOM_VTABLE(IPersistStreamInit) OLEFontImpl_IPersistStreamInit_VTable = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEFontImpl_IPersistStreamInit_QueryInterface,
  OLEFontImpl_IPersistStreamInit_AddRef,
  OLEFontImpl_IPersistStreamInit_Release,

  OLEFontImpl_IPersistStreamInit_GetClassID,
  OLEFontImpl_IPersistStreamInit_IsDirty,
  OLEFontImpl_IPersistStreamInit_Load,
  OLEFontImpl_IPersistStreamInit_Save,
  OLEFontImpl_IPersistStreamInit_GetSizeMax,
  OLEFontImpl_IPersistStreamInit_InitNew
};

/*******************************************************************************
 * StdFont ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI
SFCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
SFCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI SFCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI SFCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	return OleCreateFontIndirect(NULL,riid,ppobj);

}

static HRESULT WINAPI SFCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) SFCF_Vtbl = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	SFCF_QueryInterface,
	SFCF_AddRef,
	SFCF_Release,
	SFCF_CreateInstance,
	SFCF_LockServer
};
static IClassFactoryImpl STDFONT_CF = {&SFCF_Vtbl, 1 };

void _get_STDFONT_CF(LPVOID *ppv) { *ppv = (LPVOID)&STDFONT_CF; }
