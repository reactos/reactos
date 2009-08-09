/*
 * RichEdit GUIDs and OLE interface
 *
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2004 Aric Stewart
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "richole.h"
#include "editor.h"
#include "tom.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

/* there is no way to be consistent across different sets of headers - mingw, Wine, Win32 SDK*/

/* FIXME: the next 6 lines should be in textserv.h */
#include "initguid.h"
#define TEXTSERV_GUID(name, l, w1, w2, b1, b2) \
    DEFINE_GUID(name, l, w1, w2, b1, b2, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5)

TEXTSERV_GUID(IID_ITextServices, 0x8d33f740, 0xcf58, 0x11ce, 0xa8, 0x9d);
TEXTSERV_GUID(IID_ITextHost, 0xc5bdd8d0, 0xd26e, 0x11ce, 0xa8, 0x9e);
TEXTSERV_GUID(IID_ITextHost2, 0xc5bdd8d0, 0xd26e, 0x11ce, 0xa8, 0x9e);
DEFINE_GUID(IID_ITextDocument, 0x8cc497c0, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
DEFINE_GUID(IID_ITextRange, 0x8cc497c2, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
DEFINE_GUID(IID_ITextSelection, 0x8cc497c1, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);

typedef struct ITextSelectionImpl ITextSelectionImpl;
typedef struct IOleClientSiteImpl IOleClientSiteImpl;

typedef struct IRichEditOleImpl {
    const IRichEditOleVtbl *lpRichEditOleVtbl;
    const ITextDocumentVtbl *lpTextDocumentVtbl;
    LONG ref;

    ME_TextEditor *editor;
    ITextSelectionImpl *txtSel;
    IOleClientSiteImpl *clientSite;
} IRichEditOleImpl;

struct ITextSelectionImpl {
    const ITextSelectionVtbl *lpVtbl;
    LONG ref;

    IRichEditOleImpl *reOle;
};

struct IOleClientSiteImpl {
    const IOleClientSiteVtbl *lpVtbl;
    LONG ref;

    IRichEditOleImpl *reOle;
};

static inline IRichEditOleImpl *impl_from_IRichEditOle(IRichEditOle *iface)
{
    return (IRichEditOleImpl *)((BYTE*)iface - FIELD_OFFSET(IRichEditOleImpl, lpRichEditOleVtbl));
}

static inline IRichEditOleImpl *impl_from_ITextDocument(ITextDocument *iface)
{
    return (IRichEditOleImpl *)((BYTE*)iface - FIELD_OFFSET(IRichEditOleImpl, lpTextDocumentVtbl));
}

static HRESULT WINAPI
IRichEditOle_fnQueryInterface(IRichEditOle *me, REFIID riid, LPVOID *ppvObj)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);

    TRACE("%p %s\n", This, debugstr_guid(riid) );

    *ppvObj = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IRichEditOle))
        *ppvObj = &This->lpRichEditOleVtbl;
    else if (IsEqualGUID(riid, &IID_ITextDocument))
        *ppvObj = &This->lpTextDocumentVtbl;
    if (*ppvObj)
    {
        IRichEditOle_AddRef(me);
        return S_OK;
    }
    FIXME("%p: unhandled interface %s\n", This, debugstr_guid(riid) );
 
    return E_NOINTERFACE;   
}

static ULONG WINAPI
IRichEditOle_fnAddRef(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE("%p ref = %u\n", This, ref);

    return ref;
}

static ULONG WINAPI
IRichEditOle_fnRelease(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE ("%p ref=%u\n", This, ref);

    if (!ref)
    {
        TRACE ("Destroying %p\n", This);
        This->txtSel->reOle = NULL;
        ITextSelection_Release((ITextSelection *) This->txtSel);
        IOleClientSite_Release((IOleClientSite *) This->clientSite);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI
IRichEditOle_fnActivateAs(IRichEditOle *me, REFCLSID rclsid, REFCLSID rclsidAs)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnContextSensitiveHelp(IRichEditOle *me, BOOL fEnterMode)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnConvertObject(IRichEditOle *me, LONG iob,
               REFCLSID rclsidNew, LPCSTR lpstrUserTypeNew)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IOleClientSite_fnQueryInterface(IOleClientSite *me, REFIID riid, LPVOID *ppvObj)
{
    TRACE("%p %s\n", me, debugstr_guid(riid) );

    *ppvObj = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IOleClientSite))
        *ppvObj = me;
    if (*ppvObj)
    {
        IOleClientSite_AddRef(me);
        return S_OK;
    }
    FIXME("%p: unhandled interface %s\n", me, debugstr_guid(riid) );

    return E_NOINTERFACE;
}

