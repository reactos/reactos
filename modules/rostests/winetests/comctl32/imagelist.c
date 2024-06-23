/*
 * Unit test suite for imagelist control.
 *
 * Copyright 2004 Michael Stefaniuc
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2007 Dmitry Timoshkov
 * Copyright 2009 Owen Rudge for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "commctrl.h" /* must be included after objbase.h to get ImageList_Write */
#include "initguid.h"
#include "commoncontrols.h"
#include "shellapi.h"

#include "wine/test.h"
#include "v6util.h"
#include "resources.h"

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

static HIMAGELIST (WINAPI *pImageList_Create)(int, int, UINT, int, int);
static BOOL (WINAPI *pImageList_Destroy)(HIMAGELIST);
static int (WINAPI *pImageList_Add)(HIMAGELIST, HBITMAP, HBITMAP);
static BOOL (WINAPI *pImageList_DrawIndirect)(IMAGELISTDRAWPARAMS*);
static BOOL (WINAPI *pImageList_SetImageCount)(HIMAGELIST,UINT);
static HRESULT (WINAPI *pImageList_CoCreateInstance)(REFCLSID,const IUnknown *,
    REFIID,void **);
static HRESULT (WINAPI *pHIMAGELIST_QueryInterface)(HIMAGELIST,REFIID,void **);
static int (WINAPI *pImageList_SetColorTable)(HIMAGELIST,int,int,RGBQUAD*);
static DWORD (WINAPI *pImageList_GetFlags)(HIMAGELIST);
static BOOL (WINAPI *pImageList_BeginDrag)(HIMAGELIST, int, int, int);
static HIMAGELIST (WINAPI *pImageList_GetDragImage)(POINT *, POINT *);
static void (WINAPI *pImageList_EndDrag)(void);
static INT (WINAPI *pImageList_GetImageCount)(HIMAGELIST);
static BOOL (WINAPI *pImageList_SetDragCursorImage)(HIMAGELIST, int, int, int);
static BOOL (WINAPI *pImageList_GetIconSize)(HIMAGELIST, int *, int *);
static BOOL (WINAPI *pImageList_Remove)(HIMAGELIST, int);
static INT (WINAPI *pImageList_ReplaceIcon)(HIMAGELIST, int, HICON);
static BOOL (WINAPI *pImageList_Replace)(HIMAGELIST, int, HBITMAP, HBITMAP);
static HIMAGELIST (WINAPI *pImageList_Merge)(HIMAGELIST, int, HIMAGELIST, int, int, int);
static BOOL (WINAPI *pImageList_GetImageInfo)(HIMAGELIST, int, IMAGEINFO *);
static BOOL (WINAPI *pImageList_Write)(HIMAGELIST, IStream *);
static HIMAGELIST (WINAPI *pImageList_Read)(IStream *);
static BOOL (WINAPI *pImageList_Copy)(HIMAGELIST, int, HIMAGELIST, int, UINT);
static HIMAGELIST (WINAPI *pImageList_LoadImageW)(HINSTANCE, LPCWSTR, int, int, COLORREF, UINT, UINT);
static BOOL (WINAPI *pImageList_Draw)(HIMAGELIST,INT,HDC,INT,INT,UINT);

static HINSTANCE hinst;

/* only used in interactive mode */
static void force_redraw(HWND hwnd)
{
    if (!winetest_interactive)
        return;

    RedrawWindow(hwnd, NULL, 0, RDW_UPDATENOW);
    Sleep(1000);
}

static BOOL is_v6_test(void)
{
    return pHIMAGELIST_QueryInterface != NULL;
}

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
    HIMAGELIST himl = pImageList_Create(cx, cy, ILC_COLOR, 1, 1);
    HBITMAP hbm = CreateBitmap(48, 48, 1, 1, bitmap_bits);

    ok(himl != NULL, "Failed to create image list, %d x %d.\n", cx, cy);
    pImageList_Add(himl, hbm, NULL);
    DeleteObject(hbm);
    return himl;
}

static HWND create_window(void)
{
    char className[] = "bmwnd";
    char winName[]   = "Test Bitmap";
    HWND hWnd;
    static BOOL registered = FALSE;

    if (!registered)
    {
        WNDCLASSA cls;

        cls.style         = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
        cls.lpfnWndProc   = DefWindowProcA;
        cls.cbClsExtra    = 0;
        cls.cbWndExtra    = 0;
        cls.hInstance     = 0;
        cls.hIcon         = LoadIconA(0, (LPCSTR)IDI_APPLICATION);
        cls.hCursor       = LoadCursorA(0, (LPCSTR)IDC_ARROW);
        cls.hbrBackground = GetStockObject (WHITE_BRUSH);
        cls.lpszMenuName  = 0;
        cls.lpszClassName = className;

        RegisterClassA (&cls);
        registered = TRUE;
    }

    /* Setup window */
    hWnd = CreateWindowA (className, winName,
       WS_OVERLAPPEDWINDOW ,
       CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
       0, hinst, 0);

    if (winetest_interactive)
    {
        ShowWindow (hWnd, SW_SHOW);
        force_redraw (hWnd);
    }

    return hWnd;
}

static HDC show_image(HWND hwnd, HIMAGELIST himl, int idx, int size,
                      LPCSTR loc, BOOL clear)
{
    HDC hdc;

    if (!winetest_interactive || !himl) return NULL;

    SetWindowTextA(hwnd, loc);
    hdc = GetDC(hwnd);
    pImageList_Draw(himl, idx, hdc, 0, 0, ILD_TRANSPARENT);

    force_redraw(hwnd);

    if (clear)
    {
        BitBlt(hdc, 0, 0, size, size, hdc, size+1, size+1, SRCCOPY);
        ReleaseDC(hwnd, hdc);
        hdc = NULL;
    }

    return hdc;
}

/* Useful for checking differences */
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

static void check_bits(HWND hwnd, HIMAGELIST himl, int idx, int size,
                       const BYTE *checkbits, LPCSTR loc)
{
    BYTE bits[100*100/8];
    COLORREF c;
    HDC hdc;
    int x, y, i = -1;

    if (!winetest_interactive || !himl) return;

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
}

static void test_begindrag(void)
{
    HIMAGELIST himl = createImageList(7,13);
    HIMAGELIST drag;
    BOOL ret;
    int count;
    POINT hotspot;

    count = pImageList_GetImageCount(himl);
    ok(count > 2, "Tests need an ImageList with more than 2 images\n");

    /* Two BeginDrag() without EndDrag() in between */
    ret = pImageList_BeginDrag(himl, 1, 0, 0);
    drag = pImageList_GetDragImage(NULL, NULL);
    ok(ret && drag, "ImageList_BeginDrag() failed\n");
    ret = pImageList_BeginDrag(himl, 0, 3, 5);
    ok(!ret, "ImageList_BeginDrag() returned TRUE\n");
    drag = pImageList_GetDragImage(NULL, &hotspot);
    ok(!!drag, "No active ImageList drag left\n");
    ok(hotspot.x == 0 && hotspot.y == 0, "New ImageList drag was created\n");
    pImageList_EndDrag();
    drag = pImageList_GetDragImage(NULL, NULL);
    ok(!drag, "ImageList drag was not destroyed\n");

    /* Invalid image index */
    pImageList_BeginDrag(himl, 0, 0, 0);
    ret = pImageList_BeginDrag(himl, count, 3, 5);
    ok(!ret, "ImageList_BeginDrag() returned TRUE\n");
    drag = pImageList_GetDragImage(NULL, &hotspot);
    ok(drag && hotspot.x == 0 && hotspot.y == 0, "Active drag should not have been canceled\n");
    pImageList_EndDrag();
    drag = pImageList_GetDragImage(NULL, NULL);
    ok(!drag, "ImageList drag was not destroyed\n");
    /* Invalid negative image indexes succeed */
    ret = pImageList_BeginDrag(himl, -17, 0, 0);
    drag = pImageList_GetDragImage(NULL, NULL);
    ok(ret && drag, "ImageList drag was created\n");
    pImageList_EndDrag();
    ret = pImageList_BeginDrag(himl, -1, 0, 0);
    drag = pImageList_GetDragImage(NULL, NULL);
    ok(ret && drag, "ImageList drag was created\n");
    pImageList_EndDrag();
    pImageList_Destroy(himl);
}

static void test_hotspot(void)
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
    HWND hwnd = create_window();


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

            ret = pImageList_BeginDrag(himl1, 0, dx1, dy1);
            ok(ret != 0, "BeginDrag failed for { %d, %d }\n", dx1, dy1);
            sprintf(loc, "BeginDrag (%d,%d)\n", i, j);
            show_image(hwnd, himl1, 0, max(SIZEX1, SIZEY1), loc, TRUE);

            /* check merging the dragged image with a second image */
            ret = pImageList_SetDragCursorImage(himl2, 0, dx2, dy2);
            ok(ret != 0, "SetDragCursorImage failed for {%d, %d}{%d, %d}\n",
                    dx1, dy1, dx2, dy2);
            sprintf(loc, "SetDragCursorImage (%d,%d)\n", i, j);
            show_image(hwnd, himl2, 0, max(SIZEX2, SIZEY2), loc, TRUE);

            /* check new hotspot, it should be the same like the old one */
            himlNew = pImageList_GetDragImage(NULL, &ppt);
            ok(ppt.x == dx1 && ppt.y == dy1,
                    "Expected drag hotspot [%d,%d] got [%d,%d]\n",
                    dx1, dy1, ppt.x, ppt.y);
            /* check size of new dragged image */
            pImageList_GetIconSize(himlNew, &newx, &newy);
            correctx = max(SIZEX1, max(SIZEX2 + dx2, SIZEX1 - dx2));
            correcty = max(SIZEY1, max(SIZEY2 + dy2, SIZEY1 - dy2));
            ok(newx == correctx && newy == correcty,
                    "Expected drag image size [%d,%d] got [%d,%d]\n",
                    correctx, correcty, newx, newy);
            sprintf(loc, "GetDragImage (%d,%d)\n", i, j);
            show_image(hwnd, himlNew, 0, max(correctx, correcty), loc, TRUE);
            pImageList_EndDrag();
        }
    }
#undef SIZEX1
#undef SIZEY1
#undef SIZEX2
#undef SIZEY2
#undef HOTSPOTS_MAX
    pImageList_Destroy(himl2);
    pImageList_Destroy(himl1);
    DestroyWindow(hwnd);
}

