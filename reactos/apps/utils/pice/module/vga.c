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
#include <linux/ctype.h>


////////////////////////////////////////////////////
// PROTOTYPES
////

////////////////////////////////////////////////////
// DEFINES
////
#define LOCAL_CONSOLE // undefine this to get text only hercules version

////////////////////////////////////////////////////
// GLOBALS
////
// used for HERCUELS text and VGA text mode
WINDOW wWindowVga[4]=
{
	{1,3,1,0,FALSE},
	{5,4,1,0,FALSE},
	{10,9,1,0,FALSE},
	{20,4,1,0,FALSE}
};

// 25 line text mode
UCHAR MGATable25[]={97,80,82,15,25, 6,25,25, 2,13,11,12, 0, 0, 0, 0};

PUCHAR pScreenBufferVga;
PUCHAR pScreenBufferSaveVga = NULL; 
PUCHAR pScreenBufferTempVga;
PUCHAR pScreenBufferHardwareVga;

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
    
#ifdef LOCAL_CONSOLE
	outb_p(0x0a,0x3d4);
	outb_p(inb_p(0x3d5)&~0x20,0x3d5);
#else
	outb_p(0x0a,0x3b4);
	outb_p(inb_p(0x3b5)&~0x20,0x3b5);
#endif

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
    
#ifdef LOCAL_CONSOLE
	outb_p(0x0a,0x3d4);
	outb_p(inb_p(0x3d5)|0x20,0x3d5);
#else
	outb_p(0x0a,0x3b4);
	outb_p(inb_p(0x3b5)|0x20,0x3b5);
#endif
    
    LEAVE_FUNC();
}

//*************************************************************************
// CopyLineTo()
//
// copy a line from src to dest
//*************************************************************************
void CopyLineToVga(USHORT dest,USHORT src)
{
	USHORT i;
    PUSHORT p = (PUSHORT)pScreenBufferVga;

    ENTER_FUNC();

	for(i=0;i<GLOBAL_SCREEN_WIDTH;i++)
		p[dest*GLOBAL_SCREEN_WIDTH+i] = p[src*GLOBAL_SCREEN_WIDTH+i];

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
#ifdef LOCAL_CONSOLE
    USHORT attr;
#endif

    if(line<25)
    {
#ifdef LOCAL_CONSOLE
        attr = p[line*GLOBAL_SCREEN_WIDTH]>>8;
        attr = ((attr & 0x07)<<4) | ((attr & 0xF0)>>4);
        attr <<= 8;
        for(i=0;i<GLOBAL_SCREEN_WIDTH;i++)
            p[line*GLOBAL_SCREEN_WIDTH + i] = (p[line*GLOBAL_SCREEN_WIDTH + i] & 0x00FF) | attr;
#else
        for(i=0;i<GLOBAL_SCREEN_WIDTH;i++)
            p[line*GLOBAL_SCREEN_WIDTH + i] = p[line*GLOBAL_SCREEN_WIDTH + i] ^ 0xFF00;
#endif
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

    if(line<GLOBAL_SCREEN_HEIGHT)
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

    if(line<GLOBAL_SCREEN_HEIGHT)
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

#ifndef LOCAL_CONSOLE
		outb_p(0x0e,0x3b4);
		data=(UCHAR)((charoffset>>8)&0xFF); 
		outb_p(data,0x3b5);

		outb_p(0x0d,0x3b4);
		data=(UCHAR)(charoffset & 0xFF); 
		outb_p(data,0x3b5);
#else
		outb_p(0x0e,0x3d4);
		data=(UCHAR)((charoffset>>8)&0xFF); 
		outb_p(data,0x3d5);

        outb_p(0x0f,0x3d4);
		data=(UCHAR)(charoffset & 0xFF); 
		outb_p(data,0x3d5);
#endif
    }
}

