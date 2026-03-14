/*
 * Copyright 2022 Connor McAdams for CodeWeavers
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

#include "uiautomation.h"
#include "uia_classes.h"
#include "wine/list.h"
#include "wine/rbtree.h"
#include "assert.h"

extern HMODULE huia_module;

enum uia_prop_type {
    PROP_TYPE_UNKNOWN,
    PROP_TYPE_ELEM_PROP,
    PROP_TYPE_SPECIAL,
    PROP_TYPE_PATTERN_PROP,
};

/*
 * HUIANODEs that have an associated HWND are able to pull data from up to 4
 * different providers:
 *
 * - Override providers are used to override values from all other providers.
 * - Main providers are the base provider for an HUIANODE.
 * - Nonclient providers are used to represent the nonclient area of the HWND.
 * - HWND providers are used to represent data from the HWND as a whole, such
 *   as the bounding box.
 *
 * When a property is requested from the node, each provider is queried in
 * descending order starting with the override provider until either one
 * returns a property or there are no more providers to query.
 */
enum uia_node_prov_type {
    PROV_TYPE_OVERRIDE,
    PROV_TYPE_MAIN,
    PROV_TYPE_NONCLIENT,
    PROV_TYPE_HWND,
    PROV_TYPE_COUNT,
};

enum uia_node_flags {
    NODE_FLAG_IGNORE_CLIENTSIDE_HWND_PROVS = 0x01,
    NODE_FLAG_NO_PREPARE = 0x02,
    NODE_FLAG_IGNORE_COM_THREADING = 0x04,
};

struct uia_node {
    IWineUiaNode IWineUiaNode_iface;
    LONG ref;

    IWineUiaProvider *prov[PROV_TYPE_COUNT];
    DWORD git_cookie[PROV_TYPE_COUNT];
    int prov_count;
    int parent_link_idx;
    int creator_prov_idx;

    HWND hwnd;
    BOOL no_prepare;
    BOOL nested_node;
    BOOL disconnected;
    int creator_prov_type;
    BOOL ignore_com_threading;
    BOOL ignore_clientside_hwnd_provs;

    struct list prov_thread_list_entry;
    struct list node_map_list_entry;
    struct uia_provider_thread_map_entry *map;
};

static inline struct uia_node *impl_from_IWineUiaNode(IWineUiaNode *iface)
{
    return CONTAINING_RECORD(iface, struct uia_node, IWineUiaNode_iface);
}

struct uia_provider {
    IWineUiaProvider IWineUiaProvider_iface;
    LONG ref;

    IRawElementProviderSimple *elprov;
    BOOL refuse_hwnd_node_providers;
    BOOL return_nested_node;
    BOOL parent_check_ran;
    BOOL has_parent;
    HWND hwnd;
};

static inline struct uia_provider *impl_from_IWineUiaProvider(IWineUiaProvider *iface)
{
    return CONTAINING_RECORD(iface, struct uia_provider, IWineUiaProvider_iface);
}

struct uia_event_args
{
    struct UiaEventArgs simple_args;
    LONG ref;
};

enum uia_event_type {
    EVENT_TYPE_CLIENTSIDE,
    EVENT_TYPE_SERVERSIDE,
};

struct uia_event
{
    IWineUiaEvent IWineUiaEvent_iface;
    LONG ref;

    BOOL desktop_subtree_event;
    SAFEARRAY *runtime_id;
    int event_id;
    int scope;

    IWineUiaEventAdviser **event_advisers;
    int event_advisers_count;
    SIZE_T event_advisers_arr_size;

    struct list event_list_entry;
    struct uia_event_map_entry *event_map_entry;
    LONG event_defunct;

    LONG event_cookie;
    int event_type;
    union
    {
        struct {
            struct UiaCacheRequest cache_req;
            HRESULT (*event_callback)(struct uia_event *, struct uia_event_args *, SAFEARRAY *, BSTR);
            void *callback_data;

            struct rb_tree win_event_hwnd_map;
            BOOL event_thread_started;
            DWORD git_cookie;
        } clientside;
        struct {
            IWineUiaEvent *event_iface;

            struct rb_entry serverside_event_entry;
            LONG proc_id;
        } serverside;
     } u;
};

typedef HRESULT UiaWineEventCallback(struct uia_event *, struct uia_event_args *, SAFEARRAY *, BSTR);
typedef HRESULT UiaWineEventForEachCallback(struct uia_event *, void *);

static inline void variant_init_bool(VARIANT *v, BOOL val)
{
    V_VT(v) = VT_BOOL;
    V_BOOL(v) = val ? VARIANT_TRUE : VARIANT_FALSE;
}

static inline void variant_init_i4(VARIANT *v, int val)
{
    V_VT(v) = VT_I4;
    V_I4(v) = val;
}

static inline void get_variant_for_node(HUIANODE node, VARIANT *v)
{
#ifdef _WIN64
    V_VT(v) = VT_I8;
    V_I8(v) = (UINT64)node;
#else
    V_VT(v) = VT_I4;
    V_I4(v) = (UINT32)node;
#endif
}

