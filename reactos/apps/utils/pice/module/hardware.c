/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    hardware.c

Abstract:
	
    output to console

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

#include <asm/io.h>
#include <linux/ctype.h>
#include <asm/delay.h>

////////////////////////////////////////////////////
// PROTOTYPES
////

////////////////////////////////////////////////////
// DEFINES
////

////////////////////////////////////////////////////
// GLOBALS
////

// flagging stuff
BOOLEAN bCursorEnabled = FALSE;
BOOLEAN bConsoleIsInitialized = FALSE;


// terminal emulation
ETERMINALMODE eTerminalMode = TERMINAL_MODE_NONE;

// window stuff
WINDOW wWindow[4];

// screen parameter
ULONG GLOBAL_SCREEN_WIDTH,GLOBAL_SCREEN_HEIGHT;

// jump table to real output functions
OUTPUT_HANDLERS ohandlers;
INPUT_HANDLERS ihandlers;

// ring buffer stuff
ULONG ulInPos = 0,ulLastPos = 0;
ULONG ulOldInPos = 0,ulOldDelta = 0;
BOOLEAN bSuspendPrintRingBuffer = FALSE;

char aBuffers[LINES_IN_BUFFER][1024];

// output lock
ULONG ulOutputLock;

// color of windows pane separation bars
USHORT usCaptionColor       = BLUE;
USHORT usCaptionText        = WHITE;
USHORT usForegroundColor    = LTGRAY;
USHORT usBackgroundColor    = BLACK;
USHORT usHiLiteColor        = WHITE;

////////////////////////////////////////////////////
// FUNCTIONS
////

//*************************************************************************
// SuspendPrintRingBuffer()
//
//*************************************************************************
void SuspendPrintRingBuffer(BOOLEAN bSuspend)
{
    ENTER_FUNC();
    bSuspendPrintRingBuffer = bSuspend;
    LEAVE_FUNC();
}

//*************************************************************************
// EmptyRingBuffer()
//
//*************************************************************************
void EmptyRingBuffer(void)
{
    //ENTER_FUNC();
	ulLastPos = ulInPos = ulOldInPos = ulOldDelta = 0;
	PICE_memset(aBuffers,0,sizeof(aBuffers));
    //LEAVE_FUNC();
}

//*************************************************************************
// LinesInRingBuffer()
//
//*************************************************************************
ULONG LinesInRingBuffer(void)
{
    ULONG ulResult;

//    ENTER_FUNC();

    ulResult = (ulInPos-ulLastPos)%LINES_IN_BUFFER;

//    LEAVE_FUNC();

	return ulResult;
}

//*************************************************************************
// CheckRingBuffer()
//
//*************************************************************************
void CheckRingBuffer(void)
{
//    ENTER_FUNC();

    Acquire_Output_Lock();

	if(ulInPos != ulOldInPos )
	{
		ulOldInPos = ulInPos;
		PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);
	}

    Release_Output_Lock();

//    LEAVE_FUNC();
}

