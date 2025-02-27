/*
 *    Glyph shaping support
 *
 * Copyright 2010 Aric Stewart for CodeWeavers
 * Copyright 2014 Nikolay Sivov for CodeWeavers
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
 *
 *
 * Shaping features processing, default features, and lookups handling logic are
 * derived from HarfBuzz implementation. Consult project documentation for the full
 * list of its authors.
 */

#define COBJMACROS

#include "dwrite_private.h"
#include "scripts.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#ifdef WORDS_BIGENDIAN
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

struct scriptshaping_cache *create_scriptshaping_cache(void *context, const struct shaping_font_ops *font_ops)
{
    struct scriptshaping_cache *cache;

    if (!(cache = calloc(1, sizeof(*cache))))
        return NULL;

    cache->font = font_ops;
    cache->context = context;

    opentype_layout_scriptshaping_cache_init(cache);
    cache->upem = cache->font->get_font_upem(cache->context);

    return cache;
}

void release_scriptshaping_cache(struct scriptshaping_cache *cache)
{
    if (!cache)
        return;

    cache->font->release_font_table(cache->context, cache->gdef.table.context);
    cache->font->release_font_table(cache->context, cache->gsub.table.context);
    cache->font->release_font_table(cache->context, cache->gpos.table.context);
    free(cache);
}

static unsigned int shape_select_script(const struct scriptshaping_cache *cache, DWORD kind, const unsigned int *scripts,
        unsigned int *script_index)
{
    static const unsigned int fallback_scripts[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('D','F','L','T'),
        DWRITE_MAKE_OPENTYPE_TAG('d','f','l','t'),
        DWRITE_MAKE_OPENTYPE_TAG('l','a','t','n'),
        0,
    };
    unsigned int script;

    /* Passed scripts in ascending priority. */
    while (scripts && *scripts)
    {
        if ((script = opentype_layout_find_script(cache, kind, *scripts, script_index)))
            return script;

        scripts++;
    }

    /* 'DFLT' -> 'dflt' -> 'latn' */
    scripts = fallback_scripts;
    while (*scripts)
    {
        if ((script = opentype_layout_find_script(cache, kind, *scripts, script_index)))
            return script;
        scripts++;
    }

    return 0;
}

static DWORD shape_select_language(const struct scriptshaping_cache *cache, DWORD kind, unsigned int script_index,
        DWORD language, unsigned int *language_index)
{
    /* Specified language -> 'dflt'. */
    if ((language = opentype_layout_find_language(cache, kind, language, script_index, language_index)))
        return language;

    if ((language = opentype_layout_find_language(cache, kind, DWRITE_MAKE_OPENTYPE_TAG('d','f','l','t'),
            script_index, language_index)))
        return language;

    return 0;
}

void shape_add_feature_full(struct shaping_features *features, unsigned int tag, unsigned int flags, unsigned int value)
{
    unsigned int i = features->count;

    if (!dwrite_array_reserve((void **)&features->features, &features->capacity, features->count + 1,
            sizeof(*features->features)))
        return;

    features->features[i].tag = tag;
    features->features[i].flags = flags;
    features->features[i].max_value = value;
    features->features[i].default_value = flags & FEATURE_GLOBAL ? value : 0;
    features->features[i].stage = features->stage;
    features->count++;
}

static void shape_add_feature(struct shaping_features *features, unsigned int tag)
{
    shape_add_feature_full(features, tag, FEATURE_GLOBAL, 1);
}

void shape_enable_feature(struct shaping_features *features, unsigned int tag,
        unsigned int flags)
{
    shape_add_feature_full(features, tag, FEATURE_GLOBAL | flags, 1);
}

void shape_start_next_stage(struct shaping_features *features, stage_func func)
{
    features->stages[features->stage].func = func;
    features->stage++;
}

static int __cdecl features_sorting_compare(const void *a, const void *b)
{
    const struct shaping_feature *left = a, *right = b;
    return left->tag != right->tag ? (left->tag < right->tag ? -1 : 1) : 0;
};

