/*
 * OLE Font encapsulation implementation
 *
 * This file contains an implementation of the IFont
 * interface and the OleCreateFontIndirect API call.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 2006 (Google) Benjamin Arai
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
 */
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "oleauto.h"    /* for SysAllocString(....) */
#include "ole2.h"
#include "olectl.h"
#include "wine/debug.h"
#include "connpt.h" /* for CreateConnectionPoint */
#include "oaidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 * Declaration of constants used when serializing the font object.
 */
#define FONTPERSIST_ITALIC        0x02
#define FONTPERSIST_UNDERLINE     0x04
#define FONTPERSIST_STRIKETHROUGH 0x08


/***********************************************************************
 * List of the HFONTs it has given out, with each one having a separate
 * ref count.
 */
typedef struct _HFONTItem
{
  struct list entry;

  /* Reference count for that instance of the class. */
  LONG ref;

  /* Contain the font associated with this object. */
  HFONT gdiFont;

} HFONTItem, *PHFONTItem;

static struct list OLEFontImpl_hFontList = LIST_INIT(OLEFontImpl_hFontList);

/* Counts how many fonts contain at least one lock */
static LONG ifont_cnt = 0;

/***********************************************************************
 * Critical section for OLEFontImpl_hFontList
 */
static CRITICAL_SECTION OLEFontImpl_csHFONTLIST;
static CRITICAL_SECTION_DEBUG OLEFontImpl_csHFONTLIST_debug =
{
  0, 0, &OLEFontImpl_csHFONTLIST,
  { &OLEFontImpl_csHFONTLIST_debug.ProcessLocksList,
    &OLEFontImpl_csHFONTLIST_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": OLEFontImpl_csHFONTLIST") }
};
static CRITICAL_SECTION OLEFontImpl_csHFONTLIST = { &OLEFontImpl_csHFONTLIST_debug, -1, 0, 0, 0, 0 };

static void HFONTItem_Delete(PHFONTItem item)
{
  DeleteObject(item->gdiFont);
  list_remove(&item->entry);
  HeapFree(GetProcessHeap(), 0, item);
}

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
  const IFontVtbl*                     lpVtbl;
  const IDispatchVtbl*                 lpvtblIDispatch;
  const IPersistStreamVtbl*            lpvtblIPersistStream;
  const IConnectionPointContainerVtbl* lpvtblIConnectionPointContainer;
  const IPersistPropertyBagVtbl*       lpvtblIPersistPropertyBag;
  const IPersistStreamInitVtbl*        lpvtblIPersistStreamInit;
  /*
   * Reference count for that instance of the class.
   */
  LONG ref;

  /*
   * This structure contains the description of the class.
   */
  FONTDESC description;

  /*
   * Contain the font associated with this object.
   */
  HFONT gdiFont;

  /*
   * Size ratio
   */
  long cyLogical;
  long cyHimetric;

  IConnectionPoint *pPropertyNotifyCP;
  IConnectionPoint *pFontEventsCP;
};

/*
 * Here, I define utility macros to help with the casting of the
 * "this" parameter.
 * There is a version to accommodate all of the VTables implemented
 * by this object.
 */

static inline OLEFontImpl *impl_from_IDispatch( IDispatch *iface )
{
    return (OLEFontImpl *)((char*)iface - FIELD_OFFSET(OLEFontImpl, lpvtblIDispatch));
}

static inline OLEFontImpl *impl_from_IPersistStream( IPersistStream *iface )
{
    return (OLEFontImpl *)((char*)iface - FIELD_OFFSET(OLEFontImpl, lpvtblIPersistStream));
}

static inline OLEFontImpl *impl_from_IConnectionPointContainer( IConnectionPointContainer *iface )
{
    return (OLEFontImpl *)((char*)iface - FIELD_OFFSET(OLEFontImpl, lpvtblIConnectionPointContainer));
}

static inline OLEFontImpl *impl_from_IPersistPropertyBag( IPersistPropertyBag *iface )
{
    return (OLEFontImpl *)((char*)iface - FIELD_OFFSET(OLEFontImpl, lpvtblIPersistPropertyBag));
}

static inline OLEFontImpl *impl_from_IPersistStreamInit( IPersistStreamInit *iface )
{
    return (OLEFontImpl *)((char*)iface - FIELD_OFFSET(OLEFontImpl, lpvtblIPersistStreamInit));
}


