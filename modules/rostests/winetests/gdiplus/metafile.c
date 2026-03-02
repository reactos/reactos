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

#include <math.h>

#include "objbase.h"
#include "gdiplus.h"
#include "winspool.h"
#include "wine/test.h"

#define expect(expected,got) expect_(__LINE__, expected, got)
static inline void expect_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %.8ld, got %.8ld\n", expected, got);
}
#define expectf_(expected, got, precision) ok(fabs((expected) - (got)) <= (precision), "Expected %f, got %f\n", (expected), (got))
#define expectf(expected, got) expectf_((expected), (got), 0.001)

static BOOL save_metafiles;
static BOOL load_metafiles;

static const WCHAR description[] = L"winetest";

typedef struct emfplus_record
{
    DWORD record_type;
    DWORD flags; /* Used for EMF+ records only. */
    BOOL  todo;
    BOOL  playback_todo;
    void (*playback_fn)(GpMetafile* metafile, EmfPlusRecordType record_type,
        unsigned int flags, unsigned int dataSize, const unsigned char *pStr);
    DWORD broken_flags;
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
    if (actual->record_type > GDIP_EMFPLUS_RECORD_BASE)
    {
    todo_wine_if (expected->todo)
        ok(expected->record_type == actual->record_type && (expected->flags == actual->flags ||
            broken(expected->broken_flags == actual->flags)),
            "%s.%i: Expected record type 0x%lx, got 0x%lx. Expected flags %#lx, got %#lx.\n", desc, count,
            expected->record_type, actual->record_type, expected->flags, actual->flags);
    }
    else
    {
    todo_wine_if (expected->todo)
        ok(expected->record_type == actual->record_type,
            "%s.%i: Expected record type 0x%lx, got 0x%lx.\n", desc, count,
            expected->record_type, actual->record_type);
    }
}

typedef struct EmfPlusRecordHeader
{
    WORD Type;
    WORD Flags;
    DWORD Size;
    DWORD DataSize;
} EmfPlusRecordHeader;

typedef enum
{
    ObjectTypeInvalid,
    ObjectTypeBrush,
    ObjectTypePen,
    ObjectTypePath,
    ObjectTypeRegion,
    ObjectTypeImage,
    ObjectTypeFont,
    ObjectTypeStringFormat,
    ObjectTypeImageAttributes,
    ObjectTypeCustomLineCap,
} ObjectType;

typedef enum
{
    ImageDataTypeUnknown,
    ImageDataTypeBitmap,
    ImageDataTypeMetafile,
} ImageDataType;

typedef struct
{
    EmfPlusRecordHeader Header;
    /* EmfPlusImage */
    DWORD Version;
    ImageDataType Type;
    /* EmfPlusMetafile */
    DWORD MetafileType;
    DWORD MetafileDataSize;
    BYTE MetafileData[1];
} MetafileImageObject;

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
                    "%s: EMF+ record datasize %lu and size %lu mismatch\n", state->desc, record->DataSize, record->Size);

                ok(offset + record->DataSize <= comment->cbData,
                    "%s: EMF+ record truncated\n", state->desc);

                if (offset + record->DataSize > comment->cbData)
                    return 0;

                if (state->expected[state->count].record_type)
                {
                    actual.todo = FALSE;
                    actual.record_type = record->Type;
                    actual.flags = record->Flags;

                    check_record(state->count, state->desc, &state->expected[state->count], &actual);
                    state->count++;

                    if (state->expected[state->count-1].todo && state->expected[state->count-1].record_type != actual.record_type)
                        continue;
                }
                else
                {
                    ok(0, "%s: Unexpected EMF+ 0x%x record\n", state->desc, record->Type);
                }

                if ((record->Flags >> 8) == ObjectTypeImage && record->Type == EmfPlusRecordTypeObject)
                {
                    const MetafileImageObject *image = (const MetafileImageObject*)record;

                    if (image->Type == ImageDataTypeMetafile)
                    {
                        HENHMETAFILE hemf = SetEnhMetaFileBits(image->MetafileDataSize, image->MetafileData);
                        ok(hemf != NULL, "%s: SetEnhMetaFileBits failed\n", state->desc);

                        EnumEnhMetaFile(0, hemf, enum_emf_proc, state, NULL);
                        DeleteEnhMetaFile(hemf);
                    }
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
        actual.flags = 0;

        check_record(state->count, state->desc, &state->expected[state->count], &actual);

        state->count++;
    }
    else
    {
        ok(0, "%s: Unexpected EMF 0x%lx record\n", state->desc, lpEMFR->iType);
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

    todo_wine_if (expected[state.count].todo)
        ok(expected[state.count].record_type == 0, "%s: Got %i records, expecting more\n", desc, state.count);
}

static BOOL CALLBACK enum_metafile_proc(EmfPlusRecordType record_type, unsigned int flags,
    unsigned int dataSize, const unsigned char *pStr, void *userdata)
{
    emfplus_check_state *state = (emfplus_check_state*)userdata;
    emfplus_record actual;

    actual.todo = FALSE;
    actual.record_type = record_type;
    actual.flags = flags;

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

    todo_wine_if (expected[state.count].todo)
        ok(expected[state.count].record_type == 0, "%s: Got %i records, expecting more\n", desc, state.count);

    GdipDeleteGraphics(graphics);

    DeleteDC(hdc);
}