static void shape_merge_features(struct scriptshaping_context *context, struct shaping_features *features)
{
    const DWRITE_TYPOGRAPHIC_FEATURES **user_features = context->user_features.features;
    unsigned int i, j;

    /* For now only consider global, enabled user features. */
    if (user_features && context->user_features.range_lengths)
    {
        unsigned int flags = context->user_features.range_count == 1 &&
                context->user_features.range_lengths[0] == context->length ? FEATURE_GLOBAL : 0;

        for (i = 0; i < context->user_features.range_count; ++i)
        {
            for (j = 0; j < user_features[i]->featureCount; ++j)
                shape_add_feature_full(features, user_features[i]->features[j].nameTag, flags,
                        user_features[i]->features[j].parameter);
        }
    }

    /* Sort and merge duplicates. */
    qsort(features->features, features->count, sizeof(*features->features), features_sorting_compare);

    for (i = 1, j = 0; i < features->count; ++i)
    {
        if (features->features[i].tag != features->features[j].tag)
            features->features[++j] = features->features[i];
        else
        {
            if (features->features[i].flags & FEATURE_GLOBAL)
            {
                features->features[j].flags |= FEATURE_GLOBAL;
                features->features[j].max_value = features->features[i].max_value;
                features->features[j].default_value = features->features[i].default_value;
            }
            else
            {
                if (features->features[j].flags & FEATURE_GLOBAL)
                    features->features[j].flags ^= FEATURE_GLOBAL;
                features->features[j].max_value = max(features->features[j].max_value, features->features[i].max_value);
            }
            features->features[j].flags |= features->features[i].flags & FEATURE_HAS_FALLBACK;
            features->features[j].stage = min(features->features[j].stage, features->features[i].stage);
        }
    }
    features->count = j + 1;
}

static void default_shaper_setup_masks(struct scriptshaping_context *context,
        const struct shaping_features *features)
{
    unsigned int i;

    for (i = 0; i < context->glyph_count; ++i)
    {
        context->u.buffer.glyph_props[i].justification = iswspace(context->glyph_infos[i].codepoint) ?
                SCRIPT_JUSTIFY_BLANK : SCRIPT_JUSTIFY_CHARACTER;
    }
}

static const struct shaper default_shaper =
{
    .setup_masks = default_shaper_setup_masks
};

static void shape_set_shaper(struct scriptshaping_context *context)
{
    switch (context->script)
    {
        case Script_Arabic:
        case Script_Syriac:
            context->shaper = &arabic_shaper;
            break;
        default:
            context->shaper = &default_shaper;
    }
}

HRESULT shape_get_positions(struct scriptshaping_context *context, const unsigned int *scripts)
{
    static const struct shaping_feature common_features[] =
    {
        { DWRITE_MAKE_OPENTYPE_TAG('a','b','v','m') },
        { DWRITE_MAKE_OPENTYPE_TAG('b','l','w','m') },
        { DWRITE_MAKE_OPENTYPE_TAG('m','a','r','k'), FEATURE_MANUAL_JOINERS },
        { DWRITE_MAKE_OPENTYPE_TAG('m','k','m','k'), FEATURE_MANUAL_JOINERS },
    };
    static const unsigned int horizontal_features[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('c','u','r','s'),
        DWRITE_MAKE_OPENTYPE_TAG('d','i','s','t'),
        DWRITE_MAKE_OPENTYPE_TAG('k','e','r','n'),
    };
    struct scriptshaping_cache *cache = context->cache;
    unsigned int script_index, language_index, script, i;
    struct shaping_features features = { 0 };

    shape_set_shaper(context);

    for (i = 0; i < ARRAY_SIZE(common_features); ++i)
        shape_add_feature_full(&features, common_features[i].tag, FEATURE_GLOBAL | common_features[i].flags, 1);

    /* Horizontal features */
    if (!context->is_sideways)
    {
        for (i = 0; i < ARRAY_SIZE(horizontal_features); ++i)
            shape_add_feature(&features, horizontal_features[i]);
    }

    shape_merge_features(context, &features);

    /* Resolve script tag to actually supported script. */
    if (cache->gpos.table.data)
    {
        if ((script = shape_select_script(cache, MS_GPOS_TAG, scripts, &script_index)))
        {
            DWORD language = context->language_tag;

            if ((language = shape_select_language(cache, MS_GPOS_TAG, script_index, language, &language_index)))
            {
                TRACE("script %s, language %s.\n", debugstr_fourcc(script), language != ~0u ?
                        debugstr_fourcc(language) : "deflangsys");
                opentype_layout_apply_gpos_features(context, script_index, language_index, &features);
            }
        }
    }

    for (i = 0; i < context->glyph_count; ++i)
        if (context->u.pos.glyph_props[i].isZeroWidthSpace)
            context->advances[i] = 0.0f;

    free(features.features);

    return S_OK;
}

static unsigned int shape_get_script_lang_index(struct scriptshaping_context *context, const unsigned int *scripts,
        unsigned int table, unsigned int *script_index, unsigned int *language_index)
{
    unsigned int script;

    /* Resolve script tag to actually supported script. */
    if ((script = shape_select_script(context->cache, table, scripts, script_index)))
    {
        unsigned int language = context->language_tag;

        if ((language = shape_select_language(context->cache, table, *script_index, language, language_index)))
            return script;
    }

    return 0;
}

