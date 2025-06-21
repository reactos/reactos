/*
 *    Font and collections
 *
 * Copyright 2011 Huw Davies
 * Copyright 2012, 2014-2022 Nikolay Sivov for CodeWeavers
 * Copyright 2014 Aric Stewart for CodeWeavers
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
#include <math.h>

#define COBJMACROS

#include "dwrite_private.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);
WINE_DECLARE_DEBUG_CHANNEL(dwrite_file);

#define MS_HEAD_TAG DWRITE_MAKE_OPENTYPE_TAG('h','e','a','d')
#define MS_OS2_TAG  DWRITE_MAKE_OPENTYPE_TAG('O','S','/','2')
#define MS_CMAP_TAG DWRITE_MAKE_OPENTYPE_TAG('c','m','a','p')
#define MS_NAME_TAG DWRITE_MAKE_OPENTYPE_TAG('n','a','m','e')
#define MS_VDMX_TAG DWRITE_MAKE_OPENTYPE_TAG('V','D','M','X')
#define MS_GASP_TAG DWRITE_MAKE_OPENTYPE_TAG('g','a','s','p')
#define MS_CPAL_TAG DWRITE_MAKE_OPENTYPE_TAG('C','P','A','L')
#define MS_COLR_TAG DWRITE_MAKE_OPENTYPE_TAG('C','O','L','R')

static const IID IID_issystemcollection = {0x14d88047,0x331f,0x4cd3,{0xbc,0xa8,0x3e,0x67,0x99,0xaf,0x34,0x75}};

static const FLOAT RECOMMENDED_OUTLINE_AA_THRESHOLD = 100.0f;
static const FLOAT RECOMMENDED_OUTLINE_A_THRESHOLD = 350.0f;
static const FLOAT RECOMMENDED_NATURAL_PPEM = 20.0f;

struct cache_key
{
    float size;
    unsigned short glyph;
    unsigned short mode;
};

struct cache_entry
{
    struct wine_rb_entry entry;
    struct list mru;
    struct cache_key key;
    int advance;
    RECT bbox;
    BYTE *bitmap;
    unsigned int bitmap_size;
    unsigned int is_1bpp : 1;
    unsigned int has_contours : 1;
    unsigned int has_advance : 1;
    unsigned int has_bbox : 1;
    unsigned int has_bitmap : 1;
};

/* Ignore dx and dy because FreeType doesn't actually use it */
static inline void matrix_2x2_from_dwrite_matrix(MATRIX_2X2 *m1, const DWRITE_MATRIX *m2)
{
    m1->m11 = m2->m11;
    m1->m12 = m2->m12;
    m1->m21 = m2->m21;
    m1->m22 = m2->m22;
}

static void fontface_release_cache_entry(struct cache_entry *entry)
{
    free(entry->bitmap);
    free(entry);
}

static struct cache_entry * fontface_get_cache_entry(struct dwrite_fontface *fontface, size_t size,
        const struct cache_key *key)
{
    struct cache_entry *entry, *old_entry;
    struct wine_rb_entry *e;

    if (!(e = wine_rb_get(&fontface->cache.tree, key)))
    {
        if (!(entry = calloc(1, sizeof(*entry)))) return NULL;
        entry->key = *key;
        list_init(&entry->mru);

        size += sizeof(*entry);

        if ((fontface->cache.size + size > fontface->cache.max_size) && !list_empty(&fontface->cache.mru))
        {
            old_entry = LIST_ENTRY(list_tail(&fontface->cache.mru), struct cache_entry, mru);
            fontface->cache.size -= (old_entry->bitmap_size + sizeof(*old_entry));
            wine_rb_remove(&fontface->cache.tree, &old_entry->entry);
            list_remove(&old_entry->mru);
            fontface_release_cache_entry(old_entry);
        }

        if (wine_rb_put(&fontface->cache.tree, key, &entry->entry) == -1)
        {
            WARN("Failed to add cache entry.\n");
            free(entry);
            return NULL;
        }

        fontface->cache.size += size;
    }
    else
        entry = WINE_RB_ENTRY_VALUE(e, struct cache_entry, entry);

    list_remove(&entry->mru);
    list_add_head(&fontface->cache.mru, &entry->mru);

    return entry;
}

static int fontface_get_glyph_advance(struct dwrite_fontface *fontface, float fontsize, unsigned short glyph,
        unsigned short mode, BOOL *has_contours)
{
    struct cache_key key = { .size = fontsize, .glyph = glyph, .mode = mode };
    struct get_glyph_advance_params params;
    struct cache_entry *entry;
    unsigned int value;

    if (!(entry = fontface_get_cache_entry(fontface, 0, &key)))
        return 0;

    if (!entry->has_advance)
    {
        params.object = fontface->get_font_object(fontface);
        params.glyph = glyph;
        params.mode = mode;
        params.emsize = fontsize;
        params.advance = &entry->advance;
        params.has_contours = &value;

        UNIX_CALL(get_glyph_advance, &params);

        entry->has_contours = !!value;
        entry->has_advance = 1;
    }

    *has_contours = entry->has_contours;
    return entry->advance;
}

void dwrite_fontface_get_glyph_bbox(IDWriteFontFace *iface, struct dwrite_glyphbitmap *bitmap)
{
    struct cache_key key = { .size = bitmap->emsize, .glyph = bitmap->glyph, .mode = DWRITE_MEASURING_MODE_NATURAL };
    struct dwrite_fontface *fontface = unsafe_impl_from_IDWriteFontFace(iface);
    struct get_glyph_bbox_params params;
    struct cache_entry *entry;

    params.object = fontface->get_font_object(fontface);
    params.simulations = bitmap->simulations;
    params.glyph = bitmap->glyph;
    params.emsize = bitmap->emsize;
    matrix_2x2_from_dwrite_matrix(&params.m, bitmap->m ? bitmap->m : &identity);

    EnterCriticalSection(&fontface->cs);
    /* For now bypass cache for transformed cases. */
    if (bitmap->m && memcmp(&params.m, &identity_2x2, sizeof(params.m)))
    {
        params.bbox = &bitmap->bbox;
        UNIX_CALL(get_glyph_bbox, &params);
    }
    else if ((entry = fontface_get_cache_entry(fontface, 0, &key)))
    {
        if (!entry->has_bbox)
        {
            params.bbox = &entry->bbox;
            UNIX_CALL(get_glyph_bbox, &params);
            entry->has_bbox = 1;
        }
        bitmap->bbox = entry->bbox;
    }
    LeaveCriticalSection(&fontface->cs);
}

static unsigned int get_glyph_bitmap_pitch(DWRITE_RENDERING_MODE1 rendering_mode, INT width)
{
    return rendering_mode == DWRITE_RENDERING_MODE1_ALIASED ? ((width + 31) >> 5) << 2 : (width + 3) / 4 * 4;
}

static HRESULT dwrite_fontface_get_glyph_bitmap(struct dwrite_fontface *fontface, DWRITE_RENDERING_MODE1 rendering_mode,
        unsigned int *is_1bpp, struct dwrite_glyphbitmap *bitmap)
{
    struct cache_key key = { .size = bitmap->emsize, .glyph = bitmap->glyph, .mode = DWRITE_MEASURING_MODE_NATURAL };
    struct get_glyph_bitmap_params params;
    const RECT *bbox = &bitmap->bbox;
    unsigned int bitmap_size, _1bpp;
    struct cache_entry *entry;
    HRESULT hr = S_OK;

    bitmap_size = get_glyph_bitmap_pitch(rendering_mode, bbox->right - bbox->left) *
            (bbox->bottom - bbox->top);

    params.object = fontface->get_font_object(fontface);
    params.simulations = fontface->simulations;
    params.glyph = bitmap->glyph;
    params.mode = rendering_mode;
    params.emsize = bitmap->emsize;
    params.bbox = bitmap->bbox;
    params.pitch = bitmap->pitch;
    params.bitmap = bitmap->buf;
    params.is_1bpp = is_1bpp;
    matrix_2x2_from_dwrite_matrix(&params.m, bitmap->m ? bitmap->m : &identity);

    EnterCriticalSection(&fontface->cs);
    /* For now bypass cache for transformed cases. */
    if (bitmap->m && memcmp(&params.m, &identity_2x2, sizeof(params.m)))
    {
        UNIX_CALL(get_glyph_bitmap, &params);
    }
    else if ((entry = fontface_get_cache_entry(fontface, bitmap_size, &key)))
    {
        if (entry->has_bitmap)
        {
            memcpy(bitmap->buf, entry->bitmap, entry->bitmap_size);
        }
        else
        {
            params.is_1bpp = &_1bpp;
            UNIX_CALL(get_glyph_bitmap, &params);

            entry->bitmap_size = bitmap_size;
            if ((entry->bitmap = malloc(entry->bitmap_size)))
                memcpy(entry->bitmap, bitmap->buf, entry->bitmap_size);
            entry->is_1bpp = !!_1bpp;
            entry->has_bitmap = 1;
        }
        *is_1bpp = entry->is_1bpp;
    }
    else
        hr = E_FAIL;
    LeaveCriticalSection(&fontface->cs);

    return hr;
}

static int fontface_cache_compare(const void *k, const struct wine_rb_entry *e)
{
    const struct cache_entry *entry = WINE_RB_ENTRY_VALUE(e, const struct cache_entry, entry);
    const struct cache_key *key = k, *key2 = &entry->key;

    if (key->size != key2->size) return key->size < key2->size ? -1 : 1;
    if (key->glyph != key2->glyph) return (int)key->glyph - (int)key2->glyph;
    if (key->mode != key2->mode) return (int)key->mode - (int)key2->mode;
    return 0;
}

static void fontface_cache_init(struct dwrite_fontface *fontface)
{
    wine_rb_init(&fontface->cache.tree, fontface_cache_compare);
    list_init(&fontface->cache.mru);
    fontface->cache.max_size = 0x8000;
}

static void fontface_cache_clear(struct dwrite_fontface *fontface)
{
    struct cache_entry *entry, *entry2;

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &fontface->cache.mru, struct cache_entry, mru)
    {
        list_remove(&entry->mru);
        fontface_release_cache_entry(entry);
    }
    memset(&fontface->cache, 0, sizeof(fontface->cache));
}

struct dwrite_font_propvec {
    FLOAT stretch;
    FLOAT style;
    FLOAT weight;
};

struct dwrite_font_data
{
    LONG refcount;

    DWRITE_FONT_STYLE style;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_PANOSE panose;
    FONTSIGNATURE fontsig;
    UINT32 flags; /* enum font_flags */
    struct dwrite_font_propvec propvec;
    struct dwrite_cmap cmap;
    /* Static axis for weight/width/italic. */
    DWRITE_FONT_AXIS_VALUE axis[3];

    DWRITE_FONT_METRICS1 metrics;
    IDWriteLocalizedStrings *info_strings[DWRITE_INFORMATIONAL_STRING_SUPPORTED_SCRIPT_LANGUAGE_TAG + 1];
    IDWriteLocalizedStrings *family_names;
    IDWriteLocalizedStrings *names;

    /* data needed to create fontface instance */
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFile *file;
    UINT32 face_index;

    WCHAR *facename;

    USHORT simulations;

    LOGFONTW lf;

    /* used to mark font as tested when scanning for simulation candidate */
    unsigned int bold_sim_tested : 1;
    unsigned int oblique_sim_tested : 1;
};

struct dwrite_fontfamily_data
{
    LONG refcount;

    IDWriteLocalizedStrings *familyname;

    struct dwrite_font_data **fonts;
    size_t size;
    size_t count;

    unsigned int has_normal_face : 1;
    unsigned int has_oblique_face : 1;
    unsigned int has_italic_face : 1;
};

struct dwrite_fontcollection
{
    IDWriteFontCollection3 IDWriteFontCollection3_iface;
    LONG refcount;

    IDWriteFactory7 *factory;
    DWRITE_FONT_FAMILY_MODEL family_model;
    struct dwrite_fontfamily_data **family_data;
    size_t size;
    size_t count;
};

struct dwrite_fontfamily
{
    IDWriteFontFamily2 IDWriteFontFamily2_iface;
    IDWriteFontList2 IDWriteFontList2_iface;
    LONG refcount;

    struct dwrite_fontfamily_data *data;
    struct dwrite_fontcollection *collection;
};

struct dwrite_fontlist
{
    IDWriteFontList2 IDWriteFontList2_iface;
    LONG refcount;

    struct dwrite_font_data **fonts;
    UINT32 font_count;
    struct dwrite_fontfamily *family;
};

struct dwrite_font
{
    IDWriteFont3 IDWriteFont3_iface;
    LONG refcount;

    DWRITE_FONT_STYLE style;
    struct dwrite_font_data *data;
    struct dwrite_fontfamily *family;
};

enum runanalysis_flags {
    RUNANALYSIS_BOUNDS_READY  = 1 << 0,
    RUNANALYSIS_BITMAP_READY  = 1 << 1,
    RUNANALYSIS_USE_TRANSFORM = 1 << 2
};

struct dwrite_glyphrunanalysis
{
    IDWriteGlyphRunAnalysis IDWriteGlyphRunAnalysis_iface;
    LONG refcount;

    DWRITE_RENDERING_MODE1 rendering_mode;
    DWRITE_TEXTURE_TYPE texture_type; /* derived from rendering mode specified on creation */
    DWRITE_GLYPH_RUN run; /* glyphAdvances and glyphOffsets are not used */
    DWRITE_MATRIX m;
    UINT16 *glyphs;
    D2D_POINT_2F *origins;

    UINT8 flags;
    RECT bounds;
    BYTE *bitmap;
    UINT32 max_glyph_bitmap_size;
};

struct dwrite_colorglyphenum
{
    IDWriteColorGlyphRunEnumerator1 IDWriteColorGlyphRunEnumerator1_iface;
    LONG refcount;

    D2D1_POINT_2F origin;             /* original run origin */

    IDWriteFontFace5 *fontface;       /* for convenience */
    DWRITE_COLOR_GLYPH_RUN1 colorrun; /* returned with GetCurrentRun() */
    DWRITE_GLYPH_RUN run;             /* base run */
    UINT32 palette;                   /* palette index to get layer color from */
    FLOAT *advances;                  /* original or measured advances for base glyphs */
    FLOAT *color_advances;            /* returned color run points to this */
    DWRITE_GLYPH_OFFSET *offsets;     /* original offsets, or NULL */
    DWRITE_GLYPH_OFFSET *color_offsets; /* returned color run offsets, or NULL */
    UINT16 *glyphindices;             /* returned color run points to this */
    struct dwrite_colorglyph *glyphs; /* current glyph color info */
    BOOL has_regular_glyphs;          /* TRUE if there's any glyph without a color */
    UINT16 current_layer;             /* enumerator position, updated with MoveNext */
    UINT16 max_layer_num;             /* max number of layers for this run */
    struct dwrite_fonttable colr;     /* used to access layers */
};

struct dwrite_fontfile
{
    IDWriteFontFile IDWriteFontFile_iface;
    LONG refcount;

    IDWriteFontFileLoader *loader;
    void *reference_key;
    UINT32 key_size;
    IDWriteFontFileStream *stream;
};

struct dwrite_fontfacereference
{
    IDWriteFontFaceReference1 IDWriteFontFaceReference1_iface;
    LONG refcount;

    IDWriteFontFile *file;
    UINT32 index;
    USHORT simulations;
    DWRITE_FONT_AXIS_VALUE *axis_values;
    UINT32 axis_values_count;
    IDWriteFactory7 *factory;
};

static const IDWriteFontFaceReference1Vtbl fontfacereferencevtbl;

struct dwrite_fontresource
{
    IDWriteFontResource IDWriteFontResource_iface;
    LONG refcount;

    IDWriteFontFile *file;
    UINT32 face_index;
    IDWriteFactory7 *factory;

    struct dwrite_var_axis *axis;
    unsigned int axis_count;
};

struct dwrite_fontset_entry_desc
{
    IDWriteFontFile *file;
    DWRITE_FONT_FACE_TYPE face_type;
    unsigned int face_index;
    unsigned int simulations;
};

struct dwrite_fontset_entry
{
    LONG refcount;
    struct dwrite_fontset_entry_desc desc;
    IDWriteLocalizedStrings *props[DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FACE_NAME + 1];
};

struct dwrite_fontset
{
    IDWriteFontSet3 IDWriteFontSet3_iface;
    LONG refcount;
    IDWriteFactory7 *factory;

    struct dwrite_fontset_entry **entries;
    unsigned int count;
};

struct dwrite_fontset_builder
{
    IDWriteFontSetBuilder2 IDWriteFontSetBuilder2_iface;
    LONG refcount;
    IDWriteFactory7 *factory;

    struct dwrite_fontset_entry **entries;
    size_t count;
    size_t capacity;
};

static HRESULT fontset_create_from_font_data(IDWriteFactory7 *factory, struct dwrite_font_data **fonts,
        unsigned int count, IDWriteFontSet1 **ret);

static void dwrite_grab_font_table(void *context, UINT32 table, const BYTE **data, UINT32 *size, void **data_context)
{
    struct dwrite_fontface *fontface = context;
    BOOL exists = FALSE;

    if (FAILED(IDWriteFontFace5_TryGetFontTable(&fontface->IDWriteFontFace5_iface, table, (const void **)data,
            size, data_context, &exists)) || !exists)
    {
        *data = NULL;
        *size = 0;
        *data_context = NULL;
    }
}

static void dwrite_release_font_table(void *context, void *data_context)
{
    struct dwrite_fontface *fontface = context;
    IDWriteFontFace5_ReleaseFontTable(&fontface->IDWriteFontFace5_iface, data_context);
}

static UINT16 dwrite_get_font_upem(void *context)
{
    struct dwrite_fontface *fontface = context;
    return fontface->metrics.designUnitsPerEm;
}

static UINT16 dwritefontface_get_glyph(struct dwrite_fontface *fontface, unsigned int ch)
{
    dwrite_cmap_init(&fontface->cmap, NULL, fontface->index, fontface->type);
    return opentype_cmap_get_glyph(&fontface->cmap, ch);
}

static BOOL dwrite_has_glyph(void *context, unsigned int codepoint)
{
    struct dwrite_fontface *fontface = context;
    return !!dwritefontface_get_glyph(fontface, codepoint);
}

static UINT16 dwrite_get_glyph(void *context, unsigned int codepoint)
{
    struct dwrite_fontface *fontface = context;
    return dwritefontface_get_glyph(fontface, codepoint);
}

static const struct shaping_font_ops dwrite_font_ops =
{
    dwrite_grab_font_table,
    dwrite_release_font_table,
    dwrite_get_font_upem,
    dwrite_has_glyph,
    dwrite_get_glyph,
};

struct scriptshaping_cache *fontface_get_shaping_cache(struct dwrite_fontface *fontface)
{
    if (fontface->shaping_cache)
        return fontface->shaping_cache;

    return fontface->shaping_cache = create_scriptshaping_cache(fontface, &dwrite_font_ops);
}

static inline struct dwrite_fontface *impl_from_IDWriteFontFace5(IDWriteFontFace5 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontface, IDWriteFontFace5_iface);
}

static struct dwrite_fontface *impl_from_IDWriteFontFaceReference(IDWriteFontFaceReference *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontface, IDWriteFontFaceReference_iface);
}

static inline struct dwrite_font *impl_from_IDWriteFont3(IDWriteFont3 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_font, IDWriteFont3_iface);
}

static struct dwrite_font *unsafe_impl_from_IDWriteFont(IDWriteFont *iface);

static inline struct dwrite_fontfile *impl_from_IDWriteFontFile(IDWriteFontFile *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfile, IDWriteFontFile_iface);
}

static inline struct dwrite_fontfamily *impl_from_IDWriteFontFamily2(IDWriteFontFamily2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfamily, IDWriteFontFamily2_iface);
}

static inline struct dwrite_fontfamily *impl_family_from_IDWriteFontList2(IDWriteFontList2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfamily, IDWriteFontList2_iface);
}

static inline struct dwrite_fontcollection *impl_from_IDWriteFontCollection3(IDWriteFontCollection3 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontcollection, IDWriteFontCollection3_iface);
}

static inline struct dwrite_glyphrunanalysis *impl_from_IDWriteGlyphRunAnalysis(IDWriteGlyphRunAnalysis *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_glyphrunanalysis, IDWriteGlyphRunAnalysis_iface);
}

static inline struct dwrite_colorglyphenum *impl_from_IDWriteColorGlyphRunEnumerator1(IDWriteColorGlyphRunEnumerator1 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_colorglyphenum, IDWriteColorGlyphRunEnumerator1_iface);
}

static inline struct dwrite_fontlist *impl_from_IDWriteFontList2(IDWriteFontList2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontlist, IDWriteFontList2_iface);
}

static inline struct dwrite_fontfacereference *impl_from_IDWriteFontFaceReference1(IDWriteFontFaceReference1 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfacereference, IDWriteFontFaceReference1_iface);
}

static struct dwrite_fontresource *impl_from_IDWriteFontResource(IDWriteFontResource *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontresource, IDWriteFontResource_iface);
}

static struct dwrite_fontset_builder *impl_from_IDWriteFontSetBuilder2(IDWriteFontSetBuilder2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontset_builder, IDWriteFontSetBuilder2_iface);
}

static struct dwrite_fontset *impl_from_IDWriteFontSet3(IDWriteFontSet3 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontset, IDWriteFontSet3_iface);
}

static struct dwrite_fontset *unsafe_impl_from_IDWriteFontSet(IDWriteFontSet *iface);

static HRESULT get_cached_glyph_metrics(struct dwrite_fontface *fontface, UINT16 glyph, DWRITE_GLYPH_METRICS *metrics)
{
    static const DWRITE_GLYPH_METRICS nil;
    DWRITE_GLYPH_METRICS *block = fontface->glyphs[glyph >> GLYPH_BLOCK_SHIFT];

    if (!block || !memcmp(&block[glyph & GLYPH_BLOCK_MASK], &nil, sizeof(DWRITE_GLYPH_METRICS))) return S_FALSE;
    memcpy(metrics, &block[glyph & GLYPH_BLOCK_MASK], sizeof(*metrics));
    return S_OK;
}

static HRESULT set_cached_glyph_metrics(struct dwrite_fontface *fontface, UINT16 glyph, DWRITE_GLYPH_METRICS *metrics)
{
    DWRITE_GLYPH_METRICS **block = &fontface->glyphs[glyph >> GLYPH_BLOCK_SHIFT];

    if (!*block)
    {
        /* start new block */
        if (!(*block = calloc(GLYPH_BLOCK_SIZE, sizeof(*metrics))))
            return E_OUTOFMEMORY;
    }

    memcpy(&(*block)[glyph & GLYPH_BLOCK_MASK], metrics, sizeof(*metrics));
    return S_OK;
}

const void* get_fontface_table(IDWriteFontFace5 *fontface, UINT32 tag, struct dwrite_fonttable *table)
{
    HRESULT hr;

    if (table->data || !table->exists)
        return table->data;

    table->exists = FALSE;
    hr = IDWriteFontFace5_TryGetFontTable(fontface, tag, (const void **)&table->data, &table->size, &table->context,
        &table->exists);
    if (FAILED(hr) || !table->exists) {
        TRACE("Font does not have %s table\n", debugstr_fourcc(tag));
        return NULL;
    }

    return table->data;
}

static void init_font_prop_vec(DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style,
    struct dwrite_font_propvec *vec)
{
    vec->stretch = ((INT32)stretch - DWRITE_FONT_STRETCH_NORMAL) * 11.0f;
    vec->style = (float)style * 7.0f;
    vec->weight = ((INT32)weight - DWRITE_FONT_WEIGHT_NORMAL) / 100.0f * 5.0f;
}

static FLOAT get_font_prop_vec_distance(const struct dwrite_font_propvec *left, const struct dwrite_font_propvec *right)
{
    return powf(left->stretch - right->stretch, 2) + powf(left->style - right->style, 2) + powf(left->weight - right->weight, 2);
}

static FLOAT get_font_prop_vec_dotproduct(const struct dwrite_font_propvec *left, const struct dwrite_font_propvec *right)
{
    return left->stretch * right->stretch + left->style * right->style + left->weight * right->weight;
}

static const struct dwrite_fonttable *get_fontface_vdmx(struct dwrite_fontface *fontface)
{
    get_fontface_table(&fontface->IDWriteFontFace5_iface, MS_VDMX_TAG, &fontface->vdmx);
    return &fontface->vdmx;
}

static const struct dwrite_fonttable *get_fontface_gasp(struct dwrite_fontface *fontface)
{
    get_fontface_table(&fontface->IDWriteFontFace5_iface, MS_GASP_TAG, &fontface->gasp);
    return &fontface->gasp;
}

static const struct dwrite_fonttable *get_fontface_cpal(struct dwrite_fontface *fontface)
{
    get_fontface_table(&fontface->IDWriteFontFace5_iface, MS_CPAL_TAG, &fontface->cpal);
    return &fontface->cpal;
}

static struct dwrite_font_data * addref_font_data(struct dwrite_font_data *data)
{
    InterlockedIncrement(&data->refcount);
    return data;
}

static void release_font_data(struct dwrite_font_data *data)
{
    int i;

    if (InterlockedDecrement(&data->refcount) > 0)
        return;

    for (i = 0; i < ARRAY_SIZE(data->info_strings); ++i)
    {
        if (data->info_strings[i])
            IDWriteLocalizedStrings_Release(data->info_strings[i]);
    }
    if (data->names)
        IDWriteLocalizedStrings_Release(data->names);

    if (data->family_names)
        IDWriteLocalizedStrings_Release(data->family_names);

    dwrite_cmap_release(&data->cmap);
    IDWriteFontFile_Release(data->file);
    free(data->facename);
    free(data);
}

static void release_fontfamily_data(struct dwrite_fontfamily_data *data)
{
    size_t i;

    if (InterlockedDecrement(&data->refcount) > 0)
        return;

    for (i = 0; i < data->count; ++i)
        release_font_data(data->fonts[i]);
    free(data->fonts);
    IDWriteLocalizedStrings_Release(data->familyname);
    free(data);
}

void fontface_detach_from_cache(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    fontface->cached = NULL;
}

static BOOL is_same_fontfile(IDWriteFontFile *left, IDWriteFontFile *right)
{
    UINT32 left_key_size, right_key_size;
    const void *left_key, *right_key;
    HRESULT hr;

    if (left == right)
        return TRUE;

    hr = IDWriteFontFile_GetReferenceKey(left, &left_key, &left_key_size);
    if (FAILED(hr))
        return FALSE;

    hr = IDWriteFontFile_GetReferenceKey(right, &right_key, &right_key_size);
    if (FAILED(hr))
        return FALSE;

    if (left_key_size != right_key_size)
        return FALSE;

    return !memcmp(left_key, right_key, left_key_size);
}

static HRESULT WINAPI dwritefontface_QueryInterface(IDWriteFontFace5 *iface, REFIID riid, void **obj)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontFace5) ||
        IsEqualIID(riid, &IID_IDWriteFontFace4) ||
        IsEqualIID(riid, &IID_IDWriteFontFace3) ||
        IsEqualIID(riid, &IID_IDWriteFontFace2) ||
        IsEqualIID(riid, &IID_IDWriteFontFace1) ||
        IsEqualIID(riid, &IID_IDWriteFontFace) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IDWriteFontFaceReference))
        *obj = &fontface->IDWriteFontFaceReference_iface;
    else
        *obj = NULL;

    if (*obj)
    {
        if (InterlockedIncrement(&fontface->refcount) == 1)
        {
            InterlockedDecrement(&fontface->refcount);
            *obj = NULL;
            return E_FAIL;
        }
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontface_AddRef(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    ULONG refcount = InterlockedIncrement(&fontface->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritefontface_Release(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    ULONG refcount = InterlockedDecrement(&fontface->refcount);
    struct release_font_object_params params = { fontface->font_object };

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        UINT32 i;

        if (fontface->cached)
        {
            factory_lock(fontface->factory);
            list_remove(&fontface->cached->entry);
            factory_unlock(fontface->factory);
            free(fontface->cached);
        }
        release_scriptshaping_cache(fontface->shaping_cache);
        if (fontface->vdmx.context)
            IDWriteFontFace5_ReleaseFontTable(iface, fontface->vdmx.context);
        if (fontface->gasp.context)
            IDWriteFontFace5_ReleaseFontTable(iface, fontface->gasp.context);
        if (fontface->cpal.context)
            IDWriteFontFace5_ReleaseFontTable(iface, fontface->cpal.context);
        if (fontface->colr.context)
            IDWriteFontFace5_ReleaseFontTable(iface, fontface->colr.context);
        if (fontface->kern.context)
            IDWriteFontFace5_ReleaseFontTable(iface, fontface->kern.context);
        if (fontface->file)
            IDWriteFontFile_Release(fontface->file);
        if (fontface->names)
            IDWriteLocalizedStrings_Release(fontface->names);
        if (fontface->family_names)
            IDWriteLocalizedStrings_Release(fontface->family_names);
        for (i = 0; i < ARRAY_SIZE(fontface->info_strings); ++i)
        {
            if (fontface->info_strings[i])
                IDWriteLocalizedStrings_Release(fontface->info_strings[i]);
        }

        for (i = 0; i < ARRAY_SIZE(fontface->glyphs); i++)
            free(fontface->glyphs[i]);

        UNIX_CALL(release_font_object, &params);
        if (fontface->stream)
        {
            IDWriteFontFileStream_ReleaseFileFragment(fontface->stream, fontface->data_context);
            IDWriteFontFileStream_Release(fontface->stream);
        }
        fontface_cache_clear(fontface);

        dwrite_cmap_release(&fontface->cmap);
        IDWriteFactory7_Release(fontface->factory);
        DeleteCriticalSection(&fontface->cs);
        free(fontface);
    }

    return refcount;
}

static DWRITE_FONT_FACE_TYPE WINAPI dwritefontface_GetType(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return fontface->type;
}

static HRESULT WINAPI dwritefontface_GetFiles(IDWriteFontFace5 *iface, UINT32 *number_of_files,
    IDWriteFontFile **fontfiles)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p, %p.\n", iface, number_of_files, fontfiles);

    if (!fontfiles)
    {
        *number_of_files = 1;
        return S_OK;
    }

    if (!*number_of_files)
        return E_INVALIDARG;

    IDWriteFontFile_AddRef(fontface->file);
    *fontfiles = fontface->file;

    return S_OK;
}

static UINT32 WINAPI dwritefontface_GetIndex(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return fontface->index;
}

static DWRITE_FONT_SIMULATIONS WINAPI dwritefontface_GetSimulations(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return fontface->simulations;
}

static BOOL WINAPI dwritefontface_IsSymbolFont(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return !!(fontface->flags & FONT_IS_SYMBOL);
}

static void WINAPI dwritefontface_GetMetrics(IDWriteFontFace5 *iface, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, metrics);

    memcpy(metrics, &fontface->metrics, sizeof(*metrics));
}

static UINT16 WINAPI dwritefontface_GetGlyphCount(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    struct get_glyph_count_params params;
    unsigned int count;

    TRACE("%p.\n", iface);

    params.object = fontface->get_font_object(fontface);
    params.count = &count;
    UNIX_CALL(get_glyph_count, &params);

    return count;
}

static HRESULT WINAPI dwritefontface_GetDesignGlyphMetrics(IDWriteFontFace5 *iface,
    UINT16 const *glyphs, UINT32 glyph_count, DWRITE_GLYPH_METRICS *ret, BOOL is_sideways)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    struct get_design_glyph_metrics_params params;
    DWRITE_GLYPH_METRICS metrics;
    HRESULT hr = S_OK;
    unsigned int i;

    TRACE("%p, %p, %u, %p, %d.\n", iface, glyphs, glyph_count, ret, is_sideways);

    if (!glyphs)
        return E_INVALIDARG;

    if (is_sideways)
        FIXME("sideways metrics are not supported.\n");

    params.object = fontface->get_font_object(fontface);
    params.simulations = fontface->simulations;
    params.upem = fontface->metrics.designUnitsPerEm;
    params.ascent = fontface->typo_metrics.ascent;
    params.metrics = &metrics;

    EnterCriticalSection(&fontface->cs);
    for (i = 0; i < glyph_count; ++i)
    {

        if (get_cached_glyph_metrics(fontface, glyphs[i], &metrics) != S_OK)
        {
            params.glyph = glyphs[i];
            UNIX_CALL(get_design_glyph_metrics, &params);
            if (FAILED(hr = set_cached_glyph_metrics(fontface, glyphs[i], &metrics))) break;
        }
        ret[i] = metrics;
    }
    LeaveCriticalSection(&fontface->cs);

    return hr;
}

static HRESULT WINAPI dwritefontface_GetGlyphIndices(IDWriteFontFace5 *iface, UINT32 const *codepoints,
    UINT32 count, UINT16 *glyphs)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    unsigned int i;

    TRACE("%p, %p, %u, %p.\n", iface, codepoints, count, glyphs);

    if (!glyphs)
        return E_INVALIDARG;

    if (!codepoints)
    {
        memset(glyphs, 0, count * sizeof(*glyphs));
        return E_INVALIDARG;
    }

    for (i = 0; i < count; ++i)
        glyphs[i] = dwritefontface_get_glyph(fontface, codepoints[i]);

    return S_OK;
}

static HRESULT WINAPI dwritefontface_TryGetFontTable(IDWriteFontFace5 *iface, UINT32 table_tag,
    const void **table_data, UINT32 *table_size, void **context, BOOL *exists)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    struct file_stream_desc stream_desc;

    TRACE("%p, %s, %p, %p, %p, %p.\n", iface, debugstr_fourcc(table_tag), table_data, table_size, context, exists);

    stream_desc.stream = fontface->stream;
    stream_desc.face_type = fontface->type;
    stream_desc.face_index = fontface->index;
    return opentype_try_get_font_table(&stream_desc, table_tag, table_data, context, table_size, exists);
}

static void WINAPI dwritefontface_ReleaseFontTable(IDWriteFontFace5 *iface, void *table_context)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, table_context);

    IDWriteFontFileStream_ReleaseFileFragment(fontface->stream, table_context);
}