static BOOL CALLBACK play_metafile_proc(EmfPlusRecordType record_type, unsigned int flags,
    unsigned int dataSize, const unsigned char *pStr, void *userdata)
{
    emfplus_check_state *state = (emfplus_check_state*)userdata;
    GpStatus stat;

    if (state->expected[state->count].record_type)
    {
        BOOL match = (state->expected[state->count].record_type == record_type);

        if (match && state->expected[state->count].playback_fn)
            state->expected[state->count].playback_fn(state->metafile, record_type, flags, dataSize, pStr);
        else
        {
            stat = GdipPlayMetafileRecord(state->metafile, record_type, flags, dataSize, pStr);
            todo_wine_if (state->expected[state->count].playback_todo)
                ok(stat == Ok, "%s.%i: GdipPlayMetafileRecord failed with stat %i\n", state->desc, state->count, stat);
        }

        todo_wine_if (state->expected[state->count].todo)
            ok(state->expected[state->count].record_type == record_type,
                "%s.%i: expected record type 0x%lx, got 0x%x\n", state->desc, state->count,
                state->expected[state->count].record_type, record_type);
        state->count++;
    }
    else
    {
        todo_wine_if (state->expected[state->count].playback_todo)
            ok(0, "%s: unexpected record 0x%x\n", state->desc, record_type);

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

/* When 'save' or 'load' is specified on the command line, save or
 * load the specified filename. */
static void sync_metafile(GpMetafile **metafile, const char *filename)
{
    GpStatus stat;
    if (save_metafiles)
    {
        GpMetafile *clone;
        HENHMETAFILE hemf;

        stat = GdipCloneImage((GpImage*)*metafile, (GpImage**)&clone);
        expect(Ok, stat);

        stat = GdipGetHemfFromMetafile(clone, &hemf);
        expect(Ok, stat);

        DeleteEnhMetaFile(CopyEnhMetaFileA(hemf, filename));

        DeleteEnhMetaFile(hemf);

        stat = GdipDisposeImage((GpImage*)clone);
        expect(Ok, stat);
    }
    else if (load_metafiles)
    {
        HENHMETAFILE hemf;

        stat = GdipDisposeImage((GpImage*)*metafile);
        expect(Ok, stat);
        *metafile = NULL;

        hemf = GetEnhMetaFileA(filename);
        ok(hemf != NULL, "%s could not be opened\n", filename);

        stat = GdipCreateMetafileFromEmf(hemf, TRUE, metafile);
        expect(Ok, stat);
    }
}

static const emfplus_record empty_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
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
    UINT limit_dpi;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(NULL, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, (EmfType)MetafileTypeInvalid, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, (EmfType)MetafileTypeWmf, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, (EmfType)MetafileTypeWmfPlaceable, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusDual+1, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, NULL);
    expect(InvalidParameter, stat);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetMetafileDownLevelRasterizationLimit(metafile, NULL);
    expect(InvalidParameter, stat);

    stat = GdipGetMetafileDownLevelRasterizationLimit(NULL, &limit_dpi);
    expect(InvalidParameter, stat);

    limit_dpi = 0xdeadbeef;
    stat = GdipGetMetafileDownLevelRasterizationLimit(metafile, &limit_dpi);
    expect(Ok, stat);
    ok(limit_dpi == 96, "limit_dpi was %d\n", limit_dpi);

    stat = GdipSetMetafileDownLevelRasterizationLimit(metafile, 255);
    expect(Ok, stat);

    limit_dpi = 0xdeadbeef;
    stat = GdipGetMetafileDownLevelRasterizationLimit(metafile, &limit_dpi);
    expect(Ok, stat);
    ok(limit_dpi == 255, "limit_dpi was %d\n", limit_dpi);

    stat = GdipSetMetafileDownLevelRasterizationLimit(metafile, 0);
    expect(Ok, stat);

    limit_dpi = 0xdeadbeef;
    stat = GdipGetMetafileDownLevelRasterizationLimit(metafile, &limit_dpi);
    expect(Ok, stat);
    ok(limit_dpi == 96, "limit_dpi was %d\n", limit_dpi);

    stat = GdipSetMetafileDownLevelRasterizationLimit(metafile, 1);
    expect(InvalidParameter, stat);

    stat = GdipSetMetafileDownLevelRasterizationLimit(metafile, 9);
    expect(InvalidParameter, stat);

    stat = GdipSetMetafileDownLevelRasterizationLimit(metafile, 10);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    limit_dpi = 0xdeadbeef;
    stat = GdipGetMetafileDownLevelRasterizationLimit(metafile, &limit_dpi);
    expect(WrongState, stat);
    expect(0xdeadbeef, limit_dpi);

    stat = GdipSetMetafileDownLevelRasterizationLimit(metafile, 200);
    expect(WrongState, stat);

    check_metafile(metafile, empty_records, "empty metafile", dst_points, &frame, UnitPixel);

    sync_metafile(&metafile, "empty.emf");

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

    memset(&header, 0xaa, sizeof(header));
    stat = GdipGetMetafileHeaderFromMetafile(metafile, &header);
    expect(Ok, stat);
    expect(MetafileTypeEmfPlusOnly, header.Type);
    expect(header.EmfHeader.nBytes, header.Size);
    ok(header.Version == 0xdbc01001 || header.Version == 0xdbc01002, "Unexpected version %x\n", header.Version);
    expect(1, header.EmfPlusFlags); /* reference device was display, not printer */
    expectf(xres, header.DpiX);
    expectf(xres, header.EmfHeader.szlDevice.cx / (REAL)header.EmfHeader.szlMillimeters.cx * 25.4);
    expectf(yres, header.DpiY);
    expectf(yres, header.EmfHeader.szlDevice.cy / (REAL)header.EmfHeader.szlMillimeters.cy * 25.4);
    expect(0, header.X);
    expect(0, header.Y);
    expect(100, header.Width);
    expect(100, header.Height);
    expect(28, header.EmfPlusHeaderSize);
    expect(96, header.LogicalDpiX);
    expect(96, header.LogicalDpiY);
    expect(EMR_HEADER, header.EmfHeader.iType);
    expect(0, header.EmfHeader.rclBounds.left);
    expect(0, header.EmfHeader.rclBounds.top);
    expect(-1, header.EmfHeader.rclBounds.right);
    expect(-1, header.EmfHeader.rclBounds.bottom);
    expect(0, header.EmfHeader.rclFrame.left);
    expect(0, header.EmfHeader.rclFrame.top);
    expectf_(100.0, header.EmfHeader.rclFrame.right * xres / 2540.0, 2.0);
    expectf_(100.0, header.EmfHeader.rclFrame.bottom * yres / 2540.0, 2.0);

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
    expect(header.EmfHeader.nBytes, header.Size);
    ok(header.Version == 0xdbc01001 || header.Version == 0xdbc01002, "Unexpected version %x\n", header.Version);
    expect(1, header.EmfPlusFlags); /* reference device was display, not printer */
    expectf(xres, header.DpiX);
    expectf(xres, header.EmfHeader.szlDevice.cx / (REAL)header.EmfHeader.szlMillimeters.cx * 25.4);
    expectf(yres, header.DpiY);
    expectf(yres, header.EmfHeader.szlDevice.cy / (REAL)header.EmfHeader.szlMillimeters.cy * 25.4);
    expect(0, header.X);
    expect(0, header.Y);
    expect(100, header.Width);
    expect(100, header.Height);
    expect(28, header.EmfPlusHeaderSize);
    expect(96, header.LogicalDpiX);
    expect(96, header.LogicalDpiY);
    expect(EMR_HEADER, header.EmfHeader.iType);
    expect(0, header.EmfHeader.rclBounds.left);
    expect(0, header.EmfHeader.rclBounds.top);
    expect(-1, header.EmfHeader.rclBounds.right);
    expect(-1, header.EmfHeader.rclBounds.bottom);
    expect(0, header.EmfHeader.rclFrame.left);
    expect(0, header.EmfHeader.rclFrame.top);
    expectf_(100.0, header.EmfHeader.rclFrame.right * xres / 2540.0, 2.0);
    expectf_(100.0, header.EmfHeader.rclFrame.bottom * yres / 2540.0, 2.0);

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

    memset(&header, 0xaa, sizeof(header));
    stat = GdipGetMetafileHeaderFromMetafile(metafile, &header);
    expect(Ok, stat);
    expect(MetafileTypeEmfPlusOnly, header.Type);
    expect(header.EmfHeader.nBytes, header.Size);
    ok(header.Version == 0xdbc01001 || header.Version == 0xdbc01002, "Unexpected version %x\n", header.Version);
    expect(1, header.EmfPlusFlags); /* reference device was display, not printer */
    expectf(xres, header.DpiX);
    expectf(xres, header.EmfHeader.szlDevice.cx / (REAL)header.EmfHeader.szlMillimeters.cx * 25.4);
    expectf(yres, header.DpiY);
    expectf(yres, header.EmfHeader.szlDevice.cy / (REAL)header.EmfHeader.szlMillimeters.cy * 25.4);
    expect(0, header.X);
    expect(0, header.Y);
    expect(100, header.Width);
    expect(100, header.Height);
    expect(28, header.EmfPlusHeaderSize);
    expect(96, header.LogicalDpiX);
    expect(96, header.LogicalDpiY);
    expect(EMR_HEADER, header.EmfHeader.iType);
    expect(0, header.EmfHeader.rclBounds.left);
    expect(0, header.EmfHeader.rclBounds.top);
    expect(-1, header.EmfHeader.rclBounds.right);
    expect(-1, header.EmfHeader.rclBounds.bottom);
    expect(0, header.EmfHeader.rclFrame.left);
    expect(0, header.EmfHeader.rclFrame.top);
    expectf_(100.0, header.EmfHeader.rclFrame.right * xres / 2540.0, 2.0);
    expectf_(100.0, header.EmfHeader.rclFrame.bottom * yres / 2540.0, 2.0);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record getdc_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeGetDC },
    { EMR_CREATEBRUSHINDIRECT },
    { EMR_SELECTOBJECT },
    { EMR_RECTANGLE },
    { EMR_SELECTOBJECT },
    { EMR_DELETEOBJECT },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
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

    sync_metafile(&metafile, "getdc.emf");

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
    { EMR_HEADER },
    { EMR_CREATEBRUSHINDIRECT },
    { EMR_SELECTOBJECT },
    { EMR_RECTANGLE },
    { EMR_SELECTOBJECT },
    { EMR_DELETEOBJECT },
    { EMR_EOF },
    { 0 }
};

static const emfplus_record emfonly_draw_records[] = {
    { EMR_HEADER },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_SETMITERLIMIT, 0, 1 },
    { EMR_MODIFYWORLDTRANSFORM, 0, 1 },
    { EMR_EXTCREATEPEN, 0, 1 },
    { EMR_SELECTOBJECT, 0, 1 },
    { EMR_SELECTOBJECT, 0, 1 },
    { EMR_POLYLINE16, 0, 1 },
    { EMR_SELECTOBJECT, 0, 1 },
    { EMR_SELECTOBJECT, 0, 1 },
    { EMR_MODIFYWORLDTRANSFORM, 0, 1 },
    { EMR_DELETEOBJECT, 0, 1 },
    { EMR_SETMITERLIMIT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EMR_EOF },
    { 0, 0, 1 }
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
    HBRUSH hbrush, holdbrush;
    GpBitmap *bitmap;
    ARGB color;
    GpPen *pen;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(InvalidParameter, stat);

    memset(&header, 0xaa, sizeof(header));
    stat = GdipGetMetafileHeaderFromMetafile(metafile, &header);
    expect(Ok, stat);
    expect(MetafileTypeEmf, header.Type);
    ok(header.Version == 0xdbc01001 || header.Version == 0xdbc01002, "Unexpected version %x\n", header.Version);
    /* The rest is zeroed or seemingly random/uninitialized garbage. */

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

    sync_metafile(&metafile, "emfonly.emf");

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

    memset(&header, 0xaa, sizeof(header));
    stat = GdipGetMetafileHeaderFromMetafile(metafile, &header);
    expect(Ok, stat);
    expect(MetafileTypeEmf, header.Type);
    expect(header.EmfHeader.nBytes, header.Size);
    /* For some reason a recoreded EMF Metafile has an EMF+ version. */
    todo_wine ok(header.Version == 0xdbc01001 || header.Version == 0xdbc01002, "Unexpected version %x\n", header.Version);
    expect(0, header.EmfPlusFlags);
    expectf(xres, header.DpiX);
    expectf(xres, header.EmfHeader.szlDevice.cx / (REAL)header.EmfHeader.szlMillimeters.cx * 25.4);
    expectf(yres, header.DpiY);
    expectf(yres, header.EmfHeader.szlDevice.cy / (REAL)header.EmfHeader.szlMillimeters.cy * 25.4);
    expect(0, header.X);
    expect(0, header.Y);
    expect(100, header.Width);
    expect(100, header.Height);
    expect(0, header.EmfPlusHeaderSize);
    expect(0, header.LogicalDpiX);
    expect(0, header.LogicalDpiY);
    expect(EMR_HEADER, header.EmfHeader.iType);
    expect(25, header.EmfHeader.rclBounds.left);
    expect(25, header.EmfHeader.rclBounds.top);
    expect(74, header.EmfHeader.rclBounds.right);
    expect(74, header.EmfHeader.rclBounds.bottom);
    expect(0, header.EmfHeader.rclFrame.left);
    expect(0, header.EmfHeader.rclFrame.top);
    expectf_(100.0, header.EmfHeader.rclFrame.right * xres / 2540.0, 2.0);
    expectf_(100.0, header.EmfHeader.rclFrame.bottom * yres / 2540.0, 2.0);

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
    expect(header.EmfHeader.nBytes, header.Size);
    expect(0x10000, header.Version);
    expect(0, header.EmfPlusFlags);
    expectf(xres, header.DpiX);
    expectf(xres, header.EmfHeader.szlDevice.cx / (REAL)header.EmfHeader.szlMillimeters.cx * 25.4);
    expectf(yres, header.DpiY);
    expectf(yres, header.EmfHeader.szlDevice.cy / (REAL)header.EmfHeader.szlMillimeters.cy * 25.4);
    expect(0, header.X);
    expect(0, header.Y);
    expect(100, header.Width);
    expect(100, header.Height);
    expect(0, header.EmfPlusHeaderSize);
    expect(0, header.LogicalDpiX);
    expect(0, header.LogicalDpiY);
    expect(EMR_HEADER, header.EmfHeader.iType);
    expect(25, header.EmfHeader.rclBounds.left);
    expect(25, header.EmfHeader.rclBounds.top);
    expect(74, header.EmfHeader.rclBounds.right);
    expect(74, header.EmfHeader.rclBounds.bottom);
    expect(0, header.EmfHeader.rclFrame.left);
    expect(0, header.EmfHeader.rclFrame.top);
    expectf_(100.0, header.EmfHeader.rclFrame.right * xres / 2540.0, 2.0);
    expectf_(100.0, header.EmfHeader.rclFrame.bottom * yres / 2540.0, 2.0);

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

    memset(&header, 0xaa, sizeof(header));
    stat = GdipGetMetafileHeaderFromMetafile(metafile, &header);
    expect(Ok, stat);
    expect(MetafileTypeEmf, header.Type);
    expect(header.EmfHeader.nBytes, header.Size);
    expect(0x10000, header.Version);
    expect(0, header.EmfPlusFlags);
    expectf(xres, header.DpiX);
    expectf(xres, header.EmfHeader.szlDevice.cx / (REAL)header.EmfHeader.szlMillimeters.cx * 25.4);
    expectf(yres, header.DpiY);
    expectf(yres, header.EmfHeader.szlDevice.cy / (REAL)header.EmfHeader.szlMillimeters.cy * 25.4);
    expect(0, header.X);
    expect(0, header.Y);
    expect(100, header.Width);
    expect(100, header.Height);
    expect(0, header.EmfPlusHeaderSize);
    expect(0, header.LogicalDpiX);
    expect(0, header.LogicalDpiY);
    expect(EMR_HEADER, header.EmfHeader.iType);
    expect(25, header.EmfHeader.rclBounds.left);
    expect(25, header.EmfHeader.rclBounds.top);
    expect(74, header.EmfHeader.rclBounds.right);
    expect(74, header.EmfHeader.rclBounds.bottom);
    expect(0, header.EmfHeader.rclFrame.left);
    expect(0, header.EmfHeader.rclFrame.top);
    expectf_(100.0, header.EmfHeader.rclFrame.right * xres / 2540.0, 2.0);
    expectf_(100.0, header.EmfHeader.rclFrame.bottom * yres / 2540.0, 2.0);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    /* test drawing to metafile with gdi+ functions */
    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, stat);
    stat = GdipDrawLineI(graphics, pen, 0, 0, 10, 10);
    todo_wine expect(Ok, stat);
    GdipDeletePen(pen);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, emfonly_draw_records, "emfonly draw metafile", dst_points, &frame, UnitPixel);
    sync_metafile(&metafile, "emfonly_draw.emf");

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record fillrect_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
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

    sync_metafile(&metafile, "fillrect.emf");

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