//*************************************************************************
// AddToRingBuffer()
//
//*************************************************************************
BOOLEAN AddToRingBuffer(LPSTR p)
{
	ULONG i,j,len;
    BOOLEAN bHadReturn = FALSE;
    char temp[sizeof(aBuffers[0])];

//    ENTER_FUNC();

    // size of current string
    j=PICE_strlen(aBuffers[ulInPos]);

    // start with ':' and current has ':' in front
    if(aBuffers[ulInPos][0]==':' && *p==':')
    {
        if(j==1)
        {
            //LEAVE_FUNC();
            return FALSE;
        }
		aBuffers[ulInPos][j++]='\n';
		aBuffers[ulInPos][j]=0;
		ulInPos = (ulInPos+1)%LINES_IN_BUFFER;
		// wrap around
		if(ulInPos == ulLastPos)
		{
			ulLastPos = (ulLastPos+1)%LINES_IN_BUFFER;
			PICE_memset(aBuffers[ulInPos],0,sizeof(aBuffers[0]));
		}
        // reset to start of buffer
		j = 0;
    }
    // it's an internal print ("pICE: ...")
    else if(aBuffers[ulInPos][0]==':' && PICE_strncmpi(p,"pICE:",5)==0)
    {
		if(j==1)
		{
			PICE_memset(aBuffers[ulInPos],0,sizeof(aBuffers[0]));
		}
		else
		{
			aBuffers[ulInPos][j++]='\n';
			aBuffers[ulInPos][j]=0;
			ulInPos = (ulInPos+1)%LINES_IN_BUFFER;
			// wrap around
			if(ulInPos == ulLastPos)
			{
				ulLastPos = (ulLastPos+1)%LINES_IN_BUFFER;
				PICE_memset(aBuffers[ulInPos],0,sizeof(aBuffers[0]));
			}
		}
        // reset to start of buffer
		j = 0;
    }
    // it's a debug print and the current line is starting with ':'
    else if(aBuffers[ulInPos][0]==':' &&
			( (*p=='<' && isdigit(*(p+1)) && *(p+2)=='>') || bIsDebugPrint) )
    {
		if(j==1)
		{
			PICE_memset(aBuffers[ulInPos],0,sizeof(aBuffers[0]));
		}
		else
		{
			aBuffers[ulInPos][j++]='\n';
			aBuffers[ulInPos][j]=0;
			ulInPos = (ulInPos+1)%LINES_IN_BUFFER;
			// wrap around
			if(ulInPos == ulLastPos)
			{
				ulLastPos = (ulLastPos+1)%LINES_IN_BUFFER;
				PICE_memset(aBuffers[ulInPos],0,sizeof(aBuffers[0]));
			}
		}
        // reset to start of buffer
		j = 0;
    }
    // it's a debug print
    else if(( (*p=='<' && isdigit(*(p+1)) && *(p+2)=='>') || bIsDebugPrint) )
    {
        p += 3;
    }

    // size of new string
    len=PICE_strlen(p);

    // if combined string length too big
    // reduce to maximum
    if( (len+j) > sizeof(aBuffers[0])-2 )
    {
        PICE_memcpy(temp,p,sizeof(aBuffers[0])-2);
        p = temp;
        // assume we end in NEWLINE
        p[sizeof(aBuffers[0])-2]='\n';
        p[sizeof(aBuffers[0])-1]=0;
    }

	for(i=0;p[i]!=0;i++)
	{
		// newline
		if(p[i]=='\n')
		{
			aBuffers[ulInPos][j++]='\n';
			aBuffers[ulInPos][j]=0;
			ulInPos = (ulInPos+1)%LINES_IN_BUFFER;
			// wrap around
			if(ulInPos == ulLastPos)
			{
				ulLastPos = (ulLastPos+1)%LINES_IN_BUFFER;
				PICE_memset(aBuffers[ulInPos],0,sizeof(aBuffers[0]));
			}
            // reset to start of buffer
			j = 0;
            // notify that we had a NEWLINE
            bHadReturn = TRUE;
		}
		// backspace
		else if(p[i]=='\b')
		{
			if(j!=0)
			{
				j--;
				aBuffers[ulInPos][j] = 0;
			}
		}
		// TAB
		else if(p[i]=='\t')
		{
            // copy TAB
			aBuffers[ulInPos][j++] = p[i];
		}
		else
		{
			if((UCHAR)p[i]<0x20 && (UCHAR)p[i]>0x7f)
				p[i]=0x20;
			
			aBuffers[ulInPos][j++] = p[i];
		}
	}
//    LEAVE_FUNC();

    return bHadReturn;
}

//*************************************************************************
// ReplaceRingBufferCurrent()
//
//*************************************************************************
void ReplaceRingBufferCurrent(LPSTR s)
{
//    ENTER_FUNC();

    PICE_memset(aBuffers[ulInPos],0,sizeof(aBuffers[0]));
    PICE_strcpy(aBuffers[ulInPos],s);

//    LEAVE_FUNC();
}

