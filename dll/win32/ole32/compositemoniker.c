/*
 * CompositeMonikers implementation
 *
 * Copyright 1999  Noomen Hamza
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "ole2.h"
#include "moniker.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct CompositeMonikerImpl
{
    IMoniker IMoniker_iface;
    IROTData IROTData_iface;
    IMarshal IMarshal_iface;
    LONG ref;

    IMoniker *left;
    IMoniker *right;
    unsigned int comp_count;
} CompositeMonikerImpl;

static inline CompositeMonikerImpl *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, CompositeMonikerImpl, IMoniker_iface);
}

static const IMonikerVtbl VT_CompositeMonikerImpl;

static CompositeMonikerImpl *unsafe_impl_from_IMoniker(IMoniker *iface)
{
    if (iface->lpVtbl != &VT_CompositeMonikerImpl)
        return NULL;
    return CONTAINING_RECORD(iface, CompositeMonikerImpl, IMoniker_iface);
}

static inline CompositeMonikerImpl *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, CompositeMonikerImpl, IROTData_iface);
}

static inline CompositeMonikerImpl *impl_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, CompositeMonikerImpl, IMarshal_iface);
}

typedef struct EnumMonikerImpl
{
    IEnumMoniker IEnumMoniker_iface;
    LONG ref;
    IMoniker **monikers;
    unsigned int count;
    unsigned int pos;
} EnumMonikerImpl;

static inline EnumMonikerImpl *impl_from_IEnumMoniker(IEnumMoniker *iface)
{
    return CONTAINING_RECORD(iface, EnumMonikerImpl, IEnumMoniker_iface);
}

static HRESULT create_enumerator(IMoniker **components, unsigned int count, BOOL forward, IEnumMoniker **ret);
static HRESULT composite_get_rightmost(CompositeMonikerImpl *composite, IMoniker **left, IMoniker **rightmost);
static HRESULT composite_get_leftmost(CompositeMonikerImpl *composite, IMoniker **leftmost);

/*******************************************************************************
 *        CompositeMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    CompositeMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( ppvObject==0 )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid)
       )
        *ppvObject = iface;
    else if (IsEqualIID(&IID_IROTData, riid))
        *ppvObject = &This->IROTData_iface;
    else if (IsEqualIID(&IID_IMarshal, riid))
        *ppvObject = &This->IMarshal_iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI
CompositeMonikerImpl_AddRef(IMoniker* iface)
{
    CompositeMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI CompositeMonikerImpl_Release(IMoniker* iface)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->ref);

    TRACE("%p, refcount %lu\n", iface, refcount);

    if (!refcount)
    {
        if (moniker->left) IMoniker_Release(moniker->left);
        if (moniker->right) IMoniker_Release(moniker->right);
        free(moniker);
    }

    return refcount;
}

/******************************************************************************
 *        CompositeMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_CompositeMoniker;

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

static HRESULT WINAPI CompositeMonikerImpl_Load(IMoniker *iface, IStream *stream)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *last, *m, *c;
    DWORD i, count;
    HRESULT hr;

    TRACE("%p, %p\n", iface, stream);

    if (moniker->comp_count)
        return E_UNEXPECTED;

    hr = IStream_Read(stream, &count, sizeof(DWORD), NULL);
    if (hr != S_OK)
    {
        WARN("Failed to read component count, hr %#lx.\n", hr);
        return hr;
    }

    if (count < 2)
    {
        WARN("Unexpected component count %lu.\n", count);
        return E_UNEXPECTED;
    }

    if (FAILED(hr = OleLoadFromStream(stream, &IID_IMoniker, (void **)&last)))
        return hr;

    for (i = 1; i < count - 1; ++i)
    {
        if (FAILED(hr = OleLoadFromStream(stream, &IID_IMoniker, (void **)&m)))
        {
            WARN("Failed to initialize component %lu, hr %#lx.\n", i, hr);
            IMoniker_Release(last);
            return hr;
        }
        hr = CreateGenericComposite(last, m, &c);
        IMoniker_Release(last);
        IMoniker_Release(m);
        if (FAILED(hr)) return hr;
        last = c;
    }

    if (FAILED(hr = OleLoadFromStream(stream, &IID_IMoniker, (void **)&m)))
    {
        IMoniker_Release(last);
        return hr;
    }

    moniker->left = last;
    moniker->right = m;
    moniker->comp_count = count;

    return hr;
}

static HRESULT composite_save_components(IMoniker *moniker, IStream *stream)
{
    CompositeMonikerImpl *comp_moniker;
    HRESULT hr;

    if ((comp_moniker = unsafe_impl_from_IMoniker(moniker)))
    {
        if (SUCCEEDED(hr = composite_save_components(comp_moniker->left, stream)))
            hr = composite_save_components(comp_moniker->right, stream);
    }
    else
        hr = OleSaveToStream((IPersistStream *)moniker, stream);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_Save(IMoniker *iface, IStream *stream, BOOL clear_dirty)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    HRESULT hr;

    TRACE("%p, %p, %d\n", iface, stream, clear_dirty);

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    hr = IStream_Write(stream, &moniker->comp_count, sizeof(moniker->comp_count), NULL);
    if (FAILED(hr)) return hr;

    return composite_save_components(iface, stream);
}

/******************************************************************************
 *        CompositeMoniker_GetSizeMax
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_GetSizeMax(IMoniker* iface,ULARGE_INTEGER* pcbSize)
{
    IEnumMoniker *enumMk;
    IMoniker *pmk;
    ULARGE_INTEGER ptmpSize;

    /* The sizeMax of this object is calculated by calling  GetSizeMax on
     * each moniker within this object then summing all returned values
     */

    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    pcbSize->QuadPart = sizeof(DWORD);

    IMoniker_Enum(iface,TRUE,&enumMk);

    while(IEnumMoniker_Next(enumMk,1,&pmk,NULL)==S_OK){

        IMoniker_GetSizeMax(pmk,&ptmpSize);

        IMoniker_Release(pmk);

        pcbSize->QuadPart += ptmpSize.QuadPart + sizeof(CLSID);
    }

    IEnumMoniker_Release(enumMk);

    return S_OK;
}