static const emfplus_record clear_emf_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeClear },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_clear(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc;
    HENHMETAFILE hemf;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpPointF dst_points[3] = {{10.0,10.0},{20.0,10.0},{10.0,20.0}};
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

    stat = GdipGraphicsClear(graphics, 0xffffff00);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    sync_metafile(&metafile, "clear.emf");

    stat = GdipCreateBitmapFromScan0(30, 30, 0, PixelFormat32bppRGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    stat = GdipDrawImagePointsRect(graphics, (GpImage*)metafile, dst_points, 3,
        0.0, 0.0, 100.0, 100.0, UnitPixel, NULL, NULL, NULL);
    expect(Ok, stat);

    stat = GdipBitmapGetPixel(bitmap, 5, 5, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    stat = GdipBitmapGetPixel(bitmap, 15, 15, &color);
    expect(Ok, stat);
    expect(0xffffff00, color);

    stat = GdipBitmapGetPixel(bitmap, 25, 25, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    check_emfplus(hemf, clear_emf_records, "clear emf");

    DeleteEnhMetaFile(hemf);
}

static void test_nullframerect(void) {
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc, metafile_dc;
    GpBrush *brush;
    HBRUSH hbrush, holdbrush;
    GpRectF bounds;
    GpUnit unit;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, NULL, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    ok(bounds.Width == 1.0 || broken(bounds.Width == 0.0) /* xp sp1 */,
        "expected 1.0, got %f\n", bounds.Width);
    ok(bounds.Height == 1.0 || broken(bounds.Height == 0.0) /* xp sp1 */,
        "expected 1.0, got %f\n", bounds.Height);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangleI(graphics, brush, 25, 25, 75, 75);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    ok(bounds.Width == 1.0 || broken(bounds.Width == 0.0) /* xp sp1 */,
        "expected 1.0, got %f\n", bounds.Width);
    ok(bounds.Height == 1.0 || broken(bounds.Height == 0.0) /* xp sp1 */,
        "expected 1.0, got %f\n", bounds.Height);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf_(25.0, bounds.X, 0.05);
    expectf_(25.0, bounds.Y, 0.05);
    expectf_(75.0, bounds.Width, 0.05);
    expectf_(75.0, bounds.Height, 0.05);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, NULL, MetafileFrameUnitMillimeter, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

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

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf_(25.0, bounds.X, 0.05);
    expectf_(25.0, bounds.Y, 0.05);
    todo_wine expectf_(50.0, bounds.Width, 0.05);
    todo_wine expectf_(50.0, bounds.Height, 0.05);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, NULL, MetafileFrameUnitMillimeter,
        description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

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

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangleI(graphics, brush, 20, 40, 10, 70);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf_(20.0, bounds.X, 0.05);
    expectf_(25.0, bounds.Y, 0.05);
    expectf_(55.0, bounds.Width, 1.00);
    todo_wine expectf_(55.0, bounds.Width, 0.05);
    expectf_(85.0, bounds.Height, 0.05);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record pagetransform_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeSetPageTransform, UnitPixel },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeSetPageTransform, UnitPixel },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeSetPageTransform, UnitInch },
    { EmfPlusRecordTypeFillRects, 0x8000 },
    { EmfPlusRecordTypeSetPageTransform, UnitDisplay },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_pagetransform(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc;
    static const GpRectF frame = {0.0, 0.0, 5.0, 5.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
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

    sync_metafile(&metafile, "pagetransform.emf");

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

static const emfplus_record worldtransform_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeScaleWorldTransform },
    { EmfPlusRecordTypeFillRects, 0x8000 },
    { EmfPlusRecordTypeResetWorldTransform },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeMultiplyWorldTransform },
    { EmfPlusRecordTypeFillRects, 0x8000 },
    { EmfPlusRecordTypeRotateWorldTransform, 0x2000 },
    { EmfPlusRecordTypeFillRects, 0x8000 },
    { EmfPlusRecordTypeSetWorldTransform },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeTranslateWorldTransform, 0x2000 },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_worldtransform(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc;
    static const GpRectF frame = {0.0, 0.0, 5.0, 5.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    GpBitmap *bitmap;
    ARGB color;
    GpBrush *brush;
    GpMatrix *transform;
    BOOL identity;
    REAL elements[6];

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipCreateMatrix(&transform);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    /* initial transform */
    stat = GdipGetWorldTransform(graphics, transform);
    expect(Ok, stat);

    stat = GdipIsMatrixIdentity(transform, &identity);
    expect(Ok, stat);
    expect(TRUE, identity);

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangleI(graphics, brush, 0, 0, 1, 1);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* scale transform */
    stat = GdipScaleWorldTransform(graphics, 2.0, 4.0, MatrixOrderPrepend);
    expect(Ok, stat);

    stat = GdipGetWorldTransform(graphics, transform);
    expect(Ok, stat);

    stat = GdipGetMatrixElements(transform, elements);
    expect(Ok, stat);
    expectf(2.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(4.0, elements[3]);
    expectf(0.0, elements[4]);
    expectf(0.0, elements[5]);

    stat = GdipCreateSolidFill((ARGB)0xff00ff00, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 0.5, 0.5, 0.5, 0.25);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* reset transform */
    stat = GdipResetWorldTransform(graphics);
    expect(Ok, stat);

    stat = GdipGetWorldTransform(graphics, transform);
    expect(Ok, stat);

    stat = GdipIsMatrixIdentity(transform, &identity);
    expect(Ok, stat);
    expect(TRUE, identity);

    stat = GdipCreateSolidFill((ARGB)0xff00ffff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 1.0, 0.0, 1.0, 1.0);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* multiply transform */
    stat = GdipSetMatrixElements(transform, 2.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    expect(Ok, stat);

    stat = GdipMultiplyWorldTransform(graphics, transform, MatrixOrderPrepend);
    expect(Ok, stat);

    stat = GdipGetWorldTransform(graphics, transform);
    expect(Ok, stat);

    stat = GdipGetMatrixElements(transform, elements);
    expect(Ok, stat);
    expectf(2.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(1.0, elements[3]);
    expectf(0.0, elements[4]);
    expectf(0.0, elements[5]);

    stat = GdipCreateSolidFill((ARGB)0xffff0000, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 1.0, 1.0, 0.5, 1.0);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* rotate transform */
    stat = GdipRotateWorldTransform(graphics, 90.0, MatrixOrderAppend);
    expect(Ok, stat);

    stat = GdipGetWorldTransform(graphics, transform);
    expect(Ok, stat);

    stat = GdipGetMatrixElements(transform, elements);
    expect(Ok, stat);
    expectf(0.0, elements[0]);
    expectf(2.0, elements[1]);
    expectf(-1.0, elements[2]);
    expectf(0.0, elements[3]);
    expectf(0.0, elements[4]);
    expectf(0.0, elements[5]);

    stat = GdipCreateSolidFill((ARGB)0xffff00ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 1.0, -1.0, 0.5, 1.0);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* set transform */
    stat = GdipSetMatrixElements(transform, 1.0, 0.0, 0.0, 3.0, 0.0, 0.0);
    expect(Ok, stat);

    stat = GdipSetWorldTransform(graphics, transform);
    expect(Ok, stat);

    stat = GdipGetWorldTransform(graphics, transform);
    expect(Ok, stat);

    stat = GdipGetMatrixElements(transform, elements);
    expect(Ok, stat);
    expectf(1.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(3.0, elements[3]);
    expectf(0.0, elements[4]);
    expectf(0.0, elements[5]);

    stat = GdipCreateSolidFill((ARGB)0xffffff00, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 1.0, 1.0, 1.0, 1.0);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* translate transform */
    stat = GdipTranslateWorldTransform(graphics, -1.0, 0.0, MatrixOrderAppend);
    expect(Ok, stat);

    stat = GdipGetWorldTransform(graphics, transform);
    expect(Ok, stat);

    stat = GdipGetMatrixElements(transform, elements);
    expect(Ok, stat);
    expectf(1.0, elements[0]);
    expectf(0.0, elements[1]);
    expectf(0.0, elements[2]);
    expectf(3.0, elements[3]);
    expectf(-1.0, elements[4]);
    expectf(0.0, elements[5]);

    stat = GdipCreateSolidFill((ARGB)0xffffffff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 1.0, 1.0, 1.0, 1.0);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipDeleteMatrix(transform);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, worldtransform_records, "worldtransform metafile", dst_points, &frame, UnitPixel);

    sync_metafile(&metafile, "worldtransform.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, worldtransform_records, "worldtransform playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 80, 80, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 10, 10, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapGetPixel(bitmap, 30, 50, &color);
    expect(Ok, stat);
    expect(0xff00ff00, color);

    stat = GdipBitmapGetPixel(bitmap, 30, 10, &color);
    expect(Ok, stat);
    expect(0xff00ffff, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 30, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 10, 50, &color);
    expect(Ok, stat);
    expect(0xffff00ff, color);

    stat = GdipBitmapGetPixel(bitmap, 30, 90, &color);
    expect(Ok, stat);
    expect(0xffffff00, color);

    stat = GdipBitmapGetPixel(bitmap, 10, 90, &color);
    expect(Ok, stat);
    expect(0xffffffff, color);

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

    stat = GdipRecordMetafile(hdc, EmfTypeEmfOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
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

    stat = pGdipConvertToEmfPlus(graphics, metafile, NULL, 0, NULL, &metafile2);
    expect(InvalidParameter, stat);

    stat = pGdipConvertToEmfPlus(graphics, metafile, NULL, EmfTypeEmfPlusDual+1, NULL, &metafile2);
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

static void test_frameunit(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc;
    static const GpRectF frame = {0.0, 0.0, 5.0, 5.0};
    GpUnit unit;
    REAL dpix, dpiy;
    GpRectF bounds;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitInch, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    ok(bounds.Width == 1.0 || broken(bounds.Width == 0.0) /* xp sp1 */,
        "expected 1.0, got %f\n", bounds.Width);
    ok(bounds.Height == 1.0 || broken(bounds.Height == 0.0) /* xp sp1 */,
        "expected 1.0, got %f\n", bounds.Height);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    ok(bounds.Width == 1.0 || broken(bounds.Width == 0.0) /* xp sp1 */,
        "expected 1.0, got %f\n", bounds.Width);
    ok(bounds.Height == 1.0 || broken(bounds.Height == 0.0) /* xp sp1 */,
        "expected 1.0, got %f\n", bounds.Height);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipGetImageHorizontalResolution((GpImage*)metafile, &dpix);
    expect(Ok, stat);

    stat = GdipGetImageVerticalResolution((GpImage*)metafile, &dpiy);
    expect(Ok, stat);

    stat = GdipGetImageBounds((GpImage*)metafile, &bounds, &unit);
    expect(Ok, stat);
    expect(UnitPixel, unit);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(5.0 * dpix, bounds.Width, 1.0);
    expectf_(5.0 * dpiy, bounds.Height, 1.0);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record container_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeBeginContainerNoParams },
    { EmfPlusRecordTypeScaleWorldTransform },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeEndContainer },
    { EmfPlusRecordTypeScaleWorldTransform },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeSave },
    { EmfPlusRecordTypeRestore },
    { EmfPlusRecordTypeScaleWorldTransform },
    { EmfPlusRecordTypeBeginContainerNoParams },
    { EmfPlusRecordTypeScaleWorldTransform },
    { EmfPlusRecordTypeBeginContainerNoParams },
    { EmfPlusRecordTypeEndContainer },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeBeginContainer, UnitInch },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeEndContainer },
    { EmfPlusRecordTypeBeginContainerNoParams },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_containers(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    GpBitmap *bitmap;
    GpBrush *brush;
    ARGB color;
    HDC hdc;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    GraphicsContainer state1, state2;
    GpRectF srcrect, dstrect;
    REAL dpix, dpiy;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    /* Normal usage */
    stat = GdipBeginContainer2(graphics, &state1);
    expect(Ok, stat);

    stat = GdipScaleWorldTransform(graphics, 2.0, 2.0, MatrixOrderPrepend);
    expect(Ok, stat);

    stat = GdipCreateSolidFill((ARGB)0xff000000, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 5.0, 5.0, 5.0, 5.0);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipEndContainer(graphics, state1);
    expect(Ok, stat);

    stat = GdipScaleWorldTransform(graphics, 1.0, 1.0, MatrixOrderPrepend);
    expect(Ok, stat);

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 5.0, 5.0, 5.0, 5.0);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipSaveGraphics(graphics, &state1);
    expect(Ok, stat);

    stat = GdipRestoreGraphics(graphics, state1);
    expect(Ok, stat);

    /* Popping two states at once */
    stat = GdipScaleWorldTransform(graphics, 2.0, 2.0, MatrixOrderPrepend);
    expect(Ok, stat);

    stat = GdipBeginContainer2(graphics, &state1);
    expect(Ok, stat);

    stat = GdipScaleWorldTransform(graphics, 4.0, 4.0, MatrixOrderPrepend);
    expect(Ok, stat);

    stat = GdipBeginContainer2(graphics, &state2);
    expect(Ok, stat);

    stat = GdipEndContainer(graphics, state1);
    expect(Ok, stat);

    stat = GdipCreateSolidFill((ARGB)0xff00ff00, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 20.0, 20.0, 5.0, 5.0);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    /* With transform applied */
    stat = GdipGetDpiX(graphics, &dpix);
    expect(Ok, stat);

    stat = GdipGetDpiY(graphics, &dpiy);
    expect(Ok, stat);

    srcrect.X = 0.0;
    srcrect.Y = 0.0;
    srcrect.Width = 1.0;
    srcrect.Height = 1.0;

    dstrect.X = 25.0;
    dstrect.Y = 0.0;
    dstrect.Width = 5.0;
    dstrect.Height = 5.0;

    stat = GdipBeginContainer(graphics, &dstrect, &srcrect, UnitInch, &state1);
    expect(Ok, stat);

    stat = GdipCreateSolidFill((ARGB)0xff00ffff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 0.0, 0.0, dpix, dpiy);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipEndContainer(graphics, state1);
    expect(Ok, stat);

    /* Restoring an invalid state seems to break the graphics object? */
    if (0) {
        stat = GdipEndContainer(graphics, state1);
        expect(Ok, stat);
    }

    /* Ending metafile with a state open */
    stat = GdipBeginContainer2(graphics, &state1);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, container_records, "container metafile", dst_points, &frame, UnitPixel);

    sync_metafile(&metafile, "container.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, container_records, "container playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 80, 80, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 12, 12, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    stat = GdipBitmapGetPixel(bitmap, 8, 8, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapGetPixel(bitmap, 42, 42, &color);
    expect(Ok, stat);
    expect(0xff00ff00, color);

    stat = GdipBitmapGetPixel(bitmap, 55, 5, &color);
    expect(Ok, stat);
    expect(0xff00ffff, color);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record clipping_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeSave },
    { EmfPlusRecordTypeSetClipRect },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeRestore },
    { EmfPlusRecordTypeSetClipRect, 0x300 },
    { EmfPlusRecordTypeFillRects, 0xc000 },
    { EmfPlusRecordTypeObject, ObjectTypeRegion << 8 },
    { EmfPlusRecordTypeSetClipRegion, 0x100 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_clipping(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    GpBitmap *bitmap;
    GpRegion *region;
    GpBrush *brush;
    GpRectF rect;
    ARGB color;
    HDC hdc;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    GraphicsState state;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    if (stat != Ok)
        return;

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipSaveGraphics(graphics, &state);
    expect(Ok, stat);

    stat = GdipGetVisibleClipBounds(graphics, &rect);
    expect(Ok, stat);
    ok(rect.X == -0x400000, "rect.X = %f\n", rect.X);
    ok(rect.Y == -0x400000, "rect.Y = %f\n", rect.Y);
    ok(rect.Width == 0x800000, "rect.Width = %f\n", rect.Width);
    ok(rect.Height == 0x800000, "rect.Height = %f\n", rect.Height);

    stat = GdipSetClipRect(graphics, 30, 30, 10, 10, CombineModeReplace);
    expect(Ok, stat);

    stat = GdipGetVisibleClipBounds(graphics, &rect);
    expect(Ok, stat);
    ok(rect.X == 30, "rect.X = %f\n", rect.X);
    ok(rect.Y == 30, "rect.Y = %f\n", rect.Y);
    ok(rect.Width == 10, "rect.Width = %f\n", rect.Width);
    ok(rect.Height == 10, "rect.Height = %f\n", rect.Height);

    stat = GdipCreateSolidFill((ARGB)0xff000000, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 0, 0, 100, 100);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipRestoreGraphics(graphics, state);
    expect(Ok, stat);

    stat = GdipSetClipRect(graphics, 30, 30, 10, 10, CombineModeXor);
    expect(Ok, stat);

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    stat = GdipFillRectangle(graphics, brush, 30, 30, 20, 10);
    expect(Ok, stat);

    stat = GdipDeleteBrush(brush);
    expect(Ok, stat);

    stat = GdipCreateRegionRect(&rect, &region);
    expect(Ok, stat);

    stat = GdipSetClipRegion(graphics, region, CombineModeIntersect);
    expect(Ok, stat);

    stat = GdipDeleteRegion(region);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, clipping_records, "clipping metafile", dst_points, &frame, UnitPixel);

    sync_metafile(&metafile, "clipping.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, clipping_records, "clipping playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 80, 80, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 35, 35, &color);
    expect(Ok, stat);
    expect(0xff000000, color);

    stat = GdipBitmapGetPixel(bitmap, 45, 35, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static void test_gditransform_cb(GpMetafile* metafile, EmfPlusRecordType record_type,
    unsigned int flags, unsigned int dataSize, const unsigned char *pStr)
{
    static const XFORM xform = {0.5, 0, 0, 0.5, 0, 0};
    static const RECTL rectangle = {0,0,100,100};
    GpStatus stat;

    stat = GdipPlayMetafileRecord(metafile, EMR_SETWORLDTRANSFORM, 0, sizeof(xform), (void*)&xform);
    expect(Ok, stat);

    stat = GdipPlayMetafileRecord(metafile, EMR_RECTANGLE, 0, sizeof(rectangle), (void*)&rectangle);
    expect(Ok, stat);
}

static const emfplus_record gditransform_records[] = {
    { EMR_HEADER },
    { EMR_CREATEBRUSHINDIRECT },
    { EMR_SELECTOBJECT },
    { EMR_GDICOMMENT, 0, 0, 0, test_gditransform_cb },
    { EMR_SELECTOBJECT },
    { EMR_DELETEOBJECT },
    { EMR_EOF },
    { 0 }
};

static void test_gditransform(void)
{
    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HDC hdc, metafile_dc;
    HENHMETAFILE hemf;
    MetafileHeader header;
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpPointF dst_points[3] = {{0.0,0.0},{40.0,0.0},{0.0,40.0}};
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

    memset(&header, 0xaa, sizeof(header));
    stat = GdipGetMetafileHeaderFromMetafile(metafile, &header);
    expect(Ok, stat);
    expect(MetafileTypeEmf, header.Type);
    ok(header.Version == 0xdbc01001 || header.Version == 0xdbc01002, "Unexpected version %x\n", header.Version);

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

    hbrush = CreateSolidBrush(0xff);

    holdbrush = SelectObject(metafile_dc, hbrush);

    GdiComment(metafile_dc, 8, (const BYTE*)"winetest");

    SelectObject(metafile_dc, holdbrush);

    DeleteObject(hbrush);

    stat = GdipReleaseDC(graphics, metafile_dc);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, gditransform_records, "gditransform metafile", dst_points, &frame, UnitPixel);

    sync_metafile(&metafile, "gditransform.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, gditransform_records, "gditransform playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 10, 10, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 30, 30, &color);
    expect(Ok, stat);
    expect(0x00000000, color);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record draw_image_bitmap_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypeImage << 8 },
    { EmfPlusRecordTypeObject, (ObjectTypeImageAttributes << 8) | 1 },
    { EmfPlusRecordTypeDrawImagePoints, 0, 0, 0, NULL, 0x4000 },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static const emfplus_record draw_image_metafile_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypeImage << 8 },
    /* metafile object */
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypeImage << 8 },
    { EmfPlusRecordTypeObject, (ObjectTypeImageAttributes << 8) | 1 },
    { EmfPlusRecordTypeDrawImagePoints },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    /* end of metafile object */
    { EmfPlusRecordTypeDrawImagePoints },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_drawimage(void)
{
    static const GpPointF dst_points[3] = {{10.0,10.0},{85.0,15.0},{10.0,80.0}};
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    const ColorMatrix double_red = {{
        {2.0,0.0,0.0,0.0,0.0},
        {0.0,1.0,0.0,0.0,0.0},
        {0.0,0.0,1.0,0.0,0.0},
        {0.0,0.0,0.0,1.0,0.0},
        {0.0,0.0,0.0,0.0,1.0}}};

    GpImageAttributes *imageattr;
    GpMetafile *metafile;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpStatus stat;
    BITMAPINFO info;
    BYTE buff[400];
    GpImage *image;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = 10;
    info.bmiHeader.biHeight = 10;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    memset(buff, 0x80, sizeof(buff));
    stat = GdipCreateBitmapFromGdiDib(&info, buff, (GpBitmap**)&image);
    expect(Ok, stat);

    stat = GdipCreateImageAttributes(&imageattr);
    expect(Ok, stat);

    stat = GdipSetImageAttributesColorMatrix(imageattr, ColorAdjustTypeDefault,
            TRUE, &double_red, NULL, ColorMatrixFlagsDefault);
    expect(Ok, stat);

    stat = GdipDrawImagePointsRect(graphics, image, dst_points, 3,
            0.0, 0.0, 10.0, 10.0, UnitPixel, imageattr, NULL, NULL);
    GdipDisposeImageAttributes(imageattr);
    expect(Ok, stat);

    GdipDisposeImage(image);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "draw_image_bitmap.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, draw_image_bitmap_records, "draw image bitmap");

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    /* test drawing metafile */
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreateMetafileFromEmf(hemf, TRUE, (GpMetafile**)&image);
    expect(Ok, stat);

    stat = GdipDrawImagePointsRect(graphics, image, dst_points, 3,
            0.0, 0.0, 100.0, 100.0, UnitPixel, NULL, NULL, NULL);
    expect(Ok, stat);

    GdipDisposeImage(image);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "draw_image_metafile.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    if (GetProcAddress(GetModuleHandleA("gdiplus.dll"), "GdipConvertToEmfPlus"))
    {
        check_emfplus(hemf, draw_image_metafile_records, "draw image metafile");
    }
    else
    {
        win_skip("draw image metafile records tests skipped\n");
    }
    DeleteEnhMetaFile(hemf);

    DeleteDC(hdc);
    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record properties_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeSetTextRenderingHint, TextRenderingHintAntiAlias },
    { EmfPlusRecordTypeSetPixelOffsetMode, PixelOffsetModeHighQuality },
    { EmfPlusRecordTypeSetAntiAliasMode, (SmoothingModeAntiAlias << 1) | 1, 0, 0, NULL, 0x1 },
    { EmfPlusRecordTypeSetCompositingMode, CompositingModeSourceCopy },
    { EmfPlusRecordTypeSetCompositingQuality, CompositingQualityHighQuality },
    { EmfPlusRecordTypeSetInterpolationMode, InterpolationModeHighQualityBicubic },
    { EmfPlusRecordTypeSetRenderingOrigin },
    { EmfPlusRecordTypeSetRenderingOrigin },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_properties(void)
{
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};

    GpMetafile *metafile;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpStatus stat;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipSetTextRenderingHint(graphics, TextRenderingHintSystemDefault);
    expect(Ok, stat);
    stat = GdipSetTextRenderingHint(graphics, TextRenderingHintAntiAlias);
    expect(Ok, stat);

    stat = GdipSetPixelOffsetMode(graphics, PixelOffsetModeHighQuality);
    expect(Ok, stat);
    stat = GdipSetPixelOffsetMode(graphics, PixelOffsetModeHighQuality);
    expect(Ok, stat);

    stat = GdipSetSmoothingMode(graphics, SmoothingModeAntiAlias);
    expect(Ok, stat);
    stat = GdipSetSmoothingMode(graphics, SmoothingModeAntiAlias);
    expect(Ok, stat);

    stat = GdipSetCompositingMode(graphics, CompositingModeSourceOver);
    expect(Ok, stat);
    stat = GdipSetCompositingMode(graphics, CompositingModeSourceCopy);
    expect(Ok, stat);

    stat = GdipSetCompositingQuality(graphics, CompositingQualityHighQuality);
    expect(Ok, stat);
    stat = GdipSetCompositingQuality(graphics, CompositingQualityHighQuality);
    expect(Ok, stat);

    stat = GdipSetInterpolationMode(graphics, InterpolationModeDefault);
    expect(Ok, stat);
    stat = GdipSetInterpolationMode(graphics, InterpolationModeHighQuality);
    expect(Ok, stat);

    stat = GdipSetRenderingOrigin(graphics, 1, 2);
    expect(Ok, stat);

    stat = GdipSetRenderingOrigin(graphics, 1, 2);
    expect(Ok, stat);

    stat = GdipSetRenderingOrigin(graphics, 2, 1);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "properties.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, properties_records, "properties");
    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record draw_path_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypePen << 8 },
    { EmfPlusRecordTypeObject, (ObjectTypePath << 8) | 1 },
    { EmfPlusRecordTypeDrawPath, 1 },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_drawpath(void)
{
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};

    GpMetafile *metafile;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpStatus stat;
    GpPath *path;
    GpPen *pen;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathLine(path, 5, 5, 30, 30);
    expect(Ok, stat);

    stat = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, stat);

    stat = GdipDrawPath(graphics, pen, path);
    expect(Ok, stat);

    stat = GdipDeletePen(pen);
    expect(Ok, stat);
    stat = GdipDeletePath(path);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "draw_path.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, draw_path_records, "draw path");
    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record fill_path_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypePath << 8 },
    { EmfPlusRecordTypeFillPath, 0x8000 },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_fillpath(void)
{
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};

    GpMetafile *metafile;
    GpGraphics *graphics;
    GpSolidFill *brush;
    HENHMETAFILE hemf;
    GpStatus stat;
    GpPath *path;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathLine(path, 5, 5, 30, 30);
    expect(Ok, stat);
    stat = GdipAddPathLine(path, 30, 30, 5, 30);
    expect(Ok, stat);

    stat = GdipCreateSolidFill(0xffaabbcc, &brush);
    expect(Ok, stat);

    stat = GdipFillPath(graphics, (GpBrush*)brush, path);
    expect(Ok, stat);

    stat = GdipDeleteBrush((GpBrush*)brush);
    expect(Ok, stat);
    stat = GdipDeletePath(path);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "fill_path.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, fill_path_records, "fill path");

    /* write to disk */
    DeleteEnhMetaFile(CopyEnhMetaFileW(hemf, L"winetest.emf"));

    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    /* should succeed when given path to an EMF */
    stat = GdipCreateMetafileFromWmfFile(L"winetest.emf", NULL, &metafile);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);

    DeleteFileW(L"winetest.emf");

    stat = GdipCreateMetafileFromWmfFile(L"winetest.emf", NULL, &metafile);
    expect(GenericError, stat);
}

