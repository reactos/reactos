/*
 * OLEPICTURE test program
 *
 * Copyright 2005 Marcus Meissner
 *
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

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winerror.h>
#include <winnt.h>

#include <wtypes.h>
#include <olectl.h>
#include <objidl.h>

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %x, expected %s (%x)\n", r, #expect, expect); \
}

#define ole_check(expr) ole_expect(expr, S_OK);

static HMODULE hOleaut32;

static HRESULT (WINAPI *pOleLoadPicture)(LPSTREAM,LONG,BOOL,REFIID,LPVOID*);
static HRESULT (WINAPI *pOleCreatePictureIndirect)(PICTDESC*,REFIID,BOOL,LPVOID*);

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)

/* 1x1 pixel gif */
static const unsigned char gifimage[35] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x01,0x00,0x01,0x00,0x80,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x44,
0x01,0x00,0x3b
};

/* 1x1 pixel jpg */
static const unsigned char jpgimage[285] = {
0xff,0xd8,0xff,0xe0,0x00,0x10,0x4a,0x46,0x49,0x46,0x00,0x01,0x01,0x01,0x01,0x2c,
0x01,0x2c,0x00,0x00,0xff,0xdb,0x00,0x43,0x00,0x05,0x03,0x04,0x04,0x04,0x03,0x05,
0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x07,0x0c,0x08,0x07,0x07,0x07,0x07,0x0f,0x0b,
0x0b,0x09,0x0c,0x11,0x0f,0x12,0x12,0x11,0x0f,0x11,0x11,0x13,0x16,0x1c,0x17,0x13,
0x14,0x1a,0x15,0x11,0x11,0x18,0x21,0x18,0x1a,0x1d,0x1d,0x1f,0x1f,0x1f,0x13,0x17,
0x22,0x24,0x22,0x1e,0x24,0x1c,0x1e,0x1f,0x1e,0xff,0xdb,0x00,0x43,0x01,0x05,0x05,
0x05,0x07,0x06,0x07,0x0e,0x08,0x08,0x0e,0x1e,0x14,0x11,0x14,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0xff,0xc0,
0x00,0x11,0x08,0x00,0x01,0x00,0x01,0x03,0x01,0x22,0x00,0x02,0x11,0x01,0x03,0x11,
0x01,0xff,0xc4,0x00,0x15,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0xff,0xc4,0x00,0x14,0x10,0x01,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xc4,
0x00,0x14,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xff,0xc4,0x00,0x14,0x11,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xda,0x00,0x0c,0x03,0x01,
0x00,0x02,0x11,0x03,0x11,0x00,0x3f,0x00,0xb2,0xc0,0x07,0xff,0xd9
};

/* 1x1 pixel png */
static const unsigned char pngimage[285] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53,
0xde,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0b,0x13,0x00,0x00,0x0b,
0x13,0x01,0x00,0x9a,0x9c,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xd5,
0x06,0x03,0x0f,0x07,0x2d,0x12,0x10,0xf0,0xfd,0x00,0x00,0x00,0x0c,0x49,0x44,0x41,
0x54,0x08,0xd7,0x63,0xf8,0xff,0xff,0x3f,0x00,0x05,0xfe,0x02,0xfe,0xdc,0xcc,0x59,
0xe7,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};

/* 1x1 pixel bmp */
static const unsigned char bmpimage[66] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x00,0x00,
0x00,0x00
};

/* 2x2 pixel gif */
static const unsigned char gif4pixel[42] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x02,0x00,0x02,0x00,0xa1,0x00,0x00,0x00,0x00,0x00,
0x39,0x62,0xfc,0xff,0x1a,0xe5,0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x02,0x00,
0x02,0x00,0x00,0x02,0x03,0x14,0x16,0x05,0x00,0x3b
};

/* APM with an empty metafile with some padding zeros - looks like under Window the
 * metafile data should be at least 20 bytes */