static HRESULT compose_with(IMoniker *left, IMoniker *right, IMoniker **c)
{
    HRESULT hr = IMoniker_ComposeWith(left, right, TRUE, c);
    if (FAILED(hr) && hr != MK_E_NEEDGENERIC) return hr;
    return CreateGenericComposite(left, right, c);
}

static HRESULT WINAPI CompositeMonikerImpl_BindToObject(IMoniker *iface, IBindCtx *pbc,
        IMoniker *toleft, REFIID riid, void **result)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *left, *rightmost, *c;
    IRunningObjectTable *rot;
    IUnknown *object;
    HRESULT hr;

    TRACE("%p, %p, %p, %s, %p.\n", iface, pbc, toleft, debugstr_guid(riid), result);

    if (!result)
        return E_POINTER;

    *result = NULL;

    if (!toleft)
    {
        hr = IBindCtx_GetRunningObjectTable(pbc, &rot);
        if (SUCCEEDED(hr))
        {
            hr = IRunningObjectTable_GetObject(rot, iface, &object);
            IRunningObjectTable_Release(rot);
            if (FAILED(hr)) return E_INVALIDARG;

            hr = IUnknown_QueryInterface(object, riid, result);
            IUnknown_Release(object);
        }

        return hr;
    }

    /* Try to bind rightmost component with (toleft, composite->left) composite at its left side */
    if (FAILED(hr = composite_get_rightmost(moniker, &left, &rightmost)))
        return hr;

    hr = compose_with(toleft, left, &c);
    IMoniker_Release(left);

    if (SUCCEEDED(hr))
    {
        hr = IMoniker_BindToObject(rightmost, pbc, c, riid, result);
        IMoniker_Release(c);
    }

    IMoniker_Release(rightmost);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_BindToStorage(IMoniker *iface, IBindCtx *pbc,
        IMoniker *toleft, REFIID riid, void **result)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *left, *rightmost, *composed_left;
    HRESULT hr;

    TRACE("%p, %p, %p, %s, %p.\n", iface, pbc, toleft, debugstr_guid(riid), result);

    *result = NULL;

    if (FAILED(hr = composite_get_rightmost(moniker, &left, &rightmost)))
        return hr;

    if (toleft)
    {
        hr = compose_with(toleft, left, &composed_left);
    }
    else
    {
        composed_left = left;
        IMoniker_AddRef(composed_left);
    }

    if (SUCCEEDED(hr))
    {
        hr = IMoniker_BindToStorage(rightmost, pbc, composed_left, riid, result);
        IMoniker_Release(composed_left);
    }

    IMoniker_Release(rightmost);
    IMoniker_Release(left);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_Reduce(IMoniker *iface, IBindCtx *pbc, DWORD howfar,
        IMoniker **toleft, IMoniker **reduced)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *m, *reduced_left, *reduced_right;
    BOOL was_reduced;
    HRESULT hr;

    TRACE("%p, %p, %ld, %p, %p.\n", iface, pbc, howfar, toleft, reduced);

    if (!pbc || !reduced)
        return E_INVALIDARG;

    if (FAILED(hr = IMoniker_Reduce(moniker->left, pbc, howfar, NULL, &reduced_left)))
        return hr;

    m = moniker->left;
    if (FAILED(hr = IMoniker_Reduce(moniker->right, pbc, howfar, &m, &reduced_right)))
    {
        IMoniker_Release(reduced_left);
        return hr;
    }

    if ((was_reduced = (reduced_left != moniker->left || reduced_right != moniker->right)))
    {
        hr = CreateGenericComposite(reduced_left, reduced_right, reduced);
    }
    else
    {
        *reduced = iface;
        IMoniker_AddRef(*reduced);
    }

    IMoniker_Release(reduced_left);
    IMoniker_Release(reduced_right);

    return was_reduced ? hr : MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI CompositeMonikerImpl_ComposeWith(IMoniker *iface, IMoniker *right,
        BOOL only_if_not_generic, IMoniker **composite)
{
    TRACE("%p, %p, %d, %p.\n", iface, right, only_if_not_generic, composite);

    *composite = NULL;

    return only_if_not_generic ? MK_E_NEEDGENERIC : CreateGenericComposite(iface, right, composite);
}

static void composite_get_components(IMoniker *moniker, IMoniker **components, unsigned int *index)
{
    CompositeMonikerImpl *comp_moniker;

    if ((comp_moniker = unsafe_impl_from_IMoniker(moniker)))
    {
        composite_get_components(comp_moniker->left, components, index);
        composite_get_components(comp_moniker->right, components, index);
    }
    else
    {
        components[*index] = moniker;
        (*index)++;
    }
}