static const emfplus_record restoredc_records[] = {
    { EMR_HEADER },
    { EMR_CREATEBRUSHINDIRECT },
    { EMR_SELECTOBJECT },

    { EMR_SAVEDC },
    { EMR_SETVIEWPORTORGEX },
    { EMR_SAVEDC },
    { EMR_SETVIEWPORTORGEX },
    { EMR_SAVEDC },
    { EMR_SETVIEWPORTORGEX },

    { EMR_RECTANGLE },

    { EMR_RESTOREDC },
    { EMR_RECTANGLE },

    { EMR_RESTOREDC },
    { EMR_RECTANGLE },

    { EMR_SELECTOBJECT },
    { EMR_DELETEOBJECT },
    { EMR_EOF },
    { 0 }
};

static void test_restoredc(void)
{
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};

    GpBitmap *bitmap;
    GpGraphics *graphics;
    GpMetafile *metafile;
    GpStatus stat;
    HDC hdc, metafile_dc;
    HBRUSH hbrush, holdbrush;
    ARGB color;

    hdc = CreateCompatibleDC(0);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfOnly, &frame, MetafileFrameUnitPixel,
        L"winetest", &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipGetDC(graphics, &metafile_dc);
    expect(Ok, stat);

    hbrush = CreateSolidBrush(0xff0000);
    holdbrush = SelectObject(metafile_dc, hbrush);

    SaveDC(metafile_dc);
    SetViewportOrgEx(metafile_dc, 20, 20, NULL);

    SaveDC(metafile_dc);
    SetViewportOrgEx(metafile_dc, 40, 40, NULL);

    SaveDC(metafile_dc);
    SetViewportOrgEx(metafile_dc, 60, 60, NULL);

    Rectangle(metafile_dc, 0, 0, 3, 3);
    RestoreDC(metafile_dc, -2);

    Rectangle(metafile_dc, 0, 0, 3, 3);
    RestoreDC(metafile_dc, -1);

    Rectangle(metafile_dc, 0, 0, 3, 3);

    SelectObject(metafile_dc, holdbrush);
    DeleteObject(hbrush);

    stat = GdipReleaseDC(graphics, metafile_dc);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    check_metafile(metafile, restoredc_records, "restoredc metafile", dst_points,
        &frame, UnitPixel);
    sync_metafile(&metafile, "restoredc.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, restoredc_records, "restoredc playback", dst_points,
        &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 1, 1, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapGetPixel(bitmap, 21, 21, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipBitmapGetPixel(bitmap, 41, 41, &color);
    expect(Ok, stat);
    expect(0, color);

    stat = GdipBitmapGetPixel(bitmap, 61, 61, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)bitmap);
    expect(Ok, stat);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record drawdriverstring_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypeFont << 8 },
    { EmfPlusRecordTypeDrawDriverString, 0x8000 },
    { EmfPlusRecordTypeObject, (ObjectTypeFont << 8) | 1 },
    { EmfPlusRecordTypeObject, (ObjectTypeBrush << 8) | 2 },
    { EmfPlusRecordTypeDrawDriverString, 0x1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_drawdriverstring(void)
{
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const PointF solidpos[4] = {{10.0,10.0}, {20.0,10.0}, {30.0,10.0}, {40.0,10.0}};
    static const PointF hatchpos = {10.0,30.0};

    GpBitmap *bitmap;
    GpStatus stat;
    GpGraphics *graphics;
    GpFont *solidfont, *hatchfont;
    GpBrush *solidbrush, *hatchbrush;
    HDC hdc;
    GpMatrix *matrix;
    GpMetafile *metafile;
    LOGFONTA logfont = { 0 };

    hdc = CreateCompatibleDC(0);

    strcpy(logfont.lfFaceName, "Times New Roman");
    logfont.lfHeight = 12;
    logfont.lfCharSet = DEFAULT_CHARSET;

    stat = GdipCreateFontFromLogfontA(hdc, &logfont, &solidfont);
    if (stat == NotTrueTypeFont || stat == FileNotFound)
    {
        DeleteDC(hdc);
        skip("Times New Roman not installed.\n");
        return;
    }

    stat = GdipCloneFont(solidfont, &hatchfont);
    expect(Ok, stat);

    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel,
        L"winetest", &metafile);
    expect(Ok, stat);

    DeleteDC(hdc);
    hdc = NULL;

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&solidbrush);
    expect(Ok, stat);

    stat = GdipCreateHatchBrush(HatchStyleHorizontal, (ARGB)0xff00ff00, (ARGB)0xffff0000,
        (GpHatch**)&hatchbrush);
    expect(Ok, stat);

    stat = GdipCreateMatrix(&matrix);
    expect(Ok, stat);

    stat = GdipDrawDriverString(graphics, L"Test", 4, solidfont, solidbrush, solidpos,
        DriverStringOptionsCmapLookup, matrix);
    expect(Ok, stat);

    stat = GdipSetMatrixElements(matrix, 1.5, 0.0, 0.0, 1.5, 0.0, 0.0);
    expect(Ok, stat);

    stat = GdipDrawDriverString(graphics, L"Test ", 5, hatchfont, hatchbrush, &hatchpos,
        DriverStringOptionsCmapLookup|DriverStringOptionsRealizedAdvance, matrix);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    graphics = NULL;

    check_metafile(metafile, drawdriverstring_records, "drawdriverstring metafile", dst_points,
        &frame, UnitPixel);
    sync_metafile(&metafile, "drawdriverstring.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, drawdriverstring_records, "drawdriverstring playback",
        dst_points, &frame, UnitPixel);

    GdipDeleteMatrix(matrix);
    GdipDeleteGraphics(graphics);
    GdipDeleteBrush(solidbrush);
    GdipDeleteBrush(hatchbrush);
    GdipDeleteFont(solidfont);
    GdipDeleteFont(hatchfont);
    GdipDisposeImage((GpImage*)bitmap);
    GdipDisposeImage((GpImage*)metafile);
}