static void apply_outline_point_offset(const D2D1_POINT_2F *src, const D2D1_POINT_2F *offset,
        D2D1_POINT_2F *dst)
{
    dst->x = src->x + offset->x;
    dst->y = src->y + offset->y;
}

static HRESULT WINAPI dwritefontface_GetGlyphRunOutline(IDWriteFontFace5 *iface, FLOAT emSize,
    UINT16 const *glyphs, FLOAT const* advances, DWRITE_GLYPH_OFFSET const *offsets,
    UINT32 count, BOOL is_sideways, BOOL is_rtl, IDWriteGeometrySink *sink)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    D2D1_POINT_2F *origins, baseline_origin = { 0 };
    struct dwrite_outline outline, outline_size;
    struct get_glyph_outline_params params;
    D2D1_BEZIER_SEGMENT segment;
    D2D1_POINT_2F point;
    DWRITE_GLYPH_RUN run;
    unsigned int i, j, p;
    NTSTATUS status;
    HRESULT hr;

    TRACE("%p, %.8e, %p, %p, %p, %u, %d, %d, %p.\n", iface, emSize, glyphs, advances, offsets,
        count, is_sideways, is_rtl, sink);

    if (!glyphs || !sink)
        return E_INVALIDARG;

    if (!count)
        return S_OK;

    run.fontFace = (IDWriteFontFace *)iface;
    run.fontEmSize = emSize;
    run.glyphCount = count;
    run.glyphIndices = glyphs;
    run.glyphAdvances = advances;
    run.glyphOffsets = offsets;
    run.isSideways = is_sideways;
    run.bidiLevel = is_rtl ? 1 : 0;

    if (!(origins = malloc(sizeof(*origins) * count)))
        return E_OUTOFMEMORY;

    if (FAILED(hr = compute_glyph_origins(&run, DWRITE_MEASURING_MODE_NATURAL, baseline_origin, NULL, origins)))
    {
        free(origins);
        return hr;
    }

    ID2D1SimplifiedGeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);

    memset(&outline_size, 0, sizeof(outline_size));
    memset(&outline, 0, sizeof(outline));

    params.object = fontface->get_font_object(fontface);
    params.simulations = fontface->simulations;
    params.emsize = emSize;

    for (i = 0; i < count; ++i)
    {
        outline.tags.count = outline.points.count = 0;

        EnterCriticalSection(&fontface->cs);

        params.glyph = glyphs[i];
        params.outline = &outline_size;

        if (!(status = UNIX_CALL(get_glyph_outline, &params)))
        {
            dwrite_array_reserve((void **)&outline.tags.values, &outline.tags.size, outline_size.tags.count,
                    sizeof(*outline.tags.values));
            dwrite_array_reserve((void **)&outline.points.values, &outline.points.size, outline_size.points.count,
                    sizeof(*outline.points.values));

            params.outline = &outline;
            if ((status = UNIX_CALL(get_glyph_outline, &params)))
            {
                WARN("Failed to get glyph outline for glyph %u.\n", glyphs[i]);
            }
        }
        LeaveCriticalSection(&fontface->cs);

        if (status)
            continue;

        for (j = 0, p = 0; j < outline.tags.count; ++j)
        {
            switch (outline.tags.values[j])
            {
                case OUTLINE_BEGIN_FIGURE:
                    apply_outline_point_offset(&outline.points.values[p++], &origins[i], &point);
                    ID2D1SimplifiedGeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
                    break;
                case OUTLINE_END_FIGURE:
                    ID2D1SimplifiedGeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
                    break;
                case OUTLINE_LINE:
                    apply_outline_point_offset(&outline.points.values[p++], &origins[i], &point);
                    ID2D1SimplifiedGeometrySink_AddLines(sink, &point, 1);
                    break;
                case OUTLINE_BEZIER:
                    apply_outline_point_offset(&outline.points.values[p++], &origins[i], &segment.point1);
                    apply_outline_point_offset(&outline.points.values[p++], &origins[i], &segment.point2);
                    apply_outline_point_offset(&outline.points.values[p++], &origins[i], &segment.point3);
                    ID2D1SimplifiedGeometrySink_AddBeziers(sink, &segment, 1);
                    break;
            }
        }
    }

    free(outline.tags.values);
    free(outline.points.values);
    free(origins);

    return S_OK;
}

static DWRITE_RENDERING_MODE fontface_renderingmode_from_measuringmode(DWRITE_MEASURING_MODE measuring,
        float ppem, unsigned int gasp)
{
    DWRITE_RENDERING_MODE mode = DWRITE_RENDERING_MODE_DEFAULT;

    switch (measuring)
    {
    case DWRITE_MEASURING_MODE_NATURAL:
    {
        if (!(gasp & GASP_SYMMETRIC_SMOOTHING) && (ppem <= RECOMMENDED_NATURAL_PPEM))
            mode = DWRITE_RENDERING_MODE_NATURAL;
        else
            mode = DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC;
        break;
    }
    case DWRITE_MEASURING_MODE_GDI_CLASSIC:
        mode = DWRITE_RENDERING_MODE_GDI_CLASSIC;
        break;
    case DWRITE_MEASURING_MODE_GDI_NATURAL:
        mode = DWRITE_RENDERING_MODE_GDI_NATURAL;
        break;
    default:
        ;
    }

    return mode;
}

static HRESULT WINAPI dwritefontface_GetRecommendedRenderingMode(IDWriteFontFace5 *iface, FLOAT emSize,
    FLOAT ppdip, DWRITE_MEASURING_MODE measuring, IDWriteRenderingParams *params, DWRITE_RENDERING_MODE *mode)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    unsigned int flags;
    FLOAT ppem;

    TRACE("%p, %.8e, %.8e, %d, %p, %p.\n", iface, emSize, ppdip, measuring, params, mode);

    if (!params) {
        *mode = DWRITE_RENDERING_MODE_DEFAULT;
        return E_INVALIDARG;
    }

    *mode = IDWriteRenderingParams_GetRenderingMode(params);
    if (*mode != DWRITE_RENDERING_MODE_DEFAULT)
        return S_OK;

    ppem = emSize * ppdip;

    if (ppem >= RECOMMENDED_OUTLINE_AA_THRESHOLD) {
        *mode = DWRITE_RENDERING_MODE_OUTLINE;
        return S_OK;
    }

    flags = opentype_get_gasp_flags(get_fontface_gasp(fontface), ppem);
    *mode = fontface_renderingmode_from_measuringmode(measuring, ppem, flags);
    return S_OK;
}

static HRESULT WINAPI dwritefontface_GetGdiCompatibleMetrics(IDWriteFontFace5 *iface, FLOAT emSize, FLOAT pixels_per_dip,
    DWRITE_MATRIX const *transform, DWRITE_FONT_METRICS *metrics)
{
    DWRITE_FONT_METRICS1 metrics1;
    HRESULT hr = IDWriteFontFace5_GetGdiCompatibleMetrics(iface, emSize, pixels_per_dip, transform, &metrics1);
    memcpy(metrics, &metrics1, sizeof(*metrics));
    return hr;
}

static inline int round_metric(FLOAT metric)
{
    return (int)floorf(metric + 0.5f);
}

static UINT32 fontface_get_horz_metric_adjustment(const struct dwrite_fontface *fontface)
{
    if (!(fontface->simulations & DWRITE_FONT_SIMULATIONS_BOLD))
        return 0;

    return (fontface->metrics.designUnitsPerEm + 49) / 50;
}

static HRESULT WINAPI dwritefontface_GetGdiCompatibleGlyphMetrics(IDWriteFontFace5 *iface, FLOAT emSize, FLOAT ppdip,
    DWRITE_MATRIX const *m, BOOL use_gdi_natural, UINT16 const *glyphs, UINT32 glyph_count,
    DWRITE_GLYPH_METRICS *metrics, BOOL is_sideways)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    UINT32 adjustment = fontface_get_horz_metric_adjustment(fontface);
    DWRITE_MEASURING_MODE mode;
    FLOAT scale, size;
    HRESULT hr = S_OK;
    UINT32 i;

    TRACE("%p, %.8e, %.8e, %p, %d, %p, %u, %p, %d.\n", iface, emSize, ppdip, m, use_gdi_natural, glyphs,
        glyph_count, metrics, is_sideways);

    if (m && memcmp(m, &identity, sizeof(*m)))
        FIXME("transform is not supported, %s\n", debugstr_matrix(m));

    size = emSize * ppdip;
    scale = size / fontface->metrics.designUnitsPerEm;
    mode = use_gdi_natural ? DWRITE_MEASURING_MODE_GDI_NATURAL : DWRITE_MEASURING_MODE_GDI_CLASSIC;

    EnterCriticalSection(&fontface->cs);
    for (i = 0; i < glyph_count; ++i)
    {
        DWRITE_GLYPH_METRICS *ret = metrics + i;
        DWRITE_GLYPH_METRICS design;
        BOOL has_contours;

        hr = IDWriteFontFace5_GetDesignGlyphMetrics(iface, glyphs + i, 1, &design, is_sideways);
        if (FAILED(hr))
            break;

        ret->advanceWidth = fontface_get_glyph_advance(fontface, size, glyphs[i], mode, &has_contours);
        if (has_contours)
            ret->advanceWidth = round_metric(ret->advanceWidth * fontface->metrics.designUnitsPerEm / size + adjustment);
        else
            ret->advanceWidth = round_metric(ret->advanceWidth * fontface->metrics.designUnitsPerEm / size);

#define SCALE_METRIC(x) ret->x = round_metric(round_metric((design.x) * scale) / scale)
        SCALE_METRIC(leftSideBearing);
        SCALE_METRIC(rightSideBearing);
        SCALE_METRIC(topSideBearing);
        SCALE_METRIC(advanceHeight);
        SCALE_METRIC(bottomSideBearing);
        SCALE_METRIC(verticalOriginY);
#undef  SCALE_METRIC
    }
    LeaveCriticalSection(&fontface->cs);

    return S_OK;
}

static void WINAPI dwritefontface1_GetMetrics(IDWriteFontFace5 *iface, DWRITE_FONT_METRICS1 *metrics)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, metrics);

    *metrics = fontface->metrics;
}

static HRESULT WINAPI dwritefontface1_GetGdiCompatibleMetrics(IDWriteFontFace5 *iface, FLOAT em_size,
        FLOAT pixels_per_dip, const DWRITE_MATRIX *m, DWRITE_FONT_METRICS1 *metrics)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    const DWRITE_FONT_METRICS1 *design = &fontface->metrics;
    UINT16 ascent, descent;
    FLOAT scale;

    TRACE("%p, %.8e, %.8e, %p, %p.\n", iface, em_size, pixels_per_dip, m, metrics);

    if (em_size <= 0.0f || pixels_per_dip <= 0.0f) {
        memset(metrics, 0, sizeof(*metrics));
        return E_INVALIDARG;
    }

    em_size *= pixels_per_dip;
    if (m && m->m22 != 0.0f)
        em_size *= fabs(m->m22);

    scale = em_size / design->designUnitsPerEm;
    if (!opentype_get_vdmx_size(get_fontface_vdmx(fontface), em_size, &ascent, &descent))
    {
        ascent = round_metric(design->ascent * scale);
        descent = round_metric(design->descent * scale);
    }

#define SCALE_METRIC(x) metrics->x = round_metric(round_metric((design->x) * scale) / scale)
    metrics->designUnitsPerEm = design->designUnitsPerEm;
    metrics->ascent = round_metric(ascent / scale);
    metrics->descent = round_metric(descent / scale);

    SCALE_METRIC(lineGap);
    SCALE_METRIC(capHeight);
    SCALE_METRIC(xHeight);
    SCALE_METRIC(underlinePosition);
    SCALE_METRIC(underlineThickness);
    SCALE_METRIC(strikethroughPosition);
    SCALE_METRIC(strikethroughThickness);
    SCALE_METRIC(glyphBoxLeft);
    SCALE_METRIC(glyphBoxTop);
    SCALE_METRIC(glyphBoxRight);
    SCALE_METRIC(glyphBoxBottom);
    SCALE_METRIC(subscriptPositionX);
    SCALE_METRIC(subscriptPositionY);
    SCALE_METRIC(subscriptSizeX);
    SCALE_METRIC(subscriptSizeY);
    SCALE_METRIC(superscriptPositionX);
    SCALE_METRIC(superscriptPositionY);
    SCALE_METRIC(superscriptSizeX);
    SCALE_METRIC(superscriptSizeY);

    metrics->hasTypographicMetrics = design->hasTypographicMetrics;
#undef SCALE_METRIC

    return S_OK;
}

static void WINAPI dwritefontface1_GetCaretMetrics(IDWriteFontFace5 *iface, DWRITE_CARET_METRICS *metrics)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, metrics);

    *metrics = fontface->caret;
}

static HRESULT WINAPI dwritefontface1_GetUnicodeRanges(IDWriteFontFace5 *iface, UINT32 max_count,
    DWRITE_UNICODE_RANGE *ranges, UINT32 *count)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %u, %p, %p.\n", iface, max_count, ranges, count);

    *count = 0;
    if (max_count && !ranges)
        return E_INVALIDARG;

    dwrite_cmap_init(&fontface->cmap, NULL, fontface->index, fontface->type);
    return opentype_cmap_get_unicode_ranges(&fontface->cmap, max_count, ranges, count);
}

static BOOL WINAPI dwritefontface1_IsMonospacedFont(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return !!(fontface->flags & FONT_IS_MONOSPACED);
}

static int fontface_get_design_advance(struct dwrite_fontface *fontface, DWRITE_MEASURING_MODE measuring_mode,
        float emsize, float ppdip, const DWRITE_MATRIX *transform, UINT16 glyph, BOOL is_sideways)
{
    unsigned int adjustment = fontface_get_horz_metric_adjustment(fontface);
    BOOL has_contours;
    int advance;

    if (is_sideways)
        FIXME("Sideways mode is not supported.\n");

    switch (measuring_mode)
    {
        case DWRITE_MEASURING_MODE_NATURAL:
            advance = fontface_get_glyph_advance(fontface, fontface->metrics.designUnitsPerEm, glyph,
                    measuring_mode, &has_contours);
            if (has_contours)
                advance += adjustment;

            return advance;
        case DWRITE_MEASURING_MODE_GDI_NATURAL:
        case DWRITE_MEASURING_MODE_GDI_CLASSIC:
            emsize *= ppdip;
            if (emsize == 0.0f)
                return 0.0f;

            if (transform && memcmp(transform, &identity, sizeof(*transform)))
                FIXME("Transform is not supported.\n");

            advance = fontface_get_glyph_advance(fontface, emsize, glyph, measuring_mode, &has_contours);
            if (has_contours)
                advance = round_metric(advance * fontface->metrics.designUnitsPerEm / emsize + adjustment);
            else
                advance = round_metric(advance * fontface->metrics.designUnitsPerEm / emsize);

            return advance;
        default:
            WARN("Unknown measuring mode %u.\n", measuring_mode);
            return 0;
    }
}

static HRESULT WINAPI dwritefontface1_GetDesignGlyphAdvances(IDWriteFontFace5 *iface,
    UINT32 glyph_count, UINT16 const *glyphs, INT32 *advances, BOOL is_sideways)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    unsigned int i;

    TRACE("%p, %u, %p, %p, %d.\n", iface, glyph_count, glyphs, advances, is_sideways);

    if (is_sideways)
        FIXME("sideways mode not supported\n");

    EnterCriticalSection(&fontface->cs);
    for (i = 0; i < glyph_count; ++i)
    {
        advances[i] = fontface_get_design_advance(fontface, DWRITE_MEASURING_MODE_NATURAL,
                fontface->metrics.designUnitsPerEm, 1.0f, NULL, glyphs[i], is_sideways);
    }
    LeaveCriticalSection(&fontface->cs);

    return S_OK;
}

static HRESULT WINAPI dwritefontface1_GetGdiCompatibleGlyphAdvances(IDWriteFontFace5 *iface,
    float em_size, float ppdip, const DWRITE_MATRIX *transform, BOOL use_gdi_natural,
    BOOL is_sideways, UINT32 glyph_count, UINT16 const *glyphs, INT32 *advances)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    DWRITE_MEASURING_MODE measuring_mode;
    UINT32 i;

    TRACE("%p, %.8e, %.8e, %p, %d, %d, %u, %p, %p.\n", iface, em_size, ppdip, transform,
        use_gdi_natural, is_sideways, glyph_count, glyphs, advances);

    if (em_size < 0.0f || ppdip <= 0.0f) {
        memset(advances, 0, sizeof(*advances) * glyph_count);
        return E_INVALIDARG;
    }

    if (em_size == 0.0f) {
        memset(advances, 0, sizeof(*advances) * glyph_count);
        return S_OK;
    }

    measuring_mode = use_gdi_natural ? DWRITE_MEASURING_MODE_GDI_NATURAL : DWRITE_MEASURING_MODE_GDI_CLASSIC;

    EnterCriticalSection(&fontface->cs);
    for (i = 0; i < glyph_count; ++i)
    {
        advances[i] = fontface_get_design_advance(fontface, measuring_mode, em_size, ppdip, transform,
                glyphs[i], is_sideways);
    }
    LeaveCriticalSection(&fontface->cs);

    return S_OK;
}

static HRESULT WINAPI dwritefontface1_GetKerningPairAdjustments(IDWriteFontFace5 *iface, UINT32 count,
    const UINT16 *glyphs, INT32 *values)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %u, %p, %p.\n", iface, count, glyphs, values);

    if (!(glyphs || values) || !count)
        return E_INVALIDARG;

    if (!glyphs || count == 1)
    {
        memset(values, 0, count * sizeof(*values));
        return E_INVALIDARG;
    }

    return opentype_get_kerning_pairs(fontface, count, glyphs, values);
}

static BOOL WINAPI dwritefontface1_HasKerningPairs(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return opentype_has_kerning_pairs(fontface);
}

static HRESULT WINAPI dwritefontface1_GetRecommendedRenderingMode(IDWriteFontFace5 *iface,
    FLOAT font_emsize, FLOAT dpiX, FLOAT dpiY, const DWRITE_MATRIX *transform, BOOL is_sideways,
    DWRITE_OUTLINE_THRESHOLD threshold, DWRITE_MEASURING_MODE measuring_mode, DWRITE_RENDERING_MODE *rendering_mode)
{
    DWRITE_GRID_FIT_MODE gridfitmode;
    return IDWriteFontFace2_GetRecommendedRenderingMode((IDWriteFontFace2 *)iface, font_emsize, dpiX, dpiY, transform,
            is_sideways, threshold, measuring_mode, NULL, rendering_mode, &gridfitmode);
}

static HRESULT WINAPI dwritefontface1_GetVerticalGlyphVariants(IDWriteFontFace5 *iface, UINT32 glyph_count,
        const UINT16 *nominal_glyphs, UINT16 *glyphs)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %u, %p, %p.\n", iface, glyph_count, nominal_glyphs, glyphs);

    return opentype_get_vertical_glyph_variants(fontface, glyph_count, nominal_glyphs, glyphs);
}

static BOOL WINAPI dwritefontface1_HasVerticalGlyphVariants(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return opentype_has_vertical_variants(fontface);
}

static BOOL WINAPI dwritefontface2_IsColorFont(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return !!(fontface->flags & FONT_IS_COLORED);
}

static UINT32 WINAPI dwritefontface2_GetColorPaletteCount(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return opentype_get_cpal_palettecount(get_fontface_cpal(fontface));
}

static UINT32 WINAPI dwritefontface2_GetPaletteEntryCount(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return opentype_get_cpal_paletteentrycount(get_fontface_cpal(fontface));
}

static HRESULT WINAPI dwritefontface2_GetPaletteEntries(IDWriteFontFace5 *iface, UINT32 palette_index,
    UINT32 first_entry_index, UINT32 entry_count, DWRITE_COLOR_F *entries)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %u, %u, %u, %p.\n", iface, palette_index, first_entry_index, entry_count, entries);

    return opentype_get_cpal_entries(get_fontface_cpal(fontface), palette_index, first_entry_index, entry_count, entries);
}

static HRESULT WINAPI dwritefontface2_GetRecommendedRenderingMode(IDWriteFontFace5 *iface, FLOAT emSize,
    FLOAT dpiX, FLOAT dpiY, DWRITE_MATRIX const *m, BOOL is_sideways, DWRITE_OUTLINE_THRESHOLD threshold,
    DWRITE_MEASURING_MODE measuringmode, IDWriteRenderingParams *params, DWRITE_RENDERING_MODE *renderingmode,
    DWRITE_GRID_FIT_MODE *gridfitmode)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    unsigned int flags;
    FLOAT emthreshold;

    TRACE("%p, %.8e, %.8e, %.8e, %p, %d, %d, %d, %p, %p, %p.\n", iface, emSize, dpiX, dpiY, m, is_sideways, threshold,
        measuringmode, params, renderingmode, gridfitmode);

    if (m && memcmp(m, &identity, sizeof(*m)))
        FIXME("transform not supported %s\n", debugstr_matrix(m));

    if (is_sideways)
        FIXME("sideways mode not supported\n");

    emSize *= max(dpiX, dpiY) / 96.0f;

    *renderingmode = DWRITE_RENDERING_MODE_DEFAULT;
    *gridfitmode = DWRITE_GRID_FIT_MODE_DEFAULT;
    if (params) {
        IDWriteRenderingParams2 *params2;
        HRESULT hr;

        hr = IDWriteRenderingParams_QueryInterface(params, &IID_IDWriteRenderingParams2, (void**)&params2);
        if (hr == S_OK) {
            *renderingmode = IDWriteRenderingParams2_GetRenderingMode(params2);
            *gridfitmode = IDWriteRenderingParams2_GetGridFitMode(params2);
            IDWriteRenderingParams2_Release(params2);
        }
        else
            *renderingmode = IDWriteRenderingParams_GetRenderingMode(params);
    }

    emthreshold = threshold == DWRITE_OUTLINE_THRESHOLD_ANTIALIASED ? RECOMMENDED_OUTLINE_AA_THRESHOLD : RECOMMENDED_OUTLINE_A_THRESHOLD;

    flags = opentype_get_gasp_flags(get_fontface_gasp(fontface), emSize);

    if (*renderingmode == DWRITE_RENDERING_MODE_DEFAULT) {
        if (emSize >= emthreshold)
            *renderingmode = DWRITE_RENDERING_MODE_OUTLINE;
        else
            *renderingmode = fontface_renderingmode_from_measuringmode(measuringmode, emSize, flags);
    }

    if (*gridfitmode == DWRITE_GRID_FIT_MODE_DEFAULT) {
        if (emSize >= emthreshold)
            *gridfitmode = DWRITE_GRID_FIT_MODE_DISABLED;
        else if (measuringmode == DWRITE_MEASURING_MODE_GDI_CLASSIC || measuringmode == DWRITE_MEASURING_MODE_GDI_NATURAL)
            *gridfitmode = DWRITE_GRID_FIT_MODE_ENABLED;
        else
            *gridfitmode = flags & (GASP_GRIDFIT|GASP_SYMMETRIC_GRIDFIT) ?
                    DWRITE_GRID_FIT_MODE_ENABLED : DWRITE_GRID_FIT_MODE_DISABLED;
    }

    return S_OK;
}

static HRESULT WINAPI dwritefontface3_GetFontFaceReference(IDWriteFontFace5 *iface,
        IDWriteFontFaceReference **reference)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, reference);

    *reference = &fontface->IDWriteFontFaceReference_iface;
    IDWriteFontFaceReference_AddRef(*reference);

    return S_OK;
}

static void WINAPI dwritefontface3_GetPanose(IDWriteFontFace5 *iface, DWRITE_PANOSE *panose)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, panose);

    *panose = fontface->panose;
}

static DWRITE_FONT_WEIGHT WINAPI dwritefontface3_GetWeight(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return fontface->weight;
}

static DWRITE_FONT_STRETCH WINAPI dwritefontface3_GetStretch(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return fontface->stretch;
}

static DWRITE_FONT_STYLE WINAPI dwritefontface3_GetStyle(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return fontface->style;
}

static HRESULT WINAPI dwritefontface3_GetFamilyNames(IDWriteFontFace5 *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, names);

    return clone_localizedstrings(fontface->family_names, names);
}

static HRESULT WINAPI dwritefontface3_GetFaceNames(IDWriteFontFace5 *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, names);

    return clone_localizedstrings(fontface->names, names);
}

static HRESULT get_font_info_strings(const struct file_stream_desc *stream_desc, IDWriteFontFile *file,
        DWRITE_INFORMATIONAL_STRING_ID stringid, IDWriteLocalizedStrings **strings_cache,
        IDWriteLocalizedStrings **ret, BOOL *exists)
{
    HRESULT hr = S_OK;

    *exists = FALSE;
    *ret = NULL;

    if (stringid > DWRITE_INFORMATIONAL_STRING_SUPPORTED_SCRIPT_LANGUAGE_TAG
            || stringid <= DWRITE_INFORMATIONAL_STRING_NONE)
    {
        return S_OK;
    }

    if (!strings_cache[stringid])
    {
        struct file_stream_desc desc = *stream_desc;

        if (!desc.stream)
            hr = get_filestream_from_file(file, &desc.stream);
        if (SUCCEEDED(hr))
            opentype_get_font_info_strings(&desc, stringid, &strings_cache[stringid]);

        if (!stream_desc->stream && desc.stream)
            IDWriteFontFileStream_Release(desc.stream);
    }

    if (SUCCEEDED(hr) && strings_cache[stringid])
    {
        hr = clone_localizedstrings(strings_cache[stringid], ret);
        if (SUCCEEDED(hr))
            *exists = TRUE;
    }

    return hr;
}

static HRESULT WINAPI dwritefontface3_GetInformationalStrings(IDWriteFontFace5 *iface,
        DWRITE_INFORMATIONAL_STRING_ID stringid, IDWriteLocalizedStrings **strings, BOOL *exists)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    struct file_stream_desc stream_desc;

    TRACE("%p, %u, %p, %p.\n", iface, stringid, strings, exists);

    stream_desc.stream = fontface->stream;
    stream_desc.face_index = fontface->index;
    stream_desc.face_type = fontface->type;
    return get_font_info_strings(&stream_desc, NULL, stringid, fontface->info_strings, strings, exists);
}

static BOOL WINAPI dwritefontface3_HasCharacter(IDWriteFontFace5 *iface, UINT32 ch)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %#x.\n", iface, ch);

    return !!dwritefontface_get_glyph(fontface, ch);
}

static HRESULT WINAPI dwritefontface3_GetRecommendedRenderingMode(IDWriteFontFace5 *iface, FLOAT emSize, FLOAT dpiX, FLOAT dpiY,
    DWRITE_MATRIX const *m, BOOL is_sideways, DWRITE_OUTLINE_THRESHOLD threshold, DWRITE_MEASURING_MODE measuring_mode,
    IDWriteRenderingParams *params, DWRITE_RENDERING_MODE1 *rendering_mode, DWRITE_GRID_FIT_MODE *gridfit_mode)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);
    unsigned int flags, mode;
    FLOAT emthreshold;

    TRACE("%p, %.8e, %.8e, %.8e, %p, %d, %d, %d, %p, %p, %p.\n", iface, emSize, dpiX, dpiY, m, is_sideways, threshold,
        measuring_mode, params, rendering_mode, gridfit_mode);

    if (m && memcmp(m, &identity, sizeof(*m)))
        FIXME("transform not supported %s\n", debugstr_matrix(m));

    if (is_sideways)
        FIXME("sideways mode not supported\n");

    emSize *= max(dpiX, dpiY) / 96.0f;

    *rendering_mode = DWRITE_RENDERING_MODE1_DEFAULT;
    *gridfit_mode = DWRITE_GRID_FIT_MODE_DEFAULT;
    if (params) {
        IDWriteRenderingParams3 *params3;
        HRESULT hr;

        hr = IDWriteRenderingParams_QueryInterface(params, &IID_IDWriteRenderingParams3, (void**)&params3);
        if (hr == S_OK) {
            mode = IDWriteRenderingParams3_GetRenderingMode1(params3);
            *gridfit_mode = IDWriteRenderingParams3_GetGridFitMode(params3);
            IDWriteRenderingParams3_Release(params3);
        }
        else
            mode = IDWriteRenderingParams_GetRenderingMode(params);
        *rendering_mode = mode;
    }

    emthreshold = threshold == DWRITE_OUTLINE_THRESHOLD_ANTIALIASED ? RECOMMENDED_OUTLINE_AA_THRESHOLD : RECOMMENDED_OUTLINE_A_THRESHOLD;

    flags = opentype_get_gasp_flags(get_fontface_gasp(fontface), emSize);

    if (*rendering_mode == DWRITE_RENDERING_MODE1_DEFAULT) {
        if (emSize >= emthreshold)
            mode = DWRITE_RENDERING_MODE1_OUTLINE;
        else
            mode = fontface_renderingmode_from_measuringmode(measuring_mode, emSize, flags);
        *rendering_mode = mode;
    }

    if (*gridfit_mode == DWRITE_GRID_FIT_MODE_DEFAULT) {
        if (emSize >= emthreshold)
            *gridfit_mode = DWRITE_GRID_FIT_MODE_DISABLED;
        else if (measuring_mode == DWRITE_MEASURING_MODE_GDI_CLASSIC || measuring_mode == DWRITE_MEASURING_MODE_GDI_NATURAL)
            *gridfit_mode = DWRITE_GRID_FIT_MODE_ENABLED;
        else
            *gridfit_mode = flags & (GASP_GRIDFIT|GASP_SYMMETRIC_GRIDFIT) ?
                    DWRITE_GRID_FIT_MODE_ENABLED : DWRITE_GRID_FIT_MODE_DISABLED;
    }

    return S_OK;
}

static BOOL WINAPI dwritefontface3_IsCharacterLocal(IDWriteFontFace5 *iface, UINT32 ch)
{
    FIXME("%p, %#x: stub\n", iface, ch);

    return FALSE;
}

static BOOL WINAPI dwritefontface3_IsGlyphLocal(IDWriteFontFace5 *iface, UINT16 glyph)
{
    FIXME("%p, %u: stub\n", iface, glyph);

    return FALSE;
}

static HRESULT WINAPI dwritefontface3_AreCharactersLocal(IDWriteFontFace5 *iface, WCHAR const *text,
    UINT32 count, BOOL enqueue_if_not, BOOL *are_local)
{
    FIXME("%p, %s:%u, %d %p: stub\n", iface, debugstr_wn(text, count), count, enqueue_if_not, are_local);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface3_AreGlyphsLocal(IDWriteFontFace5 *iface, UINT16 const *glyphs,
    UINT32 count, BOOL enqueue_if_not, BOOL *are_local)
{
    FIXME("%p, %p, %u, %d, %p: stub\n", iface, glyphs, count, enqueue_if_not, are_local);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface4_GetGlyphImageFormats_(IDWriteFontFace5 *iface, UINT16 glyph,
    UINT32 ppem_first, UINT32 ppem_last, DWRITE_GLYPH_IMAGE_FORMATS *formats)
{
    FIXME("%p, %u, %u, %u, %p: stub\n", iface, glyph, ppem_first, ppem_last, formats);

    return E_NOTIMPL;
}

static DWRITE_GLYPH_IMAGE_FORMATS WINAPI dwritefontface4_GetGlyphImageFormats(IDWriteFontFace5 *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p.\n", iface);

    return fontface->glyph_image_formats;
}

static HRESULT WINAPI dwritefontface4_GetGlyphImageData(IDWriteFontFace5 *iface, UINT16 glyph,
    UINT32 ppem, DWRITE_GLYPH_IMAGE_FORMATS format, DWRITE_GLYPH_IMAGE_DATA *data, void **context)
{
    FIXME("%p, %u, %u, %d, %p, %p: stub\n", iface, glyph, ppem, format, data, context);

    return E_NOTIMPL;
}

static void WINAPI dwritefontface4_ReleaseGlyphImageData(IDWriteFontFace5 *iface, void *context)
{
    FIXME("%p, %p: stub\n", iface, context);
}

static UINT32 WINAPI dwritefontface5_GetFontAxisValueCount(IDWriteFontFace5 *iface)
{
    FIXME("%p: stub\n", iface);

    return 0;
}

static HRESULT WINAPI dwritefontface5_GetFontAxisValues(IDWriteFontFace5 *iface, DWRITE_FONT_AXIS_VALUE *axis_values,
        UINT32 value_count)
{
    FIXME("%p, %p, %u: stub\n", iface, axis_values, value_count);

    return E_NOTIMPL;
}

static BOOL WINAPI dwritefontface5_HasVariations(IDWriteFontFace5 *iface)
{
    static int once;

    if (!once++)
        FIXME("%p: stub\n", iface);

    return FALSE;
}

static HRESULT WINAPI dwritefontface5_GetFontResource(IDWriteFontFace5 *iface, IDWriteFontResource **resource)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface);

    TRACE("%p, %p.\n", iface, resource);

    return IDWriteFactory7_CreateFontResource(fontface->factory, fontface->file, fontface->index, resource);
}

static BOOL WINAPI dwritefontface5_Equals(IDWriteFontFace5 *iface, IDWriteFontFace *other)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace5(iface), *other_face;

    TRACE("%p, %p.\n", iface, other);

    if (!(other_face = unsafe_impl_from_IDWriteFontFace(other)))
        return FALSE;

    /* TODO: add variations support */

    return fontface->index == other_face->index &&
            fontface->simulations == other_face->simulations &&
            is_same_fontfile(fontface->file, other_face->file);
}

