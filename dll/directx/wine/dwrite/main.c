/*
 *    DWrite
 *
 * Copyright 2012 Nikolay Sivov for CodeWeavers
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
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "initguid.h"

#include "dwrite_private.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

HMODULE dwrite_module = 0;
static IDWriteFactory7 *shared_factory;
static void release_shared_factory(IDWriteFactory7 *factory);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        dwrite_module = hinstDLL;
        DisableThreadLibraryCalls( hinstDLL );
        if (!__wine_init_unix_call())
            UNIX_CALL(process_attach, NULL);
        init_local_fontfile_loader();
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        release_shared_factory(shared_factory);
        release_system_fallback_data();
        UNIX_CALL(process_detach, NULL);
    }
    return TRUE;
}

struct renderingparams
{
    IDWriteRenderingParams3 IDWriteRenderingParams3_iface;
    LONG refcount;

    float gamma;
    float contrast;
    float grayscalecontrast;
    float cleartype_level;
    DWRITE_PIXEL_GEOMETRY geometry;
    DWRITE_RENDERING_MODE1 mode;
    DWRITE_GRID_FIT_MODE gridfit;
};

static inline struct renderingparams *impl_from_IDWriteRenderingParams3(IDWriteRenderingParams3 *iface)
{
    return CONTAINING_RECORD(iface, struct renderingparams, IDWriteRenderingParams3_iface);
}

static HRESULT WINAPI renderingparams_QueryInterface(IDWriteRenderingParams3 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteRenderingParams3) ||
        IsEqualIID(riid, &IID_IDWriteRenderingParams2) ||
        IsEqualIID(riid, &IID_IDWriteRenderingParams1) ||
        IsEqualIID(riid, &IID_IDWriteRenderingParams) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteRenderingParams3_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI renderingparams_AddRef(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);
    ULONG refcount = InterlockedIncrement(&params->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI renderingparams_Release(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);
    ULONG refcount = InterlockedDecrement(&params->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
        free(params);

    return refcount;
}

static float WINAPI renderingparams_GetGamma(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);

    TRACE("%p.\n", iface);

    return params->gamma;
}

static float WINAPI renderingparams_GetEnhancedContrast(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);

    TRACE("%p.\n", iface);

    return params->contrast;
}

static float WINAPI renderingparams_GetClearTypeLevel(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);

    TRACE("%p.\n", iface);

    return params->cleartype_level;
}

static DWRITE_PIXEL_GEOMETRY WINAPI renderingparams_GetPixelGeometry(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);

    TRACE("%p.\n", iface);

    return params->geometry;
}

static DWRITE_RENDERING_MODE rendering_mode_from_mode1(DWRITE_RENDERING_MODE1 mode)
{
    static const DWRITE_RENDERING_MODE rendering_modes[] = {
        DWRITE_RENDERING_MODE_DEFAULT,           /* DWRITE_RENDERING_MODE1_DEFAULT */
        DWRITE_RENDERING_MODE_ALIASED,           /* DWRITE_RENDERING_MODE1_ALIASED */
        DWRITE_RENDERING_MODE_GDI_CLASSIC,       /* DWRITE_RENDERING_MODE1_GDI_CLASSIC */
        DWRITE_RENDERING_MODE_GDI_NATURAL,       /* DWRITE_RENDERING_MODE1_GDI_NATURAL */
        DWRITE_RENDERING_MODE_NATURAL,           /* DWRITE_RENDERING_MODE1_NATURAL */
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, /* DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC */
        DWRITE_RENDERING_MODE_OUTLINE,           /* DWRITE_RENDERING_MODE1_OUTLINE */
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, /* DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC_DOWNSAMPLED */
    };

    return rendering_modes[mode];
}

static DWRITE_RENDERING_MODE WINAPI renderingparams_GetRenderingMode(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);

    TRACE("%p.\n", iface);

    return rendering_mode_from_mode1(params->mode);
}

static float WINAPI renderingparams1_GetGrayscaleEnhancedContrast(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);

    TRACE("%p.\n", iface);

    return params->grayscalecontrast;
}

static DWRITE_GRID_FIT_MODE WINAPI renderingparams2_GetGridFitMode(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);

    TRACE("%p.\n", iface);

    return params->gridfit;
}

static DWRITE_RENDERING_MODE1 WINAPI renderingparams3_GetRenderingMode1(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *params = impl_from_IDWriteRenderingParams3(iface);

    TRACE("%p.\n", iface);

    return params->mode;
}

static const IDWriteRenderingParams3Vtbl renderingparamsvtbl =
{
    renderingparams_QueryInterface,
    renderingparams_AddRef,
    renderingparams_Release,
    renderingparams_GetGamma,
    renderingparams_GetEnhancedContrast,
    renderingparams_GetClearTypeLevel,
    renderingparams_GetPixelGeometry,
    renderingparams_GetRenderingMode,
    renderingparams1_GetGrayscaleEnhancedContrast,
    renderingparams2_GetGridFitMode,
    renderingparams3_GetRenderingMode1
};

static HRESULT create_renderingparams(float gamma, float contrast, float grayscalecontrast, float cleartype_level,
        DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE1 mode, DWRITE_GRID_FIT_MODE gridfit,
        IDWriteRenderingParams3 **params)
{
    struct renderingparams *object;

    *params = NULL;

    if (gamma <= 0.0f || contrast < 0.0f || grayscalecontrast < 0.0f || cleartype_level < 0.0f)
        return E_INVALIDARG;

    if ((UINT32)gridfit > DWRITE_GRID_FIT_MODE_ENABLED || (UINT32)geometry > DWRITE_PIXEL_GEOMETRY_BGR)
        return E_INVALIDARG;

    if (!(object = malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteRenderingParams3_iface.lpVtbl = &renderingparamsvtbl;
    object->refcount = 1;

    object->gamma = gamma;
    object->contrast = contrast;
    object->grayscalecontrast = grayscalecontrast;
    object->cleartype_level = cleartype_level;
    object->geometry = geometry;
    object->mode = mode;
    object->gridfit = gridfit;

    *params = &object->IDWriteRenderingParams3_iface;

    return S_OK;
}

struct localizedpair {
    WCHAR *locale;
    WCHAR *string;
};

struct localizedstrings
{
    IDWriteLocalizedStrings IDWriteLocalizedStrings_iface;
    LONG refcount;

    struct localizedpair *data;
    size_t size;
    size_t count;
};

static inline struct localizedstrings *impl_from_IDWriteLocalizedStrings(IDWriteLocalizedStrings *iface)
{
    return CONTAINING_RECORD(iface, struct localizedstrings, IDWriteLocalizedStrings_iface);
}

static HRESULT WINAPI localizedstrings_QueryInterface(IDWriteLocalizedStrings *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteLocalizedStrings))
    {
        *obj = iface;
        IDWriteLocalizedStrings_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI localizedstrings_AddRef(IDWriteLocalizedStrings *iface)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);
    ULONG refcount = InterlockedIncrement(&strings->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI localizedstrings_Release(IDWriteLocalizedStrings *iface)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);
    ULONG refcount = InterlockedDecrement(&strings->refcount);
    size_t i;

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < strings->count; ++i)
        {
            free(strings->data[i].locale);
            free(strings->data[i].string);
        }

        free(strings->data);
        free(strings);
    }

    return refcount;
}

static UINT32 WINAPI localizedstrings_GetCount(IDWriteLocalizedStrings *iface)
{
    TRACE("%p.\n", iface);

    return get_localizedstrings_count(iface);
}

static HRESULT WINAPI localizedstrings_FindLocaleName(IDWriteLocalizedStrings *iface,
    WCHAR const *locale_name, UINT32 *index, BOOL *exists)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);
    size_t i;

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_w(locale_name), index, exists);

    *exists = FALSE;
    *index = ~0;

    for (i = 0; i < strings->count; ++i)
    {
        if (!wcsicmp(strings->data[i].locale, locale_name))
        {
            *exists = TRUE;
            *index = i;
            break;
        }
    }

    return S_OK;
}

static HRESULT WINAPI localizedstrings_GetLocaleNameLength(IDWriteLocalizedStrings *iface, UINT32 index, UINT32 *length)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("%p, %u, %p.\n", iface, index, length);

    if (index >= strings->count)
    {
        *length = (UINT32)-1;
        return E_FAIL;
    }

    *length = wcslen(strings->data[index].locale);
    return S_OK;
}

static HRESULT WINAPI localizedstrings_GetLocaleName(IDWriteLocalizedStrings *iface, UINT32 index, WCHAR *buffer, UINT32 size)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("%p, %u, %p, %u.\n", iface, index, buffer, size);

    if (index >= strings->count)
    {
        if (buffer) *buffer = 0;
        return E_FAIL;
    }

    if (size < wcslen(strings->data[index].locale) + 1)
    {
        if (buffer) *buffer = 0;
        return E_NOT_SUFFICIENT_BUFFER;
    }

    wcscpy(buffer, strings->data[index].locale);
    return S_OK;
}