static const emfplus_record unknownfontdecode_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypeFont << 8 },
    { EmfPlusRecordTypeDrawDriverString, 0x8000 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_unknownfontdecode(void)
{
    static const GpPointF dst_points[3] = {{0.0,0.0},{100.0,0.0},{0.0,100.0}};
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const PointF pos = {10.0,30.0};
    static const INT testfont0_resnum = 2;

    BOOL rval;
    DWORD written, ressize;
    GpBitmap *bitmap;
    GpBrush *brush;
    GpFont *font;
    GpFontCollection *fonts;
    GpFontFamily *family;
    GpGraphics *graphics;
    GpMetafile *metafile;
    GpStatus stat;
    HANDLE file;
    HDC hdc;
    HRSRC res;
    INT fontscount;
    WCHAR path[MAX_PATH];
    void *buf;

    /* Create a custom font from a resource. */
    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"wine_testfont0.ttf");

    file = CreateFileW(path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n",
        wine_dbgstr_w(path), GetLastError());

    res = FindResourceA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(testfont0_resnum),
        (LPCSTR)RT_RCDATA);
    ok(res != 0, "couldn't find resource\n");

    buf = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    ressize = SizeofResource(GetModuleHandleA(NULL), res);

    WriteFile(file, buf, ressize, &written, NULL);
    expect(ressize, written);

    CloseHandle(file);

    stat = GdipNewPrivateFontCollection(&fonts);
    expect(Ok, stat);

    stat = GdipPrivateAddFontFile(fonts, path);
    expect(Ok, stat);

    stat = GdipGetFontCollectionFamilyCount(fonts, &fontscount);
    expect(Ok, stat);
    expect(1, fontscount);

    stat = GdipGetFontCollectionFamilyList(fonts, fontscount, &family, &fontscount);
    expect(Ok, stat);

    stat = GdipCreateFont(family, 16.0, FontStyleRegular, UnitPixel, &font);
    expect(Ok, stat);

    /* Start metafile recording. */
    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel,
        L"winetest", &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);
    hdc = NULL;

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreateSolidFill((ARGB)0xff0000ff, (GpSolidFill**)&brush);
    expect(Ok, stat);

    /* Write something with the custom font so that it is encoded. */
    stat = GdipDrawDriverString(graphics, L"Test", 4, font, brush, &pos,
        DriverStringOptionsCmapLookup|DriverStringOptionsRealizedAdvance, NULL);
    expect(Ok, stat);

    /* Delete the custom font so that it is not present during playback. */
    GdipDeleteFont(font);
    GdipDeletePrivateFontCollection(&fonts);
    rval = DeleteFileW(path);
    expect(TRUE, rval);

    GdipDeleteGraphics(graphics);
    graphics = NULL;

    check_metafile(metafile, unknownfontdecode_records, "unknownfontdecode metafile", dst_points,
        &frame, UnitPixel);
    sync_metafile(&metafile, "unknownfontdecode.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, unknownfontdecode_records, "unknownfontdecode playback",
        dst_points, &frame, UnitPixel);

    GdipDeleteGraphics(graphics);
    GdipDeleteBrush(brush);
    GdipDisposeImage((GpImage*)bitmap);
    GdipDisposeImage((GpImage*)metafile);
}

