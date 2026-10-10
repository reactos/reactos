/*
 * Copyright 2012, 2014-2022 Nikolay Sivov for CodeWeavers
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

#include <assert.h>
#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "dwrite_private.h"
#include "scripts.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

enum layout_range_attr_kind {
    LAYOUT_RANGE_ATTR_WEIGHT,
    LAYOUT_RANGE_ATTR_STYLE,
    LAYOUT_RANGE_ATTR_STRETCH,
    LAYOUT_RANGE_ATTR_FONTSIZE,
    LAYOUT_RANGE_ATTR_EFFECT,
    LAYOUT_RANGE_ATTR_INLINE,
    LAYOUT_RANGE_ATTR_UNDERLINE,
    LAYOUT_RANGE_ATTR_STRIKETHROUGH,
    LAYOUT_RANGE_ATTR_PAIR_KERNING,
    LAYOUT_RANGE_ATTR_FONTCOLL,
    LAYOUT_RANGE_ATTR_LOCALE,
    LAYOUT_RANGE_ATTR_FONTFAMILY,
    LAYOUT_RANGE_ATTR_SPACING,
    LAYOUT_RANGE_ATTR_TYPOGRAPHY
};

struct layout_range_attr_value {
    DWRITE_TEXT_RANGE range;
    union {
        DWRITE_FONT_WEIGHT weight;
        DWRITE_FONT_STYLE style;
        DWRITE_FONT_STRETCH stretch;
        FLOAT fontsize;
        IDWriteInlineObject *object;
        IUnknown *effect;
        BOOL underline;
        BOOL strikethrough;
        BOOL pair_kerning;
        IDWriteFontCollection *collection;
        const WCHAR *locale;
        const WCHAR *fontfamily;
        struct {
            FLOAT leading;
            FLOAT trailing;
            FLOAT min_advance;
        } spacing;
        IDWriteTypography *typography;
    } u;
};

enum layout_range_kind {
    LAYOUT_RANGE_REGULAR,
    LAYOUT_RANGE_UNDERLINE,
    LAYOUT_RANGE_STRIKETHROUGH,
    LAYOUT_RANGE_EFFECT,
    LAYOUT_RANGE_SPACING,
    LAYOUT_RANGE_TYPOGRAPHY
};

struct layout_range_header {
    struct list entry;
    enum layout_range_kind kind;
    DWRITE_TEXT_RANGE range;
};

struct layout_range {
    struct layout_range_header h;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;
    FLOAT fontsize;
    DWRITE_FONT_STRETCH stretch;
    IDWriteInlineObject *object;
    BOOL pair_kerning;
    IDWriteFontCollection *collection;
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    WCHAR *fontfamily;
};

struct layout_range_bool {
    struct layout_range_header h;
    BOOL value;
};

struct layout_range_iface {
    struct layout_range_header h;
    IUnknown *iface;
};

struct layout_range_spacing {
    struct layout_range_header h;
    FLOAT leading;
    FLOAT trailing;
    FLOAT min_advance;
};

enum layout_run_kind {
    LAYOUT_RUN_REGULAR,
    LAYOUT_RUN_INLINE
};

struct inline_object_run {
    IDWriteInlineObject *object;
    UINT16 length;
};

struct regular_layout_run {
    DWRITE_GLYPH_RUN_DESCRIPTION descr;
    DWRITE_GLYPH_RUN run;
    DWRITE_SCRIPT_ANALYSIS sa;
    UINT16 *glyphs;
    UINT16 *clustermap;
    FLOAT  *advances;
    DWRITE_GLYPH_OFFSET *offsets;
    UINT32 glyphcount; /* actual glyph count after shaping, not necessarily the same as reported to Draw() */
};

struct layout_run
{
    struct list entry;
    enum layout_run_kind kind;
    union
    {
        struct inline_object_run object;
        struct regular_layout_run regular;
    } u;
    float baseline;
    float height;
    unsigned int start_position; /* run text position in range [0, layout-text-length) */
};

struct layout_effective_run {
    struct list entry;
    const struct layout_run *run; /* nominal run this one is based on */
    UINT32 start;           /* relative text position, 0 means first text position of a nominal run */
    UINT32 length;          /* length in codepoints that this run covers */
    UINT32 glyphcount;      /* total glyph count in this run */
    IUnknown *effect;       /* original reference is kept only at range level */
    D2D1_POINT_2F origin;   /* baseline origin */
    FLOAT align_dx;         /* adjustment from text alignment */
    FLOAT width;            /* run width */
    UINT16 *clustermap;     /* effective clustermap, allocated separately, is not reused from nominal map */
    UINT32 line;            /* 0-based line index in line metrics array */
    BOOL underlined;        /* set if this run is underlined */
    D2D1_RECT_F bbox;       /* ink run box, top == bottom means it wasn't estimated yet */
};

struct layout_effective_inline {
    struct list entry;
    IDWriteInlineObject *object;  /* inline object, set explicitly or added when trimming a line */
    IUnknown *effect;             /* original reference is kept only at range level */
    FLOAT baseline;
    D2D1_POINT_2F origin;         /* left top corner */
    FLOAT align_dx;               /* adjustment from text alignment */
    FLOAT width;                  /* object width as it's reported it */
    BOOL  is_sideways;            /* vertical flow direction flag passed to Draw */
    BOOL  is_rtl;                 /* bidi flag passed to Draw */
    UINT32 line;                  /* 0-based line index in line metrics array */
};

struct layout_underline {
    struct list entry;
    const struct layout_effective_run *run;
    DWRITE_UNDERLINE u;
};

struct layout_strikethrough {
    struct list entry;
    const struct layout_effective_run *run;
    DWRITE_STRIKETHROUGH s;
};

struct layout_cluster {
    const struct layout_run *run; /* link to nominal run this cluster belongs to */
    UINT32 position;        /* relative to run, first cluster has 0 position */
};

struct layout_line
{
    float height;   /* height based on content */
    float baseline; /* baseline based on content */
    DWRITE_LINE_METRICS1 metrics;
};

enum layout_recompute_mask {
    RECOMPUTE_CLUSTERS            = 1 << 0,
    RECOMPUTE_MINIMAL_WIDTH       = 1 << 1,
    RECOMPUTE_LINES               = 1 << 2,
    RECOMPUTE_OVERHANGS           = 1 << 3,
    RECOMPUTE_LINES_AND_OVERHANGS = RECOMPUTE_LINES | RECOMPUTE_OVERHANGS,
    RECOMPUTE_EVERYTHING          = 0xffff
};

struct dwrite_textlayout
{
    IDWriteTextLayout4 IDWriteTextLayout4_iface;
    IDWriteTextFormat3 IDWriteTextFormat3_iface;
    IDWriteTextAnalysisSink1 IDWriteTextAnalysisSink1_iface;
    IDWriteTextAnalysisSource1 IDWriteTextAnalysisSource1_iface;
    LONG refcount;

    IDWriteFactory7 *factory;
    IDWriteFontCollection *system_collection;

    WCHAR *str;
    UINT32 len;

    struct
    {
        unsigned int offset;
        unsigned int length;
    } text_source;

    struct dwrite_textformat_data format;
    struct list strike_ranges;
    struct list underline_ranges;
    struct list typographies;
    struct list effects;
    struct list spacing;
    struct list ranges;
    struct list runs;
    /* lists ready to use by Draw() */
    struct list eruns;
    struct list inlineobjects;
    struct list underlines;
    struct list strikethrough;
    USHORT recompute;

    DWRITE_LINE_BREAKPOINT *nominal_breakpoints;
    DWRITE_LINE_BREAKPOINT *actual_breakpoints;

    struct layout_cluster *clusters;
    DWRITE_CLUSTER_METRICS *clustermetrics;
    UINT32 cluster_count;
    FLOAT  minwidth;

    struct layout_line *lines;
    size_t lines_size;

    DWRITE_TEXT_METRICS1 metrics;
    DWRITE_OVERHANG_METRICS overhangs;

    DWRITE_MEASURING_MODE measuringmode;

    /* gdi-compatible layout specifics */
    FLOAT ppdip;
    DWRITE_MATRIX transform;
};

struct dwrite_typography {
    IDWriteTypography IDWriteTypography_iface;
    LONG refcount;

    DWRITE_FONT_FEATURE *features;
    size_t capacity;
    size_t count;
};

static inline struct dwrite_textlayout *impl_from_IDWriteTextLayout4(IDWriteTextLayout4 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextLayout4_iface);
}

static inline struct dwrite_textlayout *impl_from_IDWriteTextFormat3(IDWriteTextFormat3 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextFormat3_iface);
}

static inline struct dwrite_textlayout *impl_from_IDWriteTextAnalysisSink1(IDWriteTextAnalysisSink1 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextAnalysisSink1_iface);
}

static inline struct dwrite_textlayout *impl_from_IDWriteTextAnalysisSource1(IDWriteTextAnalysisSource1 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextAnalysisSource1_iface);
}

static inline struct dwrite_typography *impl_from_IDWriteTypography(IDWriteTypography *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_typography, IDWriteTypography_iface);
}

static inline const char *debugstr_rundescr(const DWRITE_GLYPH_RUN_DESCRIPTION *descr)
{
    return wine_dbg_sprintf("[%u,%u)", descr->textPosition, descr->textPosition + descr->stringLength);
}

static inline BOOL is_layout_gdi_compatible(const struct dwrite_textlayout *layout)
{
    return layout->measuringmode != DWRITE_MEASURING_MODE_NATURAL;
}

static BOOL is_run_rtl(const struct layout_effective_run *run)
{
    return run->run->u.regular.run.bidiLevel & 1;
}

static HRESULT alloc_layout_run(enum layout_run_kind kind, unsigned int start_position,
        struct layout_run **run)
{
    if (!(*run = calloc(1, sizeof(**run))))
        return E_OUTOFMEMORY;

    (*run)->kind = kind;
    (*run)->start_position = start_position;

    return S_OK;
}

static void free_layout_runs(struct dwrite_textlayout *layout)
{
    struct layout_run *cur, *cur2;
    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->runs, struct layout_run, entry)
    {
        list_remove(&cur->entry);
        if (cur->kind == LAYOUT_RUN_REGULAR)
        {
            if (cur->u.regular.run.fontFace)
                IDWriteFontFace_Release(cur->u.regular.run.fontFace);
            free(cur->u.regular.glyphs);
            free(cur->u.regular.clustermap);
            free(cur->u.regular.advances);
            free(cur->u.regular.offsets);
        }
        free(cur);
    }
}

static void free_layout_eruns(struct dwrite_textlayout *layout)
{
    struct layout_effective_inline *in, *in2;
    struct layout_effective_run *cur, *cur2;
    struct layout_strikethrough *s, *s2;
    struct layout_underline *u, *u2;

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->eruns, struct layout_effective_run, entry)
    {
        list_remove(&cur->entry);
        free(cur->clustermap);
        free(cur);
    }

    LIST_FOR_EACH_ENTRY_SAFE(in, in2, &layout->inlineobjects, struct layout_effective_inline, entry)
    {
        list_remove(&in->entry);
        free(in);
    }

    LIST_FOR_EACH_ENTRY_SAFE(u, u2, &layout->underlines, struct layout_underline, entry)
    {
        list_remove(&u->entry);
        free(u);
    }

    LIST_FOR_EACH_ENTRY_SAFE(s, s2, &layout->strikethrough, struct layout_strikethrough, entry)
    {
        list_remove(&s->entry);
        free(s);
    }
}

/* Used to resolve break condition by forcing stronger condition over weaker. */
static inline DWRITE_BREAK_CONDITION override_break_condition(DWRITE_BREAK_CONDITION existingbreak, DWRITE_BREAK_CONDITION newbreak)
{
    switch (existingbreak) {
    case DWRITE_BREAK_CONDITION_NEUTRAL:
        return newbreak;
    case DWRITE_BREAK_CONDITION_CAN_BREAK:
        return newbreak == DWRITE_BREAK_CONDITION_NEUTRAL ? existingbreak : newbreak;
    /* let's keep stronger conditions as is */
    case DWRITE_BREAK_CONDITION_MAY_NOT_BREAK:
    case DWRITE_BREAK_CONDITION_MUST_BREAK:
        break;
    default:
        ERR("unknown break condition %d\n", existingbreak);
    }

    return existingbreak;
}

/* This helper should be used to get effective range length, in other words it returns number of text
   positions from range starting point to the end of the range, limited by layout text length */
static inline UINT32 get_clipped_range_length(const struct dwrite_textlayout *layout, const struct layout_range *range)
{
    if (range->h.range.startPosition + range->h.range.length <= layout->len)
        return range->h.range.length;
    return layout->len - range->h.range.startPosition;
}

/* Actual breakpoint data gets updated with break condition required by inline object set for range 'cur'. */
static HRESULT layout_update_breakpoints_range(struct dwrite_textlayout *layout, const struct layout_range *cur)
{
    DWRITE_BREAK_CONDITION before, after;
    UINT32 i, length;
    HRESULT hr;

    /* ignore returned conditions if failed */
    hr = IDWriteInlineObject_GetBreakConditions(cur->object, &before, &after);
    if (FAILED(hr))
        after = before = DWRITE_BREAK_CONDITION_NEUTRAL;

    if (!layout->actual_breakpoints)
    {
        if (!(layout->actual_breakpoints = calloc(layout->len, sizeof(*layout->actual_breakpoints))))
            return E_OUTOFMEMORY;
        memcpy(layout->actual_breakpoints, layout->nominal_breakpoints, sizeof(DWRITE_LINE_BREAKPOINT)*layout->len);
    }

    length = get_clipped_range_length(layout, cur);
    for (i = cur->h.range.startPosition; i < length + cur->h.range.startPosition; i++) {
        /* for first codepoint check if there's anything before it and update accordingly */
        if (i == cur->h.range.startPosition) {
            if (i > 0)
                layout->actual_breakpoints[i].breakConditionBefore = layout->actual_breakpoints[i-1].breakConditionAfter =
                    override_break_condition(layout->actual_breakpoints[i-1].breakConditionAfter, before);
            else
                layout->actual_breakpoints[i].breakConditionBefore = before;
            layout->actual_breakpoints[i].breakConditionAfter = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
        }
        /* similar check for last codepoint */
        else if (i == cur->h.range.startPosition + length - 1) {
            if (i == layout->len - 1)
                layout->actual_breakpoints[i].breakConditionAfter = after;
            else
                layout->actual_breakpoints[i].breakConditionAfter = layout->actual_breakpoints[i+1].breakConditionBefore =
                    override_break_condition(layout->actual_breakpoints[i+1].breakConditionBefore, after);
            layout->actual_breakpoints[i].breakConditionBefore = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
        }
        /* for all positions within a range disable breaks */
        else {
            layout->actual_breakpoints[i].breakConditionBefore = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
            layout->actual_breakpoints[i].breakConditionAfter = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
        }

        layout->actual_breakpoints[i].isWhitespace = 0;
        layout->actual_breakpoints[i].isSoftHyphen = 0;
    }

    return S_OK;
}

static struct layout_range *get_layout_range_by_pos(const struct dwrite_textlayout *layout, UINT32 pos)
{
    struct layout_range *cur;

    LIST_FOR_EACH_ENTRY(cur, &layout->ranges, struct layout_range, h.entry)
    {
        DWRITE_TEXT_RANGE *r = &cur->h.range;
        if (r->startPosition <= pos && pos < r->startPosition + r->length)
            return cur;
    }

    return NULL;
}

static struct layout_range_header *get_layout_range_header_by_pos(const struct list *ranges, UINT32 pos)
{
    struct layout_range_header *cur;

    LIST_FOR_EACH_ENTRY(cur, ranges, struct layout_range_header, entry)
    {
        DWRITE_TEXT_RANGE *r = &cur->range;
        if (r->startPosition <= pos && pos < r->startPosition + r->length)
            return cur;
    }

    return NULL;
}

static inline DWRITE_LINE_BREAKPOINT get_effective_breakpoint(const struct dwrite_textlayout *layout, UINT32 pos)
{
    if (layout->actual_breakpoints)
        return layout->actual_breakpoints[pos];
    return layout->nominal_breakpoints[pos];
}

static inline void init_cluster_metrics(const struct dwrite_textlayout *layout, const struct regular_layout_run *run,
    UINT16 start_glyph, UINT16 stop_glyph, UINT32 stop_position, UINT16 length, DWRITE_CLUSTER_METRICS *metrics)
{
    UINT8 breakcondition;
    UINT32 position;
    UINT16 j;

    /* For clusters made of control chars we report zero glyphs, and we need zero cluster
       width as well; advances are already computed at this point and are not necessary zero. */
    metrics->width = 0.0f;
    if (run->run.glyphCount) {
        for (j = start_glyph; j < stop_glyph; j++)
            metrics->width += run->run.glyphAdvances[j];
    }
    metrics->length = length;

    position = run->descr.textPosition + stop_position;
    if (stop_glyph == run->glyphcount)
        breakcondition = get_effective_breakpoint(layout, position).breakConditionAfter;
    else {
        breakcondition = get_effective_breakpoint(layout, position).breakConditionBefore;
        if (stop_position) position -= 1;
    }

    metrics->canWrapLineAfter = breakcondition == DWRITE_BREAK_CONDITION_CAN_BREAK ||
                                breakcondition == DWRITE_BREAK_CONDITION_MUST_BREAK;
    if (metrics->length == 1) {
        DWRITE_LINE_BREAKPOINT bp = get_effective_breakpoint(layout, position);
        metrics->isWhitespace = bp.isWhitespace;
        metrics->isNewline = metrics->canWrapLineAfter && lb_is_newline_char(layout->str[position]);
        metrics->isSoftHyphen = bp.isSoftHyphen;
    }
    else {
        metrics->isWhitespace = 0;
        metrics->isNewline = 0;
        metrics->isSoftHyphen = 0;
    }
    metrics->isRightToLeft = run->run.bidiLevel & 1;
    metrics->padding = 0;
}

/*

  All clusters in a 'run' will be added to 'layout' data, starting at index pointed to by 'cluster'.
  On return 'cluster' is updated to point to next metrics struct to be filled in on next call.
  Note that there's no need to reallocate anything at this point as we allocate one cluster per
  codepoint initially.

*/
static void layout_set_cluster_metrics(struct dwrite_textlayout *layout, const struct layout_run *r, UINT32 *cluster)
{
    DWRITE_CLUSTER_METRICS *metrics = &layout->clustermetrics[*cluster];
    struct layout_cluster *c = &layout->clusters[*cluster];
    const struct regular_layout_run *run = &r->u.regular;
    UINT32 i, start = 0;

    assert(r->kind == LAYOUT_RUN_REGULAR);

    for (i = 0; i < run->descr.stringLength; i++) {
        BOOL end = i == run->descr.stringLength - 1;

        if (run->descr.clusterMap[start] != run->descr.clusterMap[i]) {
            init_cluster_metrics(layout, run, run->descr.clusterMap[start], run->descr.clusterMap[i], i,
                i - start, metrics);
            c->position = start;
            c->run = r;

            *cluster += 1;
            metrics++;
            c++;
            start = i;
        }

        if (end) {
            init_cluster_metrics(layout, run, run->descr.clusterMap[start], run->glyphcount, i,
                i - start + 1, metrics);
            c->position = start;
            c->run = r;

            *cluster += 1;
            return;
        }
    }
}

#define SCALE_FONT_METRIC(metric, emSize, metrics) ((FLOAT)(metric) * (emSize) / (FLOAT)(metrics)->designUnitsPerEm)

static void layout_get_font_metrics(const struct dwrite_textlayout *layout, IDWriteFontFace *fontface, float emsize,
    DWRITE_FONT_METRICS *fontmetrics)
{
    if (is_layout_gdi_compatible(layout)) {
        HRESULT hr = IDWriteFontFace_GetGdiCompatibleMetrics(fontface, emsize, layout->ppdip, &layout->transform, fontmetrics);
        if (FAILED(hr))
            WARN("failed to get compat metrics, 0x%08lx\n", hr);
    }
    else
        IDWriteFontFace_GetMetrics(fontface, fontmetrics);
}

static inline void layout_get_font_height(float emsize, const DWRITE_FONT_METRICS *fontmetrics, float *baseline, float *height)
{
    *baseline = SCALE_FONT_METRIC(fontmetrics->ascent + fontmetrics->lineGap, emsize, fontmetrics);
    *height = SCALE_FONT_METRIC(fontmetrics->ascent + fontmetrics->descent + fontmetrics->lineGap, emsize, fontmetrics);
}

static inline void layout_initialize_text_source(struct dwrite_textlayout *layout, unsigned int offset,
        unsigned int length)
{
    layout->text_source.offset = offset;
    layout->text_source.length = length;
}

static HRESULT layout_itemize(struct dwrite_textlayout *layout)
{
    IDWriteTextAnalyzer2 *analyzer;
    struct layout_range *range;
    struct layout_run *r;
    HRESULT hr = S_OK;

    analyzer = get_text_analyzer();

    layout_initialize_text_source(layout, 0, layout->len);
    LIST_FOR_EACH_ENTRY(range, &layout->ranges, struct layout_range, h.entry) {
        /* We don't care about ranges that don't contain any text. */
        if (range->h.range.startPosition >= layout->len)
            break;

        /* Inline objects override actual text in range. */
        if (range->object) {
            hr = layout_update_breakpoints_range(layout, range);
            if (FAILED(hr))
                return hr;

            if (FAILED(hr = alloc_layout_run(LAYOUT_RUN_INLINE, range->h.range.startPosition, &r)))
                return hr;

            r->u.object.object = range->object;
            r->u.object.length = get_clipped_range_length(layout, range);
            list_add_tail(&layout->runs, &r->entry);
            continue;
        }

        /* Initial splitting by script. */
        hr = IDWriteTextAnalyzer2_AnalyzeScript(analyzer, (IDWriteTextAnalysisSource *)&layout->IDWriteTextAnalysisSource1_iface,
                range->h.range.startPosition, get_clipped_range_length(layout, range),
                (IDWriteTextAnalysisSink *)&layout->IDWriteTextAnalysisSink1_iface);
        if (FAILED(hr))
            break;

        /* Splitting further by bidi levels. */
        hr = IDWriteTextAnalyzer2_AnalyzeBidi(analyzer, (IDWriteTextAnalysisSource *)&layout->IDWriteTextAnalysisSource1_iface,
                range->h.range.startPosition, get_clipped_range_length(layout, range),
                (IDWriteTextAnalysisSink *)&layout->IDWriteTextAnalysisSink1_iface);
        if (FAILED(hr))
            break;
    }

    return hr;
}

