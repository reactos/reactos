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

#include "dwrite_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

struct dwrite_trimmingsign
{
    IDWriteInlineObject IDWriteInlineObject_iface;
    LONG refcount;

    IDWriteTextLayout *layout;
};

static inline struct dwrite_trimmingsign *impl_from_IDWriteInlineObject(IDWriteInlineObject *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_trimmingsign, IDWriteInlineObject_iface);
}

void release_format_data(struct dwrite_textformat_data *data)
{
    if (data->collection)
        IDWriteFontCollection_Release(data->collection);
    if (data->fallback)
        IDWriteFontFallback_Release(data->fallback);
    if (data->trimmingsign)
        IDWriteInlineObject_Release(data->trimmingsign);
    free(data->family_name);
    free(data->locale);
    free(data->axis_values);
}

static inline struct dwrite_textformat *impl_from_IDWriteTextFormat3(IDWriteTextFormat3 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textformat, IDWriteTextFormat3_iface);
}

HRESULT format_set_textalignment(struct dwrite_textformat_data *format, DWRITE_TEXT_ALIGNMENT alignment,
        BOOL *changed)
{
    if ((UINT32)alignment > DWRITE_TEXT_ALIGNMENT_JUSTIFIED)
        return E_INVALIDARG;
    if (changed) *changed = format->textalignment != alignment;
    format->textalignment = alignment;
    return S_OK;
}