//*************************************************************************
// PrintRingBuffer()
//
//*************************************************************************
void PrintRingBuffer(ULONG ulLines)
{
	ULONG ulDelta = LinesInRingBuffer();
	ULONG ulOutPos,i=0;

//    ENTER_FUNC();

	if(bSuspendPrintRingBuffer)
    {
        DPRINT((0,"PrintRingBuffer(): suspended\n"));
        LEAVE_FUNC();
        return;
    }
    
	if(!ulDelta)
    {
        DPRINT((0,"PrintRingBuffer(): no lines in ring buffer\n"));
        LEAVE_FUNC();
		return;
    }

    if(ulDelta<ulOldDelta)
    {
        DPRINT((0,"PrintRingBuffer(): lines already output\n"));
        LEAVE_FUNC();
        return;
    }

    ulOldDelta = ulDelta;

	if(ulDelta < ulLines)
    {
        DPRINT((0,"PrintRingBuffer(): less lines than requested\n"));
		ulLines = ulDelta;
    }

	ulOutPos = (ulInPos-ulLines)%LINES_IN_BUFFER;
    DPRINT((0,"PrintRingBuffer(): ulOutPos = %u\n",ulOutPos));

    Home(OUTPUT_WINDOW);

	while(ulLines--)
	{
        ClrLine(wWindow[OUTPUT_WINDOW].y+i);
		Print(OUTPUT_WINDOW_UNBUFFERED,aBuffers[ulOutPos]);
	    i++;
		ulOutPos = (ulOutPos+1)%LINES_IN_BUFFER;
	}

    if(aBuffers[ulOutPos][0]==':')
    {
        ClrLine(wWindow[OUTPUT_WINDOW].y+i);
		Print(OUTPUT_WINDOW_UNBUFFERED,aBuffers[ulOutPos]);
    	wWindow[OUTPUT_WINDOW].usCurX = 1;
    }

//    LEAVE_FUNC();
}

//*************************************************************************
// PrintRingBufferOffset()
//
//*************************************************************************
BOOLEAN PrintRingBufferOffset(ULONG ulLines,ULONG ulOffset)
{
	ULONG ulLinesInRingBuffer = LinesInRingBuffer();
	ULONG ulOutPos,i=0;

//    ENTER_FUNC();

    // no lines in ring buffer
	if(!ulLinesInRingBuffer)
    {
        DPRINT((0,"PrintRingBufferOffset(): ulLinesInRingBuffer is 0\n"));
        LEAVE_FUNC();
		return FALSE;
    }

    // more lines inc. offset to display than in ring buffer
	if(ulLinesInRingBuffer < ulLines)
    {
        ulLines = ulLinesInRingBuffer;
    }

	if(ulLinesInRingBuffer < ulOffset+ulLines)
    {
        DPRINT((0,"PrintRingBufferOffset(): ulLinesInRingBuffer < ulOffset+ulLines\n"));
        LEAVE_FUNC();
        return FALSE;
    }

    DPRINT((0,"PrintRingBufferOffset(): ulLinesInRingBuffer %u ulLines %u ulOffset %u\n",ulLinesInRingBuffer,ulLines,ulOffset));

    ulOutPos = (ulInPos-ulOffset-ulLines)%LINES_IN_BUFFER;

    DPRINT((0,"PrintRingBufferOffset(): ulOutPos = %u\n",ulOutPos));

    if(ulOutPos == ulInPos)
    {
        DPRINT((0,"PrintRingBufferOffset(): ulOutPos == ulInPos\n"));
        LEAVE_FUNC();
        return FALSE;
    }

    // start to output upper left corner of window
	Home(OUTPUT_WINDOW);

    // while not end reached...
	while(ulLines--)
	{
        ClrLine(wWindow[OUTPUT_WINDOW].y+i);
		Print(OUTPUT_WINDOW_UNBUFFERED,aBuffers[ulOutPos]);
		i++;
		ulOutPos = (ulOutPos+1)%LINES_IN_BUFFER;
	}

    if(aBuffers[ulInPos][0]==':')
    {
        ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy-1);
        wWindow[OUTPUT_WINDOW].usCurY = wWindow[OUTPUT_WINDOW].cy-1;
		Print(OUTPUT_WINDOW_UNBUFFERED,aBuffers[ulInPos]);
    	wWindow[OUTPUT_WINDOW].usCurX = strlen(aBuffers[ulInPos])+1;
    }

//    LEAVE_FUNC();
    return TRUE;
}

//*************************************************************************
// PrintRingBufferHome()
//
//*************************************************************************
BOOLEAN PrintRingBufferHome(ULONG ulLines)
{
	ULONG ulDelta = LinesInRingBuffer();
	ULONG ulOutPos,i=0;

//    ENTER_FUNC();

    // no lines in ring buffer
	if(!ulDelta)
    {
        DPRINT((0,"PrintRingBufferHome(): no lines in ring buffer\n"));
        LEAVE_FUNC();
		return FALSE;
    }

    // more lines inc. offset to display than in ring buffer
	if(ulDelta < ulLines)
    {
        ulLines = ulDelta;
    }

    // calc the start out position
	ulOutPos = ulLastPos;

    // start to output upper left corner of window
	Home(OUTPUT_WINDOW);

    // while not end reached...
	while(ulLines--)
	{
        ClrLine(wWindow[OUTPUT_WINDOW].y+i);
		Print(OUTPUT_WINDOW_UNBUFFERED,aBuffers[ulOutPos]);
		i++;
		ulOutPos = (ulOutPos+1)%LINES_IN_BUFFER;
	}

    if(aBuffers[ulInPos][0]==':')
    {
        ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].cy-1);
        wWindow[OUTPUT_WINDOW].usCurY = wWindow[OUTPUT_WINDOW].cy-1;
		Print(OUTPUT_WINDOW_UNBUFFERED,aBuffers[ulInPos]);
    	wWindow[OUTPUT_WINDOW].usCurX = strlen(aBuffers[ulInPos])+1;
    }
    
