/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    vga.c

Abstract:

    VGA HW dependent draw routines

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher
	Reactos Port by Eugene Ingerman

Revision History:

    04-Aug-1998:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"

//#include <asm/io.h>
//#include <linux/ctype.h>


////////////////////////////////////////////////////
// PROTOTYPES
////
extern void pice_save_current_registers(void);
extern void pice_restore_current_registers(void);
extern void pice_set_mode_3_80x50(void);
extern void pice_set_mode_3_80x25(void);

extern UCHAR cGraphTable[8*256];

// storage for original VGA font
UCHAR cGraphTable2[16*256];

////////////////////////////////////////////////////
// DEFINES
////
#define VGA_EXTENDED  // define this for 80x50 console mode

#ifndef VGA_EXTENDED
#define SCREEN_BUFFER_SIZE (80*25*2)
#else
#define SCREEN_BUFFER_SIZE (80*50*2)
#endif

/* Port addresses of control regs */
#define MISCOUTPUT  0x3c2
#define FEATURECONTROL 0x3da
#define SEQUENCER 0x3c4
#define CRTC 0x03d4
#define GRAPHICS 0x3ce
#define ATTRIBS 0x03c0
#define PELADDRESSWRITE 0x3c8
#define PELDATAREG 0x3c9

/* Number of regs on the various controllers */

#define MAXSEQ 5
#define MAXCRTC 0x19
#define MAXGRAPH 0x9
#define MAXATTRIB 0x015

////////////////////////////////////////////////////
// GLOBALS
////
// used for HERCULES text and VGA text mode
WINDOW wWindowVga[4]=
#ifndef VGA_EXTENDED
{
	{1,3,1,0,FALSE},
	{5,4,1,0,FALSE},
	{10,9,1,0,FALSE},
	{20,4,1,0,FALSE}
};
#else // VGA_EXTENDED
{
	{1,3,1,0,FALSE},
	{5,4,1,0,FALSE},
	{10,24,1,0,FALSE},
	{35,14,1,0,FALSE}
};
#endif // VGA_EXTENDED

PUCHAR pScreenBufferVga;
PUCHAR pScreenBufferSaveVga = NULL;
PUCHAR pScreenBufferTempVga;
PUCHAR pScreenBufferHardwareVga;
PUCHAR pFontBufferVga = NULL;

UCHAR offset_a = 0;
UCHAR offset_c = 0,offset_d = 0;
UCHAR offset_e = 0,offset_f = 0;

struct _attr
{
    union
    {
        struct
        {

            UCHAR fgcol : 4;
            UCHAR bkcol : 3;
            UCHAR blink : 1;
        }bits;
        UCHAR Asuchar;
    }u;
}attr;

unsigned char oldgraphicsmode;
unsigned char oldgraphicsmisc;
unsigned char oldsqregmapmask;
unsigned char oldsqregmemory;
unsigned char oldgraphicssetresetenable;
unsigned char oldgraphicsreadmapsel;

unsigned char read_vga_reg(int port, int reg)
{
  outportb(port,reg);
  return(inportb(port+1));
}

void write_vga_reg(int port, unsigned char reg, unsigned char value)
{
	outportb(port,reg);
	outportb(port+1,value);
}

/* Registers within controllers */
#define VREND 0x11
#define GRREGSETRESET 0
#define GRREGENABLESETRESET 1
#define GRREGREADMAPSEL 4
#define SQREGMAPMASK 2
#define SQREGMEMORY 4
#define GRREGWRMODE 5
#define GRREGMISC 6

void map_font_memory(void)
{
	oldgraphicssetresetenable = read_vga_reg(GRAPHICS, GRREGENABLESETRESET);
	oldgraphicsmode = read_vga_reg(GRAPHICS, GRREGWRMODE);
	oldgraphicsmisc = read_vga_reg(GRAPHICS, GRREGMISC);
	oldgraphicsreadmapsel = read_vga_reg(GRAPHICS, GRREGREADMAPSEL);
	oldsqregmapmask = read_vga_reg(SEQUENCER, SQREGMAPMASK);
	oldsqregmemory = read_vga_reg(SEQUENCER, SQREGMEMORY);


	/* Make sure set/reset enable is off */
	write_vga_reg(GRAPHICS,GRREGENABLESETRESET,0);
	/* Select read plane 2 */
	write_vga_reg(GRAPHICS,GRREGREADMAPSEL,0x02);
	/* Make sure write and read mode = 0 */
	write_vga_reg(GRAPHICS,GRREGWRMODE,0x00);
	/* Set mapping to 64K at a000:0 & turn off odd/even at the graphics reg */
	write_vga_reg(GRAPHICS,GRREGMISC, 0x04);
	/* Set sequencer plane to 2 */
	write_vga_reg(SEQUENCER,SQREGMAPMASK, 0x04);
	/* Turn off odd/even at the sequencer */
	write_vga_reg(SEQUENCER,SQREGMEMORY, 0x07);
}

