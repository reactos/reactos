/*
 * Copyright 2023 Connor McAdams for CodeWeavers
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

#include "uia_private.h"

#include "wine/debug.h"
#include "wine/rbtree.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

static SAFEARRAY *uia_desktop_node_rt_id;
static BOOL WINAPI uia_init_desktop_rt_id(INIT_ONCE *once, void *param, void **ctx)
{
    SAFEARRAY *sa;

    if ((sa = SafeArrayCreateVector(VT_I4, 0, 2)))
    {
        if (SUCCEEDED(write_runtime_id_base(sa, GetDesktopWindow())))
            uia_desktop_node_rt_id = sa;
        else
            SafeArrayDestroy(sa);
    }

    return !!uia_desktop_node_rt_id;
}

static SAFEARRAY *uia_get_desktop_rt_id(void)
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;

    if (!uia_desktop_node_rt_id)
        InitOnceExecuteOnce(&once, uia_init_desktop_rt_id, NULL, NULL);

    return uia_desktop_node_rt_id;
}

static int win_event_to_uia_event_id(int win_event)
{
    switch (win_event)
    {
    case EVENT_OBJECT_FOCUS: return UIA_AutomationFocusChangedEventId;
    case EVENT_SYSTEM_ALERT: return UIA_SystemAlertEventId;
    case EVENT_OBJECT_SHOW:  return UIA_StructureChangedEventId;
    case EVENT_OBJECT_DESTROY: return UIA_StructureChangedEventId;

    default:
        break;
    }

    return 0;
}

static BOOL CALLBACK uia_win_event_enum_top_level_hwnds(HWND hwnd, LPARAM lparam)
{
    struct rb_tree *hwnd_map = (struct rb_tree *)lparam;
    HRESULT hr;

    if (!uia_hwnd_is_visible(hwnd))
        return TRUE;

    hr = uia_hwnd_map_add_hwnd(hwnd_map, hwnd);
    if (FAILED(hr))
        WARN("Failed to add hwnd to map, hr %#lx\n", hr);

    return TRUE;
}

HRESULT uia_event_add_win_event_hwnd(struct uia_event *event, HWND hwnd)
{
    if (!uia_clientside_event_start_event_thread(event))
        return E_FAIL;

    if (hwnd == GetDesktopWindow())
        EnumWindows(uia_win_event_enum_top_level_hwnds, (LPARAM)&event->u.clientside.win_event_hwnd_map);

    return uia_hwnd_map_add_hwnd(&event->u.clientside.win_event_hwnd_map, hwnd);
}

/*
 * UI Automation event map.
 */
static struct uia_event_map
{
    struct rb_tree event_map;
    LONG event_count;

    /* rb_tree for serverside events, sorted by PID/event cookie. */
    struct rb_tree serverside_event_map;
    LONG serverside_event_count;
} uia_event_map;

struct uia_event_map_entry
{
    struct rb_entry entry;
    LONG refs;

    int event_id;

    /*
     * List of registered events for this event ID. Events are only removed
     * from the list when the event map entry reference count hits 0 and the
     * entry is destroyed. This avoids dealing with mid-list removal while
     * iterating over the list when an event is raised. Rather than remove
     * an event from the list, we mark an event as being defunct so it is
     * ignored.
     */
    struct list events_list;
    struct list serverside_events_list;
};

struct uia_event_identifier {
    LONG event_cookie;
    LONG proc_id;
};

static int uia_serverside_event_id_compare(const void *key, const struct rb_entry *entry)
{
    struct uia_event *event = RB_ENTRY_VALUE(entry, struct uia_event, u.serverside.serverside_event_entry);
    struct uia_event_identifier *event_id = (struct uia_event_identifier *)key;

    if (event_id->proc_id != event->u.serverside.proc_id)
        return (event_id->proc_id > event->u.serverside.proc_id) - (event_id->proc_id < event->u.serverside.proc_id);
    else
        return (event_id->event_cookie > event->event_cookie) - (event_id->event_cookie < event->event_cookie);
}

static CRITICAL_SECTION event_map_cs;
static CRITICAL_SECTION_DEBUG event_map_cs_debug =
{
    0, 0, &event_map_cs,
    { &event_map_cs_debug.ProcessLocksList, &event_map_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": event_map_cs") }
};
static CRITICAL_SECTION event_map_cs = { &event_map_cs_debug, -1, 0, 0, 0, 0 };

static int uia_event_map_id_compare(const void *key, const struct rb_entry *entry)
{
    struct uia_event_map_entry *event_entry = RB_ENTRY_VALUE(entry, struct uia_event_map_entry, entry);
    int event_id = *((int *)key);

    return (event_entry->event_id > event_id) - (event_entry->event_id < event_id);
}

static struct uia_event_map_entry *uia_get_event_map_entry_for_event(int event_id)
{
    struct uia_event_map_entry *map_entry = NULL;
    struct rb_entry *rb_entry;

    if (uia_event_map.event_count && (rb_entry = rb_get(&uia_event_map.event_map, &event_id)))
        map_entry = RB_ENTRY_VALUE(rb_entry, struct uia_event_map_entry, entry);

    return map_entry;
}

static HRESULT uia_event_map_add_event(struct uia_event *event)
{
    const int subtree_scope = TreeScope_Element | TreeScope_Descendants;
    struct uia_event_map_entry *event_entry;

    if (((event->scope & subtree_scope) == subtree_scope) && event->runtime_id &&
            !uia_compare_safearrays(uia_get_desktop_rt_id(), event->runtime_id, UIAutomationType_IntArray))
        event->desktop_subtree_event = TRUE;

    EnterCriticalSection(&event_map_cs);

    if (!(event_entry = uia_get_event_map_entry_for_event(event->event_id)))
    {
        if (!(event_entry = calloc(1, sizeof(*event_entry))))
        {
            LeaveCriticalSection(&event_map_cs);
            return E_OUTOFMEMORY;
        }

        event_entry->event_id = event->event_id;
        list_init(&event_entry->events_list);
        list_init(&event_entry->serverside_events_list);

        if (!uia_event_map.event_count)
            rb_init(&uia_event_map.event_map, uia_event_map_id_compare);
        rb_put(&uia_event_map.event_map, &event->event_id, &event_entry->entry);
        uia_event_map.event_count++;
    }

    IWineUiaEvent_AddRef(&event->IWineUiaEvent_iface);
    if (event->event_type == EVENT_TYPE_SERVERSIDE)
        list_add_head(&event_entry->serverside_events_list, &event->event_list_entry);
    else
        list_add_head(&event_entry->events_list, &event->event_list_entry);
    InterlockedIncrement(&event_entry->refs);

    event->event_map_entry = event_entry;
    LeaveCriticalSection(&event_map_cs);

    return S_OK;
}

static void uia_event_map_entry_release(struct uia_event_map_entry *entry)
{
    ULONG ref = InterlockedDecrement(&entry->refs);

    if (!ref)
    {
        struct list *cursor, *cursor2;

        EnterCriticalSection(&event_map_cs);

        /*
         * Someone grabbed this while we were waiting to enter the CS, abort
         * destruction.
         */
        if (InterlockedCompareExchange(&entry->refs, 0, 0) != 0)
        {
            LeaveCriticalSection(&event_map_cs);
            return;
        }

        rb_remove(&uia_event_map.event_map, &entry->entry);
        uia_event_map.event_count--;
        LeaveCriticalSection(&event_map_cs);

        /* Release all events in the list. */
        LIST_FOR_EACH_SAFE(cursor, cursor2, &entry->events_list)
        {
            struct uia_event *event = LIST_ENTRY(cursor, struct uia_event, event_list_entry);

            IWineUiaEvent_Release(&event->IWineUiaEvent_iface);
        }

        LIST_FOR_EACH_SAFE(cursor, cursor2, &entry->serverside_events_list)
        {
            struct uia_event *event = LIST_ENTRY(cursor, struct uia_event, event_list_entry);

            IWineUiaEvent_Release(&event->IWineUiaEvent_iface);
        }

        free(entry);
    }
}