//    LEAVE_FUNC();

    return TRUE;
}

//*************************************************************************
// ResetColor()
//
//*************************************************************************
void ResetColor(void)
{
    SetForegroundColor(COLOR_FOREGROUND);
	SetBackgroundColor(COLOR_BACKGROUND);
}

// OUTPUT handlers

//*************************************************************************
// PrintGraf()
//
//*************************************************************************
void PrintGraf(ULONG x,ULONG y,UCHAR c)
{
    ohandlers.PrintGraf(x,y,c);
}

//*************************************************************************
// Flush()
//
// Flush the stream of chars to PrintGraf()
//*************************************************************************
void Flush(void)
{
    if(ohandlers.Flush)
        ohandlers.Flush();
}

//*************************************************************************
// EnableScroll()
//
//*************************************************************************
void EnableScroll(USHORT Window)
{
    ENTER_FUNC();
	wWindow[Window].bScrollDisabled=FALSE;
    LEAVE_FUNC();
}

//*************************************************************************
// DisableScroll()
//
//*************************************************************************
void DisableScroll(USHORT Window)
{
    ENTER_FUNC();
	wWindow[Window].bScrollDisabled=TRUE;
    LEAVE_FUNC();
}


//*************************************************************************
// ShowCursor()
//
// show hardware cursor
//*************************************************************************
void ShowCursor(void)
{
    ohandlers.ShowCursor();
}

//*************************************************************************
// HideCursor()
//
// hide hardware cursor
//*************************************************************************
void HideCursor(void)
{
    ohandlers.HideCursor();
}

//*************************************************************************
// SetForegroundColor()
//
// set foreground color
//*************************************************************************
void SetForegroundColor(ECOLORS c)
{
//    ENTER_FUNC();

    ohandlers.SetForegroundColor(c);

//    LEAVE_FUNC();
}

//*************************************************************************
// SetBackgroundColor()
//
// set background color
//*************************************************************************
void SetBackgroundColor(ECOLORS c)
{
//    ENTER_FUNC();

    ohandlers.SetBackgroundColor(c);

//    LEAVE_FUNC();
}

//*************************************************************************
// PutChar()
//
// put a zero terminated string at position (x,y)
//*************************************************************************
void PutChar(LPSTR p,ULONG x,ULONG y)
{
	ULONG i;

//    ENTER_FUNC();

    Acquire_Output_Lock();

	for(i=0;p[i]!=0;i++)
	{
		if((UCHAR)p[i]>=0x20 && (UCHAR)p[i]<0x80)
		{
			PrintGraf(x+i,y,p[i]);
		}
	}

    Flush();

    Release_Output_Lock();

//    LEAVE_FUNC();
}

//*************************************************************************
// CopyLineTo()
//
// copy a line from src to dest
//*************************************************************************
void CopyLineTo(USHORT dest,USHORT src)
{
    ohandlers.CopyLineTo(dest,src);
}

//*************************************************************************
// InvertLine()
//
// invert a line on the screen
//*************************************************************************
void InvertLine(ULONG line)
{
    ohandlers.InvertLine(line);
}

//*************************************************************************
// HatchLine()
//
// hatches a line on the screen
//*************************************************************************
void HatchLine(ULONG line)
{
    ohandlers.HatchLine(line);
}

//*************************************************************************
// ClrLine()
//
// clear a line on the screen
//*************************************************************************
void ClrLine(ULONG line)
{
    ohandlers.ClrLine(line);
}

//*************************************************************************
// ScrollUp()
//
// Scroll a specific window up one line
//*************************************************************************
void ScrollUp(USHORT Window)
{
    USHORT i;

    return;

	if(!wWindow[Window].bScrollDisabled)
	{
		for(i=1;i<wWindow[Window].cy;i++)
		{
			CopyLineTo((USHORT)(wWindow[Window].y+i-1),(USHORT)(wWindow[Window].y+i));
		}
		ClrLine((USHORT)(wWindow[Window].y+wWindow[Window].cy-1));
	}
}