HRESULT format_set_paralignment(struct dwrite_textformat_data *format, DWRITE_PARAGRAPH_ALIGNMENT alignment, BOOL *changed)
{
    if ((UINT32)alignment > DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
        return E_INVALIDARG;
    if (changed) *changed = format->paralign != alignment;
    format->paralign = alignment;
    return S_OK;
}

HRESULT format_set_readingdirection(struct dwrite_textformat_data *format, DWRITE_READING_DIRECTION direction, BOOL *changed)
{
    if ((UINT32)direction > DWRITE_READING_DIRECTION_BOTTOM_TO_TOP)
        return E_INVALIDARG;
    if (changed) *changed = format->readingdir != direction;
    format->readingdir = direction;
    return S_OK;
}

HRESULT format_set_wordwrapping(struct dwrite_textformat_data *format, DWRITE_WORD_WRAPPING wrapping, BOOL *changed)
{
    if ((UINT32)wrapping > DWRITE_WORD_WRAPPING_CHARACTER)
        return E_INVALIDARG;
    if (changed) *changed = format->wrapping != wrapping;
    format->wrapping = wrapping;
    return S_OK;
}

HRESULT format_set_flowdirection(struct dwrite_textformat_data *format, DWRITE_FLOW_DIRECTION direction, BOOL *changed)
{
    if ((UINT32)direction > DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT)
        return E_INVALIDARG;
    if (changed) *changed = format->flow != direction;
    format->flow = direction;
    return S_OK;
}

HRESULT format_set_trimming(struct dwrite_textformat_data *format, DWRITE_TRIMMING const *trimming,
        IDWriteInlineObject *trimming_sign, BOOL *changed)
{
    if (changed)
        *changed = FALSE;

    if ((UINT32)trimming->granularity > DWRITE_TRIMMING_GRANULARITY_WORD)
        return E_INVALIDARG;

    if (changed) {
        *changed = !!memcmp(&format->trimming, trimming, sizeof(*trimming));
        if (format->trimmingsign != trimming_sign)
            *changed = TRUE;
    }

    format->trimming = *trimming;
    if (format->trimmingsign)
        IDWriteInlineObject_Release(format->trimmingsign);
    format->trimmingsign = trimming_sign;
    if (format->trimmingsign)
        IDWriteInlineObject_AddRef(format->trimmingsign);
    return S_OK;
}

HRESULT format_set_linespacing(struct dwrite_textformat_data *format, DWRITE_LINE_SPACING const *spacing, BOOL *changed)
{
    if (spacing->height < 0.0f || spacing->leadingBefore < 0.0f || spacing->leadingBefore > 1.0f ||
        (UINT32)spacing->method > DWRITE_LINE_SPACING_METHOD_PROPORTIONAL)
        return E_INVALIDARG;

    if (changed)
        *changed = memcmp(spacing, &format->spacing, sizeof(*spacing));

    format->spacing = *spacing;
    return S_OK;
}

HRESULT format_set_font_axisvalues(struct dwrite_textformat_data *format, DWRITE_FONT_AXIS_VALUE const *axis_values,
        unsigned int num_values)
{
    free(format->axis_values);
    format->axis_values = NULL;
    format->axis_values_count = 0;

    if (num_values)
    {
        if (!(format->axis_values = calloc(num_values, sizeof(*axis_values))))
            return E_OUTOFMEMORY;
        memcpy(format->axis_values, axis_values, num_values * sizeof(*axis_values));
        format->axis_values_count = num_values;
    }

    return S_OK;
}

HRESULT format_get_font_axisvalues(struct dwrite_textformat_data *format, DWRITE_FONT_AXIS_VALUE *axis_values,
        unsigned int num_values)
{
    if (!format->axis_values_count)
    {
        if (num_values) memset(axis_values, 0, num_values * sizeof(*axis_values));
        return S_OK;
    }

    if (num_values < format->axis_values_count)
        return E_NOT_SUFFICIENT_BUFFER;

    memcpy(axis_values, format->axis_values, min(num_values, format->axis_values_count) * sizeof(*axis_values));

    return S_OK;
}

HRESULT format_get_fontfallback(const struct dwrite_textformat_data *format, IDWriteFontFallback **fallback)
{
    *fallback = format->fallback;
    if (*fallback)
        IDWriteFontFallback_AddRef(*fallback);
    return S_OK;
}

HRESULT format_set_fontfallback(struct dwrite_textformat_data *format, IDWriteFontFallback *fallback)
{
    if (format->fallback)
        IDWriteFontFallback_Release(format->fallback);
    format->fallback = fallback;
    if (fallback)
        IDWriteFontFallback_AddRef(fallback);
    return S_OK;
}

HRESULT format_set_optical_alignment(struct dwrite_textformat_data *format, DWRITE_OPTICAL_ALIGNMENT alignment)
{
    if ((UINT32)alignment > DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS)
        return E_INVALIDARG;
    format->optical_alignment = alignment;
    return S_OK;
}

HRESULT format_set_vertical_orientation(struct dwrite_textformat_data *format, DWRITE_VERTICAL_GLYPH_ORIENTATION orientation,
        BOOL *changed)
{
    if ((UINT32)orientation > DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED)
        return E_INVALIDARG;

    if (changed)
        *changed = format->vertical_orientation != orientation;

    format->vertical_orientation = orientation;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_QueryInterface(IDWriteTextFormat3 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTextFormat3) ||
        IsEqualIID(riid, &IID_IDWriteTextFormat2) ||
        IsEqualIID(riid, &IID_IDWriteTextFormat1) ||
        IsEqualIID(riid, &IID_IDWriteTextFormat)  ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteTextFormat3_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextformat_AddRef(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);
    ULONG refcount = InterlockedIncrement(&format->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritetextformat_Release(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);
    ULONG refcount = InterlockedDecrement(&format->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        release_format_data(&format->format);
        free(format);
    }

    return refcount;
}

static HRESULT WINAPI dwritetextformat_SetTextAlignment(IDWriteTextFormat3 *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, alignment);

    return format_set_textalignment(&format->format, alignment, NULL);
}

static HRESULT WINAPI dwritetextformat_SetParagraphAlignment(IDWriteTextFormat3 *iface,
        DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, alignment);

    return format_set_paralignment(&format->format, alignment, NULL);
}

static HRESULT WINAPI dwritetextformat_SetWordWrapping(IDWriteTextFormat3 *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, wrapping);

    return format_set_wordwrapping(&format->format, wrapping, NULL);
}

static HRESULT WINAPI dwritetextformat_SetReadingDirection(IDWriteTextFormat3 *iface, DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, direction);

    return format_set_readingdirection(&format->format, direction, NULL);
}

static HRESULT WINAPI dwritetextformat_SetFlowDirection(IDWriteTextFormat3 *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, direction);

    return format_set_flowdirection(&format->format, direction, NULL);
}

