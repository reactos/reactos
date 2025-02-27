/*
 *    Text analyzing tests
 *
 * Copyright 2012-2020 Nikolay Sivov for CodeWeavers
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
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "initguid.h"
#include "windows.h"
#include "winternl.h"
#include "dwrite_3.h"
#include "usp10.h"

#include "wine/test.h"

static IDWriteFactory *factory;

static void * create_text_analyzer(REFIID riid)
{
    IDWriteTextAnalyzer *analyzer;
    void *ret = NULL;

    if (SUCCEEDED(IDWriteFactory_CreateTextAnalyzer(factory, &analyzer)))
    {
        IDWriteTextAnalyzer_QueryInterface(analyzer, riid, &ret);
        IDWriteTextAnalyzer_Release(analyzer);
    }

    return ret;
}

#define LRE 0x202a
#define RLE 0x202b
#define PDF 0x202c
#define LRO 0x202d
#define RLO 0x202e
#define LRI 0x2066
#define RLI 0x2067
#define FSI 0x2068
#define PDI 0x2069

#define MS_GDEF_TAG DWRITE_MAKE_OPENTYPE_TAG('G','D','E','F')

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#define GET_LE_WORD(x) RtlUshortByteSwap(x)
#define GET_LE_DWORD(x) RtlUlongByteSwap(x)
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#define GET_LE_WORD(x) (x)
#define GET_LE_DWORD(x) (x)
#endif

struct ot_gdef_classdef_format1
{
    WORD format;
    WORD start_glyph;
    WORD glyph_count;
    WORD classes[1];
};

struct ot_gdef_class_range
{
    WORD start_glyph;
    WORD end_glyph;
    WORD glyph_class;
};

struct ot_gdef_classdef_format2
{
    WORD format;
    WORD range_count;
    struct ot_gdef_class_range ranges[1];
};

enum analysis_kind {
    ScriptAnalysis,
    LastKind
};

static const char *get_analysis_kind_name(enum analysis_kind kind)
{
    switch (kind)
    {
    case ScriptAnalysis:
        return "ScriptAnalysis";
    default:
        return "unknown";
    }
}

struct script_analysis {
    UINT32 pos;
    UINT32 len;
    DWRITE_SCRIPT_SHAPES shapes;
};

struct call_entry {
    enum analysis_kind kind;
    struct script_analysis sa;
};

struct testcontext {
    enum analysis_kind kind;
    BOOL todo;
    int *failcount;
    const char *file;
    int line;
};

struct call_sequence
{
    int count;
    int size;
    struct call_entry *sequence;
};

#define NUM_CALL_SEQUENCES    1
#define ANALYZER_ID 0
static struct call_sequence *sequences[NUM_CALL_SEQUENCES];
static struct call_sequence *expected_seq[1];

static void add_call(struct call_sequence **seq, int sequence_index, const struct call_entry *call)
{
    struct call_sequence *call_seq = seq[sequence_index];

    if (!call_seq->sequence)
    {
        call_seq->size = 10;
        call_seq->sequence = malloc(call_seq->size * sizeof(*call_seq->sequence));
    }

    if (call_seq->count == call_seq->size)
    {
        call_seq->size *= 2;
        call_seq->sequence = realloc(call_seq->sequence, call_seq->size * sizeof(*call_seq->sequence));
    }

    assert(call_seq->sequence);

    call_seq->sequence[call_seq->count++] = *call;
}

static inline void flush_sequence(struct call_sequence **seg, int sequence_index)
{
    struct call_sequence *call_seq = seg[sequence_index];

    free(call_seq->sequence);
    call_seq->sequence = NULL;
    call_seq->count = call_seq->size = 0;
}

static void init_call_sequences(struct call_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        seq[i] = calloc(1, sizeof(*seq[i]));
}

static void test_uint(UINT32 actual, UINT32 expected, const char *name, const struct testcontext *ctxt)
{
    if (expected != actual && ctxt->todo)
    {
        (*ctxt->failcount)++;
        ok_(ctxt->file, ctxt->line) (0, "%s: \"%s\" expecting %u, got %u\n", get_analysis_kind_name(ctxt->kind), name, expected, actual);
    }
    else
        ok_(ctxt->file, ctxt->line) (expected == actual, "%s: \"%s\" expecting %u, got %u\n", get_analysis_kind_name(ctxt->kind), name,
            expected, actual);
}

static void ok_sequence_(struct call_sequence **seq, int sequence_index,
    const struct call_entry *expected, const char *context, BOOL todo,
    const char *file, int line)
{
    struct call_sequence *call_seq = seq[sequence_index];
    static const struct call_entry end_of_sequence = { LastKind };
    const struct call_entry *actual, *sequence;
    int failcount = 0;
    struct testcontext ctxt;

    add_call(seq, sequence_index, &end_of_sequence);

    sequence = call_seq->sequence;
    actual = sequence;

    ctxt.failcount = &failcount;
    ctxt.todo = todo;
    ctxt.file = file;
    ctxt.line = line;

    while (expected->kind != LastKind && actual->kind != LastKind)
    {
        if (expected->kind == actual->kind)
        {
            ctxt.kind = expected->kind;

            switch (actual->kind)
            {
            case ScriptAnalysis:
                test_uint(actual->sa.pos, expected->sa.pos, "position", &ctxt);
                test_uint(actual->sa.len, expected->sa.len, "length", &ctxt);
                test_uint(actual->sa.shapes, expected->sa.shapes, "shapes", &ctxt);
                break;
            default:
                ok(0, "%s: callback not handled, %s\n", context, get_analysis_kind_name(actual->kind));
            }
            expected++;
            actual++;
        }
        else if (todo)
        {
            failcount++;
            todo_wine
            {
                ok_(file, line) (0, "%s: call %s was expected, but got call %s instead\n",
                    context, get_analysis_kind_name(expected->kind), get_analysis_kind_name(actual->kind));
            }

            flush_sequence(seq, sequence_index);
            return;
        }
        else
        {
            ok_(file, line) (0, "%s: call %s was expected, but got call %s instead\n",
                context, get_analysis_kind_name(expected->kind), get_analysis_kind_name(actual->kind));
            expected++;
            actual++;
        }
    }

    if (todo)
    {
        todo_wine
        {
            if (expected->kind != LastKind || actual->kind != LastKind)
            {
                failcount++;
                ok_(file, line) (0, "%s: the call sequence is not complete: expected %s - actual %s\n",
                    context, get_analysis_kind_name(expected->kind), get_analysis_kind_name(actual->kind));
            }
        }
    }
    else if (expected->kind != LastKind || actual->kind != LastKind)
    {
        ok_(file, line) (0, "%s: the call sequence is not complete: expected %s - actual %s\n",
            context, get_analysis_kind_name(expected->kind), get_analysis_kind_name(actual->kind));
    }

    if (todo && !failcount) /* succeeded yet marked todo */
    {
        todo_wine
        {
            ok_(file, line)(1, "%s: marked \"todo_wine\" but succeeds\n", context);
        }
    }

    flush_sequence(seq, sequence_index);
}

#define ok_sequence(seq, index, exp, contx, todo) \
        ok_sequence_(seq, index, (exp), (contx), (todo), __FILE__, __LINE__)

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
    struct call_entry entry;
    entry.kind = ScriptAnalysis;
    entry.sa.pos = position;
    entry.sa.len = length;
    entry.sa.shapes = sa->shapes;
    add_call(sequences, ANALYZER_ID, &entry);
    return S_OK;
}

static DWRITE_SCRIPT_ANALYSIS g_sa;
static HRESULT WINAPI analysissink_SetScriptAnalysis2(IDWriteTextAnalysisSink *iface,
    UINT32 position, UINT32 length, DWRITE_SCRIPT_ANALYSIS const* sa)
{
    g_sa = *sa;
    return S_OK;
}

#define BREAKPOINT_COUNT 20
static DWRITE_LINE_BREAKPOINT g_actual_bp[BREAKPOINT_COUNT];

static HRESULT WINAPI analysissink_SetLineBreakpoints(IDWriteTextAnalysisSink *iface,
        UINT32 position,
        UINT32 length,
        DWRITE_LINE_BREAKPOINT const* breakpoints)
{
    if (position + length > BREAKPOINT_COUNT) {
        ok(0, "SetLineBreakpoints: reported pos=%u, len=%u overflows expected length %d\n", position, length, BREAKPOINT_COUNT);
        return E_FAIL;
    }
    memcpy(&g_actual_bp[position], breakpoints, length*sizeof(DWRITE_LINE_BREAKPOINT));
    return S_OK;
}

#define BIDI_LEVELS_COUNT 10
static UINT8 g_explicit_levels[BIDI_LEVELS_COUNT];
static UINT8 g_resolved_levels[BIDI_LEVELS_COUNT];
static HRESULT WINAPI analysissink_SetBidiLevel(IDWriteTextAnalysisSink *iface,
        UINT32 position,
        UINT32 length,
        UINT8 explicitLevel,
        UINT8 resolvedLevel)
{
    if (position + length > BIDI_LEVELS_COUNT) {
        ok(0, "SetBidiLevel: reported pos=%u, len=%u overflows expected length %d\n", position, length, BIDI_LEVELS_COUNT);
        return E_FAIL;
    }
    memset(g_explicit_levels + position, explicitLevel, length);
    memset(g_resolved_levels + position, resolvedLevel, length);
    return S_OK;
}

static HRESULT WINAPI analysissink_SetNumberSubstitution(IDWriteTextAnalysisSink *iface,
        UINT32 position,
        UINT32 length,
        IDWriteNumberSubstitution* substitution)
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

static IDWriteTextAnalysisSinkVtbl analysissinkvtbl2 = {
    analysissink_QueryInterface,
    analysissink_AddRef,
    analysissink_Release,
    analysissink_SetScriptAnalysis2,
    analysissink_SetLineBreakpoints,
    analysissink_SetBidiLevel,
    analysissink_SetNumberSubstitution
};

static IDWriteTextAnalysisSink analysissink = { &analysissinkvtbl };
static IDWriteTextAnalysisSink analysissink2 = { &analysissinkvtbl2 };

static HRESULT WINAPI analysissource_QueryInterface(IDWriteTextAnalysisSource *iface,
    REFIID riid, void **obj)
{
    ok(0, "QueryInterface not expected\n");
    return E_NOTIMPL;
}

static ULONG WINAPI analysissource_AddRef(IDWriteTextAnalysisSource *iface)
{
    ok(0, "AddRef not expected\n");
    return 2;
}

static ULONG WINAPI analysissource_Release(IDWriteTextAnalysisSource *iface)
{
    ok(0, "Release not expected\n");
    return 1;
}

struct testanalysissource
{
    IDWriteTextAnalysisSource IDWriteTextAnalysisSource_iface;
    const WCHAR *text;
    UINT32 text_length;
    DWRITE_READING_DIRECTION direction;
};

static void init_textsource(struct testanalysissource *source, const WCHAR *text,
        INT text_length, DWRITE_READING_DIRECTION direction)
{
    source->text = text;
    source->text_length = text_length == -1 ? lstrlenW(text) : text_length;
    source->direction = direction;
};

static inline struct testanalysissource *impl_from_IDWriteTextAnalysisSource(IDWriteTextAnalysisSource *iface)
{
    return CONTAINING_RECORD(iface, struct testanalysissource, IDWriteTextAnalysisSource_iface);
}

static HRESULT WINAPI analysissource_GetTextAtPosition(IDWriteTextAnalysisSource *iface,
    UINT32 position, WCHAR const** text, UINT32* text_len)
{
    struct testanalysissource *source = impl_from_IDWriteTextAnalysisSource(iface);

    if (position >= source->text_length)
    {
        *text = NULL;
        *text_len = 0;
    }
    else
    {
        *text = source->text + position;
        *text_len = source->text_length - position;
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
    struct testanalysissource *source = impl_from_IDWriteTextAnalysisSource(iface);
    return source->direction;
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

static struct testanalysissource analysissource = { { &analysissourcevtbl } };

static IDWriteFontFace *create_fontface(void)
{
    IDWriteGdiInterop *interop;
    IDWriteFontFace *fontface;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, L"Tahoma");

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);

    return fontface;
}

static WCHAR *create_testfontfile(const WCHAR *filename)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, filename);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", wine_dbgstr_w(pathW),
        GetLastError());

    res = FindResourceA(GetModuleHandleA(NULL), (LPCSTR)MAKEINTRESOURCE(1), (LPCSTR)RT_RCDATA);
    ok(res != 0, "couldn't find resource\n");
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "couldn't write resource\n");
    CloseHandle(file);

    return pathW;
}

#define DELETE_FONTFILE(filename) _delete_testfontfile(filename, __LINE__)
static void _delete_testfontfile(const WCHAR *filename, int line)
{
    BOOL ret = DeleteFileW(filename);
    ok_(__FILE__,line)(ret, "failed to delete file %s, error %ld\n", wine_dbgstr_w(filename), GetLastError());
}

static IDWriteFontFace *create_testfontface(const WCHAR *filename)
{
    IDWriteFontFace *face;
    IDWriteFontFile *file;
    HRESULT hr;

    hr = IDWriteFactory_CreateFontFileReference(factory, filename, NULL, &file);
    ok(hr == S_OK, "Unexpected hr %#lx.\n",hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file, 0,
        DWRITE_FONT_SIMULATIONS_NONE, &face);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDWriteFontFile_Release(file);

    return face;
}

struct sa_test {
    const WCHAR string[50];
    int str_len;
    int item_count;
    struct script_analysis sa[10];
};