static const unsigned char apmdata[] = {
0xd7,0xcd,0xc6,0x9a, 0x00,0x00,0x00,0x00, 0x00,0x00,0xee,0x02, 0xb1,0x03,0xa0,0x05,
0x00,0x00,0x00,0x00, 0xee,0x53,0x01,0x00, 0x09,0x00,0x00,0x03, 0x13,0x00,0x00,0x00,
0x01,0x00,0x05,0x00, 0x00,0x00,0x00,0x00, 0x03,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

/* MF_TEXTOUT_ON_PATH_BITS from gdi32/tests/metafile.c */
static const unsigned char metafile[] = {
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x19, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x32, 0x0a,
    0x16, 0x00, 0x0b, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x54, 0x65, 0x73, 0x74, 0x03, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x0c, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00
};

/* EMF_TEXTOUT_ON_PATH_BITS from gdi32/tests/metafile.c */
static const unsigned char enhmetafile[] = {
    0x01, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xe7, 0xff, 0xff, 0xff, 0xe9, 0xff, 0xff, 0xff,
    0x20, 0x45, 0x4d, 0x46, 0x00, 0x00, 0x01, 0x00,
    0xf4, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x40, 0x01, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xe2, 0x04, 0x00,
    0x80, 0xa9, 0x03, 0x00, 0x3b, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xc8, 0x41, 0x00, 0x80, 0xbb, 0x41,
    0x0b, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x54, 0x00, 0x00, 0x00,
    0x54, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x3c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00
};


struct NoStatStreamImpl
{
	const IStreamVtbl	*lpVtbl;   
	LONG			ref;

	HGLOBAL			supportHandle;
	ULARGE_INTEGER		streamSize;
	ULARGE_INTEGER		currentPosition;
};
typedef struct NoStatStreamImpl NoStatStreamImpl;
static NoStatStreamImpl* NoStatStreamImpl_Construct(HGLOBAL hGlobal);

static void
test_pic_with_stream(LPSTREAM stream, unsigned int imgsize)
{
	IPicture*	pic = NULL;
	HRESULT		hres;
	LPVOID		pvObj = NULL;
	OLE_HANDLE	handle, hPal;
	OLE_XSIZE_HIMETRIC	width;
	OLE_YSIZE_HIMETRIC	height;
	short		type;
	DWORD		attr;
	ULONG		res;

	pvObj = NULL;
	hres = pOleLoadPicture(stream, imgsize, TRUE, &IID_IPicture, &pvObj);
	pic = pvObj;

	ok(hres == S_OK,"OLP (NULL,..) does not return 0, but 0x%08x\n",hres);
	ok(pic != NULL,"OLP (NULL,..) returns NULL, instead of !NULL\n");
	if (pic == NULL)
		return;

	pvObj = NULL;
	hres = IPicture_QueryInterface (pic, &IID_IPicture, &pvObj);

	ok(hres == S_OK,"IPicture_QI does not return S_OK, but 0x%08x\n", hres);
	ok(pvObj != NULL,"IPicture_QI does return NULL, instead of a ptr\n");

	IPicture_Release ((IPicture*)pvObj);

	handle = 0;
	hres = IPicture_get_Handle (pic, &handle);
	ok(hres == S_OK,"IPicture_get_Handle does not return S_OK, but 0x%08x\n", hres);
	ok(handle != 0, "IPicture_get_Handle returns a NULL handle, but it should be non NULL\n");

	width = 0;
	hres = IPicture_get_Width (pic, &width);
	ok(hres == S_OK,"IPicture_get_Width does not return S_OK, but 0x%08x\n", hres);
	ok(width != 0, "IPicture_get_Width returns 0, but it should not be 0.\n");

	height = 0;
	hres = IPicture_get_Height (pic, &height);
	ok(hres == S_OK,"IPicture_get_Height does not return S_OK, but 0x%08x\n", hres);
	ok(height != 0, "IPicture_get_Height returns 0, but it should not be 0.\n");

	type = 0;
	hres = IPicture_get_Type (pic, &type);
	ok(hres == S_OK,"IPicture_get_Type does not return S_OK, but 0x%08x\n", hres);
	ok(type == PICTYPE_BITMAP, "IPicture_get_Type returns %d, but it should be PICTYPE_BITMAP(%d).\n", type, PICTYPE_BITMAP);

	attr = 0;
	hres = IPicture_get_Attributes (pic, &attr);
	ok(hres == S_OK,"IPicture_get_Attributes does not return S_OK, but 0x%08x\n", hres);
	ok(attr == 0, "IPicture_get_Attributes returns %d, but it should be 0.\n", attr);

	hPal = 0;
	hres = IPicture_get_hPal (pic, &hPal);
	ok(hres == S_OK,"IPicture_get_hPal does not return S_OK, but 0x%08x\n", hres);
	/* a single pixel b/w image has no palette */
	ok(hPal == 0, "IPicture_get_hPal returns %d, but it should be 0.\n", hPal);

	res = IPicture_Release (pic);
	ok (res == 0, "refcount after release is %d, but should be 0?\n", res);
}

static void
test_pic(const unsigned char *imgdata, unsigned int imgsize)
{
	LPSTREAM 	stream;
	HGLOBAL		hglob;
	LPBYTE		data;
	HRESULT		hres;
	LARGE_INTEGER	seekto;
	ULARGE_INTEGER	newpos1;
	DWORD * 	header;
	unsigned int 	i,j;

	/* Let the fun begin */
	hglob = GlobalAlloc (0, imgsize);
	data = GlobalLock (hglob);
	memcpy(data, imgdata, imgsize);
	GlobalUnlock(hglob); data = NULL;

	hres = CreateStreamOnHGlobal (hglob, FALSE, &stream);
	ok (hres == S_OK, "createstreamonhglobal failed? doubt it... hres 0x%08x\n", hres);

	memset(&seekto,0,sizeof(seekto));
	hres = IStream_Seek(stream,seekto,SEEK_CUR,&newpos1);
	ok (hres == S_OK, "istream seek failed? doubt it... hres 0x%08x\n", hres);
	test_pic_with_stream(stream, imgsize);
	
	IStream_Release(stream);

	/* again with Non Statable and Non Seekable stream */
	stream = (LPSTREAM)NoStatStreamImpl_Construct(hglob);
	hglob = 0;  /* Non-statable impl always deletes on release */
	test_pic_with_stream(stream, 0);

	IStream_Release(stream);
	for (i = 1; i <= 8; i++) {
		/* more fun!!! */
		hglob = GlobalAlloc (0, imgsize + i * (2 * sizeof(DWORD)));
		data = GlobalLock (hglob);
		header = (DWORD *)data;

		/* multiple copies of header */
		memcpy(data,"lt\0\0",4);
		header[1] = imgsize;
		for (j = 2; j <= i; j++) {
			memcpy(&(header[2 * (j - 1)]), header, 2 * sizeof(DWORD));
		}
		memcpy(data + i * (2 * sizeof(DWORD)), imgdata, imgsize);
		GlobalUnlock(hglob); data = NULL;

		hres = CreateStreamOnHGlobal (hglob, FALSE, &stream);
		ok (hres == S_OK, "createstreamonhglobal failed? doubt it... hres 0x%08x\n", hres);

		memset(&seekto,0,sizeof(seekto));
		hres = IStream_Seek(stream,seekto,SEEK_CUR,&newpos1);
		ok (hres == S_OK, "istream seek failed? doubt it... hres 0x%08x\n", hres);
		test_pic_with_stream(stream, imgsize);
	
		IStream_Release(stream);

		/* again with Non Statable and Non Seekable stream */
		stream = (LPSTREAM)NoStatStreamImpl_Construct(hglob);
		hglob = 0;  /* Non-statable impl always deletes on release */
		test_pic_with_stream(stream, 0);

		IStream_Release(stream);
	}
}

static void test_empty_image(void) {
	LPBYTE		data;
	LPSTREAM	stream;
	IPicture*	pic = NULL;
	HRESULT		hres;
	LPVOID		pvObj = NULL;
	HGLOBAL		hglob;
	OLE_HANDLE	handle;
	ULARGE_INTEGER	newpos1;
	LARGE_INTEGER	seekto;
	short		type;
	DWORD		attr;

	/* Empty image. Happens occasionally in VB programs. */
	hglob = GlobalAlloc (0, 8);
	data = GlobalLock (hglob);
	memcpy(data,"lt\0\0",4);
	((DWORD*)data)[1] = 0;
	hres = CreateStreamOnHGlobal (hglob, TRUE, &stream);
	ok (hres == S_OK, "CreatestreamOnHGlobal failed? doubt it... hres 0x%08x\n", hres);

	memset(&seekto,0,sizeof(seekto));
	hres = IStream_Seek(stream,seekto,SEEK_CUR,&newpos1);
	ok (hres == S_OK, "istream seek failed? doubt it... hres 0x%08x\n", hres);

	pvObj = NULL;
	hres = pOleLoadPicture(stream, 8, TRUE, &IID_IPicture, &pvObj);
	pic = pvObj;
	ok(hres == S_OK,"empty picture not loaded, hres 0x%08x\n", hres);
	ok(pic != NULL,"empty picture not loaded, pic is NULL\n");

	hres = IPicture_get_Type (pic, &type);
	ok (hres == S_OK,"empty picture get type failed with hres 0x%08x\n", hres);
	ok (type == PICTYPE_NONE,"type is %d, but should be PICTYPE_NONE(0)\n", type);

	attr = 0xdeadbeef;
	hres = IPicture_get_Attributes (pic, &attr);
	ok (hres == S_OK,"empty picture get attributes failed with hres 0x%08x\n", hres);
	ok (attr == 0,"attr is %d, but should be 0\n", attr);

	hres = IPicture_get_Handle (pic, &handle);
	ok (hres == S_OK,"empty picture get handle failed with hres 0x%08x\n", hres);
	ok (handle == 0, "empty picture get handle did not return 0, but 0x%08x\n", handle);
	IPicture_Release (pic);
}

static void test_empty_image_2(void) {
	LPBYTE		data;
	LPSTREAM	stream;
	IPicture*	pic = NULL;
	HRESULT		hres;
	LPVOID		pvObj = NULL;
	HGLOBAL		hglob;
	ULARGE_INTEGER	newpos1;
	LARGE_INTEGER	seekto;
	short		type;

	/* Empty image at random stream position. */
	hglob = GlobalAlloc (0, 200);
	data = GlobalLock (hglob);
	data += 42;
	memcpy(data,"lt\0\0",4);
	((DWORD*)data)[1] = 0;
	hres = CreateStreamOnHGlobal (hglob, TRUE, &stream);
	ok (hres == S_OK, "CreatestreamOnHGlobal failed? doubt it... hres 0x%08x\n", hres);

	memset(&seekto,0,sizeof(seekto));
	seekto.u.LowPart = 42;
	hres = IStream_Seek(stream,seekto,SEEK_CUR,&newpos1);
	ok (hres == S_OK, "istream seek failed? doubt it... hres 0x%08x\n", hres);

	pvObj = NULL;
	hres = pOleLoadPicture(stream, 8, TRUE, &IID_IPicture, &pvObj);
	pic = pvObj;
	ok(hres == S_OK,"empty picture not loaded, hres 0x%08x\n", hres);
	ok(pic != NULL,"empty picture not loaded, pic is NULL\n");

	hres = IPicture_get_Type (pic, &type);
	ok (hres == S_OK,"empty picture get type failed with hres 0x%08x\n", hres);
	ok (type == PICTYPE_NONE,"type is %d, but should be PICTYPE_NONE(0)\n", type);

	IPicture_Release (pic);
}

static void test_Invoke(void)
{
    IPictureDisp *picdisp;
    HRESULT hr;
    VARIANTARG vararg;
    DISPPARAMS dispparams;
    VARIANT varresult;
    IStream *stream;
    HGLOBAL hglob;
    void *data;

	hglob = GlobalAlloc (0, sizeof(gifimage));
	data = GlobalLock(hglob);
	memcpy(data, gifimage, sizeof(gifimage));
    GlobalUnlock(hglob);

	hr = CreateStreamOnHGlobal (hglob, FALSE, &stream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

	hr = pOleLoadPicture(stream, sizeof(gifimage), TRUE, &IID_IPictureDisp, (void **)&picdisp);
    IStream_Release(stream);
    ok_ole_success(hr, "OleLoadPicture");

    V_VT(&vararg) = VT_BOOL;
    V_BOOL(&vararg) = VARIANT_FALSE;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_IPictureDisp, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_UNKNOWNNAME, "IPictureDisp_Invoke should have returned DISP_E_UNKNOWNNAME instead of 0x%08x\n", hr);
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_IUnknown, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_UNKNOWNNAME, "IPictureDisp_Invoke should have returned DISP_E_UNKNOWNNAME instead of 0x%08x\n", hr);

    dispparams.cArgs = 0;
    dispparams.rgvarg = NULL;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "IPictureDisp_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08x\n", hr);

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYPUT, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTOPTIONAL, "IPictureDisp_Invoke should have returned DISP_E_PARAMNOTOPTIONAL instead of 0x%08x\n", hr);

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTOPTIONAL, "IPictureDisp_Invoke should have returned DISP_E_PARAMNOTOPTIONAL instead of 0x%08x\n", hr);

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, &varresult, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTOPTIONAL, "IPictureDisp_Invoke should have returned DISP_E_PARAMNOTOPTIONAL instead of 0x%08x\n", hr);

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok_ole_success(hr, "IPictureDisp_Invoke");
    ok(V_VT(&varresult) == VT_I4, "V_VT(&varresult) should have been VT_UINT instead of %d\n", V_VT(&varresult));

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IPictureDisp_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    hr = IPictureDisp_Invoke(picdisp, 0xdeadbeef, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IPictureDisp_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "IPictureDisp_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08x\n", hr);

    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "IPictureDisp_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08x\n", hr);

    IPictureDisp_Release(picdisp);
}

