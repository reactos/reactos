/*
 *    Text layout/format tests
 *
 * Copyright 2012, 2014-2020 Nikolay Sivov for CodeWeavers
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
#include <math.h>
#include <limits.h>

#include "windows.h"
#include "dwrite_3.h"

#include "wine/test.h"

struct test_fontenumerator
{
    IDWriteFontFileEnumerator IDWriteFontFileEnumerator_iface;
    LONG refcount;

    DWORD index;
    IDWriteFontFile *file;
};

static inline struct test_fontenumerator *impl_from_IDWriteFontFileEnumerator(IDWriteFontFileEnumerator* iface)
{
    return CONTAINING_RECORD(iface, struct test_fontenumerator, IDWriteFontFileEnumerator_iface);
}

static HRESULT WINAPI singlefontfileenumerator_QueryInterface(IDWriteFontFileEnumerator *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileEnumerator))
    {
        *obj = iface;
        IDWriteFontFileEnumerator_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI singlefontfileenumerator_AddRef(IDWriteFontFileEnumerator *iface)
{
    struct test_fontenumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);
    return InterlockedIncrement(&enumerator->refcount);
}

static ULONG WINAPI singlefontfileenumerator_Release(IDWriteFontFileEnumerator *iface)
{
    struct test_fontenumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);
    ULONG refcount = InterlockedDecrement(&enumerator->refcount);

    if (!refcount)
    {
        IDWriteFontFile_Release(enumerator->file);
        free(enumerator);
    }

    return refcount;
}

static HRESULT WINAPI singlefontfileenumerator_GetCurrentFontFile(IDWriteFontFileEnumerator *iface,
        IDWriteFontFile **file)
{
    struct test_fontenumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);

    IDWriteFontFile_AddRef((*file = enumerator->file));

    return S_OK;
}

static HRESULT WINAPI singlefontfileenumerator_MoveNext(IDWriteFontFileEnumerator *iface, BOOL *current)
{
    struct test_fontenumerator *enumerator = impl_from_IDWriteFontFileEnumerator(iface);

    if (enumerator->index > 1)
    {
        *current = FALSE;
        return S_OK;
    }

    enumerator->index++;
    *current = TRUE;
    return S_OK;
}

static const struct IDWriteFontFileEnumeratorVtbl singlefontfileenumeratorvtbl =
{
    singlefontfileenumerator_QueryInterface,
    singlefontfileenumerator_AddRef,
    singlefontfileenumerator_Release,
    singlefontfileenumerator_MoveNext,
    singlefontfileenumerator_GetCurrentFontFile
};

static HRESULT create_enumerator(IDWriteFontFile *file, IDWriteFontFileEnumerator **ret)
{
    struct test_fontenumerator *enumerator;

    if (!(enumerator = calloc(1, sizeof(*enumerator))))
        return E_OUTOFMEMORY;

    enumerator->IDWriteFontFileEnumerator_iface.lpVtbl = &singlefontfileenumeratorvtbl;
    enumerator->refcount = 1;
    enumerator->index = 0;
    enumerator->file = file;
    IDWriteFontFile_AddRef(file);

    *ret = &enumerator->IDWriteFontFileEnumerator_iface;
    return S_OK;
}

struct test_fontdatastream
{
    IDWriteFontFileStream IDWriteFontFileStream_iface;
    LONG refcount;

    void *data;
    DWORD size;
};

static inline struct test_fontdatastream *impl_from_IDWriteFontFileStream(IDWriteFontFileStream* iface)
{
    return CONTAINING_RECORD(iface, struct test_fontdatastream, IDWriteFontFileStream_iface);
}

static HRESULT WINAPI fontdatastream_QueryInterface(IDWriteFontFileStream *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileStream))
    {
        *obj = iface;
        IDWriteFontFileStream_AddRef(iface);
        return S_OK;
    }
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI fontdatastream_AddRef(IDWriteFontFileStream *iface)
{
    struct test_fontdatastream *stream = impl_from_IDWriteFontFileStream(iface);
    return InterlockedIncrement(&stream->refcount);
}

static ULONG WINAPI fontdatastream_Release(IDWriteFontFileStream *iface)
{
    struct test_fontdatastream *stream = impl_from_IDWriteFontFileStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    if (!refcount)
        free(stream);

    return refcount;
}

static HRESULT WINAPI fontdatastream_ReadFileFragment(IDWriteFontFileStream *iface, void const **fragment_start,
        UINT64 offset, UINT64 fragment_size, void **fragment_context)
{
    struct test_fontdatastream *stream = impl_from_IDWriteFontFileStream(iface);

    *fragment_context = NULL;

    if (offset + fragment_size > stream->size)
    {
        *fragment_start = NULL;
        return E_FAIL;
    }
    else
    {
        *fragment_start = (BYTE *)stream->data + offset;
        return S_OK;
    }
}

static void WINAPI fontdatastream_ReleaseFileFragment(IDWriteFontFileStream *iface, void *fragment_context)
{
}

static HRESULT WINAPI fontdatastream_GetFileSize(IDWriteFontFileStream *iface, UINT64 *size)
{
    struct test_fontdatastream *stream = impl_from_IDWriteFontFileStream(iface);
    *size = stream->size;
    return S_OK;
}

static HRESULT WINAPI fontdatastream_GetLastWriteTime(IDWriteFontFileStream *iface, UINT64 *last_writetime)
{
    return E_NOTIMPL;
}

static const IDWriteFontFileStreamVtbl fontdatastreamvtbl =
{
    fontdatastream_QueryInterface,
    fontdatastream_AddRef,
    fontdatastream_Release,
    fontdatastream_ReadFileFragment,
    fontdatastream_ReleaseFileFragment,
    fontdatastream_GetFileSize,
    fontdatastream_GetLastWriteTime
};

static HRESULT create_fontdatastream(LPVOID data, UINT size, IDWriteFontFileStream** iface)
{
    struct test_fontdatastream *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDWriteFontFileStream_iface.lpVtbl = &fontdatastreamvtbl;
    object->refcount = 1;
    object->data = data;
    object->size = size;

    *iface = &object->IDWriteFontFileStream_iface;

    return S_OK;
}

struct resource_file_loader
{
    IDWriteFontFileLoader IDWriteFontFileLoader_iface;
    LONG refcount;
};

static inline struct resource_file_loader *impl_from_IDWriteFontFileLoader(IDWriteFontFileLoader *iface)
{
    return CONTAINING_RECORD(iface, struct resource_file_loader, IDWriteFontFileLoader_iface);
}

static HRESULT WINAPI resourcefontfileloader_QueryInterface(IDWriteFontFileLoader *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader))
    {
        *obj = iface;
        return S_OK;
    }
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI resourcefontfileloader_AddRef(IDWriteFontFileLoader *iface)
{
    struct resource_file_loader *loader = impl_from_IDWriteFontFileLoader(iface);
    return InterlockedIncrement(&loader->refcount);
}

static ULONG WINAPI resourcefontfileloader_Release(IDWriteFontFileLoader *iface)
{
    struct resource_file_loader *loader = impl_from_IDWriteFontFileLoader(iface);
    ULONG refcount = InterlockedDecrement(&loader->refcount);

    if (!refcount)
        free(loader);

    return refcount;
}

static HRESULT WINAPI resourcefontfileloader_CreateStreamFromKey(IDWriteFontFileLoader *iface, const void *ref_key,
        UINT32 key_size, IDWriteFontFileStream **stream)
{
    LPVOID data;
    DWORD size;
    HGLOBAL mem;

    mem = LoadResource(GetModuleHandleA(NULL), *(HRSRC*)ref_key);
    ok(mem != NULL, "Failed to lock font resource\n");
    if (mem)
    {
        size = SizeofResource(GetModuleHandleA(NULL), *(HRSRC*)ref_key);
        data = LockResource(mem);
        return create_fontdatastream(data, size, stream);
    }
    return E_FAIL;
}

static const struct IDWriteFontFileLoaderVtbl resourcefontfileloadervtbl =
{
    resourcefontfileloader_QueryInterface,
    resourcefontfileloader_AddRef,
    resourcefontfileloader_Release,
    resourcefontfileloader_CreateStreamFromKey
};

static IDWriteFontFileLoader *create_resource_file_loader(void)
{
    struct resource_file_loader *object;

    object = calloc(1, sizeof(*object));

    object->IDWriteFontFileLoader_iface.lpVtbl = &resourcefontfileloadervtbl;
    object->refcount = 1;

    return &object->IDWriteFontFileLoader_iface;
}

struct resource_collection_loader
{
    IDWriteFontCollectionLoader IDWriteFontCollectionLoader_iface;
    LONG refcount;
    IDWriteFontFileLoader *loader;
};

static inline struct resource_collection_loader *impl_from_IDWriteFontFileCollectionLoader(IDWriteFontCollectionLoader* iface)
{
    return CONTAINING_RECORD(iface, struct resource_collection_loader, IDWriteFontCollectionLoader_iface);
}

static HRESULT WINAPI resourcecollectionloader_QueryInterface(IDWriteFontCollectionLoader *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontCollectionLoader))
    {
        *obj = iface;
        IDWriteFontCollectionLoader_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI resourcecollectionloader_AddRef(IDWriteFontCollectionLoader *iface)
{
    struct resource_collection_loader *loader = impl_from_IDWriteFontFileCollectionLoader(iface);
    return InterlockedIncrement(&loader->refcount);
}

static ULONG WINAPI resourcecollectionloader_Release(IDWriteFontCollectionLoader *iface)
{
    struct resource_collection_loader *loader = impl_from_IDWriteFontFileCollectionLoader(iface);
    ULONG refcount = InterlockedDecrement(&loader->refcount);

    if (!refcount)
    {
        IDWriteFontFileLoader_Release(loader->loader);
        free(loader);
    }

    return refcount;
}

static HRESULT WINAPI resourcecollectionloader_CreateEnumeratorFromKey(IDWriteFontCollectionLoader *iface,
        IDWriteFactory *factory, const void *key, UINT32 key_size, IDWriteFontFileEnumerator **enumerator)
{
    struct resource_collection_loader *loader = impl_from_IDWriteFontFileCollectionLoader(iface);
    IDWriteFontFile *font_file;
    HRESULT hr;

    hr = IDWriteFactory_CreateCustomFontFileReference(factory, key, key_size, loader->loader, &font_file);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = create_enumerator(font_file, enumerator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteFontFile_Release(font_file);
    return hr;
}

static const struct IDWriteFontCollectionLoaderVtbl resourcecollectionloadervtbl =
{
    resourcecollectionloader_QueryInterface,
    resourcecollectionloader_AddRef,
    resourcecollectionloader_Release,
    resourcecollectionloader_CreateEnumeratorFromKey
};

static IDWriteFontCollectionLoader *create_resource_collection_loader(IDWriteFontFileLoader *loader)
{
    struct resource_collection_loader *object;

    object = calloc(1, sizeof(*object));
    object->IDWriteFontCollectionLoader_iface.lpVtbl = &resourcecollectionloadervtbl;
    object->refcount = 1;
    object->loader = loader;
    IDWriteFontFileLoader_AddRef(object->loader);

    return &object->IDWriteFontCollectionLoader_iface;
}

struct testanalysissink
{
    IDWriteTextAnalysisSink IDWriteTextAnalysisSink_iface;
    DWRITE_SCRIPT_ANALYSIS sa; /* last analysis, with SetScriptAnalysis() */
};

static inline struct testanalysissink *impl_from_IDWriteTextAnalysisSink(IDWriteTextAnalysisSink *iface)
{
    return CONTAINING_RECORD(iface, struct testanalysissink, IDWriteTextAnalysisSink_iface);
}

/* test IDWriteTextAnalysisSink */
static HRESULT WINAPI analysissink_QueryInterface(IDWriteTextAnalysisSink *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextAnalysisSink) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI analysissink_AddRef(IDWriteTextAnalysisSink *iface)
{
    return 2;
}

static ULONG WINAPI analysissink_Release(IDWriteTextAnalysisSink *iface)
{
    return 1;
}

static HRESULT WINAPI analysissink_SetScriptAnalysis(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, DWRITE_SCRIPT_ANALYSIS const* sa)
{
    struct testanalysissink *sink = impl_from_IDWriteTextAnalysisSink(iface);
    sink->sa = *sa;
    return S_OK;
}

static HRESULT WINAPI analysissink_SetLineBreakpoints(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, DWRITE_LINE_BREAKPOINT const* breakpoints)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI analysissink_SetBidiLevel(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, UINT8 explicitLevel, UINT8 resolvedLevel)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI analysissink_SetNumberSubstitution(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, IDWriteNumberSubstitution* substitution)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static IDWriteTextAnalysisSinkVtbl analysissinkvtbl = {
    analysissink_QueryInterface,
    analysissink_AddRef,
    analysissink_Release,
    analysissink_SetScriptAnalysis,
    analysissink_SetLineBreakpoints,
    analysissink_SetBidiLevel,
    analysissink_SetNumberSubstitution
};

static struct testanalysissink analysissink = {
    { &analysissinkvtbl },
    { 0 }
};

/* test IDWriteTextAnalysisSource */
static HRESULT WINAPI analysissource_QueryInterface(IDWriteTextAnalysisSource *iface,
    REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextAnalysisSource) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteTextAnalysisSource_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI analysissource_AddRef(IDWriteTextAnalysisSource *iface)
{
    return 2;
}

static ULONG WINAPI analysissource_Release(IDWriteTextAnalysisSource *iface)
{
    return 1;
}

static const WCHAR *g_source;

static HRESULT WINAPI analysissource_GetTextAtPosition(IDWriteTextAnalysisSource *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    if (position >= lstrlenW(g_source))
    {
        *text = NULL;
        *text_len = 0;
    }
    else
    {
        *text = &g_source[position];
        *text_len = lstrlenW(g_source) - position;
    }

    return S_OK;
}

static HRESULT WINAPI analysissource_GetTextBeforePosition(IDWriteTextAnalysisSource *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static DWRITE_READING_DIRECTION WINAPI analysissource_GetParagraphReadingDirection(
    IDWriteTextAnalysisSource *iface)
{
    ok(0, "unexpected\n");
    return DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
}

static HRESULT WINAPI analysissource_GetLocaleName(IDWriteTextAnalysisSource *iface,
    UINT32 position, UINT32* text_len, WCHAR const** locale)
{
    *locale = NULL;
    *text_len = 0;
    return S_OK;
}

static HRESULT WINAPI analysissource_GetNumberSubstitution(IDWriteTextAnalysisSource *iface,
    UINT32 position, UINT32* text_len, IDWriteNumberSubstitution **substitution)
{
    ok(0, "unexpected\n");
    return E_NOTIMPL;
}

static IDWriteTextAnalysisSourceVtbl analysissourcevtbl = {
    analysissource_QueryInterface,
    analysissource_AddRef,
    analysissource_Release,
    analysissource_GetTextAtPosition,
    analysissource_GetTextBeforePosition,
    analysissource_GetParagraphReadingDirection,
    analysissource_GetLocaleName,
    analysissource_GetNumberSubstitution
};

static IDWriteTextAnalysisSource analysissource = { &analysissourcevtbl };

static void *create_factory_iid(REFIID riid)
{
    IUnknown *factory = NULL;
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, riid, &factory);
    return factory;
}

static IDWriteFactory *create_factory(void)
{
    IDWriteFactory *factory = create_factory_iid(&IID_IDWriteFactory);
    ok(factory != NULL, "Failed to create factory.\n");
    return factory;
}

/* obvious limitation is that only last script data is returned, so this
   helper is suitable for single script strings only */