static HRESULT WINAPI dwritetextformat_SetIncrementalTabStop(IDWriteTextFormat3 *iface, float tabstop)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %f.\n", iface, tabstop);

    if (tabstop <= 0.0f)
        return E_INVALIDARG;

    format->format.tabstop = tabstop;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetTrimming(IDWriteTextFormat3 *iface, DWRITE_TRIMMING const *trimming,
        IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %p.\n", iface, trimming, trimming_sign);

    return format_set_trimming(&format->format, trimming, trimming_sign, NULL);
}

static HRESULT WINAPI dwritetextformat_SetLineSpacing(IDWriteTextFormat3 *iface, DWRITE_LINE_SPACING_METHOD method,
        float height, float baseline)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);
    DWRITE_LINE_SPACING spacing;

    TRACE("%p, %d, %f, %f.\n", iface, method, height, baseline);

    spacing = format->format.spacing;
    spacing.method = method;
    spacing.height = height;
    spacing.baseline = baseline;

    return format_set_linespacing(&format->format, &spacing, NULL);
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextformat_GetTextAlignment(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.textalignment;
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextformat_GetParagraphAlignment(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.paralign;
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextformat_GetWordWrapping(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.wrapping;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextformat_GetReadingDirection(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.readingdir;
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextformat_GetFlowDirection(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.flow;
}

static float WINAPI dwritetextformat_GetIncrementalTabStop(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.tabstop;
}

static HRESULT WINAPI dwritetextformat_GetTrimming(IDWriteTextFormat3 *iface, DWRITE_TRIMMING *options,
        IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %p.\n", iface, options, trimming_sign);

    *options = format->format.trimming;
    if ((*trimming_sign = format->format.trimmingsign))
        IDWriteInlineObject_AddRef(*trimming_sign);

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_GetLineSpacing(IDWriteTextFormat3 *iface, DWRITE_LINE_SPACING_METHOD *method,
        float *spacing, float *baseline)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %p, %p.\n", iface, method, spacing, baseline);

    *method = format->format.spacing.method;
    *spacing = format->format.spacing.height;
    *baseline = format->format.spacing.baseline;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_GetFontCollection(IDWriteTextFormat3 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p.\n", iface, collection);

    *collection = format->format.collection;
    IDWriteFontCollection_AddRef(*collection);

    return S_OK;
}

static UINT32 WINAPI dwritetextformat_GetFontFamilyNameLength(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.family_len;
}

static HRESULT WINAPI dwritetextformat_GetFontFamilyName(IDWriteTextFormat3 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %u.\n", iface, name, size);

    if (size <= format->format.family_len)
        return E_NOT_SUFFICIENT_BUFFER;
    wcscpy(name, format->format.family_name);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextformat_GetFontWeight(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.weight;
}

static DWRITE_FONT_STYLE WINAPI dwritetextformat_GetFontStyle(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.style;
}

static DWRITE_FONT_STRETCH WINAPI dwritetextformat_GetFontStretch(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.stretch;
}

static float WINAPI dwritetextformat_GetFontSize(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.fontsize;
}

static UINT32 WINAPI dwritetextformat_GetLocaleNameLength(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.locale_len;
}

static HRESULT WINAPI dwritetextformat_GetLocaleName(IDWriteTextFormat3 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p %u.\n", iface, name, size);

    if (size <= format->format.locale_len)
        return E_NOT_SUFFICIENT_BUFFER;
    wcscpy(name, format->format.locale);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat1_SetVerticalGlyphOrientation(IDWriteTextFormat3 *iface,
        DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, orientation);

    return format_set_vertical_orientation(&format->format, orientation, NULL);
}

static DWRITE_VERTICAL_GLYPH_ORIENTATION WINAPI dwritetextformat1_GetVerticalGlyphOrientation(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.vertical_orientation;
}

static HRESULT WINAPI dwritetextformat1_SetLastLineWrapping(IDWriteTextFormat3 *iface, BOOL lastline_wrapping_enabled)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, lastline_wrapping_enabled);

    format->format.last_line_wrapping = !!lastline_wrapping_enabled;
    return S_OK;
}

static BOOL WINAPI dwritetextformat1_GetLastLineWrapping(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.last_line_wrapping;
}

static HRESULT WINAPI dwritetextformat1_SetOpticalAlignment(IDWriteTextFormat3 *iface, DWRITE_OPTICAL_ALIGNMENT alignment)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, alignment);

    return format_set_optical_alignment(&format->format, alignment);
}

static DWRITE_OPTICAL_ALIGNMENT WINAPI dwritetextformat1_GetOpticalAlignment(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.optical_alignment;
}

static HRESULT WINAPI dwritetextformat1_SetFontFallback(IDWriteTextFormat3 *iface, IDWriteFontFallback *fallback)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p.\n", iface, fallback);

    return format_set_fontfallback(&format->format, fallback);
}

static HRESULT WINAPI dwritetextformat1_GetFontFallback(IDWriteTextFormat3 *iface, IDWriteFontFallback **fallback)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p.\n", iface, fallback);

    return format_get_fontfallback(&format->format, fallback);
}