static void test_OleCreatePictureIndirect(void)
{
    IPicture *pict;
    HRESULT hr;
    short type;
    OLE_HANDLE handle;

    if(!pOleCreatePictureIndirect)
    {
        skip("Skipping OleCreatePictureIndirect tests\n");
        return;
    }

    hr = pOleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (void**)&pict);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = IPicture_get_Type(pict, &type);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(type == PICTYPE_UNINITIALIZED, "type %d\n", type);

    hr = IPicture_get_Handle(pict, &handle);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(handle == 0, "handle %08x\n", handle);

    IPicture_Release(pict);
}

static void test_apm(void)
{
    OLE_HANDLE handle;
    LPSTREAM stream;
    IPicture *pict;
    HGLOBAL hglob;
    LPBYTE *data;
    LONG cxy;
    BOOL keep;
    short type;

    hglob = GlobalAlloc (0, sizeof(apmdata));
    data = GlobalLock(hglob);
    memcpy(data, apmdata, sizeof(apmdata));

    ole_check(CreateStreamOnHGlobal(hglob, TRUE, &stream));
    ole_check(OleLoadPictureEx(stream, sizeof(apmdata), TRUE, &IID_IPicture, 100, 100, 0, (LPVOID *)&pict));

    ole_check(IPicture_get_Handle(pict, &handle));
    ok(handle != 0, "handle is null\n");

    ole_check(IPicture_get_Type(pict, &type));
    expect_eq(type, PICTYPE_METAFILE, short, "%d");

    ole_check(IPicture_get_Height(pict, &cxy));
    expect_eq(cxy,  1667, LONG, "%d");

    ole_check(IPicture_get_Width(pict, &cxy));
    expect_eq(cxy,  1323, LONG, "%d");

    ole_check(IPicture_get_KeepOriginalFormat(pict, &keep));
    todo_wine expect_eq(keep, FALSE, LONG, "%d");

    ole_expect(IPicture_get_hPal(pict, &handle), E_FAIL);
    IPicture_Release(pict);
    IStream_Release(stream);
}