static ULONG WINAPI
IOleClientSite_fnAddRef(IOleClientSite *me)
{
    IOleClientSiteImpl *This = (IOleClientSiteImpl *) me;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI
IOleClientSite_fnRelease(IOleClientSite *me)
{
    IOleClientSiteImpl *This = (IOleClientSiteImpl *) me;
    ULONG ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        heap_free(This);
    return ref;
}

static HRESULT WINAPI
IOleClientSite_fnSaveObject(IOleClientSite *me)
{
    IOleClientSiteImpl *This = (IOleClientSiteImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("stub %p\n",me);
    return E_NOTIMPL;
}


static HRESULT WINAPI
IOleClientSite_fnGetMoniker(IOleClientSite *me, DWORD dwAssign, DWORD dwWhichMoniker,
    IMoniker **ppmk)
{
    IOleClientSiteImpl *This = (IOleClientSiteImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("stub %p\n",me);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IOleClientSite_fnGetContainer(IOleClientSite *me, IOleContainer **ppContainer)
{
    IOleClientSiteImpl *This = (IOleClientSiteImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("stub %p\n",me);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IOleClientSite_fnShowObject(IOleClientSite *me)
{
    IOleClientSiteImpl *This = (IOleClientSiteImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("stub %p\n",me);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IOleClientSite_fnOnShowWindow(IOleClientSite *me, BOOL fShow)
{
    IOleClientSiteImpl *This = (IOleClientSiteImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("stub %p\n",me);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IOleClientSite_fnRequestNewObjectLayout(IOleClientSite *me)
{
    IOleClientSiteImpl *This = (IOleClientSiteImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("stub %p\n",me);
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ocst = {
    IOleClientSite_fnQueryInterface,
    IOleClientSite_fnAddRef,
    IOleClientSite_fnRelease,
    IOleClientSite_fnSaveObject,
    IOleClientSite_fnGetMoniker,
    IOleClientSite_fnGetContainer,
    IOleClientSite_fnShowObject,
    IOleClientSite_fnOnShowWindow,
    IOleClientSite_fnRequestNewObjectLayout
};

static IOleClientSiteImpl *
CreateOleClientSite(IRichEditOleImpl *reOle)
{
    IOleClientSiteImpl *clientSite = heap_alloc(sizeof *clientSite);
    if (!clientSite)
        return NULL;

    clientSite->lpVtbl = &ocst;
    clientSite->ref = 1;
    clientSite->reOle = reOle;
    return clientSite;
}

static HRESULT WINAPI
IRichEditOle_fnGetClientSite(IRichEditOle *me,
               LPOLECLIENTSITE *lplpolesite)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);

    TRACE("%p,%p\n",This, lplpolesite);

    if(!lplpolesite)
        return E_INVALIDARG;
    *lplpolesite = (IOleClientSite *) This->clientSite;
    IOleClientSite_fnAddRef(*lplpolesite);
    return S_OK;
}

static HRESULT WINAPI
IRichEditOle_fnGetClipboardData(IRichEditOle *me, CHARRANGE *lpchrg,
               DWORD reco, LPDATAOBJECT *lplpdataobj)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    CHARRANGE tmpchrg;

    TRACE("(%p,%p,%d)\n",This, lpchrg, reco);
    if(!lplpdataobj)
        return E_INVALIDARG;
    if(!lpchrg) {
        ME_GetSelection(This->editor, &tmpchrg.cpMin, &tmpchrg.cpMax);
        lpchrg = &tmpchrg;
    }
    return ME_GetDataObject(This->editor, lpchrg, lplpdataobj);
}

static LONG WINAPI IRichEditOle_fnGetLinkCount(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnGetObject(IRichEditOle *me, LONG iob,
               REOBJECT *lpreobject, DWORD dwFlags)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static LONG WINAPI
IRichEditOle_fnGetObjectCount(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return 0;
}

static HRESULT WINAPI
IRichEditOle_fnHandsOffStorage(IRichEditOle *me, LONG iob)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnImportDataObject(IRichEditOle *me, LPDATAOBJECT lpdataobj,
               CLIPFORMAT cf, HGLOBAL hMetaPict)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnInPlaceDeactivate(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnInsertObject(IRichEditOle *me, REOBJECT *reo)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    TRACE("(%p,%p)\n", This, reo);

    if (reo->cbStruct < sizeof(*reo)) return STG_E_INVALIDPARAMETER;
    if (reo->poleobj)   IOleObject_AddRef(reo->poleobj);
    if (reo->pstg)      IStorage_AddRef(reo->pstg);
    if (reo->polesite)  IOleClientSite_AddRef(reo->polesite);

    ME_InsertOLEFromCursor(This->editor, reo, 0);
    return S_OK;
}

static HRESULT WINAPI IRichEditOle_fnSaveCompleted(IRichEditOle *me, LONG iob,
               LPSTORAGE lpstg)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnSetDvaspect(IRichEditOle *me, LONG iob, DWORD dvaspect)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRichEditOle_fnSetHostNames(IRichEditOle *me,
               LPCSTR lpstrContainerApp, LPCSTR lpstrContainerObj)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p %s %s\n",This, lpstrContainerApp, lpstrContainerObj);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnSetLinkAvailable(IRichEditOle *me, LONG iob, BOOL fAvailable)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static const IRichEditOleVtbl revt = {
    IRichEditOle_fnQueryInterface,
    IRichEditOle_fnAddRef,
    IRichEditOle_fnRelease,
    IRichEditOle_fnGetClientSite,
    IRichEditOle_fnGetObjectCount,
    IRichEditOle_fnGetLinkCount,
    IRichEditOle_fnGetObject,
    IRichEditOle_fnInsertObject,
    IRichEditOle_fnConvertObject,
    IRichEditOle_fnActivateAs,
    IRichEditOle_fnSetHostNames,
    IRichEditOle_fnSetLinkAvailable,
    IRichEditOle_fnSetDvaspect,
    IRichEditOle_fnHandsOffStorage,
    IRichEditOle_fnSaveCompleted,
    IRichEditOle_fnInPlaceDeactivate,
    IRichEditOle_fnContextSensitiveHelp,
    IRichEditOle_fnGetClipboardData,
    IRichEditOle_fnImportDataObject
};

static HRESULT WINAPI
ITextDocument_fnQueryInterface(ITextDocument* me, REFIID riid,
    void** ppvObject)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    return IRichEditOle_fnQueryInterface((IRichEditOle*)&This->lpRichEditOleVtbl,
            riid, ppvObject);
}

static ULONG WINAPI
ITextDocument_fnAddRef(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    return IRichEditOle_fnAddRef((IRichEditOle*)&This->lpRichEditOleVtbl);
}

static ULONG WINAPI
ITextDocument_fnRelease(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    return IRichEditOle_fnRelease((IRichEditOle*)&This->lpRichEditOleVtbl);
}

static HRESULT WINAPI
ITextDocument_fnGetTypeInfoCount(ITextDocument* me,
    UINT* pctinfo)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetTypeInfo(ITextDocument* me, UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetIDsOfNames(ITextDocument* me, REFIID riid,
    LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnInvoke(ITextDocument* me, DISPID dispIdMember,
    REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
    VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetName(ITextDocument* me, BSTR* pName)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetSelection(ITextDocument* me, ITextSelection** ppSel)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    TRACE("(%p)\n", me);
    *ppSel = (ITextSelection *) This->txtSel;
    ITextSelection_AddRef(*ppSel);
    return S_OK;
}

static HRESULT WINAPI
ITextDocument_fnGetStoryCount(ITextDocument* me, long* pCount)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetStoryRanges(ITextDocument* me,
    ITextStoryRanges** ppStories)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetSaved(ITextDocument* me, long* pValue)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnSetSaved(ITextDocument* me, long Value)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetDefaultTabStop(ITextDocument* me, float* pValue)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnSetDefaultTabStop(ITextDocument* me, float Value)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnNew(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnOpen(ITextDocument* me, VARIANT* pVar, long Flags,
    long CodePage)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnSave(ITextDocument* me, VARIANT* pVar, long Flags,
    long CodePage)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnFreeze(ITextDocument* me, long* pCount)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnUnfreeze(ITextDocument* me, long* pCount)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnBeginEditCollection(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnEndEditCollection(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnUndo(ITextDocument* me, long Count, long* prop)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnRedo(ITextDocument* me, long Count, long* prop)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnRange(ITextDocument* me, long cp1, long cp2,
    ITextRange** ppRange)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnRangeFromPoint(ITextDocument* me, long x, long y,
    ITextRange** ppRange)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static const ITextDocumentVtbl tdvt = {
    ITextDocument_fnQueryInterface,
    ITextDocument_fnAddRef,
    ITextDocument_fnRelease,
    ITextDocument_fnGetTypeInfoCount,
    ITextDocument_fnGetTypeInfo,
    ITextDocument_fnGetIDsOfNames,
    ITextDocument_fnInvoke,
    ITextDocument_fnGetName,
    ITextDocument_fnGetSelection,
    ITextDocument_fnGetStoryCount,
    ITextDocument_fnGetStoryRanges,
    ITextDocument_fnGetSaved,
    ITextDocument_fnSetSaved,
    ITextDocument_fnGetDefaultTabStop,
    ITextDocument_fnSetDefaultTabStop,
    ITextDocument_fnNew,
    ITextDocument_fnOpen,
    ITextDocument_fnSave,
    ITextDocument_fnFreeze,
    ITextDocument_fnUnfreeze,
    ITextDocument_fnBeginEditCollection,
    ITextDocument_fnEndEditCollection,
    ITextDocument_fnUndo,
    ITextDocument_fnRedo,
    ITextDocument_fnRange,
    ITextDocument_fnRangeFromPoint
};

static HRESULT WINAPI ITextSelection_fnQueryInterface(
    ITextSelection *me,
    REFIID riid,
    void **ppvObj)
{
    *ppvObj = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDispatch)
        || IsEqualGUID(riid, &IID_ITextRange)
        || IsEqualGUID(riid, &IID_ITextSelection))
    {
        *ppvObj = me;
        ITextSelection_AddRef(me);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ITextSelection_fnAddRef(
    ITextSelection *me)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITextSelection_fnRelease(
    ITextSelection *me)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    ULONG ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        heap_free(This);
    return ref;
}

static HRESULT WINAPI ITextSelection_fnGetTypeInfoCount(
    ITextSelection *me,
    UINT *pctinfo)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetTypeInfo(
    ITextSelection *me,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetIDsOfNames(
    ITextSelection *me,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnInvoke(
    ITextSelection *me,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    FIXME("not implemented\n");
    return E_NOTIMPL;
}

/*** ITextRange methods ***/
static HRESULT WINAPI ITextSelection_fnGetText(
    ITextSelection *me,
    BSTR *pbstr)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetText(
    ITextSelection *me,
    BSTR bstr)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetChar(
    ITextSelection *me,
    long *pch)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetChar(
    ITextSelection *me,
    long ch)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetDuplicate(
    ITextSelection *me,
    ITextRange **ppRange)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetFormattedText(
    ITextSelection *me,
    ITextRange **ppRange)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetFormattedText(
    ITextSelection *me,
    ITextRange *pRange)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetStart(
    ITextSelection *me,
    long *pcpFirst)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetStart(
    ITextSelection *me,
    long cpFirst)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetEnd(
    ITextSelection *me,
    long *pcpLim)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetEnd(
    ITextSelection *me,
    long cpLim)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetFont(
    ITextSelection *me,
    ITextFont **pFont)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetFont(
    ITextSelection *me,
    ITextFont *pFont)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetPara(
    ITextSelection *me,
    ITextPara **ppPara)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetPara(
    ITextSelection *me,
    ITextPara *pPara)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetStoryLength(
    ITextSelection *me,
    long *pcch)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetStoryType(
    ITextSelection *me,
    long *pValue)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCollapse(
    ITextSelection *me,
    long bStart)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnExpand(
    ITextSelection *me,
    long Unit,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetIndex(
    ITextSelection *me,
    long Unit,
    long *pIndex)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetIndex(
    ITextSelection *me,
    long Unit,
    long Index,
    long Extend)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetRange(
    ITextSelection *me,
    long cpActive,
    long cpOther)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnInRange(
    ITextSelection *me,
    ITextRange *pRange,
    long *pb)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnInStory(
    ITextSelection *me,
    ITextRange *pRange,
    long *pb)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnIsEqual(
    ITextSelection *me,
    ITextRange *pRange,
    long *pb)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSelect(
    ITextSelection *me)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnStartOf(
    ITextSelection *me,
    long Unit,
    long Extend,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnEndOf(
    ITextSelection *me,
    long Unit,
    long Extend,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMove(
    ITextSelection *me,
    long Unit,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveStart(
    ITextSelection *me,
    long Unit,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveEnd(
    ITextSelection *me,
    long Unit,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveWhile(
    ITextSelection *me,
    VARIANT *Cset,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveStartWhile(
    ITextSelection *me,
    VARIANT *Cset,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveEndWhile(
    ITextSelection *me,
    VARIANT *Cset,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveUntil(
    ITextSelection *me,
    VARIANT *Cset,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveStartUntil(
    ITextSelection *me,
    VARIANT *Cset,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveEndUntil(
    ITextSelection *me,
    VARIANT *Cset,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindText(
    ITextSelection *me,
    BSTR bstr,
    long cch,
    long Flags,
    long *pLength)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindTextStart(
    ITextSelection *me,
    BSTR bstr,
    long cch,
    long Flags,
    long *pLength)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindTextEnd(
    ITextSelection *me,
    BSTR bstr,
    long cch,
    long Flags,
    long *pLength)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnDelete(
    ITextSelection *me,
    long Unit,
    long Count,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCut(
    ITextSelection *me,
    VARIANT *pVar)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCopy(
    ITextSelection *me,
    VARIANT *pVar)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnPaste(
    ITextSelection *me,
    VARIANT *pVar,
    long Format)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCanPaste(
    ITextSelection *me,
    VARIANT *pVar,
    long Format,
    long *pb)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCanEdit(
    ITextSelection *me,
    long *pb)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnChangeCase(
    ITextSelection *me,
    long Type)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetPoint(
    ITextSelection *me,
    long Type,
    long *cx,
    long *cy)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetPoint(
    ITextSelection *me,
    long x,
    long y,
    long Type,
    long Extend)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnScrollIntoView(
    ITextSelection *me,
    long Value)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetEmbeddedObject(
    ITextSelection *me,
    IUnknown **ppv)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

/*** ITextSelection methods ***/
static HRESULT WINAPI ITextSelection_fnGetFlags(
    ITextSelection *me,
    long *pFlags)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetFlags(
    ITextSelection *me,
    long Flags)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetType(
    ITextSelection *me,
    long *pType)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveLeft(
    ITextSelection *me,
    long Unit,
    long Count,
    long Extend,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveRight(
    ITextSelection *me,
    long Unit,
    long Count,
    long Extend,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveUp(
    ITextSelection *me,
    long Unit,
    long Count,
    long Extend,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveDown(
    ITextSelection *me,
    long Unit,
    long Count,
    long Extend,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnHomeKey(
    ITextSelection *me,
    long Unit,
    long Extend,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnEndKey(
    ITextSelection *me,
    long Unit,
    long Extend,
    long *pDelta)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnTypeText(
    ITextSelection *me,
    BSTR bstr)
{
    ITextSelectionImpl *This = (ITextSelectionImpl *) me;
    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static const ITextSelectionVtbl tsvt = {
    ITextSelection_fnQueryInterface,
    ITextSelection_fnAddRef,
    ITextSelection_fnRelease,
    ITextSelection_fnGetTypeInfoCount,
    ITextSelection_fnGetTypeInfo,
    ITextSelection_fnGetIDsOfNames,
    ITextSelection_fnInvoke,
    ITextSelection_fnGetText,
    ITextSelection_fnSetText,
    ITextSelection_fnGetChar,
    ITextSelection_fnSetChar,
    ITextSelection_fnGetDuplicate,
    ITextSelection_fnGetFormattedText,
    ITextSelection_fnSetFormattedText,
    ITextSelection_fnGetStart,
    ITextSelection_fnSetStart,
    ITextSelection_fnGetEnd,
    ITextSelection_fnSetEnd,
    ITextSelection_fnGetFont,
    ITextSelection_fnSetFont,
    ITextSelection_fnGetPara,
    ITextSelection_fnSetPara,
    ITextSelection_fnGetStoryLength,
    ITextSelection_fnGetStoryType,
    ITextSelection_fnCollapse,
    ITextSelection_fnExpand,
    ITextSelection_fnGetIndex,
    ITextSelection_fnSetIndex,
    ITextSelection_fnSetRange,
    ITextSelection_fnInRange,
    ITextSelection_fnInStory,
    ITextSelection_fnIsEqual,
    ITextSelection_fnSelect,
    ITextSelection_fnStartOf,
    ITextSelection_fnEndOf,
    ITextSelection_fnMove,
    ITextSelection_fnMoveStart,
    ITextSelection_fnMoveEnd,
    ITextSelection_fnMoveWhile,
    ITextSelection_fnMoveStartWhile,
    ITextSelection_fnMoveEndWhile,
    ITextSelection_fnMoveUntil,
    ITextSelection_fnMoveStartUntil,
    ITextSelection_fnMoveEndUntil,
    ITextSelection_fnFindText,
    ITextSelection_fnFindTextStart,
    ITextSelection_fnFindTextEnd,
    ITextSelection_fnDelete,
    ITextSelection_fnCut,
    ITextSelection_fnCopy,
    ITextSelection_fnPaste,
    ITextSelection_fnCanPaste,
    ITextSelection_fnCanEdit,
    ITextSelection_fnChangeCase,
    ITextSelection_fnGetPoint,
    ITextSelection_fnSetPoint,
    ITextSelection_fnScrollIntoView,
    ITextSelection_fnGetEmbeddedObject,
    ITextSelection_fnGetFlags,
    ITextSelection_fnSetFlags,
    ITextSelection_fnGetType,
    ITextSelection_fnMoveLeft,
    ITextSelection_fnMoveRight,
    ITextSelection_fnMoveUp,
    ITextSelection_fnMoveDown,
    ITextSelection_fnHomeKey,
    ITextSelection_fnEndKey,
    ITextSelection_fnTypeText
};

static ITextSelectionImpl *
CreateTextSelection(IRichEditOleImpl *reOle)
{
    ITextSelectionImpl *txtSel = heap_alloc(sizeof *txtSel);
    if (!txtSel)
        return NULL;

    txtSel->lpVtbl = &tsvt;
    txtSel->ref = 1;
    txtSel->reOle = reOle;
    return txtSel;
}

LRESULT CreateIRichEditOle(ME_TextEditor *editor, LPVOID *ppObj)
{
    IRichEditOleImpl *reo;

    reo = heap_alloc(sizeof(IRichEditOleImpl));
    if (!reo)
        return 0;

    reo->lpRichEditOleVtbl = &revt;
    reo->lpTextDocumentVtbl = &tdvt;
    reo->ref = 1;
    reo->editor = editor;
    reo->txtSel = CreateTextSelection(reo);
    if (!reo->txtSel)
    {
        heap_free(reo);
        return 0;
    }
    reo->clientSite = CreateOleClientSite(reo);
    if (!reo->txtSel)
    {
        ITextSelection_Release((ITextSelection *) reo->txtSel);
        heap_free(reo);
        return 0;
    }
    TRACE("Created %p\n",reo);
    *ppObj = reo;

    return 1;
}

static void convert_sizel(ME_Context *c, const SIZEL* szl, SIZE* sz)
{
  /* sizel is in .01 millimeters, sz in pixels */
  sz->cx = MulDiv(szl->cx, c->dpi.cx, 2540);
  sz->cy = MulDiv(szl->cy, c->dpi.cy, 2540);
}

/******************************************************************************
 * ME_GetOLEObjectSize
 *
 * Sets run extent for OLE objects.
 */
void ME_GetOLEObjectSize(ME_Context *c, ME_Run *run, SIZE *pSize)
{
  IDataObject*  ido;
  FORMATETC     fmt;
  STGMEDIUM     stgm;
  DIBSECTION    dibsect;
  ENHMETAHEADER emh;

  assert(run->nFlags & MERF_GRAPHICS);
  assert(run->ole_obj);

  if (run->ole_obj->sizel.cx != 0 || run->ole_obj->sizel.cy != 0)
  {
    convert_sizel(c, &run->ole_obj->sizel, pSize);
    return;
  }

  IOleObject_QueryInterface(run->ole_obj->poleobj, &IID_IDataObject, (void**)&ido);
  fmt.cfFormat = CF_BITMAP;
  fmt.ptd = NULL;
  fmt.dwAspect = DVASPECT_CONTENT;
  fmt.lindex = -1;
  fmt.tymed = TYMED_GDI;
  if (IDataObject_GetData(ido, &fmt, &stgm) != S_OK)
  {
    fmt.cfFormat = CF_ENHMETAFILE;
    fmt.tymed = TYMED_ENHMF;
    if (IDataObject_GetData(ido, &fmt, &stgm) != S_OK)
    {
      FIXME("unsupported format\n");
      pSize->cx = pSize->cy = 0;
      IDataObject_Release(ido);
      return;
    }
  }

  switch (stgm.tymed)
  {
  case TYMED_GDI:
    GetObjectW(stgm.u.hBitmap, sizeof(dibsect), &dibsect);
    pSize->cx = dibsect.dsBm.bmWidth;
    pSize->cy = dibsect.dsBm.bmHeight;
    if (!stgm.pUnkForRelease) DeleteObject(stgm.u.hBitmap);
    break;
  case TYMED_ENHMF:
    GetEnhMetaFileHeader(stgm.u.hEnhMetaFile, sizeof(emh), &emh);
    pSize->cx = emh.rclBounds.right - emh.rclBounds.left;
    pSize->cy = emh.rclBounds.bottom - emh.rclBounds.top;
    if (!stgm.pUnkForRelease) DeleteEnhMetaFile(stgm.u.hEnhMetaFile);
    break;
  default:
    FIXME("Unsupported tymed %d\n", stgm.tymed);
    break;
  }
  IDataObject_Release(ido);
  if (c->editor->nZoomNumerator != 0)
  {
    pSize->cx = MulDiv(pSize->cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    pSize->cy = MulDiv(pSize->cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
  }
}

void ME_DrawOLE(ME_Context *c, int x, int y, ME_Run *run,
                ME_Paragraph *para, BOOL selected)
{
  IDataObject*  ido;
  FORMATETC     fmt;
  STGMEDIUM     stgm;
  DIBSECTION    dibsect;
  ENHMETAHEADER emh;
  HDC           hMemDC;
  SIZE          sz;
  BOOL          has_size;

  assert(run->nFlags & MERF_GRAPHICS);
  assert(run->ole_obj);
  if (IOleObject_QueryInterface(run->ole_obj->poleobj, &IID_IDataObject, (void**)&ido) != S_OK)
  {
    FIXME("Couldn't get interface\n");
    return;
  }
  has_size = run->ole_obj->sizel.cx != 0 || run->ole_obj->sizel.cy != 0;
  fmt.cfFormat = CF_BITMAP;
  fmt.ptd = NULL;
  fmt.dwAspect = DVASPECT_CONTENT;
  fmt.lindex = -1;
  fmt.tymed = TYMED_GDI;
  if (IDataObject_GetData(ido, &fmt, &stgm) != S_OK)
  {
    fmt.cfFormat = CF_ENHMETAFILE;
    fmt.tymed = TYMED_ENHMF;
    if (IDataObject_GetData(ido, &fmt, &stgm) != S_OK)
    {
      FIXME("Couldn't get storage medium\n");
      IDataObject_Release(ido);
      return;
    }
  }
  switch (stgm.tymed)
  {
  case TYMED_GDI:
    GetObjectW(stgm.u.hBitmap, sizeof(dibsect), &dibsect);
    hMemDC = CreateCompatibleDC(c->hDC);
    SelectObject(hMemDC, stgm.u.hBitmap);
    if (!has_size && c->editor->nZoomNumerator == 0)
    {
      sz.cx = dibsect.dsBm.bmWidth;
      sz.cy = dibsect.dsBm.bmHeight;
      BitBlt(c->hDC, x, y - dibsect.dsBm.bmHeight,
             dibsect.dsBm.bmWidth, dibsect.dsBm.bmHeight,
             hMemDC, 0, 0, SRCCOPY);
    }
    else
    {
      if (has_size)
      {
        convert_sizel(c, &run->ole_obj->sizel, &sz);
      }
      else
      {
        sz.cx = MulDiv(dibsect.dsBm.bmWidth,
                       c->editor->nZoomNumerator, c->editor->nZoomDenominator);
        sz.cy = MulDiv(dibsect.dsBm.bmHeight,
                       c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      }
      StretchBlt(c->hDC, x, y - sz.cy, sz.cx, sz.cy,
                 hMemDC, 0, 0, dibsect.dsBm.bmWidth, dibsect.dsBm.bmHeight, SRCCOPY);
    }
    if (!stgm.pUnkForRelease) DeleteObject(stgm.u.hBitmap);
    break;
  case TYMED_ENHMF:
    GetEnhMetaFileHeader(stgm.u.hEnhMetaFile, sizeof(emh), &emh);
    if (!has_size && c->editor->nZoomNumerator == 0)
    {
      sz.cy = emh.rclBounds.bottom - emh.rclBounds.top;
      sz.cx = emh.rclBounds.right - emh.rclBounds.left;
    }
    else
    {
      if (has_size)
      {
        convert_sizel(c, &run->ole_obj->sizel, &sz);
      }
      else
      {
        sz.cy = MulDiv(emh.rclBounds.bottom - emh.rclBounds.top,
                       c->editor->nZoomNumerator, c->editor->nZoomDenominator);
        sz.cx = MulDiv(emh.rclBounds.right - emh.rclBounds.left,
                       c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      }
    }
    {
      RECT    rc;

      rc.left = x;
      rc.top = y - sz.cy;
      rc.right = x + sz.cx;
      rc.bottom = y;
      PlayEnhMetaFile(c->hDC, stgm.u.hEnhMetaFile, &rc);
    }
    if (!stgm.pUnkForRelease) DeleteEnhMetaFile(stgm.u.hEnhMetaFile);
    break;
  default:
    FIXME("Unsupported tymed %d\n", stgm.tymed);
    selected = FALSE;
    break;
  }
  if (selected && !c->editor->bHideSelection)
    PatBlt(c->hDC, x, y - sz.cy, sz.cx, sz.cy, DSTINVERT);
  IDataObject_Release(ido);
}

void ME_DeleteReObject(REOBJECT* reo)
{
    if (reo->poleobj)   IOleObject_Release(reo->poleobj);
    if (reo->pstg)      IStorage_Release(reo->pstg);
    if (reo->polesite)  IOleClientSite_Release(reo->polesite);
    FREE_OBJ(reo);
}

void ME_CopyReObject(REOBJECT* dst, const REOBJECT* src)
{
    *dst = *src;

    if (dst->poleobj)   IOleObject_AddRef(dst->poleobj);
    if (dst->pstg)      IStorage_AddRef(dst->pstg);
    if (dst->polesite)  IOleClientSite_AddRef(dst->polesite);
}
