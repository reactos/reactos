/*
 * Copyright 1999 Corel Corporation
 * Sean Langley
 * Copyright 2010  Geoffrey Hausheer
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ole2.h"
#include "olectl.h"
#include "oledlg.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct {
    IPropertyPageSite IPropertyPageSite_iface;
    LCID lcid;
    LONG ref;
} PropertyPageSite;

static inline PropertyPageSite *impl_from_IPropertyPageSite(IPropertyPageSite *iface)
{
    return CONTAINING_RECORD(iface, PropertyPageSite, IPropertyPageSite_iface);
}

static INT_PTR CALLBACK property_sheet_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    IPropertyPage *property_page = (IPropertyPage*)GetWindowLongPtrW(hwnd, DWLP_USER);

    switch(msg) {
    case WM_INITDIALOG: {
        RECT rect;

        property_page = (IPropertyPage*)((LPPROPSHEETPAGEW)lparam)->lParam;

        GetClientRect(hwnd, &rect);
        IPropertyPage_Activate(property_page, hwnd, &rect, TRUE);
        IPropertyPage_Show(property_page, SW_SHOW);

        SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)property_page);
        return FALSE;
    }
    case WM_DESTROY:
        IPropertyPage_Show(property_page, SW_HIDE);
        IPropertyPage_Deactivate(property_page);
        return FALSE;
    default:
        return FALSE;
    }
}

static HRESULT WINAPI PropertyPageSite_QueryInterface(IPropertyPageSite*  iface,
        REFIID  riid, void**  ppv)
{
    TRACE("(%p riid: %s)\n",iface, debugstr_guid(riid));

    if(IsEqualGUID(&IID_IUnknown, riid)
            || IsEqualGUID(&IID_IPropertyPageSite, riid))
        *ppv = iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PropertyPageSite_AddRef(IPropertyPageSite* iface)
{
    PropertyPageSite *this = impl_from_IPropertyPageSite(iface);
    LONG ref = InterlockedIncrement(&this->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);
    return ref;
}

static ULONG WINAPI PropertyPageSite_Release(IPropertyPageSite* iface)
{
    PropertyPageSite *this = impl_from_IPropertyPageSite(iface);
    LONG ref = InterlockedDecrement(&this->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);
    if(!ref)
        HeapFree(GetProcessHeap(), 0, this);
    return ref;
}

static HRESULT WINAPI PropertyPageSite_OnStatusChange(
        IPropertyPageSite *iface, DWORD dwFlags)
{
    TRACE("%p, %lx.\n", iface, dwFlags);
    return S_OK;
}

static HRESULT WINAPI PropertyPageSite_GetLocaleID(
        IPropertyPageSite *iface, LCID *pLocaleID)
{
    PropertyPageSite *this = impl_from_IPropertyPageSite(iface);

    TRACE("(%p, %p)\n", iface, pLocaleID);
    *pLocaleID = this->lcid;
    return S_OK;
}

static HRESULT WINAPI PropertyPageSite_GetPageContainer(
        IPropertyPageSite* iface, IUnknown** ppUnk)
{
    FIXME("(%p, %p)\n", iface, ppUnk);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyPageSite_TranslateAccelerator(
        IPropertyPageSite* iface, MSG *pMsg)
{
    FIXME("(%p, %p)\n", iface, pMsg);
    return E_NOTIMPL;
}

static IPropertyPageSiteVtbl PropertyPageSiteVtbl = {
    PropertyPageSite_QueryInterface,
    PropertyPageSite_AddRef,
    PropertyPageSite_Release,
    PropertyPageSite_OnStatusChange,
    PropertyPageSite_GetLocaleID,
    PropertyPageSite_GetPageContainer,
    PropertyPageSite_TranslateAccelerator
};

/***********************************************************************
 * OleCreatePropertyFrameIndirect (OLEAUT32.416)
 */