static HRESULT WINAPI dwritetextformat2_SetLineSpacing(IDWriteTextFormat3 *iface, DWRITE_LINE_SPACING const *spacing)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p.\n", iface, spacing);

    return format_set_linespacing(&format->format, spacing, NULL);
}

static HRESULT WINAPI dwritetextformat2_GetLineSpacing(IDWriteTextFormat3 *iface, DWRITE_LINE_SPACING *spacing)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p.\n", iface, spacing);

    *spacing = format->format.spacing;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat3_SetFontAxisValues(IDWriteTextFormat3 *iface,
        DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_values)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %u.\n", iface, axis_values, num_values);

    return format_set_font_axisvalues(&format->format, axis_values, num_values);
}

static UINT32 WINAPI dwritetextformat3_GetFontAxisValueCount(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.axis_values_count;
}

static HRESULT WINAPI dwritetextformat3_GetFontAxisValues(IDWriteTextFormat3 *iface,
        DWRITE_FONT_AXIS_VALUE *axis_values, UINT32 num_values)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %p, %u.\n", iface, axis_values, num_values);

    return format_get_font_axisvalues(&format->format, axis_values, num_values);
}

static DWRITE_AUTOMATIC_FONT_AXES WINAPI dwritetextformat3_GetAutomaticFontAxes(IDWriteTextFormat3 *iface)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p.\n", iface);

    return format->format.automatic_axes;
}

static HRESULT WINAPI dwritetextformat3_SetAutomaticFontAxes(IDWriteTextFormat3 *iface, DWRITE_AUTOMATIC_FONT_AXES axes)
{
    struct dwrite_textformat *format = impl_from_IDWriteTextFormat3(iface);

    TRACE("%p, %d.\n", iface, axes);

    format->format.automatic_axes = axes;

    return S_OK;
}

static const IDWriteTextFormat3Vtbl dwritetextformatvtbl =
{
    dwritetextformat_QueryInterface,
    dwritetextformat_AddRef,
    dwritetextformat_Release,
    dwritetextformat_SetTextAlignment,
    dwritetextformat_SetParagraphAlignment,
    dwritetextformat_SetWordWrapping,
    dwritetextformat_SetReadingDirection,
    dwritetextformat_SetFlowDirection,
    dwritetextformat_SetIncrementalTabStop,
    dwritetextformat_SetTrimming,
    dwritetextformat_SetLineSpacing,
    dwritetextformat_GetTextAlignment,
    dwritetextformat_GetParagraphAlignment,
    dwritetextformat_GetWordWrapping,
    dwritetextformat_GetReadingDirection,
    dwritetextformat_GetFlowDirection,
    dwritetextformat_GetIncrementalTabStop,
    dwritetextformat_GetTrimming,
    dwritetextformat_GetLineSpacing,
    dwritetextformat_GetFontCollection,
    dwritetextformat_GetFontFamilyNameLength,
    dwritetextformat_GetFontFamilyName,
    dwritetextformat_GetFontWeight,
    dwritetextformat_GetFontStyle,
    dwritetextformat_GetFontStretch,
    dwritetextformat_GetFontSize,
    dwritetextformat_GetLocaleNameLength,
    dwritetextformat_GetLocaleName,
    dwritetextformat1_SetVerticalGlyphOrientation,
    dwritetextformat1_GetVerticalGlyphOrientation,
    dwritetextformat1_SetLastLineWrapping,
    dwritetextformat1_GetLastLineWrapping,
    dwritetextformat1_SetOpticalAlignment,
    dwritetextformat1_GetOpticalAlignment,
    dwritetextformat1_SetFontFallback,
    dwritetextformat1_GetFontFallback,
    dwritetextformat2_SetLineSpacing,
    dwritetextformat2_GetLineSpacing,
    dwritetextformat3_SetFontAxisValues,
    dwritetextformat3_GetFontAxisValueCount,
    dwritetextformat3_GetFontAxisValues,
    dwritetextformat3_GetAutomaticFontAxes,
    dwritetextformat3_SetAutomaticFontAxes,
};

