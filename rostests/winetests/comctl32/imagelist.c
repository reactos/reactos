/*
 * Unit test suite for imagelist control.
 *
 * Copyright 2004 Michael Stefaniuc
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2007 Dmitry Timoshkov
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
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "commctrl.h" /* must be included after objbase.h to get ImageList_Write */

#include "wine/test.h"

#undef VISIBLE

#ifdef VISIBLE
#define WAIT Sleep (1000)
#define REDRAW(hwnd) RedrawWindow (hwnd, NULL, 0, RDW_UPDATENOW)
#else
#define WAIT
#define REDRAW(hwnd)
#endif

#define IMAGELIST_MAGIC (('L' << 8) | 'I')

#include "pshpack2.h"
/* Header used by ImageList_Read() and ImageList_Write() */
typedef struct _ILHEAD
{
    USHORT	usMagic;
    USHORT	usVersion;
    WORD	cCurImage;
    WORD	cMaxImage;
    WORD	cGrow;
    WORD	cx;
    WORD	cy;
    COLORREF	bkcolor;
    WORD	flags;
    SHORT	ovls[4];
} ILHEAD;
#include "poppack.h"

static BOOL (WINAPI *pImageList_DrawIndirect)(IMAGELISTDRAWPARAMS*) = NULL;

static HDC desktopDC;
static HINSTANCE hinst;

/* These macros build cursor/bitmap data in 4x4 pixel blocks */
#define B(x,y) ((x?0xf0:0)|(y?0xf:0))
#define ROW1(a,b,c,d,e,f,g,h) B(a,b),B(c,d),B(e,f),B(g,h)
#define ROW32(a,b,c,d,e,f,g,h) ROW1(a,b,c,d,e,f,g,h), ROW1(a,b,c,d,e,f,g,h), \
  ROW1(a,b,c,d,e,f,g,h), ROW1(a,b,c,d,e,f,g,h)
#define ROW2(a,b,c,d,e,f,g,h,i,j,k,l) ROW1(a,b,c,d,e,f,g,h),B(i,j),B(k,l)
#define ROW48(a,b,c,d,e,f,g,h,i,j,k,l) ROW2(a,b,c,d,e,f,g,h,i,j,k,l), \
  ROW2(a,b,c,d,e,f,g,h,i,j,k,l), ROW2(a,b,c,d,e,f,g,h,i,j,k,l), \
  ROW2(a,b,c,d,e,f,g,h,i,j,k,l)

static const BYTE empty_bits[48*48/8];

static const BYTE icon_bits[32*32/8] =
{
  ROW32(0,0,0,0,0,0,0,0),
  ROW32(0,0,1,1,1,1,0,0),
  ROW32(0,1,1,1,1,1,1,0),
  ROW32(0,1,1,0,0,1,1,0),
  ROW32(0,1,1,0,0,1,1,0),
  ROW32(0,1,1,1,1,1,1,0),
  ROW32(0,0,1,1,1,1,0,0),
  ROW32(0,0,0,0,0,0,0,0)
};

static const BYTE bitmap_bits[48*48/8] =
{
  ROW48(0,0,0,0,0,0,0,0,0,0,0,0),
  ROW48(0,1,1,1,1,1,1,1,1,1,1,0),
  ROW48(0,1,1,0,0,0,0,0,0,1,1,0),
  ROW48(0,1,0,0,0,0,0,0,1,0,1,0),
  ROW48(0,1,0,0,0,0,0,1,0,0,1,0),
  ROW48(0,1,0,0,0,0,1,0,0,0,1,0),
  ROW48(0,1,0,0,0,1,0,0,0,0,1,0),
  ROW48(0,1,0,0,1,0,0,0,0,0,1,0),
  ROW48(0,1,0,1,0,0,0,0,0,0,1,0),
  ROW48(0,1,1,0,0,0,0,0,0,1,1,0),
  ROW48(0,1,1,1,1,1,1,1,1,1,1,0),
  ROW48(0,0,0,0,0,0,0,0,0,0,0,0)
};

static HIMAGELIST createImageList(int cx, int cy)
{
    /* Create an ImageList and put an image into it */
    HIMAGELIST himl = ImageList_Create(cx, cy, ILC_COLOR, 1, 1);
    HBITMAP hbm = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ImageList_Add(himl, hbm, NULL);
    return himl;
}