/***********************************************************************
 * Prototypes for the implementation functions for the IFont
 * interface
 */
static OLEFontImpl* OLEFontImpl_Construct(const FONTDESC *fontDesc);
static void         OLEFontImpl_Destroy(OLEFontImpl* fontDesc);
static ULONG        WINAPI OLEFontImpl_AddRef(IFont* iface);

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

    static WCHAR fname[] = { 'S','y','s','t','e','m',0 };

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
  static const WCHAR wszName[] = {'N','a','m','e',0};
  static const WCHAR wszSize[] = {'S','i','z','e',0};
  static const WCHAR wszBold[] = {'B','o','l','d',0};
  static const WCHAR wszItalic[] = {'I','t','a','l','i','c',0};
  static const WCHAR wszUnder[] = {'U','n','d','e','r','l','i','n','e',0};
  static const WCHAR wszStrike[] = {'S','t','r','i','k','e','t','h','r','o','u','g','h',0};
  static const WCHAR wszWeight[] = {'W','e','i','g','h','t',0};
  static const WCHAR wszCharset[] = {'C','h','a','r','s','s','e','t',0};
  static const LPCWSTR dispid_mapping[] =
  {
    wszName,
    NULL,
    wszSize,
    wszBold,
    wszItalic,
    wszUnder,
    wszStrike,
    wszWeight,
    wszCharset
  };

  IEnumConnections *pEnum;
  CONNECTDATA CD;
  HRESULT hres;

  this->gdiFont = 0;
  hres = IConnectionPoint_EnumConnections(this->pPropertyNotifyCP, &pEnum);
  if (SUCCEEDED(hres))
  {
    while(IEnumConnections_Next(pEnum, 1, &CD, NULL) == S_OK) {
      IPropertyNotifySink *sink;

      IUnknown_QueryInterface(CD.pUnk, &IID_IPropertyNotifySink, (LPVOID)&sink);
      IPropertyNotifySink_OnChanged(sink, dispID);
      IPropertyNotifySink_Release(sink);
      IUnknown_Release(CD.pUnk);
    }
    IEnumConnections_Release(pEnum);
  }

  hres = IConnectionPoint_EnumConnections(this->pFontEventsCP, &pEnum);
  if (SUCCEEDED(hres))
  {
    DISPPARAMS dispparams;
    VARIANTARG vararg;

    VariantInit(&vararg);
    V_VT(&vararg) = VT_BSTR;
    V_BSTR(&vararg) = SysAllocString(dispid_mapping[dispID]);

    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = &vararg;

    while(IEnumConnections_Next(pEnum, 1, &CD, NULL) == S_OK) {
        IFontEventsDisp *disp;

        IUnknown_QueryInterface(CD.pUnk, &IID_IFontEventsDisp, (LPVOID)&disp);
        IDispatch_Invoke(disp, DISPID_FONT_CHANGED, &IID_NULL,
                         LOCALE_NEUTRAL, INVOKE_FUNC, &dispparams, NULL,
                         NULL, NULL);

        IDispatch_Release(disp);
        IUnknown_Release(CD.pUnk);
    }
    VariantClear(&vararg);
    IEnumConnections_Release(pEnum);
  }
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
    *ppvObject = (IDispatch*)&(this->lpvtblIDispatch);
  if (IsEqualGUID(&IID_IFontDisp, riid))
    *ppvObject = (IDispatch*)&(this->lpvtblIDispatch);
  if (IsEqualIID(&IID_IPersist, riid) || IsEqualGUID(&IID_IPersistStream, riid))
    *ppvObject = (IPersistStream*)&(this->lpvtblIPersistStream);
  if (IsEqualGUID(&IID_IConnectionPointContainer, riid))
    *ppvObject = (IConnectionPointContainer*)&(this->lpvtblIConnectionPointContainer);
  if (IsEqualGUID(&IID_IPersistPropertyBag, riid))
    *ppvObject = (IPersistPropertyBag*)&(this->lpvtblIPersistPropertyBag);
  if (IsEqualGUID(&IID_IPersistStreamInit, riid))
    *ppvObject = (IPersistStreamInit*)&(this->lpvtblIPersistStreamInit);

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
  OLEFontImpl *this = (OLEFontImpl *)iface;
  TRACE("(%p)->(ref=%d)\n", this, this->ref);
  return InterlockedIncrement(&this->ref);
}