static HRESULT WINAPI localizedstrings_GetStringLength(IDWriteLocalizedStrings *iface, UINT32 index, UINT32 *length)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("%p, %u, %p.\n", iface, index, length);

    if (index >= strings->count)
    {
        *length = ~0u;
        return E_FAIL;
    }

    *length = wcslen(strings->data[index].string);
    return S_OK;
}

static HRESULT WINAPI localizedstrings_GetString(IDWriteLocalizedStrings *iface, UINT32 index, WCHAR *buffer, UINT32 size)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("%p, %u, %p, %u.\n", iface, index, buffer, size);

    if (index >= strings->count)
    {
        if (buffer) *buffer = 0;
        return E_FAIL;
    }

    if (size < wcslen(strings->data[index].string) + 1)
    {
        if (buffer) *buffer = 0;
        return E_NOT_SUFFICIENT_BUFFER;
    }

    wcscpy(buffer, strings->data[index].string);
    return S_OK;
}

static const IDWriteLocalizedStringsVtbl localizedstringsvtbl =
{
    localizedstrings_QueryInterface,
    localizedstrings_AddRef,
    localizedstrings_Release,
    localizedstrings_GetCount,
    localizedstrings_FindLocaleName,
    localizedstrings_GetLocaleNameLength,
    localizedstrings_GetLocaleName,
    localizedstrings_GetStringLength,
    localizedstrings_GetString
};

HRESULT create_localizedstrings(IDWriteLocalizedStrings **strings)
{
    struct localizedstrings *object;

    *strings = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteLocalizedStrings_iface.lpVtbl = &localizedstringsvtbl;
    object->refcount = 1;

    *strings = &object->IDWriteLocalizedStrings_iface;

    return S_OK;
}

HRESULT add_localizedstring(IDWriteLocalizedStrings *iface, const WCHAR *locale, const WCHAR *string)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);
    size_t i, count = strings->count;

    /* Make sure there's no duplicates, unless it's empty. This is used by dlng/slng entries of 'meta' table. */
    if (*locale)
    {
        for (i = 0; i < count; i++)
            if (!wcsicmp(strings->data[i].locale, locale))
                return S_OK;
    }

    if (!dwrite_array_reserve((void **)&strings->data, &strings->size, strings->count + 1, sizeof(*strings->data)))
        return E_OUTOFMEMORY;

    strings->data[count].locale = wcsdup(locale);
    strings->data[count].string = wcsdup(string);
    if (!strings->data[count].locale || !strings->data[count].string)
    {
        free(strings->data[count].locale);
        free(strings->data[count].string);
        return E_OUTOFMEMORY;
    }
    wcslwr(strings->data[count].locale);

    strings->count++;

    return S_OK;
}

HRESULT clone_localizedstrings(IDWriteLocalizedStrings *iface, IDWriteLocalizedStrings **ret)
{
    struct localizedstrings *strings, *strings_clone;
    size_t i;

    *ret = NULL;

    if (!iface)
        return S_FALSE;

    strings = impl_from_IDWriteLocalizedStrings(iface);
    if (!(strings_clone = calloc(1, sizeof(*strings_clone))))
        return E_OUTOFMEMORY;

    if (!dwrite_array_reserve((void **)&strings_clone->data, &strings_clone->size, strings->count,
            sizeof(*strings_clone->data)))
    {
        free(strings_clone);
        return E_OUTOFMEMORY;
    }

    strings_clone->IDWriteLocalizedStrings_iface.lpVtbl = &localizedstringsvtbl;
    strings_clone->refcount = 1;
    strings_clone->count = strings->count;

    for (i = 0; i < strings_clone->count; ++i)
    {
        strings_clone->data[i].locale = wcsdup(strings->data[i].locale);
        strings_clone->data[i].string = wcsdup(strings->data[i].string);
    }

    *ret = &strings_clone->IDWriteLocalizedStrings_iface;

    return S_OK;
}

void set_en_localizedstring(IDWriteLocalizedStrings *iface, const WCHAR *string)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);
    UINT32 i;

    for (i = 0; i < strings->count; i++)
    {
        if (!wcsicmp(strings->data[i].locale, L"en-US"))
        {
            free(strings->data[i].string);
            strings->data[i].string = wcsdup(string);
            break;
        }
    }
}

static int __cdecl localizedstrings_sorting_compare(const void *left, const void *right)
{
    const struct localizedpair *_l = left, *_r = right;

    return wcscmp(_l->locale, _r->locale);
};

void sort_localizedstrings(IDWriteLocalizedStrings *iface)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);

    qsort(strings->data, strings->count, sizeof(*strings->data), localizedstrings_sorting_compare);
}

unsigned int get_localizedstrings_count(IDWriteLocalizedStrings *iface)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);
    return strings->count;
}

BOOL localizedstrings_contains(IDWriteLocalizedStrings *iface, const WCHAR *str)
{
    struct localizedstrings *strings = impl_from_IDWriteLocalizedStrings(iface);
    unsigned int i;

    for (i = 0; i < strings->count; ++i)
    {
        if (!wcsicmp(strings->data[i].string, str)) return TRUE;
    }

    return FALSE;
}

struct collectionloader
{
    struct list entry;
    IDWriteFontCollectionLoader *loader;
};

struct fileloader
{
    struct list entry;
    struct list fontfaces;
    IDWriteFontFileLoader *loader;
};

struct dwritefactory
{
    IDWriteFactory7 IDWriteFactory7_iface;
    LONG refcount;

    IDWriteFontCollection *system_collections[DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE + 1];
    IDWriteFontCollection1 *eudc_collection;
    IDWriteGdiInterop1 *gdiinterop;
    IDWriteFontFallback1 *fallback;

    IDWriteFontFileLoader *localfontfileloader;
    struct list localfontfaces;

    struct list collection_loaders;
    struct list file_loaders;

    CRITICAL_SECTION cs;
};

static inline struct dwritefactory *impl_from_IDWriteFactory7(IDWriteFactory7 *iface)
{
    return CONTAINING_RECORD(iface, struct dwritefactory, IDWriteFactory7_iface);
}

static void release_fontface_cache(struct list *fontfaces)
{
    struct fontfacecached *fontface, *fontface2;

    LIST_FOR_EACH_ENTRY_SAFE(fontface, fontface2, fontfaces, struct fontfacecached, entry) {
        list_remove(&fontface->entry);
        fontface_detach_from_cache(fontface->fontface);
        free(fontface);
    }
}

static void release_fileloader(struct fileloader *fileloader)
{
    list_remove(&fileloader->entry);
    release_fontface_cache(&fileloader->fontfaces);
    IDWriteFontFileLoader_Release(fileloader->loader);
    free(fileloader);
}

static void release_dwritefactory(struct dwritefactory *factory)
{
    struct fileloader *fileloader, *fileloader2;
    struct collectionloader *loader, *loader2;
    unsigned int i;

    EnterCriticalSection(&factory->cs);
    release_fontface_cache(&factory->localfontfaces);
    LeaveCriticalSection(&factory->cs);

    LIST_FOR_EACH_ENTRY_SAFE(loader, loader2, &factory->collection_loaders, struct collectionloader, entry) {
        list_remove(&loader->entry);
        IDWriteFontCollectionLoader_Release(loader->loader);
        free(loader);
    }

    LIST_FOR_EACH_ENTRY_SAFE(fileloader, fileloader2, &factory->file_loaders, struct fileloader, entry)
        release_fileloader(fileloader);

    for (i = 0; i < ARRAY_SIZE(factory->system_collections); ++i)
    {
        if (factory->system_collections[i])
            IDWriteFontCollection_Release(factory->system_collections[i]);
    }
    if (factory->eudc_collection)
        IDWriteFontCollection1_Release(factory->eudc_collection);
    if (factory->fallback)
        release_system_fontfallback(factory->fallback);

    factory->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&factory->cs);
    free(factory);
}

static void release_shared_factory(IDWriteFactory7 *iface)
{
    struct dwritefactory *factory;
    if (!iface) return;
    factory = impl_from_IDWriteFactory7(iface);
    release_dwritefactory(factory);
}

static struct fileloader *factory_get_file_loader(struct dwritefactory *factory, IDWriteFontFileLoader *loader)
{
    struct fileloader *entry, *found = NULL;

    LIST_FOR_EACH_ENTRY(entry, &factory->file_loaders, struct fileloader, entry) {
        if (entry->loader == loader) {
            found = entry;
            break;
        }
    }

    return found;
}

static struct collectionloader *factory_get_collection_loader(struct dwritefactory *factory,
        IDWriteFontCollectionLoader *loader)
{
    struct collectionloader *entry, *found = NULL;

    LIST_FOR_EACH_ENTRY(entry, &factory->collection_loaders, struct collectionloader, entry) {
        if (entry->loader == loader) {
            found = entry;
            break;
        }
    }

    return found;
}