HRESULT uia_event_for_each(int event_id, UiaWineEventForEachCallback *callback, void *user_data,
        BOOL clientside_only)
{
    struct uia_event_map_entry *event_entry;
    HRESULT hr = S_OK;
    int i;

    EnterCriticalSection(&event_map_cs);
    if ((event_entry = uia_get_event_map_entry_for_event(event_id)))
        InterlockedIncrement(&event_entry->refs);
    LeaveCriticalSection(&event_map_cs);

    if (!event_entry)
        return S_OK;

    for (i = 0; i < 2; i++)
    {
        struct list *events = !i ? &event_entry->events_list : &event_entry->serverside_events_list;
        struct list *cursor, *cursor2;

        if (i && clientside_only)
            break;

        LIST_FOR_EACH_SAFE(cursor, cursor2, events)
        {
            struct uia_event *event = LIST_ENTRY(cursor, struct uia_event, event_list_entry);

            /* Event is no longer valid. */
            if (InterlockedCompareExchange(&event->event_defunct, 0, 0) != 0)
                continue;

            hr = callback(event, user_data);
            if (FAILED(hr))
                goto exit;
        }
    }

exit:
    if (FAILED(hr))
        WARN("Event callback failed with hr %#lx\n", hr);
    uia_event_map_entry_release(event_entry);
    return hr;
}

/*
 * Functions for struct uia_event_args, a reference counted structure
 * used to store event arguments. This is necessary for serverside events
 * as they're raised on a background thread after the event raising
 * function has returned.
 */
static struct uia_event_args *create_uia_event_args(const struct uia_event_info *event_info)
{
    struct uia_event_args *args = calloc(1, sizeof(*args));

    if (!args)
        return NULL;

    args->simple_args.Type = event_info->event_arg_type;
    args->simple_args.EventId = event_info->event_id;
    args->ref = 1;

    return args;
}

static void uia_event_args_release(struct uia_event_args *args)
{
    if (!InterlockedDecrement(&args->ref))
        free(args);
}

struct event_sink_event
{
    struct list event_sink_list_entry;

    IRawElementProviderSimple *elprov;
    struct uia_event_args *args;
};

static HRESULT uia_event_sink_list_add_event(struct list *sink_events, IRawElementProviderSimple *elprov,
        struct uia_event_args *args)
{
    struct event_sink_event *sink_event = calloc(1, sizeof(*sink_event));

    if (!sink_event)
        return E_OUTOFMEMORY;

    IRawElementProviderSimple_AddRef(elprov);
    InterlockedIncrement(&args->ref);

    sink_event->elprov = elprov;
    sink_event->args = args;
    list_add_tail(sink_events, &sink_event->event_sink_list_entry);

    return S_OK;
}

/*
 * IProxyProviderWinEventSink interface implementation.
 */
struct uia_proxy_win_event_sink {
    IProxyProviderWinEventSink IProxyProviderWinEventSink_iface;
    LONG ref;

    int event_id;
    IUnknown *marshal;
    LONG sink_defunct;
    struct list sink_events;
};

static inline struct uia_proxy_win_event_sink *impl_from_IProxyProviderWinEventSink(IProxyProviderWinEventSink *iface)
{
    return CONTAINING_RECORD(iface, struct uia_proxy_win_event_sink, IProxyProviderWinEventSink_iface);
}

