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

extern int chosen_package;
extern FILE *stdout_handle;

int get_int_prop(phandle node, char *key);
int decode_int(UCHAR *p);

char *Framebuffer = NULL;
int Width, Height, Stride, Depth;


VOID
NTAPI
HalReadPCIConfig(IN USHORT BusNumber, IN PCI_SLOT_NUMBER Slot, IN PVOID Buffer, IN ULONG Offset, IN ULONG Length);

ULONG
PciEnableResources(IN USHORT BusNumber,
                   IN PCI_SLOT_NUMBER Slot,
                   ULONG Mask);

/*
 * Access to machine-specific registers (available on 586 and better only)
 * Note: the rd* operations modify the parameters directly (without using
 * pointer indirection), this allows gcc to optimize better
 */

#define rdmsr(msr,val1,val2) \
	__asm__ __volatile__("rdmsr" \
			  : "=a" (val1), "=d" (val2) \
			  : "c" (msr))

#define wrmsr(msr,val1,val2) \
	__asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))

#define MSR_LX_GLIU0_P2D_RO0 0x10000029

#if 0
VOID OlpcVideoInit()
{
    PCI_COMMON_CONFIG PciConfig;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG DeviceNumber;
    ULONG FunctionNumber;

    /* Enumerate devices on the PCI bus */
    SlotNumber.u.AsULONG = 0;
    for (DeviceNumber = 0; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
        SlotNumber.u.bits.DeviceNumber = DeviceNumber;
        for (FunctionNumber = 0; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
        {
            SlotNumber.u.bits.FunctionNumber = FunctionNumber;

            RtlZeroMemory(&PciConfig, sizeof(PCI_COMMON_CONFIG));

            HalReadPCIConfig(0,
                SlotNumber,
                &PciConfig,
                0,
                PCI_COMMON_HDR_LENGTH);

            if (PciConfig.VendorID == PCI_INVALID_VENDORID)
                continue;

            ofwprintf("Bus %d  Device %d  Func %d  VenID 0x%x  DevID 0x%x\n",
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

VOID OlpcVideoInit()
{
    PCI_COMMON_CONFIG PciConfig;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG FrameBuffer;
    ULONG FrameBufferSize;
    int i;

    /* Geode LX is located at Bus=0 Device=15 Func=0 place */
    SlotNumber.u.AsULONG = 0;
    SlotNumber.u.bits.DeviceNumber = 15;

    /* Enable all four memory resources */
    PciEnableResources(0, SlotNumber, 1 + 2 + 4 + 8);

    RtlZeroMemory(&PciConfig, sizeof(PCI_COMMON_CONFIG));

    HalReadPCIConfig(0,
                     SlotNumber,
                     &PciConfig,
                     0,
                     PCI_COMMON_HDR_LENGTH);

    /* Check if it's really Geode LX */
    if (PciConfig.VendorID != 0x1022 && PciConfig.DeviceID != 0x208F)
        return;

    /* Obtain framebuffer address from it */
    FrameBuffer = PciConfig.u.type0.BaseAddresses[0];
    FrameBufferSize = lx_framebuffer_size();

    ofwprintf("Frame buffer found at %x, size %x\n", FrameBuffer, FrameBufferSize);
    for (i=0; i<PCI_TYPE0_ADDRESSES; i++)
    {
        HalReadPCIConfig(0,
                         SlotNumber,
                         &FrameBuffer,
                         0x10 + i*4,
                         sizeof(ULONG));

        ofwprintf("Addr %x\n", FrameBuffer);
    }
}
#endif

unsigned int lx_framebuffer_size(void)
{
    unsigned int val;

    if (/*machine_is_olpc() && !olpc_has_vsa()*/TRUE)
    {
        unsigned int hi,lo;
        rdmsr(MSR_LX_GLIU0_P2D_RO0, lo, hi);

        /* Top page number */
        val = ((hi & 0xff) << 12) | ((lo & 0xfff00000) >> 20);
        val -= (lo & 0x000fffff); /* Subtract bottom page number */
        val += 1; /* Adjust page count */
        return (val << 12);
    }
}


VOID OlpcVideoInit()
{
    int OutDevice;
    unsigned int Node;
    char Type[16];
    UCHAR Buffer[4];

    /* Get the stdout device's handle */
    OutDevice = get_int_prop(chosen_package, "stdout");
    Node = OFInstanceToPackage(OutDevice);

    OFGetprop(Node, "device_type", Type, sizeof(Type));

    if (strcmp(Type, "display") != 0)
        return;

    OFGetprop(Node, "depth", (char *)&Buffer, sizeof(int));
    Depth = decode_int(Buffer);

    OFGetprop(Node, "width", (char *)&Buffer, sizeof(int));
    Width = decode_int(Buffer);
    OFGetprop(Node, "height", (char *)&Buffer, sizeof(int));
    Height = decode_int(Buffer);
    OFGetprop(Node, "linebytes", (char *)&Buffer, sizeof(int));
    Stride = decode_int(Buffer);

    ofwprintf("W: %d, H: %d, Stride: %d, Depth: %d\n", Width, Height, Stride, Depth);

    OFGetprop(Node, "address", &Buffer, sizeof(int));
    Framebuffer = decode_int(Buffer);

    ofwprintf("Addr: %x\n", Framebuffer);
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