static void test_add_remove(void)
{
    HIMAGELIST himl ;

    HICON hicon1 ;
    HICON hicon2 ;
    HICON hicon3 ;

    /* create an imagelist to play with */
    himl = pImageList_Create(84, 84, ILC_COLOR16, 0, 3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* remove when nothing exists */
    ok(!pImageList_Remove(himl, 0), "Removed nonexistent icon.\n");
    /* removing everything from an empty imagelist should succeed */
    ok(pImageList_Remove(himl, -1), "Removed nonexistent icon\n");

    /* add three */
    ok(0 == pImageList_ReplaceIcon(himl, -1, hicon1), "Failed to add icon1.\n");
    ok(1 == pImageList_ReplaceIcon(himl, -1, hicon2), "Failed to add icon2.\n");
    ok(2 == pImageList_ReplaceIcon(himl, -1, hicon3), "Failed to add icon3.\n");

    /* remove an index out of range */
    ok(!pImageList_Remove(himl, 4711), "removed nonexistent icon\n");

    /* remove three */
    ok(pImageList_Remove(himl, 0), "Can't remove 0\n");
    ok(pImageList_Remove(himl, 0), "Can't remove 0\n");
    ok(pImageList_Remove(himl, 0), "Can't remove 0\n");

    /* remove one extra */
    ok(!pImageList_Remove(himl, 0), "Removed nonexistent icon.\n");

    /* destroy it */
    ok(pImageList_Destroy(himl), "Failed to destroy imagelist.\n");

    ok(-1 == pImageList_ReplaceIcon((HIMAGELIST)0xdeadbeef, -1, hicon1), "Don't crash on bad handle\n");

    ok(DestroyIcon(hicon1), "Failed to destroy icon 1.\n");
    ok(DestroyIcon(hicon2), "Failed to destroy icon 2.\n");
    ok(DestroyIcon(hicon3), "Failed to destroy icon 3.\n");
}

static void test_imagecount(void)
{
    HIMAGELIST himl;

    ok(0 == pImageList_GetImageCount((HIMAGELIST)0xdeadbeef), "don't crash on bad handle\n");

    if (!pImageList_SetImageCount)
    {
        win_skip("ImageList_SetImageCount not available\n");
        return;
    }

    himl = pImageList_Create(84, 84, ILC_COLOR16, 0, 3);
    ok(himl != 0, "Failed to create imagelist.\n");

    ok(pImageList_SetImageCount(himl, 3), "couldn't increase image count\n");
    ok(pImageList_GetImageCount(himl) == 3, "invalid image count after increase\n");
    ok(pImageList_SetImageCount(himl, 1), "couldn't decrease image count\n");
    ok(pImageList_GetImageCount(himl) == 1, "invalid image count after decrease to 1\n");
    ok(pImageList_SetImageCount(himl, 0), "couldn't decrease image count\n");
    ok(pImageList_GetImageCount(himl) == 0, "invalid image count after decrease to 0\n");

    ok(pImageList_Destroy(himl), "Failed to destroy imagelist.\n");
}

static void test_DrawIndirect(void)
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
        win_skip("ImageList_DrawIndirect not available, skipping test\n");
        return;
    }

    hwndfortest = create_window();
    hdc = GetDC(hwndfortest);
    ok(hdc!=NULL, "couldn't get DC\n");

    /* create an imagelist to play with */
    himl = pImageList_Create(48, 48, ILC_COLOR16, 0, 3);
    ok(himl != 0, "Failed to create imagelist.\n");

    /* load the icons to add to the image list */
    hbm1 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm1 != 0, "no bitmap 1\n");
    hbm2 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm2 != 0, "no bitmap 2\n");
    hbm3 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm3 != 0, "no bitmap 3\n");

    /* add three */
    ok(0 == pImageList_Add(himl, hbm1, 0),"failed to add bitmap 1\n");
    ok(1 == pImageList_Add(himl, hbm2, 0),"failed to add bitmap 2\n");

    if (pImageList_SetImageCount)
    {
        ok(pImageList_SetImageCount(himl,3),"Setimage count failed\n");
        /*ok(2==ImageList_Add(himl, hbm3, NULL),"failed to add bitmap 3\n"); */
        ok(pImageList_Replace(himl, 2, hbm3, 0),"failed to replace bitmap 3\n");
    }

    memset(&imldp, 0, sizeof (imldp));
    ok(!pImageList_DrawIndirect(&imldp), "zero data succeeded!\n");
    imldp.cbSize = IMAGELISTDRAWPARAMS_V3_SIZE;
    ok(!pImageList_DrawIndirect(&imldp), "zero hdc succeeded!\n");
    imldp.hdcDst = hdc;
    ok(!pImageList_DrawIndirect(&imldp),"zero himl succeeded!\n");
    imldp.himl = (HIMAGELIST)0xdeadbeef;
    ok(!pImageList_DrawIndirect(&imldp),"bad himl succeeded!\n");
    imldp.himl = himl;

    force_redraw(hwndfortest);

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
    ok(pImageList_Remove(himl, 0), "removing 1st bitmap\n");
    ok(pImageList_Remove(himl, 0), "removing 2nd bitmap\n");
    ok(pImageList_Remove(himl, 0), "removing 3rd bitmap\n");

    /* destroy it */
    ok(pImageList_Destroy(himl), "Failed to destroy imagelist.\n");

    /* bitmaps should not be deleted by the imagelist */
    ok(DeleteObject(hbm1),"bitmap 1 can't be deleted\n");
    ok(DeleteObject(hbm2),"bitmap 2 can't be deleted\n");
    ok(DeleteObject(hbm3),"bitmap 3 can't be deleted\n");

    ReleaseDC(hwndfortest, hdc);
    DestroyWindow(hwndfortest);
}

static int get_color_format(HBITMAP bmp)
{
    BITMAPINFO bmi;
    HDC hdc = CreateCompatibleDC(0);
    HBITMAP hOldBmp = SelectObject(hdc, bmp);
    int ret;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ret = GetDIBits(hdc, bmp, 0, 0, 0, &bmi, DIB_RGB_COLORS);
    ok(ret, "GetDIBits failed\n");

    SelectObject(hdc, hOldBmp);
    DeleteDC(hdc);
    return bmi.bmiHeader.biBitCount;
}

static void test_merge_colors(void)
{
    HIMAGELIST himl[8], hmerge;
    int sizes[] = { ILC_COLOR, ILC_COLOR | ILC_MASK, ILC_COLOR4, ILC_COLOR8, ILC_COLOR16, ILC_COLOR24, ILC_COLOR32, ILC_COLORDDB };
    HICON hicon1;
    IMAGEINFO info;
    int bpp, i, j;

    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != NULL, "failed to create hicon1\n");

    for (i = 0; i < 8; i++)
    {
        himl[i] = pImageList_Create(32, 32, sizes[i], 0, 3);
        ok(himl[i] != NULL, "failed to create himl[%d]\n", i);
        ok(0 == pImageList_ReplaceIcon(himl[i], -1, hicon1), "Failed to add icon1 to himl[%d].\n", i);
        if (i == 0 || i == 1 || i == 7)
        {
            pImageList_GetImageInfo(himl[i], 0, &info);
            sizes[i] = get_color_format(info.hbmImage);
        }
    }
    DestroyIcon(hicon1);
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
        {
            hmerge = pImageList_Merge(himl[i], 0, himl[j], 0, 0, 0);
            ok(hmerge != NULL, "merge himl[%d], himl[%d] failed\n", i, j);

            pImageList_GetImageInfo(hmerge, 0, &info);
            bpp = get_color_format(info.hbmImage);
            /* ILC_COLOR[X] is defined as [X] */
            if (i == 4 && j == 7)
                ok(bpp == 16, /* merging ILC_COLOR16 with ILC_COLORDDB seems to be a special case */
                    "wrong biBitCount %d when merging lists %d (%d) and %d (%d)\n", bpp, i, sizes[i], j, sizes[j]);
            else
                ok(bpp == (i > j ? sizes[i] : sizes[j]),
                    "wrong biBitCount %d when merging lists %d (%d) and %d (%d)\n", bpp, i, sizes[i], j, sizes[j]);
            ok(info.hbmMask != 0, "Imagelist merged from %d and %d had no mask\n", i, j);

            pImageList_Destroy(hmerge);
        }

    for (i = 0; i < 8; i++)
        pImageList_Destroy(himl[i]);
}