static HRESULT composite_get_components_alloc(IMoniker *iface, unsigned int *count, IMoniker ***components)
{
    CompositeMonikerImpl *moniker;
    unsigned int index;

    if ((moniker = unsafe_impl_from_IMoniker(iface)))
        *count = moniker->comp_count;
    else
        *count = 1;

    if (!(*components = malloc(*count * sizeof(**components))))
        return E_OUTOFMEMORY;

    index = 0;
    composite_get_components(iface, *components, &index);

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerImpl_Enum(IMoniker *iface, BOOL forward, IEnumMoniker **ret_enum)
{
    IMoniker **monikers;
    unsigned int count;
    HRESULT hr;

    TRACE("%p, %d, %p\n", iface, forward, ret_enum);

    if (!ret_enum)
        return E_INVALIDARG;

    if (FAILED(hr = composite_get_components_alloc(iface, &count, &monikers)))
        return hr;

    hr = create_enumerator(monikers, count, forward, ret_enum);
    free(monikers);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_IsEqual(IMoniker *iface, IMoniker *other)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface), *other_moniker;
    IMoniker **components, **other_components;
    unsigned int i, count;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, other);

    if (!other)
        return E_INVALIDARG;

    if (!(other_moniker = unsafe_impl_from_IMoniker(other)))
        return S_FALSE;

    if (moniker->comp_count != other_moniker->comp_count)
        return S_FALSE;

    if (FAILED(hr = composite_get_components_alloc(iface, &count, &components))) return hr;
    if (FAILED(hr = composite_get_components_alloc(other, &count, &other_components)))
    {
        free(components);
        return hr;
    }

    for (i = 0; i < moniker->comp_count; ++i)
    {
        if ((hr = IMoniker_IsEqual(components[i], other_components[i]) != S_OK))
            break;
    }

    free(other_components);
    free(components);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_Hash(IMoniker *iface, DWORD *hash)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    DWORD left_hash, right_hash;
    HRESULT hr;

    TRACE("%p, %p\n", iface, hash);

    if (!hash)
        return E_POINTER;

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    *hash = 0;

    if (FAILED(hr = IMoniker_Hash(moniker->left, &left_hash))) return hr;
    if (FAILED(hr = IMoniker_Hash(moniker->right, &right_hash))) return hr;

    *hash = left_hash ^ right_hash;

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_IsRunning(IMoniker *iface, IBindCtx *pbc,
        IMoniker *toleft, IMoniker *newly_running)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *c, *left, *rightmost;
    IRunningObjectTable *rot;
    HRESULT hr;

    TRACE("%p, %p, %p, %p.\n", iface, pbc, toleft, newly_running);

    if (!pbc)
        return E_INVALIDARG;

    if (toleft)
    {
        if (SUCCEEDED(hr = CreateGenericComposite(toleft, iface, &c)))
        {
            hr = IMoniker_IsRunning(c, pbc, NULL, newly_running);
            IMoniker_Release(c);
        }

        return hr;
    }

    if (newly_running)
        return IMoniker_IsEqual(iface, newly_running);

    if (FAILED(hr = IBindCtx_GetRunningObjectTable(pbc, &rot)))
        return hr;

    hr = IRunningObjectTable_IsRunning(rot, iface);
    IRunningObjectTable_Release(rot);
    if (hr == S_OK) return S_OK;

    if (FAILED(hr = composite_get_rightmost(moniker, &left, &rightmost)))
        return hr;

    hr = IMoniker_IsRunning(rightmost, pbc, left, NULL);

    IMoniker_Release(left);
    IMoniker_Release(rightmost);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_GetTimeOfLastChange(IMoniker *iface, IBindCtx *pbc,
        IMoniker *toleft, FILETIME *changetime)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *left, *rightmost, *composed_left = NULL, *running = NULL;
    IRunningObjectTable *rot;
    HRESULT hr;

    TRACE("%p, %p, %p, %p.\n", iface, pbc, toleft, changetime);

    if (!changetime || !pbc)
        return E_INVALIDARG;

    if (FAILED(hr = composite_get_rightmost(moniker, &left, &rightmost)))
        return hr;

    if (toleft)
    {
        /* Compose (toleft, left) and check that against rightmost */
        if (SUCCEEDED(hr = compose_with(toleft, left, &composed_left)) && composed_left)
            hr = compose_with(composed_left, rightmost, &running);
    }
    else
    {
        composed_left = left;
        IMoniker_AddRef(composed_left);
        running = iface;
        IMoniker_AddRef(running);
    }

    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = IBindCtx_GetRunningObjectTable(pbc, &rot)))
        {
            if (IRunningObjectTable_GetTimeOfLastChange(rot, running, changetime) != S_OK)
                hr = IMoniker_GetTimeOfLastChange(rightmost, pbc, composed_left, changetime);
            IRunningObjectTable_Release(rot);
        }
    }

    if (composed_left)
        IMoniker_Release(composed_left);
    if (running)
        IMoniker_Release(running);
    IMoniker_Release(rightmost);
    IMoniker_Release(left);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_Inverse(IMoniker *iface, IMoniker **inverse)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *right_inverted, *left_inverted;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, inverse);

    if (!inverse)
        return E_INVALIDARG;

    *inverse = NULL;

    if (FAILED(hr = IMoniker_Inverse(moniker->right, &right_inverted))) return hr;
    if (FAILED(hr = IMoniker_Inverse(moniker->left, &left_inverted)))
    {
        IMoniker_Release(right_inverted);
        return hr;
    }

    hr = CreateGenericComposite(right_inverted, left_inverted, inverse);

    IMoniker_Release(left_inverted);
    IMoniker_Release(right_inverted);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_CommonPrefixWith(IMoniker *iface, IMoniker *other,
        IMoniker **prefix)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface), *other_moniker;
    unsigned int i, count, prefix_len = 0;
    IMoniker *leftmost;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, other, prefix);

    /* If the other moniker is a composite, this method compares the components of each composite from left  */
    /* to right. The returned common prefix moniker might also be a composite moniker, depending on how many */
    /* of the leftmost components were common to both monikers.                                              */

    if (prefix)
        *prefix = NULL;

    if (!other || !prefix)
        return E_INVALIDARG;

    if ((other_moniker = unsafe_impl_from_IMoniker(other)))
    {
        IMoniker **components, **other_components, **prefix_components;
        IMoniker *last, *c;

        if (FAILED(hr = composite_get_components_alloc(iface, &count, &components))) return hr;
        if (FAILED(hr = composite_get_components_alloc(other, &count, &other_components)))
        {
            free(components);
            return hr;
        }

        count = min(moniker->comp_count, other_moniker->comp_count);
        if (!(prefix_components = calloc(count, sizeof(*prefix_components))))
        {
            free(components);
            free(other_components);
            return E_OUTOFMEMORY;
        }

        /* Collect prefix components */
        for (i = 0; i < count; ++i)
        {
            IMoniker *p;

            if (FAILED(hr = IMoniker_CommonPrefixWith(components[i], other_components[i], &p)))
                break;
            prefix_components[prefix_len++] = p;
            /* S_OK means that prefix was found and is neither of tested monikers */
            if (hr == S_OK) break;
        }

        free(components);
        free(other_components);

        if (!prefix_len)
        {
            free(prefix_components);
            return MK_E_NOPREFIX;
        }

        last = prefix_components[0];
        for (i = 1; i < prefix_len; ++i)
        {
            hr = CreateGenericComposite(last, prefix_components[i], &c);
            IMoniker_Release(last);
            IMoniker_Release(prefix_components[i]);
            if (FAILED(hr)) break;
            last = c;
        }
        free(prefix_components);

        if (SUCCEEDED(hr))
        {
            *prefix = last;
            if (IMoniker_IsEqual(iface, *prefix) == S_OK)
                hr = MK_S_US;
            else if (prefix_len < count)
                hr = S_OK;
            else
                hr = prefix_len == moniker->comp_count ? MK_S_ME : MK_S_HIM;
        }

        return hr;
    }

    /* For non-composite, compare to leftmost component */
    if (SUCCEEDED(hr = composite_get_leftmost(moniker, &leftmost)))
    {
        if ((hr = IMoniker_IsEqual(leftmost, other)) == S_OK)
        {
            *prefix = leftmost;
            IMoniker_AddRef(*prefix);
        }

        hr = hr == S_OK ? MK_S_HIM : MK_E_NOPREFIX;
        IMoniker_Release(leftmost);
    }

    return hr;
}