static HRESULT WINAPI uia_proxy_win_event_sink_QueryInterface(IProxyProviderWinEventSink *iface, REFIID riid, void **obj)
{
    struct uia_proxy_win_event_sink *sink = impl_from_IProxyProviderWinEventSink(iface);

    *obj = NULL;
    if (IsEqualIID(riid, &IID_IProxyProviderWinEventSink) || IsEqualIID(riid, &IID_IUnknown))
        *obj = iface;
    else if (IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(sink->marshal, riid, obj);
    else
        return E_NOINTERFACE;

    IProxyProviderWinEventSink_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_proxy_win_event_sink_AddRef(IProxyProviderWinEventSink *iface)
{
    struct uia_proxy_win_event_sink *sink = impl_from_IProxyProviderWinEventSink(iface);
    ULONG ref = InterlockedIncrement(&sink->ref);

    TRACE("%p, refcount %ld\n", sink, ref);

    return ref;
}

static ULONG WINAPI uia_proxy_win_event_sink_Release(IProxyProviderWinEventSink *iface)
{
    struct uia_proxy_win_event_sink *sink = impl_from_IProxyProviderWinEventSink(iface);
    ULONG ref = InterlockedDecrement(&sink->ref);

    TRACE("%p, refcount %ld\n", sink, ref);

    if (!ref)
    {
        assert(list_empty(&sink->sink_events));
        IUnknown_Release(sink->marshal);
        free(sink);
    }

    return ref;
}

static HRESULT WINAPI uia_proxy_win_event_sink_AddAutomationPropertyChangedEvent(IProxyProviderWinEventSink *iface,
        IRawElementProviderSimple *elprov, PROPERTYID prop_id, VARIANT new_value)
{
    FIXME("%p, %p, %d, %s: stub\n", iface, elprov, prop_id, debugstr_variant(&new_value));
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_proxy_win_event_sink_AddAutomationEvent(IProxyProviderWinEventSink *iface,
        IRawElementProviderSimple *elprov, EVENTID event_id)
{
    struct uia_proxy_win_event_sink *sink = impl_from_IProxyProviderWinEventSink(iface);
    struct uia_event_args *args;
    HRESULT hr = S_OK;

    TRACE("%p, %p, %d\n", iface, elprov, event_id);

    if (event_id != sink->event_id)
        return S_OK;

    args = create_uia_event_args(uia_event_info_from_id(event_id));
    if (!args)
        return E_OUTOFMEMORY;

    if (InterlockedCompareExchange(&sink->sink_defunct, 0, 0) == 0)
        hr = uia_event_sink_list_add_event(&sink->sink_events, elprov, args);
    uia_event_args_release(args);
    return hr;
}

static HRESULT WINAPI uia_proxy_win_event_sink_AddStructureChangedEvent(IProxyProviderWinEventSink *iface,
        IRawElementProviderSimple *elprov, enum StructureChangeType structure_change_type, SAFEARRAY *runtime_id)
{
    FIXME("%p, %p, %d, %p: stub\n", iface, elprov, structure_change_type, runtime_id);
    return E_NOTIMPL;
}

static const IProxyProviderWinEventSinkVtbl uia_proxy_event_sink_vtbl = {
    uia_proxy_win_event_sink_QueryInterface,
    uia_proxy_win_event_sink_AddRef,
    uia_proxy_win_event_sink_Release,
    uia_proxy_win_event_sink_AddAutomationPropertyChangedEvent,
    uia_proxy_win_event_sink_AddAutomationEvent,
    uia_proxy_win_event_sink_AddStructureChangedEvent,
};

static HRESULT create_proxy_win_event_sink(struct uia_proxy_win_event_sink **out_sink, int event_id)
{
    struct uia_proxy_win_event_sink *sink = calloc(1, sizeof(*sink));
    HRESULT hr;

    *out_sink = NULL;
    if (!sink)
        return E_OUTOFMEMORY;

    sink->IProxyProviderWinEventSink_iface.lpVtbl = &uia_proxy_event_sink_vtbl;
    sink->ref = 1;
    sink->event_id = event_id;
    list_init(&sink->sink_events);

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&sink->IProxyProviderWinEventSink_iface, &sink->marshal);
    if (FAILED(hr))
    {
        free(sink);
        return hr;
    }

    *out_sink = sink;
    return S_OK;
}

/*
 * UI Automation event thread.
 */
struct uia_event_thread
{
    HANDLE hthread;
    HWND hwnd;
    LONG ref;

    struct list *event_queue;
    HWINEVENTHOOK hook;
};

#define WM_UIA_EVENT_THREAD_STOP (WM_USER + 1)
#define WM_UIA_EVENT_THREAD_PROCESS_QUEUE (WM_USER + 2)
static struct uia_event_thread event_thread;
static CRITICAL_SECTION event_thread_cs;
static CRITICAL_SECTION_DEBUG event_thread_cs_debug =
{
    0, 0, &event_thread_cs,
    { &event_thread_cs_debug.ProcessLocksList, &event_thread_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": event_thread_cs") }
};
static CRITICAL_SECTION event_thread_cs = { &event_thread_cs_debug, -1, 0, 0, 0, 0 };

enum uia_queue_event_type {
    QUEUE_EVENT_TYPE_SERVERSIDE,
    QUEUE_EVENT_TYPE_CLIENTSIDE,
    QUEUE_EVENT_TYPE_WIN_EVENT,
};

struct uia_queue_event
{
    struct list event_queue_entry;
    int queue_event_type;
};

struct uia_queue_uia_event
{
    struct uia_queue_event queue_entry;

    struct uia_event_args *args;
    struct uia_event *event;
    union {
        struct {
            HUIANODE node;
            HUIANODE nav_start_node;
        } serverside;
        struct {
            LRESULT node;
            LRESULT nav_start_node;
        } clientside;
     } u;
};

struct uia_queue_win_event
{
    struct uia_queue_event queue_entry;

    HWINEVENTHOOK hook;
    DWORD event_id;
    HWND hwnd;
    LONG obj_id;
    LONG child_id;
    DWORD thread_id;
    DWORD event_time;
};

static void uia_event_queue_push(struct uia_queue_event *event, int queue_event_type)
{
    event->queue_event_type = queue_event_type;
    EnterCriticalSection(&event_thread_cs);

    if (queue_event_type == QUEUE_EVENT_TYPE_WIN_EVENT)
    {
        struct uia_queue_win_event *win_event = (struct uia_queue_win_event *)event;

        if (win_event->hook != event_thread.hook)
        {
            free(event);
            goto exit;
        }
    }

    assert(event_thread.event_queue);
    list_add_tail(event_thread.event_queue, &event->event_queue_entry);
    PostMessageW(event_thread.hwnd, WM_UIA_EVENT_THREAD_PROCESS_QUEUE, 0, 0);

exit:
    LeaveCriticalSection(&event_thread_cs);
}

static struct uia_queue_event *uia_event_queue_pop(struct list *event_queue)
{
    struct uia_queue_event *queue_event = NULL;

    EnterCriticalSection(&event_thread_cs);

    if (!list_empty(event_queue))
    {
        queue_event = LIST_ENTRY(list_head(event_queue), struct uia_queue_event, event_queue_entry);
        list_remove(list_head(event_queue));
    }

    LeaveCriticalSection(&event_thread_cs);
    return queue_event;
}

static HRESULT uia_raise_clientside_event(struct uia_queue_uia_event *event)
{
    HUIANODE node, nav_start_node;
    HRESULT hr;

    node = nav_start_node = NULL;
    hr = uia_node_from_lresult(event->u.clientside.node, &node, 0);
    if (FAILED(hr))
    {
        WARN("Failed to create node from lresult, hr %#lx\n", hr);
        uia_node_lresult_release(event->u.clientside.nav_start_node);
        return hr;
    }

    if (event->u.clientside.nav_start_node)
    {
        hr = uia_node_from_lresult(event->u.clientside.nav_start_node, &nav_start_node, 0);
        if (FAILED(hr))
        {
            WARN("Failed to create nav_start_node from lresult, hr %#lx\n", hr);
            UiaNodeRelease(node);
            return hr;
        }
    }

    hr = uia_event_invoke(node, nav_start_node, event->args, event->event);
    UiaNodeRelease(node);
    UiaNodeRelease(nav_start_node);

    return hr;
}

static HRESULT uia_raise_serverside_event(struct uia_queue_uia_event *event)
{
    HRESULT hr = S_OK;
    LRESULT lr, lr2;
    VARIANT v, v2;

    /*
     * uia_lresult_from_node is expected to release the node here upon
     * failure.
     */
    lr = lr2 = 0;
    if (!(lr = uia_lresult_from_node(event->u.serverside.node)))
    {
        UiaNodeRelease(event->u.serverside.nav_start_node);
        return E_FAIL;
    }

    if (event->u.serverside.nav_start_node && !(lr2 = uia_lresult_from_node(event->u.serverside.nav_start_node)))
    {
        uia_node_lresult_release(lr);
        return E_FAIL;
    }

    VariantInit(&v2);
    variant_init_i4(&v, lr);
    if (lr2)
        variant_init_i4(&v2, lr2);

    hr = IWineUiaEvent_raise_event(event->event->u.serverside.event_iface, v, v2);
    if (FAILED(hr))
    {
        uia_node_lresult_release(lr);
        uia_node_lresult_release(lr2);
    }

    return hr;
}

/* Check the parent chain of HWNDs, excluding the desktop. */
static BOOL uia_win_event_hwnd_map_contains_ancestors(struct rb_tree *hwnd_map, HWND hwnd)
{
    HWND parent = GetAncestor(hwnd, GA_PARENT);
    const HWND desktop = GetDesktopWindow();

    while (parent && (parent != desktop))
    {
        if (uia_hwnd_map_check_hwnd(hwnd_map, parent))
            return TRUE;

        parent = GetAncestor(parent, GA_PARENT);
    }

    return FALSE;
}

HRESULT create_msaa_provider_from_hwnd(HWND hwnd, int in_child_id, IRawElementProviderSimple **ret_elprov)
{
    IRawElementProviderSimple *elprov;
    IAccessible *acc;
    int child_id;
    HRESULT hr;

    *ret_elprov = NULL;
    hr = AccessibleObjectFromWindow(hwnd, OBJID_CLIENT, &IID_IAccessible, (void **)&acc);
    if (FAILED(hr))
        return hr;

    child_id = in_child_id;
    if (in_child_id != CHILDID_SELF)
    {
        IDispatch *disp;
        VARIANT cid;

        disp = NULL;
        variant_init_i4(&cid, in_child_id);
        hr = IAccessible_get_accChild(acc, cid, &disp);
        if (FAILED(hr))
            TRACE("get_accChild failed with %#lx!\n", hr);

        if (SUCCEEDED(hr) && disp)
        {
            IAccessible_Release(acc);
            hr = IDispatch_QueryInterface(disp, &IID_IAccessible, (void **)&acc);
            IDispatch_Release(disp);
            if (FAILED(hr))
                return hr;

            child_id = CHILDID_SELF;
        }
    }

    hr = create_msaa_provider(acc, child_id, hwnd, TRUE, in_child_id == CHILDID_SELF, &elprov);
    IAccessible_Release(acc);
    if (FAILED(hr))
        return hr;

    *ret_elprov = elprov;
    return S_OK;
}

struct uia_elprov_event_data
{
    IRawElementProviderSimple *elprov;
    struct uia_event_args *args;
    BOOL clientside_only;

    SAFEARRAY *rt_id;
    HUIANODE node;
};

static HRESULT uia_raise_elprov_event_callback(struct uia_event *event, void *data);
static HRESULT uia_win_event_for_each_callback(struct uia_event *event, void *data)
{
    struct uia_queue_win_event *win_event = (struct uia_queue_win_event *)data;
    struct event_sink_event *sink_event, *sink_event2;
    struct uia_proxy_win_event_sink *sink;
    IRawElementProviderSimple *elprov;
    struct uia_node *node_data;
    HUIANODE node;
    HRESULT hr;
    int i;

    /*
     * Check if this HWND, or any of it's ancestors (excluding the desktop)
     * are in our scope.
     */
    if (!uia_hwnd_map_check_hwnd(&event->u.clientside.win_event_hwnd_map, win_event->hwnd) &&
            !uia_win_event_hwnd_map_contains_ancestors(&event->u.clientside.win_event_hwnd_map, win_event->hwnd))
        return S_OK;

    /* Has a native serverside provider, no need to do WinEvent translation. */
    if (UiaHasServerSideProvider(win_event->hwnd))
        return S_OK;

    /*
     * Regardless of the object ID of the WinEvent, OBJID_CLIENT is queried
     * for the HWND with the same child ID as the WinEvent.
     */
    hr = create_msaa_provider_from_hwnd(win_event->hwnd, win_event->child_id, &elprov);
    if (FAILED(hr))
        return hr;

    hr = create_uia_node_from_elprov(elprov, &node, TRUE, NODE_FLAG_IGNORE_COM_THREADING);
    IRawElementProviderSimple_Release(elprov);
    if (FAILED(hr))
        return hr;

    hr = create_proxy_win_event_sink(&sink, event->event_id);
    if (SUCCEEDED(hr))
    {
        node_data = impl_from_IWineUiaNode((IWineUiaNode *)node);
        for (i = 0; i < node_data->prov_count; i++)
        {
            hr = respond_to_win_event_on_node_provider((IWineUiaNode *)node, i, win_event->event_id, win_event->hwnd, win_event->obj_id,
                    win_event->child_id, &sink->IProxyProviderWinEventSink_iface);
            if (FAILED(hr) || !list_empty(&sink->sink_events))
                break;
        }

        InterlockedIncrement(&sink->sink_defunct);
        LIST_FOR_EACH_ENTRY_SAFE(sink_event, sink_event2, &sink->sink_events, struct event_sink_event, event_sink_list_entry)
        {
            struct uia_elprov_event_data event_data = { sink_event->elprov, sink_event->args, TRUE };
            list_remove(&sink_event->event_sink_list_entry);

            hr = uia_raise_elprov_event_callback(event, (void *)&event_data);
            if (FAILED(hr))
                WARN("uia_raise_elprov_event_callback failed with hr %#lx\n", hr);

            UiaNodeRelease(event_data.node);
            SafeArrayDestroy(event_data.rt_id);

            IRawElementProviderSimple_Release(sink_event->elprov);
            uia_event_args_release(sink_event->args);
            free(sink_event);
        }

        IProxyProviderWinEventSink_Release(&sink->IProxyProviderWinEventSink_iface);
    }

    UiaNodeRelease(node);
    return hr;
}

static void uia_event_thread_process_queue(struct list *event_queue)
{
    while (1)
    {
        struct uia_queue_event *event;
        HRESULT hr = S_OK;

        if (!(event = uia_event_queue_pop(event_queue)))
            break;

        switch (event->queue_event_type)
        {
        case QUEUE_EVENT_TYPE_SERVERSIDE:
        case QUEUE_EVENT_TYPE_CLIENTSIDE:
        {
            struct uia_queue_uia_event *uia_event = (struct uia_queue_uia_event *)event;

            if (event->queue_event_type == QUEUE_EVENT_TYPE_SERVERSIDE)
                hr = uia_raise_serverside_event(uia_event);
            else
                hr = uia_raise_clientside_event(uia_event);

            uia_event_args_release(uia_event->args);
            IWineUiaEvent_Release(&uia_event->event->IWineUiaEvent_iface);
            break;
        }

        case QUEUE_EVENT_TYPE_WIN_EVENT:
        {
            struct uia_queue_win_event *win_event = (struct uia_queue_win_event *)event;

            hr = uia_com_win_event_callback(win_event->event_id, win_event->hwnd, win_event->obj_id, win_event->child_id,
                    win_event->thread_id, win_event->event_time);
            if (FAILED(hr))
                WARN("uia_com_win_event_callback failed with hr %#lx\n", hr);

            hr = uia_event_for_each(win_event_to_uia_event_id(win_event->event_id), uia_win_event_for_each_callback,
                    (void *)win_event, TRUE);
            break;
        }

        default:
            break;
        }

        if (FAILED(hr))
            WARN("Failed to raise event type %d with hr %#lx\n", event->queue_event_type, hr);

        free(event);
    }
}

static void CALLBACK uia_event_thread_win_event_proc(HWINEVENTHOOK hook, DWORD event_id, HWND hwnd, LONG obj_id,
        LONG child_id, DWORD thread_id, DWORD event_time)
{
    struct uia_queue_win_event *win_event;

    TRACE("%p, %ld, %p, %ld, %ld, %ld, %ld\n", hook, event_id, hwnd, obj_id, child_id, thread_id, event_time);

    if (!win_event_to_uia_event_id(event_id))
        return;

    if (!(win_event = calloc(1, sizeof(*win_event))))
    {
        ERR("Failed to allocate uia_queue_win_event structure\n");
        return;
    }

    win_event->hook = hook;
    win_event->event_id = event_id;
    win_event->hwnd = hwnd;
    win_event->obj_id = obj_id;
    win_event->child_id = child_id;
    win_event->thread_id = thread_id;
    win_event->event_time = event_time;
    uia_event_queue_push(&win_event->queue_entry, QUEUE_EVENT_TYPE_WIN_EVENT);
}

static DWORD WINAPI uia_event_thread_proc(void *arg)
{
    HANDLE initialized_event = arg;
    struct list event_queue;
    HWINEVENTHOOK hook;
    HWND hwnd;
    MSG msg;

    list_init(&event_queue);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hwnd = CreateWindowW(L"Message", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (!hwnd)
    {
        WARN("CreateWindow failed: %ld\n", GetLastError());
        CoUninitialize();
        FreeLibraryAndExitThread(huia_module, 1);
    }

    event_thread.hwnd = hwnd;
    event_thread.event_queue = &event_queue;
    event_thread.hook = hook = SetWinEventHook(EVENT_MIN, EVENT_MAX, 0, uia_event_thread_win_event_proc, 0, 0,
            WINEVENT_OUTOFCONTEXT);

    /* Initialization complete, thread can now process window messages. */
    SetEvent(initialized_event);
    TRACE("Event thread started.\n");
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if ((msg.hwnd == hwnd) && ((msg.message == WM_UIA_EVENT_THREAD_STOP) ||
                    (msg.message == WM_UIA_EVENT_THREAD_PROCESS_QUEUE)))
        {
            uia_event_thread_process_queue(&event_queue);
            if (msg.message == WM_UIA_EVENT_THREAD_STOP)
                break;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    TRACE("Shutting down UI Automation event thread.\n");

    UnhookWinEvent(hook);
    DestroyWindow(hwnd);
    CoUninitialize();
    FreeLibraryAndExitThread(huia_module, 0);
}

static BOOL uia_start_event_thread(void)
{
    BOOL started = TRUE;

    EnterCriticalSection(&event_thread_cs);
    if (++event_thread.ref == 1)
    {
        HANDLE ready_event = NULL;
        HANDLE events[2];
        HMODULE hmodule;
        DWORD wait_obj;

        /* Increment DLL reference count. */
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (const WCHAR *)uia_start_event_thread, &hmodule);

        events[0] = ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!(event_thread.hthread = CreateThread(NULL, 0, uia_event_thread_proc,
                ready_event, 0, NULL)))
        {
            FreeLibrary(hmodule);
            started = FALSE;
            goto exit;
        }

        events[1] = event_thread.hthread;
        wait_obj = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (wait_obj != WAIT_OBJECT_0)
        {
            CloseHandle(event_thread.hthread);
            started = FALSE;
        }

exit:
        if (ready_event)
            CloseHandle(ready_event);
        if (!started)
            memset(&event_thread, 0, sizeof(event_thread));
    }

    LeaveCriticalSection(&event_thread_cs);
    return started;
}

static void uia_stop_event_thread(void)
{
    EnterCriticalSection(&event_thread_cs);
    if (!--event_thread.ref)
    {
        PostMessageW(event_thread.hwnd, WM_UIA_EVENT_THREAD_STOP, 0, 0);
        CloseHandle(event_thread.hthread);
        memset(&event_thread, 0, sizeof(event_thread));
    }
    LeaveCriticalSection(&event_thread_cs);
}

BOOL uia_clientside_event_start_event_thread(struct uia_event *event)
{
    if (!event->u.clientside.event_thread_started)
        event->u.clientside.event_thread_started = uia_start_event_thread();

    return event->u.clientside.event_thread_started;
}

static void uia_event_clear_advisers(struct uia_event *event)
{
    int i;

    for (i = 0; i < event->event_advisers_count; i++)
        IWineUiaEventAdviser_Release(event->event_advisers[i]);
    free(event->event_advisers);
    event->event_advisers = NULL;
    event->event_advisers_count = event->event_advisers_arr_size = 0;
}

/*
 * IWineUiaEvent interface.
 */
static inline struct uia_event *impl_from_IWineUiaEvent(IWineUiaEvent *iface)
{
    return CONTAINING_RECORD(iface, struct uia_event, IWineUiaEvent_iface);
}

static HRESULT WINAPI uia_event_QueryInterface(IWineUiaEvent *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaEvent) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaEvent_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_event_AddRef(IWineUiaEvent *iface)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    ULONG ref = InterlockedIncrement(&event->ref);

    TRACE("%p, refcount %ld\n", event, ref);
    return ref;
}