static HRESULT factory_get_system_collection(struct dwritefactory *factory,
        DWRITE_FONT_FAMILY_MODEL family_model, REFIID riid, void **out)
{
    IDWriteFontCollection *collection;
    HRESULT hr;

    *out = NULL;

    if (family_model != DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC &&
            family_model != DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE)
    {
        return E_INVALIDARG;
    }

    if (factory->system_collections[family_model])
        return IDWriteFontCollection_QueryInterface(factory->system_collections[family_model], riid, out);

    if (FAILED(hr = get_system_fontcollection(&factory->IDWriteFactory7_iface, family_model, &collection)))
    {
        WARN("Failed to create system font collection, hr %#lx.\n", hr);
        return hr;
    }

    if (InterlockedCompareExchangePointer((void **)&factory->system_collections[family_model], collection, NULL))
        IDWriteFontCollection_Release(collection);

    hr = IDWriteFontCollection_QueryInterface(factory->system_collections[family_model], riid, out);
    IDWriteFontCollection_Release(factory->system_collections[family_model]);
    return hr;
}

static HRESULT WINAPI dwritefactory_QueryInterface(IDWriteFactory7 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFactory7) ||
        IsEqualIID(riid, &IID_IDWriteFactory6) ||
        IsEqualIID(riid, &IID_IDWriteFactory5) ||
        IsEqualIID(riid, &IID_IDWriteFactory4) ||
        IsEqualIID(riid, &IID_IDWriteFactory3) ||
        IsEqualIID(riid, &IID_IDWriteFactory2) ||
        IsEqualIID(riid, &IID_IDWriteFactory1) ||
        IsEqualIID(riid, &IID_IDWriteFactory) ||
        IsEqualIID(riid, &IID_IUnknown))
   {
        *obj = iface;
        IDWriteFactory7_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefactory_AddRef(IDWriteFactory7 *iface)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    ULONG refcount = InterlockedIncrement(&factory->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritefactory_Release(IDWriteFactory7 *iface)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
        release_dwritefactory(factory);

    return refcount;
}

static HRESULT WINAPI dwritefactory_GetSystemFontCollection(IDWriteFactory7 *iface,
        IDWriteFontCollection **collection, BOOL check_for_updates)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);

    TRACE("%p, %p, %d.\n", iface, collection, check_for_updates);

    if (check_for_updates)
        FIXME("checking for system font updates not implemented\n");

    return factory_get_system_collection(factory, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE,
            &IID_IDWriteFontCollection, (void **)collection);
}

static HRESULT WINAPI dwritefactory_CreateCustomFontCollection(IDWriteFactory7 *iface,
    IDWriteFontCollectionLoader *loader, void const *key, UINT32 key_size, IDWriteFontCollection **collection)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    IDWriteFontFileEnumerator *enumerator;
    struct collectionloader *found;
    HRESULT hr;

    TRACE("%p, %p, %p, %u, %p.\n", iface, loader, key, key_size, collection);

    *collection = NULL;

    if (!loader)
        return E_INVALIDARG;

    found = factory_get_collection_loader(factory, loader);
    if (!found)
        return E_INVALIDARG;

    hr = IDWriteFontCollectionLoader_CreateEnumeratorFromKey(found->loader, (IDWriteFactory *)iface,
            key, key_size, &enumerator);
    if (FAILED(hr))
        return hr;

    hr = create_font_collection(iface, enumerator, FALSE, (IDWriteFontCollection3 **)collection);
    IDWriteFontFileEnumerator_Release(enumerator);
    return hr;
}

static HRESULT WINAPI dwritefactory_RegisterFontCollectionLoader(IDWriteFactory7 *iface,
    IDWriteFontCollectionLoader *loader)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    struct collectionloader *entry;

    TRACE("%p, %p.\n", iface, loader);

    if (!loader)
        return E_INVALIDARG;

    if (factory_get_collection_loader(factory, loader))
        return DWRITE_E_ALREADYREGISTERED;

    if (!(entry = malloc(sizeof(*entry))))
        return E_OUTOFMEMORY;

    entry->loader = loader;
    IDWriteFontCollectionLoader_AddRef(loader);
    list_add_tail(&factory->collection_loaders, &entry->entry);

    return S_OK;
}

static HRESULT WINAPI dwritefactory_UnregisterFontCollectionLoader(IDWriteFactory7 *iface,
    IDWriteFontCollectionLoader *loader)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    struct collectionloader *found;

    TRACE("%p, %p.\n", iface, loader);

    if (!loader)
        return E_INVALIDARG;

    found = factory_get_collection_loader(factory, loader);
    if (!found)
        return E_INVALIDARG;

    IDWriteFontCollectionLoader_Release(found->loader);
    list_remove(&found->entry);
    free(found);

    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateFontFileReference(IDWriteFactory7 *iface,
    WCHAR const *path, FILETIME const *writetime, IDWriteFontFile **font_file)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    UINT32 key_size;
    HRESULT hr;
    void *key;

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_w(path), writetime, font_file);

    *font_file = NULL;

    /* Get a reference key in local file loader format. */
    hr = get_local_refkey(path, writetime, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = create_font_file(factory->localfontfileloader, key, key_size, font_file);
    free(key);

    return hr;
}

static HRESULT WINAPI dwritefactory_CreateCustomFontFileReference(IDWriteFactory7 *iface,
    void const *reference_key, UINT32 key_size, IDWriteFontFileLoader *loader, IDWriteFontFile **font_file)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);

    TRACE("%p, %p, %u, %p, %p.\n", iface, reference_key, key_size, loader, font_file);

    *font_file = NULL;

    if (!loader || !(factory_get_file_loader(factory, loader) || factory->localfontfileloader == loader))
        return E_INVALIDARG;

    return create_font_file(loader, reference_key, key_size, font_file);
}

void factory_lock(IDWriteFactory7 *iface)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    EnterCriticalSection(&factory->cs);
}

void factory_unlock(IDWriteFactory7 *iface)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    LeaveCriticalSection(&factory->cs);
}

HRESULT factory_get_cached_fontface(IDWriteFactory7 *iface, IDWriteFontFile * const *font_files, UINT32 index,
        DWRITE_FONT_SIMULATIONS simulations, struct list **cached_list, REFIID riid, void **obj)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    struct fontfacecached *cached;
    IDWriteFontFileLoader *loader;
    struct list *fontfaces;
    UINT32 key_size;
    const void *key;
    HRESULT hr;

    *obj = NULL;
    *cached_list = NULL;

    hr = IDWriteFontFile_GetReferenceKey(*font_files, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFile_GetLoader(*font_files, &loader);
    if (FAILED(hr))
        return hr;

    if (loader == factory->localfontfileloader) {
        fontfaces = &factory->localfontfaces;
        IDWriteFontFileLoader_Release(loader);
    }
    else {
        struct fileloader *fileloader = factory_get_file_loader(factory, loader);
        IDWriteFontFileLoader_Release(loader);
        if (!fileloader)
            return E_INVALIDARG;
        fontfaces = &fileloader->fontfaces;
    }

    *cached_list = fontfaces;

    EnterCriticalSection(&factory->cs);

    /* search through cache list */
    LIST_FOR_EACH_ENTRY(cached, fontfaces, struct fontfacecached, entry) {
        UINT32 cached_key_size, count = 1, cached_face_index;
        DWRITE_FONT_SIMULATIONS cached_simulations;
        const void *cached_key;
        IDWriteFontFile *file;

        cached_face_index = IDWriteFontFace5_GetIndex(cached->fontface);
        cached_simulations = IDWriteFontFace5_GetSimulations(cached->fontface);

        /* skip earlier */
        if (cached_face_index != index || cached_simulations != simulations)
            continue;

        hr = IDWriteFontFace5_GetFiles(cached->fontface, &count, &file);
        if (FAILED(hr))
            break;

        hr = IDWriteFontFile_GetReferenceKey(file, &cached_key, &cached_key_size);
        IDWriteFontFile_Release(file);
        if (FAILED(hr))
            break;

        if (cached_key_size == key_size && !memcmp(cached_key, key, key_size))
        {
            if (FAILED(hr = IDWriteFontFace5_QueryInterface(cached->fontface, riid, obj)))
                WARN("Failed to get %s from fontface, hr %#lx.\n", debugstr_guid(riid), hr);

            TRACE("returning cached fontface %p\n", cached->fontface);
            break;
        }
    }

    LeaveCriticalSection(&factory->cs);

    return *obj ? S_OK : S_FALSE;
}

struct fontfacecached *factory_cache_fontface(IDWriteFactory7 *iface, struct list *fontfaces,
        IDWriteFontFace5 *fontface)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    struct fontfacecached *cached;

    /* new cache entry */
    if (!(cached = malloc(sizeof(*cached))))
        return NULL;

    cached->fontface = fontface;
    EnterCriticalSection(&factory->cs);
    list_add_tail(fontfaces, &cached->entry);
    LeaveCriticalSection(&factory->cs);

    return cached;
}