static HRESULT composite_compose_components(IMoniker **comp, unsigned int count, IMoniker **ret)
{
    IMoniker *last, *c;
    HRESULT hr = S_OK;
    unsigned int i;

    last = comp[0];
    IMoniker_AddRef(last);

    for (i = 1; i < count; ++i)
    {
        hr = CreateGenericComposite(last, comp[i], &c);
        IMoniker_Release(last);
        if (FAILED(hr)) break;
        last = c;
    }

    *ret = SUCCEEDED(hr) ? last : NULL;

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_RelativePathTo(IMoniker *iface, IMoniker *other,
        IMoniker **relpath)
{
    unsigned int count, this_count, other_count, prefix_len = 0;
    IMoniker *inv, *tail = NULL, *other_tail = NULL, *rel = NULL;
    IMoniker **components, **other_components;
    unsigned int start = 0, other_start = 0;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, other, relpath);

    if (!relpath)
        return E_INVALIDARG;

    *relpath = NULL;

    if (FAILED(hr = composite_get_components_alloc(iface, &this_count, &components))) return hr;
    if (FAILED(hr = composite_get_components_alloc(other, &other_count, &other_components)))
    {
        free(components);
        return hr;
    }

    /* Skip common prefix of equal components */
    count = min(other_count, this_count);
    while (IMoniker_IsEqual(components[prefix_len], other_components[prefix_len]) == S_OK)
    {
        if (++prefix_len == count) break;
    }

    if (prefix_len)
    {
        this_count -= prefix_len;
        other_count -= prefix_len;
        other_start += prefix_len;
        start += prefix_len;
    }
    else
    {
        /* Replace first component of the other tail with relative path */
        if (SUCCEEDED(hr = IMoniker_RelativePathTo(*components, *other_components, &rel)))
            *other_components = rel;

        this_count--;
        start++;
    }

    /* Invert left side tail */
    if (this_count && SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = composite_compose_components(&components[start], this_count, &tail)))
        {
            hr = IMoniker_Inverse(tail, &inv);
            IMoniker_Release(tail);
            tail = inv;
        }
    }

    if (other_count && SUCCEEDED(hr))
        hr = composite_compose_components(&other_components[other_start], other_count, &other_tail);

    if (tail || other_tail)
        hr = CreateGenericComposite(tail, other_tail, relpath);
    else if (SUCCEEDED(hr))
    {
        *relpath = other;
        IMoniker_AddRef(*relpath);
        hr = MK_S_HIM;
    }

    if (rel)
        IMoniker_Release(rel);
    if (tail)
        IMoniker_Release(tail);
    if (other_tail)
        IMoniker_Release(other_tail);

    free(other_components);
    free(components);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_GetDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR *displayname)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    WCHAR *left_name = NULL, *right_name = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", iface, pbc, pmkToLeft, displayname);

    if (!pbc || !displayname || !moniker->comp_count)
        return E_INVALIDARG;

    if (FAILED(hr = IMoniker_GetDisplayName(moniker->left, pbc, NULL, &left_name))) return hr;
    if (FAILED(hr = IMoniker_GetDisplayName(moniker->right, pbc, NULL, &right_name)))
    {
        CoTaskMemFree(left_name);
        return hr;
    }

    if (!(*displayname = CoTaskMemAlloc((lstrlenW(left_name) + lstrlenW(right_name) + 1) * sizeof(WCHAR))))
    {
        CoTaskMemFree(left_name);
        CoTaskMemFree(right_name);
        return E_OUTOFMEMORY;
    }

    lstrcpyW(*displayname, left_name);
    lstrcatW(*displayname, right_name);

    CoTaskMemFree(left_name);
    CoTaskMemFree(right_name);

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerImpl_ParseDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR name, ULONG *eaten, IMoniker **result)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *left, *rightmost;
    HRESULT hr;

    TRACE("%p, %p, %p, %s, %p, %p.\n", iface, pbc, pmkToLeft, debugstr_w(name), eaten, result);

    if (!pbc)
        return E_INVALIDARG;

    if (FAILED(hr = composite_get_rightmost(moniker, &left, &rightmost)))
        return hr;

    /* Let rightmost component parse the name, using what's left of the composite as a left side. */
    hr = IMoniker_ParseDisplayName(rightmost, pbc, left, name, eaten, result);

    IMoniker_Release(left);
    IMoniker_Release(rightmost);

    return hr;
}

