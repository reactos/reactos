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

#include <math.h>

#include <wine/test.h>
#include <wingdi.h>
#include <objbase.h>
#include <gdiplus.h>

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf_(expected, got, precision) ok(fabs((expected) - (got)) <= (precision), "Expected %f, got %f\n", (expected), (got))
#define expectf(expected, got) expectf_((expected), (got), 0.001)

static BOOL save_metafiles;

typedef struct emfplus_record
{
    BOOL  todo;
    ULONG record_type;
    BOOL  playback_todo;
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
                    actual.todo = FALSE;
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
        actual.todo = FALSE;
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

    actual.todo = FALSE;
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

static void save_metafile(GpMetafile *metafile, const char *filename)
{
    if (save_metafiles)
    {
        GpMetafile *clone;
        HENHMETAFILE hemf;
        GpStatus stat;

        stat = GdipCloneImage((GpImage*)metafile, (GpImage**)&clone);
        expect(Ok, stat);

        stat = GdipGetHemfFromMetafile(clone, &hemf);
        expect(Ok, stat);

        DeleteEnhMetaFile(CopyEnhMetaFileA(hemf, filename));

        DeleteEnhMetaFile(hemf);

        stat = GdipDisposeImage((GpImage*)clone);
        expect(Ok, stat);
    }
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
    GpRectF bounds;
    GpUnit unit;
    REAL xres, yres;
    HENHMETAFILE hemf, dummy;
    MetafileHeader header;
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

    save_metafile(metafile, "empty.emf");

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(100.0, bounds.Width, 0.05);
    expectf_(100.0, bounds.Height, 0.05);
    expect(UnitPixel, unit);

    stat = GdipGetImageHorizontalResolution((GpImage*)metafile, &xres);
    expect(Ok, stat);

    stat = GdipGetImageVerticalResolution((GpImage*)metafile, &yres);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &dummy);
    expect(InvalidParameter, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    check_emfplus(hemf, empty_records, "empty emf");

    memset(&header, 0xaa, sizeof(header));
    stat = GdipGetMetafileHeaderFromEmf(hemf, &header);
    expect(Ok, stat);
    expect(MetafileTypeEmfPlusOnly, header.Type);
    expect(U(header).EmfHeader.nBytes, header.Size);
    ok(header.Version == 0xdbc01001 || header.Version == 0xdbc01002, "Unexpected version %x\n", header.Version);
    expect(1, header.EmfPlusFlags); /* reference device was display, not printer */
    expectf(xres, header.DpiX);
    expectf(xres, U(header).EmfHeader.szlDevice.cx / (REAL)U(header).EmfHeader.szlMillimeters.cx * 25.4);
    expectf(yres, header.DpiY);
    expectf(yres, U(header).EmfHeader.szlDevice.cy / (REAL)U(header).EmfHeader.szlMillimeters.cy * 25.4);
    expect(0, header.X);
    expect(0, header.Y);
    expect(100, header.Width);
    expect(100, header.Height);
    expect(28, header.EmfPlusHeaderSize);
    expect(96, header.LogicalDpiX);
    expect(96, header.LogicalDpiX);
    expect(EMR_HEADER, U(header).EmfHeader.iType);
    expect(0, U(header).EmfHeader.rclBounds.left);
    expect(0, U(header).EmfHeader.rclBounds.top);
    expect(-1, U(header).EmfHeader.rclBounds.right);
    expect(-1, U(header).EmfHeader.rclBounds.bottom);
    expect(0, U(header).EmfHeader.rclFrame.left);
    expect(0, U(header).EmfHeader.rclFrame.top);
    expectf_(100.0, U(header).EmfHeader.rclFrame.right * xres / 2540.0, 2.0);
    expectf_(100.0, U(header).EmfHeader.rclFrame.bottom * yres / 2540.0, 2.0);

    stat = GdipCreateMetafileFromEmf(hemf, TRUE, &metafile);
    expect(Ok, stat);

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(100.0, bounds.Width, 0.05);
    expectf_(100.0, bounds.Height, 0.05);
    expect(UnitPixel, unit);

    stat = GdipGetImageHorizontalResolution((GpImage*)metafile, &xres);
    expect(Ok, stat);
    expectf(header.DpiX, xres);

    stat = GdipGetImageVerticalResolution((GpImage*)metafile, &yres);
    expect(Ok, stat);
    expectf(header.DpiY, yres);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
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

    save_metafile(metafile, "getdc.emf");

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
    GpRectF bounds;
    GpUnit unit;
    REAL xres, yres;
    HENHMETAFILE hemf;
    MetafileHeader header;
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

    save_metafile(metafile, "emfonly.emf");

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(100.0, bounds.Width, 0.05);
    expectf_(100.0, bounds.Height, 0.05);
    expect(UnitPixel, unit);

    stat = GdipGetImageHorizontalResolution((GpImage*)metafile, &xres);
    expect(Ok, stat);

    stat = GdipGetImageVerticalResolution((GpImage*)metafile, &yres);
    expect(Ok, stat);

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

    memset(&header, 0xaa, sizeof(header));
    stat = GdipGetMetafileHeaderFromEmf(hemf, &header);
    expect(Ok, stat);
    expect(MetafileTypeEmf, header.Type);
    expect(U(header).EmfHeader.nBytes, header.Size);
    expect(0x10000, header.Version);
    expect(0, header.EmfPlusFlags);
    expectf(xres, header.DpiX);
    expectf(xres, U(header).EmfHeader.szlDevice.cx / (REAL)U(header).EmfHeader.szlMillimeters.cx * 25.4);
    expectf(yres, header.DpiY);
    expectf(yres, U(header).EmfHeader.szlDevice.cy / (REAL)U(header).EmfHeader.szlMillimeters.cy * 25.4);
    expect(0, header.X);
    expect(0, header.Y);
    expect(100, header.Width);
    expect(100, header.Height);
    expect(0, header.EmfPlusHeaderSize);
    expect(0, header.LogicalDpiX);
    expect(0, header.LogicalDpiX);
    expect(EMR_HEADER, U(header).EmfHeader.iType);
    expect(25, U(header).EmfHeader.rclBounds.left);
    expect(25, U(header).EmfHeader.rclBounds.top);
    expect(74, U(header).EmfHeader.rclBounds.right);
    expect(74, U(header).EmfHeader.rclBounds.bottom);
    expect(0, U(header).EmfHeader.rclFrame.left);
    expect(0, U(header).EmfHeader.rclFrame.top);
    expectf_(100.0, U(header).EmfHeader.rclFrame.right * xres / 2540.0, 2.0);
    expectf_(100.0, U(header).EmfHeader.rclFrame.bottom * yres / 2540.0, 2.0);

    stat = GdipCreateMetafileFromEmf(hemf, TRUE, &metafile);
    expect(Ok, stat);

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(100.0, bounds.Width, 0.05);
    expectf_(100.0, bounds.Height, 0.05);
    expect(UnitPixel, unit);

    stat = GdipGetImageHorizontalResolution((GpImage*)metafile, &xres);
    expect(Ok, stat);
    expectf(header.DpiX, xres);

    stat = GdipGetImageVerticalResolution((GpImage*)metafile, &yres);
    expect(Ok, stat);
    expectf(header.DpiY, yres);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record fillrect_records[] = {
    {0, EMR_HEADER},
    {0, EmfPlusRecordTypeHeader},
    {0, EmfPlusRecordTypeFillRects},
    {0, EmfPlusRecordTypeEndOfFile},
    {0, EMR_EOF},
    {0}
};

static void test_fillrect(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc;
    HENHMETAFILE hemf;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    static const GpPointF dst_points_half[3] = {{0.0,0.0},{50.0,0.0},{0.0,50.0}};
    static const WCHAR description[] = {'w','i','n','e','t','e','s','t',0};
    GpBitmap *bitmap;
    ARGB color;
    GpBrush *brush;

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

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangleI(graphics, brush, 25, 25, 75, 75);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, fillrect_records, "fillrect metafile", dst_points, &frame, UnitPixel);

    save_metafile(metafile, "fillrect.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, fillrect_records, "fillrect playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 15, 15, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapSetPixel(bitmap, 50, 50, 0);
    expect(Ok, stat);

    play_metafile(metafile, graphics, fillrect_records, "fillrect playback", dst_points_half, &frame, UnitPixel);

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

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record pagetransform_records[] = {
    {0, EMR_HEADER},
    {0, EmfPlusRecordTypeHeader},
    {0, EmfPlusRecordTypeFillRects},
    {0, EmfPlusRecordTypeSetPageTransform},
    {0, EmfPlusRecordTypeFillRects},
    {0, EmfPlusRecordTypeSetPageTransform},
    {0, EmfPlusRecordTypeFillRects},
    {0, EmfPlusRecordTypeSetPageTransform},
    {0, EmfPlusRecordTypeFillRects},
    {0, EmfPlusRecordTypeSetPageTransform},
    {0, EmfPlusRecordTypeFillRects},
    {0, EmfPlusRecordTypeEndOfFile},
    {0, EMR_EOF},
    {0}
};

static void test_pagetransform(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc;
    static const GpRectF frame = {0.0, 0.0, 5.0, 5.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    static const WCHAR description[] = {'w','i','n','e','t','e','s','t',0};
    GpBitmap *bitmap;
    ARGB color;
    GpBrush *brush;
    GpUnit unit;
    REAL scale, dpix, dpiy;
    UINT width, height;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitInch, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetImageHorizontalResolution((GpImage*)metafile, &dpix);
    todo_wine expect(InvalidParameter, stat);

    stat = GdipGetImageVerticalResolution((GpImage*)metafile, &dpiy);
    todo_wine expect(InvalidParameter, stat);

    stat = GdipGetImageWidth((GpImage*)metafile, &width);
    todo_wine expect(InvalidParameter, stat);

    stat = GdipGetImageHeight((GpImage*)metafile, &height);
    todo_wine expect(InvalidParameter, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    /* initial scale */
    stat = GdipGetPageUnit(graphics, &unit);
    expect(Ok, stat);
    expect(UnitDisplay, unit);

    stat = GdipGetPageScale(graphics, &scale);
    expect(Ok, stat);
    expectf(1.0, scale);

    stat = GdipGetDpiX(graphics, &dpix);
    expect(Ok, stat);
    expectf(96.0, dpix);

    stat = GdipGetDpiY(graphics, &dpiy);
    expect(Ok, stat);
    expectf(96.0, dpiy);

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangleI(graphics, brush, 1, 2, 1, 1);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* page unit = pixels */
    stat = GdipSetPageUnit(graphics, UnitPixel);
    expect(Ok, stat);

    stat = GdipGetPageUnit(graphics, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);

    stat = GdipCreateSolidFill((ARGB)0xff00ff00, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangleI(graphics, brush, 0, 1, 1, 1);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* page scale = 3, unit = pixels */
    stat = GdipSetPageScale(graphics, 3.0);
    expect(Ok, stat);

    stat = GdipGetPageScale(graphics, &scale);
    expect(Ok, stat);
    expectf(3.0, scale);

    stat = GdipCreateSolidFill((ARGB)0xff00ffff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangleI(graphics, brush, 0, 1, 2, 2);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* page scale = 3, unit = inches */
    stat = GdipSetPageUnit(graphics, UnitInch);
    expect(Ok, stat);

    stat = GdipGetPageUnit(graphics, &unit);
    expect(Ok, stat);
    expect(UnitInch, unit);

    stat = GdipCreateSolidFill((ARGB)0xffff0000, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 1.0/96.0, 0, 1, 1);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* page scale = 3, unit = display */
    stat = GdipSetPageUnit(graphics, UnitDisplay);
    expect(Ok, stat);

    stat = GdipGetPageUnit(graphics, &unit);
    expect(Ok, stat);
    expect(UnitDisplay, unit);

    stat = GdipCreateSolidFill((ARGB)0xffff00ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 3, 3, 2, 2);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, pagetransform_records, "pagetransform metafile", dst_points, &frame, UnitPixel);

    save_metafile(metafile, "pagetransform.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, pagetransform_records, "pagetransform playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 30, 50, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapGetPixel(bitmap, 10, 30, &color);
    expect(Ok, stat);
    expect(0xff00ff00, color);

    stat = GdipBitmapGetPixel(bitmap, 20, 80, &color);
    expect(Ok, stat);
    expect(0xff00ffff, color);

    stat = GdipBitmapGetPixel(bitmap, 80, 20, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 80, 80, &color);
    expect(Ok, stat);
    expect(0xffff00ff, color);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static void test_converttoemfplus(void)
{
    GpStatus (WINAPI *pGdipConvertToEmfPlus)( const GpGraphics *graphics, GpMetafile *metafile, BOOL *succ,
              EmfType emfType, const WCHAR *description, GpMetafile **outmetafile);
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const WCHAR description[] = {'w','i','n','e','t','e','s','t',0};
    GpStatus stat;
    GpMetafile *metafile, *metafile2 = NULL, *emhmeta;
    GpGraphics *graphics;
    HDC hdc;
    BOOL succ;
    HMODULE mod = GetModuleHandleA("gdiplus.dll");

    pGdipConvertToEmfPlus = (void*)GetProcAddress( mod, "GdipConvertToEmfPlus");
    if(!pGdipConvertToEmfPlus)
    {
        /* GdipConvertToEmfPlus was introduced in Windows Vista. */
        win_skip("GDIPlus version 1.1 not available\n");
        return;
    }

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, MetafileTypeEmf, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &emhmeta);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    /* Invalid Parameters */
    stat = pGdipConvertToEmfPlus(NULL, metafile, &succ, EmfTypeEmfPlusOnly, description, &metafile2);
    expect(InvalidParameter, stat);

    stat = pGdipConvertToEmfPlus(graphics, NULL, &succ, EmfTypeEmfPlusOnly, description, &metafile2);
    expect(InvalidParameter, stat);

    stat = pGdipConvertToEmfPlus(graphics, metafile, &succ, EmfTypeEmfPlusOnly, description, NULL);
    expect(InvalidParameter, stat);

    stat = pGdipConvertToEmfPlus(graphics, metafile, NULL, MetafileTypeInvalid, NULL, &metafile2);
    expect(InvalidParameter, stat);

    stat = pGdipConvertToEmfPlus(graphics, metafile, NULL, MetafileTypeEmfPlusDual+1, NULL, &metafile2);
    expect(InvalidParameter, stat);

    /* If we are already an Enhanced Metafile then the conversion fails. */
    stat = pGdipConvertToEmfPlus(graphics, emhmeta, NULL, EmfTypeEmfPlusOnly, NULL, &metafile2);
    todo_wine expect(InvalidParameter, stat);

    stat = pGdipConvertToEmfPlus(graphics, metafile, NULL, EmfTypeEmfPlusOnly, NULL, &metafile2);
    todo_wine expect(Ok, stat);
    if(metafile2)
        GdipDisposeImage((GpImage*)metafile2);

    succ = FALSE;
    stat = pGdipConvertToEmfPlus(graphics, metafile, &succ, EmfTypeEmfPlusOnly, NULL, &metafile2);
    todo_wine expect(Ok, stat);
    if(metafile2)
        GdipDisposeImage((GpImage*)metafile2);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)emhmeta);
    expect(Ok, stat);
}

START_TEST(metafile)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    int myARGC;
    char **myARGV;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    myARGC = winetest_get_mainargs( &myARGV );

    if (myARGC >= 3 && !strcmp(myARGV[2], "save"))
        save_metafiles = TRUE;

    test_empty();
    test_getdc();
    test_emfonly();
    test_fillrect();
    test_pagetransform();
    test_converttoemfplus();

    GdiplusShutdown(gdiplusToken);
}