static const IDWriteFontFace5Vtbl dwritefontfacevtbl =
{
    dwritefontface_QueryInterface,
    dwritefontface_AddRef,
    dwritefontface_Release,
    dwritefontface_GetType,
    dwritefontface_GetFiles,
    dwritefontface_GetIndex,
    dwritefontface_GetSimulations,
    dwritefontface_IsSymbolFont,
    dwritefontface_GetMetrics,
    dwritefontface_GetGlyphCount,
    dwritefontface_GetDesignGlyphMetrics,
    dwritefontface_GetGlyphIndices,
    dwritefontface_TryGetFontTable,
    dwritefontface_ReleaseFontTable,
    dwritefontface_GetGlyphRunOutline,
    dwritefontface_GetRecommendedRenderingMode,
    dwritefontface_GetGdiCompatibleMetrics,
    dwritefontface_GetGdiCompatibleGlyphMetrics,
    dwritefontface1_GetMetrics,
    dwritefontface1_GetGdiCompatibleMetrics,
    dwritefontface1_GetCaretMetrics,
    dwritefontface1_GetUnicodeRanges,
    dwritefontface1_IsMonospacedFont,
    dwritefontface1_GetDesignGlyphAdvances,
    dwritefontface1_GetGdiCompatibleGlyphAdvances,
    dwritefontface1_GetKerningPairAdjustments,
    dwritefontface1_HasKerningPairs,
    dwritefontface1_GetRecommendedRenderingMode,
    dwritefontface1_GetVerticalGlyphVariants,
    dwritefontface1_HasVerticalGlyphVariants,
    dwritefontface2_IsColorFont,
    dwritefontface2_GetColorPaletteCount,
    dwritefontface2_GetPaletteEntryCount,
    dwritefontface2_GetPaletteEntries,
    dwritefontface2_GetRecommendedRenderingMode,
    dwritefontface3_GetFontFaceReference,
    dwritefontface3_GetPanose,
    dwritefontface3_GetWeight,
    dwritefontface3_GetStretch,
    dwritefontface3_GetStyle,
    dwritefontface3_GetFamilyNames,
    dwritefontface3_GetFaceNames,
    dwritefontface3_GetInformationalStrings,
    dwritefontface3_HasCharacter,
    dwritefontface3_GetRecommendedRenderingMode,
    dwritefontface3_IsCharacterLocal,
    dwritefontface3_IsGlyphLocal,
    dwritefontface3_AreCharactersLocal,
    dwritefontface3_AreGlyphsLocal,
    dwritefontface4_GetGlyphImageFormats_,
    dwritefontface4_GetGlyphImageFormats,
    dwritefontface4_GetGlyphImageData,
    dwritefontface4_ReleaseGlyphImageData,
    dwritefontface5_GetFontAxisValueCount,
    dwritefontface5_GetFontAxisValues,
    dwritefontface5_HasVariations,
    dwritefontface5_GetFontResource,
    dwritefontface5_Equals,
};

static HRESULT WINAPI dwritefontface_reference_QueryInterface(IDWriteFontFaceReference *iface, REFIID riid, void **obj)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFaceReference(iface);
    return IDWriteFontFace5_QueryInterface(&fontface->IDWriteFontFace5_iface, riid, obj);
}

static ULONG WINAPI dwritefontface_reference_AddRef(IDWriteFontFaceReference *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFaceReference(iface);
    return IDWriteFontFace5_AddRef(&fontface->IDWriteFontFace5_iface);
}

static ULONG WINAPI dwritefontface_reference_Release(IDWriteFontFaceReference *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFaceReference(iface);
    return IDWriteFontFace5_Release(&fontface->IDWriteFontFace5_iface);
}

static HRESULT WINAPI dwritefontface_reference_CreateFontFace(IDWriteFontFaceReference *iface,
        IDWriteFontFace3 **ret)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFaceReference(iface);

    TRACE("%p, %p.\n", iface, ret);

    *ret = (IDWriteFontFace3 *)&fontface->IDWriteFontFace5_iface;
    IDWriteFontFace3_AddRef(*ret);

    return S_OK;
}

static HRESULT WINAPI dwritefontface_reference_CreateFontFaceWithSimulations(IDWriteFontFaceReference *iface,
        DWRITE_FONT_SIMULATIONS simulations, IDWriteFontFace3 **ret)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFaceReference(iface);
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFace *face;
    BOOL is_supported;
    UINT32 face_num;
    HRESULT hr;

    TRACE("%p, %#x, %p.\n", iface, simulations, ret);

    hr = IDWriteFontFile_Analyze(fontface->file, &is_supported, &file_type, &face_type, &face_num);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFactory7_CreateFontFace(fontface->factory, face_type, 1, &fontface->file, fontface->index,
            simulations, &face);
    if (SUCCEEDED(hr))
    {
        hr = IDWriteFontFace_QueryInterface(face, &IID_IDWriteFontFace3, (void **)ret);
        IDWriteFontFace_Release(face);
    }

    return hr;
}

static BOOL WINAPI dwritefontface_reference_Equals(IDWriteFontFaceReference *iface, IDWriteFontFaceReference *ref)
{
    FIXME("%p, %p.\n", iface, ref);

    return E_NOTIMPL;
}

static UINT32 WINAPI dwritefontface_reference_GetFontFaceIndex(IDWriteFontFaceReference *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFaceReference(iface);

    TRACE("%p.\n", iface);

    return fontface->index;
}

static DWRITE_FONT_SIMULATIONS WINAPI dwritefontface_reference_GetSimulations(IDWriteFontFaceReference *iface)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFaceReference(iface);

    TRACE("%p.\n", iface);

    return fontface->simulations;
}

static HRESULT WINAPI dwritefontface_reference_GetFontFile(IDWriteFontFaceReference *iface, IDWriteFontFile **file)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFaceReference(iface);

    TRACE("%p, %p.\n", iface, file);

    *file = fontface->file;
    IDWriteFontFile_AddRef(*file);

    return S_OK;
}

static UINT64 WINAPI dwritefontface_reference_GetLocalFileSize(IDWriteFontFaceReference *iface)
{
    FIXME("%p.\n", iface);

    return 0;
}

static UINT64 WINAPI dwritefontface_reference_GetFileSize(IDWriteFontFaceReference *iface)
{
    FIXME("%p.\n", iface);

    return 0;
}

static HRESULT WINAPI dwritefontface_reference_GetFileTime(IDWriteFontFaceReference *iface, FILETIME *writetime)
{
    FIXME("%p, %p.\n", iface, writetime);

    return E_NOTIMPL;
}

static DWRITE_LOCALITY WINAPI dwritefontface_reference_GetLocality(IDWriteFontFaceReference *iface)
{
    FIXME("%p.\n", iface);

    return DWRITE_LOCALITY_LOCAL;
}

static HRESULT WINAPI dwritefontface_reference_EnqueueFontDownloadRequest(IDWriteFontFaceReference *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_reference_EnqueueCharacterDownloadRequest(IDWriteFontFaceReference *iface,
        WCHAR const *chars, UINT32 count)
{
    FIXME("%p, %s, %u.\n", iface, debugstr_wn(chars, count), count);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_reference_EnqueueGlyphDownloadRequest(IDWriteFontFaceReference *iface,
        UINT16 const *glyphs, UINT32 count)
{
    FIXME("%p, %p, %u.\n", iface, glyphs, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_reference_EnqueueFileFragmentDownloadRequest(IDWriteFontFaceReference *iface,
        UINT64 offset, UINT64 size)
{
    FIXME("%p, 0x%s, 0x%s.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(size));

    return E_NOTIMPL;
}

static const IDWriteFontFaceReferenceVtbl dwritefontface_reference_vtbl =
{
    dwritefontface_reference_QueryInterface,
    dwritefontface_reference_AddRef,
    dwritefontface_reference_Release,
    dwritefontface_reference_CreateFontFace,
    dwritefontface_reference_CreateFontFaceWithSimulations,
    dwritefontface_reference_Equals,
    dwritefontface_reference_GetFontFaceIndex,
    dwritefontface_reference_GetSimulations,
    dwritefontface_reference_GetFontFile,
    dwritefontface_reference_GetLocalFileSize,
    dwritefontface_reference_GetFileSize,
    dwritefontface_reference_GetFileTime,
    dwritefontface_reference_GetLocality,
    dwritefontface_reference_EnqueueFontDownloadRequest,
    dwritefontface_reference_EnqueueCharacterDownloadRequest,
    dwritefontface_reference_EnqueueGlyphDownloadRequest,
    dwritefontface_reference_EnqueueFileFragmentDownloadRequest,
};

static HRESULT get_fontface_from_font(struct dwrite_font *font, IDWriteFontFace5 **fontface)
{
    struct dwrite_font_data *data = font->data;
    struct fontface_desc desc;
    struct list *cached_list;
    HRESULT hr;

    *fontface = NULL;

    hr = factory_get_cached_fontface(font->family->collection->factory, &data->file, data->face_index,
            font->data->simulations, &cached_list, &IID_IDWriteFontFace4, (void **)fontface);
    if (hr == S_OK)
        return hr;

    if (FAILED(hr = get_filestream_from_file(data->file, &desc.stream)))
        return hr;

    desc.factory = font->family->collection->factory;
    desc.face_type = data->face_type;
    desc.file = data->file;
    desc.index = data->face_index;
    desc.simulations = data->simulations;
    desc.font_data = data;
    hr = create_fontface(&desc, cached_list, fontface);

    IDWriteFontFileStream_Release(desc.stream);
    return hr;
}

static HRESULT WINAPI dwritefont_QueryInterface(IDWriteFont3 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFont3) ||
        IsEqualIID(riid, &IID_IDWriteFont2) ||
        IsEqualIID(riid, &IID_IDWriteFont1) ||
        IsEqualIID(riid, &IID_IDWriteFont)  ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFont3_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefont_AddRef(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);
    ULONG refcount = InterlockedIncrement(&font->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritefont_Release(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);
    ULONG refcount = InterlockedDecrement(&font->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFontFamily2_Release(&font->family->IDWriteFontFamily2_iface);
        release_font_data(font->data);
        free(font);
    }

    return refcount;
}

static HRESULT WINAPI dwritefont_GetFontFamily(IDWriteFont3 *iface, IDWriteFontFamily **family)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %p.\n", iface, family);

    *family = (IDWriteFontFamily *)&font->family->IDWriteFontFamily2_iface;
    IDWriteFontFamily_AddRef(*family);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritefont_GetWeight(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p.\n", iface);

    return font->data->weight;
}

static DWRITE_FONT_STRETCH WINAPI dwritefont_GetStretch(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p.\n", iface);

    return font->data->stretch;
}

static DWRITE_FONT_STYLE WINAPI dwritefont_GetStyle(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p.\n", iface);

    return font->style;
}

static BOOL WINAPI dwritefont_IsSymbolFont(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p.\n", iface);

    return !!(font->data->flags & FONT_IS_SYMBOL);
}

static HRESULT WINAPI dwritefont_GetFaceNames(IDWriteFont3 *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %p.\n", iface, names);

    return clone_localizedstrings(font->data->names, names);
}

static HRESULT WINAPI dwritefont_GetInformationalStrings(IDWriteFont3 *iface,
    DWRITE_INFORMATIONAL_STRING_ID stringid, IDWriteLocalizedStrings **strings, BOOL *exists)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);
    struct dwrite_font_data *data = font->data;
    struct file_stream_desc stream_desc;

    TRACE("%p, %d, %p, %p.\n", iface, stringid, strings, exists);

    /* Stream will be created if necessary. */
    stream_desc.stream = NULL;
    stream_desc.face_index = data->face_index;
    stream_desc.face_type = data->face_type;
    return get_font_info_strings(&stream_desc, data->file, stringid, data->info_strings, strings, exists);
}

static DWRITE_FONT_SIMULATIONS WINAPI dwritefont_GetSimulations(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p.\n", iface);

    return font->data->simulations;
}

static void WINAPI dwritefont_GetMetrics(IDWriteFont3 *iface, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %p.\n", iface, metrics);

    memcpy(metrics, &font->data->metrics, sizeof(*metrics));
}

static BOOL dwritefont_has_character(struct dwrite_font *font, UINT32 ch)
{
    UINT16 glyph;
    dwrite_cmap_init(&font->data->cmap, font->data->file, font->data->face_index, font->data->face_type);
    glyph = opentype_cmap_get_glyph(&font->data->cmap, ch);
    return glyph != 0;
}

static HRESULT WINAPI dwritefont_HasCharacter(IDWriteFont3 *iface, UINT32 ch, BOOL *exists)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %#x, %p.\n", iface, ch, exists);

    *exists = dwritefont_has_character(font, ch);

    return S_OK;
}

static HRESULT WINAPI dwritefont_CreateFontFace(IDWriteFont3 *iface, IDWriteFontFace **fontface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %p.\n", iface, fontface);

    return get_fontface_from_font(font, (IDWriteFontFace5 **)fontface);
}

static void WINAPI dwritefont1_GetMetrics(IDWriteFont3 *iface, DWRITE_FONT_METRICS1 *metrics)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %p.\n", iface, metrics);

    *metrics = font->data->metrics;
}

static void WINAPI dwritefont1_GetPanose(IDWriteFont3 *iface, DWRITE_PANOSE *panose)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %p.\n", iface, panose);

    *panose = font->data->panose;
}

static HRESULT WINAPI dwritefont1_GetUnicodeRanges(IDWriteFont3 *iface, UINT32 max_count, DWRITE_UNICODE_RANGE *ranges,
        UINT32 *count)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %u, %p, %p.\n", iface, max_count, ranges, count);

    *count = 0;
    if (max_count && !ranges)
        return E_INVALIDARG;

    dwrite_cmap_init(&font->data->cmap, font->data->file, font->data->face_index, font->data->face_type);
    return opentype_cmap_get_unicode_ranges(&font->data->cmap, max_count, ranges, count);
}

static BOOL WINAPI dwritefont1_IsMonospacedFont(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p.\n", iface);

    return !!(font->data->flags & FONT_IS_MONOSPACED);
}

static BOOL WINAPI dwritefont2_IsColorFont(IDWriteFont3 *iface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p.\n", iface);

    return !!(font->data->flags & FONT_IS_COLORED);
}

static HRESULT WINAPI dwritefont3_CreateFontFace(IDWriteFont3 *iface, IDWriteFontFace3 **fontface)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %p.\n", iface, fontface);

    return get_fontface_from_font(font, (IDWriteFontFace5 **)fontface);
}

static BOOL WINAPI dwritefont3_Equals(IDWriteFont3 *iface, IDWriteFont *other)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface), *other_font;

    TRACE("%p, %p.\n", iface, other);

    if (!(other_font = unsafe_impl_from_IDWriteFont(other)))
        return FALSE;

    return font->data->face_index == other_font->data->face_index
            && font->data->simulations == other_font->data->simulations
            && is_same_fontfile(font->data->file, other_font->data->file);
}

static HRESULT WINAPI dwritefont3_GetFontFaceReference(IDWriteFont3 *iface, IDWriteFontFaceReference **reference)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %p.\n", iface, reference);

    return IDWriteFactory7_CreateFontFaceReference(font->family->collection->factory, font->data->file,
            font->data->face_index, font->data->simulations, font->data->axis, ARRAY_SIZE(font->data->axis),
            (IDWriteFontFaceReference1 **)reference);
}

static BOOL WINAPI dwritefont3_HasCharacter(IDWriteFont3 *iface, UINT32 ch)
{
    struct dwrite_font *font = impl_from_IDWriteFont3(iface);

    TRACE("%p, %#x.\n", iface, ch);

    return dwritefont_has_character(font, ch);
}

static DWRITE_LOCALITY WINAPI dwritefont3_GetLocality(IDWriteFont3 *iface)
{
    FIXME("%p: stub.\n", iface);

    return DWRITE_LOCALITY_LOCAL;
}

static const IDWriteFont3Vtbl dwritefontvtbl = {
    dwritefont_QueryInterface,
    dwritefont_AddRef,
    dwritefont_Release,
    dwritefont_GetFontFamily,
    dwritefont_GetWeight,
    dwritefont_GetStretch,
    dwritefont_GetStyle,
    dwritefont_IsSymbolFont,
    dwritefont_GetFaceNames,
    dwritefont_GetInformationalStrings,
    dwritefont_GetSimulations,
    dwritefont_GetMetrics,
    dwritefont_HasCharacter,
    dwritefont_CreateFontFace,
    dwritefont1_GetMetrics,
    dwritefont1_GetPanose,
    dwritefont1_GetUnicodeRanges,
    dwritefont1_IsMonospacedFont,
    dwritefont2_IsColorFont,
    dwritefont3_CreateFontFace,
    dwritefont3_Equals,
    dwritefont3_GetFontFaceReference,
    dwritefont3_HasCharacter,
    dwritefont3_GetLocality
};

static struct dwrite_font *unsafe_impl_from_IDWriteFont(IDWriteFont *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (IDWriteFontVtbl*)&dwritefontvtbl);
    return CONTAINING_RECORD(iface, struct dwrite_font, IDWriteFont3_iface);
}

struct dwrite_fontface *unsafe_impl_from_IDWriteFontFace(IDWriteFontFace *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (IDWriteFontFaceVtbl*)&dwritefontfacevtbl);
    return CONTAINING_RECORD(iface, struct dwrite_fontface, IDWriteFontFace5_iface);
}

static struct dwrite_fontfacereference *unsafe_impl_from_IDWriteFontFaceReference(IDWriteFontFaceReference *iface)
{
    if (!iface)
        return NULL;
    if (iface->lpVtbl != (IDWriteFontFaceReferenceVtbl *)&fontfacereferencevtbl)
        return NULL;
    return CONTAINING_RECORD((IDWriteFontFaceReference1 *)iface, struct dwrite_fontfacereference,
            IDWriteFontFaceReference1_iface);
}

void get_logfont_from_font(IDWriteFont *iface, LOGFONTW *lf)
{
    struct dwrite_font *font = unsafe_impl_from_IDWriteFont(iface);
    *lf = font->data->lf;
}

void get_logfont_from_fontface(IDWriteFontFace *iface, LOGFONTW *lf)
{
    struct dwrite_fontface *fontface = unsafe_impl_from_IDWriteFontFace(iface);
    *lf = fontface->lf;
}

HRESULT get_fontsig_from_font(IDWriteFont *iface, FONTSIGNATURE *fontsig)
{
    struct dwrite_font *font = unsafe_impl_from_IDWriteFont(iface);
    *fontsig = font->data->fontsig;
    return S_OK;
}

HRESULT get_fontsig_from_fontface(IDWriteFontFace *iface, FONTSIGNATURE *fontsig)
{
    struct dwrite_fontface *fontface = unsafe_impl_from_IDWriteFontFace(iface);
    *fontsig = fontface->fontsig;
    return S_OK;
}

static HRESULT create_font(struct dwrite_fontfamily *family, UINT32 index, IDWriteFont3 **font)
{
    struct dwrite_font *object;

    *font = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteFont3_iface.lpVtbl = &dwritefontvtbl;
    object->refcount = 1;
    object->family = family;
    IDWriteFontFamily2_AddRef(&family->IDWriteFontFamily2_iface);
    object->data = family->data->fonts[index];
    object->style = object->data->style;
    addref_font_data(object->data);

    *font = &object->IDWriteFont3_iface;

    return S_OK;
}

/* IDWriteFontList2 */
static HRESULT WINAPI dwritefontlist_QueryInterface(IDWriteFontList2 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontList2) ||
        IsEqualIID(riid, &IID_IDWriteFontList1) ||
        IsEqualIID(riid, &IID_IDWriteFontList) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFontList2_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontlist_AddRef(IDWriteFontList2 *iface)
{
    struct dwrite_fontlist *fontlist = impl_from_IDWriteFontList2(iface);
    ULONG refcount = InterlockedIncrement(&fontlist->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritefontlist_Release(IDWriteFontList2 *iface)
{
    struct dwrite_fontlist *fontlist = impl_from_IDWriteFontList2(iface);
    ULONG refcount = InterlockedDecrement(&fontlist->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        unsigned int i;

        for (i = 0; i < fontlist->font_count; i++)
            release_font_data(fontlist->fonts[i]);
        IDWriteFontFamily2_Release(&fontlist->family->IDWriteFontFamily2_iface);
        free(fontlist->fonts);
        free(fontlist);
    }

    return refcount;
}

static HRESULT WINAPI dwritefontlist_GetFontCollection(IDWriteFontList2 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_fontlist *fontlist = impl_from_IDWriteFontList2(iface);
    return IDWriteFontFamily2_GetFontCollection(&fontlist->family->IDWriteFontFamily2_iface, collection);
}

static UINT32 WINAPI dwritefontlist_GetFontCount(IDWriteFontList2 *iface)
{
    struct dwrite_fontlist *fontlist = impl_from_IDWriteFontList2(iface);

    TRACE("%p.\n", iface);

    return fontlist->font_count;
}

static HRESULT WINAPI dwritefontlist_GetFont(IDWriteFontList2 *iface, UINT32 index, IDWriteFont **font)
{
    struct dwrite_fontlist *fontlist = impl_from_IDWriteFontList2(iface);

    TRACE("%p, %u, %p.\n", iface, index, font);

    *font = NULL;

    if (fontlist->font_count == 0)
        return S_FALSE;

    if (index >= fontlist->font_count)
        return E_INVALIDARG;

    return create_font(fontlist->family, index, (IDWriteFont3 **)font);
}

static DWRITE_LOCALITY WINAPI dwritefontlist1_GetFontLocality(IDWriteFontList2 *iface, UINT32 index)
{
    FIXME("%p, %u.\n", iface, index);

    return DWRITE_LOCALITY_LOCAL;
}

static HRESULT fontlist_get_font(const struct dwrite_fontlist *fontlist, unsigned int index,
        IDWriteFont3 **font)
{
    *font = NULL;

    if (fontlist->font_count == 0)
        return S_FALSE;

    if (index >= fontlist->font_count)
        return E_FAIL;

    return create_font(fontlist->family, index, font);
}

static HRESULT WINAPI dwritefontlist1_GetFont(IDWriteFontList2 *iface, UINT32 index, IDWriteFont3 **font)
{
    struct dwrite_fontlist *fontlist = impl_from_IDWriteFontList2(iface);

    TRACE("%p, %u, %p.\n", iface, index, font);

    return fontlist_get_font(fontlist, index, font);
}

static HRESULT WINAPI dwritefontlist1_GetFontFaceReference(IDWriteFontList2 *iface, UINT32 index,
    IDWriteFontFaceReference **reference)
{
    struct dwrite_fontlist *fontlist = impl_from_IDWriteFontList2(iface);
    IDWriteFont3 *font;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, index, reference);

    *reference = NULL;

    if (SUCCEEDED(hr = fontlist_get_font(fontlist, index, &font)))
    {
        hr = IDWriteFont3_GetFontFaceReference(font, reference);
        IDWriteFont3_Release(font);
    }

    return hr;
}

static HRESULT WINAPI dwritefontlist2_GetFontSet(IDWriteFontList2 *iface, IDWriteFontSet1 **fontset)
{
    struct dwrite_fontlist *fontlist = impl_from_IDWriteFontList2(iface);

    TRACE("%p, %p.\n", iface, fontset);

    return fontset_create_from_font_data(fontlist->family->collection->factory, fontlist->fonts,
            fontlist->font_count, fontset);
}

static const IDWriteFontList2Vtbl dwritefontlistvtbl =
{
    dwritefontlist_QueryInterface,
    dwritefontlist_AddRef,
    dwritefontlist_Release,
    dwritefontlist_GetFontCollection,
    dwritefontlist_GetFontCount,
    dwritefontlist_GetFont,
    dwritefontlist1_GetFontLocality,
    dwritefontlist1_GetFont,
    dwritefontlist1_GetFontFaceReference,
    dwritefontlist2_GetFontSet,
};

static HRESULT WINAPI dwritefontfamily_QueryInterface(IDWriteFontFamily2 *iface, REFIID riid, void **obj)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontFamily2) ||
        IsEqualIID(riid, &IID_IDWriteFontFamily1) ||
        IsEqualIID(riid, &IID_IDWriteFontFamily) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IDWriteFontList2) ||
        IsEqualIID(riid, &IID_IDWriteFontList1) ||
        IsEqualIID(riid, &IID_IDWriteFontList))
    {
        *obj = &family->IDWriteFontList2_iface;
    }
    else
    {
        WARN("%s not implemented.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI dwritefontfamily_AddRef(IDWriteFontFamily2 *iface)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);
    ULONG refcount = InterlockedIncrement(&family->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritefontfamily_Release(IDWriteFontFamily2 *iface)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);
    ULONG refcount = InterlockedDecrement(&family->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFontCollection3_Release(&family->collection->IDWriteFontCollection3_iface);
        release_fontfamily_data(family->data);
        free(family);
    }

    return refcount;
}

static HRESULT WINAPI dwritefontfamily_GetFontCollection(IDWriteFontFamily2 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);

    TRACE("%p, %p.\n", iface, collection);

    *collection = (IDWriteFontCollection *)&family->collection->IDWriteFontCollection3_iface;
    IDWriteFontCollection_AddRef(*collection);
    return S_OK;
}

static UINT32 WINAPI dwritefontfamily_GetFontCount(IDWriteFontFamily2 *iface)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);

    TRACE("%p.\n", iface);

    return family->data->count;
}

static HRESULT WINAPI dwritefontfamily_GetFont(IDWriteFontFamily2 *iface, UINT32 index, IDWriteFont **font)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);

    TRACE("%p, %u, %p.\n", iface, index, font);

    *font = NULL;

    if (!family->data->count)
        return S_FALSE;

    if (index >= family->data->count)
        return E_INVALIDARG;

    return create_font(family, index, (IDWriteFont3 **)font);
}

static HRESULT WINAPI dwritefontfamily_GetFamilyNames(IDWriteFontFamily2 *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);

    TRACE("%p, %p.\n", iface, names);

    return clone_localizedstrings(family->data->familyname, names);
}

static BOOL is_better_font_match(const struct dwrite_font_propvec *next, const struct dwrite_font_propvec *cur,
    const struct dwrite_font_propvec *req)
{
    FLOAT cur_to_req = get_font_prop_vec_distance(cur, req);
    FLOAT next_to_req = get_font_prop_vec_distance(next, req);
    FLOAT cur_req_prod, next_req_prod;

    if (next_to_req < cur_to_req)
        return TRUE;

    if (next_to_req > cur_to_req)
        return FALSE;

    cur_req_prod = get_font_prop_vec_dotproduct(cur, req);
    next_req_prod = get_font_prop_vec_dotproduct(next, req);

    if (next_req_prod > cur_req_prod)
        return TRUE;

    if (next_req_prod < cur_req_prod)
        return FALSE;

    if (next->stretch > cur->stretch)
        return TRUE;
    if (next->stretch < cur->stretch)
        return FALSE;

    if (next->style > cur->style)
        return TRUE;
    if (next->style < cur->style)
        return FALSE;

    if (next->weight > cur->weight)
        return TRUE;
    if (next->weight < cur->weight)
        return FALSE;

    /* full match, no reason to prefer new variant */
    return FALSE;
}

static HRESULT WINAPI dwritefontfamily_GetFirstMatchingFont(IDWriteFontFamily2 *iface, DWRITE_FONT_WEIGHT weight,
        DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFont **font)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);
    struct dwrite_font_propvec req;
    size_t i, match;

    TRACE("%p, %d, %d, %d, %p.\n", iface, weight, stretch, style, font);

    if (!family->data->count)
    {
        *font = NULL;
        return DWRITE_E_NOFONT;
    }

    init_font_prop_vec(weight, stretch, style, &req);
    match = 0;

    for (i = 1; i < family->data->count; ++i)
    {
        if (is_better_font_match(&family->data->fonts[i]->propvec, &family->data->fonts[match]->propvec, &req))
            match = i;
    }

    return create_font(family, match, (IDWriteFont3 **)font);
}

typedef BOOL (*matching_filter_func)(const struct dwrite_font_data*);

static BOOL is_font_acceptable_for_normal(const struct dwrite_font_data *font)
{
    return font->style == DWRITE_FONT_STYLE_NORMAL || font->style == DWRITE_FONT_STYLE_ITALIC;
}

static BOOL is_font_acceptable_for_oblique_italic(const struct dwrite_font_data *font)
{
    return font->style == DWRITE_FONT_STYLE_OBLIQUE || font->style == DWRITE_FONT_STYLE_ITALIC;
}

static void matchingfonts_sort(struct dwrite_fontlist *fonts, const struct dwrite_font_propvec *req)
{
    UINT32 b = fonts->font_count - 1, j, t;

    while (1) {
        t = b;

        for (j = 0; j < b; j++) {
            if (is_better_font_match(&fonts->fonts[j+1]->propvec, &fonts->fonts[j]->propvec, req)) {
                struct dwrite_font_data *s = fonts->fonts[j];
                fonts->fonts[j] = fonts->fonts[j+1];
                fonts->fonts[j+1] = s;
                t = j;
            }
        }

        if (t == b)
            break;
        b = t;
    };
}

static HRESULT WINAPI dwritefontfamily_GetMatchingFonts(IDWriteFontFamily2 *iface, DWRITE_FONT_WEIGHT weight,
        DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFontList **ret)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);
    matching_filter_func func = NULL;
    struct dwrite_font_propvec req;
    struct dwrite_fontlist *fonts;
    size_t i;

    TRACE("%p, %d, %d, %d, %p.\n", iface, weight, stretch, style, ret);

    *ret = NULL;

    if (!(fonts = malloc(sizeof(*fonts))))
        return E_OUTOFMEMORY;

    /* Allocate as many as family has, not all of them will be necessary used. */
    if (!(fonts->fonts = calloc(family->data->count, sizeof(*fonts->fonts))))
    {
        free(fonts);
        return E_OUTOFMEMORY;
    }

    fonts->IDWriteFontList2_iface.lpVtbl = &dwritefontlistvtbl;
    fonts->refcount = 1;
    fonts->family = family;
    IDWriteFontFamily2_AddRef(&fonts->family->IDWriteFontFamily2_iface);
    fonts->font_count = 0;

    /* Normal style accepts Normal or Italic, Oblique and Italic - both Oblique and Italic styles */
    if (style == DWRITE_FONT_STYLE_NORMAL) {
        if (family->data->has_normal_face || family->data->has_italic_face)
            func = is_font_acceptable_for_normal;
    }
    else /* requested oblique or italic */ {
        if (family->data->has_oblique_face || family->data->has_italic_face)
            func = is_font_acceptable_for_oblique_italic;
    }

    for (i = 0; i < family->data->count; ++i)
    {
        if (!func || func(family->data->fonts[i]))
        {
            fonts->fonts[fonts->font_count++] = addref_font_data(family->data->fonts[i]);
        }
    }

    /* now potential matches are sorted using same criteria GetFirstMatchingFont uses */
    init_font_prop_vec(weight, stretch, style, &req);
    matchingfonts_sort(fonts, &req);

    *ret = (IDWriteFontList *)&fonts->IDWriteFontList2_iface;
    return S_OK;
}

static DWRITE_LOCALITY WINAPI dwritefontfamily1_GetFontLocality(IDWriteFontFamily2 *iface, UINT32 index)
{
    FIXME("%p, %u.\n", iface, index);

    return DWRITE_LOCALITY_LOCAL;
}

static HRESULT WINAPI dwritefontfamily1_GetFont(IDWriteFontFamily2 *iface, UINT32 index, IDWriteFont3 **font)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);

    TRACE("%p, %u, %p.\n", iface, index, font);

    *font = NULL;

    if (!family->data->count)
        return S_FALSE;

    if (index >= family->data->count)
        return E_FAIL;

    return create_font(family, index, font);
}

static HRESULT WINAPI dwritefontfamily1_GetFontFaceReference(IDWriteFontFamily2 *iface, UINT32 index,
        IDWriteFontFaceReference **reference)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);
    const struct dwrite_font_data *font;

    TRACE("%p, %u, %p.\n", iface, index, reference);

    *reference = NULL;

    if (index >= family->data->count)
        return E_FAIL;

    font = family->data->fonts[index];
    return IDWriteFactory5_CreateFontFaceReference_((IDWriteFactory5 *)family->collection->factory,
            font->file, font->face_index, font->simulations, reference);
}

static HRESULT WINAPI dwritefontfamily2_GetMatchingFonts(IDWriteFontFamily2 *iface,
        DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_values, IDWriteFontList2 **fontlist)
{
    FIXME("%p, %p, %u, %p.\n", iface, axis_values, num_values, fontlist);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontfamily2_GetFontSet(IDWriteFontFamily2 *iface, IDWriteFontSet1 **fontset)
{
    struct dwrite_fontfamily *family = impl_from_IDWriteFontFamily2(iface);

    TRACE("%p, %p.\n", iface, fontset);

    return fontset_create_from_font_data(family->collection->factory, family->data->fonts,
            family->data->count, fontset);
}

static const IDWriteFontFamily2Vtbl fontfamilyvtbl =
{
    dwritefontfamily_QueryInterface,
    dwritefontfamily_AddRef,
    dwritefontfamily_Release,
    dwritefontfamily_GetFontCollection,
    dwritefontfamily_GetFontCount,
    dwritefontfamily_GetFont,
    dwritefontfamily_GetFamilyNames,
    dwritefontfamily_GetFirstMatchingFont,
    dwritefontfamily_GetMatchingFonts,
    dwritefontfamily1_GetFontLocality,
    dwritefontfamily1_GetFont,
    dwritefontfamily1_GetFontFaceReference,
    dwritefontfamily2_GetMatchingFonts,
    dwritefontfamily2_GetFontSet,
};

static HRESULT WINAPI dwritefontfamilylist_QueryInterface(IDWriteFontList2 *iface, REFIID riid, void **obj)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily_QueryInterface(&family->IDWriteFontFamily2_iface, riid, obj);
}

static ULONG WINAPI dwritefontfamilylist_AddRef(IDWriteFontList2 *iface)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily_AddRef(&family->IDWriteFontFamily2_iface);
}

static ULONG WINAPI dwritefontfamilylist_Release(IDWriteFontList2 *iface)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily_Release(&family->IDWriteFontFamily2_iface);
}

static HRESULT WINAPI dwritefontfamilylist_GetFontCollection(IDWriteFontList2 *iface,
        IDWriteFontCollection **collection)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily_GetFontCollection(&family->IDWriteFontFamily2_iface, collection);
}

static UINT32 WINAPI dwritefontfamilylist_GetFontCount(IDWriteFontList2 *iface)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily_GetFontCount(&family->IDWriteFontFamily2_iface);
}

static HRESULT WINAPI dwritefontfamilylist_GetFont(IDWriteFontList2 *iface, UINT32 index, IDWriteFont **font)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily_GetFont(&family->IDWriteFontFamily2_iface, index, font);
}

static DWRITE_LOCALITY WINAPI dwritefontfamilylist1_GetFontLocality(IDWriteFontList2 *iface, UINT32 index)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily1_GetFontLocality(&family->IDWriteFontFamily2_iface, index);
}