static const emfplus_record fillregion_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypeRegion << 8 },
    { EmfPlusRecordTypeFillRegion, 0x8000 },
    { EmfPlusRecordTypeObject, (ObjectTypeBrush << 8) | 1 },
    { EmfPlusRecordTypeObject, (ObjectTypeRegion << 8) | 2 },
    { EmfPlusRecordTypeFillRegion, 2 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_fillregion(void)
{
    static const GpPointF dst_points[3] = {{0.0, 0.0}, {100.0, 0.0}, {0.0, 100.0}};
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpRectF solidrect = {20.0, 20.0, 20.0, 20.0};
    static const GpRectF hatchrect = {50.0, 50.0, 20.0, 20.0};

    GpStatus stat;
    GpMetafile *metafile;
    GpGraphics *graphics;
    GpBitmap *bitmap;
    GpBrush *solidbrush, *hatchbrush;
    GpRegion *solidregion, *hatchregion;
    ARGB color;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel,
        L"winetest", &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);
    hdc = NULL;

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreateRegionRect(&solidrect, &solidregion);
    expect(Ok, stat);

    stat = GdipCreateSolidFill(0xffaabbcc, (GpSolidFill**)&solidbrush);
    expect(Ok, stat);

    stat = GdipFillRegion(graphics, solidbrush, solidregion);
    expect(Ok, stat);

    stat = GdipCreateRegionRect(&hatchrect, &hatchregion);
    expect(Ok, stat);

    stat = GdipCreateHatchBrush(HatchStyleHorizontal, 0xffff0000, 0xff0000ff,
        (GpHatch**)&hatchbrush);
    expect(Ok, stat);

    stat = GdipFillRegion(graphics, hatchbrush, hatchregion);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    graphics = NULL;
    expect(Ok, stat);

    check_metafile(metafile, fillregion_records, "regionfill metafile", dst_points,
        &frame, UnitPixel);
    sync_metafile(&metafile, "regionfill.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, fillregion_records, "regionfill playback",
        dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 25, 25, &color);
    expect(Ok, stat);
    expect(0xffaabbcc, color);

    stat = GdipBitmapGetPixel(bitmap, 56, 56, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 57, 57, &color);
    expect(Ok, stat);
    expect(0xff0000ff, color);

    GdipDeleteRegion(solidregion);
    GdipDeleteRegion(hatchregion);
    GdipDeleteBrush(solidbrush);
    GdipDeleteBrush(hatchbrush);
    GdipDeleteGraphics(graphics);
    GdipDisposeImage((GpImage*)bitmap);
    GdipDisposeImage((GpImage*)metafile);
}