void unmap_font_memory(void)
{
	write_vga_reg(GRAPHICS,GRREGENABLESETRESET,oldgraphicssetresetenable);
	write_vga_reg(GRAPHICS,GRREGWRMODE,oldgraphicsmode);
	write_vga_reg(GRAPHICS,GRREGREADMAPSEL,oldgraphicsreadmapsel);
	write_vga_reg(GRAPHICS,GRREGMISC, oldgraphicsmisc);
	write_vga_reg(SEQUENCER,SQREGMAPMASK, oldsqregmapmask);
	write_vga_reg(SEQUENCER,SQREGMEMORY, oldsqregmemory);
}

/* Font and palette constants */
#define BYTESPERFONT 8
#define FONTENTRIES 256
#define FONTBUFFERSIZE 8192

void save_font(UCHAR* graph_table)
{
	PUCHAR FontBase = pFontBufferVga;
	int i,j;
	map_font_memory();

	for (i=0; i < FONTENTRIES; i++)
		for (j=0; j < 16; j++)
				graph_table[i*16+j] = FontBase[i*32+j];

	unmap_font_memory();
}

void load_font(UCHAR* graph_table,int bEnter)
{
	PUCHAR FontBase = pFontBufferVga;
	int i,j;
	map_font_memory();

	if(bEnter)
	{
#ifdef VGA_EXTENDED
		for (i=0; i < FONTENTRIES; i++)
			for (j=0; j < 8; j++)
					FontBase[i*32+j] = graph_table[i*BYTESPERFONT+j];
#else // VGA_EXTENDED
		for (i=0; i < FONTENTRIES; i++)
			for (j=0; j < 16; j++)
					FontBase[i*32+j] = graph_table[i*BYTESPERFONT+(j/2)] << (j&1);
#endif // VGA_EXTENDED
	}
	else
	{
		for (i=0; i < FONTENTRIES; i++)
			for (j=0; j < 16; j++)
					FontBase[i*32+j] = graph_table[i*16+j];
	}

	unmap_font_memory();
}

//*************************************************************************
// SetForegroundColorVga()
//
//*************************************************************************
void SetForegroundColorVga(ECOLORS col)
{
    attr.u.bits.fgcol = col;
    attr.u.bits.blink = 0;
}

//*************************************************************************
// SetBackgroundColorVga()
//
//*************************************************************************
void SetBackgroundColorVga(ECOLORS col)
{
    attr.u.bits.bkcol = col;
    attr.u.bits.blink = 0;
}

//*************************************************************************
// PrintGrafVga()
//
//*************************************************************************
void PrintGrafVga(ULONG x,ULONG y,UCHAR c)
{
    ((PUSHORT)pScreenBufferVga)[y*GLOBAL_SCREEN_WIDTH + x] = (USHORT)((attr.u.Asuchar<<8)|c);
}

//*************************************************************************
// ShowCursor()
//
// show hardware cursor
//*************************************************************************
void ShowCursorVga(void)
{
    ENTER_FUNC();

    bCursorEnabled=TRUE;

	outb_p(0x0a,0x3d4);
	outb_p(inb_p(0x3d5)&~0x20,0x3d5);

    LEAVE_FUNC();
}

//*************************************************************************
// HideCursorVga()
//
// hide hardware cursor
//*************************************************************************
void HideCursorVga(void)
{
    ENTER_FUNC();
	bCursorEnabled=FALSE;

	outb_p(0x0a,0x3d4);
	outb_p(inb_p(0x3d5)|0x20,0x3d5);

    LEAVE_FUNC();
}

//*************************************************************************
// CopyLineTo()
//
// copy a line from src to dest
//*************************************************************************
void CopyLineToVga(USHORT dest,USHORT src)
{
    PUSHORT p = (PUSHORT)pScreenBufferVga;

    ENTER_FUNC();

	PICE_memcpy(&p[dest*GLOBAL_SCREEN_WIDTH],&p[src*GLOBAL_SCREEN_WIDTH],GLOBAL_SCREEN_WIDTH*sizeof(USHORT));

    LEAVE_FUNC();
}