/************************************************************************
 * OLEFontImpl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
ULONG WINAPI OLEFontImpl_Release(
      IFont* iface)
{
  OLEFontImpl *this = (OLEFontImpl *)iface;
  ULONG ret;
  PHFONTItem ptr, next;
  TRACE("(%p)->(ref=%d)\n", this, this->ref);

  /* Decrease the reference count for current interface */
  ret = InterlockedDecrement(&this->ref);

  /* If the reference count goes down to 0, destroy. */
  if (ret == 0)
  {
    ULONG fontlist_refs = InterlockedDecrement(&ifont_cnt);
    /* Check if all HFONT list refs are zero */
    if (fontlist_refs == 0)
    {
      EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
      LIST_FOR_EACH_ENTRY_SAFE(ptr, next, &OLEFontImpl_hFontList, HFONTItem, entry)
        HFONTItem_Delete(ptr);
      LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);
    }
    OLEFontImpl_Destroy(this);
  }

  return ret;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
  TRACE("(%p)->(%d)\n", this, size.s.Lo);
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
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
    PHFONTItem newEntry;

    /*
     * The height of the font returned by the get_Size property is the
     * height of the font in points multiplied by 10000... Using some
     * simple conversions and the ratio given by the application, it can
     * be converted to a height in pixels.
     */
    IFont_get_Size(iface, &cySize);

    /* Standard ratio is 72 / 2540, or 18 / 635 in lowest terms. */
    /* Ratio is applied here relative to the standard. */
    fontHeight = MulDiv( cySize.s.Lo, this->cyLogical*635, this->cyHimetric*18 );

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

    /* Add font to the cache */
    newEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(HFONTItem));
    newEntry->ref = 1;
    newEntry->gdiFont = this->gdiFont;
    EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
    list_add_tail(&OLEFontImpl_hFontList,&newEntry->entry);
    LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);
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
  PHFONTItem newEntry;

  OLEFontImpl *this = (OLEFontImpl *)iface;
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

  fontHeight = MulDiv(cySize.s.Lo, this->cyLogical*635,this->cyHimetric*18);

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

  /* Add font to the cache */
  InterlockedIncrement(&ifont_cnt);
  newEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(HFONTItem));
  newEntry->ref = 1;
  newEntry->gdiFont = newObject->gdiFont;
  EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
  list_add_tail(&OLEFontImpl_hFontList,&newEntry->entry);
  LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);

  /* create new connection points */
  newObject->pPropertyNotifyCP = NULL;
  newObject->pFontEventsCP = NULL;
  CreateConnectionPoint((IUnknown*)newObject, &IID_IPropertyNotifySink, &newObject->pPropertyNotifyCP);
  CreateConnectionPoint((IUnknown*)newObject, &IID_IFontEventsDisp, &newObject->pFontEventsCP);

  if (!newObject->pPropertyNotifyCP || !newObject->pFontEventsCP)
  {
    OLEFontImpl_Destroy(newObject);
    return E_OUTOFMEMORY;
  }

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
  OLEFontImpl *left = (OLEFontImpl *)iface;
  OLEFontImpl *right = (OLEFontImpl *)pFontOther;
  INT ret;
  INT left_len,right_len;

  if((iface == NULL) || (pFontOther == NULL))
    return E_POINTER;
  else if (left->description.cySize.s.Lo != right->description.cySize.s.Lo)
    return S_FALSE;
  else if (left->description.cySize.s.Hi != right->description.cySize.s.Hi)
    return S_FALSE;
  else if (left->description.sWeight != right->description.sWeight)
    return S_FALSE;
  else if (left->description.sCharset != right->description.sCharset)
    return S_FALSE;
  else if (left->description.fItalic != right->description.fItalic)
    return S_FALSE;
  else if (left->description.fUnderline != right->description.fUnderline)
    return S_FALSE;
  else if (left->description.fStrikethrough != right->description.fStrikethrough)
    return S_FALSE;

  /* Check from string */
  left_len = strlenW(left->description.lpstrName);
  right_len = strlenW(right->description.lpstrName);
  ret = CompareStringW(0,0,left->description.lpstrName, left_len,
    right->description.lpstrName, right_len);
  if (ret != CSTR_EQUAL)
    return S_FALSE;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_SetRatio (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_SetRatio(
  IFont* iface,
  LONG   cyLogical,
  LONG   cyHimetric)
{
  OLEFontImpl *this = (OLEFontImpl *)iface;
  TRACE("(%p)->(%d, %d)\n", this, cyLogical, cyHimetric);

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
  HDC hdcRef;
  HFONT hOldFont, hNewFont;

  hdcRef = GetDC(0);
  OLEFontImpl_get_hFont(iface, &hNewFont);
  hOldFont = SelectObject(hdcRef, hNewFont);
  GetTextMetricsW(hdcRef, ptm);
  SelectObject(hdcRef, hOldFont);
  ReleaseDC(0, hdcRef);
  return S_OK;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
  PHFONTItem ptr, next;
  HRESULT hres = S_FALSE; /* assume not present */

  TRACE("(%p)->(%p)\n", this, hfont);

  if (!hfont)
    return E_INVALIDARG;

  /* Check of the hFont is already in the list */
  EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
  LIST_FOR_EACH_ENTRY_SAFE(ptr, next, &OLEFontImpl_hFontList, HFONTItem, entry)
  {
    if (ptr->gdiFont == hfont)
    {
      ptr->ref++;
      hres = S_OK;
      break;
    }
  }
  LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);

  return hres;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
  PHFONTItem ptr, next;
  HRESULT hres = S_FALSE; /* assume not present */

  TRACE("(%p)->(%p)\n", this, hfont);

  if (!hfont)
    return E_INVALIDARG;

  /* Check of the hFont is already in the list */
  EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
  LIST_FOR_EACH_ENTRY_SAFE(ptr, next, &OLEFontImpl_hFontList, HFONTItem, entry)
  {
    if ((ptr->gdiFont == hfont) && ptr->ref)
    {
      /* Remove from cache and delete object if not referenced */
      if (!--ptr->ref)
      {
        if (ptr->gdiFont == this->gdiFont)
          this->gdiFont = NULL;
      }
      hres = S_OK;
      break;
    }
  }
  LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);

  return hres;
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
  OLEFontImpl *this = (OLEFontImpl *)iface;
  FIXME("(%p)->(%p): Stub\n", this, hdc);
  return E_NOTIMPL;
}