static HRESULT WINAPI dwritefontfamilylist1_GetFont(IDWriteFontList2 *iface, UINT32 index, IDWriteFont3 **font)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily1_GetFont(&family->IDWriteFontFamily2_iface, index, font);
}

static HRESULT WINAPI dwritefontfamilylist1_GetFontFaceReference(IDWriteFontList2 *iface, UINT32 index,
        IDWriteFontFaceReference **reference)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);
    return dwritefontfamily1_GetFontFaceReference(&family->IDWriteFontFamily2_iface, index, reference);
}

static HRESULT WINAPI dwritefontfamilylist2_GetFontSet(IDWriteFontList2 *iface, IDWriteFontSet1 **fontset)
{
    struct dwrite_fontfamily *family = impl_family_from_IDWriteFontList2(iface);

    TRACE("%p, %p.\n", iface, fontset);

    return fontset_create_from_font_data(family->collection->factory, family->data->fonts,
            family->data->count, fontset);
}

static const IDWriteFontList2Vtbl fontfamilylistvtbl =
{
    dwritefontfamilylist_QueryInterface,
    dwritefontfamilylist_AddRef,
    dwritefontfamilylist_Release,
    dwritefontfamilylist_GetFontCollection,
    dwritefontfamilylist_GetFontCount,
    dwritefontfamilylist_GetFont,
    dwritefontfamilylist1_GetFontLocality,
    dwritefontfamilylist1_GetFont,
    dwritefontfamilylist1_GetFontFaceReference,
    dwritefontfamilylist2_GetFontSet,
};

static HRESULT create_fontfamily(struct dwrite_fontcollection *collection, UINT32 index,
        struct dwrite_fontfamily **family)
{
    struct dwrite_fontfamily *object;

    *family = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteFontFamily2_iface.lpVtbl = &fontfamilyvtbl;
    object->IDWriteFontList2_iface.lpVtbl = &fontfamilylistvtbl;
    object->refcount = 1;
    object->collection = collection;
    IDWriteFontCollection3_AddRef(&collection->IDWriteFontCollection3_iface);
    object->data = collection->family_data[index];
    InterlockedIncrement(&object->data->refcount);

    *family = object;

    return S_OK;
}

BOOL is_system_collection(IDWriteFontCollection *collection)
{
    void *obj;
    return IDWriteFontCollection_QueryInterface(collection, &IID_issystemcollection, &obj) == S_OK;
}

static HRESULT WINAPI dwritesystemfontcollection_QueryInterface(IDWriteFontCollection3 *iface, REFIID riid, void **obj)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);

    TRACE("%p, %s, %p.\n", collection, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontCollection3) ||
        IsEqualIID(riid, &IID_IDWriteFontCollection2) ||
        IsEqualIID(riid, &IID_IDWriteFontCollection1) ||
        IsEqualIID(riid, &IID_IDWriteFontCollection) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFontCollection3_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    if (IsEqualIID(riid, &IID_issystemcollection))
        return S_OK;

    WARN("%s not implemented.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static HRESULT WINAPI dwritefontcollection_QueryInterface(IDWriteFontCollection3 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontCollection3) ||
        IsEqualIID(riid, &IID_IDWriteFontCollection2) ||
        IsEqualIID(riid, &IID_IDWriteFontCollection1) ||
        IsEqualIID(riid, &IID_IDWriteFontCollection) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFontCollection3_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontcollection_AddRef(IDWriteFontCollection3 *iface)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);
    ULONG refcount = InterlockedIncrement(&collection->refcount);

    TRACE("%p, refcount %ld.\n", collection, refcount);

    return refcount;
}

static ULONG WINAPI dwritefontcollection_Release(IDWriteFontCollection3 *iface)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);
    ULONG refcount = InterlockedDecrement(&collection->refcount);
    size_t i;

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        factory_detach_fontcollection(collection->factory, iface);
        for (i = 0; i < collection->count; ++i)
            release_fontfamily_data(collection->family_data[i]);
        free(collection->family_data);
        free(collection);
    }

    return refcount;
}

static UINT32 WINAPI dwritefontcollection_GetFontFamilyCount(IDWriteFontCollection3 *iface)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);

    TRACE("%p.\n", iface);

    return collection->count;
}

static HRESULT WINAPI dwritefontcollection_GetFontFamily(IDWriteFontCollection3 *iface, UINT32 index,
        IDWriteFontFamily **ret)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);
    struct dwrite_fontfamily *family;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, index, ret);

    *ret = NULL;

    if (index >= collection->count)
        return E_FAIL;

    if (SUCCEEDED(hr = create_fontfamily(collection, index, &family)))
        *ret = (IDWriteFontFamily *)&family->IDWriteFontFamily2_iface;

    return hr;
}

static UINT32 collection_find_family(struct dwrite_fontcollection *collection, const WCHAR *name)
{
    size_t i;

    for (i = 0; i < collection->count; ++i)
    {
        IDWriteLocalizedStrings *family_name = collection->family_data[i]->familyname;
        UINT32 j, count = IDWriteLocalizedStrings_GetCount(family_name);
        HRESULT hr;

        for (j = 0; j < count; j++)
        {
            WCHAR buffer[255];
            hr = IDWriteLocalizedStrings_GetString(family_name, j, buffer, ARRAY_SIZE(buffer));
            if (SUCCEEDED(hr) && !wcsicmp(buffer, name))
                return i;
        }
    }

    return ~0u;
}

static HRESULT WINAPI dwritefontcollection_FindFamilyName(IDWriteFontCollection3 *iface, const WCHAR *name,
        UINT32 *index, BOOL *exists)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_w(name), index, exists);

    *index = collection_find_family(collection, name);
    *exists = *index != ~0u;
    return S_OK;
}

static HRESULT WINAPI dwritefontcollection_GetFontFromFontFace(IDWriteFontCollection3 *iface, IDWriteFontFace *face,
        IDWriteFont **font)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);
    struct dwrite_fontfamily *family;
    BOOL found_font = FALSE;
    IDWriteFontFile *file;
    UINT32 face_index, count;
    size_t i, j;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, face, font);

    *font = NULL;

    if (!face)
        return E_INVALIDARG;

    count = 1;
    hr = IDWriteFontFace_GetFiles(face, &count, &file);
    if (FAILED(hr))
        return hr;
    face_index = IDWriteFontFace_GetIndex(face);

    found_font = FALSE;
    for (i = 0; i < collection->count; ++i)
    {
        struct dwrite_fontfamily_data *family_data = collection->family_data[i];

        for (j = 0; j < family_data->count; ++j)
        {
            struct dwrite_font_data *font_data = family_data->fonts[j];

            if (face_index == font_data->face_index && is_same_fontfile(file, font_data->file)) {
                found_font = TRUE;
                break;
            }
        }

        if (found_font)
            break;
    }
    IDWriteFontFile_Release(file);

    if (!found_font)
        return DWRITE_E_NOFONT;

    hr = create_fontfamily(collection, i, &family);
    if (FAILED(hr))
        return hr;

    hr = create_font(family, j, (IDWriteFont3 **)font);
    IDWriteFontFamily2_Release(&family->IDWriteFontFamily2_iface);
    return hr;
}

static HRESULT WINAPI dwritefontcollection1_GetFontSet(IDWriteFontCollection3 *iface, IDWriteFontSet **fontset)
{
    FIXME("%p, %p.\n", iface, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontcollection1_GetFontFamily(IDWriteFontCollection3 *iface, UINT32 index,
        IDWriteFontFamily1 **ret)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);
    struct dwrite_fontfamily *family;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, index, ret);

    *ret = NULL;

    if (index >= collection->count)
        return E_FAIL;

    if (SUCCEEDED(hr = create_fontfamily(collection, index, &family)))
        *ret = (IDWriteFontFamily1 *)&family->IDWriteFontFamily2_iface;

    return hr;
}

static HRESULT WINAPI dwritefontcollection2_GetFontFamily(IDWriteFontCollection3 *iface,
        UINT32 index, IDWriteFontFamily2 **ret)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);
    struct dwrite_fontfamily *family;
    HRESULT hr;

    TRACE("%p, %u, %p.\n", iface, index, ret);

    *ret = NULL;

    if (index >= collection->count)
        return E_FAIL;

    if (SUCCEEDED(hr = create_fontfamily(collection, index, &family)))
        *ret = &family->IDWriteFontFamily2_iface;

    return hr;
}

static HRESULT WINAPI dwritefontcollection2_GetMatchingFonts(IDWriteFontCollection3 *iface,
        const WCHAR *familyname, DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_values,
        IDWriteFontList2 **fontlist)
{
    FIXME("%p, %s, %p, %u, %p.\n", iface, debugstr_w(familyname), axis_values, num_values, fontlist);

    return E_NOTIMPL;
}

static DWRITE_FONT_FAMILY_MODEL WINAPI dwritefontcollection2_GetFontFamilyModel(IDWriteFontCollection3 *iface)
{
    struct dwrite_fontcollection *collection = impl_from_IDWriteFontCollection3(iface);

    TRACE("%p.\n", iface);

    return collection->family_model;
}

static HRESULT WINAPI dwritefontcollection2_GetFontSet(IDWriteFontCollection3 *iface, IDWriteFontSet1 **fontset)
{
    FIXME("%p, %p.\n", iface, fontset);

    return E_NOTIMPL;
}

static HANDLE WINAPI dwritefontcollection3_GetExpirationEvent(IDWriteFontCollection3 *iface)
{
    FIXME("%p.\n", iface);

    return NULL;
}

static const IDWriteFontCollection3Vtbl fontcollectionvtbl =
{
    dwritefontcollection_QueryInterface,
    dwritefontcollection_AddRef,
    dwritefontcollection_Release,
    dwritefontcollection_GetFontFamilyCount,
    dwritefontcollection_GetFontFamily,
    dwritefontcollection_FindFamilyName,
    dwritefontcollection_GetFontFromFontFace,
    dwritefontcollection1_GetFontSet,
    dwritefontcollection1_GetFontFamily,
    dwritefontcollection2_GetFontFamily,
    dwritefontcollection2_GetMatchingFonts,
    dwritefontcollection2_GetFontFamilyModel,
    dwritefontcollection2_GetFontSet,
    dwritefontcollection3_GetExpirationEvent,
};

static const IDWriteFontCollection3Vtbl systemfontcollectionvtbl =
{
    dwritesystemfontcollection_QueryInterface,
    dwritefontcollection_AddRef,
    dwritefontcollection_Release,
    dwritefontcollection_GetFontFamilyCount,
    dwritefontcollection_GetFontFamily,
    dwritefontcollection_FindFamilyName,
    dwritefontcollection_GetFontFromFontFace,
    dwritefontcollection1_GetFontSet,
    dwritefontcollection1_GetFontFamily,
    dwritefontcollection2_GetFontFamily,
    dwritefontcollection2_GetMatchingFonts,
    dwritefontcollection2_GetFontFamilyModel,
    dwritefontcollection2_GetFontSet,
    dwritefontcollection3_GetExpirationEvent,
};

static HRESULT fontfamily_add_font(struct dwrite_fontfamily_data *family_data, struct dwrite_font_data *font_data)
{
    if (!dwrite_array_reserve((void **)&family_data->fonts, &family_data->size, family_data->count + 1,
            sizeof(*family_data->fonts)))
    {
        return E_OUTOFMEMORY;
    }

    family_data->fonts[family_data->count++] = font_data;
    if (font_data->style == DWRITE_FONT_STYLE_NORMAL)
        family_data->has_normal_face = 1;
    else if (font_data->style == DWRITE_FONT_STYLE_OBLIQUE)
        family_data->has_oblique_face = 1;
    else
        family_data->has_italic_face = 1;
    return S_OK;
}

static HRESULT fontcollection_add_family(struct dwrite_fontcollection *collection,
        struct dwrite_fontfamily_data *family)
{
    if (!dwrite_array_reserve((void **)&collection->family_data, &collection->size, collection->count + 1,
            sizeof(*collection->family_data)))
    {
        return E_OUTOFMEMORY;
    }

    collection->family_data[collection->count++] = family;
    return S_OK;
}

static HRESULT init_font_collection(struct dwrite_fontcollection *collection, IDWriteFactory7 *factory,
        DWRITE_FONT_FAMILY_MODEL family_model, BOOL is_system)
{
    collection->IDWriteFontCollection3_iface.lpVtbl = is_system ? &systemfontcollectionvtbl : &fontcollectionvtbl;
    collection->refcount = 1;
    collection->factory = factory;
    IDWriteFactory7_AddRef(collection->factory);
    collection->family_model = family_model;

    return S_OK;
}

HRESULT get_filestream_from_file(IDWriteFontFile *file, IDWriteFontFileStream **stream)
{
    IDWriteFontFileLoader *loader;
    const void *key;
    UINT32 key_size;
    HRESULT hr;

    *stream = NULL;

    hr = IDWriteFontFile_GetReferenceKey(file, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFile_GetLoader(file, &loader);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFileLoader_CreateStreamFromKey(loader, key, key_size, stream);
    IDWriteFontFileLoader_Release(loader);
    if (FAILED(hr))
        return hr;

    return hr;
}

static void fontstrings_get_en_string(IDWriteLocalizedStrings *strings, WCHAR *buffer, UINT32 size)
{
    BOOL exists = FALSE;
    UINT32 index;
    HRESULT hr;

    buffer[0] = 0;
    hr = IDWriteLocalizedStrings_FindLocaleName(strings, L"en-us", &index, &exists);
    if (FAILED(hr) || !exists)
        return;

    IDWriteLocalizedStrings_GetString(strings, index, buffer, size);
}

static int trim_spaces(WCHAR *in, WCHAR *ret)
{
    int len;

    while (iswspace(*in))
        in++;

    ret[0] = 0;
    if (!(len = wcslen(in)))
        return 0;

    while (iswspace(in[len-1]))
        len--;

    memcpy(ret, in, len*sizeof(WCHAR));
    ret[len] = 0;

    return len;
}

struct name_token {
    struct list entry;
    const WCHAR *ptr;
    INT len;     /* token length */
    INT fulllen; /* full length including following separators */
};

static inline BOOL is_name_separator_char(WCHAR ch)
{
    return ch == ' ' || ch == '.' || ch == '-' || ch == '_';
}

struct name_pattern {
    const WCHAR *part1; /* NULL indicates end of list */
    const WCHAR *part2; /* optional, if not NULL should point to non-empty string */
};

static BOOL match_pattern_list(struct list *tokens, const struct name_pattern *patterns, struct name_token *match)
{
    const struct name_pattern *pattern;
    struct name_token *token;
    int i = 0;

    while ((pattern = &patterns[i++])->part1)
    {
        int len_part1 = wcslen(pattern->part1);
        int len_part2 = pattern->part2 ? wcslen(pattern->part2) : 0;

        LIST_FOR_EACH_ENTRY(token, tokens, struct name_token, entry)
        {
            if (!len_part2)
            {
                /* simple case with single part pattern */
                if (token->len != len_part1)
                    continue;

                if (!wcsnicmp(token->ptr, pattern->part1, len_part1))
                {
                    if (match) *match = *token;
                    list_remove(&token->entry);
                    free(token);
                    return TRUE;
                }
            }
            else
            {
                struct name_token *next_token;
                struct list *next_entry;

                /* pattern parts are stored in reading order, tokens list is reversed */
                if (token->len < len_part2)
                    continue;

                /* it's possible to have combined string as a token, like ExtraCondensed */
                if (token->len == len_part1 + len_part2)
                {
                    if (wcsnicmp(token->ptr, pattern->part1, len_part1))
                        continue;

                    if (wcsnicmp(&token->ptr[len_part1], pattern->part2, len_part2))
                        continue;

                    /* combined string match */
                    if (match) *match = *token;
                    list_remove(&token->entry);
                    free(token);
                    return TRUE;
                }

                /* now it's only possible to have two tokens matched to respective pattern parts */
                if (token->len != len_part2)
                    continue;

                next_entry = list_next(tokens, &token->entry);
                if (next_entry) {
                    next_token = LIST_ENTRY(next_entry, struct name_token, entry);
                    if (next_token->len != len_part1)
                        continue;

                    if (wcsnicmp(token->ptr, pattern->part2, len_part2))
                        continue;

                    if (wcsnicmp(next_token->ptr, pattern->part1, len_part1))
                        continue;

                    /* both parts matched, remove tokens */
                    if (match) {
                        match->ptr = next_token->ptr;
                        match->len = (token->ptr - next_token->ptr) + token->len;
                    }
                    list_remove(&token->entry);
                    list_remove(&next_token->entry);
                    free(next_token);
                    free(token);
                    return TRUE;
                }
            }
        }
    }

    if (match) {
        match->ptr = NULL;
        match->len = 0;
    }
    return FALSE;
}

static DWRITE_FONT_STYLE font_extract_style(struct list *tokens, DWRITE_FONT_STYLE style, struct name_token *match)
{
    static const struct name_pattern italic_patterns[] =
    {
        { L"ita" },
        { L"ital" },
        { L"italic" },
        { L"cursive" },
        { L"kursiv" },
        { NULL }
    };

    static const struct name_pattern oblique_patterns[] =
    {
        { L"inclined" },
        { L"oblique" },
        { L"backslanted" },
        { L"backslant" },
        { L"slanted" },
        { NULL }
    };

    /* italic patterns first */
    if (match_pattern_list(tokens, italic_patterns, match))
        return DWRITE_FONT_STYLE_ITALIC;

    /* oblique patterns */
    if (match_pattern_list(tokens, oblique_patterns, match))
        return DWRITE_FONT_STYLE_OBLIQUE;

    return style;
}

static DWRITE_FONT_STRETCH font_extract_stretch(struct list *tokens, DWRITE_FONT_STRETCH stretch,
    struct name_token *match)
{
    static const struct name_pattern ultracondensed_patterns[] =
    {
        { L"extra", L"compressed" },
        { L"ext", L"compressed" },
        { L"ultra", L"compressed" },
        { L"ultra", L"condensed" },
        { L"ultra", L"cond" },
        { NULL }
    };

    static const struct name_pattern extracondensed_patterns[] =
    {
        { L"compressed" },
        { L"extra", L"condensed" },
        { L"ext", L"condensed" },
        { L"extra", L"cond" },
        { L"ext", L"cond" },
        { NULL }
    };

    static const struct name_pattern semicondensed_patterns[] =
    {
        { L"narrow" },
        { L"compact" },
        { L"semi", L"condensed" },
        { L"semi", L"cond" },
        { NULL }
    };

    static const struct name_pattern semiexpanded_patterns[] =
    {
        { L"wide" },
        { L"semi", L"expanded" },
        { L"semi", L"extended" },
        { NULL }
    };

    static const struct name_pattern extraexpanded_patterns[] =
    {
        { L"extra", L"expanded" },
        { L"ext", L"expanded" },
        { L"extra", L"extended" },
        { L"ext", L"extended" },
        { NULL }
    };

    static const struct name_pattern ultraexpanded_patterns[] =
    {
        { L"ultra", L"expanded" },
        { L"ultra", L"extended" },
        { NULL }
    };

    static const struct name_pattern condensed_patterns[] =
    {
        { L"condensed" },
        { L"cond" },
        { NULL }
    };

    static const struct name_pattern expanded_patterns[] =
    {
        { L"expanded" },
        { L"extended" },
        { NULL }
    };

    if (match_pattern_list(tokens, ultracondensed_patterns, match))
        return DWRITE_FONT_STRETCH_ULTRA_CONDENSED;

    if (match_pattern_list(tokens, extracondensed_patterns, match))
        return DWRITE_FONT_STRETCH_EXTRA_CONDENSED;

    if (match_pattern_list(tokens, semicondensed_patterns, match))
        return DWRITE_FONT_STRETCH_SEMI_CONDENSED;

    if (match_pattern_list(tokens, semiexpanded_patterns, match))
        return DWRITE_FONT_STRETCH_SEMI_EXPANDED;

    if (match_pattern_list(tokens, extraexpanded_patterns, match))
        return DWRITE_FONT_STRETCH_EXTRA_EXPANDED;

    if (match_pattern_list(tokens, ultraexpanded_patterns, match))
        return DWRITE_FONT_STRETCH_ULTRA_EXPANDED;

    if (match_pattern_list(tokens, condensed_patterns, match))
        return DWRITE_FONT_STRETCH_CONDENSED;

    if (match_pattern_list(tokens, expanded_patterns, match))
        return DWRITE_FONT_STRETCH_EXPANDED;

    return stretch;
}

static DWRITE_FONT_WEIGHT font_extract_weight(struct list *tokens, DWRITE_FONT_WEIGHT weight,
    struct name_token *match)
{
    static const struct name_pattern thin_patterns[] =
    {
        { L"extra", L"thin" },
        { L"ext", L"thin" },
        { L"ultra", L"thin" },
        { NULL }
    };

    static const struct name_pattern extralight_patterns[] =
    {
        { L"extra", L"light" },
        { L"ext", L"light" },
        { L"ultra", L"light" },
        { NULL }
    };

    static const struct name_pattern semilight_patterns[] =
    {
        { L"semi", L"light" },
        { NULL }
    };

    static const struct name_pattern demibold_patterns[] =
    {
        { L"semi", L"bold" },
        { L"demi", L"bold" },
        { NULL }
    };

    static const struct name_pattern extrabold_patterns[] =
    {
        { L"extra", L"bold" },
        { L"ext", L"bold" },
        { L"ultra", L"bold" },
        { NULL }
    };

    static const struct name_pattern extrablack_patterns[] =
    {
        { L"extra", L"black" },
        { L"ext", L"black" },
        { L"ultra", L"black" },
        { NULL }
    };

    static const struct name_pattern bold_patterns[] =
    {
        { L"bold" },
        { NULL }
    };

    static const struct name_pattern thin2_patterns[] =
    {
        { L"thin" },
        { NULL }
    };

    static const struct name_pattern light_patterns[] =
    {
        { L"light" },
        { NULL }
    };

    static const struct name_pattern medium_patterns[] =
    {
        { L"medium" },
        { NULL }
    };

    static const struct name_pattern black_patterns[] =
    {
        { L"black" },
        { L"heavy" },
        { L"nord" },
        { NULL }
    };

    static const struct name_pattern demibold2_patterns[] =
    {
        { L"demi" },
        { NULL }
    };

    static const struct name_pattern extrabold2_patterns[] =
    {
        { L"ultra" },
        { NULL }
    };

    /* FIXME: allow optional 'face' suffix, separated or not. It's removed together with
       matching pattern. */

    if (match_pattern_list(tokens, thin_patterns, match))
        return DWRITE_FONT_WEIGHT_THIN;

    if (match_pattern_list(tokens, extralight_patterns, match))
        return DWRITE_FONT_WEIGHT_EXTRA_LIGHT;

    if (match_pattern_list(tokens, semilight_patterns, match))
        return DWRITE_FONT_WEIGHT_SEMI_LIGHT;

    if (match_pattern_list(tokens, demibold_patterns, match))
        return DWRITE_FONT_WEIGHT_DEMI_BOLD;

    if (match_pattern_list(tokens, extrabold_patterns, match))
        return DWRITE_FONT_WEIGHT_EXTRA_BOLD;

    if (match_pattern_list(tokens, extrablack_patterns, match))
        return DWRITE_FONT_WEIGHT_EXTRA_BLACK;

    if (match_pattern_list(tokens, bold_patterns, match))
        return DWRITE_FONT_WEIGHT_BOLD;

    if (match_pattern_list(tokens, thin2_patterns, match))
        return DWRITE_FONT_WEIGHT_THIN;

    if (match_pattern_list(tokens, light_patterns, match))
        return DWRITE_FONT_WEIGHT_LIGHT;

    if (match_pattern_list(tokens, medium_patterns, match))
        return DWRITE_FONT_WEIGHT_MEDIUM;

    if (match_pattern_list(tokens, black_patterns, match))
        return DWRITE_FONT_WEIGHT_BLACK;

    if (match_pattern_list(tokens, black_patterns, match))
        return DWRITE_FONT_WEIGHT_BLACK;

    if (match_pattern_list(tokens, demibold2_patterns, match))
        return DWRITE_FONT_WEIGHT_DEMI_BOLD;

    if (match_pattern_list(tokens, extrabold2_patterns, match))
        return DWRITE_FONT_WEIGHT_EXTRA_BOLD;

    /* FIXME: use abbreviated names to extract weight */

    return weight;
}

struct knownweight_entry
{
    const WCHAR *nameW;
    DWRITE_FONT_WEIGHT weight;
};

static int __cdecl compare_knownweights(const void *a, const void* b)
{
    DWRITE_FONT_WEIGHT target = *(DWRITE_FONT_WEIGHT*)a;
    const struct knownweight_entry *entry = (struct knownweight_entry*)b;
    int ret = 0;

    if (target > entry->weight)
        ret = 1;
    else if (target < entry->weight)
        ret = -1;

    return ret;
}

static BOOL is_known_weight_value(DWRITE_FONT_WEIGHT weight, WCHAR *nameW)
{
    static const struct knownweight_entry knownweights[] =
    {
        { L"Thin",        DWRITE_FONT_WEIGHT_THIN },
        { L"Extra Light", DWRITE_FONT_WEIGHT_EXTRA_LIGHT },
        { L"Light",       DWRITE_FONT_WEIGHT_LIGHT },
        { L"Semi Light",  DWRITE_FONT_WEIGHT_SEMI_LIGHT },
        { L"Medium",      DWRITE_FONT_WEIGHT_MEDIUM },
        { L"Demi Bold",   DWRITE_FONT_WEIGHT_DEMI_BOLD },
        { L"Bold",        DWRITE_FONT_WEIGHT_BOLD },
        { L"Extra Bold",  DWRITE_FONT_WEIGHT_EXTRA_BOLD },
        { L"Black",       DWRITE_FONT_WEIGHT_BLACK },
        { L"Extra Black", DWRITE_FONT_WEIGHT_EXTRA_BLACK }
    };
    const struct knownweight_entry *ptr;

    ptr = bsearch(&weight, knownweights, ARRAY_SIZE(knownweights), sizeof(*knownweights),
        compare_knownweights);
    if (!ptr) {
        nameW[0] = 0;
        return FALSE;
    }

    wcscpy(nameW, ptr->nameW);
    return TRUE;
}

static inline void font_name_token_to_str(const struct name_token *name, WCHAR *strW)
{
    memcpy(strW, name->ptr, name->len * sizeof(WCHAR));
    strW[name->len] = 0;
}

/* Modifies facenameW string, and returns pointer to regular term that was removed */
static const WCHAR *facename_remove_regular_term(WCHAR *facenameW, INT len)
{
    static const WCHAR *regular_patterns[] =
    {
        L"Book",
        L"Normal",
        L"Regular",
        L"Roman",
        L"Upright",
        NULL
    };

    const WCHAR *regular_ptr = NULL, *ptr;
    int i = 0;

    if (len == -1)
        len = wcslen(facenameW);

    /* remove rightmost regular variant from face name */
    while (!regular_ptr && (ptr = regular_patterns[i++]))
    {
        int pattern_len = wcslen(ptr);
        WCHAR *src;

        if (pattern_len > len)
            continue;

        src = facenameW + len - pattern_len;
        while (src >= facenameW)
        {
            if (!wcsnicmp(src, ptr, pattern_len))
            {
                memmove(src, src + pattern_len, (len - pattern_len - (src - facenameW) + 1)*sizeof(WCHAR));
                len = wcslen(facenameW);
                regular_ptr = ptr;
                break;
            }
            else
                src--;
        }
    }

    return regular_ptr;
}

static void fontname_tokenize(struct list *tokens, const WCHAR *nameW)
{
    const WCHAR *ptr;

    list_init(tokens);
    ptr = nameW;

    while (*ptr)
    {
        struct name_token *token = malloc(sizeof(*token));
        token->ptr = ptr;
        token->len = 0;
        token->fulllen = 0;

        while (*ptr && !is_name_separator_char(*ptr)) {
            token->len++;
            token->fulllen++;
            ptr++;
        }

        /* skip separators */
        while (is_name_separator_char(*ptr)) {
            token->fulllen++;
            ptr++;
        }

        list_add_head(tokens, &token->entry);
    }
}

static void fontname_tokens_to_str(struct list *tokens, WCHAR *nameW)
{
    struct name_token *token, *token2;
    LIST_FOR_EACH_ENTRY_SAFE_REV(token, token2, tokens, struct name_token, entry) {
        int len;

        list_remove(&token->entry);

        /* don't include last separator */
        len = list_empty(tokens) ? token->len : token->fulllen;
        memcpy(nameW, token->ptr, len * sizeof(WCHAR));
        nameW += len;

        free(token);
    }
    *nameW = 0;
}

static BOOL font_apply_differentiation_rules(struct dwrite_font_data *font, WCHAR *familyW, WCHAR *faceW)
{
    struct name_token stretch_name, weight_name, style_name;
    WCHAR familynameW[255], facenameW[255], finalW[255];
    WCHAR weightW[32], stretchW[32], styleW[32];
    const WCHAR *regular_ptr = NULL;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_WEIGHT weight;
    struct list tokens;
    int len;

    /* remove leading and trailing spaces from family and face name */
    trim_spaces(familyW, familynameW);
    len = trim_spaces(faceW, facenameW);

    /* remove rightmost regular variant from face name */
    regular_ptr = facename_remove_regular_term(facenameW, len);

    /* append face name to family name, FIXME check if face name is a substring of family name */
    if (*facenameW)
    {
        wcscat(familynameW, L" ");
        wcscat(familynameW, facenameW);
    }

    /* tokenize with " .-_" */
    fontname_tokenize(&tokens, familynameW);

    /* extract and resolve style */
    font->style = font_extract_style(&tokens, font->style, &style_name);

    /* extract stretch */
    stretch = font_extract_stretch(&tokens, font->stretch, &stretch_name);

    /* extract weight */
    weight = font_extract_weight(&tokens, font->weight, &weight_name);

    /* resolve weight */
    if (weight != font->weight)
    {
        if (!(weight < DWRITE_FONT_WEIGHT_NORMAL && font->weight < DWRITE_FONT_WEIGHT_NORMAL) &&
            !(weight > DWRITE_FONT_WEIGHT_MEDIUM && font->weight > DWRITE_FONT_WEIGHT_MEDIUM) &&
            !((weight == DWRITE_FONT_WEIGHT_NORMAL && font->weight == DWRITE_FONT_WEIGHT_MEDIUM) ||
              (weight == DWRITE_FONT_WEIGHT_MEDIUM && font->weight == DWRITE_FONT_WEIGHT_NORMAL)) &&
            !(abs((int)weight - (int)font->weight) <= 150 &&
              font->weight != DWRITE_FONT_WEIGHT_NORMAL &&
              font->weight != DWRITE_FONT_WEIGHT_MEDIUM &&
              font->weight != DWRITE_FONT_WEIGHT_BOLD))
        {
            font->weight = weight;
        }
    }

    /* Resolve stretch - extracted stretch can't be normal, it will override specified stretch if
       it's leaning in opposite direction from normal comparing to specified stretch or if specified
       stretch itself is normal (extracted stretch is never normal). */
    if (stretch != font->stretch) {
        if ((font->stretch == DWRITE_FONT_STRETCH_NORMAL) ||
            (font->stretch < DWRITE_FONT_STRETCH_NORMAL && stretch > DWRITE_FONT_STRETCH_NORMAL) ||
            (font->stretch > DWRITE_FONT_STRETCH_NORMAL && stretch < DWRITE_FONT_STRETCH_NORMAL)) {

            font->stretch = stretch;
        }
    }

    /* FIXME: cleanup face name from possible 2-3 digit prefixes */

    /* get final combined string from what's left in token list, list is released */
    fontname_tokens_to_str(&tokens, finalW);

    if (!wcscmp(familyW, finalW))
        return FALSE;

    /* construct face name */
    wcscpy(familyW, finalW);

    /* resolved weight name */
    if (weight_name.ptr)
        font_name_token_to_str(&weight_name, weightW);
    /* ignore normal weight */
    else if (font->weight == DWRITE_FONT_WEIGHT_NORMAL)
        weightW[0] = 0;
    /* for known weight values use appropriate names */
    else if (is_known_weight_value(font->weight, weightW)) {
    }
    /* use Wnnn format as a fallback in case weight is not one of known values */
    else
        swprintf(weightW, ARRAY_SIZE(weightW), L"W%d", font->weight);

    /* resolved stretch name */
    if (stretch_name.ptr)
        font_name_token_to_str(&stretch_name, stretchW);
    /* ignore normal stretch */
    else if (font->stretch == DWRITE_FONT_STRETCH_NORMAL)
        stretchW[0] = 0;
    /* use predefined stretch names */
    else
    {
        static const WCHAR *stretchnamesW[] =
        {
            NULL, /* DWRITE_FONT_STRETCH_UNDEFINED */
            L"Ultra Condensed",
            L"Extra Condensed",
            L"Condensed",
            L"Semi Condensed",
            NULL, /* DWRITE_FONT_STRETCH_NORMAL */
            L"Semi Expanded",
            L"Expanded",
            L"Extra Expanded",
            L"Ultra Expanded"
        };
        wcscpy(stretchW, stretchnamesW[font->stretch]);
    }

    /* resolved style name */
    if (style_name.ptr)
        font_name_token_to_str(&style_name, styleW);
    else if (font->style == DWRITE_FONT_STYLE_NORMAL)
        styleW[0] = 0;
    /* use predefined names */
    else
        wcscpy(styleW, font->style == DWRITE_FONT_STYLE_ITALIC ? L"Italic" : L"Oblique");

    /* use Regular match if it was found initially */
    if (!*weightW && !*stretchW && !*styleW)
        wcscpy(faceW, regular_ptr ? regular_ptr : L"Regular");
    else
    {
        faceW[0] = 0;

        if (*stretchW) wcscpy(faceW, stretchW);

        if (*weightW)
        {
            if (*faceW) wcscat(faceW, L" ");
            wcscat(faceW, weightW);
        }

        if (*styleW)
        {
            if (*faceW) wcscat(faceW, L" ");
            wcscat(faceW, styleW);
        }
    }

    TRACE("resolved family %s, face %s\n", debugstr_w(familyW), debugstr_w(faceW));
    return TRUE;
}

static HRESULT init_font_data(const struct fontface_desc *desc, DWRITE_FONT_FAMILY_MODEL family_model,
        struct dwrite_font_data **ret)
{
    static const float width_axis_values[] =
    {
        0.0f, /* DWRITE_FONT_STRETCH_UNDEFINED */
        50.0f, /* DWRITE_FONT_STRETCH_ULTRA_CONDENSED */
        62.5f, /* DWRITE_FONT_STRETCH_EXTRA_CONDENSED */
        75.0f, /* DWRITE_FONT_STRETCH_CONDENSED */
        87.5f, /* DWRITE_FONT_STRETCH_SEMI_CONDENSED */
        100.0f, /* DWRITE_FONT_STRETCH_NORMAL */
        112.5f, /* DWRITE_FONT_STRETCH_SEMI_EXPANDED */
        125.0f, /* DWRITE_FONT_STRETCH_EXPANDED */
        150.0f, /* DWRITE_FONT_STRETCH_EXTRA_EXPANDED */
        200.0f, /* DWRITE_FONT_STRETCH_ULTRA_EXPANDED */
    };

    struct file_stream_desc stream_desc;
    struct dwrite_font_props props;
    struct dwrite_font_data *data;
    WCHAR familyW[255], faceW[255];
    HRESULT hr;

    *ret = NULL;

    if (!(data = calloc(1, sizeof(*data))))
        return E_OUTOFMEMORY;

    data->refcount = 1;
    data->file = desc->file;
    data->face_index = desc->index;
    data->face_type = desc->face_type;
    IDWriteFontFile_AddRef(data->file);

    stream_desc.stream = desc->stream;
    stream_desc.face_type = desc->face_type;
    stream_desc.face_index = desc->index;
    opentype_get_font_properties(&stream_desc, &props);
    opentype_get_font_metrics(&stream_desc, &data->metrics, NULL);
    opentype_get_font_facename(&stream_desc, props.lf.lfFaceName, &data->names);

    if (FAILED(hr = opentype_get_font_familyname(&stream_desc, family_model, &data->family_names)))
    {
        WARN("Unable to get family name from the font file, hr %#lx.\n", hr);
        release_font_data(data);
        return hr;
    }

    data->style = props.style;
    data->stretch = props.stretch;
    data->weight = props.weight;
    data->panose = props.panose;
    data->fontsig = props.fontsig;
    data->lf = props.lf;
    data->flags = props.flags;

    fontstrings_get_en_string(data->family_names, familyW, ARRAY_SIZE(familyW));
    fontstrings_get_en_string(data->names, faceW, ARRAY_SIZE(faceW));

    if (family_model == DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE
            && font_apply_differentiation_rules(data, familyW, faceW))
    {
        set_en_localizedstring(data->family_names, familyW);
        set_en_localizedstring(data->names, faceW);
    }

    init_font_prop_vec(data->weight, data->stretch, data->style, &data->propvec);

    data->axis[0].axisTag = DWRITE_FONT_AXIS_TAG_WEIGHT;
    data->axis[0].value = props.weight;
    data->axis[1].axisTag = DWRITE_FONT_AXIS_TAG_WIDTH;
    data->axis[1].value = width_axis_values[props.stretch];
    data->axis[2].axisTag = DWRITE_FONT_AXIS_TAG_ITALIC;
    data->axis[2].value = data->style == DWRITE_FONT_STYLE_ITALIC ? 1.0f : 0.0f;

    *ret = data;
    return S_OK;
}

static HRESULT init_font_data_from_font(const struct dwrite_font_data *src, DWRITE_FONT_SIMULATIONS simulations,
        const WCHAR *facenameW, struct dwrite_font_data **ret)
{
    struct dwrite_font_data *data;

    *ret = NULL;

    if (!(data = calloc(1, sizeof(*data))))
        return E_OUTOFMEMORY;

    *data = *src;
    data->refcount = 1;
    data->simulations |= simulations;
    if (simulations & DWRITE_FONT_SIMULATIONS_BOLD)
        data->weight = DWRITE_FONT_WEIGHT_BOLD;
    if (simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE)
        data->style = DWRITE_FONT_STYLE_OBLIQUE;
    memset(data->info_strings, 0, sizeof(data->info_strings));
    data->names = NULL;
    IDWriteFontFile_AddRef(data->file);
    IDWriteLocalizedStrings_AddRef(data->family_names);

    create_localizedstrings(&data->names);
    add_localizedstring(data->names, L"en-us", facenameW);

    init_font_prop_vec(data->weight, data->stretch, data->style, &data->propvec);

    *ret = data;
    return S_OK;
};

static HRESULT init_fontfamily_data(IDWriteLocalizedStrings *familyname, struct dwrite_fontfamily_data **ret)
{
    struct dwrite_fontfamily_data *data;

    if (!(data = calloc(1, sizeof(*data))))
        return E_OUTOFMEMORY;

    data->refcount = 1;
    data->familyname = familyname;
    IDWriteLocalizedStrings_AddRef(familyname);

    *ret = data;

    return S_OK;
}

static void fontfamily_add_bold_simulated_face(struct dwrite_fontfamily_data *family)
{
    size_t i, j, heaviest;

    for (i = 0; i < family->count; ++i)
    {
        DWRITE_FONT_WEIGHT weight = family->fonts[i]->weight;
        heaviest = i;

        if (family->fonts[i]->bold_sim_tested)
            continue;

        family->fonts[i]->bold_sim_tested = 1;
        for (j = i; j < family->count; ++j)
        {
            if (family->fonts[j]->bold_sim_tested)
                continue;

            if ((family->fonts[i]->style == family->fonts[j]->style) &&
                (family->fonts[i]->stretch == family->fonts[j]->stretch)) {
                if (family->fonts[j]->weight > weight) {
                    weight = family->fonts[j]->weight;
                    heaviest = j;
                }
                family->fonts[j]->bold_sim_tested = 1;
            }
        }

        if (weight >= DWRITE_FONT_WEIGHT_SEMI_LIGHT && weight <= 550)
        {
            static const struct name_pattern weightsim_patterns[] =
            {
                { L"extra", L"light" },
                { L"ext", L"light" },
                { L"ultra", L"light" },
                { L"semi", L"light" },
                { L"semi", L"bold" },
                { L"demi", L"bold" },
                { L"bold" },
                { L"thin" },
                { L"light" },
                { L"medium" },
                { L"demi" },
                { NULL }
            };

            WCHAR facenameW[255], initialW[255];
            struct dwrite_font_data *boldface;
            struct list tokens;

            /* add Bold simulation based on heaviest face data */

            /* Simulated face name should only contain Bold as weight term,
               so remove existing regular and weight terms. */
            fontstrings_get_en_string(family->fonts[heaviest]->names, initialW, ARRAY_SIZE(initialW));
            facename_remove_regular_term(initialW, -1);

            /* remove current weight pattern */
            fontname_tokenize(&tokens, initialW);
            match_pattern_list(&tokens, weightsim_patterns, NULL);
            fontname_tokens_to_str(&tokens, facenameW);

            /* Bold suffix for new name */
            if (*facenameW) wcscat(facenameW, L" ");
            wcscat(facenameW, L"Bold");

            if (init_font_data_from_font(family->fonts[heaviest], DWRITE_FONT_SIMULATIONS_BOLD, facenameW, &boldface) == S_OK) {
                boldface->bold_sim_tested = 1;
                boldface->lf.lfWeight += (FW_BOLD - FW_REGULAR) / 2 + 1;
                fontfamily_add_font(family, boldface);
            }
        }
    }
}

static void fontfamily_add_oblique_simulated_face(struct dwrite_fontfamily_data *family)
{
    size_t i, j;

    for (i = 0; i < family->count; ++i)
    {
        UINT32 regular = ~0u, oblique = ~0u;
        struct dwrite_font_data *obliqueface;
        WCHAR facenameW[255];

        if (family->fonts[i]->oblique_sim_tested)
            continue;

        family->fonts[i]->oblique_sim_tested = 1;
        if (family->fonts[i]->style == DWRITE_FONT_STYLE_NORMAL)
            regular = i;
        else if (family->fonts[i]->style == DWRITE_FONT_STYLE_OBLIQUE)
            oblique = i;

        /* find regular style with same weight/stretch values */
        for (j = i; j < family->count; ++j)
        {
            if (family->fonts[j]->oblique_sim_tested)
                continue;

            if ((family->fonts[i]->weight == family->fonts[j]->weight) &&
                (family->fonts[i]->stretch == family->fonts[j]->stretch)) {

                family->fonts[j]->oblique_sim_tested = 1;
                if (regular == ~0 && family->fonts[j]->style == DWRITE_FONT_STYLE_NORMAL)
                    regular = j;

                if (oblique == ~0 && family->fonts[j]->style == DWRITE_FONT_STYLE_OBLIQUE)
                    oblique = j;
            }

            if (regular != ~0u && oblique != ~0u)
                break;
        }

        /* no regular variant for this weight/stretch pair, nothing to base simulated face on */
        if (regular == ~0u)
            continue;

        /* regular face exists, and corresponding oblique is present as well, nothing to do */
        if (oblique != ~0u)
            continue;

        /* add oblique simulation based on this regular face */

        /* remove regular term if any, append 'Oblique' */
        fontstrings_get_en_string(family->fonts[regular]->names, facenameW, ARRAY_SIZE(facenameW));
        facename_remove_regular_term(facenameW, -1);

        if (*facenameW) wcscat(facenameW, L" ");
        wcscat(facenameW, L"Oblique");

        if (init_font_data_from_font(family->fonts[regular], DWRITE_FONT_SIMULATIONS_OBLIQUE, facenameW, &obliqueface) == S_OK) {
            obliqueface->oblique_sim_tested = 1;
            obliqueface->lf.lfItalic = 1;
            fontfamily_add_font(family, obliqueface);
        }
    }
}

static BOOL fontcollection_add_replacement(struct dwrite_fontcollection *collection, const WCHAR *target_name,
    const WCHAR *replacement_name)
{
    UINT32 i = collection_find_family(collection, replacement_name);
    struct dwrite_fontfamily_data *target;
    IDWriteLocalizedStrings *strings;
    HRESULT hr;

    /* replacement does not exist */
    if (i == ~0u)
        return FALSE;

    hr = create_localizedstrings(&strings);
    if (FAILED(hr))
        return FALSE;

    /* add a new family with target name, reuse font data from replacement */
    add_localizedstring(strings, L"en-us", target_name);
    hr = init_fontfamily_data(strings, &target);
    if (hr == S_OK) {
        struct dwrite_fontfamily_data *replacement = collection->family_data[i];
        WCHAR nameW[255];

        for (i = 0; i < replacement->count; ++i)
        {
            fontfamily_add_font(target, replacement->fonts[i]);
            addref_font_data(replacement->fonts[i]);
        }

        fontcollection_add_family(collection, target);
        fontstrings_get_en_string(replacement->familyname, nameW, ARRAY_SIZE(nameW));
        TRACE("replacement %s -> %s\n", debugstr_w(target_name), debugstr_w(nameW));
    }
    IDWriteLocalizedStrings_Release(strings);
    return TRUE;
}

/* Add family mappings from HKCU\Software\Wine\Fonts\Replacements. This only affects
   system font collections. */
static void fontcollection_add_replacements(struct dwrite_fontcollection *collection)
{
    DWORD max_namelen, max_datalen, i = 0, type, datalen, namelen;
    WCHAR *name;
    void *data;
    HKEY hkey;

    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Fonts\\Replacements", &hkey))
        return;

    if (RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &max_namelen, &max_datalen, NULL, NULL)) {
        RegCloseKey(hkey);
        return;
    }

    max_namelen++; /* returned value doesn't include room for '\0' */
    name = malloc(max_namelen * sizeof(WCHAR));
    data = malloc(max_datalen);

    datalen = max_datalen;
    namelen = max_namelen;
    while (RegEnumValueW(hkey, i++, name, &namelen, NULL, &type, data, &datalen) == ERROR_SUCCESS) {
        if (collection_find_family(collection, name) == ~0u) {
            if (type == REG_MULTI_SZ) {
                WCHAR *replacement = data;
                while (*replacement) {
                    if (fontcollection_add_replacement(collection, name, replacement))
                        break;
                    replacement += wcslen(replacement) + 1;
                }
            }
            else if (type == REG_SZ)
                fontcollection_add_replacement(collection, name, data);
        }
        else
	    TRACE("%s is available, won't be replaced.\n", debugstr_w(name));

        datalen = max_datalen;
        namelen = max_namelen;
    }

    free(data);
    free(name);
    RegCloseKey(hkey);
}

