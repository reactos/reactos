/*
 * Uianimation main file.
 *
 * Copyright (C) 2018 Louis Lenders
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
#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "oaidl.h"
#include "ocidl.h"

#include "initguid.h"
#include "uianimation.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uianimation);

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *, REFIID, void **);
};

static inline struct class_factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct class_factory, IClassFactory_iface );
}

static HRESULT WINAPI class_factory_QueryInterface( IClassFactory *iface, REFIID iid, void **obj )
{
    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IClassFactory ))
    {
        IClassFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI class_factory_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance( IClassFactory *iface,
                                                    IUnknown *outer, REFIID iid,
                                                    void **obj )
{
    struct class_factory *This = impl_from_IClassFactory( iface );

    TRACE( "%p %s %p\n", outer, debugstr_guid( iid ), obj );

    *obj = NULL;
    return This->create_instance( outer, iid, obj );
}

static HRESULT WINAPI class_factory_LockServer( IClassFactory *iface,
                                                BOOL lock )
{
    FIXME( "%d: stub!\n", lock );
    return S_OK;
}

static const struct IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer
};

/***********************************************************************
 *          IUIAnimationStoryboard
 */
struct animation_storyboard
{
    IUIAnimationStoryboard IUIAnimationStoryboard_iface;
    LONG ref;
};

struct animation_storyboard *impl_from_IUIAnimationStoryboard( IUIAnimationStoryboard *iface )
{
    return CONTAINING_RECORD( iface, struct animation_storyboard, IUIAnimationStoryboard_iface );
}

static HRESULT WINAPI animation_storyboard_QueryInterface( IUIAnimationStoryboard *iface,
                                                 REFIID iid, void **obj )
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IUIAnimationStoryboard ))
    {
        IUIAnimationStoryboard_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI animation_storyboard_AddRef( IUIAnimationStoryboard *iface )
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %lu\n", This, ref );
    return ref;
}

static ULONG WINAPI animation_storyboard_Release( IUIAnimationStoryboard *iface )
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %lu\n", This, ref );

    if (!ref)
        free( This );

    return ref;
}

static HRESULT WINAPI animation_storyboard_AddTransition (IUIAnimationStoryboard *iface, IUIAnimationVariable *variable,
        IUIAnimationTransition *transition)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return S_OK;
}

static HRESULT WINAPI animation_storyboard_AddKeyframeAtOffset (IUIAnimationStoryboard *iface, UI_ANIMATION_KEYFRAME existingframe,
        UI_ANIMATION_SECONDS offset, UI_ANIMATION_KEYFRAME *keyframe)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_AddKeyframeAfterTransition (IUIAnimationStoryboard *iface,IUIAnimationTransition *transition,
        UI_ANIMATION_KEYFRAME *keyframe)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return S_OK;
}

static HRESULT WINAPI animation_storyboard_AddTransitionAtKeyframe (IUIAnimationStoryboard *iface, IUIAnimationVariable *variable,
        IUIAnimationTransition *transition, UI_ANIMATION_KEYFRAME start_key)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_AddTransitionBetweenKeyframes (IUIAnimationStoryboard *iface, IUIAnimationVariable *variable,
        IUIAnimationTransition *transition, UI_ANIMATION_KEYFRAME start_key, UI_ANIMATION_KEYFRAME end_key)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_RepeatBetweenKeyframes (IUIAnimationStoryboard *iface, UI_ANIMATION_KEYFRAME start_key,
        UI_ANIMATION_KEYFRAME end_key, INT32 count)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return S_OK;
}

static HRESULT WINAPI animation_storyboard_HoldVariable (IUIAnimationStoryboard *iface,  IUIAnimationVariable *variable)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_SetLongestAcceptableDelay (IUIAnimationStoryboard *iface,  UI_ANIMATION_SECONDS delay)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_Schedule (IUIAnimationStoryboard *iface, UI_ANIMATION_SECONDS now,
        UI_ANIMATION_SCHEDULING_RESULT *result)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return 0;
}

