#include <ntddk.h>
#include "vgavideo.h"

#define NDEBUG
#include <debug.h>

static VGA_REGISTERS Mode12Regs =
{
   /* CRT Controller Registers */
   {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x59, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3},
   /* Attribute Controller Registers */
   {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x81, 0x00, 0x0F, 0x00, 0x00},
   /* Graphics Controller Registers */
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x05, 0x0F, 0xFF},
   /* Sequencer Registers */
   {0x03, 0x01, 0x0F, 0x00, 0x06},
   /* Misc Output Register */
   0xE3
};

VGA_REGISTERS TextModeRegs;

STATIC VOID FASTCALL
vgaSaveRegisters(PVGA_REGISTERS Registers)
{
   int i;

   for (i = 0; i < sizeof(Registers->CRT); i++)
   {
      VideoPortWritePortUchar(CRTC, i);
      Registers->CRT[i] = VideoPortReadPortUchar(CRTCDATA);
   }

   for (i = 0; i < sizeof(Registers->Attribute); i++)
   {
      VideoPortReadPortUchar(STATUS);
      VideoPortWritePortUchar(ATTRIB, i);
      Registers->Attribute[i] = VideoPortReadPortUchar(ATTRIBREAD);
   }

   for (i = 0; i < sizeof(Registers->Graphics); i++)
   {
      VideoPortWritePortUchar(GRAPHICS, i);
      Registers->Graphics[i] = VideoPortReadPortUchar(GRAPHICSDATA);
   }

   for (i = 0; i < sizeof(Registers->Sequencer); i++)
   {
      VideoPortWritePortUchar(SEQ, i);
      Registers->Sequencer[i] = VideoPortReadPortUchar(SEQDATA);
   }

   Registers->Misc = VideoPortReadPortUchar(MISC);
}

STATIC VOID FASTCALL
vgaSetRegisters(PVGA_REGISTERS Registers)
{
   int i;

   /* Update misc output register */
   VideoPortWritePortUchar(MISC, Registers->Misc);

   /* Synchronous reset on */
   VideoPortWritePortUchar(SEQ, 0x00);
   VideoPortWritePortUchar(SEQDATA, 0x01);

   /* Write sequencer registers */
   for (i = 1; i < sizeof(Registers->Sequencer); i++)
   {
      VideoPortWritePortUchar(SEQ, i);
      VideoPortWritePortUchar(SEQDATA, Registers->Sequencer[i]);
   }

   /* Synchronous reset off */
   VideoPortWritePortUchar(SEQ, 0x00);
   VideoPortWritePortUchar(SEQDATA, 0x03);

   /* Deprotect CRT registers 0-7 */
   VideoPortWritePortUchar(CRTC, 0x11);
   VideoPortWritePortUchar(CRTCDATA, Registers->CRT[0x11] & 0x7f);

   /* Write CRT registers */
   for (i = 0; i < sizeof(Registers->CRT); i++)
   {
      VideoPortWritePortUchar(CRTC, i);
      VideoPortWritePortUchar(CRTCDATA, Registers->CRT[i]);
   }

   /* Write graphics controller registers */
   for (i = 0; i < sizeof(Registers->Graphics); i++)
   {
      VideoPortWritePortUchar(GRAPHICS, i);
      VideoPortWritePortUchar(GRAPHICSDATA, Registers->Graphics[i]);
   }

   /* Write attribute controller registers */
   for (i = 0; i < sizeof(Registers->Attribute); i++)
   {
      VideoPortReadPortUchar(STATUS);
      VideoPortWritePortUchar(ATTRIB, i);
      VideoPortWritePortUchar(ATTRIB, Registers->Attribute[i]);
   }

   /* Renable screen. */
   VideoPortWritePortUchar(ATTRIB, 0x20);
}

VOID
InitVGAMode()
{
   vgaSaveRegisters(&TextModeRegs);
   vgaSetRegisters(&Mode12Regs);
}

VOID 
VGAResetDevice(OUT PSTATUS_BLOCK StatusBlock)
{
   vgaSetRegisters(&TextModeRegs);
}