static HWND create_a_window(void)
{
    char className[] = "bmwnd";
    char winName[]   = "Test Bitmap";
    HWND hWnd;
    static int registered = 0;

    if (!registered)
    {
        WNDCLASSA cls;

        cls.style         = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
        cls.lpfnWndProc   = DefWindowProcA;
        cls.cbClsExtra    = 0;
        cls.cbWndExtra    = 0;
        cls.hInstance     = 0;
        cls.hIcon         = LoadIconA (0, (LPSTR)IDI_APPLICATION);
        cls.hCursor       = LoadCursorA (0, (LPSTR)IDC_ARROW);
        cls.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
        cls.lpszMenuName  = 0;
        cls.lpszClassName = className;

        RegisterClassA (&cls);
        registered = 1;
    }

    /* Setup window */
    hWnd = CreateWindowA (className, winName,
       WS_OVERLAPPEDWINDOW ,
       CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
       0, hinst, 0);

#ifdef VISIBLE
    ShowWindow (hWnd, SW_SHOW);
#endif
    REDRAW(hWnd);
    WAIT;

    return hWnd;
}

static HDC show_image(HWND hwnd, HIMAGELIST himl, int idx, int size,
                      LPCSTR loc, BOOL clear)
{
    HDC hdc = NULL;
#ifdef VISIBLE
    if (!himl) return NULL;

    SetWindowText(hwnd, loc);
    hdc = GetDC(hwnd);
    ImageList_Draw(himl, idx, hdc, 0, 0, ILD_TRANSPARENT);

    REDRAW(hwnd);
    WAIT;

    if (clear)
    {
        BitBlt(hdc, 0, 0, size, size, hdc, size+1, size+1, SRCCOPY);
        ReleaseDC(hwnd, hdc);
        hdc = NULL;
    }
#endif /* VISIBLE */
    return hdc;
}

/* Useful for checking differences */
#if 0
static void dump_bits(const BYTE *p, const BYTE *q, int size)
{
  int i, j;

  size /= 8;

  for (i = 0; i < size * 2; i++)
  {
      printf("|");
      for (j = 0; j < size; j++)
          printf("%c%c", p[j] & 0xf0 ? 'X' : ' ', p[j] & 0xf ? 'X' : ' ');
      printf(" -- ");
      for (j = 0; j < size; j++)
          printf("%c%c", q[j] & 0xf0 ? 'X' : ' ', q[j] & 0xf ? 'X' : ' ');
      printf("|\n");
      p += size * 4;
      q += size * 4;
  }
  printf("\n");
}
#endif

static void check_bits(HWND hwnd, HIMAGELIST himl, int idx, int size,
                       const BYTE *checkbits, LPCSTR loc)
{
#ifdef VISIBLE
    BYTE bits[100*100/8];
    COLORREF c;
    HDC hdc;
    int x, y, i = -1;

    if (!himl) return;

    memset(bits, 0, sizeof(bits));
    hdc = show_image(hwnd, himl, idx, size, loc, FALSE);

    c = GetPixel(hdc, 0, 0);

    for (y = 0; y < size; y ++)
    {
        for (x = 0; x < size; x++)
        {
            if (!(x & 0x7)) i++;
            if (GetPixel(hdc, x, y) != c) bits[i] |= (0x80 >> (x & 0x7));
        }
    }

    BitBlt(hdc, 0, 0, size, size, hdc, size+1, size+1, SRCCOPY);
    ReleaseDC(hwnd, hdc);

    ok (memcmp(bits, checkbits, (size * size)/8) == 0,
        "%s: bits different\n", loc);
    if (memcmp(bits, checkbits, (size * size)/8))
        dump_bits(bits, checkbits, size);
#endif /* VISIBLE */
}

