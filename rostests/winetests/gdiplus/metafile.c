/*
 * Unit test suite for metafiles
 *
 * Copyright (C) 2011 Vincent Povirk for CodeWeavers
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

//#include "windows.h"
#include <wine/test.h>
#include <wingdi.h>
#include <objbase.h>
#include <gdiplus.h>

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

typedef struct emfplus_record
{
    ULONG todo;
    ULONG record_type;
    ULONG playback_todo;
} emfplus_record;

typedef struct emfplus_check_state
{
    const char *desc;
    int count;
    const struct emfplus_record *expected;
    GpMetafile *metafile;
} emfplus_check_state;

static void check_record(int count, const char *desc, const struct emfplus_record *expected, const struct emfplus_record *actual)
{
    if (expected->todo)
        todo_wine ok(expected->record_type == actual->record_type,
            "%s.%i: Expected record type 0x%x, got 0x%x\n", desc, count,
            expected->record_type, actual->record_type);
    else
        ok(expected->record_type == actual->record_type,
            "%s.%i: Expected record type 0x%x, got 0x%x\n", desc, count,
            expected->record_type, actual->record_type);
}

typedef struct EmfPlusRecordHeader
{
    WORD Type;
    WORD Flags;
    DWORD Size;
    DWORD DataSize;
} EmfPlusRecordHeader;

static int CALLBACK enum_emf_proc(HDC hDC, HANDLETABLE *lpHTable, const ENHMETARECORD *lpEMFR,
    int nObj, LPARAM lpData)
{
    emfplus_check_state *state = (emfplus_check_state*)lpData;
    emfplus_record actual;

    if (lpEMFR->iType == EMR_GDICOMMENT)
    {
        const EMRGDICOMMENT *comment = (const EMRGDICOMMENT*)lpEMFR;

        if (comment->cbData >= 4 && memcmp(comment->Data, "EMF+", 4) == 0)
        {
            int offset = 4;

            while (offset + sizeof(EmfPlusRecordHeader) <= comment->cbData)
            {
                const EmfPlusRecordHeader *record = (const EmfPlusRecordHeader*)&comment->Data[offset];

                ok(record->Size == record->DataSize + sizeof(EmfPlusRecordHeader),
                    "%s: EMF+ record datasize %u and size %u mismatch\n", state->desc, record->DataSize, record->Size);

                ok(offset + record->DataSize <= comment->cbData,
                    "%s: EMF+ record truncated\n", state->desc);

                if (offset + record->DataSize > comment->cbData)
                    return 0;

                if (state->expected[state->count].record_type)
                {
                    actual.todo = 0;
                    actual.record_type = record->Type;

                    check_record(state->count, state->desc, &state->expected[state->count], &actual);

                    state->count++;
                }
                else
                {
                    ok(0, "%s: Unexpected EMF+ 0x%x record\n", state->desc, record->Type);
                }

                offset += record->Size;
            }

            ok(offset == comment->cbData, "%s: truncated EMF+ record data?\n", state->desc);

            return 1;
        }
    }

    if (state->expected[state->count].record_type)
    {
        actual.todo = 0;
        actual.record_type = lpEMFR->iType;

        check_record(state->count, state->desc, &state->expected[state->count], &actual);

        state->count++;
    }
    else
    {
        ok(0, "%s: Unexpected EMF 0x%x record\n", state->desc, lpEMFR->iType);
    }

    return 1;
}

static void check_emfplus(HENHMETAFILE hemf, const emfplus_record *expected, const char *desc)
{
    emfplus_check_state state;

    state.desc = desc;
    state.count = 0;
    state.expected = expected;

    EnumEnhMetaFile(0, hemf, enum_emf_proc, &state, NULL);

    if (expected[state.count].todo)
        todo_wine ok(expected[state.count].record_type == 0, "%s: Got %i records, expecting more\n", desc, state.count);
    else
        ok(expected[state.count].record_type == 0, "%s: Got %i records, expecting more\n", desc, state.count);
}

static BOOL CALLBACK enum_metafile_proc(EmfPlusRecordType record_type, unsigned int flags,
    unsigned int dataSize, const unsigned char *pStr, void *userdata)
{
    emfplus_check_state *state = (emfplus_check_state*)userdata;
    emfplus_record actual;

    actual.todo = 0;
    actual.record_type = record_type;

    if (dataSize == 0)
        ok(pStr == NULL, "non-NULL pStr\n");

    if (state->expected[state->count].record_type)
    {
        check_record(state->count, state->desc, &state->expected[state->count], &actual);

        state->count++;
    }
    else
    {
        ok(0, "%s: Unexpected EMF 0x%x record\n", state->desc, record_type);
    }

    return TRUE;
}

static void check_metafile(GpMetafile *metafile, const emfplus_record *expected, const char *desc,
    const GpPointF *dst_points, const GpRectF *src_rect, Unit src_unit)
{
    GpStatus stat;
    HDC hdc;
    GpGraphics *graphics;
    emfplus_check_state state;

    state.desc = desc;
    state.count = 0;
    state.expected = expected;
    state.metafile = metafile;

    hdc = CreateCompatibleDC(0);

    stat = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, stat);

    stat = GdipEnumerateMetafileSrcRectDestPoints(graphics, metafile, dst_points,
        3, src_rect, src_unit, enum_metafile_proc, &state, NULL);
    expect(Ok, stat);

    if (expected[state.count].todo)
        todo_wine ok(expected[state.count].record_type == 0, "%s: Got %i records, expecting more\n", desc, state.count);
    else
        ok(expected[state.count].record_type == 0, "%s: Got %i records, expecting more\n", desc, state.count);

    GdipDeleteGraphics(graphics);

    DeleteDC(hdc);
}

static BOOL CALLBACK play_metafile_proc(EmfPlusRecordType record_type, unsigned int flags,
    unsigned int dataSize, const unsigned char *pStr, void *userdata)
{
    emfplus_check_state *state = (emfplus_check_state*)userdata;
    GpStatus stat;

    stat = GdipPlayMetafileRecord(state->metafile, record_type, flags, dataSize, pStr);

    if (state->expected[state->count].record_type)
    {
        if (state->expected[state->count].playback_todo)
            todo_wine ok(stat == Ok, "%s.%i: GdipPlayMetafileRecord failed with stat %i\n", state->desc, state->count, stat);
        else
            ok(stat == Ok, "%s.%i: GdipPlayMetafileRecord failed with stat %i\n", state->desc, state->count, stat);
        state->count++;
    }
    else
    {
        if (state->expected[state->count].playback_todo)
            todo_wine ok(0, "%s: too many records\n", state->desc);
        else
            ok(0, "%s: too many records\n", state->desc);

        return FALSE;
    }

    return TRUE;
}

static void play_metafile(GpMetafile *metafile, GpGraphics *graphics, const emfplus_record *expected,
    const char *desc, const GpPointF *dst_points, const GpRectF *src_rect, Unit src_unit)
{
    GpStatus stat;
    emfplus_check_state state;

    state.desc = desc;
    state.count = 0;
    state.expected = expected;
    state.metafile = metafile;

    stat = GdipEnumerateMetafileSrcRectDestPoints(graphics, metafile, dst_points,
        3, src_rect, src_unit, play_metafile_proc, &state, NULL);
    expect(Ok, stat);
}

static const emfplus_record empty_records[] = {
    {0, EMR_HEADER},
    {0, EmfPlusRecordTypeHeader},
    {0, EmfPlusRecordTypeEndOfFile},
    {0, EMR_EOF},
    {0}
};

static void test_empty(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc;
    HENHMETAFILE hemf, dummy;
    BOOL ret;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    static const WCHAR description[] = {'w','i','n','e','t','e','s','t',0};

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(NULL, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, MetafileTypeInvalid, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, MetafileTypeWmf, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, MetafileTypeWmfPlaceable, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, MetafileTypeEmfPlusDual+1, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, NULL);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, empty_records, "empty metafile", dst_points, &frame, UnitPixel);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &dummy);
    expect(InvalidParameter, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    check_emfplus(hemf, empty_records, "empty emf");

    ret = DeleteEnhMetaFile(hemf);
    ok(ret != 0, "Failed to delete enhmetafile %p\n", hemf);
}

static const emfplus_record getdc_records[] = {
    {0, EMR_HEADER},
    {0, EmfPlusRecordTypeHeader},
    {0, EmfPlusRecordTypeGetDC},
    {0, EMR_CREATEBRUSHINDIRECT},
    {0, EMR_SELECTOBJECT},
    {0, EMR_RECTANGLE},
    {0, EMR_SELECTOBJECT},
    {0, EMR_DELETEOBJECT},
    {0, EmfPlusRecordTypeEndOfFile},
    {0, EMR_EOF},
    {0}
};

static void test_getdc(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc, metafile_dc;
    HENHMETAFILE hemf;
    BOOL ret;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    static const GpPointF dst_points_half[3] = {{0.0,0.0},{50.0,0.0},{0.0,50.0}};
    static const WCHAR description[] = {'w','i','n','e','t','e','s','t',0};
    HBRUSH hbrush, holdbrush;
    GpBitmap *bitmap;
    ARGB color;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipGetDC(graphics, &metafile_dc);
    expect(Ok, stat);

    if (stat != Ok)
    {
        GdipDeleteGraphics(graphics);
        GdipDisposeImage((GpImage*)metafile);
        return;
    }

    hbrush = CreateSolidBrush(0xff0000);

    holdbrush = SelectObject(metafile_dc, hbrush);

    Rectangle(metafile_dc, 25, 25, 75, 75);

    SelectObject(metafile_dc, holdbrush);

    DeleteObject(hbrush);

    stat = GdipReleaseDC(graphics, metafile_dc);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, getdc_records, "getdc metafile", dst_points, &frame, UnitPixel);

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, getdc_records, "getdc playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 15, 15, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapSetPixel(bitmap, 50, 50, 0);
    expect(Ok, stat);

    play_metafile(metafile, graphics, getdc_records, "getdc playback", dst_points_half, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 15, 15, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapSetPixel(bitmap, 15, 15, 0);
    expect(Ok, stat);

    stat = GdipDrawImagePointsRect(graphics, (GpImage*)metafile, dst_points, 3,
        0.0, 0.0, 100.0, 100.0, UnitPixel, NULL, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap, 15, 15, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    check_emfplus(hemf, getdc_records, "getdc emf");

    ret = DeleteEnhMetaFile(hemf);
    ok(ret != 0, "Failed to delete enhmetafile %p\n", hemf);
}

static const emfplus_record emfonly_records[] = {
    {0, EMR_HEADER},
    {0, EMR_CREATEBRUSHINDIRECT},
    {0, EMR_SELECTOBJECT},
    {0, EMR_RECTANGLE},
    {0, EMR_SELECTOBJECT},
    {0, EMR_DELETEOBJECT},
    {0, EMR_EOF},
    {0}
};

static void test_emfonly(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpImage *clone;
    GpGraphics *graphics;
    HDC hdc, metafile_dc;
    HENHMETAFILE hemf;
    BOOL ret;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    static const WCHAR description[] = {'w','i','n','e','t','e','s','t',0};
    HBRUSH hbrush, holdbrush;
    GpBitmap *bitmap;
    ARGB color;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipGetDC(graphics, &metafile_dc);
    expect(Ok, stat);

    if (stat != Ok)
    {
        GdipDeleteGraphics(graphics);
        GdipDisposeImage((GpImage*)metafile);
        return;
    }

    hbrush = CreateSolidBrush(0xff0000);

    holdbrush = SelectObject(metafile_dc, hbrush);

    Rectangle(metafile_dc, 25, 25, 75, 75);

    SelectObject(metafile_dc, holdbrush);

    DeleteObject(hbrush);

    stat = GdipReleaseDC(graphics, metafile_dc);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, emfonly_records, "emfonly metafile", dst_points, &frame, UnitPixel);

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, emfonly_records, "emfonly playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 15, 15, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapSetPixel(bitmap, 50, 50, 0);
    expect(Ok, stat);

    stat = GdipDrawImagePointsRect(graphics, (GpImage*)metafile, dst_points, 3,
        0.0, 0.0, 100.0, 100.0, UnitPixel, NULL, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap, 15, 15, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipCloneImage((GpImage*)metafile, &clone);
    expect(Ok, stat);

    if (stat == Ok)
    {
        stat = GdipBitmapSetPixel(bitmap, 50, 50, 0);
        expect(Ok, stat);

        stat = GdipDrawImagePointsRect(graphics, clone, dst_points, 3,
            0.0, 0.0, 100.0, 100.0, UnitPixel, NULL, NULL, NULL);
        expect(Ok, stat);

        stat = GdipBitmapGetPixel(bitmap, 15, 15, &color);
        expect(Ok, stat);
        expect(0, color);

        stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
        expect(Ok, stat);
        expect(0xff0000ff, color);

        GdipDisposeImage(clone);
    }

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    check_emfplus(hemf, emfonly_records, "emfonly emf");

    ret = DeleteEnhMetaFile(hemf);
    ok(ret != 0, "Failed to delete enhmetafile %p\n", hemf);
}

START_TEST(metafile)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_empty();
    test_getdc();
    test_emfonly();

    GdiplusShutdown(gdiplusToken);
}