static HRESULT layout_map_run_characters(struct dwrite_textlayout *layout, struct layout_run *r,
        IDWriteFontFallback *fallback, struct layout_run **remaining)
{
    struct regular_layout_run *run = &r->u.regular;
    IDWriteFontCollection *collection;
    struct layout_range *range;
    unsigned int length;
    HRESULT hr = S_OK;

    *remaining = NULL;

    range = get_layout_range_by_pos(layout, run->descr.textPosition);
    collection = range->collection ? range->collection : layout->system_collection;

    length = run->descr.stringLength;

    while (length)
    {
        unsigned int mapped_length = 0;
        IDWriteFont *font = NULL;
        float scale = 0.0f;

        run = &r->u.regular;

        layout_initialize_text_source(layout, run->descr.textPosition, run->descr.stringLength);
        hr = IDWriteFontFallback_MapCharacters(fallback, (IDWriteTextAnalysisSource *)&layout->IDWriteTextAnalysisSource1_iface,
                0, run->descr.stringLength, collection, range->fontfamily, range->weight, range->style, range->stretch,
                &mapped_length, &font, &scale);
        if (FAILED(hr))
        {
            WARN("%s: failed to map family %s, collection %p, hr %#lx.\n", debugstr_rundescr(&run->descr),
                    debugstr_w(range->fontfamily), collection, hr);
            return hr;
        }

        if (!font)
        {
            *remaining = r;
            return S_OK;
        }

        hr = IDWriteFont_CreateFontFace(font, &run->run.fontFace);
        IDWriteFont_Release(font);
        if (FAILED(hr))
        {
            WARN("Failed to create a font face, hr %#lx.\n", hr);
            return hr;
        }

        run->run.fontEmSize = range->fontsize * scale;

        if (mapped_length < length)
        {
            struct regular_layout_run *nextrun;
            struct layout_run *nextr;

            /* Keep mapped part for current run, add another run for the rest. */
            if (FAILED(hr = alloc_layout_run(LAYOUT_RUN_REGULAR, 0, &nextr)))
                return hr;

            *nextr = *r;
            nextr->start_position = run->descr.textPosition + mapped_length;
            nextrun = &nextr->u.regular;
            nextrun->run.fontFace = NULL;
            nextrun->descr.textPosition = nextr->start_position;
            nextrun->descr.stringLength = run->descr.stringLength - mapped_length;
            nextrun->descr.string = &layout->str[nextrun->descr.textPosition];
            run->descr.stringLength = mapped_length;
            list_add_after(&r->entry, &nextr->entry);
            r = nextr;
        }

        length -= min(length, mapped_length);
    }

    return hr;
}

static HRESULT layout_run_get_last_resort_font(const struct dwrite_textlayout *layout, const struct layout_range *range,
        IDWriteFontFace **fontface, float *size)
{
    IDWriteFont *font;
    HRESULT hr;

    if (FAILED(create_matching_font(range->collection, range->fontfamily, range->weight, range->style,
            range->stretch, &IID_IDWriteFont3, (void **)&font)))
    {
        if (FAILED(hr = create_matching_font(layout->system_collection, L"Tahoma", range->weight, range->style,
                range->stretch, &IID_IDWriteFont3, (void **)&font)))
        {
            WARN("Failed to create last resort font, hr %#lx.\n", hr);
            return hr;
        }
    }

    hr = IDWriteFont_CreateFontFace(font, fontface);
    IDWriteFont_Release(font);
    if (FAILED(hr))
    {
        WARN("Failed to create last resort font face, hr %#lx.\n", hr);
        return hr;
    }

    *size = range->fontsize;

    return hr;
}

static HRESULT layout_resolve_fonts(struct dwrite_textlayout *layout)
{
    IDWriteFontFallback *system_fallback;
    struct layout_run *r, *remaining;
    HRESULT hr;

    if (FAILED(hr = IDWriteFactory7_GetSystemFontFallback(layout->factory, &system_fallback)))
    {
        WARN("Failed to get system fallback, hr %#lx.\n", hr);
        return hr;
    }

    LIST_FOR_EACH_ENTRY(r, &layout->runs, struct layout_run, entry)
    {
        struct regular_layout_run *run = &r->u.regular;

        if (r->kind == LAYOUT_RUN_INLINE)
            continue;

        /* For textual runs use both custom and system fallback. For non-visual ones only use the system fallback,
           and no hard-coded names in assumption that support for missing control characters could be easily
           added to bundled fonts. */

        if (run->sa.shapes == DWRITE_SCRIPT_SHAPES_NO_VISUAL)
        {
            if (FAILED(hr = layout_map_run_characters(layout, r, system_fallback, &remaining)))
            {
                WARN("Failed to map fonts for non-visual run, hr %#lx.\n", hr);
                break;
            }
        }
        else
        {
            if (layout->format.fallback)
                hr = layout_map_run_characters(layout, r, layout->format.fallback, &remaining);
            else
                remaining = r;

            if (remaining)
                hr = layout_map_run_characters(layout, remaining, system_fallback, &remaining);
        }

        if (remaining)
        {
            hr = layout_run_get_last_resort_font(layout, get_layout_range_by_pos(layout, remaining->u.regular.descr.textPosition),
                    &remaining->u.regular.run.fontFace, &remaining->u.regular.run.fontEmSize);
        }

        if (FAILED(hr)) break;
    }

    IDWriteFontFallback_Release(system_fallback);

    return hr;
}

struct shaping_context
{
    IDWriteTextAnalyzer2 *analyzer;
    struct regular_layout_run *run;
    DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props;
    DWRITE_SHAPING_TEXT_PROPERTIES *text_props;

    struct
    {
        DWRITE_TYPOGRAPHIC_FEATURES **features;
        unsigned int *range_lengths;
        unsigned int range_count;
    } user_features;
};

static void layout_shape_clear_user_features_context(struct shaping_context *context)
{
    unsigned int i;

    for (i = 0; i < context->user_features.range_count; ++i)
    {
        free(context->user_features.features[i]->features);
        free(context->user_features.features[i]);
    }
    free(context->user_features.features);
    memset(&context->user_features, 0, sizeof(context->user_features));
}

static void layout_shape_clear_context(struct shaping_context *context)
{
    layout_shape_clear_user_features_context(context);
    free(context->glyph_props);
    free(context->text_props);
}

static HRESULT layout_shape_add_empty_user_features_range(struct shaping_context *context, unsigned int length)
{
    DWRITE_TYPOGRAPHIC_FEATURES *features;
    unsigned int r = context->user_features.range_count;

    if (!(context->user_features.features[r] = calloc(1, sizeof(*features))))
        return E_OUTOFMEMORY;

    context->user_features.range_lengths[r] = length;
    context->user_features.range_count++;

    return S_OK;
}

static HRESULT layout_shape_get_user_features(const struct dwrite_textlayout *layout, struct shaping_context *context)
{
    unsigned int i, f, start = 0, r, covered_length = 0, length, feature_count;
    struct regular_layout_run *run = context->run;
    DWRITE_TYPOGRAPHIC_FEATURES *features;
    struct layout_range_iface *range;
    IDWriteTypography *typography;
    HRESULT hr = E_OUTOFMEMORY;

    range = (struct layout_range_iface *)get_layout_range_header_by_pos(&layout->typographies, 0);
    if (range->h.range.length >= run->descr.stringLength && !range->iface)
        return S_OK;

    if (!(context->user_features.features = calloc(run->descr.stringLength, sizeof(*context->user_features.features))))
        goto failed;
    if (!(context->user_features.range_lengths = calloc(run->descr.stringLength, sizeof(*context->user_features.range_lengths))))
        goto failed;

    for (i = run->descr.textPosition; i < run->descr.textPosition + run->descr.stringLength; ++i)
    {
        range = (struct layout_range_iface *)get_layout_range_header_by_pos(&layout->typographies, i);
        if (!range || !range->iface) continue;

        typography = (IDWriteTypography *)range->iface;
        feature_count = IDWriteTypography_GetFontFeatureCount(typography);
        if (!feature_count)
        {
            i = range->h.range.startPosition + range->h.range.length;
            continue;
        }

        if (start != i)
        {
            if (FAILED(hr = layout_shape_add_empty_user_features_range(context, i - start))) goto failed;
            covered_length += i - start;
            start += range->h.range.length;
        }

        r = context->user_features.range_count;
        if (!(features = context->user_features.features[r] = malloc(sizeof(*features))))
            goto failed;

        context->user_features.range_lengths[r] = length = min(run->descr.textPosition + run->descr.stringLength,
                range->h.range.startPosition + range->h.range.length) - i;
        features->featureCount = feature_count;
        if (!(features->features = calloc(feature_count, sizeof(*features->features))))
            goto failed;

        for (f = 0; f < feature_count; ++f)
        {
            IDWriteTypography_GetFontFeature(typography, f, &features->features[f]);
        }

        i += length;
        covered_length += length;
        context->user_features.range_count++;
    }

    if (context->user_features.range_count && covered_length < run->descr.stringLength)
    {
        if (FAILED(hr = layout_shape_add_empty_user_features_range(context, run->descr.stringLength - covered_length)))
            goto failed;
    }

    hr = S_OK;

failed:

    if (!context->user_features.range_count || FAILED(hr))
        layout_shape_clear_user_features_context(context);

    return hr;
}

static HRESULT layout_shape_get_glyphs(struct dwrite_textlayout *layout, struct shaping_context *context)
{
    struct regular_layout_run *run = context->run;
    unsigned int max_count;
    HRESULT hr;

    run->descr.localeName = get_layout_range_by_pos(layout, run->descr.textPosition)->locale;
    run->clustermap = calloc(run->descr.stringLength, sizeof(*run->clustermap));
    if (!run->clustermap)
        return E_OUTOFMEMORY;

    max_count = 3 * run->descr.stringLength / 2 + 16;
    run->glyphs = calloc(max_count, sizeof(*run->glyphs));
    if (!run->glyphs)
        return E_OUTOFMEMORY;

    context->text_props = calloc(run->descr.stringLength, sizeof(*context->text_props));
    context->glyph_props = calloc(max_count, sizeof(*context->glyph_props));
    if (!context->text_props || !context->glyph_props)
        return E_OUTOFMEMORY;

    if (FAILED(hr = layout_shape_get_user_features(layout, context)))
        return hr;

    for (;;)
    {
        hr = IDWriteTextAnalyzer2_GetGlyphs(context->analyzer, run->descr.string, run->descr.stringLength, run->run.fontFace,
                run->run.isSideways, run->run.bidiLevel & 1, &run->sa, run->descr.localeName, NULL /* FIXME */,
                (const DWRITE_TYPOGRAPHIC_FEATURES **)context->user_features.features, context->user_features.range_lengths,
                context->user_features.range_count, max_count, run->clustermap, context->text_props, run->glyphs,
                context->glyph_props, &run->glyphcount);
        if (hr == E_NOT_SUFFICIENT_BUFFER)
        {
            free(run->glyphs);
            free(context->glyph_props);

            max_count *= 2;

            run->glyphs = calloc(max_count, sizeof(*run->glyphs));
            context->glyph_props = calloc(max_count, sizeof(*context->glyph_props));
            if (!run->glyphs || !context->glyph_props)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            continue;
        }

        break;
    }

    if (FAILED(hr))
        WARN("%s: shaping failed, hr %#lx.\n", debugstr_rundescr(&run->descr), hr);

    run->run.glyphIndices = run->glyphs;
    run->descr.clusterMap = run->clustermap;

    return hr;
}

static struct layout_range_spacing *layout_get_next_spacing_range(const struct dwrite_textlayout *layout,
        const struct layout_range_spacing *cur)
{
    return (struct layout_range_spacing *)LIST_ENTRY(list_next(&layout->spacing, &cur->h.entry),
            struct layout_range_header, entry);
}

static HRESULT layout_shape_apply_character_spacing(struct dwrite_textlayout *layout, struct shaping_context *context)
{
    struct regular_layout_run *run = context->run;
    struct layout_range_spacing *first = NULL, *last = NULL, *cur;
    unsigned int i, length, pos, start, end, g0, glyph_count;
    struct layout_range_header *h;
    UINT16 *clustermap;

    LIST_FOR_EACH_ENTRY(h, &layout->spacing, struct layout_range_header, entry)
    {
        if ((h->range.startPosition >= run->descr.textPosition &&
                h->range.startPosition <= run->descr.textPosition + run->descr.stringLength) ||
            (run->descr.textPosition >= h->range.startPosition &&
                run->descr.textPosition <= h->range.startPosition + h->range.length))
        {
            if (!first) first = last = (struct layout_range_spacing *)h;
        }
        else if (last) break;
    }
    if (!first) return S_OK;

    if (!(clustermap = calloc(run->descr.stringLength, sizeof(*clustermap))))
        return E_OUTOFMEMORY;

    pos = run->descr.textPosition;

    for (cur = first;; cur = layout_get_next_spacing_range(layout, cur))
    {
        float leading, trailing;

        /* The range current spacing settings apply to. */
        start = max(pos, cur->h.range.startPosition);
        pos = end = min(pos + run->descr.stringLength, cur->h.range.startPosition + cur->h.range.length);

        /* Back to run-relative index. */
        start -= run->descr.textPosition;
        end -= run->descr.textPosition;

        length = end - start;

        g0 = run->descr.clusterMap[start];

        for (i = 0; i < length; ++i)
            clustermap[i] = run->descr.clusterMap[start + i] - run->descr.clusterMap[start];

        glyph_count = (end < run->descr.stringLength ? run->descr.clusterMap[end] + 1 : run->glyphcount) - g0;

        /* There is no direction argument for spacing interface, we have to swap arguments here to get desired output. */
        if (run->run.bidiLevel & 1)
        {
            leading = cur->trailing;
            trailing = cur->leading;
        }
        else
        {
            leading = cur->leading;
            trailing = cur->trailing;
        }
        IDWriteTextAnalyzer2_ApplyCharacterSpacing(context->analyzer, leading, trailing, cur->min_advance,
                length, glyph_count, clustermap, &run->advances[g0], &run->offsets[g0], &context->glyph_props[g0],
                &run->advances[g0], &run->offsets[g0]);

        if (cur == last) break;
    }

    free(clustermap);

    return S_OK;
}

static HRESULT layout_shape_get_positions(struct dwrite_textlayout *layout, struct shaping_context *context)
{
    struct regular_layout_run *run = context->run;
    HRESULT hr;

    run->advances = calloc(run->glyphcount, sizeof(*run->advances));
    run->offsets = calloc(run->glyphcount, sizeof(*run->offsets));
    if (!run->advances || !run->offsets)
        return E_OUTOFMEMORY;

    /* Get advances and offsets. */
    if (is_layout_gdi_compatible(layout))
        hr = IDWriteTextAnalyzer2_GetGdiCompatibleGlyphPlacements(context->analyzer, run->descr.string, run->descr.clusterMap,
                context->text_props, run->descr.stringLength, run->run.glyphIndices, context->glyph_props, run->glyphcount,
                run->run.fontFace, run->run.fontEmSize, layout->ppdip, &layout->transform,
                layout->measuringmode == DWRITE_MEASURING_MODE_GDI_NATURAL, run->run.isSideways, run->run.bidiLevel & 1,
                &run->sa, run->descr.localeName, (const DWRITE_TYPOGRAPHIC_FEATURES **)context->user_features.features,
                context->user_features.range_lengths, context->user_features.range_count, run->advances, run->offsets);
    else
        hr = IDWriteTextAnalyzer2_GetGlyphPlacements(context->analyzer, run->descr.string, run->descr.clusterMap,
                context->text_props, run->descr.stringLength, run->run.glyphIndices, context->glyph_props, run->glyphcount,
                run->run.fontFace, run->run.fontEmSize, run->run.isSideways, run->run.bidiLevel & 1, &run->sa,
                run->descr.localeName, (const DWRITE_TYPOGRAPHIC_FEATURES **)context->user_features.features,
                context->user_features.range_lengths, context->user_features.range_count, run->advances, run->offsets);

    if (FAILED(hr))
    {
        memset(run->advances, 0, run->glyphcount * sizeof(*run->advances));
        memset(run->offsets, 0, run->glyphcount * sizeof(*run->offsets));
        WARN("%s: failed to get glyph placement info, hr %#lx.\n", debugstr_rundescr(&run->descr), hr);
    }

    if (SUCCEEDED(hr))
        hr = layout_shape_apply_character_spacing(layout, context);

    run->run.glyphAdvances = run->advances;
    run->run.glyphOffsets = run->offsets;

    return hr;
}

static HRESULT layout_shape_run(struct dwrite_textlayout *layout, struct regular_layout_run *run)
{
    struct shaping_context context = { 0 };
    HRESULT hr;

    context.analyzer = get_text_analyzer();
    context.run = run;

    if (SUCCEEDED(hr = layout_shape_get_glyphs(layout, &context)))
        hr = layout_shape_get_positions(layout, &context);

    layout_shape_clear_context(&context);

    /* Special treatment for runs that don't produce visual output, shaping code adds normal glyphs for them,
       with valid cluster map and potentially with non-zero advances; layout code exposes those as zero
       width clusters. */
    if (run->sa.shapes == DWRITE_SCRIPT_SHAPES_NO_VISUAL)
        run->run.glyphCount = 0;
    else
        run->run.glyphCount = run->glyphcount;

    return hr;
}