static HRESULT WINAPI animation_storyboard_Conclude (IUIAnimationStoryboard *iface)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_Finish (IUIAnimationStoryboard *iface,  UI_ANIMATION_SECONDS deadline)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_Abandon (IUIAnimationStoryboard *iface)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_SetTag(IUIAnimationStoryboard *iface, IUnknown *object, UINT32 id)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_GetTag (IUIAnimationStoryboard *iface, IUnknown **object, UINT32 *id)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_GetStatus (IUIAnimationStoryboard *iface, UI_ANIMATION_STORYBOARD_STATUS *status)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_GetElapsedTime (IUIAnimationStoryboard *iface,  UI_ANIMATION_SECONDS *elapsed)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_storyboard_SetStoryboardEventHandler (IUIAnimationStoryboard *iface, IUIAnimationStoryboardEventHandler *handler)
{
    struct animation_storyboard *This = impl_from_IUIAnimationStoryboard( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return S_OK;
}

const struct IUIAnimationStoryboardVtbl animation_storyboard_vtbl =
{
    animation_storyboard_QueryInterface,
    animation_storyboard_AddRef,
    animation_storyboard_Release,
    animation_storyboard_AddTransition,
    animation_storyboard_AddKeyframeAtOffset,
    animation_storyboard_AddKeyframeAfterTransition,
    animation_storyboard_AddTransitionAtKeyframe,
    animation_storyboard_AddTransitionBetweenKeyframes,
    animation_storyboard_RepeatBetweenKeyframes,
    animation_storyboard_HoldVariable,
    animation_storyboard_SetLongestAcceptableDelay,
    animation_storyboard_Schedule ,
    animation_storyboard_Conclude ,
    animation_storyboard_Finish ,
    animation_storyboard_Abandon,
    animation_storyboard_SetTag,
    animation_storyboard_GetTag ,
    animation_storyboard_GetStatus ,
    animation_storyboard_GetElapsedTime,
    animation_storyboard_SetStoryboardEventHandler
};

static HRESULT animation_storyboard_create( IUIAnimationStoryboard **obj )
{
    struct animation_storyboard *This = malloc( sizeof(*This) );

    if (!This) return E_OUTOFMEMORY;
    This->IUIAnimationStoryboard_iface.lpVtbl = &animation_storyboard_vtbl;
    This->ref = 1;

    *obj = &This->IUIAnimationStoryboard_iface;

    return S_OK;
}

/***********************************************************************
 *          IUIAnimationVariable
 */
struct animation_var
{
    IUIAnimationVariable IUIAnimationVariable_iface;
    LONG ref;
    DOUBLE initial;
};

struct animation_var *impl_from_IUIAnimationVariable( IUIAnimationVariable *iface )
{
    return CONTAINING_RECORD( iface, struct animation_var, IUIAnimationVariable_iface );
}

static HRESULT WINAPI animation_var_QueryInterface( IUIAnimationVariable *iface,
                                                 REFIID iid, void **obj )
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IUIAnimationVariable ))
    {
        IUIAnimationVariable_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI animation_var_AddRef( IUIAnimationVariable *iface )
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %lu\n", This, ref );
    return ref;
}

static ULONG WINAPI animation_var_Release( IUIAnimationVariable *iface )
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %lu\n", This, ref );

    if (!ref)
        free( This );

    return ref;
}

static HRESULT WINAPI animation_var_GetValue ( IUIAnimationVariable *iface, DOUBLE *value)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_GetFinalValue ( IUIAnimationVariable *iface, DOUBLE *value)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_GetPreviousValue ( IUIAnimationVariable *iface, DOUBLE *value)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_GetIntegerValue ( IUIAnimationVariable *iface, INT32 *value)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_GetFinalIntegerValue ( IUIAnimationVariable *iface, INT32 *value)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_GetPreviousIntegerValue ( IUIAnimationVariable *iface, INT32 *value)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_GetCurrentStoryboard ( IUIAnimationVariable *iface, IUIAnimationStoryboard **storyboard)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_SetLowerBound ( IUIAnimationVariable *iface, DOUBLE bound)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_SetUpperBound ( IUIAnimationVariable *iface, DOUBLE bound)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_SetRoundingMode ( IUIAnimationVariable *iface,UI_ANIMATION_ROUNDING_MODE mode)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return S_OK;
}

static HRESULT WINAPI animation_var_SetTag ( IUIAnimationVariable *iface, IUnknown *object, UINT32 id)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_GetTag ( IUIAnimationVariable *iface, IUnknown **object, UINT32 *id)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI animation_var_SetVariableChangeHandler ( IUIAnimationVariable *iface, IUIAnimationVariableChangeHandler *handler)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return S_OK;
}