static HRESULT WINAPI dwritefactory_CreateFontFace(IDWriteFactory7 *iface, DWRITE_FONT_FACE_TYPE req_facetype,
    UINT32 files_number, IDWriteFontFile* const* font_files, UINT32 index, DWRITE_FONT_SIMULATIONS simulations,
    IDWriteFontFace **fontface)
{
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFileStream *stream;
    struct fontface_desc desc;
    struct list *fontfaces;
    BOOL is_supported;
    UINT32 face_count;
    HRESULT hr;

    TRACE("%p, %d, %u, %p, %u, %#x, %p.\n", iface, req_facetype, files_number, font_files, index,
        simulations, fontface);

    *fontface = NULL;

    if (!is_face_type_supported(req_facetype))
        return E_INVALIDARG;

    if (req_facetype != DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION && index)
        return E_INVALIDARG;

    if (!is_simulation_valid(simulations))
        return E_INVALIDARG;

    if (FAILED(hr = get_filestream_from_file(*font_files, &stream)))
        return hr;

    /* check actual file/face type */
    is_supported = FALSE;
    face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    hr = opentype_analyze_font(stream, &is_supported, &file_type, &face_type, &face_count);
    if (FAILED(hr))
        goto failed;

    if (!is_supported) {
        hr = E_FAIL;
        goto failed;
    }

    if (face_type != req_facetype) {
        hr = DWRITE_E_FILEFORMAT;
        goto failed;
    }

    hr = factory_get_cached_fontface(iface, font_files, index, simulations, &fontfaces,
            &IID_IDWriteFontFace, (void **)fontface);
    if (hr != S_FALSE)
        goto failed;

    desc.factory = iface;
    desc.face_type = req_facetype;
    desc.file = *font_files;
    desc.stream = stream;
    desc.index = index;
    desc.simulations = simulations;
    desc.font_data = NULL;
    hr = create_fontface(&desc, fontfaces, (IDWriteFontFace5 **)fontface);

failed:
    IDWriteFontFileStream_Release(stream);
    return hr;
}

static HRESULT WINAPI dwritefactory_CreateRenderingParams(IDWriteFactory7 *iface, IDWriteRenderingParams **params)
{
    HMONITOR monitor;
    POINT pt;

    TRACE("%p, %p.\n", iface, params);

    pt.x = pt.y = 0;
    monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    return IDWriteFactory7_CreateMonitorRenderingParams(iface, monitor, params);
}

static HRESULT WINAPI dwritefactory_CreateMonitorRenderingParams(IDWriteFactory7 *iface, HMONITOR monitor,
    IDWriteRenderingParams **params)
{
    IDWriteRenderingParams3 *params3;
    static int fixme_once = 0;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, monitor, params);

    if (!fixme_once++)
        FIXME("(%p): monitor setting ignored\n", monitor);

    /* FIXME: use actual per-monitor gamma factor */
    hr = IDWriteFactory7_CreateCustomRenderingParams(iface, 2.0f, 0.0f, 1.0f, 0.0f, DWRITE_PIXEL_GEOMETRY_FLAT,
        DWRITE_RENDERING_MODE1_DEFAULT, DWRITE_GRID_FIT_MODE_DEFAULT, &params3);
    *params = (IDWriteRenderingParams*)params3;
    return hr;
}

static HRESULT WINAPI dwritefactory_CreateCustomRenderingParams(IDWriteFactory7 *iface, FLOAT gamma,
        FLOAT enhancedContrast, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode,
        IDWriteRenderingParams **params)
{
    IDWriteRenderingParams3 *params3;
    HRESULT hr;

    TRACE("%p, %.8e, %.8e, %.8e, %d, %d, %p.\n", iface, gamma, enhancedContrast, cleartype_level, geometry, mode, params);

    if ((UINT32)mode > DWRITE_RENDERING_MODE_OUTLINE) {
        *params = NULL;
        return E_INVALIDARG;
    }

    hr = IDWriteFactory7_CreateCustomRenderingParams(iface, gamma, enhancedContrast, 1.0f, cleartype_level, geometry,
            (DWRITE_RENDERING_MODE1)mode, DWRITE_GRID_FIT_MODE_DEFAULT, &params3);
    *params = (IDWriteRenderingParams*)params3;
    return hr;
}

static HRESULT WINAPI dwritefactory_RegisterFontFileLoader(IDWriteFactory7 *iface, IDWriteFontFileLoader *loader)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    struct fileloader *entry;

    TRACE("%p, %p.\n", iface, loader);

    if (!loader)
        return E_INVALIDARG;

    if (factory_get_file_loader(factory, loader))
        return DWRITE_E_ALREADYREGISTERED;

    if (!(entry = malloc(sizeof(*entry))))
        return E_OUTOFMEMORY;

    entry->loader = loader;
    list_init(&entry->fontfaces);
    IDWriteFontFileLoader_AddRef(loader);
    list_add_tail(&factory->file_loaders, &entry->entry);

    return S_OK;
}

static HRESULT WINAPI dwritefactory_UnregisterFontFileLoader(IDWriteFactory7 *iface, IDWriteFontFileLoader *loader)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    struct fileloader *found;

    TRACE("%p, %p.\n", iface, loader);

    if (!loader)
        return E_INVALIDARG;

    found = factory_get_file_loader(factory, loader);
    if (!found)
        return E_INVALIDARG;

    release_fileloader(found);
    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateTextFormat(IDWriteFactory7 *iface, WCHAR const* family_name,
    IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch, FLOAT size, WCHAR const *locale, IDWriteTextFormat **format)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    HRESULT hr;

    TRACE("%p, %s, %p, %d, %d, %d, %.8e, %s, %p.\n", iface, debugstr_w(family_name), collection, weight, style, stretch,
        size, debugstr_w(locale), format);

    *format = NULL;

    if (collection)
    {
        IDWriteFontCollection_AddRef(collection);
    }
    else if (FAILED(hr = factory_get_system_collection(factory, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE,
                &IID_IDWriteFontCollection, (void **)&collection)))
    {
        return hr;
    }

    hr = create_text_format(family_name, collection, weight, style, stretch, size, locale,
            &IID_IDWriteTextFormat, (void **)format);
    IDWriteFontCollection_Release(collection);
    return hr;
}

static HRESULT WINAPI dwritefactory_CreateTypography(IDWriteFactory7 *iface, IDWriteTypography **typography)
{
    TRACE("%p, %p.\n", iface, typography);

    return create_typography(typography);
}

static HRESULT WINAPI dwritefactory_GetGdiInterop(IDWriteFactory7 *iface, IDWriteGdiInterop **gdi_interop)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, gdi_interop);

    if (factory->gdiinterop)
        IDWriteGdiInterop1_AddRef(factory->gdiinterop);
    else
        hr = create_gdiinterop(iface, &factory->gdiinterop);

    *gdi_interop = (IDWriteGdiInterop *)factory->gdiinterop;

    return hr;
}

static HRESULT WINAPI dwritefactory_CreateTextLayout(IDWriteFactory7 *iface, WCHAR const* string,
    UINT32 length, IDWriteTextFormat *format, FLOAT max_width, FLOAT max_height, IDWriteTextLayout **layout)
{
    struct textlayout_desc desc;

    TRACE("%p, %s:%u, %p, %.8e, %.8e, %p.\n", iface, debugstr_wn(string, length), length, format, max_width, max_height, layout);

    desc.factory = iface;
    desc.string = string;
    desc.length = length;
    desc.format = format;
    desc.max_width = max_width;
    desc.max_height = max_height;
    desc.is_gdi_compatible = FALSE;
    desc.ppdip = 1.0f;
    desc.transform = NULL;
    desc.use_gdi_natural = FALSE;
    return create_textlayout(&desc, layout);
}

static HRESULT WINAPI dwritefactory_CreateGdiCompatibleTextLayout(IDWriteFactory7 *iface, WCHAR const* string,
    UINT32 length, IDWriteTextFormat *format, FLOAT max_width, FLOAT max_height, FLOAT pixels_per_dip,
    DWRITE_MATRIX const* transform, BOOL use_gdi_natural, IDWriteTextLayout **layout)
{
    struct textlayout_desc desc;

    TRACE("%p, %s:%u, %p, %.8e, %.8e, %.8e, %p, %d, %p.\n", iface, debugstr_wn(string, length), length, format,
            max_width, max_height, pixels_per_dip, transform, use_gdi_natural, layout);

    desc.factory = iface;
    desc.string = string;
    desc.length = length;
    desc.format = format;
    desc.max_width = max_width;
    desc.max_height = max_height;
    desc.is_gdi_compatible = TRUE;
    desc.ppdip = pixels_per_dip;
    desc.transform = transform;
    desc.use_gdi_natural = use_gdi_natural;
    return create_textlayout(&desc, layout);
}

static HRESULT WINAPI dwritefactory_CreateEllipsisTrimmingSign(IDWriteFactory7 *iface, IDWriteTextFormat *format,
    IDWriteInlineObject **trimming_sign)
{
    TRACE("%p, %p, %p.\n", iface, format, trimming_sign);

    return create_trimmingsign(iface, format, trimming_sign);
}