static void test_metafile(void)
{
    LPSTREAM stream;
    IPicture *pict;
    HGLOBAL hglob;
    LPBYTE *data;

    hglob = GlobalAlloc (0, sizeof(metafile));
    data = GlobalLock(hglob);
    memcpy(data, metafile, sizeof(metafile));

    ole_check(CreateStreamOnHGlobal(hglob, TRUE, &stream));
    /* Windows does not load simple metafiles */
    ole_expect(OleLoadPictureEx(stream, sizeof(metafile), TRUE, &IID_IPicture, 100, 100, 0, (LPVOID *)&pict), E_FAIL);

    IStream_Release(stream);
}

static void test_enhmetafile(void)
{
    OLE_HANDLE handle;
    LPSTREAM stream;
    IPicture *pict;
    HGLOBAL hglob;
    LPBYTE *data;
    LONG cxy;
    BOOL keep;
    short type;

    hglob = GlobalAlloc (0, sizeof(enhmetafile));
    data = GlobalLock(hglob);
    memcpy(data, enhmetafile, sizeof(enhmetafile));

    ole_check(CreateStreamOnHGlobal(hglob, TRUE, &stream));
    ole_check(OleLoadPictureEx(stream, sizeof(enhmetafile), TRUE, &IID_IPicture, 10, 10, 0, (LPVOID *)&pict));

    ole_check(IPicture_get_Handle(pict, &handle));
    ok(handle != 0, "handle is null\n");

    ole_check(IPicture_get_Type(pict, &type));
    expect_eq(type, PICTYPE_ENHMETAFILE, short, "%d");

    ole_check(IPicture_get_Height(pict, &cxy));
    expect_eq(cxy, -23, LONG, "%d");

    ole_check(IPicture_get_Width(pict, &cxy));
    expect_eq(cxy, -25, LONG, "%d");

    ole_check(IPicture_get_KeepOriginalFormat(pict, &keep));
    todo_wine expect_eq(keep, FALSE, LONG, "%d");

    IPicture_Release(pict);
    IStream_Release(stream);
}

