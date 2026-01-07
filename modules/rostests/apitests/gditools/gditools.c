

/* SDK/DDK/NDK Headers. */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winddi.h>
#include <winuser.h>
#include <prntfont.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* Public Win32K Headers */
#include <ntgdityp.h>
#include <ntgdi.h>
#include <ntgdihdl.h>

#include "gditools.h"

HBITMAP ghbmp1, ghbmp1_InvCol, ghbmp1_RB, ghbmp4, ghbmp8, ghbmp16, ghbmp24, ghbmp32;
HBITMAP ghbmpDIB1, ghbmpDIB1_InvCol, ghbmpDIB1_RB, ghbmpDIB4, ghbmpDIB8, ghbmpDIB16, ghbmpDIB24, ghbmpDIB32;
HDC ghdcDIB1, ghdcDIB1_InvCol, ghdcDIB1_RB, ghdcDIB4, ghdcDIB8, ghdcDIB16, ghdcDIB24, ghdcDIB32;
PVOID gpvDIB1, gpvDIB1_InvCol, gpvDIB1_RB, gpvDIB4, gpvDIB8, gpvDIB16, gpvDIB24, gpvDIB32;
ULONG (*gpDIB32)[8][8];
HPALETTE ghpal;
HDC ghdcInfo;

MYPAL gpal =
{
    0x300, 8,
    {
        { 0x10, 0x20, 0x30, PC_NOCOLLAPSE },
        { 0x20, 0x30, 0x40, PC_NOCOLLAPSE },
        { 0x30, 0x40, 0x50, PC_NOCOLLAPSE },
        { 0x40, 0x50, 0x60, PC_NOCOLLAPSE },
        { 0x50, 0x60, 0x70, PC_NOCOLLAPSE },
        { 0x60, 0x70, 0x80, PC_NOCOLLAPSE },
        { 0x70, 0x80, 0x90, PC_NOCOLLAPSE },
        { 0x80, 0x90, 0xA0, PC_NOCOLLAPSE },
    }
};

PENTRY
GdiQueryTable(
    VOID)
{
    PTEB pTeb = NtCurrentTeb();
    PPEB pPeb = pTeb->ProcessEnvironmentBlock;
    return pPeb->GdiSharedHandleTable;
}

BOOL
GdiIsHandleValid(
    _In_ HGDIOBJ hobj)
{
    PENTRY pentHmgr = GdiQueryTable();
    USHORT Index = (ULONG_PTR)hobj & 0xFFFF;
    PENTRY pentry = &pentHmgr[Index];

    if ((pentry->einfo.pobj == NULL) ||
        ((LONG_PTR)pentry->einfo.pobj > 0) ||
        (pentry->FullUnique != (USHORT)((ULONG_PTR)hobj >> 16)))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
GdiIsHandleValidEx(
    _In_ HGDIOBJ hobj,
    _In_ GDILOOBJTYPE ObjectType)
{
    PENTRY pentHmgr = GdiQueryTable();
    USHORT Index = (ULONG_PTR)hobj & 0xFFFF;
    PENTRY pentry = &pentHmgr[Index];

    if ((pentry->einfo.pobj == NULL) ||
        ((LONG_PTR)pentry->einfo.pobj > 0) ||
        (pentry->FullUnique != (USHORT)((ULONG_PTR)hobj >> 16)) ||
        (pentry->Objt != (UCHAR)(ObjectType >> 16)) ||
        (pentry->Flags != (UCHAR)(ObjectType >> 24)))
    {
        return FALSE;
    }

    return TRUE;
}

PVOID
GdiGetHandleUserData(
    _In_ HGDIOBJ hobj)
{
    PENTRY pentHmgr = GdiQueryTable();
    USHORT Index = (ULONG_PTR)hobj;
    PENTRY pentry = &pentHmgr[Index];

    if (!GdiIsHandleValid(hobj))
    {
        return NULL;
    }

    return pentry->pUser;
}

BOOL
ChangeScreenBpp(
    _In_ ULONG cBitsPixel,
    _Out_ PULONG pcOldBitsPixel)
{
    DEVMODEW dm = { .dmSize = sizeof(dm) };

    if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm))
    {
        printf("EnumDisplaySettingsW failed\n");
        return FALSE;
    }

    *pcOldBitsPixel = dm.dmBitsPerPel;

    if (dm.dmBitsPerPel != cBitsPixel)
    {
        dm.dmBitsPerPel = cBitsPixel;
        if (ChangeDisplaySettingsExW(NULL, &dm, NULL, CDS_UPDATEREGISTRY | CDS_GLOBAL, NULL) != DISP_CHANGE_SUCCESSFUL)
        {
            printf("Failed to change display settings to %lu bpp. Current bpp: %u\n", cBitsPixel, *pcOldBitsPixel);
            return FALSE;
        }
    }

    return TRUE;
}

