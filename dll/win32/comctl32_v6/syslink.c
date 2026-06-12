/*
 * SysLink control
 *
 * Copyright 2004 - 2006 Thomas Weidenmueller <w3seek@reactos.com>
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

#define COBJMACROS
#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "oaidl.h"
#include "initguid.h"
#include "oleacc.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(syslink);

typedef struct
{
    int nChars;
    int nSkip;
    RECT rc;
} DOC_TEXTBLOCK, *PDOC_TEXTBLOCK;

#define LIF_FLAGSMASK   (LIF_STATE | LIF_ITEMID | LIF_URL)
#define LIS_MASK        (LIS_FOCUSED | LIS_ENABLED | LIS_VISITED)

typedef enum
{
    slText = 0,
    slLink
} SL_ITEM_TYPE;

typedef struct _DOC_ITEM
{
    struct list entry;
    UINT nText;             /* Number of characters of the text */
    SL_ITEM_TYPE Type;      /* type of the item */
    PDOC_TEXTBLOCK Blocks;  /* Array of text blocks */
    union
    {
        struct
        {
            UINT state;     /* Link state */
            WCHAR *szID;    /* Link ID string */
            WCHAR *szUrl;   /* Link URL string */
        } Link;
        struct
        {
            UINT Dummy;
        } Text;
    } u;
    WCHAR Text[1];          /* Text of the document item */
} DOC_ITEM, *PDOC_ITEM;

typedef struct SYSLINK_INFO SYSLINK_INFO;

typedef struct
{
    IAccessible IAccessible_iface;
    IOleWindow IOleWindow_iface;
    struct SYSLINK_INFO *infoPtr;
    LONG refcount;
} SYSLINK_ACC;

struct SYSLINK_INFO
{
    HWND      Self;         /* The window handle for this control */
    HWND      Notify;       /* The parent handle to receive notifications */
    DWORD     Style;        /* Styles for this control */
    struct list Items;      /* Document items list */
    BOOL      HasFocus;     /* Whether the control has the input focus */
    int       MouseDownID;  /* ID of the link that the mouse button first selected */
    HFONT     Font;         /* Handle to the font for text */
    HFONT     LinkFont;     /* Handle to the font for links */
    COLORREF  TextColor;    /* Color of the text */
    COLORREF  LinkColor;    /* Color of links */
    COLORREF  VisitedColor; /* Color of visited links */
    WCHAR     BreakChar;    /* Break Character for the current font */
    BOOL      IgnoreReturn; /* (infoPtr->Style & LWS_IGNORERETURN) on creation */
    SYSLINK_ACC *AccessibleImpl; /* IAccessible implementation */
};

/* Control configuration constants */

#define SL_LEFTMARGIN   (0)
#define SL_TOPMARGIN    (0)
#define SL_RIGHTMARGIN  (0)
#define SL_BOTTOMMARGIN (0)

static inline SYSLINK_ACC *impl_from_IAccessible(IAccessible *iface)
{
    return CONTAINING_RECORD(iface, SYSLINK_ACC, IAccessible_iface);
}

static inline SYSLINK_ACC *impl_from_IOleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, SYSLINK_ACC, IOleWindow_iface);
}

static HRESULT WINAPI Accessible_QueryInterface(IAccessible *iface, REFIID iid, void **ppv)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IAccessible, iid))
    {
        *ppv = &This->IAccessible_iface;
    }
    else if (IsEqualIID(&IID_IOleWindow, iid))
    {
        *ppv = &This->IOleWindow_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Accessible_AddRef(IAccessible *iface)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    return InterlockedIncrement(&This->refcount);
}

static ULONG WINAPI Accessible_Release(IAccessible *iface)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    ULONG ref = InterlockedDecrement(&This->refcount);

    if (ref == 0)
        Free(This);

    return ref;
}

static HRESULT WINAPI Accessible_GetTypeInfo(IAccessible *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_GetIDsOfNames(IAccessible *iface, REFIID iid, LPOLESTR *names, UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_Invoke(IAccessible *iface, DISPID dispid, REFIID iid, LCID lcid,
    WORD flags, DISPPARAMS *dispparams, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_GetTypeInfoCount(IAccessible *iface, UINT *count)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT Accessible_FindChild(SYSLINK_ACC *This, VARIANT childid, DOC_ITEM** result)
{
    DOC_ITEM *current;
    int index;

    if (!This->infoPtr)
    {
        WARN("control was destroyed\n");
        return E_FAIL;
    }

    if (V_VT(&childid) == VT_EMPTY)
        index = 0;
    else if (V_VT(&childid) == VT_I4)
        index = V_I4(&childid);
    else {
        WARN("not implemented for vt %s\n", debugstr_vt(V_VT(&childid)));
        return E_INVALIDARG;
    }

    if (index == 0)
    {
        *result = NULL;
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(current, &This->infoPtr->Items, DOC_ITEM, entry)
    {
        if (current->Type != slLink)
            continue;
        if (!--index)
        {
            *result = current;
            return S_OK;
        }
    }

    WARN("index out of range\n");
    return E_INVALIDARG;
}

static HRESULT WINAPI Accessible_get_accParent(IAccessible *iface, IDispatch** disp)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accChildCount(IAccessible *iface, LONG *count)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    DOC_ITEM *current;
    LONG result = 0;

    TRACE("%p\n", iface);

    if (!This->infoPtr)
    {
        WARN("control was destroyed\n");
        return E_FAIL;
    }

    LIST_FOR_EACH_ENTRY(current, &This->infoPtr->Items, DOC_ITEM, entry)
    {
        if (current->Type == slLink)
            result++;
    }

    *count = result;

    return S_OK;
}

static HRESULT WINAPI Accessible_get_accChild(IAccessible *iface, VARIANT childid, IDispatch **disp)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    HRESULT hr;
    DOC_ITEM* item;

    TRACE("%p, %s\n", iface, debugstr_variant(&childid));

    *disp = NULL;

    hr = Accessible_FindChild(This, childid, &item);
    if (FAILED(hr))
        return hr;

    if (item)
        return S_FALSE;
    else
        return E_INVALIDARG;
}

static HRESULT WINAPI Accessible_get_accName(IAccessible *iface, VARIANT childid, BSTR *name)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    HRESULT hr;
    DOC_ITEM* item;
    BSTR result;

    TRACE("%p, %s\n", iface, debugstr_variant(&childid));

    if (!name)
        return E_POINTER;

    *name = NULL;

    hr = Accessible_FindChild(This, childid, &item);
    if (FAILED(hr))
        return hr;

    if (item)
    {
        result = SysAllocString(item->Text);
        if (!result)
            return E_OUTOFMEMORY;
    }
    else
    {
        UINT total_length = 0, i;

        LIST_FOR_EACH_ENTRY(item, &This->infoPtr->Items, DOC_ITEM, entry)
        {
            total_length += item->nText;
        }

        result = SysAllocStringLen(NULL, total_length);
        if (!result)
            return E_OUTOFMEMORY;

        i = 0;
        LIST_FOR_EACH_ENTRY(item, &This->infoPtr->Items, DOC_ITEM, entry)
        {
            memcpy(&result[i], item->Text, item->nText * sizeof(*result));
            i += item->nText;
        }
    }

    *name = result;

    return S_OK;
}

static HRESULT WINAPI Accessible_get_accValue(IAccessible *iface, VARIANT childid, BSTR *value)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accDescription(IAccessible *iface, VARIANT childid, BSTR *description)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accRole(IAccessible *iface, VARIANT childid, VARIANT *role)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    HRESULT hr;
    DOC_ITEM* item;

    TRACE("%p, %s\n", iface, debugstr_variant(&childid));

    hr = Accessible_FindChild(This, childid, &item);
    if (FAILED(hr))
        return hr;

    V_VT(role) = VT_I4;

    if (item)
        V_I4(role) = ROLE_SYSTEM_LINK;
    else
        V_I4(role) = ROLE_SYSTEM_CLIENT;

    return S_OK;
}