/******************************************************************************
 *        CompositeMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_GENERICCOMPOSITE;

    return S_OK;
}

/*******************************************************************************
 *        CompositeMonikerIROTData_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
CompositeMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,
               VOID** ppvObject)
{
    CompositeMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppvObject);

    return CompositeMonikerImpl_QueryInterface(&This->IMoniker_iface, riid, ppvObject);
}

/***********************************************************************
 *        CompositeMonikerIROTData_AddRef
 */
static ULONG WINAPI
CompositeMonikerROTDataImpl_AddRef(IROTData *iface)
{
    CompositeMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_AddRef(&This->IMoniker_iface);
}

/***********************************************************************
 *        CompositeMonikerIROTData_Release
 */
static ULONG WINAPI CompositeMonikerROTDataImpl_Release(IROTData* iface)
{
    CompositeMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_Release(&This->IMoniker_iface);
}

static HRESULT composite_get_moniker_comparison_data(IMoniker *moniker,
        BYTE *data, ULONG max_len, ULONG *ret_len)
{
    IROTData *rot_data;
    HRESULT hr;

    if (FAILED(hr = IMoniker_QueryInterface(moniker, &IID_IROTData, (void **)&rot_data)))
    {
        WARN("Failed to get IROTData for component moniker, hr %#lx.\n", hr);
        return hr;
    }

    hr = IROTData_GetComparisonData(rot_data, data, max_len, ret_len);
    IROTData_Release(rot_data);

    return hr;
}

static HRESULT WINAPI CompositeMonikerROTDataImpl_GetComparisonData(IROTData *iface,
        BYTE *data, ULONG max_len, ULONG *ret_len)
{
    CompositeMonikerImpl *moniker = impl_from_IROTData(iface);
    HRESULT hr;
    ULONG len;

    TRACE("%p, %p, %lu, %p\n", iface, data, max_len, ret_len);

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    /* Get required size first */
    *ret_len = sizeof(CLSID);

    len = 0;
    hr = composite_get_moniker_comparison_data(moniker->left, NULL, 0, &len);
    if (SUCCEEDED(hr) || hr == E_OUTOFMEMORY)
        *ret_len += len;
    else
    {
        WARN("Failed to get comparison data length for left component, hr %#lx.\n", hr);
        return hr;
    }

    len = 0;
    hr = composite_get_moniker_comparison_data(moniker->right, NULL, 0, &len);
    if (SUCCEEDED(hr) || hr == E_OUTOFMEMORY)
        *ret_len += len;
    else
    {
        WARN("Failed to get comparison data length for right component, hr %#lx.\n", hr);
        return hr;
    }

    if (max_len < *ret_len)
        return E_OUTOFMEMORY;

    memcpy(data, &CLSID_CompositeMoniker, sizeof(CLSID));
    data += sizeof(CLSID);
    max_len -= sizeof(CLSID);
    if (FAILED(hr = composite_get_moniker_comparison_data(moniker->left, data, max_len, &len)))
    {
        WARN("Failed to get comparison data for left component, hr %#lx.\n", hr);
        return hr;
    }
    data += len;
    max_len -= len;
    if (FAILED(hr = composite_get_moniker_comparison_data(moniker->right, data, max_len, &len)))
    {
        WARN("Failed to get comparison data for right component, hr %#lx.\n", hr);
        return hr;
    }

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_QueryInterface(IMarshal *iface, REFIID riid, LPVOID *ppv)
{
    CompositeMonikerImpl *This = impl_from_IMarshal(iface);

    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppv);

    return CompositeMonikerImpl_QueryInterface(&This->IMoniker_iface, riid, ppv);
}

static ULONG WINAPI CompositeMonikerMarshalImpl_AddRef(IMarshal *iface)
{
    CompositeMonikerImpl *This = impl_from_IMarshal(iface);

    TRACE("(%p)\n",iface);

    return CompositeMonikerImpl_AddRef(&This->IMoniker_iface);
}

