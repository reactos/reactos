/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    hercules.c

Abstract:

    HW dependent draw routines

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

#include "charset.h"
#include "logo.h"

////////////////////////////////////////////////////
// PROTOTYPES
////

////////////////////////////////////////////////////
// DEFINES
////

////////////////////////////////////////////////////
// GLOBALS
////
// cursor state
BOOLEAN bRev=FALSE;

// HERCULES graphics adapter stuff
// 43 line graphics mode
UCHAR MGATable43[]={53,45,46, 7,96, 2,91,91, 2, 3, 0, 0, 0, 0, 0, 0};

PUCHAR pVgaOffset[4];
// END of HERCULES graphics adapter stuff

// used for HERCULES graphics mode
WINDOW wWindowHercGraph[4]=
{
	{1,3,1,0,FALSE},
	{5,6,1,0,FALSE},
	{12,19,1,0,FALSE},
	{32,12,1,0,FALSE}
};
// used for HERCUELS text and VGA text mode
WINDOW wWindowHerc[4]=
{
	{1,3,1,0,FALSE},
	{5,4,1,0,FALSE},
	{10,9,1,0,FALSE},
	{20,4,1,0,FALSE}
};

PUCHAR pScreenBufferHercules;

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
void SetForegroundColorHercules(ECOLORS col)
{
    attr.u.bits.fgcol = col;
    attr.u.bits.blink = 0;
}

//*************************************************************************
// SetBackgroundColorVga()
//
//*************************************************************************
void SetBackgroundColorHercules(ECOLORS col)
{
    attr.u.bits.bkcol = col;
    attr.u.bits.blink = 0;
}

//*************************************************************************
// PrintGrafHercules()
//
//*************************************************************************
void PrintGrafHercules(ULONG x,ULONG y,UCHAR c)
{
    ULONG i;
    PUCHAR p;
    ULONG _line = y<<3;

	if(!pScreenBufferHercules)
		return;

    p=&cGraphTable[(ULONG)c<<3];

    if((attr.u.bits.bkcol == COLOR_FOREGROUND && attr.u.bits.fgcol == COLOR_BACKGROUND) ||
       (attr.u.bits.bkcol == COLOR_CAPTION && attr.u.bits.fgcol == COLOR_TEXT) )
	    for(i=0 ;i<8 ;i++,_line++)
	    {
            *(PUCHAR)(pVgaOffset[_line & 0x3] + ( 90* (_line >> 2) ) + x) = ~*p++;
        }
    else
	    for(i=0 ;i<8 ;i++,_line++)
	    {
            *(PUCHAR)(pVgaOffset[_line & 0x3] + ( 90* (_line >> 2) ) + x) = *p++;
        }
}


//*************************************************************************
// FlushHercules()
//
//*************************************************************************
void FlushHercules(void)
{
}

//*************************************************************************
// ShowCursor()
//
// show hardware cursor
//*************************************************************************
void ShowCursorHercules(void)
{
    ENTER_FUNC();

    bCursorEnabled=TRUE;

    LEAVE_FUNC();
}

//*************************************************************************
// HideCursorHercules()
//
// hide hardware cursor
//*************************************************************************
void HideCursorHercules(void)
{
    ENTER_FUNC();

    bCursorEnabled=FALSE;

    LEAVE_FUNC();
}

//*************************************************************************
// CopyLineTo()
//
// copy a line from src to dest
//*************************************************************************
void CopyLineToHercules(USHORT dest,USHORT src)
{
	USHORT i,j;
	PULONG pDest,pSrc;

    ENTER_FUNC();

    dest <<= 3;
    src <<= 3;
	for(i=0;i<8;i++)
	{
		(PUCHAR)pDest = (PUCHAR)pScreenBufferHercules + ( ( ( dest+i )&3) <<13 )+	90 * ((dest+i) >> 2);
		(PUCHAR)pSrc = (PUCHAR)pScreenBufferHercules + ( ( ( src+i )&3) <<13 )+	90 * ((src+i) >> 2);
		for(j=0;j<(GLOBAL_SCREEN_WIDTH>>2);j++)
		{
			*pDest++=*pSrc++;
		}
	}

    LEAVE_FUNC();
}