static HRESULT WINAPI Accessible_get_accState(IAccessible *iface, VARIANT childid, VARIANT *state)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    HRESULT hr;
    DOC_ITEM* item;
    GUITHREADINFO info;
    BOOL focused = 0;

    TRACE("%p, %s\n", iface, debugstr_variant(&childid));

    hr = Accessible_FindChild(This, childid, &item);
    if (FAILED(hr))
        return hr;

    V_VT(state) = VT_I4;
    V_I4(state) = 0;

    info.cbSize = sizeof(info);
    if(GetGUIThreadInfo(0, &info) && info.hwndFocus == This->infoPtr->Self)
        focused = 1;

    if (item)
    {
        V_I4(state) |= STATE_SYSTEM_FOCUSABLE|STATE_SYSTEM_LINKED;
        if (focused && (item->u.Link.state & LIS_FOCUSED) == LIS_FOCUSED)
            V_I4(state) |= STATE_SYSTEM_FOCUSED;
    }
    else
    {
        LONG style = GetWindowLongW(This->infoPtr->Self, GWL_STYLE);

        if (style & WS_DISABLED)
            V_I4(state) |= STATE_SYSTEM_UNAVAILABLE;
        else
            V_I4(state) |= STATE_SYSTEM_FOCUSABLE;
        if (!(style & WS_VISIBLE))
            V_I4(state) |= STATE_SYSTEM_INVISIBLE;
        if (focused)
            V_I4(state) |= STATE_SYSTEM_FOCUSED;
    }

    return S_OK;
}

static HRESULT WINAPI Accessible_get_accHelp(IAccessible *iface, VARIANT childid, BSTR *help)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accHelpTopic(IAccessible *iface, BSTR *helpFile, VARIANT childid, LONG *topic)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accKeyboardShortcut(IAccessible *iface, VARIANT childid, BSTR *shortcut)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accFocus(IAccessible *iface, VARIANT *childid)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accSelection(IAccessible *iface, VARIANT *childid)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accDefaultAction(IAccessible *iface, VARIANT childid, BSTR *action)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    HRESULT hr;
    DOC_ITEM* item;

    TRACE("%p, %s\n", iface, debugstr_variant(&childid));

    if (!action)
        return E_POINTER;

    *action = NULL;

    hr = Accessible_FindChild(This, childid, &item);
    if (FAILED(hr))
        return hr;

    if (item)
    {
        *action = SysAllocString(L"Click");
        if (!*action)
            return E_OUTOFMEMORY;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

static HRESULT WINAPI Accessible_accSelect(IAccessible *iface, LONG flags, VARIANT childid)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accLocation(IAccessible *iface, LONG *left, LONG *top, LONG *width, LONG *height, VARIANT childid)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    HRESULT hr;
    DOC_ITEM* item;
    RECT rc = {0};
    POINT point;

    TRACE("%p, %s\n", iface, debugstr_variant(&childid));

    hr = Accessible_FindChild(This, childid, &item);
    if (FAILED(hr))
        return hr;

    if (item)
    {
        int n = item->nText;
        PDOC_TEXTBLOCK block = item->Blocks;

        while (n > 0)
        {
            UnionRect(&rc, &rc, &block->rc);
            n -= block->nChars + block->nSkip;
            block++;
        }
    }
    else
    {
        GetClientRect(This->infoPtr->Self, &rc);
    }

    point.x = rc.left;
    point.y = rc.top;
    MapWindowPoints(This->infoPtr->Self, NULL, &point, 1);
    *left = point.x;
    *top = point.y;
    *width = rc.right - rc.left;
    *height = rc.bottom - rc.top;

    TRACE("<-- (%li,%li,%li,%li)\n", *left, *top, *width, *height);

    return S_OK;
}

static HRESULT WINAPI Accessible_accNavigate(IAccessible *iface, LONG dir, VARIANT start, VARIANT *end)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accHitTest(IAccessible *iface, LONG left, LONG top, VARIANT* childid)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static LRESULT SYSLINK_SendParentNotify (const SYSLINK_INFO *infoPtr, UINT code, const DOC_ITEM *Link, int iLink);

static HRESULT WINAPI Accessible_accDoDefaultAction(IAccessible *iface, VARIANT childid)
{
    SYSLINK_ACC *This = impl_from_IAccessible(iface);
    HRESULT hr;
    DOC_ITEM* item;

    TRACE("%p, %s\n", iface, debugstr_variant(&childid));

    hr = Accessible_FindChild(This, childid, &item);
    if (FAILED(hr))
        return hr;

    if (!item)
        /* Not supported for whole control. */
        return E_INVALIDARG;

    SYSLINK_SendParentNotify(This->infoPtr, NM_CLICK, item, V_I4(&childid) - 1);

    return S_OK;
}

static HRESULT WINAPI Accessible_put_accName(IAccessible *iface, VARIANT childid, BSTR name)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_put_accValue(IAccessible *iface, VARIANT childid, BSTR value)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static const IAccessibleVtbl Accessible_Vtbl = {
    Accessible_QueryInterface,
    Accessible_AddRef,
    Accessible_Release,
    Accessible_GetTypeInfoCount,
    Accessible_GetTypeInfo,
    Accessible_GetIDsOfNames,
    Accessible_Invoke,
    Accessible_get_accParent,
    Accessible_get_accChildCount,
    Accessible_get_accChild,
    Accessible_get_accName,
    Accessible_get_accValue,
    Accessible_get_accDescription,
    Accessible_get_accRole,
    Accessible_get_accState,
    Accessible_get_accHelp,
    Accessible_get_accHelpTopic,
    Accessible_get_accKeyboardShortcut,
    Accessible_get_accFocus,
    Accessible_get_accSelection,
    Accessible_get_accDefaultAction,
    Accessible_accSelect,
    Accessible_accLocation,
    Accessible_accNavigate,
    Accessible_accHitTest,
    Accessible_accDoDefaultAction,
    Accessible_put_accName,
    Accessible_put_accValue
};

static HRESULT WINAPI Accessible_Window_QueryInterface(IOleWindow *iface, REFIID iid, void **ppv)
{
    SYSLINK_ACC *This = impl_from_IOleWindow(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, iid, ppv);
}

