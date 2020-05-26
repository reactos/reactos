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

static HDC olefont_hdc;

/***********************************************************************
 * List of the HFONTs it has given out, with each one having a separate
 * ref count.
 */
typedef struct _HFONTItem
{
  struct list entry;

  /* Reference count of any IFont objects that own this hfont */
  LONG int_refs;

  /* Total reference count of any refs held by the application obtained by AddRefHfont plus any internal refs */
  LONG total_refs;

  /* The font associated with this object. */
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

static HDC get_dc(void)
{
    HDC hdc;
    EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
    if(!olefont_hdc)
        olefont_hdc = CreateCompatibleDC(NULL);
    hdc = olefont_hdc;
    LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);
    return hdc;
}

static void delete_dc(void)
{
    EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
    if(olefont_hdc)
    {
        DeleteDC(olefont_hdc);
        olefont_hdc = NULL;
    }
    LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);
}

static void HFONTItem_Delete(PHFONTItem item)
{
  DeleteObject(item->gdiFont);
  list_remove(&item->entry);
  HeapFree(GetProcessHeap(), 0, item);
}

/* Find hfont item entry in the list.  Should be called while holding the crit sect */
static HFONTItem *find_hfontitem(HFONT hfont)
{
    HFONTItem *item;

    LIST_FOR_EACH_ENTRY(item, &OLEFontImpl_hFontList, HFONTItem, entry)
    {
        if (item->gdiFont == hfont)
            return item;
    }
    return NULL;
}

/* Add an item to the list with one internal reference */
static HRESULT add_hfontitem(HFONT hfont)
{
    HFONTItem *new_item = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_item));

    if(!new_item) return E_OUTOFMEMORY;

    new_item->int_refs = 1;
    new_item->total_refs = 1;
    new_item->gdiFont = hfont;
    EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
    list_add_tail(&OLEFontImpl_hFontList,&new_item->entry);
    LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);
    return S_OK;
}

static HRESULT inc_int_ref(HFONT hfont)
{
    HFONTItem *item;
    HRESULT hr = S_FALSE;

    EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
    item = find_hfontitem(hfont);

    if(item)
    {
        item->int_refs++;
        item->total_refs++;
        hr = S_OK;
    }
    LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);

    return hr;
}

/* decrements the internal ref of a hfont item.  If both refs are zero it'll
   remove the item from the list and delete the hfont */
static HRESULT dec_int_ref(HFONT hfont)
{
    HFONTItem *item;
    HRESULT hr = S_FALSE;

    EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
    item = find_hfontitem(hfont);

    if(item)
    {
        item->int_refs--;
        item->total_refs--;
        if(item->int_refs == 0 && item->total_refs == 0)
            HFONTItem_Delete(item);
        hr = S_OK;
    }
    LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);

    return hr;
}

static HRESULT inc_ext_ref(HFONT hfont)
{
    HFONTItem *item;
    HRESULT hr = S_FALSE;

    EnterCriticalSection(&OLEFontImpl_csHFONTLIST);

    item = find_hfontitem(hfont);
    if(item)
    {
        item->total_refs++;
        hr = S_OK;
    }
    LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);

    return hr;
}

static HRESULT dec_ext_ref(HFONT hfont)
{
    HFONTItem *item;
    HRESULT hr = S_FALSE;

    EnterCriticalSection(&OLEFontImpl_csHFONTLIST);

    item = find_hfontitem(hfont);
    if(item)
    {
        if(--item->total_refs >= 0) hr = S_OK;
    }
    LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);

    return hr;
}

static WCHAR *strdupW(const WCHAR* str)
{
    WCHAR *ret;
    DWORD size = (lstrlenW(str) + 1) * sizeof(WCHAR);

    ret = HeapAlloc(GetProcessHeap(), 0, size);
    if(ret)
        memcpy(ret, str, size);
    return ret;
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
  IFont                       IFont_iface;
  IDispatch                   IDispatch_iface;
  IPersistStream              IPersistStream_iface;
  IConnectionPointContainer   IConnectionPointContainer_iface;
  IPersistPropertyBag         IPersistPropertyBag_iface;
  IPersistStreamInit          IPersistStreamInit_iface;
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
  BOOL dirty;
  /*
   * Size ratio
   */
  LONG cyLogical;
  LONG cyHimetric;

  /*
   * Stash realized height (pixels) from TEXTMETRIC - used in get_Size()
   */
  LONG nRealHeight;

  IConnectionPoint *pPropertyNotifyCP;
  IConnectionPoint *pFontEventsCP;
};

static inline OLEFontImpl *impl_from_IFont(IFont *iface)
{
    return CONTAINING_RECORD(iface, OLEFontImpl, IFont_iface);
}

static inline OLEFontImpl *impl_from_IDispatch( IDispatch *iface )
{
    return CONTAINING_RECORD(iface, OLEFontImpl, IDispatch_iface);
}

static inline OLEFontImpl *impl_from_IPersistStream( IPersistStream *iface )
{
    return CONTAINING_RECORD(iface, OLEFontImpl, IPersistStream_iface);
}

static inline OLEFontImpl *impl_from_IConnectionPointContainer( IConnectionPointContainer *iface )
{
    return CONTAINING_RECORD(iface, OLEFontImpl, IConnectionPointContainer_iface);
}