static void test_Render(void)
{
    IPicture *pic;
    HRESULT hres;
    short type;
    PICTDESC desc;
    HDC hdc = GetDC(0);

    /* test IPicture::Render return code on uninitialized picture */
    OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (VOID**)&pic);
    hres = IPicture_get_Type(pic, &type);
    ok(hres == S_OK, "IPicture_get_Type does not return S_OK, but 0x%08x\n", hres);
    ok(type == PICTYPE_UNINITIALIZED, "Expected type = PICTYPE_UNINITIALIZED, got = %d\n", type);
    /* zero dimensions */
    hres = IPicture_Render(pic, hdc, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 10, 0, 0, 10, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 10, 0, 0, 0, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 10, 0, 0, 0, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 0, 10, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 0, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 0, 0, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    /* nonzero dimensions, PICTYPE_UNINITIALIZED */
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 10, 0, 0, 10, 10, NULL);
    ole_expect(hres, S_OK);
    IPicture_Release(pic);

    desc.cbSizeofstruct = sizeof(PICTDESC);
    desc.picType = PICTYPE_ICON;
    desc.u.icon.hicon = LoadIcon(NULL, IDI_APPLICATION);
    if(!desc.u.icon.hicon){
        win_skip("LoadIcon failed. Skipping...\n");
        ReleaseDC(NULL, hdc);
        return;
    }

    OleCreatePictureIndirect(&desc, &IID_IPicture, TRUE, (VOID**)&pic);
    /* zero dimensions, PICTYPE_ICON */
    hres = IPicture_Render(pic, hdc, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 10, 0, 0, 10, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 10, 0, 0, 0, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 10, 0, 0, 0, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 0, 10, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 10, 0, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = IPicture_Render(pic, hdc, 0, 0, 0, 0, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    IPicture_Release(pic);

    ReleaseDC(NULL, hdc);
}

static void test_get_Attributes(void)
{
    IPicture *pic;
    HRESULT hres;
    short type;
    DWORD attr;

    OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (VOID**)&pic);
    hres = IPicture_get_Type(pic, &type);
    ok(hres == S_OK, "IPicture_get_Type does not return S_OK, but 0x%08x\n", hres);
    ok(type == PICTYPE_UNINITIALIZED, "Expected type = PICTYPE_UNINITIALIZED, got = %d\n", type);

    hres = IPicture_get_Attributes(pic, NULL);
    ole_expect(hres, E_POINTER);

    attr = 0xdeadbeef;
    hres = IPicture_get_Attributes(pic, &attr);
    ole_expect(hres, S_OK);
    ok(attr == 0, "IPicture_get_Attributes does not reset attr to zero, got %d\n", attr);

    IPicture_Release(pic);
}

