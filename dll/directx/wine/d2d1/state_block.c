/*
 * Copyright 2015 Henri Verbeet for CodeWeavers
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

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static inline struct d2d_state_block *impl_from_ID2D1DrawingStateBlock1(ID2D1DrawingStateBlock1 *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_state_block, ID2D1DrawingStateBlock1_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_state_block_QueryInterface(ID2D1DrawingStateBlock1 *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1DrawingStateBlock1)
            || IsEqualGUID(iid, &IID_ID2D1DrawingStateBlock)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1DrawingStateBlock1_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_state_block_AddRef(ID2D1DrawingStateBlock1 *iface)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);
    ULONG refcount = InterlockedIncrement(&state_block->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_state_block_Release(ID2D1DrawingStateBlock1 *iface)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);
    ULONG refcount = InterlockedDecrement(&state_block->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (state_block->text_rendering_params)
            IDWriteRenderingParams_Release(state_block->text_rendering_params);
        ID2D1Factory_Release(state_block->factory);
        free(state_block);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_state_block_GetFactory(ID2D1DrawingStateBlock1 *iface, ID2D1Factory **factory)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = state_block->factory);
}

static void STDMETHODCALLTYPE d2d_state_block_GetDescription(ID2D1DrawingStateBlock1 *iface,
        D2D1_DRAWING_STATE_DESCRIPTION *desc)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &state_block->drawing_state, sizeof(*desc));
}

static void STDMETHODCALLTYPE d2d_state_block_SetDescription(ID2D1DrawingStateBlock1 *iface,
        const D2D1_DRAWING_STATE_DESCRIPTION *desc)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(&state_block->drawing_state, desc, sizeof(*desc));
}

static void STDMETHODCALLTYPE d2d_state_block_SetTextRenderingParams(ID2D1DrawingStateBlock1 *iface,
        IDWriteRenderingParams *text_rendering_params)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);

    TRACE("iface %p, text_rendering_params %p.\n", iface, text_rendering_params);

    if (text_rendering_params)
        IDWriteRenderingParams_AddRef(text_rendering_params);
    if (state_block->text_rendering_params)
        IDWriteRenderingParams_Release(state_block->text_rendering_params);
    state_block->text_rendering_params = text_rendering_params;
}

static void STDMETHODCALLTYPE d2d_state_block_GetTextRenderingParams(ID2D1DrawingStateBlock1 *iface,
        IDWriteRenderingParams **text_rendering_params)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);

    TRACE("iface %p, text_rendering_params %p.\n", iface, text_rendering_params);

    if ((*text_rendering_params = state_block->text_rendering_params))
        IDWriteRenderingParams_AddRef(*text_rendering_params);
}

static void STDMETHODCALLTYPE d2d_state_block_GetDescription1(ID2D1DrawingStateBlock1 *iface,
        D2D1_DRAWING_STATE_DESCRIPTION1 *desc)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state_block->drawing_state;
}

static void STDMETHODCALLTYPE d2d_state_block_SetDescription1(ID2D1DrawingStateBlock1 *iface,
        const D2D1_DRAWING_STATE_DESCRIPTION1 *desc)
{
    struct d2d_state_block *state_block = impl_from_ID2D1DrawingStateBlock1(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    state_block->drawing_state = *desc;
}

static const struct ID2D1DrawingStateBlock1Vtbl d2d_state_block_vtbl =
{
    d2d_state_block_QueryInterface,
    d2d_state_block_AddRef,
    d2d_state_block_Release,
    d2d_state_block_GetFactory,
    d2d_state_block_GetDescription,
    d2d_state_block_SetDescription,
    d2d_state_block_SetTextRenderingParams,
    d2d_state_block_GetTextRenderingParams,
    d2d_state_block_GetDescription1,
    d2d_state_block_SetDescription1,
};

void d2d_state_block_init(struct d2d_state_block *state_block, ID2D1Factory *factory,
        const D2D1_DRAWING_STATE_DESCRIPTION1 *desc, IDWriteRenderingParams *text_rendering_params)
{
    static const D2D1_MATRIX_3X2_F identity =
    {{{
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    }}};

    state_block->ID2D1DrawingStateBlock1_iface.lpVtbl = &d2d_state_block_vtbl;
    state_block->refcount = 1;
    ID2D1Factory_AddRef(state_block->factory = factory);
    if (desc)
        state_block->drawing_state = *desc;
    else
        state_block->drawing_state.transform = identity;
    if ((state_block->text_rendering_params = text_rendering_params))
        IDWriteRenderingParams_AddRef(state_block->text_rendering_params);
}

struct d2d_state_block *unsafe_impl_from_ID2D1DrawingStateBlock(ID2D1DrawingStateBlock *iface)
{
    if (!iface)
        return NULL;
    if (iface->lpVtbl != (ID2D1DrawingStateBlockVtbl *)&d2d_state_block_vtbl)
    {
        WARN("Unexpected state block vtbl %p.\n", iface->lpVtbl);
        return NULL;
    }
    return CONTAINING_RECORD(iface, struct d2d_state_block, ID2D1DrawingStateBlock1_iface);
}