HRESULT create_font_collection(IDWriteFactory7 *factory, IDWriteFontFileEnumerator *enumerator, BOOL is_system,
    IDWriteFontCollection3 **ret)
{
    struct fontfile_enum {
        struct list entry;
        IDWriteFontFile *file;
    };
    struct fontfile_enum *fileenum, *fileenum2;
    struct dwrite_fontcollection *collection;
    struct list scannedfiles;
    BOOL current = FALSE;
    HRESULT hr = S_OK;
    size_t i;

    *ret = NULL;

    if (!(collection = calloc(1, sizeof(*collection))))
        return E_OUTOFMEMORY;

    hr = init_font_collection(collection, factory, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE, is_system);
    if (FAILED(hr))
    {
        free(collection);
        return hr;
    }

    *ret = &collection->IDWriteFontCollection3_iface;

    TRACE("building font collection:\n");

    list_init(&scannedfiles);
    while (hr == S_OK) {
        DWRITE_FONT_FACE_TYPE face_type;
        DWRITE_FONT_FILE_TYPE file_type;
        BOOL supported, same = FALSE;
        IDWriteFontFileStream *stream;
        IDWriteFontFile *file;
        UINT32 face_count;

        current = FALSE;
        hr = IDWriteFontFileEnumerator_MoveNext(enumerator, &current);
        if (FAILED(hr) || !current)
            break;

        hr = IDWriteFontFileEnumerator_GetCurrentFontFile(enumerator, &file);
        if (FAILED(hr))
            break;

        /* check if we've scanned this file already */
        LIST_FOR_EACH_ENTRY(fileenum, &scannedfiles, struct fontfile_enum, entry) {
            if ((same = is_same_fontfile(fileenum->file, file)))
                break;
        }

        if (same) {
            IDWriteFontFile_Release(file);
            continue;
        }

        if (FAILED(get_filestream_from_file(file, &stream))) {
            IDWriteFontFile_Release(file);
            continue;
        }

        /* Unsupported formats are skipped. */
        hr = opentype_analyze_font(stream, &supported, &file_type, &face_type, &face_count);
        if (FAILED(hr) || !supported || face_count == 0) {
            TRACE("Unsupported font (%p, 0x%08lx, %d, %u)\n", file, hr, supported, face_count);
            IDWriteFontFileStream_Release(stream);
            IDWriteFontFile_Release(file);
            hr = S_OK;
            continue;
        }

        /* add to scanned list */
        fileenum = malloc(sizeof(*fileenum));
        fileenum->file = file;
        list_add_tail(&scannedfiles, &fileenum->entry);

        for (i = 0; i < face_count; ++i)
        {
            struct dwrite_font_data *font_data;
            struct fontface_desc desc;
            WCHAR familyW[255];
            UINT32 index;

            desc.factory = factory;
            desc.face_type = face_type;
            desc.file = file;
            desc.stream = stream;
            desc.index = i;
            desc.simulations = DWRITE_FONT_SIMULATIONS_NONE;
            desc.font_data = NULL;

            /* Allocate an initialize new font data structure. */
            hr = init_font_data(&desc, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE, &font_data);
            if (FAILED(hr))
            {
                /* move to next one */
                hr = S_OK;
                continue;
            }

            fontstrings_get_en_string(font_data->family_names, familyW, ARRAY_SIZE(familyW));

            /* ignore dot named faces */
            if (familyW[0] == '.')
            {
                WARN("Ignoring face %s\n", debugstr_w(familyW));
                release_font_data(font_data);
                continue;
            }

            index = collection_find_family(collection, familyW);
            if (index != ~0u)
                hr = fontfamily_add_font(collection->family_data[index], font_data);
            else {
                struct dwrite_fontfamily_data *family_data;

                /* create and init new family */
                hr = init_fontfamily_data(font_data->family_names, &family_data);
                if (hr == S_OK) {
                    /* add font to family, family - to collection */
                    hr = fontfamily_add_font(family_data, font_data);
                    if (hr == S_OK)
                        hr = fontcollection_add_family(collection, family_data);

                    if (FAILED(hr))
                        release_fontfamily_data(family_data);
                }
            }

            if (FAILED(hr))
            {
                release_font_data(font_data);
                break;
            }
        }

        IDWriteFontFileStream_Release(stream);
    }

    LIST_FOR_EACH_ENTRY_SAFE(fileenum, fileenum2, &scannedfiles, struct fontfile_enum, entry)
    {
        IDWriteFontFile_Release(fileenum->file);
        list_remove(&fileenum->entry);
        free(fileenum);
    }

    for (i = 0; i < collection->count; ++i)
    {
        fontfamily_add_bold_simulated_face(collection->family_data[i]);
        fontfamily_add_oblique_simulated_face(collection->family_data[i]);
    }

    if (is_system)
        fontcollection_add_replacements(collection);

    return hr;
}

static HRESULT collection_add_font_entry(struct dwrite_fontcollection *collection, const struct fontface_desc *desc)
{
    struct dwrite_font_data *font_data;
    WCHAR familyW[255];
    UINT32 index;
    HRESULT hr;

    if (FAILED(hr = init_font_data(desc, collection->family_model, &font_data)))
        return hr;

    fontstrings_get_en_string(font_data->family_names, familyW, ARRAY_SIZE(familyW));

    /* ignore dot named faces */
    if (familyW[0] == '.')
    {
        WARN("Ignoring face %s\n", debugstr_w(familyW));
        release_font_data(font_data);
        return S_OK;
    }

    index = collection_find_family(collection, familyW);
    if (index != ~0u)
        hr = fontfamily_add_font(collection->family_data[index], font_data);
    else
    {
        struct dwrite_fontfamily_data *family_data;

        /* Create and initialize new family */
        hr = init_fontfamily_data(font_data->family_names, &family_data);
        if (hr == S_OK)
        {
            /* add font to family, family - to collection */
            hr = fontfamily_add_font(family_data, font_data);
            if (hr == S_OK)
                hr = fontcollection_add_family(collection, family_data);

            if (FAILED(hr))
                release_fontfamily_data(family_data);
        }
    }

    if (FAILED(hr))
        release_font_data(font_data);

    return hr;
}

HRESULT create_font_collection_from_set(IDWriteFactory7 *factory, IDWriteFontSet *fontset,
        DWRITE_FONT_FAMILY_MODEL family_model, REFGUID riid, void **ret)
{
    struct dwrite_fontset *set = unsafe_impl_from_IDWriteFontSet(fontset);
    struct dwrite_fontcollection *collection;
    HRESULT hr = S_OK;
    size_t i;

    *ret = NULL;

    if (!(collection = calloc(1, sizeof(*collection))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = init_font_collection(collection, factory, family_model, FALSE)))
    {
        free(collection);
        return hr;
    }

    for (i = 0; i < set->count; ++i)
    {
        const struct dwrite_fontset_entry *entry = set->entries[i];
        IDWriteFontFileStream *stream;
        struct fontface_desc desc;

        if (FAILED(get_filestream_from_file(entry->desc.file, &stream)))
        {
            WARN("Failed to get file stream.\n");
            continue;
        }

        desc.factory = factory;
        desc.face_type = entry->desc.face_type;
        desc.file = entry->desc.file;
        desc.stream = stream;
        desc.index = entry->desc.face_index;
        desc.simulations = entry->desc.simulations;
        desc.font_data = NULL;

        if (FAILED(hr = collection_add_font_entry(collection, &desc)))
            WARN("Failed to add font collection element, hr %#lx.\n", hr);

        IDWriteFontFileStream_Release(stream);
    }

    if (family_model == DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE)
    {
        for (i = 0; i < collection->count; ++i)
        {
            fontfamily_add_bold_simulated_face(collection->family_data[i]);
            fontfamily_add_oblique_simulated_face(collection->family_data[i]);
        }
    }

    hr = IDWriteFontCollection3_QueryInterface(&collection->IDWriteFontCollection3_iface, riid, ret);
    IDWriteFontCollection3_Release(&collection->IDWriteFontCollection3_iface);

    return hr;
}

struct system_fontfile_enumerator
{
    IDWriteFontFileEnumerator IDWriteFontFileEnumerator_iface;
    LONG refcount;

    IDWriteFactory7 *factory;
    HKEY hkey;
    int index;

    WCHAR *filename;
    DWORD filename_size;
};

static inline struct system_fontfile_enumerator *impl_from_IDWriteFontFileEnumerator(IDWriteFontFileEnumerator* iface)
{
    return CONTAINING_RECORD(iface, struct system_fontfile_enumerator, IDWriteFontFileEnumerator_iface);
}

static HRESULT WINAPI systemfontfileenumerator_QueryInterface(IDWriteFontFileEnumerator *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteFontFileEnumerator) || IsEqualIID(riid, &IID_IUnknown)) {
        IDWriteFontFileEnumerator_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI systemfontfileenumerator_AddRef(IDWriteFontFileEnumerator *iface)
{
    struct system_fontfile_enumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);
    return InterlockedIncrement(&enumerator->refcount);
}

static ULONG WINAPI systemfontfileenumerator_Release(IDWriteFontFileEnumerator *iface)
{
    struct system_fontfile_enumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);
    ULONG refcount = InterlockedDecrement(&enumerator->refcount);

    if (!refcount)
    {
        IDWriteFactory7_Release(enumerator->factory);
        RegCloseKey(enumerator->hkey);
        free(enumerator->filename);
        free(enumerator);
    }

    return refcount;
}

static HRESULT create_local_file_reference(IDWriteFactory7 *factory, const WCHAR *filename, IDWriteFontFile **file)
{
    HRESULT hr;

    /* Fonts installed in 'Fonts' system dir don't get full path in registry font files cache */
    if (!wcschr(filename, '\\'))
    {
        WCHAR fullpathW[MAX_PATH];

        GetWindowsDirectoryW(fullpathW, ARRAY_SIZE(fullpathW));
        wcscat(fullpathW, L"\\fonts\\");
        wcscat(fullpathW, filename);

        hr = IDWriteFactory7_CreateFontFileReference(factory, fullpathW, NULL, file);
    }
    else
        hr = IDWriteFactory7_CreateFontFileReference(factory, filename, NULL, file);

    return hr;
}

static HRESULT WINAPI systemfontfileenumerator_GetCurrentFontFile(IDWriteFontFileEnumerator *iface, IDWriteFontFile **file)
{
    struct system_fontfile_enumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);

    *file = NULL;

    if (enumerator->index < 0 || !enumerator->filename || !*enumerator->filename)
        return E_FAIL;

    return create_local_file_reference(enumerator->factory, enumerator->filename, file);
}

static HRESULT WINAPI systemfontfileenumerator_MoveNext(IDWriteFontFileEnumerator *iface, BOOL *current)
{
    struct system_fontfile_enumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);
    WCHAR name_buf[256], *name = name_buf;
    DWORD name_count, max_name_count = ARRAY_SIZE(name_buf), type, data_size;
    HRESULT hr = S_OK;
    LONG r;

    *current = FALSE;
    enumerator->index++;

    /* iterate until we find next string value */
    for (;;) {
        do {
            name_count = max_name_count;
            data_size = enumerator->filename_size - sizeof(*enumerator->filename);

            r = RegEnumValueW(enumerator->hkey, enumerator->index, name, &name_count,
                              NULL, &type, (BYTE *)enumerator->filename, &data_size);
            if (r == ERROR_MORE_DATA) {
                if (name_count >= max_name_count) {
                    if (name != name_buf) free(name);
                    max_name_count *= 2;
                    name = malloc(max_name_count * sizeof(*name));
                    if (!name) return E_OUTOFMEMORY;
                }
                if (data_size > enumerator->filename_size - sizeof(*enumerator->filename))
                {
                    free(enumerator->filename);
                    enumerator->filename_size = max(data_size + sizeof(*enumerator->filename), enumerator->filename_size * 2);
                    if (!(enumerator->filename = malloc(enumerator->filename_size)))
                    {
                        hr = E_OUTOFMEMORY;
                        goto err;
                    }
                }
            }
        } while (r == ERROR_MORE_DATA);

        if (r != ERROR_SUCCESS) {
            enumerator->filename[0] = 0;
            break;
        }
        enumerator->filename[data_size / sizeof(*enumerator->filename)] = 0;
        if (type == REG_SZ && *name != '@') {
            *current = TRUE;
            break;
        }
        enumerator->index++;
    }
    TRACE("index = %d, current = %d\n", enumerator->index, *current);

err:
    if (name != name_buf) free(name);
    return hr;
}

static const IDWriteFontFileEnumeratorVtbl systemfontfileenumeratorvtbl =
{
    systemfontfileenumerator_QueryInterface,
    systemfontfileenumerator_AddRef,
    systemfontfileenumerator_Release,
    systemfontfileenumerator_MoveNext,
    systemfontfileenumerator_GetCurrentFontFile
};

static HRESULT create_system_fontfile_enumerator(IDWriteFactory7 *factory, IDWriteFontFileEnumerator **ret)
{
    struct system_fontfile_enumerator *enumerator;

    *ret = NULL;

    if (!(enumerator = calloc(1, sizeof(*enumerator))))
        return E_OUTOFMEMORY;

    enumerator->IDWriteFontFileEnumerator_iface.lpVtbl = &systemfontfileenumeratorvtbl;
    enumerator->refcount = 1;
    enumerator->factory = factory;
    enumerator->index = -1;
    enumerator->filename_size = MAX_PATH * sizeof(*enumerator->filename);
    enumerator->filename = malloc(enumerator->filename_size);
    if (!enumerator->filename)
    {
        free(enumerator);
        return E_OUTOFMEMORY;
    }

    IDWriteFactory7_AddRef(factory);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0,
            GENERIC_READ, &enumerator->hkey))
    {
        ERR("failed to open fonts list key\n");
        IDWriteFactory7_Release(factory);
        free(enumerator->filename);
        free(enumerator);
        return E_FAIL;
    }

    *ret = &enumerator->IDWriteFontFileEnumerator_iface;

    return S_OK;
}

HRESULT get_system_fontcollection(IDWriteFactory7 *factory, DWRITE_FONT_FAMILY_MODEL family_model,
        IDWriteFontCollection **collection)
{
    IDWriteFontFileEnumerator *enumerator;
    IDWriteFontSet *fontset;
    HRESULT hr;

    *collection = NULL;

    if (family_model == DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC)
    {
        if (SUCCEEDED(hr = create_system_fontset(factory, &IID_IDWriteFontSet, (void **)&fontset)))
        {
            hr = create_font_collection_from_set(factory, fontset, family_model,
                    &IID_IDWriteFontCollection, (void **)collection);
            IDWriteFontSet_Release(fontset);
        }
    }
    else
    {
        if (SUCCEEDED(hr = create_system_fontfile_enumerator(factory, &enumerator)))
        {
            TRACE("Building system font collection for factory %p.\n", factory);
            hr = create_font_collection(factory, enumerator, TRUE, (IDWriteFontCollection3 **)collection);
            IDWriteFontFileEnumerator_Release(enumerator);
        }
    }

    return hr;
}

static HRESULT eudc_collection_add_family(IDWriteFactory7 *factory, struct dwrite_fontcollection *collection,
    const WCHAR *keynameW, const WCHAR *pathW)
{
    struct dwrite_fontfamily_data *family_data;
    IDWriteLocalizedStrings *names;
    DWRITE_FONT_FACE_TYPE face_type;
    DWRITE_FONT_FILE_TYPE file_type;
    IDWriteFontFileStream *stream;
    IDWriteFontFile *file;
    UINT32 face_count, i;
    BOOL supported;
    HRESULT hr;

    /* create font file from this path */
    hr = create_local_file_reference(factory, pathW, &file);
    if (FAILED(hr))
        return S_FALSE;

    if (FAILED(get_filestream_from_file(file, &stream))) {
        IDWriteFontFile_Release(file);
        return S_FALSE;
    }

    /* Unsupported formats are skipped. */
    hr = opentype_analyze_font(stream, &supported, &file_type, &face_type, &face_count);
    if (FAILED(hr) || !supported || face_count == 0) {
        TRACE("Unsupported font (%p, 0x%08lx, %d, %u)\n", file, hr, supported, face_count);
        IDWriteFontFileStream_Release(stream);
        IDWriteFontFile_Release(file);
        return S_FALSE;
    }

    /* create and init new family */

    /* Family names are added for non-specific locale, represented with empty string.
       Default family appears with empty family name. */
    create_localizedstrings(&names);
    if (!wcsicmp(keynameW, L"SystemDefaultEUDCFont"))
        add_localizedstring(names, L"", L"");
    else
        add_localizedstring(names, L"", keynameW);

    hr = init_fontfamily_data(names, &family_data);
    IDWriteLocalizedStrings_Release(names);
    if (hr != S_OK) {
        IDWriteFontFile_Release(file);
        return hr;
    }

    /* fill with faces */
    for (i = 0; i < face_count; i++) {
        struct dwrite_font_data *font_data;
        struct fontface_desc desc;

        /* Allocate new font data structure. */
        desc.factory = factory;
        desc.face_type = face_type;
        desc.index = i;
        desc.file = file;
        desc.stream = stream;
        desc.simulations = DWRITE_FONT_SIMULATIONS_NONE;
        desc.font_data = NULL;

        hr = init_font_data(&desc, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE, &font_data);
        if (FAILED(hr))
            continue;

        /* add font to family */
        hr = fontfamily_add_font(family_data, font_data);
        if (hr != S_OK)
            release_font_data(font_data);
    }

    /* add family to collection */
    hr = fontcollection_add_family(collection, family_data);
    if (FAILED(hr))
        release_fontfamily_data(family_data);
    IDWriteFontFileStream_Release(stream);
    IDWriteFontFile_Release(file);

    return hr;
}

HRESULT get_eudc_fontcollection(IDWriteFactory7 *factory, IDWriteFontCollection3 **ret)
{
    struct dwrite_fontcollection *collection;
    WCHAR eudckeypathW[16];
    HKEY eudckey;
    UINT32 index;
    BOOL exists;
    LONG retval;
    HRESULT hr;
    size_t i;

    TRACE("building EUDC font collection for factory %p, ACP %u\n", factory, GetACP());

    *ret = NULL;

    if (!(collection = calloc(1, sizeof(*collection))))
        return E_OUTOFMEMORY;

    hr = init_font_collection(collection, factory, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE, FALSE);
    if (FAILED(hr))
    {
        free(collection);
        return hr;
    }

    *ret = &collection->IDWriteFontCollection3_iface;

    /* return empty collection if EUDC fonts are not configured */
    swprintf(eudckeypathW, ARRAY_SIZE(eudckeypathW), L"EUDC\\%u", GetACP());
    if (RegOpenKeyExW(HKEY_CURRENT_USER, eudckeypathW, 0, GENERIC_READ, &eudckey))
        return S_OK;

    retval = ERROR_SUCCESS;
    index = 0;
    while (retval != ERROR_NO_MORE_ITEMS) {
        WCHAR keynameW[64], pathW[MAX_PATH];
        DWORD type, path_len, name_len;

        path_len = ARRAY_SIZE(pathW);
        name_len = ARRAY_SIZE(keynameW);
        retval = RegEnumValueW(eudckey, index++, keynameW, &name_len, NULL, &type, (BYTE*)pathW, &path_len);
        if (retval || type != REG_SZ)
            continue;

        hr = eudc_collection_add_family(factory, collection, keynameW, pathW);
        if (hr != S_OK)
            WARN("failed to add family %s, path %s\n", debugstr_w(keynameW), debugstr_w(pathW));
    }
    RegCloseKey(eudckey);

    /* try to add global default if not defined for specific codepage */
    exists = FALSE;
    hr = IDWriteFontCollection3_FindFamilyName(&collection->IDWriteFontCollection3_iface, L"",
        &index, &exists);
    if (FAILED(hr) || !exists)
    {
        hr = eudc_collection_add_family(factory, collection, L"", L"EUDC.TTE");
        if (hr != S_OK)
            WARN("failed to add global default EUDC font, 0x%08lx\n", hr);
    }

    /* EUDC collection offers simulated faces too */
    for (i = 0; i < collection->count; ++i)
    {
        fontfamily_add_bold_simulated_face(collection->family_data[i]);
        fontfamily_add_oblique_simulated_face(collection->family_data[i]);
    }

    return S_OK;
}