static inline OLEFontImpl *impl_from_IPersistPropertyBag( IPersistPropertyBag *iface )
{
    return CONTAINING_RECORD(iface, OLEFontImpl, IPersistPropertyBag_iface);
}

static inline OLEFontImpl *impl_from_IPersistStreamInit( IPersistStreamInit *iface )
{
    return CONTAINING_RECORD(iface, OLEFontImpl, IPersistStreamInit_iface);
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
  OLEFontImpl* newFont;
  HRESULT      hr;
  FONTDESC     fd;

  TRACE("(%p, %s, %p)\n", lpFontDesc, debugstr_guid(riid), ppvObj);

  if (!ppvObj) return E_POINTER;

  *ppvObj = 0;

  if (!lpFontDesc) {
    static WCHAR fname[] = { 'S','y','s','t','e','m',0 };

    fd.cbSizeofstruct = sizeof(fd);
    fd.lpstrName      = fname;
    fd.cySize.s.Lo    = 80000;
    fd.cySize.s.Hi    = 0;
    fd.sWeight 	      = 0;
    fd.sCharset       = 0;
    fd.fItalic        = FALSE;
    fd.fUnderline     = FALSE;
    fd.fStrikethrough = FALSE;
    lpFontDesc = &fd;
  }

  newFont = OLEFontImpl_Construct(lpFontDesc);
  if (!newFont) return E_OUTOFMEMORY;

  hr = IFont_QueryInterface(&newFont->IFont_iface, riid, ppvObj);
  IFont_Release(&newFont->IFont_iface);

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
  static const WCHAR wszCharset[] = {'C','h','a','r','s','e','t',0};
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

  this->dirty = TRUE;

  hres = IConnectionPoint_EnumConnections(this->pPropertyNotifyCP, &pEnum);
  if (SUCCEEDED(hres))
  {
    while(IEnumConnections_Next(pEnum, 1, &CD, NULL) == S_OK) {
      IPropertyNotifySink *sink;

      IUnknown_QueryInterface(CD.pUnk, &IID_IPropertyNotifySink, (void**)&sink);
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

        IUnknown_QueryInterface(CD.pUnk, &IID_IFontEventsDisp, (void**)&disp);
        IFontEventsDisp_Invoke(disp, DISPID_FONT_CHANGED, &IID_NULL,
                               LOCALE_NEUTRAL, INVOKE_FUNC, &dispparams, NULL,
                               NULL, NULL);

        IFontEventsDisp_Release(disp);
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
static HRESULT WINAPI OLEFontImpl_QueryInterface(
  IFont*  iface,
  REFIID  riid,
  void**  ppvObject)
{
  OLEFontImpl *this = impl_from_IFont(iface);

  TRACE("(%p)->(%s, %p)\n", this, debugstr_guid(riid), ppvObject);

  *ppvObject = 0;

  if (IsEqualGUID(&IID_IUnknown, riid) ||
      IsEqualGUID(&IID_IFont, riid))
  {
    *ppvObject = this;
  }
  else if (IsEqualGUID(&IID_IDispatch, riid) ||
           IsEqualGUID(&IID_IFontDisp, riid))
  {
    *ppvObject = &this->IDispatch_iface;
  }
  else if (IsEqualGUID(&IID_IPersist, riid) ||
           IsEqualGUID(&IID_IPersistStream, riid))
  {
    *ppvObject = &this->IPersistStream_iface;
  }
  else if (IsEqualGUID(&IID_IConnectionPointContainer, riid))
  {
    *ppvObject = &this->IConnectionPointContainer_iface;
  }
  else if (IsEqualGUID(&IID_IPersistPropertyBag, riid))
  {
    *ppvObject = &this->IPersistPropertyBag_iface;
  }
  else if (IsEqualGUID(&IID_IPersistStreamInit, riid))
  {
    *ppvObject = &this->IPersistStreamInit_iface;
  }

  if (!*ppvObject)
  {
    FIXME("() : asking for unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  IFont_AddRef(iface);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_AddRef (IUnknown)
 */
static ULONG WINAPI OLEFontImpl_AddRef(
  IFont* iface)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(ref=%d)\n", this, this->ref);
  return InterlockedIncrement(&this->ref);
}

/************************************************************************
 * OLEFontImpl_Release (IUnknown)
 */
static ULONG WINAPI OLEFontImpl_Release(IFont* iface)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  ULONG ref;

  TRACE("(%p)->(ref=%d)\n", this, this->ref);

  ref = InterlockedDecrement(&this->ref);

  if (ref == 0)
  {
    ULONG fontlist_refs = InterlockedDecrement(&ifont_cnt);

    /* Final IFont object so destroy font cache */
    if (fontlist_refs == 0)
    {
      HFONTItem *item, *cursor2;

      EnterCriticalSection(&OLEFontImpl_csHFONTLIST);
      LIST_FOR_EACH_ENTRY_SAFE(item, cursor2, &OLEFontImpl_hFontList, HFONTItem, entry)
        HFONTItem_Delete(item);
      LeaveCriticalSection(&OLEFontImpl_csHFONTLIST);
      delete_dc();
    }
    else
    {
      dec_int_ref(this->gdiFont);
    }
    OLEFontImpl_Destroy(this);
  }

  return ref;
}

typedef struct
{
    short orig_cs;
    short avail_cs;
} enum_data;

static int CALLBACK font_enum_proc(const LOGFONTW *elf, const TEXTMETRICW *ntm, DWORD type, LPARAM lp)
{
    enum_data *data = (enum_data*)lp;

    if(elf->lfCharSet == data->orig_cs)
    {
        data->avail_cs = data->orig_cs;
        return 0;
    }
    if(data->avail_cs == -1) data->avail_cs = elf->lfCharSet;
    return 1;
}

static void realize_font(OLEFontImpl *This)
{
    LOGFONTW logFont;
    INT fontHeight;
    WCHAR text_face[LF_FACESIZE];
    HDC hdc = get_dc();
    HFONT old_font;
    TEXTMETRICW tm;

    if (!This->dirty) return;

    text_face[0] = 0;

    if(This->gdiFont)
    {
        old_font = SelectObject(hdc, This->gdiFont);
        GetTextFaceW(hdc, ARRAY_SIZE(text_face), text_face);
        SelectObject(hdc, old_font);
        dec_int_ref(This->gdiFont);
        This->gdiFont = 0;
    }

    memset(&logFont, 0, sizeof(LOGFONTW));

    lstrcpynW(logFont.lfFaceName, This->description.lpstrName, LF_FACESIZE);
    logFont.lfCharSet = This->description.sCharset;

    /* If the font name has been changed then enumerate all charsets
       and pick one that'll result in the font specified being selected */
    if(text_face[0] && lstrcmpiW(text_face, This->description.lpstrName))
    {
        enum_data data;
        data.orig_cs = This->description.sCharset;
        data.avail_cs = -1;
        logFont.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExW(get_dc(), &logFont, font_enum_proc, (LPARAM)&data, 0);
        if(data.avail_cs != -1) logFont.lfCharSet = data.avail_cs;
    }

    /*
     * The height of the font returned by the get_Size property is the
     * height of the font in points multiplied by 10000... Using some
     * simple conversions and the ratio given by the application, it can
     * be converted to a height in pixels.
     *
     * Standard ratio is 72 / 2540, or 18 / 635 in lowest terms.
     * Ratio is applied here relative to the standard.
     */

    fontHeight = MulDiv( This->description.cySize.s.Lo, This->cyLogical*635, This->cyHimetric*18 );

    logFont.lfHeight          = ((fontHeight%10000L)>5000L) ? (-fontHeight/10000L) - 1 :
                                                                  (-fontHeight/10000L);
    logFont.lfItalic          = This->description.fItalic;
    logFont.lfUnderline       = This->description.fUnderline;
    logFont.lfStrikeOut       = This->description.fStrikethrough;
    logFont.lfWeight          = This->description.sWeight;
    logFont.lfOutPrecision    = OUT_CHARACTER_PRECIS;
    logFont.lfClipPrecision   = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality         = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily  = DEFAULT_PITCH;

    This->gdiFont = CreateFontIndirectW(&logFont);
    This->dirty = FALSE;

    add_hfontitem(This->gdiFont);

    /* Fixup the name and charset properties so that they match the
       selected font */
    old_font = SelectObject(get_dc(), This->gdiFont);
    GetTextFaceW(hdc, ARRAY_SIZE(text_face), text_face);
    if(lstrcmpiW(text_face, This->description.lpstrName))
    {
        HeapFree(GetProcessHeap(), 0, This->description.lpstrName);
        This->description.lpstrName = strdupW(text_face);
    }
    GetTextMetricsW(hdc, &tm);
    This->description.sCharset = tm.tmCharSet;
    /* While we have it handy, stash the realized font height for use by get_Size() */
    This->nRealHeight = tm.tmHeight - tm.tmInternalLeading; /* corresponds to LOGFONT lfHeight */
    SelectObject(hdc, old_font);
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
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, pname);

  if (pname==0)
    return E_POINTER;

  realize_font(this);

  if (this->description.lpstrName!=0)
    *pname = SysAllocString(this->description.lpstrName);
  else
    *pname = 0;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Name (IFont)
 */
static HRESULT WINAPI OLEFontImpl_put_Name(
  IFont* iface,
  BSTR name)
{
  OLEFontImpl *This = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", This, name);

  if (!name)
    return CTL_E_INVALIDPROPERTYVALUE;

  HeapFree(GetProcessHeap(), 0, This->description.lpstrName);
  This->description.lpstrName = strdupW(name);
  if (!This->description.lpstrName) return E_OUTOFMEMORY;

  TRACE("new name %s\n", debugstr_w(This->description.lpstrName));
  OLEFont_SendNotify(This, DISPID_FONT_NAME);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Size (IFont)
 */
static HRESULT WINAPI OLEFontImpl_get_Size(
  IFont* iface,
  CY*    psize)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, psize);

  if (!psize) return E_POINTER;

  realize_font(this);

  /*
   * Convert realized font height in pixels to points descaled by current
   * scaling ratio then scaled up by 10000.
   */
  psize->s.Lo = MulDiv(this->nRealHeight,
                       this->cyHimetric * 72 * 10000,
                       this->cyLogical * 2540);
  psize->s.Hi = 0;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Size (IFont)
 */
static HRESULT WINAPI OLEFontImpl_put_Size(
  IFont* iface,
  CY     size)
{
  OLEFontImpl *this = impl_from_IFont(iface);
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
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, pbold);

  if (!pbold) return E_POINTER;

  realize_font(this);

  *pbold = this->description.sWeight > 550;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Bold (IFont)
 */
static HRESULT WINAPI OLEFontImpl_put_Bold(
  IFont* iface,
  BOOL bold)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%d)\n", this, bold);
  this->description.sWeight = bold ? FW_BOLD : FW_NORMAL;
  OLEFont_SendNotify(this, DISPID_FONT_BOLD);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Italic (IFont)
 */
static HRESULT WINAPI OLEFontImpl_get_Italic(
  IFont*  iface,
  BOOL* pitalic)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, pitalic);

  if (pitalic==0)
    return E_POINTER;

  realize_font(this);

  *pitalic = this->description.fItalic;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Italic (IFont)
 */
static HRESULT WINAPI OLEFontImpl_put_Italic(
  IFont* iface,
  BOOL italic)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%d)\n", this, italic);

  this->description.fItalic = italic;

  OLEFont_SendNotify(this, DISPID_FONT_ITALIC);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Underline (IFont)
 */
static HRESULT WINAPI OLEFontImpl_get_Underline(
  IFont*  iface,
  BOOL* punderline)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, punderline);

  if (punderline==0)
    return E_POINTER;

  realize_font(this);

  *punderline = this->description.fUnderline;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Underline (IFont)
 */