//*************************************************************************
// InvertLineHercules()
//
// invert a line on the screen
//*************************************************************************
void InvertLineHercules(ULONG line)
{
    ULONG i,j;
    ULONG _line = line<<3;
	PUSHORT p;

    //ENTER_FUNC();

	for(j=0;j<8;j++)
	{
		p=(PUSHORT)( pVgaOffset[_line&3] + (90*(_line>>2)) );
		for(i=0;i<(GLOBAL_SCREEN_WIDTH>>1);i++)
		{
			p[i]=~p[i];
		}
		_line++;
	}

    //LEAVE_FUNC();
}

//*************************************************************************
// HatchLineHercules()
//
// hatches a line on the screen
//*************************************************************************
void HatchLineHercules(ULONG line)
{
	USHORT cc;
    ULONG i,j;
    ULONG _line = (line<<3) ;
	PUSHORT p;
    USHORT mask_odd[]={0x8888,0x2222};
    USHORT mask_even[]={0xaaaa,0x5555};
    PUSHORT pmask;

    ENTER_FUNC();

    pmask = (line&1)?mask_odd:mask_even;

	for(j=0;j<8;j++,_line++)
	{
		p=(PUSHORT)( pVgaOffset[_line&3] + (90*(_line>>2)) );
		for(i=0;i<(GLOBAL_SCREEN_WIDTH/sizeof(USHORT));i++)
		{
			cc = p[i];

			p[i]=(p[i]^pmask[j&1])|cc;
		}
	}

    LEAVE_FUNC();
}

//*************************************************************************
// ClrLineHercules()
//
// clear a line on the screen
//*************************************************************************
void ClrLineHercules(ULONG line)
{
    ULONG j;
    BOOLEAN bTemplateLine=( (USHORT)line==wWindow[DATA_WINDOW].y-1 ||
                            (USHORT)line==wWindow[SOURCE_WINDOW].y-1 ||
                            (USHORT)line==wWindow[OUTPUT_WINDOW].y-1 ||
                            0);
	ULONG _line = line<<3;
    ULONG cc=0;
   	PUCHAR p;

//    ENTER_FUNC();

    if(line > GLOBAL_SCREEN_HEIGHT )
    {
        DPRINT((0,"ClrLineHercules(): line %u is out of screen\n",line));
        //LEAVE_FUNC();
        return;
    }

    if(attr.u.bits.bkcol == COLOR_CAPTION && attr.u.bits.fgcol == COLOR_TEXT )
        cc=~cc;

    if(bTemplateLine)
    {
	    for(j=0;j<8;j++,_line++)
	    {
            p = (PUCHAR)(pVgaOffset[_line&3] + (90*(_line>>2)) );

/*
			if(j==2 || j==5)cc=0xFF;
			else if(j==3)cc=0xaa;
			else if(j==4)cc=0x55;
			else cc = 0;*/
			if(j==2 || j==5)cc=0xFF;
			else cc = 0;

		    PICE_memset(p,(UCHAR)cc,GLOBAL_SCREEN_WIDTH);
	    }
    }
    else
    {
        for(j=0;j<8;j++,_line++)
        {
            p = (PUCHAR)(pVgaOffset[_line&3] + (90*(_line>>2)) );

            PICE_memset(p,(UCHAR)cc,GLOBAL_SCREEN_WIDTH);
        }
    }
    //LEAVE_FUNC();
}

//*************************************************************************
// PrintLogoHercules()
//
//*************************************************************************
void PrintLogoHercules(BOOLEAN bShow)
{
    LONG x,y;
    PUCHAR p;

    p=(PUCHAR)pScreenBufferHercules;
    for(y=0;y<24;y++)
    {
        for(x=0;x<8;x++)
        {
    	    p[ ( 0x2000* (( y + 8 ) & 0x3) )+
	    	    ( 90* ( (y + 8 ) >> 2) )+
		        (81+x)] = cLogo[y*8+x];
        }
    }
}

