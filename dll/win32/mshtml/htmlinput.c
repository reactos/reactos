/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    const IHTMLInputElementVtbl *lpHTMLInputElementVtbl;
    const IHTMLInputTextElementVtbl *lpHTMLInputTextElementVtbl;

    nsIDOMHTMLInputElement *nsinput;
} HTMLInputElement;

#define HTMLINPUT(x)      ((IHTMLInputElement*)      &(x)->lpHTMLInputElementVtbl)
#define HTMLINPUTTEXT(x)  ((IHTMLInputTextElement*)  &(x)->lpHTMLInputTextElementVtbl)

#define HTMLINPUT_THIS(iface) DEFINE_THIS(HTMLInputElement, HTMLInputElement, iface)

static HRESULT WINAPI HTMLInputElement_QueryInterface(IHTMLInputElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLInputElement_AddRef(IHTMLInputElement *iface)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLInputElement_Release(IHTMLInputElement *iface)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLInputElement_GetTypeInfoCount(IHTMLInputElement *iface, UINT *pctinfo)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLInputElement_GetTypeInfo(IHTMLInputElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLInputElement_GetIDsOfNames(IHTMLInputElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLInputElement_Invoke(IHTMLInputElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLInputElement_put_type(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_type(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    nsAString type_str;
    const PRUnichar *type;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&type_str, NULL);
    nsres = nsIDOMHTMLInputElement_GetType(This->nsinput, &type_str);

    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&type_str, &type);
        *p = SysAllocString(type);
    }else {
        ERR("GetType failed: %08x\n", nsres);
    }

    nsAString_Finish(&type_str);

    TRACE("type=%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_value(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    nsAString val_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_Init(&val_str, v);
    nsres = nsIDOMHTMLInputElement_SetValue(This->nsinput, &val_str);
    nsAString_Finish(&val_str);
    if(NS_FAILED(nsres))
        ERR("SetValue failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_value(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    nsAString value_str;
    const PRUnichar *value = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);

    nsres = nsIDOMHTMLInputElement_GetValue(This->nsinput, &value_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&value_str, &value);
        *p = SysAllocString(value);
    }else {
        ERR("GetValue failed: %08x\n", nsres);
    }

    nsAString_Finish(&value_str);

    TRACE("value=%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_name(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_name(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    nsAString name_str;
    const PRUnichar *name;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);

    nsres = nsIDOMHTMLInputElement_GetName(This->nsinput, &name_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&name_str, &name);
        *p = SysAllocString(name);
    }else {
        ERR("GetName failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Finish(&name_str);

    TRACE("name=%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_status(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_status(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_disabled(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLInputElement_SetDisabled(This->nsinput, v != VARIANT_FALSE);
    if(NS_FAILED(nsres))
        ERR("SetDisabled failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_disabled(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    PRBool disabled = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    nsIDOMHTMLInputElement_GetDisabled(This->nsinput, &disabled);

    *p = disabled ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_form(IHTMLInputElement *iface, IHTMLFormElement **p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_size(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_size(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_maxLength(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_maxLength(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_select(IHTMLInputElement *iface)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onchange(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onchange(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onselect(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onselect(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_defaultValue(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_defaultValue(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_readOnly(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_readOnly(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_createTextRange(IHTMLInputElement *iface, IHTMLTxtRange **range)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_indeterminate(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_indeterminate(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_defaultChecked(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_defaultChecked(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_checked(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_checked(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    PRBool checked;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLInputElement_GetChecked(This->nsinput, &checked);
    if(NS_FAILED(nsres)) {
        ERR("GetChecked failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = checked ? VARIANT_TRUE : VARIANT_FALSE;
    TRACE("checked=%x\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_border(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_border(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_vspace(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_vspace(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_hspace(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_hspace(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_alt(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_alt(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_src(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_src(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_lowsrc(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_lowsrc(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_vrml(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_vrml(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_dynsrc(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_dynsrc(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_readyState(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_complete(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_loop(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_loop(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_align(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_align(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onload(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onload(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onerror(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onerror(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onabort(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onabort(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_width(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_width(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_height(IHTMLInputElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_height(IHTMLInputElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_start(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_start(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLINPUT_THIS

static const IHTMLInputElementVtbl HTMLInputElementVtbl = {
    HTMLInputElement_QueryInterface,
    HTMLInputElement_AddRef,
    HTMLInputElement_Release,
    HTMLInputElement_GetTypeInfoCount,
    HTMLInputElement_GetTypeInfo,
    HTMLInputElement_GetIDsOfNames,
    HTMLInputElement_Invoke,
    HTMLInputElement_put_type,
    HTMLInputElement_get_type,
    HTMLInputElement_put_value,
    HTMLInputElement_get_value,
    HTMLInputElement_put_name,
    HTMLInputElement_get_name,
    HTMLInputElement_put_status,
    HTMLInputElement_get_status,
    HTMLInputElement_put_disabled,
    HTMLInputElement_get_disabled,
    HTMLInputElement_get_form,
    HTMLInputElement_put_size,
    HTMLInputElement_get_size,
    HTMLInputElement_put_maxLength,
    HTMLInputElement_get_maxLength,
    HTMLInputElement_select,
    HTMLInputElement_put_onchange,
    HTMLInputElement_get_onchange,
    HTMLInputElement_put_onselect,
    HTMLInputElement_get_onselect,
    HTMLInputElement_put_defaultValue,
    HTMLInputElement_get_defaultValue,
    HTMLInputElement_put_readOnly,
    HTMLInputElement_get_readOnly,
    HTMLInputElement_createTextRange,
    HTMLInputElement_put_indeterminate,
    HTMLInputElement_get_indeterminate,
    HTMLInputElement_put_defaultChecked,
    HTMLInputElement_get_defaultChecked,
    HTMLInputElement_put_checked,
    HTMLInputElement_get_checked,
    HTMLInputElement_put_border,
    HTMLInputElement_get_border,
    HTMLInputElement_put_vspace,
    HTMLInputElement_get_vspace,
    HTMLInputElement_put_hspace,
    HTMLInputElement_get_hspace,
    HTMLInputElement_put_alt,
    HTMLInputElement_get_alt,
    HTMLInputElement_put_src,
    HTMLInputElement_get_src,
    HTMLInputElement_put_lowsrc,
    HTMLInputElement_get_lowsrc,
    HTMLInputElement_put_vrml,
    HTMLInputElement_get_vrml,
    HTMLInputElement_put_dynsrc,
    HTMLInputElement_get_dynsrc,
    HTMLInputElement_get_readyState,
    HTMLInputElement_get_complete,
    HTMLInputElement_put_loop,
    HTMLInputElement_get_loop,
    HTMLInputElement_put_align,
    HTMLInputElement_get_align,
    HTMLInputElement_put_onload,
    HTMLInputElement_get_onload,
    HTMLInputElement_put_onerror,
    HTMLInputElement_get_onerror,
    HTMLInputElement_put_onabort,
    HTMLInputElement_get_onabort,
    HTMLInputElement_put_width,
    HTMLInputElement_get_width,
    HTMLInputElement_put_height,
    HTMLInputElement_get_height,
    HTMLInputElement_put_start,
    HTMLInputElement_get_start
};

#define HTMLINPUTTEXT_THIS(iface) DEFINE_THIS(HTMLInputElement, HTMLInputTextElement, iface)

static HRESULT WINAPI HTMLInputTextElement_QueryInterface(IHTMLInputTextElement *iface,
        REFIID riid, void **ppv)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLInputTextElement_AddRef(IHTMLInputTextElement *iface)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLInputTextElement_Release(IHTMLInputTextElement *iface)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLInputTextElement_GetTypeInfoCount(IHTMLInputTextElement *iface, UINT *pctinfo)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputTextElement_GetTypeInfo(IHTMLInputTextElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputTextElement_GetIDsOfNames(IHTMLInputTextElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputTextElement_Invoke(IHTMLInputTextElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputTextElement_get_type(IHTMLInputTextElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_type(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_put_value(IHTMLInputTextElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLInputElement_put_value(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_value(IHTMLInputTextElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputTextElement_get_value(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_put_name(IHTMLInputTextElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLInputElement_put_name(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_name(IHTMLInputTextElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_name(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_put_status(IHTMLInputTextElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputTextElement_get_status(IHTMLInputTextElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);
    TRACE("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputTextElement_put_disabled(IHTMLInputTextElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return IHTMLInputElement_put_disabled(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_disabled(IHTMLInputTextElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_disabled(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_get_form(IHTMLInputTextElement *iface, IHTMLFormElement **p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_form(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_put_defaultValue(IHTMLInputTextElement *iface, BSTR v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLInputElement_put_defaultValue(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_defaultValue(IHTMLInputTextElement *iface, BSTR *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_defaultValue(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_put_size(IHTMLInputTextElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return IHTMLInputElement_put_size(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_size(IHTMLInputTextElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_size(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_put_maxLength(IHTMLInputTextElement *iface, long v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return IHTMLInputElement_put_maxLength(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_maxLength(IHTMLInputTextElement *iface, long *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_maxLength(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_select(IHTMLInputTextElement *iface)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)\n", This);

    return IHTMLInputElement_select(HTMLINPUT(This));
}

static HRESULT WINAPI HTMLInputTextElement_put_onchange(IHTMLInputTextElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLInputElement_put_onchange(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_onchange(IHTMLInputTextElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_onchange(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_put_onselect(IHTMLInputTextElement *iface, VARIANT v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLInputElement_put_onselect(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_onselect(IHTMLInputTextElement *iface, VARIANT *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_onselect(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_put_readOnly(IHTMLInputTextElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return IHTMLInputElement_put_readOnly(HTMLINPUT(This), v);
}

static HRESULT WINAPI HTMLInputTextElement_get_readOnly(IHTMLInputTextElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_readOnly(HTMLINPUT(This), p);
}

static HRESULT WINAPI HTMLInputTextElement_createTextRange(IHTMLInputTextElement *iface, IHTMLTxtRange **range)
{
    HTMLInputElement *This = HTMLINPUTTEXT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, range);

    return IHTMLInputElement_createTextRange(HTMLINPUT(This), range);
}

#undef HTMLINPUT_THIS

static const IHTMLInputTextElementVtbl HTMLInputTextElementVtbl = {
    HTMLInputTextElement_QueryInterface,
    HTMLInputTextElement_AddRef,
    HTMLInputTextElement_Release,
    HTMLInputTextElement_GetTypeInfoCount,
    HTMLInputTextElement_GetTypeInfo,
    HTMLInputTextElement_GetIDsOfNames,
    HTMLInputTextElement_Invoke,
    HTMLInputTextElement_get_type,
    HTMLInputTextElement_put_value,
    HTMLInputTextElement_get_value,
    HTMLInputTextElement_put_name,
    HTMLInputTextElement_get_name,
    HTMLInputTextElement_put_status,
    HTMLInputTextElement_get_status,
    HTMLInputTextElement_put_disabled,
    HTMLInputTextElement_get_disabled,
    HTMLInputTextElement_get_form,
    HTMLInputTextElement_put_defaultValue,
    HTMLInputTextElement_get_defaultValue,
    HTMLInputTextElement_put_size,
    HTMLInputTextElement_get_size,
    HTMLInputTextElement_put_maxLength,
    HTMLInputTextElement_get_maxLength,
    HTMLInputTextElement_select,
    HTMLInputTextElement_put_onchange,
    HTMLInputTextElement_get_onchange,
    HTMLInputTextElement_put_onselect,
    HTMLInputTextElement_get_onselect,
    HTMLInputTextElement_put_readOnly,
    HTMLInputTextElement_get_readOnly,
    HTMLInputTextElement_createTextRange
};

#define HTMLINPUT_NODE_THIS(iface) DEFINE_THIS2(HTMLInputElement, element.node, iface)

static HRESULT HTMLInputElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLInputElement *This = HTMLINPUT_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLINPUT(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLINPUT(This);
    }else if(IsEqualGUID(&IID_IHTMLInputElement, riid)) {
        TRACE("(%p)->(IID_IHTMLInputElement %p)\n", This, ppv);
        *ppv = HTMLINPUT(This);
    }else if(IsEqualGUID(&IID_IHTMLInputTextElement, riid)) {
        TRACE("(%p)->(IID_IHTMLInputTextElement %p)\n", This, ppv);
        *ppv = HTMLINPUT(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLInputElement_destructor(HTMLDOMNode *iface)
{
    HTMLInputElement *This = HTMLINPUT_NODE_THIS(iface);

    nsIDOMHTMLInputElement_Release(This->nsinput);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLINPUT_NODE_THIS

static const NodeImplVtbl HTMLInputElementImplVtbl = {
    HTMLInputElement_QI,
    HTMLInputElement_destructor
};

static const tid_t HTMLInputElement_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLInputElement_tid,
    0
};
static dispex_static_data_t HTMLInputElement_dispex = {
    NULL,
    DispHTMLInputElement_tid,
    NULL,
    HTMLInputElement_iface_tids
};

HTMLElement *HTMLInputElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLInputElement *ret = heap_alloc_zero(sizeof(HTMLInputElement));
    nsresult nsres;

    ret->lpHTMLInputElementVtbl = &HTMLInputElementVtbl;
    ret->element.node.vtbl = &HTMLInputElementImplVtbl;

    init_dispex(&ret->element.node.dispex, (IUnknown*)HTMLINPUT(ret), &HTMLInputElement_dispex);
    HTMLElement_Init(&ret->element);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLInputElement,
                                             (void**)&ret->nsinput);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLInputElement interface: %08x\n", nsres);

    return &ret->element;
}