static struct sa_test sa_tests[] = {
    {
      /* just 1 char string */
      {'t',0}, -1, 1,
          { { 0, 1, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      {'t','e','s','t',0}, -1, 1,
          { { 0, 4, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      {' ',' ',' ',' ','!','$','[','^','{','~',0}, -1, 1,
          { { 0, 10, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      {' ',' ',' ','1','2',' ',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* digits only */
      {'1','2',0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Arabic */
      {0x064a,0x064f,0x0633,0x0627,0x0648,0x0650,0x064a,0x0661,0}, -1, 1,
          { { 0, 8, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Arabic */
      {0x0627,0x0644,0x0635,0x0651,0x0650,0x062d,0x0629,0x064f,' ',0x062a,0x064e,
       0x0627,0x062c,0x064c,' ',0x0639,0x064e,0x0644,0x0649,' ',
       0x0631,0x064f,0x0624,0x0648,0x0633,0x0650,' ',0x0627,0x0644,
       0x0623,0x0635,0x0650,0x062d,0x0651,0x064e,0x0627,0x0621,0x0650,0x06f0,0x06f5,0}, -1, 1,
          { { 0, 40, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Arabic, Latin */
      {'1','2','3','-','5','2',0x064a,0x064f,0x0633,0x0627,0x0648,0x0650,0x064a,'7','1','.',0}, -1, 1,
          { { 0, 16, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Arabic, English */
      {'A','B','C','-','D','E','F',' ',0x0621,0x0623,0x0624,0}, -1, 2,
          { { 0, 8, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 8, 3, DWRITE_SCRIPT_SHAPES_DEFAULT },
          }
    },
    {
      /* leading space, Arabic, English */
      {' ',0x0621,0x0623,0x0624,'A','B','C','-','D','E','F',0}, -1, 2,
          { { 0, 4, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 4, 7, DWRITE_SCRIPT_SHAPES_DEFAULT },
          }
    },
    {
      /* English, Arabic, trailing space */
      {'A','B','C','-','D','E','F',0x0621,0x0623,0x0624,' ',0}, -1, 2,
          { { 0, 7, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 7, 4, DWRITE_SCRIPT_SHAPES_DEFAULT },
          }
    },
    {
      /* C1 Controls, Latin-1 Supplement */
      {0x80,0x90,0x9f,0xa0,0xc0,0xb8,0xbf,0xc0,0xff,0}, -1, 2,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_NO_VISUAL },
            { 3, 6, DWRITE_SCRIPT_SHAPES_DEFAULT },
          }
    },
    {
      /* Latin Extended-A */
      {0x100,0x120,0x130,0x140,0x150,0x160,0x170,0x17f,0}, -1, 1,
          { { 0, 8, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Latin Extended-B */
      {0x180,0x190,0x1bf,0x1c0,0x1c3,0x1c4,0x1cc,0x1dc,0x1ff,0x217,0x21b,0x24f,0}, -1, 1,
          { { 0, 12, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* IPA Extensions */
      {0x250,0x260,0x270,0x290,0x2af,0}, -1, 1,
          { { 0, 5, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Spacing Modifier Letters */
      {0x2b0,0x2ba,0x2d7,0x2dd,0x2ef,0x2ff,0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Combining Diacritical Marks */
      {0x300,0x320,0x340,0x345,0x350,0x36f,0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Greek and Coptic */
      {0x370,0x388,0x3d8,0x3e1,0x3e2,0x3fa,0x3ff,0}, -1, 3,
          { { 0, 4, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 4, 1, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 5, 2, DWRITE_SCRIPT_SHAPES_DEFAULT }
          }
    },
    {
      /* Cyrillic and Cyrillic Supplement */
      {0x400,0x40f,0x410,0x44f,0x450,0x45f,0x460,0x481,0x48a,0x4f0,0x4fa,0x4ff,0x500,0x510,0x520,0}, -1, 1,
          { { 0, 15, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Armenian */
      {0x531,0x540,0x559,0x55f,0x570,0x589,0x58a,0}, -1, 1,
          { { 0, 7, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Hebrew */
      {0x5e9,0x5dc,0x5d5,0x5dd,0}, -1, 1,
          { { 0, 4, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Latin, Hebrew, Latin */
      {'p','a','r','t',' ','o','n','e',' ',0x5d7,0x5dc,0x5e7,' ',0x5e9,0x5ea,0x5d9,0x5d9,0x5dd,' ','p','a','r','t',' ','t','h','r','e','e',0}, -1, 3,
          { { 0, 9, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 9, 10, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 19, 10, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Syriac */
      {0x710,0x712,0x712,0x714,'.',0}, -1, 1,
          { { 0, 5, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Arabic Supplement */
      {0x750,0x760,0x76d,'.',0}, -1, 1,
          { { 0, 4, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Thaana */
      {0x780,0x78e,0x798,0x7a6,0x7b0,'.',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* N'Ko */
      {0x7c0,0x7ca,0x7e8,0x7eb,0x7f6,'.',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Thaana */
      {0x780,0x798,0x7a5,0x7a6,0x7b0,'.',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Devanagari */
      {0x926,0x947,0x935,0x928,0x93e,0x917,0x930,0x940,'.',0}, -1, 1,
          { { 0, 9, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Bengali */
      {0x9ac,0x9be,0x982,0x9b2,0x9be,'.',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Gurmukhi */
      {0xa17,0xa41,0xa30,0xa2e,0xa41,0xa16,0xa40,'.',0}, -1, 1,
          { { 0, 8, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Gujarati */
      {0xa97,0xac1,0xa9c,0xab0,0xabe,0xaa4,0xac0,'.',0}, -1, 1,
          { { 0, 8, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Oriya */
      {0xb13,0xb21,0xb3c,0xb3f,0xb06,'.',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Tamil */
      {0xba4,0xbae,0xbbf,0xbb4,0xbcd,'.',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Telugu */
      {0xc24,0xc46,0xc32,0xc41,0xc17,0xc41,'.',0}, -1, 1,
          { { 0, 7, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Kannada */
      {0xc95,0xca8,0xccd,0xca8,0xca1,'.',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Malayalam */
      {0xd2e,0xd32,0xd2f,0xd3e,0xd33,0xd02,'.',0}, -1, 1,
          { { 0, 7, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Sinhala */
      {0xd82,0xd85,0xd9a,0xdcf,'.',0}, -1, 1,
          { { 0, 5, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Thai */
      {0x0e04,0x0e27,0x0e32,0x0e21,0x0e1e,0x0e22,0x0e32,0x0e22,0x0e32,0x0e21,
       0x0e2d,0x0e22,0x0e39,0x0e48,0x0e17,0x0e35,0x0e48,0x0e44,0x0e2b,0x0e19,
       0x0e04,0x0e27,0x0e32,0x0e21,0x0e2a, 0x0e33,0x0e40,0x0e23,0x0e47,0x0e08,
       0x0e2d,0x0e22,0x0e39,0x0e48,0x0e17,0x0e35,0x0e48,0x0e19,0x0e31,0x0e48,0x0e19,'.',0}, -1, 1,
          { { 0, 42, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Lao */
      {0xead,0xeb1,0xe81,0xeaa,0xead,0xe99,0xea5,0xeb2,0xea7,'.',0}, -1, 1,
          { { 0, 10, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Tibetan */
      {0xf04,0xf05,0xf0e,0x020,0xf51,0xf7c,0xf53,0xf0b,0xf5a,0xf53,0xf0b,
       0xf51,0xf44,0xf0b,0xf54,0xf7c,0xf0d,'.',0}, -1, 1,
          { { 0, 18, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Myanmar */
      {0x1019,0x103c,0x1014,0x103a,0x1019,0x102c,0x1021,0x1000,0x1039,0x1001,0x101b,0x102c,'.',0}, -1, 1,
          { { 0, 13, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Georgian */
      {0x10a0,0x10d0,0x10da,0x10f1,0x10fb,0x2d00,'.',0}, -1, 1,
          { { 0, 7, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Hangul */
      {0x1100,0x1110,0x1160,0x1170,0x11a8,'.',0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Ethiopic */
      {0x130d,0x12d5,0x12dd,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Cherokee */
      {0x13e3,0x13b3,0x13a9,0x0020,0x13a6,0x13ec,0x13c2,0x13af,0x13cd,0x13d7,0}, -1, 1,
          { { 0, 10, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Canadian */
      {0x1403,0x14c4,0x1483,0x144e,0x1450,0x1466,0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Ogham */
      {0x169b,0x1691,0x168c,0x1690,0x168b,0x169c,0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Runic */
      {0x16a0,0x16a1,0x16a2,0x16a3,0x16a4,0x16a5,0}, -1, 1,
          { { 0, 6, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Khmer */
      {0x1781,0x17c1,0x1798,0x179a,0x1797,0x17b6,0x179f,0x17b6,0x19e0,0}, -1, 1,
          { { 0, 9, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Mongolian */
      {0x182e,0x1823,0x1829,0x182d,0x1823,0x182f,0x0020,0x182a,0x1822,0x1834,0x1822,0x182d,0x180c,0}, -1, 1,
          { { 0, 13, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Limbu */
      {0x1900,0x1910,0x1920,0x1930,0}, -1, 1,
          { { 0, 4, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Tai Le */
      {0x1956,0x196d,0x1970,0x1956,0x196c,0x1973,0x1951,0x1968,0x1952,0x1970,0}, -1, 1,
          { { 0, 10, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* New Tai Lue */
      {0x1992,0x19c4,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Buginese */
      {0x1a00,0x1a10,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Tai Tham */
      {0x1a20,0x1a40,0x1a50,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Balinese */
      {0x1b00,0x1b05,0x1b20,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Sundanese */
      {0x1b80,0x1b85,0x1ba0,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Batak */
      {0x1bc0,0x1be5,0x1bfc,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Lepcha */
      {0x1c00,0x1c20,0x1c40,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Ol Chiki */
      {0x1c50,0x1c5a,0x1c77,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Sundanese Supplement */
      {0x1cc0,0x1cc5,0x1cc7,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Phonetic Extensions */
      {0x1d00,0x1d40,0x1d70,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Combining diacritical marks */
      {0x1dc0,0x300,0x1ddf,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Latin Extended Additional, Extended-C */
      {0x1e00,0x1d00,0x2c60,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Greek Extended */
      {0x3f0,0x1f00,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* General Punctuation */
      {0x1dc0,0x2000,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Superscripts and Subscripts */
      {0x2070,0x2086,0x2000,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Currency, Combining Diacritical Marks for Symbols. Letterlike Symbols.. */
      {0x20a0,0x20b8,0x2000,0x20d0,0x2100,0x2150,0x2190,0x2200,0x2300,0x2400,0x2440,0x2460,0x2500,0x2580,0x25a0,0x2600,
       0x2700,0x27c0,0x27f0,0x2900,0x2980,0x2a00,0x2b00,0}, -1, 1,
          { { 0, 23, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Braille */
      {0x2800,0x2070,0x2000,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Glagolitic */
      {0x2c00,0x2c12,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* Coptic */
      {0x2c80,0x3e2,0x1f00,0}, -1, 2,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 2, 1, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* Tifinagh */
      {0x2d30,0x2d4a,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT }}
    },
    {
      /* LRE/PDF */
      {LRE,PDF,'a','b','c','\r',0}, -1, 3,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_NO_VISUAL },
            { 2, 3, DWRITE_SCRIPT_SHAPES_DEFAULT   },
            { 5, 1, DWRITE_SCRIPT_SHAPES_NO_VISUAL } }
    },
    {
      /* LRE/PDF and other visual and non-visual codes from Common script range */
      {LRE,PDF,'r','!',0x200b,'\r',0}, -1, 3,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_NO_VISUAL },
            { 2, 2, DWRITE_SCRIPT_SHAPES_DEFAULT   },
            { 4, 2, DWRITE_SCRIPT_SHAPES_NO_VISUAL } }
    },
    {
      /* Inherited on its own */
      {0x300,0x300,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* Inherited followed by Latin */
      {0x300,0x300,'a',0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* Inherited mixed with Arabic and Latin */
      {0x300,'+',0x627,0x300,'a',0}, -1, 2,
          { { 0, 4, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 4, 1, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      {'a',0x300,'+',0x627,0x300,')','a',0}, -1, 3,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 3, 3, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 6, 1, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    /* Paired punctuation */
    {
      {0x627,'(','a',')','a',0}, -1, 2,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 2, 3, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      {0x627,'[','a',']',0x627,0}, -1, 3,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 2, 2, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 4, 1, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    /* Combining marks */
    {
      /* dotted circle - Common, followed by accent - Inherited */
      {0x25cc,0x300,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* combining mark with explicit script value */
      {0x25cc,0x300,0x5c4,0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* inherited merges with following explicit script */
      {0x25cc,0x300,'a',0}, -1, 1,
          { { 0, 3, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* TAKRI LETTER A U+11680 */
      {0xd805,0xde80,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* Musical symbols, U+1D173 */
      {0xd834,0xdd73,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_NO_VISUAL } }
    },
    {
      /* Tags, U+E0020 */
      {0xdb40,0xdc20,0}, -1, 1,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_NO_VISUAL } }
    },
    {
      /* Null at start of string */
      L"\0test", 5, 2,
          { { 0, 1, DWRITE_SCRIPT_SHAPES_NO_VISUAL },
            { 1, 4, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* Null embedded in string */
      L"te\0st", 5, 3,
          { { 0, 2, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 2, 1, DWRITE_SCRIPT_SHAPES_NO_VISUAL },
            { 3, 2, DWRITE_SCRIPT_SHAPES_DEFAULT } }
    },
    {
      /* Null at end of string */
      L"test\0", 5, 2,
          { { 0, 4, DWRITE_SCRIPT_SHAPES_DEFAULT },
            { 4, 1, DWRITE_SCRIPT_SHAPES_NO_VISUAL } }
    },
};

static void init_expected_sa(struct call_sequence **seq, const struct sa_test *test)
{
    static const struct call_entry end_of_sequence = { LastKind };
    int i;

    flush_sequence(seq, ANALYZER_ID);

    /* add expected calls */
    for (i = 0; i < test->item_count; i++)
    {
        struct call_entry call;

        call.kind = ScriptAnalysis;
        call.sa.pos = test->sa[i].pos;
        call.sa.len = test->sa[i].len;
        call.sa.shapes = test->sa[i].shapes;
        add_call(seq, 0, &call);
    }

    /* and stop marker */
    add_call(seq, 0, &end_of_sequence);
}

static void get_script_analysis(const WCHAR *str, DWRITE_SCRIPT_ANALYSIS *sa)
{
    IDWriteTextAnalyzer *analyzer;
    HRESULT hr;

    init_textsource(&analysissource, str, -1, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer);
    ok(!!analyzer, "Failed to create analyzer instance.\n");

    hr = IDWriteTextAnalyzer_AnalyzeScript(analyzer, &analysissource.IDWriteTextAnalysisSource_iface, 0,
        lstrlenW(analysissource.text), &analysissink2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    *sa = g_sa;

    IDWriteTextAnalyzer_Release(analyzer);
}

static void test_AnalyzeScript(void)
{
    IDWriteTextAnalyzer *analyzer;
    HRESULT hr;
    UINT i;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer);
    ok(!!analyzer, "Failed to create analyzer instance.\n");

    for (i = 0; i < ARRAY_SIZE(sa_tests); i++)
    {
        init_textsource(&analysissource, sa_tests[i].string, sa_tests[i].str_len,
            DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);

        winetest_push_context("Test %s", wine_dbgstr_wn(sa_tests[i].string, sa_tests[i].str_len));

        init_expected_sa(expected_seq, &sa_tests[i]);
        hr = IDWriteTextAnalyzer_AnalyzeScript(analyzer, &analysissource.IDWriteTextAnalysisSource_iface, 0,
            sa_tests[i].str_len == -1 ? lstrlenW(sa_tests[i].string) : sa_tests[i].str_len, &analysissink);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_sequence(sequences, ANALYZER_ID, expected_seq[0]->sequence,
            wine_dbgstr_wn(sa_tests[i].string, sa_tests[i].str_len), FALSE);

        winetest_pop_context();
    }

    IDWriteTextAnalyzer_Release(analyzer);
}

struct linebreaks_test {
    const WCHAR text[BREAKPOINT_COUNT+1];
    DWRITE_LINE_BREAKPOINT bp[BREAKPOINT_COUNT];
};

static const struct linebreaks_test linebreaks_tests[] =
{
    { {'A','-','B',' ','C',0x58a,'D',0x2010,'E',0x2012,'F',0x2013,'\t',0xc,0xb,0x2028,0x2029,0x200b,0},
      {
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     1, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 1, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_MUST_BREAK,    1, 0 },
          { DWRITE_BREAK_CONDITION_MUST_BREAK,    DWRITE_BREAK_CONDITION_MUST_BREAK,    1, 0 },
          { DWRITE_BREAK_CONDITION_MUST_BREAK,    DWRITE_BREAK_CONDITION_MUST_BREAK,    1, 0 },
          { DWRITE_BREAK_CONDITION_MUST_BREAK,    DWRITE_BREAK_CONDITION_MUST_BREAK,    1, 0 },
          { DWRITE_BREAK_CONDITION_MUST_BREAK,    DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
      }
    },
    /* Soft hyphen, visible word dividers */
    { {'A',0xad,'B',0x5be,'C',0xf0b,'D',0x1361,'E',0x17d8,'F',0x17da,'G',0},
      {
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 1 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, 0, 0 },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_CAN_BREAK,     0, 0 },
      }
    },
    /* LB30 changes in Unicode 13 regarding East Asian parentheses */
    { {0x5f35,'G',0x300c,0},
      {
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK     },
          { DWRITE_BREAK_CONDITION_CAN_BREAK,     DWRITE_BREAK_CONDITION_MAY_NOT_BREAK },
          { DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, DWRITE_BREAK_CONDITION_CAN_BREAK     },
      }
    },
    { { 0 } }
};

static void compare_breakpoints(const struct linebreaks_test *test, DWRITE_LINE_BREAKPOINT *actual)
{
    static const char *conditions[] = {"N","CB","NB","B"};
    const WCHAR *text = test->text;
    int cmp = memcmp(test->bp, actual, sizeof(*actual)*BREAKPOINT_COUNT);
    ok(!cmp, "%s: got wrong breakpoint data\n", wine_dbgstr_w(test->text));
    if (cmp) {
        int i = 0;
        while (*text) {
            ok(!memcmp(&test->bp[i], &actual[i], sizeof(*actual)),
                "%s: got [%s, %s] (%s, %s), expected [%s, %s] (%s, %s)\n",
                wine_dbgstr_wn(&test->text[i], 1),
                conditions[g_actual_bp[i].breakConditionBefore],
                conditions[g_actual_bp[i].breakConditionAfter],
                g_actual_bp[i].isWhitespace ? "WS"  : "0",
                g_actual_bp[i].isSoftHyphen ? "SHY" : "0",
                conditions[test->bp[i].breakConditionBefore],
                conditions[test->bp[i].breakConditionAfter],
                test->bp[i].isWhitespace ? "WS"  : "0",
                test->bp[i].isSoftHyphen ? "SHY" : "0");
            if (g_actual_bp[i].isSoftHyphen)
                ok(!g_actual_bp[i].isWhitespace, "%s: soft hyphen marked as a whitespace\n",
                    wine_dbgstr_wn(&test->text[i], 1));
            text++;
            i++;
        }
    }
}

static void test_AnalyzeLineBreakpoints(void)
{
    const struct linebreaks_test *ptr = linebreaks_tests;
    IDWriteTextAnalyzer *analyzer;
    UINT32 i = 0;
    HRESULT hr;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer);
    ok(!!analyzer, "Failed to create analyzer instance.\n");

    init_textsource(&analysissource, L"", 0, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    hr = IDWriteTextAnalyzer_AnalyzeLineBreakpoints(analyzer, &analysissource.IDWriteTextAnalysisSource_iface, 0, 0,
        &analysissink);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    while (*ptr->text)
    {
        UINT32 len;

        init_textsource(&analysissource, ptr->text, -1, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);

        len = lstrlenW(ptr->text);
        if (len > BREAKPOINT_COUNT) {
            ok(0, "test %u: increase BREAKPOINT_COUNT to at least %u\n", i, len);
            i++;
            ptr++;
            continue;
        }

        memset(g_actual_bp, 0, sizeof(g_actual_bp));
        hr = IDWriteTextAnalyzer_AnalyzeLineBreakpoints(analyzer, &analysissource.IDWriteTextAnalysisSource_iface,
            0, len, &analysissink);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        compare_breakpoints(ptr, g_actual_bp);

        i++;
        ptr++;
    }

    IDWriteTextAnalyzer_Release(analyzer);
}

static void test_GetScriptProperties(void)
{
    IDWriteTextAnalyzer1 *analyzer;
    DWRITE_SCRIPT_PROPERTIES props;
    DWRITE_SCRIPT_ANALYSIS sa;
    HRESULT hr;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer1);
    if (!analyzer)
    {
        win_skip("GetScriptProperties() is not supported.\n");
        return;
    }

    sa.script = 1000;
    hr = IDWriteTextAnalyzer1_GetScriptProperties(analyzer, sa, &props);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    if (0) /* Crashes on Windows */
        hr = IDWriteTextAnalyzer1_GetScriptProperties(analyzer, sa, NULL);

    sa.script = 0;
    hr = IDWriteTextAnalyzer1_GetScriptProperties(analyzer, sa, &props);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDWriteTextAnalyzer1_Release(analyzer);
}

struct textcomplexity_test {
    const WCHAR text[5];
    UINT32 length;
    BOOL simple;
    UINT32 len_read;
};

static const struct textcomplexity_test textcomplexity_tests[] = {
    { {0},                     1, FALSE, 1 },
    { {0},                     0,  TRUE, 0 },
    { {0x610,0},               0,  TRUE, 0 },
    { {'A','B','C','D',0},     3,  TRUE, 3 },
    { {'A','B','C','D',0},     5,  TRUE, 4 },
    { {'A','B','C','D',0},    10,  TRUE, 4 },
    { {'A',0x610,'C','D',0},   1,  TRUE, 1 },
    { {'A',0x610,'C','D',0},   2, FALSE, 2 },
    { {0x610,'A','C','D',0},   1, FALSE, 1 },
    { {0x610,'A','C','D',0},   2, FALSE, 1 },
    { {0x610,0x610,'C','D',0}, 2, FALSE, 2 },
    { {0xd800,'A','B',0},      1, FALSE, 1 },
    { {0xd800,'A','B',0},      2, FALSE, 1 },
    { {0xdc00,'A','B',0},      2, FALSE, 1 },
    { {0x202a,'A',0x202c,0},   3, FALSE, 1 },
    { {0x200e,'A',0},          2, FALSE, 1 },
    { {0x200f,'A',0},          2, FALSE, 1 },
    { {0x202d,'A',0},          2, FALSE, 1 },
    { {0x202e,'A',0},          2, FALSE, 1 },

};

static void test_GetTextComplexity(void)
{
    IDWriteTextAnalyzer1 *analyzer;
    IDWriteFontFace *fontface;
    UINT16 indices[10];
    BOOL simple;
    HRESULT hr;
    UINT32 len;
    int i;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer1);
    if (!analyzer)
    {
        win_skip("GetTextComplexity() is not supported.\n");
        return;
    }

if (0) { /* crashes on native */
    hr = IDWriteTextAnalyzer1_GetTextComplexity(analyzer, NULL, 0, NULL, NULL, NULL, NULL);
    hr = IDWriteTextAnalyzer1_GetTextComplexity(analyzer, NULL, 0, NULL, NULL, &len, NULL);
    hr = IDWriteTextAnalyzer1_GetTextComplexity(analyzer, L"ABC", 3, NULL, NULL, NULL, NULL);
    hr = IDWriteTextAnalyzer1_GetTextComplexity(analyzer, L"ABC", 3, NULL, NULL, &len, NULL);
    hr = IDWriteTextAnalyzer1_GetTextComplexity(analyzer, L"ABC", 3, NULL, &simple, NULL, NULL);
}

    len = 1;
    simple = TRUE;
    hr = IDWriteTextAnalyzer1_GetTextComplexity(analyzer, NULL, 0, NULL, &simple, &len, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %d\n", len);
    ok(simple == FALSE, "got %d\n", simple);

    len = 1;
    simple = TRUE;
    indices[0] = 1;
    hr = IDWriteTextAnalyzer1_GetTextComplexity(analyzer, L"ABC", 3, NULL, &simple, &len, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %d\n", len);
    ok(simple == FALSE, "got %d\n", simple);
    ok(indices[0] == 1, "got %d\n", indices[0]);

    fontface = create_fontface();

    for (i = 0; i < ARRAY_SIZE(textcomplexity_tests); ++i)
    {
       const struct textcomplexity_test *ptr = &textcomplexity_tests[i];
       len = 1;
       simple = !ptr->simple;
       indices[0] = 0;
       hr = IDWriteTextAnalyzer1_GetTextComplexity(analyzer, ptr->text, ptr->length, fontface, &simple, &len, indices);
       ok(hr == S_OK, "%d: Unexpected hr %#lx.\n", i, hr);
       ok(len == ptr->len_read, "%d: read length: got %d, expected %d\n", i, len, ptr->len_read);
       ok(simple == ptr->simple, "%d: simple: got %d, expected %d\n", i, simple, ptr->simple);
       if (simple && ptr->length)
           ok(indices[0] > 0, "%d: got %d\n", i, indices[0]);
       else
           ok(indices[0] == 0, "%d: got %d\n", i, indices[0]);
    }

    IDWriteFontFace_Release(fontface);
    IDWriteTextAnalyzer1_Release(analyzer);
}

static void test_numbersubstitution(void)
{
    IDWriteNumberSubstitution *substitution;
    HRESULT hr;

    /* locale is not specified, method does not require it */
    hr = IDWriteFactory_CreateNumberSubstitution(factory, DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE, NULL, FALSE, &substitution);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDWriteNumberSubstitution_Release(substitution);

    /* invalid locale name, method does not require it */
    hr = IDWriteFactory_CreateNumberSubstitution(factory, DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE, L"dummy",
            FALSE, &substitution);
    ok(hr == S_OK, "Failed to create number substitution, hr %#lx.\n", hr);
    IDWriteNumberSubstitution_Release(substitution);

    /* invalid method */
    hr = IDWriteFactory_CreateNumberSubstitution(factory, DWRITE_NUMBER_SUBSTITUTION_METHOD_TRADITIONAL+1, NULL, FALSE, &substitution);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* invalid method */
    hr = IDWriteFactory_CreateNumberSubstitution(factory, -1, NULL, FALSE, &substitution);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* invalid locale */
    hr = IDWriteFactory_CreateNumberSubstitution(factory, DWRITE_NUMBER_SUBSTITUTION_METHOD_TRADITIONAL, NULL, FALSE, &substitution);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateNumberSubstitution(factory, DWRITE_NUMBER_SUBSTITUTION_METHOD_TRADITIONAL, L"dummy",
            FALSE, &substitution);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateNumberSubstitution(factory, DWRITE_NUMBER_SUBSTITUTION_METHOD_CONTEXTUAL, L"dummy",
            FALSE, &substitution);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDWriteFactory_CreateNumberSubstitution(factory, DWRITE_NUMBER_SUBSTITUTION_METHOD_NATIONAL, L"dummy",
            FALSE, &substitution);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* invalid locale, but it's not needed for this method */
    hr = IDWriteFactory_CreateNumberSubstitution(factory, DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE, L"dummy", FALSE,
            &substitution);
    ok(hr == S_OK, "Failed to create number substitution, hr %#lx.\n", hr);
    IDWriteNumberSubstitution_Release(substitution);
}

static void get_fontface_glyphs(IDWriteFontFace *fontface, const WCHAR *str, UINT16 *glyphs)
{
    while (*str) {
        UINT32 codepoint = *str;
        HRESULT hr;

        hr = IDWriteFontFace_GetGlyphIndices(fontface, &codepoint, 1, glyphs++);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        str++;
    }
}

static void get_fontface_advances(IDWriteFontFace *fontface, FLOAT emsize, const UINT16 *glyphs, FLOAT *advances, UINT32 count)
{
    DWRITE_FONT_METRICS fontmetrics;
    UINT32 i;

    IDWriteFontFace_GetMetrics(fontface, &fontmetrics);
    for (i = 0; i < count; i++) {
        DWRITE_GLYPH_METRICS metrics;
        HRESULT hr;

        hr = IDWriteFontFace_GetDesignGlyphMetrics(fontface, glyphs + i, 1, &metrics, FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        advances[i] = (FLOAT)metrics.advanceWidth * emsize / (FLOAT)fontmetrics.designUnitsPerEm;
    }
}

enum ot_gdef_class
{
    GDEF_CLASS_UNCLASSIFIED = 0,
    GDEF_CLASS_BASE = 1,
    GDEF_CLASS_LIGATURE = 2,
    GDEF_CLASS_MARK = 3,
    GDEF_CLASS_COMPONENT = 4,
    GDEF_CLASS_MAX = GDEF_CLASS_COMPONENT,
};

struct dwrite_fonttable
{
    BYTE *data;
    void *context;
    unsigned int size;
};

static const void *table_read_ensure(const struct dwrite_fonttable *table, unsigned int offset, unsigned int size)
{
    if (size > table->size || offset > table->size - size)
        return NULL;

    return table->data + offset;
}

static WORD table_read_be_word(const struct dwrite_fonttable *table, unsigned int offset)
{
    const WORD *ptr = table_read_ensure(table, offset, sizeof(*ptr));
    return ptr ? GET_BE_WORD(*ptr) : 0;
}

static int gdef_class_compare_format2(const void *g, const void *r)
{
    const struct ot_gdef_class_range *range = r;
    UINT16 glyph = *(UINT16 *)g;

    if (glyph < GET_BE_WORD(range->start_glyph))
        return -1;
    else if (glyph > GET_BE_WORD(range->end_glyph))
        return 1;
    else
        return 0;
}

static unsigned int get_glyph_class(const struct dwrite_fonttable *table, UINT16 glyph)
{
    unsigned int glyph_class = GDEF_CLASS_UNCLASSIFIED, offset;
    WORD format, count;

    offset = table_read_be_word(table, 4);

    format = table_read_be_word(table, offset);

    if (format == 1)
    {
        const struct ot_gdef_classdef_format1 *format1;

        count = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gdef_classdef_format1, glyph_count));
        format1 = table_read_ensure(table, offset, FIELD_OFFSET(struct ot_gdef_classdef_format1, classes[count]));
        if (format1)
        {
            WORD start_glyph = GET_BE_WORD(format1->start_glyph);
            if (glyph >= start_glyph && (glyph - start_glyph) < count)
            {
                glyph_class = GET_BE_WORD(format1->classes[glyph - start_glyph]);
                if (glyph_class > GDEF_CLASS_MAX)
                     glyph_class = GDEF_CLASS_UNCLASSIFIED;
            }
        }
    }
    else if (format == 2)
    {
        const struct ot_gdef_classdef_format2 *format2;

        count = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gdef_classdef_format2, range_count));
        format2 = table_read_ensure(table, offset, FIELD_OFFSET(struct ot_gdef_classdef_format2, ranges[count]));
        if (format2)
        {
            const struct ot_gdef_class_range *range = bsearch(&glyph, format2->ranges, count,
                    sizeof(struct ot_gdef_class_range), gdef_class_compare_format2);
            glyph_class = range && glyph <= GET_BE_WORD(range->end_glyph) ?
                    GET_BE_WORD(range->glyph_class) : GDEF_CLASS_UNCLASSIFIED;
            if (glyph_class > GDEF_CLASS_MAX)
                 glyph_class = GDEF_CLASS_UNCLASSIFIED;
        }
    }

    return glyph_class;
}

static void get_enus_string(IDWriteLocalizedStrings *strings, WCHAR *buff, unsigned int size)
{
    BOOL exists = FALSE;
    unsigned int index;
    HRESULT hr;

    hr = IDWriteLocalizedStrings_FindLocaleName(strings, L"en-us", &index, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Not all fonts have an en-us name! */
    if (!exists)
        index = 0;

    hr = IDWriteLocalizedStrings_GetString(strings, index, buff, size);
    ok(hr == S_OK, "Failed to get name string, hr %#lx.\n", hr);
}

static void test_glyph_props(IDWriteTextAnalyzer *analyzer, const WCHAR *family, const WCHAR *face,
        IDWriteFontFace *fontface)
{
    unsigned int i, ch, count, offset;
    struct dwrite_fonttable gdef;
    DWRITE_UNICODE_RANGE *ranges;
    IDWriteFontFace1 *fontface1;
    BOOL exists = FALSE;
    HRESULT hr;

    hr = IDWriteFontFace_TryGetFontTable(fontface, MS_GDEF_TAG, (const void **)&gdef.data, &gdef.size,
            &gdef.context, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    if (!exists)
        return;

    offset = table_read_be_word(&gdef, 4);
    if (!offset)
    {
        IDWriteFontFace_ReleaseFontTable(fontface, gdef.context);
        return;
    }

    if (FAILED(IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void **)&fontface1)))
    {
        IDWriteFontFace_ReleaseFontTable(fontface, gdef.context);
        return;
    }

    hr = IDWriteFontFace1_GetUnicodeRanges(fontface1, 0, NULL, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);

    ranges = malloc(count * sizeof(*ranges));

    hr = IDWriteFontFace1_GetUnicodeRanges(fontface1, count, ranges, &count);
    ok(hr == S_OK, "Failed to get ranges, hr %#lx.\n", hr);

    for (i = 0; i < count; ++i)
    {
        if (ranges[i].first > 0xffff)
            break;

        for (ch = ranges[i].first; ch <= ranges[i].last; ch++)
        {
            DWRITE_SHAPING_TEXT_PROPERTIES text_props[10];
            DWRITE_SHAPING_GLYPH_PROPERTIES glyph_props[10];
            UINT16 glyphs[10], clustermap[10], glyph;
            unsigned int actual_count, glyph_class;
            DWRITE_SCRIPT_ANALYSIS sa;
            WCHAR text[1];

            hr = IDWriteFontFace1_GetGlyphIndices(fontface1, &ch, 1, &glyph);
            ok(hr == S_OK, "Failed to get glyph index, hr %#lx.\n", hr);

            if (!glyph)
                continue;

            sa.script = 999;
            sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;
            text[0] = (WCHAR)ch;
            memset(glyph_props, 0, sizeof(glyph_props));
            hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, text, 1, fontface, FALSE, FALSE, &sa, NULL,
                    NULL, NULL, NULL, 0, ARRAY_SIZE(glyphs), clustermap, text_props, glyphs, glyph_props, &actual_count);
            ok(hr == S_OK, "Failed to shape, hr %#lx.\n", hr);
            if (actual_count > 1)
                continue;

            glyph_class = get_glyph_class(&gdef, glyphs[0]);

            switch (glyph_class)
            {
                case GDEF_CLASS_MARK:
                    ok(glyph_props[0].isDiacritic && glyph_props[0].isZeroWidthSpace,
                            "%#x -> %u: unexpected glyph properties %u/%u. Class %u. Font %s - %s.\n",
                            text[0], glyphs[0], glyph_props[0].isDiacritic, glyph_props[0].isZeroWidthSpace,
                            glyph_class, wine_dbgstr_w(family), wine_dbgstr_w(face));
                    break;
                default:
                    break;
            }

            if (glyph_props[0].isDiacritic)
                ok(glyph_props[0].isZeroWidthSpace,
                        "%#x -> %u: unexpected glyph properties %u/%u. Class %u. Font %s - %s.\n", text[0], glyphs[0],
                        glyph_props[0].isDiacritic, glyph_props[0].isZeroWidthSpace, glyph_class,
                        wine_dbgstr_w(family), wine_dbgstr_w(face));
        }
    }

    free(ranges);

    IDWriteFontFace_ReleaseFontTable(fontface, gdef.context);
    IDWriteFontFace1_Release(fontface1);
}

static void test_GetGlyphs(void)
{
    static const WCHAR test1W[] = {'<','B',' ','C',0};
    static const WCHAR test2W[] = {'<','B','\t','C',0};
    static const WCHAR test3W[] = {0x202a,0x202c,0};
    DWRITE_SHAPING_GLYPH_PROPERTIES shapingprops[20];
    DWRITE_SHAPING_TEXT_PROPERTIES props[20];
    UINT32 maxglyphcount, actual_count;
    FLOAT advances[10], advances2[10];
    IDWriteFontCollection *syscoll;
    IDWriteTextAnalyzer *analyzer;
    IDWriteFontFace *fontface;
    DWRITE_SCRIPT_ANALYSIS sa;
    DWRITE_GLYPH_OFFSET offsets[10];
    UINT16 clustermap[10];
    UINT16 glyphs1[10];
    UINT16 glyphs2[10];
    unsigned int i, j;
    HRESULT hr;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer);
    ok(!!analyzer, "Failed to create analyzer instance.\n");

    fontface = create_fontface();

    maxglyphcount = 1;
    sa.script = 0;
    sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test1W, lstrlenW(test1W), fontface, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs1, shapingprops, &actual_count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);

if (0) {
    /* NULL fontface - crashes on Windows */
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test1W, lstrlenW(test1W), NULL, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs1, shapingprops, &actual_count);
}

    /* invalid script id */
    maxglyphcount = 10;
    actual_count = 0;
    sa.script = 999;
    sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test1W, lstrlenW(test1W), fontface, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs1, shapingprops, &actual_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_count == 4, "got %d\n", actual_count);
    ok(sa.script == 999, "got %u\n", sa.script);

    /* no '\t' -> ' ' replacement */
    maxglyphcount = 10;
    actual_count = 0;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test1W, lstrlenW(test1W), fontface, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs1, shapingprops, &actual_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_count == 4, "got %d\n", actual_count);

    actual_count = 0;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test2W, lstrlenW(test2W), fontface, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs2, shapingprops, &actual_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_count == 4, "got %d\n", actual_count);
    ok(glyphs1[2] != glyphs2[2], "got %d\n", glyphs1[2]);

    /* check that mirroring works */
    maxglyphcount = 10;
    actual_count = 0;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test1W, lstrlenW(test1W), fontface, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs1, shapingprops, &actual_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_count == 4, "got %d\n", actual_count);

    actual_count = 0;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test1W, lstrlenW(test1W), fontface, FALSE, TRUE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs2, shapingprops, &actual_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_count == 4, "got %d\n", actual_count);
    ok(glyphs1[0] != glyphs2[0], "got %d\n", glyphs1[0]);

    /* embedded control codes, with unknown script id 0 */
    get_fontface_glyphs(fontface, test3W, glyphs2);
    get_fontface_advances(fontface, 10.0, glyphs2, advances2, 2);

    actual_count = 0;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test3W, lstrlenW(test3W), fontface, FALSE, TRUE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs1, shapingprops, &actual_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_count == 2, "got %d\n", actual_count);
    ok(glyphs1[0] == glyphs2[0], "got %u, expected %u\n", glyphs1[0], glyphs2[0]);
    ok(glyphs1[1] == glyphs2[1], "got %u, expected %u\n", glyphs1[1], glyphs2[1]);
    ok(shapingprops[0].isClusterStart == 1, "got %d\n", shapingprops[0].isClusterStart);
    ok(shapingprops[0].isZeroWidthSpace == 0, "got %d\n", shapingprops[0].isZeroWidthSpace);
    ok(shapingprops[1].isClusterStart == 1, "got %d\n", shapingprops[1].isClusterStart);
    ok(shapingprops[1].isZeroWidthSpace == 0, "got %d\n", shapingprops[1].isZeroWidthSpace);
    ok(clustermap[0] == 0, "got %d\n", clustermap[0]);
    ok(clustermap[1] == 1, "got %d\n", clustermap[1]);

    memset(advances, 0, sizeof(advances));
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, test3W, clustermap, props, lstrlenW(test3W),
        glyphs1, shapingprops, actual_count, fontface, 10.0, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == advances2[0], "got %.2f, expected %.2f\n", advances[0], advances2[0]);
    ok(advances[1] == advances2[1], "got %.2f, expected %.2f\n", advances[1], advances2[1]);

    /* embedded control codes with proper script */
    sa.script = 0;
    get_script_analysis(test3W, &sa);
    ok(sa.script != 0, "got %d\n", sa.script);
    actual_count = 0;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test3W, lstrlenW(test3W), fontface, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs1, shapingprops, &actual_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_count == 2, "got %d\n", actual_count);
    ok(glyphs1[0] == glyphs2[0], "got %u, expected %u\n", glyphs1[0], glyphs2[0]);
    ok(glyphs1[1] == glyphs2[1], "got %u, expected %u\n", glyphs1[1], glyphs2[1]);
    ok(shapingprops[0].isClusterStart == 1, "got %d\n", shapingprops[0].isClusterStart);
    ok(shapingprops[0].isZeroWidthSpace == 0, "got %d\n", shapingprops[0].isZeroWidthSpace);
    ok(shapingprops[1].isClusterStart == 1, "got %d\n", shapingprops[1].isClusterStart);
    ok(shapingprops[1].isZeroWidthSpace == 0, "got %d\n", shapingprops[1].isZeroWidthSpace);
    ok(clustermap[0] == 0, "got %d\n", clustermap[0]);
    ok(clustermap[1] == 1, "got %d\n", clustermap[1]);

    memset(advances, 0, sizeof(advances));
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, test3W, clustermap, props, lstrlenW(test3W),
        glyphs1, shapingprops, actual_count, fontface, 10.0, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == advances2[0], "got %.2f, expected %.2f\n", advances[0], advances2[0]);
    ok(advances[1] == advances2[1], "got %.2f, expected %.2f\n", advances[1], advances2[1]);

    /* DWRITE_SCRIPT_SHAPES_NO_VISUAL run */
    maxglyphcount = 10;
    actual_count = 0;
    sa.script = 0;
    sa.shapes = DWRITE_SCRIPT_SHAPES_NO_VISUAL;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, test1W, lstrlenW(test1W), fontface, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, maxglyphcount, clustermap, props, glyphs1, shapingprops, &actual_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(actual_count == 4, "got %d\n", actual_count);
    ok(sa.script == 0, "got %u\n", sa.script);
    ok(!shapingprops[0].isZeroWidthSpace, "got %d\n", shapingprops[0].isZeroWidthSpace);

    IDWriteFontFace_Release(fontface);

    /* Test setting glyph properties from GDEF. */
    hr = IDWriteFactory_GetSystemFontCollection(factory, &syscoll, FALSE);
    ok(hr == S_OK, "Failed to get system collection, hr %#lx.\n", hr);

    for (i = 0; i < IDWriteFontCollection_GetFontFamilyCount(syscoll); ++i)
    {
        IDWriteLocalizedStrings *names;
        IDWriteFontFamily *family;
        WCHAR familyW[256];

        hr = IDWriteFontCollection_GetFontFamily(syscoll, i, &family);
        ok(hr == S_OK, "Failed to get font family, hr %#lx.\n", hr);

        hr = IDWriteFontFamily_GetFamilyNames(family, &names);
        ok(hr == S_OK, "Failed to get family names, hr %#lx.\n", hr);
        get_enus_string(names, familyW, ARRAY_SIZE(familyW));
        IDWriteLocalizedStrings_Release(names);

        for (j = 0; j < IDWriteFontFamily_GetFontCount(family); ++j)
        {
            IDWriteFont *font;
            WCHAR faceW[256];

            hr = IDWriteFontFamily_GetFont(family, j, &font);
            ok(hr == S_OK, "Failed to get font instance, hr %#lx.\n", hr);

            hr = IDWriteFont_CreateFontFace(font, &fontface);
            ok(hr == S_OK, "Failed to create fontface, hr %#lx.\n", hr);

            hr = IDWriteFont_GetFaceNames(font, &names);
            ok(hr == S_OK, "Failed to get face names, hr %#lx.\n", hr);
            get_enus_string(names, faceW, ARRAY_SIZE(faceW));
            IDWriteLocalizedStrings_Release(names);

            test_glyph_props(analyzer, familyW, faceW, fontface);

            IDWriteFontFace_Release(fontface);
            IDWriteFont_Release(font);
        }

        IDWriteFontFamily_Release(family);
    }

    IDWriteFontCollection_Release(syscoll);

    IDWriteTextAnalyzer_Release(analyzer);
}

static void test_GetTypographicFeatures(void)
{
    static const WCHAR arabicW[] = {0x064a,0x064f,0x0633,0};
    DWRITE_FONT_FEATURE_TAG tags[20];
    IDWriteTextAnalyzer2 *analyzer;
    IDWriteFontFace *fontface;
    DWRITE_SCRIPT_ANALYSIS sa;
    UINT32 count;
    HRESULT hr;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer2);
    if (!analyzer)
    {
        win_skip("GetTypographicFeatures() is not supported.\n");
        return;
    }

    fontface = create_fontface();

    get_script_analysis(L"abc", &sa);
    count = 0;
    hr = IDWriteTextAnalyzer2_GetTypographicFeatures(analyzer, fontface, sa, NULL, 0, &count, NULL);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(!!count, "Unexpected count %u.\n", count);

    /* invalid locale name is ignored */
    get_script_analysis(L"abc", &sa);
    count = 0;
    hr = IDWriteTextAnalyzer2_GetTypographicFeatures(analyzer, fontface, sa, L"cadabra", 0, &count, NULL);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(!!count, "Unexpected count %u.\n", count);

    /* Make some calls for different scripts. */

    get_script_analysis(arabicW, &sa);
    memset(tags, 0, sizeof(tags));
    count = 0;
    hr = IDWriteTextAnalyzer2_GetTypographicFeatures(analyzer, fontface, sa, NULL, ARRAY_SIZE(tags), &count, tags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!count, "Unexpected count %u.\n", count);

    get_script_analysis(L"abc", &sa);
    memset(tags, 0, sizeof(tags));
    count = 0;
    hr = IDWriteTextAnalyzer2_GetTypographicFeatures(analyzer, fontface, sa, NULL, ARRAY_SIZE(tags), &count, tags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!count, "Unexpected count %u.\n", count);

    IDWriteFontFace_Release(fontface);
    IDWriteTextAnalyzer2_Release(analyzer);
}

static void test_GetGlyphPlacements(void)
{
    DWRITE_SHAPING_GLYPH_PROPERTIES glyphprops[2];
    DWRITE_SHAPING_TEXT_PROPERTIES textprops[2];
    static const WCHAR aW[] = {'A','D',0};
    UINT16 clustermap[2], glyphs[2];
    DWRITE_GLYPH_OFFSET offsets[2];
    IDWriteTextAnalyzer *analyzer;
    IDWriteFontFace *fontface;
    DWRITE_SCRIPT_ANALYSIS sa;
    FLOAT advances[2];
    UINT32 count, len;
    WCHAR *path;
    HRESULT hr;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer);
    ok(!!analyzer, "Failed to create analyzer instance.\n");

    path = create_testfontfile(L"wine_test_font.ttf");
    fontface = create_testfontface(path);

    get_script_analysis(aW, &sa);
    count = 0;
    len = lstrlenW(aW);
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, aW, len, fontface, FALSE, FALSE, &sa, NULL,
        NULL, NULL, NULL, 0, len, clustermap, textprops, glyphs, glyphprops, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 2, "got %u\n", count);

    /* just return on zero glyphs */
    advances[0] = advances[1] = 1.0;
    offsets[0].advanceOffset = offsets[0].ascenderOffset = 2.0;
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, clustermap, textprops,
        len, glyphs, glyphprops, 0, fontface, 0.0, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == 1.0, "got %.2f\n", advances[0]);
    ok(offsets[0].advanceOffset == 2.0 && offsets[0].ascenderOffset == 2.0, "got %.2f,%.2f\n",
        offsets[0].advanceOffset, offsets[0].ascenderOffset);

    /* advances/offsets are scaled with provided font emSize and designed eM box size */
    advances[0] = advances[1] = 1.0;
    memset(offsets, 0xcc, sizeof(offsets));
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, clustermap, textprops,
        len, glyphs, glyphprops, len, fontface, 0.0, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == 0.0, "got %.2f\n", advances[0]);
    ok(offsets[0].advanceOffset == 0.0 && offsets[0].ascenderOffset == 0.0, "got %.2f,%.2f\n",
        offsets[0].advanceOffset, offsets[0].ascenderOffset);

    advances[0] = advances[1] = 1.0;
    memset(offsets, 0xcc, sizeof(offsets));
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, clustermap, textprops,
        len, glyphs, glyphprops, len, fontface, 2048.0, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == 1000.0, "got %.2f\n", advances[0]);
    ok(offsets[0].advanceOffset == 0.0 && offsets[0].ascenderOffset == 0.0, "got %.2f,%.2f\n",
        offsets[0].advanceOffset, offsets[0].ascenderOffset);

    advances[0] = advances[1] = 1.0;
    memset(offsets, 0xcc, sizeof(offsets));
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, clustermap, textprops,
        len, glyphs, glyphprops, len, fontface, 1024.0, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == 500.0, "got %.2f\n", advances[0]);
    ok(advances[1] == 500.0, "got %.2f\n", advances[1]);
    ok(offsets[0].advanceOffset == 0.0 && offsets[0].ascenderOffset == 0.0, "got %.2f,%.2f\n",
        offsets[0].advanceOffset, offsets[0].ascenderOffset);

    advances[0] = advances[1] = 1.0;
    memset(offsets, 0xcc, sizeof(offsets));
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, clustermap, textprops,
        len, glyphs, glyphprops, len, fontface, 20.48, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == 10.0, "got %.2f\n", advances[0]);
    ok(advances[1] == 10.0, "got %.2f\n", advances[1]);
    ok(offsets[0].advanceOffset == 0.0 && offsets[0].ascenderOffset == 0.0, "got %.2f,%.2f\n",
        offsets[0].advanceOffset, offsets[0].ascenderOffset);

    /* without clustermap */
    advances[0] = advances[1] = 1.0;
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, NULL, textprops,
        len, glyphs, glyphprops, len, fontface, 1024.0, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == 500.0, "got %.2f\n", advances[0]);
    ok(advances[1] == 500.0, "got %.2f\n", advances[1]);

    /* it's happy to use negative size too */
    advances[0] = advances[1] = 1.0;
    memset(offsets, 0xcc, sizeof(offsets));
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, clustermap, textprops,
        len, glyphs, glyphprops, len, fontface, -10.24, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == -5.0, "got %.2f\n", advances[0]);
    ok(offsets[0].advanceOffset == 0.0 && offsets[0].ascenderOffset == 0.0, "got %.2f,%.2f\n",
        offsets[0].advanceOffset, offsets[0].ascenderOffset);

    /* DWRITE_SCRIPT_SHAPES_NO_VISUAL has no effect on placement */
    sa.shapes = DWRITE_SCRIPT_SHAPES_NO_VISUAL;
    advances[0] = advances[1] = 1.0f;
    memset(offsets, 0xcc, sizeof(offsets));
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, clustermap, textprops,
        len, glyphs, glyphprops, len, fontface, 2048.0f, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == 1000.0f, "got %.2f\n", advances[0]);
    ok(advances[1] == 1000.0f, "got %.2f\n", advances[1]);
    ok(offsets[0].advanceOffset == 0.0f && offsets[0].ascenderOffset == 0.0f, "got %.2f,%.2f\n",
        offsets[0].advanceOffset, offsets[0].ascenderOffset);

    /* isZeroWidthSpace */
    sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;
    advances[0] = advances[1] = 1.0f;
    memset(offsets, 0xcc, sizeof(offsets));
    glyphprops[0].isZeroWidthSpace = 1;
    hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, aW, clustermap, textprops,
        len, glyphs, glyphprops, len, fontface, 2048.0f, FALSE, FALSE, &sa, NULL, NULL,
        NULL, 0, advances, offsets);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(advances[0] == 0.0f, "got %.2f\n", advances[0]);
    ok(advances[1] == 1000.0f, "got %.2f\n", advances[1]);
    ok(offsets[0].advanceOffset == 0.0f && offsets[0].ascenderOffset == 0.0f, "got %.2f,%.2f\n",
        offsets[0].advanceOffset, offsets[0].ascenderOffset);

    IDWriteTextAnalyzer_Release(analyzer);
    IDWriteFontFace_Release(fontface);
    DELETE_FONTFILE(path);
}

struct spacing_test {
    FLOAT leading;
    FLOAT trailing;
    FLOAT min_advance;
    FLOAT advances[3];
    FLOAT offsets[3];
    FLOAT modified_advances[3];
    FLOAT modified_offsets[3];
    BOOL  single_cluster;
    DWRITE_SHAPING_GLYPH_PROPERTIES props[3];
};

static const struct spacing_test spacing_tests[] =
{
/* Default spacing glyph properties. */
#define P_S { 0 }
/* isZeroWidthSpace */
#define P_Z { 0, 0, 0, 1, 0 }
/* isDiacritic */
#define P_D { 0, 0, 1, 0, 0 }
/* isDiacritic + isZeroWidthSpace, that's how diacritics are shaped. */
#define P_D_Z { 0, 0, 1, 1, 0 }

    {   0.0,   0.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 } }, /* 0 */
    {   0.0,   0.0,  2.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 } },
    {   1.0,   0.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 11.0, 12.0 }, {  3.0,  4.0 } },
    {   1.0,   1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 12.0, 13.0 }, {  3.0,  4.0 } },
    {   1.0,  -1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  3.0,  4.0 } },
    {   0.0, -10.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  1.0 }, {  2.0,  3.0 } }, /* 5 */
    {  -5.0,  -4.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0,  5.0 }, { -1.0, -0.5 } },
    {  -5.0,  -5.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0,  5.0 }, { -0.5,  0.0 } },
    {   2.0,   0.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  7.0,  7.0 }, {  6.0,  6.5 } },
    {   2.0,   1.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  8.0,  8.0 }, {  6.0,  6.5 } },
    {   2.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  7.0,  7.0 }, {  4.0,  5.0 } }, /* 10 */
    {   1.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  6.0,  6.0 }, {  3.0,  4.0 } },
    { -10.0,   1.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  6.0,  6.0 }, { -3.0, -3.0 } },
    {   0.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0,  5.0 }, {  2.0,  3.0 } },
    {   1.0, -10.0, -5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  0.0 }, {  2.0,  3.0 } },
    { -10.0,   1.0,  5.0, {  8.0, 11.0 }, { 2.0, 3.0 }, {  6.0,  6.0 }, { -1.0, -3.0 } }, /* 15 */
    /* cluster of more than 1 glyph */
    {  0.0f,   0.0f,  0.0f, { 10.0f, 11.0f }, { 2.0f, 3.0f }, { 10.0f, 11.0f }, {  2.0f, 3.0f }, TRUE },
    {  1.0f,   0.0f,  0.0f, { 10.0f, 11.0f }, { 2.0f, 3.5f }, { 11.0f, 11.0f }, {  3.0f, 3.5f }, TRUE },
    {  1.0f,   1.0f,  0.0f, { 10.0f, 11.0f }, { 2.0f, 3.0f }, { 11.0f, 12.0f }, {  3.0f, 3.0f }, TRUE },
    {   1.0,  -1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 11.0, 10.0 }, {  3.0, 3.0 }, TRUE },
    {   0.0, -10.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0,  1.0 }, {  2.0, 3.0 }, TRUE }, /* 20 */
    {   0.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0,  1.0 }, {  2.0, 3.0 }, TRUE },
    {   1.0, -10.0, -5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  0.0 }, {  2.0, 3.0 }, TRUE },
    {  -5.0, -10.0,  4.0, { 10.0, 11.0, 12.0 }, { 2.0, 3.0, 4.0 }, { 5.0, 11.0, 2.0 }, { -3.0, 3.0, 4.0 }, TRUE },
    { -10.0, -10.0,  4.0, { 10.0, 11.0, 12.0 }, { 2.0, 3.0, 4.0 }, { 0.0, 11.0, 2.0 }, { -8.0, 3.0, 4.0 }, TRUE },
    { -10.0, -10.0,  5.0, { 10.0,  1.0, 12.0 }, { 2.0, 3.0, 4.0 }, { 1.0,  1.0, 3.0 }, { -7.0, 3.0, 4.0 }, TRUE }, /* 25 */
    { -10.0,   1.0,  5.0, { 10.0,  1.0,  2.0 }, { 2.0, 3.0, 4.0 }, { 2.0,  1.0, 3.0 }, { -6.0, 3.0, 4.0 }, TRUE },
    {   1.0, -10.0,  5.0, {  2.0,  1.0, 10.0 }, { 2.0, 3.0, 4.0 }, { 3.0,  1.0, 2.0 }, {  3.0, 3.0, 4.0 }, TRUE },
    { -10.0, -10.0,  5.0, { 11.0,  1.0, 11.0 }, { 2.0, 3.0, 4.0 }, { 2.0,  1.0, 2.0 }, { -7.0, 3.0, 4.0 }, TRUE },
    /* isZeroWidthSpace set */
    {   1.0,   0.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 12.0 }, {  2.0,  4.0 }, FALSE, { P_Z, P_S } },
    {   1.0,   1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 13.0 }, {  2.0,  4.0 }, FALSE, { P_Z, P_S } }, /* 30 */
    {   1.0,  -1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  3.0,  3.0 }, FALSE, { P_S, P_Z } },
    {   0.0, -10.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0,  1.0 }, {  2.0,  3.0 }, FALSE, { P_Z, P_S } },
    {  -5.0,  -4.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0, 11.0 }, { -1.0,  3.0 }, FALSE, { P_S, P_Z } },
    {  -5.0,  -5.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, FALSE, { P_Z, P_Z } },
    {   2.0,   0.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  7.0,  2.0 }, {  6.0,  3.0 }, FALSE, { P_S, P_Z } }, /* 35 */
    {   2.0,   1.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  8.0,  2.0 }, {  6.0,  3.0 }, FALSE, { P_S, P_Z } },
    {   2.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, FALSE, { P_Z, P_Z } },
    {   1.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  6.0, 11.0 }, {  3.0,  3.0 }, FALSE, { P_S, P_Z } },
    { -10.0,   1.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, FALSE, { P_Z, P_Z } },
    {   0.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0, 11.0 }, {  2.0,  3.0 }, FALSE, { P_S, P_Z } }, /* 40 */
    {   1.0, -10.0, -5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  0.0 }, {  2.0,  3.0 }, FALSE, { P_Z, P_S } },
    { -10.0,   1.0,  5.0, {  8.0, 11.0 }, { 2.0, 3.0 }, {  6.0, 11.0 }, { -1.0,  3.0 }, FALSE, { P_S, P_Z } },
    /* isDiacritic */
    {   1.0,   0.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 11.0, 12.0 }, {  3.0,  4.0 }, FALSE, { P_D, P_S } },
    {   1.0,   1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 12.0, 13.0 }, {  3.0,  4.0 }, FALSE, { P_D, P_S } },
    {   1.0,  -1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  3.0,  4.0 }, FALSE, { P_S, P_D } }, /* 45 */
    {   0.0, -10.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  1.0 }, {  2.0,  3.0 }, FALSE, { P_D, P_S } },
    {  -5.0,  -4.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0,  5.0 }, { -1.0, -0.5 }, FALSE, { P_S, P_D } },
    {  -5.0,  -5.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0,  5.0 }, { -0.5,  0.0 }, FALSE, { P_D, P_D } },
    {   2.0,   0.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  7.0,  7.0 }, {  6.0,  6.5 }, FALSE, { P_S, P_D } },
    {   2.0,   1.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  8.0,  8.0 }, {  6.0,  6.5 }, FALSE, { P_S, P_D } }, /* 50 */
    {   2.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  7.0,  7.0 }, {  4.0,  5.0 }, FALSE, { P_D, P_D } },
    {   1.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  6.0,  6.0 }, {  3.0,  4.0 }, FALSE, { P_S, P_D } },
    { -10.0,   1.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  6.0,  6.0 }, { -3.0, -3.0 }, FALSE, { P_D, P_D } },
    {   0.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0,  5.0 }, {  2.0,  3.0 }, FALSE, { P_S, P_D } },
    {   1.0, -10.0, -5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  0.0 }, {  2.0,  3.0 }, FALSE, { P_D, P_S } }, /* 55 */
    { -10.0,   1.0,  5.0, {  8.0, 11.0 }, { 2.0, 3.0 }, {  6.0,  6.0 }, { -1.0, -3.0 }, FALSE, { P_S, P_D } },
    /* isZeroWidthSpace in a cluster */
    {   1.0,   0.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 12.0 }, {  3.0,  4.0 }, TRUE, { P_Z, P_S } },
    {   1.0,   1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 13.0 }, {  3.0,  4.0 }, TRUE, { P_Z, P_S } },
    {   1.0,  -1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  3.0,  4.0 }, TRUE, { P_S, P_Z } },
    {   0.0, -10.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0,  1.0 }, {  2.0,  3.0 }, TRUE, { P_Z, P_S } }, /* 60 */
    {  -5.0,  -4.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  1.0, 11.0 }, { -3.0,  7.0 }, TRUE, { P_S, P_Z } },
    {  -5.0,  -5.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, TRUE, { P_Z, P_Z } },
    {   0.0,   0.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  3.0,  2.0 }, {  3.0,  2.0 }, TRUE, { P_S, P_Z } },
    {   0.0,   0.0,  5.0, {  1.0,  2.0, 1.0 }, { 2.0, 3.0, 5.0 }, { 1.5, 2.0, 1.5 }, { 2.5, 3.0, 5.0 }, TRUE, { P_S, P_Z, P_S } },
    {   0.0,   0.0,  5.0, {  1.0,  2.0, 1.0 }, { 2.0, 3.0, 5.0 }, { 1.5, 2.5, 1.0 }, { 2.5, 3.0, 4.5 }, TRUE, { P_S, P_S, P_Z } },
    {   0.0,   0.0,  5.0, {  1.0,  2.0, 1.0 }, { 2.0, 3.0, 5.0 }, { 2.0, 2.0, 1.0 }, { 2.5, 2.5, 4.5 }, TRUE, { P_S, P_Z, P_Z } },
    {   0.0,   0.0,  5.0, {  1.0,  2.0, 1.0 }, { 2.0, 3.0, 5.0 }, { 1.0, 2.0, 1.0 }, { 2.0, 3.0, 5.0 }, TRUE, { P_Z, P_Z, P_Z } },
    {   2.0,   1.0,  1.0, {  1.0,  2.0, 3.0 }, { 2.0, 3.0, 4.0 }, {  3.0,  2.0, 4.0 }, {  4.0,  3.0, 4.0 }, TRUE, { P_S, P_Z, P_S } },
    {   0.0,   0.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, { 3.0, 2.0 }, { 3.0, 2.0 }, TRUE, { P_S, P_Z } },
    {   0.0,   0.0,  5.0, {  1.0,  2.0, 6.0 }, { 2.0, 3.0, 4.0 }, { 1.0, 2.0, 6.0 }, { 2.0, 3.0, 4.0 }, TRUE, { P_S, P_Z, P_S } },
    {   2.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, TRUE, { P_Z, P_Z } }, /* 65 */
    {   1.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  1.0, 11.0 }, {  3.0, 13.0 }, TRUE, { P_S, P_Z } },
    { -10.0,   1.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, TRUE, { P_Z, P_Z } },
    {   0.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0, 11.0 }, {  2.0, 13.0 }, TRUE, { P_S, P_Z } },
    {   1.0, -10.0, -5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  0.0 }, {  2.0,  3.0 }, TRUE, { P_Z, P_S } },
    { -10.0,   1.0,  5.0, {  8.0, 11.0 }, { 2.0, 3.0 }, { -1.0, 11.0 }, { -8.0,  2.0 }, TRUE, { P_S, P_Z } }, /* 70 */
    /* isDiacritic in a cluster */
    {   1.0,   0.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 11.0, 11.0 }, {  3.0,  3.0 }, TRUE, { P_D, P_S } },
    {   1.0,   1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 11.0, 12.0 }, {  3.0,  3.0 }, TRUE, { P_D, P_S } },
    {   1.0,  -1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 11.0, 10.0 }, {  3.0,  3.0 }, TRUE, { P_S, P_D } },
    {   0.0, -10.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0,  1.0 }, {  2.0,  3.0 }, TRUE, { P_D, P_S } },
    {  -5.0,  -4.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0,  7.0 }, { -3.0,  3.0 }, TRUE, { P_S, P_D } }, /* 75 */
    {  -5.0,  -5.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0,  6.0 }, { -3.0,  3.0 }, TRUE, { P_D, P_D } },
    {   2.0,   0.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  4.0,  3.0 }, {  5.0,  3.0 }, TRUE, { P_S, P_D } },
    {   2.0,   1.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  4.0,  4.0 }, {  5.0,  3.0 }, TRUE, { P_S, P_D } },
    {   2.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 12.0,  1.0 }, {  4.0,  3.0 }, TRUE, { P_D, P_D } },
    {   1.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 11.0,  1.0 }, {  3.0,  3.0 }, TRUE, { P_S, P_D } }, /* 80 */
    { -10.0,   1.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0, 12.0 }, { -8.0,  3.0 }, TRUE, { P_D, P_D } },
    {   0.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0,  1.0 }, {  2.0,  3.0 }, TRUE, { P_S, P_D } },
    {   1.0, -10.0, -5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  0.0 }, {  2.0,  3.0 }, TRUE, { P_D, P_S } },
    { -10.0,   1.0,  5.0, {  8.0, 11.0 }, { 2.0, 3.0 }, { -2.0, 12.0 }, { -8.0,  3.0 }, TRUE, { P_S, P_D } },
    /* isZeroWidthSpace + isDiacritic */
    {   1.0,   0.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 12.0 }, {  2.0,  4.0 }, FALSE, { P_D_Z, P_S } }, /* 85 */
    {   1.0,   1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 13.0 }, {  2.0,  4.0 }, FALSE, { P_D_Z, P_S } },
    {   1.0,  -1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  3.0,  3.0 }, FALSE, { P_S, P_D_Z } },
    {   0.0, -10.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0,  1.0 }, {  2.0,  3.0 }, FALSE, { P_D_Z, P_S } },
    {  -5.0,  -4.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0, 11.0 }, { -1.0,  3.0 }, FALSE, { P_S, P_D_Z } },
    {  -5.0,  -5.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, FALSE, { P_D_Z, P_D_Z } }, /* 90 */
    {   2.0,   0.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  7.0,  2.0 }, {  6.0,  3.0 }, FALSE, { P_S, P_D_Z } },
    {   2.0,   1.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  8.0,  2.0 }, {  6.0,  3.0 }, FALSE, { P_S, P_D_Z } },
    {   2.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, FALSE, { P_D_Z, P_D_Z } },
    {   1.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  6.0, 11.0 }, {  3.0,  3.0 }, FALSE, { P_S, P_D_Z } },
    { -10.0,   1.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, FALSE, { P_D_Z, P_D_Z } }, /* 95 */
    {   0.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  5.0, 11.0 }, {  2.0,  3.0 }, FALSE, { P_S, P_D_Z } },
    {   1.0, -10.0, -5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  0.0 }, {  2.0,  3.0 }, FALSE, { P_D_Z, P_S } },
    { -10.0,   1.0,  5.0, {  8.0, 11.0 }, { 2.0, 3.0 }, {  6.0, 11.0 }, { -1.0,  3.0 }, FALSE, { P_S, P_D_Z } },
    /* isZeroWidthSpace + isDiacritic in a cluster */
    {   1.0,   0.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 12.0 }, {  3.0,  4.0 }, TRUE, { P_D_Z, P_S } },
    {   1.0,   1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 13.0 }, {  3.0,  4.0 }, TRUE, { P_D_Z, P_S } }, /* 100 */
    {   1.0,  -1.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  3.0,  4.0 }, TRUE, { P_S, P_D_Z } },
    {   0.0, -10.0,  0.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0,  1.0 }, {  2.0,  3.0 }, TRUE, { P_D_Z, P_S } },
    {  -5.0,  -4.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  1.0, 11.0 }, { -3.0,  7.0 }, TRUE, { P_S, P_D_Z } },
    {  -5.0,  -5.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, TRUE, { P_D_Z, P_D_Z } },
    {   2.0,   0.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  5.0,  2.0 }, {  5.0,  2.0 }, TRUE, { P_S, P_D_Z } }, /* 105 */
    {   2.0,   1.0,  5.0, {  1.0,  2.0 }, { 2.0, 3.0 }, {  6.0,  2.0 }, {  5.0,  1.0 }, TRUE, { P_S, P_D_Z } },
    {   2.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, TRUE, { P_D_Z, P_D_Z } },
    {   1.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  1.0, 11.0 }, {  3.0, 13.0 }, TRUE, { P_S, P_D_Z } },
    { -10.0,   1.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, { 10.0, 11.0 }, {  2.0,  3.0 }, TRUE, { P_D_Z, P_D_Z } },
    {   0.0, -10.0,  5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0, 11.0 }, {  2.0, 13.0 }, TRUE, { P_S, P_D_Z } }, /* 110 */
    {   1.0, -10.0, -5.0, { 10.0, 11.0 }, { 2.0, 3.0 }, {  0.0,  0.0 }, {  2.0,  3.0 }, TRUE, { P_D_Z, P_S } },
    { -10.0,   1.0,  5.0, {  8.0, 11.0 }, { 2.0, 3.0 }, { -1.0, 11.0 }, { -8.0,  2.0 }, TRUE, { P_S, P_D_Z } },