static HRESULT WINAPI OLEFontImpl_put_Underline(
  IFont* iface,
  BOOL underline)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%d)\n", this, underline);

  this->description.fUnderline = underline;

  OLEFont_SendNotify(this, DISPID_FONT_UNDER);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Strikethrough (IFont)
 */
static HRESULT WINAPI OLEFontImpl_get_Strikethrough(
  IFont*  iface,
  BOOL* pstrikethrough)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, pstrikethrough);

  if (pstrikethrough==0)
    return E_POINTER;

  realize_font(this);

  *pstrikethrough = this->description.fStrikethrough;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Strikethrough (IFont)
 */
static HRESULT WINAPI OLEFontImpl_put_Strikethrough(
 IFont* iface,
 BOOL strikethrough)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%d)\n", this, strikethrough);

  this->description.fStrikethrough = strikethrough;
  OLEFont_SendNotify(this, DISPID_FONT_STRIKE);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Weight (IFont)
 */
static HRESULT WINAPI OLEFontImpl_get_Weight(
  IFont* iface,
  short* pweight)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, pweight);

  if (pweight==0)
    return E_POINTER;

  realize_font(this);

  *pweight = this->description.sWeight;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Weight (IFont)
 */
static HRESULT WINAPI OLEFontImpl_put_Weight(
  IFont* iface,
  short  weight)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%d)\n", this, weight);

  this->description.sWeight = weight;

  OLEFont_SendNotify(this, DISPID_FONT_WEIGHT);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Charset (IFont)
 */