static ULONG WINAPI uia_event_Release(IWineUiaEvent *iface)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    ULONG ref = InterlockedDecrement(&event->ref);

    TRACE("%p, refcount %ld\n", event, ref);
    if (!ref)
    {
        /*
         * If this event has an event_map_entry, it should've been released
         * before hitting a reference count of 0.
         */
        assert(!event->event_map_entry);

        SafeArrayDestroy(event->runtime_id);
        if (event->event_type == EVENT_TYPE_CLIENTSIDE)
        {
            uia_cache_request_destroy(&event->u.clientside.cache_req);
            if (event->u.clientside.event_thread_started)
                uia_stop_event_thread();
            uia_hwnd_map_destroy(&event->u.clientside.win_event_hwnd_map);
        }
        else
        {
            EnterCriticalSection(&event_map_cs);
            rb_remove(&uia_event_map.serverside_event_map, &event->u.serverside.serverside_event_entry);
            uia_event_map.serverside_event_count--;
            LeaveCriticalSection(&event_map_cs);
            if (event->u.serverside.event_iface)
                IWineUiaEvent_Release(event->u.serverside.event_iface);
            uia_stop_event_thread();
        }

        uia_event_clear_advisers(event);
        free(event);
    }

    return ref;
}

static HRESULT WINAPI uia_event_advise_events(IWineUiaEvent *iface, BOOL advise_added, LONG adviser_start_idx)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    HRESULT hr;
    int i;

    TRACE("%p, %d, %ld\n", event, advise_added, adviser_start_idx);

    for (i = adviser_start_idx; i < event->event_advisers_count; i++)
    {
        hr = IWineUiaEventAdviser_advise(event->event_advisers[i], advise_added, (UINT_PTR)event);
        if (FAILED(hr))
            return hr;
    }

    /*
     * First call to advise events on a serverside provider, add it to the
     * events list so it can be raised.
     */
    if (!adviser_start_idx && advise_added && event->event_type == EVENT_TYPE_SERVERSIDE)
    {
        hr = uia_event_map_add_event(event);
        if (FAILED(hr))
            WARN("Failed to add event to event map, hr %#lx\n", hr);
    }

    /*
     * Once we've advised of removal, no need to keep the advisers around.
     * We can also release our reference to the event map.
     */
    if (!advise_added)
    {
        InterlockedIncrement(&event->event_defunct);
        uia_event_map_entry_release(event->event_map_entry);
        event->event_map_entry = NULL;

        uia_event_clear_advisers(event);
    }

    return S_OK;
}