//*************************************************************************
// Home()
//
// cursor to home position
//*************************************************************************
void Home(USHORT Window)
{
	wWindow[Window].usCurX=0;
    wWindow[Window].usCurY=0;

}

//*************************************************************************
// Clear()
//
// clear a specific window
//*************************************************************************
void Clear(USHORT Window)
{
    ULONG j;

    for(j=0;j<wWindow[Window].cy;j++)
    {
		ClrLine(wWindow[Window].y+j);
    }

    Home(Window);
}

//*************************************************************************
// PrintCaption()
//
//*************************************************************************
void PrintCaption(void)
{
    const char title[]=" PrivateICE system level debugger (LINUX) (c) 1998-2001 PrivateTOOLS ";

    SetForegroundColor(COLOR_TEXT);
	SetBackgroundColor(COLOR_CAPTION);

	ClrLine(0);	
	PutChar((LPSTR)title,
		   (GLOBAL_SCREEN_WIDTH-sizeof(title))/2,
           0);

    ResetColor();
}

//*************************************************************************
// PrintTemplate()
//
// print the screen template
//*************************************************************************
void PrintTemplate(void)
{
    USHORT i,j;

	ENTER_FUNC();

    ResetColor();

    for(j=0;j<4;j++)
    {
	    for(i=wWindow[j].y;i<(wWindow[j].y+wWindow[j].cy);i++)
        {
		    ClrLine(i);
        }
    }

    PrintCaption();

	SetForegroundColor(COLOR_TEXT);
	SetBackgroundColor(COLOR_CAPTION);

	ClrLine(wWindow[DATA_WINDOW].y-1);	
	ClrLine(wWindow[SOURCE_WINDOW].y-1);	
	ClrLine(wWindow[OUTPUT_WINDOW].y-1);	

    ResetColor();

	ShowRunningMsg();
    PrintLogo(TRUE);

	LEAVE_FUNC();
}

//*************************************************************************
// PrintLogo()
//
//*************************************************************************
void PrintLogo(BOOLEAN bShow)
{
    ohandlers.PrintLogo(bShow);
}

//*************************************************************************
// PrintCursor()
//
// emulate a blinking cursor block
//*************************************************************************
void PrintCursor(BOOLEAN bForce)
{
    ohandlers.PrintCursor(bForce);
}

//*************************************************************************
// Print()
//
//*************************************************************************
void Print(USHORT Window,LPSTR p)
{
	ULONG i;

    //ENTER_FUNC();
    if(!bConsoleIsInitialized)
    {
        DPRINT((0,"Print(): console is not initialized!\n"));
        //LEAVE_FUNC();
        return;
    }


    // the OUTPUT_WINDOW is specially handled 
	if(Window == OUTPUT_WINDOW)
	{
        DPRINT((0,"Print(): OUTPUT_WINDOW\n"));
		if(AddToRingBuffer(p))
        {
            DPRINT((0,"Print(): checking ring buffer\n"));
            CheckRingBuffer();
        }
        else
        {
            DPRINT((0,"Print(): outputting a line from ring buffer\n"));
            wWindow[OUTPUT_WINDOW].usCurX = 0;
		    ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].usCurY);
    		Print(OUTPUT_WINDOW_UNBUFFERED,aBuffers[ulInPos]);
        }
	}
	else
	{
        BOOLEAN bOutput = TRUE; 

		if(Window == OUTPUT_WINDOW_UNBUFFERED)
        {
			Window = OUTPUT_WINDOW;
        }

        for(i=0;p[i]!=0;i++)
		{
            if(wWindow[Window].usCurX > (GLOBAL_SCREEN_WIDTH-1))
                bOutput = FALSE;

			// newline
			if(p[i]=='\n')
			{
				wWindow[Window].usCurX = 0;
				wWindow[Window].usCurY++;
				if(wWindow[Window].usCurY>=wWindow[Window].cy)
				{
					wWindow[Window].usCurY=wWindow[Window].cy-1;
					ScrollUp(Window);
				}
                if(wWindow[Window].bScrollDisabled==TRUE)break;
			}
			// backspace
			else if(p[i]=='\b')
			{
				if(wWindow[Window].usCurX>0)
				{
					wWindow[Window].usCurX--;
                    if(bOutput)
					    PrintGraf(wWindow[Window].usCurX,wWindow[Window].y+wWindow[Window].usCurY,0x20);
				}

			}
			// TAB
			else if(p[i]=='\t')
			{
				if((wWindow[Window].usCurX + 4) < (GLOBAL_SCREEN_WIDTH-1))
				{
					wWindow[Window].usCurX += 4;
				}
			}
			else
			{
				if((UCHAR)p[i]<0x20 && (UCHAR)p[i]>0x7f)
					p[i]=0x20;

                if(bOutput)
    				PrintGraf(wWindow[Window].usCurX,wWindow[Window].y+wWindow[Window].usCurY,p[i]);

				wWindow[Window].usCurX++;
			}
		}

        // flush
        Flush();
	}
    //LEAVE_FUNC();
}