static const emfplus_record lineargradient_records[] = {
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypeBrush << 8 },
    { EmfPlusRecordTypeFillRects, 0x4000 },
    { EmfPlusRecordTypeObject, (ObjectTypeBrush << 8) | 1 },
    { EmfPlusRecordTypeFillRects, 0x4000 },
    { EmfPlusRecordTypeObject, (ObjectTypeBrush << 8) | 2 },
    { EmfPlusRecordTypeFillRects, 0x4000 },
    { EmfPlusRecordTypeObject, (ObjectTypeBrush << 8) | 3 },
    { EmfPlusRecordTypeFillRects, 0x4000 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_lineargradient(void)
{
    static const GpPointF dst_points[3] = {{0.0, 0.0}, {100.0, 0.0}, {0.0, 100.0}};
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    static const GpRectF horizrect = {10.0, 10.0, 20.0, 20.0};
    static const GpRectF vertrect = {50.0, 10.0, 20.0, 20.0};
    static const GpRectF blendrect = {10.0, 50.0, 20.0, 20.0};
    static const GpRectF presetrect = {50.0, 50.0, 20.0, 20.0};
    static const REAL blendfac[3] = {0.0, 0.9, 1.0};
    static const REAL blendpos[3] = {0.0, 0.5, 1.0};
    static const ARGB pblendcolor[3] = {0xffff0000, 0xff00ff00, 0xff0000ff};
    static const REAL pblendpos[3] = {0.0, 0.5, 1.0};

    ARGB color;
    GpBitmap *bitmap;
    GpBrush *horizbrush, *vertbrush, *blendbrush, *presetbrush;
    GpGraphics *graphics;
    GpMetafile *metafile;
    GpStatus stat;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel,
        L"winetest", &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);
    hdc = NULL;

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    /* Test various brush types to cover all valid combinations
       of optional serialized data. */
    stat = GdipCreateLineBrushFromRect(&horizrect, 0xffff0000, 0xff0000ff,
        LinearGradientModeHorizontal, WrapModeTile, (GpLineGradient**)&horizbrush);
    expect(Ok, stat);

    stat = GdipCreateLineBrushFromRect(&vertrect, 0xffff0000, 0xff0000ff,
        LinearGradientModeVertical, WrapModeTile, (GpLineGradient**)&vertbrush);
    expect(Ok, stat);

    stat = GdipCreateLineBrushFromRect(&blendrect, 0xffff0000, 0xff0000ff,
        LinearGradientModeHorizontal, WrapModeTile, (GpLineGradient**)&blendbrush);
    expect(Ok, stat);

    stat = GdipSetLineBlend((GpLineGradient*)blendbrush, blendfac, blendpos, 3);
    expect(Ok, stat);

    stat = GdipCreateLineBrushFromRect(&presetrect, 0xffff0000, 0xff0000ff,
        LinearGradientModeVertical, WrapModeTile, (GpLineGradient**)&presetbrush);
    expect(Ok, stat);

    stat = GdipSetLinePresetBlend((GpLineGradient*)presetbrush, pblendcolor, pblendpos, 3);
    expect(Ok, stat);

    stat = GdipFillRectangles(graphics, vertbrush, &vertrect, 1);
    expect(Ok, stat);

    stat = GdipFillRectangles(graphics, horizbrush, &horizrect, 1);
    expect(Ok, stat);

    stat = GdipFillRectangles(graphics, blendbrush, &blendrect, 1);
    expect(Ok, stat);

    stat = GdipFillRectangles(graphics, presetbrush, &presetrect, 1);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    graphics = NULL;
    expect(Ok, stat);

    check_metafile(metafile, lineargradient_records, "lineargradient metafile", dst_points,
        &frame, UnitPixel);
    sync_metafile(&metafile, "lineargradient.emf");

    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat32bppARGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage*)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(metafile, graphics, lineargradient_records, "lineargradient playback",
        dst_points, &frame, UnitPixel);

    /* Verify horizontal gradient fill. */
    stat = GdipBitmapGetPixel(bitmap, 10, 10, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 18, 10, &color);
    expect(Ok, stat);
    expect(0xff990066, color);

    /* Verify vertical gradient fill. */
    stat = GdipBitmapGetPixel(bitmap, 50, 10, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 18, &color);
    expect(Ok, stat);
    expect(0xff990066, color);

    /* Verify custom blend gradient fill. */
    stat = GdipBitmapGetPixel(bitmap, 10, 50, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 18, 50, &color);
    expect(Ok, stat);
    expect(0xff4700b8, color);

    /* Verify preset color gradient fill. */
    stat = GdipBitmapGetPixel(bitmap, 50, 50, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 50, 60, &color);
    expect(Ok, stat);
    expect(0xff00ff00, color);

    GdipDeleteBrush(vertbrush);
    GdipDeleteBrush(horizbrush);
    GdipDeleteBrush(blendbrush);
    GdipDeleteBrush(presetbrush);
    GdipDeleteGraphics(graphics);
    GdipDisposeImage((GpImage*)bitmap);
    GdipDisposeImage((GpImage*)metafile);
}

static HDC create_printer_dc(void)
{
    char buffer[260];
    DWORD len;
    PRINTER_INFO_2A *pbuf = NULL;
    DRIVER_INFO_3A *dbuf = NULL;
    HANDLE hprn = 0;
    HDC hdc = 0;
    HMODULE winspool = LoadLibraryA("winspool.drv");
    BOOL (WINAPI *pOpenPrinterA)(LPSTR, HANDLE *, LPPRINTER_DEFAULTSA);
    BOOL (WINAPI *pGetDefaultPrinterA)(LPSTR, LPDWORD);
    BOOL (WINAPI *pGetPrinterA)(HANDLE, DWORD, LPBYTE, DWORD, LPDWORD);
    BOOL (WINAPI *pGetPrinterDriverA)(HANDLE, LPSTR, DWORD, LPBYTE, DWORD, LPDWORD);
    BOOL (WINAPI *pClosePrinter)(HANDLE);

    pGetDefaultPrinterA = (void *)GetProcAddress(winspool, "GetDefaultPrinterA");
    pOpenPrinterA = (void *)GetProcAddress(winspool, "OpenPrinterA");
    pGetPrinterA = (void *)GetProcAddress(winspool, "GetPrinterA");
    pGetPrinterDriverA = (void *)GetProcAddress(winspool, "GetPrinterDriverA");
    pClosePrinter = (void *)GetProcAddress(winspool, "ClosePrinter");

    if (!pGetDefaultPrinterA || !pOpenPrinterA || !pGetPrinterA || !pGetPrinterDriverA || !pClosePrinter)
        goto done;

    len = sizeof(buffer);
    if (!pGetDefaultPrinterA(buffer, &len)) goto done;
    if (!pOpenPrinterA(buffer, &hprn, NULL)) goto done;

    pGetPrinterA(hprn, 2, NULL, 0, &len);
    pbuf = malloc(len);
    if (!pGetPrinterA(hprn, 2, (LPBYTE)pbuf, len, &len)) goto done;

    pGetPrinterDriverA(hprn, NULL, 3, NULL, 0, &len);
    dbuf = malloc(len);
    if (!pGetPrinterDriverA(hprn, NULL, 3, (LPBYTE)dbuf, len, &len)) goto done;

    hdc = CreateDCA(dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName, pbuf->pDevMode);
    trace("hdc %p for driver '%s' printer '%s' port '%s'\n", hdc,
          dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName);
done:
    free(dbuf);
    free(pbuf);
    if (hprn) pClosePrinter(hprn);
    if (winspool) FreeLibrary(winspool);
    return hdc;
}

static void test_printer_dc(void)
{
    HDC hdc;
    Status status;
    RectF frame = { 0.0, 0.0, 1.0, 1.0 };
    GpMetafile *metafile;
    GpGraphics *graphics;
    REAL dpix, dpiy;

    hdc = create_printer_dc();
    if (!hdc)
    {
        skip("could not create a DC for the default printer\n");
        return;
    }

    status = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitInch, L"winetest", &metafile);
    expect(Ok, status);

    status = GdipGetImageGraphicsContext((GpImage *)metafile, &graphics);
    expect(Ok, status);

    GdipGetDpiX(graphics, &dpix);
    GdipGetDpiX(graphics, &dpiy);
    expectf((REAL)(GetDeviceCaps(hdc, LOGPIXELSX)), dpix);
    expectf((REAL)(GetDeviceCaps(hdc, LOGPIXELSY)), dpiy);

    GdipDeleteGraphics(graphics);
    GdipDisposeImage((GpImage *)metafile);
}

