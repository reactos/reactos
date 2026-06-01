/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#ifdef __REACTOS__
#include <wine/config.h>
#include <wine/port.h>
#endif

#include <assert.h>
#include <math.h>

#include "jscript.h"

#include "wine/rbtree.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;
    struct wine_rb_tree map;
    struct list entries;
    size_t size;
} MapInstance;

struct jsval_map_entry {
    struct wine_rb_entry entry;
    jsval_t key;
    jsval_t value;

    /*
     * We need to maintain a list as well to support traversal in forEach.
     * If the entry is removed while being processed by forEach, it's
     * still kept in the list and released later, when it's safe.
     */
    struct list list_entry;
    unsigned int ref;
    BOOL deleted;
};

static int jsval_map_compare(const void *k, const struct wine_rb_entry *e)
{
    const struct jsval_map_entry *entry = WINE_RB_ENTRY_VALUE(e, const struct jsval_map_entry, entry);
    const jsval_t *key = k;
    union {
        double d;
        INT64 n;
    } bits1, bits2;

    if(jsval_type(entry->key) != jsval_type(*key))
        return (int)jsval_type(entry->key) - (int)jsval_type(*key);

    switch(jsval_type(*key)) {
    case JSV_UNDEFINED:
    case JSV_NULL:
        return 0;
    case JSV_OBJECT:
        if(get_object(*key) == get_object(entry->key)) return 0;
        return get_object(*key) < get_object(entry->key) ? -1 : 1;
    case JSV_STRING:
        return jsstr_cmp(get_string(*key), get_string(entry->key));
    case JSV_NUMBER:
        if(isnan(get_number(*key))) return isnan(get_number(entry->key)) ? 0 : -1;
        if(isnan(get_number(entry->key))) return 1;

        /* native treats -0 differently than 0, so need to compare bitwise */
        bits1.d = get_number(*key);
        bits2.d = get_number(entry->key);
        return (bits1.n == bits2.n) ? 0 : (bits1.n < bits2.n ? -1 : 1);
    case JSV_BOOL:
        if(get_bool(*key) == get_bool(entry->key)) return 0;
        return get_bool(*key) ? 1 : -1;
    default:
        assert(0);
        return 0;
    }
}

static HRESULT get_map_this(script_ctx_t *ctx, jsval_t vthis, MapInstance **ret)
{
    jsdisp_t *jsdisp;

    if(!is_object_instance(vthis))
        return JS_E_OBJECT_EXPECTED;
    if(!(jsdisp = to_jsdisp(get_object(vthis))) || !is_class(jsdisp, JSCLASS_MAP)) {
        WARN("not a Map object passed as 'this'\n");
        return throw_error(ctx, JS_E_WRONG_THIS, L"Map");
    }

    *ret = CONTAINING_RECORD(jsdisp, MapInstance, dispex);
    return S_OK;
}

static HRESULT get_set_this(script_ctx_t *ctx, jsval_t vthis, MapInstance **ret)
{
    jsdisp_t *jsdisp;

    if(!is_object_instance(vthis))
        return JS_E_OBJECT_EXPECTED;
    if(!(jsdisp = to_jsdisp(get_object(vthis))) || !is_class(jsdisp, JSCLASS_SET)) {
        WARN("not a Set object passed as 'this'\n");
        return throw_error(ctx, JS_E_WRONG_THIS, L"Set");
    }

    *ret = CONTAINING_RECORD(jsdisp, MapInstance, dispex);
    return S_OK;
}

static struct jsval_map_entry *get_map_entry(MapInstance *map, jsval_t key)
{
    struct wine_rb_entry *entry;
    if(!(entry = wine_rb_get(&map->map, &key))) return NULL;
    return CONTAINING_RECORD(entry, struct jsval_map_entry, entry);
}

static void grab_map_entry(struct jsval_map_entry *entry)
{
    entry->ref++;
}

