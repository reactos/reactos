#define NTOS_KERNEL_MODE
#include <ntos.h>
#include <ntos/ntddvid.h>
#include <ddk/ntddk.h>
#include <debug.h>
#include "vgavideo.h"

void outxay(PUSHORT ad, UCHAR x, UCHAR y)
{
  USHORT xy = (x << 8) + y;

  VideoPortWritePortUshort(ad, xy);
}

void setMode(VideoMode mode)
{
  unsigned char x;
  unsigned int y, c, a, m, n;

  VideoPortWritePortUchar((PUCHAR)MISC, mode.Misc);
  VideoPortWritePortUchar((PUCHAR)STATUS, 0);
  VideoPortWritePortUchar((PUCHAR)FEATURE, mode.Feature);

  for(x=0; x<5; x++)
  {
    outxay((PUSHORT)SEQ, mode.Seq[x], x);
  }

  VideoPortWritePortUshort((PUSHORT)CRTC, 0x11);
  VideoPortWritePortUshort((PUSHORT)CRTC, (mode.Crtc[0x11] & 0x7f));

  for(x=0; x<25; x++)
  {
    outxay((PUSHORT)CRTC, mode.Crtc[x], x);
  }

  for(x=0; x<9; x++)
  {
    outxay((PUSHORT)GRAPHICS, mode.Gfx[x], x);
  }

  x=VideoPortReadPortUchar((PUCHAR)FEATURE);

  for(x=0; x<21; x++)
  {
    VideoPortWritePortUchar((PUCHAR)ATTRIB, x);
    VideoPortWritePortUchar((PUCHAR)ATTRIB, mode.Attrib[x]);
  }

  x=VideoPortReadPortUchar((PUCHAR)STATUS);

  VideoPortWritePortUchar((PUCHAR)ATTRIB, 0x20);
}

VideoMode Mode12 = {
    0xa000, 0xe3, 0x00,

    {0x03, 0x01, 0x0f, 0x00, 0x06 },

    {0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x59, 0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3,
     0xff},

    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff},

    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
     0x0c, 0x0d, 0x0e, 0x0f, 0x81, 0x00, 0x0f, 0x00, 0x00}
};

void InitVGAMode()
{
  int i;
  VIDEO_X86_BIOS_ARGUMENTS vxba;
  NTSTATUS vps;
  
  // FIXME: Use Vidport to map the memory properly
  vidmem = (char *)(0xd0000000 + 0xa0000);
  memset(&vxba, 0, sizeof(vxba));
  vxba.Eax = 0x0012;
  vps = VideoPortInt10(NULL, &vxba);

  // Get VGA registers into the correct state (mainly for setting up the palette registers correctly)
  setMode(Mode12);

  // Get the VGA into the mode we want to work with
  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);     // Set
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,0);        // the MASK
  WRITE_PORT_USHORT((PUSHORT)0x3ce,0x0205); // write mode = 2 (bits 0,1) read mode = 0  (bit 3)
  i = READ_REGISTER_UCHAR(vidmem);          // Update bit buffer
  WRITE_REGISTER_UCHAR(vidmem, 0);          // Write the pixel
  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,0xff);

  // Zero out video memory (clear a possibly trashed screen)
  RtlZeroMemory(vidmem, 64000);

  vgaPreCalc();
}

VOID  VGAResetDevice(OUT PSTATUS_BLOCK  StatusBlock)
{
  char *vidmem;
  HANDLE Event;
  OBJECT_ATTRIBUTES Attr;
  UNICODE_STRING Name = UNICODE_STRING_INITIALIZER(L"\\TextConsoleRefreshEvent");
  NTSTATUS Status;
  VIDEO_X86_BIOS_ARGUMENTS vxba;
  NTSTATUS vps;
  ULONG ThreadRelease = 1;
  
  CHECKPOINT;
  Event = 0;

  memset(&vxba, 0, sizeof(vxba));
  vxba.Eax = 0x0003;
  vps = VideoPortInt10(NULL, &vxba);
  memset(&vxba, 0, sizeof(vxba));
  vxba.Eax = 0x1112;
  vps = VideoPortInt10(NULL, &vxba);
  InitializeObjectAttributes( &Attr, &Name, 0, 0, 0 );
  Status = ZwOpenEvent( &Event, STANDARD_RIGHTS_ALL, &Attr );
  if( !NT_SUCCESS( Status ) )
    DbgPrint( "VGA: Failed to open refresh event\n" );
  else {
    ZwSetEvent( Event, &ThreadRelease );
    ZwClose( Event );
  }
}
