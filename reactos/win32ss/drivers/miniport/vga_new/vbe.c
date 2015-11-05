/*
 * PROJECT:         ReactOS VGA Miniport Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            win32ss/drivers/miniport/vga_new/vbe.c
 * PURPOSE:         Main VESA VBE 1.02+ SVGA Miniport Handling Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "vga.h"

/* GLOBALS ********************************************************************/

static const CHAR Nv11Board[] = "NV11 (GeForce2) Board";
static const CHAR Nv11Chip[] = "Chip Rev B2";
static const CHAR Nv11Vendor[] = "NVidia Corporation";
static const CHAR IntelBrookdale[] = "Brookdale-G Graphics Controller";
static const PCHAR BrokenVesaBiosList[] =
{
    "SiS 5597",
    "MGA-G100",
    "3Dfx Banshee",
    "Voodoo3 2000 LC ",
    "Voodoo3 3000 LC ",
    "Voodoo4 4500 ",
    "ArtX I",
    "ATI S1-370TL"
};

BOOLEAN g_bIntelBrookdaleBIOS;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
IsVesaBiosOk(IN PVIDEO_PORT_INT10_INTERFACE Interface,
             IN ULONG OemRevision,
             IN PCHAR Vendor,
             IN PCHAR Product,
             IN PCHAR Revision)
{
    ULONG i;
    CHAR Version[21];

    /* If the broken VESA bios found, turn VESA off */
    VideoDebugPrint((0, "Vendor: %s Product: %s Revision: %s (%lx)\n", Vendor, Product, Revision, OemRevision));
    for (i = 0; i < (sizeof(BrokenVesaBiosList) / sizeof(PCHAR)); i++)
    {
        if (!strncmp(Product, BrokenVesaBiosList[i], strlen(BrokenVesaBiosList[i]))) return FALSE;
    }

    /* For Brookdale-G (Intel), special hack used */
    g_bIntelBrookdaleBIOS = !strncmp(Product, IntelBrookdale, sizeof(IntelBrookdale) - 1);

    /* For NVIDIA make sure */
    if (!(strncmp(Vendor, Nv11Vendor, sizeof(Nv11Vendor) - 1)) &&
        !(strncmp(Product, Nv11Board, sizeof(Nv11Board) - 1)) &&
        !(strncmp(Revision, Nv11Chip, sizeof(Nv11Chip) - 1)) &&
        (OemRevision == 0x311))
    {
        /* Read version */
        if (Interface->Int10ReadMemory(Interface->Context,
                                       0xC000,
                                       345,
                                       Version,
                                       sizeof(Version))) return FALSE;
        if (!strncmp(Version, "Version 3.11.01.24N16", sizeof(Version))) return FALSE;
    }

    /* VESA ok */
    VideoDebugPrint((0, "Vesa ok\n"));
    return TRUE;
}

BOOLEAN
NTAPI
ValidateVbeInfo(IN PHW_DEVICE_EXTENSION VgaExtension,
                IN PVBE_INFO VbeInfo)
{
    BOOLEAN VesaBiosOk;
    PVOID Context;
    CHAR ProductRevision[80];
    CHAR OemString[80];
    CHAR ProductName[80];
    CHAR VendorName[80];
    VP_STATUS Status;

    /* Set default */
    VesaBiosOk = FALSE;
    Context = VgaExtension->Int10Interface.Context;

    /* Check magic and version */
    if (VbeInfo->Info.Signature != VESA_MAGIC) return VesaBiosOk;
    if (VbeInfo->Info.Version < 0x102) return VesaBiosOk;

    /* Read strings */
    Status = VgaExtension->Int10Interface.Int10ReadMemory(Context,
                                                          HIWORD(VbeInfo->Info.OemStringPtr),
                                                          LOWORD(VbeInfo->Info.OemStringPtr),
                                                          OemString,
                                                          sizeof(OemString));
    if (Status != NO_ERROR) return VesaBiosOk;
    Status = VgaExtension->Int10Interface.Int10ReadMemory(Context,
                                                          HIWORD(VbeInfo->Info.OemVendorNamePtr),
                                                          LOWORD(VbeInfo->Info.OemVendorNamePtr),
                                                          VendorName,
                                                          sizeof(VendorName));
    if (Status != NO_ERROR) return VesaBiosOk;
    Status = VgaExtension->Int10Interface.Int10ReadMemory(Context,
                                                          HIWORD(VbeInfo->Info.OemProductNamePtr),
                                                          LOWORD(VbeInfo->Info.OemProductNamePtr),
                                                          ProductName,
                                                          sizeof(ProductName));
    if (Status != NO_ERROR) return VesaBiosOk;
    Status = VgaExtension->Int10Interface.Int10ReadMemory(Context,
                                                          HIWORD(VbeInfo->Info.OemProductRevPtr),
                                                          LOWORD(VbeInfo->Info.OemProductRevPtr),
                                                          ProductRevision,
                                                          sizeof(ProductRevision));
    if (Status != NO_ERROR) return VesaBiosOk;

    /* Null-terminate strings */
    VendorName[sizeof(OemString) - 1] = ANSI_NULL;
    ProductName[sizeof(OemString) - 1] = ANSI_NULL;
    ProductRevision[sizeof(OemString) - 1] = ANSI_NULL;
    OemString[sizeof(OemString) - 1] = ANSI_NULL;

    /* Check for known bad BIOS */
    VesaBiosOk = IsVesaBiosOk(&VgaExtension->Int10Interface,
                              VbeInfo->Info.OemSoftwareRevision,
                              VendorName,
                              ProductName,
                              ProductRevision);
    VgaExtension->VesaBiosOk = VesaBiosOk;
    return VesaBiosOk;
}

