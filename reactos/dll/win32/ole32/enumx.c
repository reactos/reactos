/*
 * IEnum* implementation
 *
 * Copyright 2006 Mike McCormack
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

struct tagEnumSTATPROPSETSTG_impl
{
    const void *vtbl;
    LONG ref;
    struct list elements;
    struct list *current;
    ULONG elem_size;
    GUID riid;
    IUnknown *parent;
    enumx_copy_cb copy_cb;
};

/************************************************************************
 * enumx_QueryInterface
 */
HRESULT WINAPI enumx_QueryInterface(
    enumx_impl *This,
    REFIID riid,
    void** ppvObject)
{
    if ( ppvObject==0 )
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&This->riid, riid))
    {
        IUnknown_AddRef(((IUnknown*)This));
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

/************************************************************************
 * enumx_AddRef
 */
ULONG WINAPI enumx_AddRef(enumx_impl *This)
{
    return InterlockedIncrement(&This->ref);
}

/************************************************************************
 * enumx_Release
 */
ULONG WINAPI enumx_Release(enumx_impl *This)
{
    ULONG ref;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        while (!list_empty(&This->elements))
        {
             struct list *x = list_head(&This->elements);
             list_remove(x);
             HeapFree(GetProcessHeap(), 0, x);
        }
        IUnknown_Release(This->parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/************************************************************************
 * enumx_Next
 */
HRESULT WINAPI enumx_Next(enumx_impl *This, ULONG celt,
                                 void *rgelt, ULONG *pceltFetched)
{
    unsigned char *p;
    ULONG count = 0;

    TRACE("%p %u %p\n", This, celt, pceltFetched);

    if (This->current == NULL)
        This->current = list_head(&This->elements);
    p = rgelt;
    while (count < celt && This->current && This->current != &This->elements)
    {
        if (This->copy_cb)
            This->copy_cb(This->parent, &This->current[1], p);
        else
            memcpy(p, &This->current[1], This->elem_size);
        p += This->elem_size;
        This->current = This->current->next;
        count++;
    }
    if (pceltFetched)
        *pceltFetched = count;
    if (count < celt)
        return S_FALSE;
    return S_OK;
}

/************************************************************************
 * enumx_Skip
 */
HRESULT WINAPI enumx_Skip(enumx_impl *This, ULONG celt)
{
    ULONG count = 0;

    TRACE("%p %u\n", This, celt);

    if (This->current == NULL)
        This->current = list_head(&This->elements);

    while (count < celt && This->current && This->current != &This->elements)
        count++;

    return S_OK;
}

/************************************************************************
 * enumx_Reset
 */
HRESULT WINAPI enumx_Reset(enumx_impl *This)
{
    TRACE("\n");

    This->current = NULL;
    return S_OK;
}

/************************************************************************
 * enumx_fnClone
 */
HRESULT WINAPI enumx_Clone(
    enumx_impl *iface,
    enumx_impl **ppenum)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * enumx_allocate
 *
 * Allocate a generic enumerator
 */
enumx_impl *enumx_allocate(REFIID riid, const void *vtbl, ULONG elem_size,
                           IUnknown *parent, enumx_copy_cb copy_cb)
{
    enumx_impl *enumx;

    enumx = HeapAlloc(GetProcessHeap(), 0, sizeof *enumx);
    if (enumx)
    {
        enumx->vtbl = vtbl;
        enumx->ref = 1;
        enumx->current = NULL;
        enumx->elem_size = elem_size;
        enumx->riid = *riid;
        enumx->parent = parent;
        enumx->copy_cb = copy_cb;

        IUnknown_AddRef(parent);

        list_init(&enumx->elements);
    }

    return enumx;
}

/************************************************************************
 * enumx_add_element
 *
 * Add an element to the enumeration.
 */
void *enumx_add_element(enumx_impl *enumx, const void *data)
{
    struct list *element;

    element = HeapAlloc(GetProcessHeap(), 0, sizeof *element + enumx->elem_size);
    if (!element)
        return NULL;
    memcpy(&element[1], data, enumx->elem_size);
    list_add_tail(&enumx->elements, element);
    return &element[1];
}