//*************************************************************************
// InvertLineVga()
//
// invert a line on the screen
//*************************************************************************
void InvertLineVga(ULONG line)
{
    ULONG i;
    PUSHORT p = (PUSHORT)pScreenBufferVga;
    USHORT attr;

    if(line < GLOBAL_SCREEN_HEIGHT)
    {
        attr = p[line*GLOBAL_SCREEN_WIDTH]>>8;
        attr = ((attr & 0x07)<<4) | ((attr & 0xF0)>>4);
        attr <<= 8;
        for(i=0;i<GLOBAL_SCREEN_WIDTH;i++)
            p[line*GLOBAL_SCREEN_WIDTH + i] = (p[line*GLOBAL_SCREEN_WIDTH + i] & 0x00FF) | attr;
    }
}

//*************************************************************************
// HatchLineVga()
//
// hatches a line on the screen
//*************************************************************************
void HatchLineVga(ULONG line)
{
    ULONG i;
    PUSHORT p = (PUSHORT)pScreenBufferVga;

    if(line < GLOBAL_SCREEN_HEIGHT)
    {
        for(i=0;i<GLOBAL_SCREEN_WIDTH;i++)
            p[line*GLOBAL_SCREEN_WIDTH + i] = (p[line*GLOBAL_SCREEN_WIDTH + i] & 0xF0FF) | 0x0c00;
    }
}

//*************************************************************************
// ClrLineVga()
//
// clear a line on the screen
//*************************************************************************
void ClrLineVga(ULONG line)
{
    ULONG i;
    PUSHORT p = (PUSHORT)pScreenBufferVga;

    if(line < GLOBAL_SCREEN_HEIGHT)
    {
        for(i=0;i<GLOBAL_SCREEN_WIDTH;i++)
            p[line*GLOBAL_SCREEN_WIDTH + i] = (USHORT)((attr.u.Asuchar<<8) | 0x20);
    }
}

//*************************************************************************
// PrintLogoVga()
//
//*************************************************************************
void PrintLogoVga(BOOLEAN bShow)
{
    NOT_IMPLEMENTED();
}

//*************************************************************************
// PrintCursorVga()
//
// emulate a blinking cursor block
//*************************************************************************
void PrintCursorVga(BOOLEAN bForce)
{
    static ULONG count=0;
    USHORT charoffset;
    UCHAR data;
    ULONG x=wWindow[OUTPUT_WINDOW].usCurX,y=wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].usCurY;

	if( count++>250 )
	{
        count=0;

        charoffset = (y* GLOBAL_SCREEN_WIDTH + x);

		outb_p(0x0e,0x3d4);
		data=(UCHAR)((charoffset>>8)&0xFF);
		outb_p(data,0x3d5);

        outb_p(0x0f,0x3d4);
		data=(UCHAR)(charoffset & 0xFF);
		outb_p(data,0x3d5);
    }
}

//*************************************************************************
// SaveGraphicsVga()
//
//*************************************************************************
void SaveGraphicsStateVga(void)
{
	UCHAR data;

	// save current regs
	pice_save_current_registers();

	// unprotect crtc regs 0-7
	outb_p(0x11,0x3d4);
	data = inb_p(0x3d5);
	outb_p(data & 0x7F,0x3d5);

	// save current font
	save_font(cGraphTable2);

	// restore original regs
#ifdef VGA_EXTENDED
	pice_set_mode_3_80x50();
#else
	pice_set_mode_3_80x25();
#endif

	// load a font
	load_font(cGraphTable,1);

	// copy the screen content to temp area
    PICE_memcpy(pScreenBufferTempVga,pScreenBufferHardwareVga,SCREEN_BUFFER_SIZE);
    // copy the console to the screen
    PICE_memcpy(pScreenBufferHardwareVga,pScreenBufferVga,SCREEN_BUFFER_SIZE);
    // save original pointer
    pScreenBufferSaveVga = pScreenBufferVga;
    // pScreenBufferVga now points to screen
    pScreenBufferVga = pScreenBufferHardwareVga;
}