static HRESULT WINAPI dwritefactory_CreateTextAnalyzer(IDWriteFactory7 *iface, IDWriteTextAnalyzer **analyzer)
{
    TRACE("%p, %p.\n", iface, analyzer);

    *analyzer = (IDWriteTextAnalyzer *)get_text_analyzer();

    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateNumberSubstitution(IDWriteFactory7 *iface,
        DWRITE_NUMBER_SUBSTITUTION_METHOD method, WCHAR const* locale, BOOL ignore_user_override,
        IDWriteNumberSubstitution **substitution)
{
    TRACE("%p, %d, %s, %d, %p.\n", iface, method, debugstr_w(locale), ignore_user_override, substitution);

    return create_numbersubstitution(method, locale, ignore_user_override, substitution);
}

static HRESULT WINAPI dwritefactory_CreateGlyphRunAnalysis(IDWriteFactory7 *iface, DWRITE_GLYPH_RUN const *run,
    FLOAT ppdip, DWRITE_MATRIX const* transform, DWRITE_RENDERING_MODE rendering_mode,
    DWRITE_MEASURING_MODE measuring_mode, FLOAT originX, FLOAT originY, IDWriteGlyphRunAnalysis **analysis)
{
    struct glyphrunanalysis_desc desc;
    DWRITE_MATRIX m, scale = { 0 };

    TRACE("%p, %p, %.8e, %p, %d, %d, %.8e, %.8e, %p.\n", iface, run, ppdip, transform, rendering_mode,
        measuring_mode, originX, originY, analysis);

    if (ppdip <= 0.0f) {
        *analysis = NULL;
        return E_INVALIDARG;
    }

    m = transform ? *transform : identity;
    scale.m11 = ppdip;
    scale.m22 = ppdip;
    dwrite_matrix_multiply(&m, &scale);
    desc.run = run;
    desc.transform = &m;
    desc.rendering_mode = (DWRITE_RENDERING_MODE1)rendering_mode;
    desc.measuring_mode = measuring_mode;
    desc.gridfit_mode = DWRITE_GRID_FIT_MODE_DEFAULT;
    desc.aa_mode = DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE;
    desc.origin.x = originX;
    desc.origin.y = originY;
    return create_glyphrunanalysis(&desc, analysis);
}

static HRESULT WINAPI dwritefactory1_GetEudcFontCollection(IDWriteFactory7 *iface, IDWriteFontCollection **collection,
    BOOL check_for_updates)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %d.\n", iface, collection, check_for_updates);

    if (check_for_updates)
        FIXME("checking for eudc updates not implemented\n");

    if (factory->eudc_collection)
        IDWriteFontCollection1_AddRef(factory->eudc_collection);
    else {
        IDWriteFontCollection3 *eudc_collection;

        if (FAILED(hr = get_eudc_fontcollection(iface, &eudc_collection)))
        {
            *collection = NULL;
            WARN("Failed to get EUDC collection, hr %#lx.\n", hr);
            return hr;
        }

        if (InterlockedCompareExchangePointer((void **)&factory->eudc_collection, eudc_collection, NULL))
            IDWriteFontCollection3_Release(eudc_collection);
    }

    *collection = (IDWriteFontCollection *)factory->eudc_collection;

    return hr;
}

static HRESULT WINAPI dwritefactory1_CreateCustomRenderingParams(IDWriteFactory7 *iface, FLOAT gamma,
    FLOAT enhcontrast, FLOAT enhcontrast_grayscale, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry,
    DWRITE_RENDERING_MODE mode, IDWriteRenderingParams1** params)
{
    IDWriteRenderingParams3 *params3;
    HRESULT hr;

    TRACE("%p, %.8e, %.8e, %.8e, %.8e, %d, %d, %p.\n", iface, gamma, enhcontrast, enhcontrast_grayscale,
        cleartype_level, geometry, mode, params);

    if ((UINT32)mode > DWRITE_RENDERING_MODE_OUTLINE) {
        *params = NULL;
        return E_INVALIDARG;
    }

    hr = IDWriteFactory7_CreateCustomRenderingParams(iface, gamma, enhcontrast, enhcontrast_grayscale,
        cleartype_level, geometry, (DWRITE_RENDERING_MODE1)mode, DWRITE_GRID_FIT_MODE_DEFAULT, &params3);
    *params = (IDWriteRenderingParams1*)params3;
    return hr;
}

static HRESULT WINAPI dwritefactory2_GetSystemFontFallback(IDWriteFactory7 *iface, IDWriteFontFallback **fallback)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);

    TRACE("%p, %p.\n", iface, fallback);

    *fallback = NULL;

    if (!factory->fallback)
    {
        HRESULT hr = create_system_fontfallback(iface, &factory->fallback);
        if (FAILED(hr))
            return hr;
    }

    *fallback = (IDWriteFontFallback *)factory->fallback;
    IDWriteFontFallback_AddRef(*fallback);
    return S_OK;
}

static HRESULT WINAPI dwritefactory2_CreateFontFallbackBuilder(IDWriteFactory7 *iface,
        IDWriteFontFallbackBuilder **fallbackbuilder)
{
    TRACE("%p, %p.\n", iface, fallbackbuilder);

    return create_fontfallback_builder(iface, fallbackbuilder);
}

static HRESULT WINAPI dwritefactory2_TranslateColorGlyphRun(IDWriteFactory7 *iface, FLOAT originX, FLOAT originY,
    const DWRITE_GLYPH_RUN *run, const DWRITE_GLYPH_RUN_DESCRIPTION *run_desc, DWRITE_MEASURING_MODE measuring_mode,
    const DWRITE_MATRIX *transform, UINT32 palette, IDWriteColorGlyphRunEnumerator **layers)
{
    D2D1_POINT_2F origin = { originX, originY };

    TRACE("%p, %.8e, %.8e, %p, %p, %d, %p, %u, %p.\n", iface, originX, originY, run, run_desc, measuring_mode,
            transform, palette, layers);

    return create_colorglyphenum(origin, run, run_desc, DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE
            | DWRITE_GLYPH_IMAGE_FORMATS_CFF | DWRITE_GLYPH_IMAGE_FORMATS_COLR,
            measuring_mode, transform, palette, (IDWriteColorGlyphRunEnumerator1 **)layers);
}

static HRESULT WINAPI dwritefactory2_CreateCustomRenderingParams(IDWriteFactory7 *iface, FLOAT gamma, FLOAT contrast,
    FLOAT grayscalecontrast, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode,
    DWRITE_GRID_FIT_MODE gridfit, IDWriteRenderingParams2 **params)
{
    IDWriteRenderingParams3 *params3;
    HRESULT hr;

    TRACE("%p, %.8e, %.8e, %.8e, %.8e, %d, %d, %d, %p.\n", iface, gamma, contrast, grayscalecontrast, cleartype_level,
        geometry, mode, gridfit, params);

    if ((UINT32)mode > DWRITE_RENDERING_MODE_OUTLINE) {
        *params = NULL;
        return E_INVALIDARG;
    }

    hr = IDWriteFactory7_CreateCustomRenderingParams(iface, gamma, contrast, grayscalecontrast,
        cleartype_level, geometry, (DWRITE_RENDERING_MODE1)mode, DWRITE_GRID_FIT_MODE_DEFAULT, &params3);
    *params = (IDWriteRenderingParams2*)params3;
    return hr;
}

static HRESULT factory_create_glyphrun_analysis(const DWRITE_GLYPH_RUN *run, const DWRITE_MATRIX *transform,
    DWRITE_RENDERING_MODE1 rendering_mode, DWRITE_MEASURING_MODE measuring_mode, DWRITE_GRID_FIT_MODE gridfit_mode,
    DWRITE_TEXT_ANTIALIAS_MODE aa_mode, FLOAT originX, FLOAT originY, IDWriteGlyphRunAnalysis **analysis)
{
    struct glyphrunanalysis_desc desc;

    desc.run = run;
    desc.transform = transform;
    desc.rendering_mode = rendering_mode;
    desc.measuring_mode = measuring_mode;
    desc.gridfit_mode = gridfit_mode;
    desc.aa_mode = aa_mode;
    desc.origin.x = originX;
    desc.origin.y = originY;
    return create_glyphrunanalysis(&desc, analysis);
}

static HRESULT WINAPI dwritefactory2_CreateGlyphRunAnalysis(IDWriteFactory7 *iface, const DWRITE_GLYPH_RUN *run,
    const DWRITE_MATRIX *transform, DWRITE_RENDERING_MODE rendering_mode, DWRITE_MEASURING_MODE measuring_mode,
    DWRITE_GRID_FIT_MODE gridfit_mode, DWRITE_TEXT_ANTIALIAS_MODE aa_mode, FLOAT originX, FLOAT originY,
    IDWriteGlyphRunAnalysis **analysis)
{
    TRACE("%p, %p, %p, %d, %d, %d, %d, %.8e, %.8e, %p.\n", iface, run, transform, rendering_mode, measuring_mode,
        gridfit_mode, aa_mode, originX, originY, analysis);

    return factory_create_glyphrun_analysis(run, transform, (DWRITE_RENDERING_MODE1)rendering_mode, measuring_mode,
        gridfit_mode, aa_mode, originX, originY, analysis);
}