static HRESULT WINAPI uia_event_set_event_data(IWineUiaEvent *iface, const GUID *event_guid, LONG scope,
        VARIANT runtime_id, IWineUiaEvent *event_iface)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);

    TRACE("%p, %s, %ld, %s, %p\n", event, debugstr_guid(event_guid), scope, debugstr_variant(&runtime_id), event_iface);

    assert(event->event_type == EVENT_TYPE_SERVERSIDE);

    event->event_id = UiaLookupId(AutomationIdentifierType_Event, event_guid);
    event->scope = scope;
    if (V_VT(&runtime_id) == (VT_I4 | VT_ARRAY))
    {
        HRESULT hr;

        hr = SafeArrayCopy(V_ARRAY(&runtime_id), &event->runtime_id);
        if (FAILED(hr))
        {
            WARN("Failed to copy runtime id, hr %#lx\n", hr);
            return hr;
        }
    }
    event->u.serverside.event_iface = event_iface;
    IWineUiaEvent_AddRef(event_iface);

    return S_OK;
}

static HRESULT WINAPI uia_event_raise_event(IWineUiaEvent *iface, VARIANT in_node, VARIANT in_nav_start_node)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    struct uia_queue_uia_event *queue_event;
    struct uia_event_args *args;

    TRACE("%p, %s, %s\n", iface, debugstr_variant(&in_node), debugstr_variant(&in_nav_start_node));

    assert(event->event_type != EVENT_TYPE_SERVERSIDE);

    if (!(queue_event = calloc(1, sizeof(*queue_event))))
        return E_OUTOFMEMORY;

    if (!(args = create_uia_event_args(uia_event_info_from_id(event->event_id))))
    {
        free(queue_event);
        return E_OUTOFMEMORY;
    }

    queue_event->args = args;
    queue_event->event = event;
    queue_event->u.clientside.node = V_I4(&in_node);
    if (V_VT(&in_nav_start_node) == VT_I4)
        queue_event->u.clientside.nav_start_node = V_I4(&in_nav_start_node);

    IWineUiaEvent_AddRef(&event->IWineUiaEvent_iface);
    uia_event_queue_push(&queue_event->queue_entry, QUEUE_EVENT_TYPE_CLIENTSIDE);

    return S_OK;
}

static const IWineUiaEventVtbl uia_event_vtbl = {
    uia_event_QueryInterface,
    uia_event_AddRef,
    uia_event_Release,
    uia_event_advise_events,
    uia_event_set_event_data,
    uia_event_raise_event,
};

static struct uia_event *unsafe_impl_from_IWineUiaEvent(IWineUiaEvent *iface)
{
    if (!iface || (iface->lpVtbl != &uia_event_vtbl))
        return NULL;

    return CONTAINING_RECORD(iface, struct uia_event, IWineUiaEvent_iface);
}

static HRESULT create_uia_event(struct uia_event **out_event, LONG event_cookie, int event_type)
{
    struct uia_event *event = calloc(1, sizeof(*event));

    *out_event = NULL;
    if (!event)
        return E_OUTOFMEMORY;

    event->IWineUiaEvent_iface.lpVtbl = &uia_event_vtbl;
    event->ref = 1;
    event->event_cookie = event_cookie;
    event->event_type = event_type;
    *out_event = event;

    return S_OK;
}

static HRESULT create_clientside_uia_event(struct uia_event **out_event, int event_id, int scope,
        UiaWineEventCallback *cback, void *cback_data, SAFEARRAY *runtime_id)
{
    struct uia_event *event = NULL;
    static LONG next_event_cookie;
    HRESULT hr;

    *out_event = NULL;
    hr = create_uia_event(&event, InterlockedIncrement(&next_event_cookie), EVENT_TYPE_CLIENTSIDE);
    if (FAILED(hr))
        return hr;

    event->runtime_id = runtime_id;
    event->event_id = event_id;
    event->scope = scope;
    event->u.clientside.event_callback = cback;
    event->u.clientside.callback_data = cback_data;
    uia_hwnd_map_init(&event->u.clientside.win_event_hwnd_map);

    *out_event = event;
    return S_OK;
}

HRESULT create_serverside_uia_event(struct uia_event **out_event, LONG process_id, LONG event_cookie)
{
    struct uia_event_identifier event_identifier = { event_cookie, process_id };
    struct rb_entry *rb_entry;
    struct uia_event *event;
    HRESULT hr = S_OK;

    /*
     * Attempt to lookup an existing event for this PID/event_cookie. If there
     * is one, return S_FALSE.
     */
    *out_event = NULL;
    EnterCriticalSection(&event_map_cs);
    if (uia_event_map.serverside_event_count && (rb_entry = rb_get(&uia_event_map.serverside_event_map, &event_identifier)))
    {
        *out_event = RB_ENTRY_VALUE(rb_entry, struct uia_event, u.serverside.serverside_event_entry);
        hr = S_FALSE;
        goto exit;
    }

    hr = create_uia_event(&event, event_cookie, EVENT_TYPE_SERVERSIDE);
    if (FAILED(hr))
        goto exit;

    if (!uia_start_event_thread())
    {
        free(event);
        hr = E_FAIL;
        goto exit;
    }

    event->u.serverside.proc_id = process_id;
    uia_event_map.serverside_event_count++;
    if (uia_event_map.serverside_event_count == 1)
        rb_init(&uia_event_map.serverside_event_map, uia_serverside_event_id_compare);
    rb_put(&uia_event_map.serverside_event_map, &event_identifier, &event->u.serverside.serverside_event_entry);
    *out_event = event;

exit:
    LeaveCriticalSection(&event_map_cs);
    return hr;
}

