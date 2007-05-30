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

typedef struct IRichEditOleImpl {
    const IRichEditOleVtbl *lpRichEditOleVtbl;
    const ITextDocumentVtbl *lpTextDocumentVtbl;
    LONG ref;

    ME_TextEditor *editor;
} IRichEditOleImpl;

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
        richedit_free(This);
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
IRichEditOle_fnGetClientSite(IRichEditOle *me,
               LPOLECLIENTSITE *lplpolesite)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
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
        ME_GetSelection(This->editor, (int*)&tmpchrg.cpMin, (int*)&tmpchrg.cpMax);
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
    return E_NOTIMPL;
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
IRichEditOle_fnInsertObject(IRichEditOle *me, REOBJECT *lpreobject)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
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
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
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

LRESULT CreateIRichEditOle(ME_TextEditor *editor, LPVOID *ppObj)
{
    IRichEditOleImpl *reo;

    reo = richedit_alloc(sizeof(IRichEditOleImpl));
    if (!reo)
        return 0;

    reo->lpRichEditOleVtbl = &revt;
    reo->lpTextDocumentVtbl = &tdvt;
    reo->ref = 1;
    reo->editor = editor;
    TRACE("Created %p\n",reo);
    *ppObj = (LPVOID) reo;

    return 1;
}