#undef P_S
#undef P_D
#undef P_Z
#undef P_D_Z
};

static void test_ApplyCharacterSpacing(void)
{
    DWRITE_SHAPING_GLYPH_PROPERTIES props[3];
    IDWriteTextAnalyzer1 *analyzer;
    UINT16 clustermap[2];
    HRESULT hr;
    int i;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer1);
    if (!analyzer)
    {
        win_skip("ApplyCharacterSpacing() is not supported.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(spacing_tests); ++i)
    {
        const struct spacing_test *ptr = spacing_tests + i;
        DWRITE_GLYPH_OFFSET offsets[3];
        UINT32 glyph_count;
        FLOAT advances[3];

        offsets[0].advanceOffset = ptr->offsets[0];
        offsets[1].advanceOffset = ptr->offsets[1];
        offsets[2].advanceOffset = ptr->offsets[2];
        /* Ascender offsets are never touched as spacing applies in reading direction only,
           we'll only test them to see if they are not changed */
        offsets[0].ascenderOffset = 23.0;
        offsets[1].ascenderOffset = 32.0;
        offsets[2].ascenderOffset = 31.0;

        advances[0] = advances[1] = 123.45f;
        memcpy(props, ptr->props, sizeof(props));
        glyph_count = ptr->advances[2] > 0.0 ? 3 : 2;
        if (ptr->single_cluster)
        {
            clustermap[0] = 0;
            clustermap[1] = 0;
            props[0].isClusterStart = 1;
        }
        else
        {
            /* trivial case with one glyph per cluster */
            clustermap[0] = 0;
            clustermap[1] = 1;
            props[0].isClusterStart = props[1].isClusterStart = 1;
        }

        winetest_push_context("Test %u", i);

        hr = IDWriteTextAnalyzer1_ApplyCharacterSpacing(analyzer,
            ptr->leading,
            ptr->trailing,
            ptr->min_advance,
            ARRAY_SIZE(clustermap),
            glyph_count,
            clustermap,
            ptr->advances,
            offsets,
            props,
            advances,
            offsets);
        ok(hr == (ptr->min_advance < 0.0f ? E_INVALIDARG : S_OK), "Unexpected hr %#lx.\n", hr);

        if (hr == S_OK) {
            ok(ptr->modified_advances[0] == advances[0], "Got advance[0] %.2f, expected %.2f.\n", advances[0], ptr->modified_advances[0]);
            ok(ptr->modified_advances[1] == advances[1], "Got advance[1] %.2f, expected %.2f.\n", advances[1], ptr->modified_advances[1]);
            if (glyph_count > 2)
                ok(ptr->modified_advances[2] == advances[2], "Got advance[2] %.2f, expected %.2f.\n", advances[2], ptr->modified_advances[2]);

            ok(ptr->modified_offsets[0] == offsets[0].advanceOffset, "Got offset[0] %.2f, expected %.2f.\n",
                offsets[0].advanceOffset, ptr->modified_offsets[0]);
            ok(ptr->modified_offsets[1] == offsets[1].advanceOffset, "Got offset[1] %.2f, expected %.2f.\n",
                offsets[1].advanceOffset, ptr->modified_offsets[1]);
            if (glyph_count > 2)
                ok(ptr->modified_offsets[2] == offsets[2].advanceOffset, "Got offset[2] %.2f, expected %.2f.\n",
                    offsets[2].advanceOffset, ptr->modified_offsets[2]);

            ok(offsets[0].ascenderOffset == 23.0, "Unexpected ascenderOffset %.2f.\n", offsets[0].ascenderOffset);
            ok(offsets[1].ascenderOffset == 32.0, "Unexpected ascenderOffset %.2f.\n", offsets[1].ascenderOffset);
            ok(offsets[2].ascenderOffset == 31.0, "Unexpected ascenderOffset %.2f.\n", offsets[2].ascenderOffset);
        }
        else {
            ok(ptr->modified_advances[0] == advances[0], "Got advance[0] %.2f, expected %.2f.\n", advances[0], ptr->modified_advances[0]);
            ok(ptr->modified_advances[1] == advances[1], "Got advance[1] %.2f, expected %.2f.\n", advances[1], ptr->modified_advances[1]);
            ok(ptr->offsets[0] == offsets[0].advanceOffset, "Got offset[0] %.2f, expected %.2f.\n",
                offsets[0].advanceOffset, ptr->modified_offsets[0]);
            ok(ptr->offsets[1] == offsets[1].advanceOffset, "Got offset[1] %.2f, expected %.2f.\n",
                offsets[1].advanceOffset, ptr->modified_offsets[1]);
            ok(offsets[0].ascenderOffset == 23.0, "Unexpected ascenderOffset %.2f.\n", offsets[0].ascenderOffset);
            ok(offsets[1].ascenderOffset == 32.0, "Unexpected ascenderOffset %.2f.\n", offsets[1].ascenderOffset);
        }

        /* same, with argument aliasing */
        memcpy(advances, ptr->advances, glyph_count * sizeof(*advances));
        offsets[0].advanceOffset = ptr->offsets[0];
        offsets[1].advanceOffset = ptr->offsets[1];
        offsets[2].advanceOffset = ptr->offsets[2];
        /* Ascender offsets are never touched as spacing applies in reading direction only,
           we'll only test them to see if they are not changed */
        offsets[0].ascenderOffset = 23.0f;
        offsets[1].ascenderOffset = 32.0f;
        offsets[2].ascenderOffset = 31.0f;

        hr = IDWriteTextAnalyzer1_ApplyCharacterSpacing(analyzer,
            ptr->leading,
            ptr->trailing,
            ptr->min_advance,
            ARRAY_SIZE(clustermap),
            glyph_count,
            clustermap,
            advances,
            offsets,
            props,
            advances,
            offsets);
        ok(hr == (ptr->min_advance < 0.0f ? E_INVALIDARG : S_OK), "Unexpected hr %#lx.\n", hr);

        if (hr == S_OK)
        {
            ok(ptr->modified_advances[0] == advances[0], "Got advance[0] %.2f, expected %.2f.\n", advances[0], ptr->modified_advances[0]);
            ok(ptr->modified_advances[1] == advances[1], "Got advance[1] %.2f, expected %.2f.\n", advances[1], ptr->modified_advances[1]);
            if (glyph_count > 2)
                ok(ptr->modified_advances[2] == advances[2], "Got advance[2] %.2f, expected %.2f.\n", advances[2], ptr->modified_advances[2]);

            ok(ptr->modified_offsets[0] == offsets[0].advanceOffset, "Got offset[0] %.2f, expected %.2f.\n",
                offsets[0].advanceOffset, ptr->modified_offsets[0]);
            ok(ptr->modified_offsets[1] == offsets[1].advanceOffset, "Got offset[1] %.2f, expected %.2f.\n",
                offsets[1].advanceOffset, ptr->modified_offsets[1]);
            if (glyph_count > 2)
                ok(ptr->modified_offsets[2] == offsets[2].advanceOffset, "Got offset[2] %.2f, expected %.2f.\n",
                    offsets[2].advanceOffset, ptr->modified_offsets[2]);

            ok(offsets[0].ascenderOffset == 23.0f, "Unexpected ascenderOffset %.2f.\n", offsets[0].ascenderOffset);
            ok(offsets[1].ascenderOffset == 32.0f, "Unexpected ascenderOffset %.2f.\n", offsets[1].ascenderOffset);
            ok(offsets[2].ascenderOffset == 31.0f, "Unexpected ascenderOffset %.2f.\n", offsets[2].ascenderOffset);
        }
        else
        {
            /* with aliased advances original values are retained */
            ok(ptr->advances[0] == advances[0], "Got advance[0] %.2f, expected %.2f.\n", advances[0], ptr->advances[0]);
            ok(ptr->advances[1] == advances[1], "Got advance[1] %.2f, expected %.2f.\n", advances[1], ptr->advances[1]);
            ok(ptr->offsets[0] == offsets[0].advanceOffset, "Got offset[0] %.2f, expected %.2f.\n",
                offsets[0].advanceOffset, ptr->modified_offsets[0]);
            ok(ptr->offsets[1] == offsets[1].advanceOffset, "Got offset[1] %.2f, expected %.2f.\n",
                offsets[1].advanceOffset, ptr->modified_offsets[1]);
            ok(offsets[0].ascenderOffset == 23.0f, "Unexpected ascenderOffset %.2f.\n", offsets[0].ascenderOffset);
            ok(offsets[1].ascenderOffset == 32.0f, "Unexpected ascenderOffset %.2f.\n", offsets[1].ascenderOffset);
        }

        winetest_pop_context();
    }

    IDWriteTextAnalyzer1_Release(analyzer);
}

struct orientation_transf_test {
    DWRITE_GLYPH_ORIENTATION_ANGLE angle;
    BOOL is_sideways;
    DWRITE_MATRIX m;
};

static const struct orientation_transf_test ot_tests[] = {
    { DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES,   FALSE, {  1.0,  0.0,  0.0,  1.0, 0.0, 0.0 } },
    { DWRITE_GLYPH_ORIENTATION_ANGLE_90_DEGREES,  FALSE, {  0.0,  1.0, -1.0,  0.0, 0.0, 0.0 } },
    { DWRITE_GLYPH_ORIENTATION_ANGLE_180_DEGREES, FALSE, { -1.0,  0.0,  0.0, -1.0, 0.0, 0.0 } },
    { DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES, FALSE, {  0.0, -1.0,  1.0,  0.0, 0.0, 0.0 } },
    { DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES,   TRUE,  {  0.0,  1.0, -1.0,  0.0, 0.0, 0.0 } },
    { DWRITE_GLYPH_ORIENTATION_ANGLE_90_DEGREES,  TRUE,  { -1.0,  0.0,  0.0, -1.0, 0.0, 0.0 } },
    { DWRITE_GLYPH_ORIENTATION_ANGLE_180_DEGREES, TRUE,  {  0.0, -1.0,  1.0,  0.0, 0.0, 0.0 } },
    { DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES, TRUE,  {  1.0,  0.0,  0.0,  1.0, 0.0, 0.0 } }
};

static inline const char *dbgstr_matrix(const DWRITE_MATRIX *m)
{
    static char buff[64];
    sprintf(buff, "{%.2f, %.2f, %.2f, %.2f, %.2f, %.2f}", m->m11, m->m12,
        m->m21, m->m22, m->dx, m->dy);
    return buff;
}

static void test_GetGlyphOrientationTransform(void)
{
    IDWriteTextAnalyzer2 *analyzer2;
    IDWriteTextAnalyzer1 *analyzer;
    FLOAT originx, originy;
    DWRITE_MATRIX m;
    HRESULT hr;
    int i;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer1);
    if (!analyzer)
    {
        win_skip("GetGlyphOrientationTransform() is not supported.\n");
        return;
    }

    /* invalid angle value */
    memset(&m, 0xcc, sizeof(m));
    hr = IDWriteTextAnalyzer1_GetGlyphOrientationTransform(analyzer,
        DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES + 1, FALSE, &m);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(m.m11 == 0.0, "got %.2f\n", m.m11);

    for (i = 0; i < ARRAY_SIZE(ot_tests); ++i)
    {
        memset(&m, 0, sizeof(m));
        hr = IDWriteTextAnalyzer1_GetGlyphOrientationTransform(analyzer, ot_tests[i].angle,
            ot_tests[i].is_sideways, &m);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!memcmp(&ot_tests[i].m, &m, sizeof(m)), "%d: wrong matrix %s\n", i, dbgstr_matrix(&m));
    }

    IDWriteTextAnalyzer1_Release(analyzer);

    analyzer2 = create_text_analyzer(&IID_IDWriteTextAnalyzer2);
    if (!analyzer2)
    {
        win_skip("IDWriteTextAnalyzer2::GetGlyphOrientationTransform() is not supported.\n");
        return;
    }

    /* invalid angle value */
    memset(&m, 0xcc, sizeof(m));
    hr = IDWriteTextAnalyzer2_GetGlyphOrientationTransform(analyzer2,
        DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES + 1, FALSE, 0.0, 0.0, &m);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(m.m11 == 0.0, "got %.2f\n", m.m11);

    originx = 50.0;
    originy = 60.0;
    for (i = 0; i < ARRAY_SIZE(ot_tests); i++) {
        DWRITE_GLYPH_ORIENTATION_ANGLE angle = DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES;
        DWRITE_MATRIX m_exp;

        memset(&m, 0, sizeof(m));

        /* zero offset gives same result as a call from IDWriteTextAnalyzer1 */
        hr = IDWriteTextAnalyzer2_GetGlyphOrientationTransform(analyzer2, ot_tests[i].angle,
            ot_tests[i].is_sideways, 0.0, 0.0, &m);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!memcmp(&ot_tests[i].m, &m, sizeof(m)), "%d: wrong matrix %s\n", i, dbgstr_matrix(&m));

        m_exp = ot_tests[i].m;
        hr = IDWriteTextAnalyzer2_GetGlyphOrientationTransform(analyzer2, ot_tests[i].angle,
            ot_tests[i].is_sideways, originx, originy, &m);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* 90 degrees more for sideways */
        if (ot_tests[i].is_sideways) {
            switch (ot_tests[i].angle)
            {
            case DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES:
                angle = DWRITE_GLYPH_ORIENTATION_ANGLE_90_DEGREES;
                break;
            case DWRITE_GLYPH_ORIENTATION_ANGLE_90_DEGREES:
                angle = DWRITE_GLYPH_ORIENTATION_ANGLE_180_DEGREES;
                break;
            case DWRITE_GLYPH_ORIENTATION_ANGLE_180_DEGREES:
                angle = DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES;
                break;
            case DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES:
                angle = DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES;
                break;
            default:
                ;
            }
        }
        else
            angle = ot_tests[i].angle;

        /* set expected offsets */
        switch (angle)
        {
        case DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES:
            break;
        case DWRITE_GLYPH_ORIENTATION_ANGLE_90_DEGREES:
            m_exp.dx = originx + originy;
            m_exp.dy = originy - originx;
            break;
        case DWRITE_GLYPH_ORIENTATION_ANGLE_180_DEGREES:
            m_exp.dx = originx + originx;
            m_exp.dy = originy + originy;
            break;
        case DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES:
            m_exp.dx = originx - originy;
            m_exp.dy = originy + originx;
            break;
        default:
            ;
        }

        ok(!memcmp(&m_exp, &m, sizeof(m)), "%d: wrong matrix %s\n", i, dbgstr_matrix(&m));
    }

    IDWriteTextAnalyzer2_Release(analyzer2);
}