static HRESULT layout_compute_runs(struct dwrite_textlayout *layout)
{
    struct layout_run *r;
    UINT32 cluster = 0;
    HRESULT hr;

    free_layout_eruns(layout);
    free_layout_runs(layout);

    /* Cluster data arrays are allocated once, assuming one text position per cluster. */
    if (!layout->clustermetrics && layout->len)
    {
        layout->clustermetrics = calloc(layout->len, sizeof(*layout->clustermetrics));
        layout->clusters = calloc(layout->len, sizeof(*layout->clusters));
        if (!layout->clustermetrics || !layout->clusters)
        {
            free(layout->clustermetrics);
            free(layout->clusters);
            return E_OUTOFMEMORY;
        }
    }
    layout->cluster_count = 0;

    if (FAILED(hr = layout_itemize(layout))) {
        WARN("Itemization failed, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = layout_resolve_fonts(layout))) {
        WARN("Failed to resolve layout fonts, hr %#lx.\n", hr);
        return hr;
    }

    /* fill run info */
    LIST_FOR_EACH_ENTRY(r, &layout->runs, struct layout_run, entry) {
        struct regular_layout_run *run = &r->u.regular;
        DWRITE_FONT_METRICS fontmetrics = { 0 };

        /* we need to do very little in case of inline objects */
        if (r->kind == LAYOUT_RUN_INLINE) {
            DWRITE_CLUSTER_METRICS *metrics = &layout->clustermetrics[cluster];
            struct layout_cluster *c = &layout->clusters[cluster];
            DWRITE_INLINE_OBJECT_METRICS inlinemetrics;

            metrics->width = 0.0f;
            metrics->length = r->u.object.length;
            metrics->canWrapLineAfter = 0;
            metrics->isWhitespace = 0;
            metrics->isNewline = 0;
            metrics->isSoftHyphen = 0;
            metrics->isRightToLeft = 0;
            metrics->padding = 0;
            c->run = r;
            c->position = 0; /* there's always one cluster per inline object, so 0 is valid value */
            cluster++;

            /* it's not fatal if GetMetrics() fails, all returned metrics are ignored */
            hr = IDWriteInlineObject_GetMetrics(r->u.object.object, &inlinemetrics);
            if (FAILED(hr)) {
                memset(&inlinemetrics, 0, sizeof(inlinemetrics));
                hr = S_OK;
            }
            metrics->width = inlinemetrics.width;
            r->baseline = inlinemetrics.baseline;
            r->height = inlinemetrics.height;

            /* FIXME: use resolved breakpoints in this case too */

            continue;
        }

        if (FAILED(hr = layout_shape_run(layout, run)))
            WARN("%s: shaping failed, hr %#lx.\n", debugstr_rundescr(&run->descr), hr);

        /* baseline derived from font metrics */
        layout_get_font_metrics(layout, run->run.fontFace, run->run.fontEmSize, &fontmetrics);
        layout_get_font_height(run->run.fontEmSize, &fontmetrics, &r->baseline, &r->height);

        layout_set_cluster_metrics(layout, r, &cluster);
    }

    if (hr == S_OK) {
        layout->cluster_count = cluster;
        if (cluster)
            layout->clustermetrics[cluster-1].canWrapLineAfter = 1;
    }

    return hr;
}

static HRESULT layout_compute(struct dwrite_textlayout *layout)
{
    HRESULT hr;

    if (!(layout->recompute & RECOMPUTE_CLUSTERS))
        return S_OK;

    /* nominal breakpoints are evaluated only once, because string never changes */
    if (!layout->nominal_breakpoints)
    {
        IDWriteTextAnalyzer2 *analyzer;

        if (!(layout->nominal_breakpoints = calloc(layout->len, sizeof(*layout->nominal_breakpoints))))
            return E_OUTOFMEMORY;

        analyzer = get_text_analyzer();

        layout_initialize_text_source(layout, 0, layout->len);
        if (FAILED(hr = IDWriteTextAnalyzer2_AnalyzeLineBreakpoints(analyzer,
                (IDWriteTextAnalysisSource *)&layout->IDWriteTextAnalysisSource1_iface,
                0, layout->len, (IDWriteTextAnalysisSink *)&layout->IDWriteTextAnalysisSink1_iface)))
            WARN("Line breakpoints analysis failed, hr %#lx.\n", hr);
    }

    free(layout->actual_breakpoints);
    layout->actual_breakpoints = NULL;

    hr = layout_compute_runs(layout);

    if (TRACE_ON(dwrite)) {
        struct layout_run *cur;

        LIST_FOR_EACH_ENTRY(cur, &layout->runs, struct layout_run, entry) {
            if (cur->kind == LAYOUT_RUN_INLINE)
                TRACE("run inline object %p, len %u\n", cur->u.object.object, cur->u.object.length);
            else
                TRACE("run [%u,%u], len %u, bidilevel %u\n", cur->u.regular.descr.textPosition, cur->u.regular.descr.textPosition +
                    cur->u.regular.descr.stringLength-1, cur->u.regular.descr.stringLength, cur->u.regular.run.bidiLevel);
        }
    }

    layout->recompute &= ~RECOMPUTE_CLUSTERS;
    return hr;
}

static inline float get_cluster_range_width(const struct dwrite_textlayout *layout, UINT32 start, UINT32 end)
{
    float width = 0.0f;
    for (; start < end; start++)
        width += layout->clustermetrics[start].width;
    return width;
}

static inline IUnknown *layout_get_effect_from_pos(const struct dwrite_textlayout *layout, UINT32 pos)
{
    struct layout_range_header *h = get_layout_range_header_by_pos(&layout->effects, pos);
    return ((struct layout_range_iface*)h)->iface;
}

/* A set of parameters that additionally splits resulting runs. It happens after shaping and all text processing,
   no glyph changes are possible. It's understandable for drawing effects, because DrawGlyphRun() reports them as
   one of the arguments, but it also happens for decorations, so every effective run has uniform
   underline/strikethough/effect tuple. */
struct layout_final_splitting_params {
    BOOL strikethrough;
    BOOL underline;
    IUnknown *effect;
};

static inline BOOL layout_get_strikethrough_from_pos(const struct dwrite_textlayout *layout, UINT32 pos)
{
    struct layout_range_header *h = get_layout_range_header_by_pos(&layout->strike_ranges, pos);
    return ((struct layout_range_bool*)h)->value;
}

static inline BOOL layout_get_underline_from_pos(const struct dwrite_textlayout *layout, UINT32 pos)
{
    struct layout_range_header *h = get_layout_range_header_by_pos(&layout->underline_ranges, pos);
    return ((struct layout_range_bool*)h)->value;
}

static void layout_splitting_params_from_pos(const struct dwrite_textlayout *layout, UINT32 pos,
    struct layout_final_splitting_params *params)
{
    params->strikethrough = layout_get_strikethrough_from_pos(layout, pos);
    params->underline = layout_get_underline_from_pos(layout, pos);
    params->effect = layout_get_effect_from_pos(layout, pos);
}

static BOOL is_same_splitting_params(const struct layout_final_splitting_params *left,
    const struct layout_final_splitting_params *right)
{
    return left->strikethrough == right->strikethrough &&
           left->underline == right->underline &&
           left->effect == right->effect;
}

static void layout_get_erun_font_metrics(const struct dwrite_textlayout *layout, const struct layout_effective_run *erun,
    DWRITE_FONT_METRICS *metrics)
{
    memset(metrics, 0, sizeof(*metrics));
    if (is_layout_gdi_compatible(layout)) {
        HRESULT hr = IDWriteFontFace_GetGdiCompatibleMetrics(
            erun->run->u.regular.run.fontFace,
            erun->run->u.regular.run.fontEmSize,
            layout->ppdip,
            &layout->transform,
            metrics);
        if (FAILED(hr))
            WARN("failed to get font metrics, 0x%08lx\n", hr);
    }
    else
        IDWriteFontFace_GetMetrics(erun->run->u.regular.run.fontFace, metrics);
}

/* Effective run is built from consecutive clusters of a single nominal run, 'first_cluster' is 0 based cluster index,
   'cluster_count' indicates how many clusters to add, including first one. */
static HRESULT layout_add_effective_run(struct dwrite_textlayout *layout, const struct layout_run *r, UINT32 first_cluster,
    UINT32 cluster_count, UINT32 line, FLOAT origin_x, struct layout_final_splitting_params *params)
{
    BOOL is_rtl = layout->format.readingdir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
    UINT32 i, start, length, last_cluster;
    struct layout_effective_run *run;

    if (r->kind == LAYOUT_RUN_INLINE)
    {
        struct layout_effective_inline *inlineobject;

        if (!(inlineobject = malloc(sizeof(*inlineobject))))
            return E_OUTOFMEMORY;

        inlineobject->object = r->u.object.object;
        inlineobject->width = get_cluster_range_width(layout, first_cluster, first_cluster + cluster_count);
        inlineobject->origin.x = is_rtl ? origin_x - inlineobject->width : origin_x;
        inlineobject->origin.y = 0.0f; /* set after line is built */
        inlineobject->align_dx = 0.0f;
        inlineobject->baseline = r->baseline;

        /* It's not clear how these two are set, possibly directionality
           is derived from surrounding text (replaced text could have
           different ranges which differ in reading direction). */
        inlineobject->is_sideways = FALSE;
        inlineobject->is_rtl = FALSE;
        inlineobject->line = line;

        /* effect assigned from start position and on is used for inline objects */
        inlineobject->effect = layout_get_effect_from_pos(layout, layout->clusters[first_cluster].position +
                layout->clusters[first_cluster].run->start_position);

        list_add_tail(&layout->inlineobjects, &inlineobject->entry);
        return S_OK;
    }

    if (!(run = malloc(sizeof(*run))))
        return E_OUTOFMEMORY;

    /* No need to iterate for that, use simple fact that:
       <last cluster position> = <first cluster position> + <sum of cluster lengths not including last one> */
    last_cluster = first_cluster + cluster_count - 1;
    length = layout->clusters[last_cluster].position - layout->clusters[first_cluster].position +
        layout->clustermetrics[last_cluster].length;

    if (!(run->clustermap = calloc(length, sizeof(*run->clustermap))))
    {
        free(run);
        return E_OUTOFMEMORY;
    }

    run->run = r;
    run->start = start = layout->clusters[first_cluster].position;
    run->length = length;
    run->width = get_cluster_range_width(layout, first_cluster, first_cluster + cluster_count);
    memset(&run->bbox, 0, sizeof(run->bbox));

    /* Adjust by run width if direction differs. */
    if (is_run_rtl(run) != is_rtl)
        run->origin.x = origin_x + (is_rtl ? -run->width : run->width);
    else
        run->origin.x = origin_x;

    run->origin.y = 0.0f; /* set after line is built */
    run->align_dx = 0.0f;
    run->line = line;

    if (r->u.regular.run.glyphCount) {
        /* Trim leading and trailing clusters. */
        run->glyphcount = r->u.regular.run.glyphCount - r->u.regular.clustermap[start];
        if (start + length < r->u.regular.descr.stringLength)
            run->glyphcount -= r->u.regular.run.glyphCount - r->u.regular.clustermap[start + length];
    }
    else
        run->glyphcount = 0;

    /* cluster map needs to be shifted */
    for (i = 0; i < length; i++)
        run->clustermap[i] = r->u.regular.clustermap[start + i] - r->u.regular.clustermap[start];

    run->effect = params->effect;
    run->underlined = params->underline;
    list_add_tail(&layout->eruns, &run->entry);

    /* Strikethrough style is guaranteed to be consistent within effective run,
       its width equals to run width, thickness and offset are derived from
       font metrics, rest of the values are from layout or run itself */
    if (params->strikethrough)
    {
        struct layout_strikethrough *s;
        DWRITE_FONT_METRICS metrics;

        if (!(s = malloc(sizeof(*s))))
            return E_OUTOFMEMORY;

        layout_get_erun_font_metrics(layout, run, &metrics);
        s->s.width = get_cluster_range_width(layout, first_cluster, first_cluster + cluster_count);
        s->s.thickness = SCALE_FONT_METRIC(metrics.strikethroughThickness, r->u.regular.run.fontEmSize, &metrics);
        /* Negative offset moves it above baseline as Y coordinate grows downward. */
        s->s.offset = -SCALE_FONT_METRIC(metrics.strikethroughPosition, r->u.regular.run.fontEmSize, &metrics);
        s->s.readingDirection = layout->format.readingdir;
        s->s.flowDirection = layout->format.flow;
        s->s.localeName = r->u.regular.descr.localeName;
        s->s.measuringMode = layout->measuringmode;
        s->run = run;

        list_add_tail(&layout->strikethrough, &s->entry);
    }

    return S_OK;
}

static void layout_apply_line_spacing(struct dwrite_textlayout *layout, UINT32 line)
{
    switch (layout->format.spacing.method)
    {
    case DWRITE_LINE_SPACING_METHOD_DEFAULT:
        layout->lines[line].metrics.height = layout->lines[line].height;
        layout->lines[line].metrics.baseline = layout->lines[line].baseline;
        break;
    case DWRITE_LINE_SPACING_METHOD_UNIFORM:
        layout->lines[line].metrics.height = layout->format.spacing.height;
        layout->lines[line].metrics.baseline = layout->format.spacing.baseline;
        break;
    case DWRITE_LINE_SPACING_METHOD_PROPORTIONAL:
        layout->lines[line].metrics.height = layout->lines[line].height * layout->format.spacing.height;
        layout->lines[line].metrics.baseline = layout->lines[line].baseline * layout->format.spacing.baseline;
        break;
    default:
        ERR("Unknown spacing method %u\n", layout->format.spacing.method);
    }
}

static HRESULT layout_set_line_metrics(struct dwrite_textlayout *layout, DWRITE_LINE_METRICS1 *metrics)
{
    size_t i = layout->metrics.lineCount;

    if (!dwrite_array_reserve((void **)&layout->lines, &layout->lines_size, layout->metrics.lineCount + 1,
            sizeof(*layout->lines)))
    {
        return E_OUTOFMEMORY;
    }

    layout->lines[i].metrics = *metrics;
    layout->lines[i].height = metrics->height;
    layout->lines[i].baseline = metrics->baseline;

    if (layout->format.spacing.method != DWRITE_LINE_SPACING_METHOD_DEFAULT)
        layout_apply_line_spacing(layout, i);

    layout->metrics.lineCount++;
    return S_OK;
}

static inline struct layout_effective_run *layout_get_next_erun(const struct dwrite_textlayout *layout,
    const struct layout_effective_run *cur)
{
    struct list *e;

    if (!cur)
        e = list_head(&layout->eruns);
    else
        e = list_next(&layout->eruns, &cur->entry);
    if (!e)
        return NULL;
    return LIST_ENTRY(e, struct layout_effective_run, entry);
}

static inline struct layout_effective_run *layout_get_prev_erun(const struct dwrite_textlayout *layout,
    const struct layout_effective_run *cur)
{
    struct list *e;

    if (!cur)
        e = list_tail(&layout->eruns);
    else
        e = list_prev(&layout->eruns, &cur->entry);
    if (!e)
        return NULL;
    return LIST_ENTRY(e, struct layout_effective_run, entry);
}

static inline struct layout_effective_inline *layout_get_next_inline_run(const struct dwrite_textlayout *layout,
    const struct layout_effective_inline *cur)
{
    struct list *e;

    if (!cur)
        e = list_head(&layout->inlineobjects);
    else
        e = list_next(&layout->inlineobjects, &cur->entry);
    if (!e)
        return NULL;
    return LIST_ENTRY(e, struct layout_effective_inline, entry);
}

static float layout_get_line_width(const struct dwrite_textlayout *layout, const struct layout_effective_run *erun,
        const struct layout_effective_inline *inrun, UINT32 line)
{
    FLOAT width = 0.0f;

    while (erun && erun->line == line) {
        width += erun->width;
        erun = layout_get_next_erun(layout, erun);
        if (!erun)
            break;
    }

    while (inrun && inrun->line == line) {
        width += inrun->width;
        inrun = layout_get_next_inline_run(layout, inrun);
        if (!inrun)
            break;
    }

    return width;
}

static inline BOOL should_skip_transform(const DWRITE_MATRIX *m, FLOAT *det)
{
    *det = m->m11 * m->m22 - m->m12 * m->m21;
    /* on certain conditions we can skip transform */
    return (!memcmp(m, &identity, sizeof(*m)) || fabsf(*det) <= 1e-10f);
}

static inline void layout_apply_snapping(D2D1_POINT_2F *vec, BOOL skiptransform, FLOAT ppdip,
    const DWRITE_MATRIX *m, FLOAT det)
{
    if (!skiptransform) {
        D2D1_POINT_2F vec2;

        /* apply transform */
        vec->x *= ppdip;
        vec->y *= ppdip;

        vec2.x = m->m11 * vec->x + m->m21 * vec->y + m->dx;
        vec2.y = m->m12 * vec->x + m->m22 * vec->y + m->dy;

        /* snap */
        vec2.x = floorf(vec2.x + 0.5f);
        vec2.y = floorf(vec2.y + 0.5f);

        /* apply inverted transform, we don't care about X component at this point */
        vec->x = (m->m22 * vec2.x - m->m21 * vec2.y + m->m21 * m->dy - m->m22 * m->dx) / det;
        vec->x /= ppdip;

        vec->y = (-m->m12 * vec2.x + m->m11 * vec2.y - (m->m11 * m->dy - m->m12 * m->dx)) / det;
        vec->y /= ppdip;
    }
    else {
        vec->x = floorf(vec->x * ppdip + 0.5f) / ppdip;
        vec->y = floorf(vec->y * ppdip + 0.5f) / ppdip;
    }
}

static void layout_apply_leading_alignment(struct dwrite_textlayout *layout)
{
    BOOL is_rtl = layout->format.readingdir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
    struct layout_effective_inline *inrun;
    struct layout_effective_run *erun;

    erun = layout_get_next_erun(layout, NULL);
    inrun = layout_get_next_inline_run(layout, NULL);

    while (erun) {
        erun->align_dx = 0.0f;
        erun = layout_get_next_erun(layout, erun);
    }

    while (inrun) {
        inrun->align_dx = 0.0f;
        inrun = layout_get_next_inline_run(layout, inrun);
    }

    layout->metrics.left = is_rtl ? layout->metrics.layoutWidth - layout->metrics.width : 0.0f;
}

static void layout_apply_trailing_alignment(struct dwrite_textlayout *layout)
{
    BOOL is_rtl = layout->format.readingdir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
    struct layout_effective_inline *inrun;
    struct layout_effective_run *erun;
    UINT32 line;

    erun = layout_get_next_erun(layout, NULL);
    inrun = layout_get_next_inline_run(layout, NULL);

    for (line = 0; line < layout->metrics.lineCount; line++) {
        FLOAT width = layout_get_line_width(layout, erun, inrun, line);
        FLOAT shift = layout->metrics.layoutWidth - width;

        if (is_rtl)
            shift *= -1.0f;

        while (erun && erun->line == line) {
            erun->align_dx = shift;
            erun = layout_get_next_erun(layout, erun);
        }

        while (inrun && inrun->line == line) {
            inrun->align_dx = shift;
            inrun = layout_get_next_inline_run(layout, inrun);
        }
    }

    layout->metrics.left = is_rtl ? 0.0f : layout->metrics.layoutWidth - layout->metrics.width;
}

static inline float layout_get_centered_shift(const struct dwrite_textlayout *layout, BOOL skiptransform,
    FLOAT width, FLOAT det)
{
    if (is_layout_gdi_compatible(layout)) {
        D2D1_POINT_2F vec = { layout->metrics.layoutWidth - width, 0.0f};
        layout_apply_snapping(&vec, skiptransform, layout->ppdip, &layout->transform, det);
        return floorf(vec.x / 2.0f);
    }
    else
        return (layout->metrics.layoutWidth - width) / 2.0f;
}

static void layout_apply_centered_alignment(struct dwrite_textlayout *layout)
{
    BOOL is_rtl = layout->format.readingdir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
    struct layout_effective_inline *inrun;
    struct layout_effective_run *erun;
    BOOL skiptransform;
    UINT32 line;
    FLOAT det;

    erun = layout_get_next_erun(layout, NULL);
    inrun = layout_get_next_inline_run(layout, NULL);

    skiptransform = should_skip_transform(&layout->transform, &det);

    for (line = 0; line < layout->metrics.lineCount; line++) {
        FLOAT width = layout_get_line_width(layout, erun, inrun, line);
        FLOAT shift = layout_get_centered_shift(layout, skiptransform, width, det);

        if (is_rtl)
            shift *= -1.0f;

        while (erun && erun->line == line) {
            erun->align_dx = shift;
            erun = layout_get_next_erun(layout, erun);
        }

        while (inrun && inrun->line == line) {
            inrun->align_dx = shift;
            inrun = layout_get_next_inline_run(layout, inrun);
        }
    }

    layout->metrics.left = (layout->metrics.layoutWidth - layout->metrics.width) / 2.0f;
}

static void layout_apply_text_alignment(struct dwrite_textlayout *layout)
{
    switch (layout->format.textalignment)
    {
    case DWRITE_TEXT_ALIGNMENT_LEADING:
        layout_apply_leading_alignment(layout);
        break;
    case DWRITE_TEXT_ALIGNMENT_TRAILING:
        layout_apply_trailing_alignment(layout);
        break;
    case DWRITE_TEXT_ALIGNMENT_CENTER:
        layout_apply_centered_alignment(layout);
        break;
    case DWRITE_TEXT_ALIGNMENT_JUSTIFIED:
        FIXME("alignment %d not implemented\n", layout->format.textalignment);
        break;
    default:
        ;
    }
}

static void layout_apply_par_alignment(struct dwrite_textlayout *layout)
{
    struct layout_effective_inline *inrun;
    struct layout_effective_run *erun;
    FLOAT origin_y = 0.0f;
    UINT32 line;

    /* alignment mode defines origin, after that all run origins are updated
       the same way */

    switch (layout->format.paralign)
    {
    case DWRITE_PARAGRAPH_ALIGNMENT_NEAR:
        origin_y = 0.0f;
        break;
    case DWRITE_PARAGRAPH_ALIGNMENT_FAR:
        origin_y = layout->metrics.layoutHeight - layout->metrics.height;
        break;
    case DWRITE_PARAGRAPH_ALIGNMENT_CENTER:
        origin_y = (layout->metrics.layoutHeight - layout->metrics.height) / 2.0f;
        break;
    default:
        ;
    }

    layout->metrics.top = origin_y;

    erun = layout_get_next_erun(layout, NULL);
    inrun = layout_get_next_inline_run(layout, NULL);
    for (line = 0; line < layout->metrics.lineCount; line++)
    {
        float pos_y = origin_y + layout->lines[line].metrics.baseline;

        while (erun && erun->line == line) {
            erun->origin.y = pos_y;
            erun = layout_get_next_erun(layout, erun);
        }

        while (inrun && inrun->line == line) {
            inrun->origin.y = pos_y - inrun->baseline;
            inrun = layout_get_next_inline_run(layout, inrun);
        }

        origin_y += layout->lines[line].metrics.height;
    }
}

struct layout_underline_splitting_params {
    const WCHAR *locale; /* points to range data, no additional allocation */
    IUnknown *effect;    /* does not hold another reference */
};

static void init_u_splitting_params_from_erun(const struct layout_effective_run *erun,
    struct layout_underline_splitting_params *params)
{
    params->locale = erun->run->u.regular.descr.localeName;
    params->effect = erun->effect;
}

static BOOL is_same_u_splitting(const struct layout_underline_splitting_params *left,
        const struct layout_underline_splitting_params *right)
{
    return left->effect == right->effect && !wcsicmp(left->locale, right->locale);
}

static HRESULT layout_add_underline(struct dwrite_textlayout *layout, struct layout_effective_run *first,
    struct layout_effective_run *last)
{
    FLOAT thickness, offset, runheight;
    struct layout_effective_run *cur;
    DWRITE_FONT_METRICS metrics;

    if (first == layout_get_prev_erun(layout, last)) {
        layout_get_erun_font_metrics(layout, first, &metrics);
        thickness = SCALE_FONT_METRIC(metrics.underlineThickness, first->run->u.regular.run.fontEmSize, &metrics);
        offset = SCALE_FONT_METRIC(metrics.underlinePosition, first->run->u.regular.run.fontEmSize, &metrics);
        runheight = SCALE_FONT_METRIC(metrics.capHeight, first->run->u.regular.run.fontEmSize, &metrics);
    }
    else {
        FLOAT width = 0.0f;

        /* Single underline is added for consecutive underlined runs. In this case underline parameters are
           calculated as weighted average, where run width acts as a weight. */
        thickness = offset = runheight = 0.0f;
        cur = first;
        do {
            layout_get_erun_font_metrics(layout, cur, &metrics);

            thickness += SCALE_FONT_METRIC(metrics.underlineThickness, cur->run->u.regular.run.fontEmSize, &metrics) * cur->width;
            offset += SCALE_FONT_METRIC(metrics.underlinePosition, cur->run->u.regular.run.fontEmSize, &metrics) * cur->width;
            runheight = max(SCALE_FONT_METRIC(metrics.capHeight, cur->run->u.regular.run.fontEmSize, &metrics), runheight);
            width += cur->width;

            cur = layout_get_next_erun(layout, cur);
        } while (cur != last);

        thickness /= width;
        offset /= width;
    }

    cur = first;
    do {
        struct layout_underline_splitting_params params, prev_params;
        struct layout_effective_run *next, *w;
        struct layout_underline *u;

        init_u_splitting_params_from_erun(cur, &prev_params);
        while ((next = layout_get_next_erun(layout, cur)) != last) {
            init_u_splitting_params_from_erun(next, &params);
            if (!is_same_u_splitting(&prev_params, &params))
                break;
            cur = next;
        }

        if (!(u = malloc(sizeof(*u))))
            return E_OUTOFMEMORY;

        w = cur;
        u->u.width = 0.0f;
        while (w != next) {
            u->u.width += w->width;
            w = layout_get_next_erun(layout, w);
        }

        u->u.thickness = thickness;
        /* Font metrics convention is to have it negative when below baseline, for rendering
           however Y grows from baseline down for horizontal baseline. */
        u->u.offset = -offset;
        u->u.runHeight = runheight;
        u->u.readingDirection = is_run_rtl(cur) ? DWRITE_READING_DIRECTION_RIGHT_TO_LEFT :
            DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
        u->u.flowDirection = layout->format.flow;
        u->u.localeName = cur->run->u.regular.descr.localeName;
        u->u.measuringMode = layout->measuringmode;
        u->run = cur;
        list_add_tail(&layout->underlines, &u->entry);

        cur = next;
    } while (cur != last);

    return S_OK;
}

static inline struct regular_layout_run * layout_get_last_run(const struct dwrite_textlayout *layout)
{
    struct layout_run *r;
    struct list *e;

    if (!(e = list_tail(&layout->runs))) return NULL;
    r = LIST_ENTRY(e, struct layout_run, entry);
    if (r->kind != LAYOUT_RUN_REGULAR) return NULL;
    return &r->u.regular;
}

/* Adds a dummy line if:
   - there's no text, metrics come from first range in this case;
   - last ended with a mandatory break, metrics come from last text position.
*/
static HRESULT layout_set_dummy_line_metrics(struct dwrite_textlayout *layout)
{
    DWRITE_LINE_METRICS1 metrics = { 0 };
    DWRITE_FONT_METRICS fontmetrics;
    struct regular_layout_run *run;
    IDWriteFontFace *fontface;
    float size;
    HRESULT hr;

    if (layout->cluster_count && !layout->clustermetrics[layout->cluster_count - 1].isNewline)
        return S_OK;

    if (!layout->cluster_count)
    {
        if (FAILED(hr = layout_run_get_last_resort_font(layout, get_layout_range_by_pos(layout, 0), &fontface, &size)))
            return hr;
    }
    else if (!(run = layout_get_last_run(layout)))
    {
        return S_OK;
    }
    else
    {
        fontface = run->run.fontFace;
        IDWriteFontFace_AddRef(fontface);
        size = run->run.fontEmSize;
    }

    layout_get_font_metrics(layout, fontface, size, &fontmetrics);
    layout_get_font_height(size, &fontmetrics, &metrics.baseline, &metrics.height);
    IDWriteFontFace_Release(fontface);

    return layout_set_line_metrics(layout, &metrics);
}

static void layout_add_line(struct dwrite_textlayout *layout, UINT32 first_cluster, UINT32 last_cluster,
        UINT32 *textpos)
{
    BOOL is_rtl = layout->format.readingdir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
    struct layout_final_splitting_params params, prev_params;
    DWRITE_INLINE_OBJECT_METRICS sign_metrics = { 0 };
    UINT32 line = layout->metrics.lineCount, i;
    DWRITE_LINE_METRICS1 metrics = { 0 };
    UINT32 index, start, pos = *textpos;
    FLOAT descent, trailingspacewidth;
    BOOL append_trimming_run = FALSE;
    const struct layout_run *run;
    float width = 0.0f, origin_x;
    HRESULT hr;

    /* Take a look at clusters we got for this line in reverse order to set trailing properties for current line */
    for (index = last_cluster, trailingspacewidth = 0.0f; index >= first_cluster; index--) {
        DWRITE_CLUSTER_METRICS *cluster = &layout->clustermetrics[index];
        struct layout_cluster *lc = &layout->clusters[index];
        WCHAR ch;

        /* This also filters out clusters added from inline objects, those are never
           treated as a white space. */
        if (!cluster->isWhitespace)
            break;

        /* Every isNewline cluster is also isWhitespace, but not every
           newline character cluster has isNewline set, so go back to original string. */
        ch = lc->run->u.regular.descr.string[lc->position];
        if (cluster->length == 1 && lb_is_newline_char(ch))
            metrics.newlineLength += cluster->length;

        metrics.trailingWhitespaceLength += cluster->length;
        trailingspacewidth += cluster->width;

        if (index == 0)
            break;
    }

    /* Line metrics length includes trailing whitespace length too */
    for (i = first_cluster; i <= last_cluster; i++)
        metrics.length += layout->clustermetrics[i].length;

    /* Ignore trailing whitespaces */
    while (last_cluster > first_cluster) {
        if (!layout->clustermetrics[last_cluster].isWhitespace)
            break;

        last_cluster--;
    }

    /* Does not include trailing space width */
    if (!layout->clustermetrics[last_cluster].isWhitespace)
        width = get_cluster_range_width(layout, first_cluster, last_cluster + 1);

    /* Append trimming run if necessary */
    if (width > layout->metrics.layoutWidth && layout->format.trimmingsign != NULL &&
            layout->format.trimming.granularity != DWRITE_TRIMMING_GRANULARITY_NONE) {
        FLOAT trimmed_width = width;

        hr = IDWriteInlineObject_GetMetrics(layout->format.trimmingsign, &sign_metrics);
        if (SUCCEEDED(hr)) {
            while (last_cluster > first_cluster) {
                if (trimmed_width + sign_metrics.width <= layout->metrics.layoutWidth)
                    break;
                if (layout->format.trimming.granularity == DWRITE_TRIMMING_GRANULARITY_CHARACTER)
                    trimmed_width -= layout->clustermetrics[last_cluster--].width;
                else {
                    while (last_cluster > first_cluster) {
                        trimmed_width -= layout->clustermetrics[last_cluster].width;
                        if (layout->clustermetrics[last_cluster--].canWrapLineAfter)
                            break;
                    }
                }
            }
            append_trimming_run = TRUE;
        }
        else
            WARN("Failed to get trimming sign metrics, lines won't be trimmed, hr %#lx.\n", hr);

        width = trimmed_width + sign_metrics.width;
    }

    layout_splitting_params_from_pos(layout, pos, &params);
    prev_params = params;
    run = layout->clusters[first_cluster].run;

    /* Form runs from a range of clusters; this is what will be reported with DrawGlyphRun() */
    origin_x = is_rtl ? layout->metrics.layoutWidth : 0.0f;
    for (start = first_cluster, i = first_cluster; i <= last_cluster; i++) {
        layout_splitting_params_from_pos(layout, pos, &params);

        if (run != layout->clusters[i].run || !is_same_splitting_params(&prev_params, &params)) {
            hr = layout_add_effective_run(layout, run, start, i - start, line, origin_x, &prev_params);
            if (FAILED(hr))
                return;

            origin_x += is_rtl ? -get_cluster_range_width(layout, start, i) :
                get_cluster_range_width(layout, start, i);
            run = layout->clusters[i].run;
            start = i;
        }

        prev_params = params;
        pos += layout->clustermetrics[i].length;
    }

    /* Final run from what's left from cluster range */
    hr = layout_add_effective_run(layout, run, start, i - start, line, origin_x, &prev_params);
    if (FAILED(hr))
        return;

    if (get_cluster_range_width(layout, start, i) + sign_metrics.width > layout->metrics.layoutWidth)
        append_trimming_run = FALSE;

    if (append_trimming_run) {
        struct layout_effective_inline *trimming_sign;

        if (!(trimming_sign = calloc(1, sizeof(*trimming_sign))))
            return;

        trimming_sign->object = layout->format.trimmingsign;
        trimming_sign->width = sign_metrics.width;
        origin_x += is_rtl ? -get_cluster_range_width(layout, start, i) : get_cluster_range_width(layout, start, i);
        trimming_sign->origin.x = is_rtl ? origin_x - trimming_sign->width : origin_x;
        trimming_sign->origin.y = 0.0f; /* set after line is built */
        trimming_sign->align_dx = 0.0f;
        trimming_sign->baseline = sign_metrics.baseline;

        trimming_sign->is_sideways = FALSE;
        trimming_sign->is_rtl = FALSE;
        trimming_sign->line = line;

        trimming_sign->effect = layout_get_effect_from_pos(layout, layout->clusters[i].position +
                layout->clusters[i].run->start_position);

        list_add_tail(&layout->inlineobjects, &trimming_sign->entry);
    }

    /* Look for max baseline and descent for this line */
    for (index = first_cluster, metrics.baseline = 0.0f, descent = 0.0f; index <= last_cluster; index++) {
        const struct layout_run *cur = layout->clusters[index].run;
        FLOAT cur_descent = cur->height - cur->baseline;

        if (cur->baseline > metrics.baseline)
            metrics.baseline = cur->baseline;
        if (cur_descent > descent)
            descent = cur_descent;
    }

    layout->metrics.width = max(width, layout->metrics.width);
    layout->metrics.widthIncludingTrailingWhitespace = max(width + trailingspacewidth,
        layout->metrics.widthIncludingTrailingWhitespace);

    metrics.height = descent + metrics.baseline;
    metrics.isTrimmed = append_trimming_run || width > layout->metrics.layoutWidth;
    layout_set_line_metrics(layout, &metrics);

    *textpos += metrics.length;
}

static void layout_set_line_positions(struct dwrite_textlayout *layout)
{
    struct layout_effective_inline *inrun;
    struct layout_effective_run *erun;
    FLOAT origin_y;
    UINT32 line;

    /* Now all line info is here, update effective runs positions in flow direction */
    erun = layout_get_next_erun(layout, NULL);
    inrun = layout_get_next_inline_run(layout, NULL);

    for (line = 0, origin_y = 0.0f; line < layout->metrics.lineCount; line++)
    {
        float pos_y = origin_y + layout->lines[line].metrics.baseline;

        /* For all runs on this line */
        while (erun && erun->line == line) {
            erun->origin.y = pos_y;
            erun = layout_get_next_erun(layout, erun);
        }

        /* Same for inline runs */
        while (inrun && inrun->line == line) {
            inrun->origin.y = pos_y - inrun->baseline;
            inrun = layout_get_next_inline_run(layout, inrun);
        }

        origin_y += layout->lines[line].metrics.height;
    }

    layout->metrics.height = origin_y;

    /* Initial paragraph alignment is always near */
    if (layout->format.paralign != DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
        layout_apply_par_alignment(layout);
}

static BOOL layout_can_wrap_after(const struct dwrite_textlayout *layout, UINT32 cluster)
{
    if (layout->format.wrapping == DWRITE_WORD_WRAPPING_CHARACTER)
        return TRUE;

    return layout->clustermetrics[cluster].canWrapLineAfter;
}

static HRESULT layout_compute_effective_runs(struct dwrite_textlayout *layout)
{
    BOOL is_rtl = layout->format.readingdir == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
    struct layout_effective_run *erun, *first_underlined;
    UINT32 i, start, textpos, last_breaking_point;
    DWRITE_LINE_METRICS1 metrics;
    FLOAT width;
    UINT32 line;
    HRESULT hr;

    if (!(layout->recompute & RECOMPUTE_LINES))
        return S_OK;

    free_layout_eruns(layout);

    hr = layout_compute(layout);
    if (FAILED(hr))
        return hr;

    layout->metrics.lineCount = 0;
    memset(&metrics, 0, sizeof(metrics));

    layout->metrics.height = 0.0f;
    layout->metrics.width = 0.0f;
    layout->metrics.widthIncludingTrailingWhitespace = 0.0f;

    last_breaking_point = ~0u;

    for (i = 0, start = 0, width = 0.0f, textpos = 0; i < layout->cluster_count; i++) {
        BOOL overflow = FALSE;

        while (i < layout->cluster_count && !layout->clustermetrics[i].isNewline) {
            /* Check for overflow */
            overflow = ((width + layout->clustermetrics[i].width > layout->metrics.layoutWidth) &&
                    (layout->format.wrapping != DWRITE_WORD_WRAPPING_NO_WRAP));
            if (overflow)
                break;

            if (layout_can_wrap_after(layout, i))
                last_breaking_point = i;
            width += layout->clustermetrics[i].width;
            i++;
        }
        i = min(i, layout->cluster_count - 1);

        /* Ignore if overflown on whitespace */
        if (overflow && !(layout->clustermetrics[i].isWhitespace && layout_can_wrap_after(layout, i))) {
            /* Use most recently found breaking point */
            if (last_breaking_point != ~0u) {
                i = last_breaking_point;
                last_breaking_point = ~0u;
            }
            else {
                /* Otherwise proceed forward to next newline or breaking point */
                for (; i < layout->cluster_count; i++)
                    if (layout_can_wrap_after(layout, i) || layout->clustermetrics[i].isNewline)
                        break;
            }
        }
        i = min(i, layout->cluster_count - 1);

        layout_add_line(layout, start, i, &textpos);
        start = i + 1;
        width = 0.0f;
    }

    if (FAILED(hr = layout_set_dummy_line_metrics(layout)))
        return hr;

    layout->metrics.left = is_rtl ? layout->metrics.layoutWidth - layout->metrics.width : 0.0f;
    layout->metrics.top = 0.0f;
    layout->metrics.maxBidiReorderingDepth = 1; /* FIXME */

    /* Add explicit underlined runs */
    erun = layout_get_next_erun(layout, NULL);
    first_underlined = erun && erun->underlined ? erun : NULL;
    for (line = 0; line < layout->metrics.lineCount; line++) {
        while (erun && erun->line == line) {
            erun = layout_get_next_erun(layout, erun);

            if (first_underlined && (!erun || !erun->underlined)) {
                layout_add_underline(layout, first_underlined, erun);
                first_underlined = NULL;
            }
            else if (!first_underlined && erun && erun->underlined)
                first_underlined = erun;
        }
    }

    /* Position runs in flow direction */
    layout_set_line_positions(layout);

    /* Initial alignment is always leading */
    if (layout->format.textalignment != DWRITE_TEXT_ALIGNMENT_LEADING)
        layout_apply_text_alignment(layout);

    layout->recompute &= ~RECOMPUTE_LINES;
    return hr;
}

static BOOL is_same_layout_attrvalue(struct layout_range_header const *h, enum layout_range_attr_kind attr,
        struct layout_range_attr_value *value)
{
    struct layout_range_spacing const *range_spacing = (struct layout_range_spacing*)h;
    struct layout_range_iface const *range_iface = (struct layout_range_iface*)h;
    struct layout_range_bool const *range_bool = (struct layout_range_bool*)h;
    struct layout_range const *range = (struct layout_range*)h;

    switch (attr) {
    case LAYOUT_RANGE_ATTR_WEIGHT:
        return range->weight == value->u.weight;
    case LAYOUT_RANGE_ATTR_STYLE:
        return range->style == value->u.style;
    case LAYOUT_RANGE_ATTR_STRETCH:
        return range->stretch == value->u.stretch;
    case LAYOUT_RANGE_ATTR_FONTSIZE:
        return range->fontsize == value->u.fontsize;
    case LAYOUT_RANGE_ATTR_INLINE:
        return range->object == value->u.object;
    case LAYOUT_RANGE_ATTR_EFFECT:
        return range_iface->iface == value->u.effect;
    case LAYOUT_RANGE_ATTR_UNDERLINE:
        return range_bool->value == value->u.underline;
    case LAYOUT_RANGE_ATTR_STRIKETHROUGH:
        return range_bool->value == value->u.strikethrough;
    case LAYOUT_RANGE_ATTR_PAIR_KERNING:
        return range->pair_kerning == value->u.pair_kerning;
    case LAYOUT_RANGE_ATTR_FONTCOLL:
        return range->collection == value->u.collection;
    case LAYOUT_RANGE_ATTR_LOCALE:
        return !wcsicmp(range->locale, value->u.locale);
    case LAYOUT_RANGE_ATTR_FONTFAMILY:
        return !wcscmp(range->fontfamily, value->u.fontfamily);
    case LAYOUT_RANGE_ATTR_SPACING:
        return range_spacing->leading == value->u.spacing.leading &&
               range_spacing->trailing == value->u.spacing.trailing &&
               range_spacing->min_advance == value->u.spacing.min_advance;
    case LAYOUT_RANGE_ATTR_TYPOGRAPHY:
        return range_iface->iface == (IUnknown*)value->u.typography;
    default:
        ;
    }

    return FALSE;
}

static inline BOOL is_same_layout_attributes(struct layout_range_header const *hleft, struct layout_range_header const *hright)
{
    switch (hleft->kind)
    {
    case LAYOUT_RANGE_REGULAR:
    {
        struct layout_range const *left = (struct layout_range const*)hleft;
        struct layout_range const *right = (struct layout_range const*)hright;
        return left->weight == right->weight &&
               left->style  == right->style &&
               left->stretch == right->stretch &&
               left->fontsize == right->fontsize &&
               left->object == right->object &&
               left->pair_kerning == right->pair_kerning &&
               left->collection == right->collection &&
              !wcsicmp(left->locale, right->locale) &&
              !wcscmp(left->fontfamily, right->fontfamily);
    }
    case LAYOUT_RANGE_UNDERLINE:
    case LAYOUT_RANGE_STRIKETHROUGH:
    {
        struct layout_range_bool const *left = (struct layout_range_bool const*)hleft;
        struct layout_range_bool const *right = (struct layout_range_bool const*)hright;
        return left->value == right->value;
    }
    case LAYOUT_RANGE_EFFECT:
    case LAYOUT_RANGE_TYPOGRAPHY:
    {
        struct layout_range_iface const *left = (struct layout_range_iface const*)hleft;
        struct layout_range_iface const *right = (struct layout_range_iface const*)hright;
        return left->iface == right->iface;
    }
    case LAYOUT_RANGE_SPACING:
    {
        struct layout_range_spacing const *left = (struct layout_range_spacing const*)hleft;
        struct layout_range_spacing const *right = (struct layout_range_spacing const*)hright;
        return left->leading == right->leading &&
               left->trailing == right->trailing &&
               left->min_advance == right->min_advance;
    }
    default:
        FIXME("unknown range kind %d\n", hleft->kind);
        return FALSE;
    }
}

static inline BOOL is_same_text_range(const DWRITE_TEXT_RANGE *left, const DWRITE_TEXT_RANGE *right)
{
    return left->startPosition == right->startPosition && left->length == right->length;
}

/* Allocates range and inits it with default values from text format. */
static struct layout_range_header *alloc_layout_range(struct dwrite_textlayout *layout, const DWRITE_TEXT_RANGE *r,
    enum layout_range_kind kind)
{
    struct layout_range_header *h;

    switch (kind)
    {
    case LAYOUT_RANGE_REGULAR:
    {
        struct layout_range *range;

        if (!(range = calloc(1, sizeof(*range))))
            return NULL;

        range->weight = layout->format.weight;
        range->style  = layout->format.style;
        range->stretch = layout->format.stretch;
        range->fontsize = layout->format.fontsize;

        range->fontfamily = wcsdup(layout->format.family_name);
        if (!range->fontfamily)
        {
            free(range);
            return NULL;
        }

        range->collection = layout->format.collection;
        if (range->collection)
            IDWriteFontCollection_AddRef(range->collection);
        wcscpy(range->locale, layout->format.locale);

        h = &range->h;
        break;
    }
    case LAYOUT_RANGE_UNDERLINE:
    case LAYOUT_RANGE_STRIKETHROUGH:
    {
        struct layout_range_bool *range;

        if (!(range = calloc(1, sizeof(*range))))
            return NULL;

        h = &range->h;
        break;
    }
    case LAYOUT_RANGE_EFFECT:
    case LAYOUT_RANGE_TYPOGRAPHY:
    {
        struct layout_range_iface *range;

        if (!(range = calloc(1, sizeof(*range))))
            return NULL;

        h = &range->h;
        break;
    }
    case LAYOUT_RANGE_SPACING:
    {
        struct layout_range_spacing *range;

        if (!(range = calloc(1, sizeof(*range))))
            return NULL;

        h = &range->h;
        break;
    }
    default:
        FIXME("unknown range kind %d\n", kind);
        return NULL;
    }

    h->kind = kind;
    h->range = *r;
    return h;
}

static struct layout_range_header *alloc_layout_range_from(struct layout_range_header *h, const DWRITE_TEXT_RANGE *r)
{
    struct layout_range_header *ret;

    switch (h->kind)
    {
    case LAYOUT_RANGE_REGULAR:
    {
        struct layout_range *from = (struct layout_range *)h, *range;

        if (!(range = malloc(sizeof(*range))))
            return NULL;

        *range = *from;
        range->fontfamily = wcsdup(from->fontfamily);
        if (!range->fontfamily)
        {
            free(range);
            return NULL;
        }

        /* update refcounts */
        if (range->object)
            IDWriteInlineObject_AddRef(range->object);
        if (range->collection)
            IDWriteFontCollection_AddRef(range->collection);
        ret = &range->h;
        break;
    }
    case LAYOUT_RANGE_UNDERLINE:
    case LAYOUT_RANGE_STRIKETHROUGH:
    {
        struct layout_range_bool *strike = malloc(sizeof(*strike));
        if (!strike) return NULL;

        *strike = *(struct layout_range_bool*)h;
        ret = &strike->h;
        break;
    }
    case LAYOUT_RANGE_EFFECT:
    case LAYOUT_RANGE_TYPOGRAPHY:
    {
        struct layout_range_iface *effect = malloc(sizeof(*effect));
        if (!effect) return NULL;

        *effect = *(struct layout_range_iface*)h;
        if (effect->iface)
            IUnknown_AddRef(effect->iface);
        ret = &effect->h;
        break;
    }
    case LAYOUT_RANGE_SPACING:
    {
        struct layout_range_spacing *spacing = malloc(sizeof(*spacing));
        if (!spacing) return NULL;

        *spacing = *(struct layout_range_spacing*)h;
        ret = &spacing->h;
        break;
    }
    default:
        FIXME("unknown range kind %d\n", h->kind);
        return NULL;
    }

    ret->range = *r;
    return ret;
}

static void free_layout_range(struct layout_range_header *h)
{
    if (!h)
        return;

    switch (h->kind)
    {
    case LAYOUT_RANGE_REGULAR:
    {
        struct layout_range *range = (struct layout_range*)h;

        if (range->object)
            IDWriteInlineObject_Release(range->object);
        if (range->collection)
            IDWriteFontCollection_Release(range->collection);
        free(range->fontfamily);
        break;
    }
    case LAYOUT_RANGE_EFFECT:
    case LAYOUT_RANGE_TYPOGRAPHY:
    {
        struct layout_range_iface *range = (struct layout_range_iface*)h;
        if (range->iface)
            IUnknown_Release(range->iface);
        break;
    }
    default:
        ;
    }

    free(h);
}

static void free_layout_ranges_list(struct dwrite_textlayout *layout)
{
    struct layout_range_header *cur, *cur2;

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->ranges, struct layout_range_header, entry) {
        list_remove(&cur->entry);
        free_layout_range(cur);
    }

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->underline_ranges, struct layout_range_header, entry) {
        list_remove(&cur->entry);
        free_layout_range(cur);
    }

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->strike_ranges, struct layout_range_header, entry) {
        list_remove(&cur->entry);
        free_layout_range(cur);
    }

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->effects, struct layout_range_header, entry) {
        list_remove(&cur->entry);
        free_layout_range(cur);
    }

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->spacing, struct layout_range_header, entry) {
        list_remove(&cur->entry);
        free_layout_range(cur);
    }

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->typographies, struct layout_range_header, entry) {
        list_remove(&cur->entry);
        free_layout_range(cur);
    }
}