static HRESULT WINAPI dwritefontfile_QueryInterface(IDWriteFontFile *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFile))
    {
        *obj = iface;
        IDWriteFontFile_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontfile_AddRef(IDWriteFontFile *iface)
{
    struct dwrite_fontfile *file = impl_from_IDWriteFontFile(iface);
    ULONG refcount = InterlockedIncrement(&file->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritefontfile_Release(IDWriteFontFile *iface)
{
    struct dwrite_fontfile *file = impl_from_IDWriteFontFile(iface);
    ULONG refcount = InterlockedDecrement(&file->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFontFileLoader_Release(file->loader);
        if (file->stream)
            IDWriteFontFileStream_Release(file->stream);
        free(file->reference_key);
        free(file);
    }

    return refcount;
}

static HRESULT WINAPI dwritefontfile_GetReferenceKey(IDWriteFontFile *iface, const void **key, UINT32 *key_size)
{
    struct dwrite_fontfile *file = impl_from_IDWriteFontFile(iface);

    TRACE("%p, %p, %p.\n", iface, key, key_size);

    *key = file->reference_key;
    *key_size = file->key_size;

    return S_OK;
}

static HRESULT WINAPI dwritefontfile_GetLoader(IDWriteFontFile *iface, IDWriteFontFileLoader **loader)
{
    struct dwrite_fontfile *file = impl_from_IDWriteFontFile(iface);

    TRACE("%p, %p.\n", iface, loader);

    *loader = file->loader;
    IDWriteFontFileLoader_AddRef(*loader);

    return S_OK;
}

static HRESULT WINAPI dwritefontfile_Analyze(IDWriteFontFile *iface, BOOL *is_supported, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type, UINT32 *face_count)
{
    struct dwrite_fontfile *file = impl_from_IDWriteFontFile(iface);
    IDWriteFontFileStream *stream;
    HRESULT hr;

    TRACE("%p, %p, %p, %p, %p.\n", iface, is_supported, file_type, face_type, face_count);

    *is_supported = FALSE;
    *file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    if (face_type)
        *face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    *face_count = 0;

    hr = IDWriteFontFileLoader_CreateStreamFromKey(file->loader, file->reference_key, file->key_size, &stream);
    if (FAILED(hr))
        return hr;

    hr = opentype_analyze_font(stream, is_supported, file_type, face_type, face_count);

    /* TODO: Further Analysis */
    IDWriteFontFileStream_Release(stream);
    return S_OK;
}

static const IDWriteFontFileVtbl dwritefontfilevtbl =
{
    dwritefontfile_QueryInterface,
    dwritefontfile_AddRef,
    dwritefontfile_Release,
    dwritefontfile_GetReferenceKey,
    dwritefontfile_GetLoader,
    dwritefontfile_Analyze,
};

HRESULT create_font_file(IDWriteFontFileLoader *loader, const void *reference_key, UINT32 key_size,
        IDWriteFontFile **ret)
{
    struct dwrite_fontfile *file;
    void *key;

    *ret = NULL;

    file = calloc(1, sizeof(*file));
    key = malloc(key_size);
    if (!file || !key)
    {
        free(file);
        free(key);
        return E_OUTOFMEMORY;
    }

    file->IDWriteFontFile_iface.lpVtbl = &dwritefontfilevtbl;
    file->refcount = 1;
    IDWriteFontFileLoader_AddRef(loader);
    file->loader = loader;
    file->stream = NULL;
    file->reference_key = key;
    memcpy(file->reference_key, reference_key, key_size);
    file->key_size = key_size;

    *ret = &file->IDWriteFontFile_iface;

    return S_OK;
}

static UINT64 dwrite_fontface_get_font_object(struct dwrite_fontface *fontface)
{
    struct create_font_object_params create_params;
    struct release_font_object_params release_params;
    UINT64 font_object, size;
    const void *data_ptr;
    void *data_context;

    if (!fontface->font_object && SUCCEEDED(IDWriteFontFileStream_GetFileSize(fontface->stream, &size)))
    {
        if (SUCCEEDED(IDWriteFontFileStream_ReadFileFragment(fontface->stream, &data_ptr, 0, size, &data_context)))
        {
            create_params.data = data_ptr;
            create_params.size = size;
            create_params.index = fontface->index;
            create_params.object = &font_object;

            UNIX_CALL(create_font_object, &create_params);

            if (!font_object)
            {
                WARN("Backend failed to create font object.\n");
                IDWriteFontFileStream_ReleaseFileFragment(fontface->stream, data_context);
                return 0;
            }

            if (!InterlockedCompareExchange64((LONGLONG *)&fontface->font_object, font_object, 0))
            {
                fontface->data_context = data_context;
            }
            else
            {
                release_params.object = font_object;
                UNIX_CALL(release_font_object, &release_params);
                IDWriteFontFileStream_ReleaseFileFragment(fontface->stream, data_context);
            }
        }
    }

    return fontface->font_object;
}

HRESULT create_fontface(const struct fontface_desc *desc, struct list *cached_list, IDWriteFontFace5 **ret)
{
    struct file_stream_desc stream_desc;
    struct dwrite_font_data *font_data;
    struct dwrite_fontface *fontface;
    HRESULT hr;
    int i;

    *ret = NULL;

    if (!(fontface = calloc(1, sizeof(*fontface))))
        return E_OUTOFMEMORY;

    fontface->IDWriteFontFace5_iface.lpVtbl = &dwritefontfacevtbl;
    fontface->IDWriteFontFaceReference_iface.lpVtbl = &dwritefontface_reference_vtbl;
    fontface->refcount = 1;
    fontface->type = desc->face_type;
    fontface->vdmx.exists = TRUE;
    fontface->gasp.exists = TRUE;
    fontface->cpal.exists = TRUE;
    fontface->colr.exists = TRUE;
    fontface->kern.exists = TRUE;
    fontface->index = desc->index;
    fontface->simulations = desc->simulations;
    fontface->factory = desc->factory;
    IDWriteFactory7_AddRef(fontface->factory);
    fontface->file = desc->file;
    IDWriteFontFile_AddRef(fontface->file);
    fontface->stream = desc->stream;
    IDWriteFontFileStream_AddRef(fontface->stream);
    InitializeCriticalSection(&fontface->cs);
    fontface_cache_init(fontface);

    stream_desc.stream = fontface->stream;
    stream_desc.face_type = desc->face_type;
    stream_desc.face_index = desc->index;
    opentype_get_font_metrics(&stream_desc, &fontface->metrics, &fontface->caret);
    opentype_get_font_typo_metrics(&stream_desc, &fontface->typo_metrics.ascent, &fontface->typo_metrics.descent);
    if (desc->simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE) {
        /* TODO: test what happens if caret is already slanted */
        if (fontface->caret.slopeRise == 1) {
            fontface->caret.slopeRise = fontface->metrics.designUnitsPerEm;
            fontface->caret.slopeRun = fontface->caret.slopeRise / 3;
        }
    }
    fontface->glyph_image_formats = opentype_get_glyph_image_formats(&fontface->IDWriteFontFace5_iface);

    /* Font properties are reused from font object when 'normal' face creation path is used:
       collection -> family -> matching font -> fontface.

       If face is created directly from factory we have to go through properties resolution.
    */
    if (desc->font_data)
    {
        font_data = addref_font_data(desc->font_data);
    }
    else
    {
        hr = init_font_data(desc, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE, &font_data);
        if (FAILED(hr))
        {
            IDWriteFontFace5_Release(&fontface->IDWriteFontFace5_iface);
            return hr;
        }
    }

    fontface->weight = font_data->weight;
    fontface->style = font_data->style;
    fontface->stretch = font_data->stretch;
    fontface->panose = font_data->panose;
    fontface->fontsig = font_data->fontsig;
    fontface->lf = font_data->lf;
    fontface->flags |= font_data->flags & (FONT_IS_SYMBOL | FONT_IS_MONOSPACED | FONT_IS_COLORED);
    fontface->names = font_data->names;
    if (fontface->names)
        IDWriteLocalizedStrings_AddRef(fontface->names);
    fontface->family_names = font_data->family_names;
    if (fontface->family_names)
        IDWriteLocalizedStrings_AddRef(fontface->family_names);
    memcpy(fontface->info_strings, font_data->info_strings, sizeof(fontface->info_strings));
    for (i = 0; i < ARRAY_SIZE(fontface->info_strings); ++i)
    {
        if (fontface->info_strings[i])
            IDWriteLocalizedStrings_AddRef(fontface->info_strings[i]);
    }
    fontface->cmap.stream = fontface->stream;
    IDWriteFontFileStream_AddRef(fontface->cmap.stream);
    release_font_data(font_data);

    fontface->cached = factory_cache_fontface(fontface->factory, cached_list, &fontface->IDWriteFontFace5_iface);
    fontface->get_font_object = dwrite_fontface_get_font_object;

    *ret = &fontface->IDWriteFontFace5_iface;

    return S_OK;
}

/* IDWriteLocalFontFileLoader and its required IDWriteFontFileStream */
struct local_refkey
{
    FILETIME writetime;
    WCHAR name[1];
};

struct local_cached_stream
{
    struct list entry;
    IDWriteFontFileStream *stream;
    struct local_refkey *key;
    UINT32 key_size;
};

struct dwrite_localfontfilestream
{
    IDWriteFontFileStream IDWriteFontFileStream_iface;
    LONG refcount;

    struct local_cached_stream *entry;
    const void *file_ptr;
    UINT64 size;
};

struct dwrite_localfontfileloader
{
    IDWriteLocalFontFileLoader IDWriteLocalFontFileLoader_iface;
    LONG refcount;

    struct list streams;
    CRITICAL_SECTION cs;
};

static struct dwrite_localfontfileloader local_fontfile_loader;

struct dwrite_inmemory_stream_data
{
    LONG refcount;
    IUnknown *owner;
    void *data;
    UINT32 size;
};

struct dwrite_inmemory_filestream
{
    IDWriteFontFileStream IDWriteFontFileStream_iface;
    LONG refcount;

    struct dwrite_inmemory_stream_data *data;
};

struct dwrite_inmemory_fileloader
{
    IDWriteInMemoryFontFileLoader IDWriteInMemoryFontFileLoader_iface;
    LONG refcount;

    struct dwrite_inmemory_stream_data **streams;
    size_t size;
    size_t count;
};

static inline struct dwrite_localfontfileloader *impl_from_IDWriteLocalFontFileLoader(IDWriteLocalFontFileLoader *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_localfontfileloader, IDWriteLocalFontFileLoader_iface);
}

static inline struct dwrite_localfontfilestream *impl_from_IDWriteFontFileStream(IDWriteFontFileStream *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_localfontfilestream, IDWriteFontFileStream_iface);
}

static inline struct dwrite_inmemory_fileloader *impl_from_IDWriteInMemoryFontFileLoader(IDWriteInMemoryFontFileLoader *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_inmemory_fileloader, IDWriteInMemoryFontFileLoader_iface);
}

static inline struct dwrite_inmemory_filestream *inmemory_impl_from_IDWriteFontFileStream(IDWriteFontFileStream *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_inmemory_filestream, IDWriteFontFileStream_iface);
}

static void release_inmemory_stream(struct dwrite_inmemory_stream_data *stream)
{
    if (InterlockedDecrement(&stream->refcount) == 0)
    {
        if (stream->owner)
            IUnknown_Release(stream->owner);
        else
            free(stream->data);
        free(stream);
    }
}

static HRESULT WINAPI localfontfilestream_QueryInterface(IDWriteFontFileStream *iface, REFIID riid, void **obj)
{
    struct dwrite_localfontfilestream *stream = impl_from_IDWriteFontFileStream(iface);

    TRACE_(dwrite_file)("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontFileStream) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        if (InterlockedIncrement(&stream->refcount) == 1)
        {
            InterlockedDecrement(&stream->refcount);
            *obj = NULL;
            return E_FAIL;
        }
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI localfontfilestream_AddRef(IDWriteFontFileStream *iface)
{
    struct dwrite_localfontfilestream *stream = impl_from_IDWriteFontFileStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE_(dwrite_file)("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static inline void release_cached_stream(struct local_cached_stream *stream)
{
    list_remove(&stream->entry);
    free(stream->key);
    free(stream);
}

static ULONG WINAPI localfontfilestream_Release(IDWriteFontFileStream *iface)
{
    struct dwrite_localfontfilestream *stream = impl_from_IDWriteFontFileStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE_(dwrite_file)("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        UnmapViewOfFile(stream->file_ptr);

        EnterCriticalSection(&local_fontfile_loader.cs);
        release_cached_stream(stream->entry);
        LeaveCriticalSection(&local_fontfile_loader.cs);

        free(stream);
    }

    return refcount;
}

static HRESULT WINAPI localfontfilestream_ReadFileFragment(IDWriteFontFileStream *iface, void const **fragment_start,
    UINT64 offset, UINT64 fragment_size, void **fragment_context)
{
    struct dwrite_localfontfilestream *stream = impl_from_IDWriteFontFileStream(iface);

    TRACE_(dwrite_file)("%p, %p, 0x%s, 0x%s, %p.\n", iface, fragment_start,
          wine_dbgstr_longlong(offset), wine_dbgstr_longlong(fragment_size), fragment_context);

    *fragment_context = NULL;

    if ((offset >= stream->size - 1) || (fragment_size > stream->size - offset))
    {
        *fragment_start = NULL;
        return E_FAIL;
    }

    *fragment_start = (char *)stream->file_ptr + offset;
    return S_OK;
}

static void WINAPI localfontfilestream_ReleaseFileFragment(IDWriteFontFileStream *iface, void *fragment_context)
{
    TRACE_(dwrite_file)("%p, %p.\n", iface, fragment_context);
}

static HRESULT WINAPI localfontfilestream_GetFileSize(IDWriteFontFileStream *iface, UINT64 *size)
{
    struct dwrite_localfontfilestream *stream = impl_from_IDWriteFontFileStream(iface);

    TRACE_(dwrite_file)("%p, %p.\n", iface, size);

    *size = stream->size;
    return S_OK;
}

static HRESULT WINAPI localfontfilestream_GetLastWriteTime(IDWriteFontFileStream *iface, UINT64 *last_writetime)
{
    struct dwrite_localfontfilestream *stream = impl_from_IDWriteFontFileStream(iface);
    ULARGE_INTEGER li;

    TRACE_(dwrite_file)("%p, %p.\n", iface, last_writetime);

    li.u.LowPart = stream->entry->key->writetime.dwLowDateTime;
    li.u.HighPart = stream->entry->key->writetime.dwHighDateTime;
    *last_writetime = li.QuadPart;

    return S_OK;
}

static const IDWriteFontFileStreamVtbl localfontfilestreamvtbl =
{
    localfontfilestream_QueryInterface,
    localfontfilestream_AddRef,
    localfontfilestream_Release,
    localfontfilestream_ReadFileFragment,
    localfontfilestream_ReleaseFileFragment,
    localfontfilestream_GetFileSize,
    localfontfilestream_GetLastWriteTime
};

static HRESULT create_localfontfilestream(const void *file_ptr, UINT64 size, struct local_cached_stream *entry,
        IDWriteFontFileStream **ret)
{
    struct dwrite_localfontfilestream *object;

    *ret = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteFontFileStream_iface.lpVtbl = &localfontfilestreamvtbl;
    object->refcount = 1;

    object->file_ptr = file_ptr;
    object->size = size;
    object->entry = entry;

    *ret = &object->IDWriteFontFileStream_iface;

    return S_OK;
}

static HRESULT WINAPI localfontfileloader_QueryInterface(IDWriteLocalFontFileLoader *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteLocalFontFileLoader) ||
        IsEqualIID(riid, &IID_IDWriteFontFileLoader) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteLocalFontFileLoader_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI localfontfileloader_AddRef(IDWriteLocalFontFileLoader *iface)
{
    struct dwrite_localfontfileloader *loader = impl_from_IDWriteLocalFontFileLoader(iface);
    ULONG refcount = InterlockedIncrement(&loader->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI localfontfileloader_Release(IDWriteLocalFontFileLoader *iface)
{
    struct dwrite_localfontfileloader *loader = impl_from_IDWriteLocalFontFileLoader(iface);
    ULONG refcount = InterlockedDecrement(&loader->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static HRESULT create_local_cached_stream(const void *key, UINT32 key_size, struct local_cached_stream **ret)
{
    const struct local_refkey *refkey = key;
    struct local_cached_stream *stream;
    IDWriteFontFileStream *filestream;
    HANDLE file, mapping;
    LARGE_INTEGER size;
    void *file_ptr;
    HRESULT hr = S_OK;

    *ret = NULL;

    file = CreateFileW(refkey->name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        WARN_(dwrite_file)("Failed to open the file %s, error %ld.\n", debugstr_w(refkey->name), GetLastError());
        return E_FAIL;
    }

    GetFileSizeEx(file, &size);
    mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if (!mapping)
        return E_FAIL;

    file_ptr = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);
    if (!file_ptr) {
        ERR("mapping failed, file size %s, error %ld\n", wine_dbgstr_longlong(size.QuadPart), GetLastError());
        return E_FAIL;
    }

    if (!(stream = malloc(sizeof(*stream))))
    {
        UnmapViewOfFile(file_ptr);
        return E_OUTOFMEMORY;
    }

    if (!(stream->key = malloc(key_size)))
    {
        UnmapViewOfFile(file_ptr);
        free(stream);
        return E_OUTOFMEMORY;
    }

    stream->key_size = key_size;
    memcpy(stream->key, key, key_size);

    if (FAILED(hr = create_localfontfilestream(file_ptr, size.QuadPart, stream, &filestream)))
    {
        UnmapViewOfFile(file_ptr);
        free(stream->key);
        free(stream);
        return hr;
    }

    stream->stream = filestream;

    *ret = stream;

    return S_OK;
}

static HRESULT WINAPI localfontfileloader_CreateStreamFromKey(IDWriteLocalFontFileLoader *iface, const void *key,
        UINT32 key_size, IDWriteFontFileStream **ret)
{
    struct dwrite_localfontfileloader *loader = impl_from_IDWriteLocalFontFileLoader(iface);
    struct local_cached_stream *stream;
    HRESULT hr = S_OK;

    TRACE("%p, %p, %u, %p.\n", iface, key, key_size, ret);

    EnterCriticalSection(&loader->cs);

    *ret = NULL;

    /* search cache first */
    LIST_FOR_EACH_ENTRY(stream, &loader->streams, struct local_cached_stream, entry)
    {
        if (key_size == stream->key_size && !memcmp(stream->key, key, key_size)) {
            IDWriteFontFileStream_QueryInterface(stream->stream, &IID_IDWriteFontFileStream, (void **)ret);
            break;
        }
    }

    if (*ret == NULL && (hr = create_local_cached_stream(key, key_size, &stream)) == S_OK)
    {
        list_add_head(&loader->streams, &stream->entry);
        *ret = stream->stream;
    }

    LeaveCriticalSection(&loader->cs);

    return hr;
}

static HRESULT WINAPI localfontfileloader_GetFilePathLengthFromKey(IDWriteLocalFontFileLoader *iface, void const *key,
        UINT32 key_size, UINT32 *length)
{
    const struct local_refkey *refkey = key;

    TRACE("%p, %p, %u, %p.\n", iface, key, key_size, length);

    *length = wcslen(refkey->name);
    return S_OK;
}

static HRESULT WINAPI localfontfileloader_GetFilePathFromKey(IDWriteLocalFontFileLoader *iface, void const *key,
        UINT32 key_size, WCHAR *path, UINT32 length)
{
    const struct local_refkey *refkey = key;

    TRACE("%p, %p, %u, %p, %u.\n", iface, key, key_size, path, length);

    if (length < wcslen(refkey->name))
        return E_INVALIDARG;

    wcscpy(path, refkey->name);
    return S_OK;
}

static HRESULT WINAPI localfontfileloader_GetLastWriteTimeFromKey(IDWriteLocalFontFileLoader *iface, void const *key,
        UINT32 key_size, FILETIME *writetime)
{
    const struct local_refkey *refkey = key;

    TRACE("%p, %p, %u, %p.\n", iface, key, key_size, writetime);

    *writetime = refkey->writetime;
    return S_OK;
}

static const IDWriteLocalFontFileLoaderVtbl localfontfileloadervtbl =
{
    localfontfileloader_QueryInterface,
    localfontfileloader_AddRef,
    localfontfileloader_Release,
    localfontfileloader_CreateStreamFromKey,
    localfontfileloader_GetFilePathLengthFromKey,
    localfontfileloader_GetFilePathFromKey,
    localfontfileloader_GetLastWriteTimeFromKey
};

void init_local_fontfile_loader(void)
{
    local_fontfile_loader.IDWriteLocalFontFileLoader_iface.lpVtbl = &localfontfileloadervtbl;
    local_fontfile_loader.refcount = 1;
    list_init(&local_fontfile_loader.streams);
    InitializeCriticalSectionEx(&local_fontfile_loader.cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    local_fontfile_loader.cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": localfileloader.lock");
}

IDWriteFontFileLoader *get_local_fontfile_loader(void)
{
    return (IDWriteFontFileLoader *)&local_fontfile_loader.IDWriteLocalFontFileLoader_iface;
}

HRESULT get_local_refkey(const WCHAR *path, const FILETIME *writetime, void **key, UINT32 *size)
{
    struct local_refkey *refkey;

    if (!path)
        return E_INVALIDARG;

    *size = FIELD_OFFSET(struct local_refkey, name) + (wcslen(path)+1)*sizeof(WCHAR);
    *key = NULL;

    if (!(refkey = malloc(*size)))
        return E_OUTOFMEMORY;

    if (writetime)
        refkey->writetime = *writetime;
    else {
        WIN32_FILE_ATTRIBUTE_DATA info;

        if (GetFileAttributesExW(path, GetFileExInfoStandard, &info))
            refkey->writetime = info.ftLastWriteTime;
        else
            memset(&refkey->writetime, 0, sizeof(refkey->writetime));
    }
    wcscpy(refkey->name, path);

    *key = refkey;

    return S_OK;
}

static HRESULT WINAPI glyphrunanalysis_QueryInterface(IDWriteGlyphRunAnalysis *iface, REFIID riid, void **ppv)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IDWriteGlyphRunAnalysis) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        IDWriteGlyphRunAnalysis_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI glyphrunanalysis_AddRef(IDWriteGlyphRunAnalysis *iface)
{
    struct dwrite_glyphrunanalysis *analysis = impl_from_IDWriteGlyphRunAnalysis(iface);
    ULONG refcount = InterlockedIncrement(&analysis->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI glyphrunanalysis_Release(IDWriteGlyphRunAnalysis *iface)
{
    struct dwrite_glyphrunanalysis *analysis = impl_from_IDWriteGlyphRunAnalysis(iface);
    ULONG refcount = InterlockedDecrement(&analysis->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        if (analysis->run.fontFace)
            IDWriteFontFace_Release(analysis->run.fontFace);
        free(analysis->glyphs);
        free(analysis->origins);
        free(analysis->bitmap);
        free(analysis);
    }

    return refcount;
}

static void glyphrunanalysis_get_texturebounds(struct dwrite_glyphrunanalysis *analysis, RECT *bounds)
{
    struct dwrite_glyphbitmap glyph_bitmap;
    UINT32 i;

    if (analysis->flags & RUNANALYSIS_BOUNDS_READY) {
        *bounds = analysis->bounds;
        return;
    }

    if (analysis->run.isSideways)
        FIXME("sideways runs are not supported.\n");

    memset(&glyph_bitmap, 0, sizeof(glyph_bitmap));
    glyph_bitmap.simulations = IDWriteFontFace_GetSimulations(analysis->run.fontFace);
    glyph_bitmap.emsize = analysis->run.fontEmSize;
    if (analysis->flags & RUNANALYSIS_USE_TRANSFORM)
        glyph_bitmap.m = &analysis->m;

    for (i = 0; i < analysis->run.glyphCount; i++) {
        RECT *bbox = &glyph_bitmap.bbox;
        UINT32 bitmap_size;

        glyph_bitmap.glyph = analysis->run.glyphIndices[i];
        dwrite_fontface_get_glyph_bbox(analysis->run.fontFace, &glyph_bitmap);

        bitmap_size = get_glyph_bitmap_pitch(analysis->rendering_mode, bbox->right - bbox->left) *
            (bbox->bottom - bbox->top);
        if (bitmap_size > analysis->max_glyph_bitmap_size)
            analysis->max_glyph_bitmap_size = bitmap_size;

        OffsetRect(bbox, analysis->origins[i].x, analysis->origins[i].y);
        UnionRect(&analysis->bounds, &analysis->bounds, bbox);
    }

    analysis->flags |= RUNANALYSIS_BOUNDS_READY;
    *bounds = analysis->bounds;
}

static HRESULT WINAPI glyphrunanalysis_GetAlphaTextureBounds(IDWriteGlyphRunAnalysis *iface,
        DWRITE_TEXTURE_TYPE type, RECT *bounds)
{
    struct dwrite_glyphrunanalysis *analysis = impl_from_IDWriteGlyphRunAnalysis(iface);

    TRACE("%p, %d, %p.\n", iface, type, bounds);

    if ((UINT32)type > DWRITE_TEXTURE_CLEARTYPE_3x1) {
        SetRectEmpty(bounds);
        return E_INVALIDARG;
    }

    if (type != analysis->texture_type)
    {
        SetRectEmpty(bounds);
        return S_OK;
    }

    glyphrunanalysis_get_texturebounds(analysis, bounds);
    return S_OK;
}

static inline BYTE *get_pixel_ptr(BYTE *ptr, DWRITE_TEXTURE_TYPE type, const RECT *runbounds, const RECT *bounds)
{
    if (type == DWRITE_TEXTURE_CLEARTYPE_3x1)
        return ptr + (runbounds->top - bounds->top) * (bounds->right - bounds->left) * 3 +
            (runbounds->left - bounds->left) * 3;
    else
        return ptr + (runbounds->top - bounds->top) * (bounds->right - bounds->left) +
            runbounds->left - bounds->left;
}

static HRESULT glyphrunanalysis_render(struct dwrite_glyphrunanalysis *analysis)
{
    static const BYTE masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    struct dwrite_fontface *fontface = unsafe_impl_from_IDWriteFontFace(analysis->run.fontFace);
    struct dwrite_glyphbitmap glyph_bitmap;
    D2D_POINT_2F origin;
    UINT32 i, size;
    RECT *bbox;

    size = (analysis->bounds.right - analysis->bounds.left)*(analysis->bounds.bottom - analysis->bounds.top);
    if (analysis->texture_type == DWRITE_TEXTURE_CLEARTYPE_3x1)
        size *= 3;
    if (!(analysis->bitmap = calloc(1, size)))
    {
        WARN("Failed to allocate run bitmap, %s, type %s.\n", wine_dbgstr_rect(&analysis->bounds),
                analysis->texture_type == DWRITE_TEXTURE_CLEARTYPE_3x1 ? "3x1" : "1x1");
        return E_OUTOFMEMORY;
    }

    origin.x = origin.y = 0.0f;

    memset(&glyph_bitmap, 0, sizeof(glyph_bitmap));
    glyph_bitmap.simulations = fontface->simulations;
    glyph_bitmap.emsize = analysis->run.fontEmSize;
    if (analysis->flags & RUNANALYSIS_USE_TRANSFORM)
        glyph_bitmap.m = &analysis->m;
    if (!(glyph_bitmap.buf = malloc(analysis->max_glyph_bitmap_size)))
        return E_OUTOFMEMORY;

    bbox = &glyph_bitmap.bbox;

    for (i = 0; i < analysis->run.glyphCount; ++i)
    {
        BYTE *src = glyph_bitmap.buf, *dst;
        int x, y, width, height;
        unsigned int is_1bpp;

        glyph_bitmap.glyph = analysis->run.glyphIndices[i];
        dwrite_fontface_get_glyph_bbox(analysis->run.fontFace, &glyph_bitmap);

        if (IsRectEmpty(bbox))
            continue;

        width = bbox->right - bbox->left;
        height = bbox->bottom - bbox->top;

        glyph_bitmap.pitch = get_glyph_bitmap_pitch(analysis->rendering_mode, width);
        memset(src, 0, height * glyph_bitmap.pitch);

        if (FAILED(dwrite_fontface_get_glyph_bitmap(fontface, analysis->rendering_mode, &is_1bpp, &glyph_bitmap)))
        {
            WARN("Failed to render glyph[%u] = %#x.\n", i, glyph_bitmap.glyph);
            continue;
        }

        OffsetRect(bbox, analysis->origins[i].x, analysis->origins[i].y);

        /* blit to analysis bitmap */
        dst = get_pixel_ptr(analysis->bitmap, analysis->texture_type, bbox, &analysis->bounds);

        if (is_1bpp) {
            /* convert 1bpp to 8bpp/24bpp */
            if (analysis->texture_type == DWRITE_TEXTURE_CLEARTYPE_3x1) {
                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++)
                        if (src[x / 8] & masks[x % 8])
                            dst[3*x] = dst[3*x+1] = dst[3*x+2] = DWRITE_ALPHA_MAX;
                    src += glyph_bitmap.pitch;
                    dst += (analysis->bounds.right - analysis->bounds.left) * 3;
                }
            }
            else {
                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++)
                        if (src[x / 8] & masks[x % 8])
                            dst[x] = DWRITE_ALPHA_MAX;
                    src += glyph_bitmap.pitch;
                    dst += analysis->bounds.right - analysis->bounds.left;
                }
            }
        }
        else {
            if (analysis->texture_type == DWRITE_TEXTURE_CLEARTYPE_3x1) {
                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++)
                        dst[3*x] = dst[3*x+1] = dst[3*x+2] = src[x] | dst[3*x];
                    src += glyph_bitmap.pitch;
                    dst += (analysis->bounds.right - analysis->bounds.left) * 3;
                }
            }
            else {
                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++)
                        dst[x] |= src[x];
                    src += glyph_bitmap.pitch;
                    dst += analysis->bounds.right - analysis->bounds.left;
                }
            }
        }
    }
    free(glyph_bitmap.buf);

    analysis->flags |= RUNANALYSIS_BITMAP_READY;

    /* we don't need this anymore */
    free(analysis->glyphs);
    free(analysis->origins);
    IDWriteFontFace_Release(analysis->run.fontFace);

    analysis->glyphs = NULL;
    analysis->origins = NULL;
    analysis->run.glyphIndices = NULL;
    analysis->run.fontFace = NULL;

    return S_OK;
}

static HRESULT WINAPI glyphrunanalysis_CreateAlphaTexture(IDWriteGlyphRunAnalysis *iface, DWRITE_TEXTURE_TYPE type,
    RECT const *bounds, BYTE *bitmap, UINT32 size)
{
    struct dwrite_glyphrunanalysis *analysis = impl_from_IDWriteGlyphRunAnalysis(iface);
    UINT32 required;
    RECT runbounds;

    TRACE("%p, %d, %s, %p, %u.\n", iface, type, wine_dbgstr_rect(bounds), bitmap, size);

    if (!bounds || !bitmap || (UINT32)type > DWRITE_TEXTURE_CLEARTYPE_3x1)
        return E_INVALIDARG;

    /* make sure buffer is large enough for requested texture type */
    required = (bounds->right - bounds->left) * (bounds->bottom - bounds->top);
    if (analysis->texture_type == DWRITE_TEXTURE_CLEARTYPE_3x1)
        required *= 3;

    if (size < required)
        return E_NOT_SUFFICIENT_BUFFER;

    /* validate requested texture type */
    if (analysis->texture_type != type)
        return DWRITE_E_UNSUPPORTEDOPERATION;

    memset(bitmap, 0, size);
    glyphrunanalysis_get_texturebounds(analysis, &runbounds);
    if (IntersectRect(&runbounds, &runbounds, bounds))
    {
        int pixel_size = type == DWRITE_TEXTURE_CLEARTYPE_3x1 ? 3 : 1;
        int src_width = (analysis->bounds.right - analysis->bounds.left) * pixel_size;
        int dst_width = (bounds->right - bounds->left) * pixel_size;
        int draw_width = (runbounds.right - runbounds.left) * pixel_size;
        BYTE *src, *dst;
        int y;

        if (!(analysis->flags & RUNANALYSIS_BITMAP_READY))
        {
            HRESULT hr;

            if (FAILED(hr = glyphrunanalysis_render(analysis)))
                return hr;
        }

        src = get_pixel_ptr(analysis->bitmap, type, &runbounds, &analysis->bounds);
        dst = get_pixel_ptr(bitmap, type, &runbounds, bounds);

        for (y = 0; y < runbounds.bottom - runbounds.top; y++) {
            memcpy(dst, src, draw_width);
            src += src_width;
            dst += dst_width;
        }
    }

    return S_OK;
}

static HRESULT WINAPI glyphrunanalysis_GetAlphaBlendParams(IDWriteGlyphRunAnalysis *iface, IDWriteRenderingParams *params,
    FLOAT *gamma, FLOAT *contrast, FLOAT *cleartypelevel)
{
    struct dwrite_glyphrunanalysis *analysis = impl_from_IDWriteGlyphRunAnalysis(iface);

    TRACE("%p, %p, %p, %p, %p.\n", iface, params, gamma, contrast, cleartypelevel);

    if (!params)
        return E_INVALIDARG;

    switch (analysis->rendering_mode)
    {
    case DWRITE_RENDERING_MODE1_GDI_CLASSIC:
    case DWRITE_RENDERING_MODE1_GDI_NATURAL:
    {
        UINT value = 0;
        SystemParametersInfoW(SPI_GETFONTSMOOTHINGCONTRAST, 0, &value, 0);
        *gamma = (FLOAT)value / 1000.0f;
        *contrast = 0.0f;
        *cleartypelevel = 1.0f;
        break;
    }
    case DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC_DOWNSAMPLED:
        WARN("NATURAL_SYMMETRIC_DOWNSAMPLED mode is ignored.\n");
        /* fallthrough */
    case DWRITE_RENDERING_MODE1_ALIASED:
    case DWRITE_RENDERING_MODE1_NATURAL:
    case DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC:
        *gamma = IDWriteRenderingParams_GetGamma(params);
        *contrast = IDWriteRenderingParams_GetEnhancedContrast(params);
        *cleartypelevel = IDWriteRenderingParams_GetClearTypeLevel(params);
        break;
    default:
        ;
    }

    return S_OK;
}

static const IDWriteGlyphRunAnalysisVtbl glyphrunanalysisvtbl =
{
    glyphrunanalysis_QueryInterface,
    glyphrunanalysis_AddRef,
    glyphrunanalysis_Release,
    glyphrunanalysis_GetAlphaTextureBounds,
    glyphrunanalysis_CreateAlphaTexture,
    glyphrunanalysis_GetAlphaBlendParams
};

static inline void transform_point(D2D_POINT_2F *point, const DWRITE_MATRIX *m)
{
    D2D_POINT_2F ret;
    ret.x = point->x * m->m11 + point->y * m->m21 + m->dx;
    ret.y = point->x * m->m12 + point->y * m->m22 + m->dy;
    *point = ret;
}

float fontface_get_scaled_design_advance(struct dwrite_fontface *fontface, DWRITE_MEASURING_MODE measuring_mode,
        float emsize, float ppdip, const DWRITE_MATRIX *transform, UINT16 glyph, BOOL is_sideways)
{
    unsigned int upem = fontface->metrics.designUnitsPerEm;
    int advance;

    if (is_sideways)
        FIXME("Sideways mode is not supported.\n");

    EnterCriticalSection(&fontface->cs);
    advance = fontface_get_design_advance(fontface, measuring_mode, emsize, ppdip, transform, glyph, is_sideways);
    LeaveCriticalSection(&fontface->cs);

    switch (measuring_mode)
    {
        case DWRITE_MEASURING_MODE_NATURAL:
            return (float)advance * emsize / (float)upem;
        case DWRITE_MEASURING_MODE_GDI_NATURAL:
        case DWRITE_MEASURING_MODE_GDI_CLASSIC:
            return ppdip > 0.0f ? floorf(advance * emsize * ppdip / upem + 0.5f) / ppdip : 0.0f;
        default:
            WARN("Unknown measuring mode %u.\n", measuring_mode);
            return 0.0f;
    }
}