static void test_GetBaseline(void)
{
    DWRITE_SCRIPT_ANALYSIS sa = { 0 };
    IDWriteTextAnalyzer1 *analyzer;
    IDWriteFontFace *fontface;
    INT32 baseline;
    BOOL exists;
    HRESULT hr;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer1);
    if (!analyzer)
    {
        win_skip("GetBaseline() is not supported.\n");
        return;
    }

    fontface = create_fontface();

    /* Tahoma does not have a BASE table. */

    exists = TRUE;
    baseline = 456;
    hr = IDWriteTextAnalyzer1_GetBaseline(analyzer, fontface, DWRITE_BASELINE_DEFAULT, FALSE,
           TRUE, sa, NULL, &baseline, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!baseline, "Unexpected baseline %d.\n", baseline);
    ok(!exists, "Unexpected flag %d.\n", exists);

    exists = TRUE;
    baseline = 456;
    hr = IDWriteTextAnalyzer1_GetBaseline(analyzer, fontface, DWRITE_BASELINE_DEFAULT, FALSE,
           FALSE, sa, NULL, &baseline, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!baseline, "Unexpected baseline %d.\n", baseline);
    ok(!exists, "Unexpected flag %d.\n", exists);

    exists = TRUE;
    baseline = 0;
    hr = IDWriteTextAnalyzer1_GetBaseline(analyzer, fontface, DWRITE_BASELINE_CENTRAL, FALSE,
           TRUE, sa, NULL, &baseline, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(baseline != 0, "Unexpected baseline %d.\n", baseline);
    ok(!exists, "Unexpected flag %d.\n", exists);

    exists = TRUE;
    baseline = 0;
    hr = IDWriteTextAnalyzer1_GetBaseline(analyzer, fontface, DWRITE_BASELINE_CENTRAL, FALSE,
           FALSE, sa, NULL, &baseline, &exists);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!baseline, "Unexpected baseline %d.\n", baseline);
    ok(!exists, "Unexpected flag %d.\n", exists);

    exists = TRUE;
    baseline = 456;
    hr = IDWriteTextAnalyzer1_GetBaseline(analyzer, fontface, DWRITE_BASELINE_DEFAULT + 100, FALSE,
           TRUE, sa, NULL, &baseline, &exists);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!baseline, "Unexpected baseline %d.\n", baseline);
    ok(!exists, "Unexpected flag %d.\n", exists);

    IDWriteFontFace_Release(fontface);
    IDWriteTextAnalyzer1_Release(analyzer);
}