HRESULT shape_get_glyphs(struct scriptshaping_context *context, const unsigned int *scripts)
{
    static const unsigned int common_features[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('c','c','m','p'),
        DWRITE_MAKE_OPENTYPE_TAG('l','o','c','l'),
        DWRITE_MAKE_OPENTYPE_TAG('r','l','i','g'),
    };
    static const unsigned int horizontal_features[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('c','a','l','t'),
        DWRITE_MAKE_OPENTYPE_TAG('c','l','i','g'),
        DWRITE_MAKE_OPENTYPE_TAG('l','i','g','a'),
        DWRITE_MAKE_OPENTYPE_TAG('r','c','l','t'),
    };
    unsigned int script_index, language_index;
    struct shaping_features features = { 0 };
    unsigned int i;

    shape_set_shaper(context);

    if (!context->is_sideways)
    {
        if (context->is_rtl)
        {
            shape_enable_feature(&features, DWRITE_MAKE_OPENTYPE_TAG('r','t','l','a'), 0);
            shape_add_feature_full(&features, DWRITE_MAKE_OPENTYPE_TAG('r','t','l','m'), 0, 1);
        }
        else
        {
            shape_enable_feature(&features, DWRITE_MAKE_OPENTYPE_TAG('l','t','r','a'), 0);
            shape_enable_feature(&features, DWRITE_MAKE_OPENTYPE_TAG('l','t','r','m'), 0);
        }
    }

    if (context->shaper->collect_features)
        context->shaper->collect_features(context, &features);

    for (i = 0; i < ARRAY_SIZE(common_features); ++i)
        shape_add_feature(&features, common_features[i]);

    /* Horizontal features */
    if (!context->is_sideways)
    {
        for (i = 0; i < ARRAY_SIZE(horizontal_features); ++i)
            shape_add_feature(&features, horizontal_features[i]);
    }
    else
        shape_enable_feature(&features, DWRITE_MAKE_OPENTYPE_TAG('v','e','r','t'), FEATURE_GLOBAL_SEARCH);

    shape_merge_features(context, &features);

    /* Resolve script tag to actually supported script. */
    shape_get_script_lang_index(context, scripts, MS_GSUB_TAG, &script_index, &language_index);
    opentype_layout_apply_gsub_features(context, script_index, language_index, &features);

    free(features.features);

    return (context->glyph_count <= context->u.subst.max_glyph_count) ? S_OK : E_NOT_SUFFICIENT_BUFFER;
}

static int __cdecl tag_array_sorting_compare(const void *a, const void *b)
{
    unsigned int left = GET_BE_DWORD(*(unsigned int *)a), right = GET_BE_DWORD(*(unsigned int *)b);
    return left != right ? (left < right ? -1 : 1) : 0;
};

HRESULT shape_get_typographic_features(struct scriptshaping_context *context, const unsigned int *scripts,
        unsigned int max_tagcount, unsigned int *actual_tagcount, DWRITE_FONT_FEATURE_TAG *tags)
{
    unsigned int i, j, script_index, language_index;
    struct tag_array t = { 0 };

    /* Collect from both tables, sort and remove duplicates. */

    shape_get_script_lang_index(context, scripts, MS_GSUB_TAG, &script_index, &language_index);
    opentype_get_typographic_features(&context->cache->gsub, script_index, language_index, &t);

    shape_get_script_lang_index(context, scripts, MS_GPOS_TAG, &script_index, &language_index);
    opentype_get_typographic_features(&context->cache->gpos, script_index, language_index, &t);

    if (t.count == 0)
    {
        *actual_tagcount = 0;
        return S_OK;
    }

    /* Sort and remove duplicates. */
    qsort(t.tags, t.count, sizeof(*t.tags), tag_array_sorting_compare);

    for (i = 1, j = 0; i < t.count; ++i)
    {
        if (t.tags[i] != t.tags[j])
            t.tags[++j] = t.tags[i];
    }
    t.count = j + 1;

    if (t.count <= max_tagcount)
        memcpy(tags, t.tags, t.count * sizeof(*t.tags));

    *actual_tagcount = t.count;

    free(t.tags);

    return t.count <= max_tagcount ? S_OK : E_NOT_SUFFICIENT_BUFFER;
}

HRESULT shape_check_typographic_feature(struct scriptshaping_context *context, const unsigned int *scripts,
        unsigned int tag, unsigned int glyph_count, const UINT16 *glyphs, UINT8 *feature_applies)
{
    static const unsigned int tables[] = { MS_GSUB_TAG, MS_GPOS_TAG };
    struct shaping_feature feature = { .tag = tag };
    unsigned int script_index, language_index;
    unsigned int i;

    memset(feature_applies, 0, glyph_count * sizeof(*feature_applies));

    for (i = 0; i < ARRAY_SIZE(tables); ++i)
    {
        shape_get_script_lang_index(context, scripts, tables[i], &script_index, &language_index);
        context->table = tables[i] == MS_GSUB_TAG ? &context->cache->gsub : &context->cache->gpos;
        /* Skip second table if feature applies to all. */
        if (opentype_layout_check_feature(context, script_index, language_index, &feature, glyph_count,
                glyphs, feature_applies))
        {
            break;
        }
    }

    return S_OK;
}