static void test_get_Handle(void)
{
    IPicture *pic;
    HRESULT hres;

    OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (VOID**)&pic);

    hres = IPicture_get_Handle(pic, NULL);
    ole_expect(hres, E_POINTER);

    IPicture_Release(pic);
}

static void test_get_Type(void)
{
    IPicture *pic;
    HRESULT hres;

    OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (VOID**)&pic);

    hres = IPicture_get_Type(pic, NULL);
    ole_expect(hres, E_POINTER);

    IPicture_Release(pic);
}

START_TEST(olepicture)
{
	hOleaut32 = GetModuleHandleA("oleaut32.dll");
	pOleLoadPicture = (void*)GetProcAddress(hOleaut32, "OleLoadPicture");
	pOleCreatePictureIndirect = (void*)GetProcAddress(hOleaut32, "OleCreatePictureIndirect");
	if (!pOleLoadPicture)
	{
	    skip("OleLoadPicture is not available\n");
	    return;
	}

	/* Test regular 1x1 pixel images of gif, jpg, bmp type */
        test_pic(gifimage, sizeof(gifimage));
	test_pic(jpgimage, sizeof(jpgimage));
	test_pic(bmpimage, sizeof(bmpimage));
        test_pic(gif4pixel, sizeof(gif4pixel));
	/* FIXME: No PNG support yet in Wine or in older Windows... */
	if (0) test_pic(pngimage, sizeof(pngimage));
	test_empty_image();
	test_empty_image_2();
        test_apm();
        test_metafile();
        test_enhmetafile();

	test_Invoke();
        test_OleCreatePictureIndirect();
        test_Render();
        test_get_Attributes();
        test_get_Handle();
        test_get_Type();
}


/* Helper functions only ... */