static HRESULT WINAPI dwritefactory3_CreateGlyphRunAnalysis(IDWriteFactory7 *iface, DWRITE_GLYPH_RUN const *run,
    DWRITE_MATRIX const *transform, DWRITE_RENDERING_MODE1 rendering_mode, DWRITE_MEASURING_MODE measuring_mode,
    DWRITE_GRID_FIT_MODE gridfit_mode, DWRITE_TEXT_ANTIALIAS_MODE aa_mode, FLOAT originX, FLOAT originY,
    IDWriteGlyphRunAnalysis **analysis)
{
    TRACE("%p, %p, %p, %d, %d, %d, %d, %.8e, %.8e, %p.\n", iface, run, transform, rendering_mode, measuring_mode,
        gridfit_mode, aa_mode, originX, originY, analysis);

    return factory_create_glyphrun_analysis(run, transform, rendering_mode, measuring_mode, gridfit_mode,
        aa_mode, originX, originY, analysis);
}

static HRESULT WINAPI dwritefactory3_CreateCustomRenderingParams(IDWriteFactory7 *iface, FLOAT gamma, FLOAT contrast,
        FLOAT grayscale_contrast, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY pixel_geometry,
        DWRITE_RENDERING_MODE1 rendering_mode, DWRITE_GRID_FIT_MODE gridfit_mode, IDWriteRenderingParams3 **params)
{
    TRACE("%p, %.8e, %.8e, %.8e, %.8e, %d, %d, %d, %p.\n", iface, gamma, contrast, grayscale_contrast, cleartype_level,
        pixel_geometry, rendering_mode, gridfit_mode, params);

    return create_renderingparams(gamma, contrast, grayscale_contrast, cleartype_level, pixel_geometry, rendering_mode,
        gridfit_mode, params);
}

static HRESULT WINAPI dwritefactory3_CreateFontFaceReference_(IDWriteFactory7 *iface, IDWriteFontFile *file,
        UINT32 index, DWRITE_FONT_SIMULATIONS simulations, IDWriteFontFaceReference **reference)
{
    TRACE("%p, %p, %u, %x, %p.\n", iface, file, index, simulations, reference);

    return create_fontfacereference(iface, file, index, simulations, NULL, 0, (IDWriteFontFaceReference1 **)reference);
}

static HRESULT WINAPI dwritefactory3_CreateFontFaceReference(IDWriteFactory7 *iface, WCHAR const *path,
        FILETIME const *writetime, UINT32 index, DWRITE_FONT_SIMULATIONS simulations,
        IDWriteFontFaceReference **reference)
{
    IDWriteFontFile *file;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %#x, %p.\n", iface, debugstr_w(path), writetime, index, simulations, reference);

    hr = IDWriteFactory7_CreateFontFileReference(iface, path, writetime, &file);
    if (FAILED(hr))
    {
        *reference = NULL;
        return hr;
    }

    hr = create_fontfacereference(iface, file, index, simulations, NULL, 0, (IDWriteFontFaceReference1 **)reference);
    IDWriteFontFile_Release(file);
    return hr;
}

static HRESULT create_system_path_list(WCHAR ***ret, unsigned int *ret_count)
{
    unsigned int index = 0, value_size, max_name_count;
    WCHAR **paths = NULL, *name, *value = NULL;
    DWORD name_count, type, data_size;
    size_t capacity = 0, count = 0;
    HKEY hkey;
    LONG r;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
            0, GENERIC_READ, &hkey))
    {
        return E_UNEXPECTED;
    }

    value_size = MAX_PATH * sizeof(*value);
    value = malloc(value_size);

    max_name_count = MAX_PATH;
    name = malloc(max_name_count * sizeof(*name));

    for (;;)
    {
        if (!value)
        {
            value_size = MAX_PATH * sizeof(*value);
            value = malloc(value_size);
        }

        do
        {
            name_count = max_name_count;
            data_size = value_size - sizeof(*value);

            r = RegEnumValueW(hkey, index, name, &name_count, NULL, &type, (BYTE *)value, &data_size);
            if (r == ERROR_MORE_DATA)
            {
                if (name_count >= max_name_count)
                {
                    max_name_count *= 2;
                    free(name);
                    name = malloc(max_name_count * sizeof(*name));
                }

                if (data_size > value_size - sizeof(*value))
                {
                    free(value);
                    value_size = max(data_size + sizeof(*value), value_size * 2);
                    value = malloc(value_size);
                }
            }
        } while (r == ERROR_MORE_DATA);

        if (r != ERROR_SUCCESS)
            break;

        value[data_size / sizeof(*value)] = 0;
        if (type == REG_SZ && *name != '@')
        {
            if (dwrite_array_reserve((void **)&paths, &capacity, count + 1, sizeof(*paths)))
            {
                if (!wcschr(value, '\\'))
                {
                    WCHAR *ptrW;

                    ptrW = malloc((MAX_PATH + wcslen(value)) * sizeof(WCHAR));
                    GetWindowsDirectoryW(ptrW, MAX_PATH);
                    wcscat(ptrW, L"\\fonts\\");
                    wcscat(ptrW, value);

                    free(value);
                    value = ptrW;
                }

                paths[count++] = value;
                value = NULL;
            }
        }
        index++;
    }

    free(value);
    free(name);

    *ret = paths;
    *ret_count = count;

    RegCloseKey(hkey);

    return S_OK;
}

static int __cdecl create_system_fontset_compare(const void *left, const void *right)
{
    const WCHAR *_l = *(WCHAR **)left, *_r = *(WCHAR **)right;
    return wcsicmp(_l, _r);
};

HRESULT create_system_fontset(IDWriteFactory7 *factory, REFIID riid, void **obj)
{
    IDWriteFontSetBuilder2 *builder;
    IDWriteFontSet *fontset;
    unsigned int i, j, count;
    WCHAR **paths;
    HRESULT hr;

    *obj = NULL;

    if (FAILED(hr = create_fontset_builder(factory, &builder))) return hr;

    if (SUCCEEDED(hr = create_system_path_list(&paths, &count)))
    {
        /* Sort, skip duplicates. */

        qsort(paths, count, sizeof(*paths), create_system_fontset_compare);

        for (i = 0, j = 0; i < count; ++i)
        {
            if (i != j && !wcsicmp(paths[i], paths[j])) continue;

            if (FAILED(hr = IDWriteFontSetBuilder2_AddFontFile(builder, paths[i])) && hr != DWRITE_E_FILEFORMAT)
                WARN("Failed to add font file, hr %#lx, path %s.\n", hr, debugstr_w(paths[i]));

            j = i;
        }

        for (i = 0; i < count; ++i)
            free(paths[i]);
        free(paths);
    }

    if (SUCCEEDED(hr = IDWriteFontSetBuilder2_CreateFontSet(builder, &fontset)))
    {
        hr = IDWriteFontSet_QueryInterface(fontset, riid, obj);
        IDWriteFontSet_Release(fontset);
    }

    IDWriteFontSetBuilder2_Release(builder);

    return hr;
}

static HRESULT WINAPI dwritefactory3_GetSystemFontSet(IDWriteFactory7 *iface, IDWriteFontSet **fontset)
{
    TRACE("%p, %p.\n", iface, fontset);

    return create_system_fontset(iface, &IID_IDWriteFontSet, (void **)fontset);
}

static HRESULT WINAPI dwritefactory3_CreateFontSetBuilder(IDWriteFactory7 *iface, IDWriteFontSetBuilder **builder)
{
    TRACE("%p, %p.\n", iface, builder);

    return create_fontset_builder(iface, (IDWriteFontSetBuilder2 **)builder);
}

static HRESULT WINAPI dwritefactory3_CreateFontCollectionFromFontSet(IDWriteFactory7 *iface, IDWriteFontSet *fontset,
        IDWriteFontCollection1 **collection)
{
    TRACE("%p, %p, %p.\n", iface, fontset, collection);

    return create_font_collection_from_set(iface, fontset, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE,
            &IID_IDWriteFontCollection1, (void **)collection);
}

static HRESULT WINAPI dwritefactory3_GetSystemFontCollection(IDWriteFactory7 *iface, BOOL include_downloadable,
    IDWriteFontCollection1 **collection, BOOL check_for_updates)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);

    TRACE("%p, %d, %p, %d.\n", iface, include_downloadable, collection, check_for_updates);

    if (include_downloadable)
        FIXME("remote fonts are not supported\n");

    if (check_for_updates)
        FIXME("checking for system font updates not implemented\n");

    return factory_get_system_collection(factory, DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE,
            &IID_IDWriteFontCollection1, (void **)collection);
}