static void testHotspot (void)
{
    struct hotspot {
        int dx;
        int dy;
    };

#define SIZEX1 47
#define SIZEY1 31
#define SIZEX2 11
#define SIZEY2 17
#define HOTSPOTS_MAX 4       /* Number of entries in hotspots */
    static const struct hotspot hotspots[HOTSPOTS_MAX] = {
        { 10, 7 },
        { SIZEX1, SIZEY1 },
        { -9, -8 },
        { -7, 35 }
    };
    int i, j, ret;
    HIMAGELIST himl1 = createImageList(SIZEX1, SIZEY1);
    HIMAGELIST himl2 = createImageList(SIZEX2, SIZEY2);
    HWND hwnd = create_a_window();


    for (i = 0; i < HOTSPOTS_MAX; i++) {
        for (j = 0; j < HOTSPOTS_MAX; j++) {
            int dx1 = hotspots[i].dx;
            int dy1 = hotspots[i].dy;
            int dx2 = hotspots[j].dx;
            int dy2 = hotspots[j].dy;
            int correctx, correcty, newx, newy;
            char loc[256];
            HIMAGELIST himlNew;
            POINT ppt;

            ret = ImageList_BeginDrag(himl1, 0, dx1, dy1);
            ok(ret != 0, "BeginDrag failed for { %d, %d }\n", dx1, dy1);
            sprintf(loc, "BeginDrag (%d,%d)\n", i, j);
            show_image(hwnd, himl1, 0, max(SIZEX1, SIZEY1), loc, TRUE);

            /* check merging the dragged image with a second image */
            ret = ImageList_SetDragCursorImage(himl2, 0, dx2, dy2);
            ok(ret != 0, "SetDragCursorImage failed for {%d, %d}{%d, %d}\n",
                    dx1, dy1, dx2, dy2);
            sprintf(loc, "SetDragCursorImage (%d,%d)\n", i, j);
            show_image(hwnd, himl2, 0, max(SIZEX2, SIZEY2), loc, TRUE);

            /* check new hotspot, it should be the same like the old one */
            himlNew = ImageList_GetDragImage(NULL, &ppt);
            ok(ppt.x == dx1 && ppt.y == dy1,
                    "Expected drag hotspot [%d,%d] got [%d,%d]\n",
                    dx1, dy1, ppt.x, ppt.y);
            /* check size of new dragged image */
            ImageList_GetIconSize(himlNew, &newx, &newy);
            correctx = max(SIZEX1, max(SIZEX2 + dx2, SIZEX1 - dx2));
            correcty = max(SIZEY1, max(SIZEY2 + dy2, SIZEY1 - dy2));
            ok(newx == correctx && newy == correcty,
                    "Expected drag image size [%d,%d] got [%d,%d]\n",
                    correctx, correcty, newx, newy);
            sprintf(loc, "GetDragImage (%d,%d)\n", i, j);
            show_image(hwnd, himlNew, 0, max(correctx, correcty), loc, TRUE);
            ImageList_EndDrag();
        }
    }
#undef SIZEX1
#undef SIZEY1
#undef SIZEX2
#undef SIZEY2
#undef HOTSPOTS_MAX
    ImageList_Destroy(himl2);
    ImageList_Destroy(himl1);
    DestroyWindow(hwnd);
}