static struct layout_range_header *find_outer_range(struct list *ranges, const DWRITE_TEXT_RANGE *range)
{
    struct layout_range_header *cur;

    LIST_FOR_EACH_ENTRY(cur, ranges, struct layout_range_header, entry) {

        if (cur->range.startPosition > range->startPosition)
            return NULL;

        if ((cur->range.startPosition + cur->range.length < range->startPosition + range->length) &&
            (range->startPosition < cur->range.startPosition + cur->range.length))
            return NULL;
        if (cur->range.startPosition + cur->range.length >= range->startPosition + range->length)
            return cur;
    }

    return NULL;
}

static inline BOOL set_layout_range_iface_attr(IUnknown **dest, IUnknown *value)
{
    if (*dest == value) return FALSE;

    if (*dest)
        IUnknown_Release(*dest);
    *dest = value;
    if (*dest)
        IUnknown_AddRef(*dest);

    return TRUE;
}

static BOOL set_layout_range_attrval(struct layout_range_header *h, enum layout_range_attr_kind attr, struct layout_range_attr_value *value)
{
    struct layout_range_spacing *dest_spacing = (struct layout_range_spacing*)h;
    struct layout_range_iface *dest_iface = (struct layout_range_iface*)h;
    struct layout_range_bool *dest_bool = (struct layout_range_bool*)h;
    struct layout_range *dest = (struct layout_range*)h;

    BOOL changed = FALSE;

    switch (attr) {
    case LAYOUT_RANGE_ATTR_WEIGHT:
        changed = dest->weight != value->u.weight;
        dest->weight = value->u.weight;
        break;
    case LAYOUT_RANGE_ATTR_STYLE:
        changed = dest->style != value->u.style;
        dest->style = value->u.style;
        break;
    case LAYOUT_RANGE_ATTR_STRETCH:
        changed = dest->stretch != value->u.stretch;
        dest->stretch = value->u.stretch;
        break;
    case LAYOUT_RANGE_ATTR_FONTSIZE:
        changed = dest->fontsize != value->u.fontsize;
        dest->fontsize = value->u.fontsize;
        break;
    case LAYOUT_RANGE_ATTR_INLINE:
        changed = set_layout_range_iface_attr((IUnknown**)&dest->object, (IUnknown*)value->u.object);
        break;
    case LAYOUT_RANGE_ATTR_EFFECT:
        changed = set_layout_range_iface_attr(&dest_iface->iface, value->u.effect);
        break;
    case LAYOUT_RANGE_ATTR_UNDERLINE:
        changed = dest_bool->value != value->u.underline;
        dest_bool->value = value->u.underline;
        break;
    case LAYOUT_RANGE_ATTR_STRIKETHROUGH:
        changed = dest_bool->value != value->u.strikethrough;
        dest_bool->value = value->u.strikethrough;
        break;
    case LAYOUT_RANGE_ATTR_PAIR_KERNING:
        changed = dest->pair_kerning != value->u.pair_kerning;
        dest->pair_kerning = value->u.pair_kerning;
        break;
    case LAYOUT_RANGE_ATTR_FONTCOLL:
        changed = set_layout_range_iface_attr((IUnknown**)&dest->collection, (IUnknown*)value->u.collection);
        break;
    case LAYOUT_RANGE_ATTR_LOCALE:
        changed = !!wcsicmp(dest->locale, value->u.locale);
        if (changed)
        {
            wcscpy(dest->locale, value->u.locale);
            wcslwr(dest->locale);
        }
        break;
    case LAYOUT_RANGE_ATTR_FONTFAMILY:
        changed = !!wcscmp(dest->fontfamily, value->u.fontfamily);
        if (changed)
        {
            free(dest->fontfamily);
            dest->fontfamily = wcsdup(value->u.fontfamily);
        }
        break;
    case LAYOUT_RANGE_ATTR_SPACING:
        changed = dest_spacing->leading != value->u.spacing.leading ||
            dest_spacing->trailing != value->u.spacing.trailing ||
            dest_spacing->min_advance != value->u.spacing.min_advance;
        dest_spacing->leading = value->u.spacing.leading;
        dest_spacing->trailing = value->u.spacing.trailing;
        dest_spacing->min_advance = value->u.spacing.min_advance;
        break;
    case LAYOUT_RANGE_ATTR_TYPOGRAPHY:
        changed = set_layout_range_iface_attr(&dest_iface->iface, (IUnknown*)value->u.typography);
        break;
    default:
        ;
    }

    return changed;
}

