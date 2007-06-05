/* $Id: xboxvideo.c 25800 2007-02-14 20:30:33Z ion $
 *
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Programmer: Aleksey Bragin <aleksey reactos org>
 */

#include <freeldr.h>

volatile char *video_mem = 0;

VOID OlpcVideoInit()
{
    PCI_COMMON_CONFIG PciConfig;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG DeviceNumber;
    ULONG FunctionNumber;
    ULONG Size;

    /* Enumerate devices on the PCI bus */
    SlotNumber.u.AsULONG = 0;
    for (DeviceNumber = 0; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
        SlotNumber.u.bits.DeviceNumber = DeviceNumber;
        for (FunctionNumber = 0; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
        {
            SlotNumber.u.bits.FunctionNumber = FunctionNumber;

            RtlZeroMemory(&PciConfig,
                sizeof(PCI_COMMON_CONFIG));

            /*Size = HalGetBusData(PCIConfiguration,
                DeviceExtension->BusNumber,
                SlotNumber.u.AsULONG,
                &PciConfig,
                PCI_COMMON_HDR_LENGTH);*/
            ofwprintf("Size %lu\n", Size);
            if (Size < PCI_COMMON_HDR_LENGTH)
            {
                if (FunctionNumber == 0)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }

            ofwprintf("Bus %1lu  Device %2lu  Func %1lu  VenID 0x%04hx  DevID 0x%04hx\n",
                0,
                DeviceNumber,
                FunctionNumber,
                PciConfig.VendorID,
                PciConfig.DeviceID);

            /* Skip to next device if the current one is not a multifunction device */
            if ((FunctionNumber == 0) &&
                ((PciConfig.HeaderType & 0x80) == 0))
            {
                break;
            }
        }
    }
}

void OlpcVideoClearScreen( UCHAR Attr )
{
}

void OlpcVideoGetDisplaySize( PULONG Width, PULONG Height, PULONG Depth )
{
    *Width = 80;
    *Height = 25;
    *Depth = 16;
}

VIDEODISPLAYMODE OlpcVideoSetDisplayMode( char *DisplayMode, BOOLEAN Init )
{
    //printf( "DisplayMode: %s %s\n", DisplayMode, Init ? "true" : "false" );
    if( Init && !video_mem )
	{
	video_mem = MmAllocateMemory( OlpcVideoGetBufferSize() );
    }
    return VideoTextMode;
}

ULONG OlpcVideoGetBufferSize()
{
    ULONG Width, Height, Depth;
    MachVideoGetDisplaySize( &Width, &Height, &Depth );
    return Width * Height * Depth / 8;
}

void OlpcVideoHideShowTextCursor( BOOLEAN Show )
{
    ofwprintf("HideShowTextCursor(%s)\n", Show ? "true" : "false");
}

VOID OlpcVideoPutChar( int Ch, UCHAR Attr, unsigned X, unsigned Y )
{
    ofwprintf( "\033[%d;%dH%c", Y, X, Ch );
}

VOID OlpcVideoCopyOffScreenBufferToVRAM( PVOID Buffer )
{
	int i,j;
    ULONG w,h,d;
    PCHAR ChBuf = Buffer;
    int offset = 0;

    MachVideoGetDisplaySize( &w, &h, &d );

    for( i = 0; i < h; i++ ) {
	for( j = 0; j < w; j++ ) {
	    offset = (j * 2) + (i * w * 2);
	    if( ChBuf[offset] != video_mem[offset] ) {
		video_mem[offset] = ChBuf[offset];
		MachVideoPutChar(ChBuf[offset],0,j+1,i+1);
	    }
	}
    }
}

BOOLEAN OlpcVideoIsPaletteFixed()
{
    return FALSE;
}

VOID OlpcVideoSetPaletteColor( UCHAR Color, 
                              UCHAR Red, UCHAR Green, UCHAR Blue )
{
    //ofwprintf( "SetPaletteColor(%x,%x,%x,%x)\n", Color, Red, Green, Blue );
}

VOID OlpcVideoGetPaletteColor( UCHAR Color, 
                              UCHAR *Red, UCHAR *Green, UCHAR *Blue )
{
    //ofwprintf( "GetPaletteColor(%x)\n", Color);
}

VOID OlpcVideoSync()
{
    //ofwprintf( "Sync\n" );
}

VOID OlpcVideoPrepareForReactOS(IN BOOLEAN Setup)
{
}