static HRESULT WINAPI dwritefactory3_GetFontDownloadQueue(IDWriteFactory7 *iface, IDWriteFontDownloadQueue **queue)
{
    FIXME("%p, %p: stub\n", iface, queue);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory4_TranslateColorGlyphRun(IDWriteFactory7 *iface, D2D1_POINT_2F origin,
        DWRITE_GLYPH_RUN const *run, DWRITE_GLYPH_RUN_DESCRIPTION const *run_desc,
        DWRITE_GLYPH_IMAGE_FORMATS desired_formats, DWRITE_MEASURING_MODE measuring_mode, DWRITE_MATRIX const *transform,
        UINT32 palette, IDWriteColorGlyphRunEnumerator1 **layers)
{
    TRACE("%p, %.8e, %.8e, %p, %p, %u, %d, %p, %u, %p.\n", iface, origin.x, origin.y, run, run_desc, desired_formats,
            measuring_mode, transform, palette, layers);

    return create_colorglyphenum(origin, run, run_desc, desired_formats, measuring_mode, transform, palette, layers);
}

HRESULT compute_glyph_origins(DWRITE_GLYPH_RUN const *run, DWRITE_MEASURING_MODE measuring_mode,
    D2D1_POINT_2F baseline_origin, DWRITE_MATRIX const *transform, D2D1_POINT_2F *origins)
{
    struct dwrite_fontface *font_obj;
    unsigned int i;
    float advance;

    font_obj = unsafe_impl_from_IDWriteFontFace(run->fontFace);

    for (i = 0; i < run->glyphCount; ++i)
    {
        origins[i] = baseline_origin;

        if (run->bidiLevel & 1)
        {
            advance = fontface_get_scaled_design_advance(font_obj, measuring_mode, run->fontEmSize,
                    1.0f, transform, run->glyphIndices[i], run->isSideways);

            origins[i].x -= advance;

            if (run->glyphOffsets)
            {
                origins[i].x -= run->glyphOffsets[i].advanceOffset;
                origins[i].y -= run->glyphOffsets[i].ascenderOffset;
            }

            baseline_origin.x -= run->glyphAdvances ? run->glyphAdvances[i] : advance;
        }
        else
        {
            if (run->glyphOffsets)
            {
                origins[i].x += run->glyphOffsets[i].advanceOffset;
                origins[i].y -= run->glyphOffsets[i].ascenderOffset;
            }

            baseline_origin.x += run->glyphAdvances ? run->glyphAdvances[i] :
                    fontface_get_scaled_design_advance(font_obj, measuring_mode, run->fontEmSize, 1.0f, transform,
                            run->glyphIndices[i], run->isSideways);

        }
    }

    return S_OK;
}

static HRESULT WINAPI dwritefactory4_ComputeGlyphOrigins_(IDWriteFactory7 *iface, DWRITE_GLYPH_RUN const *run,
    D2D1_POINT_2F baseline_origin, D2D1_POINT_2F *origins)
{
    TRACE("%p, %p, {%.8e,%.8e}, %p.\n", iface, run, baseline_origin.x, baseline_origin.y, origins);

    return compute_glyph_origins(run, DWRITE_MEASURING_MODE_NATURAL, baseline_origin, NULL, origins);
}

static HRESULT WINAPI dwritefactory4_ComputeGlyphOrigins(IDWriteFactory7 *iface, DWRITE_GLYPH_RUN const *run,
    DWRITE_MEASURING_MODE measuring_mode, D2D1_POINT_2F baseline_origin, DWRITE_MATRIX const *transform,
    D2D1_POINT_2F *origins)
{
    TRACE("%p, %p, %d, {%.8e,%.8e}, %p, %p.\n", iface, run, measuring_mode, baseline_origin.x, baseline_origin.y,
        transform, origins);

    return compute_glyph_origins(run, measuring_mode, baseline_origin, transform, origins);
}

static HRESULT WINAPI dwritefactory5_CreateFontSetBuilder(IDWriteFactory7 *iface, IDWriteFontSetBuilder1 **builder)
{
    TRACE("%p, %p.\n", iface, builder);

    return create_fontset_builder(iface, (IDWriteFontSetBuilder2 **)builder);
}

static HRESULT WINAPI dwritefactory5_CreateInMemoryFontFileLoader(IDWriteFactory7 *iface,
        IDWriteInMemoryFontFileLoader **loader)
{
    TRACE("%p, %p.\n", iface, loader);

    return create_inmemory_fileloader(loader);
}

static HRESULT WINAPI dwritefactory5_CreateHttpFontFileLoader(IDWriteFactory7 *iface, WCHAR const *referrer_url,
        WCHAR const *extra_headers, IDWriteRemoteFontFileLoader **loader)
{
    FIXME("%p, %s, %s, %p: stub\n", iface, debugstr_w(referrer_url), wine_dbgstr_w(extra_headers), loader);

    return E_NOTIMPL;
}

static DWRITE_CONTAINER_TYPE WINAPI dwritefactory5_AnalyzeContainerType(IDWriteFactory7 *iface, void const *data,
        UINT32 data_size)
{
    TRACE("%p, %p, %u.\n", iface, data, data_size);

    return opentype_analyze_container_type(data, data_size);
}

static HRESULT WINAPI dwritefactory5_UnpackFontFile(IDWriteFactory7 *iface, DWRITE_CONTAINER_TYPE container_type, void const *data,
        UINT32 data_size, IDWriteFontFileStream **stream)
{
    FIXME("%p, %d, %p, %u, %p: stub\n", iface, container_type, data, data_size, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory6_CreateFontFaceReference(IDWriteFactory7 *iface, IDWriteFontFile *file,
        UINT32 face_index, DWRITE_FONT_SIMULATIONS simulations, DWRITE_FONT_AXIS_VALUE const *axis_values,
        UINT32 axis_values_count, IDWriteFontFaceReference1 **reference)
{
    TRACE("%p, %p, %u, %#x, %p, %u, %p.\n", iface, file, face_index, simulations, axis_values, axis_values_count,
            reference);

    return create_fontfacereference(iface, file, face_index, simulations, axis_values, axis_values_count, reference);
}

static HRESULT WINAPI dwritefactory6_CreateFontResource(IDWriteFactory7 *iface, IDWriteFontFile *file,
        UINT32 face_index, IDWriteFontResource **resource)
{
    TRACE("%p, %p, %u, %p.\n", iface, file, face_index, resource);

    return create_font_resource(iface, file, face_index, resource);
}

static HRESULT WINAPI dwritefactory6_GetSystemFontSet(IDWriteFactory7 *iface, BOOL include_downloadable,
        IDWriteFontSet1 **fontset)
{
    TRACE("%p, %d, %p.\n", iface, include_downloadable, fontset);

    if (include_downloadable)
        FIXME("Downloadable fonts are not supported.\n");

    return create_system_fontset(iface, &IID_IDWriteFontSet1, (void **)fontset);
}

static HRESULT WINAPI dwritefactory6_GetSystemFontCollection(IDWriteFactory7 *iface, BOOL include_downloadable,
        DWRITE_FONT_FAMILY_MODEL family_model, IDWriteFontCollection2 **collection)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);

    TRACE("%p, %d, %d, %p.\n", iface, include_downloadable, family_model, collection);

    if (include_downloadable)
        FIXME("remote fonts are not supported\n");

    return factory_get_system_collection(factory, family_model, &IID_IDWriteFontCollection2, (void **)collection);
}

static HRESULT WINAPI dwritefactory6_CreateFontCollectionFromFontSet(IDWriteFactory7 *iface, IDWriteFontSet *fontset,
        DWRITE_FONT_FAMILY_MODEL family_model, IDWriteFontCollection2 **collection)
{
    TRACE("%p, %p, %d, %p.\n", iface, fontset, family_model, collection);

    return create_font_collection_from_set(iface, fontset, family_model, &IID_IDWriteFontCollection2, (void **)collection);
}

static HRESULT WINAPI dwritefactory6_CreateFontSetBuilder(IDWriteFactory7 *iface, IDWriteFontSetBuilder2 **builder)
{
    TRACE("%p, %p.\n", iface, builder);

    return create_fontset_builder(iface, builder);
}

static HRESULT WINAPI dwritefactory6_CreateTextFormat(IDWriteFactory7 *iface, const WCHAR *family_name,
        IDWriteFontCollection *collection, DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 num_axis,
        float size, const WCHAR *locale, IDWriteTextFormat3 **format)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    HRESULT hr;

    TRACE("%p, %s, %p, %p, %u, %.8e, %s, %p.\n", iface, debugstr_w(family_name), collection, axis_values, num_axis,
            size, debugstr_w(locale), format);

    *format = NULL;

    if (axis_values)
        FIXME("Axis values are ignored.\n");

    if (collection)
    {
        IDWriteFontCollection_AddRef(collection);
    }
    else if (FAILED(hr = factory_get_system_collection(factory, DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC,
            &IID_IDWriteFontCollection, (void **)&collection)))
    {
        return hr;
    }

    hr = create_text_format(family_name, collection, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, locale, &IID_IDWriteTextFormat3, (void **)format);
    IDWriteFontCollection_Release(collection);
    return hr;
}

static HRESULT WINAPI dwritefactory7_GetSystemFontSet(IDWriteFactory7 *iface, BOOL include_downloadable,
        IDWriteFontSet2 **fontset)
{
    TRACE("%p, %d, %p.\n", iface, include_downloadable, fontset);

    if (include_downloadable)
        FIXME("Downloadable fonts are not supported.\n");

    return create_system_fontset(iface, &IID_IDWriteFontSet2, (void **)fontset);
}

static HRESULT WINAPI dwritefactory7_GetSystemFontCollection(IDWriteFactory7 *iface, BOOL include_downloadable,
        DWRITE_FONT_FAMILY_MODEL family_model, IDWriteFontCollection3 **collection)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);

    TRACE("%p, %d, %d, %p.\n", iface, include_downloadable, family_model, collection);

    if (include_downloadable)
        FIXME("remote fonts are not supported\n");

    return factory_get_system_collection(factory, family_model, &IID_IDWriteFontCollection3, (void **)collection);
}