static void test_merge(void)
{
    HIMAGELIST himl1, himl2, hmerge;
    HICON hicon1;
    HWND hwnd = create_window();

    himl1 = pImageList_Create(32, 32, 0, 0, 3);
    ok(himl1 != NULL,"failed to create himl1\n");

    himl2 = pImageList_Create(32, 32, 0, 0, 3);
    ok(himl2 != NULL,"failed to create himl2\n");

    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != NULL, "failed to create hicon1\n");

    if (!himl1 || !himl2 || !hicon1)
        return;

    ok(0 == pImageList_ReplaceIcon(himl2, -1, hicon1), "Failed to add icon1 to himl2.\n");
    check_bits(hwnd, himl2, 0, 32, icon_bits, "add icon1 to himl2");

    /* If himl1 has no images, merge still succeeds */
    hmerge = pImageList_Merge(himl1, -1, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl1,-1 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1,-1");
    pImageList_Destroy(hmerge);

    hmerge = pImageList_Merge(himl1, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL,"merge himl1,0 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1,0");
    pImageList_Destroy(hmerge);

    /* Same happens if himl2 is empty */
    pImageList_Destroy(himl2);
    himl2 = pImageList_Create(32, 32, 0, 0, 3);
    ok(himl2 != NULL,"failed to recreate himl2\n");
    if (!himl2)
        return;

    hmerge = pImageList_Merge(himl1, -1, himl2, -1, 0, 0);
    ok(hmerge != NULL, "merge himl2,-1 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2,-1");
    pImageList_Destroy(hmerge);

    hmerge = pImageList_Merge(himl1, -1, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl2,0 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2,0");
    pImageList_Destroy(hmerge);

    /* Now try merging an image with itself */
    ok(0 == pImageList_ReplaceIcon(himl2, -1, hicon1), "Failed to re-add icon1 to himl2.\n");

    hmerge = pImageList_Merge(himl2, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl2 with itself failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2 with itself");
    pImageList_Destroy(hmerge);

    /* Try merging 2 different image lists */
    ok(0 == pImageList_ReplaceIcon(himl1, -1, hicon1), "Failed to add icon1 to himl1.\n");

    hmerge = pImageList_Merge(himl1, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl1 with himl2 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1 with himl2");
    pImageList_Destroy(hmerge);

    hmerge = pImageList_Merge(himl1, 0, himl2, 0, 8, 16);
    ok(hmerge != NULL, "merge himl1 with himl2 8,16 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1 with himl2, 8,16");
    pImageList_Destroy(hmerge);

    pImageList_Destroy(himl1);
    pImageList_Destroy(himl2);
    DestroyIcon(hicon1);
    DestroyWindow(hwnd);
}

/*********************** imagelist storage test ***************************/

#define BMP_CX 48

struct memstream
{
    IStream IStream_iface;
    IStream *stream;
};

static struct memstream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct memstream, IStream_iface);
}

static HRESULT STDMETHODCALLTYPE Test_Stream_QueryInterface(IStream *iface, REFIID riid,
                                                            void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG STDMETHODCALLTYPE Test_Stream_AddRef(IStream *iface)
{
    ok(0, "unexpected call\n");
    return 2;
}

static ULONG STDMETHODCALLTYPE Test_Stream_Release(IStream *iface)
{
    ok(0, "unexpected call\n");
    return 1;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Read(IStream *iface, void *pv, ULONG cb,
                                                  ULONG *pcbRead)
{
    struct memstream *stream = impl_from_IStream(iface);
    return IStream_Read(stream->stream, pv, cb, pcbRead);
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Write(IStream *iface, const void *pv, ULONG cb,
                                                   ULONG *pcbWritten)
{
    struct memstream *stream = impl_from_IStream(iface);
    return IStream_Write(stream->stream, pv, cb, pcbWritten);
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Seek(IStream *iface, LARGE_INTEGER offset, DWORD origin,
        ULARGE_INTEGER *new_pos)
{
    struct memstream *stream = impl_from_IStream(iface);

    if (is_v6_test())
    {
        ok(origin == STREAM_SEEK_CUR, "Unexpected origin %d.\n", origin);
        ok(offset.QuadPart == 0, "Unexpected offset %s.\n", wine_dbgstr_longlong(offset.QuadPart));
        ok(new_pos != NULL, "Unexpected out position pointer.\n");
        return IStream_Seek(stream->stream, offset, origin, new_pos);
    }
    else
    {
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }
}

static HRESULT STDMETHODCALLTYPE Test_Stream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_CopyTo(IStream *iface, IStream *pstm,
                                                    ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead,
                                                    ULARGE_INTEGER *pcbWritten)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Revert(IStream *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                                        ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_UnlockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                                          ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Stat(IStream *iface, STATSTG *pstatstg,
                                                  DWORD grfStatFlag)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Clone(IStream *iface, IStream **ppstm)
{
    ok(0, "unexpected call\n");
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

static void init_memstream(struct memstream *stream)
{
    HRESULT hr;

    stream->IStream_iface.lpVtbl = &Test_Stream_Vtbl;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream->stream);
    ok(hr == S_OK, "Failed to create a stream, hr %#x.\n", hr);
}

static void cleanup_memstream(struct memstream *stream)
{
    IStream_Release(stream->stream);
}

static INT DIB_GetWidthBytes( int width, int bpp )
{
    return ((width * bpp + 31) / 8) & ~3;
}

static ULONG check_bitmap_data(const ILHEAD *header, const char *bm_data,
    ULONG bm_data_size, const SIZE *bmpsize, INT bpp, const char *comment)
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
    ok(bmih->biPlanes == 1, "wrong biPlanes %d\n", bmih->biPlanes);
    ok(bmih->biBitCount == bpp, "wrong biBitCount %d\n", bmih->biBitCount);

    image_size = DIB_GetWidthBytes(bmih->biWidth, bmih->biBitCount) * bmih->biHeight;
    ok(bmih->biSizeImage == image_size, "wrong biSizeImage %u\n", bmih->biSizeImage);
    ok(bmih->biWidth == bmpsize->cx && bmih->biHeight == bmpsize->cy, "Unexpected bitmap size %d x %d, "
            "expected %d x %d\n", bmih->biWidth, bmih->biHeight, bmpsize->cx, bmpsize->cy);

if (0)
{
    char fname[256];
    FILE *f;
    sprintf(fname, "bmp_%s.bmp", comment);
    f = fopen(fname, "wb");
    fwrite(bm_data, 1, bm_data_size, f);
    fclose(f);
}

    return hdr_size + image_size;
}

static BOOL is_v6_header(const ILHEAD *header)
{
    return (header->usVersion & 0xff00) == 0x600;
}

static void check_ilhead_data(const ILHEAD *ilh, INT cx, INT cy, INT cur, INT max, INT grow, INT flags)
{
    INT grow_aligned;

    ok(ilh->usMagic == IMAGELIST_MAGIC, "wrong usMagic %4x (expected %02x)\n", ilh->usMagic, IMAGELIST_MAGIC);
    ok(ilh->usVersion == 0x101 ||
            ilh->usVersion == 0x600 || /* WinXP/W2k3 */
            ilh->usVersion == 0x620, "Unknown usVersion %#x.\n", ilh->usVersion);
    ok(ilh->cCurImage == cur, "wrong cCurImage %d (expected %d)\n", ilh->cCurImage, cur);

    grow = max(grow, 1);
    grow_aligned = (WORD)(grow + 3) & ~3;

    if (is_v6_header(ilh))
    {
        grow = (WORD)(grow + 2 + 3) & ~3;
        ok(ilh->cGrow == grow || broken(ilh->cGrow == grow_aligned) /* XP/Vista */,
            "Unexpected cGrow %d, expected %d\n", ilh->cGrow, grow);
    }
    else
    {
        ok(ilh->cMaxImage == max, "wrong cMaxImage %d (expected %d)\n", ilh->cMaxImage, max);
        ok(ilh->cGrow == grow_aligned, "Unexpected cGrow %d, expected %d\n", ilh->cGrow, grow_aligned);
    }

    ok(ilh->cx == cx, "wrong cx %d (expected %d)\n", ilh->cx, cx);
    ok(ilh->cy == cy, "wrong cy %d (expected %d)\n", ilh->cy, cy);
    ok(ilh->bkcolor == CLR_NONE, "wrong bkcolor %x\n", ilh->bkcolor);
    ok(ilh->flags == flags || broken(!(ilh->flags & 0xfe) && (flags & 0xfe) == ILC_COLOR4),  /* <= w2k */
       "wrong flags %04x\n", ilh->flags);
    ok(ilh->ovls[0] == -1, "wrong ovls[0] %04x\n", ilh->ovls[0]);
    ok(ilh->ovls[1] == -1, "wrong ovls[1] %04x\n", ilh->ovls[1]);
    ok(ilh->ovls[2] == -1, "wrong ovls[2] %04x\n", ilh->ovls[2]);
    ok(ilh->ovls[3] == -1, "wrong ovls[3] %04x\n", ilh->ovls[3]);
}

static HBITMAP create_bitmap(INT cx, INT cy, COLORREF color, const char *comment)
{
    HDC hdc;
    BITMAPINFO bmi;
    HBITMAP hbmp, hbmp_old;
    HBRUSH hbrush;
    RECT rc = { 0, 0, cx, cy };

    hdc = CreateCompatibleDC(0);

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biHeight = cx;
    bmi.bmiHeader.biWidth = cy;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biCompression = BI_RGB;
    hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);

    hbmp_old = SelectObject(hdc, hbmp);

    hbrush = CreateSolidBrush(color);
    FillRect(hdc, &rc, hbrush);
    DeleteObject(hbrush);

    DrawTextA(hdc, comment, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hbmp_old);
    DeleteDC(hdc);

    return hbmp;
}

static inline void imagelist_get_bitmap_size(const ILHEAD *header, SIZE *sz)
{
    const int tile_count = 4;

    if (is_v6_header(header))
    {
        sz->cx = header->cx;
        sz->cy = header->cMaxImage * header->cy;
    }
    else
    {
        sz->cx = header->cx * tile_count;
        sz->cy = ((header->cMaxImage + tile_count - 1) / tile_count) * header->cy;
    }
}

/* Grow argument matches what was used when imagelist was created. */
static void check_iml_data(HIMAGELIST himl, INT cx, INT cy, INT cur, INT max, INT grow,
        INT flags, const char *comment)
{
    INT ret, cxx, cyy, size;
    struct memstream stream;
    const ILHEAD *header;
    LARGE_INTEGER mv;
    HIMAGELIST himl2;
    HGLOBAL hglobal;
    STATSTG stat;
    char *data;
    HRESULT hr;
    SIZE bmpsize;
    BOOL b;

    ret = pImageList_GetImageCount(himl);
    ok(ret == cur, "%s: expected image count %d got %d\n", comment, cur, ret);

    ret = pImageList_GetIconSize(himl, &cxx, &cyy);
    ok(ret, "ImageList_GetIconSize failed\n");
    ok(cxx == cx, "%s: wrong cx %d (expected %d)\n", comment, cxx, cx);
    ok(cyy == cy, "%s: wrong cy %d (expected %d)\n", comment, cyy, cy);

    init_memstream(&stream);
    b = pImageList_Write(himl, &stream.IStream_iface);
    ok(b, "%s: ImageList_Write failed\n", comment);

    hr = GetHGlobalFromStream(stream.stream, &hglobal);
    ok(hr == S_OK, "%s: Failed to get hglobal, %#x\n", comment, hr);

    hr = IStream_Stat(stream.stream, &stat, STATFLAG_NONAME);
    ok(hr == S_OK, "Stat() failed, hr %#x.\n", hr);

    data = GlobalLock(hglobal);

    ok(data != 0, "%s: ImageList_Write didn't write any data\n", comment);
    ok(stat.cbSize.LowPart > sizeof(ILHEAD), "%s: ImageList_Write wrote not enough data\n", comment);

    header = (const ILHEAD *)data;
    check_ilhead_data(header, cx, cy, cur, max, grow, flags);
    imagelist_get_bitmap_size(header, &bmpsize);
    size = check_bitmap_data(header, data + sizeof(ILHEAD), stat.cbSize.LowPart - sizeof(ILHEAD),
            &bmpsize, flags & 0xfe, comment);
    if (!is_v6_header(header) && size < stat.cbSize.LowPart - sizeof(ILHEAD))  /* mask is present */
    {
        ok( flags & ILC_MASK, "%s: extra data %u/%u but mask not expected\n", comment, stat.cbSize.LowPart, size );
        check_bitmap_data(header, data + sizeof(ILHEAD) + size, stat.cbSize.LowPart - sizeof(ILHEAD) - size,
            &bmpsize, 1, comment);
    }

    /* rewind and reconstruct from stream */
    mv.QuadPart = 0;
    IStream_Seek(stream.stream, mv, STREAM_SEEK_SET, NULL);
    himl2 = pImageList_Read(&stream.IStream_iface);
    ok(himl2 != NULL, "%s: Failed to deserialize imagelist\n", comment);
    pImageList_Destroy(himl2);

    GlobalUnlock(hglobal);
    cleanup_memstream(&stream);
}

static void image_list_add_bitmap(HIMAGELIST himl, BYTE grey, int i)
{
    char comment[16];
    HBITMAP hbm;
    int ret;

    sprintf(comment, "%d", i);
    hbm = create_bitmap(BMP_CX, BMP_CX, RGB(grey, grey, grey), comment);
    ret = pImageList_Add(himl, hbm, NULL);
    ok(ret != -1, "Failed to add image to imagelist.\n");
    DeleteObject(hbm);
}

static void image_list_init(HIMAGELIST himl, INT grow)
{
    unsigned int i;
    static const struct test_data
    {
        BYTE grey;
        INT cx, cy, cur, max, bpp;
        const char *comment;
    } td[] =
    {
        { 255, BMP_CX, BMP_CX, 1, 2, 24, "total 1" },
        { 170, BMP_CX, BMP_CX, 2, 7, 24, "total 2" },
        { 85, BMP_CX, BMP_CX, 3, 7, 24, "total 3" },
        { 0, BMP_CX, BMP_CX, 4, 7, 24, "total 4" },
        { 0, BMP_CX, BMP_CX, 5, 7, 24, "total 5" },
        { 85, BMP_CX, BMP_CX, 6, 7, 24, "total 6" },
        { 170, BMP_CX, BMP_CX, 7, 12, 24, "total 7" },
        { 255, BMP_CX, BMP_CX, 8, 12, 24, "total 8" },
        { 255, BMP_CX, BMP_CX, 9, 12, 24, "total 9" },
        { 170, BMP_CX, BMP_CX, 10, 12, 24, "total 10" },
        { 85, BMP_CX, BMP_CX, 11, 12, 24, "total 11" },
        { 0, BMP_CX, BMP_CX, 12, 17, 24, "total 12" },
        { 0, BMP_CX, BMP_CX, 13, 17, 24, "total 13" },
        { 85, BMP_CX, BMP_CX, 14, 17, 24, "total 14" },
        { 170, BMP_CX, BMP_CX, 15, 17, 24, "total 15" },
        { 255, BMP_CX, BMP_CX, 16, 17, 24, "total 16" },
        { 255, BMP_CX, BMP_CX, 17, 22, 24, "total 17" },
        { 170, BMP_CX, BMP_CX, 18, 22, 24, "total 18" },
        { 85, BMP_CX, BMP_CX, 19, 22, 24, "total 19" },
        { 0, BMP_CX, BMP_CX, 20, 22, 24, "total 20" },
        { 0, BMP_CX, BMP_CX, 21, 22, 24, "total 21" },
        { 85, BMP_CX, BMP_CX, 22, 27, 24, "total 22" },
        { 170, BMP_CX, BMP_CX, 23, 27, 24, "total 23" },
        { 255, BMP_CX, BMP_CX, 24, 27, 24, "total 24" }
    };

    check_iml_data(himl, BMP_CX, BMP_CX, 0, 2, grow, ILC_COLOR24, "total 0");

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        image_list_add_bitmap(himl, td[i].grey, i + 1);
        check_iml_data(himl, td[i].cx, td[i].cy, td[i].cur, td[i].max, grow, td[i].bpp, td[i].comment);
    }
}

static void test_imagelist_storage(void)
{
    HIMAGELIST himl;
    INT ret, grow;
    HBITMAP hbm;
    HICON icon;

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 1, 1);
    ok(himl != 0, "ImageList_Create failed\n");

    check_iml_data(himl, BMP_CX, BMP_CX, 0, 2, 1, ILC_COLOR24, "empty");

    image_list_init(himl, 1);
    check_iml_data(himl, BMP_CX, BMP_CX, 24, 27, 1, ILC_COLOR24, "orig");

    ret = pImageList_Remove(himl, 4);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 23, 27, 1, ILC_COLOR24, "1");

    ret = pImageList_Remove(himl, 5);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 22, 27, 1, ILC_COLOR24, "2");

    ret = pImageList_Remove(himl, 6);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 21, 27, 1, ILC_COLOR24, "3");

    ret = pImageList_Remove(himl, 7);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, 1, ILC_COLOR24, "4");

    ret = pImageList_Remove(himl, -2);
    ok(!ret, "ImageList_Remove(-2) should fail\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, 1, ILC_COLOR24, "5");

    ret = pImageList_Remove(himl, 20);
    ok(!ret, "ImageList_Remove(20) should fail\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, 1, ILC_COLOR24, "6");

    ret = pImageList_Remove(himl, -1);
    ok(ret, "ImageList_Remove(-1) failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 4, 1, ILC_COLOR24, "7");

    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    /* test ImageList_Create storage allocation */

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 0, 32);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 1, 32, ILC_COLOR24, "init 0 grow 32");
    hbm = create_bitmap(BMP_CX * 9, BMP_CX, 0, "9");
    ret = pImageList_Add(himl, hbm, NULL);
    ok(ret == 0, "ImageList_Add returned %d, expected 0\n", ret);
    check_iml_data(himl, BMP_CX, BMP_CX, 1, 34, 32, ILC_COLOR24, "add 1 x 9");
    DeleteObject(hbm);
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 4, 4);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 5, 4, ILC_COLOR24, "init 4 grow 4");
    hbm = create_bitmap(BMP_CX, BMP_CX * 9, 0, "9");
    ret = pImageList_Add(himl, hbm, NULL);
    ok(ret == 0, "ImageList_Add returned %d, expected 0\n", ret);
    check_iml_data(himl, BMP_CX, BMP_CX, 9, 15, 4, ILC_COLOR24, "add 9 x 1");
    ret = pImageList_Add(himl, hbm, NULL);
    ok(ret == 9, "ImageList_Add returned %d, expected 9\n", ret);
    check_iml_data(himl, BMP_CX, BMP_CX, 18, 25, 4, ILC_COLOR24, "add 9 x 1");
    DeleteObject(hbm);
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 207, 209);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 208, 209, ILC_COLOR24, "init 207 grow 209");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 209, 207);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 210, 207, ILC_COLOR24, "init 209 grow 207");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 14, 4);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 15, 4, ILC_COLOR24, "init 14 grow 4");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 5, 9);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 6, 9, ILC_COLOR24, "init 5 grow 9");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 9, 5);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 10, 5, ILC_COLOR24, "init 9 grow 5");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 2, 4);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 3, 4, ILC_COLOR24, "init 2 grow 4");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 4, 2);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 5, 2, ILC_COLOR24, "init 4 grow 2");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR8, 4, 2);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 5, 2, ILC_COLOR8, "bpp 8");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR4, 4, 2);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 5, 2, ILC_COLOR4, "bpp 4");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, 0, 4, 2);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 5, 2, ILC_COLOR4, "bpp default");
    icon = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 0, "Failed to add icon.\n");
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 1, "Failed to add icon.\n");
    DestroyIcon( icon );
    check_iml_data(himl, BMP_CX, BMP_CX, 2, 5, 2, ILC_COLOR4, "bpp default");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24|ILC_MASK, 4, 2);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 5, 2, ILC_COLOR24|ILC_MASK, "bpp 24 + mask");
    icon = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 0, "Failed to add icon.\n");
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 1, "Failed to add icon.\n");
    DestroyIcon( icon );
    check_iml_data(himl, BMP_CX, BMP_CX, 2, 5, 2, ILC_COLOR24|ILC_MASK, "bpp 24 + mask");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR4|ILC_MASK, 4, 2);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 5, 2, ILC_COLOR4|ILC_MASK, "bpp 4 + mask");
    icon = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 0, "Failed to add icon.\n");
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 1, "Failed to add icon.\n");
    DestroyIcon( icon );
    check_iml_data(himl, BMP_CX, BMP_CX, 2, 5, 2, ILC_COLOR4|ILC_MASK, "bpp 4 + mask");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR4|ILC_MASK, 2, 99);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 3, 99, ILC_COLOR4|ILC_MASK, "init 2 grow 99");
    icon = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 0, "Failed to add icon.\n");
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 1, "Failed to add icon.\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 2, 3, 99, ILC_COLOR4|ILC_MASK, "init 2 grow 99 2 icons");
    ok( pImageList_ReplaceIcon(himl, -1, icon) == 2, "Failed to add icon\n");
    DestroyIcon( icon );
    check_iml_data(himl, BMP_CX, BMP_CX, 3, 104, 99, ILC_COLOR4|ILC_MASK, "init 2 grow 99 3 icons");
    ok( pImageList_Remove(himl, -1) == TRUE, "Failed to remove icon.\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 100, 99, ILC_COLOR4|ILC_MASK, "init 2 grow 99 empty");
    ok( pImageList_SetImageCount(himl, 22) == TRUE, "Failed to set image count.\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 22, 23, 99, ILC_COLOR4|ILC_MASK, "init 2 grow 99 set count 22");
    ok( pImageList_SetImageCount(himl, 0) == TRUE, "Failed to set image count.\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 1, 99, ILC_COLOR4|ILC_MASK, "init 2 grow 99 set count 0");
    ok( pImageList_SetImageCount(himl, 42) == TRUE, "Failed to set image count.\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 42, 43, 99, ILC_COLOR4|ILC_MASK, "init 2 grow 99 set count 42");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    for (grow = 1; grow <= 16; grow++)
    {
        himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR4|ILC_MASK, 2, grow);
        ok(himl != 0, "ImageList_Create failed\n");
        check_iml_data(himl, BMP_CX, BMP_CX, 0, 3, grow, ILC_COLOR4|ILC_MASK, "grow test");
        ret = pImageList_Destroy(himl);
        ok(ret, "ImageList_Destroy failed\n");
    }

    himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR4|ILC_MASK, 2, -20);
    ok(himl != 0, "ImageList_Create failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 3, -20, ILC_COLOR4|ILC_MASK, "init 2 grow -20");
    ret = pImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    /* Version 6 implementation hangs on large grow values. */
    if (!is_v6_test())
    {
        himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR4|ILC_MASK, 2, 65536+12);
        ok(himl != 0, "ImageList_Create failed\n");
        check_iml_data(himl, BMP_CX, BMP_CX, 0, 3, 65536+12, ILC_COLOR4|ILC_MASK, "init 2 grow 65536+12");
        ret = pImageList_Destroy(himl);
        ok(ret, "ImageList_Destroy failed\n");

        himl = pImageList_Create(BMP_CX, BMP_CX, ILC_COLOR4|ILC_MASK, 2, 65535);
        ok(himl != 0, "ImageList_Create failed\n");
        check_iml_data(himl, BMP_CX, BMP_CX, 0, 3, 65535, ILC_COLOR4|ILC_MASK, "init 2 grow 65535");
        ret = pImageList_Destroy(himl);
        ok(ret, "ImageList_Destroy failed\n");
    }
}

static void test_shell_imagelist(void)
{
    HRESULT (WINAPI *pSHGetImageList)(INT, REFIID, void**);
    IImageList *iml = NULL;
    HMODULE hShell32;
    HRESULT hr;
    int out = 0;
    RECT rect;
    int cx, cy;

    /* Try to load function from shell32 */
    hShell32 = LoadLibraryA("shell32.dll");
    pSHGetImageList = (void*)GetProcAddress(hShell32, (LPCSTR) 727);

    if (!pSHGetImageList)
    {
        win_skip("SHGetImageList not available, skipping test\n");
        FreeLibrary(hShell32);
        return;
    }

    /* Get system image list */
    hr = pSHGetImageList(SHIL_SYSSMALL, &IID_IImageList, (void**)&iml);
    ok(SUCCEEDED(hr), "SHGetImageList failed, hr=%x\n", hr);

    if (hr != S_OK) {
        FreeLibrary(hShell32);
        return;
    }

    IImageList_GetImageCount(iml, &out);
    ok(out > 0, "IImageList_GetImageCount returned out <= 0\n");

    /* Fetch the small icon size */
    cx = GetSystemMetrics(SM_CXSMICON);
    cy = GetSystemMetrics(SM_CYSMICON);

    /* Check icon size matches */
    IImageList_GetImageRect(iml, 0, &rect);
    ok(((rect.right == cx) && (rect.bottom == cy)),
                 "IImageList_GetImageRect returned r:%d,b:%d\n",
                 rect.right, rect.bottom);

    IImageList_Release(iml);
    FreeLibrary(hShell32);
}

static HBITMAP create_test_bitmap(HDC hdc, int bpp, UINT32 pixel1, UINT32 pixel2)
{
    HBITMAP hBitmap;
    UINT32 *buffer = NULL;
    BITMAPINFO bitmapInfo = {{sizeof(BITMAPINFOHEADER), 2, 1, 1, bpp, BI_RGB,
                                0, 0, 0, 0, 0}};

    hBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, (void**)&buffer, NULL, 0);
    ok(hBitmap != NULL && buffer != NULL, "CreateDIBSection failed.\n");

    if(!hBitmap || !buffer)
    {
        DeleteObject(hBitmap);
        return NULL;
    }

    buffer[0] = pixel1;
    buffer[1] = pixel2;

    return hBitmap;
}