#define FL_INVERT_COLORS 0x01
#define FL_RED_BLUE 0x02

static
BOOL
InitPerBitDepth(
    _In_ ULONG cBitsPerPixel,
    _In_ ULONG cx,
    _In_ ULONG cy,
    _Out_opt_ HBITMAP *phbmpDDB,
    _Out_ HDC *phdcDIB,
    _Out_ HBITMAP *phbmpDIB,
    _Out_ PVOID *ppvBits,
    _In_ ULONG flags)
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        ULONG bmiColors[256];
    } bmiBuffer;
    LPBITMAPINFO pbmi = (LPBITMAPINFO)&bmiBuffer;

    if (phbmpDDB != NULL)
    {
        /* Create a bitmap */
        *phbmpDDB = CreateBitmap(cx, cy, 1, cBitsPerPixel, NULL);
        if (*phbmpDDB == NULL)
        {
            printf("CreateBitmap failed %lu\n", cBitsPerPixel);
            return FALSE;
        }
    }

    /* Setup bitmap info */
    memset(&bmiBuffer, 0, sizeof(bmiBuffer));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = cx;
    pbmi->bmiHeader.biHeight = -(LONG)cy;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = cBitsPerPixel;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    if (cBitsPerPixel == 1)
    {
        if (flags & FL_RED_BLUE)
        {
            bmiBuffer.bmiColors[0] = RGB(0xFF, 0x00, 0x00);
            bmiBuffer.bmiColors[1] = RGB(0x00, 0x00, 0xFF);
        }
        else if (flags & FL_INVERT_COLORS)
        {
            bmiBuffer.bmiColors[0] = 0xFFFFFF;
            bmiBuffer.bmiColors[1] = 0;
        }
        else
        {
            bmiBuffer.bmiColors[0] = 0;
            bmiBuffer.bmiColors[1] = 0xFFFFFF;
        }
        pbmi->bmiHeader.biClrUsed = 2;
    }

    /* Create a compatible DC for the DIB */
    *phdcDIB = CreateCompatibleDC(0);
    if (*phdcDIB == NULL)
    {
        printf("CreateCompatibleDC failed %lu\n", cBitsPerPixel);
        return FALSE;
    }

    /* Create the DIB section with the same values */
    *phbmpDIB = CreateDIBSection(*phdcDIB, pbmi, DIB_RGB_COLORS, ppvBits, 0, 0 );
    if (*phbmpDIB == NULL)
    {
        printf("CreateDIBSection failed. %lu\n", cBitsPerPixel);
        return FALSE;
    }

    if (SelectObject(*phdcDIB, *phbmpDIB) == NULL)
    {
        printf("SelectObject failed for %lu bpp DIB\n", cBitsPerPixel);
        return FALSE;
    }

    return TRUE;
}

BOOL GdiToolsInit(void)
{
    /* Initialize a logical palette */
    ghpal = CreatePalette((LOGPALETTE*)&gpal);
    if (!ghpal)
    {
        printf("failed to create a palette\n");
        return FALSE;
    }

    if (!InitPerBitDepth(1, 9, 9, &ghbmp1, &ghdcDIB1, &ghbmpDIB1, &gpvDIB1, 0) ||
        !InitPerBitDepth(1, 9, 9, NULL, &ghdcDIB1_InvCol, &ghbmpDIB1_InvCol, &gpvDIB1_InvCol, FL_INVERT_COLORS) ||
        !InitPerBitDepth(1, 9, 9, NULL, &ghdcDIB1_RB, &ghbmpDIB1_RB, &gpvDIB1_RB, FL_RED_BLUE) ||
        !InitPerBitDepth(4, 5, 5, &ghbmp4, &ghdcDIB4, &ghbmpDIB4, &gpvDIB4, 0) ||
        !InitPerBitDepth(8, 5, 5, &ghbmp8, &ghdcDIB8, &ghbmpDIB8, &gpvDIB8, 0) ||
        !InitPerBitDepth(16, 8, 8, &ghbmp16, &ghdcDIB16, &ghbmpDIB16, &gpvDIB16, 0) ||
        !InitPerBitDepth(24, 8, 8, &ghbmp24, &ghdcDIB24, &ghbmpDIB24, &gpvDIB24, 0) ||
        !InitPerBitDepth(32, 8, 8, &ghbmp32, &ghdcDIB32, &ghbmpDIB32, &gpvDIB32, 0))
    {
        printf("failed to create objects\n");
        return FALSE;
    }

    gpDIB32 = gpvDIB32;

    /* Create an Info-DC */
    ghdcInfo = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
    if (!ghdcInfo)
    {
        printf("failed to create info DC\n");
        return FALSE;
    }

    return TRUE;
}