VP_STATUS
NTAPI
VbeSetColorLookup(IN PHW_DEVICE_EXTENSION VgaExtension,
                  IN PVIDEO_CLUT ClutBuffer)
{
    PVBE_COLOR_REGISTER VesaClut;
    INT10_BIOS_ARGUMENTS BiosArguments;
    PVOID Context;
    ULONG Entries;
    ULONG BufferSize = 4 * 1024;
    USHORT TrampolineMemorySegment, TrampolineMemoryOffset;
    VP_STATUS Status;
    USHORT i;
    PVIDEOMODE CurrentMode = VgaExtension->CurrentMode;

    Entries = ClutBuffer->NumEntries;
    
    VideoDebugPrint((0, "Setting %lu entries.\n", Entries));
    
    /* 
     * For Vga compatible modes, write them directly.
     * Otherwise, the LGPL VGABIOS (used in bochs) fails!
     * It is also said that this way is faster.
     */
    if(!CurrentMode->NonVgaMode)
    {
        for (i=ClutBuffer->FirstEntry; i<ClutBuffer->FirstEntry + Entries; i++)
        {
            VideoPortWritePortUchar((PUCHAR)0x03c8, i);
            VideoPortWritePortUchar((PUCHAR)0x03c9, ClutBuffer->LookupTable[i].RgbArray.Red);
            VideoPortWritePortUchar((PUCHAR)0x03c9, ClutBuffer->LookupTable[i].RgbArray.Green);
            VideoPortWritePortUchar((PUCHAR)0x03c9, ClutBuffer->LookupTable[i].RgbArray.Blue);
        }
        return NO_ERROR;
    }

    /* Allocate INT10 context/buffer */
    VesaClut = VideoPortAllocatePool(VgaExtension, 1, sizeof(ULONG) * Entries, ' agV');
    if (!VesaClut) return ERROR_INVALID_PARAMETER;
    if (!VgaExtension->Int10Interface.Size) return ERROR_INVALID_PARAMETER;
    Context = VgaExtension->Int10Interface.Context;
    Status = VgaExtension->Int10Interface.Int10AllocateBuffer(Context,
                                                            &TrampolineMemorySegment,
                                                            &TrampolineMemoryOffset,
                                                            &BufferSize);
    if (Status != NO_ERROR) return ERROR_INVALID_PARAMETER;

    /* VESA has color registers backward! */
    for (i = 0; i < Entries; i++)
    {
        VesaClut[i].Blue = ClutBuffer->LookupTable[i].RgbArray.Blue;
        VesaClut[i].Green = ClutBuffer->LookupTable[i].RgbArray.Green;
        VesaClut[i].Red = ClutBuffer->LookupTable[i].RgbArray.Red;
        VesaClut[i].Pad = 0;
    }
    Status = VgaExtension->Int10Interface.Int10WriteMemory(Context,
                                                         TrampolineMemorySegment,
                                                         TrampolineMemoryOffset,
                                                         VesaClut,
                                                         Entries * sizeof(ULONG));
    if (Status != NO_ERROR) return ERROR_INVALID_PARAMETER;

    /* Write the palette */
    VideoPortZeroMemory(&BiosArguments, sizeof(BiosArguments));
    BiosArguments.Ebx = 0;
    BiosArguments.Ecx = Entries;
    BiosArguments.Edx = ClutBuffer->FirstEntry;
    BiosArguments.Edi = TrampolineMemoryOffset;
    BiosArguments.SegEs = TrampolineMemorySegment;
    BiosArguments.Eax = VBE_SET_GET_PALETTE_DATA;
    Status = VgaExtension->Int10Interface.Int10CallBios(Context, &BiosArguments);
    if (Status != NO_ERROR) return ERROR_INVALID_PARAMETER;
    VideoPortFreePool(VgaExtension, VesaClut);
    VideoDebugPrint((Error, "VBE Status: %lx\n", BiosArguments.Eax));
    if (VBE_GETRETURNCODE(BiosArguments.Eax) == VBE_SUCCESS)
        return NO_ERROR;
    return ERROR_INVALID_PARAMETER;
}

/* EOF */