static BOOL colour_match(UINT32 x, UINT32 y)
{
    const INT32 tolerance = 8;

    const INT32 dr = abs((INT32)(x & 0x000000FF) - (INT32)(y & 0x000000FF));
    const INT32 dg = abs((INT32)((x & 0x0000FF00) >> 8) - (INT32)((y & 0x0000FF00) >> 8));
    const INT32 db = abs((INT32)((x & 0x00FF0000) >> 16) - (INT32)((y & 0x00FF0000) >> 16));

    return (dr <= tolerance && dg <= tolerance && db <= tolerance);
}

static void check_ImageList_DrawIndirect(IMAGELISTDRAWPARAMS *ildp, UINT32 *bits,
                                         UINT32 expected, int line)
{
    bits[0] = 0x00FFFFFF;
    pImageList_DrawIndirect(ildp);
    ok(colour_match(bits[0], expected),
       "ImageList_DrawIndirect: Pixel %08X, Expected a close match to %08X from line %d\n",
       bits[0] & 0x00FFFFFF, expected, line);
}


static void check_ImageList_DrawIndirect_fStyle(HDC hdc, HIMAGELIST himl, UINT32 *bits, int i,
                                                UINT fStyle, UINT32 expected, int line)
{
    IMAGELISTDRAWPARAMS ildp = {sizeof(IMAGELISTDRAWPARAMS), himl, i, hdc,
        0, 0, 0, 0, 0, 0, CLR_NONE, CLR_NONE, fStyle, 0, ILS_NORMAL, 0, 0x00000000};
    check_ImageList_DrawIndirect(&ildp, bits, expected, line);
}