HRESULT create_glyphrunanalysis(const struct glyphrunanalysis_desc *desc, IDWriteGlyphRunAnalysis **ret)
{
    struct dwrite_glyphrunanalysis *analysis;
    unsigned int i;

    *ret = NULL;

    /* Check rendering, antialiasing, measuring, and grid fitting modes. */
    if ((UINT32)desc->rendering_mode >= DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC_DOWNSAMPLED ||
            desc->rendering_mode == DWRITE_RENDERING_MODE1_OUTLINE ||
            desc->rendering_mode == DWRITE_RENDERING_MODE1_DEFAULT)
        return E_INVALIDARG;

    if ((UINT32)desc->aa_mode > DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE)
        return E_INVALIDARG;

    if ((UINT32)desc->gridfit_mode > DWRITE_GRID_FIT_MODE_ENABLED)
        return E_INVALIDARG;

    if ((UINT32)desc->measuring_mode > DWRITE_MEASURING_MODE_GDI_NATURAL)
        return E_INVALIDARG;

    if (!(analysis = calloc(1, sizeof(*analysis))))
        return E_OUTOFMEMORY;

    analysis->IDWriteGlyphRunAnalysis_iface.lpVtbl = &glyphrunanalysisvtbl;
    analysis->refcount = 1;
    analysis->rendering_mode = desc->rendering_mode;

    if (desc->rendering_mode == DWRITE_RENDERING_MODE1_ALIASED
            || desc->aa_mode == DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE)
        analysis->texture_type = DWRITE_TEXTURE_ALIASED_1x1;
    else
        analysis->texture_type = DWRITE_TEXTURE_CLEARTYPE_3x1;

    analysis->run = *desc->run;
    IDWriteFontFace_AddRef(analysis->run.fontFace);
    analysis->glyphs = calloc(desc->run->glyphCount, sizeof(*analysis->glyphs));
    analysis->origins = calloc(desc->run->glyphCount, sizeof(*analysis->origins));

    if (!analysis->glyphs || !analysis->origins)
    {
        free(analysis->glyphs);
        free(analysis->origins);

        analysis->glyphs = NULL;
        analysis->origins = NULL;

        IDWriteGlyphRunAnalysis_Release(&analysis->IDWriteGlyphRunAnalysis_iface);
        return E_OUTOFMEMORY;
    }

    /* check if transform is usable */
    if (desc->transform && memcmp(desc->transform, &identity, sizeof(*desc->transform))) {
        analysis->m = *desc->transform;
        analysis->flags |= RUNANALYSIS_USE_TRANSFORM;
    }

    analysis->run.glyphIndices = analysis->glyphs;
    memcpy(analysis->glyphs, desc->run->glyphIndices, desc->run->glyphCount*sizeof(*desc->run->glyphIndices));

    compute_glyph_origins(desc->run, desc->measuring_mode, desc->origin, desc->transform, analysis->origins);
    if (analysis->flags & RUNANALYSIS_USE_TRANSFORM)
    {
        for (i = 0; i < desc->run->glyphCount; ++i)
            transform_point(&analysis->origins[i], &analysis->m);
    }

    *ret = &analysis->IDWriteGlyphRunAnalysis_iface;
    return S_OK;
}

/* IDWriteColorGlyphRunEnumerator1 */
static HRESULT WINAPI colorglyphenum_QueryInterface(IDWriteColorGlyphRunEnumerator1 *iface, REFIID riid, void **ppv)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IDWriteColorGlyphRunEnumerator1) ||
        IsEqualIID(riid, &IID_IDWriteColorGlyphRunEnumerator) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        IDWriteColorGlyphRunEnumerator1_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI colorglyphenum_AddRef(IDWriteColorGlyphRunEnumerator1 *iface)
{
    struct dwrite_colorglyphenum *glyphenum = impl_from_IDWriteColorGlyphRunEnumerator1(iface);
    ULONG refcount = InterlockedIncrement(&glyphenum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI colorglyphenum_Release(IDWriteColorGlyphRunEnumerator1 *iface)
{
    struct dwrite_colorglyphenum *glyphenum = impl_from_IDWriteColorGlyphRunEnumerator1(iface);
    ULONG refcount = InterlockedDecrement(&glyphenum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        free(glyphenum->advances);
        free(glyphenum->color_advances);
        free(glyphenum->offsets);
        free(glyphenum->color_offsets);
        free(glyphenum->glyphindices);
        free(glyphenum->glyphs);
        if (glyphenum->colr.context)
            IDWriteFontFace5_ReleaseFontTable(glyphenum->fontface, glyphenum->colr.context);
        IDWriteFontFace5_Release(glyphenum->fontface);
        free(glyphenum);
    }

    return refcount;
}

static FLOAT get_glyph_origin(const struct dwrite_colorglyphenum *glyphenum, UINT32 g)
{
    BOOL is_rtl = glyphenum->run.bidiLevel & 1;
    FLOAT origin = 0.0f;

    if (g == 0)
        return 0.0f;

    while (g--)
        origin += is_rtl ? -glyphenum->advances[g] : glyphenum->advances[g];
    return origin;
}

static BOOL colorglyphenum_build_color_run(struct dwrite_colorglyphenum *glyphenum)
{
    DWRITE_COLOR_GLYPH_RUN1 *colorrun = &glyphenum->colorrun;
    FLOAT advance_adj = 0.0f;
    BOOL got_palette_index;
    UINT32 g;

    /* start with regular glyphs */
    if (glyphenum->current_layer == 0 && glyphenum->has_regular_glyphs) {
        UINT32 first_glyph = 0;

        for (g = 0; g < glyphenum->run.glyphCount; g++) {
            if (glyphenum->glyphs[g].num_layers == 0) {
                glyphenum->glyphindices[g] = glyphenum->glyphs[g].glyph;
                first_glyph = min(first_glyph, g);
            }
            else
                glyphenum->glyphindices[g] = 1;
            glyphenum->color_advances[g] = glyphenum->advances[g];
            if (glyphenum->color_offsets)
                glyphenum->color_offsets[g] = glyphenum->offsets[g];
        }

        colorrun->baselineOriginX = glyphenum->origin.x + get_glyph_origin(glyphenum, first_glyph);
        colorrun->baselineOriginY = glyphenum->origin.y;
        colorrun->glyphRun.glyphCount = glyphenum->run.glyphCount;
        colorrun->paletteIndex = 0xffff;
        memset(&colorrun->runColor, 0, sizeof(colorrun->runColor));
        glyphenum->has_regular_glyphs = FALSE;
        return TRUE;
    }
    else {
        colorrun->glyphRun.glyphCount = 0;
        got_palette_index = FALSE;
    }

    advance_adj = 0.0f;
    for (g = 0; g < glyphenum->run.glyphCount; g++) {

        glyphenum->glyphindices[g] = 1;

        /* all glyph layers were returned */
        if (glyphenum->glyphs[g].layer == glyphenum->glyphs[g].num_layers) {
            advance_adj += glyphenum->advances[g];
            continue;
        }

        if (glyphenum->current_layer == glyphenum->glyphs[g].layer && (!got_palette_index || colorrun->paletteIndex == glyphenum->glyphs[g].palette_index)) {
            UINT32 index = colorrun->glyphRun.glyphCount;
            if (!got_palette_index) {
                colorrun->paletteIndex = glyphenum->glyphs[g].palette_index;
                /* use foreground color or request one from the font */
                memset(&colorrun->runColor, 0, sizeof(colorrun->runColor));
                if (colorrun->paletteIndex != 0xffff)
                {
                    HRESULT hr = IDWriteFontFace5_GetPaletteEntries(glyphenum->fontface, glyphenum->palette,
                            colorrun->paletteIndex, 1, &colorrun->runColor);
                    if (FAILED(hr))
                        WARN("failed to get palette entry, fontface %p, palette %u, index %u, 0x%08lx\n", glyphenum->fontface,
                            glyphenum->palette, colorrun->paletteIndex, hr);
                }
                /* found a glyph position new color run starts from, origin is "original origin + distance to this glyph" */
                colorrun->baselineOriginX = glyphenum->origin.x + get_glyph_origin(glyphenum, g);
                colorrun->baselineOriginY = glyphenum->origin.y;
                glyphenum->color_advances[index] = glyphenum->advances[g];
                got_palette_index = TRUE;
            }

            glyphenum->glyphindices[index] = glyphenum->glyphs[g].glyph;
            /* offsets are relative to glyph origin, nothing to fix up */
            if (glyphenum->color_offsets)
                glyphenum->color_offsets[index] = glyphenum->offsets[g];
            opentype_colr_next_glyph(&glyphenum->colr, glyphenum->glyphs + g);
            if (index)
                glyphenum->color_advances[index-1] += advance_adj;
            colorrun->glyphRun.glyphCount++;
            advance_adj = 0.0f;
        }
        else
            advance_adj += glyphenum->advances[g];
    }

    /* reset last advance */
    if (colorrun->glyphRun.glyphCount)
        glyphenum->color_advances[colorrun->glyphRun.glyphCount-1] = 0.0f;

    return colorrun->glyphRun.glyphCount > 0;
}

static HRESULT WINAPI colorglyphenum_MoveNext(IDWriteColorGlyphRunEnumerator1 *iface, BOOL *has_run)
{
    struct dwrite_colorglyphenum *glyphenum = impl_from_IDWriteColorGlyphRunEnumerator1(iface);

    TRACE("%p, %p.\n", iface, has_run);

    *has_run = FALSE;

    glyphenum->colorrun.glyphRun.glyphCount = 0;
    while (glyphenum->current_layer < glyphenum->max_layer_num)
    {
        if (colorglyphenum_build_color_run(glyphenum))
            break;
        else
            glyphenum->current_layer++;
    }

    *has_run = glyphenum->colorrun.glyphRun.glyphCount > 0;

    return S_OK;
}

static HRESULT colorglyphenum_get_current_run(const struct dwrite_colorglyphenum *glyphenum,
        DWRITE_COLOR_GLYPH_RUN1 const **run)
{
    if (glyphenum->colorrun.glyphRun.glyphCount == 0)
    {
        *run = NULL;
        return E_NOT_VALID_STATE;
    }

    *run = &glyphenum->colorrun;
    return S_OK;
}

static HRESULT WINAPI colorglyphenum_GetCurrentRun(IDWriteColorGlyphRunEnumerator1 *iface,
        DWRITE_COLOR_GLYPH_RUN const **run)
{
    struct dwrite_colorglyphenum *glyphenum = impl_from_IDWriteColorGlyphRunEnumerator1(iface);

    TRACE("%p, %p.\n", iface, run);

    return colorglyphenum_get_current_run(glyphenum, (DWRITE_COLOR_GLYPH_RUN1 const **)run);
}

static HRESULT WINAPI colorglyphenum1_GetCurrentRun(IDWriteColorGlyphRunEnumerator1 *iface,
        DWRITE_COLOR_GLYPH_RUN1 const **run)
{
    struct dwrite_colorglyphenum *glyphenum = impl_from_IDWriteColorGlyphRunEnumerator1(iface);

    TRACE("%p, %p.\n", iface, run);

    return colorglyphenum_get_current_run(glyphenum, run);
}

static const IDWriteColorGlyphRunEnumerator1Vtbl colorglyphenumvtbl =
{
    colorglyphenum_QueryInterface,
    colorglyphenum_AddRef,
    colorglyphenum_Release,
    colorglyphenum_MoveNext,
    colorglyphenum_GetCurrentRun,
    colorglyphenum1_GetCurrentRun,
};

HRESULT create_colorglyphenum(D2D1_POINT_2F origin, const DWRITE_GLYPH_RUN *run,
        const DWRITE_GLYPH_RUN_DESCRIPTION *rundescr, DWRITE_GLYPH_IMAGE_FORMATS formats, DWRITE_MEASURING_MODE measuring_mode,
        const DWRITE_MATRIX *transform, unsigned int palette, IDWriteColorGlyphRunEnumerator1 **ret)
{
    struct dwrite_colorglyphenum *colorglyphenum;
    BOOL colorfont, has_colored_glyph;
    struct dwrite_fontface *fontface;
    unsigned int i;

    *ret = NULL;

    fontface = unsafe_impl_from_IDWriteFontFace(run->fontFace);

    colorfont = IDWriteFontFace5_IsColorFont(&fontface->IDWriteFontFace5_iface) &&
            IDWriteFontFace5_GetColorPaletteCount(&fontface->IDWriteFontFace5_iface) > palette;
    if (!colorfont)
        return DWRITE_E_NOCOLOR;

    if (!(formats & (DWRITE_GLYPH_IMAGE_FORMATS_COLR |
                     DWRITE_GLYPH_IMAGE_FORMATS_SVG |
                     DWRITE_GLYPH_IMAGE_FORMATS_PNG |
                     DWRITE_GLYPH_IMAGE_FORMATS_JPEG |
                     DWRITE_GLYPH_IMAGE_FORMATS_TIFF |
                     DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8)))
    {
        return DWRITE_E_NOCOLOR;
    }

    if (formats & ~(DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE | DWRITE_GLYPH_IMAGE_FORMATS_CFF | DWRITE_GLYPH_IMAGE_FORMATS_COLR))
    {
        FIXME("Unimplemented formats requested %#x.\n", formats);
        return E_NOTIMPL;
    }

    if (!(colorglyphenum = calloc(1, sizeof(*colorglyphenum))))
        return E_OUTOFMEMORY;

    colorglyphenum->IDWriteColorGlyphRunEnumerator1_iface.lpVtbl = &colorglyphenumvtbl;
    colorglyphenum->refcount = 1;
    colorglyphenum->origin = origin;
    colorglyphenum->fontface = &fontface->IDWriteFontFace5_iface;
    IDWriteFontFace5_AddRef(colorglyphenum->fontface);
    colorglyphenum->glyphs = NULL;
    colorglyphenum->run = *run;
    colorglyphenum->run.glyphIndices = NULL;
    colorglyphenum->run.glyphAdvances = NULL;
    colorglyphenum->run.glyphOffsets = NULL;
    colorglyphenum->palette = palette;
    memset(&colorglyphenum->colr, 0, sizeof(colorglyphenum->colr));
    colorglyphenum->colr.exists = TRUE;
    get_fontface_table(&fontface->IDWriteFontFace5_iface, MS_COLR_TAG, &colorglyphenum->colr);
    colorglyphenum->current_layer = 0;
    colorglyphenum->max_layer_num = 0;

    colorglyphenum->glyphs = calloc(run->glyphCount, sizeof(*colorglyphenum->glyphs));

    has_colored_glyph = FALSE;
    colorglyphenum->has_regular_glyphs = FALSE;
    for (i = 0; i < run->glyphCount; i++) {
        if (opentype_get_colr_glyph(&colorglyphenum->colr, run->glyphIndices[i], colorglyphenum->glyphs + i) == S_OK) {
            colorglyphenum->max_layer_num = max(colorglyphenum->max_layer_num, colorglyphenum->glyphs[i].num_layers);
            has_colored_glyph = TRUE;
        }
        if (colorglyphenum->glyphs[i].num_layers == 0)
            colorglyphenum->has_regular_glyphs = TRUE;
    }

    /* It's acceptable to have a subset of glyphs mapped to color layers, for regular runs client
       is supposed to proceed normally, like if font had no color info at all. */
    if (!has_colored_glyph) {
        IDWriteColorGlyphRunEnumerator1_Release(&colorglyphenum->IDWriteColorGlyphRunEnumerator1_iface);
        return DWRITE_E_NOCOLOR;
    }

    colorglyphenum->advances = calloc(run->glyphCount, sizeof(*colorglyphenum->advances));
    colorglyphenum->color_advances = calloc(run->glyphCount, sizeof(*colorglyphenum->color_advances));
    colorglyphenum->glyphindices = calloc(run->glyphCount, sizeof(*colorglyphenum->glyphindices));
    if (run->glyphOffsets) {
        colorglyphenum->offsets = calloc(run->glyphCount, sizeof(*colorglyphenum->offsets));
        colorglyphenum->color_offsets = calloc(run->glyphCount, sizeof(*colorglyphenum->color_offsets));
        memcpy(colorglyphenum->offsets, run->glyphOffsets, run->glyphCount * sizeof(*run->glyphOffsets));
    }

    colorglyphenum->colorrun.glyphRun.fontFace = run->fontFace;
    colorglyphenum->colorrun.glyphRun.fontEmSize = run->fontEmSize;
    colorglyphenum->colorrun.glyphRun.glyphIndices = colorglyphenum->glyphindices;
    colorglyphenum->colorrun.glyphRun.glyphAdvances = colorglyphenum->color_advances;
    colorglyphenum->colorrun.glyphRun.glyphOffsets = colorglyphenum->color_offsets;
    colorglyphenum->colorrun.glyphRunDescription = NULL; /* FIXME */
    colorglyphenum->colorrun.measuringMode = measuring_mode;
    colorglyphenum->colorrun.glyphImageFormat = DWRITE_GLYPH_IMAGE_FORMATS_NONE; /* FIXME */

    if (run->glyphAdvances)
        memcpy(colorglyphenum->advances, run->glyphAdvances, run->glyphCount * sizeof(FLOAT));
    else
    {
        for (i = 0; i < run->glyphCount; ++i)
            colorglyphenum->advances[i] = fontface_get_scaled_design_advance(fontface, measuring_mode,
                    run->fontEmSize, 1.0f, transform, run->glyphIndices[i], run->isSideways);
    }

    *ret = &colorglyphenum->IDWriteColorGlyphRunEnumerator1_iface;

    return S_OK;
}

/* IDWriteFontFaceReference */
static HRESULT WINAPI fontfacereference_QueryInterface(IDWriteFontFaceReference1 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontFaceReference1) ||
            IsEqualIID(riid, &IID_IDWriteFontFaceReference) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFontFaceReference1_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI fontfacereference_AddRef(IDWriteFontFaceReference1 *iface)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);
    ULONG refcount = InterlockedIncrement(&reference->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI fontfacereference_Release(IDWriteFontFaceReference1 *iface)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);
    ULONG refcount = InterlockedDecrement(&reference->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFontFile_Release(reference->file);
        IDWriteFactory7_Release(reference->factory);
        free(reference->axis_values);
        free(reference);
    }

    return refcount;
}

static HRESULT WINAPI fontfacereference_CreateFontFace(IDWriteFontFaceReference1 *iface, IDWriteFontFace3 **fontface)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);

    TRACE("%p, %p.\n", iface, fontface);

    return IDWriteFontFaceReference1_CreateFontFaceWithSimulations(iface, reference->simulations, fontface);
}

static HRESULT WINAPI fontfacereference_CreateFontFaceWithSimulations(IDWriteFontFaceReference1 *iface,
    DWRITE_FONT_SIMULATIONS simulations, IDWriteFontFace3 **ret)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFace *fontface;
    BOOL is_supported;
    UINT32 face_num;
    HRESULT hr;

    TRACE("%p, %#x, %p.\n", iface, simulations, ret);

    hr = IDWriteFontFile_Analyze(reference->file, &is_supported, &file_type, &face_type, &face_num);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFactory7_CreateFontFace(reference->factory, face_type, 1, &reference->file, reference->index,
            simulations, &fontface);
    if (SUCCEEDED(hr))
    {
        hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace3, (void **)ret);
        IDWriteFontFace_Release(fontface);
    }

    return hr;
}

static BOOL WINAPI fontfacereference_Equals(IDWriteFontFaceReference1 *iface, IDWriteFontFaceReference *ref)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);
    struct dwrite_fontfacereference *other = unsafe_impl_from_IDWriteFontFaceReference(ref);
    BOOL ret;

    TRACE("%p, %p.\n", iface, ref);

    ret = is_same_fontfile(reference->file, other->file) && reference->index == other->index &&
            reference->simulations == other->simulations;
    if (reference->axis_values_count)
    {
        ret &= reference->axis_values_count == other->axis_values_count &&
                !memcmp(reference->axis_values, other->axis_values, reference->axis_values_count * sizeof(*reference->axis_values));
    }

    return ret;
}

static UINT32 WINAPI fontfacereference_GetFontFaceIndex(IDWriteFontFaceReference1 *iface)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);

    TRACE("%p.\n", iface);

    return reference->index;
}

static DWRITE_FONT_SIMULATIONS WINAPI fontfacereference_GetSimulations(IDWriteFontFaceReference1 *iface)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);

    TRACE("%p.\n", iface);

    return reference->simulations;
}

static HRESULT WINAPI fontfacereference_GetFontFile(IDWriteFontFaceReference1 *iface, IDWriteFontFile **file)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);
    IDWriteFontFileLoader *loader;
    const void *key;
    UINT32 key_size;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, file);

    hr = IDWriteFontFile_GetReferenceKey(reference->file, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFile_GetLoader(reference->file, &loader);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFactory7_CreateCustomFontFileReference(reference->factory, key, key_size, loader, file);
    IDWriteFontFileLoader_Release(loader);

    return hr;
}

static UINT64 WINAPI fontfacereference_GetLocalFileSize(IDWriteFontFaceReference1 *iface)
{
    FIXME("%p.\n", iface);

    return 0;
}

static UINT64 WINAPI fontfacereference_GetFileSize(IDWriteFontFaceReference1 *iface)
{
    FIXME("%p.\n", iface);

    return 0;
}

static HRESULT WINAPI fontfacereference_GetFileTime(IDWriteFontFaceReference1 *iface, FILETIME *writetime)
{
    FIXME("%p, %p.\n", iface, writetime);

    return E_NOTIMPL;
}

static DWRITE_LOCALITY WINAPI fontfacereference_GetLocality(IDWriteFontFaceReference1 *iface)
{
    FIXME("%p.\n", iface);

    return DWRITE_LOCALITY_LOCAL;
}

static HRESULT WINAPI fontfacereference_EnqueueFontDownloadRequest(IDWriteFontFaceReference1 *iface)
{
    FIXME("%p.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI fontfacereference_EnqueueCharacterDownloadRequest(IDWriteFontFaceReference1 *iface,
        WCHAR const *chars, UINT32 count)
{
    FIXME("%p, %s, %u.\n", iface, debugstr_wn(chars, count), count);

    return E_NOTIMPL;
}

static HRESULT WINAPI fontfacereference_EnqueueGlyphDownloadRequest(IDWriteFontFaceReference1 *iface,
        UINT16 const *glyphs, UINT32 count)
{
    FIXME("%p, %p, %u.\n", iface, glyphs, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI fontfacereference_EnqueueFileFragmentDownloadRequest(IDWriteFontFaceReference1 *iface,
        UINT64 offset, UINT64 size)
{
    FIXME("%p, 0x%s, 0x%s.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(size));

    return E_NOTIMPL;
}

static HRESULT WINAPI fontfacereference1_CreateFontFace(IDWriteFontFaceReference1 *iface, IDWriteFontFace5 **fontface)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);
    IDWriteFontFace3 *fontface3;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, fontface);

    /* FIXME: created instance should likely respect given axis. */
    if (SUCCEEDED(hr = IDWriteFontFaceReference1_CreateFontFaceWithSimulations(iface, reference->simulations,
            &fontface3)))
    {
        hr = IDWriteFontFace3_QueryInterface(fontface3, &IID_IDWriteFontFace5, (void **)fontface);
        IDWriteFontFace3_Release(fontface3);
    }

    return hr;
}

static UINT32 WINAPI fontfacereference1_GetFontAxisValueCount(IDWriteFontFaceReference1 *iface)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);

    TRACE("%p.\n", iface);

    return reference->axis_values_count;
}

static HRESULT WINAPI fontfacereference1_GetFontAxisValues(IDWriteFontFaceReference1 *iface,
        DWRITE_FONT_AXIS_VALUE *axis_values, UINT32 value_count)
{
    struct dwrite_fontfacereference *reference = impl_from_IDWriteFontFaceReference1(iface);

    TRACE("%p, %p, %u.\n", iface, axis_values, value_count);

    if (value_count < reference->axis_values_count)
        return E_NOT_SUFFICIENT_BUFFER;

    memcpy(axis_values, reference->axis_values, reference->axis_values_count * sizeof(*axis_values));

    return S_OK;
}

static const IDWriteFontFaceReference1Vtbl fontfacereferencevtbl =
{
    fontfacereference_QueryInterface,
    fontfacereference_AddRef,
    fontfacereference_Release,
    fontfacereference_CreateFontFace,
    fontfacereference_CreateFontFaceWithSimulations,
    fontfacereference_Equals,
    fontfacereference_GetFontFaceIndex,
    fontfacereference_GetSimulations,
    fontfacereference_GetFontFile,
    fontfacereference_GetLocalFileSize,
    fontfacereference_GetFileSize,
    fontfacereference_GetFileTime,
    fontfacereference_GetLocality,
    fontfacereference_EnqueueFontDownloadRequest,
    fontfacereference_EnqueueCharacterDownloadRequest,
    fontfacereference_EnqueueGlyphDownloadRequest,
    fontfacereference_EnqueueFileFragmentDownloadRequest,
    fontfacereference1_CreateFontFace,
    fontfacereference1_GetFontAxisValueCount,
    fontfacereference1_GetFontAxisValues,
};

HRESULT create_fontfacereference(IDWriteFactory7 *factory, IDWriteFontFile *file, UINT32 index,
        DWRITE_FONT_SIMULATIONS simulations, DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 axis_values_count,
        IDWriteFontFaceReference1 **ret)
{
    struct dwrite_fontfacereference *object;

    *ret = NULL;

    if (!is_simulation_valid(simulations))
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteFontFaceReference1_iface.lpVtbl = &fontfacereferencevtbl;
    object->refcount = 1;

    object->factory = factory;
    IDWriteFactory7_AddRef(object->factory);
    object->file = file;
    IDWriteFontFile_AddRef(object->file);
    object->index = index;
    object->simulations = simulations;
    if (axis_values_count)
    {
        if (!(object->axis_values = malloc(axis_values_count * sizeof(*axis_values))))
        {
            IDWriteFontFaceReference1_Release(&object->IDWriteFontFaceReference1_iface);
            return E_OUTOFMEMORY;
        }
        memcpy(object->axis_values, axis_values, axis_values_count * sizeof(*axis_values));
        object->axis_values_count = axis_values_count;
    }

    *ret = &object->IDWriteFontFaceReference1_iface;

    return S_OK;
}

static HRESULT WINAPI inmemoryfilestream_QueryInterface(IDWriteFontFileStream *iface, REFIID riid, void **obj)
{
    TRACE_(dwrite_file)("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontFileStream) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteFontFileStream_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    WARN("%s not implemented.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI inmemoryfilestream_AddRef(IDWriteFontFileStream *iface)
{
    struct dwrite_inmemory_filestream *stream = inmemory_impl_from_IDWriteFontFileStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE_(dwrite_file)("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI inmemoryfilestream_Release(IDWriteFontFileStream *iface)
{
    struct dwrite_inmemory_filestream *stream = inmemory_impl_from_IDWriteFontFileStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE_(dwrite_file)("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        release_inmemory_stream(stream->data);
        free(stream);
    }

    return refcount;
}

static HRESULT WINAPI inmemoryfilestream_ReadFileFragment(IDWriteFontFileStream *iface, void const **fragment_start,
        UINT64 offset, UINT64 fragment_size, void **fragment_context)
{
    struct dwrite_inmemory_filestream *stream = inmemory_impl_from_IDWriteFontFileStream(iface);

    TRACE_(dwrite_file)("%p, %p, 0x%s, 0x%s, %p.\n", iface, fragment_start,
          wine_dbgstr_longlong(offset), wine_dbgstr_longlong(fragment_size), fragment_context);

    *fragment_context = NULL;

    if ((offset >= stream->data->size - 1) || (fragment_size > stream->data->size - offset)) {
        *fragment_start = NULL;
        return E_FAIL;
    }

    *fragment_start = (char *)stream->data->data + offset;
    return S_OK;
}

static void WINAPI inmemoryfilestream_ReleaseFileFragment(IDWriteFontFileStream *iface, void *fragment_context)
{
    TRACE_(dwrite_file)("%p, %p.\n", iface, fragment_context);
}

static HRESULT WINAPI inmemoryfilestream_GetFileSize(IDWriteFontFileStream *iface, UINT64 *size)
{
    struct dwrite_inmemory_filestream *stream = inmemory_impl_from_IDWriteFontFileStream(iface);

    TRACE_(dwrite_file)("%p, %p.\n", iface, size);

    *size = stream->data->size;

    return S_OK;
}

static HRESULT WINAPI inmemoryfilestream_GetLastWriteTime(IDWriteFontFileStream *iface, UINT64 *last_writetime)
{
    TRACE_(dwrite_file)("%p, %p.\n", iface, last_writetime);

    *last_writetime = 0;

    return E_NOTIMPL;
}

static const IDWriteFontFileStreamVtbl inmemoryfilestreamvtbl = {
    inmemoryfilestream_QueryInterface,
    inmemoryfilestream_AddRef,
    inmemoryfilestream_Release,
    inmemoryfilestream_ReadFileFragment,
    inmemoryfilestream_ReleaseFileFragment,
    inmemoryfilestream_GetFileSize,
    inmemoryfilestream_GetLastWriteTime,
};

static HRESULT WINAPI inmemoryfontfileloader_QueryInterface(IDWriteInMemoryFontFileLoader *iface,
        REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteInMemoryFontFileLoader) ||
        IsEqualIID(riid, &IID_IDWriteFontFileLoader) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteInMemoryFontFileLoader_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI inmemoryfontfileloader_AddRef(IDWriteInMemoryFontFileLoader *iface)
{
    struct dwrite_inmemory_fileloader *loader = impl_from_IDWriteInMemoryFontFileLoader(iface);
    ULONG refcount = InterlockedIncrement(&loader->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI inmemoryfontfileloader_Release(IDWriteInMemoryFontFileLoader *iface)
{
    struct dwrite_inmemory_fileloader *loader = impl_from_IDWriteInMemoryFontFileLoader(iface);
    ULONG refcount = InterlockedDecrement(&loader->refcount);
    size_t i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < loader->count; ++i)
            release_inmemory_stream(loader->streams[i]);
        free(loader->streams);
        free(loader);
    }

    return refcount;
}

static HRESULT WINAPI inmemoryfontfileloader_CreateStreamFromKey(IDWriteInMemoryFontFileLoader *iface,
        void const *key, UINT32 key_size, IDWriteFontFileStream **ret)
{
    struct dwrite_inmemory_fileloader *loader = impl_from_IDWriteInMemoryFontFileLoader(iface);
    struct dwrite_inmemory_filestream *stream;
    DWORD index;

    TRACE("%p, %p, %u, %p.\n", iface, key, key_size, ret);

    *ret = NULL;

    if (key_size != sizeof(DWORD))
        return E_INVALIDARG;

    index = *(DWORD *)key;

    if (index >= loader->count)
        return E_INVALIDARG;

    if (!(stream = malloc(sizeof(*stream))))
        return E_OUTOFMEMORY;

    stream->IDWriteFontFileStream_iface.lpVtbl = &inmemoryfilestreamvtbl;
    stream->refcount = 1;
    stream->data = loader->streams[index];
    InterlockedIncrement(&stream->data->refcount);

    *ret = &stream->IDWriteFontFileStream_iface;

    return S_OK;
}

static HRESULT WINAPI inmemoryfontfileloader_CreateInMemoryFontFileReference(IDWriteInMemoryFontFileLoader *iface,
        IDWriteFactory *factory, void const *data, UINT32 data_size, IUnknown *owner, IDWriteFontFile **fontfile)
{
    struct dwrite_inmemory_fileloader *loader = impl_from_IDWriteInMemoryFontFileLoader(iface);
    struct dwrite_inmemory_stream_data *stream;
    DWORD key;

    TRACE("%p, %p, %p, %u, %p, %p.\n", iface, factory, data, data_size, owner, fontfile);

    *fontfile = NULL;

    if (!dwrite_array_reserve((void **)&loader->streams, &loader->size, loader->count + 1, sizeof(*loader->streams)))
        return E_OUTOFMEMORY;

    if (!(stream = malloc(sizeof(*stream))))
        return E_OUTOFMEMORY;

    stream->refcount = 1;
    stream->size = data_size;
    stream->owner = owner;
    if (stream->owner) {
        IUnknown_AddRef(stream->owner);
        stream->data = (void *)data;
    }
    else {
        if (!(stream->data = malloc(data_size)))
        {
            free(stream);
            return E_OUTOFMEMORY;
        }
        memcpy(stream->data, data, data_size);
    }

    key = loader->count;
    loader->streams[loader->count++] = stream;

    return IDWriteFactory_CreateCustomFontFileReference(factory, &key, sizeof(key),
            (IDWriteFontFileLoader *)&loader->IDWriteInMemoryFontFileLoader_iface, fontfile);
}

static UINT32 WINAPI inmemoryfontfileloader_GetFileCount(IDWriteInMemoryFontFileLoader *iface)
{
    struct dwrite_inmemory_fileloader *loader = impl_from_IDWriteInMemoryFontFileLoader(iface);

    TRACE("%p.\n", iface);

    return loader->count;
}

static const IDWriteInMemoryFontFileLoaderVtbl inmemoryfontfileloadervtbl =
{
    inmemoryfontfileloader_QueryInterface,
    inmemoryfontfileloader_AddRef,
    inmemoryfontfileloader_Release,
    inmemoryfontfileloader_CreateStreamFromKey,
    inmemoryfontfileloader_CreateInMemoryFontFileReference,
    inmemoryfontfileloader_GetFileCount,
};

HRESULT create_inmemory_fileloader(IDWriteInMemoryFontFileLoader **ret)
{
    struct dwrite_inmemory_fileloader *loader;

    *ret = NULL;

    if (!(loader = calloc(1, sizeof(*loader))))
        return E_OUTOFMEMORY;

    loader->IDWriteInMemoryFontFileLoader_iface.lpVtbl = &inmemoryfontfileloadervtbl;
    loader->refcount = 1;

    *ret = &loader->IDWriteInMemoryFontFileLoader_iface;

    return S_OK;
}

static HRESULT WINAPI dwritefontresource_QueryInterface(IDWriteFontResource *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontResource) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFontResource_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontresource_AddRef(IDWriteFontResource *iface)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);
    ULONG refcount = InterlockedIncrement(&resource->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritefontresource_Release(IDWriteFontResource *iface)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);
    ULONG refcount = InterlockedDecrement(&resource->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFactory7_Release(resource->factory);
        IDWriteFontFile_Release(resource->file);
        free(resource->axis);
        free(resource);
    }

    return refcount;
}

static HRESULT WINAPI dwritefontresource_GetFontFile(IDWriteFontResource *iface, IDWriteFontFile **fontfile)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);

    TRACE("%p, %p.\n", iface, fontfile);

    *fontfile = resource->file;
    IDWriteFontFile_AddRef(*fontfile);

    return S_OK;
}