static HRESULT uia_event_add_event_adviser(IWineUiaEventAdviser *adviser, struct uia_event *event)
{
    if (!uia_array_reserve((void **)&event->event_advisers, &event->event_advisers_arr_size,
                event->event_advisers_count + 1, sizeof(*event->event_advisers)))
        return E_OUTOFMEMORY;

    event->event_advisers[event->event_advisers_count] = adviser;
    IWineUiaEventAdviser_AddRef(adviser);
    event->event_advisers_count++;

    return S_OK;
}

/*
 * IWineUiaEventAdviser interface.
 */
struct uia_event_adviser {
    IWineUiaEventAdviser IWineUiaEventAdviser_iface;
    LONG ref;

    IRawElementProviderAdviseEvents *advise_events;
    DWORD git_cookie;
};

static inline struct uia_event_adviser *impl_from_IWineUiaEventAdviser(IWineUiaEventAdviser *iface)
{
    return CONTAINING_RECORD(iface, struct uia_event_adviser, IWineUiaEventAdviser_iface);
}

static HRESULT WINAPI uia_event_adviser_QueryInterface(IWineUiaEventAdviser *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaEventAdviser) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaEventAdviser_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_event_adviser_AddRef(IWineUiaEventAdviser *iface)
{
    struct uia_event_adviser *adv_events = impl_from_IWineUiaEventAdviser(iface);
    ULONG ref = InterlockedIncrement(&adv_events->ref);

    TRACE("%p, refcount %ld\n", adv_events, ref);
    return ref;
}

static ULONG WINAPI uia_event_adviser_Release(IWineUiaEventAdviser *iface)
{
    struct uia_event_adviser *adv_events = impl_from_IWineUiaEventAdviser(iface);
    ULONG ref = InterlockedDecrement(&adv_events->ref);

    TRACE("%p, refcount %ld\n", adv_events, ref);
    if (!ref)
    {
        if (adv_events->git_cookie)
        {
            if (FAILED(unregister_interface_in_git(adv_events->git_cookie)))
                WARN("Failed to revoke advise events interface from GIT\n");
        }
        IRawElementProviderAdviseEvents_Release(adv_events->advise_events);
        free(adv_events);
    }

    return ref;
}

static HRESULT WINAPI uia_event_adviser_advise(IWineUiaEventAdviser *iface, BOOL advise_added, LONG_PTR huiaevent)
{
    struct uia_event_adviser *adv_events = impl_from_IWineUiaEventAdviser(iface);
    struct uia_event *event_data = (struct uia_event *)huiaevent;
    IRawElementProviderAdviseEvents *advise_events;
    HRESULT hr;

    TRACE("%p, %d, %#Ix\n", adv_events, advise_added, huiaevent);

    if (adv_events->git_cookie)
    {
        hr = get_interface_in_git(&IID_IRawElementProviderAdviseEvents, adv_events->git_cookie,
                (IUnknown **)&advise_events);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        advise_events = adv_events->advise_events;
        IRawElementProviderAdviseEvents_AddRef(advise_events);
    }

    if (advise_added)
        hr = IRawElementProviderAdviseEvents_AdviseEventAdded(advise_events, event_data->event_id, NULL);
    else
        hr = IRawElementProviderAdviseEvents_AdviseEventRemoved(advise_events, event_data->event_id, NULL);

    IRawElementProviderAdviseEvents_Release(advise_events);
    return hr;
}

static const IWineUiaEventAdviserVtbl uia_event_adviser_vtbl = {
    uia_event_adviser_QueryInterface,
    uia_event_adviser_AddRef,
    uia_event_adviser_Release,
    uia_event_adviser_advise,
};

HRESULT uia_event_add_provider_event_adviser(IRawElementProviderAdviseEvents *advise_events, struct uia_event *event)
{
    struct uia_event_adviser *adv_events;
    IRawElementProviderSimple *elprov;
    enum ProviderOptions prov_opts;
    HRESULT hr;

    hr = IRawElementProviderAdviseEvents_QueryInterface(advise_events, &IID_IRawElementProviderSimple,
            (void **)&elprov);
    if (FAILED(hr))
    {
        ERR("Failed to get IRawElementProviderSimple from advise events\n");
        return E_FAIL;
    }

    hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opts);
    IRawElementProviderSimple_Release(elprov);
    if (FAILED(hr))
        return hr;

    if (!(adv_events = calloc(1, sizeof(*adv_events))))
        return E_OUTOFMEMORY;

    if (prov_opts & ProviderOptions_UseComThreading)
    {
        hr = register_interface_in_git((IUnknown *)advise_events, &IID_IRawElementProviderAdviseEvents,
                &adv_events->git_cookie);
        if (FAILED(hr))
        {
            free(adv_events);
            return hr;
        }
    }

    adv_events->IWineUiaEventAdviser_iface.lpVtbl = &uia_event_adviser_vtbl;
    adv_events->ref = 1;
    adv_events->advise_events = advise_events;
    IRawElementProviderAdviseEvents_AddRef(advise_events);

    hr = uia_event_add_event_adviser(&adv_events->IWineUiaEventAdviser_iface, event);
    IWineUiaEventAdviser_Release(&adv_events->IWineUiaEventAdviser_iface);

    return hr;
}

/*
 * IWineUiaEventAdviser interface for serverside events.
 */
struct uia_serverside_event_adviser {
    IWineUiaEventAdviser IWineUiaEventAdviser_iface;
    LONG ref;

    IWineUiaEvent *event_iface;
};

static inline struct uia_serverside_event_adviser *impl_from_serverside_IWineUiaEventAdviser(IWineUiaEventAdviser *iface)
{
    return CONTAINING_RECORD(iface, struct uia_serverside_event_adviser, IWineUiaEventAdviser_iface);
}

static HRESULT WINAPI uia_serverside_event_adviser_QueryInterface(IWineUiaEventAdviser *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaEventAdviser) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaEventAdviser_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_serverside_event_adviser_AddRef(IWineUiaEventAdviser *iface)
{
    struct uia_serverside_event_adviser *adv_events = impl_from_serverside_IWineUiaEventAdviser(iface);
    ULONG ref = InterlockedIncrement(&adv_events->ref);

    TRACE("%p, refcount %ld\n", adv_events, ref);
    return ref;
}

static ULONG WINAPI uia_serverside_event_adviser_Release(IWineUiaEventAdviser *iface)
{
    struct uia_serverside_event_adviser *adv_events = impl_from_serverside_IWineUiaEventAdviser(iface);
    ULONG ref = InterlockedDecrement(&adv_events->ref);

    TRACE("%p, refcount %ld\n", adv_events, ref);
    if (!ref)
    {
        IWineUiaEvent_Release(adv_events->event_iface);
        free(adv_events);
    }
    return ref;
}

static HRESULT WINAPI uia_serverside_event_adviser_advise(IWineUiaEventAdviser *iface, BOOL advise_added, LONG_PTR huiaevent)
{
    struct uia_serverside_event_adviser *adv_events = impl_from_serverside_IWineUiaEventAdviser(iface);
    struct uia_event *event_data = (struct uia_event *)huiaevent;
    HRESULT hr;

    TRACE("%p, %d, %#Ix\n", adv_events, advise_added, huiaevent);

    if (advise_added)
    {
        const struct uia_event_info *event_info = uia_event_info_from_id(event_data->event_id);
        VARIANT v;

        VariantInit(&v);
        if (event_data->runtime_id)
        {
            V_VT(&v) = VT_I4 | VT_ARRAY;
            V_ARRAY(&v) = event_data->runtime_id;
        }

        hr = IWineUiaEvent_set_event_data(adv_events->event_iface, event_info->guid, event_data->scope, v,
                &event_data->IWineUiaEvent_iface);
        if (FAILED(hr))
        {
            WARN("Failed to set event data on serverside event, hr %#lx\n", hr);
            return hr;
        }
    }

    return IWineUiaEvent_advise_events(adv_events->event_iface, advise_added, 0);
}