/*
 * Virtual function tables for the OLEFontImpl class.
 */
static const IFontVtbl OLEFontImpl_VTable =
{
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
  OLEFontImpl *this = impl_from_IDispatch(iface);

  return IFont_QueryInterface((IFont *)this, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IDispatch_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IDispatch_Release(
  IDispatch* iface)
{
  OLEFontImpl *this = impl_from_IDispatch(iface);

  return IFont_Release((IFont *)this);
}

/************************************************************************
 * OLEFontImpl_IDispatch_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IDispatch_AddRef(
  IDispatch* iface)
{
  OLEFontImpl *this = impl_from_IDispatch(iface);

  return IFont_AddRef((IFont *)this);
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
  OLEFontImpl *this = impl_from_IDispatch(iface);
  TRACE("(%p)->(%p)\n", this, pctinfo);
  *pctinfo = 1;

  return S_OK;
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
  static const WCHAR stdole2tlb[] = {'s','t','d','o','l','e','2','.','t','l','b',0};
  ITypeLib *tl;
  HRESULT hres;

  OLEFontImpl *this = impl_from_IDispatch(iface);
  TRACE("(%p, iTInfo=%d, lcid=%04x, %p)\n", this, iTInfo, (int)lcid, ppTInfo);
  if (iTInfo != 0)
    return E_FAIL;
  hres = LoadTypeLib(stdole2tlb, &tl);
  if (FAILED(hres)) {
    ERR("Could not load the stdole2.tlb?\n");
    return hres;
  }
  hres = ITypeLib_GetTypeInfoOfGuid(tl, &IID_IFontDisp, ppTInfo);
  ITypeLib_Release(tl);
  if (FAILED(hres)) {
    FIXME("Did not IDispatch typeinfo from typelib, hres %x\n",hres);
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
  ITypeInfo * pTInfo;
  HRESULT hres;

  OLEFontImpl *this = impl_from_IDispatch(iface);

  TRACE("(%p,%s,%p,cNames=%d,lcid=%04x,%p)\n", this, debugstr_guid(riid),
        rgszNames, cNames, (int)lcid, rgDispId);

  if (cNames == 0)
  {
    return E_INVALIDARG;
  }
  else
  {
    /* retrieve type information */
    hres = OLEFontImpl_GetTypeInfo(iface, 0, lcid, &pTInfo);

    if (FAILED(hres))
    {
      ERR("GetTypeInfo failed.\n");
      return hres;
    }

    /* convert names to DISPIDs */
    hres = DispGetIDsOfNames (pTInfo, rgszNames, cNames, rgDispId);
    ITypeInfo_Release(pTInfo);

    return hres;
  }
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
  OLEFontImpl *this = impl_from_IDispatch(iface);
  HRESULT hr;

  TRACE("%p->(%d,%s,0x%x,0x%x,%p,%p,%p,%p)\n", this, dispIdMember,
    debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult, pExepInfo,
    puArgErr);

  /* validate parameters */

  if (!IsEqualIID(riid, &IID_NULL))
  {
    ERR("riid was %s instead of IID_NULL\n", debugstr_guid(riid));
    return DISP_E_UNKNOWNINTERFACE;
  }

  if (wFlags & DISPATCH_PROPERTYGET)
  {
    if (!pVarResult)
    {
      ERR("null pVarResult not allowed when DISPATCH_PROPERTYGET specified\n");
      return DISP_E_PARAMNOTOPTIONAL;
    }
  }
  else if (wFlags & DISPATCH_PROPERTYPUT)
  {
    if (!pDispParams)
    {
      ERR("null pDispParams not allowed when DISPATCH_PROPERTYPUT specified\n");
      return DISP_E_PARAMNOTOPTIONAL;
    }
    if (pDispParams->cArgs != 1)
    {
      ERR("param count for DISPATCH_PROPERTYPUT was %d instead of 1\n", pDispParams->cArgs);
      return DISP_E_BADPARAMCOUNT;
    }
  }
  else
  {
    ERR("one of DISPATCH_PROPERTYGET or DISPATCH_PROPERTYPUT must be specified\n");
    return DISP_E_MEMBERNOTFOUND;
  }

  switch (dispIdMember) {
  case DISPID_FONT_NAME:
    if (wFlags & DISPATCH_PROPERTYGET) {
      V_VT(pVarResult) = VT_BSTR;
      return IFont_get_Name((IFont *)this, &V_BSTR(pVarResult));
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BSTR);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Name((IFont *)this, V_BSTR(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_BOLD:
    if (wFlags & DISPATCH_PROPERTYGET) {
      BOOL value;
      hr = IFont_get_Bold((IFont *)this, &value);
      V_VT(pVarResult) = VT_BOOL;
      V_BOOL(pVarResult) = value ? VARIANT_TRUE : VARIANT_FALSE;
      return hr;
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BOOL);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Bold((IFont *)this, V_BOOL(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_ITALIC:
    if (wFlags & DISPATCH_PROPERTYGET) {
      BOOL value;
      hr = IFont_get_Italic((IFont *)this, &value);
      V_VT(pVarResult) = VT_BOOL;
      V_BOOL(pVarResult) = value ? VARIANT_TRUE : VARIANT_FALSE;
      return hr;
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BOOL);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Italic((IFont *)this, V_BOOL(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_UNDER:
    if (wFlags & DISPATCH_PROPERTYGET) {
      BOOL value;
      hr = IFont_get_Underline((IFont *)this, &value);
      V_VT(pVarResult) = VT_BOOL;
      V_BOOL(pVarResult) = value ? VARIANT_TRUE : VARIANT_FALSE;
      return hr;
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BOOL);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Underline((IFont *)this, V_BOOL(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_STRIKE:
    if (wFlags & DISPATCH_PROPERTYGET) {
      BOOL value;
      hr = IFont_get_Strikethrough((IFont *)this, &value);
      V_VT(pVarResult) = VT_BOOL;
      V_BOOL(pVarResult) = value ? VARIANT_TRUE : VARIANT_FALSE;
      return hr;
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BOOL);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Strikethrough((IFont *)this, V_BOOL(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_SIZE:
    if (wFlags & DISPATCH_PROPERTYGET) {
      V_VT(pVarResult) = VT_CY;
      return OLEFontImpl_get_Size((IFont *)this, &V_CY(pVarResult));
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_CY);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Size((IFont *)this, V_CY(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_WEIGHT:
    if (wFlags & DISPATCH_PROPERTYGET) {
      V_VT(pVarResult) = VT_I2;
      return OLEFontImpl_get_Weight((IFont *)this, &V_I2(pVarResult));
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_I2);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Weight((IFont *)this, V_I2(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_CHARSET:
    if (wFlags & DISPATCH_PROPERTYGET) {
      V_VT(pVarResult) = VT_I2;
      return OLEFontImpl_get_Charset((IFont *)this, &V_I2(pVarResult));
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_I2);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Charset((IFont *)this, V_I2(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  default:
    ERR("member not found for dispid 0x%x\n", dispIdMember);
    return DISP_E_MEMBERNOTFOUND;
  }
}

static const IDispatchVtbl OLEFontImpl_IDispatch_VTable =
{
  OLEFontImpl_IDispatch_QueryInterface,
  OLEFontImpl_IDispatch_AddRef,
  OLEFontImpl_IDispatch_Release,
  OLEFontImpl_GetTypeInfoCount,
  OLEFontImpl_GetTypeInfo,
  OLEFontImpl_GetIDsOfNames,
  OLEFontImpl_Invoke
};

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
  OLEFontImpl *this = impl_from_IPersistStream(iface);

  return IFont_QueryInterface((IFont *)this, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IPersistStream_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IPersistStream_Release(
  IPersistStream* iface)
{
  OLEFontImpl *this = impl_from_IPersistStream(iface);

  return IFont_Release((IFont *)this);
}

/************************************************************************
 * OLEFontImpl_IPersistStream_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IPersistStream_AddRef(
  IPersistStream* iface)
{
  OLEFontImpl *this = impl_from_IPersistStream(iface);

  return IFont_AddRef((IFont *)this);
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

  *pClassID = CLSID_StdFont;

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

  OLEFontImpl *this = impl_from_IPersistStream(iface);

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

  OLEFontImpl *this = impl_from_IPersistStream(iface);

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
  OLEFontImpl *this = impl_from_IPersistStream(iface);

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
      pcbSize->u.LowPart += WideCharToMultiByte( CP_ACP, 0, this->description.lpstrName,
                                                 strlenW(this->description.lpstrName),
                                                 NULL, 0, NULL, NULL );

  return S_OK;
}

static const IPersistStreamVtbl OLEFontImpl_IPersistStream_VTable =
{
  OLEFontImpl_IPersistStream_QueryInterface,
  OLEFontImpl_IPersistStream_AddRef,
  OLEFontImpl_IPersistStream_Release,
  OLEFontImpl_GetClassID,
  OLEFontImpl_IsDirty,
  OLEFontImpl_Load,
  OLEFontImpl_Save,
  OLEFontImpl_GetSizeMax
};

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
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);

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
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);

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
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);

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
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);

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
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);
  TRACE("(%p)->(%s, %p)\n", this, debugstr_guid(riid), ppCp);

  if(IsEqualIID(riid, &IID_IPropertyNotifySink)) {
    return IConnectionPoint_QueryInterface(this->pPropertyNotifyCP,
                                           &IID_IConnectionPoint,
                                           (LPVOID)ppCp);
  } else if(IsEqualIID(riid, &IID_IFontEventsDisp)) {
    return IConnectionPoint_QueryInterface(this->pFontEventsCP,
                                           &IID_IConnectionPoint,
                                           (LPVOID)ppCp);
  } else {
    FIXME("no connection point for %s\n", debugstr_guid(riid));
    return CONNECT_E_NOCONNECTION;
  }
}

static const IConnectionPointContainerVtbl
     OLEFontImpl_IConnectionPointContainer_VTable =
{
  OLEFontImpl_IConnectionPointContainer_QueryInterface,
  OLEFontImpl_IConnectionPointContainer_AddRef,
  OLEFontImpl_IConnectionPointContainer_Release,
  OLEFontImpl_EnumConnectionPoints,
  OLEFontImpl_FindConnectionPoint
};

/************************************************************************
 * OLEFontImpl implementation of IPersistPropertyBag.
 */
static HRESULT WINAPI OLEFontImpl_IPersistPropertyBag_QueryInterface(
   IPersistPropertyBag *iface, REFIID riid, LPVOID *ppvObj
) {
  OLEFontImpl *this = impl_from_IPersistPropertyBag(iface);
  return IFont_QueryInterface((IFont *)this,riid,ppvObj);
}

static ULONG WINAPI OLEFontImpl_IPersistPropertyBag_AddRef(
   IPersistPropertyBag *iface
) {
  OLEFontImpl *this = impl_from_IPersistPropertyBag(iface);
  return IFont_AddRef((IFont *)this);
}

static ULONG WINAPI OLEFontImpl_IPersistPropertyBag_Release(
   IPersistPropertyBag *iface
) {
  OLEFontImpl *this = impl_from_IPersistPropertyBag(iface);
  return IFont_Release((IFont *)this);
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
/* (from Visual Basic 6 property bag)
         Name            =   "MS Sans Serif"
         Size            =   13.8
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
*/
    static const WCHAR sAttrName[] = {'N','a','m','e',0};
    static const WCHAR sAttrSize[] = {'S','i','z','e',0};
    static const WCHAR sAttrCharset[] = {'C','h','a','r','s','e','t',0};
    static const WCHAR sAttrWeight[] = {'W','e','i','g','h','t',0};
    static const WCHAR sAttrUnderline[] = {'U','n','d','e','r','l','i','n','e',0};
    static const WCHAR sAttrItalic[] = {'I','t','a','l','i','c',0};
    static const WCHAR sAttrStrikethrough[] = {'S','t','r','i','k','e','t','h','r','o','u','g','h',0};
    VARIANT rawAttr;
    VARIANT valueAttr;
    HRESULT iRes = S_OK;
    OLEFontImpl *this = impl_from_IPersistPropertyBag(iface);

    VariantInit(&rawAttr);
    VariantInit(&valueAttr);

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrName, &rawAttr, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&rawAttr, &valueAttr, 0, VT_BSTR);
            if (iRes == S_OK)
                iRes = IFont_put_Name((IFont *)this, V_BSTR(&valueAttr));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;
        VariantClear(&rawAttr);
        VariantClear(&valueAttr);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrSize, &rawAttr, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&rawAttr, &valueAttr, 0, VT_CY);
            if (iRes == S_OK)
                iRes = IFont_put_Size((IFont *)this, V_CY(&valueAttr));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;
        VariantClear(&rawAttr);
        VariantClear(&valueAttr);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrCharset, &rawAttr, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&rawAttr, &valueAttr, 0, VT_I2);
            if (iRes == S_OK)
                iRes = IFont_put_Charset((IFont *)this, V_I2(&valueAttr));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;
        VariantClear(&rawAttr);
        VariantClear(&valueAttr);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrWeight, &rawAttr, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&rawAttr, &valueAttr, 0, VT_I2);
            if (iRes == S_OK)
                iRes = IFont_put_Weight((IFont *)this, V_I2(&valueAttr));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;
        VariantClear(&rawAttr);
        VariantClear(&valueAttr);

    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrUnderline, &rawAttr, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&rawAttr, &valueAttr, 0, VT_BOOL);
            if (iRes == S_OK)
                iRes = IFont_put_Underline((IFont *)this, V_BOOL(&valueAttr));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;
        VariantClear(&rawAttr);
        VariantClear(&valueAttr);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrItalic, &rawAttr, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&rawAttr, &valueAttr, 0, VT_BOOL);
            if (iRes == S_OK)
                iRes = IFont_put_Italic((IFont *)this, V_BOOL(&valueAttr));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;
        VariantClear(&rawAttr);
        VariantClear(&valueAttr);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrStrikethrough, &rawAttr, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&rawAttr, &valueAttr, 0, VT_BOOL);
            if (iRes == S_OK)
                IFont_put_Strikethrough((IFont *)this, V_BOOL(&valueAttr));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;
        VariantClear(&rawAttr);
        VariantClear(&valueAttr);
    }

    if (FAILED(iRes))
        WARN("-- 0x%08x\n", iRes);
    return iRes;
}

static HRESULT WINAPI OLEFontImpl_IPersistPropertyBag_Save(
   IPersistPropertyBag *iface, IPropertyBag* pPropBag, BOOL fClearDirty,
   BOOL fSaveAllProperties
) {
  FIXME("(%p,%p,%d,%d), stub!\n", iface, pPropBag, fClearDirty, fSaveAllProperties);
  return E_FAIL;
}

static const IPersistPropertyBagVtbl OLEFontImpl_IPersistPropertyBag_VTable = 
{
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
  OLEFontImpl *this = impl_from_IPersistStreamInit(iface);
  return IFont_QueryInterface((IFont *)this,riid,ppvObj);
}

static ULONG WINAPI OLEFontImpl_IPersistStreamInit_AddRef(
   IPersistStreamInit *iface
) {
  OLEFontImpl *this = impl_from_IPersistStreamInit(iface);
  return IFont_AddRef((IFont *)this);
}

static ULONG WINAPI OLEFontImpl_IPersistStreamInit_Release(
   IPersistStreamInit *iface
) {
  OLEFontImpl *this = impl_from_IPersistStreamInit(iface);
  return IFont_Release((IFont *)this);
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

static const IPersistStreamInitVtbl OLEFontImpl_IPersistStreamInit_VTable = 
{
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

/************************************************************************
 * OLEFontImpl_Construct
 *
 * This method will construct a new instance of the OLEFontImpl
 * class.
 *
 * The caller of this method must release the object when it's
 * done with it.
 */
static OLEFontImpl* OLEFontImpl_Construct(const FONTDESC *fontDesc)
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
  newObject->lpVtbl = &OLEFontImpl_VTable;
  newObject->lpvtblIDispatch = &OLEFontImpl_IDispatch_VTable;
  newObject->lpvtblIPersistStream = &OLEFontImpl_IPersistStream_VTable;
  newObject->lpvtblIConnectionPointContainer = &OLEFontImpl_IConnectionPointContainer_VTable;
  newObject->lpvtblIPersistPropertyBag = &OLEFontImpl_IPersistPropertyBag_VTable;
  newObject->lpvtblIPersistStreamInit = &OLEFontImpl_IPersistStreamInit_VTable;

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
  newObject->cyLogical  = 72L;
  newObject->cyHimetric = 2540L;
  newObject->pPropertyNotifyCP = NULL;
  newObject->pFontEventsCP = NULL;

  CreateConnectionPoint((IUnknown*)newObject, &IID_IPropertyNotifySink, &newObject->pPropertyNotifyCP);
  CreateConnectionPoint((IUnknown*)newObject, &IID_IFontEventsDisp, &newObject->pFontEventsCP);

  if (!newObject->pPropertyNotifyCP || !newObject->pFontEventsCP)
  {
    OLEFontImpl_Destroy(newObject);
    return NULL;
  }

  InterlockedIncrement(&ifont_cnt);

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

  HeapFree(GetProcessHeap(), 0, fontDesc->description.lpstrName);

  if (fontDesc->gdiFont!=0)
    DeleteObject(fontDesc->gdiFont);

  if (fontDesc->pPropertyNotifyCP)
      IConnectionPoint_Release(fontDesc->pPropertyNotifyCP);
  if (fontDesc->pFontEventsCP)
      IConnectionPoint_Release(fontDesc->pFontEventsCP);

  HeapFree(GetProcessHeap(), 0, fontDesc);
}

/*******************************************************************************
 * StdFont ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl    *lpVtbl;
    LONG                        ref;
} IClassFactoryImpl;

static HRESULT WINAPI
SFCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
SFCF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SFCF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	/* static class, won't be  freed */
	return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI SFCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	return OleCreateFontIndirect(NULL,riid,ppobj);

}

static HRESULT WINAPI SFCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static const IClassFactoryVtbl SFCF_Vtbl = {
	SFCF_QueryInterface,
	SFCF_AddRef,
	SFCF_Release,
	SFCF_CreateInstance,
	SFCF_LockServer
};
static IClassFactoryImpl STDFONT_CF = {&SFCF_Vtbl, 1 };

void _get_STDFONT_CF(LPVOID *ppv) { *ppv = (LPVOID)&STDFONT_CF; }
