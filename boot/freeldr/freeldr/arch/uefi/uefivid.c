/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>
#include "../vidfb.h"
#include <reactos/drivers/acpi/acpi.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

/* GLOBALS ********************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
EFI_GUID EfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID AppleGraphInfoProtocol = APPLE_GRAPH_INFO_PROTOCOL_GUID;

ULONG_PTR VramAddress;
ULONG VramSize;
PCM_FRAMEBUF_DEVICE_DATA FrameBufferData = NULL;

typedef struct _UEFI_BGRT_LOGO
{
    BOOLEAN Valid;
    BOOLEAN TopDown;
    PUCHAR PixelData;
    ULONG Width;
    ULONG Height;
    ULONG RowStride;
    USHORT BitsPerPixel;
    ULONG SourceX;
    ULONG SourceY;
    ULONG PositionX;
    ULONG PositionY;
    ULONG DrawWidth;
    ULONG DrawHeight;
} UEFI_BGRT_LOGO, *PUEFI_BGRT_LOGO;

static UEFI_BGRT_LOGO UefiBgrtLogo = {0};

#define BMP_SIGNATURE 0x4D42
#define BI_RGB        0

#define LOWEST_SUPPORTED_RES 1

#include <pshpack1.h>
typedef struct _BMP_FILE_HEADER
{
    USHORT Type;
    ULONG Size;
    USHORT Reserved1;
    USHORT Reserved2;
    ULONG BitsOffset;
} BMP_FILE_HEADER, *PBMP_FILE_HEADER;

typedef struct _BMP_INFO_HEADER
{
    ULONG Size;
    LONG Width;
    LONG Height;
    USHORT Planes;
    USHORT BitCount;
    ULONG Compression;
    ULONG SizeImage;
    LONG XPelsPerMeter;
    LONG YPelsPerMeter;
    ULONG ClrUsed;
    ULONG ClrImportant;
} BMP_INFO_HEADER, *PBMP_INFO_HEADER;
#include <poppack.h>

C_ASSERT(sizeof(BMP_FILE_HEADER) == 14);
C_ASSERT(sizeof(BMP_INFO_HEADER) == 40);

/* FUNCTIONS ******************************************************************/

/* EFI 1.x */
#ifdef EFI_UGA_DRAW_PROTOCOL_GUID

/* NOTE: EFI UGA does not support any other format than 32-bit xRGB, and
 * no direct access to the underlying hardware framebuffer is offered */
C_ASSERT(sizeof(EFI_UGA_PIXEL) == sizeof(ULONG));

#endif /* EFI */

/* UEFI support, see efi/GraphicsOutput.h */
#ifdef EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID // __GRAPHICS_OUTPUT_H__

C_ASSERT(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) == sizeof(ULONG));

/**
 * @brief   Maps UEFI GOP pixel format to pixel masks.
 * @see     EFI_PIXEL_BITMASK
 **/
static EFI_PIXEL_BITMASK EfiPixelMasks[] =
{ /* Red,        Green,      Blue,       Reserved */
    {0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000},   // PixelRedGreenBlueReserved8BitPerColor
    {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000},   // PixelBlueGreenRedReserved8BitPerColor
    {0,          0,          0,          0}             // PixelBitMask, PixelBltOnly, ...
};

#endif /* UEFI */