//*************************************************************************
// PrintCursorHercules()
//
// emulate a blinking cursor block
//*************************************************************************
void PrintCursorHercules(BOOLEAN bForce)
{
    static ULONG count=0;

	if( (bForce) || ((count++>100) && bCursorEnabled)  )
	{
    	ULONG i;
	    ULONG x,y;
        ULONG _line;

        x=wWindow[OUTPUT_WINDOW].usCurX;
        y=wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].usCurY;

        _line = y<<3;
		for(i=0;i<8;i++,_line++)
		{
            *(PUCHAR)(pVgaOffset[_line & 0x3] + ( 90* (_line >> 2) ) + x) ^= 0xFF ;
		}
		bRev=!bRev;
        count=0;
    }

	KeStallExecutionProcessor(2500);
}

//*************************************************************************
// SaveGraphicsHercules()
//
//*************************************************************************
void SaveGraphicsStateHercules(void)
{
    // not implemented
}

//*************************************************************************
// RestoreGraphicsStateHercules()
//
//*************************************************************************
void RestoreGraphicsStateHercules(void)
{
    // not implemented
}

//*************************************************************************
// ConsoleInitHercules()
//
// init terminal screen
//*************************************************************************
BOOLEAN ConsoleInitHercules(void)
{
	BOOLEAN bResult = FALSE;
	PUCHAR pMGATable = MGATable43;
	UCHAR i,reg,data;

    ENTER_FUNC();

    ohandlers.CopyLineTo            = CopyLineToHercules;
    ohandlers.PrintGraf             = PrintGrafHercules;
    ohandlers.Flush                 = FlushHercules;
    ohandlers.ClrLine               = ClrLineHercules;
    ohandlers.InvertLine            = InvertLineHercules;
    ohandlers.HatchLine             = HatchLineHercules;
    ohandlers.PrintLogo             = PrintLogoHercules;
    ohandlers.PrintCursor           = PrintCursorHercules;
    ohandlers.SaveGraphicsState     = SaveGraphicsStateHercules;
    ohandlers.RestoreGraphicsState  = RestoreGraphicsStateHercules;
    ohandlers.ShowCursor            = ShowCursorHercules;
    ohandlers.HideCursor            = HideCursorHercules;
    ohandlers.SetForegroundColor    = SetForegroundColorHercules;
    ohandlers.SetBackgroundColor    = SetBackgroundColorHercules;

    ihandlers.GetKeyPolled          = KeyboardGetKeyPolled;
    ihandlers.FlushKeyboardQueue    = KeyboardFlushKeyboardQueue;

    // init HERCULES adapter
	outb_p(0,0x3b8);
	outb_p(0x03,0x3bf);
	for(i=0;i<sizeof(MGATable43);i++)
	{
		reg=i;
		outb_p(reg,0x3b4);
		data=pMGATable[i];
		outb_p(data,0x3b5);
	}
	outb_p(0x0a,0x3b8);

    SetWindowGeometry(wWindowHercGraph);

    GLOBAL_SCREEN_WIDTH = 90;
    GLOBAL_SCREEN_HEIGHT = 45;

    attr.u.Asuchar = 0x07;

	pScreenBufferHercules=MmMapIoSpace(0xb0000,FRAMEBUFFER_SIZE,MmWriteCombined);

    DPRINT((0,"VGA memory phys. 0xb0000 mapped to virt. 0x%x\n",pScreenBufferHercules));

	if(pScreenBufferHercules)
	{
        for(i=0;i<4;i++)
        {
            pVgaOffset[i] = (PUCHAR)pScreenBufferHercules+0x2000*i;
        	DPRINT((0,"VGA offset %u = 0x%.8X\n",i,pVgaOffset[i]));
        }
		bResult = TRUE;

		PICE_memset(pScreenBufferHercules,0x0,FRAMEBUFFER_SIZE);

        EmptyRingBuffer();

        DPRINT((0,"ConsoleInitHercules() SUCCESS!\n"));
	}

    LEAVE_FUNC();

	return bResult;
}

//*************************************************************************
// ConsoleShutdownHercules()
//
// exit terminal screen
//*************************************************************************
void ConsoleShutdownHercules(void)
{
    ENTER_FUNC();

	// HERC video off
	outb_p(0,0x3b8);
	outb_p(0,0x3bf);

	if(pScreenBufferHercules)
		MmUnmapIoSpace(pScreenBufferHercules,FRAMEBUFFER_SIZE);

    LEAVE_FUNC();
}