static const IWineUiaEventAdviserVtbl uia_serverside_event_adviser_vtbl = {
    uia_serverside_event_adviser_QueryInterface,
    uia_serverside_event_adviser_AddRef,
    uia_serverside_event_adviser_Release,
    uia_serverside_event_adviser_advise,
};

HRESULT uia_event_add_serverside_event_adviser(IWineUiaEvent *serverside_event, struct uia_event *event)
{
    struct uia_serverside_event_adviser *adv_events;
    HRESULT hr;

    /*
     * Need to create a proxy IWineUiaEvent for our clientside event to use
     * this serverside IWineUiaEvent proxy from the appropriate apartment.
     */
    if (!event->u.clientside.git_cookie)
    {
        if (!uia_clientside_event_start_event_thread(event))
            return E_FAIL;

        hr = register_interface_in_git((IUnknown *)&event->IWineUiaEvent_iface, &IID_IWineUiaEvent,
                &event->u.clientside.git_cookie);
        if (FAILED(hr))
            return hr;
    }

    if (!(adv_events = calloc(1, sizeof(*adv_events))))
        return E_OUTOFMEMORY;

    adv_events->IWineUiaEventAdviser_iface.lpVtbl = &uia_serverside_event_adviser_vtbl;
    adv_events->ref = 1;
    adv_events->event_iface = serverside_event;
    IWineUiaEvent_AddRef(serverside_event);

    hr = uia_event_add_event_adviser(&adv_events->IWineUiaEventAdviser_iface, event);
    IWineUiaEventAdviser_Release(&adv_events->IWineUiaEventAdviser_iface);

    return hr;
}

static HRESULT uia_event_advise(struct uia_event *event, BOOL advise_added, LONG start_idx)
{
    IWineUiaEvent *event_iface;
    HRESULT hr;

    if (event->u.clientside.git_cookie)
    {
        hr = get_interface_in_git(&IID_IWineUiaEvent, event->u.clientside.git_cookie,
                (IUnknown **)&event_iface);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        event_iface = &event->IWineUiaEvent_iface;
        IWineUiaEvent_AddRef(event_iface);
    }

    hr = IWineUiaEvent_advise_events(event_iface, advise_added, start_idx);
    IWineUiaEvent_Release(event_iface);

    return hr;
}

HRESULT uia_event_advise_node(struct uia_event *event, HUIANODE node)
{
    int old_event_advisers_count = event->event_advisers_count;
    HRESULT hr;

    hr = attach_event_to_uia_node(node, event);
    if (FAILED(hr))
        return hr;

    if (event->event_advisers_count != old_event_advisers_count)
        hr = uia_event_advise(event, TRUE, old_event_advisers_count);

    return hr;
}

/***********************************************************************
 *          UiaEventAddWindow (uiautomationcore.@)
 */
HRESULT WINAPI UiaEventAddWindow(HUIAEVENT huiaevent, HWND hwnd)
{
    struct uia_event *event = unsafe_impl_from_IWineUiaEvent((IWineUiaEvent *)huiaevent);
    HUIANODE node;
    HRESULT hr;

    TRACE("(%p, %p)\n", huiaevent, hwnd);

    if (!event)
        return E_INVALIDARG;

    assert(event->event_type == EVENT_TYPE_CLIENTSIDE);

    hr = UiaNodeFromHandle(hwnd, &node);
    if (FAILED(hr))
        return hr;

    hr = uia_event_advise_node(event, node);
    UiaNodeRelease(node);

    return hr;
}

static HRESULT uia_clientside_event_callback(struct uia_event *event, struct uia_event_args *args,
        SAFEARRAY *cache_req, BSTR tree_struct)
{
    UiaEventCallback *event_callback = (UiaEventCallback *)event->u.clientside.callback_data;

    event_callback(&args->simple_args, cache_req, tree_struct);

    return S_OK;
}

HRESULT uia_add_clientside_event(HUIANODE huianode, EVENTID event_id, enum TreeScope scope, PROPERTYID *prop_ids,
        int prop_ids_count, struct UiaCacheRequest *cache_req, SAFEARRAY *rt_id, UiaWineEventCallback *cback,
        void *cback_data, HUIAEVENT *huiaevent)
{
    struct uia_event *event;
    SAFEARRAY *sa;
    HRESULT hr;

    hr = SafeArrayCopy(rt_id, &sa);
    if (FAILED(hr))
        return hr;

    hr = create_clientside_uia_event(&event, event_id, scope, cback, cback_data, sa);
    if (FAILED(hr))
    {
        SafeArrayDestroy(sa);
        return hr;
    }

    hr = uia_cache_request_clone(&event->u.clientside.cache_req, cache_req);
    if (FAILED(hr))
        goto exit;

    hr = attach_event_to_uia_node(huianode, event);
    if (FAILED(hr))
        goto exit;

    hr = uia_event_advise(event, TRUE, 0);
    if (FAILED(hr))
        goto exit;

    hr = uia_event_map_add_event(event);
    if (FAILED(hr))
        goto exit;

    *huiaevent = (HUIAEVENT)event;

exit:
    if (FAILED(hr))
        IWineUiaEvent_Release(&event->IWineUiaEvent_iface);

    return hr;
}

