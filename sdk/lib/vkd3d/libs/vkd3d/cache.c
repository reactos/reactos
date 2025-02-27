/*
 * Copyright 2024 Stefan Dösinger for CodeWeavers
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

#include "vkd3d_private.h"

struct vkd3d_cache_entry_header
{
    uint64_t hash;
    uint64_t key_size;
    uint64_t value_size;
};

struct vkd3d_shader_cache
{
    unsigned int refcount;
    struct vkd3d_mutex lock;

    struct rb_tree tree;
};

struct shader_cache_entry
{
    struct vkd3d_cache_entry_header h;
    struct rb_entry entry;
    uint8_t *payload;
};

struct shader_cache_key
{
    uint64_t hash;
    const void *key;
    uint64_t key_size;
};

static int vkd3d_shader_cache_compare_key(const void *key, const struct rb_entry *entry)
{
    const struct shader_cache_entry *e = RB_ENTRY_VALUE(entry, struct shader_cache_entry, entry);
    const struct shader_cache_key *k = key;
    int ret;

    if ((ret = vkd3d_u64_compare(k->hash, e->h.hash)))
        return ret;
    if ((ret = vkd3d_u64_compare(k->key_size, e->h.key_size)))
        return ret;

    /* Until now we have not seen an actual hash collision. If the key didn't match it was always
     * due to a bug in the serialization code or memory corruption. If you see this FIXME please
     * investigate. */
    if ((ret = memcmp(k->key, e->payload, k->key_size)))
        FIXME("Actual case of a hash collision found.\n");
    return ret;
}

static void vkd3d_shader_cache_add_entry(struct vkd3d_shader_cache *cache,
        struct shader_cache_entry *e)
{
    const struct shader_cache_key k =
    {
        .hash = e->h.hash,
        .key_size = e->h.key_size,
        .key = e->payload
    };

    rb_put(&cache->tree, &k, &e->entry);
}

int vkd3d_shader_open_cache(struct vkd3d_shader_cache **cache)
{
    struct vkd3d_shader_cache *object;

    TRACE("%p.\n", cache);

    object = vkd3d_malloc(sizeof(*object));
    if (!object)
        return VKD3D_ERROR_OUT_OF_MEMORY;

    object->refcount = 1;
    rb_init(&object->tree, vkd3d_shader_cache_compare_key);
    vkd3d_mutex_init(&object->lock);

    *cache = object;

    return VKD3D_OK;
}

unsigned int vkd3d_shader_cache_incref(struct vkd3d_shader_cache *cache)
{
    unsigned int refcount = vkd3d_atomic_increment_u32(&cache->refcount);
    TRACE("cache %p refcount %u.\n", cache, refcount);
    return refcount;
}

static void vkd3d_shader_cache_destroy_entry(struct rb_entry *entry, void *context)
{
    struct shader_cache_entry *e = RB_ENTRY_VALUE(entry, struct shader_cache_entry, entry);
    vkd3d_free(e->payload);
    vkd3d_free(e);
}

unsigned int vkd3d_shader_cache_decref(struct vkd3d_shader_cache *cache)
{
    unsigned int refcount = vkd3d_atomic_decrement_u32(&cache->refcount);
    TRACE("cache %p refcount %u.\n", cache, refcount);

    if (refcount)
        return refcount;

    rb_destroy(&cache->tree, vkd3d_shader_cache_destroy_entry, NULL);
    vkd3d_mutex_destroy(&cache->lock);

    vkd3d_free(cache);
    return 0;
}

static uint64_t vkd3d_shader_cache_hash_key(const void *key, size_t size)
{
    static const uint64_t fnv_prime = 0x00000100000001b3;
    uint64_t hash = 0xcbf29ce484222325;
    const uint8_t *k = key;
    size_t i;

    for (i = 0; i < size; ++i)
        hash = (hash ^ k[i]) * fnv_prime;

    return hash;
}

static void vkd3d_shader_cache_lock(struct vkd3d_shader_cache *cache)
{
    vkd3d_mutex_lock(&cache->lock);
}

static void vkd3d_shader_cache_unlock(struct vkd3d_shader_cache *cache)
{
    vkd3d_mutex_unlock(&cache->lock);
}

int vkd3d_shader_cache_put(struct vkd3d_shader_cache *cache,
        const void *key, size_t key_size, const void *value, size_t value_size)
{
    struct shader_cache_entry *e;
    struct shader_cache_key k;
    struct rb_entry *entry;
    enum vkd3d_result ret;

    TRACE("%p, %p, %#zx, %p, %#zx.\n", cache, key, key_size, value, value_size);

    k.hash = vkd3d_shader_cache_hash_key(key, key_size);
    k.key = key;
    k.key_size = key_size;

    vkd3d_shader_cache_lock(cache);

    entry = rb_get(&cache->tree, &k);
    e = entry ? RB_ENTRY_VALUE(entry, struct shader_cache_entry, entry) : NULL;

    if (e)
    {
        WARN("Key already exists, returning VKD3D_ERROR_KEY_ALREADY_EXISTS.\n");
        ret = VKD3D_ERROR_KEY_ALREADY_EXISTS;
        goto done;
    }

    e = vkd3d_malloc(sizeof(*e));
    if (!e)
    {
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
        goto done;
    }
    e->payload = vkd3d_malloc(key_size + value_size);
    if (!e->payload)
    {
        vkd3d_free(e);
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    e->h.key_size = key_size;
    e->h.value_size = value_size;
    e->h.hash = k.hash;
    memcpy(e->payload, key, key_size);
    memcpy(e->payload + key_size, value, value_size);

    vkd3d_shader_cache_add_entry(cache, e);
    TRACE("Cache entry %#"PRIx64" stored.\n", k.hash);
    ret = VKD3D_OK;

done:
    vkd3d_shader_cache_unlock(cache);
    return ret;
}

int vkd3d_shader_cache_get(struct vkd3d_shader_cache *cache,
        const void *key, size_t key_size, void *value, size_t *value_size)
{
    struct shader_cache_entry *e;
    struct shader_cache_key k;
    struct rb_entry *entry;
    enum vkd3d_result ret;
    size_t size_in;

    TRACE("%p, %p, %#zx, %p, %p.\n", cache, key, key_size, value, value_size);

    size_in = *value_size;

    k.hash = vkd3d_shader_cache_hash_key(key, key_size);
    k.key = key;
    k.key_size = key_size;

    vkd3d_shader_cache_lock(cache);

    entry = rb_get(&cache->tree, &k);
    if (!entry)
    {
        WARN("Entry not found.\n");
        ret = VKD3D_ERROR_NOT_FOUND;
        goto done;
    }

    e = RB_ENTRY_VALUE(entry, struct shader_cache_entry, entry);

    *value_size = e->h.value_size;
    if (!value)
    {
        TRACE("Found item %#"PRIx64", returning needed size %#"PRIx64".\n",
                e->h.hash, e->h.value_size);
        ret = VKD3D_OK;
        goto done;
    }

    if (size_in < e->h.value_size)
    {
        WARN("Output buffer is too small for item %#"PRIx64", got %#zx want %#"PRIx64".\n",
                e->h.hash, size_in, e->h.value_size);
        ret = VKD3D_ERROR_MORE_DATA;
        goto done;
    }

    memcpy(value, e->payload + e->h.key_size, e->h.value_size);
    ret = VKD3D_OK;
    TRACE("Returning cached item %#"PRIx64".\n", e->h.hash);

done:
    vkd3d_shader_cache_unlock(cache);
    return ret;
}