struct dwrite_textformat *unsafe_impl_from_IDWriteTextFormat(IDWriteTextFormat *iface)
{
    return (iface->lpVtbl == (IDWriteTextFormatVtbl *)&dwritetextformatvtbl) ?
        CONTAINING_RECORD(iface, struct dwrite_textformat, IDWriteTextFormat3_iface) : NULL;
}

HRESULT create_text_format(const WCHAR *family_name, IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight,
        DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch, float size, const WCHAR *locale,
        REFIID riid, void **out)
{
    struct dwrite_textformat *object;
    HRESULT hr;

    *out = NULL;

    if (size <= 0.0f)
        return E_INVALIDARG;

    if ((UINT32)weight > DWRITE_FONT_WEIGHT_ULTRA_BLACK
        || stretch == DWRITE_FONT_STRETCH_UNDEFINED
        || (UINT32)stretch > DWRITE_FONT_STRETCH_ULTRA_EXPANDED
        || (UINT32)style > DWRITE_FONT_STYLE_ITALIC)
    {
        return E_INVALIDARG;
    }

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteTextFormat3_iface.lpVtbl = &dwritetextformatvtbl;
    object->refcount = 1;
    object->format.family_name = wcsdup(family_name);
    object->format.family_len = wcslen(family_name);
    object->format.locale = wcsdup(locale);
    object->format.locale_len = wcslen(locale);
    /* Force locale name to lower case, layout will inherit this modified value. */
    wcslwr(object->format.locale);
    object->format.weight = weight;
    object->format.style = style;
    object->format.fontsize = size;
    object->format.tabstop = 4.0f * size;
    object->format.stretch = stretch;
    object->format.last_line_wrapping = TRUE;
    object->format.collection = collection;
    IDWriteFontCollection_AddRef(object->format.collection);

    hr = IDWriteTextFormat3_QueryInterface(&object->IDWriteTextFormat3_iface, riid, out);
    IDWriteTextFormat3_Release(&object->IDWriteTextFormat3_iface);

    return hr;
}