static const emfplus_record draw_ellipse_records[] =
{
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypePen << 8 },
    { EmfPlusRecordTypeDrawEllipse, 0x4000 },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_drawellipse(void)
{
    static const GpRectF frame = { 0.0f, 0.0f, 100.0f, 100.0f };

    GpMetafile *metafile;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpStatus stat;
    GpPen *pen;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage *)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, stat);

    stat = GdipDrawEllipse(graphics, pen, 1.0f, 1.0f, 16.0f, 32.0f);
    expect(Ok, stat);

    stat = GdipDeletePen(pen);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "draw_ellipse.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, draw_ellipse_records, "draw ellipse");
    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record fill_ellipse_records[] =
{
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeFillEllipse, 0xc000 },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_fillellipse(void)
{
    static const GpRectF frame = { 0.0f, 0.0f, 100.0f, 100.0f };

    GpMetafile *metafile;
    GpGraphics *graphics;
    GpSolidFill *brush;
    HENHMETAFILE hemf;
    GpStatus stat;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage*)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreateSolidFill(0xffaabbcc, &brush);
    expect(Ok, stat);

    stat = GdipFillEllipse(graphics, (GpBrush *)brush, 0.0f, 0.0f, 10.0f, 20.0f);
    expect(Ok, stat);

    stat = GdipDeleteBrush((GpBrush*)brush);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "fill_ellipse.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, fill_ellipse_records, "fill ellipse");

    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record draw_rectangle_records[] =
{
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypePen << 8 },
    { EmfPlusRecordTypeDrawRects, 0x4000 },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_drawrectangle(void)
{
    static const GpRectF frame = { 0.0f, 0.0f, 100.0f, 100.0f };

    GpMetafile *metafile;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpStatus stat;
    GpPen *pen;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage *)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, stat);

    stat = GdipDrawRectangle(graphics, pen, 1.0f, 1.0f, 16.0f, 32.0f);
    expect(Ok, stat);

    stat = GdipDeletePen(pen);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "draw_rectangle.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, draw_rectangle_records, "draw rectangle");
    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static void test_offsetclip(void)
{
    static const GpRectF frame = { 0.0f, 0.0f, 100.0f, 100.0f };
    static const emfplus_record offset_clip_records[] =
    {
       { EMR_HEADER },
       { EmfPlusRecordTypeHeader },
       { EmfPlusRecordTypeOffsetClip },
       { EmfPlusRecordTypeOffsetClip },
       { EmfPlusRecordTypeEndOfFile },
       { EMR_EOF },
       { 0 },
    };

    GpMetafile *metafile;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpStatus stat;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage *)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipTranslateClip(graphics, 1.0f, -1.0f);
    expect(Ok, stat);

    stat = GdipTranslateClipI(graphics, 2, 3);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "offset_clip.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, offset_clip_records, "offset clip");
    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static void test_resetclip(void)
{
    static const GpRectF frame = { 0.0f, 0.0f, 100.0f, 100.0f };
    static const emfplus_record reset_clip_records[] =
    {
       { EMR_HEADER },
       { EmfPlusRecordTypeHeader },
       { EmfPlusRecordTypeResetClip },
       { EmfPlusRecordTypeResetClip },
       { EmfPlusRecordTypeEndOfFile },
       { EMR_EOF },
       { 0 },
    };

    GpMetafile *metafile;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpStatus stat;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage *)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipResetClip(graphics);
    expect(Ok, stat);

    stat = GdipResetClip(graphics);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "reset_clip.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, reset_clip_records, "reset clip");
    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static void test_setclippath(void)
{
    static const GpRectF frame = { 0.0f, 0.0f, 100.0f, 100.0f };
    static const emfplus_record set_clip_path_records[] =
    {
       { EMR_HEADER },
       { EmfPlusRecordTypeHeader },
       { EmfPlusRecordTypeObject, ObjectTypePath << 8 },
       { EmfPlusRecordTypeSetClipPath, 0x100 },
       { EmfPlusRecordTypeEndOfFile },
       { EMR_EOF },
       { 0 },
    };

    GpMetafile *metafile;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpStatus stat;
    GpPath *path;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage *)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, stat);
    stat = GdipAddPathLine(path, 5, 5, 30, 30);
    expect(Ok, stat);
    stat = GdipAddPathLine(path, 30, 30, 5, 30);
    expect(Ok, stat);

    stat = GdipSetClipPath(graphics, path, CombineModeIntersect);
    expect(Ok, stat);

    stat = GdipDeletePath(path);
    expect(Ok, stat);

    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    sync_metafile(&metafile, "set_clip_path.emf");

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, set_clip_path_records, "set clip path");
    DeleteEnhMetaFile(hemf);

    stat = GdipDisposeImage((GpImage*)metafile);
    expect(Ok, stat);
}

static const emfplus_record pen_dc_records[] =
{
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypePen << 8 },
    { EmfPlusRecordTypeObject, (ObjectTypePath << 8) | 1 },
    { EmfPlusRecordTypeDrawPath, 1 },
    { EMR_SAVEDC, 0, 1 },
    { EMR_SETICMMODE, 0, 1 },
    { EMR_BITBLT, 0, 1 },
    { EMR_RESTOREDC, 0, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static const emfplus_record pen_bitmap_records[] =
{
    { EMR_HEADER },
    { EmfPlusRecordTypeHeader },
    { EmfPlusRecordTypeObject, ObjectTypePen << 8 },
    { EmfPlusRecordTypeObject, (ObjectTypePath << 8) | 1 },
    { EmfPlusRecordTypeDrawPath, 1 },
    { EmfPlusRecordTypeEndOfFile },
    { EMR_EOF },
    { 0 }
};

static void test_pen(void)
{
    static const GpPointF dst_points[3] = {{0.0, 0.0}, {100.0, 0.0}, {0.0, 100.0}};
    static const GpRectF frame = {0.0, 0.0, 100.0, 100.0};
    GpMetafile *metafile, *clone_metafile;
    GpPath *draw_path, *line_cap_path;
    GpCustomLineCap *custom_line_cap;
    GpGraphics *graphics;
    HENHMETAFILE hemf;
    GpBitmap *bitmap;
    GpStatus stat;
    ARGB color;
    GpPen *pen;
    BOOL ret;
    HDC hdc;

    /* Record */
    hdc = CreateCompatibleDC(0);
    stat = GdipRecordMetafile(hdc, EmfTypeEmfPlusOnly, &frame, MetafileFrameUnitPixel, description, &metafile);
    expect(Ok, stat);
    DeleteDC(hdc);

    stat = GdipGetImageGraphicsContext((GpImage *)metafile, &graphics);
    expect(Ok, stat);

    stat = GdipCreatePath(FillModeAlternate, &draw_path);
    expect(Ok, stat);
    stat = GdipAddPathLine(draw_path, 25, 25, 25, 75);
    expect(Ok, stat);

    stat = GdipCreatePen1((ARGB)0xffff0000, 1.0f, UnitPixel, &pen);
    expect(Ok, stat);
    stat = GdipCreatePath(FillModeAlternate, &line_cap_path);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(line_cap_path, 5.0, 5.0, 10.0, 10.0);
    expect(Ok, stat);
    stat = GdipCreateCustomLineCap(NULL, line_cap_path, LineCapCustom, 0.0, &custom_line_cap);
    expect(Ok, stat);
    stat = GdipSetPenCustomStartCap(pen, custom_line_cap);
    expect(Ok, stat);
    stat = GdipSetPenCustomEndCap(pen, custom_line_cap);
    expect(Ok, stat);
    stat = GdipDeleteCustomLineCap(custom_line_cap);
    expect(Ok, stat);
    stat = GdipDeletePath(line_cap_path);
    expect(Ok, stat);

    stat = GdipDrawPath(graphics, pen, draw_path);
    expect(Ok, stat);

    stat = GdipDeletePen(pen);
    expect(Ok, stat);
    stat = GdipDeletePath(draw_path);
    expect(Ok, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);

    sync_metafile(&metafile, "pen.emf");
    GdipCloneImage((GpImage *)metafile, (GpImage **)&clone_metafile);

    stat = GdipGetHemfFromMetafile(metafile, &hemf);
    expect(Ok, stat);

    check_emfplus(hemf, pen_dc_records, "pen record");

    ret = DeleteEnhMetaFile(hemf);
    ok(ret != 0, "Failed to delete enhmetafile.\n");
    stat = GdipDisposeImage((GpImage *)metafile);
    expect(Ok, stat);

    /* Play back */
    stat = GdipCreateBitmapFromScan0(100, 100, 0, PixelFormat24bppRGB, NULL, &bitmap);
    expect(Ok, stat);

    stat = GdipGetImageGraphicsContext((GpImage *)bitmap, &graphics);
    expect(Ok, stat);

    play_metafile(clone_metafile, graphics, pen_bitmap_records, "pen playback", dst_points, &frame, UnitPixel);

    stat = GdipBitmapGetPixel(bitmap, 10, 10, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipBitmapGetPixel(bitmap, 40, 90, &color);
    expect(Ok, stat);
    expect(0xffff0000, color);

    stat = GdipDisposeImage((GpImage *)clone_metafile);
    expect(Ok, stat);
    stat = GdipDeleteGraphics(graphics);
    expect(Ok, stat);
    stat = GdipDisposeImage((GpImage *)bitmap);
    expect(Ok, stat);
}

START_TEST(metafile)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    int myARGC;
    char **myARGV;
    HMODULE hmsvcrt;
    int (CDECL * _controlfp_s)(unsigned int *cur, unsigned int newval, unsigned int mask);

    /* Enable all FP exceptions except _EM_INEXACT, which gdi32 can trigger */
    hmsvcrt = LoadLibraryA("msvcrt");
    _controlfp_s = (void*)GetProcAddress(hmsvcrt, "_controlfp_s");
    if (_controlfp_s) _controlfp_s(0, 0, 0x0008001e);

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    myARGC = winetest_get_mainargs( &myARGV );

    if (myARGC >= 3)
    {
        if (!strcmp(myARGV[2], "save"))
            save_metafiles = TRUE;
        else if (!strcmp(myARGV[2], "load"))
            load_metafiles = TRUE;
    }

    test_empty();
    test_getdc();
    test_emfonly();
    test_fillrect();
    test_clear();
    test_nullframerect();
    test_pagetransform();
    test_worldtransform();
    test_converttoemfplus();
    test_frameunit();
    test_containers();
    test_clipping();
    test_gditransform();
    test_drawimage();
    test_properties();
    test_drawpath();
    test_fillpath();
    test_restoredc();
    test_drawdriverstring();
    test_unknownfontdecode();
    test_fillregion();
    test_lineargradient();
    test_printer_dc();
    test_drawellipse();
    test_fillellipse();
    test_drawrectangle();
    test_offsetclip();
    test_resetclip();
    test_setclippath();
    test_pen();

    GdiplusShutdown(gdiplusToken);
}