static inline BOOL uia_array_reserve(void **elements, SIZE_T *capacity, SIZE_T count, SIZE_T size)
{
    SIZE_T max_capacity, new_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(1, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = count;

    new_elements = _recalloc(*elements, new_capacity, size);
    if (!new_elements)
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

enum provider_method_flags {
    PROV_METHOD_FLAG_RETURN_NODE_LRES = 0x0001,
};

/* uia_client.c */
int get_node_provider_type_at_idx(struct uia_node *node, int idx);
HRESULT get_focus_from_node_provider(IWineUiaNode *node, int idx, LONG flags, VARIANT *ret_val);
HRESULT respond_to_win_event_on_node_provider(IWineUiaNode *node, int idx, DWORD win_event, HWND hwnd, LONG obj_id,
        LONG child_id, IProxyProviderWinEventSink *sink);
HRESULT create_node_from_node_provider(IWineUiaNode *node, int idx, LONG flags, VARIANT *ret_val);
HRESULT attach_event_to_uia_node(HUIANODE node, struct uia_event *event);
HRESULT clone_uia_node(HUIANODE in_node, HUIANODE *out_node);
HRESULT navigate_uia_node(struct uia_node *node, int nav_dir, HUIANODE *out_node);
HRESULT create_uia_node_from_elprov(IRawElementProviderSimple *elprov, HUIANODE *out_node,
        BOOL get_hwnd_providers, int node_flags);
HRESULT uia_node_from_lresult(LRESULT lr, HUIANODE *huianode, int node_flags);
void uia_node_lresult_release(LRESULT lr);
HRESULT create_uia_node_from_hwnd(HWND hwnd, HUIANODE *out_node, int node_flags);
HRESULT uia_condition_check(HUIANODE node, struct UiaCondition *condition);
BOOL uia_condition_matched(HRESULT hr);

/* uia_com_client.c */
HRESULT uia_com_win_event_callback(DWORD event_id, HWND hwnd, LONG obj_id, LONG child_id, DWORD thread_id,
        DWORD event_time);
HRESULT create_uia_iface(IUnknown **iface, BOOL is_cui8);

/* uia_event.c */
HRESULT uia_event_add_win_event_hwnd(struct uia_event *event, HWND hwnd);
HRESULT uia_event_for_each(int event_id, UiaWineEventForEachCallback *callback, void *user_data,
        BOOL clientside_only);
BOOL uia_clientside_event_start_event_thread(struct uia_event *event);
HRESULT create_msaa_provider_from_hwnd(HWND hwnd, int in_child_id, IRawElementProviderSimple **ret_elprov);
HRESULT create_serverside_uia_event(struct uia_event **out_event, LONG process_id, LONG event_cookie);
HRESULT uia_event_add_provider_event_adviser(IRawElementProviderAdviseEvents *advise_events,
        struct uia_event *event);
HRESULT uia_event_add_serverside_event_adviser(IWineUiaEvent *serverside_event, struct uia_event *event);
HRESULT uia_event_advise_node(struct uia_event *event, HUIANODE node);
HRESULT uia_add_clientside_event(HUIANODE huianode, EVENTID event_id, enum TreeScope scope, PROPERTYID *prop_ids,
        int prop_ids_count, struct UiaCacheRequest *cache_req, SAFEARRAY *rt_id, UiaWineEventCallback *cback,
        void *cback_data, HUIAEVENT *huiaevent);
HRESULT uia_event_invoke(HUIANODE node, HUIANODE nav_start_node, struct uia_event_args *args,
        struct uia_event *event);
HRESULT uia_event_check_node_within_event_scope(struct uia_event *event, HUIANODE node, SAFEARRAY *rt_id,
        HUIANODE *clientside_nav_node_out);

/* uia_ids.c */
const struct uia_prop_info *uia_prop_info_from_id(PROPERTYID prop_id);
const struct uia_event_info *uia_event_info_from_id(EVENTID event_id);
const struct uia_pattern_info *uia_pattern_info_from_id(PATTERNID pattern_id);
const struct uia_control_type_info *uia_control_type_info_from_id(CONTROLTYPEID control_type_id);

/* uia_provider.c */
HRESULT create_base_hwnd_provider(HWND hwnd, IRawElementProviderSimple **elprov);
void uia_stop_provider_thread(void);
void uia_provider_thread_remove_node(HUIANODE node);
LRESULT uia_lresult_from_node(HUIANODE huianode);
HRESULT create_msaa_provider(IAccessible *acc, LONG child_id, HWND hwnd, BOOL root_acc_known,
        BOOL is_root_acc, IRawElementProviderSimple **elprov);

/* uia_utils.c */
HRESULT register_interface_in_git(IUnknown *iface, REFIID riid, DWORD *ret_cookie);
HRESULT unregister_interface_in_git(DWORD git_cookie);
HRESULT get_interface_in_git(REFIID riid, DWORD git_cookie, IUnknown **ret_iface);
HRESULT write_runtime_id_base(SAFEARRAY *sa, HWND hwnd);
void uia_cache_request_destroy(struct UiaCacheRequest *cache_req);
HRESULT uia_cache_request_clone(struct UiaCacheRequest *dst, struct UiaCacheRequest *src);
HRESULT get_safearray_dim_bounds(SAFEARRAY *sa, UINT dim, LONG *lbound, LONG *elems);
HRESULT get_safearray_bounds(SAFEARRAY *sa, LONG *lbound, LONG *elems);
int uia_compare_safearrays(SAFEARRAY *sa1, SAFEARRAY *sa2, int prop_type);
BOOL uia_hwnd_is_visible(HWND hwnd);
BOOL uia_is_top_level_hwnd(HWND hwnd);
BOOL uia_hwnd_map_check_hwnd(struct rb_tree *hwnd_map, HWND hwnd);
HRESULT uia_hwnd_map_add_hwnd(struct rb_tree *hwnd_map, HWND hwnd);
void uia_hwnd_map_remove_hwnd(struct rb_tree *hwnd_map, HWND hwnd);
void uia_hwnd_map_init(struct rb_tree *hwnd_map);
void uia_hwnd_map_destroy(struct rb_tree *hwnd_map);