static HRESULT WINAPI OLEFontImpl_get_Charset(
  IFont* iface,
  short* pcharset)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, pcharset);

  if (pcharset==0)
    return E_POINTER;

  realize_font(this);

  *pcharset = this->description.sCharset;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Charset (IFont)
 */
static HRESULT WINAPI OLEFontImpl_put_Charset(
  IFont* iface,
  short charset)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%d)\n", this, charset);

  this->description.sCharset = charset;
  OLEFont_SendNotify(this, DISPID_FONT_CHARSET);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_hFont (IFont)
 */
static HRESULT WINAPI OLEFontImpl_get_hFont(
  IFont*   iface,
  HFONT* phfont)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%p)\n", this, phfont);
  if (phfont==NULL)
    return E_POINTER;

  realize_font(this);

  *phfont = this->gdiFont;
  TRACE("Returning %p\n", *phfont);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_Clone (IFont)
 */
static HRESULT WINAPI OLEFontImpl_Clone(
  IFont*  iface,
  IFont** ppfont)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  OLEFontImpl* newObject;

  TRACE("(%p)->(%p)\n", this, ppfont);

  if (ppfont == NULL)
    return E_POINTER;

  *ppfont = NULL;

  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(OLEFontImpl));
  if (newObject==NULL)
    return E_OUTOFMEMORY;

  *newObject = *this;
  /* allocate separate buffer */
  newObject->description.lpstrName = strdupW(this->description.lpstrName);

  /* Increment internal ref in hfont item list */
  if(newObject->gdiFont) inc_int_ref(newObject->gdiFont);

  InterlockedIncrement(&ifont_cnt);

  newObject->pPropertyNotifyCP = NULL;
  newObject->pFontEventsCP = NULL;
  CreateConnectionPoint((IUnknown*)&newObject->IFont_iface, &IID_IPropertyNotifySink,
                         &newObject->pPropertyNotifyCP);
  CreateConnectionPoint((IUnknown*)&newObject->IFont_iface, &IID_IFontEventsDisp,
                         &newObject->pFontEventsCP);

  if (!newObject->pPropertyNotifyCP || !newObject->pFontEventsCP)
  {
    OLEFontImpl_Destroy(newObject);
    return E_OUTOFMEMORY;
  }

  /* The cloned object starts with a reference count of 1 */
  newObject->ref = 1;

  *ppfont = &newObject->IFont_iface;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_IsEqual (IFont)
 */