static void get_script_analysis(const WCHAR *str, UINT32 len, DWRITE_SCRIPT_ANALYSIS *sa)
{
    IDWriteTextAnalyzer *analyzer;
    IDWriteFactory *factory;
    HRESULT hr;

    g_source = str;

    factory = create_factory();
    hr = IDWriteFactory_CreateTextAnalyzer(factory, &analyzer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextAnalyzer_AnalyzeScript(analyzer, &analysissource, 0, len, &analysissink.IDWriteTextAnalysisSink_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    *sa = analysissink.sa;
    IDWriteFactory_Release(factory);
}

static IDWriteFontFace *get_fontface_from_format(IDWriteTextFormat *format)
{
    IDWriteFontCollection *collection;
    IDWriteFontFamily *family;
    IDWriteFontFace *fontface;
    IDWriteFont *font;
    WCHAR nameW[255];
    UINT32 index;
    BOOL exists;
    HRESULT hr;

    hr = IDWriteTextFormat_GetFontCollection(format, &collection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_GetFontFamilyName(format, nameW, ARRAY_SIZE(nameW));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontCollection_FindFamilyName(collection, nameW, &index, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontCollection_GetFontFamily(collection, index, &family);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDWriteFontCollection_Release(collection);

    hr = IDWriteFontFamily_GetFirstMatchingFont(family,
        IDWriteTextFormat_GetFontWeight(format),
        IDWriteTextFormat_GetFontStretch(format),
        IDWriteTextFormat_GetFontStyle(format),
        &font);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteFont_Release(font);
    IDWriteFontFamily_Release(family);

    return fontface;
}

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

enum drawcall_modifiers_kind {
    DRAW_EFFECT         = 0x1000
};

enum drawcall_kind {
    DRAW_GLYPHRUN      = 0,
    DRAW_UNDERLINE     = 1,
    DRAW_STRIKETHROUGH = 2,
    DRAW_INLINE        = 3,
    DRAW_LAST_KIND     = 4,
    DRAW_TOTAL_KINDS   = 5,
    DRAW_KINDS_MASK    = 0xff
};

static const char *get_draw_kind_name(unsigned short kind)
{
    static const char *kind_names[] = {
      "GLYPH_RUN",
      "UNDERLINE",
      "STRIKETHROUGH",
      "INLINE",
      "END_OF_SEQ",

      "GLYPH_RUN|EFFECT",
      "UNDERLINE|EFFECT",
      "STRIKETHROUGH|EFFECT",
      "INLINE|EFFECT",
      "END_OF_SEQ"
    };
    if ((kind & DRAW_KINDS_MASK) > DRAW_LAST_KIND)
        return "unknown";
    return (kind & DRAW_EFFECT) ? kind_names[(kind & DRAW_KINDS_MASK) + DRAW_TOTAL_KINDS] :
        kind_names[kind];
}

struct drawcall_entry {
    enum drawcall_kind kind;
    WCHAR string[10]; /* only meaningful for DrawGlyphRun() */
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    UINT32 glyphcount; /* only meaningful for DrawGlyphRun() */
    UINT32 bidilevel;
};

struct drawcall_sequence
{
    int count;
    int size;
    struct drawcall_entry *sequence;
};

struct drawtestcontext {
    unsigned short kind;
    BOOL todo;
    int *failcount;
    const char *file;
    int line;
};

#define NUM_CALL_SEQUENCES 1
#define RENDERER_ID        0
static struct drawcall_sequence *sequences[NUM_CALL_SEQUENCES];
static struct drawcall_sequence *expected_seq[1];

static void add_call(struct drawcall_sequence **seq, int sequence_index, const struct drawcall_entry *call)
{
    struct drawcall_sequence *call_seq = seq[sequence_index];

    if (!call_seq->sequence) {
        call_seq->size = 10;
        call_seq->sequence = malloc(call_seq->size * sizeof(*call_seq->sequence));
    }

    if (call_seq->count == call_seq->size) {
        call_seq->size *= 2;
        call_seq->sequence = realloc(call_seq->sequence, call_seq->size * sizeof(*call_seq->sequence));
    }

    assert(call_seq->sequence);
    call_seq->sequence[call_seq->count++] = *call;
}

static inline void flush_sequence(struct drawcall_sequence **seg, int sequence_index)
{
    struct drawcall_sequence *call_seq = seg[sequence_index];

    free(call_seq->sequence);
    call_seq->sequence = NULL;
    call_seq->count = call_seq->size = 0;
}

static void init_call_sequences(struct drawcall_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        seq[i] = calloc(1, sizeof(*seq[i]));
}

static void ok_sequence_(struct drawcall_sequence **seq, int sequence_index,
    const struct drawcall_entry *expected, const char *context, BOOL todo,
    const char *file, int line)
{
    static const struct drawcall_entry end_of_sequence = { DRAW_LAST_KIND };
    struct drawcall_sequence *call_seq = seq[sequence_index];
    const struct drawcall_entry *actual, *sequence;
    int failcount = 0;

    add_call(seq, sequence_index, &end_of_sequence);

    sequence = call_seq->sequence;
    actual = sequence;

    while (expected->kind != DRAW_LAST_KIND && actual->kind != DRAW_LAST_KIND) {
        if (expected->kind != actual->kind) {
            if (todo) {
                failcount++;
                todo_wine
                    ok_(file, line) (0, "%s: call %s was expected, but got call %s instead\n",
                        context, get_draw_kind_name(expected->kind), get_draw_kind_name(actual->kind));

                flush_sequence(seq, sequence_index);
                return;
            }
            else
                ok_(file, line) (0, "%s: call %s was expected, but got call %s instead\n",
                    context, get_draw_kind_name(expected->kind), get_draw_kind_name(actual->kind));
        }
        else if ((expected->kind & DRAW_KINDS_MASK) == DRAW_GLYPHRUN) {
            int cmp = lstrcmpW(expected->string, actual->string);
            if (cmp != 0 && todo) {
                failcount++;
            todo_wine
                ok_(file, line) (0, "%s: glyphrun string %s was expected, but got %s instead\n",
                    context, wine_dbgstr_w(expected->string), wine_dbgstr_w(actual->string));
            }
            else
                ok_(file, line) (cmp == 0, "%s: glyphrun string %s was expected, but got %s instead\n",
                    context, wine_dbgstr_w(expected->string), wine_dbgstr_w(actual->string));

            cmp = lstrcmpW(expected->locale, actual->locale);
            if (cmp != 0 && todo) {
                failcount++;
            todo_wine
                ok_(file, line) (0, "%s: glyph run locale %s was expected, but got %s instead\n",
                    context, wine_dbgstr_w(expected->locale), wine_dbgstr_w(actual->locale));
            }
            else
                ok_(file, line) (cmp == 0, "%s: glyph run locale %s was expected, but got %s instead\n",
                    context, wine_dbgstr_w(expected->locale), wine_dbgstr_w(actual->locale));

            if (expected->glyphcount != actual->glyphcount && todo) {
                failcount++;
            todo_wine
                ok_(file, line) (0, "%s: wrong glyph count, %u was expected, but got %u instead\n",
                    context, expected->glyphcount, actual->glyphcount);
            }
            else
                ok_(file, line) (expected->glyphcount == actual->glyphcount,
                    "%s: wrong glyph count, %u was expected, but got %u instead\n",
                    context, expected->glyphcount, actual->glyphcount);

            if (expected->bidilevel != actual->bidilevel && todo) {
                failcount++;
            todo_wine
                ok_(file, line) (0, "%s: wrong bidi level, %u was expected, but got %u instead\n",
                    context, expected->bidilevel, actual->bidilevel);
            }
            else
                ok_(file, line) (expected->bidilevel == actual->bidilevel,
                    "%s: wrong bidi level, %u was expected, but got %u instead\n",
                    context, expected->bidilevel, actual->bidilevel);
        }
        else if ((expected->kind & DRAW_KINDS_MASK) == DRAW_UNDERLINE) {
            int cmp = lstrcmpW(expected->locale, actual->locale);
            if (cmp != 0 && todo) {
                failcount++;
            todo_wine
                ok_(file, line) (0, "%s: underline locale %s was expected, but got %s instead\n",
                    context, wine_dbgstr_w(expected->locale), wine_dbgstr_w(actual->locale));
            }
            else
                ok_(file, line) (cmp == 0, "%s: underline locale %s was expected, but got %s instead\n",
                    context, wine_dbgstr_w(expected->locale), wine_dbgstr_w(actual->locale));
        }
        expected++;
        actual++;
    }

    if (todo) {
        todo_wine {
            if (expected->kind != DRAW_LAST_KIND || actual->kind != DRAW_LAST_KIND) {
                failcount++;
                ok_(file, line) (0, "%s: the call sequence is not complete: expected %s - actual %s\n",
                    context, get_draw_kind_name(expected->kind), get_draw_kind_name(actual->kind));
            }
        }
    }
    else if (expected->kind != DRAW_LAST_KIND || actual->kind != DRAW_LAST_KIND)
        ok_(file, line) (0, "%s: the call sequence is not complete: expected %s - actual %s\n",
            context, get_draw_kind_name(expected->kind), get_draw_kind_name(actual->kind));

    if (todo && !failcount) /* succeeded yet marked todo */
        todo_wine
            ok_(file, line)(1, "%s: marked \"todo_wine\" but succeeds\n", context);

    flush_sequence(seq, sequence_index);
}

#define ok_sequence(seq, index, exp, contx, todo) \
        ok_sequence_(seq, index, (exp), (contx), (todo), __FILE__, __LINE__)

static HRESULT WINAPI testrenderer_QI(IDWriteTextRenderer *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteTextRenderer) ||
        IsEqualIID(riid, &IID_IDWritePixelSnapping) ||
        IsEqualIID(riid, &IID_IUnknown)
    ) {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;

    /* IDWriteTextRenderer1 overrides drawing calls, ignore for now */
    if (IsEqualIID(riid, &IID_IDWriteTextRenderer1))
        return E_NOINTERFACE;

    ok(0, "unexpected QI %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI testrenderer_AddRef(IDWriteTextRenderer *iface)
{
    return 2;
}

static ULONG WINAPI testrenderer_Release(IDWriteTextRenderer *iface)
{
    return 1;
}

struct renderer_context {
    BOOL gdicompat;
    BOOL use_gdi_natural;
    BOOL snapping_disabled;
    DWRITE_MATRIX m;
    FLOAT ppdip;
    FLOAT originX;
    FLOAT originY;
    IDWriteTextFormat *format;
    const WCHAR *familyW;
};

static HRESULT WINAPI testrenderer_IsPixelSnappingDisabled(IDWriteTextRenderer *iface,
    void *context, BOOL *disabled)
{
    struct renderer_context *ctxt = (struct renderer_context*)context;
    if (ctxt)
        *disabled = ctxt->snapping_disabled;
    else
        *disabled = TRUE;
    return S_OK;
}

static HRESULT WINAPI testrenderer_GetCurrentTransform(IDWriteTextRenderer *iface,
    void *context, DWRITE_MATRIX *m)
{
    struct renderer_context *ctxt = (struct renderer_context*)context;
    ok(!ctxt->snapping_disabled, "expected enabled snapping\n");
    *m = ctxt->m;
    return S_OK;
}

static HRESULT WINAPI testrenderer_GetPixelsPerDip(IDWriteTextRenderer *iface,
    void *context, FLOAT *pixels_per_dip)
{
    struct renderer_context *ctxt = (struct renderer_context*)context;
    *pixels_per_dip = ctxt->ppdip;
    return S_OK;
}

#define TEST_MEASURING_MODE(ctxt, mode) test_measuring_mode(ctxt, mode, __LINE__)
static void test_measuring_mode(const struct renderer_context *ctxt, DWRITE_MEASURING_MODE mode, int line)
{
    if (ctxt->gdicompat) {
        if (ctxt->use_gdi_natural)
            ok_(__FILE__, line)(mode == DWRITE_MEASURING_MODE_GDI_NATURAL, "got %d\n", mode);
        else
            ok_(__FILE__, line)(mode == DWRITE_MEASURING_MODE_GDI_CLASSIC, "got %d\n", mode);
    }
    else
        ok_(__FILE__, line)(mode == DWRITE_MEASURING_MODE_NATURAL, "got %d\n", mode);
}

static HRESULT WINAPI testrenderer_DrawGlyphRun(IDWriteTextRenderer *iface,
    void *context,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE mode,
    DWRITE_GLYPH_RUN const *run,
    DWRITE_GLYPH_RUN_DESCRIPTION const *descr,
    IUnknown *effect)
{
    struct renderer_context *ctxt = (struct renderer_context*)context;
    struct drawcall_entry entry;
    DWRITE_SCRIPT_ANALYSIS sa;

    if (ctxt) {
        TEST_MEASURING_MODE(ctxt, mode);
        ctxt->originX = baselineOriginX;
        ctxt->originY = baselineOriginY;
    }

    ok(descr->stringLength < ARRAY_SIZE(entry.string), "string is too long\n");
    if (descr->stringLength && descr->stringLength < ARRAY_SIZE(entry.string)) {
        memcpy(entry.string, descr->string, descr->stringLength*sizeof(WCHAR));
        entry.string[descr->stringLength] = 0;
    }
    else
        entry.string[0] = 0;

    /* see what's reported for control codes runs */
    get_script_analysis(descr->string, descr->stringLength, &sa);
    if (sa.shapes == DWRITE_SCRIPT_SHAPES_NO_VISUAL) {
        UINT32 i;

        /* glyphs are not reported at all for control code runs */
        ok(run->glyphCount == 0, "got %u\n", run->glyphCount);
        ok(run->glyphAdvances != NULL, "advances array %p\n", run->glyphAdvances);
        ok(run->glyphOffsets != NULL, "offsets array %p\n", run->glyphOffsets);
        ok(run->fontFace != NULL, "got %p\n", run->fontFace);
        /* text positions are still valid */
        ok(descr->string != NULL, "got string %p\n", descr->string);
        ok(descr->stringLength > 0, "got string length %u\n", descr->stringLength);
        ok(descr->clusterMap != NULL, "clustermap %p\n", descr->clusterMap);
        for (i = 0; i < descr->stringLength; i++)
            ok(descr->clusterMap[i] == i, "got %u\n", descr->clusterMap[i]);
    }

    entry.kind = DRAW_GLYPHRUN;
    if (effect)
        entry.kind |= DRAW_EFFECT;
    ok(lstrlenW(descr->localeName) < LOCALE_NAME_MAX_LENGTH, "unexpectedly long locale name\n");
    lstrcpyW(entry.locale, descr->localeName);
    entry.glyphcount = run->glyphCount;
    entry.bidilevel = run->bidiLevel;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawUnderline(IDWriteTextRenderer *iface,
    void *context,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_UNDERLINE const* underline,
    IUnknown *effect)
{
    struct renderer_context *ctxt = (struct renderer_context*)context;
    struct drawcall_entry entry = { 0 };

    if (ctxt)
        TEST_MEASURING_MODE(ctxt, underline->measuringMode);

    ok(underline->runHeight > 0.0f, "Expected non-zero run height\n");
    if (ctxt && ctxt->format) {
        DWRITE_FONT_METRICS metrics;
        IDWriteFontFace *fontface;
        FLOAT emsize;

        fontface = get_fontface_from_format(ctxt->format);
        emsize = IDWriteTextFormat_GetFontSize(ctxt->format);
        IDWriteFontFace_GetMetrics(fontface, &metrics);

        ok(emsize == metrics.designUnitsPerEm, "Unexpected font size %f\n", emsize);
        /* Expected height is in design units, allow some absolute difference from it. Seems to only happen on Vista */
        ok(fabs(metrics.capHeight - underline->runHeight) < 2.0f, "Expected runHeight %u, got %f, family %s\n",
                metrics.capHeight, underline->runHeight, wine_dbgstr_w(ctxt->familyW));

        IDWriteFontFace_Release(fontface);
    }

    entry.kind = DRAW_UNDERLINE;
    if (effect)
        entry.kind |= DRAW_EFFECT;
    lstrcpyW(entry.locale, underline->localeName);
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawStrikethrough(IDWriteTextRenderer *iface,
    void *context,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_STRIKETHROUGH const* strikethrough,
    IUnknown *effect)
{
    struct renderer_context *ctxt = (struct renderer_context*)context;
    struct drawcall_entry entry = { 0 };

    if (ctxt)
        TEST_MEASURING_MODE(ctxt, strikethrough->measuringMode);

    entry.kind = DRAW_STRIKETHROUGH;
    if (effect)
        entry.kind |= DRAW_EFFECT;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static HRESULT WINAPI testrenderer_DrawInlineObject(IDWriteTextRenderer *iface,
    void *context,
    FLOAT originX,
    FLOAT originY,
    IDWriteInlineObject *object,
    BOOL is_sideways,
    BOOL is_rtl,
    IUnknown *effect)
{
    struct drawcall_entry entry = { 0 };
    entry.kind = DRAW_INLINE;
    if (effect)
        entry.kind |= DRAW_EFFECT;
    add_call(sequences, RENDERER_ID, &entry);
    return S_OK;
}

static const IDWriteTextRendererVtbl testrenderervtbl = {
    testrenderer_QI,
    testrenderer_AddRef,
    testrenderer_Release,
    testrenderer_IsPixelSnappingDisabled,
    testrenderer_GetCurrentTransform,
    testrenderer_GetPixelsPerDip,
    testrenderer_DrawGlyphRun,
    testrenderer_DrawUnderline,
    testrenderer_DrawStrikethrough,
    testrenderer_DrawInlineObject
};

static IDWriteTextRenderer testrenderer = { &testrenderervtbl };

/* test IDWriteInlineObject */
struct test_inline_obj
{
    IDWriteInlineObject IDWriteInlineObject_iface;
    LONG refcount;

    DWRITE_INLINE_OBJECT_METRICS metrics;
    DWRITE_OVERHANG_METRICS overhangs;
};

static inline struct test_inline_obj *impl_from_IDWriteInlineObject(IDWriteInlineObject *iface)
{
    return CONTAINING_RECORD(iface, struct test_inline_obj, IDWriteInlineObject_iface);
}

static HRESULT WINAPI testinlineobj_QI(IDWriteInlineObject *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteInlineObject) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteInlineObject_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testinlineobj_AddRef(IDWriteInlineObject *iface)
{
    struct test_inline_obj *obj = impl_from_IDWriteInlineObject(iface);
    return InterlockedIncrement(&obj->refcount);
}

static ULONG WINAPI testinlineobj_Release(IDWriteInlineObject *iface)
{
    struct test_inline_obj *obj = impl_from_IDWriteInlineObject(iface);
    ULONG refcount = InterlockedDecrement(&obj->refcount);

    if (!refcount)
        free(obj);

    return refcount;
}

static HRESULT WINAPI testinlineobj_Draw(IDWriteInlineObject *iface,
    void* client_drawingontext, IDWriteTextRenderer* renderer,
    FLOAT originX, FLOAT originY, BOOL is_sideways, BOOL is_rtl, IUnknown *drawing_effect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testinlineobj_GetMetrics(IDWriteInlineObject *iface, DWRITE_INLINE_OBJECT_METRICS *metrics)
{
    metrics->width = 123.0;
    return 0x8faecafe;
}

static HRESULT WINAPI testinlineobj_GetOverhangMetrics(IDWriteInlineObject *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testinlineobj_GetBreakConditions(IDWriteInlineObject *iface, DWRITE_BREAK_CONDITION *before,
    DWRITE_BREAK_CONDITION *after)
{
    *before = *after = DWRITE_BREAK_CONDITION_MUST_BREAK;
    return 0x8feacafe;
}

static HRESULT WINAPI testinlineobj2_GetBreakConditions(IDWriteInlineObject *iface, DWRITE_BREAK_CONDITION *before,
    DWRITE_BREAK_CONDITION *after)
{
    *before = *after = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
    return S_OK;
}

static const IDWriteInlineObjectVtbl testinlineobjvtbl = {
    testinlineobj_QI,
    testinlineobj_AddRef,
    testinlineobj_Release,
    testinlineobj_Draw,
    testinlineobj_GetMetrics,
    testinlineobj_GetOverhangMetrics,
    testinlineobj_GetBreakConditions
};

static const IDWriteInlineObjectVtbl testinlineobjvtbl2 =
{
    testinlineobj_QI,
    testinlineobj_AddRef,
    testinlineobj_Release,
    testinlineobj_Draw,
    testinlineobj_GetMetrics,
    testinlineobj_GetOverhangMetrics,
    testinlineobj2_GetBreakConditions
};

static struct test_inline_obj *create_test_inline_object(const IDWriteInlineObjectVtbl *vtable)
{
    struct test_inline_obj *object = calloc(1, sizeof(*object));

    object->IDWriteInlineObject_iface.lpVtbl = vtable;
    object->refcount = 1;

    return object;
}

static HRESULT WINAPI testinlineobj3_GetMetrics(IDWriteInlineObject *iface, DWRITE_INLINE_OBJECT_METRICS *metrics)
{
    struct test_inline_obj *obj = impl_from_IDWriteInlineObject(iface);
    *metrics = obj->metrics;
    return S_OK;
}

static HRESULT WINAPI testinlineobj3_GetOverhangMetrics(IDWriteInlineObject *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    struct test_inline_obj *obj = impl_from_IDWriteInlineObject(iface);
    *overhangs = obj->overhangs;
    /* Return value is ignored. */
    return E_NOTIMPL;
}

static const IDWriteInlineObjectVtbl testinlineobjvtbl3 = {
    testinlineobj_QI,
    testinlineobj_AddRef,
    testinlineobj_Release,
    testinlineobj_Draw,
    testinlineobj3_GetMetrics,
    testinlineobj3_GetOverhangMetrics,
    testinlineobj_GetBreakConditions,
};

struct test_effect
{
    IUnknown IUnknown_iface;
    LONG ref;
};

static inline struct test_effect *test_effect_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct test_effect, IUnknown_iface);
}

static HRESULT WINAPI testeffect_QI(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    ok(0, "Unexpected riid %s.\n", wine_dbgstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testeffect_AddRef(IUnknown *iface)
{
    struct test_effect *effect = test_effect_from_IUnknown(iface);
    return InterlockedIncrement(&effect->ref);
}

static ULONG WINAPI testeffect_Release(IUnknown *iface)
{
    struct test_effect *effect = test_effect_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&effect->ref);

    if (!ref)
        free(effect);

    return ref;
}

static const IUnknownVtbl testeffectvtbl = {
    testeffect_QI,
    testeffect_AddRef,
    testeffect_Release
};

static IUnknown *create_test_effect(void)
{
    struct test_effect *effect;

    effect = calloc(1, sizeof(*effect));
    effect->IUnknown_iface.lpVtbl = &testeffectvtbl;
    effect->ref = 1;

    return &effect->IUnknown_iface;
}

static void test_CreateTextLayout(void)
{
    IDWriteTextLayout4 *layout4;
    IDWriteTextLayout2 *layout2 = NULL;
    IDWriteTextLayout *layout;
    IDWriteTextFormat3 *format3;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, NULL, 0, NULL, 0.0f, 0.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, NULL, 0.0f, 0.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, NULL, 1.0f, 0.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, NULL, 0.0f, 1.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, NULL, 1000.0f, 1000.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create format, hr %#lx.\n", hr);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, NULL, 0, format, 100.0f, 100.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void *)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, -100.0f, 100.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "Unexpected pointer %p.\n", layout);

    layout = (void *)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, 100.0f, -100.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "Unexpected pointer %p.\n", layout);

    layout = (void *)0xdeadbeef;
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, -100.0f, -100.0f, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "Unexpected pointer %p.\n", layout);

    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 0, format, 0.0f, 0.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    IDWriteTextLayout_Release(layout);

    EXPECT_REF(format, 1);
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    EXPECT_REF(format, 1);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout2, (void**)&layout2);
    if (hr == S_OK) {
        IDWriteTextLayout1 *layout1;
        IDWriteTextFormat1 *format1;
        IDWriteTextFormat *format;

        hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextLayout1, (void**)&layout1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IDWriteTextLayout1_Release(layout1);

        EXPECT_REF(layout2, 2);
        hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextFormat1, (void**)&format1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(layout2, 3);

        hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextFormat, (void**)&format);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(format == (IDWriteTextFormat*)format1, "got %p, %p\n", format, format1);
        ok(format != (IDWriteTextFormat*)layout2, "got %p, %p\n", format, layout2);
        EXPECT_REF(layout2, 4);

        hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextLayout1, (void**)&layout1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IDWriteTextLayout1_Release(layout1);

        IDWriteTextFormat1_Release(format1);
        IDWriteTextFormat_Release(format);

        hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat1, (void**)&format1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(layout2, 3);

        hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat, (void**)&format);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(format == (IDWriteTextFormat*)format1, "got %p, %p\n", format, format1);
        EXPECT_REF(layout2, 4);

        IDWriteTextFormat1_Release(format1);
        IDWriteTextFormat_Release(format);
    }
    else
        win_skip("IDWriteTextLayout2 is not supported.\n");

    if (layout2 && SUCCEEDED(IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextLayout4, (void **)&layout4)))
    {
        hr = IDWriteTextLayout4_QueryInterface(layout4, &IID_IDWriteTextFormat3, (void **)&format3);
        ok(hr == S_OK, "Failed to get text format, hr %#lx.\n", hr);
        IDWriteTextFormat3_Release(format3);
    }
    else
        win_skip("IDWriteTextLayout4 is not supported.\n");

    if (layout2)
        IDWriteTextLayout2_Release(layout2);
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static DWRITE_MATRIX layoutcreate_transforms[] = {
    { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 },
    { 1.0, 0.0, 0.0, 1.0, 0.3, 0.2 },
    { 1.0, 0.0, 0.0, 1.0,-0.3,-0.2 },

    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
    { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
    { 1.0, 2.0, 0.5, 1.0, 0.0, 0.0 },
};

static void test_CreateGdiCompatibleTextLayout(void)
{
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    FLOAT dimension;
    HRESULT hr;
    int i;

    factory = create_factory();

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, NULL, 0, NULL, 0.0f, 0.0f, 0.0f, NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, NULL, 0.0f, 0.0f, 0.0f, NULL,
            FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, NULL, 1.0f, 0.0f, 0.0f, NULL,
            FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, NULL, 1.0f, 0.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, NULL, 1000.0f, 1000.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    /* create with text format */
    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);
    EXPECT_REF(format, 1);

    layout = (void*)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, NULL, 0, format, 100.0f, 100.0f, 1.0f, NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "got %p\n", layout);

    layout = (void *)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, -100.0f, 100.0f, 1.0f,
            NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "Unexpected pointer %p.\n", layout);

    layout = (void *)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, 100.0f, -100.0f, 1.0f,
            NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "Unexpected pointer %p.\n", layout);

    layout = (void *)0xdeadbeef;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, -100.0f, -100.0f, 1.0f,
            NULL, FALSE, &layout);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(layout == NULL, "Unexpected pointer %p.\n", layout);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    EXPECT_REF(format, 1);
    EXPECT_REF(layout, 1);

    IDWriteTextLayout_AddRef(layout);
    EXPECT_REF(format, 1);
    EXPECT_REF(layout, 2);
    IDWriteTextLayout_Release(layout);
    IDWriteTextLayout_Release(layout);

    /* zero length string is okay */
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 0, format, 100.0f, 100.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    dimension = IDWriteTextLayout_GetMaxWidth(layout);
    ok(dimension == 100.0, "got %f\n", dimension);

    dimension = IDWriteTextLayout_GetMaxHeight(layout);
    ok(dimension == 100.0, "got %f\n", dimension);

    IDWriteTextLayout_Release(layout);

    /* negative, zero ppdip */
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 1, format, 100.0f, 100.0f, -1.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    IDWriteTextLayout_Release(layout);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 1, format, 100.0f, 100.0f, 0.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    IDWriteTextLayout_Release(layout);

    /* transforms */
    for (i = 0; i < ARRAY_SIZE(layoutcreate_transforms); ++i)
    {
        hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 1, format, 100.0f, 100.0f, 1.0f,
                &layoutcreate_transforms[i], FALSE, &layout);
        ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
        IDWriteTextLayout_Release(layout);
    }

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_CreateTextFormat(void)
{
    IDWriteFontCollection *collection, *syscoll;
    DWRITE_PARAGRAPH_ALIGNMENT paralign;
    DWRITE_READING_DIRECTION readdir;
    DWRITE_WORD_WRAPPING wrapping;
    DWRITE_TEXT_ALIGNMENT align;
    DWRITE_FLOW_DIRECTION flow;
    DWRITE_LINE_SPACING_METHOD method;
    DWRITE_TRIMMING trimming;
    IDWriteTextFormat *format;
    FLOAT spacing, baseline;
    IDWriteInlineObject *trimmingsign;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    /* zero/negative font size */
    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 0.0f, L"en-us", &format);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, -10.0f, L"en-us", &format);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* invalid font properties */
    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, 1000, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_ITALIC + 1, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_ITALIC,
            10, 10.0f, L"en-us", &format);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_ITALIC,
            DWRITE_FONT_STRETCH_UNDEFINED, 10.0f, L"en-us", &format);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* empty family name */
    hr = IDWriteFactory_CreateTextFormat(factory, L"", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);
    IDWriteTextFormat_Release(format);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    if (0) /* crashes on native */
        hr = IDWriteTextFormat_GetFontCollection(format, NULL);

    collection = NULL;
    hr = IDWriteTextFormat_GetFontCollection(format, &collection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(collection != NULL, "got %p\n", collection);

    hr = IDWriteFactory_GetSystemFontCollection(factory, &syscoll, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(collection == syscoll, "got %p, was %p\n", syscoll, collection);
    IDWriteFontCollection_Release(syscoll);
    IDWriteFontCollection_Release(collection);

    /* default format properties */
    align = IDWriteTextFormat_GetTextAlignment(format);
    ok(align == DWRITE_TEXT_ALIGNMENT_LEADING, "got %d\n", align);

    paralign = IDWriteTextFormat_GetParagraphAlignment(format);
    ok(paralign == DWRITE_PARAGRAPH_ALIGNMENT_NEAR, "got %d\n", paralign);

    wrapping = IDWriteTextFormat_GetWordWrapping(format);
    ok(wrapping == DWRITE_WORD_WRAPPING_WRAP, "got %d\n", wrapping);

    readdir = IDWriteTextFormat_GetReadingDirection(format);
    ok(readdir == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT, "got %d\n", readdir);

    flow = IDWriteTextFormat_GetFlowDirection(format);
    ok(flow == DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM, "got %d\n", flow);

    hr = IDWriteTextFormat_GetLineSpacing(format, &method, &spacing, &baseline);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(spacing == 0.0, "got %f\n", spacing);
    ok(baseline == 0.0, "got %f\n", baseline);
    ok(method == DWRITE_LINE_SPACING_METHOD_DEFAULT, "got %d\n", method);

    trimming.granularity = DWRITE_TRIMMING_GRANULARITY_WORD;
    trimming.delimiter = 10;
    trimming.delimiterCount = 10;
    trimmingsign = (void*)0xdeadbeef;
    hr = IDWriteTextFormat_GetTrimming(format, &trimming, &trimmingsign);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(trimming.granularity == DWRITE_TRIMMING_GRANULARITY_NONE, "got %d\n", trimming.granularity);
    ok(trimming.delimiter == 0, "got %d\n", trimming.delimiter);
    ok(trimming.delimiterCount == 0, "got %d\n", trimming.delimiterCount);
    ok(trimmingsign == NULL, "got %p\n", trimmingsign);

    /* setters */
    hr = IDWriteTextFormat_SetTextAlignment(format, DWRITE_TEXT_ALIGNMENT_LEADING);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetTextAlignment(format, DWRITE_TEXT_ALIGNMENT_JUSTIFIED+1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetParagraphAlignment(format, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetParagraphAlignment(format, DWRITE_PARAGRAPH_ALIGNMENT_CENTER+1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetWordWrapping(format, DWRITE_WORD_WRAPPING_WRAP);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetWordWrapping(format, DWRITE_WORD_WRAPPING_CHARACTER+1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetReadingDirection(format, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);


    hr = IDWriteTextFormat_SetTrimming(format, &trimming, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* invalid granularity */
    trimming.granularity = 10;
    trimming.delimiter = 0;
    trimming.delimiterCount = 0;
    hr = IDWriteTextFormat_SetTrimming(format, &trimming, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_GetLocaleName(void)
{
    IDWriteTextFormat *format, *format2;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    WCHAR buff[10];
    UINT32 len;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"ru", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 0, format, 100.0f, 100.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat, (void**)&format2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = IDWriteTextFormat_GetLocaleNameLength(format2);
    ok(len == 2, "got %u\n", len);
    len = IDWriteTextFormat_GetLocaleNameLength(format);
    ok(len == 2, "got %u\n", len);
    hr = IDWriteTextFormat_GetLocaleName(format2, buff, len);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    hr = IDWriteTextFormat_GetLocaleName(format2, buff, len+1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buff, L"ru"), "Unexpected locale name %s.\n", wine_dbgstr_w(buff));
    hr = IDWriteTextFormat_GetLocaleName(format, buff, len);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    hr = IDWriteTextFormat_GetLocaleName(format, buff, len+1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buff, L"ru"), "Unexpected locale name %s.\n", wine_dbgstr_w(buff));

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteTextFormat_Release(format2);
    IDWriteFactory_Release(factory);
}

static const struct drawcall_entry drawellipsis_seq[] = {
    { DRAW_GLYPHRUN, {0x2026, 0}, {'e','n','-','g','b',0}, 1 },
    { DRAW_LAST_KIND }
};

static void test_CreateEllipsisTrimmingSign(void)
{
    DWRITE_INLINE_OBJECT_METRICS metrics;
    DWRITE_BREAK_CONDITION before, after;
    struct renderer_context ctxt;
    IDWriteTextFormat *format;
    IDWriteInlineObject *sign;
    IDWriteFactory *factory;
    IUnknown *unk, *effect;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-GB", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    sign = (void *)0xdeadbeef;
    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, NULL, &sign);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!sign, "Unexpected pointer %p.\n", sign);

    EXPECT_REF(format, 1);
    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(format, 1);

    hr = IDWriteInlineObject_QueryInterface(sign, &IID_IDWriteTextLayout, (void**)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

if (0) {/* crashes on native */
    hr = IDWriteInlineObject_GetBreakConditions(sign, NULL, NULL);
    hr = IDWriteInlineObject_GetMetrics(sign, NULL);
}
    metrics.width = 0.0;
    metrics.height = 123.0;
    metrics.baseline = 123.0;
    metrics.supportsSideways = TRUE;
    hr = IDWriteInlineObject_GetMetrics(sign, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(metrics.width > 0.0, "got %.2f\n", metrics.width);
    ok(metrics.height == 0.0, "got %.2f\n", metrics.height);
    ok(metrics.baseline == 0.0, "got %.2f\n", metrics.baseline);
    ok(!metrics.supportsSideways, "got %d\n", metrics.supportsSideways);

    before = after = DWRITE_BREAK_CONDITION_CAN_BREAK;
    hr = IDWriteInlineObject_GetBreakConditions(sign, &before, &after);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(before == DWRITE_BREAK_CONDITION_NEUTRAL, "got %d\n", before);
    ok(after == DWRITE_BREAK_CONDITION_NEUTRAL, "got %d\n", after);

    /* Draw tests */
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteInlineObject_Draw(sign, NULL, &testrenderer, 0.0, 0.0, FALSE, FALSE, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, drawellipsis_seq, "ellipsis sign draw test", FALSE);

    effect = create_test_effect();

    EXPECT_REF(effect, 1);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteInlineObject_Draw(sign, NULL, &testrenderer, 0.0f, 0.0f, FALSE, FALSE, effect);
    ok(hr == S_OK, "Failed to draw trimming sign, hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, drawellipsis_seq, "ellipsis sign draw with effect test", FALSE);
    EXPECT_REF(effect, 1);

    IUnknown_Release(effect);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteInlineObject_Draw(sign, NULL, &testrenderer, 0.0f, 0.0f, FALSE, FALSE, (void *)0xdeadbeef);
    ok(hr == S_OK, "Failed to draw trimming sign, hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, drawellipsis_seq, "ellipsis sign draw with effect test", FALSE);

    memset(&ctxt, 0, sizeof(ctxt));
    hr = IDWriteInlineObject_Draw(sign, &ctxt, &testrenderer, 123.0f, 456.0f, FALSE, FALSE, NULL);
    ok(hr == S_OK, "Failed to draw trimming sign, hr %#lx.\n", hr);
    ok(ctxt.originX == 123.0f && ctxt.originY == 456.0f, "Unexpected drawing origin\n");

    IDWriteInlineObject_Release(sign);

    /* Centered format */
    hr = IDWriteTextFormat_SetTextAlignment(format, DWRITE_TEXT_ALIGNMENT_CENTER);
    ok(hr == S_OK, "Failed to set text alignment, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&ctxt, 0, sizeof(ctxt));
    hr = IDWriteInlineObject_Draw(sign, &ctxt, &testrenderer, 123.0f, 456.0f, FALSE, FALSE, NULL);
    ok(hr == S_OK, "Failed to draw trimming sign, hr %#lx.\n", hr);
    ok(ctxt.originX == 123.0f && ctxt.originY == 456.0f, "Unexpected drawing origin\n");

    IDWriteInlineObject_Release(sign);

    /* non-orthogonal flow/reading combination */
    hr = IDWriteTextFormat_SetReadingDirection(format, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* vista, win7 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK) {
        hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
        ok(hr == DWRITE_E_FLOWDIRECTIONCONFLICTS, "Unexpected hr %#lx.\n", hr);
    }

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_fontweight(void)
{
    IDWriteTextFormat *format, *fmt2;
    IDWriteTextLayout *layout;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    FLOAT size;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0, L"ru", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat, (void**)&fmt2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    weight = IDWriteTextFormat_GetFontWeight(fmt2);
    ok(weight == DWRITE_FONT_WEIGHT_BOLD, "got %u\n", weight);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontWeight(layout, 0, &weight, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 0 && range.length == ~0u, "got %u, %u\n", range.startPosition, range.length);

    range.startPosition = 0;
    range.length = 6;
    hr = IDWriteTextLayout_SetFontWeight(layout, DWRITE_FONT_WEIGHT_NORMAL, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontWeight(layout, 0, &weight, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 0 && range.length == 6, "got %u, %u\n", range.startPosition, range.length);

    /* IDWriteTextFormat methods output doesn't reflect layout changes */
    weight = IDWriteTextFormat_GetFontWeight(fmt2);
    ok(weight == DWRITE_FONT_WEIGHT_BOLD, "got %u\n", weight);

    range.length = 0;
    weight = DWRITE_FONT_WEIGHT_BOLD;
    hr = IDWriteTextLayout_GetFontWeight(layout, 0, &weight, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL, "got %d\n", weight);
    ok(range.length == 6, "got %d\n", range.length);

    range.startPosition = 0;
    range.length = 6;
    hr = IDWriteTextLayout_SetFontWeight(layout, 1000, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    size = IDWriteTextLayout_GetMaxWidth(layout);
    ok(size == 100.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxWidth(layout, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    size = IDWriteTextLayout_GetMaxWidth(layout);
    ok(size == 0.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxWidth(layout, -1.0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    size = IDWriteTextLayout_GetMaxWidth(layout);
    ok(size == 0.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxWidth(layout, 100.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    size = IDWriteTextLayout_GetMaxWidth(layout);
    ok(size == 100.0, "got %.2f\n", size);

    size = IDWriteTextLayout_GetMaxHeight(layout);
    ok(size == 100.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxHeight(layout, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    size = IDWriteTextLayout_GetMaxHeight(layout);
    ok(size == 0.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxHeight(layout, -1.0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    size = IDWriteTextLayout_GetMaxHeight(layout);
    ok(size == 0.0, "got %.2f\n", size);

    hr = IDWriteTextLayout_SetMaxHeight(layout, 100.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    size = IDWriteTextLayout_GetMaxHeight(layout);
    ok(size == 100.0, "got %.2f\n", size);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(fmt2);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetInlineObject(void)
{
    IDWriteInlineObject *inlineobj, *inlineobj2, *inlinetest;
    DWRITE_TEXT_RANGE range, r2;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"ru", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(inlineobj, 1);
    EXPECT_REF(inlineobj2, 1);

    inlinetest = (void*)0x1;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inlinetest == NULL, "got %p\n", inlinetest);

    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(inlineobj, 2);

    inlinetest = (void*)0x1;
    hr = IDWriteTextLayout_GetInlineObject(layout, 2, &inlinetest, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inlinetest == NULL, "got %p\n", inlinetest);

    inlinetest = NULL;
    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, &r2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 2, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);

    /* get from somewhere inside a range */
    inlinetest = NULL;
    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 1, &inlinetest, &r2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 2, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj2, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    inlinetest = NULL;
    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 1, &inlinetest, &r2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inlinetest == inlineobj2, "got %p\n", inlinetest);
    ok(r2.startPosition == 1 && r2.length == 1, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);
    EXPECT_REF(inlineobj2, 2);

    inlinetest = NULL;
    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, &r2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 1, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, &r2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 2, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);

    range.startPosition = 1;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(inlineobj, 2);

    r2.startPosition = r2.length = 100;
    hr = IDWriteTextLayout_GetInlineObject(layout, 0, &inlinetest, &r2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inlinetest == inlineobj, "got %p\n", inlinetest);
    ok(r2.startPosition == 0 && r2.length == 3, "got %d, %d\n", r2.startPosition, r2.length);
    IDWriteInlineObject_Release(inlinetest);

    EXPECT_REF(inlineobj, 2);
    EXPECT_REF(inlineobj2, 1);

    IDWriteTextLayout_Release(layout);

    EXPECT_REF(inlineobj, 1);

    IDWriteInlineObject_Release(inlineobj);
    IDWriteInlineObject_Release(inlineobj2);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

/* drawing calls sequence doesn't depend on run order, instead all runs are
   drawn first, inline objects next and then underline/strikes */
static const struct drawcall_entry draw_seq[] = {
    { DRAW_GLYPHRUN, {'s',0}, {'r','u',0}, 1 },
    { DRAW_GLYPHRUN, {'r','i',0}, {'r','u',0}, 2 },
    { DRAW_GLYPHRUN|DRAW_EFFECT, {'n',0}, {'r','u',0}, 1 },
    { DRAW_GLYPHRUN, {'g',0}, {'r','u',0}, 1 },
    { DRAW_INLINE },
    { DRAW_UNDERLINE, {0}, {'r','u',0} },
    { DRAW_STRIKETHROUGH },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_trimmed_seq[] = {
    { DRAW_GLYPHRUN, {'a',0}, {'e','n','-','u','s',0}, 1 },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq2[] = {
    { DRAW_GLYPHRUN, {'s',0}, {'r','u',0}, 1 },
    { DRAW_GLYPHRUN, {'t',0}, {'r','u',0}, 1 },
    { DRAW_GLYPHRUN, {'r',0}, {'r','u',0}, 1 },
    { DRAW_GLYPHRUN, {'i',0}, {'r','u',0}, 1 },
    { DRAW_GLYPHRUN, {'n',0}, {'r','u',0}, 1 },
    { DRAW_GLYPHRUN, {'g',0}, {'r','u',0}, 1 },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq3[] = {
    { DRAW_GLYPHRUN, {0x202a,0x202c,0}, {'r','u',0}, 0 },
    { DRAW_GLYPHRUN, {'a','b',0}, {'r','u',0}, 2 },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq4[] = {
    { DRAW_GLYPHRUN, {'s','t','r',0}, {'r','u',0}, 3 },
    { DRAW_GLYPHRUN, {'i','n','g',0}, {'r','u',0}, 3  },
    { DRAW_STRIKETHROUGH },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_seq5[] = {
    { DRAW_GLYPHRUN, {'s','t',0}, {'r','u',0}, 2 },
    { DRAW_GLYPHRUN, {'r','i',0}, {'r','u',0}, 2 },
    { DRAW_GLYPHRUN, {'n','g',0}, {'r','u',0}, 2 },
    { DRAW_STRIKETHROUGH },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry empty_seq[] = {
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_single_run_seq[] = {
    { DRAW_GLYPHRUN, {'s','t','r','i','n','g',0}, {'r','u',0}, 6 },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draw_ltr_reordered_run_seq[] = {
    { DRAW_GLYPHRUN, {'1','2','3','-','5','2',0}, {'r','u',0}, 6 },
    { DRAW_GLYPHRUN, {0x64a,0x64f,0x633,0x627,0x648,0x650,0x64a,0}, {'r','u',0}, 7, 1 },
    { DRAW_GLYPHRUN, {'7','1',0}, {'r','u',0}, 2, 2 },
    { DRAW_GLYPHRUN, {'.',0}, {'r','u',0}, 1 },
    { DRAW_LAST_KIND }
};

static void test_Draw(void)
{
    static const WCHAR str3W[] = {'1','2','3','-','5','2',0x64a,0x64f,0x633,0x627,0x648,0x650,
        0x64a,'7','1','.',0};
    static const WCHAR str2W[] = {0x202a,0x202c,'a','b',0};
    IDWriteInlineObject *inlineobj;
    struct renderer_context ctxt;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    DWRITE_TEXT_METRICS tm;
    DWRITE_MATRIX m;
    HRESULT hr;

    factory = create_factory();

    memset(&ctxt, 0, sizeof(ctxt));
    ctxt.snapping_disabled = TRUE;

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"ru", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 5;
    range.length = 1;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 4;
    range.length = 1;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, (IUnknown*)inlineobj, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq, "draw test", FALSE);
    IDWriteTextLayout_Release(layout);

    /* with reduced width DrawGlyphRun() is called for every line */
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, 5.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq2, "draw test 2", TRUE);
    hr = IDWriteTextLayout_GetMetrics(layout, &tm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(tm.lineCount == 6, "got %u\n", tm.lineCount);
    IDWriteTextLayout_Release(layout);

    /* string with control characters */
    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 4, format, 500.0, 100.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq3, "draw test 3", FALSE);
    IDWriteTextLayout_Release(layout);

    /* strikethrough splits ranges from renderer point of view, but doesn't break
       shaping */
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, 500.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    flush_sequence(sequences, RENDERER_ID);

    range.startPosition = 0;
    range.length = 3;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq4, "draw test 4", FALSE);
    IDWriteTextLayout_Release(layout);

    /* Strike through somewhere in the middle */
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, 500.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    flush_sequence(sequences, RENDERER_ID);

    range.startPosition = 2;
    range.length = 2;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_seq5, "draw test 5", FALSE);
    IDWriteTextLayout_Release(layout);

    /* empty string */
    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 0, format, 500.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, empty_seq, "draw test 6", FALSE);
    IDWriteTextLayout_Release(layout);

    ctxt.gdicompat = TRUE;
    ctxt.use_gdi_natural = TRUE;

    /* different parameter combinations with gdi-compatible layout */
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, 1.0f, NULL,
            TRUE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 7", FALSE);

    /* text alignment keeps pixel-aligned origin */
    hr = IDWriteTextLayout_GetMetrics(layout, &tm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tm.width == floorf(tm.width), "got %f\n", tm.width);

    hr = IDWriteTextLayout_SetMaxWidth(layout, tm.width + 3.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDWriteTextLayout_SetTextAlignment(layout, DWRITE_TEXT_ALIGNMENT_CENTER);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ctxt.originX = ctxt.originY = 0.0;
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 7", FALSE);
    ok(ctxt.originX != 0.0 && ctxt.originX == floorf(ctxt.originX), "got %f\n", ctxt.originX);

    IDWriteTextLayout_Release(layout);

    ctxt.gdicompat = TRUE;
    ctxt.use_gdi_natural = FALSE;

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 8", FALSE);
    IDWriteTextLayout_Release(layout);

    ctxt.gdicompat = TRUE;
    ctxt.use_gdi_natural = TRUE;

    m.m11 = m.m22 = 2.0;
    m.m12 = m.m21 = m.dx = m.dy = 0.0;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, 1.0f, &m,
            TRUE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 9", FALSE);
    IDWriteTextLayout_Release(layout);

    ctxt.gdicompat = TRUE;
    ctxt.use_gdi_natural = FALSE;

    m.m11 = m.m22 = 2.0;
    m.m12 = m.m21 = m.dx = m.dy = 0.0;
    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, 1.0f, &m,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_single_run_seq, "draw test 10", FALSE);
    IDWriteTextLayout_Release(layout);

    IDWriteInlineObject_Release(inlineobj);

    /* text that triggers bidi run reordering */
    hr = IDWriteFactory_CreateTextLayout(factory, str3W, lstrlenW(str3W), format, 1000.0f, 100.0f, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ctxt.gdicompat = FALSE;
    ctxt.use_gdi_natural = FALSE;
    ctxt.snapping_disabled = TRUE;

    hr = IDWriteTextLayout_SetReadingDirection(layout, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK, "Failed to set reading direction, hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0f, 0.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_ltr_reordered_run_seq, "draw test 11", FALSE);

    IDWriteTextLayout_Release(layout);

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_typography(void)
{
    DWRITE_FONT_FEATURE feature;
    IDWriteTypography *typography;
    IDWriteFactory *factory;
    UINT32 count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTypography(factory, &typography);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    feature.nameTag = DWRITE_FONT_FEATURE_TAG_KERNING;
    feature.parameter = 1;
    hr = IDWriteTypography_AddFontFeature(typography, feature);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IDWriteTypography_GetFontFeatureCount(typography);
    ok(count == 1, "got %u\n", count);

    /* duplicated features work just fine */
    feature.nameTag = DWRITE_FONT_FEATURE_TAG_KERNING;
    feature.parameter = 0;
    hr = IDWriteTypography_AddFontFeature(typography, feature);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IDWriteTypography_GetFontFeatureCount(typography);
    ok(count == 2, "got %u\n", count);

    memset(&feature, 0xcc, sizeof(feature));
    hr = IDWriteTypography_GetFontFeature(typography, 0, &feature);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(feature.nameTag == DWRITE_FONT_FEATURE_TAG_KERNING, "got tag %x\n", feature.nameTag);
    ok(feature.parameter == 1, "got %u\n", feature.parameter);

    memset(&feature, 0xcc, sizeof(feature));
    hr = IDWriteTypography_GetFontFeature(typography, 1, &feature);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(feature.nameTag == DWRITE_FONT_FEATURE_TAG_KERNING, "got tag %x\n", feature.nameTag);
    ok(feature.parameter == 0, "got %u\n", feature.parameter);

    hr = IDWriteTypography_GetFontFeature(typography, 2, &feature);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* duplicated with same parameter value */
    feature.nameTag = DWRITE_FONT_FEATURE_TAG_KERNING;
    feature.parameter = 0;
    hr = IDWriteTypography_AddFontFeature(typography, feature);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IDWriteTypography_GetFontFeatureCount(typography);
    ok(count == 3, "got %u\n", count);

    memset(&feature, 0xcc, sizeof(feature));
    hr = IDWriteTypography_GetFontFeature(typography, 2, &feature);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(feature.nameTag == DWRITE_FONT_FEATURE_TAG_KERNING, "got tag %x\n", feature.nameTag);
    ok(feature.parameter == 0, "got %u\n", feature.parameter);

    IDWriteTypography_Release(typography);
    IDWriteFactory_Release(factory);
}

static void test_GetClusterMetrics(void)
{
    static const WCHAR str_white_spaceW[] = {
        /* BK - FORM FEED, LINE TABULATION, LINE SEP, PARA SEP */ 0xc, 0xb, 0x2028, 0x2029,
        /* ZW - ZERO WIDTH SPACE */ 0x200b,
        /* SP - SPACE  */ 0x20
    };
    static const WCHAR str5W[] = {'a','\r','b','\n','c','\n','\r','d','\r','\n','e',0xb,'f',0xc,
        'g',0x0085,'h',0x2028,'i',0x2029,0xad,0xa,0};
    static const WCHAR str3W[] = {0x2066,')',')',0x661,'(',0x627,')',0};
    static const WCHAR str2W[] = {0x202a,0x202c,'a',0};

    struct test_inline_obj *testinlineobj, *testinlineobj2, *testinlineobj3;
    DWRITE_INLINE_OBJECT_METRICS inline_metrics;
    DWRITE_CLUSTER_METRICS metrics[22];
    DWRITE_TEXT_METRICS text_metrics;
    DWRITE_TRIMMING trimming_options;
    IDWriteTextLayout1 *layout1;
    IDWriteInlineObject *trimm;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_LINE_METRICS line;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    UINT32 count, i;
    FLOAT width;
    HRESULT hr;

    testinlineobj = create_test_inline_object(&testinlineobjvtbl);
    ok(!!testinlineobj, "Failed to create test inline object.\n");
    testinlineobj2 = create_test_inline_object(&testinlineobjvtbl);
    ok(!!testinlineobj2, "Failed to create test inline object.\n");
    testinlineobj3 = create_test_inline_object(&testinlineobjvtbl2);
    ok(!!testinlineobj3, "Failed to create test inline object.\n");

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, str3W, 7, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDWriteTextLayout_GetClusterMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(count == 7, "got %u\n", count);
    IDWriteTextLayout_Release(layout);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(count == 4, "got %u\n", count);

    /* check every cluster width */
    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 4, "got %u\n", count);
    for (i = 0; i < count; i++) {
        ok(metrics[i].width > 0.0, "%u: got width %.2f\n", i, metrics[i].width);
        ok(metrics[i].length == 1, "%u: got length %u\n", i, metrics[i].length);
    }

    /* apply spacing and check widths again */
    if (IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout1, (void**)&layout1) == S_OK) {
        DWRITE_CLUSTER_METRICS metrics2[4];
        FLOAT leading, trailing, min_advance;
        DWRITE_TEXT_RANGE r;

        leading = trailing = min_advance = 2.0;
        hr = IDWriteTextLayout1_GetCharacterSpacing(layout1, 500, &leading, &trailing,
            &min_advance, &r);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(leading == 0.0 && trailing == 0.0 && min_advance == 0.0,
            "got %.2f, %.2f, %.2f\n", leading, trailing, min_advance);
        ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);

        leading = trailing = min_advance = 2.0;
        hr = IDWriteTextLayout1_GetCharacterSpacing(layout1, 0, &leading, &trailing,
            &min_advance, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(leading == 0.0 && trailing == 0.0 && min_advance == 0.0,
            "got %.2f, %.2f, %.2f\n", leading, trailing, min_advance);

        r.startPosition = 0;
        r.length = 4;
        hr = IDWriteTextLayout1_SetCharacterSpacing(layout1, 10.0, 15.0, 0.0, r);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        count = 0;
        hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics2, ARRAY_SIZE(metrics2), &count);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(count == 4, "got %u\n", count);
        for (i = 0; i < count; ++i)
        {
            ok(metrics2[i].width > metrics[i].width, "%u: got width %.2f, was %.2f\n", i, metrics2[i].width,
                metrics[i].width);
            ok(metrics2[i].length == 1, "%u: got length %u\n", i, metrics2[i].length);
        }

        /* back to defaults */
        r.startPosition = 0;
        r.length = 4;
        hr = IDWriteTextLayout1_SetCharacterSpacing(layout1, 0.0, 0.0, 0.0, r);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* negative advance limit */
        r.startPosition = 0;
        r.length = 4;
        hr = IDWriteTextLayout1_SetCharacterSpacing(layout1, 0.0, 0.0, -10.0, r);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        IDWriteTextLayout1_Release(layout1);
    }
    else
        win_skip("IDWriteTextLayout1 is not supported, cluster spacing test skipped.\n");

    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &trimm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, trimm, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* inline object takes a separate cluster, replaced codepoints number doesn't matter */
    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(count == 3, "got %u\n", count);

    count = 0;
    memset(&metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 1, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(count == 3, "got %u\n", count);
    ok(metrics[0].length == 2, "got %u\n", metrics[0].length);

    hr = IDWriteInlineObject_GetMetrics(trimm, &inline_metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(inline_metrics.width > 0.0 && inline_metrics.width == metrics[0].width, "got %.2f, expected %.2f\n",
        inline_metrics.width, metrics[0].width);

    IDWriteTextLayout_Release(layout);

    /* text with non-visual control codes */
    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 3, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* bidi control codes take a separate cluster */
    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 3, "got %u\n", count);

    ok(metrics[0].width == 0.0, "got %.2f\n", metrics[0].width);
    ok(metrics[0].length == 1, "got %d\n", metrics[0].length);
    ok(metrics[0].canWrapLineAfter == 0, "got %d\n", metrics[0].canWrapLineAfter);
    ok(metrics[0].isWhitespace == 0, "got %d\n", metrics[0].isWhitespace);
    ok(metrics[0].isNewline == 0, "got %d\n", metrics[0].isNewline);
    ok(metrics[0].isSoftHyphen == 0, "got %d\n", metrics[0].isSoftHyphen);
    ok(metrics[0].isRightToLeft == 0, "got %d\n", metrics[0].isRightToLeft);

    ok(metrics[1].width == 0.0, "got %.2f\n", metrics[1].width);
    ok(metrics[1].length == 1, "got %d\n", metrics[1].length);
    ok(metrics[1].canWrapLineAfter == 0, "got %d\n", metrics[1].canWrapLineAfter);
    ok(metrics[1].isWhitespace == 0, "got %d\n", metrics[1].isWhitespace);
    ok(metrics[1].isNewline == 0, "got %d\n", metrics[1].isNewline);
    ok(metrics[1].isSoftHyphen == 0, "got %d\n", metrics[1].isSoftHyphen);
    ok(metrics[1].isRightToLeft == 0, "got %d\n", metrics[1].isRightToLeft);

    ok(metrics[2].width > 0.0, "got %.2f\n", metrics[2].width);
    ok(metrics[2].length == 1, "got %d\n", metrics[2].length);
    ok(metrics[2].canWrapLineAfter == 1, "got %d\n", metrics[2].canWrapLineAfter);
    ok(metrics[2].isWhitespace == 0, "got %d\n", metrics[2].isWhitespace);
    ok(metrics[2].isNewline == 0, "got %d\n", metrics[2].isNewline);
    ok(metrics[2].isSoftHyphen == 0, "got %d\n", metrics[2].isSoftHyphen);
    ok(metrics[2].isRightToLeft == 0, "got %d\n", metrics[2].isRightToLeft);

    IDWriteTextLayout_Release(layout);

    /* single inline object that fails to report its metrics */
    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 4;
    hr = IDWriteTextLayout_SetInlineObject(layout, &testinlineobj->IDWriteInlineObject_iface, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);

    /* object sets a width to 123.0, but returns failure from GetMetrics() */
    ok(metrics[0].width == 0.0, "got %.2f\n", metrics[0].width);
    ok(metrics[0].length == 4, "got %d\n", metrics[0].length);
    ok(metrics[0].canWrapLineAfter == 1, "got %d\n", metrics[0].canWrapLineAfter);
    ok(metrics[0].isWhitespace == 0, "got %d\n", metrics[0].isWhitespace);
    ok(metrics[0].isNewline == 0, "got %d\n", metrics[0].isNewline);
    ok(metrics[0].isSoftHyphen == 0, "got %d\n", metrics[0].isSoftHyphen);
    ok(metrics[0].isRightToLeft == 0, "got %d\n", metrics[0].isRightToLeft);

    /* now set two inline object for [0,1] and [2,3], both fail to report break conditions */
    range.startPosition = 2;
    range.length = 2;
    hr = IDWriteTextLayout_SetInlineObject(layout, &testinlineobj2->IDWriteInlineObject_iface, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);

    ok(metrics[0].width == 0.0, "got %.2f\n", metrics[0].width);
    ok(metrics[0].length == 2, "got %d\n", metrics[0].length);
    ok(metrics[0].canWrapLineAfter == 0, "got %d\n", metrics[0].canWrapLineAfter);
    ok(metrics[0].isWhitespace == 0, "got %d\n", metrics[0].isWhitespace);
    ok(metrics[0].isNewline == 0, "got %d\n", metrics[0].isNewline);
    ok(metrics[0].isSoftHyphen == 0, "got %d\n", metrics[0].isSoftHyphen);
    ok(metrics[0].isRightToLeft == 0, "got %d\n", metrics[0].isRightToLeft);

    ok(metrics[1].width == 0.0, "got %.2f\n", metrics[1].width);
    ok(metrics[1].length == 2, "got %d\n", metrics[1].length);
    ok(metrics[1].canWrapLineAfter == 1, "got %d\n", metrics[1].canWrapLineAfter);
    ok(metrics[1].isWhitespace == 0, "got %d\n", metrics[1].isWhitespace);
    ok(metrics[1].isNewline == 0, "got %d\n", metrics[1].isNewline);
    ok(metrics[1].isSoftHyphen == 0, "got %d\n", metrics[1].isSoftHyphen);
    ok(metrics[1].isRightToLeft == 0, "got %d\n", metrics[1].isRightToLeft);

    IDWriteTextLayout_Release(layout);

    /* zero length string */
    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 0, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 1;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 0, "got %u\n", count);
    IDWriteTextLayout_Release(layout);

    /* Whitespace */
    hr = IDWriteFactory_CreateTextLayout(factory, L"a ", 2, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 2, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);
    ok(metrics[0].isWhitespace == 0, "got %d\n", metrics[0].isWhitespace);
    ok(metrics[0].canWrapLineAfter == 0, "got %d\n", metrics[0].canWrapLineAfter);
    ok(metrics[1].isWhitespace == 1, "got %d\n", metrics[1].isWhitespace);
    ok(metrics[1].canWrapLineAfter == 1, "got %d\n", metrics[1].canWrapLineAfter);
    IDWriteTextLayout_Release(layout);

    /* Layout is fully covered by inline object with after condition DWRITE_BREAK_CONDITION_MAY_NOT_BREAK. */
    hr = IDWriteFactory_CreateTextLayout(factory, L"a ", 2, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    range.startPosition = 0;
    hr = IDWriteTextLayout_SetInlineObject(layout, &testinlineobj3->IDWriteInlineObject_iface, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 2, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);
    ok(metrics[0].canWrapLineAfter == 1, "got %d\n", metrics[0].canWrapLineAfter);

    IDWriteTextLayout_Release(layout);

    /* compare natural cluster width with gdi layout */
    hr = IDWriteFactory_CreateTextLayout(factory, L"a ", 1, format, 100.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);
    ok(metrics[0].width != floorf(metrics[0].width), "got %f\n", metrics[0].width);

    IDWriteTextLayout_Release(layout);

    hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"a ", 1, format, 100.0f, 100.0f, 1.0f, NULL,
            FALSE, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);
    ok(metrics[0].width == floorf(metrics[0].width), "got %f\n", metrics[0].width);

    IDWriteTextLayout_Release(layout);

    /* isNewline tests */
    hr = IDWriteFactory_CreateTextLayout(factory, str5W, lstrlenW(str5W), format, 100.0f, 200.0f, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 22, "got %u\n", count);

    ok(metrics[1].isNewline == 1, "got %d\n", metrics[1].isNewline);
    ok(metrics[3].isNewline == 1, "got %d\n", metrics[3].isNewline);
    ok(metrics[5].isNewline == 1, "got %d\n", metrics[5].isNewline);
    ok(metrics[6].isNewline == 1, "got %d\n", metrics[6].isNewline);
    ok(metrics[9].isNewline == 1, "got %d\n", metrics[9].isNewline);
    ok(metrics[11].isNewline == 1, "got %d\n", metrics[11].isNewline);
    ok(metrics[13].isNewline == 1, "got %d\n", metrics[13].isNewline);
    ok(metrics[15].isNewline == 1, "got %d\n", metrics[15].isNewline);
    ok(metrics[17].isNewline == 1, "got %d\n", metrics[17].isNewline);
    ok(metrics[19].isNewline == 1, "got %d\n", metrics[19].isNewline);
    ok(metrics[21].isNewline == 1, "got %d\n", metrics[21].isNewline);

    ok(metrics[0].isNewline == 0, "got %d\n", metrics[0].isNewline);
    ok(metrics[2].isNewline == 0, "got %d\n", metrics[2].isNewline);
    ok(metrics[4].isNewline == 0, "got %d\n", metrics[4].isNewline);
    ok(metrics[7].isNewline == 0, "got %d\n", metrics[7].isNewline);
    ok(metrics[8].isNewline == 0, "got %d\n", metrics[8].isNewline);
    ok(metrics[10].isNewline == 0, "got %d\n", metrics[10].isNewline);
    ok(metrics[12].isNewline == 0, "got %d\n", metrics[12].isNewline);
    ok(metrics[14].isNewline == 0, "got %d\n", metrics[14].isNewline);
    ok(metrics[16].isNewline == 0, "got %d\n", metrics[16].isNewline);
    ok(metrics[18].isNewline == 0, "got %d\n", metrics[18].isNewline);
    ok(metrics[20].isNewline == 0, "got %d\n", metrics[20].isNewline);

    for (i = 0; i < count; i++) {
        ok(metrics[i].length == 1, "%d: got %d\n", i, metrics[i].length);
        ok(metrics[i].isSoftHyphen == (i == count - 2), "%d: got %d\n", i, metrics[i].isSoftHyphen);
        if (metrics[i].isSoftHyphen)
            ok(!metrics[i].isWhitespace, "%u: got %d\n", i, metrics[i].isWhitespace);
        if (metrics[i].isNewline) {
            ok(metrics[i].width == 0.0f, "%u: got width %f\n", i, metrics[i].width);
            ok(metrics[i].isWhitespace == 1, "%u: got %d\n", i, metrics[i].isWhitespace);
            ok(metrics[i].canWrapLineAfter == 1, "%u: got %d\n", i, metrics[i].canWrapLineAfter);
        }
    }

    IDWriteTextLayout_Release(layout);

    /* Test whitespace resolution from linebreaking classes BK, ZW, and SP */
    hr = IDWriteFactory_CreateTextLayout(factory, str_white_spaceW, ARRAY_SIZE(str_white_spaceW), format,
        100.0f, 200.0f, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 20, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 6, "got %u\n", count);

    ok(metrics[0].isWhitespace == 1, "got %d\n", metrics[0].isWhitespace);
    ok(metrics[1].isWhitespace == 1, "got %d\n", metrics[1].isWhitespace);
    ok(metrics[2].isWhitespace == 1, "got %d\n", metrics[2].isWhitespace);
    ok(metrics[3].isWhitespace == 1, "got %d\n", metrics[3].isWhitespace);
    ok(metrics[4].isWhitespace == 0, "got %d\n", metrics[4].isWhitespace);
    ok(metrics[5].isWhitespace == 1, "got %d\n", metrics[5].isWhitespace);

    IDWriteTextLayout_Release(layout);

    /* Trigger line trimming. */
    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 100.0f, 200.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 4, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 4, "got %u\n", count);

    hr = IDWriteTextLayout_GetMetrics(layout, &text_metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    width = metrics[0].width + inline_metrics.width;
    ok(width < text_metrics.width, "unexpected trimming sign width\n");

    /* enable trimming, reduce layout width so only first cluster and trimming sign fits */
    trimming_options.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
    trimming_options.delimiter = 0;
    trimming_options.delimiterCount = 0;
    hr = IDWriteTextLayout_SetTrimming(layout, &trimming_options, trimm);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetMaxWidth(layout, width);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 4, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 4, "got %u\n", count);

    hr = IDWriteTextLayout_GetLineMetrics(layout, &line, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);
    ok(line.length == 4, "got %u\n", line.length);
    ok(line.isTrimmed, "got %d\n", line.isTrimmed);

    IDWriteTextLayout_Release(layout);

    /* NO_WRAP, check cluster wrapping attribute. */
    hr = IDWriteTextFormat_SetWordWrapping(format, DWRITE_WORD_WRAPPING_NO_WRAP);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a b", 3, format, 1000.0f, 200.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 3, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 3, "got %u\n", count);

    ok(metrics[0].canWrapLineAfter == 0, "got %d\n", metrics[0].canWrapLineAfter);
    ok(metrics[1].canWrapLineAfter == 1, "got %d\n", metrics[1].canWrapLineAfter);
    ok(metrics[2].canWrapLineAfter == 1, "got %d\n", metrics[2].canWrapLineAfter);

    IDWriteTextLayout_Release(layout);

    /* Single cluster layout, trigger trimming. */
    hr = IDWriteFactory_CreateTextLayout(factory, L"a b", 1, format, 1000.0f, 200.0f, &layout);
    ok(hr == S_OK, "Failed to create layout, hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 1, &count);
    ok(hr == S_OK, "Failed to get cluster metrics, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected cluster count %u.\n", count);

    hr = IDWriteTextLayout_SetMaxWidth(layout, metrics[0].width / 2.0f);
    ok(hr == S_OK, "Failed to set layout width, hr %#lx.\n", hr);

    trimming_options.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
    trimming_options.delimiter = 0;
    trimming_options.delimiterCount = 0;
    hr = IDWriteTextLayout_SetTrimming(layout, &trimming_options, trimm);
    ok(hr == S_OK, "Failed to set trimming options, hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, 1, &count);
    ok(hr == S_OK, "Failed to get cluster metrics, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected cluster count %u.\n", count);

    hr = IDWriteTextLayout_GetLineMetrics(layout, &line, 1, &count);
    ok(hr == S_OK, "Failed to get line metrics, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected line count %u.\n", count);
    ok(line.length == 1, "Unexpected line length %u.\n", line.length);
    ok(line.isTrimmed, "Unexpected trimming flag %x.\n", line.isTrimmed);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0f, 0.0f);
    ok(hr == S_OK, "Draw() failed, hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draw_trimmed_seq, "Trimmed draw test", FALSE);

    IDWriteTextLayout_Release(layout);

    IDWriteInlineObject_Release(&testinlineobj->IDWriteInlineObject_iface);
    IDWriteInlineObject_Release(&testinlineobj2->IDWriteInlineObject_iface);
    IDWriteInlineObject_Release(&testinlineobj3->IDWriteInlineObject_iface);
    IDWriteInlineObject_Release(trimm);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetLocaleName(void)
{
    WCHAR buffW[LOCALE_NAME_MAX_LENGTH + 5];
    IDWriteTextFormat *format, *format2;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    /* create format with mixed case locale name, get it back */
    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"eN-uS", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteTextFormat_GetLocaleName(format, buffW, ARRAY_SIZE(buffW));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"en-us"), "Unexpected locale name %s.\n", wine_dbgstr_w(buffW));

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat, (void**)&format2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_GetLocaleName(format2, buffW, ARRAY_SIZE(buffW));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"en-us"), "Unexpected locale name %s.\n", wine_dbgstr_w(buffW));

    hr = IDWriteTextLayout_GetLocaleName(layout, 0, buffW, ARRAY_SIZE(buffW), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"en-us"), "Unexpected locale name %s.\n", wine_dbgstr_w(buffW));

    IDWriteTextFormat_Release(format2);
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetLocaleName(layout, L"en-us", range);
    ok(hr == S_OK, "Failed to set locale name, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetLocaleName(layout, NULL, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* invalid locale name is allowed */
    hr = IDWriteTextLayout_SetLocaleName(layout, L"abcd", range);
    ok(hr == S_OK, "Failed to set locale name, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_GetLocaleName(layout, 0, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    if (0) /* crashes on native */
        hr = IDWriteTextLayout_GetLocaleName(layout, 0, NULL, 1, NULL);

    buffW[0] = 0;
    range.length = 0;
    hr = IDWriteTextLayout_GetLocaleName(layout, 0, buffW, ARRAY_SIZE(buffW), &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"abcd"), "Unexpected locale name %s.\n", wine_dbgstr_w(buffW));
    ok(range.startPosition == 0 && range.length == 1, "got %u,%u\n", range.startPosition, range.length);

    /* get with a shorter buffer */
    buffW[0] = 0xa;
    hr = IDWriteTextLayout_GetLocaleName(layout, 0, buffW, 1, NULL);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0, "got %x\n", buffW[0]);

    /* name is too long */
    lstrcpyW(buffW, L"abcd");
    while (lstrlenW(buffW) <= LOCALE_NAME_MAX_LENGTH)
        lstrcatW(buffW, L"abcd");

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetLocaleName(layout, buffW, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    buffW[0] = 0;
    hr = IDWriteTextLayout_GetLocaleName(layout, 0, buffW, ARRAY_SIZE(buffW), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"abcd"), "Unexpected locale name %s.\n", wine_dbgstr_w(buffW));

    /* set initial locale name for whole text, except with a different casing */
    range.startPosition = 0;
    range.length = 4;
    hr = IDWriteTextLayout_SetLocaleName(layout, L"eN-uS", range);
    ok(hr == S_OK, "Failed to set locale name, hr %#lx.\n", hr);

    buffW[0] = 0;
    range.length = 0;
    hr = IDWriteTextLayout_GetLocaleName(layout, 0, buffW, ARRAY_SIZE(buffW), &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"en-us"), "Unexpected locale name %s.\n", wine_dbgstr_w(buffW));
    ok((range.startPosition == 0 && range.length == ~0u) ||
        broken(range.startPosition == 0 && range.length == 4) /* vista/win7 */, "got %u,%u\n", range.startPosition, range.length);

    /* check what's returned for positions after the text */
    buffW[0] = 0;
    range.length = 0;
    hr = IDWriteTextLayout_GetLocaleName(layout, 100, buffW, ARRAY_SIZE(buffW), &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"en-us"), "Unexpected locale name %s.\n", wine_dbgstr_w(buffW));
    ok((range.startPosition == 0 && range.length == ~0u) ||
        broken(range.startPosition == 4 && range.length == ~0u-4) /* vista/win7 */, "got %u,%u\n", range.startPosition, range.length);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetPairKerning(void)
{
    static const WCHAR strW[] = {'a','e',0x0300,'d',0}; /* accent grave */
    DWRITE_CLUSTER_METRICS clusters[4];
    IDWriteTextLayout1 *layout1;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    BOOL kerning;
    UINT32 count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDWriteTextFormat_Release(format);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout1, (void**)&layout1);
    IDWriteTextLayout_Release(layout);

    if (hr != S_OK) {
        win_skip("SetPairKerning() is not supported.\n");
        IDWriteFactory_Release(factory);
        return;
    }

if (0) { /* crashes on native */
    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, NULL, NULL);
    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, NULL, &range);
}

    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, &kerning, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 0;
    kerning = TRUE;
    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, &kerning, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!kerning, "got %d\n", kerning);
    ok(range.length == ~0u, "got %u\n", range.length);

    count = 0;
    hr = IDWriteTextLayout1_GetClusterMetrics(layout1, clusters, 4, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(count == 3, "Unexpected cluster count %u.\n", count);
    ok(clusters[0].length == 1, "got %u\n", clusters[0].length);
    todo_wine ok(clusters[1].length == 2, "got %u\n", clusters[1].length);
    ok(clusters[2].length == 1, "got %u\n", clusters[2].length);

    /* pair kerning flag participates in itemization - combining characters
       breaks */
    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout1_SetPairKerning(layout1, 2, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    kerning = FALSE;
    hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, &kerning, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(kerning == TRUE, "got %d\n", kerning);

    count = 0;
    hr = IDWriteTextLayout1_GetClusterMetrics(layout1, clusters, 4, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 4, "got %u\n", count);
    ok(clusters[0].length == 1, "got %u\n", clusters[0].length);
    ok(clusters[1].length == 1, "got %u\n", clusters[1].length);
    ok(clusters[2].length == 1, "got %u\n", clusters[2].length);
    ok(clusters[3].length == 1, "got %u\n", clusters[3].length);

    IDWriteTextLayout1_Release(layout1);
    IDWriteFactory_Release(factory);
}

static void test_SetVerticalGlyphOrientation(void)
{
    DWRITE_VERTICAL_GLYPH_ORIENTATION orientation;
    IDWriteTextLayout2 *layout2;
    IDWriteTextFormat1 *format1;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    IDWriteTextFormat_Release(format);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout2, (void**)&layout2);
    IDWriteTextLayout_Release(layout);

    if (hr != S_OK) {
        win_skip("SetVerticalGlyphOrientation() is not supported.\n");
        IDWriteFactory_Release(factory);
        return;
    }

    orientation = IDWriteTextLayout2_GetVerticalGlyphOrientation(layout2);
    ok(orientation == DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT, "got %d\n", orientation);

    hr = IDWriteTextLayout2_SetVerticalGlyphOrientation(layout2, DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED+1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextFormat1, (void **)&format1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    orientation = IDWriteTextFormat1_GetVerticalGlyphOrientation(format1);
    ok(orientation == DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT, "Unexpected orientation %d.\n", orientation);

    hr = IDWriteTextLayout2_SetVerticalGlyphOrientation(layout2, DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    orientation = IDWriteTextLayout2_GetVerticalGlyphOrientation(layout2);
    ok(orientation == DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED, "Unexpected orientation %d.\n", orientation);

    orientation = IDWriteTextFormat1_GetVerticalGlyphOrientation(format1);
    ok(orientation == DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED, "Unexpected orientation %d.\n", orientation);

    hr = IDWriteTextFormat1_SetVerticalGlyphOrientation(format1, DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    orientation = IDWriteTextLayout2_GetVerticalGlyphOrientation(layout2);
    ok(orientation == DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT, "Unexpected orientation %d.\n", orientation);

    IDWriteTextFormat1_Release(format1);

    IDWriteTextLayout2_Release(layout2);
    IDWriteFactory_Release(factory);
}

static void test_DetermineMinWidth(void)
{
    struct minwidth_test {
        const WCHAR text[10];    /* text to create a layout for */
        const WCHAR mintext[10]; /* text that represents sequence of minimal width */
    } minwidth_tests[] = {
        { {' ','a','b',' ',0}, {'a','b',0} },
        { {'a','\n',' ',' ',0}, {'a',0} },
        { {'a','\n',' ',' ','b',0}, {'b',0} },
        { {'a','b','c','\n',' ',' ','b',0}, {'a','b','c',0} },
    };
    DWRITE_CLUSTER_METRICS metrics[10];
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    UINT32 count, i, j;
    FLOAT minwidth;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_DetermineMinWidth(layout, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    IDWriteTextLayout_Release(layout);

    /* empty string */
    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 0, format, 100.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    minwidth = 1.0f;
    hr = IDWriteTextLayout_DetermineMinWidth(layout, &minwidth);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(minwidth == 0.0f, "got %f\n", minwidth);
    IDWriteTextLayout_Release(layout);

    for (i = 0; i < ARRAY_SIZE(minwidth_tests); i++) {
        FLOAT width = 0.0f;

        /* measure expected width */
        hr = IDWriteFactory_CreateTextLayout(factory, minwidth_tests[i].mintext, lstrlenW(minwidth_tests[i].mintext), format, 1000.0f, 1000.0f, &layout);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextLayout_GetClusterMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        for (j = 0; j < count; j++)
            width += metrics[j].width;

        IDWriteTextLayout_Release(layout);

        hr = IDWriteFactory_CreateTextLayout(factory, minwidth_tests[i].text, lstrlenW(minwidth_tests[i].text), format, 1000.0f, 1000.0f, &layout);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        minwidth = 0.0f;
        hr = IDWriteTextLayout_DetermineMinWidth(layout, &minwidth);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(minwidth == width, "test %u: expected width %f, got %f\n", i, width, minwidth);

        IDWriteTextLayout_Release(layout);
    }

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFontSize(void)
{
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    FLOAT size;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    /* negative/zero size */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontSize(layout, -15.0, r);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetFontSize(layout, 0.0, r);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    r.startPosition = 1;
    r.length = 0;
    size = 0.0;
    hr = IDWriteTextLayout_GetFontSize(layout, 0, &size, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);
    ok(size == 10.0, "got %.2f\n", size);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontSize(layout, 15.0, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* zero length range */
    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_SetFontSize(layout, 123.0, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    size = 0.0;
    hr = IDWriteTextLayout_GetFontSize(layout, 1, &size, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == 15.0, "got %.2f\n", size);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontSize(layout, 15.0, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    size = 0.0;
    hr = IDWriteTextLayout_GetFontSize(layout, 1, &size, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == 15.0, "got %.2f\n", size);

    size = 0.0;
    hr = IDWriteTextLayout_GetFontSize(layout, 0, &size, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(size == 15.0, "got %.2f\n", size);

    size = 15.0;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontSize(layout, 20, &size, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 4 && r.length == ~0u-4, "got %u, %u\n", r.startPosition, r.length);
    ok(size == 10.0, "got %.2f\n", size);

    r.startPosition = 100;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontSize(layout, 25.0, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    size = 15.0;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontSize(layout, 100, &size, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 100 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(size == 25.0, "got %.2f\n", size);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFontFamilyName(void)
{
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    WCHAR nameW[50];
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    /* NULL name */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, NULL, r);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    r.startPosition = 1;
    r.length = 0;
    nameW[0] = 0;
    hr = IDWriteTextLayout_GetFontFamilyName(layout, 1, nameW, ARRAY_SIZE(nameW), &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);

    /* set name only different in casing */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, L"TaHoma", r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* zero length range */
    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, L"Arial", r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    r.startPosition = 0;
    r.length = 0;
    nameW[0] = 0;
    hr = IDWriteTextLayout_GetFontFamilyName(layout, 1, nameW, ARRAY_SIZE(nameW), &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(nameW, L"TaHoma"), "Unexpected family name %s.\n", wine_dbgstr_w(nameW));
    ok(r.startPosition == 1 && r.length == 1, "got %u, %u\n", r.startPosition, r.length);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, L"Arial", r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_GetFontFamilyName(layout, 1, nameW, ARRAY_SIZE(nameW), &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 1 && r.length == 1, "got %u, %u\n", r.startPosition, r.length);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, L"Arial", r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    nameW[0] = 0;
    hr = IDWriteTextLayout_GetFontFamilyName(layout, 1, nameW, ARRAY_SIZE(nameW), &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(!lstrcmpW(nameW, L"Arial"), "Unexpected family name %s.\n", wine_dbgstr_w(nameW));

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFontStyle(void)
{
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_FONT_STYLE style;
    DWRITE_TEXT_RANGE r;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    /* invalid style value */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_ITALIC+1, r);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_GetFontStyle(layout, 0, &style, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);
    ok(style == DWRITE_FONT_STYLE_NORMAL, "got %d\n", style);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_ITALIC, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* zero length range */
    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_NORMAL, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    style = DWRITE_FONT_STYLE_NORMAL;
    hr = IDWriteTextLayout_GetFontStyle(layout, 1, &style, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(style == DWRITE_FONT_STYLE_ITALIC, "got %d\n", style);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_OBLIQUE, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    style = DWRITE_FONT_STYLE_ITALIC;
    hr = IDWriteTextLayout_GetFontStyle(layout, 1, &style, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(style == DWRITE_FONT_STYLE_OBLIQUE, "got %d\n", style);

    style = DWRITE_FONT_STYLE_ITALIC;
    hr = IDWriteTextLayout_GetFontStyle(layout, 0, &style, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(style == DWRITE_FONT_STYLE_OBLIQUE, "got %d\n", style);

    style = DWRITE_FONT_STYLE_ITALIC;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontStyle(layout, 20, &style, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 4 && r.length == ~0u-4, "got %u, %u\n", r.startPosition, r.length);
    ok(style == DWRITE_FONT_STYLE_NORMAL, "got %d\n", style);

    r.startPosition = 100;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_OBLIQUE, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    style = DWRITE_FONT_STYLE_NORMAL;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontStyle(layout, 100, &style, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 100 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(style == DWRITE_FONT_STYLE_OBLIQUE, "got %d\n", style);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFontStretch(void)
{
    DWRITE_FONT_STRETCH stretch;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    /* invalid stretch value */
    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_ULTRA_EXPANDED+1, r);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    r.startPosition = 1;
    r.length = 0;
    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    hr = IDWriteTextLayout_GetFontStretch(layout, 0, &stretch, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);
    ok(stretch == DWRITE_FONT_STRETCH_NORMAL, "got %d\n", stretch);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_CONDENSED, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* zero length range */
    r.startPosition = 1;
    r.length = 0;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_NORMAL, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    hr = IDWriteTextLayout_GetFontStretch(layout, 1, &stretch, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(stretch == DWRITE_FONT_STRETCH_CONDENSED, "got %d\n", stretch);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_EXPANDED, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    hr = IDWriteTextLayout_GetFontStretch(layout, 1, &stretch, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(stretch == DWRITE_FONT_STRETCH_EXPANDED, "got %d\n", stretch);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    hr = IDWriteTextLayout_GetFontStretch(layout, 0, &stretch, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(stretch == DWRITE_FONT_STRETCH_EXPANDED, "got %d\n", stretch);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontStretch(layout, 20, &stretch, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 4 && r.length == ~0u-4, "got %u, %u\n", r.startPosition, r.length);
    ok(stretch == DWRITE_FONT_STRETCH_NORMAL, "got %d\n", stretch);

    r.startPosition = 100;
    r.length = 4;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_EXPANDED, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stretch = DWRITE_FONT_STRETCH_UNDEFINED;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetFontStretch(layout, 100, &stretch, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 100 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(stretch == DWRITE_FONT_STRETCH_EXPANDED, "got %d\n", stretch);

    /* trying to set undefined value */
    r.startPosition = 0;
    r.length = 2;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_UNDEFINED, r);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetStrikethrough(void)
{
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE r;
    BOOL value;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    r.startPosition = 1;
    r.length = 0;
    value = TRUE;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 0, &value, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 0 && r.length == ~0u, "got %u, %u\n", r.startPosition, r.length);
    ok(value == FALSE, "got %d\n", value);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    value = FALSE;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 1, &value, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == TRUE, "got %d\n", value);
    ok(r.startPosition == 1 && r.length == 1, "got %u, %u\n", r.startPosition, r.length);

    value = TRUE;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 20, &value, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 2 && r.length == ~0u-2, "got %u, %u\n", r.startPosition, r.length);
    ok(value == FALSE, "got %d\n", value);

    r.startPosition = 100;
    r.length = 4;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    value = FALSE;
    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 100, &value, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 100 && r.length == 4, "got %u, %u\n", r.startPosition, r.length);
    ok(value == TRUE, "got %d\n", value);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_GetMetrics(void)
{
    static const WCHAR str2W[] = {0x2066,')',')',0x661,'(',0x627,')',0};
    DWRITE_CLUSTER_METRICS clusters[4];
    DWRITE_TEXT_METRICS metrics;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    UINT32 count, i;
    FLOAT width;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 500.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, 4, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 4, "got %u\n", count);
    for (i = 0, width = 0.0; i < count; i++)
        width += clusters[i].width;

    memset(&metrics, 0xcc, sizeof(metrics));
    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(metrics.left == 0.0, "got %.2f\n", metrics.left);
    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.width == width, "got %.2f, expected %.2f\n", metrics.width, width);
    ok(metrics.widthIncludingTrailingWhitespace == width, "got %.2f, expected %.2f\n",
        metrics.widthIncludingTrailingWhitespace, width);
    ok(metrics.height > 0.0, "got %.2f\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "got %.2f\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 1000.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.maxBidiReorderingDepth == 1, "got %u\n", metrics.maxBidiReorderingDepth);
    ok(metrics.lineCount == 1, "got %u\n", metrics.lineCount);

    IDWriteTextLayout_Release(layout);

    /* a string with more complex bidi sequence */
    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 7, format, 500.0, 1000.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&metrics, 0xcc, sizeof(metrics));
    metrics.maxBidiReorderingDepth = 0;
    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(metrics.left == 0.0, "got %.2f\n", metrics.left);
    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.width > 0.0, "got %.2f\n", metrics.width);
    ok(metrics.widthIncludingTrailingWhitespace > 0.0, "got %.2f\n", metrics.widthIncludingTrailingWhitespace);
    ok(metrics.height > 0.0, "got %.2f\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "got %.2f\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 1000.0, "got %.2f\n", metrics.layoutHeight);
    todo_wine
    ok(metrics.maxBidiReorderingDepth > 1, "got %u\n", metrics.maxBidiReorderingDepth);
    ok(metrics.lineCount == 1, "got %u\n", metrics.lineCount);

    IDWriteTextLayout_Release(layout);

    /* single cluster layout */
    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 500.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);

    memset(&metrics, 0xcc, sizeof(metrics));
    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(metrics.left == 0.0, "got %.2f\n", metrics.left);
    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.width == clusters[0].width, "got %.2f, expected %.2f\n", metrics.width, clusters[0].width);
    ok(metrics.widthIncludingTrailingWhitespace == clusters[0].width, "got %.2f\n", metrics.widthIncludingTrailingWhitespace);
    ok(metrics.height > 0.0, "got %.2f\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "got %.2f\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 1000.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.maxBidiReorderingDepth == 1, "got %u\n", metrics.maxBidiReorderingDepth);
    ok(metrics.lineCount == 1, "got %u\n", metrics.lineCount);
    IDWriteTextLayout_Release(layout);

    /* Whitespace only. */
    hr = IDWriteFactory_CreateTextLayout(factory, L" ", 1, format, 500.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    memset(&metrics, 0xcc, sizeof(metrics));
    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Failed to get layout metrics, hr %#lx.\n", hr);
    ok(metrics.left == 0.0f, "Unexpected value for left %f.\n", metrics.left);
    ok(metrics.top == 0.0f, "Unexpected value for top %f.\n", metrics.top);
    ok(metrics.width == 0.0f, "Unexpected width %f.\n", metrics.width);
    ok(metrics.widthIncludingTrailingWhitespace > 0.0f, "Unexpected full width %f.\n",
            metrics.widthIncludingTrailingWhitespace);
    ok(metrics.height > 0.0, "Unexpected height %f.\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "Unexpected box width %f.\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 1000.0, "Unexpected box height %f.\n", metrics.layoutHeight);
    ok(metrics.maxBidiReorderingDepth == 1, "Unexpected reordering depth %u.\n", metrics.maxBidiReorderingDepth);
    ok(metrics.lineCount == 1, "Unexpected line count %u.\n", metrics.lineCount);
    IDWriteTextLayout_Release(layout);

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetFlowDirection(void)
{
    DWRITE_READING_DIRECTION reading;
    DWRITE_FLOW_DIRECTION flow;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    flow = IDWriteTextFormat_GetFlowDirection(format);
    ok(flow == DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM, "got %d\n", flow);

    reading = IDWriteTextFormat_GetReadingDirection(format);
    ok(reading == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT, "got %d\n", reading);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 500.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    IDWriteTextLayout_Release(layout);

    hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* vista,win7 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 500.0f, 1000.0f, &layout);
        ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
        IDWriteTextLayout_Release(layout);

        hr = IDWriteTextFormat_SetReadingDirection(format, DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextFormat_SetFlowDirection(format, DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 500.0f, 1000.0f, &layout);
        ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
        IDWriteTextLayout_Release(layout);
    }
    else
        win_skip("DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT is not supported\n");

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static const struct drawcall_entry draweffect_seq[] = {
    { DRAW_GLYPHRUN|DRAW_EFFECT, {'a','e',0x0300,0}, {'e','n','-','u','s',0}, 2 },
    { DRAW_GLYPHRUN, {'d',0}, {'e','n','-','u','s',0}, 1 },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draweffect2_seq[] = {
    { DRAW_GLYPHRUN|DRAW_EFFECT, {'a','e',0}, {'e','n','-','u','s',0}, 2 },
    { DRAW_GLYPHRUN, {'c','d',0}, {'e','n','-','u','s',0}, 2 },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draweffect3_seq[] = {
    { DRAW_INLINE|DRAW_EFFECT },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry draweffect4_seq[] = {
    { DRAW_INLINE },
    { DRAW_LAST_KIND }
};

static void test_SetDrawingEffect(void)
{
    static const WCHAR strW[] = {'a','e',0x0300,'d',0}; /* accent grave */
    IDWriteInlineObject *sign;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    IUnknown *unk, *effect;
    DWRITE_TEXT_RANGE r;
    HRESULT hr;
    LONG ref;

    factory = create_factory();

    effect = create_test_effect();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    /* string with combining mark */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 500.0, 1000.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* set effect past the end of text */
    r.startPosition = 100;
    r.length = 10;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, effect, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    r.startPosition = r.length = 0;
    hr = IDWriteTextLayout_GetDrawingEffect(layout, 101, &unk, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 100 && r.length == 10, "got %u, %u\n", r.startPosition, r.length);
    IUnknown_Release(unk);

    r.startPosition = r.length = 0;
    unk = (void*)0xdeadbeef;
    hr = IDWriteTextLayout_GetDrawingEffect(layout, 1000, &unk, &r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(r.startPosition == 110 && r.length == ~0u-110, "got %u, %u\n", r.startPosition, r.length);
    ok(unk == NULL, "got %p\n", unk);

    /* effect is applied to clusters, not individual text positions */
    r.startPosition = 0;
    r.length = 2;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, effect, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect_seq, "effect draw test", TRUE);
    IDWriteTextLayout_Release(layout);

    /* simple string */
    hr = IDWriteFactory_CreateTextLayout(factory, L"aecd", 4, format, 500.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    r.startPosition = 0;
    r.length = 2;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, effect, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect2_seq, "effect draw test 2", FALSE);
    IDWriteTextLayout_Release(layout);

    /* Inline object - effect set for same range */
    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"aecd", 4, format, 500.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetInlineObject(layout, sign, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetDrawingEffect(layout, effect, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect3_seq, "effect draw test 3", FALSE);

    /* now set effect somewhere inside a range replaced by inline object */
    hr = IDWriteTextLayout_SetDrawingEffect(layout, NULL, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    r.startPosition = 1;
    r.length = 1;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, effect, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* no effect is reported in this case */
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect4_seq, "effect draw test 4", FALSE);

    r.startPosition = 0;
    r.length = 4;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, NULL, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    r.startPosition = 0;
    r.length = 1;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, effect, r);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* first range position is all that matters for inline ranges */
    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, draweffect3_seq, "effect draw test 5", FALSE);

    IDWriteTextLayout_Release(layout);

    ref = IUnknown_Release(effect);
    ok(ref == 0, "Unexpected effect refcount %lu\n", ref);
    IDWriteInlineObject_Release(sign);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static BOOL get_enus_string(IDWriteLocalizedStrings *strings, WCHAR *buff, UINT32 size)
{
    UINT32 index;
    BOOL exists = FALSE;
    HRESULT hr;

    hr = IDWriteLocalizedStrings_FindLocaleName(strings, L"en-us", &index, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    if (exists) {
        hr = IDWriteLocalizedStrings_GetString(strings, index, buff, size);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    else
        *buff = 0;

    return exists;
}

static void test_GetLineMetrics(void)
{
    static const WCHAR str3W[] = {'a','\r','b','\n','c','\n','\r','d','\r','\n',0};
    static const WCHAR strW[] = {'a','b','c','d',' ',0};
    static const WCHAR str2W[] = {'a','b','\r','c','d',0};
    static const WCHAR str4W[] = {'a','\r',0};
    IDWriteFontCollection *syscollection;
    DWRITE_FONT_METRICS fontmetrics;
    DWRITE_LINE_METRICS metrics[6];
    UINT32 count, i, familycount;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE range;
    WCHAR nameW[256];
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 2048.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 5, format, 30000.0, 1000.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got count %u\n", count);

    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(metrics[0].length == 5, "got %u\n", metrics[0].length);
    ok(metrics[0].trailingWhitespaceLength == 1, "got %u\n", metrics[0].trailingWhitespaceLength);

    ok(metrics[0].newlineLength == 0, "got %u\n", metrics[0].newlineLength);
    ok(metrics[0].isTrimmed == FALSE, "got %d\n", metrics[0].isTrimmed);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);

    /* Test line height and baseline calculation */
    hr = IDWriteFactory_GetSystemFontCollection(factory, &syscollection, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    familycount = IDWriteFontCollection_GetFontFamilyCount(syscollection);

    for (i = 0; i < familycount; i++) {
        IDWriteLocalizedStrings *names;
        IDWriteFontFamily *family;
        IDWriteFont *font;
        BOOL exists;

        format = NULL;
        layout = NULL;

        hr = IDWriteFontCollection_GetFontFamily(syscollection, i, &family);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, &font);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFont_CreateFontFace(font, &fontface);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFontFamily_GetFamilyNames(family, &names);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (!(exists = get_enus_string(names, nameW, ARRAY_SIZE(nameW)))) {
            IDWriteLocalFontFileLoader *localloader;
            IDWriteFontFileLoader *loader;
            IDWriteFontFile *file;
            const void *key;
            UINT32 keysize;
            UINT32 count;

            count = 1;
            hr = IDWriteFontFace_GetFiles(fontface, &count, &file);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IDWriteFontFile_GetLoader(file, &loader);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IDWriteFontFileLoader_QueryInterface(loader, &IID_IDWriteLocalFontFileLoader, (void**)&localloader);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            IDWriteFontFileLoader_Release(loader);

            hr = IDWriteFontFile_GetReferenceKey(file, &key, &keysize);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IDWriteLocalFontFileLoader_GetFilePathFromKey(localloader, key, keysize, nameW, ARRAY_SIZE(nameW));
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            skip("Failed to get English family name, font file %s\n", wine_dbgstr_w(nameW));

            IDWriteLocalFontFileLoader_Release(localloader);
            IDWriteFontFile_Release(file);
        }

        IDWriteLocalizedStrings_Release(names);
        IDWriteFont_Release(font);

        if (!exists)
            goto cleanup;

        IDWriteFontFace_GetMetrics(fontface, &fontmetrics);
        hr = IDWriteFactory_CreateTextFormat(factory, nameW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, fontmetrics.designUnitsPerEm, L"en-us", &format);
        ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

        hr = IDWriteFactory_CreateTextLayout(factory, L"", 1, format, 30000.0f, 100.0f, &layout);
        ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

        memset(metrics, 0, sizeof(metrics));
        count = 0;
        hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(count == 1, "got %u\n", count);

        ok(metrics[0].baseline == fontmetrics.ascent + fontmetrics.lineGap, "%s: got %.2f, expected %d, "
            "linegap %d\n", wine_dbgstr_w(nameW), metrics[0].baseline, fontmetrics.ascent + fontmetrics.lineGap,
            fontmetrics.lineGap);
        ok(metrics[0].height == fontmetrics.ascent + fontmetrics.descent + fontmetrics.lineGap,
            "%s: got %.2f, expected %d, linegap %d\n", wine_dbgstr_w(nameW), metrics[0].height,
            fontmetrics.ascent + fontmetrics.descent + fontmetrics.lineGap, fontmetrics.lineGap);

    cleanup:
        if (layout)
            IDWriteTextLayout_Release(layout);
        if (format)
            IDWriteTextFormat_Release(format);
        IDWriteFontFace_Release(fontface);
        IDWriteFontFamily_Release(family);
    }
    IDWriteFontCollection_Release(syscollection);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 2048.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    fontface = get_fontface_from_format(format);
    ok(fontface != NULL, "got %p\n", fontface);

    /* force 2 lines */
    hr = IDWriteFactory_CreateTextLayout(factory, str2W, 5, format, 10000.0, 1000.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(metrics, 0, sizeof(metrics));
    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);
    /* baseline is relative to a line, and is not accumulated */
    ok(metrics[0].baseline == metrics[1].baseline, "got %.2f, %.2f\n", metrics[0].baseline,
        metrics[1].baseline);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);

    /* line breaks */
    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, str3W, 10, format, 100.0, 300.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(metrics, 0xcc, sizeof(metrics));
    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 6, "got %u\n", count);

    ok(metrics[0].length == 2, "got %u\n", metrics[0].length);
    ok(metrics[1].length == 2, "got %u\n", metrics[1].length);
    ok(metrics[2].length == 2, "got %u\n", metrics[2].length);
    ok(metrics[3].length == 1, "got %u\n", metrics[3].length);
    ok(metrics[4].length == 3, "got %u\n", metrics[4].length);
    ok(metrics[5].length == 0, "got %u\n", metrics[5].length);

    ok(metrics[0].newlineLength == 1, "got %u\n", metrics[0].newlineLength);
    ok(metrics[1].newlineLength == 1, "got %u\n", metrics[1].newlineLength);
    ok(metrics[2].newlineLength == 1, "got %u\n", metrics[2].newlineLength);
    ok(metrics[3].newlineLength == 1, "got %u\n", metrics[3].newlineLength);
    ok(metrics[4].newlineLength == 2, "got %u\n", metrics[4].newlineLength);
    ok(metrics[5].newlineLength == 0, "got %u\n", metrics[5].newlineLength);

    ok(metrics[0].trailingWhitespaceLength == 1, "got %u\n", metrics[0].newlineLength);
    ok(metrics[1].trailingWhitespaceLength == 1, "got %u\n", metrics[1].newlineLength);
    ok(metrics[2].trailingWhitespaceLength == 1, "got %u\n", metrics[2].newlineLength);
    ok(metrics[3].trailingWhitespaceLength == 1, "got %u\n", metrics[3].newlineLength);
    ok(metrics[4].trailingWhitespaceLength == 2, "got %u\n", metrics[4].newlineLength);
    ok(metrics[5].trailingWhitespaceLength == 0, "got %u\n", metrics[5].newlineLength);

    IDWriteTextLayout_Release(layout);

    /* empty text layout */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 0, format, 100.0f, 300.0f, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);
    ok(metrics[0].length == 0, "got %u\n", metrics[0].length);
    ok(metrics[0].trailingWhitespaceLength == 0, "got %u\n", metrics[0].trailingWhitespaceLength);
    ok(metrics[0].newlineLength == 0, "got %u\n", metrics[0].newlineLength);
    ok(metrics[0].height > 0.0f, "got %f\n", metrics[0].height);
    ok(metrics[0].baseline > 0.0f, "got %f\n", metrics[0].baseline);
    ok(!metrics[0].isTrimmed, "got %d\n", metrics[0].isTrimmed);

    /* change font size at first position, see if metrics changed */
    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetFontSize(layout, 80.0f, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics + 1, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);
    ok(metrics[1].height > metrics[0].height, "got %f\n", metrics[1].height);
    ok(metrics[1].baseline > metrics[0].baseline, "got %f\n", metrics[1].baseline);

    /* revert font size back to format value, set different size for position 1 */
    hr = IDWriteTextLayout_SetFontSize(layout, 12.0f, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetFontSize(layout, 80.0f, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(metrics + 1, 0, sizeof(*metrics));
    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics + 1, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);
    ok(metrics[1].height == metrics[0].height, "got %f\n", metrics[1].height);
    ok(metrics[1].baseline == metrics[0].baseline, "got %f\n", metrics[1].baseline);

    IDWriteTextLayout_Release(layout);

    /* text is "a\r" */
    hr = IDWriteFactory_CreateTextLayout(factory, str4W, 2, format, 100.0f, 300.0f, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    memset(metrics, 0, sizeof(metrics));
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);
    ok(metrics[0].length == 2, "got %u\n", metrics[0].length);
    ok(metrics[0].newlineLength == 1, "got %u\n", metrics[0].newlineLength);
    ok(metrics[0].height > 0.0f, "got %f\n", metrics[0].height);
    ok(metrics[0].baseline > 0.0f, "got %f\n", metrics[0].baseline);
    ok(metrics[1].length == 0, "got %u\n", metrics[1].length);
    ok(metrics[1].newlineLength == 0, "got %u\n", metrics[1].newlineLength);
    ok(metrics[1].height > 0.0f, "got %f\n", metrics[1].height);
    ok(metrics[1].baseline > 0.0f, "got %f\n", metrics[1].baseline);

    range.startPosition = 1;
    range.length = 1;
    hr = IDWriteTextLayout_SetFontSize(layout, 80.0f, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics + 2, 2, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);
    ok(metrics[3].height > metrics[1].height, "got %f, old %f\n", metrics[3].height, metrics[1].height);
    ok(metrics[3].baseline > metrics[1].baseline, "got %f, old %f\n", metrics[3].baseline, metrics[1].baseline);

    /* revert to original format */
    hr = IDWriteTextLayout_SetFontSize(layout, 12.0f, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics + 2, 2, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);
    ok(metrics[3].height == metrics[1].height, "got %f, old %f\n", metrics[3].height, metrics[1].height);
    ok(metrics[3].baseline == metrics[1].baseline, "got %f, old %f\n", metrics[3].baseline, metrics[1].baseline);

    /* Switch to uniform spacing */
    hr = IDWriteTextLayout_SetLineSpacing(layout, DWRITE_LINE_SPACING_METHOD_UNIFORM, 456.0f, 123.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);

    for (i = 0; i < count; i++) {
        ok(metrics[i].height == 456.0f, "%u: got line height %f\n", i, metrics[i].height);
        ok(metrics[i].baseline == 123.0f, "%u: got line baseline %f\n", i, metrics[i].baseline);
    }

    IDWriteTextLayout_Release(layout);

    /* Switch to proportional */
    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_PROPORTIONAL, 2.0f, 4.0f);
    if (hr == S_OK) {
        hr = IDWriteFactory_CreateTextLayout(factory, str4W, 1, format, 100.0f, 300.0f, &layout);
        ok(hr == S_OK, "Failed to create layout, hr %#lx.\n", hr);

        hr = IDWriteTextLayout_GetLineMetrics(layout, metrics, ARRAY_SIZE(metrics), &count);
        ok(hr == S_OK, "Failed to get line metrics, hr %#lx.\n", hr);
        ok(count == 1, "Unexpected line count %u\n", count);

        /* Back to default mode. */
        hr = IDWriteTextLayout_SetLineSpacing(layout, DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0f, 0.0f);
        ok(hr == S_OK, "Failed to set spacing method, hr %#lx.\n", hr);

        hr = IDWriteTextLayout_GetLineMetrics(layout, metrics + 1, 1, &count);
        ok(hr == S_OK, "Failed to get line metrics, hr %#lx.\n", hr);
        ok(count == 1, "Unexpected line count %u\n", count);

        /* Proportional spacing applies multipliers to default, content based spacing. */
        ok(metrics[0].height == 2.0f * metrics[1].height, "Unexpected line height %f.\n", metrics[0].height);
        ok(metrics[0].baseline == 4.0f * metrics[1].baseline, "Unexpected line baseline %f.\n", metrics[0].baseline);

        IDWriteTextLayout_Release(layout);
    }
    else
        win_skip("Proportional spacing is not supported.\n");

    IDWriteTextFormat_Release(format);
    IDWriteFontFace_Release(fontface);
    IDWriteFactory_Release(factory);
}

static void test_SetTextAlignment(void)
{
    static const WCHAR stringsW[][10] = {
        {'a',0},
        {0}
    };

    DWRITE_CLUSTER_METRICS clusters[10];
    DWRITE_TEXT_METRICS metrics;
    IDWriteTextFormat1 *format1;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_TEXT_ALIGNMENT v;
    UINT32 count, i;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    v = IDWriteTextFormat_GetTextAlignment(format);
    ok(v == DWRITE_TEXT_ALIGNMENT_LEADING, "got %d\n", v);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 500.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    v = IDWriteTextLayout_GetTextAlignment(layout);
    ok(v == DWRITE_TEXT_ALIGNMENT_LEADING, "got %d\n", v);

    hr = IDWriteTextLayout_SetTextAlignment(layout, DWRITE_TEXT_ALIGNMENT_TRAILING);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetTextAlignment(layout, DWRITE_TEXT_ALIGNMENT_TRAILING);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    v = IDWriteTextFormat_GetTextAlignment(format);
    ok(v == DWRITE_TEXT_ALIGNMENT_LEADING, "got %d\n", v);

    v = IDWriteTextLayout_GetTextAlignment(layout);
    ok(v == DWRITE_TEXT_ALIGNMENT_TRAILING, "got %d\n", v);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat1, (void**)&format1);
    if (hr == S_OK) {
        hr = IDWriteTextFormat1_SetTextAlignment(format1, DWRITE_TEXT_ALIGNMENT_CENTER);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        v = IDWriteTextFormat_GetTextAlignment(format);
        ok(v == DWRITE_TEXT_ALIGNMENT_LEADING, "got %d\n", v);

        v = IDWriteTextLayout_GetTextAlignment(layout);
        ok(v == DWRITE_TEXT_ALIGNMENT_CENTER, "got %d\n", v);

        v = IDWriteTextFormat1_GetTextAlignment(format1);
        ok(v == DWRITE_TEXT_ALIGNMENT_CENTER, "got %d\n", v);

        IDWriteTextFormat1_Release(format1);
    }
    else
        win_skip("IDWriteTextFormat1 is not supported\n");

    IDWriteTextLayout_Release(layout);

    for (i = 0; i < ARRAY_SIZE(stringsW); i++) {
        FLOAT text_width;

        hr = IDWriteTextFormat_SetTextAlignment(format, DWRITE_TEXT_ALIGNMENT_LEADING);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFactory_CreateTextLayout(factory, stringsW[i], lstrlenW(stringsW[i]), format, 500.0f, 100.0f, &layout);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextLayout_SetWordWrapping(layout, DWRITE_WORD_WRAPPING_NO_WRAP);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        count = 0;
        hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, ARRAY_SIZE(clusters), &count);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (stringsW[i][0])
            ok(count > 0, "got %u\n", count);
        else
            ok(count == 0, "got %u\n", count);

        text_width = 0.0f;
        while (count)
            text_width += clusters[--count].width;

        /* maxwidth is 500, leading alignment */
        hr = IDWriteTextLayout_SetTextAlignment(layout, DWRITE_TEXT_ALIGNMENT_LEADING);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ok(metrics.left == 0.0f, "got %.2f\n", metrics.left);
        ok(metrics.width == text_width, "got %.2f\n", metrics.width);
        ok(metrics.layoutWidth == 500.0f, "got %.2f\n", metrics.layoutWidth);
        ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);

        /* maxwidth is 500, trailing alignment */
        hr = IDWriteTextLayout_SetTextAlignment(layout, DWRITE_TEXT_ALIGNMENT_TRAILING);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ok(metrics.left == metrics.layoutWidth - metrics.width, "got %.2f\n", metrics.left);
        ok(metrics.width == text_width, "got %.2f\n", metrics.width);
        ok(metrics.layoutWidth == 500.0f, "got %.2f\n", metrics.layoutWidth);
        ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);
        IDWriteTextLayout_Release(layout);

        /* initially created with trailing alignment */
        hr = IDWriteTextFormat_SetTextAlignment(format, DWRITE_TEXT_ALIGNMENT_TRAILING);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFactory_CreateTextLayout(factory, stringsW[i], lstrlenW(stringsW[i]), format, 500.0f, 100.0f, &layout);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ok(metrics.left == metrics.layoutWidth - metrics.width, "got %.2f\n", metrics.left);
        ok(metrics.width == text_width, "got %.2f\n", metrics.width);
        ok(metrics.layoutWidth == 500.0f, "got %.2f\n", metrics.layoutWidth);
        ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);
        IDWriteTextLayout_Release(layout);

        if (stringsW[i][0]) {
            /* max width less than total run width, trailing alignment */
            hr = IDWriteTextFormat_SetWordWrapping(format, DWRITE_WORD_WRAPPING_NO_WRAP);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IDWriteFactory_CreateTextLayout(factory, stringsW[i], lstrlenW(stringsW[i]), format, clusters[0].width, 100.0f, &layout);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(metrics.left == metrics.layoutWidth - metrics.width, "got %.2f\n", metrics.left);
            ok(metrics.width == text_width, "got %.2f\n", metrics.width);
            ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);
            IDWriteTextLayout_Release(layout);
        }

        /* maxwidth is 500, centered */
        hr = IDWriteTextFormat_SetTextAlignment(format, DWRITE_TEXT_ALIGNMENT_CENTER);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFactory_CreateTextLayout(factory, stringsW[i], lstrlenW(stringsW[i]), format, 500.0f, 100.0f, &layout);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(metrics.left == (metrics.layoutWidth - metrics.width) / 2.0f, "got %.2f\n", metrics.left);
        ok(metrics.width == text_width, "got %.2f\n", metrics.width);
        ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);

        IDWriteTextLayout_Release(layout);
    }

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetParagraphAlignment(void)
{
    DWRITE_TEXT_METRICS metrics;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_PARAGRAPH_ALIGNMENT v;
    DWRITE_LINE_METRICS lines[1];
    UINT32 count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    v = IDWriteTextFormat_GetParagraphAlignment(format);
    ok(v == DWRITE_PARAGRAPH_ALIGNMENT_NEAR, "got %d\n", v);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 500.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    v = IDWriteTextLayout_GetParagraphAlignment(layout);
    ok(v == DWRITE_PARAGRAPH_ALIGNMENT_NEAR, "got %d\n", v);

    hr = IDWriteTextLayout_SetParagraphAlignment(layout, DWRITE_PARAGRAPH_ALIGNMENT_FAR);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetParagraphAlignment(layout, DWRITE_PARAGRAPH_ALIGNMENT_FAR);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    v = IDWriteTextFormat_GetParagraphAlignment(format);
    ok(v == DWRITE_PARAGRAPH_ALIGNMENT_NEAR, "got %d\n", v);

    v = IDWriteTextLayout_GetParagraphAlignment(layout);
    ok(v == DWRITE_PARAGRAPH_ALIGNMENT_FAR, "got %d\n", v);

    hr = IDWriteTextLayout_SetParagraphAlignment(layout, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    v = IDWriteTextLayout_GetParagraphAlignment(layout);
    ok(v == DWRITE_PARAGRAPH_ALIGNMENT_CENTER, "got %d\n", v);

    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, lines, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);

    /* maxheight is 100, near alignment */
    hr = IDWriteTextLayout_SetParagraphAlignment(layout, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.height == lines[0].height, "got %.2f\n", metrics.height);
    ok(metrics.layoutHeight == 100.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);

    /* maxwidth is 100, far alignment */
    hr = IDWriteTextLayout_SetParagraphAlignment(layout, DWRITE_PARAGRAPH_ALIGNMENT_FAR);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(metrics.top == metrics.layoutHeight - metrics.height, "got %.2f\n", metrics.top);
    ok(metrics.height == lines[0].height, "got %.2f\n", metrics.height);
    ok(metrics.layoutHeight == 100.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);
    IDWriteTextLayout_Release(layout);

    /* initially created with centered alignment */
    hr = IDWriteTextFormat_SetParagraphAlignment(format, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 500.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(metrics.top == (metrics.layoutHeight - lines[0].height) / 2, "got %.2f\n", metrics.top);
    ok(metrics.height == lines[0].height, "got %.2f\n", metrics.height);
    ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);
    IDWriteTextLayout_Release(layout);

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_SetReadingDirection(void)
{
    DWRITE_CLUSTER_METRICS clusters[1];
    DWRITE_TEXT_METRICS metrics;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_READING_DIRECTION v;
    DWRITE_LINE_METRICS lines[1];
    UINT32 count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    v = IDWriteTextFormat_GetReadingDirection(format);
    ok(v == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT, "got %d\n", v);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 500.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    v = IDWriteTextLayout_GetReadingDirection(layout);
    ok(v == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT, "got %d\n", v);

    v = IDWriteTextFormat_GetReadingDirection(format);
    ok(v == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT, "got %d\n", v);

    hr = IDWriteTextLayout_SetReadingDirection(layout, DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, lines, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);

    /* leading alignment, RTL */
    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(metrics.left == metrics.layoutWidth - clusters[0].width, "got %.2f\n", metrics.left);
    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.width == clusters[0].width, "got %.2f\n", metrics.width);
    ok(metrics.height == lines[0].height, "got %.2f\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "got %.2f\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 100.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);

    /* trailing alignment, RTL */
    hr = IDWriteTextLayout_SetTextAlignment(layout, DWRITE_TEXT_ALIGNMENT_TRAILING);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(metrics.left == 0.0, "got %.2f\n", metrics.left);
    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.width == clusters[0].width, "got %.2f\n", metrics.width);
    ok(metrics.height == lines[0].height, "got %.2f\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "got %.2f\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 100.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);

    /* centered alignment, RTL */
    hr = IDWriteTextLayout_SetTextAlignment(layout, DWRITE_TEXT_ALIGNMENT_CENTER);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(metrics.left == (metrics.layoutWidth - clusters[0].width) / 2.0, "got %.2f\n", metrics.left);
    ok(metrics.top == 0.0, "got %.2f\n", metrics.top);
    ok(metrics.width == clusters[0].width, "got %.2f\n", metrics.width);
    ok(metrics.height == lines[0].height, "got %.2f\n", metrics.height);
    ok(metrics.layoutWidth == 500.0, "got %.2f\n", metrics.layoutWidth);
    ok(metrics.layoutHeight == 100.0, "got %.2f\n", metrics.layoutHeight);
    ok(metrics.lineCount == 1, "got %d\n", metrics.lineCount);

    IDWriteTextLayout_Release(layout);

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static inline FLOAT get_scaled_font_metric(UINT32 metric, FLOAT emSize, const DWRITE_FONT_METRICS *metrics)
{
    return (FLOAT)metric * emSize / (FLOAT)metrics->designUnitsPerEm;
}

static FLOAT snap_coord(const DWRITE_MATRIX *m, FLOAT ppdip, FLOAT coord)
{
    FLOAT vec[2], det, vec2[2];
    BOOL transform;

    /* has to be a diagonal matrix */
    if ((ppdip <= 0.0) ||
        (m->m11 * m->m22 != 0.0 && (m->m12 != 0.0 || m->m21 != 0.0)) ||
        (m->m12 * m->m21 != 0.0 && (m->m11 != 0.0 || m->m22 != 0.0)))
        return coord;

    det = m->m11 * m->m22 - m->m12 * m->m21;
    transform = fabsf(det) > 1e-10;

    if (transform) {
        /* apply transform */
        vec[0] = 0.0;
        vec[1] = coord * ppdip;

        vec2[0] = m->m11 * vec[0] + m->m21 * vec[1] + m->dx;
        vec2[1] = m->m12 * vec[0] + m->m22 * vec[1] + m->dy;

        /* snap */
        vec2[0] = floorf(vec2[0] + 0.5f);
        vec2[1] = floorf(vec2[1] + 0.5f);

        /* apply inverted transform */
        vec[1] = (-m->m12 * vec2[0] + m->m11 * vec2[1] - (m->m11 * m->dy - m->m12 * m->dx)) / det;
        vec[1] /= ppdip;
    }
    else
        vec[1] = floorf(coord * ppdip + 0.5f) / ppdip;
    return vec[1];
}

static inline BOOL float_eq(FLOAT left, FLOAT right)
{
    int x = *(int *)&left;
    int y = *(int *)&right;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return abs(x - y) <= 16;
}

struct snapping_test {
    DWRITE_MATRIX m;
    FLOAT ppdip;
};

static struct snapping_test snapping_tests[] = {
    { {  0.0,  1.0,  2.0,  0.0, 0.2, 0.3 },   1.0 },
    { {  0.0,  1.0,  2.0,  0.0, 0.0, 0.0 },   1.0 },
    { {  1.0,  0.0,  0.0,  1.0, 0.0, 0.0 },   1.0 }, /* identity transform */
    { {  1.0,  0.0,  0.0,  1.0, 0.0, 0.0 },   0.9 },
    { {  1.0,  0.0,  0.0,  1.0, 0.0, 0.0 },  -1.0 },
    { {  1.0,  0.0,  0.0,  1.0, 0.0, 0.0 },   0.0 },
    { {  1.0,  0.0,  0.0,  1.0, 0.0, 0.3 },   1.0 }, /* simple Y shift */
    { {  1.0,  0.0,  0.0,  1.0, 0.0, 0.0 },  10.0 }, /* identity, 10 ppdip */
    { {  1.0,  0.0,  0.0, 10.0, 0.0, 0.0 },  10.0 },
    { {  0.0,  1.0,  1.0,  0.0, 0.2, 0.6 },   1.0 },
    { {  0.0,  2.0,  2.0,  0.0, 0.2, 0.6 },   1.0 },
    { {  0.0,  0.5, -0.5,  0.0, 0.2, 0.6 },   1.0 },
    { {  1.0,  2.0,  0.0,  1.0, 0.2, 0.6 },   1.0 },
    { {  1.0,  1.0,  0.0,  1.0, 0.2, 0.6 },   1.0 },
    { {  0.5,  0.5, -0.5,  0.5, 0.2, 0.6 },   1.0 }, /*  45 degrees rotation */
    { {  0.5,  0.5, -0.5,  0.5, 0.0, 0.0 }, 100.0 }, /*  45 degrees rotation */
    { {  1.0,  0.0,  0.0,  1.0, 0.0, 0.0 }, 100.0 },
    { {  0.0,  1.0, -1.0,  0.0, 0.2, 0.6 },   1.0 }, /*  90 degrees rotation */
    { { -1.0,  0.0,  0.0, -1.0, 0.2, 0.6 },   1.0 }, /* 180 degrees rotation */
    { {  0.0, -1.0,  1.0,  0.0, 0.2, 0.6 },   1.0 }, /* 270 degrees rotation */
    { {  1.0,  0.0,  0.0,  1.0,-0.1, 0.2 },   1.0 },
    { {  0.0,  1.0, -1.0,  0.0,-0.2,-0.3 },   1.0 }, /*  90 degrees rotation */
    { { -1.0,  0.0,  0.0, -1.0,-0.3,-1.6 },   1.0 }, /* 180 degrees rotation */
    { {  0.0, -1.0,  1.0,  0.0,-0.7, 0.6 },  10.0 }, /* 270 degrees rotation */
    { {  0.0,  2.0,  1.0,  0.0, 0.2, 0.6 },   1.0 },
    { {  0.0,  0.0,  1.0,  0.0, 0.0, 0.0 },   1.0 },
    { {  3.0,  0.0,  0.0,  5.0, 0.2,-0.3 },  10.0 },
    { {  0.0, -3.0,  5.0,  0.0,-0.1, 0.7 },  10.0 },
};

static DWRITE_MATRIX compattransforms[] = {
    { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 },
    { 1.0, 0.0, 0.0, 1.0, 0.2, 0.3 },
    { 2.0, 0.0, 0.0, 2.0, 0.2, 0.3 },
    { 2.0, 1.0, 2.0, 2.0, 0.2, 0.3 },
};

static void test_pixelsnapping(void)
{
    IDWriteTextLayout *layout, *layout2;
    struct renderer_context ctxt;
    DWRITE_FONT_METRICS metrics;
    IDWriteTextFormat *format;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    FLOAT baseline, originX;
    HRESULT hr;
    int i, j;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    fontface = get_fontface_from_format(format);
    IDWriteFontFace_GetMetrics(fontface, &metrics);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 500.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    /* disabled snapping */
    ctxt.snapping_disabled = TRUE;
    ctxt.gdicompat = FALSE;
    ctxt.use_gdi_natural = FALSE;
    ctxt.ppdip = 1.0f;
    memset(&ctxt.m, 0, sizeof(ctxt.m));
    ctxt.m.m11 = ctxt.m.m22 = 1.0;
    originX = 0.1;

    hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, originX, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    baseline = get_scaled_font_metric(metrics.ascent, 12.0, &metrics);
    ok(ctxt.originX == originX, "got %f, originX %f\n", ctxt.originX, originX);
    ok(ctxt.originY == baseline, "got %f, baseline %f\n", ctxt.originY, baseline);
    ok(floor(baseline) != baseline, "got %f\n", baseline);

    ctxt.snapping_disabled = FALSE;

    for (i = 0; i < ARRAY_SIZE(snapping_tests); i++) {
        struct snapping_test *ptr = &snapping_tests[i];
        FLOAT expectedY;

        ctxt.m = ptr->m;
        ctxt.ppdip = ptr->ppdip;
        ctxt.originX = 678.9;
        ctxt.originY = 678.9;

        expectedY = snap_coord(&ctxt.m, ctxt.ppdip, baseline);
        hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, originX, 0.0);
        ok(hr == S_OK, "%d: unexpected hr %#lx.\n", i, hr);
        ok(ctxt.originX == originX, "%d: got %f, originX %f\n", i, ctxt.originX, originX);
        ok(float_eq(ctxt.originY, expectedY), "%d: got %f, expected %f, baseline %f\n",
            i, ctxt.originY, expectedY, baseline);

        /* gdicompat layout transform doesn't affect snapping */
        for (j = 0; j < ARRAY_SIZE(compattransforms); ++j)
        {
            hr = IDWriteFactory_CreateGdiCompatibleTextLayout(factory, L"a", 1, format, 500.0f, 100.0f,
                    1.0f, &compattransforms[j], FALSE, &layout2);
            ok(hr == S_OK, "%d: failed to create text layout, hr %#lx.\n", i, hr);

            expectedY = snap_coord(&ctxt.m, ctxt.ppdip, baseline);
            hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, originX, 0.0);
            ok(hr == S_OK, "%d: unexpected hr %#lx.\n", i, hr);
            ok(ctxt.originX == originX, "%d: got %f, originX %f\n", i, ctxt.originX, originX);
            ok(float_eq(ctxt.originY, expectedY), "%d: got %f, expected %f, baseline %f\n",
                i, ctxt.originY, expectedY, baseline);

            IDWriteTextLayout_Release(layout2);
        }
    }

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFontFace_Release(fontface);
    IDWriteFactory_Release(factory);
}

static void test_SetWordWrapping(void)
{
    static const WCHAR strW[] = {'a',' ','s','o','m','e',' ','t','e','x','t',' ','a','n','d',
        ' ','a',' ','b','i','t',' ','m','o','r','e','\n','b'};
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory *factory;
    DWRITE_WORD_WRAPPING v;
    UINT32 count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    v = IDWriteTextFormat_GetWordWrapping(format);
    ok(v == DWRITE_WORD_WRAPPING_WRAP, "got %d\n", v);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, ARRAY_SIZE(strW), format, 10.0f, 100.0f, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    v = IDWriteTextLayout_GetWordWrapping(layout);
    ok(v == DWRITE_WORD_WRAPPING_WRAP, "got %d\n", v);

    hr = IDWriteTextLayout_SetWordWrapping(layout, DWRITE_WORD_WRAPPING_NO_WRAP);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetWordWrapping(layout, DWRITE_WORD_WRAPPING_NO_WRAP);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    v = IDWriteTextFormat_GetWordWrapping(format);
    ok(v == DWRITE_WORD_WRAPPING_WRAP, "got %d\n", v);

    /* disable wrapping, text has explicit newline */
    hr = IDWriteTextLayout_SetWordWrapping(layout, DWRITE_WORD_WRAPPING_NO_WRAP);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);

    hr = IDWriteTextLayout_SetWordWrapping(layout, DWRITE_WORD_WRAPPING_WRAP);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetLineMetrics(layout, NULL, 0, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(count > 2, "got %u\n", count);

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

/* Collection dedicated to fallback testing */

static const WCHAR g_blahfontW[] = {'B','l','a','h',0};
static HRESULT WINAPI fontcollection_QI(IDWriteFontCollection *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDWriteFontCollection) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteFontCollection_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI fontcollection_AddRef(IDWriteFontCollection *iface)
{
    return 2;
}

static ULONG WINAPI fontcollection_Release(IDWriteFontCollection *iface)
{
    return 1;
}

static UINT32 WINAPI fontcollection_GetFontFamilyCount(IDWriteFontCollection *iface)
{
    ok(0, "unexpected call\n");
    return 0;
}

static HRESULT WINAPI fontcollection_GetFontFamily(IDWriteFontCollection *iface, UINT32 index, IDWriteFontFamily **family)
{
    if (index == 123456)
    {
        IDWriteFactory *factory = create_factory();
        IDWriteFontCollection *syscollection;
        BOOL exists;
        HRESULT hr;

        hr = IDWriteFactory_GetSystemFontCollection(factory, &syscollection, FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFontCollection_FindFamilyName(syscollection, L"Tahoma", &index, &exists);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFontCollection_GetFontFamily(syscollection, index, family);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IDWriteFontCollection_Release(syscollection);
        IDWriteFactory_Release(factory);
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI fontcollection_FindFamilyName(IDWriteFontCollection *iface, WCHAR const *name, UINT32 *index, BOOL *exists)
{
    if (!lstrcmpW(name, g_blahfontW)) {
        *index = 123456;
        *exists = TRUE;
        return S_OK;
    }
    ok(0, "unexpected call, name %s\n", wine_dbgstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI fontcollection_GetFontFromFontFace(IDWriteFontCollection *iface, IDWriteFontFace *face, IDWriteFont **font)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IDWriteFontCollectionVtbl fallbackcollectionvtbl = {
    fontcollection_QI,
    fontcollection_AddRef,
    fontcollection_Release,
    fontcollection_GetFontFamilyCount,
    fontcollection_GetFontFamily,
    fontcollection_FindFamilyName,
    fontcollection_GetFontFromFontFace
};

static IDWriteFontCollection fallbackcollection = { &fallbackcollectionvtbl };

static void get_font_name(IDWriteFont *font, WCHAR *name, UINT32 size)
{
    IDWriteLocalizedStrings *names;
    IDWriteFontFamily *family;
    UINT32 index;
    BOOL exists;
    HRESULT hr;

    hr = IDWriteFont_GetFontFamily(font, &family);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontFamily_GetFamilyNames(family, &names);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteLocalizedStrings_FindLocaleName(names, L"en-us", &index, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(index != UINT_MAX && exists, "Name was not found.\n");
    hr = IDWriteLocalizedStrings_GetString(names, index, name, size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteLocalizedStrings_Release(names);
    IDWriteFontFamily_Release(family);
}

static void utf32_to_utf16(UINT32 C, WCHAR **ptr)
{
    if (C > 0xffff)
    {
        WCHAR X = (WCHAR)C;
        UINT32 U = (C >> 16) & ((1 << 5) - 1);
        WCHAR W = (WCHAR)U - 1;

        **ptr = 0xD800 | (W << 6) | X >> 10; *ptr += 1;
        **ptr = (WCHAR)(0xDC00 | (X & ((1 << 10) - 1))); *ptr += 1;
    }
    else
    {
        **ptr = C; *ptr += 1;
    }
}

static void test_MapCharacters(void)
{
    static const WCHAR str2W[] = {'a',0x3058,'b',0};
    static const UINT32 variation_selectors[] =
    {
        0x180b, 0x180c, 0x180d,

        0xfe00, 0xfe01, 0xfe02, 0xfe03, 0xfe04, 0xfe05, 0xfe06, 0xfe07,
        0xfe08, 0xfe09, 0xfe0a, 0xfe0b, 0xfe0c, 0xfe0d, 0xfe0e, 0xfe0f,

        0xe0100, 0xe0121, 0xe01ef,
    };
    IDWriteFontCollectionLoader *resource_collection_loader;
    IDWriteFontFileLoader *resource_loader;
    IDWriteFontCollection *collection;
    IDWriteLocalizedStrings *strings;
    UINT32 mappedlength, vs_length;
    IDWriteFontFallback *fallback;
    IDWriteFactory2 *factory2;
    IDWriteFont *font;
    WCHAR buffW[50];
    WCHAR name[64];
    unsigned int i;
    HRSRC hrsrc;
    BOOL exists;
    FLOAT scale;
    WCHAR *ptr;
    HRESULT hr;

    factory2 = create_factory_iid(&IID_IDWriteFactory2);
    if (!factory2)
    {
        win_skip("MapCharacters() is not supported.\n");
        return;
    }

    fallback = NULL;
    hr = IDWriteFactory2_GetSystemFontFallback(factory2, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fallback != NULL, "got %p\n", fallback);

    mappedlength = 1;
    scale = 0.0f;
    font = (void*)0xdeadbeef;
    hr = IDWriteFontFallback_MapCharacters(fallback, NULL, 0, 0, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 0, "got %u\n", mappedlength);
    ok(scale == 1.0f, "got %f\n", scale);
    ok(font == NULL, "got %p\n", font);

    /* zero length source */
    g_source = L"abc";
    mappedlength = 1;
    scale = 0.0f;
    font = (void*)0xdeadbeef;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 0, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 0, "got %u\n", mappedlength);
    ok(scale == 1.0f, "got %f\n", scale);
    ok(font == NULL, "got %p\n", font);

    g_source = L"abc";
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 1, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "got %u\n", mappedlength);
    ok(scale == 1.0f, "got %f\n", scale);
    ok(font != NULL, "got %p\n", font);
    if (font)
        IDWriteFont_Release(font);

    /* same Latin text, full length */
    g_source = L"abc";
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 3, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 3, "got %u\n", mappedlength);

    ok(scale == 1.0f, "got %f\n", scale);
    ok(font != NULL, "got %p\n", font);
    if (font)
        IDWriteFont_Release(font);

    /* string 'a\x3058b' */
    g_source = str2W;
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 3, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "got %u\n", mappedlength);
    ok(scale == 1.0f, "got %f\n", scale);
    ok(font != NULL, "got %p\n", font);
    if (font)
        IDWriteFont_Release(font);

    g_source = str2W;
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 1, 2, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "Unexpected length %u.\n", mappedlength);
    ok(scale == 1.0f, "got %f\n", scale);
    ok(font != NULL, "got %p\n", font);
    if (font)
        IDWriteFont_Release(font);

    /* Try with explicit collection, Tahoma will be forced. */
    /* 1. Latin part */
    g_source = str2W;
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 3, &fallbackcollection, g_blahfontW, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "got %u\n", mappedlength);
    ok(scale == 1.0f, "got %f\n", scale);
    ok(font != NULL, "got %p\n", font);

    exists = FALSE;
    hr = IDWriteFont_GetInformationalStrings(font, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &strings, &exists);
    ok(hr == S_OK && exists, "Unexpected hr %#lx, exists %d.\n", hr, exists);
    hr = IDWriteLocalizedStrings_GetString(strings, 0, buffW, ARRAY_SIZE(buffW));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"Tahoma"), "Unexpected string %s.\n", wine_dbgstr_w(buffW));
    IDWriteLocalizedStrings_Release(strings);
    IDWriteFont_Release(font);

    /* 2. Hiragana character, force Tahoma font does not support Japanese */
    g_source = str2W;
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 1, 1, &fallbackcollection, g_blahfontW, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "got %u\n", mappedlength);
    ok(scale == 1.0f, "got %f\n", scale);
    ok(font != NULL, "got %p\n", font);

    if (font)
    {
        exists = FALSE;
        hr = IDWriteFont_GetInformationalStrings(font, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &strings, &exists);
        ok(hr == S_OK && exists, "Unexpected hr %#lx, exists %d.\n", hr, exists);
        hr = IDWriteLocalizedStrings_GetString(strings, 0, buffW, ARRAY_SIZE(buffW));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(lstrcmpW(buffW, L"Tahoma"), "Unexpected string %s.\n", wine_dbgstr_w(buffW));
        IDWriteLocalizedStrings_Release(strings);
        IDWriteFont_Release(font);
    }

    /* Control characters. The test is using a bundled font that does not support them. */
    resource_loader = create_resource_file_loader();
    resource_collection_loader = create_resource_collection_loader(resource_loader);

    hr = IDWriteFactory2_RegisterFontFileLoader(factory2, resource_loader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDWriteFactory2_RegisterFontCollectionLoader(factory2, resource_collection_loader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hrsrc = FindResourceA(GetModuleHandleA(NULL), (LPCSTR)MAKEINTRESOURCE(1), (LPCSTR)RT_RCDATA);
    ok(!!hrsrc, "Failed to find font resource\n");

    hr = IDWriteFactory2_CreateCustomFontCollection(factory2, resource_collection_loader, &hrsrc, sizeof(hrsrc), &collection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n",hr);

    /* Variation selectors are skipped. */
    for (i = 0; i < ARRAY_SIZE(variation_selectors); ++i)
    {
        g_source = buffW;

        vs_length = variation_selectors[i] > 0xffff ? 2 : 1;

        winetest_push_context("Test %#x", variation_selectors[i]);

        /* Selector within supported text. */

        font = NULL;
        ptr = buffW;

        *ptr = 'A'; ptr++;
        utf32_to_utf16(variation_selectors[i], &ptr);
        *ptr = 'A';

        hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, vs_length + 1, collection, L"wine_test",
                DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(mappedlength == vs_length + 1, "Unexpected mapped length %u.\n", mappedlength);
        get_font_name(font, name, ARRAY_SIZE(name));
        ok(!wcscmp(name, L"wine_test"), "Unexpected font %s.\n", debugstr_w(name));
        IDWriteFont_Release(font);

        /* Only selectors. */
        font = NULL;
        ptr = buffW;
        utf32_to_utf16(variation_selectors[i], &ptr);
        hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, vs_length, collection, L"wine_test",
                DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(mappedlength == vs_length, "Unexpected mapped length %u.\n", mappedlength);
        if (variation_selectors[i] < 0xfe00)
        {
            /* Mongolian selectors belong to Mongolian range, and are mapped to a specific font. */
            if (font) IDWriteFont_Release(font);
        }
        else
        {
            /* Variation selectors do not have a fallback mapping on their own. */
            ok(!font, "Unexpected font instance.\n");
        }

        /* Leading selector. Only use VS-1..16 so we don't hit a valid mapping range. */
        if (variation_selectors[i] >= 0xfe00)
        {
            ptr = buffW;
            utf32_to_utf16(variation_selectors[i], &ptr);
            utf32_to_utf16(variation_selectors[i], &ptr);
            *ptr = 'A';

            font = (void *)0xdeadbeef;
            hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, vs_length * 2 + 1, collection, L"wine_test",
                    DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(mappedlength == vs_length * 2, "Unexpected mapped length %u.\n", mappedlength);
            ok(!font, "Unexpected font instance.\n");

            hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, vs_length * 2 + 1, NULL, NULL,
                    DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(mappedlength == vs_length * 2, "Unexpected mapped length %u.\n", mappedlength);
            ok(!font, "Unexpected font instance.\n");
        }

        /* Trailing selector. */
        ptr = buffW;
        *ptr = 'A'; ptr++;
        utf32_to_utf16(variation_selectors[i], &ptr);

        font = (void *)0xdeadbeef;
        hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, vs_length + 1, collection, L"wine_test",
                DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(mappedlength == vs_length + 1, "Unexpected mapped length %u.\n", mappedlength);
        get_font_name(font, name, ARRAY_SIZE(name));
        ok(!wcscmp(name, L"wine_test"), "Unexpected font %s.\n", debugstr_w(name));
        IDWriteFont_Release(font);

        winetest_pop_context();
    }

    IDWriteFontCollection_Release(collection);

    hr = IDWriteFactory2_UnregisterFontCollectionLoader(factory2, resource_collection_loader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDWriteFactory2_UnregisterFontFileLoader(factory2, resource_loader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteFontCollectionLoader_Release(resource_collection_loader);
    IDWriteFontFileLoader_Release(resource_loader);

    IDWriteFontFallback_Release(fallback);
    IDWriteFactory2_Release(factory2);
}

static BOOL is_font_name_match(const WCHAR *name, const WCHAR *list)
{
     WCHAR *p, str[256];

     wcscpy(str, list);
     for (p = wcstok(str, L";"); p; p = wcstok(NULL, L";"))
     {
         if (!wcscmp(p, name))
             return TRUE;
     }
     return FALSE;
}

static void test_system_fallback(void)
{
    static const struct fallback_test
    {
        const WCHAR text[2];
        const WCHAR *name;
    }
    tests[] =
    {
        { { 0x25d4, 0}, L"Segoe UI Symbol;Meiryo UI" },
    };
    IDWriteFontFallback *fallback;
    IDWriteFactory2 *factory;
    IDWriteFont *font;
    UINT32 i, length;
    WCHAR name[256];
    BOOL exists;
    float scale;
    HRESULT hr;

    if (!(factory = create_factory_iid(&IID_IDWriteFactory2)))
    {
        win_skip("System font fallback API is not supported.\n");
        return;
    }

    hr = IDWriteFactory2_GetSystemFontFallback(factory, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        font = NULL;

        g_source = tests[i].text;
        hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 1, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &length, &font, &scale);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (hr != S_OK) continue;
        ok(length == 1, "Unexpected length %u\n", length);
        ok(scale == 1.0f, "got %f\n", scale);
        if (!font) continue;

        get_font_name(font, name, ARRAY_SIZE(name));
        todo_wine
        ok(is_font_name_match(name, tests[i].name), "%u: unexpected name %s.\n", i, wine_dbgstr_w(name));

        hr = IDWriteFont_HasCharacter(font, g_source[0], &exists);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine
        ok(exists, "%s missing character %#x\n", wine_dbgstr_w(name), g_source[0]);

        IDWriteFont_Release(font);
    }

    IDWriteFontFallback_Release(fallback);
    IDWriteFactory2_Release(factory);
}

static void test_FontFallbackBuilder(void)
{
    IDWriteFontFallback *fallback, *fallback2;
    IDWriteFontFallbackBuilder *builder;
    IDWriteFontFallback1 *fallback1;
    DWRITE_UNICODE_RANGE range;
    IDWriteFactory2 *factory2;
    IDWriteFactory *factory;
    const WCHAR *familyW;
    UINT32 mappedlength;
    IDWriteFont *font;
    FLOAT scale;
    HRESULT hr;
    ULONG ref;

    factory = create_factory();

    hr = IDWriteFactory_QueryInterface(factory, &IID_IDWriteFactory2, (void**)&factory2);
    IDWriteFactory_Release(factory);

    if (hr != S_OK) {
        win_skip("IDWriteFontFallbackBuilder is not supported\n");
        return;
    }

    EXPECT_REF(factory2, 1);
    hr = IDWriteFactory2_CreateFontFallbackBuilder(factory2, &builder);
    EXPECT_REF(factory2, 2);

    fallback = NULL;
    EXPECT_REF(factory2, 2);
    EXPECT_REF(builder, 1);
    hr = IDWriteFontFallbackBuilder_CreateFontFallback(builder, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(factory2, 3);
    EXPECT_REF(fallback, 1);
    EXPECT_REF(builder, 1);

    IDWriteFontFallback_AddRef(fallback);
    EXPECT_REF(builder, 1);
    EXPECT_REF(fallback, 2);
    todo_wine EXPECT_REF(factory2, 3);
    IDWriteFontFallback_Release(fallback);

    /* New instance is created every time, even if mappings have not changed. */
    hr = IDWriteFontFallbackBuilder_CreateFontFallback(builder, &fallback2);
    ok(hr == S_OK, "Failed to create fallback object, hr %#lx.\n", hr);
    ok(fallback != fallback2, "Unexpected fallback instance.\n");
    IDWriteFontFallback_Release(fallback2);

    hr = IDWriteFontFallbackBuilder_AddMapping(builder, NULL, 0, NULL, 0, NULL, NULL, NULL, 0.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.first = 'A';
    range.last = 'B';
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 0, NULL, 0, NULL, NULL, NULL, 0.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 0, NULL, 0, NULL, NULL, NULL, 1.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 0, &familyW, 1, NULL, NULL, NULL, 1.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontFallbackBuilder_AddMapping(builder, NULL, 0, &familyW, 1, NULL, NULL, NULL, 1.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* negative scaling factor */
    range.first = range.last = 0;
    familyW = g_blahfontW;
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, NULL, NULL, NULL, -1.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, NULL, NULL, NULL, 0.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* empty range */
    range.first = range.last = 0;
    familyW = g_blahfontW;
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, NULL, NULL, NULL, 1.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.first = range.last = 0;
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, NULL, NULL, NULL, 2.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.first = range.last = 'A';
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, NULL, NULL, NULL, 3.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.first = 'B';
    range.last = 'A';
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, NULL, NULL, NULL, 4.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteFontFallback_Release(fallback);

    if (0) /* crashes on native */
        hr = IDWriteFontFallbackBuilder_CreateFontFallback(builder, NULL);

    hr = IDWriteFontFallbackBuilder_CreateFontFallback(builder, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* fallback font missing from system collection */
    g_source = L"A";
    mappedlength = 0;
    scale = 0.0f;
    font = (void*)0xdeadbeef;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 1, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "Unexpected length %u.\n", mappedlength);
    ok(scale == 1.0f, "Unexpected scale %f.\n", scale);
    ok(!font, "Unexpected font instance %p.\n", font);

    IDWriteFontFallback_Release(fallback);

    /* remap with custom collection */
    range.first = range.last = 'A';
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, &fallbackcollection, NULL, NULL, 5.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontFallbackBuilder_CreateFontFallback(builder, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    g_source = L"A";
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 1, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "Unexpected length %u.\n", mappedlength);
    ok(scale == 5.0f, "Unexpected scale %f.\n", scale);
    ok(font != NULL, "Expected font instance %p.\n", font);
    if (font)
        IDWriteFont_Release(font);

    IDWriteFontFallback_Release(fallback);

    range.first = 'B';
    range.last = 'A';
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, &fallbackcollection, NULL, NULL, 6.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontFallbackBuilder_CreateFontFallback(builder, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    g_source = L"A";
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 1, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "Unexpected length %u.\n", mappedlength);
    ok(scale == 5.0f, "Unexpected scale %f.\n", scale);
    ok(font != NULL, "Expected font instance %p.\n", font);
    if (font)
        IDWriteFont_Release(font);

    IDWriteFontFallback_Release(fallback);

    /* explicit locale */
    range.first = 'A';
    range.last = 'B';
    hr = IDWriteFontFallbackBuilder_AddMapping(builder, &range, 1, &familyW, 1, &fallbackcollection, L"locale", NULL, 6.0f);
    ok(hr == S_OK, "Failed to add mapping, hr %#lx.\n", hr);

    hr = IDWriteFontFallbackBuilder_CreateFontFallback(builder, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    g_source = L"A";
    mappedlength = 0;
    scale = 0.0f;
    font = NULL;
    hr = IDWriteFontFallback_MapCharacters(fallback, &analysissource, 0, 1, NULL, NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedlength, &font, &scale);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(mappedlength == 1, "Unexpected length %u.\n", mappedlength);
    ok(scale == 5.0f, "Unexpected scale %f.\n", scale);
    ok(font != NULL, "Expected font instance %p.\n", font);
    if (font)
        IDWriteFont_Release(font);

    if (SUCCEEDED(IDWriteFontFallback_QueryInterface(fallback, &IID_IDWriteFontFallback1, (void **)&fallback1)))
    {
        IDWriteFontFallback1_Release(fallback1);
    }
    else
        win_skip("IDWriteFontFallback1 is not supported.\n");

    IDWriteFontFallback_Release(fallback);

    IDWriteFontFallbackBuilder_Release(builder);
    ref = IDWriteFactory2_Release(factory2);
    ok(ref == 0, "Factory is not released, ref %lu.\n", ref);
}

static void test_fallback(void)
{
    IDWriteFontFallback *fallback, *fallback2;
    IDWriteFontFallback1 *fallback1;
    DWRITE_CLUSTER_METRICS clusters[4];
    DWRITE_TEXT_METRICS metrics;
    IDWriteTextLayout2 *layout2;
    IDWriteTextFormat1 *format1;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    IDWriteFactory2 *factory2;
    IDWriteFactory *factory;
    UINT32 count, i;
    FLOAT width;
    HRESULT hr;

    factory = create_factory();

    /* Font does not exist in system collection. */
    hr = IDWriteFactory_CreateTextFormat(factory, g_blahfontW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, 4, &count);
    ok(hr == S_OK, "Failed to get cluster metrics, hr %#lx.\n", hr);
    ok(count == 4, "Unexpected count %u.\n", count);
    for (i = 0, width = 0.0; i < count; i++)
        width += clusters[i].width;

    memset(&metrics, 0xcc, sizeof(metrics));
    hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
    ok(hr == S_OK, "Failed to get layout metrics, hr %#lx.\n", hr);
    ok(metrics.width > 0.0 && metrics.width == width, "Unexpected width %.2f, expected %.2f.\n", metrics.width, width);
    ok(metrics.height > 0.0, "Unexpected height %.2f.\n", metrics.height);
    ok(metrics.lineCount == 1, "Unexpected line count %u.\n", metrics.lineCount);
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    /* Existing font. */
    hr = IDWriteFactory_CreateTextLayout(factory, L"abcd", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    IDWriteTextFormat_Release(format);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout2, (void**)&layout2);
    IDWriteTextLayout_Release(layout);

    if (hr != S_OK) {
        win_skip("GetFontFallback() is not supported.\n");
        IDWriteFactory_Release(factory);
        return;
    }

    if (0) /* crashes on native */
        hr = IDWriteTextLayout2_GetFontFallback(layout2, NULL);

    fallback = (void*)0xdeadbeef;
    hr = IDWriteTextLayout2_GetFontFallback(layout2, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fallback == NULL, "got %p\n", fallback);

    hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextFormat1, (void**)&format1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    fallback = (void*)0xdeadbeef;
    hr = IDWriteTextFormat1_GetFontFallback(format1, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fallback == NULL, "got %p\n", fallback);

    hr = IDWriteFactory_QueryInterface(factory, &IID_IDWriteFactory2, (void**)&factory2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    fallback = NULL;
    hr = IDWriteFactory2_GetSystemFontFallback(factory2, &fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fallback != NULL, "got %p\n", fallback);

    hr = IDWriteTextFormat1_SetFontFallback(format1, fallback);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    fallback2 = (void*)0xdeadbeef;
    hr = IDWriteTextLayout2_GetFontFallback(layout2, &fallback2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fallback2 == fallback, "got %p\n", fallback2);

    hr = IDWriteTextLayout2_SetFontFallback(layout2, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    fallback2 = (void*)0xdeadbeef;
    hr = IDWriteTextFormat1_GetFontFallback(format1, &fallback2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fallback2 == NULL, "got %p\n", fallback2);

    if (SUCCEEDED(IDWriteFontFallback_QueryInterface(fallback, &IID_IDWriteFontFallback1, (void **)&fallback1)))
    {
        IDWriteFontFallback1_Release(fallback1);
    }
    else
        win_skip("IDWriteFontFallback1 is not supported.\n");

    IDWriteFontFallback_Release(fallback);
    IDWriteTextFormat1_Release(format1);
    IDWriteTextLayout2_Release(layout2);
    IDWriteFactory_Release(factory);
}

static void test_SetTypography(void)
{
    IDWriteTypography *typography, *typography2;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"afib", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);
    IDWriteTextFormat_Release(format);

    hr = IDWriteFactory_CreateTypography(factory, &typography);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(typography, 1);
    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetTypography(layout, typography, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(typography, 2);

    hr = IDWriteTextLayout_GetTypography(layout, 0, &typography2, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(typography2 == typography, "got %p, expected %p\n", typography2, typography);
    IDWriteTypography_Release(typography2);
    IDWriteTypography_Release(typography);

    hr = IDWriteFactory_CreateTypography(factory, &typography2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetTypography(layout, typography2, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(typography2, 2);
    IDWriteTypography_Release(typography2);

    hr = IDWriteTextLayout_GetTypography(layout, 0, &typography, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.length == 1, "got %u\n", range.length);

    IDWriteTypography_Release(typography);

    IDWriteTextLayout_Release(layout);
    IDWriteFactory_Release(factory);
}

static void test_SetLastLineWrapping(void)
{
    IDWriteTextLayout2 *layout2;
    IDWriteTextFormat1 *format1;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    HRESULT hr;
    BOOL ret;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat1, (void**)&format1);
    IDWriteTextFormat_Release(format);
    if (hr != S_OK) {
        win_skip("SetLastLineWrapping() is not supported\n");
        IDWriteFactory_Release(factory);
        return;
    }

    ret = IDWriteTextFormat1_GetLastLineWrapping(format1);
    ok(ret, "got %d\n", ret);

    hr = IDWriteTextFormat1_SetLastLineWrapping(format1, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, (IDWriteTextFormat *)format1, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout2, (void**)&layout2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDWriteTextLayout_Release(layout);

    ret = IDWriteTextLayout2_GetLastLineWrapping(layout2);
    ok(!ret, "got %d\n", ret);

    hr = IDWriteTextLayout2_SetLastLineWrapping(layout2, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteTextLayout2_Release(layout2);
    IDWriteTextFormat1_Release(format1);
    IDWriteFactory_Release(factory);
}

static void test_SetOpticalAlignment(void)
{
    DWRITE_OPTICAL_ALIGNMENT alignment;
    IDWriteTextLayout2 *layout2;
    IDWriteTextFormat1 *format1;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat1, (void**)&format1);
    IDWriteTextFormat_Release(format);
    if (hr != S_OK) {
        win_skip("SetOpticalAlignment() is not supported\n");
        IDWriteFactory_Release(factory);
        return;
    }

    alignment = IDWriteTextFormat1_GetOpticalAlignment(format1);
    ok(alignment == DWRITE_OPTICAL_ALIGNMENT_NONE, "got %d\n", alignment);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, (IDWriteTextFormat *)format1, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout2, (void**)&layout2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat1_Release(format1);

    alignment = IDWriteTextLayout2_GetOpticalAlignment(layout2);
    ok(alignment == DWRITE_OPTICAL_ALIGNMENT_NONE, "got %d\n", alignment);

    hr = IDWriteTextLayout2_QueryInterface(layout2, &IID_IDWriteTextFormat1, (void**)&format1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    alignment = IDWriteTextFormat1_GetOpticalAlignment(format1);
    ok(alignment == DWRITE_OPTICAL_ALIGNMENT_NONE, "got %d\n", alignment);

    hr = IDWriteTextLayout2_SetOpticalAlignment(layout2, DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout2_SetOpticalAlignment(layout2, DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS+1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    alignment = IDWriteTextFormat1_GetOpticalAlignment(format1);
    ok(alignment == DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS, "got %d\n", alignment);

    hr = IDWriteTextFormat1_SetOpticalAlignment(format1, DWRITE_OPTICAL_ALIGNMENT_NONE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat1_SetOpticalAlignment(format1, DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS+1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    alignment = IDWriteTextLayout2_GetOpticalAlignment(layout2);
    ok(alignment == DWRITE_OPTICAL_ALIGNMENT_NONE, "got %d\n", alignment);

    IDWriteTextLayout2_Release(layout2);
    IDWriteTextFormat1_Release(format1);
    IDWriteFactory_Release(factory);
}

static const struct drawcall_entry drawunderline_seq[] = {
    { DRAW_GLYPHRUN, {'a','e',0x0300,0}, {'e','n','-','u','s',0}, 2 }, /* reported runs can't mix different underline values */
    { DRAW_GLYPHRUN, {'d',0}, {'e','n','-','u','s',0}, 1 },
    { DRAW_UNDERLINE, {0}, {'e','n','-','u','s',0} },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry drawunderline2_seq[] = {
    { DRAW_GLYPHRUN, {'a',0}, {'e','n','-','u','s',0}, 1 },
    { DRAW_GLYPHRUN, {'e',0}, {'e','n','-','u','s',0}, 1 },
    { DRAW_UNDERLINE, {0}, {'e','n','-','u','s',0} },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry drawunderline3_seq[] = {
    { DRAW_GLYPHRUN, {'a',0}, {'e','n','-','c','a',0}, 1 },
    { DRAW_GLYPHRUN, {'e',0}, {'e','n','-','u','s',0}, 1 },
    { DRAW_UNDERLINE, {0}, {'e','n','-','c','a',0} },
    { DRAW_UNDERLINE, {0}, {'e','n','-','u','s',0} },
    { DRAW_LAST_KIND }
};

static const struct drawcall_entry drawunderline4_seq[] = {
    { DRAW_GLYPHRUN, {'a',0}, {'e','n','-','u','s',0}, 1 },
    { DRAW_GLYPHRUN, {'e',0}, {'e','n','-','u','s',0}, 1 },
    { DRAW_UNDERLINE, {0}, {'e','n','-','u','s',0} },
    { DRAW_STRIKETHROUGH },
    { DRAW_LAST_KIND }
};

static void test_SetUnderline(void)
{
    static const WCHAR strW[] = {'a','e',0x0300,'d',0}; /* accent grave */
    IDWriteFontCollection *syscollection;
    DWRITE_CLUSTER_METRICS clusters[4];
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    UINT32 count, i;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, strW, 4, format, 1000.0, 1000.0, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, ARRAY_SIZE(clusters), &count);
    ok(hr == S_OK, "Failed to get cluster metrics, hr %#lx.\n", hr);
    todo_wine
    ok(count == 3, "Unexpected cluster count %u.\n", count);

    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, ARRAY_SIZE(clusters), &count);
    ok(hr == S_OK, "Failed to get cluster metrics, hr %#lx.\n", hr);
    todo_wine
    ok(count == 3, "Unexpected cluster count %u.\n", count);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0, 0.0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, drawunderline_seq, "draw underline test", TRUE);

    IDWriteTextLayout_Release(layout);

    /* 2 characters, same font, significantly different font size. Set underline for both, see how many
       underline drawing calls is there. */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 2, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetFontSize(layout, 100.0f, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0f, 0.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, drawunderline2_seq, "draw underline test 2", FALSE);

    /* now set different locale for second char, draw again */
    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetLocaleName(layout, L"en-CA", range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0f, 0.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, drawunderline3_seq, "draw underline test 2", FALSE);

    IDWriteTextLayout_Release(layout);

    /* 2 characters, same font properties, first with strikethrough, both underlined */
    hr = IDWriteFactory_CreateTextLayout(factory, strW, 2, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 1;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = 2;
    hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flush_sequence(sequences, RENDERER_ID);
    hr = IDWriteTextLayout_Draw(layout, NULL, &testrenderer, 0.0f, 0.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_sequence(sequences, RENDERER_ID, drawunderline4_seq, "draw underline test 4", FALSE);

    IDWriteTextLayout_Release(layout);

    IDWriteTextFormat_Release(format);

    /* Test runHeight value with all available fonts */
    hr = IDWriteFactory_GetSystemFontCollection(factory, &syscollection, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    count = IDWriteFontCollection_GetFontFamilyCount(syscollection);

    for (i = 0; i < count; ++i)
    {
        DWRITE_FONT_METRICS fontmetrics;
        IDWriteLocalizedStrings *names;
        struct renderer_context ctxt;
        IDWriteFontFamily *family;
        IDWriteFontFace *fontface;
        WCHAR nameW[256], str[1];
        IDWriteFont *font;
        UINT32 codepoint;
        UINT16 glyph;
        BOOL exists;

        format = NULL;
        layout = NULL;

        hr = IDWriteFontCollection_GetFontFamily(syscollection, i, &family);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, &font);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFont_CreateFontFace(font, &fontface);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteFontFamily_GetFamilyNames(family, &names);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (!(exists = get_enus_string(names, nameW, ARRAY_SIZE(nameW)))) {
            IDWriteLocalFontFileLoader *localloader;
            IDWriteFontFileLoader *loader;
            IDWriteFontFile *file;
            const void *key;
            UINT32 keysize;
            UINT32 count;

            count = 1;
            hr = IDWriteFontFace_GetFiles(fontface, &count, &file);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IDWriteFontFile_GetLoader(file, &loader);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IDWriteFontFileLoader_QueryInterface(loader, &IID_IDWriteLocalFontFileLoader, (void**)&localloader);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            IDWriteFontFileLoader_Release(loader);

            hr = IDWriteFontFile_GetReferenceKey(file, &key, &keysize);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IDWriteLocalFontFileLoader_GetFilePathFromKey(localloader, key, keysize, nameW, ARRAY_SIZE(nameW));
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            skip("Failed to get English family name, font file %s\n", wine_dbgstr_w(nameW));

            IDWriteLocalFontFileLoader_Release(localloader);
            IDWriteFontFile_Release(file);
        }

        IDWriteLocalizedStrings_Release(names);
        IDWriteFont_Release(font);

        if (!exists)
            goto cleanup;

        IDWriteFontFace_GetMetrics(fontface, &fontmetrics);
        hr = IDWriteFactory_CreateTextFormat(factory, nameW, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, fontmetrics.designUnitsPerEm, L"en-us", &format);
        ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

        /* Look for first supported character to avoid triggering fallback path. With fallback it's harder to test
           DrawUnderline() metrics, because actual resolved fontface is not passed to it. Grabbing fontface instance
           from corresponding DrawGlyphRun() call is not straightforward. */
        for (codepoint = ' '; codepoint < 0xffff; ++codepoint)
        {
            glyph = 0;
            hr = IDWriteFontFace_GetGlyphIndices(fontface, &codepoint, 1, &glyph);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (glyph)
                break;
        }

        if (!glyph)
        {
            skip("Couldn't find reasonable test string.\n");
            goto cleanup;
        }

        str[0] = codepoint;
        hr = IDWriteFactory_CreateTextLayout(factory, str, 1, format, 30000.0f, 100.0f, &layout);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        range.startPosition = 0;
        range.length = 2;
        hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        memset(&ctxt, 0, sizeof(ctxt));
        ctxt.format = format;
        ctxt.familyW = nameW;
        hr = IDWriteTextLayout_Draw(layout, &ctxt, &testrenderer, 0.0f, 0.0f);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    cleanup:
        if (layout)
            IDWriteTextLayout_Release(layout);
        if (format)
            IDWriteTextFormat_Release(format);
        IDWriteFontFace_Release(fontface);
        IDWriteFontFamily_Release(family);
    }
    IDWriteFontCollection_Release(syscollection);

    IDWriteFactory_Release(factory);
}

static void test_InvalidateLayout(void)
{
    IDWriteTextLayout3 *layout3;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout3, (void**)&layout3);
    if (hr == S_OK) {
        IDWriteTextFormat1 *format1;
        IDWriteTextFormat2 *format2;

        hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat2, (void**)&format2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IDWriteTextFormat2_Release(format2);

        hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat2, (void**)&format2);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Unexpected hr %#lx.\n", hr);
        if (hr == S_OK) {
            ok(format != (IDWriteTextFormat *)format2, "Unexpected interface pointer.\n");
            IDWriteTextFormat2_Release(format2);
        }

        hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextFormat1, (void**)&format1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextFormat1_QueryInterface(format1, &IID_IDWriteTextFormat2, (void**)&format2);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
            IDWriteTextFormat2_Release(format2);
        IDWriteTextFormat1_Release(format1);

        hr = IDWriteTextLayout3_QueryInterface(layout3, &IID_IDWriteTextFormat2, (void**)&format2);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
            IDWriteTextFormat2_Release(format2);

        hr = IDWriteTextLayout3_InvalidateLayout(layout3);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IDWriteTextLayout3_Release(layout3);
    }
    else
        win_skip("IDWriteTextLayout3::InvalidateLayout() is not supported.\n");

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_line_spacing(void)
{
    IDWriteTextFormat2 *format2;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0f, 0.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0f, -10.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_DEFAULT, -10.0f, 0.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetLineSpacing(format, DWRITE_LINE_SPACING_METHOD_PROPORTIONAL+1, 0.0f, 0.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetLineSpacing(layout, DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0f, 0.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetLineSpacing(layout, DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0f, -10.0f);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetLineSpacing(layout, DWRITE_LINE_SPACING_METHOD_DEFAULT, -10.0f, 0.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetLineSpacing(layout, DWRITE_LINE_SPACING_METHOD_PROPORTIONAL+1, 0.0f, 0.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    if (IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat2, (void**)&format2) == S_OK) {
        DWRITE_LINE_SPACING spacing;

        hr = IDWriteTextFormat2_GetLineSpacing(format2, &spacing);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ok(spacing.method == DWRITE_LINE_SPACING_METHOD_DEFAULT, "got method %d\n", spacing.method);
        ok(spacing.height == 0.0f, "got %f\n", spacing.height);
        ok(spacing.baseline == -10.0f, "got %f\n", spacing.baseline);
        ok(spacing.leadingBefore == 0.0f, "got %f\n", spacing.leadingBefore);
        ok(spacing.fontLineGapUsage == DWRITE_FONT_LINE_GAP_USAGE_DEFAULT, "got %f\n", spacing.leadingBefore);

        spacing.leadingBefore = -1.0f;

        hr = IDWriteTextFormat2_SetLineSpacing(format2, &spacing);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        spacing.leadingBefore = 1.1f;

        hr = IDWriteTextFormat2_SetLineSpacing(format2, &spacing);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        spacing.leadingBefore = 1.0f;

        hr = IDWriteTextFormat2_SetLineSpacing(format2, &spacing);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        spacing.method = DWRITE_LINE_SPACING_METHOD_PROPORTIONAL + 1;
        hr = IDWriteTextFormat2_SetLineSpacing(format2, &spacing);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        spacing.method = DWRITE_LINE_SPACING_METHOD_PROPORTIONAL;
        spacing.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_ENABLED + 1;
        hr = IDWriteTextFormat2_SetLineSpacing(format2, &spacing);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDWriteTextFormat2_GetLineSpacing(format2, &spacing);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(spacing.fontLineGapUsage == DWRITE_FONT_LINE_GAP_USAGE_ENABLED + 1, "got %d\n", spacing.fontLineGapUsage);

        IDWriteTextFormat2_Release(format2);
    }

    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_GetOverhangMetrics(void)
{
    static const struct overhangs_test
    {
        FLOAT uniform_baseline;
        DWRITE_INLINE_OBJECT_METRICS metrics;
        DWRITE_OVERHANG_METRICS overhang_metrics;
        DWRITE_OVERHANG_METRICS expected;
    } overhangs_tests[] = {
        { 16.0f, { 10.0f, 50.0f, 20.0f }, { 1.0f, 2.0f, 3.0f, 4.0f }, { 1.0f, 6.0f, 3.0f, 0.0f } },
        { 15.0f, { 10.0f, 50.0f, 20.0f }, { 1.0f, 2.0f, 3.0f, 4.0f }, { 1.0f, 7.0f, 3.0f, -1.0f } },
        { 16.0f, { 10.0f, 50.0f, 20.0f }, { -1.0f, 0.0f, -3.0f, 4.0f }, { -1.0f, 4.0f, -3.0f, 0.0f } },
        { 15.0f, { 10.0f, 50.0f, 20.0f }, { -1.0f, 10.0f, 3.0f, -4.0f }, { -1.0f, 15.0f, 3.0f, -9.0f } },
    };
    struct test_inline_obj *inline_obj;
    IDWriteFactory *factory;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    HRESULT hr;
    UINT32 i;

    inline_obj = create_test_inline_object(&testinlineobjvtbl3);
    ok(!!inline_obj, "Failed to create test inline object.\n");

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 100.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"A", 1, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(overhangs_tests); i++) {
        const struct overhangs_test *test = &overhangs_tests[i];
        DWRITE_OVERHANG_METRICS overhang_metrics;
        DWRITE_TEXT_RANGE range = { 0, 1 };
        DWRITE_TEXT_METRICS metrics;

        inline_obj->metrics = test->metrics;
        inline_obj->overhangs = test->overhang_metrics;

        hr = IDWriteTextLayout_SetLineSpacing(layout, DWRITE_LINE_SPACING_METHOD_UNIFORM, test->metrics.height * 2.0f,
                test->uniform_baseline);
        ok(hr == S_OK, "Failed to set line spacing, hr %#lx.\n", hr);

        hr = IDWriteTextLayout_SetInlineObject(layout, NULL, range);
        ok(hr == S_OK, "Failed to reset inline object, hr %#lx.\n", hr);

        hr = IDWriteTextLayout_SetInlineObject(layout, &inline_obj->IDWriteInlineObject_iface, range);
        ok(hr == S_OK, "Failed to set inline object, hr %#lx.\n", hr);

        hr = IDWriteTextLayout_GetMetrics(layout, &metrics);
        ok(hr == S_OK, "Failed to get layout metrics, hr %#lx.\n", hr);

        ok(metrics.width == test->metrics.width, "%u: unexpected formatted width.\n", i);
        ok(metrics.height == test->metrics.height * 2.0f, "%u: unexpected formatted height.\n", i);

        hr = IDWriteTextLayout_SetMaxWidth(layout, metrics.width);
        hr = IDWriteTextLayout_SetMaxHeight(layout, test->metrics.height);

        hr = IDWriteTextLayout_GetOverhangMetrics(layout, &overhang_metrics);
        ok(hr == S_OK, "Failed to get overhang metrics, hr %#lx.\n", hr);

        ok(!memcmp(&overhang_metrics, &test->expected, sizeof(overhang_metrics)),
                "%u: unexpected overhang metrics (%f, %f, %f, %f).\n", i, overhang_metrics.left, overhang_metrics.top,
                overhang_metrics.right, overhang_metrics.bottom);
    }

    IDWriteInlineObject_Release(&inline_obj->IDWriteInlineObject_iface);
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_tab_stops(void)
{
    DWRITE_CLUSTER_METRICS clusters[4];
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    DWRITE_TEXT_RANGE range;
    FLOAT tabstop, size;
    UINT count;
    HRESULT hr;

    factory = create_factory();

    /* Default tab stop value. */
    for (size = 1.0f; size < 25.0f; size += 5.0f)
    {
        hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &format);
        ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

        tabstop = IDWriteTextFormat_GetIncrementalTabStop(format);
        ok(tabstop == 4.0f * size, "Unexpected tab stop %f.\n", tabstop);

        IDWriteTextFormat_Release(format);
    }

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetIncrementalTabStop(format, 0.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_SetIncrementalTabStop(format, -10.0f);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    tabstop = IDWriteTextFormat_GetIncrementalTabStop(format);
    ok(tabstop == 40.0f, "Unexpected tab stop %f.\n", tabstop);

    hr = IDWriteTextFormat_SetIncrementalTabStop(format, 100.0f);
    ok(hr == S_OK, "Failed to set tab stop value, hr %#lx.\n", hr);

    tabstop = IDWriteTextFormat_GetIncrementalTabStop(format);
    ok(tabstop == 100.0f, "Unexpected tab stop %f.\n", tabstop);

    hr = IDWriteFactory_CreateTextLayout(factory, L"\ta\tb", 4, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %lx.\n", hr);

    hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, 4, &count);
    ok(hr == S_OK, "Failed to get cluster metrics, hr %#lx.\n", hr);
    ok(clusters[0].isWhitespace, "Unexpected isWhitespace.\n");
    ok(!clusters[1].isWhitespace, "Unexpected isWhitespace.\n");
    ok(clusters[2].isWhitespace, "Unexpected isWhitespace.\n");
    ok(!clusters[3].isWhitespace, "Unexpected isWhitespace.\n");
todo_wine {
    ok(clusters[0].width == tabstop, "Unexpected tab width.\n");
    ok(clusters[1].width + clusters[2].width == tabstop, "Unexpected tab width.\n");
}
    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontSize(layout, 20.0f, range);
    ok(hr == S_OK, "Failed to set font size, hr %#lx.\n", hr);

    tabstop = IDWriteTextLayout_GetIncrementalTabStop(layout);
    ok(tabstop == 100.0f, "Unexpected tab stop %f.\n", tabstop);

    hr = IDWriteTextLayout_GetClusterMetrics(layout, clusters, 4, &count);
    ok(hr == S_OK, "Failed to get cluster metrics, hr %#lx.\n", hr);
    ok(clusters[0].isWhitespace, "Unexpected isWhitespace.\n");
    ok(!clusters[1].isWhitespace, "Unexpected isWhitespace.\n");
    ok(clusters[2].isWhitespace, "Unexpected isWhitespace.\n");
    ok(!clusters[3].isWhitespace, "Unexpected isWhitespace.\n");
todo_wine {
    ok(clusters[0].width == tabstop, "Unexpected tab width.\n");
    ok(clusters[1].width + clusters[2].width == tabstop, "Unexpected tab width.\n");
}
    IDWriteTextLayout_Release(layout);

    IDWriteTextFormat_Release(format);

    IDWriteFactory_Release(factory);
}

static void test_automatic_font_axes(void)
{
    DWRITE_AUTOMATIC_FONT_AXES axes;
    IDWriteTextLayout4 *layout4 = NULL;
    IDWriteTextFormat3 *format3;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %lx.\n", hr);

    IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout4, (void **)&layout4);

    IDWriteTextLayout_Release(layout);

    if (!layout4)
    {
        win_skip("Text layout does not support variable fonts.\n");
        IDWriteFactory_Release(factory);
        IDWriteTextFormat_Release(format);
        return;
    }

    hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat3, (void **)&format3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    axes = IDWriteTextFormat3_GetAutomaticFontAxes(format3);
    ok(axes == DWRITE_AUTOMATIC_FONT_AXES_NONE, "Unexpected automatic axes %u.\n", axes);

    hr = IDWriteTextFormat3_SetAutomaticFontAxes(format3, DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat3_SetAutomaticFontAxes(format3, DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE + 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteTextFormat3_Release(format3);

    axes = IDWriteTextLayout4_GetAutomaticFontAxes(layout4);
    ok(axes == DWRITE_AUTOMATIC_FONT_AXES_NONE, "Unexpected automatic axes %u.\n", axes);

    hr = IDWriteTextLayout4_SetAutomaticFontAxes(layout4, DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE + 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout4_SetAutomaticFontAxes(layout4, DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteTextLayout4_Release(layout4);

    /* Out of range values allow for formats, but not for layouts. */
    hr = IDWriteFactory_CreateTextLayout(factory, L"a", 1, format, 1000.0f, 1000.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %lx.\n", hr);

    hr = IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout4, (void **)&layout4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    axes = IDWriteTextLayout4_GetAutomaticFontAxes(layout4);
    ok(axes == DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE + 1, "Unexpected automatic axes %u.\n", axes);

    hr = IDWriteTextLayout4_QueryInterface(layout4, &IID_IDWriteTextFormat3, (void **)&format3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    axes = IDWriteTextFormat3_GetAutomaticFontAxes(format3);
    ok(axes == DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE + 1, "Unexpected automatic axes %u.\n", axes);

    hr = IDWriteTextLayout4_SetAutomaticFontAxes(layout4, DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    axes = IDWriteTextFormat3_GetAutomaticFontAxes(format3);
    ok(axes == DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE, "Unexpected automatic axes %u.\n", axes);

    hr = IDWriteTextFormat3_SetAutomaticFontAxes(format3, DWRITE_AUTOMATIC_FONT_AXES_OPTICAL_SIZE + 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IDWriteTextFormat3_Release(format3);

    IDWriteTextLayout_Release(layout);

    IDWriteTextLayout4_Release(layout4);
    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_text_format_axes(void)
{
    IDWriteFontCollection *collection;
    IDWriteFontCollection2 *collection2;
    DWRITE_FONT_AXIS_VALUE axes[2];
    IDWriteTextFormat3 *format3;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_WEIGHT weight;
    IDWriteTextFormat *format;
    IDWriteFactory6 *factory;
    DWRITE_FONT_STYLE style;
    DWRITE_FONT_FAMILY_MODEL model;
    unsigned int count;
    HRESULT hr;

    factory = create_factory_iid(&IID_IDWriteFactory6);

    if (!factory)
    {
        win_skip("Text format does not support variations.\n");
        return;
    }

    hr = IDWriteFactory6_CreateTextFormat(factory, L"test_family", NULL, NULL, 0, 10.0f, L"en-us", &format3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat3_GetFontCollection(format3, &collection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFontCollection_QueryInterface(collection, &IID_IDWriteFontCollection2, (void **)&collection2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    model = IDWriteFontCollection2_GetFontFamilyModel(collection2);
    ok(model == DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC, "Unexpected model %d.\n", model);

    IDWriteFontCollection_Release(collection);
    IDWriteFontCollection2_Release(collection2);

    count = IDWriteTextFormat3_GetFontAxisValueCount(format3);
    ok(!count, "Unexpected axis count %u.\n", count);

    stretch = IDWriteTextFormat3_GetFontStretch(format3);
    ok(stretch == DWRITE_FONT_STRETCH_NORMAL, "Unexpected font stretch %d.\n", stretch);

    style = IDWriteTextFormat3_GetFontStyle(format3);
    ok(style == DWRITE_FONT_STYLE_NORMAL, "Unexpected font style %d.\n", style);

    weight = IDWriteTextFormat3_GetFontWeight(format3);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL, "Unexpected font weight %d.\n", weight);

    /* Regular properties are not set from axis values. */
    axes[0].axisTag = DWRITE_FONT_AXIS_TAG_WEIGHT;
    axes[0].value = 200.0f;
    hr = IDWriteTextFormat3_SetFontAxisValues(format3, axes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    weight = IDWriteTextFormat3_GetFontWeight(format3);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL, "Unexpected font weight %d.\n", weight);

    IDWriteTextFormat3_Release(format3);

    hr = IDWriteFactory_CreateTextFormat((IDWriteFactory *)factory, L"test_family", NULL,
            DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_ITALIC, DWRITE_FONT_STRETCH_EXPANDED,
            10.0f, L"en-us", &format);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat_QueryInterface(format, &IID_IDWriteTextFormat3, (void **)&format3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IDWriteTextFormat3_GetFontAxisValueCount(format3);
    ok(!count, "Unexpected axis count %u.\n", count);

    axes[0].axisTag = DWRITE_FONT_AXIS_TAG_WEIGHT;
    hr = IDWriteTextFormat3_GetFontAxisValues(format3, axes, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(axes[0].axisTag == 0 && axes[0].value == 0.0f, "Unexpected value.\n");

    axes[0].axisTag = DWRITE_FONT_AXIS_TAG_WEIGHT;
    axes[0].value = 200.0f;
    axes[1].axisTag = DWRITE_FONT_AXIS_TAG_WIDTH;
    axes[1].value = 2.0f;
    hr = IDWriteTextFormat3_SetFontAxisValues(format3, axes, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IDWriteTextFormat3_GetFontAxisValueCount(format3);
    ok(count == 2, "Unexpected axis count %u.\n", count);

    hr = IDWriteTextFormat3_GetFontAxisValues(format3, axes, 1);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat3_GetFontAxisValues(format3, axes, 0);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat3_GetFontAxisValues(format3, axes, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextFormat3_SetFontAxisValues(format3, axes, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = IDWriteTextFormat3_GetFontAxisValueCount(format3);
    ok(!count, "Unexpected axis count %u.\n", count);

    IDWriteTextFormat3_Release(format3);
    IDWriteTextFormat_Release(format);

    IDWriteFactory6_Release(factory);
}

static void test_layout_range_length(void)
{
    IDWriteFontCollection *collection, *collection2;
    IDWriteInlineObject *sign, *object;
    IDWriteTypography *typography;
    DWRITE_FONT_STRETCH stretch;
    IDWriteTextLayout1 *layout1;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    HRESULT hr;
    BOOL value;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"ru", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    /* Range length is validated when setting properties. */

    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    /* Weight */
    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontWeight(layout, DWRITE_FONT_WEIGHT_NORMAL, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetFontWeight(layout, DWRITE_FONT_WEIGHT_NORMAL, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetFontWeight(layout, DWRITE_FONT_WEIGHT_HEAVY, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontWeight(layout, 0, &weight, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 0 && range.length == 10, "Unexpected range (%u, %u).\n", range.startPosition, range.length);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontWeight(layout, 10, &weight, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 10 && range.length == ~0u - 10, "Unexpected range (%u, %u).\n",
            range.startPosition, range.length);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontWeight(layout, DWRITE_FONT_WEIGHT_NORMAL, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Family name */
    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, L"family", range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, L"family", range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, L"family", range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontFamilyName(layout, L"Tahoma", range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Style */
    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_ITALIC, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_ITALIC, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_ITALIC, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontStyle(layout, 0, &style, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 0 && range.length == 10, "Unexpected range (%u, %u).\n", range.startPosition, range.length);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontStyle(layout, 10, &style, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 10 && range.length == ~0u - 10, "Unexpected range (%u, %u).\n",
            range.startPosition, range.length);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontStyle(layout, DWRITE_FONT_STYLE_NORMAL, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Stretch */
    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_CONDENSED, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_CONDENSED, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_CONDENSED, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontStretch(layout, 0, &stretch, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 0 && range.length == 10, "Unexpected range (%u, %u).\n", range.startPosition, range.length);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetFontStretch(layout, 10, &stretch, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 10 && range.length == ~0u - 10, "Unexpected range (%u, %u).\n",
            range.startPosition, range.length);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontStretch(layout, DWRITE_FONT_STRETCH_NORMAL, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Underline */
    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetUnderline(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetUnderline(layout, 0, &value, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 0 && range.length == 10, "Unexpected range (%u, %u).\n", range.startPosition, range.length);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetUnderline(layout, 10, &value, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 10 && range.length == ~0u - 10, "Unexpected range (%u, %u).\n",
            range.startPosition, range.length);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetUnderline(layout, FALSE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Strikethrough */
    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetStrikethrough(layout, TRUE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 0, &value, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 0 && range.length == 10, "Unexpected range (%u, %u).\n", range.startPosition, range.length);

    range.startPosition = range.length = 0;
    hr = IDWriteTextLayout_GetStrikethrough(layout, 10, &value, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 10 && range.length == ~0u - 10, "Unexpected range (%u, %u).\n",
            range.startPosition, range.length);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetStrikethrough(layout, FALSE, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Locale name */
    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetLocaleName(layout, L"locale", range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetLocaleName(layout, L"locale", range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetLocaleName(layout, L"locale", range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetLocaleName(layout, L"ru", range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Inline object */
    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &sign);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetInlineObject(layout, sign, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetInlineObject(layout, sign, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetInlineObject(layout, sign, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = range.length = 0;
    object = NULL;
    hr = IDWriteTextLayout_GetInlineObject(layout, 10, &object, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.startPosition == 10 && range.length == ~0u - 10, "Unexpected range (%u, %u).\n",
            range.startPosition, range.length);
    IDWriteInlineObject_Release(object);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetInlineObject(layout, NULL, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Drawing effect */
    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, (IUnknown *)sign, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, (IUnknown *)sign, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, (IUnknown *)sign, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetDrawingEffect(layout, NULL, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteInlineObject_Release(sign);

    /* Typography */
    hr = IDWriteFactory_CreateTypography(factory, &typography);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetTypography(layout, typography, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetTypography(layout, typography, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetTypography(layout, typography, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetTypography(layout, NULL, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteTypography_Release(typography);

    /* Font collection */
    hr = IDWriteFactory_GetSystemFontCollection(factory, &collection, FALSE);
    ok(hr == S_OK, "Failed to get system collection, hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontCollection(layout, collection, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 9;
    hr = IDWriteTextLayout_SetFontCollection(layout, collection, range);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 10;
    range.length = ~0u - 10;
    hr = IDWriteTextLayout_SetFontCollection(layout, collection, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = 0;
    range.length = ~0u;
    hr = IDWriteTextLayout_SetFontCollection(layout, NULL, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    range.startPosition = range.length = 0;
    collection2 = NULL;
    hr = IDWriteTextLayout_GetFontCollection(layout, 10, &collection2, &range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(range.length == ~0u, "Unexpected range length %u.\n", range.length);
    if (collection2)
        IDWriteFontCollection_Release(collection2);

    IDWriteFontCollection_Release(collection);

    if (SUCCEEDED(IDWriteTextLayout_QueryInterface(layout, &IID_IDWriteTextLayout1, (void **)&layout1)))
    {
        /* Pair kerning */
        range.startPosition = 10;
        range.length = ~0u;
        hr = IDWriteTextLayout1_SetPairKerning(layout1, TRUE, range);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        range.startPosition = 10;
        range.length = ~0u - 9;
        hr = IDWriteTextLayout1_SetPairKerning(layout1, TRUE, range);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        range.startPosition = 10;
        range.length = ~0u - 10;
        hr = IDWriteTextLayout1_SetPairKerning(layout1, TRUE, range);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        range.startPosition = range.length = 0;
        hr = IDWriteTextLayout1_GetPairKerning(layout1, 0, &value, &range);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(range.startPosition == 0 && range.length == 10, "Unexpected range (%u, %u).\n", range.startPosition, range.length);

        range.startPosition = range.length = 0;
        value = FALSE;
        hr = IDWriteTextLayout1_GetPairKerning(layout1, 10, &value, &range);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(range.startPosition == 10 && range.length == ~0u - 10, "Unexpected range (%u, %u).\n",
                range.startPosition, range.length);
        ok(!!value, "Unexpected value %d.\n", value);

        range.startPosition = 0;
        range.length = ~0u;
        hr = IDWriteTextLayout1_SetPairKerning(layout1, FALSE, range);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IDWriteTextLayout1_Release(layout1);
    }

    IDWriteTextLayout_Release(layout);

    IDWriteTextFormat_Release(format);
    IDWriteFactory_Release(factory);
}

static void test_HitTestTextRange(void)
{
    DWRITE_HIT_TEST_METRICS metrics[10];
    IDWriteInlineObject *inlineobj;
    DWRITE_LINE_METRICS line;
    IDWriteTextFormat *format;
    IDWriteTextLayout *layout;
    DWRITE_TEXT_RANGE range;
    IDWriteFactory *factory;
    unsigned int count;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateTextFormat(factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"ru", &format);
    ok(hr == S_OK, "Failed to create text format, hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(factory, L"string", 6, format, 100.0f, 100.0f, &layout);
    ok(hr == S_OK, "Failed to create text layout, hr %#lx.\n", hr);

    /* Start index exceeding layout text length, dummy range returned. */
    count = 0;
    hr = IDWriteTextLayout_HitTestTextRange(layout, 7, 10, 0.0f, 0.0f, metrics, ARRAY_SIZE(metrics), &count);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
if (SUCCEEDED(hr))
{
    ok(count == 1, "Unexpected metrics count %u.\n", count);
    ok(metrics[0].textPosition == 6 && metrics[0].length == 0, "Unexpected metrics range %u, %u.\n",
            metrics[0].textPosition, metrics[0].length);
    ok(!!metrics[0].isText, "Expected text range.\n");
}
    /* Length exceeding layout text length, trimmed. */
    count = 0;
    hr = IDWriteTextLayout_HitTestTextRange(layout, 0, 10, 0.0f, 0.0f, metrics, ARRAY_SIZE(metrics), &count);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
if (SUCCEEDED(hr))
{
    ok(count == 1, "Unexpected metrics count %u.\n", count);
    ok(metrics[0].textPosition == 0 && metrics[0].length == 6, "Unexpected metrics range %u, %u.\n",
            metrics[0].textPosition, metrics[0].length);
    ok(!!metrics[0].isText, "Expected text range.\n");
}
    /* Change font size for second half. */
    range.startPosition = 3;
    range.length = 3;
    hr = IDWriteTextLayout_SetFontSize(layout, 20.0f, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_HitTestTextRange(layout, 0, 6, 0.0f, 0.0f, metrics, ARRAY_SIZE(metrics), &count);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
if (SUCCEEDED(hr))
{
    ok(count == 1, "Unexpected metrics count %u.\n", count);
    ok(metrics[0].textPosition == 0 && metrics[0].length == 6, "Unexpected metrics range %u, %u.\n",
            metrics[0].textPosition, metrics[0].length);
    ok(!!metrics[0].isText, "Expected text range.\n");

    hr = IDWriteTextLayout_GetLineMetrics(layout, &line, 1, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(line.height == metrics[0].height, "Unexpected range height.\n");
}
    /* With inline object. */
    hr = IDWriteFactory_CreateEllipsisTrimmingSign(factory, format, &inlineobj);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteTextLayout_SetInlineObject(layout, inlineobj, range);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    count = 0;
    hr = IDWriteTextLayout_HitTestTextRange(layout, 0, 6, 0.0f, 0.0f, metrics, ARRAY_SIZE(metrics), &count);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
if (SUCCEEDED(hr))
{
    ok(count == 2, "Unexpected metrics count %u.\n", count);
    ok(metrics[0].textPosition == 0 && metrics[0].length == 3, "Unexpected metrics range %u, %u.\n",
            metrics[0].textPosition, metrics[0].length);
    ok(!!metrics[0].isText, "Expected text range.\n");
    ok(metrics[1].textPosition == 3 && metrics[1].length == 3, "Unexpected metrics range %u, %u.\n",
            metrics[1].textPosition, metrics[1].length);
    ok(!metrics[1].isText, "Unexpected text range.\n");
}
    count = 0;
    hr = IDWriteTextLayout_HitTestTextRange(layout, 7, 10, 0.0f, 0.0f, metrics, ARRAY_SIZE(metrics), &count);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
if (SUCCEEDED(hr))
{
    ok(count == 1, "Unexpected metrics count %u.\n", count);
    ok(metrics[0].textPosition == 6 && metrics[0].length == 0, "Unexpected metrics range %u, %u.\n",
            metrics[0].textPosition, metrics[0].length);
    ok(!metrics[0].isText, "Unexpected text range.\n");
}
    IDWriteInlineObject_Release(inlineobj);
    IDWriteTextLayout_Release(layout);
    IDWriteTextFormat_Release(format);

    IDWriteFactory_Release(factory);
}

START_TEST(layout)
{
    IDWriteFactory *factory;

    if (!(factory = create_factory())) {
        win_skip("failed to create factory\n");
        return;
    }

    init_call_sequences(sequences, NUM_CALL_SEQUENCES);
    init_call_sequences(expected_seq, 1);

    test_CreateTextLayout();
    test_CreateGdiCompatibleTextLayout();
    test_CreateTextFormat();
    test_GetLocaleName();
    test_CreateEllipsisTrimmingSign();
    test_fontweight();
    test_SetInlineObject();
    test_Draw();
    test_typography();
    test_GetClusterMetrics();
    test_SetLocaleName();
    test_SetPairKerning();
    test_SetVerticalGlyphOrientation();
    test_fallback();
    test_DetermineMinWidth();
    test_SetFontSize();
    test_SetFontFamilyName();
    test_SetFontStyle();
    test_SetFontStretch();
    test_SetStrikethrough();
    test_GetMetrics();
    test_SetFlowDirection();
    test_SetDrawingEffect();
    test_GetLineMetrics();
    test_SetTextAlignment();
    test_SetParagraphAlignment();
    test_SetReadingDirection();
    test_pixelsnapping();
    test_SetWordWrapping();
    test_MapCharacters();
    test_system_fallback();
    test_FontFallbackBuilder();
    test_SetTypography();
    test_SetLastLineWrapping();
    test_SetOpticalAlignment();
    test_SetUnderline();
    test_InvalidateLayout();
    test_line_spacing();
    test_GetOverhangMetrics();
    test_tab_stops();
    test_automatic_font_axes();
    test_text_format_axes();
    test_layout_range_length();
    test_HitTestTextRange();

    IDWriteFactory_Release(factory);
}