static HRESULT WINAPI dwritetrimmingsign_QueryInterface(IDWriteInlineObject *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteInlineObject)) {
        *obj = iface;
        IDWriteInlineObject_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetrimmingsign_AddRef(IDWriteInlineObject *iface)
{
    struct dwrite_trimmingsign *sign = impl_from_IDWriteInlineObject(iface);
    ULONG refcount = InterlockedIncrement(&sign->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritetrimmingsign_Release(IDWriteInlineObject *iface)
{
    struct dwrite_trimmingsign *sign = impl_from_IDWriteInlineObject(iface);
    ULONG refcount = InterlockedDecrement(&sign->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteTextLayout_Release(sign->layout);
        free(sign);
    }

    return refcount;
}

static HRESULT WINAPI dwritetrimmingsign_Draw(IDWriteInlineObject *iface, void *context, IDWriteTextRenderer *renderer,
        float originX, float originY, BOOL is_sideways, BOOL is_rtl, IUnknown *effect)
{
    struct dwrite_trimmingsign *sign = impl_from_IDWriteInlineObject(iface);
    DWRITE_LINE_METRICS line;
    UINT32 line_count;

    TRACE("%p, %p, %p, %.2f, %.2f, %d, %d, %p.\n", iface, context, renderer, originX, originY,
            is_sideways, is_rtl, effect);

    IDWriteTextLayout_GetLineMetrics(sign->layout, &line, 1, &line_count);
    return IDWriteTextLayout_Draw(sign->layout, context, renderer, originX, originY - line.baseline);
}

static HRESULT WINAPI dwritetrimmingsign_GetMetrics(IDWriteInlineObject *iface, DWRITE_INLINE_OBJECT_METRICS *ret)
{
    struct dwrite_trimmingsign *sign = impl_from_IDWriteInlineObject(iface);
    DWRITE_TEXT_METRICS metrics;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, ret);

    hr = IDWriteTextLayout_GetMetrics(sign->layout, &metrics);
    if (FAILED(hr))
    {
        memset(ret, 0, sizeof(*ret));
        return hr;
    }

    ret->width = metrics.width;
    ret->height = 0.0f;
    ret->baseline = 0.0f;
    ret->supportsSideways = FALSE;
    return S_OK;
}

static HRESULT WINAPI dwritetrimmingsign_GetOverhangMetrics(IDWriteInlineObject *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    struct dwrite_trimmingsign *sign = impl_from_IDWriteInlineObject(iface);

    TRACE("%p, %p.\n", iface, overhangs);

    return IDWriteTextLayout_GetOverhangMetrics(sign->layout, overhangs);
}

static HRESULT WINAPI dwritetrimmingsign_GetBreakConditions(IDWriteInlineObject *iface, DWRITE_BREAK_CONDITION *before,
        DWRITE_BREAK_CONDITION *after)
{
    TRACE("%p, %p, %p.\n", iface, before, after);

    *before = *after = DWRITE_BREAK_CONDITION_NEUTRAL;
    return S_OK;
}

static const IDWriteInlineObjectVtbl dwritetrimmingsignvtbl =
{
    dwritetrimmingsign_QueryInterface,
    dwritetrimmingsign_AddRef,
    dwritetrimmingsign_Release,
    dwritetrimmingsign_Draw,
    dwritetrimmingsign_GetMetrics,
    dwritetrimmingsign_GetOverhangMetrics,
    dwritetrimmingsign_GetBreakConditions
};

static inline BOOL is_reading_direction_horz(DWRITE_READING_DIRECTION direction)
{
    return (direction == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT) ||
           (direction == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
}

static inline BOOL is_reading_direction_vert(DWRITE_READING_DIRECTION direction)
{
    return (direction == DWRITE_READING_DIRECTION_TOP_TO_BOTTOM) ||
           (direction == DWRITE_READING_DIRECTION_BOTTOM_TO_TOP);
}

static inline BOOL is_flow_direction_horz(DWRITE_FLOW_DIRECTION direction)
{
    return (direction == DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT) ||
           (direction == DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT);
}

static inline BOOL is_flow_direction_vert(DWRITE_FLOW_DIRECTION direction)
{
    return (direction == DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM) ||
           (direction == DWRITE_FLOW_DIRECTION_BOTTOM_TO_TOP);
}

HRESULT create_trimmingsign(IDWriteFactory7 *factory, IDWriteTextFormat *format, IDWriteInlineObject **sign)
{
    struct dwrite_trimmingsign *object;
    DWRITE_READING_DIRECTION reading;
    DWRITE_FLOW_DIRECTION flow;
    HRESULT hr;

    *sign = NULL;

    if (!format)
        return E_INVALIDARG;

    /* Validate reading/flow direction here, layout creation won't complain about
       invalid combinations. */
    reading = IDWriteTextFormat_GetReadingDirection(format);
    flow = IDWriteTextFormat_GetFlowDirection(format);

    if ((is_reading_direction_horz(reading) && is_flow_direction_horz(flow)) ||
        (is_reading_direction_vert(reading) && is_flow_direction_vert(flow)))
        return DWRITE_E_FLOWDIRECTIONCONFLICTS;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteInlineObject_iface.lpVtbl = &dwritetrimmingsignvtbl;
    object->refcount = 1;

    hr = IDWriteFactory7_CreateTextLayout(factory, L"\x2026", 1, format, 0.0f, 0.0f, &object->layout);
    if (FAILED(hr))
    {
        free(object);
        return hr;
    }

    IDWriteTextLayout_SetWordWrapping(object->layout, DWRITE_WORD_WRAPPING_NO_WRAP);
    IDWriteTextLayout_SetParagraphAlignment(object->layout, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    IDWriteTextLayout_SetTextAlignment(object->layout, DWRITE_TEXT_ALIGNMENT_LEADING);

    *sign = &object->IDWriteInlineObject_iface;

    return S_OK;
}