static UINT32 WINAPI dwritefontresource_GetFontFaceIndex(IDWriteFontResource *iface)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);

    TRACE("%p.\n", iface);

    return resource->face_index;
}

static UINT32 WINAPI dwritefontresource_GetFontAxisCount(IDWriteFontResource *iface)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);

    TRACE("%p.\n", iface);

    return resource->axis_count;
}

static HRESULT WINAPI dwritefontresource_GetDefaultFontAxisValues(IDWriteFontResource *iface,
        DWRITE_FONT_AXIS_VALUE *values, UINT32 count)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);
    unsigned int i;

    TRACE("%p, %p, %u.\n", iface, values, count);

    if (count < resource->axis_count)
        return E_NOT_SUFFICIENT_BUFFER;

    for (i = 0; i < resource->axis_count; ++i)
    {
        values[i].axisTag = resource->axis[i].tag;
        values[i].value = resource->axis[i].default_value;
    }

    return S_OK;
}

static HRESULT WINAPI dwritefontresource_GetFontAxisRanges(IDWriteFontResource *iface,
        DWRITE_FONT_AXIS_RANGE *ranges, UINT32 count)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);
    unsigned int i;

    TRACE("%p, %p, %u.\n", iface, ranges, count);

    if (count < resource->axis_count)
        return E_NOT_SUFFICIENT_BUFFER;

    for (i = 0; i < resource->axis_count; ++i)
    {
        ranges[i].axisTag = resource->axis[i].tag;
        ranges[i].minValue = resource->axis[i].min_value;
        ranges[i].maxValue = resource->axis[i].max_value;
    }

    return S_OK;
}

static DWRITE_FONT_AXIS_ATTRIBUTES WINAPI dwritefontresource_GetFontAxisAttributes(IDWriteFontResource *iface,
        UINT32 axis)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);

    TRACE("%p, %u.\n", iface, axis);

    return axis < resource->axis_count ? resource->axis[axis].attributes : 0;
}

static HRESULT WINAPI dwritefontresource_GetAxisNames(IDWriteFontResource *iface, UINT32 axis,
        IDWriteLocalizedStrings **names)
{
    FIXME("%p, %u, %p.\n", iface, axis, names);

    return E_NOTIMPL;
}

static UINT32 WINAPI dwritefontresource_GetAxisValueNameCount(IDWriteFontResource *iface, UINT32 axis)
{
    FIXME("%p, %u.\n", iface, axis);

    return 0;
}

static HRESULT WINAPI dwritefontresource_GetAxisValueNames(IDWriteFontResource *iface, UINT32 axis,
        UINT32 axis_value, DWRITE_FONT_AXIS_RANGE *axis_range, IDWriteLocalizedStrings **names)
{
    FIXME("%p, %u, %u, %p, %p.\n", iface, axis, axis_value, axis_range, names);

    return E_NOTIMPL;
}

static BOOL WINAPI dwritefontresource_HasVariations(IDWriteFontResource *iface)
{
    FIXME("%p.\n", iface);

    return FALSE;
}

static HRESULT WINAPI dwritefontresource_CreateFontFace(IDWriteFontResource *iface,
        DWRITE_FONT_SIMULATIONS simulations, DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_values,
        IDWriteFontFace5 **fontface)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);
    IDWriteFontFaceReference1 *reference;
    HRESULT hr;

    TRACE("%p, %#x, %p, %u, %p.\n", iface, simulations, axis_values, num_values, fontface);

    hr = IDWriteFactory7_CreateFontFaceReference(resource->factory, resource->file, resource->face_index,
            simulations, axis_values, num_values, &reference);
    if (SUCCEEDED(hr))
    {
        hr = IDWriteFontFaceReference1_CreateFontFace(reference, fontface);
        IDWriteFontFaceReference1_Release(reference);
    }

    return hr;
}

static HRESULT WINAPI dwritefontresource_CreateFontFaceReference(IDWriteFontResource *iface,
        DWRITE_FONT_SIMULATIONS simulations, DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_values,
        IDWriteFontFaceReference1 **reference)
{
    struct dwrite_fontresource *resource = impl_from_IDWriteFontResource(iface);

    TRACE("%p, %#x, %p, %u, %p.\n", iface, simulations, axis_values, num_values, reference);

    return IDWriteFactory7_CreateFontFaceReference(resource->factory, resource->file, resource->face_index,
            simulations, axis_values, num_values, reference);
}

static const IDWriteFontResourceVtbl fontresourcevtbl =
{
    dwritefontresource_QueryInterface,
    dwritefontresource_AddRef,
    dwritefontresource_Release,
    dwritefontresource_GetFontFile,
    dwritefontresource_GetFontFaceIndex,
    dwritefontresource_GetFontAxisCount,
    dwritefontresource_GetDefaultFontAxisValues,
    dwritefontresource_GetFontAxisRanges,
    dwritefontresource_GetFontAxisAttributes,
    dwritefontresource_GetAxisNames,
    dwritefontresource_GetAxisValueNameCount,
    dwritefontresource_GetAxisValueNames,
    dwritefontresource_HasVariations,
    dwritefontresource_CreateFontFace,
    dwritefontresource_CreateFontFaceReference,
};

HRESULT create_font_resource(IDWriteFactory7 *factory, IDWriteFontFile *file, UINT32 face_index,
        IDWriteFontResource **ret)
{
    struct dwrite_fontresource *resource;
    struct file_stream_desc stream_desc;
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    unsigned int face_count;
    BOOL supported = FALSE;
    HRESULT hr;

    *ret = NULL;

    if (FAILED(hr = IDWriteFontFile_Analyze(file, &supported, &file_type, &face_type, &face_count)))
        return hr;

    if (!supported)
        return DWRITE_E_FILEFORMAT;

    if (!(resource = calloc(1, sizeof(*resource))))
        return E_OUTOFMEMORY;

    resource->IDWriteFontResource_iface.lpVtbl = &fontresourcevtbl;
    resource->refcount = 1;
    resource->face_index = face_index;
    resource->file = file;
    IDWriteFontFile_AddRef(resource->file);
    resource->factory = factory;
    IDWriteFactory7_AddRef(resource->factory);

    get_filestream_from_file(file, &stream_desc.stream);
    stream_desc.face_type = face_type;
    stream_desc.face_index = face_index;

    opentype_get_font_var_axis(&stream_desc, &resource->axis, &resource->axis_count);

    if (stream_desc.stream)
        IDWriteFontFileStream_Release(stream_desc.stream);

    *ret = &resource->IDWriteFontResource_iface;

    return S_OK;
}

static HRESULT WINAPI dwritefontset_QueryInterface(IDWriteFontSet3 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontSet3) ||
            IsEqualIID(riid, &IID_IDWriteFontSet2) ||
            IsEqualIID(riid, &IID_IDWriteFontSet1) ||
            IsEqualIID(riid, &IID_IDWriteFontSet))
    {
        *obj = iface;
        IDWriteFontSet3_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontset_AddRef(IDWriteFontSet3 *iface)
{
    struct dwrite_fontset *set = impl_from_IDWriteFontSet3(iface);
    ULONG refcount = InterlockedIncrement(&set->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

#define MISSING_SET_PROP ((void *)0x1)

static void release_fontset_entry(struct dwrite_fontset_entry *entry)
{
    unsigned int i;

    if (InterlockedDecrement(&entry->refcount) > 0)
        return;
    IDWriteFontFile_Release(entry->desc.file);
    for (i = 0; i < ARRAY_SIZE(entry->props); ++i)
    {
        if (entry->props[i] && entry->props[i] != MISSING_SET_PROP)
            IDWriteLocalizedStrings_Release(entry->props[i]);
    }
    free(entry);
}

static struct dwrite_fontset_entry * addref_fontset_entry(struct dwrite_fontset_entry *entry)
{
    InterlockedIncrement(&entry->refcount);
    return entry;
}

static IDWriteLocalizedStrings * fontset_entry_get_property(struct dwrite_fontset_entry *entry,
        DWRITE_FONT_PROPERTY_ID property)
{
    struct file_stream_desc stream_desc = { 0 };
    IDWriteLocalizedStrings *value;

    assert(property > DWRITE_FONT_PROPERTY_ID_NONE && property <= DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FACE_NAME);

    if (entry->props[property] == MISSING_SET_PROP)
        return NULL;

    if ((value = entry->props[property]))
    {
        IDWriteLocalizedStrings_AddRef(value);
        return value;
    }

    get_filestream_from_file(entry->desc.file, &stream_desc.stream);
    stream_desc.face_type = entry->desc.face_type;
    stream_desc.face_index = entry->desc.face_index;

    if (property == DWRITE_FONT_PROPERTY_ID_FULL_NAME)
        opentype_get_font_info_strings(&stream_desc, DWRITE_INFORMATIONAL_STRING_FULL_NAME, &value);
    else if (property == DWRITE_FONT_PROPERTY_ID_POSTSCRIPT_NAME)
        opentype_get_font_info_strings(&stream_desc, DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME, &value);
    else if (property == DWRITE_FONT_PROPERTY_ID_DESIGN_SCRIPT_LANGUAGE_TAG)
        opentype_get_font_info_strings(&stream_desc, DWRITE_INFORMATIONAL_STRING_DESIGN_SCRIPT_LANGUAGE_TAG, &value);
    else if (property == DWRITE_FONT_PROPERTY_ID_SUPPORTED_SCRIPT_LANGUAGE_TAG)
        opentype_get_font_info_strings(&stream_desc, DWRITE_INFORMATIONAL_STRING_SUPPORTED_SCRIPT_LANGUAGE_TAG, &value);
    else if (property == DWRITE_FONT_PROPERTY_ID_WIN32_FAMILY_NAME)
        opentype_get_font_info_strings(&stream_desc, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &value);
    else
        WARN("Unsupported property %u.\n", property);

    if (stream_desc.stream)
        IDWriteFontFileStream_Release(stream_desc.stream);

    if (value)
    {
        entry->props[property] = value;
        IDWriteLocalizedStrings_AddRef(value);
    }
    else
        entry->props[property] = MISSING_SET_PROP;

    return value;
}

static void init_fontset(struct dwrite_fontset *object, IDWriteFactory7 *factory,
       struct dwrite_fontset_entry **entries, unsigned int count);

static ULONG WINAPI dwritefontset_Release(IDWriteFontSet3 *iface)
{
    struct dwrite_fontset *set = impl_from_IDWriteFontSet3(iface);
    ULONG refcount = InterlockedDecrement(&set->refcount);
    unsigned int i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFactory7_Release(set->factory);
        for (i = 0; i < set->count; ++i)
            release_fontset_entry(set->entries[i]);
        free(set->entries);
        free(set);
    }

    return refcount;
}

static UINT32 WINAPI dwritefontset_GetFontCount(IDWriteFontSet3 *iface)
{
    struct dwrite_fontset *set = impl_from_IDWriteFontSet3(iface);

    TRACE("%p.\n", iface);

    return set->count;
}

static HRESULT WINAPI dwritefontset_GetFontFaceReference(IDWriteFontSet3 *iface, UINT32 index,
        IDWriteFontFaceReference **reference)
{
    struct dwrite_fontset *set = impl_from_IDWriteFontSet3(iface);

    TRACE("%p, %u, %p.\n", iface, index, reference);

    *reference = NULL;

    if (index >= set->count)
        return E_INVALIDARG;

    return IDWriteFactory7_CreateFontFaceReference_(set->factory, set->entries[index]->desc.file,
            set->entries[index]->desc.face_index, set->entries[index]->desc.simulations, reference);
}

static HRESULT WINAPI dwritefontset_FindFontFaceReference(IDWriteFontSet3 *iface,
        IDWriteFontFaceReference *reference, UINT32 *index, BOOL *exists)
{
    FIXME("%p, %p, %p, %p.\n", iface, reference, index, exists);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset_FindFontFace(IDWriteFontSet3 *iface, IDWriteFontFace *fontface,
        UINT32 *index, BOOL *exists)
{
    FIXME("%p, %p, %p, %p.\n", iface, fontface, index, exists);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset_GetPropertyValues__(IDWriteFontSet3 *iface, DWRITE_FONT_PROPERTY_ID id,
        IDWriteStringList **values)
{
    FIXME("%p, %d, %p.\n", iface, id, values);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset_GetPropertyValues_(IDWriteFontSet3 *iface, DWRITE_FONT_PROPERTY_ID id,
        WCHAR const *preferred_locales, IDWriteStringList **values)
{
    FIXME("%p, %d, %s, %p.\n", iface, id, debugstr_w(preferred_locales), values);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset_GetPropertyValues(IDWriteFontSet3 *iface, UINT32 index, DWRITE_FONT_PROPERTY_ID id,
        BOOL *exists, IDWriteLocalizedStrings **values)
{
    struct dwrite_fontset *set = impl_from_IDWriteFontSet3(iface);

    TRACE("%p, %u, %d, %p, %p.\n", iface, index, id, exists, values);

    if (!(id > DWRITE_FONT_PROPERTY_ID_NONE && id <= DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FACE_NAME) ||
            index >= set->count)
    {
        *values = NULL;
        *exists = FALSE;
        return E_INVALIDARG;
    }

    *values = fontset_entry_get_property(set->entries[index], id);
    *exists = !!*values;

    return S_OK;
}

static HRESULT WINAPI dwritefontset_GetPropertyOccurrenceCount(IDWriteFontSet3 *iface, DWRITE_FONT_PROPERTY const *property,
        UINT32 *count)
{
    FIXME("%p, %p, %p.\n", iface, property, count);

    return E_NOTIMPL;
}

static BOOL fontset_entry_is_matching(struct dwrite_fontset_entry *entry, DWRITE_FONT_PROPERTY const *props,
        unsigned int count)
{
    IDWriteLocalizedStrings *value;
    unsigned int i;
    BOOL ret;

    for (i = 0; i < count; ++i)
    {
        switch (props[i].propertyId)
        {
            case DWRITE_FONT_PROPERTY_ID_POSTSCRIPT_NAME:
            case DWRITE_FONT_PROPERTY_ID_FULL_NAME:
            case DWRITE_FONT_PROPERTY_ID_DESIGN_SCRIPT_LANGUAGE_TAG:
            case DWRITE_FONT_PROPERTY_ID_SUPPORTED_SCRIPT_LANGUAGE_TAG:
            case DWRITE_FONT_PROPERTY_ID_WIN32_FAMILY_NAME:
                if (!(value = fontset_entry_get_property(entry, props[i].propertyId)))
                    return FALSE;

                ret = localizedstrings_contains(value, props[i].propertyValue);
                IDWriteLocalizedStrings_Release(value);
                if (!ret) return FALSE;
                break;
            case DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FAMILY_NAME:
            case DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FAMILY_NAME:
            case DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FACE_NAME:
            case DWRITE_FONT_PROPERTY_ID_SEMANTIC_TAG:
            case DWRITE_FONT_PROPERTY_ID_WEIGHT:
            case DWRITE_FONT_PROPERTY_ID_STRETCH:
            case DWRITE_FONT_PROPERTY_ID_STYLE:
            case DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FACE_NAME:
                FIXME("Unsupported property %d.\n", props[i].propertyId);
                /* fallthrough */
            default:
                return FALSE;
        }
    }

    return TRUE;
}

static HRESULT WINAPI dwritefontset_GetMatchingFonts_(IDWriteFontSet3 *iface, WCHAR const *family, DWRITE_FONT_WEIGHT weight,
        DWRITE_FONT_STRETCH stretch,  DWRITE_FONT_STYLE style, IDWriteFontSet **fontset)
{
    FIXME("%p, %s, %d, %d, %d, %p.\n", iface, debugstr_w(family), weight, stretch, style, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset_GetMatchingFonts(IDWriteFontSet3 *iface, DWRITE_FONT_PROPERTY const *props, UINT32 count,
        IDWriteFontSet **filtered_set)
{
    struct dwrite_fontset *set = impl_from_IDWriteFontSet3(iface);
    struct dwrite_fontset_entry **entries;
    unsigned int i, matched_count = 0;
    struct dwrite_fontset *object;

    TRACE("%p, %p, %u, %p.\n", iface, props, count, filtered_set);

    if (!props && count)
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (!(entries = calloc(set->count, sizeof(*entries))))
    {
        free(object);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < set->count; ++i)
    {
        if (fontset_entry_is_matching(set->entries[i], props, count))
        {
            entries[matched_count++] = addref_fontset_entry(set->entries[i]);
        }
    }

    if (!matched_count)
    {
        free(entries);
        entries = NULL;
    }

    init_fontset(object, set->factory, entries, matched_count);

    *filtered_set = (IDWriteFontSet *)&object->IDWriteFontSet3_iface;

    return S_OK;
}

static HRESULT WINAPI dwritefontset1_GetMatchingFonts(IDWriteFontSet3 *iface, DWRITE_FONT_PROPERTY const *property,
        DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_values, IDWriteFontSet1 **fontset)
{
    FIXME("%p, %p, %p, %u, %p.\n", iface, property, axis_values, num_values, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFirstFontResources(IDWriteFontSet3 *iface, IDWriteFontSet1 **fontset)
{
    FIXME("%p, %p.\n", iface, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFilteredFonts__(IDWriteFontSet3 *iface, UINT32 const *indices,
        UINT32 num_indices, IDWriteFontSet1 **fontset)
{
    FIXME("%p, %p, %u, %p.\n", iface, indices, num_indices, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFilteredFonts_(IDWriteFontSet3 *iface, DWRITE_FONT_AXIS_RANGE const *axis_ranges,
        UINT32 num_ranges, BOOL select_any_range, IDWriteFontSet1 **fontset)
{
    FIXME("%p, %p, %u, %d, %p.\n", iface, axis_ranges, num_ranges, select_any_range, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFilteredFonts(IDWriteFontSet3 *iface, DWRITE_FONT_PROPERTY const *props,
        UINT32 num_properties, BOOL select_any_property, IDWriteFontSet1 **fontset)
{
    FIXME("%p, %p, %u, %d, %p.\n", iface, props, num_properties, select_any_property, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFilteredFontIndices_(IDWriteFontSet3 *iface, DWRITE_FONT_AXIS_RANGE const *ranges,
        UINT32 num_ranges, BOOL select_any_range, UINT32 *indices, UINT32 num_indices, UINT32 *actual_num_indices)
{
    FIXME("%p, %p, %u, %d, %p, %u, %p.\n", iface, ranges, num_ranges, select_any_range, indices, num_indices, actual_num_indices);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFilteredFontIndices(IDWriteFontSet3 *iface, DWRITE_FONT_PROPERTY const *props,
        UINT32 num_properties, BOOL select_any_range, UINT32 *indices, UINT32 num_indices, UINT32 *actual_num_indices)
{
    FIXME("%p, %p, %u, %d, %p, %u, %p.\n", iface, props, num_properties, select_any_range, indices,
            num_indices, actual_num_indices);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFontAxisRanges_(IDWriteFontSet3 *iface, UINT32 font_index,
        DWRITE_FONT_AXIS_RANGE *axis_ranges, UINT32 num_ranges, UINT32 *actual_num_ranges)
{
    FIXME("%p, %u, %p, %u, %p.\n", iface, font_index, axis_ranges, num_ranges, actual_num_ranges);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFontAxisRanges(IDWriteFontSet3 *iface, DWRITE_FONT_AXIS_RANGE *axis_ranges,
        UINT32 num_ranges, UINT32 *actual_num_ranges)
{
    FIXME("%p, %p, %u, %p.\n", iface, axis_ranges, num_ranges, actual_num_ranges);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_GetFontFaceReference(IDWriteFontSet3 *iface, UINT32 index,
        IDWriteFontFaceReference1 **reference)
{
    FIXME("%p, %u, %p.\n", iface, index, reference);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontset1_CreateFontResource(IDWriteFontSet3 *iface, UINT32 index, IDWriteFontResource **resource)
{
    struct dwrite_fontset *set = impl_from_IDWriteFontSet3(iface);

    TRACE("%p, %u, %p.\n", iface, index, resource);

    *resource = NULL;

    if (index >= set->count)
        return E_INVALIDARG;

    return IDWriteFactory7_CreateFontResource(set->factory, set->entries[index]->desc.file,
            set->entries[index]->desc.face_index, resource);
}

static HRESULT WINAPI dwritefontset1_CreateFontFace(IDWriteFontSet3 *iface, UINT32 index, IDWriteFontFace5 **fontface)
{
    FIXME("%p, %u, %p.\n", iface, index, fontface);

    return E_NOTIMPL;
}

static DWRITE_LOCALITY WINAPI dwritefontset1_GetFontLocality(IDWriteFontSet3 *iface, UINT32 index)
{
    FIXME("%p, %u.\n", iface, index);

    return DWRITE_LOCALITY_LOCAL;
}

static HANDLE WINAPI dwritefontset2_GetExpirationEvent(IDWriteFontSet3 *iface)
{
    FIXME("%p.\n", iface);

    return NULL;
}

static DWRITE_FONT_SOURCE_TYPE WINAPI dwritefontset3_GetFontSourceType(IDWriteFontSet3 *iface, UINT32 index)
{
    FIXME("%p, %u.\n", iface, index);

    return DWRITE_FONT_SOURCE_TYPE_UNKNOWN;
}

static UINT32 WINAPI dwritefontset3_GetFontSourceNameLength(IDWriteFontSet3 *iface, UINT32 index)
{
    FIXME("%p, %u.\n", iface, index);

    return 0;
}

static HRESULT WINAPI dwritefontset3_GetFontSourceName(IDWriteFontSet3 *iface, UINT32 index, WCHAR *buffer, UINT32 buffer_size)
{
    FIXME("%p, %u, %p, %u.\n", iface, index, buffer, buffer_size);

    return E_NOTIMPL;
}

static const IDWriteFontSet3Vtbl fontsetvtbl =
{
    dwritefontset_QueryInterface,
    dwritefontset_AddRef,
    dwritefontset_Release,
    dwritefontset_GetFontCount,
    dwritefontset_GetFontFaceReference,
    dwritefontset_FindFontFaceReference,
    dwritefontset_FindFontFace,
    dwritefontset_GetPropertyValues__,
    dwritefontset_GetPropertyValues_,
    dwritefontset_GetPropertyValues,
    dwritefontset_GetPropertyOccurrenceCount,
    dwritefontset_GetMatchingFonts_,
    dwritefontset_GetMatchingFonts,
    dwritefontset1_GetMatchingFonts,
    dwritefontset1_GetFirstFontResources,
    dwritefontset1_GetFilteredFonts__,
    dwritefontset1_GetFilteredFonts_,
    dwritefontset1_GetFilteredFonts,
    dwritefontset1_GetFilteredFontIndices_,
    dwritefontset1_GetFilteredFontIndices,
    dwritefontset1_GetFontAxisRanges_,
    dwritefontset1_GetFontAxisRanges,
    dwritefontset1_GetFontFaceReference,
    dwritefontset1_CreateFontResource,
    dwritefontset1_CreateFontFace,
    dwritefontset1_GetFontLocality,
    dwritefontset2_GetExpirationEvent,
    dwritefontset3_GetFontSourceType,
    dwritefontset3_GetFontSourceNameLength,
    dwritefontset3_GetFontSourceName,
};

static struct dwrite_fontset *unsafe_impl_from_IDWriteFontSet(IDWriteFontSet *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (IDWriteFontSetVtbl *)&fontsetvtbl);
    return CONTAINING_RECORD(iface, struct dwrite_fontset, IDWriteFontSet3_iface);
}

static HRESULT fontset_create_entry(IDWriteFontFile *file, DWRITE_FONT_FACE_TYPE face_type,
        unsigned int face_index, unsigned int simulations, struct dwrite_fontset_entry **ret)
{
    struct dwrite_fontset_entry *entry;

    if (!(entry = calloc(1, sizeof(*entry))))
        return E_OUTOFMEMORY;

    entry->refcount = 1;
    entry->desc.file = file;
    IDWriteFontFile_AddRef(entry->desc.file);
    entry->desc.face_type = face_type;
    entry->desc.face_index = face_index;
    entry->desc.simulations = simulations;

    *ret = entry;

    return S_OK;
}

static void init_fontset(struct dwrite_fontset *object, IDWriteFactory7 *factory,
        struct dwrite_fontset_entry **entries, unsigned int count)
{
    object->IDWriteFontSet3_iface.lpVtbl = &fontsetvtbl;
    object->refcount = 1;
    object->factory = factory;
    IDWriteFactory7_AddRef(object->factory);
    object->entries = entries;
    object->count = count;
}

static HRESULT fontset_create_from_font_data(IDWriteFactory7 *factory, struct dwrite_font_data **fonts,
        unsigned int count, IDWriteFontSet1 **ret)
{
    struct dwrite_fontset_entry **entries = NULL;
    struct dwrite_fontset *object;
    unsigned int i;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (count)
    {
        entries = calloc(count, sizeof(*entries));

        /* FIXME: set available properties too */

        for (i = 0; i < count; ++i)
        {
            fontset_create_entry(fonts[i]->file, fonts[i]->face_type, fonts[i]->face_index,
                    fonts[i]->simulations, &entries[i]);
        }
    }
    init_fontset(object, factory, entries, count);

    *ret = (IDWriteFontSet1 *)&object->IDWriteFontSet3_iface;

    return S_OK;
}

static HRESULT fontset_builder_create_fontset(IDWriteFactory7 *factory, struct dwrite_fontset_entry **src_entries,
        unsigned int count, IDWriteFontSet **ret)
{
    struct dwrite_fontset_entry **entries = NULL;
    struct dwrite_fontset *object;
    unsigned int i;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (count)
    {
        entries = calloc(count, sizeof(*entries));

        for (i = 0; i < count; ++i)
            entries[i] = addref_fontset_entry(src_entries[i]);
    }
    init_fontset(object, factory, entries, count);

    *ret = (IDWriteFontSet *)&object->IDWriteFontSet3_iface;

    return S_OK;
}

static HRESULT WINAPI dwritefontsetbuilder_QueryInterface(IDWriteFontSetBuilder2 *iface,
        REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontSetBuilder2) ||
            IsEqualIID(riid, &IID_IDWriteFontSetBuilder1) ||
            IsEqualIID(riid, &IID_IDWriteFontSetBuilder) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFontSetBuilder2_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontsetbuilder_AddRef(IDWriteFontSetBuilder2 *iface)
{
    struct dwrite_fontset_builder *builder = impl_from_IDWriteFontSetBuilder2(iface);
    ULONG refcount = InterlockedIncrement(&builder->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritefontsetbuilder_Release(IDWriteFontSetBuilder2 *iface)
{
    struct dwrite_fontset_builder *builder = impl_from_IDWriteFontSetBuilder2(iface);
    ULONG refcount = InterlockedDecrement(&builder->refcount);
    unsigned int i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFactory7_Release(builder->factory);
        for (i = 0; i < builder->count; ++i)
            release_fontset_entry(builder->entries[i]);
        free(builder->entries);
        free(builder);
    }

    return refcount;
}

static HRESULT fontset_builder_add_entry(struct dwrite_fontset_builder *builder, const struct dwrite_fontset_entry_desc *desc)
{
    struct dwrite_fontset_entry *entry;
    HRESULT hr;

    if (!dwrite_array_reserve((void **)&builder->entries, &builder->capacity, builder->count + 1,
            sizeof(*builder->entries)))
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = fontset_create_entry(desc->file, desc->face_type, desc->face_index, desc->simulations, &entry)))
        return hr;

    builder->entries[builder->count++] = entry;

    return S_OK;
}

static HRESULT fontset_builder_add_file(struct dwrite_fontset_builder *builder, IDWriteFontFile *file)
{
    struct dwrite_fontset_entry_desc desc = { 0 };
    DWRITE_FONT_FILE_TYPE filetype;
    unsigned int i, face_count;
    BOOL supported = FALSE;
    HRESULT hr;

    desc.file = file;
    if (FAILED(hr = IDWriteFontFile_Analyze(desc.file, &supported, &filetype, &desc.face_type, &face_count)))
        return hr;

    if (!supported)
        return DWRITE_E_FILEFORMAT;

    for (i = 0; i < face_count; ++i)
    {
        desc.face_index = i;
        if (FAILED(hr = fontset_builder_add_entry(builder, &desc)))
            break;
    }

    return hr;
}

static HRESULT WINAPI dwritefontsetbuilder_AddFontFaceReference_(IDWriteFontSetBuilder2 *iface,
        IDWriteFontFaceReference *ref, DWRITE_FONT_PROPERTY const *props, UINT32 prop_count)
{
    FIXME("%p, %p, %p, %u.\n", iface, ref, props, prop_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontsetbuilder_AddFontFaceReference(IDWriteFontSetBuilder2 *iface,
        IDWriteFontFaceReference *reference)
{
    struct dwrite_fontset_builder *builder = impl_from_IDWriteFontSetBuilder2(iface);
    struct dwrite_fontset_entry_desc desc = { 0 };
    DWRITE_FONT_FILE_TYPE file_type;
    unsigned int face_count;
    BOOL supported;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, reference);

    if (FAILED(hr = IDWriteFontFaceReference_GetFontFile(reference, &desc.file))) return hr;

    if (SUCCEEDED(hr = IDWriteFontFile_Analyze(desc.file, &supported, &file_type, &desc.face_type, &face_count)))
    {
        if (!supported)
            hr = DWRITE_E_FILEFORMAT;

        if (SUCCEEDED(hr))
        {
            desc.face_index = IDWriteFontFaceReference_GetFontFaceIndex(reference);
            desc.simulations = IDWriteFontFaceReference_GetSimulations(reference);
            hr = fontset_builder_add_entry(builder, &desc);
        }
    }

    IDWriteFontFile_Release(desc.file);

    return hr;
}

static HRESULT WINAPI dwritefontsetbuilder_AddFontSet(IDWriteFontSetBuilder2 *iface, IDWriteFontSet *fontset)
{
    FIXME("%p, %p.\n", iface, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontsetbuilder_CreateFontSet(IDWriteFontSetBuilder2 *iface, IDWriteFontSet **fontset)
{
    struct dwrite_fontset_builder *builder = impl_from_IDWriteFontSetBuilder2(iface);

    TRACE("%p, %p.\n", iface, fontset);

    return fontset_builder_create_fontset(builder->factory, builder->entries, builder->count, fontset);
}

static HRESULT WINAPI dwritefontsetbuilder1_AddFontFile(IDWriteFontSetBuilder2 *iface, IDWriteFontFile *file)
{
    struct dwrite_fontset_builder *builder = impl_from_IDWriteFontSetBuilder2(iface);

    TRACE("%p, %p.\n", iface, file);

    return fontset_builder_add_file(builder, file);
}

static HRESULT WINAPI dwritefontsetbuilder2_AddFont(IDWriteFontSetBuilder2 *iface, IDWriteFontFile *file,
        unsigned int face_index, DWRITE_FONT_SIMULATIONS simulations, const DWRITE_FONT_AXIS_VALUE *axis_values,
        unsigned int num_values, const DWRITE_FONT_AXIS_RANGE *axis_ranges, unsigned int num_ranges,
        const DWRITE_FONT_PROPERTY *props, unsigned int num_properties)
{
    FIXME("%p, %p, %u, %#x, %p, %u, %p, %u, %p, %u.\n", iface, file, face_index, simulations, axis_values, num_values,
            axis_ranges, num_ranges, props, num_properties);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontsetbuilder2_AddFontFile(IDWriteFontSetBuilder2 *iface, const WCHAR *filepath)
{
    struct dwrite_fontset_builder *builder = impl_from_IDWriteFontSetBuilder2(iface);
    IDWriteFontFile *file;
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_w(filepath));

    if (FAILED(hr = IDWriteFactory7_CreateFontFileReference(builder->factory, filepath, NULL, &file)))
        return hr;

    hr = fontset_builder_add_file(builder, file);
    IDWriteFontFile_Release(file);
    return hr;
}

static const IDWriteFontSetBuilder2Vtbl fontsetbuildervtbl =
{
    dwritefontsetbuilder_QueryInterface,
    dwritefontsetbuilder_AddRef,
    dwritefontsetbuilder_Release,
    dwritefontsetbuilder_AddFontFaceReference_,
    dwritefontsetbuilder_AddFontFaceReference,
    dwritefontsetbuilder_AddFontSet,
    dwritefontsetbuilder_CreateFontSet,
    dwritefontsetbuilder1_AddFontFile,
    dwritefontsetbuilder2_AddFont,
    dwritefontsetbuilder2_AddFontFile,
};

HRESULT create_fontset_builder(IDWriteFactory7 *factory, IDWriteFontSetBuilder2 **ret)
{
    struct dwrite_fontset_builder *builder;

    *ret = NULL;

    if (!(builder = calloc(1, sizeof(*builder))))
        return E_OUTOFMEMORY;

    builder->IDWriteFontSetBuilder2_iface.lpVtbl = &fontsetbuildervtbl;
    builder->refcount = 1;
    builder->factory = factory;
    IDWriteFactory7_AddRef(builder->factory);

    *ret = &builder->IDWriteFontSetBuilder2_iface;

    return S_OK;
}