static BOOL DoTest1(void)
{
    HIMAGELIST himl ;

    HICON hicon1 ;
    HICON hicon2 ;
    HICON hicon3 ;

    /* create an imagelist to play with */
    himl = ImageList_Create(84,84,0x10,0,3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* remove when nothing exists */
    ok(!ImageList_Remove(himl,0),"removed nonexistent icon\n");
    /* removing everything from an empty imagelist should succeed */
    ok(ImageList_RemoveAll(himl),"removed nonexistent icon\n");

    /* add three */
    ok(0==ImageList_AddIcon(himl, hicon1),"failed to add icon1\n");
    ok(1==ImageList_AddIcon(himl, hicon2),"failed to add icon2\n");
    ok(2==ImageList_AddIcon(himl, hicon3),"failed to add icon3\n");

    /* remove an index out of range */
    ok(!ImageList_Remove(himl,4711),"removed nonexistent icon\n");

    /* remove three */
    ok(ImageList_Remove(himl,0),"can't remove 0\n");
    ok(ImageList_Remove(himl,0),"can't remove 0\n");
    ok(ImageList_Remove(himl,0),"can't remove 0\n");

    /* remove one extra */
    ok(!ImageList_Remove(himl,0),"removed nonexistent icon\n");

    /* check SetImageCount/GetImageCount */
    ok(ImageList_SetImageCount(himl, 3), "couldn't increase image count\n");
    ok(ImageList_GetImageCount(himl) == 3, "invalid image count after increase\n");
    ok(ImageList_SetImageCount(himl, 1), "couldn't decrease image count\n");
    ok(ImageList_GetImageCount(himl) == 1, "invalid image count after decrease to 1\n");
    ok(ImageList_SetImageCount(himl, 0), "couldn't decrease image count\n");
    ok(ImageList_GetImageCount(himl) == 0, "invalid image count after decrease to 0\n");

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    ok(DestroyIcon(hicon1),"icon 1 wasn't deleted\n");
    ok(DestroyIcon(hicon2),"icon 2 wasn't deleted\n");
    ok(DestroyIcon(hicon3),"icon 3 wasn't deleted\n");

    return TRUE;
}

static BOOL DoTest2(void)
{
    HIMAGELIST himl ;

    HICON hicon1 ;
    HICON hicon2 ;
    HICON hicon3 ;

    /* create an imagelist to play with */
    himl = ImageList_Create(84,84,0x10,0,3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* add three */
    ok(0==ImageList_AddIcon(himl, hicon1),"failed to add icon1\n");
    ok(1==ImageList_AddIcon(himl, hicon2),"failed to add icon2\n");
    ok(2==ImageList_AddIcon(himl, hicon3),"failed to add icon3\n");

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    ok(DestroyIcon(hicon1),"icon 1 wasn't deleted\n");
    ok(DestroyIcon(hicon2),"icon 2 wasn't deleted\n");
    ok(DestroyIcon(hicon3),"icon 3 wasn't deleted\n");

    return TRUE;
}

static BOOL DoTest3(void)
{
    HIMAGELIST himl;

    HBITMAP hbm1;
    HBITMAP hbm2;
    HBITMAP hbm3;

    IMAGELISTDRAWPARAMS imldp;
    HDC hdc;
    HWND hwndfortest;

    if (!pImageList_DrawIndirect)
    {
        HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");
        pImageList_DrawIndirect = (void*)GetProcAddress(hComCtl32, "ImageList_DrawIndirect");
        if (!pImageList_DrawIndirect)
        {
            trace("ImageList_DrawIndirect not available, skipping test\n");
            return TRUE;
        }
    }

    hwndfortest = create_a_window();
    hdc = GetDC(hwndfortest);
    ok(hdc!=NULL, "couldn't get DC\n");

    /* create an imagelist to play with */
    himl = ImageList_Create(48,48,0x10,0,3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hbm1 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm1 != 0, "no bitmap 1\n");
    hbm2 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm2 != 0, "no bitmap 2\n");
    hbm3 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm3 != 0, "no bitmap 3\n");

    /* add three */
    ok(0==ImageList_Add(himl, hbm1, 0),"failed to add bitmap 1\n");
    ok(1==ImageList_Add(himl, hbm2, 0),"failed to add bitmap 2\n");

    ok(ImageList_SetImageCount(himl,3),"Setimage count failed\n");
    /*ok(2==ImageList_Add(himl, hbm3, NULL),"failed to add bitmap 3\n"); */
    ok(ImageList_Replace(himl, 2, hbm3, 0),"failed to replace bitmap 3\n");

    memset(&imldp, 0, sizeof (imldp));
    ok(!pImageList_DrawIndirect(&imldp), "zero data succeeded!\n");
    imldp.cbSize = sizeof (imldp);
    ok(!pImageList_DrawIndirect(&imldp), "zero hdc succeeded!\n");
    imldp.hdcDst = hdc;
    ok(!pImageList_DrawIndirect(&imldp),"zero himl succeeded!\n");
    imldp.himl = himl;
    if (!pImageList_DrawIndirect(&imldp))
    {
      /* Earlier versions of native comctl32 use a smaller structure */
      imldp.cbSize -= 3 * sizeof(DWORD);
      ok(pImageList_DrawIndirect(&imldp),"DrawIndirect should succeed\n");
    }
    REDRAW(hwndfortest);
    WAIT;

    imldp.fStyle = SRCCOPY;
    imldp.rgbBk = CLR_DEFAULT;
    imldp.rgbFg = CLR_DEFAULT;
    imldp.y = 100;
    imldp.x = 100;
    ok(pImageList_DrawIndirect(&imldp),"should succeed\n");
    imldp.i ++;
    ok(pImageList_DrawIndirect(&imldp),"should succeed\n");
    imldp.i ++;
    ok(pImageList_DrawIndirect(&imldp),"should succeed\n");
    imldp.i ++;
    ok(!pImageList_DrawIndirect(&imldp),"should fail\n");

    /* remove three */
    ok(ImageList_Remove(himl, 0), "removing 1st bitmap\n");
    ok(ImageList_Remove(himl, 0), "removing 2nd bitmap\n");
    ok(ImageList_Remove(himl, 0), "removing 3rd bitmap\n");

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    /* bitmaps should not be deleted by the imagelist */
    ok(DeleteObject(hbm1),"bitmap 1 can't be deleted\n");
    ok(DeleteObject(hbm2),"bitmap 2 can't be deleted\n");
    ok(DeleteObject(hbm3),"bitmap 3 can't be deleted\n");

    ReleaseDC(hwndfortest, hdc);
    DestroyWindow(hwndfortest);

    return TRUE;
}