static HRESULT WINAPI animation_var_SetVariableIntegerChangeHandler ( IUIAnimationVariable *iface,
        IUIAnimationVariableIntegerChangeHandler *handler)
{
    struct animation_var *This = impl_from_IUIAnimationVariable( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return S_OK;
}

const struct IUIAnimationVariableVtbl animation_var_vtbl =
{
    animation_var_QueryInterface,
    animation_var_AddRef,
    animation_var_Release,
    animation_var_GetValue,
    animation_var_GetFinalValue,
    animation_var_GetPreviousValue,
    animation_var_GetIntegerValue,
    animation_var_GetFinalIntegerValue,
    animation_var_GetPreviousIntegerValue,
    animation_var_GetCurrentStoryboard,
    animation_var_SetLowerBound,
    animation_var_SetUpperBound,
    animation_var_SetRoundingMode,
    animation_var_SetTag,
    animation_var_GetTag,
    animation_var_SetVariableChangeHandler,
    animation_var_SetVariableIntegerChangeHandler,
};

static HRESULT animation_var_create(DOUBLE initial, IUIAnimationVariable **obj )
{
    struct animation_var *This = malloc( sizeof(*This) );

    if (!This) return E_OUTOFMEMORY;
    This->IUIAnimationVariable_iface.lpVtbl = &animation_var_vtbl;
    This->ref = 1;
    This->initial = initial;

    *obj = &This->IUIAnimationVariable_iface;

    return S_OK;
}

/***********************************************************************
 *          IUIAnimationManager
 */
struct manager
{
    IUIAnimationManager IUIAnimationManager_iface;
    LONG ref;
};

struct manager *impl_from_IUIAnimationManager( IUIAnimationManager *iface )
{
    return CONTAINING_RECORD( iface, struct manager, IUIAnimationManager_iface );
}

static HRESULT WINAPI manager_QueryInterface( IUIAnimationManager *iface, REFIID iid, void **obj )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IUIAnimationManager ))
    {
        IUIAnimationManager_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI manager_AddRef( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %lu\n", This, ref );
    return ref;
}

static ULONG WINAPI manager_Release( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %lu\n", This, ref );

    if (!ref)
    {
        free( This );
    }

    return ref;
}

static HRESULT WINAPI manager_CreateAnimationVariable( IUIAnimationManager *iface, DOUBLE initial_value, IUIAnimationVariable **variable )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    TRACE( "(%p)->(%f, %p)\n", This, initial_value, variable );
    return animation_var_create(initial_value, variable);
}

static HRESULT WINAPI manager_ScheduleTransition( IUIAnimationManager *iface, IUIAnimationVariable *variable, IUIAnimationTransition *transition, UI_ANIMATION_SECONDS current_time )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p, %p)\n", This, variable, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_CreateStoryboard( IUIAnimationManager *iface, IUIAnimationStoryboard **storyboard )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    TRACE( "(%p)->(%p)\n", This, storyboard );
    return animation_storyboard_create(storyboard);
}

static HRESULT WINAPI manager_FinishAllStoryboards( IUIAnimationManager *iface, UI_ANIMATION_SECONDS max_time )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%f)\n", This, max_time );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_AbandonAllStoryboards( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_Update( IUIAnimationManager *iface, UI_ANIMATION_SECONDS cur_time, UI_ANIMATION_UPDATE_RESULT *update_result )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%f, %p)\n", This, cur_time, update_result );
    if (update_result)
        *update_result = UI_ANIMATION_UPDATE_VARIABLES_CHANGED;
    return S_OK;
}

static HRESULT WINAPI manager_GetVariableFromTag( IUIAnimationManager *iface, IUnknown *object, UINT32 id, IUIAnimationVariable **variable )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p, %p)\n", This, object, variable );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_GetStoryboardFromTag( IUIAnimationManager *iface, IUnknown *object, UINT32 id, IUIAnimationStoryboard **storyboard )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p, %d, %p)\n", This, object, id, storyboard );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_GetStatus( IUIAnimationManager *iface, UI_ANIMATION_MANAGER_STATUS *status )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, status );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetAnimationMode( IUIAnimationManager *iface, UI_ANIMATION_MODE mode )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%d)\n", This, mode );
    return S_OK;
}

static HRESULT WINAPI manager_Pause( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_Resume( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetManagerEventHandler( IUIAnimationManager *iface, IUIAnimationManagerEventHandler *handler )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, handler );
    return S_OK;
}