static void check_ImageList_DrawIndirect_ILD_ROP(HDC hdc, HIMAGELIST himl, UINT32 *bits, int i,
                                                DWORD dwRop, UINT32 expected, int line)
{
    IMAGELISTDRAWPARAMS ildp = {sizeof(IMAGELISTDRAWPARAMS), himl, i, hdc,
        0, 0, 0, 0, 0, 0, CLR_NONE, CLR_NONE, ILD_IMAGE | ILD_ROP, dwRop, ILS_NORMAL, 0, 0x00000000};
    check_ImageList_DrawIndirect(&ildp, bits, expected, line);
}

static void check_ImageList_DrawIndirect_fState(HDC hdc, HIMAGELIST himl, UINT32 *bits, int i, UINT fStyle,
                                                UINT fState, DWORD Frame, UINT32 expected, int line)
{
    IMAGELISTDRAWPARAMS ildp = {sizeof(IMAGELISTDRAWPARAMS), himl, i, hdc,
        0, 0, 0, 0, 0, 0, CLR_NONE, CLR_NONE, fStyle, 0, fState, Frame, 0x00000000};
    check_ImageList_DrawIndirect(&ildp, bits, expected, line);
}

static void check_ImageList_DrawIndirect_broken(HDC hdc, HIMAGELIST himl, UINT32 *bits, int i,
                                                UINT fStyle, UINT fState, DWORD Frame, UINT32 expected,
                                                UINT32 broken_expected, int line)
{
    IMAGELISTDRAWPARAMS ildp = {sizeof(IMAGELISTDRAWPARAMS), himl, i, hdc,
        0, 0, 0, 0, 0, 0, CLR_NONE, CLR_NONE, fStyle, 0, fState, Frame, 0x00000000};
    bits[0] = 0x00FFFFFF;
    pImageList_DrawIndirect(&ildp);
    ok(colour_match(bits[0], expected) ||
       broken(colour_match(bits[0], broken_expected)),
       "ImageList_DrawIndirect: Pixel %08X, Expected a close match to %08X from line %d\n",
       bits[0] & 0x00FFFFFF, expected, line);
}

static void test_ImageList_DrawIndirect(void)
{
    HIMAGELIST himl = NULL;
    int ret;
    HDC hdcDst = NULL;
    HBITMAP hbmOld = NULL, hbmDst = NULL;
    HBITMAP hbmMask = NULL, hbmInverseMask = NULL;
    HBITMAP hbmImage = NULL, hbmAlphaImage = NULL, hbmTransparentImage = NULL;
    int iImage = -1, iAlphaImage = -1, iTransparentImage = -1;
    UINT32 *bits = 0;
    UINT32 maskBits = 0x00000000, inverseMaskBits = 0xFFFFFFFF;
    int bpp, broken_value;

    BITMAPINFO bitmapInfo = {{sizeof(BITMAPINFOHEADER), 2, 1, 1, 32, BI_RGB,
                                0, 0, 0, 0, 0}};

    hdcDst = CreateCompatibleDC(0);
    ok(hdcDst != 0, "CreateCompatibleDC(0) failed to return a valid DC\n");
    if (!hdcDst)
        return;
    bpp = GetDeviceCaps(hdcDst, BITSPIXEL);

    hbmMask = CreateBitmap(2, 1, 1, 1, &maskBits);
    ok(hbmMask != 0, "CreateBitmap failed\n");
    if(!hbmMask) goto cleanup;

    hbmInverseMask = CreateBitmap(2, 1, 1, 1, &inverseMaskBits);
    ok(hbmInverseMask != 0, "CreateBitmap failed\n");
    if(!hbmInverseMask) goto cleanup;

    himl = pImageList_Create(2, 1, ILC_COLOR32, 0, 1);
    ok(himl != 0, "ImageList_Create failed\n");
    if(!himl) goto cleanup;

    /* Add a no-alpha image */
    hbmImage = create_test_bitmap(hdcDst, 32, 0x00ABCDEF, 0x00ABCDEF);
    if(!hbmImage) goto cleanup;

    iImage = pImageList_Add(himl, hbmImage, hbmMask);
    ok(iImage != -1, "ImageList_Add failed\n");
    if(iImage == -1) goto cleanup;

    /* Add an alpha image */
    hbmAlphaImage = create_test_bitmap(hdcDst, 32, 0x89ABCDEF, 0x89ABCDEF);
    if(!hbmAlphaImage) goto cleanup;

    iAlphaImage = pImageList_Add(himl, hbmAlphaImage, hbmMask);
    ok(iAlphaImage != -1, "ImageList_Add failed\n");
    if(iAlphaImage == -1) goto cleanup;

    /* Add a transparent alpha image */
    hbmTransparentImage = create_test_bitmap(hdcDst, 32, 0x00ABCDEF, 0x89ABCDEF);
    if(!hbmTransparentImage) goto cleanup;

    iTransparentImage = pImageList_Add(himl, hbmTransparentImage, hbmMask);
    ok(iTransparentImage != -1, "ImageList_Add failed\n");
    if(iTransparentImage == -1) goto cleanup;

    /* 32-bit Tests */
    bitmapInfo.bmiHeader.biBitCount = 32;
    hbmDst = CreateDIBSection(hdcDst, &bitmapInfo, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok (hbmDst && bits, "CreateDIBSection failed to return a valid bitmap and buffer\n");
    if (!hbmDst || !bits)
        goto cleanup;
    hbmOld = SelectObject(hdcDst, hbmDst);

    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_NORMAL, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_TRANSPARENT, 0x00ABCDEF, __LINE__);
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_BLEND25, ILS_NORMAL, 0, 0x00E8F1FA, 0x00D4D9DD, __LINE__);
    if (bpp == 16 || bpp == 24) broken_value = 0x00D4D9DD;
    else broken_value = 0x00B4BDC4;
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_BLEND50, ILS_NORMAL, 0, 0x00E8F1FA, broken_value, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_MASK, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_IMAGE, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_PRESERVEALPHA, 0x00ABCDEF, __LINE__);

    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_NORMAL, 0x00D3E5F7, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_TRANSPARENT, 0x00D3E5F7, __LINE__);

    if (bpp == 16 || bpp == 24) broken_value = 0x00D4D9DD;
    else broken_value =  0x009DA8B1;
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_BLEND25, ILS_NORMAL, 0, 0x00E8F1FA, broken_value, __LINE__);
    if (bpp == 16 || bpp == 24) broken_value = 0x00D4D9DD;
    else broken_value =  0x008C99A3;
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_BLEND50, ILS_NORMAL, 0, 0x00E8F1FA, broken_value, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_MASK, 0x00D3E5F7, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_IMAGE, 0x00D3E5F7, __LINE__);
    todo_wine check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_PRESERVEALPHA, 0x005D6F81, __LINE__);

    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iTransparentImage, ILD_NORMAL, 0x00FFFFFF, __LINE__);

    check_ImageList_DrawIndirect_ILD_ROP(hdcDst, himl, bits, iImage, SRCCOPY, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_ILD_ROP(hdcDst, himl, bits, iImage, SRCINVERT, 0x00543210, __LINE__);

    /* ILD_ROP is ignored when the image has an alpha channel */
    check_ImageList_DrawIndirect_ILD_ROP(hdcDst, himl, bits, iAlphaImage, SRCCOPY, 0x00D3E5F7, __LINE__);
    check_ImageList_DrawIndirect_ILD_ROP(hdcDst, himl, bits, iAlphaImage, SRCINVERT, 0x00D3E5F7, __LINE__);

    todo_wine check_ImageList_DrawIndirect_fState(hdcDst, himl, bits, iImage, ILD_NORMAL, ILS_SATURATE, 0, 0x00CCCCCC, __LINE__);
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_NORMAL, ILS_SATURATE, 0, 0x00AFAFAF, 0x00F0F0F0, __LINE__);

    check_ImageList_DrawIndirect_fState(hdcDst, himl, bits, iImage, ILD_NORMAL, ILS_GLOW, 0, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_fState(hdcDst, himl, bits, iImage, ILD_NORMAL, ILS_SHADOW, 0, 0x00ABCDEF, __LINE__);

    check_ImageList_DrawIndirect_fState(hdcDst, himl, bits, iImage, ILD_NORMAL, ILS_ALPHA, 127, 0x00D5E6F7, __LINE__);
    check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_NORMAL, ILS_ALPHA, 127, 0x00E9F2FB, 0x00AEB7C0, __LINE__);
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_NORMAL, ILS_NORMAL, 127, 0x00E9F2FB, 0x00D3E5F7, __LINE__);