static inline BOOL is_in_layout_range(const DWRITE_TEXT_RANGE *outer, const DWRITE_TEXT_RANGE *inner)
{
    return (inner->startPosition >= outer->startPosition) &&
           (inner->startPosition + inner->length <= outer->startPosition + outer->length);
}

static inline HRESULT return_range(const struct layout_range_header *h, DWRITE_TEXT_RANGE *r)
{
    if (r) *r = h->range;
    return S_OK;
}

/* Sets attribute value for given range, does all needed splitting/merging of existing ranges. */
static HRESULT set_layout_range_attr(struct dwrite_textlayout *layout, enum layout_range_attr_kind attr, struct layout_range_attr_value *value)
{
    struct layout_range_header *cur, *right, *left, *outer;
    BOOL changed = FALSE;
    struct list *ranges;
    DWRITE_TEXT_RANGE r;

    /* ignore zero length ranges */
    if (value->range.length == 0)
        return S_OK;

    if (~0u - value->range.startPosition < value->range.length)
        return E_INVALIDARG;

    /* select from ranges lists */
    switch (attr)
    {
    case LAYOUT_RANGE_ATTR_WEIGHT:
    case LAYOUT_RANGE_ATTR_STYLE:
    case LAYOUT_RANGE_ATTR_STRETCH:
    case LAYOUT_RANGE_ATTR_FONTSIZE:
    case LAYOUT_RANGE_ATTR_INLINE:
    case LAYOUT_RANGE_ATTR_PAIR_KERNING:
    case LAYOUT_RANGE_ATTR_FONTCOLL:
    case LAYOUT_RANGE_ATTR_LOCALE:
    case LAYOUT_RANGE_ATTR_FONTFAMILY:
        ranges = &layout->ranges;
        break;
    case LAYOUT_RANGE_ATTR_UNDERLINE:
        ranges = &layout->underline_ranges;
        break;
    case LAYOUT_RANGE_ATTR_STRIKETHROUGH:
        ranges = &layout->strike_ranges;
        break;
    case LAYOUT_RANGE_ATTR_EFFECT:
        ranges = &layout->effects;
        break;
    case LAYOUT_RANGE_ATTR_SPACING:
        ranges = &layout->spacing;
        break;
    case LAYOUT_RANGE_ATTR_TYPOGRAPHY:
        ranges = &layout->typographies;
        break;
    default:
        FIXME("unknown attr kind %d\n", attr);
        return E_FAIL;
    }

    /* If new range is completely within existing range, split existing range in two */
    if ((outer = find_outer_range(ranges, &value->range))) {

        /* no need to add same range */
        if (is_same_layout_attrvalue(outer, attr, value))
            return S_OK;

        /* for matching range bounds just replace data */
        if (is_same_text_range(&outer->range, &value->range)) {
            changed = set_layout_range_attrval(outer, attr, value);
            goto done;
        }

        /* add new range to the left */
        if (value->range.startPosition == outer->range.startPosition) {
            left = alloc_layout_range_from(outer, &value->range);
            if (!left) return E_OUTOFMEMORY;

            changed = set_layout_range_attrval(left, attr, value);
            list_add_before(&outer->entry, &left->entry);
            outer->range.startPosition += value->range.length;
            outer->range.length -= value->range.length;
            goto done;
        }

        /* add new range to the right */
        if (value->range.startPosition + value->range.length == outer->range.startPosition + outer->range.length) {
            right = alloc_layout_range_from(outer, &value->range);
            if (!right) return E_OUTOFMEMORY;

            changed = set_layout_range_attrval(right, attr, value);
            list_add_after(&outer->entry, &right->entry);
            outer->range.length -= value->range.length;
            goto done;
        }

        r.startPosition = value->range.startPosition + value->range.length;
        r.length = outer->range.length + outer->range.startPosition - r.startPosition;

        /* right part */
        right = alloc_layout_range_from(outer, &r);
        /* new range in the middle */
        cur = alloc_layout_range_from(outer, &value->range);
        if (!right || !cur) {
            free_layout_range(right);
            free_layout_range(cur);
            return E_OUTOFMEMORY;
        }

        /* reuse container range as a left part */
        outer->range.length = value->range.startPosition - outer->range.startPosition;

        /* new part */
        set_layout_range_attrval(cur, attr, value);

        list_add_after(&outer->entry, &cur->entry);
        list_add_after(&cur->entry, &right->entry);

        layout->recompute = RECOMPUTE_EVERYTHING;
        return S_OK;
    }

    /* Now it's only possible that given range contains some existing ranges, fully or partially.
       Update all of them. */
    left = get_layout_range_header_by_pos(ranges, value->range.startPosition);
    if (left->range.startPosition == value->range.startPosition)
        changed = set_layout_range_attrval(left, attr, value);
    else /* need to split */ {
        r.startPosition = value->range.startPosition;
        r.length = left->range.length - value->range.startPosition + left->range.startPosition;
        left->range.length -= r.length;
        cur = alloc_layout_range_from(left, &r);
        changed = set_layout_range_attrval(cur, attr, value);
        list_add_after(&left->entry, &cur->entry);
    }
    cur = LIST_ENTRY(list_next(ranges, &left->entry), struct layout_range_header, entry);

    /* for all existing ranges covered by new one update value */
    while (cur && is_in_layout_range(&value->range, &cur->range)) {
        changed |= set_layout_range_attrval(cur, attr, value);
        cur = LIST_ENTRY(list_next(ranges, &cur->entry), struct layout_range_header, entry);
    }

    /* it's possible rightmost range intersects */
    if (cur && (cur->range.startPosition < value->range.startPosition + value->range.length)) {
        r.startPosition = cur->range.startPosition;
        r.length = value->range.startPosition + value->range.length - cur->range.startPosition;
        left = alloc_layout_range_from(cur, &r);
        changed |= set_layout_range_attrval(left, attr, value);
        cur->range.startPosition += left->range.length;
        cur->range.length -= left->range.length;
        list_add_before(&cur->entry, &left->entry);
    }

done:
    if (changed) {
        struct list *next, *i;

        layout->recompute = RECOMPUTE_EVERYTHING;
        i = list_head(ranges);
        while ((next = list_next(ranges, i))) {
            struct layout_range_header *next_range = LIST_ENTRY(next, struct layout_range_header, entry);

            cur = LIST_ENTRY(i, struct layout_range_header, entry);
            if (is_same_layout_attributes(cur, next_range)) {
                /* remove similar range */
                cur->range.length += next_range->range.length;
                list_remove(next);
                free_layout_range(next_range);
            }
            else
                i = list_next(ranges, i);
        }
    }

    return S_OK;
}

static inline const WCHAR *get_string_attribute_ptr(const struct layout_range *range, enum layout_range_attr_kind kind)
{
    const WCHAR *str;

    switch (kind) {
        case LAYOUT_RANGE_ATTR_LOCALE:
            str = range->locale;
            break;
        case LAYOUT_RANGE_ATTR_FONTFAMILY:
            str = range->fontfamily;
            break;
        default:
            str = NULL;
    }

    return str;
}

static HRESULT get_string_attribute_length(const struct dwrite_textlayout *layout, enum layout_range_attr_kind kind,
        UINT32 position, UINT32 *length, DWRITE_TEXT_RANGE *r)
{
    struct layout_range *range;
    const WCHAR *str;

    range = get_layout_range_by_pos(layout, position);
    if (!range) {
        *length = 0;
        return S_OK;
    }

    str = get_string_attribute_ptr(range, kind);
    *length = wcslen(str);
    return return_range(&range->h, r);
}