static inline BOOL float_eq(FLOAT left, FLOAT right)
{
    int x = *(int *)&left;
    int y = *(int *)&right;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return abs(x - y) <= 8;
}

static void test_GetGdiCompatibleGlyphPlacements(void)
{
    DWRITE_SHAPING_GLYPH_PROPERTIES glyphprops[1];
    DWRITE_SHAPING_TEXT_PROPERTIES textprops[1];
    DWRITE_SCRIPT_ANALYSIS sa = { 0 };
    IDWriteTextAnalyzer *analyzer;
    IDWriteFontFace *fontface;
    UINT16 clustermap[1];
    HRESULT hr;
    UINT32 count;
    UINT16 glyphs[1];
    FLOAT advance;
    DWRITE_GLYPH_OFFSET offsets[1];
    DWRITE_FONT_METRICS fontmetrics;
    float emsize;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer);
    ok(!!analyzer, "Failed to create analyzer instance.\n");

    fontface = create_fontface();

    IDWriteFontFace_GetMetrics(fontface, &fontmetrics);

    count = 0;
    hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, L"A", 1, fontface, FALSE, FALSE, &sa, NULL, NULL, NULL, NULL, 0, 1,
            clustermap, textprops, glyphs, glyphprops, &count);
    ok(hr == S_OK, "Failed to get glyphs, hr %#lx.\n", hr);
    ok(count == 1, "got %u\n", count);

    for (emsize = 12.0f; emsize <= 20.0f; emsize += 1.0f)
    {
        FLOAT compatadvance, expected, ppdip;
        DWRITE_GLYPH_METRICS metrics;

        hr = IDWriteTextAnalyzer_GetGlyphPlacements(analyzer, L"A", clustermap, textprops, 1, glyphs, glyphprops,
                count, fontface, emsize, FALSE, FALSE, &sa, NULL, NULL, NULL, 0, &advance, offsets);
        ok(hr == S_OK, "Failed to get glyph placements, hr %#lx.\n", hr);
        ok(advance > 0.0f, "Unexpected advance %f.\n", advance);

        /* 1 ppdip, no transform */
        ppdip = 1.0;
        hr = IDWriteFontFace_GetGdiCompatibleGlyphMetrics(fontface, emsize, ppdip, NULL, FALSE,
            glyphs, 1, &metrics, FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        expected = floorf(metrics.advanceWidth * emsize * ppdip / fontmetrics.designUnitsPerEm + 0.5f) / ppdip;
        hr = IDWriteTextAnalyzer_GetGdiCompatibleGlyphPlacements(analyzer, L"A", clustermap, textprops, 1, glyphs,
                glyphprops, count, fontface, emsize, ppdip, NULL, FALSE, FALSE, FALSE, &sa, NULL, NULL, NULL, 0,
                &compatadvance, offsets);
        ok(hr == S_OK, "Failed to get glyph placements, hr %#lx.\n", hr);
        ok(compatadvance == expected, "%.0f: got advance %f, expected %f, natural %f\n", emsize,
            compatadvance, expected, advance);

        /* 1.2 ppdip, no transform */
        ppdip = 1.2f;
        hr = IDWriteFontFace_GetGdiCompatibleGlyphMetrics(fontface, emsize, ppdip, NULL, FALSE,
            glyphs, 1, &metrics, FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        expected = floorf(metrics.advanceWidth * emsize * ppdip / fontmetrics.designUnitsPerEm + 0.5f) / ppdip;
        hr = IDWriteTextAnalyzer_GetGdiCompatibleGlyphPlacements(analyzer, L"A", clustermap, textprops, 1, glyphs,
                glyphprops, count, fontface, emsize, ppdip, NULL, FALSE, FALSE, FALSE, &sa, NULL, NULL, NULL, 0,
                &compatadvance, offsets);
        ok(hr == S_OK, "Failed to get glyph placements, hr %#lx.\n", hr);
        ok(float_eq(compatadvance, expected), "%.0f: got advance %f, expected %f, natural %f\n", emsize,
            compatadvance, expected, advance);
    }

    IDWriteFontFace_Release(fontface);
    IDWriteTextAnalyzer_Release(analyzer);
}

struct bidi_test
{
    const WCHAR text[BIDI_LEVELS_COUNT];
    DWRITE_READING_DIRECTION direction;
    UINT8 explicit[BIDI_LEVELS_COUNT];
    UINT8 resolved[BIDI_LEVELS_COUNT];
    BOOL todo;
};

static const struct bidi_test bidi_tests[] = {
    {
      { 0x645, 0x6cc, 0x200c, 0x6a9, 0x646, 0x645, 0 },
      DWRITE_READING_DIRECTION_RIGHT_TO_LEFT,
      { 1, 1, 1, 1, 1, 1 },
      { 1, 1, 1, 1, 1, 1 }
    },
    {
      { 0x645, 0x6cc, 0x200c, 0x6a9, 0x646, 0x645, 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 0, 0, 0, 0, 0, 0 },
      { 1, 1, 1, 1, 1, 1 }
    },
    {
      { 0x200c, 0x645, 0x6cc, 0x6a9, 0x646, 0x645, 0 },
      DWRITE_READING_DIRECTION_RIGHT_TO_LEFT,
      { 1, 1, 1, 1, 1, 1 },
      { 1, 1, 1, 1, 1, 1 }
    },
    {
      { 0x200c, 0x645, 0x6cc, 0x6a9, 0x646, 0x645, 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 0, 0, 0, 0, 0, 0 },
      { 0, 1, 1, 1, 1, 1 }
    },
    {
      { 'A', 0x200c, 'B', 0 },
      DWRITE_READING_DIRECTION_RIGHT_TO_LEFT,
      { 1, 1, 1 },
      { 2, 2, 2 }
    },
    {
      { 'A', 0x200c, 'B', 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 0, 0, 0 },
      { 0, 0, 0 }
    },
    {
      { LRE, PDF, 'a', 'b', 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 2, 2, 0, 0 },
      { 0, 0, 0, 0 },
    },
    {
      { 'a', LRE, PDF, 'b', 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 0, 2, 2, 0 },
      { 0, 0, 0, 0 },
    },
    {
      { RLE, PDF, 'a', 'b', 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 1, 1, 0, 0 },
      { 0, 0, 0, 0 },
    },
    {
      { 'a', RLE, PDF, 'b', 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 },
    },
    {
      { 'a', RLE, PDF, 'b', 0 },
      DWRITE_READING_DIRECTION_RIGHT_TO_LEFT,
      { 1, 3, 3, 1 },
      { 2, 2, 2, 2 },
    },
    {
      { LRE, PDF, 'a', 'b', 0 },
      DWRITE_READING_DIRECTION_RIGHT_TO_LEFT,
      { 2, 2, 1, 1 },
      { 1, 1, 2, 2 },
    },
    {
      { PDF, 'a', 'b', 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }
    },
    {
      { LRE, 'a', 'b', PDF, 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 2, 2, 2, 2 },
      { 0, 2, 2, 2 },
      TRUE
    },
    {
      { LRI, 'a', 'b', PDI, 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      TRUE
    },
    {
      { RLI, 'a', 'b', PDI, 0 },
      DWRITE_READING_DIRECTION_LEFT_TO_RIGHT,
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      TRUE
    },
    {
      { 0 }
    }
};

static void compare_bidi_levels(unsigned int seq, const struct bidi_test *test, UINT32 len, UINT8 *explicit, UINT8 *resolved)
{
    unsigned int i, failcount = 0;
    BOOL match;

    match = !memcmp(explicit, test->explicit, len);
    if (!match) {
        if (test->todo) {
            failcount++;
        todo_wine
            ok(0, "test %u: %s wrong explicit levels:\n", seq, wine_dbgstr_w(test->text));
        }
        else
            ok(0, "test %u: %s wrong explicit levels:\n", seq, wine_dbgstr_w(test->text));

        for (i = 0; i < len; i++) {
            if (test->explicit[i] != explicit[i]) {
               if (test->todo) {
                   failcount++;
               todo_wine
                   ok(0, "\tat %u, explicit level %u, expected %u\n", i, explicit[i], test->explicit[i]);
               }
               else
                   ok(0, "\tat %u, explicit level %u, expected %u\n", i, explicit[i], test->explicit[i]);
            }
        }
    }

    match = !memcmp(resolved, test->resolved, len);
    if (!match) {
        if (test->todo) {
            failcount++;
        todo_wine
            ok(0, "test %u: %s wrong resolved levels:\n", seq, wine_dbgstr_w(test->text));
        }
        else
            ok(0, "test %u: %s wrong resolved levels:\n", seq, wine_dbgstr_w(test->text));

        for (i = 0; i < len; i++) {
            if (test->resolved[i] != resolved[i]) {
               if (test->todo) {
                   failcount++;
               todo_wine
                   ok(0, "\tat %u, resolved level %u, expected %u\n", i, resolved[i], test->resolved[i]);
               }
               else
                   ok(0, "\tat %u, resolved level %u, expected %u\n", i, resolved[i], test->resolved[i]);
            }
        }
    }

    todo_wine_if(test->todo && failcount == 0)
        ok(1, "test %u: marked as \"todo_wine\" but succeeds\n", seq);
}

static void test_AnalyzeBidi(void)
{
    const struct bidi_test *ptr = bidi_tests;
    IDWriteTextAnalyzer *analyzer;
    UINT32 i = 0;
    HRESULT hr;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer);
    ok(!!analyzer, "Failed to create analyzer instance.\n");

    while (*ptr->text)
    {
        UINT32 len;

        init_textsource(&analysissource, ptr->text, -1, ptr->direction);

        len = lstrlenW(ptr->text);
        if (len > BIDI_LEVELS_COUNT) {
            ok(0, "test %u: increase BIDI_LEVELS_COUNT to at least %u\n", i, len);
            i++;
            ptr++;
            continue;
        }

        memset(g_explicit_levels, 0, sizeof(g_explicit_levels));
        memset(g_resolved_levels, 0, sizeof(g_resolved_levels));
        hr = IDWriteTextAnalyzer_AnalyzeBidi(analyzer, &analysissource.IDWriteTextAnalysisSource_iface, 0,
            len, &analysissink);
        ok(hr == S_OK, "%u: unexpected hr %#lx.\n", i, hr);
        compare_bidi_levels(i, ptr, len, g_explicit_levels, g_resolved_levels);

        i++;
        ptr++;
    }

    IDWriteTextAnalyzer_Release(analyzer);
}

enum script_id
{
    Script_Unknown = 0,
    Script_Arabic = 3,
    Script_Latin = 49,
};

static void test_glyph_justification_property(void)
{
    static const struct
    {
        enum script_id script;
        const WCHAR *text;
        unsigned short justification[10];
    } tests[] =
    {
        {
            Script_Latin,
            L"a b\tc",
            {
                SCRIPT_JUSTIFY_CHARACTER,
                SCRIPT_JUSTIFY_BLANK,
                SCRIPT_JUSTIFY_CHARACTER,
                SCRIPT_JUSTIFY_BLANK,
                SCRIPT_JUSTIFY_CHARACTER,
            },
        },
        {
            Script_Latin,
            L" a b",
            {
                SCRIPT_JUSTIFY_BLANK,
                SCRIPT_JUSTIFY_CHARACTER,
                SCRIPT_JUSTIFY_BLANK,
                SCRIPT_JUSTIFY_CHARACTER,
            },
        },
        {
            Script_Arabic,
            L" a b",
            {
                SCRIPT_JUSTIFY_ARABIC_BLANK,
                SCRIPT_JUSTIFY_NONE,
                SCRIPT_JUSTIFY_ARABIC_BLANK,
                SCRIPT_JUSTIFY_NONE,
            },
        },
        { Script_Unknown, L"a", { SCRIPT_JUSTIFY_CHARACTER } },
        { Script_Latin, L"\x640", { SCRIPT_JUSTIFY_CHARACTER } },
        { Script_Arabic, L"\x640", { SCRIPT_JUSTIFY_ARABIC_KASHIDA } },

        { Script_Arabic, L"\x633\x627", { SCRIPT_JUSTIFY_ARABIC_SEEN, SCRIPT_JUSTIFY_ARABIC_ALEF } },
        { Script_Arabic, L"\x633\x625", { SCRIPT_JUSTIFY_ARABIC_SEEN, SCRIPT_JUSTIFY_ARABIC_ALEF } },
        { Script_Arabic, L"\x633\x623", { SCRIPT_JUSTIFY_ARABIC_SEEN, SCRIPT_JUSTIFY_ARABIC_ALEF } },
        { Script_Arabic, L"\x633\x622", { SCRIPT_JUSTIFY_ARABIC_SEEN, SCRIPT_JUSTIFY_ARABIC_ALEF } },

        { Script_Arabic, L"\x644\x647", { SCRIPT_JUSTIFY_NONE, SCRIPT_JUSTIFY_ARABIC_HA } },

        { Script_Arabic, L"\x628\x631", { SCRIPT_JUSTIFY_NONE, SCRIPT_JUSTIFY_ARABIC_BARA } },
        { Script_Arabic, L"\x645\x631", { SCRIPT_JUSTIFY_NONE, SCRIPT_JUSTIFY_ARABIC_BARA } },
        { Script_Arabic, L"\x645\x632", { SCRIPT_JUSTIFY_NONE, SCRIPT_JUSTIFY_ARABIC_BARA } },

        { Script_Arabic, L"\x644\x633\x645", { SCRIPT_JUSTIFY_NONE, SCRIPT_JUSTIFY_ARABIC_SEEN_M, SCRIPT_JUSTIFY_ARABIC_NORMAL } },
    };
    DWRITE_SHAPING_GLYPH_PROPERTIES glyph_props[16];
    DWRITE_SHAPING_TEXT_PROPERTIES text_props[16];
    UINT16 clustermap[16], glyphs[16];
    IDWriteTextAnalyzer *analyzer;
    DWRITE_SCRIPT_ANALYSIS sa;
    IDWriteFontFace *fontface;
    UINT32 glyph_count;
    unsigned int i, j;
    HRESULT hr;

    analyzer = create_text_analyzer(&IID_IDWriteTextAnalyzer);
    ok(!!analyzer, "Failed to create analyzer instance.\n");

    fontface = create_fontface();

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        if (tests[i].script == Script_Arabic && !strcmp(winetest_platform, "wine"))
            continue;

        winetest_push_context("Test %s", debugstr_w(tests[i].text));

        sa.script = tests[i].script;
        sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;

        /* Use RTL for Arabic, it affects returned justification classes. */
        hr = IDWriteTextAnalyzer_GetGlyphs(analyzer, tests[i].text, wcslen(tests[i].text), fontface,
                FALSE, sa.script == Script_Arabic, &sa, L"en-US", NULL, NULL, NULL, 0, ARRAY_SIZE(glyphs), clustermap,
                text_props, glyphs, glyph_props, &glyph_count);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        for (j = 0; j < glyph_count; ++j)
        {
            winetest_push_context("Glyph %u", j);
            ok(glyph_props[j].justification == tests[i].justification[j], "Unexpected justification value %u.\n",
                    glyph_props[j].justification);
            winetest_pop_context();
        }

        winetest_pop_context();
    }

    IDWriteFontFace_Release(fontface);
}

START_TEST(analyzer)
{
    HRESULT hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr != S_OK)
    {
        win_skip("failed to create factory\n");
        return;
    }

    init_call_sequences(sequences, NUM_CALL_SEQUENCES);
    init_call_sequences(expected_seq, 1);

    test_AnalyzeScript();
    test_AnalyzeLineBreakpoints();
    test_AnalyzeBidi();
    test_GetScriptProperties();
    test_GetTextComplexity();
    test_GetGlyphs();
    test_numbersubstitution();
    test_GetTypographicFeatures();
    test_GetGlyphPlacements();
    test_ApplyCharacterSpacing();
    test_GetGlyphOrientationTransform();
    test_GetBaseline();
    test_GetGdiCompatibleGlyphPlacements();
    test_glyph_justification_property();

    IDWriteFactory_Release(factory);
}