cleanup:

    if(hbmOld)
        SelectObject(hdcDst, hbmOld);
    if(hbmDst)
        DeleteObject(hbmDst);

    DeleteDC(hdcDst);

    if(hbmMask)
        DeleteObject(hbmMask);
    if(hbmInverseMask)
        DeleteObject(hbmInverseMask);

    if(hbmImage)
        DeleteObject(hbmImage);
    if(hbmAlphaImage)
        DeleteObject(hbmAlphaImage);
    if(hbmTransparentImage)
        DeleteObject(hbmTransparentImage);

    if(himl)
    {
        ret = pImageList_Destroy(himl);
        ok(ret, "ImageList_Destroy failed\n");
    }
}

static void test_iimagelist(void)
{
    IImageList *imgl, *imgl2;
    IImageList2 *imagelist;
    HIMAGELIST himl;
    HRESULT hr;
    ULONG ret;

    if (!pHIMAGELIST_QueryInterface)
    {
        win_skip("XP imagelist functions not available\n");
        return;
    }

    /* test reference counting on destruction */
    imgl = (IImageList*)createImageList(32, 32);
    ret = IImageList_AddRef(imgl);
    ok(ret == 2, "Expected 2, got %d\n", ret);
    ret = pImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ret = pImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ret = pImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);

    imgl = (IImageList*)createImageList(32, 32);
    ret = IImageList_AddRef(imgl);
    ok(ret == 2, "Expected 2, got %d\n", ret);
    ret = pImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ret = IImageList_Release(imgl);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ret = pImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);

    /* ref counting, HIMAGELIST_QueryInterface adds a reference */
    imgl = (IImageList*)createImageList(32, 32);
    hr = pHIMAGELIST_QueryInterface((HIMAGELIST)imgl, &IID_IImageList, (void**)&imgl2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(imgl2 == imgl, "got different pointer\n");
    ret = IImageList_Release(imgl);
    ok(ret == 1, "got %u\n", ret);
    IImageList_Release(imgl);

    if (!pImageList_CoCreateInstance)
    {
        win_skip("Vista imagelist functions not available\n");
        return;
    }

    hr = pImageList_CoCreateInstance(&CLSID_ImageList, NULL, &IID_IImageList, (void **) &imgl);
    ok(SUCCEEDED(hr), "ImageList_CoCreateInstance failed, hr=%x\n", hr);

    if (hr == S_OK)
        IImageList_Release(imgl);

    himl = createImageList(32, 32);

    if (!himl)
        return;

    hr = (pHIMAGELIST_QueryInterface)(himl, &IID_IImageList, (void **) &imgl);
    ok(SUCCEEDED(hr), "HIMAGELIST_QueryInterface failed, hr=%x\n", hr);

    if (hr == S_OK)
        IImageList_Release(imgl);

    pImageList_Destroy(himl);

    /* IImageList2 */
    hr = pImageList_CoCreateInstance(&CLSID_ImageList, NULL, &IID_IImageList2, (void**)&imagelist);
    if (hr != S_OK)
    {
        win_skip("IImageList2 is not supported.\n");
        return;
    }
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IImageList2_Release(imagelist);
}

static void test_IImageList_Add_Remove(void)
{
    IImageList *imgl;
    HIMAGELIST himl;
    HRESULT hr;

    HICON hicon1;
    HICON hicon2;
    HICON hicon3;

    int ret;

    /* create an imagelist to play with */
    himl = pImageList_Create(84, 84, ILC_COLOR16, 0, 3);
    ok(himl != 0,"failed to create imagelist\n");

    imgl = (IImageList *) himl;

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* remove when nothing exists */
    hr = IImageList_Remove(imgl, 0);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    /* removing everything from an empty imagelist should succeed */
    hr = IImageList_Remove(imgl, -1);
    ok(hr == S_OK, "removed nonexistent icon\n");

    /* add three */
    ret = -1;
    ok( IImageList_ReplaceIcon(imgl, -1, hicon1, &ret) == S_OK && (ret == 0),"failed to add icon1\n");
    ret = -1;
    ok( IImageList_ReplaceIcon(imgl, -1, hicon2, &ret) == S_OK && (ret == 1),"failed to add icon2\n");
    ret = -1;
    ok( IImageList_ReplaceIcon(imgl, -1, hicon3, &ret) == S_OK && (ret == 2),"failed to add icon3\n");

    /* remove an index out of range */
    ok( IImageList_Remove(imgl, 4711) == E_INVALIDARG, "got 0x%08x\n", hr);

    /* remove three */
    ok( IImageList_Remove(imgl,0) == S_OK, "can't remove 0\n");
    ok( IImageList_Remove(imgl,0) == S_OK, "can't remove 0\n");
    ok( IImageList_Remove(imgl,0) == S_OK, "can't remove 0\n");

    /* remove one extra */
    ok( IImageList_Remove(imgl, 0) == E_INVALIDARG, "got 0x%08x\n", hr);

    IImageList_Release(imgl);
    ok(DestroyIcon(hicon1),"icon 1 wasn't deleted\n");
    ok(DestroyIcon(hicon2),"icon 2 wasn't deleted\n");
    ok(DestroyIcon(hicon3),"icon 3 wasn't deleted\n");
}

static void test_IImageList_Get_SetImageCount(void)
{
    IImageList *imgl;
    HIMAGELIST himl;
    HRESULT hr;
    INT ret;

    /* create an imagelist to play with */
    himl = pImageList_Create(84, 84, ILC_COLOR16, 0, 3);
    ok(himl != 0,"failed to create imagelist\n");

    imgl = (IImageList *) himl;

    /* check SetImageCount/GetImageCount */
    hr = IImageList_SetImageCount(imgl, 3);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ret = 0;
    hr = IImageList_GetImageCount(imgl, &ret);
    ok(hr == S_OK && ret == 3, "invalid image count after increase\n");
    hr = IImageList_SetImageCount(imgl, 1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ret = 0;
    hr = IImageList_GetImageCount(imgl, &ret);
    ok(hr == S_OK && ret == 1, "invalid image count after decrease to 1\n");
    hr = IImageList_SetImageCount(imgl, 0);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ret = -1;
    hr = IImageList_GetImageCount(imgl, &ret);
    ok(hr == S_OK && ret == 0, "invalid image count after decrease to 0\n");

    IImageList_Release(imgl);
}

static void test_IImageList_Draw(void)
{
    IImageList *imgl;
    HIMAGELIST himl;

    HBITMAP hbm1;
    HBITMAP hbm2;
    HBITMAP hbm3;

    IMAGELISTDRAWPARAMS imldp;
    HWND hwndfortest;
    HRESULT hr;
    HDC hdc;
    int ret;

    hwndfortest = create_window();
    hdc = GetDC(hwndfortest);
    ok(hdc!=NULL, "couldn't get DC\n");

    /* create an imagelist to play with */
    himl = pImageList_Create(48, 48, ILC_COLOR16, 0, 3);
    ok(himl!=0,"failed to create imagelist\n");

    imgl = (IImageList *) himl;

    /* load the icons to add to the image list */
    hbm1 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm1 != 0, "no bitmap 1\n");
    hbm2 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm2 != 0, "no bitmap 2\n");
    hbm3 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm3 != 0, "no bitmap 3\n");

    /* add three */
    ret = -1;
    ok( IImageList_Add(imgl, hbm1, 0, &ret) == S_OK && (ret == 0), "failed to add bitmap 1\n");
    ret = -1;
    ok( IImageList_Add(imgl, hbm2, 0, &ret) == S_OK && (ret == 1), "failed to add bitmap 2\n");

    ok( IImageList_SetImageCount(imgl, 3) == S_OK, "Setimage count failed\n");
    ok( IImageList_Replace(imgl, 2, hbm3, 0) == S_OK, "failed to replace bitmap 3\n");

if (0)
{
    /* crashes on native */
    IImageList_Draw(imgl, NULL);
}

    memset(&imldp, 0, sizeof (imldp));
    hr = IImageList_Draw(imgl, &imldp);
    ok( hr == E_INVALIDARG, "got 0x%08x\n", hr);

    imldp.cbSize = IMAGELISTDRAWPARAMS_V3_SIZE;
    imldp.hdcDst = hdc;
    imldp.himl = himl;

    force_redraw(hwndfortest);

    imldp.fStyle = SRCCOPY;
    imldp.rgbBk = CLR_DEFAULT;
    imldp.rgbFg = CLR_DEFAULT;
    imldp.y = 100;
    imldp.x = 100;
    ok( IImageList_Draw(imgl, &imldp) == S_OK, "should succeed\n");
    imldp.i ++;
    ok( IImageList_Draw(imgl, &imldp) == S_OK, "should succeed\n");
    imldp.i ++;
    ok( IImageList_Draw(imgl, &imldp) == S_OK, "should succeed\n");
    imldp.i ++;
    ok( IImageList_Draw(imgl, &imldp) == E_INVALIDARG, "should fail\n");

    /* remove three */
    ok( IImageList_Remove(imgl, 0) == S_OK, "removing 1st bitmap\n");
    ok( IImageList_Remove(imgl, 0) == S_OK, "removing 2nd bitmap\n");
    ok( IImageList_Remove(imgl, 0) == S_OK, "removing 3rd bitmap\n");

    /* destroy it */
    IImageList_Release(imgl);

    /* bitmaps should not be deleted by the imagelist */
    ok(DeleteObject(hbm1),"bitmap 1 can't be deleted\n");
    ok(DeleteObject(hbm2),"bitmap 2 can't be deleted\n");
    ok(DeleteObject(hbm3),"bitmap 3 can't be deleted\n");

    ReleaseDC(hwndfortest, hdc);
    DestroyWindow(hwndfortest);
}

static void test_IImageList_Merge(void)
{
    HIMAGELIST himl1, himl2;
    IImageList *imgl1, *imgl2, *merge;
    HICON hicon1;
    HWND hwnd = create_window();
    HRESULT hr;
    int ret;

    himl1 = pImageList_Create(32,32,0,0,3);
    ok(himl1 != NULL,"failed to create himl1\n");

    himl2 = pImageList_Create(32,32,0,0,3);
    ok(himl2 != NULL,"failed to create himl2\n");

    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != NULL, "failed to create hicon1\n");

    if (!himl1 || !himl2 || !hicon1)
        return;

    /* cast to IImageList */
    imgl1 = (IImageList *) himl1;
    imgl2 = (IImageList *) himl2;

    ret = -1;
    ok( IImageList_ReplaceIcon(imgl2, -1, hicon1, &ret) == S_OK && (ret == 0),"add icon1 to himl2 failed\n");