static ULONG WINAPI Accessible_Window_AddRef(IOleWindow *iface)
{
    SYSLINK_ACC *This = impl_from_IOleWindow(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI Accessible_Window_Release(IOleWindow *iface)
{
    SYSLINK_ACC *This = impl_from_IOleWindow(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI Accessible_GetWindow(IOleWindow *iface, HWND *hwnd)
{
    SYSLINK_ACC *This = impl_from_IOleWindow(iface);

    TRACE("%p\n", This);

    if (!This->infoPtr)
        return E_FAIL;

    *hwnd = This->infoPtr->Self;
    return S_OK;
}

static HRESULT WINAPI Accessible_ContextSensitiveHelp(IOleWindow *This, BOOL fEnterMode)
{
    FIXME("%p\b", This);
    return E_NOTIMPL;
}

static const IOleWindowVtbl Accessible_Window_Vtbl = {
    Accessible_Window_QueryInterface,
    Accessible_Window_AddRef,
    Accessible_Window_Release,
    Accessible_GetWindow,
    Accessible_ContextSensitiveHelp
};

static void Accessible_Create(SYSLINK_INFO* infoPtr)
{
    SYSLINK_ACC *This;

    This = Alloc(sizeof(*This));
    if (!This) return;

    This->IAccessible_iface.lpVtbl = &Accessible_Vtbl;
    This->IOleWindow_iface.lpVtbl = &Accessible_Window_Vtbl;
    This->infoPtr = infoPtr;
    This->refcount = 1;
    infoPtr->AccessibleImpl = This;
}

static void Accessible_WindowDestroyed(SYSLINK_ACC *This)
{
    This->infoPtr = NULL;
    IAccessible_Release(&This->IAccessible_iface);
}

/***********************************************************************
 * SYSLINK_FreeDocItem
 * Frees all data and gdi objects associated with a document item
 */
static VOID SYSLINK_FreeDocItem (PDOC_ITEM DocItem)
{
    if(DocItem->Type == slLink)
    {
        Free(DocItem->u.Link.szID);
        Free(DocItem->u.Link.szUrl);
    }

    Free(DocItem->Blocks);

    /* we don't free Text because it's just a pointer to a character in the
       entire window text string */

    Free(DocItem);
}

/***********************************************************************
 * SYSLINK_AppendDocItem
 * Create and append a new document item.
 */
static PDOC_ITEM SYSLINK_AppendDocItem (SYSLINK_INFO *infoPtr, LPCWSTR Text, UINT textlen,
                                        SL_ITEM_TYPE type, PDOC_ITEM LastItem)
{
    PDOC_ITEM Item;

    textlen = min(textlen, lstrlenW(Text));
    Item = Alloc(FIELD_OFFSET(DOC_ITEM, Text[textlen + 1]));
    if(Item == NULL)
    {
        ERR("Failed to alloc DOC_ITEM structure!\n");
        return NULL;
    }

    Item->nText = textlen;
    Item->Type = type;
    Item->Blocks = NULL;
    lstrcpynW(Item->Text, Text, textlen + 1);
    if (LastItem)
        list_add_after(&LastItem->entry, &Item->entry);
    else
        list_add_tail(&infoPtr->Items, &Item->entry);

    return Item;
}

/***********************************************************************
 * SYSLINK_ClearDoc
 * Clears the document tree
 */
static VOID SYSLINK_ClearDoc (SYSLINK_INFO *infoPtr)
{
    DOC_ITEM *Item, *Item2;

    LIST_FOR_EACH_ENTRY_SAFE(Item, Item2, &infoPtr->Items, DOC_ITEM, entry)
    {
        list_remove(&Item->entry);
        SYSLINK_FreeDocItem(Item);
    }
}

/***********************************************************************
 * SYSLINK_ParseText
 * Parses the window text string and creates a document. Returns the
 * number of document items created.
 */
static UINT SYSLINK_ParseText (SYSLINK_INFO *infoPtr, LPCWSTR Text)
{
    LPCWSTR current, textstart = NULL, linktext = NULL, firsttag = NULL;
    int taglen = 0, textlen = 0, linklen = 0, docitems = 0;
    PDOC_ITEM Last = NULL;
    SL_ITEM_TYPE CurrentType = slText;
    LPCWSTR lpID, lpUrl;
    UINT lenId, lenUrl;

    TRACE("(%p %s)\n", infoPtr, debugstr_w(Text));

    for(current = Text; *current != 0;)
    {
        if(*current == '<')
        {
            if(!wcsnicmp(current, L"<a", 2) && (CurrentType == slText))
            {
                BOOL ValidParam = FALSE, ValidLink = FALSE;

                if(*(current + 2) == '>')
                {
                    /* we just have to deal with a <a> tag */
                    taglen = 3;
                    ValidLink = TRUE;
                    ValidParam = TRUE;
                    firsttag = current;
                    linklen = 0;
                    lpID = NULL;
                    lpUrl = NULL;
                }
                else if(*(current + 2) == infoPtr->BreakChar)
                {
                    /* we expect parameters, parse them */
                    LPCWSTR *CurrentParameter = NULL, tmp;
                    UINT *CurrentParameterLen = NULL;

                    taglen = 3;
                    tmp = current + taglen;
                    lpID = NULL;
                    lpUrl = NULL;
                    
CheckParameter:
                    /* compare the current position with all known parameters */
                    if(!wcsnicmp(tmp, L"href=\"", 6))
                    {
                        taglen += 6;
                        ValidParam = TRUE;
                        CurrentParameter = &lpUrl;
                        CurrentParameterLen = &lenUrl;
                    }
                    else if(!wcsnicmp(tmp, L"id=\"", 4))
                    {
                        taglen += 4;
                        ValidParam = TRUE;
                        CurrentParameter = &lpID;
                        CurrentParameterLen = &lenId;
                    }
                    else
                    {
                        ValidParam = FALSE;
                    }
                    
                    if(ValidParam)
                    {
                        /* we got a known parameter, now search until the next " character.
                           If we can't find a " character, there's a syntax error and we just assume it's text */
                        ValidParam = FALSE;
                        *CurrentParameter = current + taglen;
                        *CurrentParameterLen = 0;

                        for(tmp = *CurrentParameter; *tmp != 0; tmp++)
                        {
                            taglen++;
                            if(*tmp == '\"')
                            {
                                ValidParam = TRUE;
                                tmp++;
                                break;
                            }
                            (*CurrentParameterLen)++;
                        }
                    }
                    if(ValidParam)
                    {
                        /* we're done with this parameter, now there are only 2 possibilities:
                         * 1. another parameter is coming, so expect a ' ' (space) character
                         * 2. the tag is being closed, so expect a '<' character
                         */
                        if(*tmp == infoPtr->BreakChar)
                        {
                            /* we expect another parameter, do the whole thing again */
                            taglen++;
                            tmp++;
                            goto CheckParameter;
                        }
                        else if(*tmp == '>')
                        {
                            /* the tag is being closed, we're done */
                            ValidLink = TRUE;
                            taglen++;
                        }
                    }
                }
                
                if(ValidLink && ValidParam)
                {
                    /* the <a ...> tag appears to be valid. save all information
                       so we can add the link if we find a valid </a> tag later */
                    CurrentType = slLink;
                    linktext = current + taglen;
                    linklen = 0;
                    firsttag = current;
                }
                else
                {
                    taglen = 1;
                    lpID = NULL;
                    lpUrl = NULL;
                    if(textstart == NULL)
                    {
                        textstart = current;
                    }
                }
            }
            else if (!wcsnicmp(current, L"</a>", 4) && (CurrentType == slLink) && firsttag)
            {
                /* there's a <a...> tag opened, first add the previous text, if present */
                if(textstart != NULL && textlen > 0 && firsttag > textstart)
                {
                    Last = SYSLINK_AppendDocItem(infoPtr, textstart, firsttag - textstart, slText, Last);
                    if(Last == NULL)
                    {
                        ERR("Unable to create new document item!\n");
                        return docitems;
                    }
                    docitems++;
                    textstart = NULL;
                    textlen = 0;
                }
                
                /* now it's time to add the link to the document */
                current += 4;
                if(linktext != NULL && linklen > 0)
                {
                    Last = SYSLINK_AppendDocItem(infoPtr, linktext, linklen, slLink, Last);
                    if(Last == NULL)
                    {
                        ERR("Unable to create new document item!\n");
                        return docitems;
                    }
                    docitems++;
                    if(CurrentType == slLink)
                    {
                        int nc;

                        if(!(infoPtr->Style & WS_DISABLED))
                        {
                            Last->u.Link.state |= LIS_ENABLED;
                        }
                        /* Copy the tag parameters */
                        if(lpID != NULL)
                        {
                            nc = min(lenId, lstrlenW(lpID));
                            nc = min(nc, MAX_LINKID_TEXT - 1);
                            Last->u.Link.szID = Alloc((nc + 1) * sizeof(WCHAR));
                            if(Last->u.Link.szID != NULL)
                            {
                                lstrcpynW(Last->u.Link.szID, lpID, nc + 1);
                            }
                        }
                        else
                            Last->u.Link.szID = NULL;
                        if(lpUrl != NULL)
                        {
                            nc = min(lenUrl, lstrlenW(lpUrl));
                            nc = min(nc, L_MAX_URL_LENGTH - 1);
                            Last->u.Link.szUrl = Alloc((nc + 1) * sizeof(WCHAR));
                            if(Last->u.Link.szUrl != NULL)
                            {
                                lstrcpynW(Last->u.Link.szUrl, lpUrl, nc + 1);
                            }
                        }
                        else
                            Last->u.Link.szUrl = NULL;
                    }
                    linktext = NULL;
                }
                CurrentType = slText;
                firsttag = NULL;
                textstart = NULL;
                continue;
            }
            else
            {
                /* we don't know what tag it is, so just continue */
                taglen = 1;
                linklen++;
                if(CurrentType == slText && textstart == NULL)
                {
                    textstart = current;
                }
            }
            
            textlen += taglen;
            current += taglen;
        }
        else
        {
            textlen++;
            linklen++;

            /* save the pointer of the current text item if we couldn't find a tag */
            if(textstart == NULL && CurrentType == slText)
            {
                textstart = current;
            }
            
            current++;
        }
    }
    
    if(textstart != NULL && textlen > 0)
    {
        Last = SYSLINK_AppendDocItem(infoPtr, textstart, textlen, CurrentType, Last);
        if(Last == NULL)
        {
            ERR("Unable to create new document item!\n");
            return docitems;
        }
        if(CurrentType == slLink)
        {
            int nc;

            if(!(infoPtr->Style & WS_DISABLED))
            {
                Last->u.Link.state |= LIS_ENABLED;
            }
            /* Copy the tag parameters */
            if(lpID != NULL)
            {
                nc = min(lenId, lstrlenW(lpID));
                nc = min(nc, MAX_LINKID_TEXT - 1);
                Last->u.Link.szID = Alloc((nc + 1) * sizeof(WCHAR));
                if(Last->u.Link.szID != NULL)
                {
                    lstrcpynW(Last->u.Link.szID, lpID, nc + 1);
                }
            }
            else
                Last->u.Link.szID = NULL;
            if(lpUrl != NULL)
            {
                nc = min(lenUrl, lstrlenW(lpUrl));
                nc = min(nc, L_MAX_URL_LENGTH - 1);
                Last->u.Link.szUrl = Alloc((nc + 1) * sizeof(WCHAR));
                if(Last->u.Link.szUrl != NULL)
                {
                    lstrcpynW(Last->u.Link.szUrl, lpUrl, nc + 1);
                }
            }
            else
                Last->u.Link.szUrl = NULL;
        }
        docitems++;
    }

    if(linktext != NULL && linklen > 0)
    {
        /* we got an unclosed link, just display the text */
        Last = SYSLINK_AppendDocItem(infoPtr, linktext, linklen, slText, Last);
        if(Last == NULL)
        {
            ERR("Unable to create new document item!\n");
            return docitems;
        }
        docitems++;
    }

    return docitems;
}

/***********************************************************************
 * SYSLINK_RepaintLink
 * Repaints a link.
 */
static VOID SYSLINK_RepaintLink (const SYSLINK_INFO *infoPtr, const DOC_ITEM *DocItem)
{
    PDOC_TEXTBLOCK bl;
    int n;

    if(DocItem->Type != slLink)
    {
        ERR("DocItem not a link!\n");
        return;
    }
    
    bl = DocItem->Blocks;
    if (bl != NULL)
    {
        n = DocItem->nText;
        
        while(n > 0)
        {
            InvalidateRect(infoPtr->Self, &bl->rc, TRUE);
            n -= bl->nChars + bl->nSkip;
            bl++;
        }
    }
}

/***********************************************************************
 * SYSLINK_GetLinkItemByIndex
 * Retrieves a document link by its index
 */
static PDOC_ITEM SYSLINK_GetLinkItemByIndex (const SYSLINK_INFO *infoPtr, int iLink)
{
    DOC_ITEM *Current;

    LIST_FOR_EACH_ENTRY(Current, &infoPtr->Items, DOC_ITEM, entry)
    {
        if ((Current->Type == slLink) && (iLink-- <= 0))
            return Current;
    }
    return NULL;
}

/***********************************************************************
 * SYSLINK_GetFocusLink
 * Retrieves the link that has the LIS_FOCUSED bit
 */
static PDOC_ITEM SYSLINK_GetFocusLink (const SYSLINK_INFO *infoPtr, int *LinkId)
{
    DOC_ITEM *Current;
    int id = 0;

    LIST_FOR_EACH_ENTRY(Current, &infoPtr->Items, DOC_ITEM, entry)
    {
        if(Current->Type == slLink)
        {
            if(Current->u.Link.state & LIS_FOCUSED)
            {
                if(LinkId != NULL)
                    *LinkId = id;
                return Current;
            }
            id++;
        }
    }

    return NULL;
}

/***********************************************************************
 * SYSLINK_GetNextLink
 * Gets the next link
 */
static PDOC_ITEM SYSLINK_GetNextLink (const SYSLINK_INFO *infoPtr, PDOC_ITEM Current)
{
    DOC_ITEM *Next;

    LIST_FOR_EACH_ENTRY(Next, Current ? &Current->entry : &infoPtr->Items, DOC_ITEM, entry)
    {
        if (Next->Type == slLink)
        {
            return Next;
        }
    }
    return NULL;
}

/***********************************************************************
 * SYSLINK_GetPrevLink
 * Gets the previous link
 */
static PDOC_ITEM SYSLINK_GetPrevLink (const SYSLINK_INFO *infoPtr, PDOC_ITEM Current)
{
    DOC_ITEM *Prev;

    LIST_FOR_EACH_ENTRY_REV(Prev, Current ? &Current->entry : list_tail(&infoPtr->Items), DOC_ITEM, entry)
    {
        if (Prev->Type == slLink)
        {
            return Prev;
        }
    }

    return NULL;
}

/***********************************************************************
 * SYSLINK_WrapLine
 * Tries to wrap a line.
 */
static BOOL SYSLINK_WrapLine (LPWSTR Text, WCHAR BreakChar, int x, int *LineLen,
                             int nFit, LPSIZE Extent)
{
    int i;

    for (i = 0; i < nFit; i++) if (Text[i] == '\r' || Text[i] == '\n') break;

    if (i == *LineLen) return FALSE;

    /* check if we're in the middle of a word */
    if (Text[i] != '\r' && Text[i] != '\n' && Text[i] != BreakChar)
    {
        /* search for the beginning of the word */
        while (i && Text[i - 1] != BreakChar) i--;

        if (i == 0)
        {
            Extent->cx = 0;
            Extent->cy = 0;
            if (x == SL_LEFTMARGIN) i = max( nFit, 1 );
        }
    }
    *LineLen = i;
    return TRUE;
}

/***********************************************************************
 * SYSLINK_Render
 * Renders the document in memory
 */
static VOID SYSLINK_Render (const SYSLINK_INFO *infoPtr, HDC hdc, PRECT pRect)
{
    RECT rc;
    PDOC_ITEM Current;
    HGDIOBJ hOldFont;
    int x, y, LineHeight;
    SIZE szDoc;
    TEXTMETRICW tm;

    szDoc.cx = szDoc.cy = 0;

    rc = *pRect;
    rc.right -= SL_RIGHTMARGIN;
    rc.bottom -= SL_BOTTOMMARGIN;

    if(rc.right - SL_LEFTMARGIN < 0)
        rc.right = MAXLONG;
    if (rc.bottom - SL_TOPMARGIN < 0)
        rc.bottom = MAXLONG;
    
    hOldFont = SelectObject(hdc, infoPtr->Font);
    
    x = SL_LEFTMARGIN;
    y = SL_TOPMARGIN;
    GetTextMetricsW( hdc, &tm );
    LineHeight = tm.tmHeight + tm.tmExternalLeading;

    LIST_FOR_EACH_ENTRY(Current, &infoPtr->Items, DOC_ITEM, entry)
    {
        int n, nBlocks;
        LPWSTR tx;
        PDOC_TEXTBLOCK bl, cbl;
        INT nFit;
        SIZE szDim;
        int SkipChars = 0;

        if(Current->nText == 0)
        {
            continue;
        }

        tx = Current->Text;
        n = Current->nText;

        Free(Current->Blocks);
        Current->Blocks = NULL;
        bl = NULL;
        nBlocks = 0;

        if(Current->Type == slText)
        {
            SelectObject(hdc, infoPtr->Font);
        }
        else if(Current->Type == slLink)
        {
            SelectObject(hdc, infoPtr->LinkFont);
        }
        
        while(n > 0)
        {
            /* skip break characters unless they're the first of the doc item */
            if(tx != Current->Text || x == SL_LEFTMARGIN)
            {
                if (n && *tx == '\r')
                {
                    tx++;
                    SkipChars++;
                    n--;
                }
                if (n && *tx == '\n')
                {
                    tx++;
                    SkipChars++;
                    n--;
                }
                while(n > 0 && (*tx) == infoPtr->BreakChar)
                {
                    tx++;
                    SkipChars++;
                    n--;
                }
            }

            if((n == 0 && SkipChars != 0) ||
               GetTextExtentExPointW(hdc, tx, n, rc.right - x, &nFit, NULL, &szDim))
            {
                int LineLen = n;
                BOOL Wrap = FALSE;
                PDOC_TEXTBLOCK nbl;
                
                if(n != 0)
                {
                    Wrap = SYSLINK_WrapLine(tx, infoPtr->BreakChar, x, &LineLen, nFit, &szDim);

                    if(LineLen == 0)
                    {
                        /* move one line down, the word didn't fit into the line */
                        x = SL_LEFTMARGIN;
                        y += LineHeight;
                        continue;
                    }

                    if(LineLen != n)
                    {
                        if(!GetTextExtentExPointW(hdc, tx, LineLen, rc.right - x, NULL, NULL, &szDim))
                        {
                            if(bl != NULL)
                            {
                                Free(bl);
                                bl = NULL;
                                nBlocks = 0;
                            }
                            break;
                        }
                    }
                }
                
                nbl = ReAlloc(bl, (nBlocks + 1) * sizeof(DOC_TEXTBLOCK));
                if (nbl != NULL)
                {
                    bl = nbl;
                    nBlocks++;

                    cbl = bl + nBlocks - 1;
                    
                    cbl->nChars = LineLen;
                    cbl->nSkip = SkipChars;
                    SetRect(&cbl->rc, x, y, x + szDim.cx, y + szDim.cy);

                    if (cbl->rc.right > szDoc.cx)
                        szDoc.cx = cbl->rc.right;
                    if (cbl->rc.bottom > szDoc.cy)
                        szDoc.cy = cbl->rc.bottom;

                    if(LineLen != 0)
                    {
                        x += szDim.cx;
                        if(Wrap)
                        {
                            x = SL_LEFTMARGIN;
                            y += LineHeight;
                        }
                    }
                }
                else
                {
                    Free(bl);
                    bl = NULL;
                    nBlocks = 0;

                    ERR("Failed to alloc DOC_TEXTBLOCK structure!\n");
                    break;
                }
                n -= LineLen;
                tx += LineLen;
                SkipChars = 0;
            }
            else
            {
                n--;
            }
        }

        if(nBlocks != 0)
        {
            Current->Blocks = bl;
        }
    }
    
    SelectObject(hdc, hOldFont);

    pRect->right = pRect->left + szDoc.cx;
    pRect->bottom = pRect->top + szDoc.cy;
}

/***********************************************************************
 * SYSLINK_Draw
 * Draws the SysLink control.
 */
static LRESULT SYSLINK_Draw (const SYSLINK_INFO *infoPtr, HDC hdc)
{
    RECT rc;
    PDOC_ITEM Current;
    HFONT hOldFont;
    COLORREF OldTextColor, OldBkColor;
    HBRUSH hBrush;
    UINT text_flags = ETO_CLIPPED;
    UINT mode = GetBkMode( hdc );

    hOldFont = SelectObject(hdc, infoPtr->Font);
    OldTextColor = SetTextColor(hdc, infoPtr->TextColor);
    OldBkColor = SetBkColor(hdc, comctl32_color.clrWindow);

    GetClientRect(infoPtr->Self, &rc);
    rc.right -= SL_RIGHTMARGIN + SL_LEFTMARGIN;
    rc.bottom -= SL_BOTTOMMARGIN + SL_TOPMARGIN;

    if(rc.right < 0 || rc.bottom < 0) return 0;

    hBrush = (HBRUSH)SendMessageW(infoPtr->Notify, WM_CTLCOLORSTATIC,
                                  (WPARAM)hdc, (LPARAM)infoPtr->Self);
    if (!(infoPtr->Style & LWS_TRANSPARENT))
    {
        FillRect(hdc, &rc, hBrush);
        if (GetBkMode( hdc ) == OPAQUE) text_flags |= ETO_OPAQUE;
    }
    else SetBkMode( hdc, TRANSPARENT );

#ifndef __REACTOS__
    DeleteObject(hBrush);
#endif

    LIST_FOR_EACH_ENTRY(Current, &infoPtr->Items, DOC_ITEM, entry)
    {
        int n;
        LPWSTR tx;
        PDOC_TEXTBLOCK bl;
        
        bl = Current->Blocks;
        if(bl != NULL)
        {
            tx = Current->Text;
            n = Current->nText;

            if(Current->Type == slText)
            {
                 SelectObject(hdc, infoPtr->Font);
                 SetTextColor(hdc, infoPtr->TextColor);
            }
            else
            {
                 SelectObject(hdc, infoPtr->LinkFont);
                 SetTextColor(hdc, (!(Current->u.Link.state & LIS_VISITED) ? infoPtr->LinkColor : infoPtr->VisitedColor));
            }

            while(n > 0)
            {
                tx += bl->nSkip;
                ExtTextOutW(hdc, bl->rc.left, bl->rc.top, text_flags, &bl->rc, tx, bl->nChars, NULL);
                if((Current->Type == slLink) && (Current->u.Link.state & LIS_FOCUSED) && infoPtr->HasFocus)
                {
                    COLORREF PrevTextColor;
                    PrevTextColor = SetTextColor(hdc, infoPtr->TextColor);
                    DrawFocusRect(hdc, &bl->rc);
                    SetTextColor(hdc, PrevTextColor);
                }
                tx += bl->nChars;
                n -= bl->nChars + bl->nSkip;
                bl++;
            }
        }
    }

    SetBkColor(hdc, OldBkColor);
    SetTextColor(hdc, OldTextColor);
    SelectObject(hdc, hOldFont);
    SetBkMode(hdc, mode);
    return 0;
}


/***********************************************************************
 * SYSLINK_Paint
 * Handles the WM_PAINT message.
 */
static LRESULT SYSLINK_Paint (const SYSLINK_INFO *infoPtr, HDC hdcParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = hdcParam ? hdcParam : BeginPaint (infoPtr->Self, &ps);
    if (hdc)
    {
        SYSLINK_Draw (infoPtr, hdc);
        if (!hdcParam) EndPaint (infoPtr->Self, &ps);
    }
    return 0;
}

/***********************************************************************
 *           SYSLINK_SetFont
 * Set new Font for the SysLink control.
 */
static HFONT SYSLINK_SetFont (SYSLINK_INFO *infoPtr, HFONT hFont, BOOL bRedraw)
{
    HDC hdc;
    LOGFONTW lf;
    TEXTMETRICW tm;
    RECT rcClient;
    HFONT hOldFont = infoPtr->Font;
    infoPtr->Font = hFont;
    
    /* free the underline font */
    if(infoPtr->LinkFont != NULL)
    {
        DeleteObject(infoPtr->LinkFont);
        infoPtr->LinkFont = NULL;
    }

    /* Render text position and word wrapping in memory */
    if (GetClientRect(infoPtr->Self, &rcClient))
    {
        hdc = GetDC(infoPtr->Self);
        if(hdc != NULL)
        {
            /* create a new underline font */
            if(GetTextMetricsW(hdc, &tm) &&
               GetObjectW(infoPtr->Font, sizeof(LOGFONTW), &lf))
            {
                lf.lfUnderline = TRUE;
                infoPtr->LinkFont = CreateFontIndirectW(&lf);
                infoPtr->BreakChar = tm.tmBreakChar;
            }
            else
            {
                ERR("Failed to create link font!\n");
            }

            SYSLINK_Render(infoPtr, hdc, &rcClient);
            ReleaseDC(infoPtr->Self, hdc);
        }
    }
    
    if(bRedraw)
    {
        RedrawWindow(infoPtr->Self, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    
    return hOldFont;
}

/***********************************************************************
 *           SYSLINK_SetText
 * Set new text for the SysLink control.
 */
static LRESULT SYSLINK_SetText (SYSLINK_INFO *infoPtr, LPCWSTR Text)
{
    /* clear the document */
    SYSLINK_ClearDoc(infoPtr);

    if(Text == NULL || *Text == 0)
    {
        return TRUE;
    }

    /* let's parse the string and create a document */
    if(SYSLINK_ParseText(infoPtr, Text) > 0)
    {
        RECT rcClient;

        /* Render text position and word wrapping in memory */
        if (GetClientRect(infoPtr->Self, &rcClient))
        {
            HDC hdc = GetDC(infoPtr->Self);
            if (hdc != NULL)
            {
                SYSLINK_Render(infoPtr, hdc, &rcClient);
                ReleaseDC(infoPtr->Self, hdc);

                InvalidateRect(infoPtr->Self, NULL, TRUE);
            }
        }
    }
    
    return TRUE;
}

/***********************************************************************
 *           SYSLINK_SetFocusLink
 * Updates the focus status bits and focuses the specified link.
 * If no document item is specified, the focus bit will be removed from all links.
 * Returns the previous focused item.
 */
static PDOC_ITEM SYSLINK_SetFocusLink (const SYSLINK_INFO *infoPtr, const DOC_ITEM *DocItem)
{
    PDOC_ITEM Current, PrevFocus = NULL;

    LIST_FOR_EACH_ENTRY(Current, &infoPtr->Items, DOC_ITEM, entry)
    {
        if(Current->Type == slLink)
        {
            if((PrevFocus == NULL) && (Current->u.Link.state & LIS_FOCUSED))
            {
                PrevFocus = Current;
            }
            
            if(Current == DocItem)
            {
                Current->u.Link.state |= LIS_FOCUSED;
            }
            else
            {
                Current->u.Link.state &= ~LIS_FOCUSED;
            }
        }
    }
    
    return PrevFocus;
}

/***********************************************************************
 *           SYSLINK_SetItem
 * Sets the states and attributes of a link item.
 */
static LRESULT SYSLINK_SetItem (const SYSLINK_INFO *infoPtr, const LITEM *Item)
{
    PDOC_ITEM di;
    int nc;
    PWSTR szId = NULL;
    PWSTR szUrl = NULL;
    BOOL Repaint = FALSE;

    if(!(Item->mask & LIF_ITEMINDEX) || !(Item->mask & (LIF_FLAGSMASK)))
    {
        ERR("Invalid Flags!\n");
        return FALSE;
    }

    di = SYSLINK_GetLinkItemByIndex(infoPtr, Item->iLink);
    if(di == NULL)
    {
        ERR("Link %d couldn't be found\n", Item->iLink);
        return FALSE;
    }

    if(Item->mask & LIF_ITEMID)
    {
        nc = min(lstrlenW(Item->szID), MAX_LINKID_TEXT - 1);
        szId = Alloc((nc + 1) * sizeof(WCHAR));
        if(szId)
        {
            lstrcpynW(szId, Item->szID, nc + 1);
        }
        else
        {
            ERR("Unable to allocate memory for link id\n");
            return FALSE;
        }
    }

    if(Item->mask & LIF_URL)
    {
        nc = min(lstrlenW(Item->szUrl), L_MAX_URL_LENGTH - 1);
        szUrl = Alloc((nc + 1) * sizeof(WCHAR));
        if(szUrl)
        {
            lstrcpynW(szUrl, Item->szUrl, nc + 1);
        }
        else
        {
            Free(szId);

            ERR("Unable to allocate memory for link url\n");
            return FALSE;
        }
    }

    if(Item->mask & LIF_ITEMID)
    {
        Free(di->u.Link.szID);
        di->u.Link.szID = szId;
    }

    if(Item->mask & LIF_URL)
    {
        Free(di->u.Link.szUrl);
        di->u.Link.szUrl = szUrl;
    }

    if(Item->mask & LIF_STATE)
    {
        UINT oldstate = di->u.Link.state;
        /* clear the masked bits */
        di->u.Link.state &= ~(Item->stateMask & LIS_MASK);
        /* copy the bits */
        di->u.Link.state |= (Item->state & Item->stateMask) & LIS_MASK;
        Repaint = (oldstate != di->u.Link.state);
        
        /* update the focus */
        SYSLINK_SetFocusLink(infoPtr, ((di->u.Link.state & LIS_FOCUSED) ? di : NULL));
    }
    
    if(Repaint)
    {
        SYSLINK_RepaintLink(infoPtr, di);
    }
    
    return TRUE;
}

/***********************************************************************
 *           SYSLINK_GetItem
 * Retrieves the states and attributes of a link item.
 */
static LRESULT SYSLINK_GetItem (const SYSLINK_INFO *infoPtr, PLITEM Item)
{
    PDOC_ITEM di;
    
    if(!(Item->mask & LIF_ITEMINDEX) || !(Item->mask & (LIF_FLAGSMASK)))
    {
        ERR("Invalid Flags!\n");
        return FALSE;
    }
    
    di = SYSLINK_GetLinkItemByIndex(infoPtr, Item->iLink);
    if(di == NULL)
    {
        ERR("Link %d couldn't be found\n", Item->iLink);
        return FALSE;
    }
    
    if(Item->mask & LIF_STATE)
    {
        Item->state = (di->u.Link.state & Item->stateMask);
        if(!infoPtr->HasFocus)
        {
            /* remove the LIS_FOCUSED bit if the control doesn't have focus */
            Item->state &= ~LIS_FOCUSED;
        }
    }
    
    if(Item->mask & LIF_ITEMID)
    {
        if(di->u.Link.szID)
        {
            lstrcpyW(Item->szID, di->u.Link.szID);
        }
        else
        {
            Item->szID[0] = 0;
        }
    }
    
    if(Item->mask & LIF_URL)
    {
        if(di->u.Link.szUrl)
        {
            lstrcpyW(Item->szUrl, di->u.Link.szUrl);
        }
        else
        {
            Item->szUrl[0] = 0;
        }
    }
    
    return TRUE;
}

/***********************************************************************
 *           SYSLINK_PtInDocItem
 * Determines if a point is in the region of a document item
 */
static BOOL SYSLINK_PtInDocItem (const DOC_ITEM *DocItem, POINT pt)
{
    PDOC_TEXTBLOCK bl;
    int n;

    bl = DocItem->Blocks;
    if (bl != NULL)
    {
        n = DocItem->nText;

        while(n > 0)
        {
            if (PtInRect(&bl->rc, pt))
            {
                return TRUE;
            }
            n -= bl->nChars + bl->nSkip;
            bl++;
        }
    }
    
    return FALSE;
}

/***********************************************************************
 *           SYSLINK_HitTest
 * Determines the link the user clicked on.
 */
static LRESULT SYSLINK_HitTest (const SYSLINK_INFO *infoPtr, PLHITTESTINFO HitTest)
{
    PDOC_ITEM Current;
    int id = 0;

    LIST_FOR_EACH_ENTRY(Current, &infoPtr->Items, DOC_ITEM, entry)
    {
        if(Current->Type == slLink)
        {
            if(SYSLINK_PtInDocItem(Current, HitTest->pt))
            {
                HitTest->item.mask = 0;
                HitTest->item.iLink = id;
                HitTest->item.state = 0;
                HitTest->item.stateMask = 0;
                if(Current->u.Link.szID)
                {
                    lstrcpyW(HitTest->item.szID, Current->u.Link.szID);
                }
                else
                {
                    HitTest->item.szID[0] = 0;
                }
                if(Current->u.Link.szUrl)
                {
                    lstrcpyW(HitTest->item.szUrl, Current->u.Link.szUrl);
                }
                else
                {
                    HitTest->item.szUrl[0] = 0;
                }
                return TRUE;
            }
            id++;
        }
    }
    
    return FALSE;
}

/***********************************************************************
 *           SYSLINK_GetIdealHeight
 * Returns the preferred height of a link at the current control's width.
 */
static LRESULT SYSLINK_GetIdealHeight (const SYSLINK_INFO *infoPtr)
{
    HDC hdc = GetDC(infoPtr->Self);
    if(hdc != NULL)
    {
        LRESULT height;
        TEXTMETRICW tm;
        HGDIOBJ hOldFont = SelectObject(hdc, infoPtr->Font);
        
        if(GetTextMetricsW(hdc, &tm))
        {
            height = tm.tmHeight;
        }
        else
        {
            height = 0;
        }
        SelectObject(hdc, hOldFont);
        ReleaseDC(infoPtr->Self, hdc);
        
        return height;
    }
    return 0;
}

/***********************************************************************
 *           SYSLINK_SendParentNotify
 * Sends a WM_NOTIFY message to the parent window.
 */
static LRESULT SYSLINK_SendParentNotify (const SYSLINK_INFO *infoPtr, UINT code, const DOC_ITEM *Link, int iLink)
{
    NMLINK nml;

    nml.hdr.hwndFrom = infoPtr->Self;
    nml.hdr.idFrom = GetWindowLongPtrW(infoPtr->Self, GWLP_ID);
    nml.hdr.code = code;

    nml.item.mask = 0;
    nml.item.iLink = iLink;
    nml.item.state = 0;
    nml.item.stateMask = 0;
    if(Link->u.Link.szID)
    {
        lstrcpyW(nml.item.szID, Link->u.Link.szID);
    }
    else
    {
        nml.item.szID[0] = 0;
    }
    if(Link->u.Link.szUrl)
    {
        lstrcpyW(nml.item.szUrl, Link->u.Link.szUrl);
    }
    else
    {
        nml.item.szUrl[0] = 0;
    }

    return SendMessageW(infoPtr->Notify, WM_NOTIFY, nml.hdr.idFrom, (LPARAM)&nml);
}

/***********************************************************************
 *           SYSLINK_SetFocus
 * Handles receiving the input focus.
 */
static LRESULT SYSLINK_SetFocus (SYSLINK_INFO *infoPtr)
{
    PDOC_ITEM Focus;
    
    infoPtr->HasFocus = TRUE;

    /* We always select the first link, even if we activated the control using
       SHIFT+TAB. This is the default behavior */
    Focus = SYSLINK_GetNextLink(infoPtr, NULL);
    if(Focus != NULL)
    {
        SYSLINK_SetFocusLink(infoPtr, Focus);
        SYSLINK_RepaintLink(infoPtr, Focus);
    }
    return 0;
}

/***********************************************************************
 *           SYSLINK_KillFocus
 * Handles losing the input focus.
 */
static LRESULT SYSLINK_KillFocus (SYSLINK_INFO *infoPtr)
{
    PDOC_ITEM Focus;
    
    infoPtr->HasFocus = FALSE;
    Focus = SYSLINK_GetFocusLink(infoPtr, NULL);
    
    if(Focus != NULL)
    {
        SYSLINK_RepaintLink(infoPtr, Focus);
    }

    return 0;
}

/***********************************************************************
 *           SYSLINK_LinkAtPt
 * Returns a link at the specified position
 */
static PDOC_ITEM SYSLINK_LinkAtPt (const SYSLINK_INFO *infoPtr, const POINT *pt, int *LinkId, BOOL MustBeEnabled)
{
    PDOC_ITEM Current;
    int id = 0;

    LIST_FOR_EACH_ENTRY(Current, &infoPtr->Items, DOC_ITEM, entry)
    {
        if(Current->Type == slLink)
        {
            if(SYSLINK_PtInDocItem(Current, *pt) && (!MustBeEnabled || (Current->u.Link.state & LIS_ENABLED)))
            {
                if(LinkId != NULL)
                {
                    *LinkId = id;
                }
                return Current;
            }
            id++;
        }
    }

    return NULL;
}

/***********************************************************************
 *           SYSLINK_LButtonDown
 * Handles mouse clicks
 */
static LRESULT SYSLINK_LButtonDown (SYSLINK_INFO *infoPtr, const POINT *pt)
{
    PDOC_ITEM Current, Old;
    int id;

    Current = SYSLINK_LinkAtPt(infoPtr, pt, &id, TRUE);
    if(Current != NULL)
    {
      SetFocus(infoPtr->Self);

      Old = SYSLINK_SetFocusLink(infoPtr, Current);
      if(Old != NULL && Old != Current)
      {
          SYSLINK_RepaintLink(infoPtr, Old);
      }
      infoPtr->MouseDownID = id;
      SYSLINK_RepaintLink(infoPtr, Current);
    }

    return 0;
}

/***********************************************************************
 *           SYSLINK_LButtonUp
 * Handles mouse clicks
 */
static LRESULT SYSLINK_LButtonUp (SYSLINK_INFO *infoPtr, const POINT *pt)
{
    if(infoPtr->MouseDownID > -1)
    {
        PDOC_ITEM Current;
        int id;
        
        Current = SYSLINK_LinkAtPt(infoPtr, pt, &id, TRUE);
        if((Current != NULL) && (Current->u.Link.state & LIS_FOCUSED) && (infoPtr->MouseDownID == id))
        {
            SYSLINK_SendParentNotify(infoPtr, NM_CLICK, Current, id);
        }
    }

    infoPtr->MouseDownID = -1;

    return 0;
}

/***********************************************************************
 *           SYSLINK_OnEnter
 * Handles ENTER key events
 */
static BOOL SYSLINK_OnEnter (const SYSLINK_INFO *infoPtr)
{
    if(infoPtr->HasFocus && !infoPtr->IgnoreReturn)
    {
        PDOC_ITEM Focus;
        int id;
        
        Focus = SYSLINK_GetFocusLink(infoPtr, &id);
        if(Focus)
        {
            SYSLINK_SendParentNotify(infoPtr, NM_RETURN, Focus, id);
            return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *           SYSKEY_SelectNextPrevLink
 * Changes the currently focused link
 */
static BOOL SYSKEY_SelectNextPrevLink (const SYSLINK_INFO *infoPtr, BOOL Prev)
{
    if(infoPtr->HasFocus)
    {
        PDOC_ITEM Focus;
        int id;

        Focus = SYSLINK_GetFocusLink(infoPtr, &id);
        if(Focus != NULL)
        {
            PDOC_ITEM NewFocus, OldFocus;

            if(Prev)
                NewFocus = SYSLINK_GetPrevLink(infoPtr, Focus);
            else
                NewFocus = SYSLINK_GetNextLink(infoPtr, Focus);

            if(NewFocus != NULL)
            {
                OldFocus = SYSLINK_SetFocusLink(infoPtr, NewFocus);

                if(OldFocus && OldFocus != NewFocus)
                {
                    SYSLINK_RepaintLink(infoPtr, OldFocus);
                }
                SYSLINK_RepaintLink(infoPtr, NewFocus);
                return TRUE;
            }
        }
    }
    return FALSE;
}

/***********************************************************************
 *           SYSKEY_SelectNextPrevLink
 * Determines if there's a next or previous link to decide whether the control
 * should capture the tab key message
 */
static BOOL SYSLINK_NoNextLink (const SYSLINK_INFO *infoPtr, BOOL Prev)
{
    PDOC_ITEM Focus, NewFocus;

    Focus = SYSLINK_GetFocusLink(infoPtr, NULL);
    if(Prev)
        NewFocus = SYSLINK_GetPrevLink(infoPtr, Focus);
    else
        NewFocus = SYSLINK_GetNextLink(infoPtr, Focus);

    return NewFocus == NULL;
}

/***********************************************************************
 *           SYSLINK_GetIdealSize
 * Calculates the ideal size of a link control at a given maximum width.
 */
static LONG SYSLINK_GetIdealSize (const SYSLINK_INFO *infoPtr, int cxMaxWidth, SIZE *lpSize)
{
    RECT rc;
    HDC hdc;

    rc.left = rc.top = rc.bottom = 0;
    rc.right = cxMaxWidth;

    hdc = GetDC(infoPtr->Self);
    if (hdc != NULL)
    {
        HGDIOBJ hOldFont = SelectObject(hdc, infoPtr->Font);

        SYSLINK_Render(infoPtr, hdc, &rc);

        SelectObject(hdc, hOldFont);
        ReleaseDC(infoPtr->Self, hdc);

        lpSize->cx = rc.right;
        lpSize->cy = rc.bottom;
    }

    return rc.bottom;
}

/***********************************************************************
 *           SysLinkWindowProc
 */
static LRESULT WINAPI SysLinkWindowProc(HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam)
{
    SYSLINK_INFO *infoPtr;

    TRACE("hwnd %p, msg %04x, wparam %Ix, lParam %Ix\n", hwnd, message, wParam, lParam);

    infoPtr = (SYSLINK_INFO *)GetWindowLongPtrW(hwnd, 0);

    if (!infoPtr && message != WM_CREATE)
        return DefWindowProcW(hwnd, message, wParam, lParam);

    switch(message) {
    case WM_PRINTCLIENT:
    case WM_PAINT:
        return SYSLINK_Paint (infoPtr, (HDC)wParam);

    case WM_ERASEBKGND:
        if (!(infoPtr->Style & LWS_TRANSPARENT))
        {
            HDC hdc = (HDC)wParam;
            HBRUSH brush = CreateSolidBrush( comctl32_color.clrWindow );
            RECT rect;

            GetClipBox( hdc, &rect );
            FillRect( hdc, &rect, brush );
            DeleteObject( brush );
            return 1;
        }
        return 0;

    case WM_SETCURSOR:
    {
        LHITTESTINFO ht;
        DWORD mp = GetMessagePos();
        
        ht.pt.x = (short)LOWORD(mp);
        ht.pt.y = (short)HIWORD(mp);
        
        ScreenToClient(infoPtr->Self, &ht.pt);
        if(SYSLINK_HitTest (infoPtr, &ht))
        {
            SetCursor(LoadCursorW(0, (LPCWSTR)IDC_HAND));
            return TRUE;
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    case WM_SIZE:
    {
        RECT rcClient;
        if (GetClientRect(infoPtr->Self, &rcClient))
        {
            HDC hdc = GetDC(infoPtr->Self);
            if(hdc != NULL)
            {
                SYSLINK_Render(infoPtr, hdc, &rcClient);
                ReleaseDC(infoPtr->Self, hdc);
            }
        }
        return 0;
    }

    case WM_GETFONT:
        return (LRESULT)infoPtr->Font;

    case WM_SETFONT:
        return (LRESULT)SYSLINK_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_SETTEXT:
        SYSLINK_SetText(infoPtr, (LPWSTR)lParam);
        return DefWindowProcW(hwnd, message, wParam, lParam);

    case WM_LBUTTONDOWN:
    {
        POINT pt;
        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        return SYSLINK_LButtonDown(infoPtr, &pt);
    }
    case WM_LBUTTONUP:
    {
        POINT pt;
        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        return SYSLINK_LButtonUp(infoPtr, &pt);
    }
    
    case WM_KEYDOWN:
    {
        switch(wParam)
        {
        case VK_RETURN:
            SYSLINK_OnEnter(infoPtr);
            return 0;
        case VK_TAB:
        {
            BOOL shift = GetKeyState(VK_SHIFT) & 0x8000;
            SYSKEY_SelectNextPrevLink(infoPtr, shift);
            return 0;
        }
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
    
    case WM_GETDLGCODE:
    {
        LRESULT Ret = DLGC_HASSETSEL;
        int vk = (lParam != 0 ? (int)((LPMSG)lParam)->wParam : 0);
        switch(vk)
        {
        case VK_RETURN:
            Ret |= DLGC_WANTMESSAGE;
            break;
        case VK_TAB:
        {
            BOOL shift = GetKeyState(VK_SHIFT) & 0x8000;
            if(!SYSLINK_NoNextLink(infoPtr, shift))
            {
                Ret |= DLGC_WANTTAB;
            }
            else
            {
                Ret |= DLGC_WANTCHARS;
            }
            break;
        }
        }
        return Ret;
    }
    
    case WM_NCHITTEST:
    {
        POINT pt;
        RECT rc;
        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        
        GetClientRect(infoPtr->Self, &rc);
        ScreenToClient(infoPtr->Self, &pt);
        if(pt.x < 0 || pt.y < 0 || pt.x > rc.right || pt.y > rc.bottom)
        {
            return HTNOWHERE;
        }

        if(SYSLINK_LinkAtPt(infoPtr, &pt, NULL, FALSE))
        {
            return HTCLIENT;
        }
        
        return HTTRANSPARENT;
    }

    case LM_HITTEST:
        return SYSLINK_HitTest(infoPtr, (PLHITTESTINFO)lParam);

    case LM_SETITEM:
        return SYSLINK_SetItem(infoPtr, (PLITEM)lParam);

    case LM_GETITEM:
        return SYSLINK_GetItem(infoPtr, (PLITEM)lParam);

    case LM_GETIDEALHEIGHT:
        if (lParam)
            return SYSLINK_GetIdealSize(infoPtr, (int)wParam, (SIZE *)lParam);
        else
            return SYSLINK_GetIdealHeight(infoPtr);

    case WM_SETFOCUS:
        return SYSLINK_SetFocus(infoPtr);

    case WM_KILLFOCUS:
        return SYSLINK_KillFocus(infoPtr);

    case WM_ENABLE:
        infoPtr->Style &= ~WS_DISABLED;
        infoPtr->Style |= (wParam ? 0 : WS_DISABLED);
        InvalidateRect (infoPtr->Self, NULL, FALSE);
        return 0;

    case WM_STYLECHANGED:
        if (wParam == GWL_STYLE)
            infoPtr->Style = ((LPSTYLESTRUCT)lParam)->styleNew;
        return 0;

    case WM_CREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW*)lParam;

        /* allocate memory for info struct */
        infoPtr = Alloc (sizeof(SYSLINK_INFO));
        if (!infoPtr) return -1;
        SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

        /* initialize the info struct */
        infoPtr->Self = hwnd;
        infoPtr->Notify = cs->hwndParent;
        infoPtr->Style = cs->style;
        infoPtr->Font = 0;
        infoPtr->LinkFont = 0;
        list_init(&infoPtr->Items);
        infoPtr->HasFocus = FALSE;
        infoPtr->MouseDownID = -1;
        infoPtr->TextColor = comctl32_color.clrWindowText;
        infoPtr->LinkColor = comctl32_color.clrHighlight;
        infoPtr->VisitedColor = comctl32_color.clrHighlight;
        infoPtr->BreakChar = ' ';
        infoPtr->IgnoreReturn = infoPtr->Style & LWS_IGNORERETURN;
        TRACE("SysLink Ctrl creation, hwnd=%p\n", hwnd);
        SYSLINK_SetText(infoPtr, cs->lpszName);
        return 0;
    }
    case WM_DESTROY:
        TRACE("SysLink Ctrl destruction, hwnd=%p\n", hwnd);
        SYSLINK_ClearDoc(infoPtr);
#ifndef __REACTOS__
        if(infoPtr->Font != 0) DeleteObject(infoPtr->Font);
#endif
        if(infoPtr->LinkFont != 0) DeleteObject(infoPtr->LinkFont);
        SetWindowLongPtrW(hwnd, 0, 0);
        if(infoPtr->AccessibleImpl) Accessible_WindowDestroyed(infoPtr->AccessibleImpl);
        Free (infoPtr);
        return 0;

    case WM_GETOBJECT:
    {
        if ((DWORD)lParam == (DWORD)OBJID_CLIENT) {
            if (!infoPtr->AccessibleImpl)
                Accessible_Create(infoPtr);

            if (infoPtr->AccessibleImpl)
                return LresultFromObject(&IID_IAccessible, wParam, (IUnknown*)&infoPtr->AccessibleImpl->IAccessible_iface);
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    case WM_SYSCOLORCHANGE:
        COMCTL32_RefreshSysColors();
        return 0;

    default:
        if ((message >= WM_USER) && (message < WM_APP) && !COMCTL32_IsReflectedMessage(message))
        {
            ERR("unknown msg %04x, wp %Ix, lp %Ix\n", message, wParam, lParam );
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}


/***********************************************************************
 * SYSLINK_Register [Internal]
 *
 * Registers the SysLink window class.
 */
VOID SYSLINK_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(wndClass));
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc   = SysLinkWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof (SYSLINK_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.lpszClassName = WC_LINK;

    RegisterClassW (&wndClass);
}