static HRESULT WINAPI manager_SetCancelPriorityComparison( IUIAnimationManager *iface, IUIAnimationPriorityComparison *comparison )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, comparison );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetTrimPriorityComparison( IUIAnimationManager *iface, IUIAnimationPriorityComparison *comparison )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, comparison );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetCompressPriorityComparison( IUIAnimationManager *iface, IUIAnimationPriorityComparison *comparison)
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, comparison );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetConcludePriorityComparison( IUIAnimationManager *iface, IUIAnimationPriorityComparison *comparison )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, comparison );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetDefaultLongestAcceptableDelay( IUIAnimationManager *iface, UI_ANIMATION_SECONDS delay )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%f)\n", This, delay );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_Shutdown( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

const struct IUIAnimationManagerVtbl manager_vtbl =
{
    manager_QueryInterface,
    manager_AddRef,
    manager_Release,
    manager_CreateAnimationVariable,
    manager_ScheduleTransition,
    manager_CreateStoryboard,
    manager_FinishAllStoryboards,
    manager_AbandonAllStoryboards,
    manager_Update,
    manager_GetVariableFromTag,
    manager_GetStoryboardFromTag,
    manager_GetStatus,
    manager_SetAnimationMode,
    manager_Pause,
    manager_Resume,
    manager_SetManagerEventHandler,
    manager_SetCancelPriorityComparison,
    manager_SetTrimPriorityComparison,
    manager_SetCompressPriorityComparison,
    manager_SetConcludePriorityComparison,
    manager_SetDefaultLongestAcceptableDelay,
    manager_Shutdown
};

static HRESULT manager_create( IUnknown *outer, REFIID iid, void **obj )
{
    struct manager *This = malloc( sizeof(*This) );
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->IUIAnimationManager_iface.lpVtbl = &manager_vtbl;
    This->ref = 1;

    hr = IUIAnimationManager_QueryInterface( &This->IUIAnimationManager_iface, iid, obj );

    IUIAnimationManager_Release( &This->IUIAnimationManager_iface );
    return hr;
}

/***********************************************************************
 *          IUIAnimationTimer
 */
struct timer
{
    IUIAnimationTimer IUIAnimationTimer_iface;
    LONG ref;
};

struct timer *impl_from_IUIAnimationTimer( IUIAnimationTimer *iface )
{
    return CONTAINING_RECORD( iface, struct timer, IUIAnimationTimer_iface );
}

static HRESULT WINAPI timer_QueryInterface( IUIAnimationTimer *iface, REFIID iid, void **obj )
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IUIAnimationTimer ))
    {
        IUIAnimationTimer_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI timer_AddRef( IUIAnimationTimer *iface )
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %lu\n", This, ref );
    return ref;
}

static ULONG WINAPI timer_Release( IUIAnimationTimer *iface )
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %lu\n", This, ref );

    if (!ref)
        free( This );

    return ref;
}

static HRESULT WINAPI timer_SetTimerUpdateHandler (IUIAnimationTimer *iface,
        IUIAnimationTimerUpdateHandler *update_handler,
        UI_ANIMATION_IDLE_BEHAVIOR idle_behaviour)
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    FIXME( "stub (%p)->(%p, %d)\n", This, update_handler, idle_behaviour );
    return E_NOTIMPL;
}

 static HRESULT WINAPI timer_SetTimerEventHandler (IUIAnimationTimer *iface,
        IUIAnimationTimerEventHandler *handler)
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    FIXME( "stub (%p)->()\n", This );
    return S_OK;
}

static HRESULT WINAPI timer_Enable (IUIAnimationTimer *iface)
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    FIXME( "stub (%p)->()\n", This );
    return S_OK;
}

static HRESULT WINAPI timer_Disable (IUIAnimationTimer *iface)
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI timer_IsEnabled (IUIAnimationTimer *iface)
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI timer_GetTime (IUIAnimationTimer *iface, UI_ANIMATION_SECONDS *seconds)
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    FIXME( "stub (%p)->(%p)\n", This, seconds );
    return S_OK;
}

static HRESULT WINAPI timer_SetFrameRateThreshold (IUIAnimationTimer *iface, UINT32 frames_per_sec)
{
    struct timer *This = impl_from_IUIAnimationTimer( iface );
    FIXME( "stub (%p)->(%d)\n", This, frames_per_sec );
    return E_NOTIMPL;
}

const struct IUIAnimationTimerVtbl timer_vtbl =
{
    timer_QueryInterface,
    timer_AddRef,
    timer_Release,
    timer_SetTimerUpdateHandler,
    timer_SetTimerEventHandler,
    timer_Enable,
    timer_Disable,
    timer_IsEnabled,
    timer_GetTime,
    timer_SetFrameRateThreshold,
};