static
PRSDP_DESCRIPTOR
UefiFindAcpiBios(VOID)
{
    UINTN i;
    RSDP_DESCRIPTOR* Rsdp = NULL;
    EFI_GUID Acpi20Guid = EFI_ACPI_20_TABLE_GUID;
    EFI_GUID AcpiGuid = EFI_ACPI_TABLE_GUID;

    for (i = 0; i < GlobalSystemTable->NumberOfTableEntries; ++i)
    {
        EFI_GUID* VendorGuid = &GlobalSystemTable->ConfigurationTable[i].VendorGuid;

        if (!memcmp(VendorGuid, &Acpi20Guid, sizeof(*VendorGuid)) ||
            !memcmp(VendorGuid, &AcpiGuid, sizeof(*VendorGuid)))
        {
            Rsdp = (PRSDP_DESCRIPTOR)GlobalSystemTable->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    return Rsdp;
}

static
PDESCRIPTION_HEADER
UefiFindAcpiTable(
    _In_ ULONG Signature)
{
    UINTN Index, Count;
    PRSDP_DESCRIPTOR Rsdp;

    Rsdp = UefiFindAcpiBios();
    if (Rsdp == NULL)
        return NULL;

    if ((Rsdp->revision > 0) && (Rsdp->xsdt_physical_address != 0))
    {
        PXSDT Xsdt = (PXSDT)(ULONG_PTR)Rsdp->xsdt_physical_address;

        if ((Xsdt != NULL) && (Xsdt->Header.Length >= sizeof(Xsdt->Header)))
        {
            Count = (Xsdt->Header.Length - sizeof(Xsdt->Header)) / sizeof(Xsdt->Tables[0]);
            for (Index = 0; Index < Count; ++Index)
            {
                PDESCRIPTION_HEADER Header =
                    (PDESCRIPTION_HEADER)(ULONG_PTR)Xsdt->Tables[Index].QuadPart;

                if ((Header != NULL) && (Header->Signature == Signature))
                    return Header;
            }
        }
    }

    if (Rsdp->rsdt_physical_address != 0)
    {
        PRSDT Rsdt = (PRSDT)(ULONG_PTR)Rsdp->rsdt_physical_address;

        if ((Rsdt != NULL) && (Rsdt->Header.Length >= sizeof(Rsdt->Header)))
        {
            Count = (Rsdt->Header.Length - sizeof(Rsdt->Header)) / sizeof(Rsdt->Tables[0]);
            for (Index = 0; Index < Count; ++Index)
            {
                PDESCRIPTION_HEADER Header =
                    (PDESCRIPTION_HEADER)(ULONG_PTR)Rsdt->Tables[Index];

                if ((Header != NULL) && (Header->Signature == Signature))
                    return Header;
            }
        }
    }

    return NULL;
}

static
BOOLEAN
UefiCalculateBmpRowStride(
    _In_ ULONG Width,
    _In_ USHORT BitsPerPixel,
    _Out_ PULONG RowStride)
{
    ULONGLONG BitsPerRow, Stride;

    BitsPerRow = (ULONGLONG)Width * BitsPerPixel;
    Stride = ((BitsPerRow + 31) / 32) * sizeof(ULONG);
    if (Stride > MAXULONG)
        return FALSE;

    *RowStride = (ULONG)Stride;
    return TRUE;
}

VOID
UefiPrepareLogoAxis(
    _In_ ULONG BitmapSize,
    _In_ ULONG ScreenSize,
    _Out_ PULONG SourceOffset,
    _Out_ PULONG DestinationOffset,
    _Out_ PULONG DrawSize)
{
    if (BitmapSize >= ScreenSize)
    {
        *SourceOffset = (BitmapSize - ScreenSize) / 2;
        *DestinationOffset = 0;
        *DrawSize = ScreenSize;
        return;
    }

    *SourceOffset = 0;
    *DrawSize = BitmapSize;
    *DestinationOffset = (ScreenSize - BitmapSize) / 2;
}

static
VOID
UefiInitializeBgrtLogo(VOID)
{
    PBGRT_TABLE BgrtTable;
    PBMP_FILE_HEADER FileHeader;
    PBMP_INFO_HEADER InfoHeader;
    ULONG RowStride, HeaderSize;
    ULONG Width, Height;
    ULONGLONG ImageSize;
    LONGLONG SignedHeight;

    RtlZeroMemory(&UefiBgrtLogo, sizeof(UefiBgrtLogo));

    if ((FrameBufferData == NULL) ||
        (FrameBufferData->BitsPerPixel != 32) ||
        (FrameBufferData->ScreenWidth == 0) ||
        (FrameBufferData->ScreenHeight == 0))
    {
        return;
    }

    BgrtTable = (PBGRT_TABLE)UefiFindAcpiTable(BGRT_SIGNATURE);
    if (BgrtTable == NULL)
    {
        TRACE("No BGRT table found\n");
        return;
    }

    if (BgrtTable->Header.Length < sizeof(*BgrtTable))
    {
        WARN("BGRT table is too small (%lu)\n", BgrtTable->Header.Length);
        return;
    }

    if ((BgrtTable->Status & BGRT_STATUS_IMAGE_VALID) == 0)
    {
        TRACE("BGRT image is not marked valid\n");
        return;
    }

    if (BgrtTable->ImageType != BgrtImageTypeBitmap)
    {
        WARN("Unsupported BGRT image type %u\n", BgrtTable->ImageType);
        return;
    }

    if ((BgrtTable->LogoAddress == 0) ||
        (BgrtTable->LogoAddress != (ULONGLONG)(ULONG_PTR)BgrtTable->LogoAddress))
    {
        WARN("Unsupported BGRT logo address 0x%llx\n", BgrtTable->LogoAddress);
        return;
    }

    FileHeader = (PBMP_FILE_HEADER)(ULONG_PTR)BgrtTable->LogoAddress;
    if (FileHeader->Type != BMP_SIGNATURE)
    {
        WARN("BGRT logo does not contain a BMP signature\n");
        return;
    }

    InfoHeader = (PBMP_INFO_HEADER)(FileHeader + 1);
    if (InfoHeader->Size < sizeof(*InfoHeader))
    {
        WARN("Unsupported BMP info header size %lu\n", InfoHeader->Size);
        return;
    }

    if ((InfoHeader->Width <= 0) || (InfoHeader->Height == 0))
    {
        WARN("Unsupported BMP dimensions %ld x %ld\n",
             InfoHeader->Width, InfoHeader->Height);
        return;
    }

    if (InfoHeader->Planes != 1)
    {
        WARN("Unsupported BMP plane count %u\n", InfoHeader->Planes);
        return;
    }

    if ((InfoHeader->BitCount != 24) && (InfoHeader->BitCount != 32))
    {
        WARN("Unsupported BMP bit depth %u\n", InfoHeader->BitCount);
        return;
    }

    if (InfoHeader->Compression != BI_RGB)
    {
        WARN("Unsupported BMP compression %lu\n", InfoHeader->Compression);
        return;
    }

    Width = (ULONG)InfoHeader->Width;
    SignedHeight = (LONGLONG)InfoHeader->Height;
    Height = (ULONG)(SignedHeight < 0 ? -SignedHeight : SignedHeight);

    if (!UefiCalculateBmpRowStride(Width, InfoHeader->BitCount, &RowStride))
    {
        WARN("BMP row stride overflows\n");
        return;
    }

    if (InfoHeader->Size > MAXULONG - sizeof(*FileHeader))
    {
        WARN("BMP header size overflows\n");
        return;
    }

    HeaderSize = sizeof(*FileHeader) + InfoHeader->Size;
    ImageSize = (ULONGLONG)RowStride * Height;

    if ((FileHeader->BitsOffset < HeaderSize) ||
        (FileHeader->Size < FileHeader->BitsOffset) ||
        (ImageSize > (ULONGLONG)FileHeader->Size - FileHeader->BitsOffset))
    {
        WARN("BGRT BMP image bounds are invalid\n");
        return;
    }

    UefiBgrtLogo.Valid = TRUE;
    UefiBgrtLogo.TopDown = (SignedHeight < 0);
    UefiBgrtLogo.PixelData = (PUCHAR)FileHeader + FileHeader->BitsOffset;
    UefiBgrtLogo.Width = Width;
    UefiBgrtLogo.Height = Height;
    UefiBgrtLogo.RowStride = RowStride;
    UefiBgrtLogo.BitsPerPixel = InfoHeader->BitCount;

    UefiPrepareLogoAxis(UefiBgrtLogo.Width,
                        FrameBufferData->ScreenWidth,
                        &UefiBgrtLogo.SourceX,
                        &UefiBgrtLogo.PositionX,
                        &UefiBgrtLogo.DrawWidth);

    UefiPrepareLogoAxis(UefiBgrtLogo.Height,
                        FrameBufferData->ScreenHeight,
                        &UefiBgrtLogo.SourceY,
                        &UefiBgrtLogo.PositionY,
                        &UefiBgrtLogo.DrawHeight);

    TRACE("BGRT logo ready: %lux%lu @ (%lu,%lu), crop (%lu,%lu) -> %lux%lu\n",
          UefiBgrtLogo.Width, UefiBgrtLogo.Height,
          UefiBgrtLogo.PositionX, UefiBgrtLogo.PositionY,
          UefiBgrtLogo.SourceX, UefiBgrtLogo.SourceY,
          UefiBgrtLogo.DrawWidth, UefiBgrtLogo.DrawHeight);
}

static
VOID
UefiDrawBgrtLogo(VOID)
{
    PUCHAR VramBase;
    ULONG BytesPerPixel, Pitch;
    ULONG Row, Col;

    if (!UefiBgrtLogo.Valid || !UiProgressBar.Show)
        return;

    if ((UefiBgrtLogo.DrawWidth == 0) || (UefiBgrtLogo.DrawHeight == 0))
        return;

    if ((FrameBufferData == NULL) || (FrameBufferData->BitsPerPixel != 32))
        return;

    BytesPerPixel = FrameBufferData->BitsPerPixel / 8;
    Pitch = FrameBufferData->PixelsPerScanLine * BytesPerPixel;
    VramBase = (PUCHAR)VramAddress;

    for (Row = 0; Row < UefiBgrtLogo.DrawHeight; ++Row)
    {
        ULONG SourceRowIndex = UefiBgrtLogo.SourceY + Row;
        PUCHAR SourceRow;
        PULONG DestinationRow;

        if (!UefiBgrtLogo.TopDown)
            SourceRowIndex = UefiBgrtLogo.Height - 1 - SourceRowIndex;

        SourceRow = UefiBgrtLogo.PixelData + SourceRowIndex * UefiBgrtLogo.RowStride;
        SourceRow += UefiBgrtLogo.SourceX * (UefiBgrtLogo.BitsPerPixel / 8);

        DestinationRow = (PULONG)(VramBase +
                                  (UefiBgrtLogo.PositionY + Row) * Pitch +
                                  UefiBgrtLogo.PositionX * BytesPerPixel);

        if (UefiBgrtLogo.BitsPerPixel == 24)
        {
            for (Col = 0; Col < UefiBgrtLogo.DrawWidth; ++Col)
            {
                DestinationRow[Col] = 0xFF000000 |
                                      (SourceRow[2] << 16) |
                                      (SourceRow[1] << 8) |
                                      SourceRow[0];
                SourceRow += 3;
            }
        }
        else
        {
            for (Col = 0; Col < UefiBgrtLogo.DrawWidth; ++Col)
            {
                DestinationRow[Col] = 0xFF000000 |
                                      (SourceRow[2] << 16) |
                                      (SourceRow[1] << 8) |
                                      SourceRow[0];
                SourceRow += 4;
            }
        }
    }
}

static
EFI_STATUS
UefiInitializeGop(VOID)
{
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;

    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK* pPixelBitmask;
    ULONG BitsPerPixel;

    Status = GlobalSystemTable->BootServices->LocateProtocol(&EfiGraphicsOutputProtocol, 0, (void**)&gop);
    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to find GOP with status %d\n", Status);
        return Status;
    }

    /* We don't need high resolutions for freeldr */
    gop->SetMode(gop, LOWEST_SUPPORTED_RES);

    /* Physical format of the pixel */
    PixelFormat = gop->Mode->Info->PixelFormat;
    switch (PixelFormat)
    {
        case PixelRedGreenBlueReserved8BitPerColor:
        case PixelBlueGreenRedReserved8BitPerColor:
        {
            pPixelBitmask = &EfiPixelMasks[PixelFormat];
            BitsPerPixel = RTL_BITS_OF(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
            break;
        }

        case PixelBitMask:
        {
            /*
             * When the GOP pixel format is given by PixelBitMask, the pixel
             * element size _may be_ different from 4 bytes.
             * See UEFI Spec Rev.2.10 Section 12.9 "Graphics Output Protocol":
             * example code "GetPixelElementSize()" function.
             */
            pPixelBitmask = &gop->Mode->Info->PixelInformation;
            BitsPerPixel =
                PixelBitmasksToBpp(pPixelBitmask->RedMask,
                                   pPixelBitmask->GreenMask,
                                   pPixelBitmask->BlueMask,
                                   pPixelBitmask->ReservedMask);
            break;
        }

        case PixelBltOnly:
        default:
        {
            ERR("Unsupported UEFI GOP format %lu\n", PixelFormat);
            pPixelBitmask = NULL;
            BitsPerPixel = 0;
            break;
        }
    }

    VramAddress = (ULONG_PTR)gop->Mode->FrameBufferBase;
    VramSize = gop->Mode->FrameBufferSize;
    if (!VidFbInitializeVideo(&FrameBufferData,
                              VramAddress,
                              VramSize,
                              gop->Mode->Info->HorizontalResolution,
                              gop->Mode->Info->VerticalResolution,
                              gop->Mode->Info->PixelsPerScanLine,
                              BitsPerPixel,
                              (PPIXEL_BITMASK)pPixelBitmask))
    {
        ERR("Couldn't initialize video framebuffer\n");
        Status = EFI_UNSUPPORTED;
    }
    else
    {
        UefiInitializeBgrtLogo();
    }
    return Status;
}

static
EFI_STATUS
UefiInitializeAppleGraphics(VOID)
{
    EFI_STATUS Status;
    APPLE_GRAPH_INFO_PROTOCOL *AppleGraph = NULL;
    EFI_PIXEL_BITMASK* pPixelBitmask;

    UINT64 BaseAddress, FrameBufferSize;
    UINT32 BytesPerRow, Width, Height, Depth;

    Status = GlobalSystemTable->BootServices->LocateProtocol(&AppleGraphInfoProtocol, 0, (VOID**)&AppleGraph);
    if (Status != EFI_SUCCESS)
    {
        ERR("Failed to find Apple Graphics Info with status %d\n", Status);
        return Status;
    }

    Status = AppleGraph->GetInfo(AppleGraph,
                                 &BaseAddress,
                                 &FrameBufferSize,
                                 &BytesPerRow,
                                 &Width,
                                 &Height,
                                 &Depth);

    if (Status != EFI_SUCCESS)
    {
        ERR("Failed to get graphics info from Apple Scren Info: %d\n", Status);
        return Status;
    }

    /* All devices requiring Apple Graphics Info use PixelBlueGreenRedReserved8BitPerColor. */
    pPixelBitmask = &EfiPixelMasks[PixelBlueGreenRedReserved8BitPerColor];

    VramAddress = (ULONG_PTR)BaseAddress;
    VramSize = (ULONG)FrameBufferSize;
    if (!VidFbInitializeVideo(&FrameBufferData,
                              VramAddress,
                              VramSize,
                              Width,
                              Height,
                              (BytesPerRow / 4),
                              Depth,
                              (PPIXEL_BITMASK)pPixelBitmask))
    {
        ERR("Couldn't initialize video framebuffer\n");
        Status = EFI_UNSUPPORTED;
    }
    else
    {
        UefiInitializeBgrtLogo();
    }

    return Status;
}

EFI_STATUS
UefiInitializeVideo(VOID)
{
    EFI_STATUS Status;

    /* First, try GOP */
    Status = UefiInitializeGop();
    if (Status == EFI_SUCCESS)
        return Status;

    /* Try Apple Graphics Info if that fails */
    TRACE("Failed to detect GOP, trying Apple Graphics Info\n");
    Status = UefiInitializeAppleGraphics();
    if (Status == EFI_SUCCESS)
        return Status;

    /* We didn't find GOP or Apple Graphics Info, probably a UGA-only system */
    ERR("Cannot find framebuffer!\n");
    return Status;
}


VOID
UefiVideoClearScreen(UCHAR Attr)
{
    FbConsClearScreen(Attr);
}

VOID
UefiVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    FbConsPutChar(Ch, Attr, X, Y);
}

VOID
UefiVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    FbConsGetDisplaySize(Width, Height, Depth);
}

VIDEODISPLAYMODE
UefiVideoSetDisplayMode(PCSTR DisplayMode, BOOLEAN Init)
{
    /* We only have one mode, semi-text */
    return VideoTextMode;
}

ULONG
UefiVideoGetBufferSize(VOID)
{
    return FbConsGetBufferSize();
}

VOID
UefiVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    FbConsCopyOffScreenBufferToVRAM(Buffer);
    UefiDrawBgrtLogo();
}

VOID
UefiVideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    /* We don't have a cursor yet */
}

VOID
UefiVideoHideShowTextCursor(BOOLEAN Show)
{
    /* We don't have a cursor yet */
}

BOOLEAN
UefiVideoIsPaletteFixed(VOID)
{
    return 0;
}

VOID
UefiVideoSetPaletteColor(UCHAR Color, UCHAR Red,
                         UCHAR Green, UCHAR Blue)
{
    /* Not supported */
}

VOID
UefiVideoGetPaletteColor(UCHAR Color, UCHAR* Red,
                         UCHAR* Green, UCHAR* Blue)
{
    /* Not supported */
}