//*************************************************************************
// RestoreGraphicsStateVga()
//
//*************************************************************************
void RestoreGraphicsStateVga(void)
{
	UCHAR data;

	// unprotect crtc regs 0-7
	outb_p(0x11,0x3d4);
	data = inb_p(0x3d5);
	outb_p(data & 0x7F,0x3d5);

	// restore original regs
	pice_restore_current_registers();

	// load a font
	load_font(cGraphTable2,0);

    pScreenBufferVga = pScreenBufferSaveVga;
    // copy screen to the console
    PICE_memcpy(pScreenBufferVga,pScreenBufferHardwareVga,SCREEN_BUFFER_SIZE);
    // copy the temp area to the screen
    PICE_memcpy(pScreenBufferHardwareVga,pScreenBufferTempVga,SCREEN_BUFFER_SIZE);
}

//*************************************************************************
// ConsoleInitVga()
//
// init terminal screen
//*************************************************************************
BOOLEAN ConsoleInitVga(void)
{
	BOOLEAN bResult = FALSE;
    PUSHORT p;
	PHYSICAL_ADDRESS FrameBuffer;
	PHYSICAL_ADDRESS FontBuffer;


    ENTER_FUNC();

    ohandlers.CopyLineTo            = CopyLineToVga;
    ohandlers.PrintGraf             = PrintGrafVga;
    ohandlers.ClrLine               = ClrLineVga;
    ohandlers.InvertLine            = InvertLineVga;
    ohandlers.HatchLine             = HatchLineVga;
    ohandlers.PrintLogo             = PrintLogoVga;
    ohandlers.PrintCursor           = PrintCursorVga;
    ohandlers.SaveGraphicsState     = SaveGraphicsStateVga;
    ohandlers.RestoreGraphicsState  = RestoreGraphicsStateVga;
    ohandlers.ShowCursor            = ShowCursorVga;
    ohandlers.HideCursor            = HideCursorVga;
    ohandlers.SetForegroundColor    = SetForegroundColorVga;
    ohandlers.SetBackgroundColor    = SetBackgroundColorVga;

    ihandlers.GetKeyPolled          = KeyboardGetKeyPolled;
    ihandlers.FlushKeyboardQueue    = KeyboardFlushKeyboardQueue;

    SetWindowGeometry(wWindowVga);

    GLOBAL_SCREEN_WIDTH = 80;
#ifndef VGA_EXTENDED
    GLOBAL_SCREEN_HEIGHT = 25;
#else // VGA_EXTENDED
    GLOBAL_SCREEN_HEIGHT = 50;
#endif // VGA_EXTENDED

    attr.u.Asuchar = 0x07;

    // the real framebuffer
	FrameBuffer.u.LowPart = 0xB8000;
	pScreenBufferHardwareVga = MmMapIoSpace(FrameBuffer,SCREEN_BUFFER_SIZE,FALSE);

	//The real font buffer
	FontBuffer.u.LowPart = 0xA0000;
	pFontBufferVga = MmMapIoSpace(FontBuffer,FONTBUFFERSIZE,FALSE);

    // the console
	pScreenBufferVga = PICE_malloc(SCREEN_BUFFER_SIZE,NONPAGEDPOOL);
    // the save area
	pScreenBufferTempVga = PICE_malloc(SCREEN_BUFFER_SIZE,NONPAGEDPOOL);

	if(pScreenBufferVga)
	{
        DPRINT((0,"VGA memory phys. 0x000b0000 mapped to virt. 0x%x\n",pScreenBufferVga));

        bResult = TRUE;

        p = (PUSHORT)pScreenBufferVga;

		PICE_memset(pScreenBufferVga,0x0,SCREEN_BUFFER_SIZE);

        DPRINT((0,"VGA memory cleared!\n"));

        EmptyRingBuffer();

        DPRINT((0,"ConsoleInitVga() SUCCESS!\n"));
	}

    LEAVE_FUNC();

	return bResult;
}

//*************************************************************************
// ConsoleShutdownVga()
//
// exit terminal screen
//*************************************************************************
void ConsoleShutdownVga(void)
{
    ENTER_FUNC();

	if(pScreenBufferVga)
    {
        PICE_free(pScreenBufferVga);
        PICE_free(pScreenBufferTempVga);
		MmUnmapIoSpace(pScreenBufferHardwareVga,SCREEN_BUFFER_SIZE);
		MmUnmapIoSpace(pFontBufferVga,FONTBUFFERSIZE);
    }

    LEAVE_FUNC();
}