static HRESULT timer_create( IUnknown *outer, REFIID iid, void **obj )
{
    struct timer *This = malloc( sizeof(*This) );
    HRESULT hr;

    if (!This)
        return E_OUTOFMEMORY;
    This->IUIAnimationTimer_iface.lpVtbl = &timer_vtbl;
    This->ref = 1;

    hr = IUIAnimationTimer_QueryInterface( &This->IUIAnimationTimer_iface, iid, obj );

    IUIAnimationTimer_Release( &This->IUIAnimationTimer_iface );
    return hr;
}

/***********************************************************************
 *          IUIAnimationTransitionFactory
 */
struct tr_factory
{
    IUIAnimationTransitionFactory IUIAnimationTransitionFactory_iface;
    LONG ref;
};

struct tr_factory *impl_from_IUIAnimationTransitionFactory( IUIAnimationTransitionFactory *iface )
{
    return CONTAINING_RECORD( iface, struct tr_factory, IUIAnimationTransitionFactory_iface );
}

static HRESULT WINAPI tr_factory_QueryInterface( IUIAnimationTransitionFactory *iface, REFIID iid, void **obj )
{
    struct tr_factory *This = impl_from_IUIAnimationTransitionFactory( iface );

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IUIAnimationTransitionFactory ))
    {
        IUIAnimationTransitionFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI tr_factory_AddRef( IUIAnimationTransitionFactory *iface )
{
    struct tr_factory *This = impl_from_IUIAnimationTransitionFactory( iface );
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %lu\n", This, ref );
    return ref;
}

static ULONG WINAPI tr_factory_Release( IUIAnimationTransitionFactory *iface )
{
    struct tr_factory *This = impl_from_IUIAnimationTransitionFactory( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %lu\n", This, ref );

    if (!ref)
        free( This );

    return ref;
}

static HRESULT WINAPI tr_factory_CreateTransition(IUIAnimationTransitionFactory *iface,
          IUIAnimationInterpolator *interpolator, IUIAnimationTransition **transition)
{
    struct tr_factory *This = impl_from_IUIAnimationTransitionFactory( iface );
    FIXME( "stub (%p)->(%p, %p)\n", This, interpolator, transition );
    return E_NOTIMPL;
}

const struct IUIAnimationTransitionFactoryVtbl tr_factory_vtbl =
{
    tr_factory_QueryInterface,
    tr_factory_AddRef,
    tr_factory_Release,
    tr_factory_CreateTransition
};

static HRESULT transition_create( IUnknown *outer, REFIID iid, void **obj )
{
    struct tr_factory *This = malloc( sizeof(*This) );
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->IUIAnimationTransitionFactory_iface.lpVtbl = &tr_factory_vtbl;
    This->ref = 1;

    hr = IUIAnimationTransitionFactory_QueryInterface( &This->IUIAnimationTransitionFactory_iface, iid, obj );

    IUIAnimationTransitionFactory_Release( &This->IUIAnimationTransitionFactory_iface );
    return hr;
}

/***********************************************************************
 *          IUITransitionLibrary
 */
struct tr_library
{
    IUIAnimationTransitionLibrary IUIAnimationTransitionLibrary_iface;
    LONG ref;
};

struct tr_library *impl_from_IUIAnimationTransitionLibrary( IUIAnimationTransitionLibrary *iface )
{
    return CONTAINING_RECORD( iface, struct tr_library, IUIAnimationTransitionLibrary_iface );
}

static HRESULT WINAPI tr_library_QueryInterface( IUIAnimationTransitionLibrary *iface,
                                                 REFIID iid, void **obj )
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IUIAnimationTransitionLibrary ))
    {
        IUIAnimationTransitionLibrary_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI tr_library_AddRef( IUIAnimationTransitionLibrary *iface )
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %lu\n", This, ref );
    return ref;
}

static ULONG WINAPI tr_library_Release( IUIAnimationTransitionLibrary *iface )
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %lu\n", This, ref );

    if (!ref)
        free( This );

    return ref;
}

