#include <ntddk.h>
#include <debug.h>
#include "vgavideo.h"

void InitVGAMode()
{
   int i;
   VIDEO_X86_BIOS_ARGUMENTS vxba;
   VP_STATUS vps;

   // FIXME: Use Vidport to map the memory properly
   vidmem = (char *)(0xd0000000 + 0xa0000);
   memset(&vxba, 0, sizeof(vxba));
   vxba.Eax = 0x0012;
   vps = VideoPortInt10(NULL, &vxba);

   // Get the VGA into the mode we want to work with
   WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);     // Set
   WRITE_PORT_UCHAR((PUCHAR)0x3cf,0);        // the MASK
   WRITE_PORT_USHORT((PUSHORT)0x3ce,0x0205); // write mode = 2 (bits 0,1) read mode = 0  (bit 3)
   i = READ_REGISTER_UCHAR(vidmem);          // Update bit buffer
   WRITE_REGISTER_UCHAR(vidmem, 0);          // Write the pixel
   WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
   WRITE_PORT_UCHAR((PUCHAR)0x3cf,0xff);

   vgaPreCalc();
}


VOID  VGAResetDevice(OUT PSTATUS_BLOCK  StatusBlock)
{
  char *vidmem;
  HANDLE Event;
  OBJECT_ATTRIBUTES Attr;
  UNICODE_STRING Name;
  NTSTATUS Status;
  VIDEO_X86_BIOS_ARGUMENTS vxba;
  VP_STATUS vps;
  
  CHECKPOINT;
  Event = 0;

  vxba.Eax = 0x0003;
  vps = VideoPortInt10(NULL, &vxba);
  RtlInitUnicodeString( &Name, L"\\TextConsoleRefreshEvent" );
  InitializeObjectAttributes( &Attr, &Name, 0, 0, 0 );
  Status = NtOpenEvent( &Event, STANDARD_RIGHTS_ALL, &Attr );
  if( !NT_SUCCESS( Status ) )
    DbgPrint( "VGA: Failed to open refresh event\n" );
  else {
    NtSetEvent( Event, 1 );
    NtClose( Event );
  }
}