static HRESULT WINAPI OLEFontImpl_IsEqual(
  IFont* iface,
  IFont* pFontOther)
{
  OLEFontImpl *left = impl_from_IFont(iface);
  OLEFontImpl *right = impl_from_IFont(pFontOther);
  INT ret;
  INT left_len,right_len;

  if(pFontOther == NULL)
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
  left_len = lstrlenW(left->description.lpstrName);
  right_len = lstrlenW(right->description.lpstrName);
  ret = CompareStringW(0,0,left->description.lpstrName, left_len,
    right->description.lpstrName, right_len);
  if (ret != CSTR_EQUAL)
    return S_FALSE;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_SetRatio (IFont)
 */
static HRESULT WINAPI OLEFontImpl_SetRatio(
  IFont* iface,
  LONG   cyLogical,
  LONG   cyHimetric)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  TRACE("(%p)->(%d, %d)\n", this, cyLogical, cyHimetric);

  if(cyLogical == 0 || cyHimetric == 0)
    return E_FAIL;

  /* cyLogical and cyHimetric both set to 1 is a special case that
     does not change the scaling but also does not fail */
  if(cyLogical == 1 && cyHimetric == 1)
    return S_OK;

  this->cyLogical  = cyLogical;
  this->cyHimetric = cyHimetric;
  this->dirty = TRUE;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_QueryTextMetrics (IFont)
 */
static HRESULT      WINAPI OLEFontImpl_QueryTextMetrics(
  IFont*         iface,
  TEXTMETRICOLE* ptm)
{
  HDC hdcRef;
  HFONT hOldFont, hNewFont;

  hdcRef = GetDC(0);
  IFont_get_hFont(iface, &hNewFont);
  hOldFont = SelectObject(hdcRef, hNewFont);
  GetTextMetricsW(hdcRef, ptm);
  SelectObject(hdcRef, hOldFont);
  ReleaseDC(0, hdcRef);
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_AddRefHfont (IFont)
 */
static HRESULT WINAPI OLEFontImpl_AddRefHfont(
  IFont*  iface,
  HFONT hfont)
{
    OLEFontImpl *this = impl_from_IFont(iface);

    TRACE("(%p)->(%p)\n", this, hfont);

    if (!hfont) return E_INVALIDARG;

    return inc_ext_ref(hfont);
}

/************************************************************************
 * OLEFontImpl_ReleaseHfont (IFont)
 */
static HRESULT WINAPI OLEFontImpl_ReleaseHfont(
  IFont*  iface,
  HFONT hfont)
{
    OLEFontImpl *this = impl_from_IFont(iface);

    TRACE("(%p)->(%p)\n", this, hfont);

    if (!hfont) return E_INVALIDARG;

    return dec_ext_ref(hfont);
}

/************************************************************************
 * OLEFontImpl_SetHdc (IFont)
 */
static HRESULT WINAPI OLEFontImpl_SetHdc(
  IFont* iface,
  HDC  hdc)
{
  OLEFontImpl *this = impl_from_IFont(iface);
  FIXME("(%p)->(%p): Stub\n", this, hdc);
  return E_NOTIMPL;
}

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
 */
static HRESULT WINAPI OLEFontImpl_IDispatch_QueryInterface(
  IDispatch* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  OLEFontImpl *this = impl_from_IDispatch(iface);
  return IFont_QueryInterface(&this->IFont_iface, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IDispatch_Release (IUnknown)
 */
static ULONG WINAPI OLEFontImpl_IDispatch_Release(
  IDispatch* iface)
{
  OLEFontImpl *this = impl_from_IDispatch(iface);
  return IFont_Release(&this->IFont_iface);
}

/************************************************************************
 * OLEFontImpl_IDispatch_AddRef (IUnknown)
 */
static ULONG WINAPI OLEFontImpl_IDispatch_AddRef(
  IDispatch* iface)
{
  OLEFontImpl *this = impl_from_IDispatch(iface);
  return IFont_AddRef(&this->IFont_iface);
}

/************************************************************************
 * OLEFontImpl_GetTypeInfoCount (IDispatch)
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

  if (cNames == 0) return E_INVALIDARG;

  hres = IDispatch_GetTypeInfo(iface, 0, lcid, &pTInfo);
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

/************************************************************************
 * OLEFontImpl_Invoke (IDispatch)
 * 
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
      return IFont_get_Name(&this->IFont_iface, &V_BSTR(pVarResult));
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BSTR);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Name(&this->IFont_iface, V_BSTR(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_BOLD:
    if (wFlags & DISPATCH_PROPERTYGET) {
      BOOL value;
      hr = IFont_get_Bold(&this->IFont_iface, &value);
      V_VT(pVarResult) = VT_BOOL;
      V_BOOL(pVarResult) = value ? VARIANT_TRUE : VARIANT_FALSE;
      return hr;
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BOOL);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Bold(&this->IFont_iface, V_BOOL(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_ITALIC:
    if (wFlags & DISPATCH_PROPERTYGET) {
      BOOL value;
      hr = IFont_get_Italic(&this->IFont_iface, &value);
      V_VT(pVarResult) = VT_BOOL;
      V_BOOL(pVarResult) = value ? VARIANT_TRUE : VARIANT_FALSE;
      return hr;
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BOOL);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Italic(&this->IFont_iface, V_BOOL(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_UNDER:
    if (wFlags & DISPATCH_PROPERTYGET) {
      BOOL value;
      hr = IFont_get_Underline(&this->IFont_iface, &value);
      V_VT(pVarResult) = VT_BOOL;
      V_BOOL(pVarResult) = value ? VARIANT_TRUE : VARIANT_FALSE;
      return hr;
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BOOL);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Underline(&this->IFont_iface, V_BOOL(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_STRIKE:
    if (wFlags & DISPATCH_PROPERTYGET) {
      BOOL value;
      hr = IFont_get_Strikethrough(&this->IFont_iface, &value);
      V_VT(pVarResult) = VT_BOOL;
      V_BOOL(pVarResult) = value ? VARIANT_TRUE : VARIANT_FALSE;
      return hr;
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_BOOL);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Strikethrough(&this->IFont_iface, V_BOOL(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_SIZE:
    if (wFlags & DISPATCH_PROPERTYGET) {
      V_VT(pVarResult) = VT_CY;
      return IFont_get_Size(&this->IFont_iface, &V_CY(pVarResult));
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_CY);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Size(&this->IFont_iface, V_CY(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_WEIGHT:
    if (wFlags & DISPATCH_PROPERTYGET) {
      V_VT(pVarResult) = VT_I2;
      return IFont_get_Weight(&this->IFont_iface, &V_I2(pVarResult));
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_I2);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Weight(&this->IFont_iface, V_I2(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_FONT_CHARSET:
    if (wFlags & DISPATCH_PROPERTYGET) {
      V_VT(pVarResult) = VT_I2;
      return OLEFontImpl_get_Charset(&this->IFont_iface, &V_I2(pVarResult));
    } else {
      VARIANTARG vararg;

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_I2);
      if (FAILED(hr))
        return hr;

      hr = IFont_put_Charset(&this->IFont_iface, V_I2(&vararg));

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
 */
static HRESULT WINAPI OLEFontImpl_IPersistStream_QueryInterface(
  IPersistStream* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  OLEFontImpl *this = impl_from_IPersistStream(iface);

  return IFont_QueryInterface(&this->IFont_iface, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IPersistStream_Release (IUnknown)
 */
static ULONG WINAPI OLEFontImpl_IPersistStream_Release(
  IPersistStream* iface)
{
  OLEFontImpl *this = impl_from_IPersistStream(iface);

  return IFont_Release(&this->IFont_iface);
}

/************************************************************************
 * OLEFontImpl_IPersistStream_AddRef (IUnknown)
 */
static ULONG WINAPI OLEFontImpl_IPersistStream_AddRef(
  IPersistStream* iface)
{
  OLEFontImpl *this = impl_from_IPersistStream(iface);

  return IFont_AddRef(&this->IFont_iface);
}

/************************************************************************
 * OLEFontImpl_GetClassID (IPersistStream)
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
  OLEFontImpl *this = impl_from_IPersistStream(iface);
  BYTE  version, attributes, string_size;
  char readBuffer[0x100];
  ULONG cbRead;
  INT len;

  /* Version */
  IStream_Read(pLoadStream, &version, sizeof(BYTE), &cbRead);
  if ((cbRead != sizeof(BYTE)) || (version != 0x01)) return E_FAIL;

  /* Charset */
  IStream_Read(pLoadStream, &this->description.sCharset, sizeof(WORD), &cbRead);
  if (cbRead != sizeof(WORD)) return E_FAIL;

  /* Attributes */
  IStream_Read(pLoadStream, &attributes, sizeof(BYTE), &cbRead);
  if (cbRead != sizeof(BYTE)) return E_FAIL;

  this->description.fItalic        = (attributes & FONTPERSIST_ITALIC) != 0;
  this->description.fStrikethrough = (attributes & FONTPERSIST_STRIKETHROUGH) != 0;
  this->description.fUnderline     = (attributes & FONTPERSIST_UNDERLINE) != 0;

  /* Weight */
  IStream_Read(pLoadStream, &this->description.sWeight, sizeof(WORD), &cbRead);
  if (cbRead != sizeof(WORD)) return E_FAIL;

  /* Size */
  IStream_Read(pLoadStream, &this->description.cySize.s.Lo, sizeof(DWORD), &cbRead);
  if (cbRead != sizeof(DWORD)) return E_FAIL;

  this->description.cySize.s.Hi = 0;

  /* Name */
  IStream_Read(pLoadStream, &string_size, sizeof(BYTE), &cbRead);
  if (cbRead != sizeof(BYTE)) return E_FAIL;

  IStream_Read(pLoadStream, readBuffer, string_size, &cbRead);
  if (cbRead != string_size) return E_FAIL;

  HeapFree(GetProcessHeap(), 0, this->description.lpstrName);

  len = MultiByteToWideChar( CP_ACP, 0, readBuffer, string_size, NULL, 0 );
  this->description.lpstrName = HeapAlloc( GetProcessHeap(), 0, (len+1) * sizeof(WCHAR) );
  MultiByteToWideChar( CP_ACP, 0, readBuffer, string_size, this->description.lpstrName, len );
  this->description.lpstrName[len] = 0;

  /* Ensure use of this font causes a new one to be created */
  dec_int_ref(this->gdiFont);
  this->gdiFont = 0;
  this->dirty = TRUE;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_Save (IPersistStream)
 */
static HRESULT WINAPI OLEFontImpl_Save(
  IPersistStream*  iface,
  IStream*         pOutStream,
  BOOL             fClearDirty)
{
  OLEFontImpl *this = impl_from_IPersistStream(iface);
  BYTE  attributes, string_size;
  const BYTE version = 0x01;
  char* writeBuffer = NULL;
  ULONG written;

  TRACE("(%p)->(%p %d)\n", this, pOutStream, fClearDirty);

  /* Version */
  IStream_Write(pOutStream, &version, sizeof(BYTE), &written);
  if (written != sizeof(BYTE)) return E_FAIL;

  /* Charset */
  IStream_Write(pOutStream, &this->description.sCharset, sizeof(WORD), &written);
  if (written != sizeof(WORD)) return E_FAIL;

  /* Attributes */
  attributes = 0;

  if (this->description.fItalic)
    attributes |= FONTPERSIST_ITALIC;

  if (this->description.fStrikethrough)
    attributes |= FONTPERSIST_STRIKETHROUGH;

  if (this->description.fUnderline)
    attributes |= FONTPERSIST_UNDERLINE;

  IStream_Write(pOutStream, &attributes, sizeof(BYTE), &written);
  if (written != sizeof(BYTE)) return E_FAIL;

  /* Weight */
  IStream_Write(pOutStream, &this->description.sWeight, sizeof(WORD), &written);
  if (written != sizeof(WORD)) return E_FAIL;

  /* Size */
  IStream_Write(pOutStream, &this->description.cySize.s.Lo, sizeof(DWORD), &written);
  if (written != sizeof(DWORD)) return E_FAIL;

  /* FontName */
  if (this->description.lpstrName)
    string_size = WideCharToMultiByte( CP_ACP, 0, this->description.lpstrName,
                                       lstrlenW(this->description.lpstrName), NULL, 0, NULL, NULL );
  else
    string_size = 0;

  IStream_Write(pOutStream, &string_size, sizeof(BYTE), &written);
  if (written != sizeof(BYTE)) return E_FAIL;

  if (string_size)
  {
      if (!(writeBuffer = HeapAlloc( GetProcessHeap(), 0, string_size ))) return E_OUTOFMEMORY;
      WideCharToMultiByte( CP_ACP, 0, this->description.lpstrName,
                           lstrlenW(this->description.lpstrName),
                           writeBuffer, string_size, NULL, NULL );

      IStream_Write(pOutStream, writeBuffer, string_size, &written);
      HeapFree(GetProcessHeap(), 0, writeBuffer);

      if (written != string_size) return E_FAIL;
  }

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_GetSizeMax (IPersistStream)
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
                                                 lstrlenW(this->description.lpstrName),
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
 */
static HRESULT WINAPI OLEFontImpl_IConnectionPointContainer_QueryInterface(
  IConnectionPointContainer* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);

  return IFont_QueryInterface(&this->IFont_iface, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IConnectionPointContainer_Release (IUnknown)
 */
static ULONG WINAPI OLEFontImpl_IConnectionPointContainer_Release(
  IConnectionPointContainer* iface)
{
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);

  return IFont_Release(&this->IFont_iface);
}

/************************************************************************
 * OLEFontImpl_IConnectionPointContainer_AddRef (IUnknown)
 */
static ULONG WINAPI OLEFontImpl_IConnectionPointContainer_AddRef(
  IConnectionPointContainer* iface)
{
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);

  return IFont_AddRef(&this->IFont_iface);
}

/************************************************************************
 * OLEFontImpl_EnumConnectionPoints (IConnectionPointContainer)
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
 */
static HRESULT WINAPI OLEFontImpl_FindConnectionPoint(
   IConnectionPointContainer* iface,
   REFIID riid,
   IConnectionPoint **ppCp)
{
  OLEFontImpl *this = impl_from_IConnectionPointContainer(iface);
  TRACE("(%p)->(%s, %p)\n", this, debugstr_guid(riid), ppCp);

  if(IsEqualIID(riid, &IID_IPropertyNotifySink)) {
    return IConnectionPoint_QueryInterface(this->pPropertyNotifyCP, &IID_IConnectionPoint,
                                           (void**)ppCp);
  } else if(IsEqualIID(riid, &IID_IFontEventsDisp)) {
    return IConnectionPoint_QueryInterface(this->pFontEventsCP, &IID_IConnectionPoint,
                                           (void**)ppCp);
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
  return IFont_QueryInterface(&this->IFont_iface,riid,ppvObj);
}

static ULONG WINAPI OLEFontImpl_IPersistPropertyBag_AddRef(
   IPersistPropertyBag *iface
) {
  OLEFontImpl *this = impl_from_IPersistPropertyBag(iface);
  return IFont_AddRef(&this->IFont_iface);
}

static ULONG WINAPI OLEFontImpl_IPersistPropertyBag_Release(
   IPersistPropertyBag *iface
) {
  OLEFontImpl *this = impl_from_IPersistPropertyBag(iface);
  return IFont_Release(&this->IFont_iface);
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
    OLEFontImpl *this = impl_from_IPersistPropertyBag(iface);
    VARIANT value;
    HRESULT iRes;

    VariantInit(&value);

    iRes = IPropertyBag_Read(pPropBag, sAttrName, &value, pErrorLog);
    if (iRes == S_OK)
    {
        iRes = VariantChangeType(&value, &value, 0, VT_BSTR);
        if (iRes == S_OK)
            iRes = IFont_put_Name(&this->IFont_iface, V_BSTR(&value));
    }
    else if (iRes == E_INVALIDARG)
        iRes = S_OK;

    VariantClear(&value);

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrSize, &value, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&value, &value, 0, VT_CY);
            if (iRes == S_OK)
                iRes = IFont_put_Size(&this->IFont_iface, V_CY(&value));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;

        VariantClear(&value);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrCharset, &value, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&value, &value, 0, VT_I2);
            if (iRes == S_OK)
                iRes = IFont_put_Charset(&this->IFont_iface, V_I2(&value));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;

        VariantClear(&value);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrWeight, &value, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&value, &value, 0, VT_I2);
            if (iRes == S_OK)
                iRes = IFont_put_Weight(&this->IFont_iface, V_I2(&value));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;

        VariantClear(&value);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrUnderline, &value, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&value, &value, 0, VT_BOOL);
            if (iRes == S_OK)
                iRes = IFont_put_Underline(&this->IFont_iface, V_BOOL(&value));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;

        VariantClear(&value);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrItalic, &value, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&value, &value, 0, VT_BOOL);
            if (iRes == S_OK)
                iRes = IFont_put_Italic(&this->IFont_iface, V_BOOL(&value));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;

        VariantClear(&value);
    }

    if (iRes == S_OK) {
        iRes = IPropertyBag_Read(pPropBag, sAttrStrikethrough, &value, pErrorLog);
        if (iRes == S_OK)
        {
            iRes = VariantChangeType(&value, &value, 0, VT_BOOL);
            if (iRes == S_OK)
                IFont_put_Strikethrough(&this->IFont_iface, V_BOOL(&value));
        }
        else if (iRes == E_INVALIDARG)
            iRes = S_OK;

        VariantClear(&value);
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
  return IFont_QueryInterface(&this->IFont_iface,riid,ppvObj);
}

static ULONG WINAPI OLEFontImpl_IPersistStreamInit_AddRef(
   IPersistStreamInit *iface
) {
  OLEFontImpl *this = impl_from_IPersistStreamInit(iface);
  return IFont_AddRef(&this->IFont_iface);
}

static ULONG WINAPI OLEFontImpl_IPersistStreamInit_Release(
   IPersistStreamInit *iface
) {
  OLEFontImpl *this = impl_from_IPersistStreamInit(iface);
  return IFont_Release(&this->IFont_iface);
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
  OLEFontImpl* newObject;

  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(OLEFontImpl));

  if (newObject==0)
    return newObject;

  newObject->IFont_iface.lpVtbl = &OLEFontImpl_VTable;
  newObject->IDispatch_iface.lpVtbl = &OLEFontImpl_IDispatch_VTable;
  newObject->IPersistStream_iface.lpVtbl = &OLEFontImpl_IPersistStream_VTable;
  newObject->IConnectionPointContainer_iface.lpVtbl = &OLEFontImpl_IConnectionPointContainer_VTable;
  newObject->IPersistPropertyBag_iface.lpVtbl = &OLEFontImpl_IPersistPropertyBag_VTable;
  newObject->IPersistStreamInit_iface.lpVtbl = &OLEFontImpl_IPersistStreamInit_VTable;

  newObject->ref = 1;

  newObject->description.cbSizeofstruct = sizeof(FONTDESC);
  newObject->description.lpstrName      = strdupW(fontDesc->lpstrName);
  newObject->description.cySize         = fontDesc->cySize;
  newObject->description.sWeight        = fontDesc->sWeight;
  newObject->description.sCharset       = fontDesc->sCharset;
  newObject->description.fItalic        = fontDesc->fItalic;
  newObject->description.fUnderline     = fontDesc->fUnderline;
  newObject->description.fStrikethrough = fontDesc->fStrikethrough;

  newObject->gdiFont  = 0;
  newObject->dirty = TRUE;
  newObject->cyLogical  = GetDeviceCaps(get_dc(), LOGPIXELSY);
  newObject->cyHimetric = 2540L;
  newObject->pPropertyNotifyCP = NULL;
  newObject->pFontEventsCP = NULL;

  CreateConnectionPoint((IUnknown*)&newObject->IFont_iface, &IID_IPropertyNotifySink, &newObject->pPropertyNotifyCP);
  CreateConnectionPoint((IUnknown*)&newObject->IFont_iface, &IID_IFontEventsDisp, &newObject->pFontEventsCP);

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
    IClassFactory IClassFactory_iface;
    LONG          ref;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
        return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI SFCF_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(&IID_IClassFactory, riid) || IsEqualIID(&IID_IUnknown, riid))
    {
        *obj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI
SFCF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SFCF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
	/* static class, won't be  freed */
	return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI SFCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	return OleCreateFontIndirect(NULL,riid,ppobj);

}

static HRESULT WINAPI SFCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
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
static IClassFactoryImpl STDFONT_CF = {{&SFCF_Vtbl}, 1 };

void _get_STDFONT_CF(LPVOID *ppv) { *ppv = &STDFONT_CF; }