//*************************************************************************
// SaveGraphicsState()
//
//*************************************************************************
void SaveGraphicsState(void)
{
    ohandlers.SaveGraphicsState();
}

//*************************************************************************
// RestoreGraphicsState()
//
//*************************************************************************
void RestoreGraphicsState(void)
{
    ohandlers.RestoreGraphicsState();
}

//*************************************************************************
// SetWindowGeometry()
//
//*************************************************************************
void SetWindowGeometry(PVOID pWindow)
{
    PICE_memcpy(wWindow,pWindow,sizeof(wWindow));
}

// INPUT handlers

//*************************************************************************
// GetKeyPolled()
//
//*************************************************************************
UCHAR GetKeyPolled(void)
{
    return ihandlers.GetKeyPolled();
}

//*************************************************************************
// FlushKeyboardQueue()
//
//*************************************************************************
void FlushKeyboardQueue(void)
{
    ihandlers.FlushKeyboardQueue();
}


//*************************************************************************
// ConsoleInit()
//
// init terminal screen
//*************************************************************************
BOOLEAN ConsoleInit(void) 
{
    BOOLEAN bResult = FALSE;

    ENTER_FUNC();

    // preset ohandlers and ihandler to NULL
    memset((void*)&ohandlers,0,sizeof(ohandlers));
    memset((void*)&ihandlers,0,sizeof(ihandlers));

    switch(eTerminalMode)
    {
        case TERMINAL_MODE_HERCULES_GRAPHICS:
            bResult = ConsoleInitHercules();
            break;
        case TERMINAL_MODE_HERCULES_TEXT:
            break;
        case TERMINAL_MODE_VGA_TEXT:
            bResult = ConsoleInitVga();
            break;
        case TERMINAL_MODE_SERIAL:
            bResult = ConsoleInitSerial();
            break;
        case TERMINAL_MODE_NONE:
        default:
            // fail
            break;
    }

    // check that outputhandlers have all been set
    // ohandlers.Flush may be zero on return 
    if( !ohandlers.ClrLine              ||
        !ohandlers.CopyLineTo           ||
        !ohandlers.HatchLine            ||
        !ohandlers.HideCursor           ||
        !ohandlers.InvertLine           ||
        !ohandlers.PrintCursor          ||
        !ohandlers.PrintGraf            ||
        !ohandlers.PrintLogo            ||
        !ohandlers.RestoreGraphicsState ||
        !ohandlers.SaveGraphicsState    ||
        !ohandlers.SetBackgroundColor   ||
        !ohandlers.SetForegroundColor   ||
        !ohandlers.ShowCursor)
    {
        bResult = FALSE;
    }

    // check that inputhandlers were installed
    if( !ihandlers.GetKeyPolled ||
        !ihandlers.FlushKeyboardQueue)
    {
        bResult = FALSE;
    }

    LEAVE_FUNC();

    bConsoleIsInitialized = bResult;

    return bResult;
}

//*************************************************************************
// ConsoleShutdown()
//
// exit terminal screen
//*************************************************************************
void ConsoleShutdown(void) 
{
    ENTER_FUNC();

    // sleep for a few seconds
    __udelay(1000*5000);

    switch(eTerminalMode)
    {
        case TERMINAL_MODE_HERCULES_GRAPHICS:
            ConsoleShutdownHercules();
            break;
        case TERMINAL_MODE_HERCULES_TEXT:
            break;
        case TERMINAL_MODE_VGA_TEXT:
            ConsoleShutdownVga();
            break;
        case TERMINAL_MODE_SERIAL:
            ConsoleShutdownSerial();
            break;
        case TERMINAL_MODE_NONE:
        default:
            // fail
            break;
    }

    LEAVE_FUNC();
}

// EOF