static const IDWriteFactory7Vtbl dwritefactoryvtbl =
{
    dwritefactory_QueryInterface,
    dwritefactory_AddRef,
    dwritefactory_Release,
    dwritefactory_GetSystemFontCollection,
    dwritefactory_CreateCustomFontCollection,
    dwritefactory_RegisterFontCollectionLoader,
    dwritefactory_UnregisterFontCollectionLoader,
    dwritefactory_CreateFontFileReference,
    dwritefactory_CreateCustomFontFileReference,
    dwritefactory_CreateFontFace,
    dwritefactory_CreateRenderingParams,
    dwritefactory_CreateMonitorRenderingParams,
    dwritefactory_CreateCustomRenderingParams,
    dwritefactory_RegisterFontFileLoader,
    dwritefactory_UnregisterFontFileLoader,
    dwritefactory_CreateTextFormat,
    dwritefactory_CreateTypography,
    dwritefactory_GetGdiInterop,
    dwritefactory_CreateTextLayout,
    dwritefactory_CreateGdiCompatibleTextLayout,
    dwritefactory_CreateEllipsisTrimmingSign,
    dwritefactory_CreateTextAnalyzer,
    dwritefactory_CreateNumberSubstitution,
    dwritefactory_CreateGlyphRunAnalysis,
    dwritefactory1_GetEudcFontCollection,
    dwritefactory1_CreateCustomRenderingParams,
    dwritefactory2_GetSystemFontFallback,
    dwritefactory2_CreateFontFallbackBuilder,
    dwritefactory2_TranslateColorGlyphRun,
    dwritefactory2_CreateCustomRenderingParams,
    dwritefactory2_CreateGlyphRunAnalysis,
    dwritefactory3_CreateGlyphRunAnalysis,
    dwritefactory3_CreateCustomRenderingParams,
    dwritefactory3_CreateFontFaceReference_,
    dwritefactory3_CreateFontFaceReference,
    dwritefactory3_GetSystemFontSet,
    dwritefactory3_CreateFontSetBuilder,
    dwritefactory3_CreateFontCollectionFromFontSet,
    dwritefactory3_GetSystemFontCollection,
    dwritefactory3_GetFontDownloadQueue,
    dwritefactory4_TranslateColorGlyphRun,
    dwritefactory4_ComputeGlyphOrigins_,
    dwritefactory4_ComputeGlyphOrigins,
    dwritefactory5_CreateFontSetBuilder,
    dwritefactory5_CreateInMemoryFontFileLoader,
    dwritefactory5_CreateHttpFontFileLoader,
    dwritefactory5_AnalyzeContainerType,
    dwritefactory5_UnpackFontFile,
    dwritefactory6_CreateFontFaceReference,
    dwritefactory6_CreateFontResource,
    dwritefactory6_GetSystemFontSet,
    dwritefactory6_GetSystemFontCollection,
    dwritefactory6_CreateFontCollectionFromFontSet,
    dwritefactory6_CreateFontSetBuilder,
    dwritefactory6_CreateTextFormat,
    dwritefactory7_GetSystemFontSet,
    dwritefactory7_GetSystemFontCollection,
};

static ULONG WINAPI shareddwritefactory_AddRef(IDWriteFactory7 *iface)
{
    TRACE("%p.\n", iface);

    return 2;
}

static ULONG WINAPI shareddwritefactory_Release(IDWriteFactory7 *iface)
{
    TRACE("%p.\n", iface);

    return 1;
}

static const IDWriteFactory7Vtbl shareddwritefactoryvtbl =
{
    dwritefactory_QueryInterface,
    shareddwritefactory_AddRef,
    shareddwritefactory_Release,
    dwritefactory_GetSystemFontCollection,
    dwritefactory_CreateCustomFontCollection,
    dwritefactory_RegisterFontCollectionLoader,
    dwritefactory_UnregisterFontCollectionLoader,
    dwritefactory_CreateFontFileReference,
    dwritefactory_CreateCustomFontFileReference,
    dwritefactory_CreateFontFace,
    dwritefactory_CreateRenderingParams,
    dwritefactory_CreateMonitorRenderingParams,
    dwritefactory_CreateCustomRenderingParams,
    dwritefactory_RegisterFontFileLoader,
    dwritefactory_UnregisterFontFileLoader,
    dwritefactory_CreateTextFormat,
    dwritefactory_CreateTypography,
    dwritefactory_GetGdiInterop,
    dwritefactory_CreateTextLayout,
    dwritefactory_CreateGdiCompatibleTextLayout,
    dwritefactory_CreateEllipsisTrimmingSign,
    dwritefactory_CreateTextAnalyzer,
    dwritefactory_CreateNumberSubstitution,
    dwritefactory_CreateGlyphRunAnalysis,
    dwritefactory1_GetEudcFontCollection,
    dwritefactory1_CreateCustomRenderingParams,
    dwritefactory2_GetSystemFontFallback,
    dwritefactory2_CreateFontFallbackBuilder,
    dwritefactory2_TranslateColorGlyphRun,
    dwritefactory2_CreateCustomRenderingParams,
    dwritefactory2_CreateGlyphRunAnalysis,
    dwritefactory3_CreateGlyphRunAnalysis,
    dwritefactory3_CreateCustomRenderingParams,
    dwritefactory3_CreateFontFaceReference_,
    dwritefactory3_CreateFontFaceReference,
    dwritefactory3_GetSystemFontSet,
    dwritefactory3_CreateFontSetBuilder,
    dwritefactory3_CreateFontCollectionFromFontSet,
    dwritefactory3_GetSystemFontCollection,
    dwritefactory3_GetFontDownloadQueue,
    dwritefactory4_TranslateColorGlyphRun,
    dwritefactory4_ComputeGlyphOrigins_,
    dwritefactory4_ComputeGlyphOrigins,
    dwritefactory5_CreateFontSetBuilder,
    dwritefactory5_CreateInMemoryFontFileLoader,
    dwritefactory5_CreateHttpFontFileLoader,
    dwritefactory5_AnalyzeContainerType,
    dwritefactory5_UnpackFontFile,
    dwritefactory6_CreateFontFaceReference,
    dwritefactory6_CreateFontResource,
    dwritefactory6_GetSystemFontSet,
    dwritefactory6_GetSystemFontCollection,
    dwritefactory6_CreateFontCollectionFromFontSet,
    dwritefactory6_CreateFontSetBuilder,
    dwritefactory6_CreateTextFormat,
    dwritefactory7_GetSystemFontSet,
    dwritefactory7_GetSystemFontCollection,
};

static void init_dwritefactory(struct dwritefactory *factory, DWRITE_FACTORY_TYPE type)
{
    factory->IDWriteFactory7_iface.lpVtbl = type == DWRITE_FACTORY_TYPE_SHARED ?
            &shareddwritefactoryvtbl : &dwritefactoryvtbl;
    factory->refcount = 1;
    factory->localfontfileloader = get_local_fontfile_loader();

    list_init(&factory->collection_loaders);
    list_init(&factory->file_loaders);
    list_init(&factory->localfontfaces);

    InitializeCriticalSectionEx(&factory->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    factory->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": dwritefactory.lock");
}

void factory_detach_fontcollection(IDWriteFactory7 *iface, IDWriteFontCollection3 *collection)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(factory->system_collections); ++i)
        InterlockedCompareExchangePointer((void **)&factory->system_collections[i], NULL, collection);
    InterlockedCompareExchangePointer((void **)&factory->eudc_collection, NULL, collection);
    IDWriteFactory7_Release(iface);
}

void factory_detach_gdiinterop(IDWriteFactory7 *iface, IDWriteGdiInterop1 *interop)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory7(iface);
    factory->gdiinterop = NULL;
    IDWriteFactory7_Release(iface);
}

HRESULT WINAPI DWriteCreateFactory(DWRITE_FACTORY_TYPE type, REFIID riid, IUnknown **ret)
{
    struct dwritefactory *factory;
    HRESULT hr;

    TRACE("%d, %s, %p.\n", type, debugstr_guid(riid), ret);

    *ret = NULL;

    if (type == DWRITE_FACTORY_TYPE_SHARED && shared_factory)
        return IDWriteFactory7_QueryInterface(shared_factory, riid, (void**)ret);

    if (!(factory = calloc(1, sizeof(*factory))))
        return E_OUTOFMEMORY;

    init_dwritefactory(factory, type);

    if (type == DWRITE_FACTORY_TYPE_SHARED)
        if (InterlockedCompareExchangePointer((void **)&shared_factory, &factory->IDWriteFactory7_iface, NULL))
        {
            release_shared_factory(&factory->IDWriteFactory7_iface);
            return IDWriteFactory7_QueryInterface(shared_factory, riid, (void**)ret);
        }

    hr = IDWriteFactory7_QueryInterface(&factory->IDWriteFactory7_iface, riid, (void**)ret);
    IDWriteFactory7_Release(&factory->IDWriteFactory7_iface);
    return hr;
}
