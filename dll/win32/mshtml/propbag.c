/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#include "mshtml_private.h"

typedef struct {
    IPropertyBag  IPropertyBag_iface;
    IPropertyBag2 IPropertyBag2_iface;

    LONG ref;

    struct list props;
} PropertyBag;

typedef struct {
    struct list entry;
    WCHAR *name;
    WCHAR *value;
} param_prop_t;

static void free_prop(param_prop_t *prop)
{
    list_remove(&prop->entry);

    heap_free(prop->name);
    heap_free(prop->value);
    heap_free(prop);
}

static param_prop_t *find_prop(PropertyBag *prop_bag, const WCHAR *name)
{
    param_prop_t *iter;

    LIST_FOR_EACH_ENTRY(iter, &prop_bag->props, param_prop_t, entry) {
        if(!strcmpiW(iter->name, name))
            return iter;
    }

    return NULL;
}

static HRESULT add_prop(PropertyBag *prop_bag, const WCHAR *name, const WCHAR *value)
{
    param_prop_t *prop;

    if(!name || !value)
        return S_OK;

    TRACE("%p %s %s\n", prop_bag, debugstr_w(name), debugstr_w(value));

    prop = heap_alloc(sizeof(*prop));
    if(!prop)
        return E_OUTOFMEMORY;

    prop->name = heap_strdupW(name);
    prop->value = heap_strdupW(value);
    if(!prop->name || !prop->value) {
        list_init(&prop->entry);
        free_prop(prop);
        return E_OUTOFMEMORY;
    }

    list_add_tail(&prop_bag->props, &prop->entry);
    return S_OK;
}

static inline PropertyBag *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, PropertyBag, IPropertyBag_iface);
}

static HRESULT WINAPI PropertyBag_QueryInterface(IPropertyBag *iface, REFIID riid, void **ppv)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IPropertyBag_iface;
    }else if(IsEqualGUID(&IID_IPropertyBag, riid)) {
        TRACE("(%p)->(IID_IPropertyBag %p)\n", This, ppv);
        *ppv = &This->IPropertyBag_iface;
    }else if(IsEqualGUID(&IID_IPropertyBag2, riid)) {
        TRACE("(%p)->(IID_IPropertyBag2 %p)\n", This, ppv);
        *ppv = &This->IPropertyBag2_iface;
    }else {
        WARN("Unsopported interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PropertyBag_AddRef(IPropertyBag *iface)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI PropertyBag_Release(IPropertyBag *iface)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        while(!list_empty(&This->props))
            free_prop(LIST_ENTRY(This->props.next, param_prop_t, entry));
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI PropertyBag_Read(IPropertyBag *iface, LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);
    param_prop_t *prop;
    VARIANT v;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(pszPropName), pVar, pErrorLog);

    prop = find_prop(This, pszPropName);
    if(!prop) {
        TRACE("Not found\n");
        return E_INVALIDARG;
    }

    V_BSTR(&v) = SysAllocString(prop->value);
    if(!V_BSTR(&v))
        return E_OUTOFMEMORY;

    if(V_VT(pVar) != VT_BSTR) {
        HRESULT hres;

        V_VT(&v) = VT_BSTR;
        hres = VariantChangeType(pVar, &v, 0, V_VT(pVar));
        SysFreeString(V_BSTR(&v));
        return hres;
    }

    V_BSTR(pVar) = V_BSTR(&v);
    return S_OK;
}

static HRESULT WINAPI PropertyBag_Write(IPropertyBag *iface, LPCOLESTR pszPropName, VARIANT *pVar)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(pszPropName), debugstr_variant(pVar));
    return E_NOTIMPL;
}

static const IPropertyBagVtbl PropertyBagVtbl = {
    PropertyBag_QueryInterface,
    PropertyBag_AddRef,
    PropertyBag_Release,
    PropertyBag_Read,
    PropertyBag_Write
};

static inline PropertyBag *impl_from_IPropertyBag2(IPropertyBag2 *iface)
{
    return CONTAINING_RECORD(iface, PropertyBag, IPropertyBag2_iface);
}

static HRESULT WINAPI PropertyBag2_QueryInterface(IPropertyBag2 *iface, REFIID riid, void **ppv)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    return IPropertyBag_QueryInterface(&This->IPropertyBag_iface, riid, ppv);
}

static ULONG WINAPI PropertyBag2_AddRef(IPropertyBag2 *iface)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    return IPropertyBag_AddRef(&This->IPropertyBag_iface);
}

static ULONG WINAPI PropertyBag2_Release(IPropertyBag2 *iface)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    return IPropertyBag_Release(&This->IPropertyBag_iface);
}