static void testMerge(void)
{
    HIMAGELIST himl1, himl2, hmerge;
    HICON hicon1;
    HWND hwnd = create_a_window();

    himl1 = ImageList_Create(32,32,0,0,3);
    ok(himl1 != NULL,"failed to create himl1\n");

    himl2 = ImageList_Create(32,32,0,0,3);
    ok(himl2 != NULL,"failed to create himl2\n");

    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != NULL, "failed to create hicon1\n");

    if (!himl1 || !himl2 || !hicon1)
        return;

    ok(0==ImageList_AddIcon(himl2, hicon1),"add icon1 to himl2 failed\n");
    check_bits(hwnd, himl2, 0, 32, icon_bits, "add icon1 to himl2");

    /* If himl1 has no images, merge still succeeds */
    hmerge = ImageList_Merge(himl1, -1, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl1,-1 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1,-1");
    if (hmerge) ImageList_Destroy(hmerge);

    hmerge = ImageList_Merge(himl1, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL,"merge himl1,0 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1,0");
    if (hmerge) ImageList_Destroy(hmerge);

    /* Same happens if himl2 is empty */
    ImageList_Destroy(himl2);
    himl2 = ImageList_Create(32,32,0,0,3);
    ok(himl2 != NULL,"failed to recreate himl2\n");
    if (!himl2)
        return;

    hmerge = ImageList_Merge(himl1, -1, himl2, -1, 0, 0);
    ok(hmerge != NULL, "merge himl2,-1 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2,-1");
    if (hmerge) ImageList_Destroy(hmerge);

    hmerge = ImageList_Merge(himl1, -1, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl2,0 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2,0");
    if (hmerge) ImageList_Destroy(hmerge);

    /* Now try merging an image with itself */
    ok(0==ImageList_AddIcon(himl2, hicon1),"re-add icon1 to himl2 failed\n");

    hmerge = ImageList_Merge(himl2, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl2 with itself failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2 with itself");
    if (hmerge) ImageList_Destroy(hmerge);

    /* Try merging 2 different image lists */
    ok(0==ImageList_AddIcon(himl1, hicon1),"add icon1 to himl1 failed\n");

    hmerge = ImageList_Merge(himl1, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl1 with himl2 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1 with himl2");
    if (hmerge) ImageList_Destroy(hmerge);

    hmerge = ImageList_Merge(himl1, 0, himl2, 0, 8, 16);
    ok(hmerge != NULL, "merge himl1 with himl2 8,16 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1 with himl2, 8,16");
    if (hmerge) ImageList_Destroy(hmerge);

    ImageList_Destroy(himl1);
    ImageList_Destroy(himl2);
    DestroyIcon(hicon1);
    DestroyWindow(hwnd);
}

/*********************** imagelist storage test ***************************/

#define BMP_CX 48

struct my_IStream
{
    IStream is;
    char *iml_data; /* written imagelist data */
    ULONG iml_data_size;
};

static HRESULT STDMETHODCALLTYPE Test_Stream_QueryInterface(
    IStream* This,
    REFIID riid,
    void** ppvObject)
{
    assert(0);
    return E_NOTIMPL;
}

static ULONG STDMETHODCALLTYPE Test_Stream_AddRef(
    IStream* This)
{
    assert(0);
    return 2;
}

static ULONG STDMETHODCALLTYPE Test_Stream_Release(
    IStream* This)
{
    assert(0);
    return 1;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Read(
    IStream* This,
    void* pv,
    ULONG cb,
    ULONG* pcbRead)
{
    assert(0);
    return E_NOTIMPL;
}

static BOOL allocate_storage(struct my_IStream *my_is, ULONG add)
{
    my_is->iml_data_size += add;

    if (!my_is->iml_data)
        my_is->iml_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, my_is->iml_data_size);
    else
        my_is->iml_data = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, my_is->iml_data, my_is->iml_data_size);

    return my_is->iml_data ? TRUE : FALSE;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Write(
    IStream* This,
    const void* pv,
    ULONG cb,
    ULONG* pcbWritten)
{
    struct my_IStream *my_is = (struct my_IStream *)This;
    ULONG current_iml_data_size = my_is->iml_data_size;

    if (!allocate_storage(my_is, cb)) return E_FAIL;

    memcpy(my_is->iml_data + current_iml_data_size, pv, cb);
    if (pcbWritten) *pcbWritten = cb;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Seek(
    IStream* This,
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER* plibNewPosition)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_SetSize(
    IStream* This,
    ULARGE_INTEGER libNewSize)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_CopyTo(
    IStream* This,
    IStream* pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER* pcbRead,
    ULARGE_INTEGER* pcbWritten)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Commit(
    IStream* This,
    DWORD grfCommitFlags)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Revert(
    IStream* This)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_LockRegion(
    IStream* This,
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_UnlockRegion(
    IStream* This,
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Stat(
    IStream* This,
    STATSTG* pstatstg,
    DWORD grfStatFlag)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Clone(
    IStream* This,
    IStream** ppstm)
{
    assert(0);
    return E_NOTIMPL;
}

static const IStreamVtbl Test_Stream_Vtbl =
{
    Test_Stream_QueryInterface,
    Test_Stream_AddRef,
    Test_Stream_Release,
    Test_Stream_Read,
    Test_Stream_Write,
    Test_Stream_Seek,
    Test_Stream_SetSize,
    Test_Stream_CopyTo,
    Test_Stream_Commit,
    Test_Stream_Revert,
    Test_Stream_LockRegion,
    Test_Stream_UnlockRegion,
    Test_Stream_Stat,
    Test_Stream_Clone
};

static struct my_IStream Test_Stream = { { &Test_Stream_Vtbl }, 0, 0 };

static INT DIB_GetWidthBytes( int width, int bpp )
{
    int words;

    switch (bpp)
    {
	case 1:  words = (width + 31) / 32; break;
	case 4:  words = (width + 7) / 8; break;
	case 8:  words = (width + 3) / 4; break;
	case 15:
	case 16: words = (width + 1) / 2; break;
	case 24: words = (width * 3 + 3)/4; break;
	case 32: words = width; break;

        default:
            words=0;
            trace("Unknown depth %d, please report.\n", bpp );
            assert(0);
            break;
    }
    return 4 * words;
}

static void check_bitmap_data(const char *bm_data, ULONG bm_data_size,
                              INT width, INT height, INT bpp,
                              const char *comment)
{
    const BITMAPFILEHEADER *bmfh = (const BITMAPFILEHEADER *)bm_data;
    const BITMAPINFOHEADER *bmih = (const BITMAPINFOHEADER *)(bm_data + sizeof(*bmfh));
    ULONG hdr_size, image_size;

    hdr_size = sizeof(*bmfh) + sizeof(*bmih);
    if (bmih->biBitCount <= 8) hdr_size += (1 << bpp) * sizeof(RGBQUAD);

    ok(bmfh->bfType == (('M' << 8) | 'B'), "wrong bfType 0x%02x\n", bmfh->bfType);
    ok(bmfh->bfSize == hdr_size, "wrong bfSize 0x%02x\n", bmfh->bfSize);
    ok(bmfh->bfReserved1 == 0, "wrong bfReserved1 0x%02x\n", bmfh->bfReserved1);
    ok(bmfh->bfReserved2 == 0, "wrong bfReserved2 0x%02x\n", bmfh->bfReserved2);
    ok(bmfh->bfOffBits == hdr_size, "wrong bfOffBits 0x%02x\n", bmfh->bfOffBits);

    ok(bmih->biSize == sizeof(*bmih), "wrong biSize %d\n", bmih->biSize);
    ok(bmih->biWidth == width, "wrong biWidth %d (expected %d)\n", bmih->biWidth, width);
    ok(bmih->biHeight == height, "wrong biHeight %d (expected %d)\n", bmih->biHeight, height);
    ok(bmih->biPlanes == 1, "wrong biPlanes %d\n", bmih->biPlanes);
    ok(bmih->biBitCount == bpp, "wrong biBitCount %d\n", bmih->biBitCount);

    image_size = DIB_GetWidthBytes(bmih->biWidth, bmih->biBitCount) * bmih->biHeight;
    ok(bmih->biSizeImage == image_size, "wrong biSizeImage %u\n", bmih->biSizeImage);
#if 0
{
    char fname[256];
    FILE *f;
    sprintf(fname, "bmp_%s.bmp", comment);
    f = fopen(fname, "wb");
    fwrite(bm_data, 1, bm_data_size, f);
    fclose(f);
}
#endif
}

static void check_ilhead_data(const char *ilh_data, INT cx, INT cy, INT cur, INT max)
{
    ILHEAD *ilh = (ILHEAD *)ilh_data;

    ok(ilh->usMagic == IMAGELIST_MAGIC, "wrong usMagic %4x (expected %02x)\n", ilh->usMagic, IMAGELIST_MAGIC);
    ok(ilh->usVersion == 0x101, "wrong usVersion %x (expected 0x101)\n", ilh->usVersion);
    ok(ilh->cCurImage == cur, "wrong cCurImage %d (expected %d)\n", ilh->cCurImage, cur);
    ok(ilh->cMaxImage == max, "wrong cMaxImage %d (expected %d)\n", ilh->cMaxImage, max);
    ok(ilh->cGrow == 4, "wrong cGrow %d (expected 4)\n", ilh->cGrow);
    ok(ilh->cx == cx, "wrong cx %d (expected %d)\n", ilh->cx, cx);
    ok(ilh->cy == cy, "wrong cy %d (expected %d)\n", ilh->cy, cy);
    ok(ilh->bkcolor == CLR_NONE, "wrong bkcolor %x\n", ilh->bkcolor);
    ok(ilh->flags == ILC_COLOR24, "wrong flags %04x\n", ilh->flags);
    ok(ilh->ovls[0] == -1, "wrong ovls[0] %04x\n", ilh->ovls[0]);
    ok(ilh->ovls[1] == -1, "wrong ovls[1] %04x\n", ilh->ovls[1]);
    ok(ilh->ovls[2] == -1, "wrong ovls[2] %04x\n", ilh->ovls[2]);
    ok(ilh->ovls[3] == -1, "wrong ovls[3] %04x\n", ilh->ovls[3]);
}

static HBITMAP create_bitmap(INT cx, INT cy, COLORREF color, const char *comment)
{
    HDC hdc;
    char bmibuf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *bmi = (BITMAPINFO *)bmibuf;
    HBITMAP hbmp, hbmp_old;
    HBRUSH hbrush;
    RECT rc = { 0, 0, cx, cy };

    hdc = CreateCompatibleDC(0);

    memset(bmi, 0, sizeof(*bmi));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = cx;
    bmi->bmiHeader.biWidth = cy;
    bmi->bmiHeader.biBitCount = 24;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;
    hbmp = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);

    hbmp_old = SelectObject(hdc, hbmp);

    hbrush = CreateSolidBrush(color);
    FillRect(hdc, &rc, hbrush);
    DeleteObject(hbrush);

    DrawText(hdc, comment, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hbmp_old);
    DeleteDC(hdc);

    return hbmp;
}

static void image_list_init(HIMAGELIST himl)
{
    HBITMAP hbm;
    char comment[16];
    INT n = 1;

#define add_bitmap(grey) \
    sprintf(comment, "%d", n++); \
    hbm = create_bitmap(BMP_CX, BMP_CX, RGB((grey),(grey),(grey)), comment); \
    ImageList_Add(himl, hbm, NULL);

    add_bitmap(255); add_bitmap(170); add_bitmap(85); add_bitmap(0);
    add_bitmap(0); add_bitmap(85); add_bitmap(170); add_bitmap(255);
    add_bitmap(255); add_bitmap(170); add_bitmap(85); add_bitmap(0);
    add_bitmap(0); add_bitmap(85); add_bitmap(170); add_bitmap(255);
    add_bitmap(255); add_bitmap(170); add_bitmap(85); add_bitmap(0);
    add_bitmap(0); add_bitmap(85); add_bitmap(170); add_bitmap(255);
#undef add_bitmap
}

#define iml_clear_stream_data() \
    HeapFree(GetProcessHeap(), 0, Test_Stream.iml_data); \
    Test_Stream.iml_data = NULL; \
    Test_Stream.iml_data_size = 0;

static void check_iml_data(HIMAGELIST himl, INT cx, INT cy, INT cur, INT max,
                           INT width, INT height, INT bpp, const char *comment)
{
    INT ret, cxx, cyy;

    ret = ImageList_GetImageCount(himl);
    ok(ret == cur, "expected cur %d got %d\n", cur, ret);

    ret = ImageList_GetIconSize(himl, &cxx, &cyy);
    ok(ret, "ImageList_GetIconSize failed\n");
    ok(cxx == cx, "wrong cx %d (expected %d)\n", cxx, cx);
    ok(cyy == cy, "wrong cy %d (expected %d)\n", cyy, cy);

    iml_clear_stream_data();
    ret = ImageList_Write(himl, &Test_Stream.is);
    ok(ret, "ImageList_Write failed\n");

    ok(Test_Stream.iml_data != 0, "ImageList_Write didn't write any data\n");
    ok(Test_Stream.iml_data_size > sizeof(ILHEAD), "ImageList_Write wrote not enough data\n");

    check_ilhead_data(Test_Stream.iml_data, cx, cy, cur, max);
    check_bitmap_data(Test_Stream.iml_data + sizeof(ILHEAD),
                      Test_Stream.iml_data_size - sizeof(ILHEAD),
                      width, height, bpp, comment);
}

static void test_imagelist_storage(void)
{
    HIMAGELIST himl;
    BOOL ret;

    himl = ImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 1, 1);
    ok(himl != 0, "ImageList_Create failed\n");

    check_iml_data(himl, BMP_CX, BMP_CX, 0, 2, BMP_CX * 4, BMP_CX * 1, 24, "empty");

    image_list_init(himl);
    check_iml_data(himl, BMP_CX, BMP_CX, 24, 27, BMP_CX * 4, BMP_CX * 7, 24, "orig");

    ret = ImageList_Remove(himl, 4);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 23, 27, BMP_CX * 4, BMP_CX * 7, 24, "1");

    ret = ImageList_Remove(himl, 5);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 22, 27, BMP_CX * 4, BMP_CX * 7, 24, "2");

    ret = ImageList_Remove(himl, 6);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 21, 27, BMP_CX * 4, BMP_CX * 7, 24, "3");

    ret = ImageList_Remove(himl, 7);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, BMP_CX * 4, BMP_CX * 7, 24, "4");

    ret = ImageList_Remove(himl, -2);
    ok(!ret, "ImageList_Remove(-2) should fail\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, BMP_CX * 4, BMP_CX * 7, 24, "5");

    ret = ImageList_Remove(himl, 20);
    ok(!ret, "ImageList_Remove(20) should fail\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, BMP_CX * 4, BMP_CX * 7, 24, "6");

    ret = ImageList_Remove(himl, -1);
    ok(ret, "ImageList_Remove(-1) failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 4, BMP_CX * 4, BMP_CX * 1, 24, "7");

    ret = ImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    iml_clear_stream_data();
}

START_TEST(imagelist)
{
    desktopDC=GetDC(NULL);
    hinst = GetModuleHandleA(NULL);

    InitCommonControls();

    testHotspot();
    DoTest1();
    DoTest2();
    DoTest3();
    testMerge();
    test_imagelist_storage();
}