static void NoStatStreamImpl_Destroy(NoStatStreamImpl* This)
{
  GlobalFree(This->supportHandle);
  This->supportHandle=0;
  HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI NoStatStreamImpl_AddRef(
		IStream* iface)
{
  NoStatStreamImpl* const This=(NoStatStreamImpl*)iface;
  return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI NoStatStreamImpl_QueryInterface(
		  IStream*     iface,
		  REFIID         riid,	      /* [in] */
		  void**         ppvObject)   /* [iid_is][out] */
{
  NoStatStreamImpl* const This=(NoStatStreamImpl*)iface;
  if (ppvObject==0) return E_INVALIDARG;
  *ppvObject = 0;
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
  {
    *ppvObject = This;
  }
  else if (memcmp(&IID_IStream, riid, sizeof(IID_IStream)) == 0)
  {
    *ppvObject = This;
  }

  if ((*ppvObject)==0)
    return E_NOINTERFACE;
  NoStatStreamImpl_AddRef(iface);
  return S_OK;
}

static ULONG WINAPI NoStatStreamImpl_Release(
		IStream* iface)
{
  NoStatStreamImpl* const This=(NoStatStreamImpl*)iface;
  ULONG newRef = InterlockedDecrement(&This->ref);
  if (newRef==0)
    NoStatStreamImpl_Destroy(This);
  return newRef;
}

static HRESULT WINAPI NoStatStreamImpl_Read(
		  IStream*     iface,
		  void*          pv,        /* [length_is][size_is][out] */
		  ULONG          cb,        /* [in] */
		  ULONG*         pcbRead)   /* [out] */
{
  NoStatStreamImpl* const This=(NoStatStreamImpl*)iface;
  void* supportBuffer;
  ULONG bytesReadBuffer;
  ULONG bytesToReadFromBuffer;

  if (pcbRead==0)
    pcbRead = &bytesReadBuffer;
  bytesToReadFromBuffer = min( This->streamSize.u.LowPart - This->currentPosition.u.LowPart, cb);
  supportBuffer = GlobalLock(This->supportHandle);
  memcpy(pv, (char *) supportBuffer+This->currentPosition.u.LowPart, bytesToReadFromBuffer);
  This->currentPosition.u.LowPart+=bytesToReadFromBuffer;
  *pcbRead = bytesToReadFromBuffer;
  GlobalUnlock(This->supportHandle);
  if(*pcbRead == cb)
    return S_OK;
  return S_FALSE;
}

static HRESULT WINAPI NoStatStreamImpl_Write(
	          IStream*     iface,
		  const void*    pv,          /* [size_is][in] */
		  ULONG          cb,          /* [in] */
		  ULONG*         pcbWritten)  /* [out] */
{
  NoStatStreamImpl* const This=(NoStatStreamImpl*)iface;
  void*          supportBuffer;
  ULARGE_INTEGER newSize;
  ULONG          bytesWritten = 0;

  if (pcbWritten == 0)
    pcbWritten = &bytesWritten;
  if (cb == 0)
    return S_OK;
  newSize.u.HighPart = 0;
  newSize.u.LowPart = This->currentPosition.u.LowPart + cb;
  if (newSize.u.LowPart > This->streamSize.u.LowPart)
   IStream_SetSize(iface, newSize);

  supportBuffer = GlobalLock(This->supportHandle);
  memcpy((char *) supportBuffer+This->currentPosition.u.LowPart, pv, cb);
  This->currentPosition.u.LowPart+=cb;
  *pcbWritten = cb;
  GlobalUnlock(This->supportHandle);
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_Seek(
		  IStream*      iface,
		  LARGE_INTEGER   dlibMove,         /* [in] */
		  DWORD           dwOrigin,         /* [in] */
		  ULARGE_INTEGER* plibNewPosition) /* [out] */
{
  NoStatStreamImpl* const This=(NoStatStreamImpl*)iface;
  ULARGE_INTEGER newPosition;
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      newPosition.u.HighPart = 0;
      newPosition.u.LowPart = 0;
      break;
    case STREAM_SEEK_CUR:
      newPosition = This->currentPosition;
      break;
    case STREAM_SEEK_END:
      newPosition = This->streamSize;
      break;
    default:
      return STG_E_INVALIDFUNCTION;
  }
  if (dlibMove.QuadPart < 0 && newPosition.QuadPart < -dlibMove.QuadPart)
      return STG_E_INVALIDFUNCTION;
  newPosition.QuadPart += dlibMove.QuadPart;
  if (plibNewPosition) *plibNewPosition = newPosition;
  This->currentPosition = newPosition;
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_SetSize(
				     IStream*      iface,
				     ULARGE_INTEGER  libNewSize)   /* [in] */
{
  NoStatStreamImpl* const This=(NoStatStreamImpl*)iface;
  HGLOBAL supportHandle;
  if (libNewSize.u.HighPart != 0)
    return STG_E_INVALIDFUNCTION;
  if (This->streamSize.u.LowPart == libNewSize.u.LowPart)
    return S_OK;
  supportHandle = GlobalReAlloc(This->supportHandle, libNewSize.u.LowPart, 0);
  if (supportHandle == 0)
    return STG_E_MEDIUMFULL;
  This->supportHandle = supportHandle;
  This->streamSize.u.LowPart = libNewSize.u.LowPart;
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_CopyTo(
				    IStream*      iface,
				    IStream*      pstm,         /* [unique][in] */
				    ULARGE_INTEGER  cb,           /* [in] */
				    ULARGE_INTEGER* pcbRead,      /* [out] */
				    ULARGE_INTEGER* pcbWritten)   /* [out] */
{
  HRESULT        hr = S_OK;
  BYTE           tmpBuffer[128];
  ULONG          bytesRead, bytesWritten, copySize;
  ULARGE_INTEGER totalBytesRead;
  ULARGE_INTEGER totalBytesWritten;

  if ( pstm == 0 )
    return STG_E_INVALIDPOINTER;
  totalBytesRead.u.LowPart = totalBytesRead.u.HighPart = 0;
  totalBytesWritten.u.LowPart = totalBytesWritten.u.HighPart = 0;

  while ( cb.u.LowPart > 0 )
  {
    if ( cb.u.LowPart >= 128 )
      copySize = 128;
    else
      copySize = cb.u.LowPart;
    IStream_Read(iface, tmpBuffer, copySize, &bytesRead);
    totalBytesRead.u.LowPart += bytesRead;
    IStream_Write(pstm, tmpBuffer, bytesRead, &bytesWritten);
    totalBytesWritten.u.LowPart += bytesWritten;
    if (bytesRead != bytesWritten)
    {
      hr = STG_E_MEDIUMFULL;
      break;
    }
    if (bytesRead!=copySize)
      cb.u.LowPart = 0;
    else
      cb.u.LowPart -= bytesRead;
  }
  if (pcbRead)
  {
    pcbRead->u.LowPart = totalBytesRead.u.LowPart;
    pcbRead->u.HighPart = totalBytesRead.u.HighPart;
  }

  if (pcbWritten)
  {
    pcbWritten->u.LowPart = totalBytesWritten.u.LowPart;
    pcbWritten->u.HighPart = totalBytesWritten.u.HighPart;
  }
  return hr;
}

static HRESULT WINAPI NoStatStreamImpl_Commit(IStream* iface,DWORD grfCommitFlags)
{
  return S_OK;
}
static HRESULT WINAPI NoStatStreamImpl_Revert(IStream* iface) { return S_OK; }

static HRESULT WINAPI NoStatStreamImpl_LockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_UnlockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_Stat(
		  IStream*     iface,
		  STATSTG*     pstatstg,     /* [out] */
		  DWORD        grfStatFlag)  /* [in] */
{
  return E_NOTIMPL;
}

static HRESULT WINAPI NoStatStreamImpl_Clone(
		  IStream*     iface,
		  IStream**    ppstm) /* [out] */
{
  return E_NOTIMPL;
}
static const IStreamVtbl NoStatStreamImpl_Vtbl;

/*
    Build an object that implements IStream, without IStream_Stat capabilities.
    Receives a memory handle with data buffer. If memory handle is non-null,
    it is assumed to be unlocked, otherwise an internal memory handle is allocated.
    In any case the object takes ownership of memory handle and will free it on
    object release.
 */
static NoStatStreamImpl* NoStatStreamImpl_Construct(HGLOBAL hGlobal)
{
  NoStatStreamImpl* newStream;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(NoStatStreamImpl));
  if (newStream!=0)
  {
    newStream->lpVtbl = &NoStatStreamImpl_Vtbl;
    newStream->ref    = 1;
    newStream->supportHandle = hGlobal;

    if (!newStream->supportHandle)
      newStream->supportHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD |
					     GMEM_SHARE, 0);
    newStream->currentPosition.u.HighPart = 0;
    newStream->currentPosition.u.LowPart = 0;
    newStream->streamSize.u.HighPart = 0;
    newStream->streamSize.u.LowPart  = GlobalSize(newStream->supportHandle);
  }
  return newStream;
}


static const IStreamVtbl NoStatStreamImpl_Vtbl =
{
    NoStatStreamImpl_QueryInterface,
    NoStatStreamImpl_AddRef,
    NoStatStreamImpl_Release,
    NoStatStreamImpl_Read,
    NoStatStreamImpl_Write,
    NoStatStreamImpl_Seek,
    NoStatStreamImpl_SetSize,
    NoStatStreamImpl_CopyTo,
    NoStatStreamImpl_Commit,
    NoStatStreamImpl_Revert,
    NoStatStreamImpl_LockRegion,
    NoStatStreamImpl_UnlockRegion,
    NoStatStreamImpl_Stat,
    NoStatStreamImpl_Clone
};