/***********************************************************************
 *          UiaAddEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaAddEvent(HUIANODE huianode, EVENTID event_id, UiaEventCallback *callback, enum TreeScope scope,
        PROPERTYID *prop_ids, int prop_ids_count, struct UiaCacheRequest *cache_req, HUIAEVENT *huiaevent)
{
    const struct uia_event_info *event_info = uia_event_info_from_id(event_id);
    SAFEARRAY *sa;
    HRESULT hr;

    TRACE("(%p, %d, %p, %#x, %p, %d, %p, %p)\n", huianode, event_id, callback, scope, prop_ids, prop_ids_count,
            cache_req, huiaevent);

    if (!huianode || !callback || !cache_req || !huiaevent)
        return E_INVALIDARG;

    if (!event_info)
        WARN("No event information for event ID %d\n", event_id);

    *huiaevent = NULL;
    if (event_info && (event_info->event_arg_type == EventArgsType_PropertyChanged))
    {
        FIXME("Property changed event registration currently unimplemented\n");
        return E_NOTIMPL;
    }

    hr = UiaGetRuntimeId(huianode, &sa);
    if (FAILED(hr))
        return hr;

    hr = uia_add_clientside_event(huianode, event_id, scope, prop_ids, prop_ids_count, cache_req, sa,
            uia_clientside_event_callback, (void *)callback, huiaevent);
    SafeArrayDestroy(sa);

    return hr;
}

/***********************************************************************
 *          UiaRemoveEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRemoveEvent(HUIAEVENT huiaevent)
{
    struct uia_event *event = unsafe_impl_from_IWineUiaEvent((IWineUiaEvent *)huiaevent);
    HRESULT hr;

    TRACE("(%p)\n", event);

    if (!event)
        return E_INVALIDARG;

    assert(event->event_type == EVENT_TYPE_CLIENTSIDE);
    hr = uia_event_advise(event, FALSE, 0);
    if (FAILED(hr))
        return hr;

    if (event->u.clientside.git_cookie)
    {
        hr = unregister_interface_in_git(event->u.clientside.git_cookie);
        if (FAILED(hr))
            return hr;
    }

    IWineUiaEvent_Release(&event->IWineUiaEvent_iface);
    return S_OK;
}

HRESULT uia_event_invoke(HUIANODE node, HUIANODE nav_start_node, struct uia_event_args *args, struct uia_event *event)
{
    HRESULT hr = S_OK;

    if (event->event_type == EVENT_TYPE_CLIENTSIDE)
    {
        SAFEARRAY *out_req;
        BSTR tree_struct;

        if (nav_start_node && (hr = uia_event_check_node_within_event_scope(event, nav_start_node, NULL, NULL)) != S_OK)
            return hr;

        hr = UiaGetUpdatedCache(node, &event->u.clientside.cache_req, NormalizeState_View, NULL, &out_req,
                &tree_struct);
        if (SUCCEEDED(hr))
        {
            hr = event->u.clientside.event_callback(event, args, out_req, tree_struct);
            if (FAILED(hr))
                WARN("Event callback failed with hr %#lx\n", hr);
            SafeArrayDestroy(out_req);
            SysFreeString(tree_struct);
        }
    }
    else
    {
        struct uia_queue_uia_event *queue_event;
        HUIANODE node2, nav_start_node2;

        if (!(queue_event = calloc(1, sizeof(*queue_event))))
            return E_OUTOFMEMORY;

        node2 = nav_start_node2 = NULL;
        hr = clone_uia_node(node, &node2);
        if (FAILED(hr))
        {
            free(queue_event);
            return hr;
        }

        if (nav_start_node)
        {
            hr = clone_uia_node(nav_start_node, &nav_start_node2);
            if (FAILED(hr))
            {
                free(queue_event);
                UiaNodeRelease(node2);
                return hr;
            }
        }

        queue_event->args = args;
        queue_event->event = event;
        queue_event->u.serverside.node = node2;
        queue_event->u.serverside.nav_start_node = nav_start_node2;

        InterlockedIncrement(&args->ref);
        IWineUiaEvent_AddRef(&event->IWineUiaEvent_iface);
        uia_event_queue_push(&queue_event->queue_entry, QUEUE_EVENT_TYPE_SERVERSIDE);
    }

    return hr;
}

static void set_refuse_hwnd_providers(struct uia_node *node, BOOL refuse_hwnd_providers)
{
    struct uia_provider *prov_data = impl_from_IWineUiaProvider(node->prov[get_node_provider_type_at_idx(node, 0)]);

    prov_data->refuse_hwnd_node_providers = refuse_hwnd_providers;
}

/*
 * Check if a node is within the scope of a registered event.
 * If it is, return S_OK.
 * If it isn't, return S_FALSE.
 * Upon failure, return a failure HR.
 */
HRESULT uia_event_check_node_within_event_scope(struct uia_event *event, HUIANODE node, SAFEARRAY *rt_id,
        HUIANODE *clientside_nav_node_out)
{
    struct UiaPropertyCondition prop_cond = { ConditionType_Property, UIA_RuntimeIdPropertyId };
    struct uia_node *node_data = impl_from_IWineUiaNode((IWineUiaNode *)node);
    BOOL in_scope = FALSE;
    HRESULT hr = S_FALSE;

    if (clientside_nav_node_out)
        *clientside_nav_node_out = NULL;

    if (event->event_type == EVENT_TYPE_SERVERSIDE)
        assert(clientside_nav_node_out);

    /* Event is no longer valid. */
    if (InterlockedCompareExchange(&event->event_defunct, 0, 0) != 0)
        return S_FALSE;

    /* Can't match an event that doesn't have a runtime ID, early out. */
    if (!event->runtime_id)
        return S_FALSE;

    if (event->desktop_subtree_event)
        return S_OK;

    if (rt_id && !uia_compare_safearrays(rt_id, event->runtime_id, UIAutomationType_IntArray))
        return (event->scope & TreeScope_Element) ? S_OK : S_FALSE;

    if (!(event->scope & (TreeScope_Descendants | TreeScope_Children)))
        return S_FALSE;

    V_VT(&prop_cond.Value) = VT_I4 | VT_ARRAY;
    V_ARRAY(&prop_cond.Value) = event->runtime_id;

    IWineUiaNode_AddRef(&node_data->IWineUiaNode_iface);
    while (1)
    {
        HUIANODE node2 = NULL;

        /*
         * When trying to match serverside events through navigation, we
         * don't want any clientside providers added in the server process.
         * Once we encounter a provider with an HWND, we pass it off to the
         * client for any further navigation.
         */
        if (event->event_type == EVENT_TYPE_SERVERSIDE)
        {
            if (node_data->hwnd)
            {
                *clientside_nav_node_out = (HUIANODE)&node_data->IWineUiaNode_iface;
                IWineUiaNode_AddRef(&node_data->IWineUiaNode_iface);
                in_scope = TRUE;
                break;
            }
            set_refuse_hwnd_providers(node_data, TRUE);
        }

        hr = navigate_uia_node(node_data, NavigateDirection_Parent, &node2);
        if (FAILED(hr) || !node2)
            break;

        IWineUiaNode_Release(&node_data->IWineUiaNode_iface);

        node_data = impl_from_IWineUiaNode((IWineUiaNode *)node2);
        hr = uia_condition_check(node2, (struct UiaCondition *)&prop_cond);
        if (FAILED(hr))
            break;

        if (uia_condition_matched(hr))
        {
            in_scope = TRUE;
            break;
        }

        if (!(event->scope & TreeScope_Descendants))
            break;
    }
    IWineUiaNode_Release(&node_data->IWineUiaNode_iface);

    if (FAILED(hr))
        return hr;

    return in_scope ? S_OK : S_FALSE;
}

static HRESULT uia_raise_elprov_event_callback(struct uia_event *event, void *data)
{
    struct uia_elprov_event_data *event_data = (struct uia_elprov_event_data *)data;
    HUIANODE nav_node = NULL;
    HRESULT hr = S_OK;

    if (!event_data->node)
    {
        /*
         * For events raised on server-side providers, we don't want to add any
         * clientside HWND providers.
         */
        hr = create_uia_node_from_elprov(event_data->elprov, &event_data->node, event_data->clientside_only, 0);
        if (FAILED(hr))
            return hr;

        hr = UiaGetRuntimeId(event_data->node, &event_data->rt_id);
        if (FAILED(hr))
            return hr;
    }

    hr = uia_event_check_node_within_event_scope(event, event_data->node, event_data->rt_id, &nav_node);
    if (hr == S_OK)
        hr = uia_event_invoke(event_data->node, nav_node, event_data->args, event);

    UiaNodeRelease(nav_node);
    return hr;
}

static HRESULT uia_raise_elprov_event(IRawElementProviderSimple *elprov, struct uia_event_args *args)
{
    struct uia_elprov_event_data event_data = { elprov, args };
    enum ProviderOptions prov_opts = 0;
    HRESULT hr;

    hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opts);
    if (FAILED(hr))
        return hr;

    event_data.clientside_only = !(prov_opts & ProviderOptions_ServerSideProvider);
    hr = uia_event_for_each(args->simple_args.EventId, uia_raise_elprov_event_callback, (void *)&event_data,
            event_data.clientside_only);
    if (FAILED(hr))
        WARN("uia_event_for_each failed with hr %#lx\n", hr);

    UiaNodeRelease(event_data.node);
    SafeArrayDestroy(event_data.rt_id);

    return hr;
}

/***********************************************************************
 *          UiaRaiseAutomationEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseAutomationEvent(IRawElementProviderSimple *elprov, EVENTID id)
{
    const struct uia_event_info *event_info = uia_event_info_from_id(id);
    struct uia_event_args *args;
    HRESULT hr;

    TRACE("(%p, %d)\n", elprov, id);

    if (!elprov)
        return E_INVALIDARG;

    if (!event_info || event_info->event_arg_type != EventArgsType_Simple)
    {
        if (!event_info)
            FIXME("No event info structure for event id %d\n", id);
        else
            WARN("Wrong event raising function for event args type %d\n", event_info->event_arg_type);

        return S_OK;
    }

    args = create_uia_event_args(event_info);
    if (!args)
        return E_OUTOFMEMORY;

    hr = uia_raise_elprov_event(elprov, args);
    uia_event_args_release(args);
    if (FAILED(hr))
        return hr;

    return S_OK;
}