HRESULT WINAPI OleCreatePropertyFrameIndirect(LPOCPFIPARAMS lpParams)
{
    PROPSHEETHEADERW property_sheet;
    PROPSHEETPAGEW property_sheet_page;
    struct {
        DLGTEMPLATE template;
        WORD menu;
        WORD class;
        WORD title;
    } *dialogs;
    IPropertyPage **property_page;
    PropertyPageSite *property_page_site;
    HRESULT res;
    ULONG i;
    HMODULE hcomctl;
    HRSRC property_sheet_dialog_find = NULL;
    HGLOBAL property_sheet_dialog_load = NULL;
    WCHAR *property_sheet_dialog_data = NULL;
    HDC hdc;
    LOGFONTW font_desc;
    HFONT hfont;
    LONG font_width = 4, font_height = 8;

    if(!lpParams)
        return E_POINTER;

    TRACE("%ld, %p, %d, %d, %s, %ld, %p, %ld, %p, %ld, %ld.\n", lpParams->cbStructSize,
            lpParams->hWndOwner, lpParams->x, lpParams->y,
            debugstr_w(lpParams->lpszCaption), lpParams->cObjects,
            lpParams->lplpUnk, lpParams->cPages, lpParams->lpPages,
            lpParams->lcid, lpParams->dispidInitialProperty);

    if(!lpParams->lpPages)
        return E_POINTER;

    if(lpParams->cbStructSize != sizeof(OCPFIPARAMS)) {
        WARN("incorrect structure size\n");
        return E_INVALIDARG;
    }

    if(lpParams->dispidInitialProperty)
        FIXME("dispidInitialProperty not yet implemented\n");

    hdc = GetDC(NULL);
    hcomctl = LoadLibraryW(L"comctl32.dll");
    if(hcomctl)
        property_sheet_dialog_find = FindResourceW(hcomctl,
                MAKEINTRESOURCEW(1006 /*IDD_PROPSHEET*/), (LPWSTR)RT_DIALOG);
    if(property_sheet_dialog_find)
        property_sheet_dialog_load = LoadResource(hcomctl, property_sheet_dialog_find);
    if(property_sheet_dialog_load)
        property_sheet_dialog_data = LockResource(property_sheet_dialog_load);

    if(property_sheet_dialog_data) {
        if(property_sheet_dialog_data[1] == 0xffff) {
            ERR("Expected DLGTEMPLATE structure\n");
            FreeLibrary(hcomctl);
            return E_OUTOFMEMORY;
        }

        property_sheet_dialog_data += sizeof(DLGTEMPLATE)/sizeof(WCHAR);
        /* Skip menu, class and title */
        property_sheet_dialog_data += lstrlenW(property_sheet_dialog_data)+1;
        property_sheet_dialog_data += lstrlenW(property_sheet_dialog_data)+1;
        property_sheet_dialog_data += lstrlenW(property_sheet_dialog_data)+1;

        memset(&font_desc, 0, sizeof(LOGFONTW));
        /* Calculate logical height */
        font_desc.lfHeight = -MulDiv(property_sheet_dialog_data[0],
                GetDeviceCaps(hdc, LOGPIXELSY), 72);
        font_desc.lfCharSet = DEFAULT_CHARSET;
        memcpy(font_desc.lfFaceName, property_sheet_dialog_data+1,
                sizeof(WCHAR)*(lstrlenW(property_sheet_dialog_data+1)+1));
        hfont = CreateFontIndirectW(&font_desc);

        if(hfont) {
            hfont = SelectObject(hdc, hfont);
            font_width = GdiGetCharDimensions(hdc, NULL, &font_height);
            SelectObject(hdc, hfont);
        }
    }
    if(hcomctl)
        FreeLibrary(hcomctl);
    ReleaseDC(NULL, hdc);

    memset(&property_sheet, 0, sizeof(property_sheet));
    property_sheet.dwSize = sizeof(property_sheet);
    if(lpParams->lpszCaption) {
        property_sheet.dwFlags = PSH_PROPTITLE;
        property_sheet.pszCaption = lpParams->lpszCaption;
    }

    property_sheet.phpage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            lpParams->cPages*sizeof(HPROPSHEETPAGE));
    property_page = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            lpParams->cPages*sizeof(IPropertyPage*));
    dialogs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            lpParams->cPages*sizeof(*dialogs));
    if(!property_sheet.phpage || !property_page || !dialogs) {
        HeapFree(GetProcessHeap(), 0, property_sheet.phpage);
        HeapFree(GetProcessHeap(), 0, property_page);
        HeapFree(GetProcessHeap(), 0, dialogs);
        return E_OUTOFMEMORY;
    }

    memset(&property_sheet_page, 0, sizeof(PROPSHEETPAGEW));
    property_sheet_page.dwSize = sizeof(PROPSHEETPAGEW);
    property_sheet_page.dwFlags = PSP_DLGINDIRECT|PSP_USETITLE;
    property_sheet_page.pfnDlgProc = property_sheet_proc;

    for(i=0; i<lpParams->cPages; i++) {
        PROPPAGEINFO page_info;

        res = CoCreateInstance(&lpParams->lpPages[i], NULL, CLSCTX_INPROC_SERVER,
                &IID_IPropertyPage, (void**)&property_page[i]);
        if(FAILED(res))
            continue;

        property_page_site = HeapAlloc(GetProcessHeap(), 0, sizeof(PropertyPageSite));
        if(!property_page_site)
            continue;
        property_page_site->IPropertyPageSite_iface.lpVtbl = &PropertyPageSiteVtbl;
        property_page_site->ref = 1;
        property_page_site->lcid = lpParams->lcid;

        res = IPropertyPage_SetPageSite(property_page[i],
                &property_page_site->IPropertyPageSite_iface);
        IPropertyPageSite_Release(&property_page_site->IPropertyPageSite_iface);
        if(FAILED(res))
            continue;

        res = IPropertyPage_SetObjects(property_page[i],
                lpParams->cObjects, lpParams->lplpUnk);
        if(FAILED(res))
            WARN("SetObjects() failed, hr %#lx.\n", res);

        res = IPropertyPage_GetPageInfo(property_page[i], &page_info);
        if(FAILED(res))
            continue;

        dialogs[i].template.cx = MulDiv(page_info.size.cx, 4, font_width);
        dialogs[i].template.cy = MulDiv(page_info.size.cy, 8, font_height);

        property_sheet_page.pResource = &dialogs[i].template;
        property_sheet_page.lParam = (LPARAM)property_page[i];
        property_sheet_page.pszTitle = page_info.pszTitle;

        property_sheet.phpage[property_sheet.nPages++] =
            CreatePropertySheetPageW(&property_sheet_page);
    }

    PropertySheetW(&property_sheet);

    for(i=0; i<lpParams->cPages; i++) {
        if(property_page[i])
            IPropertyPage_Release(property_page[i]);
    }

    HeapFree(GetProcessHeap(), 0, dialogs);
    HeapFree(GetProcessHeap(), 0, property_page);
    HeapFree(GetProcessHeap(), 0, property_sheet.phpage);
    return S_OK;
}

/***********************************************************************
 * OleCreatePropertyFrame (OLEAUT32.417)
 */
HRESULT WINAPI OleCreatePropertyFrame(
        HWND hwndOwner, UINT x, UINT y, LPCOLESTR lpszCaption, ULONG cObjects,
        LPUNKNOWN* ppUnk, ULONG cPages, LPCLSID pPageClsID, LCID lcid,
        DWORD dwReserved, LPVOID pvReserved)
{
    OCPFIPARAMS ocpf;

    ocpf.cbStructSize =  sizeof(OCPFIPARAMS);
    ocpf.hWndOwner    = hwndOwner;
    ocpf.x            = x;
    ocpf.y            = y;
    ocpf.lpszCaption  = lpszCaption;
    ocpf.cObjects     = cObjects;
    ocpf.lplpUnk      = ppUnk;
    ocpf.cPages       = cPages;
    ocpf.lpPages      = pPageClsID;
    ocpf.lcid         = lcid;
    ocpf.dispidInitialProperty = 0;

    return OleCreatePropertyFrameIndirect(&ocpf);
}