static HRESULT WINAPI tr_library_CreateInstantaneousTransition(IUIAnimationTransitionLibrary *iface,
        double finalValue, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %p)\n", This, finalValue, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateConstantTransition(IUIAnimationTransitionLibrary *iface,
        double duration, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %p)\n", This, duration, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateDiscreteTransition(IUIAnimationTransitionLibrary *iface,
        double delay, double finalValue, double hold, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateLinearTransition(IUIAnimationTransitionLibrary *iface,
        double duration, double finalValue, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %f, %p)\n", This, duration, finalValue, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateLinearTransitionFromSpeed(IUIAnimationTransitionLibrary *iface,
        double speed, double finalValue, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %f, %p)\n", This, speed, finalValue, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateSinusoidalTransitionFromVelocity(IUIAnimationTransitionLibrary *iface,
        double duration, double period, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %f, %p)\n", This, duration, period, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateSinusoidalTransitionFromRange(IUIAnimationTransitionLibrary *iface,
        double duration, double minimumValue, double maximumValue, double period,
        UI_ANIMATION_SLOPE slope, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %f, %f, %f, %d, %p)\n", This, duration, minimumValue, maximumValue, period, slope, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateAccelerateDecelerateTransition(IUIAnimationTransitionLibrary *iface,
        double duration, double finalValue, double accelerationRatio, double decelerationRatio,
        IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %f, %f, %f, %p)\n", This, duration, finalValue, accelerationRatio, decelerationRatio, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateReversalTransition(IUIAnimationTransitionLibrary *iface, double duration,
        IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %p)\n", This, duration, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateCubicTransition(IUIAnimationTransitionLibrary *iface, double duration,
        double finalValue, double finalVelocity, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %f, %f, %p)\n", This, duration, finalValue, finalVelocity, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateSmoothStopTransition(IUIAnimationTransitionLibrary *iface,
        double maximumDuration, double finalValue, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(%f, %f, %p)\n", This, maximumDuration, finalValue, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI tr_library_CreateParabolicTransitionFromAcceleration(IUIAnimationTransitionLibrary *iface,
        double finalValue, double finalVelocity, double acceleration, IUIAnimationTransition **transition)
{
    struct tr_library *This = impl_from_IUIAnimationTransitionLibrary( iface );
    FIXME( "stub (%p)->(  )\n", This );
    return E_NOTIMPL;
}

const struct IUIAnimationTransitionLibraryVtbl tr_library_vtbl =
{
    tr_library_QueryInterface,
    tr_library_AddRef,
    tr_library_Release,
    tr_library_CreateInstantaneousTransition,
    tr_library_CreateConstantTransition,
    tr_library_CreateDiscreteTransition,
    tr_library_CreateLinearTransition,
    tr_library_CreateLinearTransitionFromSpeed,
    tr_library_CreateSinusoidalTransitionFromVelocity,
    tr_library_CreateSinusoidalTransitionFromRange,
    tr_library_CreateAccelerateDecelerateTransition,
    tr_library_CreateReversalTransition,
    tr_library_CreateCubicTransition,
    tr_library_CreateSmoothStopTransition,
    tr_library_CreateParabolicTransitionFromAcceleration,
};

static HRESULT library_create( IUnknown *outer, REFIID iid, void **obj )
{
    struct tr_library *This = malloc( sizeof(*This) );
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->IUIAnimationTransitionLibrary_iface.lpVtbl = &tr_library_vtbl;
    This->ref = 1;

    hr = IUIAnimationTransitionLibrary_QueryInterface( &This->IUIAnimationTransitionLibrary_iface, iid, obj );

    IUIAnimationTransitionLibrary_Release( &This->IUIAnimationTransitionLibrary_iface );
    return hr;
}

static struct class_factory manager_cf = { { &class_factory_vtbl }, manager_create };
static struct class_factory timer_cf   = { { &class_factory_vtbl }, timer_create };
static struct class_factory transition_cf = { { &class_factory_vtbl }, transition_create };
static struct class_factory library_cf = { { &class_factory_vtbl }, library_create };

/******************************************************************
 *             DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID iid, void **obj )
{
    IClassFactory *cf = NULL;

    TRACE( "(%s %s %p)\n", debugstr_guid( clsid ), debugstr_guid( iid ), obj );

    if (IsEqualCLSID( clsid, &CLSID_UIAnimationManager ))
        cf = &manager_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_UIAnimationTimer ))
        cf = &timer_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_UIAnimationTransitionFactory ))
        cf = &transition_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_UIAnimationTransitionLibrary ))
        cf = &library_cf.IClassFactory_iface;

    if (!cf)
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, iid, obj );
}
