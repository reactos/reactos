/*
 * Animation Controller operations specific to D3DX9.
 *
 * Copyright (C) 2015 Christian Costa
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

#include "config.h"
#include "wine/port.h"

#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct d3dx9_animation_controller
{
    ID3DXAnimationController ID3DXAnimationController_iface;
    LONG ref;

    UINT max_outputs;
    UINT max_sets;
    UINT max_tracks;
    UINT max_events;
};

static inline struct d3dx9_animation_controller *impl_from_ID3DXAnimationController(ID3DXAnimationController *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_animation_controller, ID3DXAnimationController_iface);
}

static HRESULT WINAPI d3dx9_animation_controller_QueryInterface(ID3DXAnimationController *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXAnimationController))
    {
        iface->lpVtbl->AddRef(iface);
        *out = iface;
        return D3D_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_animation_controller_AddRef(ID3DXAnimationController *iface)
{
    struct d3dx9_animation_controller *animation = impl_from_ID3DXAnimationController(iface);
    ULONG refcount = InterlockedIncrement(&animation->ref);

    TRACE("%p increasing refcount to %u.\n", animation, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_animation_controller_Release(ID3DXAnimationController *iface)
{
    struct d3dx9_animation_controller *animation = impl_from_ID3DXAnimationController(iface);
    ULONG refcount = InterlockedDecrement(&animation->ref);

    TRACE("%p decreasing refcount to %u.\n", animation, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, animation);
    }

    return refcount;
}

static UINT WINAPI d3dx9_animation_controller_GetMaxNumAnimationOutputs(ID3DXAnimationController *iface)
{
    struct d3dx9_animation_controller *animation = impl_from_ID3DXAnimationController(iface);

    TRACE("iface %p.\n", iface);

    return animation->max_outputs;
}

static UINT WINAPI d3dx9_animation_controller_GetMaxNumAnimationSets(ID3DXAnimationController *iface)
{
    struct d3dx9_animation_controller *animation = impl_from_ID3DXAnimationController(iface);

    TRACE("iface %p.\n", iface);

    return animation->max_sets;
}

static UINT WINAPI d3dx9_animation_controller_GetMaxNumTracks(ID3DXAnimationController *iface)
{
    struct d3dx9_animation_controller *animation = impl_from_ID3DXAnimationController(iface);

    TRACE("iface %p.\n", iface);

    return animation->max_tracks;
}

static UINT WINAPI d3dx9_animation_controller_GetMaxNumEvents(ID3DXAnimationController *iface)
{
    struct d3dx9_animation_controller *animation = impl_from_ID3DXAnimationController(iface);

    TRACE("iface %p.\n", iface);

    return animation->max_events;
}

static HRESULT WINAPI d3dx9_animation_controller_RegisterAnimationOutput(ID3DXAnimationController *iface,
        const char *name, D3DXMATRIX *matrix, D3DXVECTOR3 *scale, D3DXQUATERNION *rotation, D3DXVECTOR3 *translation)
{
    FIXME("iface %p, name %s, matrix %p, scale %p, rotation %p, translation %p stub.\n", iface, debugstr_a(name),
            matrix, scale, rotation, translation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_RegisterAnimationSet(ID3DXAnimationController *iface,
        ID3DXAnimationSet *anim_set)
{
    FIXME("iface %p, anim_set %p stub.\n", iface, anim_set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_UnregisterAnimationSet(ID3DXAnimationController *iface,
        ID3DXAnimationSet *anim_set)
{
    FIXME("iface %p, anim_set %p stub.\n", iface, anim_set);

    return E_NOTIMPL;
}

static UINT WINAPI d3dx9_animation_controller_GetNumAnimationSets(ID3DXAnimationController *iface)
{
    FIXME("iface %p stub.\n", iface);

    return 0;
}

static HRESULT WINAPI d3dx9_animation_controller_GetAnimationSet(ID3DXAnimationController *iface,
        UINT index, ID3DXAnimationSet **anim_set)
{
    FIXME("iface %p, index %u, anim_set %p stub.\n", iface, index, anim_set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_GetAnimationSetByName(ID3DXAnimationController *iface,
        const char *name, ID3DXAnimationSet **anim_set)
{
    FIXME("iface %p, name %s, anim_set %p stub.\n", iface, debugstr_a(name), anim_set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_AdvanceTime(ID3DXAnimationController *iface, double time_delta,
        ID3DXAnimationCallbackHandler *callback_handler)
{
    FIXME("iface %p, time_delta %.16e, callback_handler %p stub.\n", iface, time_delta, callback_handler);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_Reset(ID3DXAnimationController *iface)
{
    FIXME("iface %p stub.\n", iface);

    return E_NOTIMPL;
}

static double WINAPI d3dx9_animation_controller_GetTime(ID3DXAnimationController *iface)
{
    FIXME("iface %p stub.\n", iface);

    return 0.0;
}

static HRESULT WINAPI d3dx9_animation_controller_SetTrackAnimationSet(ID3DXAnimationController *iface,
        UINT track, ID3DXAnimationSet *anim_set)
{
    FIXME("iface %p, track %u, anim_set %p stub.\n", iface, track, anim_set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_GetTrackAnimationSet(ID3DXAnimationController *iface,
        UINT track, ID3DXAnimationSet **anim_set)
{
    FIXME("iface %p, track %u, anim_set %p stub.\n", iface, track, anim_set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_SetTrackPriority(ID3DXAnimationController *iface,
        UINT track, D3DXPRIORITY_TYPE priority)
{
    FIXME("iface %p, track %u, priority %u stub.\n", iface, track, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_SetTrackSpeed(ID3DXAnimationController *iface,
        UINT track, float speed)
{
    FIXME("iface %p, track %u, speed %.8e stub.\n", iface, track, speed);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_SetTrackWeight(ID3DXAnimationController *iface,
        UINT track, float weight)
{
    FIXME("iface %p, track %u, weight %.8e stub.\n", iface, track, weight);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_SetTrackPosition(ID3DXAnimationController *iface,
        UINT track, double position)
{
    FIXME("iface %p, track %u, position %.16e stub.\n", iface, track, position);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_SetTrackEnable(ID3DXAnimationController *iface,
        UINT track, BOOL enable)
{
    FIXME("iface %p, track %u, enable %#x stub.\n", iface, track, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_SetTrackDesc(ID3DXAnimationController *iface,
        UINT track, D3DXTRACK_DESC *desc)
{
    FIXME("iface %p, track %u, desc %p stub.\n", iface, track, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_GetTrackDesc(ID3DXAnimationController *iface,
        UINT track, D3DXTRACK_DESC *desc)
{
    FIXME("iface %p, track %u, desc %p stub.\n", iface, track, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_SetPriorityBlend(ID3DXAnimationController *iface,
        float blend_weight)
{
    FIXME("iface %p, blend_weight %.8e stub.\n", iface, blend_weight);

    return E_NOTIMPL;
}

static float WINAPI d3dx9_animation_controller_GetPriorityBlend(ID3DXAnimationController *iface)
{
    FIXME("iface %p stub.\n", iface);

    return 0.0f;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_KeyTrackSpeed(ID3DXAnimationController *iface,
        UINT track, float new_speed, double start_time, double duration, D3DXTRANSITION_TYPE transition)
{
    FIXME("iface %p, track %u, new_speed %.8e, start_time %.16e, duration %.16e, transition %u stub.\n", iface,
            track, new_speed, start_time, duration, transition);

    return 0;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_KeyTrackWeight(ID3DXAnimationController *iface,
        UINT track, float new_weight, double start_time, double duration, D3DXTRANSITION_TYPE transition)
{
    FIXME("iface %p, track %u, new_weight %.8e, start_time %.16e, duration %.16e, transition %u stub.\n", iface,
            track, new_weight, start_time, duration, transition);

    return 0;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_KeyTrackPosition(ID3DXAnimationController *iface,
        UINT track, double new_position, double start_time)
{
    FIXME("iface %p, track %u, new_position %.16e, start_time %.16e stub.\n", iface,
            track, new_position, start_time);

    return 0;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_KeyTrackEnable(ID3DXAnimationController *iface,
        UINT track, BOOL new_enable, double start_time)
{
    FIXME("iface %p, track %u, new_enable %#x, start_time %.16e stub.\n", iface,
            track, new_enable, start_time);

    return 0;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_KeyTrackBlend(ID3DXAnimationController *iface,
        float new_blend_weight, double start_time, double duration, D3DXTRANSITION_TYPE transition)
{
    FIXME("iface %p, new_blend_weight %.8e, start_time %.16e, duration %.16e, transition %u stub.\n", iface,
            new_blend_weight, start_time, duration, transition);

    return 0;
}

static HRESULT WINAPI d3dx9_animation_controller_UnkeyEvent(ID3DXAnimationController *iface, D3DXEVENTHANDLE event)
{
    FIXME("iface %p, event %u stub.\n", iface, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_UnkeyAllTrackEvents(ID3DXAnimationController *iface, UINT track)
{
    FIXME("iface %p, track %u stub.\n", iface, track);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_UnkeyAllPriorityBlends(ID3DXAnimationController *iface)
{
    FIXME("iface %p stub.\n", iface);

    return E_NOTIMPL;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_GetCurrentTrackEvent(ID3DXAnimationController *iface,
        UINT track, D3DXEVENT_TYPE event_type)
{
    FIXME("iface %p, track %u, event_type %u stub.\n", iface, track, event_type);

    return 0;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_GetCurrentPriorityBlend(ID3DXAnimationController *iface)
{
    FIXME("iface %p stub.\n", iface);

    return 0;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_GetUpcomingTrackEvent(ID3DXAnimationController *iface,
        UINT track, D3DXEVENTHANDLE event)
{
    FIXME("iface %p, track %u, event %u stub.\n", iface, track, event);

    return 0;
}

static D3DXEVENTHANDLE WINAPI d3dx9_animation_controller_GetUpcomingPriorityBlend(ID3DXAnimationController *iface,
        D3DXEVENTHANDLE event)
{
    FIXME("iface %p, event %u stub.\n", iface, event);

    return 0;
}

static HRESULT WINAPI d3dx9_animation_controller_ValidateEvent(ID3DXAnimationController *iface, D3DXEVENTHANDLE event)
{
    FIXME("iface %p, event %u stub.\n", iface, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_GetEventDesc(ID3DXAnimationController *iface,
        D3DXEVENTHANDLE event, D3DXEVENT_DESC *desc)
{
    FIXME("iface %p, event %u, desc %p stub.\n", iface, event, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_animation_controller_CloneAnimationController(ID3DXAnimationController *iface, UINT max_outputs,
        UINT max_sets, UINT max_tracks, UINT max_events, ID3DXAnimationController **anim_controller)
{
    FIXME("iface %p, max_outputs %u, max_sets %u, max_tracks %u, max_events %u, anim_controller %p stub.\n",
            iface, max_outputs, max_sets, max_tracks, max_events, anim_controller);

    return E_NOTIMPL;
}

static const struct ID3DXAnimationControllerVtbl d3dx9_animation_controller_vtbl =
{
    d3dx9_animation_controller_QueryInterface,
    d3dx9_animation_controller_AddRef,
    d3dx9_animation_controller_Release,
    d3dx9_animation_controller_GetMaxNumAnimationOutputs,
    d3dx9_animation_controller_GetMaxNumAnimationSets,
    d3dx9_animation_controller_GetMaxNumTracks,
    d3dx9_animation_controller_GetMaxNumEvents,
    d3dx9_animation_controller_RegisterAnimationOutput,
    d3dx9_animation_controller_RegisterAnimationSet,
    d3dx9_animation_controller_UnregisterAnimationSet,
    d3dx9_animation_controller_GetNumAnimationSets,
    d3dx9_animation_controller_GetAnimationSet,
    d3dx9_animation_controller_GetAnimationSetByName,
    d3dx9_animation_controller_AdvanceTime,
    d3dx9_animation_controller_Reset,
    d3dx9_animation_controller_GetTime,
    d3dx9_animation_controller_SetTrackAnimationSet,
    d3dx9_animation_controller_GetTrackAnimationSet,
    d3dx9_animation_controller_SetTrackPriority,
    d3dx9_animation_controller_SetTrackSpeed,
    d3dx9_animation_controller_SetTrackWeight,
    d3dx9_animation_controller_SetTrackPosition,
    d3dx9_animation_controller_SetTrackEnable,
    d3dx9_animation_controller_SetTrackDesc,
    d3dx9_animation_controller_GetTrackDesc,
    d3dx9_animation_controller_SetPriorityBlend,
    d3dx9_animation_controller_GetPriorityBlend,
    d3dx9_animation_controller_KeyTrackSpeed,
    d3dx9_animation_controller_KeyTrackWeight,
    d3dx9_animation_controller_KeyTrackPosition,
    d3dx9_animation_controller_KeyTrackEnable,
    d3dx9_animation_controller_KeyTrackBlend,
    d3dx9_animation_controller_UnkeyEvent,
    d3dx9_animation_controller_UnkeyAllTrackEvents,
    d3dx9_animation_controller_UnkeyAllPriorityBlends,
    d3dx9_animation_controller_GetCurrentTrackEvent,
    d3dx9_animation_controller_GetCurrentPriorityBlend,
    d3dx9_animation_controller_GetUpcomingTrackEvent,
    d3dx9_animation_controller_GetUpcomingPriorityBlend,
    d3dx9_animation_controller_ValidateEvent,
    d3dx9_animation_controller_GetEventDesc,
    d3dx9_animation_controller_CloneAnimationController
};

HRESULT WINAPI D3DXCreateAnimationController(UINT max_outputs, UINT max_sets,
        UINT max_tracks, UINT max_events, ID3DXAnimationController **controller)
{
    struct d3dx9_animation_controller *object;

    TRACE("max_outputs %u, max_sets %u, max_tracks %u, max_events %u, controller %p.\n",
            max_outputs, max_sets, max_tracks, max_events, controller);

    if (!max_outputs || !max_sets || !max_tracks || !max_events || !controller)
        return D3D_OK;

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXAnimationController_iface.lpVtbl = &d3dx9_animation_controller_vtbl;
    object->ref = 1;
    object->max_outputs = max_outputs;
    object->max_sets    = max_sets;
    object->max_tracks  = max_tracks;
    object->max_events  = max_events;

    *controller = &object->ID3DXAnimationController_iface;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateKeyframedAnimationSet(const char *name, double ticks_per_second,
        D3DXPLAYBACK_TYPE playback_type, UINT animation_count, UINT callback_key_count,
        const D3DXKEY_CALLBACK *callback_keys, ID3DXKeyframedAnimationSet **animation_set)
{
    FIXME("name %s, ticks_per_second %.16e, playback_type %u, animation_count %u, "
            "callback_key_count %u, callback_keys %p, animation_set %p stub.\n",
            debugstr_a(name), ticks_per_second, playback_type, animation_count,
            callback_key_count, callback_keys, animation_set);

    return E_NOTIMPL;
}