//*************************************************************************
// SaveGraphicsVga()
//
//*************************************************************************
void SaveGraphicsStateVga(void)
{
#ifdef LOCAL_CONSOLE
    // copy the screen content to temp area
    PICE_memcpy(pScreenBufferTempVga,pScreenBufferHardwareVga,FRAMEBUFFER_SIZE);
    // copy the console to the screen
    PICE_memcpy(pScreenBufferHardwareVga,pScreenBufferVga,FRAMEBUFFER_SIZE);
    // save original pointer
    pScreenBufferSaveVga = pScreenBufferVga;
    // pScreenBufferVga now points to screen
    pScreenBufferVga = pScreenBufferHardwareVga;

    // save video RAM start address
    outb_p(0xc,0x3d4);
    offset_c = inb_p(0x3d5);
    outb_p(0x0,0x3d5);
    outb_p(0xd,0x3d4);
    offset_d = inb_p(0x3d5);
    outb_p(0x0,0x3d5);

    // cursor state
    outb_p(0x0a,0x3d4);
	offset_a = inb_p(0x3d5);
    // cursor position
	outb_p(0x0e,0x3d4);
	offset_e = inb_p(0x3d5);
	outb_p(0x0f,0x3d4);
	offset_f = inb_p(0x3d5);
#endif
}

//*************************************************************************
// RestoreGraphicsStateVga()
//
//*************************************************************************
void RestoreGraphicsStateVga(void)
{
#ifdef LOCAL_CONSOLE
    pScreenBufferVga = pScreenBufferSaveVga;
    // copy screen to the console
    PICE_memcpy(pScreenBufferVga,pScreenBufferHardwareVga,FRAMEBUFFER_SIZE);
    // copy the temp area to the screen
    PICE_memcpy(pScreenBufferHardwareVga,pScreenBufferTempVga,FRAMEBUFFER_SIZE);

    // restore video RAM start address
    outb_p(0xc,0x3d4);
    outb_p(offset_c,0x3d5);
    outb_p(0xd,0x3d4);
    outb_p(offset_d,0x3d5);

    // cursor state
    outb_p(0x0a,0x3d4);
	outb_p(offset_a,0x3d5);
    // cursor position
	outb_p(0x0e,0x3d4);
    outb_p(offset_e,0x3d5);
	outb_p(0x0f,0x3d4);
    outb_p(offset_f,0x3d5);
#endif
}

//*************************************************************************
// ConsoleInitVga()
//
// init terminal screen
//*************************************************************************
BOOLEAN ConsoleInitVga(void) 
{
	BOOLEAN bResult = FALSE;
#ifndef LOCAL_CONSOLE
	PUCHAR pMGATable = MGATable25;
	UCHAR i,reg,data;
#endif
    PUSHORT p;

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
    GLOBAL_SCREEN_HEIGHT = 25;

    attr.u.Asuchar = 0x07;

#ifdef LOCAL_CONSOLE
    // the real framebuffer
	pScreenBufferHardwareVga = ioremap(0xB8000,FRAMEBUFFER_SIZE); 
    // the console
	pScreenBufferVga = vmalloc(FRAMEBUFFER_SIZE); 
    // the save area
	pScreenBufferTempVga = vmalloc(FRAMEBUFFER_SIZE); 
#else
	outb_p(0,0x3b8);
	outb_p(0,0x3bf);
	for(i=0;i<sizeof(MGATable25);i++) 
	{ 
		reg=i; 
		outb_p(reg,0x3b4);
		data=pMGATable[i]; 
		outb_p(data,0x3b5);
	}
	outb_p(0x08,0x3b8);

	pScreenBufferVga=ioremap(0xB0000,FRAMEBUFFER_SIZE); 
#endif 
	if(pScreenBufferVga)
	{
        DPRINT((0,"VGA memory phys. 0x000b0000 mapped to virt. 0x%x\n",pScreenBufferVga)); 

        bResult = TRUE;

        p = (PUSHORT)pScreenBufferVga;
        
		PICE_memset(pScreenBufferVga,0x0,FRAMEBUFFER_SIZE);

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
 
#ifdef LOCAL_CONSOLE
	if(pScreenBufferVga)
    {
        vfree(pScreenBufferVga);
        vfree(pScreenBufferTempVga);
		iounmap(pScreenBufferHardwareVga); 
    }
#else
	// HERC video off 
	outb_p(0,0x3b8);
	outb_p(0,0x3bf);

	if(pScreenBufferVga)
		iounmap(pScreenBufferVga); 
#endif

    LEAVE_FUNC();
}
