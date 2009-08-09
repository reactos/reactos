/*
 * Implementation of IPersist interfaces for WebBrowser control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2005 Jacek Caban
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

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IPersistStorage interface
 */

#define PERSTORAGE_THIS(ifce) DEFINE_THIS(WebBrowser, PersistStorage, iface)

static HRESULT WINAPI PersistStorage_QueryInterface(IPersistStorage *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    return IWebBrowser_QueryInterface(WEBBROWSER(This), riid, ppobj);
}

static ULONG WINAPI PersistStorage_AddRef(IPersistStorage *iface)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    return IWebBrowser_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI PersistStorage_Release(IPersistStorage *iface)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    return IWebBrowser_Release(WEBBROWSER(This));
}

static HRESULT WINAPI PersistStorage_GetClassID(IPersistStorage *iface, CLSID *pClassID)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStorage_IsDirty(IPersistStorage *iface)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStorage_InitNew(IPersistStorage *iface, LPSTORAGE pStg)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pStg);
    return S_OK;
}

static HRESULT WINAPI PersistStorage_Load(IPersistStorage *iface, LPSTORAGE pStg)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pStg);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStorage_Save(IPersistStorage *iface, LPSTORAGE pStg,
        BOOL fSameAsLoad)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    FIXME("(%p)->(%p %x)\n", This, pStg, fSameAsLoad);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStorage_SaveCompleted(IPersistStorage *iface, LPSTORAGE pStgNew)
{
    WebBrowser *This = PERSTORAGE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pStgNew);
    return E_NOTIMPL;
}

static const IPersistStorageVtbl PersistStorageVtbl =
{
    PersistStorage_QueryInterface,
    PersistStorage_AddRef,
    PersistStorage_Release,
    PersistStorage_GetClassID,
    PersistStorage_IsDirty,
    PersistStorage_InitNew,
    PersistStorage_Load,
    PersistStorage_Save,
    PersistStorage_SaveCompleted
};

/**********************************************************************
 * Implement the IPersistMemory interface
 */

#define PERMEMORY_THIS(ifce) DEFINE_THIS(WebBrowser, PersistMemory, iface)

static HRESULT WINAPI PersistMemory_QueryInterface(IPersistMemory *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    return IWebBrowser_QueryInterface(WEBBROWSER(This), riid, ppobj);
}

static ULONG WINAPI PersistMemory_AddRef(IPersistMemory *iface)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    return IWebBrowser_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI PersistMemory_Release(IPersistMemory *iface)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    return IWebBrowser_Release(WEBBROWSER(This));
}

static HRESULT WINAPI PersistMemory_GetClassID(IPersistMemory *iface, CLSID *pClassID)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMemory_IsDirty(IPersistMemory *iface)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMemory_InitNew(IPersistMemory *iface)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    FIXME("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI PersistMemory_Load(IPersistMemory *iface, LPVOID pMem, ULONG cbSize)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    FIXME("(%p)->(%p %x)\n", This, pMem, cbSize);
    return S_OK;
}

static HRESULT WINAPI PersistMemory_Save(IPersistMemory *iface, LPVOID pMem,
        BOOL fClearDirty, ULONG cbSize)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    FIXME("(%p)->(%p %x %x)\n", This, pMem, fClearDirty, cbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMemory_GetSizeMax(IPersistMemory *iface, ULONG *pCbSize)
{
    WebBrowser *This = PERMEMORY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pCbSize);
    return E_NOTIMPL;
}

static const IPersistMemoryVtbl PersistMemoryVtbl =
{
    PersistMemory_QueryInterface,
    PersistMemory_AddRef,
    PersistMemory_Release,
    PersistMemory_GetClassID,
    PersistMemory_IsDirty,
    PersistMemory_Load,
    PersistMemory_Save,
    PersistMemory_GetSizeMax,
    PersistMemory_InitNew
};

/**********************************************************************
 * Implement the IPersistStreamInit interface
 */

#define PERSTRINIT_THIS(iface) DEFINE_THIS(WebBrowser, PersistStreamInit, iface)

static HRESULT WINAPI PersistStreamInit_QueryInterface(IPersistStreamInit *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    return IWebBrowser_QueryInterface(WEBBROWSER(This), riid, ppobj);
}

static ULONG WINAPI PersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    return IWebBrowser_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI PersistStreamInit_Release(IPersistStreamInit *iface)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    return IWebBrowser_Release(WEBBROWSER(This));
}

static HRESULT WINAPI PersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *pClassID)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    return IPersistStorage_GetClassID(PERSTORAGE(This), pClassID);
}

static HRESULT WINAPI PersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    return IPersistStorage_IsDirty(PERSTORAGE(This));
}

static HRESULT WINAPI PersistStreamInit_Load(IPersistStreamInit *iface, LPSTREAM pStg)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pStg);
    return S_OK;
}

static HRESULT WINAPI PersistStreamInit_Save(IPersistStreamInit *iface, LPSTREAM pStg,
        BOOL fSameAsLoad)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    FIXME("(%p)->(%p %x)\n", This, pStg, fSameAsLoad);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_GetSizeMax(IPersistStreamInit *iface,
        ULARGE_INTEGER *pcbSize)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    WebBrowser *This = PERSTRINIT_THIS(iface);
    FIXME("(%p)\n", This);
    return S_OK;
}

#undef PERSTRINIT_THIS

static const IPersistStreamInitVtbl PersistStreamInitVtbl =
{
    PersistStreamInit_QueryInterface,
    PersistStreamInit_AddRef,
    PersistStreamInit_Release,
    PersistStreamInit_GetClassID,
    PersistStreamInit_IsDirty,
    PersistStreamInit_Load,
    PersistStreamInit_Save,
    PersistStreamInit_GetSizeMax,
    PersistStreamInit_InitNew
};

void WebBrowser_Persist_Init(WebBrowser *This)
{
    This->lpPersistStorageVtbl    = &PersistStorageVtbl;
    This->lpPersistMemoryVtbl     = &PersistMemoryVtbl;
    This->lpPersistStreamInitVtbl = &PersistStreamInitVtbl;
}