static HRESULT WINAPI PropertyBag2_Read(IPropertyBag2 *iface, ULONG cProperties, PROPBAG2 *pPropBag,
        IErrorLog *pErrLog, VARIANT *pvarValue, HRESULT *phrError)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%d %p %p %p %p)\n", This, cProperties, pPropBag, pErrLog, pvarValue, phrError);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag2_Write(IPropertyBag2 *iface, ULONG cProperties, PROPBAG2 *pPropBag, VARIANT *pvarValue)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%d %p %s)\n", This, cProperties, pPropBag, debugstr_variant(pvarValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag2_CountProperties(IPropertyBag2 *iface, ULONG *pcProperties)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%p)\n", This, pcProperties);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag2_GetPropertyInfo(IPropertyBag2 *iface, ULONG iProperty, ULONG cProperties,
        PROPBAG2 *pPropBag, ULONG *pcProperties)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%u %u %p %p)\n", This, iProperty, cProperties, pPropBag, pcProperties);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag2_LoadObject(IPropertyBag2 *iface, LPCOLESTR pstrName, DWORD dwHint,
        IUnknown *pUnkObject, IErrorLog *pErrLog)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%s %x %p %p)\n", This, debugstr_w(pstrName), dwHint, pUnkObject, pErrLog);
    return E_NOTIMPL;
}

static const IPropertyBag2Vtbl PropertyBag2Vtbl = {
    PropertyBag2_QueryInterface,
    PropertyBag2_AddRef,
    PropertyBag2_Release,
    PropertyBag2_Read,
    PropertyBag2_Write,
    PropertyBag2_CountProperties,
    PropertyBag2_GetPropertyInfo,
    PropertyBag2_LoadObject
};

static HRESULT fill_props(nsIDOMHTMLElement *nselem, PropertyBag *prop_bag)
{
    const PRUnichar *name, *value;
    nsAString name_str, value_str;
    nsIDOMHTMLCollection *params;
    nsIDOMHTMLElement *param_elem;
    UINT32 length, i;
    nsIDOMNode *nsnode;
    nsresult nsres;
    HRESULT hres = S_OK;

    static const PRUnichar nameW[] = {'n','a','m','e',0};
    static const PRUnichar paramW[] = {'p','a','r','a','m',0};
    static const PRUnichar valueW[] = {'v','a','l','u','e',0};

    nsAString_InitDepend(&name_str, paramW);
    nsres = nsIDOMHTMLElement_GetElementsByTagName(nselem, &name_str, &params);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres))
        return E_FAIL;

    nsres = nsIDOMHTMLCollection_GetLength(params, &length);
    if(NS_FAILED(nsres))
        length = 0;

    for(i=0; i < length; i++) {
        nsres = nsIDOMHTMLCollection_Item(params, i, &nsnode);
        if(NS_FAILED(nsres)) {
            hres = E_FAIL;
            break;
        }

        nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMHTMLElement, (void**)&param_elem);
        nsIDOMNode_Release(nsnode);
        if(NS_FAILED(nsres)) {
            hres = E_FAIL;
            break;
        }

        nsres = get_elem_attr_value(param_elem, nameW, &name_str, &name);
        if(NS_SUCCEEDED(nsres)) {
            nsres = get_elem_attr_value(param_elem, valueW, &value_str, &value);
            if(NS_SUCCEEDED(nsres)) {
                hres = add_prop(prop_bag, name, value);
                nsAString_Finish(&value_str);
            }

            nsAString_Finish(&name_str);
        }

        nsIDOMHTMLElement_Release(param_elem);
        if(FAILED(hres))
            break;
        if(NS_FAILED(nsres)) {
            hres = E_FAIL;
            break;
        }
    }

    nsIDOMHTMLCollection_Release(params);
    return hres;
}

HRESULT create_param_prop_bag(nsIDOMHTMLElement *nselem, IPropertyBag **ret)
{
    PropertyBag *prop_bag;
    HRESULT hres;

    prop_bag = heap_alloc(sizeof(*prop_bag));
    if(!prop_bag)
        return E_OUTOFMEMORY;

    prop_bag->IPropertyBag_iface.lpVtbl  = &PropertyBagVtbl;
    prop_bag->IPropertyBag2_iface.lpVtbl = &PropertyBag2Vtbl;
    prop_bag->ref = 1;

    list_init(&prop_bag->props);
    hres = fill_props(nselem, prop_bag);
    if(FAILED(hres) || list_empty(&prop_bag->props)) {
        IPropertyBag_Release(&prop_bag->IPropertyBag_iface);
        *ret = NULL;
        return hres;
    }

    *ret = &prop_bag->IPropertyBag_iface;
    return S_OK;
}