static void release_map_entry(struct jsval_map_entry *entry)
{
    if(--entry->ref) return;
    jsval_release(entry->key);
    jsval_release(entry->value);
    list_remove(&entry->list_entry);
    free(entry);
}

static void delete_map_entry(MapInstance *map, struct jsval_map_entry *entry)
{
    map->size--;
    wine_rb_remove(&map->map, &entry->entry);
    entry->deleted = TRUE;
    release_map_entry(entry);
}

static HRESULT set_map_entry(MapInstance *map, jsval_t key, jsval_t value, jsval_t *r)
{
    struct jsval_map_entry *entry;
    HRESULT hres;

    if((entry = get_map_entry(map, key))) {
        jsval_t val;
        hres = jsval_copy(value, &val);
        if(FAILED(hres))
            return hres;

        jsval_release(entry->value);
        entry->value = val;
    }else {
        if(!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;

        hres = jsval_copy(key, &entry->key);
        if(SUCCEEDED(hres)) {
            hres = jsval_copy(value, &entry->value);
            if(FAILED(hres))
                jsval_release(entry->key);
        }
        if(FAILED(hres)) {
            free(entry);
            return hres;
        }
        grab_map_entry(entry);
        wine_rb_put(&map->map, &entry->key, &entry->entry);
        list_add_tail(&map->entries, &entry->list_entry);
        map->size++;
    }

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT iterate_map(MapInstance *map, script_ctx_t *ctx, unsigned argc, jsval_t *argv, jsval_t *r)
{
    struct list *iter = list_head(&map->entries);
    jsval_t context_this = jsval_undefined();
    HRESULT hres;

    if(!argc || !is_object_instance(argv[0])) {
        FIXME("invalid callback %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        return E_FAIL;
    }

    if(argc > 1)
        context_this = argv[1];

    while(iter) {
        struct jsval_map_entry *entry = LIST_ENTRY(iter, struct jsval_map_entry, list_entry);
        jsval_t args[3], v;

        if(entry->deleted) {
            iter = list_next(&map->entries, iter);
            continue;
        }

        args[0] = entry->value;
        args[1] = entry->key;
        args[2] = jsval_obj(&map->dispex);
        grab_map_entry(entry);
        hres = disp_call_value(ctx, get_object(argv[0]), context_this, DISPATCH_METHOD, ARRAY_SIZE(args), args, &v);
        iter = list_next(&map->entries, iter);
        release_map_entry(entry);
        if(FAILED(hres))
            return hres;
        jsval_release(v);
    }

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT Map_clear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    struct jsval_map_entry *entry, *entry2;
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p\n", map);

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &map->entries, struct jsval_map_entry, list_entry)
        delete_map_entry(map, entry);

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT Map_delete(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc >= 1 ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", map, debugstr_jsval(key));

    if((entry = get_map_entry(map, key))) delete_map_entry(map, entry);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT Map_forEach(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", map, debugstr_jsval(argc >= 1 ? argv[0] : jsval_undefined()));

    return iterate_map(map, ctx, argc, argv, r);
}

static HRESULT Map_get(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc >= 1 ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", map, debugstr_jsval(key));

    if(!(entry = get_map_entry(map, key))) {
        if(r) *r = jsval_undefined();
        return S_OK;
    }

    return r ? jsval_copy(entry->value, r) : S_OK;
}

static HRESULT Map_set(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc >= 1 ? argv[0] : jsval_undefined();
    jsval_t value = argc >= 2 ? argv[1] : jsval_undefined();
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s %s)\n", map, debugstr_jsval(key), debugstr_jsval(value));

    return set_map_entry(map, key, value, r);
}

static HRESULT Map_has(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc >= 1 ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", map, debugstr_jsval(key));

    entry = get_map_entry(map, key);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT Map_get_size(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    MapInstance *map = (MapInstance*)jsthis;

    TRACE("%p\n", map);

    *r = jsval_number(map->size);
    return S_OK;
}

static HRESULT Map_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static void Map_destructor(jsdisp_t *dispex)
{
    MapInstance *map = (MapInstance*)dispex;

    while(!list_empty(&map->entries)) {
        struct jsval_map_entry *entry = LIST_ENTRY(list_head(&map->entries),
                                                   struct jsval_map_entry, list_entry);
        assert(!entry->deleted);
        release_map_entry(entry);
    }
}

static HRESULT Map_gc_traverse(struct gc_ctx *gc_ctx, enum gc_traverse_op op, jsdisp_t *dispex)
{
    MapInstance *map = (MapInstance*)dispex;
    struct jsval_map_entry *entry, *entry2;
    HRESULT hres;

    if(op == GC_TRAVERSE_UNLINK) {
        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &map->entries, struct jsval_map_entry, list_entry)
            release_map_entry(entry);
        wine_rb_destroy(&map->map, NULL, NULL);
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(entry, &map->entries, struct jsval_map_entry, list_entry) {
        hres = gc_process_linked_val(gc_ctx, op, dispex, &entry->key);
        if(FAILED(hres))
            return hres;
        hres = gc_process_linked_val(gc_ctx, op, dispex, &entry->value);
        if(FAILED(hres))
            return hres;
    }
    return S_OK;
}

static const builtin_prop_t Map_prototype_props[] = {
    {L"clear",      Map_clear,     PROPF_METHOD},
    {L"delete" ,    Map_delete,    PROPF_METHOD|1},
    {L"forEach",    Map_forEach,   PROPF_METHOD|1},
    {L"get",        Map_get,       PROPF_METHOD|1},
    {L"has",        Map_has,       PROPF_METHOD|1},
    {L"set",        Map_set,       PROPF_METHOD|2},
};

static const builtin_prop_t Map_props[] = {
    {L"size",       NULL,0,        Map_get_size, builtin_set_const},
};

static const builtin_info_t Map_prototype_info = {
    .class     = JSCLASS_OBJECT,
    .call      = Map_value,
    .props_cnt = ARRAY_SIZE(Map_prototype_props),
    .props     = Map_prototype_props,
};

static const builtin_info_t Map_info = {
    .class       = JSCLASS_MAP,
    .call        = Map_value,
    .props_cnt   = ARRAY_SIZE(Map_props),
    .props       = Map_props,
    .destructor  = Map_destructor,
    .gc_traverse = Map_gc_traverse,
};

static HRESULT Map_constructor(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    MapInstance *map;
    HRESULT hres;

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        TRACE("\n");

        if(!r)
            return S_OK;
        if(!(map = calloc(1, sizeof(*map))))
            return E_OUTOFMEMORY;

        hres = init_dispex(&map->dispex, ctx, &Map_info, ctx->map_prototype);
        if(FAILED(hres))
            return hres;

        wine_rb_init(&map->map, jsval_map_compare);
        list_init(&map->entries);
        *r = jsval_obj(&map->dispex);
        return S_OK;

    case DISPATCH_METHOD:
        return throw_error(ctx, JS_E_WRONG_THIS, L"Map");

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

static HRESULT Set_add(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc ? argv[0] : jsval_undefined();
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", set, debugstr_jsval(key));

    return set_map_entry(set, key, key, r);
}

static HRESULT Set_clear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    struct jsval_map_entry *entry, *entry2;
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p\n", set);

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &set->entries, struct jsval_map_entry, list_entry)
        delete_map_entry(set, entry);

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT Set_delete(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", set, debugstr_jsval(key));

    if((entry = get_map_entry(set, key))) delete_map_entry(set, entry);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT Set_forEach(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", set, debugstr_jsval(argc ? argv[0] : jsval_undefined()));

    return iterate_map(set, ctx, argc, argv, r);
}

static HRESULT Set_has(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", set, debugstr_jsval(key));

    entry = get_map_entry(set, key);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT Set_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t Set_prototype_props[] = {
    {L"add",        Set_add,       PROPF_METHOD|1},
    {L"clear",      Set_clear,     PROPF_METHOD},
    {L"delete" ,    Set_delete,    PROPF_METHOD|1},
    {L"forEach",    Set_forEach,   PROPF_METHOD|1},
    {L"has",        Set_has,       PROPF_METHOD|1},
};

static const builtin_info_t Set_prototype_info = {
    .class     = JSCLASS_OBJECT,
    .call      = Set_value,
    .props_cnt = ARRAY_SIZE(Set_prototype_props),
    .props     = Set_prototype_props,
};

static const builtin_info_t Set_info = {
    .class       = JSCLASS_SET,
    .call        = Set_value,
    .props_cnt   = ARRAY_SIZE(Map_props),
    .props       = Map_props,
    .destructor  = Map_destructor,
    .gc_traverse = Map_gc_traverse,
};

static HRESULT Set_constructor(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    MapInstance *set;
    HRESULT hres;

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        TRACE("\n");

        if(!r)
            return S_OK;
        if(!(set = calloc(1, sizeof(*set))))
            return E_OUTOFMEMORY;

        hres = init_dispex(&set->dispex, ctx, &Set_info, ctx->set_prototype);
        if(FAILED(hres))
            return hres;

        wine_rb_init(&set->map, jsval_map_compare);
        list_init(&set->entries);
        *r = jsval_obj(&set->dispex);
        return S_OK;

    case DISPATCH_METHOD:
        return throw_error(ctx, JS_E_WRONG_THIS, L"Set");

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

typedef struct {
    jsdisp_t dispex;
    struct rb_tree map;
} WeakMapInstance;

static int weakmap_compare(const void *k, const struct rb_entry *e)
{
    ULONG_PTR a = (ULONG_PTR)k, b = (ULONG_PTR)RB_ENTRY_VALUE(e, const struct weakmap_entry, entry)->key;
    return (a > b) - (a < b);
}

static HRESULT get_weakmap_this(script_ctx_t *ctx, jsval_t vthis, WeakMapInstance **ret)
{
    jsdisp_t *jsdisp;

    if(!is_object_instance(vthis))
        return JS_E_OBJECT_EXPECTED;
    if(!(jsdisp = to_jsdisp(get_object(vthis))) || !is_class(jsdisp, JSCLASS_WEAKMAP)) {
        WARN("not a WeakMap object passed as 'this'\n");
        throw_error(ctx, JS_E_WRONG_THIS, L"WeakMap");
        return DISP_E_EXCEPTION;
    }

    *ret = CONTAINING_RECORD(jsdisp, WeakMapInstance, dispex);
    return S_OK;
}

static struct weakmap_entry *get_weakmap_entry(WeakMapInstance *weakmap, jsdisp_t *key)
{
    struct rb_entry *entry;
    if(!(entry = rb_get(&weakmap->map, key))) return NULL;
    return CONTAINING_RECORD(entry, struct weakmap_entry, entry);
}

void remove_weakmap_entry(struct weakmap_entry *entry)
{
    WeakMapInstance *weakmap = (WeakMapInstance*)entry->weakmap;
    struct list *next = entry->weak_refs_entry.next;

    if(next->next != &entry->weak_refs_entry)
        list_remove(&entry->weak_refs_entry);
    else {
        struct weak_refs_entry *weak_refs_entry = LIST_ENTRY(next, struct weak_refs_entry, list);
        entry->key->has_weak_refs = FALSE;
        rb_remove(&entry->key->ctx->thread_data->weak_refs, &weak_refs_entry->entry);
        free(weak_refs_entry);
    }
    rb_remove(&weakmap->map, &entry->entry);
    jsval_release(entry->value);
    free(entry);
}

static HRESULT WeakMap_clear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    WeakMapInstance *weakmap;
    HRESULT hres;

    hres = get_weakmap_this(ctx, vthis, &weakmap);
    if(FAILED(hres))
        return hres;

    TRACE("%p\n", weakmap);

    while(weakmap->map.root)
        remove_weakmap_entry(RB_ENTRY_VALUE(weakmap->map.root, struct weakmap_entry, entry));

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT WeakMap_delete(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *key = (argc >= 1 && is_object_instance(argv[0])) ? to_jsdisp(get_object(argv[0])) : NULL;
    struct weakmap_entry *entry;
    WeakMapInstance *weakmap;
    HRESULT hres;

    hres = get_weakmap_this(ctx, vthis, &weakmap);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%p)\n", weakmap, key);

    if((entry = get_weakmap_entry(weakmap, key)))
        remove_weakmap_entry(entry);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT WeakMap_get(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *key = (argc >= 1 && is_object_instance(argv[0])) ? to_jsdisp(get_object(argv[0])) : NULL;
    struct weakmap_entry *entry;
    WeakMapInstance *weakmap;
    HRESULT hres;

    hres = get_weakmap_this(ctx, vthis, &weakmap);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%p)\n", weakmap, key);

    if(!(entry = get_weakmap_entry(weakmap, key))) {
        if(r) *r = jsval_undefined();
        return S_OK;
    }

    return r ? jsval_copy(entry->value, r) : S_OK;
}

static HRESULT WeakMap_set(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *key = (argc >= 1 && is_object_instance(argv[0])) ? to_jsdisp(get_object(argv[0])) : NULL;
    jsval_t value = argc >= 2 ? argv[1] : jsval_undefined();
    struct weakmap_entry *entry;
    WeakMapInstance *weakmap;
    HRESULT hres;

    hres = get_weakmap_this(ctx, vthis, &weakmap);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%p %s)\n", weakmap, key, debugstr_jsval(value));

    if(!key)
        return JS_E_KEY_NOT_OBJECT;

    if((entry = get_weakmap_entry(weakmap, key))) {
        jsval_t val;
        hres = jsval_copy(value, &val);
        if(FAILED(hres))
            return hres;

        jsval_release(entry->value);
        entry->value = val;
    }else {
        struct weak_refs_entry *weak_refs_entry;

        if(!(entry = malloc(sizeof(*entry))))
            return E_OUTOFMEMORY;

        hres = jsval_copy(value, &entry->value);
        if(FAILED(hres)) {
            free(entry);
            return hres;
        }

        if(key->has_weak_refs)
            weak_refs_entry = RB_ENTRY_VALUE(rb_get(&ctx->thread_data->weak_refs, key), struct weak_refs_entry, entry);
        else {
            if(!(weak_refs_entry = malloc(sizeof(*weak_refs_entry)))) {
                jsval_release(entry->value);
                free(entry);
                return E_OUTOFMEMORY;
            }
            rb_put(&ctx->thread_data->weak_refs, key, &weak_refs_entry->entry);
            list_init(&weak_refs_entry->list);
            key->has_weak_refs = TRUE;
        }
        list_add_tail(&weak_refs_entry->list, &entry->weak_refs_entry);

        entry->key = key;
        entry->weakmap = &weakmap->dispex;
        rb_put(&weakmap->map, key, &entry->entry);
    }

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT WeakMap_has(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *key = (argc >= 1 && is_object_instance(argv[0])) ? to_jsdisp(get_object(argv[0])) : NULL;
    WeakMapInstance *weakmap;
    HRESULT hres;

    hres = get_weakmap_this(ctx, vthis, &weakmap);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%p)\n", weakmap, key);

    if(r) *r = jsval_bool(!!get_weakmap_entry(weakmap, key));
    return S_OK;
}

static HRESULT WeakMap_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static void WeakMap_destructor(jsdisp_t *dispex)
{
    WeakMapInstance *weakmap = (WeakMapInstance*)dispex;

    while(weakmap->map.root)
        remove_weakmap_entry(RB_ENTRY_VALUE(weakmap->map.root, struct weakmap_entry, entry));
}

static HRESULT WeakMap_gc_traverse(struct gc_ctx *gc_ctx, enum gc_traverse_op op, jsdisp_t *dispex)
{
    WeakMapInstance *weakmap = (WeakMapInstance*)dispex;
    struct weakmap_entry *entry;
    HRESULT hres;

    if(op == GC_TRAVERSE_UNLINK) {
        while(weakmap->map.root)
            remove_weakmap_entry(RB_ENTRY_VALUE(weakmap->map.root, struct weakmap_entry, entry));
        return S_OK;
    }

    RB_FOR_EACH_ENTRY(entry, &weakmap->map, struct weakmap_entry, entry) {
        /* Only traverse the values if the key turned out to be alive, which means it might not have traversed
           the associated values with it from this WeakMap yet (because it wasn't considered alive back then).
           We need both the key and the WeakMap for the entry to actually be accessible (and thus traversed). */
        if(op == GC_TRAVERSE && entry->key->gc_marked)
            continue;

        hres = gc_process_linked_val(gc_ctx, op, dispex, &entry->value);
        if(FAILED(hres))
            return hres;
    }
    return S_OK;
}

static const builtin_prop_t WeakMap_prototype_props[] = {
    {L"clear",      WeakMap_clear,     PROPF_METHOD},
    {L"delete",     WeakMap_delete,    PROPF_METHOD|1},
    {L"get",        WeakMap_get,       PROPF_METHOD|1},
    {L"has",        WeakMap_has,       PROPF_METHOD|1},
    {L"set",        WeakMap_set,       PROPF_METHOD|2},
};

static const builtin_info_t WeakMap_prototype_info = {
    .class     = JSCLASS_OBJECT,
    .call      = WeakMap_value,
    .props_cnt = ARRAY_SIZE(WeakMap_prototype_props),
    .props     = WeakMap_prototype_props,
};

static const builtin_info_t WeakMap_info = {
    .class       = JSCLASS_WEAKMAP,
    .call        = WeakMap_value,
    .destructor  = WeakMap_destructor,
    .gc_traverse = WeakMap_gc_traverse,
};

static HRESULT WeakMap_constructor(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    WeakMapInstance *weakmap;
    HRESULT hres;

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        TRACE("\n");

        if(!r)
            return S_OK;
        if(!(weakmap = calloc(1, sizeof(*weakmap))))
            return E_OUTOFMEMORY;

        hres = init_dispex(&weakmap->dispex, ctx, &WeakMap_info, ctx->weakmap_prototype);
        if(FAILED(hres))
            return hres;

        rb_init(&weakmap->map, weakmap_compare);
        *r = jsval_obj(&weakmap->dispex);
        return S_OK;

    case DISPATCH_METHOD:
        return throw_error(ctx, JS_E_WRONG_THIS, L"WeakMap");

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

HRESULT init_set_constructor(script_ctx_t *ctx)
{
    jsdisp_t *constructor;
    HRESULT hres;

    if(ctx->version < SCRIPTLANGUAGEVERSION_ES6)
        return S_OK;

    hres = create_dispex(ctx, &Set_prototype_info, ctx->object_prototype, &ctx->set_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, Set_constructor, L"Set", NULL,
                                      PROPF_CONSTR, ctx->set_prototype, &constructor);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Set", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(constructor));
    jsdisp_release(constructor);
    if(FAILED(hres))
        return hres;

    hres = create_dispex(ctx, &Map_prototype_info, ctx->object_prototype, &ctx->map_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, Map_constructor, L"Map", NULL,
                                      PROPF_CONSTR, ctx->map_prototype, &constructor);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Map", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(constructor));
    jsdisp_release(constructor);
    if(FAILED(hres))
        return hres;

    hres = create_dispex(ctx, &WeakMap_prototype_info, ctx->object_prototype, &ctx->weakmap_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, WeakMap_constructor, L"WeakMap", NULL,
                                      PROPF_CONSTR, ctx->weakmap_prototype, &constructor);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"WeakMap", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(constructor));
    jsdisp_release(constructor);
    return hres;
}
