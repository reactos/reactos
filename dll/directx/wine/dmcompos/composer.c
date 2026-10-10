/* IDirectMusicComposer
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmcompos_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmcompos);

typedef struct IDirectMusicComposerImpl {
    IDirectMusicComposer IDirectMusicComposer_iface;
    LONG  ref;
} IDirectMusicComposerImpl;

static inline IDirectMusicComposerImpl *impl_from_IDirectMusicComposer(IDirectMusicComposer *iface)
{
  return CONTAINING_RECORD(iface, IDirectMusicComposerImpl, IDirectMusicComposer_iface);
}

static HRESULT WINAPI IDirectMusicComposerImpl_QueryInterface(IDirectMusicComposer *iface,
        REFIID riid, void **ret_iface)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicComposer))
{
        *ret_iface = iface;
        IDirectMusicComposer_AddRef(iface);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);
    *ret_iface = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicComposerImpl_AddRef(IDirectMusicComposer *iface)
{
    IDirectMusicComposerImpl *This = impl_from_IDirectMusicComposer(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicComposerImpl_Release(IDirectMusicComposer *iface)
{
    IDirectMusicComposerImpl *This = impl_from_IDirectMusicComposer(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) free(This);

    return ref;
}

/* IDirectMusicComposerImpl IDirectMusicComposer part: */
static HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromTemplate(IDirectMusicComposer *iface,
        IDirectMusicStyle *style, IDirectMusicSegment *template, WORD activity, IDirectMusicChordMap *chordmap,
        IDirectMusicSegment **segment)
{
    IDirectMusicComposerImpl *This = impl_from_IDirectMusicComposer(iface);
    IDirectMusicTrack *track;
    HRESULT hr;

    FIXME("(%p, %p, %p, %d, %p, %p): semi-stub\n", This, style, template, activity, chordmap, segment);

    if (!segment)
        return E_POINTER;
    if (!template)
        return E_INVALIDARG;

    if (!style) {
        hr = IDirectMusicSegment_GetTrack(template, &CLSID_DirectMusicStyleTrack, 0xFFFFFFFF,
                DMUS_SEG_ANYTRACK, &track);
        if (FAILED(hr))
            return E_INVALIDARG;
        else
            IDirectMusicTrack_Release(track);   /* Temp to not leak memory */
    }
    if (!chordmap) {
        hr = IDirectMusicSegment_GetTrack(template, &CLSID_DirectMusicChordMapTrack, 0xFFFFFFFF,
                DMUS_SEG_ANYTRACK, &track);
        if (FAILED(hr))
            return E_INVALIDARG;
        else
            IDirectMusicTrack_Release(track);   /* Temp to not leak memory */
    }

    return IDirectMusicSegment_Clone(template, -1, 0, segment);
}

static HRESULT WINAPI IDirectMusicComposerImpl_ComposeSegmentFromShape(IDirectMusicComposer *iface,
        IDirectMusicStyle *pStyle, WORD wNumMeasures, WORD wShape, WORD wActivity, BOOL fIntro,
        BOOL fEnd, IDirectMusicChordMap *pChordMap, IDirectMusicSegment **ppSegment)
{
        IDirectMusicComposerImpl *This = impl_from_IDirectMusicComposer(iface);
	FIXME("(%p, %p, %d, %d, %d, %d, %d, %p, %p): stub\n", This, pStyle, wNumMeasures, wShape, wActivity, fIntro, fEnd, pChordMap, ppSegment);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_ComposeTransition(IDirectMusicComposer *iface,
        IDirectMusicSegment *pFromSeg, IDirectMusicSegment *pToSeg, MUSIC_TIME mtTime,
        WORD wCommand, DWORD dwFlags, IDirectMusicChordMap *pChordMap,
        IDirectMusicSegment **ppTransSeg)
{
        IDirectMusicComposerImpl *This = impl_from_IDirectMusicComposer(iface);
	FIXME("(%p, %p, %p, %ld, %d, %ld, %p, %p): stub\n", This, pFromSeg, pToSeg, mtTime, wCommand, dwFlags, pChordMap, ppTransSeg);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_AutoTransition(IDirectMusicComposer *iface,
        IDirectMusicPerformance *pPerformance, IDirectMusicSegment *pToSeg, WORD wCommand,
        DWORD dwFlags, IDirectMusicChordMap *pChordMap, IDirectMusicSegment **ppTransSeg,
        IDirectMusicSegmentState **ppToSegState, IDirectMusicSegmentState **ppTransSegState)
{
        IDirectMusicComposerImpl *This = impl_from_IDirectMusicComposer(iface);
	FIXME("(%p, %p, %d, %ld, %p, %p, %p, %p): stub\n", This, pPerformance, wCommand, dwFlags, pChordMap, ppTransSeg, ppToSegState, ppTransSegState);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_ComposeTemplateFromShape(IDirectMusicComposer *iface,
        WORD wNumMeasures, WORD wShape, BOOL fIntro, BOOL fEnd, WORD wEndLength,
        IDirectMusicSegment **ppTemplate)
{
        IDirectMusicComposerImpl *This = impl_from_IDirectMusicComposer(iface);
	FIXME("(%p, %d, %d, %d, %d, %d, %p): stub\n", This, wNumMeasures, wShape, fIntro, fEnd, wEndLength, ppTemplate);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicComposerImpl_ChangeChordMap(IDirectMusicComposer *iface,
        IDirectMusicSegment *pSegment, BOOL fTrackScale, IDirectMusicChordMap *pChordMap)
{
        IDirectMusicComposerImpl *This = impl_from_IDirectMusicComposer(iface);
	FIXME("(%p, %p, %d, %p): stub\n", This, pSegment, fTrackScale, pChordMap);
	return S_OK;
}

static const IDirectMusicComposerVtbl dmcomposer_vtbl = {
	IDirectMusicComposerImpl_QueryInterface,
	IDirectMusicComposerImpl_AddRef,
	IDirectMusicComposerImpl_Release,
	IDirectMusicComposerImpl_ComposeSegmentFromTemplate,
	IDirectMusicComposerImpl_ComposeSegmentFromShape,
	IDirectMusicComposerImpl_ComposeTransition,
	IDirectMusicComposerImpl_AutoTransition,
	IDirectMusicComposerImpl_ComposeTemplateFromShape,
	IDirectMusicComposerImpl_ChangeChordMap
};

/* for ClassFactory */
HRESULT create_dmcomposer(REFIID riid, void **ret_iface)
{
    IDirectMusicComposerImpl *obj;
    HRESULT hr;

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicComposer_iface.lpVtbl = &dmcomposer_vtbl;
    obj->ref = 1;

    hr = IDirectMusicComposer_QueryInterface(&obj->IDirectMusicComposer_iface, riid, ret_iface);
    IDirectMusicComposer_Release(&obj->IDirectMusicComposer_iface);

    return hr;
}