if (0)
{
    /* null cases that crash on native */
    IImageList_Merge(imgl1, -1, NULL, 0, 0, 0, &IID_IImageList, (void**)&merge);
    IImageList_Merge(imgl1, -1, (IUnknown*) imgl2, 0, 0, 0, &IID_IImageList, NULL);
}

    /* If himl1 has no images, merge still succeeds */
    hr = IImageList_Merge(imgl1, -1, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(hr == S_OK, "merge himl1,-1 failed\n");
    if (hr == S_OK) IImageList_Release(merge);

    hr = IImageList_Merge(imgl1, 0, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(hr == S_OK, "merge himl1,0 failed\n");
    if (hr == S_OK) IImageList_Release(merge);

    /* Same happens if himl2 is empty */
    IImageList_Release(imgl2);
    himl2 = pImageList_Create(32,32,0,0,3);
    ok(himl2 != NULL,"failed to recreate himl2\n");

    imgl2 = (IImageList *) himl2;

    hr = IImageList_Merge(imgl1, -1, (IUnknown *) imgl2, -1, 0, 0, &IID_IImageList, (void **) &merge);
    ok(hr == S_OK, "merge himl2,-1 failed\n");
    if (hr == S_OK) IImageList_Release(merge);

    hr = IImageList_Merge(imgl1, -1, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(hr == S_OK, "merge himl2,0 failed\n");
    if (hr == S_OK) IImageList_Release(merge);

    /* Now try merging an image with itself */
    ret = -1;
    ok( IImageList_ReplaceIcon(imgl2, -1, hicon1, &ret) == S_OK && (ret == 0),"re-add icon1 to himl2 failed\n");

    hr = IImageList_Merge(imgl2, 0, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(hr == S_OK, "merge himl2 with itself failed\n");
    if (hr == S_OK) IImageList_Release(merge);

    /* Try merging 2 different image lists */
    ret = -1;
    ok( IImageList_ReplaceIcon(imgl1, -1, hicon1, &ret) == S_OK && (ret == 0),"add icon1 to himl1 failed\n");

    hr = IImageList_Merge(imgl1, 0, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(hr == S_OK, "merge himl1 with himl2 failed\n");
    if (hr == S_OK) IImageList_Release(merge);

    hr = IImageList_Merge(imgl1, 0, (IUnknown *) imgl2, 0, 8, 16, &IID_IImageList, (void **) &merge);
    ok(hr == S_OK, "merge himl1 with himl2 8,16 failed\n");
    if (hr == S_OK) IImageList_Release(merge);

    IImageList_Release(imgl1);
    IImageList_Release(imgl2);

    DestroyIcon(hicon1);
    DestroyWindow(hwnd);
}

static void test_iconsize(void)
{
    HIMAGELIST himl;
    INT cx, cy;
    BOOL ret;

    himl = pImageList_Create(16, 16, ILC_COLOR16, 0, 3);
    /* null pointers, not zero imagelist dimensions */
    ret = pImageList_GetIconSize(himl, NULL, NULL);
    ok(!ret, "got %d\n", ret);

    /* doesn't touch return pointers */
    cx = 0x1abe11ed;
    ret = pImageList_GetIconSize(himl, &cx, NULL);
    ok(!ret, "got %d\n", ret);
    ok(cx == 0x1abe11ed, "got %d\n", cx);

    cy = 0x1abe11ed;
    ret = pImageList_GetIconSize(himl, NULL, &cy);
    ok(!ret, "got %d\n", ret);
    ok(cy == 0x1abe11ed, "got %d\n", cy);

    pImageList_Destroy(himl);

    ret = pImageList_GetIconSize((HIMAGELIST)0xdeadbeef, &cx, &cy);
    ok(!ret, "got %d\n", ret);
}

static void test_create_destroy(void)
{
    HIMAGELIST himl;
    INT cx, cy;
    BOOL rc;
    INT ret;

    /* list with zero or negative image dimensions */
    himl = pImageList_Create(0, 0, ILC_COLOR16, 0, 3);
    ok(himl == NULL, "got %p\n", himl);

    himl = pImageList_Create(0, 16, ILC_COLOR16, 0, 3);
    ok(himl == NULL, "got %p\n", himl);

    himl = pImageList_Create(16, 0, ILC_COLOR16, 0, 3);
    ok(himl == NULL, "got %p\n", himl);

    himl = pImageList_Create(16, -1, ILC_COLOR16, 0, 3);
    ok(himl == NULL, "got %p\n", himl);

    himl = pImageList_Create(-1, 16, ILC_COLOR16, 0, 3);
    ok(himl == NULL, "got %p\n", himl);

    rc = pImageList_Destroy((HIMAGELIST)0xdeadbeef);
    ok(rc == FALSE, "ImageList_Destroy(0xdeadbeef) should fail and not crash\n");

    /* DDB image lists */
    himl = pImageList_Create(0, 14, ILC_COLORDDB, 4, 4);
    ok(himl != NULL, "got %p\n", himl);

    pImageList_GetIconSize(himl, &cx, &cy);
    ok (cx == 0, "Wrong cx (%i)\n", cx);
    ok (cy == 14, "Wrong cy (%i)\n", cy);
    pImageList_Destroy(himl);

    himl = pImageList_Create(0, 0, ILC_COLORDDB, 4, 4);
    ok(himl != NULL, "got %p\n", himl);
    pImageList_GetIconSize(himl, &cx, &cy);
    ok (cx == 0, "Wrong cx (%i)\n", cx);
    ok (cy == 0, "Wrong cy (%i)\n", cy);
    pImageList_Destroy(himl);

    himl = pImageList_Create(0, 0, ILC_COLORDDB, 0, 4);
    ok(himl != NULL, "got %p\n", himl);
    pImageList_GetIconSize(himl, &cx, &cy);
    ok (cx == 0, "Wrong cx (%i)\n", cx);
    ok (cy == 0, "Wrong cy (%i)\n", cy);

    pImageList_SetImageCount(himl, 3);
    ret = pImageList_GetImageCount(himl);
    ok(ret == 3, "Unexpected image count after increase\n");

    /* Trying to actually add an image causes a crash on Windows */
    pImageList_Destroy(himl);

    /* Negative values fail */
    himl = pImageList_Create(-1, -1, ILC_COLORDDB, 4, 4);
    ok(himl == NULL, "got %p\n", himl);
    himl = pImageList_Create(-1, 1, ILC_COLORDDB, 4, 4);
    ok(himl == NULL, "got %p\n", himl);
    himl = pImageList_Create(1, -1, ILC_COLORDDB, 4, 4);
    ok(himl == NULL, "got %p\n", himl);
}

static void check_color_table(const char *name, HDC hdc, HIMAGELIST himl, UINT ilc,
                              RGBQUAD *expect, RGBQUAD *broken_expect)
{
    IMAGEINFO info;
    INT ret;
#ifdef __REACTOS__
    char bmi_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors) + 256 * sizeof(RGBQUAD)];
#else
    char bmi_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
#endif
    BITMAPINFO *bmi = (BITMAPINFO *)bmi_buffer;
    int i, depth = ilc & 0xfe;

    ret = pImageList_GetImageInfo(himl, 0, &info);
    ok(ret, "got %d\n", ret);
    ok(info.hbmImage != NULL, "got %p\n", info.hbmImage);

    memset(bmi_buffer, 0, sizeof(bmi_buffer));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    ret = GetDIBits(hdc, info.hbmImage, 0, 0, NULL, bmi, DIB_RGB_COLORS);
    ok(ret, "got %d\n", ret);
    ok(bmi->bmiHeader.biBitCount == depth, "got %d\n", bmi->bmiHeader.biBitCount);

    ret = GetDIBits(hdc, info.hbmImage, 0, 0, NULL, bmi, DIB_RGB_COLORS);
    ok(ret, "got %d\n", ret);
    ok(bmi->bmiHeader.biBitCount == depth, "got %d\n", bmi->bmiHeader.biBitCount);

    for (i = 0; i < (1 << depth); i++)
        ok((bmi->bmiColors[i].rgbRed == expect[i].rgbRed &&
            bmi->bmiColors[i].rgbGreen == expect[i].rgbGreen &&
            bmi->bmiColors[i].rgbBlue == expect[i].rgbBlue) ||
           (broken_expect && broken(bmi->bmiColors[i].rgbRed == broken_expect[i].rgbRed &&
                  bmi->bmiColors[i].rgbGreen == broken_expect[i].rgbGreen &&
                  bmi->bmiColors[i].rgbBlue == broken_expect[i].rgbBlue)),
           "%d: %s: got color[%d] %02x %02x %02x expect %02x %02x %02x\n", depth, name, i,
           bmi->bmiColors[i].rgbRed, bmi->bmiColors[i].rgbGreen, bmi->bmiColors[i].rgbBlue,
           expect[i].rgbRed, expect[i].rgbGreen, expect[i].rgbBlue);
}

static void get_default_color_table(HDC hdc, int bpp, RGBQUAD *table)
{
#ifdef __REACTOS__
     char bmi_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors) + 256 * sizeof(RGBQUAD)];
#else
    char bmi_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
#endif
    BITMAPINFO *bmi = (BITMAPINFO *)bmi_buffer;
    HBITMAP tmp;
    int i;
    HPALETTE pal;
    PALETTEENTRY entries[256];

    switch (bpp)
    {
    case 4:
        tmp = CreateBitmap( 1, 1, 1, 1, NULL );
        memset(bmi_buffer, 0, sizeof(bmi_buffer));
        bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
        bmi->bmiHeader.biHeight = 1;
        bmi->bmiHeader.biWidth = 1;
        bmi->bmiHeader.biBitCount = bpp;
        bmi->bmiHeader.biPlanes = 1;
        bmi->bmiHeader.biCompression = BI_RGB;
        GetDIBits( hdc, tmp, 0, 0, NULL, bmi, DIB_RGB_COLORS );

        memcpy(table, bmi->bmiColors, (1 << bpp) * sizeof(RGBQUAD));
        table[7] = bmi->bmiColors[8];
        table[8] = bmi->bmiColors[7];
        DeleteObject( tmp );
        break;

    case 8:
        pal = CreateHalftonePalette(hdc);
        GetPaletteEntries(pal, 0, 256, entries);
        for (i = 0; i < 256; i++)
        {
            table[i].rgbRed = entries[i].peRed;
            table[i].rgbGreen = entries[i].peGreen;
            table[i].rgbBlue = entries[i].peBlue;
            table[i].rgbReserved = 0;
        }
        DeleteObject(pal);
        break;

    default:
        ok(0, "unhandled depth %d\n", bpp);
    }
}

static void test_color_table(UINT ilc)
{
    HIMAGELIST himl;
    INT ret;
#ifdef __REACTOS__
    char bmi_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors) + 256 * sizeof(RGBQUAD)];
#else
    char bmi_buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