static ULONG WINAPI CompositeMonikerMarshalImpl_Release(IMarshal *iface)
{
    CompositeMonikerImpl *This = impl_from_IMarshal(iface);

    TRACE("(%p)\n",iface);

    return CompositeMonikerImpl_Release(&This->IMoniker_iface);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_GetUnmarshalClass(
  IMarshal *iface, REFIID riid, void *pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, CLSID* pCid)
{
    CompositeMonikerImpl *This = impl_from_IMarshal(iface);

    TRACE("%s, %p, %lx, %p, %lx, %p.\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pCid);

    return IMoniker_GetClassID(&This->IMoniker_iface, pCid);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_GetMarshalSizeMax(
  IMarshal *iface, REFIID riid, void *pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, DWORD* pSize)
{
    CompositeMonikerImpl *moniker = impl_from_IMarshal(iface);
    HRESULT hr;
    ULONG size;

    TRACE("%s, %p, %lx, %p, %lx, %p.\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pSize);

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    *pSize = 0x10; /* to match native */

    if (FAILED(hr = CoGetMarshalSizeMax(&size, &IID_IMoniker, (IUnknown *)moniker->left, dwDestContext,
            pvDestContext, mshlflags)))
    {
        return hr;
    }
    *pSize += size;

    if (FAILED(hr = CoGetMarshalSizeMax(&size, &IID_IMoniker, (IUnknown *)moniker->right, dwDestContext,
            pvDestContext, mshlflags)))
    {
        return hr;
    }
    *pSize += size;

    return hr;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_MarshalInterface(IMarshal *iface, IStream *stream,
        REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext, DWORD flags)
{
    CompositeMonikerImpl *moniker = impl_from_IMarshal(iface);
    HRESULT hr;

    TRACE("%p, %p, %s, %p, %lx, %p, %#lx\n", iface, stream, debugstr_guid(riid), pv, dwDestContext, pvDestContext, flags);

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    if (FAILED(hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker->left, dwDestContext, pvDestContext, flags)))
    {
        WARN("Failed to marshal left component, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker->right, dwDestContext, pvDestContext, flags)))
        WARN("Failed to marshal right component, hr %#lx.\n", hr);

    return hr;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_UnmarshalInterface(IMarshal *iface, IStream *stream,
        REFIID riid, void **ppv)
{
    CompositeMonikerImpl *moniker = impl_from_IMarshal(iface);
    HRESULT hr;

    TRACE("%p, %p, %s, %p\n", iface, stream, debugstr_guid(riid), ppv);

    if (moniker->left)
    {
        IMoniker_Release(moniker->left);
        moniker->left = NULL;
    }

    if (moniker->right)
    {
        IMoniker_Release(moniker->right);
        moniker->right = NULL;
    }

    if (FAILED(hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&moniker->left)))
    {
        WARN("Failed to unmarshal left moniker, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&moniker->right)))
    {
        WARN("Failed to unmarshal right moniker, hr %#lx.\n", hr);
        return hr;
    }

    return IMoniker_QueryInterface(&moniker->IMoniker_iface, riid, ppv);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_ReleaseMarshalData(IMarshal *iface, IStream *pStm)
{
    TRACE("(%p)\n", pStm);
    /* can't release a state-based marshal as nothing on server side to
     * release */
    return S_OK;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_DisconnectObject(IMarshal *iface,
    DWORD dwReserved)
{
    TRACE("%#lx\n", dwReserved);
    /* can't disconnect a state-based marshal as nothing on server side to
     * disconnect from */
    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_QueryInterface
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_QueryInterface(IEnumMoniker* iface,REFIID riid,void** ppvObject)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( ppvObject==0 )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IEnumMoniker, riid))
        *ppvObject = iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    IEnumMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_AddRef
 ******************************************************************************/
static ULONG WINAPI
EnumMonikerImpl_AddRef(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);

}

static ULONG WINAPI EnumMonikerImpl_Release(IEnumMoniker *iface)
{
    EnumMonikerImpl *e = impl_from_IEnumMoniker(iface);
    ULONG refcount = InterlockedDecrement(&e->ref);
    unsigned int i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < e->count; ++i)
            IMoniker_Release(e->monikers[i]);
        free(e->monikers);
        free(e);
    }

    return refcount;
}

static HRESULT WINAPI EnumMonikerImpl_Next(IEnumMoniker *iface, ULONG count,
        IMoniker **m, ULONG *fetched)
{
    EnumMonikerImpl *e = impl_from_IEnumMoniker(iface);
    unsigned int i;

    TRACE("%p, %lu, %p, %p.\n", iface, count, m, fetched);

    if (!m)
        return E_INVALIDARG;

    *m = NULL;

    /* retrieve the requested number of moniker from the current position */
    for (i = 0; (e->pos < e->count) && (i < count); ++i)
    {
        m[i] = e->monikers[e->pos++];
        IMoniker_AddRef(m[i]);
    }

    if (fetched)
        *fetched = i;

    return i == count ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumMonikerImpl_Skip(IEnumMoniker *iface, ULONG count)
{
    EnumMonikerImpl *e = impl_from_IEnumMoniker(iface);

    TRACE("%p, %lu.\n", iface, count);

    if (!count)
        return S_OK;

    if ((e->pos + count) >= e->count)
        return S_FALSE;

    e->pos += count;

    return S_OK;
}

static HRESULT WINAPI EnumMonikerImpl_Reset(IEnumMoniker* iface)
{
    EnumMonikerImpl *e = impl_from_IEnumMoniker(iface);

    TRACE("%p.\n", iface);

    e->pos = 0;

    return S_OK;
}

static HRESULT WINAPI EnumMonikerImpl_Clone(IEnumMoniker *iface, IEnumMoniker **ret)
{
    TRACE("%p, %p.\n", iface, ret);

    if (!ret)
        return E_INVALIDARG;

    *ret = NULL;

    return E_NOTIMPL;
}

static const IEnumMonikerVtbl VT_EnumMonikerImpl =
{
    EnumMonikerImpl_QueryInterface,
    EnumMonikerImpl_AddRef,
    EnumMonikerImpl_Release,
    EnumMonikerImpl_Next,
    EnumMonikerImpl_Skip,
    EnumMonikerImpl_Reset,
    EnumMonikerImpl_Clone
};

static HRESULT create_enumerator(IMoniker **components, unsigned int count, BOOL forward, IEnumMoniker **ret)
{
    EnumMonikerImpl *object;
    unsigned int i;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumMoniker_iface.lpVtbl = &VT_EnumMonikerImpl;
    object->ref = 1;
    object->count = count;

    if (!(object->monikers = calloc(count, sizeof(*object->monikers))))
    {
        free(object);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        object->monikers[i] = forward ? components[i] : components[count - i - 1];
        IMoniker_AddRef(object->monikers[i]);
    }

    *ret = &object->IEnumMoniker_iface;

    return S_OK;
}

static const IMonikerVtbl VT_CompositeMonikerImpl =
{
    CompositeMonikerImpl_QueryInterface,
    CompositeMonikerImpl_AddRef,
    CompositeMonikerImpl_Release,
    CompositeMonikerImpl_GetClassID,
    CompositeMonikerImpl_IsDirty,
    CompositeMonikerImpl_Load,
    CompositeMonikerImpl_Save,
    CompositeMonikerImpl_GetSizeMax,
    CompositeMonikerImpl_BindToObject,
    CompositeMonikerImpl_BindToStorage,
    CompositeMonikerImpl_Reduce,
    CompositeMonikerImpl_ComposeWith,
    CompositeMonikerImpl_Enum,
    CompositeMonikerImpl_IsEqual,
    CompositeMonikerImpl_Hash,
    CompositeMonikerImpl_IsRunning,
    CompositeMonikerImpl_GetTimeOfLastChange,
    CompositeMonikerImpl_Inverse,
    CompositeMonikerImpl_CommonPrefixWith,
    CompositeMonikerImpl_RelativePathTo,
    CompositeMonikerImpl_GetDisplayName,
    CompositeMonikerImpl_ParseDisplayName,
    CompositeMonikerImpl_IsSystemMoniker
};

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static const IROTDataVtbl VT_ROTDataImpl =
{
    CompositeMonikerROTDataImpl_QueryInterface,
    CompositeMonikerROTDataImpl_AddRef,
    CompositeMonikerROTDataImpl_Release,
    CompositeMonikerROTDataImpl_GetComparisonData
};

static const IMarshalVtbl VT_MarshalImpl =
{
    CompositeMonikerMarshalImpl_QueryInterface,
    CompositeMonikerMarshalImpl_AddRef,
    CompositeMonikerMarshalImpl_Release,
    CompositeMonikerMarshalImpl_GetUnmarshalClass,
    CompositeMonikerMarshalImpl_GetMarshalSizeMax,
    CompositeMonikerMarshalImpl_MarshalInterface,
    CompositeMonikerMarshalImpl_UnmarshalInterface,
    CompositeMonikerMarshalImpl_ReleaseMarshalData,
    CompositeMonikerMarshalImpl_DisconnectObject
};

struct comp_node
{
    IMoniker *moniker;
    struct comp_node *parent;
    struct comp_node *left;
    struct comp_node *right;
};

static HRESULT moniker_get_tree_representation(IMoniker *moniker, struct comp_node *parent,
        struct comp_node **ret)
{
    CompositeMonikerImpl *comp_moniker;
    struct comp_node *node;

    if (!(node = calloc(1, sizeof(*node))))
        return E_OUTOFMEMORY;
    node->parent = parent;

    if ((comp_moniker = unsafe_impl_from_IMoniker(moniker)))
    {
        moniker_get_tree_representation(comp_moniker->left, node, &node->left);
        moniker_get_tree_representation(comp_moniker->right, node, &node->right);
    }
    else
    {
        node->moniker = moniker;
        IMoniker_AddRef(node->moniker);
    }

    *ret = node;

    return S_OK;
}

static struct comp_node *moniker_tree_get_rightmost(struct comp_node *root)
{
    if (!root->left && !root->right) return root->moniker ? root : NULL;
    while (root->right) root = root->right;
    return root;
}

static struct comp_node *moniker_tree_get_leftmost(struct comp_node *root)
{
    if (!root->left && !root->right) return root->moniker ? root : NULL;
    while (root->left) root = root->left;
    return root;
}

static void moniker_tree_node_release(struct comp_node *node)
{
    if (node->moniker)
        IMoniker_Release(node->moniker);
    free(node);
}

static void moniker_tree_release(struct comp_node *node)
{
    if (node->left)
        moniker_tree_node_release(node->left);
    if (node->right)
        moniker_tree_node_release(node->right);
    moniker_tree_node_release(node);
}

static void moniker_tree_replace_node(struct comp_node *node, struct comp_node *replace_with)
{
    if (node->parent)
    {
        if (node->parent->left == node) node->parent->left = replace_with;
        else node->parent->right = replace_with;
        replace_with->parent = node->parent;
    }
    else if (replace_with->moniker)
    {
        /* Replacing root with non-composite */
        node->moniker = replace_with->moniker;
        IMoniker_AddRef(node->moniker);
        node->left = node->right = NULL;
        moniker_tree_node_release(replace_with);
    }
    else
    {
        /* Attaching composite branches to the root */
        node->left = replace_with->left;
        node->right = replace_with->right;
        moniker_tree_node_release(replace_with);
    }
}

static void moniker_tree_discard(struct comp_node *node, BOOL left)
{
    if (node->parent)
    {
        moniker_tree_replace_node(node->parent, left ? node->parent->left : node->parent->right);
        moniker_tree_node_release(node);
    }
    else
    {
        IMoniker_Release(node->moniker);
        node->moniker = NULL;
    }
}

static HRESULT moniker_create_from_tree(const struct comp_node *root, unsigned int *count, IMoniker **moniker)
{
    IMoniker *left_moniker, *right_moniker;
    HRESULT hr;

    *moniker = NULL;

    /* Non-composite node */
    if (!root->left && !root->right)
    {
        (*count)++;
        *moniker = root->moniker;
        if (*moniker) IMoniker_AddRef(*moniker);
        return S_OK;
    }

    if (FAILED(hr = moniker_create_from_tree(root->left, count, &left_moniker))) return hr;
    if (FAILED(hr = moniker_create_from_tree(root->right, count, &right_moniker)))
    {
        IMoniker_Release(left_moniker);
        return hr;
    }

    hr = CreateGenericComposite(left_moniker, right_moniker, moniker);
    IMoniker_Release(left_moniker);
    IMoniker_Release(right_moniker);
    return hr;
}

static void moniker_get_tree_comp_count(const struct comp_node *root, unsigned int *count)
{
    if (!root->left && !root->right)
    {
        (*count)++;
        return;
    }

    moniker_get_tree_comp_count(root->left, count);
    moniker_get_tree_comp_count(root->right, count);
}

static HRESULT composite_get_rightmost(CompositeMonikerImpl *composite, IMoniker **left, IMoniker **rightmost)
{
    struct comp_node *root, *node;
    unsigned int count;
    HRESULT hr;

    /* Shortcut for trivial case when right component is non-composite */
    if (!unsafe_impl_from_IMoniker(composite->right))
    {
        *left = composite->left;
        IMoniker_AddRef(*left);
        *rightmost = composite->right;
        IMoniker_AddRef(*rightmost);
        return S_OK;
    }

    *left = *rightmost = NULL;

    if (FAILED(hr = moniker_get_tree_representation(&composite->IMoniker_iface, NULL, &root)))
        return hr;

    if (!(node = moniker_tree_get_rightmost(root)))
    {
        WARN("Couldn't get right most component.\n");
        moniker_tree_release(root);
        return E_FAIL;
    }

    *rightmost = node->moniker;
    IMoniker_AddRef(*rightmost);
    moniker_tree_discard(node, TRUE);

    hr = moniker_create_from_tree(root, &count, left);
    moniker_tree_release(root);
    if (FAILED(hr))
    {
        IMoniker_Release(*rightmost);
        *rightmost = NULL;
    }

    return hr;
}

static HRESULT composite_get_leftmost(CompositeMonikerImpl *composite, IMoniker **leftmost)
{
    struct comp_node *root, *node;
    HRESULT hr;

    if (!unsafe_impl_from_IMoniker(composite->left))
    {
        *leftmost = composite->left;
        IMoniker_AddRef(*leftmost);
        return S_OK;
    }

    if (FAILED(hr = moniker_get_tree_representation(&composite->IMoniker_iface, NULL, &root)))
        return hr;

    if (!(node = moniker_tree_get_leftmost(root)))
    {
        WARN("Couldn't get left most component.\n");
        moniker_tree_release(root);
        return E_FAIL;
    }

    *leftmost = node->moniker;
    IMoniker_AddRef(*leftmost);

    moniker_tree_release(root);

    return S_OK;
}

static HRESULT moniker_simplify_composition(IMoniker *left, IMoniker *right,
        unsigned int *count, IMoniker **new_left, IMoniker **new_right)
{
    struct comp_node *left_tree, *right_tree;
    unsigned int modified = 0;
    HRESULT hr = S_OK;
    IMoniker *c;

    *count = 0;

    moniker_get_tree_representation(left, NULL, &left_tree);
    moniker_get_tree_representation(right, NULL, &right_tree);

    /* Simplify by composing trees together, in a non-generic way. */
    for (;;)
    {
        struct comp_node *l, *r;

        if (!(l = moniker_tree_get_rightmost(left_tree))) break;
        if (!(r = moniker_tree_get_leftmost(right_tree))) break;

        c = NULL;
        if (FAILED(IMoniker_ComposeWith(l->moniker, r->moniker, TRUE, &c))) break;
        modified++;

        if (c)
        {
            /* Replace with composed moniker on the left side */
            IMoniker_Release(l->moniker);
            l->moniker = c;
        }
        else
            moniker_tree_discard(l, TRUE);
        moniker_tree_discard(r, FALSE);
    }

    if (!modified)
    {
        *new_left = left;
        IMoniker_AddRef(*new_left);
        *new_right = right;
        IMoniker_AddRef(*new_right);

        moniker_get_tree_comp_count(left_tree, count);
        moniker_get_tree_comp_count(right_tree, count);
    }
    else
    {
        hr = moniker_create_from_tree(left_tree, count, new_left);
        if (SUCCEEDED(hr))
            hr = moniker_create_from_tree(right_tree, count, new_right);
    }

    moniker_tree_release(left_tree);
    moniker_tree_release(right_tree);

    if (FAILED(hr))
    {
        if (*new_left) IMoniker_Release(*new_left);
        if (*new_right) IMoniker_Release(*new_right);
        *new_left = *new_right = NULL;
    }

    return hr;
}

static HRESULT create_composite(IMoniker *left, IMoniker *right, IMoniker **moniker)
{
    IMoniker *new_left, *new_right;
    CompositeMonikerImpl *object;
    HRESULT hr;

    *moniker = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMoniker_iface.lpVtbl = &VT_CompositeMonikerImpl;
    object->IROTData_iface.lpVtbl = &VT_ROTDataImpl;
    object->IMarshal_iface.lpVtbl = &VT_MarshalImpl;
    object->ref = 1;

    /* Uninitialized moniker created by object activation */
    if (!left && !right)
    {
        *moniker = &object->IMoniker_iface;
        return S_OK;
    }

    if (FAILED(hr = moniker_simplify_composition(left, right, &object->comp_count, &new_left, &new_right)))
    {
        IMoniker_Release(&object->IMoniker_iface);
        return hr;
    }

    if (!new_left || !new_right)
    {
        *moniker = new_left ? new_left : new_right;
        IMoniker_Release(&object->IMoniker_iface);
        return S_OK;
    }

    object->left = new_left;
    object->right = new_right;

    *moniker = &object->IMoniker_iface;

    return S_OK;
}

/******************************************************************************
 *        CreateGenericComposite	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI CreateGenericComposite(IMoniker *left, IMoniker *right, IMoniker **composite)
{
    TRACE("%p, %p, %p\n", left, right, composite);

    if (!composite)
        return E_POINTER;

    if (!left && right)
    {
        *composite = right;
        IMoniker_AddRef(*composite);
        return S_OK;
    }
    else if (left && !right)
    {
        *composite = left;
        IMoniker_AddRef(*composite);
        return S_OK;
    }
    else if (!left && !right)
        return S_OK;

    return create_composite(left, right, composite);
}

/******************************************************************************
 *        MonikerCommonPrefixWith	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI
MonikerCommonPrefixWith(IMoniker* pmkThis,IMoniker* pmkOther,IMoniker** ppmkCommon)
{
    FIXME("(),stub!\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CompositeMoniker_CreateInstance(IClassFactory *iface,
    IUnknown *pUnk, REFIID riid, void **ppv)
{
    IMoniker* pMoniker;
    HRESULT  hr;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = create_composite(NULL, NULL, &pMoniker);

    if (SUCCEEDED(hr))
    {
        hr = IMoniker_QueryInterface(pMoniker, riid, ppv);
        IMoniker_Release(pMoniker);
    }

    return hr;
}