static HRESULT get_string_attribute_value(const struct dwrite_textlayout *layout, enum layout_range_attr_kind kind,
        UINT32 position, WCHAR *ret, UINT32 length, DWRITE_TEXT_RANGE *r)
{
    struct layout_range *range;
    const WCHAR *str;

    if (length == 0)
        return E_INVALIDARG;

    ret[0] = 0;
    range = get_layout_range_by_pos(layout, position);
    if (!range)
        return E_INVALIDARG;

    str = get_string_attribute_ptr(range, kind);
    if (length < wcslen(str) + 1)
        return E_NOT_SUFFICIENT_BUFFER;

    wcscpy(ret, str);
    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_QueryInterface(IDWriteTextLayout4 *iface, REFIID riid, void **obj)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IDWriteTextLayout4) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout3) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout2) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout1) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IDWriteTextFormat3) ||
             IsEqualIID(riid, &IID_IDWriteTextFormat2) ||
             IsEqualIID(riid, &IID_IDWriteTextFormat1) ||
             IsEqualIID(riid, &IID_IDWriteTextFormat))
    {
        *obj = &layout->IDWriteTextFormat3_iface;
    }

    if (*obj) {
        IDWriteTextLayout4_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextlayout_AddRef(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    ULONG refcount = InterlockedIncrement(&layout->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritetextlayout_Release(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    ULONG refcount = InterlockedDecrement(&layout->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFactory7_Release(layout->factory);
        if (layout->system_collection)
            IDWriteFontCollection_Release(layout->system_collection);
        free_layout_ranges_list(layout);
        free_layout_eruns(layout);
        free_layout_runs(layout);
        release_format_data(&layout->format);
        free(layout->nominal_breakpoints);
        free(layout->actual_breakpoints);
        free(layout->clustermetrics);
        free(layout->clusters);
        free(layout->lines);
        free(layout->str);
        free(layout);
    }

    return refcount;
}

static HRESULT WINAPI dwritetextlayout_SetTextAlignment(IDWriteTextLayout4 *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_SetTextAlignment(&layout->IDWriteTextFormat3_iface, alignment);
}

static HRESULT WINAPI dwritetextlayout_SetParagraphAlignment(IDWriteTextLayout4 *iface,
        DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_SetParagraphAlignment(&layout->IDWriteTextFormat3_iface, alignment);
}

static HRESULT WINAPI dwritetextlayout_SetWordWrapping(IDWriteTextLayout4 *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_SetWordWrapping(&layout->IDWriteTextFormat3_iface, wrapping);
}

static HRESULT WINAPI dwritetextlayout_SetReadingDirection(IDWriteTextLayout4 *iface,
        DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_SetReadingDirection(&layout->IDWriteTextFormat3_iface, direction);
}

static HRESULT WINAPI dwritetextlayout_SetFlowDirection(IDWriteTextLayout4 *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_SetFlowDirection(&layout->IDWriteTextFormat3_iface, direction);
}

static HRESULT WINAPI dwritetextlayout_SetIncrementalTabStop(IDWriteTextLayout4 *iface, FLOAT tabstop)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_SetIncrementalTabStop(&layout->IDWriteTextFormat3_iface, tabstop);
}

static HRESULT WINAPI dwritetextlayout_SetTrimming(IDWriteTextLayout4 *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_SetTrimming(&layout->IDWriteTextFormat3_iface, trimming, trimming_sign);
}

static HRESULT WINAPI dwritetextlayout_SetLineSpacing(IDWriteTextLayout4 *iface, DWRITE_LINE_SPACING_METHOD spacing,
    FLOAT line_spacing, FLOAT baseline)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat1_SetLineSpacing((IDWriteTextFormat1 *)&layout->IDWriteTextFormat3_iface, spacing,
            line_spacing, baseline);
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextlayout_GetTextAlignment(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetTextAlignment(&layout->IDWriteTextFormat3_iface);
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextlayout_GetParagraphAlignment(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetParagraphAlignment(&layout->IDWriteTextFormat3_iface);
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextlayout_GetWordWrapping(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetWordWrapping(&layout->IDWriteTextFormat3_iface);
}

static DWRITE_READING_DIRECTION WINAPI dwritetextlayout_GetReadingDirection(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetReadingDirection(&layout->IDWriteTextFormat3_iface);
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextlayout_GetFlowDirection(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetFlowDirection(&layout->IDWriteTextFormat3_iface);
}

static FLOAT WINAPI dwritetextlayout_GetIncrementalTabStop(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetIncrementalTabStop(&layout->IDWriteTextFormat3_iface);
}

static HRESULT WINAPI dwritetextlayout_GetTrimming(IDWriteTextLayout4 *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetTrimming(&layout->IDWriteTextFormat3_iface, options, trimming_sign);
}

static HRESULT WINAPI dwritetextlayout_GetLineSpacing(IDWriteTextLayout4 *iface, DWRITE_LINE_SPACING_METHOD *method,
    FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat_GetLineSpacing((IDWriteTextFormat *)&layout->IDWriteTextFormat3_iface, method,
            spacing, baseline);
}

static HRESULT WINAPI dwritetextlayout_GetFontCollection(IDWriteTextLayout4 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetFontCollection(&layout->IDWriteTextFormat3_iface, collection);
}

static UINT32 WINAPI dwritetextlayout_GetFontFamilyNameLength(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetFontFamilyNameLength(&layout->IDWriteTextFormat3_iface);
}

static HRESULT WINAPI dwritetextlayout_GetFontFamilyName(IDWriteTextLayout4 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetFontFamilyName(&layout->IDWriteTextFormat3_iface, name, size);
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextlayout_GetFontWeight(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetFontWeight(&layout->IDWriteTextFormat3_iface);
}

static DWRITE_FONT_STYLE WINAPI dwritetextlayout_GetFontStyle(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetFontStyle(&layout->IDWriteTextFormat3_iface);
}

static DWRITE_FONT_STRETCH WINAPI dwritetextlayout_GetFontStretch(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetFontStretch(&layout->IDWriteTextFormat3_iface);
}

static FLOAT WINAPI dwritetextlayout_GetFontSize(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetFontSize(&layout->IDWriteTextFormat3_iface);
}

static UINT32 WINAPI dwritetextlayout_GetLocaleNameLength(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetLocaleNameLength(&layout->IDWriteTextFormat3_iface);
}

static HRESULT WINAPI dwritetextlayout_GetLocaleName(IDWriteTextLayout4 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    return IDWriteTextFormat3_GetLocaleName(&layout->IDWriteTextFormat3_iface, name, size);
}

static HRESULT WINAPI dwritetextlayout_SetMaxWidth(IDWriteTextLayout4 *iface, FLOAT maxWidth)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    BOOL changed;

    TRACE("%p, %.8e.\n", iface, maxWidth);

    if (maxWidth < 0.0f)
        return E_INVALIDARG;

    changed = layout->metrics.layoutWidth != maxWidth;
    layout->metrics.layoutWidth = maxWidth;

    if (changed)
        layout->recompute |= RECOMPUTE_LINES_AND_OVERHANGS;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_SetMaxHeight(IDWriteTextLayout4 *iface, FLOAT maxHeight)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    BOOL changed;

    TRACE("%p, %.8e.\n", iface, maxHeight);

    if (maxHeight < 0.0f)
        return E_INVALIDARG;

    changed = layout->metrics.layoutHeight != maxHeight;
    layout->metrics.layoutHeight = maxHeight;

    if (changed)
        layout->recompute |= RECOMPUTE_LINES_AND_OVERHANGS;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_SetFontCollection(IDWriteTextLayout4 *iface, IDWriteFontCollection *collection,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %p, %s.\n", iface, collection, debugstr_range(&range));

    value.range = range;
    value.u.collection = collection;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_FONTCOLL, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontFamilyName(IDWriteTextLayout4 *iface, WCHAR const *name,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %s, %s.\n", iface, debugstr_w(name), debugstr_range(&range));

    if (!name)
        return E_INVALIDARG;

    value.range = range;
    value.u.fontfamily = name;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_FONTFAMILY, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontWeight(IDWriteTextLayout4 *iface, DWRITE_FONT_WEIGHT weight,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %d, %s.\n", iface, weight, debugstr_range(&range));

    if ((UINT32)weight > DWRITE_FONT_WEIGHT_ULTRA_BLACK)
        return E_INVALIDARG;

    value.range = range;
    value.u.weight = weight;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_WEIGHT, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontStyle(IDWriteTextLayout4 *iface, DWRITE_FONT_STYLE style,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %d, %s.\n", iface, style, debugstr_range(&range));

    if ((UINT32)style > DWRITE_FONT_STYLE_ITALIC)
        return E_INVALIDARG;

    value.range = range;
    value.u.style = style;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_STYLE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontStretch(IDWriteTextLayout4 *iface, DWRITE_FONT_STRETCH stretch,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %d, %s.\n", iface, stretch, debugstr_range(&range));

    if (stretch == DWRITE_FONT_STRETCH_UNDEFINED || (UINT32)stretch > DWRITE_FONT_STRETCH_ULTRA_EXPANDED)
        return E_INVALIDARG;

    value.range = range;
    value.u.stretch = stretch;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_STRETCH, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontSize(IDWriteTextLayout4 *iface, FLOAT size, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %.8e, %s.\n", iface, size, debugstr_range(&range));

    if (size <= 0.0f)
        return E_INVALIDARG;

    value.range = range;
    value.u.fontsize = size;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_FONTSIZE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetUnderline(IDWriteTextLayout4 *iface, BOOL underline, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %d, %s.\n", iface, underline, debugstr_range(&range));

    value.range = range;
    value.u.underline = underline;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_UNDERLINE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetStrikethrough(IDWriteTextLayout4 *iface, BOOL strikethrough,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %d, %s.\n", iface, strikethrough, debugstr_range(&range));

    value.range = range;
    value.u.strikethrough = strikethrough;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_STRIKETHROUGH, &value);
}

static HRESULT WINAPI dwritetextlayout_SetDrawingEffect(IDWriteTextLayout4 *iface, IUnknown* effect,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %p, %s.\n", iface, effect, debugstr_range(&range));

    value.range = range;
    value.u.effect = effect;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_EFFECT, &value);
}

static HRESULT WINAPI dwritetextlayout_SetInlineObject(IDWriteTextLayout4 *iface, IDWriteInlineObject *object,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %p, %s.\n", iface, object, debugstr_range(&range));

    value.range = range;
    value.u.object = object;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_INLINE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetTypography(IDWriteTextLayout4 *iface, IDWriteTypography *typography,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %p, %s.\n", iface, typography, debugstr_range(&range));

    value.range = range;
    value.u.typography = typography;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_TYPOGRAPHY, &value);
}

static HRESULT WINAPI dwritetextlayout_SetLocaleName(IDWriteTextLayout4 *iface, WCHAR const* locale,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %s, %s.\n", iface, debugstr_w(locale), debugstr_range(&range));

    if (!locale || wcslen(locale) > LOCALE_NAME_MAX_LENGTH-1)
        return E_INVALIDARG;

    value.range = range;
    value.u.locale = locale;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_LOCALE, &value);
}

static FLOAT WINAPI dwritetextlayout_GetMaxWidth(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p.\n", iface);

    return layout->metrics.layoutWidth;
}

static FLOAT WINAPI dwritetextlayout_GetMaxHeight(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p.\n", iface);

    return layout->metrics.layoutHeight;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontCollection(IDWriteTextLayout4 *iface, UINT32 position,
        IDWriteFontCollection **collection, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, collection, r);

    range = get_layout_range_by_pos(layout, position);
    *collection = range->collection;
    if (*collection)
        IDWriteFontCollection_AddRef(*collection);

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontFamilyNameLength(IDWriteTextLayout4 *iface,
    UINT32 position, UINT32 *length, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %d, %p, %p.\n", iface, position, length, r);

    return get_string_attribute_length(layout, LAYOUT_RANGE_ATTR_FONTFAMILY, position, length, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontFamilyName(IDWriteTextLayout4 *iface,
    UINT32 position, WCHAR *name, UINT32 length, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %u, %p, %u, %p.\n", iface, position, name, length, r);

    return get_string_attribute_value(layout, LAYOUT_RANGE_ATTR_FONTFAMILY, position, name, length, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontWeight(IDWriteTextLayout4 *iface,
    UINT32 position, DWRITE_FONT_WEIGHT *weight, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, weight, r);

    range = get_layout_range_by_pos(layout, position);
    *weight = range->weight;

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontStyle(IDWriteTextLayout4 *iface,
    UINT32 position, DWRITE_FONT_STYLE *style, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, style, r);

    range = get_layout_range_by_pos(layout, position);
    *style = range->style;
    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontStretch(IDWriteTextLayout4 *iface,
    UINT32 position, DWRITE_FONT_STRETCH *stretch, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, stretch, r);

    range = get_layout_range_by_pos(layout, position);
    *stretch = range->stretch;
    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontSize(IDWriteTextLayout4 *iface,
    UINT32 position, FLOAT *size, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, size, r);

    range = get_layout_range_by_pos(layout, position);
    *size = range->fontsize;
    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_GetUnderline(IDWriteTextLayout4 *iface,
    UINT32 position, BOOL *underline, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_bool *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, underline, r);

    range = (struct layout_range_bool *)get_layout_range_header_by_pos(&layout->underline_ranges, position);
    *underline = range->value;

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_GetStrikethrough(IDWriteTextLayout4 *iface,
    UINT32 position, BOOL *strikethrough, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_bool *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, strikethrough, r);

    range = (struct layout_range_bool *)get_layout_range_header_by_pos(&layout->strike_ranges, position);
    *strikethrough = range->value;

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_GetDrawingEffect(IDWriteTextLayout4 *iface,
    UINT32 position, IUnknown **effect, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_iface *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, effect, r);

    range = (struct layout_range_iface *)get_layout_range_header_by_pos(&layout->effects, position);
    *effect = range->iface;
    if (*effect)
        IUnknown_AddRef(*effect);

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_GetInlineObject(IDWriteTextLayout4 *iface,
    UINT32 position, IDWriteInlineObject **object, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, object, r);

    range = get_layout_range_by_pos(layout, position);
    *object = range->object;
    if (*object)
        IDWriteInlineObject_AddRef(*object);

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_GetTypography(IDWriteTextLayout4 *iface,
    UINT32 position, IDWriteTypography** typography, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_iface *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, typography, r);

    range = (struct layout_range_iface *)get_layout_range_header_by_pos(&layout->typographies, position);
    *typography = (IDWriteTypography *)range->iface;
    if (*typography)
        IDWriteTypography_AddRef(*typography);

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetLocaleNameLength(IDWriteTextLayout4 *iface,
        UINT32 position, UINT32 *length, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %u, %p, %p.\n", iface, position, length, r);

    return get_string_attribute_length(layout, LAYOUT_RANGE_ATTR_LOCALE, position, length, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetLocaleName(IDWriteTextLayout4 *iface,
        UINT32 position, WCHAR *locale, UINT32 length, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %u, %p, %u, %p.\n", iface, position, locale, length, r);

    return get_string_attribute_value(layout, LAYOUT_RANGE_ATTR_LOCALE, position, locale, length, r);
}

static inline FLOAT renderer_apply_snapping(FLOAT coord, BOOL skiptransform, FLOAT ppdip, FLOAT det,
    const DWRITE_MATRIX *m)
{
    D2D1_POINT_2F vec, vec2;

    if (!skiptransform) {
        /* apply transform */
        vec.x = 0.0f;
        vec.y = coord * ppdip;

        vec2.x = m->m11 * vec.x + m->m21 * vec.y + m->dx;
        vec2.y = m->m12 * vec.x + m->m22 * vec.y + m->dy;

        /* snap */
        vec2.x = floorf(vec2.x + 0.5f);
        vec2.y = floorf(vec2.y + 0.5f);

        /* apply inverted transform, we don't care about X component at this point */
        vec.y = (-m->m12 * vec2.x + m->m11 * vec2.y - (m->m11 * m->dy - m->m12 * m->dx)) / det;
        vec.y /= ppdip;
    }
    else
        vec.y = floorf(coord * ppdip + 0.5f) / ppdip;

    return vec.y;
}

static HRESULT WINAPI dwritetextlayout_Draw(IDWriteTextLayout4 *iface,
    void *context, IDWriteTextRenderer* renderer, FLOAT origin_x, FLOAT origin_y)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    BOOL disabled = FALSE, skiptransform = FALSE;
    struct layout_effective_inline *inlineobject;
    struct layout_effective_run *run;
    struct layout_strikethrough *s;
    struct layout_underline *u;
    FLOAT det = 0.0f, ppdip = 0.0f;
    DWRITE_MATRIX m = { 0 };
    HRESULT hr;

    TRACE("%p, %p, %p, %.8e, %.8e.\n", iface, context, renderer, origin_x, origin_y);

    hr = layout_compute_effective_runs(layout);
    if (FAILED(hr))
        return hr;

    hr = IDWriteTextRenderer_IsPixelSnappingDisabled(renderer, context, &disabled);
    if (FAILED(hr))
        return hr;

    if (!disabled) {
        hr = IDWriteTextRenderer_GetPixelsPerDip(renderer, context, &ppdip);
        if (FAILED(hr))
            return hr;

        hr = IDWriteTextRenderer_GetCurrentTransform(renderer, context, &m);
        if (FAILED(hr))
            return hr;

        /* it's only allowed to have a diagonal/antidiagonal transform matrix */
        if (ppdip <= 0.0f ||
            (m.m11 * m.m22 != 0.0f && (m.m12 != 0.0f || m.m21 != 0.0f)) ||
            (m.m12 * m.m21 != 0.0f && (m.m11 != 0.0f || m.m22 != 0.0f)))
            disabled = TRUE;
        else
            skiptransform = should_skip_transform(&m, &det);
    }

#define SNAP_COORD(x) (disabled ? (x) : renderer_apply_snapping((x), skiptransform, ppdip, det, &m))
    /* 1. Regular runs */
    LIST_FOR_EACH_ENTRY(run, &layout->eruns, struct layout_effective_run, entry)
    {
        const struct regular_layout_run *regular = &run->run->u.regular;
        UINT32 start_glyph = regular->clustermap[run->start];
        DWRITE_GLYPH_RUN_DESCRIPTION descr;
        DWRITE_GLYPH_RUN glyph_run;

        /* Everything but cluster map will be reused from nominal run, as we only need
           to adjust some pointers. Cluster map however is rebuilt when effective run is added,
           it can't be reused because it has to start with 0 index for each reported run. */
        glyph_run = regular->run;
        glyph_run.glyphCount = run->glyphcount;

        /* fixup glyph data arrays */
        glyph_run.glyphIndices += start_glyph;
        glyph_run.glyphAdvances += start_glyph;
        glyph_run.glyphOffsets += start_glyph;

        /* description */
        descr = regular->descr;
        descr.stringLength = run->length;
        descr.string += run->start;
        descr.clusterMap = run->clustermap;
        descr.textPosition += run->start;

        /* return value is ignored */
        IDWriteTextRenderer_DrawGlyphRun(renderer,
            context,
            run->origin.x + run->align_dx + origin_x,
            SNAP_COORD(run->origin.y + origin_y),
            layout->measuringmode,
            &glyph_run,
            &descr,
            run->effect);
    }

    /* 2. Inline objects */
    LIST_FOR_EACH_ENTRY(inlineobject, &layout->inlineobjects, struct layout_effective_inline, entry)
    {
        IDWriteTextRenderer_DrawInlineObject(renderer,
            context,
            inlineobject->origin.x + inlineobject->align_dx + origin_x,
            SNAP_COORD(inlineobject->origin.y + origin_y),
            inlineobject->object,
            inlineobject->is_sideways,
            inlineobject->is_rtl,
            inlineobject->effect);
    }

    /* 3. Underlines */
    LIST_FOR_EACH_ENTRY(u, &layout->underlines, struct layout_underline, entry)
    {
        IDWriteTextRenderer_DrawUnderline(renderer,
            context,
            /* horizontal underline always grows from left to right, width is always added to origin regardless of run direction */
            (is_run_rtl(u->run) ? u->run->origin.x - u->run->width : u->run->origin.x) + u->run->align_dx + origin_x,
            SNAP_COORD(u->run->origin.y + origin_y),
            &u->u,
            u->run->effect);
    }

    /* 4. Strikethrough */
    LIST_FOR_EACH_ENTRY(s, &layout->strikethrough, struct layout_strikethrough, entry)
    {
        IDWriteTextRenderer_DrawStrikethrough(renderer,
            context,
            s->run->origin.x + s->run->align_dx + origin_x,
            SNAP_COORD(s->run->origin.y + origin_y),
            &s->s,
            s->run->effect);
    }
#undef SNAP_COORD

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_GetLineMetrics(IDWriteTextLayout4 *iface,
    DWRITE_LINE_METRICS *metrics, UINT32 max_count, UINT32 *count)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    unsigned int line_count;
    HRESULT hr;
    size_t i;

    TRACE("%p, %p, %u, %p.\n", iface, metrics, max_count, count);

    if (FAILED(hr = layout_compute_effective_runs(layout)))
        return hr;

    if (metrics)
    {
        line_count = min(max_count, layout->metrics.lineCount);
        for (i = 0; i < line_count; ++i)
            memcpy(&metrics[i], &layout->lines[i].metrics, sizeof(*metrics));
    }

    *count = layout->metrics.lineCount;
    return max_count >= layout->metrics.lineCount ? S_OK : E_NOT_SUFFICIENT_BUFFER;
}

static HRESULT layout_update_metrics(struct dwrite_textlayout *layout)
{
    return layout_compute_effective_runs(layout);
}

static HRESULT WINAPI dwritetextlayout_GetMetrics(IDWriteTextLayout4 *iface, DWRITE_TEXT_METRICS *metrics)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, metrics);

    hr = layout_update_metrics(layout);
    if (hr == S_OK)
        memcpy(metrics, &layout->metrics, sizeof(*metrics));

    return hr;
}

static void d2d_rect_offset(D2D1_RECT_F *rect, FLOAT x, FLOAT y)
{
    rect->left += x;
    rect->right += x;
    rect->top += y;
    rect->bottom += y;
}

static BOOL d2d_rect_is_empty(const D2D1_RECT_F *rect)
{
    return ((rect->left >= rect->right) || (rect->top >= rect->bottom));
}

static void d2d_rect_union(D2D1_RECT_F *dst, const D2D1_RECT_F *src)
{
    if (d2d_rect_is_empty(dst)) {
        if (d2d_rect_is_empty(src)) {
            dst->left = dst->right = dst->top = dst->bottom = 0.0f;
            return;
        }
        else
            *dst = *src;
    }
    else {
        if (!d2d_rect_is_empty(src)) {
            dst->left   = min(dst->left, src->left);
            dst->right  = max(dst->right, src->right);
            dst->top    = min(dst->top, src->top);
            dst->bottom = max(dst->bottom, src->bottom);
        }
    }
}

static void layout_get_erun_bbox(struct dwrite_textlayout *layout, struct layout_effective_run *run, D2D1_RECT_F *bbox)
{
    const struct regular_layout_run *regular = &run->run->u.regular;
    UINT32 start_glyph = regular->clustermap[run->start];
    D2D1_POINT_2F baseline_origin = { 0 }, *origins;
    DWRITE_GLYPH_RUN glyph_run;
    unsigned int i;
    HRESULT hr;

    if (run->bbox.top == run->bbox.bottom)
    {
        struct dwrite_glyphbitmap glyph_bitmap;
        RECT *bbox;

        glyph_run = regular->run;
        glyph_run.glyphCount = run->glyphcount;
        glyph_run.glyphIndices = &regular->run.glyphIndices[start_glyph];
        glyph_run.glyphAdvances = &regular->run.glyphAdvances[start_glyph];
        glyph_run.glyphOffsets = &regular->run.glyphOffsets[start_glyph];

        memset(&glyph_bitmap, 0, sizeof(glyph_bitmap));
        glyph_bitmap.simulations = IDWriteFontFace_GetSimulations(glyph_run.fontFace);
        glyph_bitmap.emsize = glyph_run.fontEmSize;

        bbox = &glyph_bitmap.bbox;

        if (!(origins = calloc(glyph_run.glyphCount, sizeof(*origins))))
            return;

        if (FAILED(hr = compute_glyph_origins(&glyph_run, layout->measuringmode, baseline_origin, &layout->transform, origins)))
        {
            WARN("Failed to compute glyph origins, hr %#lx.\n", hr);
            free(origins);
            return;
        }

        for (i = 0; i < glyph_run.glyphCount; ++i)
        {
            D2D1_RECT_F glyph_bbox;

            glyph_bitmap.glyph = glyph_run.glyphIndices[i];
            dwrite_fontface_get_glyph_bbox(glyph_run.fontFace, &glyph_bitmap);

            glyph_bbox.left = bbox->left;
            glyph_bbox.top = bbox->top;
            glyph_bbox.right = bbox->right;
            glyph_bbox.bottom = bbox->bottom;

            d2d_rect_offset(&glyph_bbox, origins[i].x, origins[i].y);
            d2d_rect_union(&run->bbox, &glyph_bbox);
        }

        free(origins);
    }

    *bbox = run->bbox;
    d2d_rect_offset(bbox, run->origin.x + run->align_dx, run->origin.y);
}

static void layout_get_inlineobj_bbox(const struct layout_effective_inline *run, D2D1_RECT_F *bbox)
{
    DWRITE_OVERHANG_METRICS overhang_metrics = { 0 };
    DWRITE_INLINE_OBJECT_METRICS metrics = { 0 };
    HRESULT hr;

    if (FAILED(hr = IDWriteInlineObject_GetMetrics(run->object, &metrics))) {
        WARN("Failed to get inline object metrics, hr %#lx.\n", hr);
        memset(bbox, 0, sizeof(*bbox));
        return;
    }

    bbox->left = run->origin.x + run->align_dx;
    bbox->right = bbox->left + metrics.width;
    bbox->top = run->origin.y;
    bbox->bottom = bbox->top + metrics.height;

    IDWriteInlineObject_GetOverhangMetrics(run->object, &overhang_metrics);

    bbox->left -= overhang_metrics.left;
    bbox->right += overhang_metrics.right;
    bbox->top -= overhang_metrics.top;
    bbox->bottom += overhang_metrics.bottom;
}

static HRESULT WINAPI dwritetextlayout_GetOverhangMetrics(IDWriteTextLayout4 *iface,
        DWRITE_OVERHANG_METRICS *overhangs)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_effective_inline *inline_run;
    struct layout_effective_run *run;
    D2D1_RECT_F bbox = { 0 };
    HRESULT hr;

    TRACE("%p, %p.\n", iface, overhangs);

    memset(overhangs, 0, sizeof(*overhangs));

    if (!(layout->recompute & RECOMPUTE_OVERHANGS))
    {
        *overhangs = layout->overhangs;
        return S_OK;
    }

    hr = layout_compute_effective_runs(layout);
    if (FAILED(hr))
        return hr;

    LIST_FOR_EACH_ENTRY(run, &layout->eruns, struct layout_effective_run, entry)
    {
        D2D1_RECT_F run_bbox;

        layout_get_erun_bbox(layout, run, &run_bbox);
        d2d_rect_union(&bbox, &run_bbox);
    }

    LIST_FOR_EACH_ENTRY(inline_run, &layout->inlineobjects, struct layout_effective_inline, entry)
    {
        D2D1_RECT_F object_bbox;

        layout_get_inlineobj_bbox(inline_run, &object_bbox);
        d2d_rect_union(&bbox, &object_bbox);
    }

    /* Deltas from layout box. */
    layout->overhangs.left = -bbox.left;
    layout->overhangs.top = -bbox.top;
    layout->overhangs.right = bbox.right - layout->metrics.layoutWidth;
    layout->overhangs.bottom = bbox.bottom - layout->metrics.layoutHeight;
    layout->recompute &= ~RECOMPUTE_OVERHANGS;

    *overhangs = layout->overhangs;

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_GetClusterMetrics(IDWriteTextLayout4 *iface,
    DWRITE_CLUSTER_METRICS *metrics, UINT32 max_count, UINT32 *count)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    HRESULT hr;

    TRACE("%p, %p, %u, %p.\n", iface, metrics, max_count, count);

    hr = layout_compute(layout);
    if (FAILED(hr))
        return hr;

    if (metrics)
        memcpy(metrics, layout->clustermetrics, sizeof(DWRITE_CLUSTER_METRICS) * min(max_count, layout->cluster_count));

    *count = layout->cluster_count;
    return max_count >= layout->cluster_count ? S_OK : E_NOT_SUFFICIENT_BUFFER;
}

static HRESULT WINAPI dwritetextlayout_DetermineMinWidth(IDWriteTextLayout4 *iface, FLOAT* min_width)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    UINT32 start;
    FLOAT width;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, min_width);

    if (!min_width)
        return E_INVALIDARG;

    if (!(layout->recompute & RECOMPUTE_MINIMAL_WIDTH))
        goto width_done;

    *min_width = 0.0f;
    hr = layout_compute(layout);
    if (FAILED(hr))
        return hr;

    /* Find widest word without emergency breaking between clusters, trailing whitespaces
       preceding breaking point do not contribute to word width. */
    for (start = 0; start < layout->cluster_count;)
    {
        UINT32 end = start, j, next;

        /* Last cluster always could be wrapped after. */
        while (!layout->clustermetrics[end].canWrapLineAfter)
            end++;
        /* make is so current cluster range that we can wrap after is [start,end) */
        end++;

        next = end;

        /* Ignore trailing whitespace clusters, in case of single space range will
           be reduced to empty range, or [start,start+1). */
        while (end > start && layout->clustermetrics[end-1].isWhitespace)
            end--;

        /* check if cluster range exceeds last minimal width */
        width = 0.0f;
        for (j = start; j < end; j++)
            width += layout->clustermetrics[j].width;

        start = next;

        if (width > layout->minwidth)
            layout->minwidth = width;
    }
    layout->recompute &= ~RECOMPUTE_MINIMAL_WIDTH;

width_done:
    *min_width = layout->minwidth;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_HitTestPoint(IDWriteTextLayout4 *iface,
    FLOAT pointX, FLOAT pointY, BOOL* is_trailinghit, BOOL* is_inside, DWRITE_HIT_TEST_METRICS *metrics)
{
    FIXME("%p, %.8e, %.8e, %p, %p, %p): stub\n", iface, pointX, pointY, is_trailinghit, is_inside, metrics);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestTextPosition(IDWriteTextLayout4 *iface,
        UINT32 textPosition, BOOL is_trailinghit, FLOAT *pointX, FLOAT *pointY, DWRITE_HIT_TEST_METRICS *metrics)
{
    FIXME("%p, %u, %d, %p, %p, %p): stub\n", iface, textPosition, is_trailinghit, pointX, pointY, metrics);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestTextRange(IDWriteTextLayout4 *iface,
    UINT32 textPosition, UINT32 textLength, FLOAT originX, FLOAT originY,
    DWRITE_HIT_TEST_METRICS *metrics, UINT32 max_metricscount, UINT32* actual_metricscount)
{
    FIXME("%p, %u, %u, %f, %f, %p, %u, %p): stub\n", iface, textPosition, textLength, originX, originY, metrics,
        max_metricscount, actual_metricscount);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout1_SetPairKerning(IDWriteTextLayout4 *iface, BOOL is_pairkerning_enabled,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %d, %s.\n", iface, is_pairkerning_enabled, debugstr_range(&range));

    value.range = range;
    value.u.pair_kerning = !!is_pairkerning_enabled;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_PAIR_KERNING, &value);
}

static HRESULT WINAPI dwritetextlayout1_GetPairKerning(IDWriteTextLayout4 *iface, UINT32 position,
        BOOL *is_pairkerning_enabled, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range *range;

    TRACE("%p, %u, %p, %p.\n", iface, position, is_pairkerning_enabled, r);

    range = get_layout_range_by_pos(layout, position);
    *is_pairkerning_enabled = range->pair_kerning;

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout1_SetCharacterSpacing(IDWriteTextLayout4 *iface, FLOAT leading, FLOAT trailing,
    FLOAT min_advance, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_attr_value value;

    TRACE("%p, %.8e, %.8e, %.8e, %s.\n", iface, leading, trailing, min_advance, debugstr_range(&range));

    if (min_advance < 0.0f)
        return E_INVALIDARG;

    value.range = range;
    value.u.spacing.leading = leading;
    value.u.spacing.trailing = trailing;
    value.u.spacing.min_advance = min_advance;
    return set_layout_range_attr(layout, LAYOUT_RANGE_ATTR_SPACING, &value);
}

static HRESULT WINAPI dwritetextlayout1_GetCharacterSpacing(IDWriteTextLayout4 *iface, UINT32 position, FLOAT *leading,
    FLOAT *trailing, FLOAT *min_advance, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    struct layout_range_spacing *range;

    TRACE("%p, %u, %p, %p, %p, %p.\n", iface, position, leading, trailing, min_advance, r);

    range = (struct layout_range_spacing *)get_layout_range_header_by_pos(&layout->spacing, position);
    *leading = range->leading;
    *trailing = range->trailing;
    *min_advance = range->min_advance;

    return return_range(&range->h, r);
}

static HRESULT WINAPI dwritetextlayout2_GetMetrics(IDWriteTextLayout4 *iface, DWRITE_TEXT_METRICS1 *metrics)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, metrics);

    if (SUCCEEDED(hr = layout_update_metrics(layout)))
        *metrics = layout->metrics;

    return hr;
}

static HRESULT layout_set_vertical_orientation(struct dwrite_textlayout *layout,
        DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    BOOL changed;
    HRESULT hr;

    if (FAILED(hr = format_set_vertical_orientation(&layout->format, orientation, &changed)))
        return hr;

    if (changed)
        layout->recompute = RECOMPUTE_EVERYTHING;

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout2_SetVerticalGlyphOrientation(IDWriteTextLayout4 *iface,
        DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %d.\n", iface, orientation);

    return layout_set_vertical_orientation(layout, orientation);
}

static DWRITE_VERTICAL_GLYPH_ORIENTATION WINAPI dwritetextlayout2_GetVerticalGlyphOrientation(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p.\n", iface);

    return layout->format.vertical_orientation;
}

static HRESULT WINAPI dwritetextlayout2_SetLastLineWrapping(IDWriteTextLayout4 *iface, BOOL lastline_wrapping_enabled)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %d.\n", iface, lastline_wrapping_enabled);

    return IDWriteTextFormat3_SetLastLineWrapping(&layout->IDWriteTextFormat3_iface, lastline_wrapping_enabled);
}

static BOOL WINAPI dwritetextlayout2_GetLastLineWrapping(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p.\n", iface);

    return IDWriteTextFormat3_GetLastLineWrapping(&layout->IDWriteTextFormat3_iface);
}

static HRESULT WINAPI dwritetextlayout2_SetOpticalAlignment(IDWriteTextLayout4 *iface,
        DWRITE_OPTICAL_ALIGNMENT alignment)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %d.\n", iface, alignment);

    return IDWriteTextFormat3_SetOpticalAlignment(&layout->IDWriteTextFormat3_iface, alignment);
}

static DWRITE_OPTICAL_ALIGNMENT WINAPI dwritetextlayout2_GetOpticalAlignment(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p.\n", iface);

    return IDWriteTextFormat3_GetOpticalAlignment(&layout->IDWriteTextFormat3_iface);
}

static HRESULT WINAPI dwritetextlayout2_SetFontFallback(IDWriteTextLayout4 *iface, IDWriteFontFallback *fallback)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %p.\n", iface, fallback);

    return format_set_fontfallback(&layout->format, fallback);
}

static HRESULT WINAPI dwritetextlayout2_GetFontFallback(IDWriteTextLayout4 *iface, IDWriteFontFallback **fallback)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %p.\n", iface, fallback);

    return format_get_fontfallback(&layout->format, fallback);
}

static HRESULT WINAPI dwritetextlayout3_InvalidateLayout(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p.\n", iface);

    layout->recompute = RECOMPUTE_EVERYTHING;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout3_SetLineSpacing(IDWriteTextLayout4 *iface, DWRITE_LINE_SPACING const *spacing)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    BOOL changed;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, spacing);

    hr = format_set_linespacing(&layout->format, spacing, &changed);
    if (FAILED(hr))
        return hr;

    if (changed)
    {
        if (!(layout->recompute & RECOMPUTE_LINES))
        {
            UINT32 line;

            for (line = 0; line < layout->metrics.lineCount; line++)
                layout_apply_line_spacing(layout, line);

            layout_set_line_positions(layout);
        }

        layout->recompute |= RECOMPUTE_OVERHANGS;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout3_GetLineSpacing(IDWriteTextLayout4 *iface, DWRITE_LINE_SPACING *spacing)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %p.\n", iface, spacing);

    *spacing = layout->format.spacing;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout3_GetLineMetrics(IDWriteTextLayout4 *iface, DWRITE_LINE_METRICS1 *metrics,
    UINT32 max_count, UINT32 *count)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);
    unsigned int line_count;
    HRESULT hr;
    size_t i;

    TRACE("%p, %p, %u, %p.\n", iface, metrics, max_count, count);

    if (FAILED(hr = layout_compute_effective_runs(layout)))
        return hr;

    if (metrics)
    {
        line_count = min(max_count, layout->metrics.lineCount);
        for (i = 0; i < line_count; ++i)
            metrics[i] = layout->lines[i].metrics;
    }

    *count = layout->metrics.lineCount;
    return max_count >= layout->metrics.lineCount ? S_OK : E_NOT_SUFFICIENT_BUFFER;
}

static HRESULT WINAPI dwritetextlayout4_SetFontAxisValues(IDWriteTextLayout4 *iface,
        DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_values, DWRITE_TEXT_RANGE range)
{
    FIXME("%p, %p, %u, %s.\n", iface, axis_values, num_values, debugstr_range(&range));

    return E_NOTIMPL;
}

static UINT32 WINAPI dwritetextlayout4_GetFontAxisValueCount(IDWriteTextLayout4 *iface, UINT32 pos)
{
    FIXME("%p, %u.\n", iface, pos);

    return 0;
}

static HRESULT WINAPI dwritetextlayout4_GetFontAxisValues(IDWriteTextLayout4 *iface, UINT32 pos,
        DWRITE_FONT_AXIS_VALUE *values, UINT32 num_values, DWRITE_TEXT_RANGE *range)
{
    FIXME("%p, %u, %p, %u, %p.\n", iface, pos, values, num_values, range);

    return E_NOTIMPL;
}

static DWRITE_AUTOMATIC_FONT_AXES WINAPI dwritetextlayout4_GetAutomaticFontAxes(IDWriteTextLayout4 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p.\n", iface);

    return layout->format.automatic_axes;
}

static HRESULT WINAPI dwritetextlayout4_SetAutomaticFontAxes(IDWriteTextLayout4 *iface,
        DWRITE_AUTOMATIC_FONT_AXES axes)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextLayout4(iface);

    TRACE("%p, %d.\n", iface, axes);

    if ((unsigned int)axes > DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE)
        return E_INVALIDARG;

    layout->format.automatic_axes = axes;
    return S_OK;
}

static const IDWriteTextLayout4Vtbl dwritetextlayoutvtbl =
{
    dwritetextlayout_QueryInterface,
    dwritetextlayout_AddRef,
    dwritetextlayout_Release,
    dwritetextlayout_SetTextAlignment,
    dwritetextlayout_SetParagraphAlignment,
    dwritetextlayout_SetWordWrapping,
    dwritetextlayout_SetReadingDirection,
    dwritetextlayout_SetFlowDirection,
    dwritetextlayout_SetIncrementalTabStop,
    dwritetextlayout_SetTrimming,
    dwritetextlayout_SetLineSpacing,
    dwritetextlayout_GetTextAlignment,
    dwritetextlayout_GetParagraphAlignment,
    dwritetextlayout_GetWordWrapping,
    dwritetextlayout_GetReadingDirection,
    dwritetextlayout_GetFlowDirection,
    dwritetextlayout_GetIncrementalTabStop,
    dwritetextlayout_GetTrimming,
    dwritetextlayout_GetLineSpacing,
    dwritetextlayout_GetFontCollection,
    dwritetextlayout_GetFontFamilyNameLength,
    dwritetextlayout_GetFontFamilyName,
    dwritetextlayout_GetFontWeight,
    dwritetextlayout_GetFontStyle,
    dwritetextlayout_GetFontStretch,
    dwritetextlayout_GetFontSize,
    dwritetextlayout_GetLocaleNameLength,
    dwritetextlayout_GetLocaleName,
    dwritetextlayout_SetMaxWidth,
    dwritetextlayout_SetMaxHeight,
    dwritetextlayout_SetFontCollection,
    dwritetextlayout_SetFontFamilyName,
    dwritetextlayout_SetFontWeight,
    dwritetextlayout_SetFontStyle,
    dwritetextlayout_SetFontStretch,
    dwritetextlayout_SetFontSize,
    dwritetextlayout_SetUnderline,
    dwritetextlayout_SetStrikethrough,
    dwritetextlayout_SetDrawingEffect,
    dwritetextlayout_SetInlineObject,
    dwritetextlayout_SetTypography,
    dwritetextlayout_SetLocaleName,
    dwritetextlayout_GetMaxWidth,
    dwritetextlayout_GetMaxHeight,
    dwritetextlayout_layout_GetFontCollection,
    dwritetextlayout_layout_GetFontFamilyNameLength,
    dwritetextlayout_layout_GetFontFamilyName,
    dwritetextlayout_layout_GetFontWeight,
    dwritetextlayout_layout_GetFontStyle,
    dwritetextlayout_layout_GetFontStretch,
    dwritetextlayout_layout_GetFontSize,
    dwritetextlayout_GetUnderline,
    dwritetextlayout_GetStrikethrough,
    dwritetextlayout_GetDrawingEffect,
    dwritetextlayout_GetInlineObject,
    dwritetextlayout_GetTypography,
    dwritetextlayout_layout_GetLocaleNameLength,
    dwritetextlayout_layout_GetLocaleName,
    dwritetextlayout_Draw,
    dwritetextlayout_GetLineMetrics,
    dwritetextlayout_GetMetrics,
    dwritetextlayout_GetOverhangMetrics,
    dwritetextlayout_GetClusterMetrics,
    dwritetextlayout_DetermineMinWidth,
    dwritetextlayout_HitTestPoint,
    dwritetextlayout_HitTestTextPosition,
    dwritetextlayout_HitTestTextRange,
    dwritetextlayout1_SetPairKerning,
    dwritetextlayout1_GetPairKerning,
    dwritetextlayout1_SetCharacterSpacing,
    dwritetextlayout1_GetCharacterSpacing,
    dwritetextlayout2_GetMetrics,
    dwritetextlayout2_SetVerticalGlyphOrientation,
    dwritetextlayout2_GetVerticalGlyphOrientation,
    dwritetextlayout2_SetLastLineWrapping,
    dwritetextlayout2_GetLastLineWrapping,
    dwritetextlayout2_SetOpticalAlignment,
    dwritetextlayout2_GetOpticalAlignment,
    dwritetextlayout2_SetFontFallback,
    dwritetextlayout2_GetFontFallback,
    dwritetextlayout3_InvalidateLayout,
    dwritetextlayout3_SetLineSpacing,
    dwritetextlayout3_GetLineSpacing,
    dwritetextlayout3_GetLineMetrics,
    dwritetextlayout4_SetFontAxisValues,
    dwritetextlayout4_GetFontAxisValueCount,
    dwritetextlayout4_GetFontAxisValues,
    dwritetextlayout4_GetAutomaticFontAxes,
    dwritetextlayout4_SetAutomaticFontAxes,
};

static HRESULT WINAPI dwritetextformat_layout_QueryInterface(IDWriteTextFormat3 *iface, REFIID riid, void **obj)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    return IDWriteTextLayout4_QueryInterface(&layout->IDWriteTextLayout4_iface, riid, obj);
}

static ULONG WINAPI dwritetextformat_layout_AddRef(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    return IDWriteTextLayout4_AddRef(&layout->IDWriteTextLayout4_iface);
}

static ULONG WINAPI dwritetextformat_layout_Release(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    return IDWriteTextLayout4_Release(&layout->IDWriteTextLayout4_iface);
}

static HRESULT WINAPI dwritetextformat_layout_SetTextAlignment(IDWriteTextFormat3 *iface,
        DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    BOOL changed;
    HRESULT hr;

    TRACE("%p, %d.\n", iface, alignment);

    hr = format_set_textalignment(&layout->format, alignment, &changed);
    if (FAILED(hr))
        return hr;

    if (changed)
    {
        /* if layout is not ready there's nothing to align */
        if (!(layout->recompute & RECOMPUTE_LINES))
            layout_apply_text_alignment(layout);
        layout->recompute |= RECOMPUTE_OVERHANGS;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_layout_SetParagraphAlignment(IDWriteTextFormat3 *iface,
        DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    BOOL changed;
    HRESULT hr;

    TRACE("%p, %d.\n", iface, alignment);

    hr = format_set_paralignment(&layout->format, alignment, &changed);
    if (FAILED(hr))
        return hr;

    if (changed)
    {
        /* if layout is not ready there's nothing to align */
        if (!(layout->recompute & RECOMPUTE_LINES))
            layout_apply_par_alignment(layout);
        layout->recompute |= RECOMPUTE_OVERHANGS;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_layout_SetWordWrapping(IDWriteTextFormat3 *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    BOOL changed;
    HRESULT hr;

    TRACE("%p, %d.\n", iface, wrapping);

    hr = format_set_wordwrapping(&layout->format, wrapping, &changed);
    if (FAILED(hr))
        return hr;

    if (changed)
        layout->recompute |= RECOMPUTE_LINES_AND_OVERHANGS;

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_layout_SetReadingDirection(IDWriteTextFormat3 *iface,
        DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    BOOL changed;
    HRESULT hr;

    TRACE("%p, %d.\n", iface, direction);

    hr = format_set_readingdirection(&layout->format, direction, &changed);
    if (FAILED(hr))
        return hr;

    if (changed)
        layout->recompute = RECOMPUTE_EVERYTHING;

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_layout_SetFlowDirection(IDWriteTextFormat3 *iface,
        DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    BOOL changed;
    HRESULT hr;

    TRACE("%p, %d.\n", iface, direction);

    hr = format_set_flowdirection(&layout->format, direction, &changed);
    if (FAILED(hr))
        return hr;

    if (changed)
        layout->recompute = RECOMPUTE_EVERYTHING;

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_layout_SetIncrementalTabStop(IDWriteTextFormat3 *iface, FLOAT tabstop)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %.8e.\n", iface, tabstop);

    if (tabstop <= 0.0f)
        return E_INVALIDARG;

    layout->format.tabstop = tabstop;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_layout_SetTrimming(IDWriteTextFormat3 *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    BOOL changed;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, trimming, trimming_sign);

    hr = format_set_trimming(&layout->format, trimming, trimming_sign, &changed);

    if (changed)
        layout->recompute |= RECOMPUTE_LINES_AND_OVERHANGS;

    return hr;
}

static HRESULT WINAPI dwritetextformat_layout_SetLineSpacing(IDWriteTextFormat3 *iface,
        DWRITE_LINE_SPACING_METHOD method, FLOAT height, FLOAT baseline)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    DWRITE_LINE_SPACING spacing;

    TRACE("%p, %d, %.8e, %.8e.\n", iface, method, height, baseline);

    spacing = layout->format.spacing;
    spacing.method = method;
    spacing.height = height;
    spacing.baseline = baseline;
    return IDWriteTextLayout4_SetLineSpacing(&layout->IDWriteTextLayout4_iface, &spacing);
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextformat_layout_GetTextAlignment(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.textalignment;
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextformat_layout_GetParagraphAlignment(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.paralign;
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextformat_layout_GetWordWrapping(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.wrapping;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextformat_layout_GetReadingDirection(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.readingdir;
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextformat_layout_GetFlowDirection(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.flow;
}

static FLOAT WINAPI dwritetextformat_layout_GetIncrementalTabStop(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.tabstop;
}

static HRESULT WINAPI dwritetextformat_layout_GetTrimming(IDWriteTextFormat3 *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %p.\n", iface, options, trimming_sign);

    *options = layout->format.trimming;
    *trimming_sign = layout->format.trimmingsign;
    if (*trimming_sign)
        IDWriteInlineObject_AddRef(*trimming_sign);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_layout_GetLineSpacing(IDWriteTextFormat3 *iface,
        DWRITE_LINE_SPACING_METHOD *method, FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %p, %p.\n", iface, method, spacing, baseline);

    *method = layout->format.spacing.method;
    *spacing = layout->format.spacing.height;
    *baseline = layout->format.spacing.baseline;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_layout_GetFontCollection(IDWriteTextFormat3 *iface,
        IDWriteFontCollection **collection)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p.\n", iface, collection);

    *collection = layout->format.collection;
    if (*collection)
        IDWriteFontCollection_AddRef(*collection);
    return S_OK;
}

static UINT32 WINAPI dwritetextformat_layout_GetFontFamilyNameLength(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.family_len;
}

static HRESULT WINAPI dwritetextformat_layout_GetFontFamilyName(IDWriteTextFormat3 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %u.\n", iface, name, size);

    if (size <= layout->format.family_len) return E_NOT_SUFFICIENT_BUFFER;
    wcscpy(name, layout->format.family_name);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextformat_layout_GetFontWeight(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.weight;
}

static DWRITE_FONT_STYLE WINAPI dwritetextformat_layout_GetFontStyle(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.style;
}

static DWRITE_FONT_STRETCH WINAPI dwritetextformat_layout_GetFontStretch(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.stretch;
}

static FLOAT WINAPI dwritetextformat_layout_GetFontSize(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.fontsize;
}

static UINT32 WINAPI dwritetextformat_layout_GetLocaleNameLength(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.locale_len;
}

static HRESULT WINAPI dwritetextformat_layout_GetLocaleName(IDWriteTextFormat3 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %u.\n", iface, name, size);

    if (size <= layout->format.locale_len) return E_NOT_SUFFICIENT_BUFFER;
    wcscpy(name, layout->format.locale);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat1_layout_SetVerticalGlyphOrientation(IDWriteTextFormat3 *iface,
        DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, orientation);

    return layout_set_vertical_orientation(layout, orientation);
}

static DWRITE_VERTICAL_GLYPH_ORIENTATION WINAPI dwritetextformat1_layout_GetVerticalGlyphOrientation(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.vertical_orientation;
}

static HRESULT WINAPI dwritetextformat1_layout_SetLastLineWrapping(IDWriteTextFormat3 *iface,
        BOOL lastline_wrapping_enabled)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, lastline_wrapping_enabled);

    layout->format.last_line_wrapping = !!lastline_wrapping_enabled;
    return S_OK;
}

static BOOL WINAPI dwritetextformat1_layout_GetLastLineWrapping(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.last_line_wrapping;
}

static HRESULT WINAPI dwritetextformat1_layout_SetOpticalAlignment(IDWriteTextFormat3 *iface,
        DWRITE_OPTICAL_ALIGNMENT alignment)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, alignment);

    return format_set_optical_alignment(&layout->format, alignment);
}

static DWRITE_OPTICAL_ALIGNMENT WINAPI dwritetextformat1_layout_GetOpticalAlignment(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.optical_alignment;
}

static HRESULT WINAPI dwritetextformat1_layout_SetFontFallback(IDWriteTextFormat3 *iface,
        IDWriteFontFallback *fallback)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p.\n", iface, fallback);

    return IDWriteTextLayout4_SetFontFallback(&layout->IDWriteTextLayout4_iface, fallback);
}

static HRESULT WINAPI dwritetextformat1_layout_GetFontFallback(IDWriteTextFormat3 *iface,
        IDWriteFontFallback **fallback)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p.\n", iface, fallback);

    return IDWriteTextLayout4_GetFontFallback(&layout->IDWriteTextLayout4_iface, fallback);
}

static HRESULT WINAPI dwritetextformat2_layout_SetLineSpacing(IDWriteTextFormat3 *iface,
        DWRITE_LINE_SPACING const *spacing)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    return IDWriteTextLayout4_SetLineSpacing(&layout->IDWriteTextLayout4_iface, spacing);
}

static HRESULT WINAPI dwritetextformat2_layout_GetLineSpacing(IDWriteTextFormat3 *iface, DWRITE_LINE_SPACING *spacing)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    return IDWriteTextLayout4_GetLineSpacing(&layout->IDWriteTextLayout4_iface, spacing);
}

static HRESULT WINAPI dwritetextformat3_layout_SetFontAxisValues(IDWriteTextFormat3 *iface,
        DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_values)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %u.\n", iface, axis_values, num_values);

    return format_set_font_axisvalues(&layout->format, axis_values, num_values);
}

static UINT32 WINAPI dwritetextformat3_layout_GetFontAxisValueCount(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return layout->format.axis_values_count;
}

static HRESULT WINAPI dwritetextformat3_layout_GetFontAxisValues(IDWriteTextFormat3 *iface,
        DWRITE_FONT_AXIS_VALUE *axis_values, UINT32 num_values)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %u.\n", iface, axis_values, num_values);

    return format_get_font_axisvalues(&layout->format, axis_values, num_values);
}

static DWRITE_AUTOMATIC_FONT_AXES WINAPI dwritetextformat3_layout_GetAutomaticFontAxes(IDWriteTextFormat3 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    return IDWriteTextLayout4_GetAutomaticFontAxes(&layout->IDWriteTextLayout4_iface);
}

static HRESULT WINAPI dwritetextformat3_layout_SetAutomaticFontAxes(IDWriteTextFormat3 *iface,
        DWRITE_AUTOMATIC_FONT_AXES axes)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextFormat3(iface);
    return IDWriteTextLayout4_SetAutomaticFontAxes(&layout->IDWriteTextLayout4_iface, axes);
}

static const IDWriteTextFormat3Vtbl dwritetextformat3_layout_vtbl =
{
    dwritetextformat_layout_QueryInterface,
    dwritetextformat_layout_AddRef,
    dwritetextformat_layout_Release,
    dwritetextformat_layout_SetTextAlignment,
    dwritetextformat_layout_SetParagraphAlignment,
    dwritetextformat_layout_SetWordWrapping,
    dwritetextformat_layout_SetReadingDirection,
    dwritetextformat_layout_SetFlowDirection,
    dwritetextformat_layout_SetIncrementalTabStop,
    dwritetextformat_layout_SetTrimming,
    dwritetextformat_layout_SetLineSpacing,
    dwritetextformat_layout_GetTextAlignment,
    dwritetextformat_layout_GetParagraphAlignment,
    dwritetextformat_layout_GetWordWrapping,
    dwritetextformat_layout_GetReadingDirection,
    dwritetextformat_layout_GetFlowDirection,
    dwritetextformat_layout_GetIncrementalTabStop,
    dwritetextformat_layout_GetTrimming,
    dwritetextformat_layout_GetLineSpacing,
    dwritetextformat_layout_GetFontCollection,
    dwritetextformat_layout_GetFontFamilyNameLength,
    dwritetextformat_layout_GetFontFamilyName,
    dwritetextformat_layout_GetFontWeight,
    dwritetextformat_layout_GetFontStyle,
    dwritetextformat_layout_GetFontStretch,
    dwritetextformat_layout_GetFontSize,
    dwritetextformat_layout_GetLocaleNameLength,
    dwritetextformat_layout_GetLocaleName,
    dwritetextformat1_layout_SetVerticalGlyphOrientation,
    dwritetextformat1_layout_GetVerticalGlyphOrientation,
    dwritetextformat1_layout_SetLastLineWrapping,
    dwritetextformat1_layout_GetLastLineWrapping,
    dwritetextformat1_layout_SetOpticalAlignment,
    dwritetextformat1_layout_GetOpticalAlignment,
    dwritetextformat1_layout_SetFontFallback,
    dwritetextformat1_layout_GetFontFallback,
    dwritetextformat2_layout_SetLineSpacing,
    dwritetextformat2_layout_GetLineSpacing,
    dwritetextformat3_layout_SetFontAxisValues,
    dwritetextformat3_layout_GetFontAxisValueCount,
    dwritetextformat3_layout_GetFontAxisValues,
    dwritetextformat3_layout_GetAutomaticFontAxes,
    dwritetextformat3_layout_SetAutomaticFontAxes,
};

static HRESULT WINAPI dwritetextlayout_sink_QueryInterface(IDWriteTextAnalysisSink1 *iface,
    REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextAnalysisSink1) ||
        IsEqualIID(riid, &IID_IDWriteTextAnalysisSink) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteTextAnalysisSink1_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextlayout_sink_AddRef(IDWriteTextAnalysisSink1 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSink1(iface);
    return IDWriteTextLayout4_AddRef(&layout->IDWriteTextLayout4_iface);
}

static ULONG WINAPI dwritetextlayout_sink_Release(IDWriteTextAnalysisSink1 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSink1(iface);
    return IDWriteTextLayout4_Release(&layout->IDWriteTextLayout4_iface);
}

static HRESULT WINAPI dwritetextlayout_sink_SetScriptAnalysis(IDWriteTextAnalysisSink1 *iface,
    UINT32 position, UINT32 length, DWRITE_SCRIPT_ANALYSIS const* sa)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSink1(iface);
    struct layout_run *run;
    HRESULT hr;

    TRACE("[%u,%u) script=%u:%s\n", position, position + length, sa->script, debugstr_sa_script(sa->script));

    if (FAILED(hr = alloc_layout_run(LAYOUT_RUN_REGULAR, position, &run)))
        return hr;

    run->u.regular.descr.string = &layout->str[position];
    run->u.regular.descr.stringLength = length;
    run->u.regular.descr.textPosition = position;
    run->u.regular.sa = *sa;
    list_add_tail(&layout->runs, &run->entry);
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_sink_SetLineBreakpoints(IDWriteTextAnalysisSink1 *iface,
    UINT32 position, UINT32 length, DWRITE_LINE_BREAKPOINT const* breakpoints)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSink1(iface);

    if (position + length > layout->len)
        return E_FAIL;

    memcpy(&layout->nominal_breakpoints[position], breakpoints, length*sizeof(DWRITE_LINE_BREAKPOINT));
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_sink_SetBidiLevel(IDWriteTextAnalysisSink1 *iface, UINT32 position,
    UINT32 length, UINT8 explicitLevel, UINT8 resolvedLevel)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSink1(iface);
    struct layout_run *cur_run;
    HRESULT hr;

    TRACE("[%u,%u) %u %u\n", position, position + length, explicitLevel, resolvedLevel);

    LIST_FOR_EACH_ENTRY(cur_run, &layout->runs, struct layout_run, entry) {
        struct regular_layout_run *cur = &cur_run->u.regular;
        struct layout_run *run;

        if (cur_run->kind == LAYOUT_RUN_INLINE)
            continue;

        /* FIXME: levels are reported in a natural forward direction, so start loop from a run we ended on */
        if (position < cur->descr.textPosition || position >= cur->descr.textPosition + cur->descr.stringLength)
            continue;

        /* full hit - just set run level */
        if (cur->descr.textPosition == position && cur->descr.stringLength == length) {
            cur->run.bidiLevel = resolvedLevel;
            break;
        }

        /* current run is fully covered, move to next one */
        if (cur->descr.textPosition == position && cur->descr.stringLength < length) {
            cur->run.bidiLevel = resolvedLevel;
            position += cur->descr.stringLength;
            length -= cur->descr.stringLength;
            continue;
        }

        /* all fully covered runs are processed at this point, reuse existing run for remaining
           reported bidi range and add another run for the rest of original one */

        if (FAILED(hr = alloc_layout_run(LAYOUT_RUN_REGULAR, position + length, &run)))
            return hr;

        *run = *cur_run;
        run->u.regular.descr.textPosition = position + length;
        run->u.regular.descr.stringLength = cur->descr.stringLength - length;
        run->u.regular.descr.string = &layout->str[position + length];

        /* reduce existing run */
        cur->run.bidiLevel = resolvedLevel;
        cur->descr.stringLength = length;

        list_add_after(&cur_run->entry, &run->entry);
        break;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_sink_SetNumberSubstitution(IDWriteTextAnalysisSink1 *iface,
    UINT32 position, UINT32 length, IDWriteNumberSubstitution* substitution)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_sink_SetGlyphOrientation(IDWriteTextAnalysisSink1 *iface,
    UINT32 position, UINT32 length, DWRITE_GLYPH_ORIENTATION_ANGLE angle, UINT8 adjusted_bidi_level,
    BOOL is_sideways, BOOL is_rtl)
{
    return E_NOTIMPL;
}

static const IDWriteTextAnalysisSink1Vtbl dwritetextlayoutsinkvtbl = {
    dwritetextlayout_sink_QueryInterface,
    dwritetextlayout_sink_AddRef,
    dwritetextlayout_sink_Release,
    dwritetextlayout_sink_SetScriptAnalysis,
    dwritetextlayout_sink_SetLineBreakpoints,
    dwritetextlayout_sink_SetBidiLevel,
    dwritetextlayout_sink_SetNumberSubstitution,
    dwritetextlayout_sink_SetGlyphOrientation
};

static HRESULT WINAPI dwritetextlayout_source_QueryInterface(IDWriteTextAnalysisSource1 *iface,
    REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextAnalysisSource1) ||
        IsEqualIID(riid, &IID_IDWriteTextAnalysisSource) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteTextAnalysisSource1_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextlayout_source_AddRef(IDWriteTextAnalysisSource1 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSource1(iface);
    return IDWriteTextLayout4_AddRef(&layout->IDWriteTextLayout4_iface);
}

static ULONG WINAPI dwritetextlayout_source_Release(IDWriteTextAnalysisSource1 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSource1(iface);
    return IDWriteTextLayout4_Release(&layout->IDWriteTextLayout4_iface);
}

static HRESULT WINAPI dwritetextlayout_source_GetTextAtPosition(IDWriteTextAnalysisSource1 *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSource1(iface);

    TRACE("%p, %u, %p, %p.\n", iface, position, text, text_len);

    if (position < layout->text_source.length)
    {
        *text = &layout->str[position + layout->text_source.offset];
        *text_len = layout->text_source.length - position;
    }
    else
    {
        *text = NULL;
        *text_len = 0;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_source_GetTextBeforePosition(IDWriteTextAnalysisSource1 *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSource1(iface);

    TRACE("%p, %u, %p, %p.\n", iface, position, text, text_len);

    if (position && position < layout->text_source.length)
    {
        *text = &layout->str[layout->text_source.offset];
        *text_len = position;
    }
    else
    {
        *text = NULL;
        *text_len = 0;
    }

    return S_OK;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextlayout_source_GetParagraphReadingDirection(IDWriteTextAnalysisSource1 *iface)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSource1(iface);
    return IDWriteTextLayout4_GetReadingDirection(&layout->IDWriteTextLayout4_iface);
}

static HRESULT WINAPI dwritetextlayout_source_GetLocaleName(IDWriteTextAnalysisSource1 *iface,
    UINT32 position, UINT32* text_len, WCHAR const** locale)
{
    struct dwrite_textlayout *layout = impl_from_IDWriteTextAnalysisSource1(iface);
    struct layout_range *range, *next;
    unsigned int end;

    if (position < layout->text_source.length)
    {
        position += layout->text_source.offset;
        end = layout->text_source.offset + layout->text_source.length;

        range = get_layout_range_by_pos(layout, position);

        *locale = range->locale;
        *text_len = range->h.range.startPosition + range->h.range.length - position;

        next = LIST_ENTRY(list_next(&layout->ranges, &range->h.entry), struct layout_range, h.entry);
        while (next && next->h.range.startPosition < end && !wcscmp(range->locale, next->locale))
        {
            *text_len += next->h.range.length;
            next = LIST_ENTRY(list_next(&layout->ranges, &next->h.entry), struct layout_range, h.entry);
        }

        *text_len = min(*text_len, layout->text_source.length - position);
    }
    else
    {
        *locale = NULL;
        *text_len = 0;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_source_GetNumberSubstitution(IDWriteTextAnalysisSource1 *iface,
    UINT32 position, UINT32* text_len, IDWriteNumberSubstitution **substitution)
{
    FIXME("%u %p %p: stub\n", position, text_len, substitution);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_source_GetVerticalGlyphOrientation(IDWriteTextAnalysisSource1 *iface,
    UINT32 position, UINT32 *length, DWRITE_VERTICAL_GLYPH_ORIENTATION *orientation, UINT8 *bidi_level)
{
    FIXME("%u %p %p %p: stub\n", position, length, orientation, bidi_level);
    return E_NOTIMPL;
}

static const IDWriteTextAnalysisSource1Vtbl dwritetextlayoutsourcevtbl = {
    dwritetextlayout_source_QueryInterface,
    dwritetextlayout_source_AddRef,
    dwritetextlayout_source_Release,
    dwritetextlayout_source_GetTextAtPosition,
    dwritetextlayout_source_GetTextBeforePosition,
    dwritetextlayout_source_GetParagraphReadingDirection,
    dwritetextlayout_source_GetLocaleName,
    dwritetextlayout_source_GetNumberSubstitution,
    dwritetextlayout_source_GetVerticalGlyphOrientation
};

static HRESULT layout_format_from_textformat(struct dwrite_textlayout *layout, IDWriteTextFormat *format)
{
    struct dwrite_textformat *textformat;
    IDWriteTextFormat1 *format1;
    IDWriteTextFormat3 *format3;
    UINT32 len;
    HRESULT hr;

    if ((textformat = unsafe_impl_from_IDWriteTextFormat(format))) {
        layout->format = textformat->format;

        layout->format.locale = wcsdup(textformat->format.locale);
        layout->format.family_name = wcsdup(textformat->format.family_name);
        if (!layout->format.locale || !layout->format.family_name)
        {
            free(layout->format.locale);
            free(layout->format.family_name);
            return E_OUTOFMEMORY;
        }

        if (layout->format.trimmingsign)
            IDWriteInlineObject_AddRef(layout->format.trimmingsign);
        if (layout->format.collection)
            IDWriteFontCollection_AddRef(layout->format.collection);
        if (layout->format.fallback)
            IDWriteFontFallback_AddRef(layout->format.fallback);

        return S_OK;
    }

    layout->format.weight  = IDWriteTextFormat_GetFontWeight(format);
    layout->format.style   = IDWriteTextFormat_GetFontStyle(format);
    layout->format.stretch = IDWriteTextFormat_GetFontStretch(format);
    layout->format.fontsize= IDWriteTextFormat_GetFontSize(format);
    layout->format.tabstop = IDWriteTextFormat_GetIncrementalTabStop(format);
    layout->format.textalignment = IDWriteTextFormat_GetTextAlignment(format);
    layout->format.paralign = IDWriteTextFormat_GetParagraphAlignment(format);
    layout->format.wrapping = IDWriteTextFormat_GetWordWrapping(format);
    layout->format.readingdir = IDWriteTextFormat_GetReadingDirection(format);
    layout->format.flow = IDWriteTextFormat_GetFlowDirection(format);
    hr = IDWriteTextFormat_GetLineSpacing(format, &layout->format.spacing.method,
        &layout->format.spacing.height, &layout->format.spacing.baseline);
    if (FAILED(hr))
        return hr;

    hr = IDWriteTextFormat_GetTrimming(format, &layout->format.trimming, &layout->format.trimmingsign);
    if (FAILED(hr))
        return hr;

    /* locale name and length */
    len = IDWriteTextFormat_GetLocaleNameLength(format);
    if (!(layout->format.locale = malloc((len + 1) * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    hr = IDWriteTextFormat_GetLocaleName(format, layout->format.locale, len+1);
    if (FAILED(hr))
        return hr;
    layout->format.locale_len = len;

    /* font family name and length */
    len = IDWriteTextFormat_GetFontFamilyNameLength(format);
    if (!(layout->format.family_name = malloc((len + 1) * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    hr = IDWriteTextFormat_GetFontFamilyName(format, layout->format.family_name, len+1);
    if (FAILED(hr))
        return hr;
    layout->format.family_len = len;

    hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat1, (void**)&format1);
    if (hr == S_OK)
    {
        IDWriteTextFormat2 *format2;

        layout->format.vertical_orientation = IDWriteTextFormat1_GetVerticalGlyphOrientation(format1);
        layout->format.optical_alignment = IDWriteTextFormat1_GetOpticalAlignment(format1);
        IDWriteTextFormat1_GetFontFallback(format1, &layout->format.fallback);

        if (IDWriteTextFormat1_QueryInterface(format1, &IID_IDWriteTextFormat2, (void**)&format2) == S_OK) {
            IDWriteTextFormat2_GetLineSpacing(format2, &layout->format.spacing);
            IDWriteTextFormat2_Release(format2);
        }

        IDWriteTextFormat1_Release(format1);
    }

    hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat3, (void **)&format3);
    if (hr == S_OK)
    {
        layout->format.automatic_axes = IDWriteTextFormat3_GetAutomaticFontAxes(format3);
        IDWriteTextFormat3_Release(format3);
    }

    return IDWriteTextFormat_GetFontCollection(format, &layout->format.collection);
}

static HRESULT init_textlayout(const struct textlayout_desc *desc, struct dwrite_textlayout *layout)
{
    struct layout_range_header *range, *strike, *underline, *effect, *spacing, *typography;
    static const DWRITE_TEXT_RANGE r = { 0, ~0u };
    HRESULT hr;

    layout->IDWriteTextLayout4_iface.lpVtbl = &dwritetextlayoutvtbl;
    layout->IDWriteTextFormat3_iface.lpVtbl = &dwritetextformat3_layout_vtbl;
    layout->IDWriteTextAnalysisSink1_iface.lpVtbl = &dwritetextlayoutsinkvtbl;
    layout->IDWriteTextAnalysisSource1_iface.lpVtbl = &dwritetextlayoutsourcevtbl;
    layout->refcount = 1;
    layout->len = desc->length;
    layout->recompute = RECOMPUTE_EVERYTHING;
    list_init(&layout->eruns);
    list_init(&layout->inlineobjects);
    list_init(&layout->underlines);
    list_init(&layout->strikethrough);
    list_init(&layout->runs);
    list_init(&layout->ranges);
    list_init(&layout->strike_ranges);
    list_init(&layout->underline_ranges);
    list_init(&layout->effects);
    list_init(&layout->spacing);
    list_init(&layout->typographies);
    layout->metrics.layoutWidth = desc->max_width;
    layout->metrics.layoutHeight = desc->max_height;

    layout->str = heap_strdupnW(desc->string, desc->length);
    if (desc->length && !layout->str) {
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    hr = layout_format_from_textformat(layout, desc->format);
    if (FAILED(hr))
        goto fail;

    range = alloc_layout_range(layout, &r, LAYOUT_RANGE_REGULAR);
    strike = alloc_layout_range(layout, &r, LAYOUT_RANGE_STRIKETHROUGH);
    underline = alloc_layout_range(layout, &r, LAYOUT_RANGE_UNDERLINE);
    effect = alloc_layout_range(layout, &r, LAYOUT_RANGE_EFFECT);
    spacing = alloc_layout_range(layout, &r, LAYOUT_RANGE_SPACING);
    typography = alloc_layout_range(layout, &r, LAYOUT_RANGE_TYPOGRAPHY);
    if (!range || !strike || !effect || !spacing || !typography || !underline) {
        free_layout_range(range);
        free_layout_range(strike);
        free_layout_range(underline);
        free_layout_range(effect);
        free_layout_range(spacing);
        free_layout_range(typography);
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    layout->measuringmode = desc->is_gdi_compatible ? (desc->use_gdi_natural ? DWRITE_MEASURING_MODE_GDI_NATURAL :
            DWRITE_MEASURING_MODE_GDI_CLASSIC) : DWRITE_MEASURING_MODE_NATURAL;
    layout->ppdip = desc->ppdip;
    layout->transform = desc->transform ? *desc->transform : identity;

    layout->factory = desc->factory;
    IDWriteFactory7_AddRef(layout->factory);
    list_add_head(&layout->ranges, &range->entry);
    list_add_head(&layout->strike_ranges, &strike->entry);
    list_add_head(&layout->underline_ranges, &underline->entry);
    list_add_head(&layout->effects, &effect->entry);
    list_add_head(&layout->spacing, &spacing->entry);
    list_add_head(&layout->typographies, &typography->entry);

    if (FAILED(hr = IDWriteFactory5_GetSystemFontCollection((IDWriteFactory5 *)layout->factory, FALSE,
            (IDWriteFontCollection1 **)&layout->system_collection, FALSE)))
    {
        goto fail;
    }

    return S_OK;

fail:
    IDWriteTextLayout4_Release(&layout->IDWriteTextLayout4_iface);
    return hr;
}

HRESULT create_textlayout(const struct textlayout_desc *desc, IDWriteTextLayout **layout)
{
    struct dwrite_textlayout *object;
    HRESULT hr;

    *layout = NULL;

    if (desc->max_width < 0.0f || desc->max_height < 0.0f)
        return E_INVALIDARG;

    if (!desc->format || !desc->string)
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = init_textlayout(desc, object);
    if (hr == S_OK)
        *layout = (IDWriteTextLayout *)&object->IDWriteTextLayout4_iface;

    return hr;
}

static HRESULT WINAPI dwritetypography_QueryInterface(IDWriteTypography *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTypography) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteTypography_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetypography_AddRef(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);
    ULONG refcount = InterlockedIncrement(&typography->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritetypography_Release(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);
    ULONG refcount = InterlockedDecrement(&typography->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        free(typography->features);
        free(typography);
    }

    return refcount;
}

static HRESULT WINAPI dwritetypography_AddFontFeature(IDWriteTypography *iface, DWRITE_FONT_FEATURE feature)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("%p, %s, %u.\n", iface, debugstr_fourcc(feature.nameTag), feature.parameter);

    if (!dwrite_array_reserve((void **)&typography->features, &typography->capacity, typography->count + 1,
            sizeof(*typography->features)))
    {
        return E_OUTOFMEMORY;
    }

    typography->features[typography->count++] = feature;

    return S_OK;
}

static UINT32 WINAPI dwritetypography_GetFontFeatureCount(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("%p.\n", iface);

    return typography->count;
}

static HRESULT WINAPI dwritetypography_GetFontFeature(IDWriteTypography *iface, UINT32 index,
        DWRITE_FONT_FEATURE *feature)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("%p, %u, %p.\n", iface, index, feature);

    if (index >= typography->count)
        return E_INVALIDARG;

    *feature = typography->features[index];
    return S_OK;
}

static const IDWriteTypographyVtbl dwritetypographyvtbl = {
    dwritetypography_QueryInterface,
    dwritetypography_AddRef,
    dwritetypography_Release,
    dwritetypography_AddFontFeature,
    dwritetypography_GetFontFeatureCount,
    dwritetypography_GetFontFeature
};

HRESULT create_typography(IDWriteTypography **ret)
{
    struct dwrite_typography *typography;

    *ret = NULL;

    if (!(typography = calloc(1, sizeof(*typography))))
        return E_OUTOFMEMORY;

    typography->IDWriteTypography_iface.lpVtbl = &dwritetypographyvtbl;
    typography->refcount = 1;

    *ret = &typography->IDWriteTypography_iface;

    return S_OK;
}