#endif
    BITMAPINFO *bmi = (BITMAPINFO *)bmi_buffer;
    HDC hdc = CreateCompatibleDC(0);
    HBITMAP dib4, dib8, dib32;
    RGBQUAD rgb[256], default_table[256];

    get_default_color_table(hdc, ilc & 0xfe, default_table);

    himl = pImageList_Create(16, 16, ilc, 0, 3);
    ok(himl != NULL, "got %p\n", himl);

    memset(bmi_buffer, 0, sizeof(bmi_buffer));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = 16;
    bmi->bmiHeader.biWidth = 16;
    bmi->bmiHeader.biBitCount = 8;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmi->bmiColors[0].rgbRed = 0xff;
    bmi->bmiColors[1].rgbGreen = 0xff;
    bmi->bmiColors[2].rgbBlue = 0xff;

    dib8 = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);

    bmi->bmiHeader.biBitCount = 4;
    bmi->bmiColors[0].rgbRed   = 0xff;
    bmi->bmiColors[0].rgbGreen = 0x00;
    bmi->bmiColors[0].rgbBlue  = 0xff;
    bmi->bmiColors[1].rgbRed   = 0xff;
    bmi->bmiColors[1].rgbGreen = 0xff;
    bmi->bmiColors[1].rgbBlue  = 0x00;
    bmi->bmiColors[2].rgbRed   = 0x00;
    bmi->bmiColors[2].rgbGreen = 0xff;
    bmi->bmiColors[2].rgbBlue  = 0xff;

    dib4 = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);

    bmi->bmiHeader.biBitCount = 32;

    dib32 = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);

    /* add 32 first then 8.  This won't set the color table */
    ret = pImageList_Add(himl, dib32, NULL);
    ok(ret == 0, "got %d\n", ret);
    ret = pImageList_Add(himl, dib8, NULL);
    ok(ret == 1, "got %d\n", ret);

    check_color_table("add 32, 8", hdc, himl, ilc, default_table, NULL);

    /* since the previous _Adds didn't set the color table, this one will */
    ret = pImageList_Remove(himl, -1);
    ok(ret, "got %d\n", ret);
    ret = pImageList_Add(himl, dib8, NULL);
    ok(ret == 0, "got %d\n", ret);

    memset(rgb, 0, sizeof(rgb));
    rgb[0].rgbRed = 0xff;
    rgb[1].rgbGreen = 0xff;
    rgb[2].rgbBlue = 0xff;
    check_color_table("remove all, add 8", hdc, himl, ilc, rgb, default_table);

    /* remove all, add 4. Color table remains the same since it's implicitly
       been set by the previous _Add */
    ret = pImageList_Remove(himl, -1);
    ok(ret, "got %d\n", ret);
    ret = pImageList_Add(himl, dib4, NULL);
    ok(ret == 0, "got %d\n", ret);
    check_color_table("remove all, add 4", hdc, himl, ilc, rgb, default_table);

    pImageList_Destroy(himl);
    himl = pImageList_Create(16, 16, ilc, 0, 3);
    ok(himl != NULL, "got %p\n", himl);

    /* add 4 */
    ret = pImageList_Add(himl, dib4, NULL);
    ok(ret == 0, "got %d\n", ret);

    memset(rgb, 0, 16 * sizeof(rgb[0]));
    rgb[0].rgbRed = 0xff;
    rgb[0].rgbBlue = 0xff;
    rgb[1].rgbRed = 0xff;
    rgb[1].rgbGreen = 0xff;
    rgb[2].rgbGreen = 0xff;
    rgb[2].rgbBlue = 0xff;
    memcpy(rgb + 16, default_table + 16, 240 * sizeof(rgb[0]));

    check_color_table("add 4", hdc, himl, ilc, rgb, default_table);

    pImageList_Destroy(himl);
    himl = pImageList_Create(16, 16, ilc, 0, 3);
    ok(himl != NULL, "got %p\n", himl);

    /* set color table, add 8 */
    ret = pImageList_Remove(himl, -1);
    ok(ret, "got %d\n", ret);
    memset(rgb, 0, sizeof(rgb));
    rgb[0].rgbRed = 0xcc;
    rgb[1].rgbBlue = 0xcc;
    ret = pImageList_SetColorTable(himl, 0, 2, rgb);
    ok(ret == 2, "got %d\n", ret);
    /* the table is set, so this doesn't change it */
    ret = pImageList_Add(himl, dib8, NULL);
    ok(ret == 0, "got %d\n", ret);

    memcpy(rgb + 2, default_table + 2, 254 * sizeof(rgb[0]));
    check_color_table("SetColorTable", hdc, himl, ilc, rgb, NULL);

    DeleteObject(dib32);
    DeleteObject(dib8);
    DeleteObject(dib4);
    DeleteDC(hdc);
    pImageList_Destroy(himl);
}

static void test_copy(void)
{
    HIMAGELIST dst, src;
    BOOL ret;
    int count;

    dst = pImageList_Create(5, 11, ILC_COLOR, 1, 1);
    count = pImageList_GetImageCount(dst);
    ok(!count, "ImageList not empty.\n");
    src = createImageList(7, 13);
    count = pImageList_GetImageCount(src);
    ok(count > 2, "Tests need an ImageList with more than 2 images\n");

    /* ImageList_Copy() cannot copy between two ImageLists */
    ret = pImageList_Copy(dst, 0, src, 2, ILCF_MOVE);
    ok(!ret, "ImageList_Copy() should have returned FALSE\n");
    count = pImageList_GetImageCount(dst);
    ok(count == 0, "Expected no image in dst ImageList, got %d\n", count);

    pImageList_Destroy(dst);
    pImageList_Destroy(src);
}

static void test_loadimage(void)
{
    HIMAGELIST list;
    DWORD flags;

    list = pImageList_LoadImageW( hinst, MAKEINTRESOURCEW(IDB_BITMAP_128x15), 16, 1, CLR_DEFAULT,
                                 IMAGE_BITMAP, LR_CREATEDIBSECTION );
    ok( list != NULL, "got %p\n", list );
    flags = pImageList_GetFlags( list );
    ok( flags == (ILC_COLOR4 | ILC_MASK), "got %08x\n", flags );
    pImageList_Destroy( list );

    list = pImageList_LoadImageW( hinst, MAKEINTRESOURCEW(IDB_BITMAP_128x15), 16, 1, CLR_NONE,
                                 IMAGE_BITMAP, LR_CREATEDIBSECTION );
    ok( list != NULL, "got %p\n", list );
    flags = pImageList_GetFlags( list );
    ok( flags == ILC_COLOR4, "got %08x\n", flags );
    pImageList_Destroy( list );
}

static void test_IImageList_Clone(void)
{
    IImageList *imgl, *imgl2;
    HIMAGELIST himl;
    HRESULT hr;
    ULONG ref;

    himl = pImageList_Create(16, 16, ILC_COLOR16, 0, 3);
    imgl = (IImageList*)himl;

if (0)
{
    /* crashes on native */
    IImageList_Clone(imgl, &IID_IImageList, NULL);
}

    hr = IImageList_Clone(imgl, &IID_IImageList, (void**)&imgl2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ref = IImageList_Release(imgl2);
    ok(ref == 0, "got %u\n", ref);

    IImageList_Release(imgl);
}

static void test_IImageList_GetBkColor(void)
{
    IImageList *imgl;
    HIMAGELIST himl;
    COLORREF color;
    HRESULT hr;

    himl = pImageList_Create(16, 16, ILC_COLOR16, 0, 3);
    imgl = (IImageList*)himl;

if (0)
{
    /* crashes on native */
    IImageList_GetBkColor(imgl, NULL);
}

    hr = IImageList_GetBkColor(imgl, &color);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IImageList_Release(imgl);
}

static void test_IImageList_SetBkColor(void)
{
    IImageList *imgl;
    HIMAGELIST himl;
    COLORREF color;
    HRESULT hr;

    himl = pImageList_Create(16, 16, ILC_COLOR16, 0, 3);
    imgl = (IImageList*)himl;

if (0)
{
    /* crashes on native */
    IImageList_SetBkColor(imgl, RGB(0, 0, 0), NULL);
}

    hr = IImageList_SetBkColor(imgl, CLR_NONE, &color);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IImageList_SetBkColor(imgl, CLR_NONE, &color);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    color = 0xdeadbeef;
    hr = IImageList_GetBkColor(imgl, &color);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(color == CLR_NONE, "got %x\n", color);

    IImageList_Release(imgl);
}

static void test_IImageList_GetImageCount(void)
{
    IImageList *imgl;
    HIMAGELIST himl;
    int count;
    HRESULT hr;

    himl = pImageList_Create(16, 16, ILC_COLOR16, 0, 3);
    imgl = (IImageList*)himl;

if (0)
{
    /* crashes on native */
    IImageList_GetImageCount(imgl, NULL);
}

    count = -1;
    hr = IImageList_GetImageCount(imgl, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 0, "got %d\n", count);

    IImageList_Release(imgl);
}

static void test_IImageList_GetIconSize(void)
{
    IImageList *imgl;
    HIMAGELIST himl;
    int cx, cy;
    HRESULT hr;

    himl = pImageList_Create(16, 16, ILC_COLOR16, 0, 3);
    imgl = (IImageList*)himl;

    hr = IImageList_GetIconSize(imgl, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IImageList_GetIconSize(imgl, &cx, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IImageList_GetIconSize(imgl, NULL, &cy);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    IImageList_Release(imgl);
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
#define X2(f, ord) p##f = (void*)GetProcAddress(hComCtl32, (const char *)ord);
    X(ImageList_Create);
    X(ImageList_Destroy);
    X(ImageList_Add);
    X(ImageList_DrawIndirect);
    X(ImageList_SetImageCount);
    X(ImageList_SetImageCount);
    X2(ImageList_SetColorTable, 390);
    X(ImageList_GetFlags);
    X(ImageList_BeginDrag);
    X(ImageList_GetDragImage);
    X(ImageList_EndDrag);
    X(ImageList_GetImageCount);
    X(ImageList_SetDragCursorImage);
    X(ImageList_GetIconSize);
    X(ImageList_Remove);
    X(ImageList_ReplaceIcon);
    X(ImageList_Replace);
    X(ImageList_Merge);
    X(ImageList_GetImageInfo);
    X(ImageList_Write);
    X(ImageList_Read);
    X(ImageList_Copy);
    X(ImageList_LoadImageW);
    X(ImageList_CoCreateInstance);
    X(HIMAGELIST_QueryInterface);
    X(ImageList_Draw);
#undef X
#undef X2
}

START_TEST(imagelist)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    init_functions();

    hinst = GetModuleHandleA(NULL);

    test_create_destroy();
    test_begindrag();
    test_hotspot();
    test_add_remove();
    test_imagecount();
    test_DrawIndirect();
    test_merge();
    test_merge_colors();
    test_imagelist_storage();
    test_iconsize();
    test_color_table(ILC_COLOR4);
    test_color_table(ILC_COLOR8);
    test_copy();
    test_loadimage();

    /* Now perform v6 tests */
    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    init_functions();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* Do v6.0 tests */
    test_add_remove();
    test_imagecount();
    test_DrawIndirect();
    test_merge();
    test_imagelist_storage();
    test_iconsize();
    test_color_table(ILC_COLOR4);
    test_color_table(ILC_COLOR8);
    test_copy();
    test_loadimage();

    test_ImageList_DrawIndirect();
    test_shell_imagelist();
    test_iimagelist();

    test_IImageList_Add_Remove();
    test_IImageList_Get_SetImageCount();
    test_IImageList_Draw();
    test_IImageList_Merge();
    test_IImageList_Clone();
    test_IImageList_GetBkColor();
    test_IImageList_SetBkColor();
    test_IImageList_GetImageCount();
    test_IImageList_GetIconSize();

    CoUninitialize();

    unload_v6_module(ctx_cookie, hCtx);
}
