/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    shell.c

Abstract:

    user interface for debugger

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    16-Jul-1998:	created
    22-Sep-1998:	rewrite of keyboard hooking through patching the original keyboard driver
	29-Sep-1998:	started documentation on project
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"


////////////////////////////////////////////////////
// DEFINES
////
#define LINES_IN_COMMAND_BUFFER (64)

////////////////////////////////////////////////////
// PROTOTYPES
////

////////////////////////////////////////////////////
// GLOBALS
////

ULONG bPreviousCommandWasGo = FALSE;

// flags to set when we need to pass things to the old INT handlers
ULONG dwCallOldInt1Handler = 0;
ULONG dwCallOldInt3Handler = 0;
ULONG dwCallOldIntEHandler = 0;
ULONG dwCallOldGPFaultHandler = 0;

ULONG g_ulLineNumberStart=0;
ULONG ulWindowOffset = 0;
BOOLEAN bStepThroughSource=FALSE;
BOOLEAN bStepInto = FALSE;

// key handling
UCHAR ucConverted; // key converted from scancode to ANSI

volatile BOOLEAN bControl=FALSE; // TRUE when CTRL key was pressed
volatile BOOLEAN bShift=FALSE; // TRUE when SHIFT key was pressed
volatile BOOLEAN bAlt=FALSE; // TRUE when ALT key was pressed
volatile ULONG OldInt31Handler; // address of old keyboard ISR
volatile ULONG OldGlobalInt31Handler; // address of old global keyboard ISR
volatile BOOLEAN bEnterNow=FALSE; // TRUE if already stopped
volatile BOOLEAN bNotifyToExit=FALSE; // TRUE when debugger should leave
volatile BOOLEAN bSkipMainLoop=FALSE; // TRUE when debugger should skip main loop
volatile UCHAR ucKeyPressedWhileIdle=0; // key pressed when system was stopped
volatile BOOLEAN bInDebuggerShell=FALSE; // TRUE while in DebuggerShell()
BOOLEAN bIrqStateAtBreak;

ULONG ulRealStackPtr;
static ULONG PCR_SEL = PCR_SELECTOR;
static ULONG OLD_PCR;

char tempShell[256]; // temporary string container

// old address of display memory
USHORT OldSelector=0;
ULONG OldOffset=0;

ULONG ulLastLineDisplayedOffset = 0;

// functions of function keys
char *szFunctionKeys[10]={
	"mod", // F1
	"proc", // F2
	"src", // F3
	"code", // F4
	"x", // F5
	"vma", // F6
	"", // F7
	"t", // F8
	"", // F9
	"p"  // F10
};

// new stack for "deep parsing"
ULONG aulNewStack[0x20000];
ULONG ulOldStack;

// registers save area (context)
ULONG CurrentEIP,CurrentEFL;
ULONG CurrentEAX,CurrentEBX,CurrentECX,CurrentEDX;
ULONG CurrentESP,CurrentEBP,CurrentESI,CurrentEDI;
USHORT CurrentCS,CurrentDS=0,CurrentES,CurrentFS,CurrentGS,CurrentSS;
ULONG CurrentDR0,CurrentDR1,CurrentDR2,CurrentDR3,CurrentDR6,CurrentDR7;
ULONG CurrentCR0,CurrentCR2,CurrentCR3;
// previous context
ULONG OldEIP=0,OldEFL;
ULONG OldEAX,OldEBX,OldECX,OldEDX;
ULONG OldESP,OldEBP,OldESI,OldEDI;
USHORT OldCS=0,OldDS,OldES,OldFS,OldGS,OldSS;

ULONG CurrentProcess;

UCHAR ucCommandBuffer[256];
USHORT usCurrentPosInInputBuffer=0;
volatile BOOLEAN bSingleStep=FALSE;

// the last command lines
char aszCommandLines[LINES_IN_COMMAND_BUFFER][sizeof(ucCommandBuffer)+2];
ULONG ulCommandInPos=0,ulCommandLastPos=0;
ULONG ulCommandCurrentPos=0;


extern ULONG KeyboardIRQL;

//*************************************************************************
// GetLinesInCommandHistory()
//
//*************************************************************************
ULONG GetLinesInCommandHistory(void)
{
    ULONG ulResult = (ulCommandInPos-ulCommandLastPos)%LINES_IN_COMMAND_BUFFER;

    ENTER_FUNC();

    DPRINT((0,"GetLinesInCommandHistory() returns %u (ulIn %u ulLast %u)\n",ulResult,ulCommandInPos,ulCommandLastPos));

    LEAVE_FUNC();

    return ulResult;
}

//*************************************************************************
// AddToCommandLineHistory()
//
//*************************************************************************
void AddToCommandLineHistory(LPSTR s)
{
    ULONG i;

    ENTER_FUNC();
    DPRINT((0,"AddToCommandLineHistory(%s)\n",s));

    if(PICE_strlen(s))
    {
        for(i=0;i<LINES_IN_COMMAND_BUFFER;i++)
        {
            if(PICE_strcmpi(&aszCommandLines[i][1],s) == 0)
            {
                DPRINT((0,"AddToCommandLineHistory(): command line already exists\n"));
                LEAVE_FUNC();
                return;
            }
        }
        aszCommandLines[ulCommandInPos][0]=':';
        PICE_strcpy(&aszCommandLines[ulCommandInPos][1],s);
        ulCommandCurrentPos = ulCommandInPos = (ulCommandInPos +1)%LINES_IN_COMMAND_BUFFER;
        if(ulCommandInPos == ulCommandLastPos)
        {
            ulCommandLastPos = (ulCommandLastPos+1)%LINES_IN_COMMAND_BUFFER;
        }
    }

    LEAVE_FUNC();
}

//*************************************************************************
// GetFromCommandLineHistory()
//
//*************************************************************************
LPSTR GetFromCommandLineHistory(ULONG ulCurrentCommandPos)
{
    LPSTR pRet;

    ENTER_FUNC();

    DPRINT((0,"GetFromCommandLineHistory(): current = %u\n",ulCurrentCommandPos));

    // skip leading ':'
    pRet = aszCommandLines[ulCurrentCommandPos] + 1;

    DPRINT((0,"GetFromCommandLineHistory(%s)\n",pRet));

    LEAVE_FUNC();

    return pRet;
}

//*************************************************************************
// ShowStatusLine()
//
//*************************************************************************
void ShowStatusLine(void)
{
	PEPROCESS pCurrentProcess = IoGetCurrentProcess();
    LPSTR pProcessName;

    ENTER_FUNC();

    if(IsAddressValid((ULONG)pCurrentProcess))
    {
        SetForegroundColor(COLOR_TEXT);
	    SetBackgroundColor(COLOR_CAPTION);

		ClrLine(wWindow[OUTPUT_WINDOW].y-1);

        pProcessName = pCurrentProcess->ImageFileName;
        if(IsAddressValid((ULONG)pProcessName) )
        {
		    PICE_sprintf(tempShell,
                    " PROCESS(%.8X \"%s\") ",
                    (ULONG)pCurrentProcess,pProcessName);
        }
        else
        {
		    PICE_sprintf(tempShell,
                    " PROCESS(%.8X) ",
                    (ULONG)pCurrentProcess);
        }
		PutChar(tempShell,1,wWindow[OUTPUT_WINDOW].y-1);

        ResetColor();
    }

    LEAVE_FUNC();
}

//*************************************************************************
// ProcessBootParams()
//
//*************************************************************************
void ProcessBootParams(void)
{
    LPSTR p1,p2;

    ENTER_FUNC();
    if(*szBootParams)
    {
        DPRINT((0,"ProcessBootParams()\n"));

        p1 = szBootParams;

        while(*p1)
        {
            p2 = ucCommandBuffer;
            DPRINT((0,"ProcessBootParams(): boot params = %s\n",p1));
            while(*p1 && *p1!=';')
            {
                *p2++ = *p1++;
            }
            *p2=0;
            DPRINT((0,"ProcessBootParams(): cmd buf = %s\n",ucCommandBuffer));
            if(*p1 != ';')
            {
                DPRINT((0,"ProcessBootParams(): error in cmd buf\n"));
                break;
            }
            p1++;
            DPRINT((0,"ProcessBootParams(): next cmd buf = %s\n",p1));

    		Parse(ucCommandBuffer,TRUE);
        }
        PICE_memset(ucCommandBuffer,0,sizeof(ucCommandBuffer));
        *szBootParams = 0;
    }
    LEAVE_FUNC();
}

//*************************************************************************
// bNoCtrlKeys()
//
//*************************************************************************
BOOLEAN inline bNoCtrlKeys(void)
{
    return (!bControl && !bAlt && !bShift);
}


//*************************************************************************
// DebuggerShell()
//
// handle user interface when stopped system
//*************************************************************************
void DebuggerShell(void)
{
	ARGS Args;
    UCHAR speaker;
	PEPROCESS pCurrentProcess;

    ENTER_FUNC();

    // save the graphics state
    SaveGraphicsState();

	// tell USER we are stopped
	ShowStoppedMsg();

    FlushKeyboardQueue();

	CheckRingBuffer();

    // kill the speakers annoying beep
	speaker = inb_p((PCHAR)0x61);
	speaker &= 0xFC;
	outb_p(speaker,(PCHAR)0x61);

    ProcessBootParams();

    DPRINT((0,"DebuggerShell(): DisplayRegs()\n"));
	// display register contents
	DisplayRegs();

    DPRINT((0,"DebuggerShell(): DisplayMemory()\n"));
	// display data window
	Args.Value[0]=OldSelector;
	Args.Value[1]=OldOffset;
	Args.Count=2;
	DisplayMemory(&Args);

    DPRINT((0,"DebuggerShell(): Unassemble()\n"));

    // disassembly from current address
    PICE_memset(&Args,0,sizeof(ARGS));
	Args.Value[0]=CurrentCS;
	Args.Value[1]=CurrentEIP;
	Args.Count=2;
	Unassemble(&Args);

    // try to find current process's name
    pCurrentProcess = IoGetCurrentProcess();
    CurrentProcess = (ULONG)pCurrentProcess;

    // display status line
    ShowStatusLine();

	// switch on cursor
	ShowCursor();

    // while we are not told to exit
	while(bNotifyToExit==FALSE)
	{
		// emulate graphics cursor
		PrintCursor(FALSE);

		// we have a key press
		if((ucKeyPressedWhileIdle = GetKeyPolled())!=0)
		{
            DPRINT((0,"DebuggerShell(): key = %x control = %u shift = %u\n",ucKeyPressedWhileIdle,bControl,bShift));

            // if cursor reversed, normalize it again (only graphics)
            if(bRev)
            {
			    PrintCursor(TRUE);
            }

			// convert key to ANSI, if success add to command buffer and try to
			// find a command that fits the already entered letters
            ucConverted = AsciiFromScan((UCHAR)(ucKeyPressedWhileIdle&0x7f));

#if 0
            PICE_sprintf(tempShell,"%u -> %u",ucKeyPressedWhileIdle, ucConverted);
            PutChar(tempShell,GLOBAL_SCREEN_WIDTH-32,wWindow[OUTPUT_WINDOW].y-1);
#endif

			if(!bControl && !bAlt && ucConverted)
			{
                DPRINT((0,"DebuggerShell(): normal key\n"));
                if(!(usCurrentPosInInputBuffer==0 && ucConverted==' '))
                {
				    // if we have space in the command buffer
				    // put the character there
				    if(usCurrentPosInInputBuffer<sizeof(ucCommandBuffer)-1)
				    {
					    ucCommandBuffer[usCurrentPosInInputBuffer++]=ucConverted;
					    // output the character
					    PICE_sprintf(tempShell,"%c",ucConverted);
                        wWindow[OUTPUT_WINDOW].usCurX = 1;
					    Print(OUTPUT_WINDOW,tempShell);
				    }
				    // if we have something in command buffer
				    // try to find command help that fits
				    if(usCurrentPosInInputBuffer)
				    {
					    FindCommand(ucCommandBuffer);
				    }
				    else ShowStoppedMsg();
                }
			}
            // normal key while holding down CONTROL
            else if(bControl && !bAlt && !bShift && ucConverted)
            {
                if(ucConverted == 'f')
                    bNotifyToExit = TRUE;
            }
            // normal key while holding down ALT
            else if(!bControl && bAlt && !bShift && ucConverted)
            {
            }
            // normal key while holding down ALT & CONTROL
            else if(bControl && bAlt && !bShift && ucConverted)
            {
            }
			// we didn't get a converted key
			// so this must be a control key
			else
			{
				// RETURN
				if(bNoCtrlKeys() && ucKeyPressedWhileIdle == SCANCODE_ENTER)
				{
                    DPRINT((0,"DebuggerShell(): RETURN\n"));
					ucCommandBuffer[usCurrentPosInInputBuffer]=0;
					if(ucCommandBuffer[0])
					{
                        AddToCommandLineHistory(ucCommandBuffer);
                        ClrLine(wWindow[OUTPUT_WINDOW].y+wWindow[OUTPUT_WINDOW].usCurY);
                        ulLastLineDisplayedOffset = 0;
                        PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);
                        // setup a safe stack for parsing
                        __asm__ __volatile__("\n\t \
                            movl %2,%%eax\n\t \
                            movl %%esp,%%ebx\n\t \
                            mov  %%ebx,%0\n\t \
                            leal _aulNewStack,%%ebx\n\t \
                            addl $0x1FFF0,%%ebx\n\t \
                            movl %%ebx,%%esp\n\t \
                            pushl $0\n\t \
                            pushl %%eax\n\t \
                            call _Parse\n\t \
                            movl %0,%%ebx\n\t \
                            movl %%ebx,%%esp"
                            :"=m" (ulOldStack)
                            :"m" (ulOldStack),"m" (ucCommandBuffer)
                            :"eax","ebx");

                        ShowStoppedMsg();
					}
                    else
                    {
                        if(ulLastLineDisplayedOffset)
                        {
                            ulLastLineDisplayedOffset = 0;
                            PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);
                        }
                    }
					usCurrentPosInInputBuffer=0;
					PICE_memset(&ucCommandBuffer,0,sizeof(ucCommandBuffer));
				}
				// backspace
				else if(bNoCtrlKeys() && ucKeyPressedWhileIdle == SCANCODE_BACKSPACE)
				{
                    DPRINT((0,"DebuggerShell(): BACKSPACE\n"));
					if(usCurrentPosInInputBuffer)
					{
						if(usCurrentPosInInputBuffer)
                            FindCommand(ucCommandBuffer);
						else
                            ShowStoppedMsg();

						usCurrentPosInInputBuffer--;
						ucCommandBuffer[usCurrentPosInInputBuffer]=0;
						Print(OUTPUT_WINDOW,"\b");
					}
				}
				// Tab
				else if(bNoCtrlKeys() && ucKeyPressedWhileIdle==SCANCODE_TAB)
				{
                    DPRINT((0,"DebuggerShell(): TAB\n"));
					if(usCurrentPosInInputBuffer)
					{
						LPSTR pCmd;

						if((pCmd=FindCommand(ucCommandBuffer)) )
						{
							ULONG i;

							// clear the displayed command line
							for(i=0;i<usCurrentPosInInputBuffer;i++)
								Print(OUTPUT_WINDOW,"\b");
							// clear command buffer
							PICE_memset(&ucCommandBuffer,0,sizeof(ucCommandBuffer));
							// copy the found command into command buffer
							PICE_strcpy(ucCommandBuffer,pCmd);
							PICE_strcat(ucCommandBuffer," ");
							usCurrentPosInInputBuffer = PICE_strlen(ucCommandBuffer);
							Print(OUTPUT_WINDOW,ucCommandBuffer);
						}
					}
				}
				else
				{
					// function keys
					if(bNoCtrlKeys() && ucKeyPressedWhileIdle>=59 && ucKeyPressedWhileIdle<69)
					{
                        DPRINT((0,"DebuggerShell(): FUNCTION %u\n",ucKeyPressedWhileIdle-59));
                        PICE_sprintf(tempShell,":");
                        ReplaceRingBufferCurrent(tempShell);
                        PICE_memset(&ucCommandBuffer,0,sizeof(ucCommandBuffer));
            			usCurrentPosInInputBuffer=0;
                        PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);
						PICE_strcpy(ucCommandBuffer,szFunctionKeys[ucKeyPressedWhileIdle-59]);
						usCurrentPosInInputBuffer=PICE_strlen(ucCommandBuffer);
						if(ucCommandBuffer[0])
						{
                            ulLastLineDisplayedOffset = 0;
                            PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);

                            // setup a safe stack for parsing
                            __asm__ __volatile__("\n\t \
                                movl %2,%%eax\n\t \
                                movl %%esp,%%ebx\n\t \
                                mov  %%ebx,%0\n\t \
                                leal _aulNewStack,%%ebx\n\t \
                                addl $0x1FFF0,%%ebx\n\t \
                                movl %%ebx,%%esp\n\t \
                                pushl $1\n\t \
                                pushl %%eax\n\t \
                                call _Parse\n\t \
                                movl %0,%%ebx\n\t \
                                movl %%ebx,%%esp"
                                :"=m" (ulOldStack)
                                :"m" (ulOldStack),"m" (ucCommandBuffer)
                                :"eax","ebx");
							PICE_memset(&ucCommandBuffer,0,sizeof(ucCommandBuffer));
							usCurrentPosInInputBuffer=0;
						}
					}
                    else
                    {
                        switch(ucKeyPressedWhileIdle)
                        {
                            case SCANCODE_ESC:
                                if(usCurrentPosInInputBuffer)
                                {
                                    PICE_sprintf(tempShell,":");
                                    ReplaceRingBufferCurrent(tempShell);
                        		    PICE_memset(&ucCommandBuffer,0,sizeof(ucCommandBuffer));
            					    usCurrentPosInInputBuffer=0;
                                    Print(OUTPUT_WINDOW,"");
                                    ShowStoppedMsg();
                                }
                                break;
                            case SCANCODE_HOME: // home
                                DPRINT((0,"DebuggerShell(): HOME\n"));
                                // memory window
                                if(bAlt)
                                {
                                    DPRINT((0,"DebuggerShell(): data window home\n"));
                                    OldOffset=0x0;
	                                // display data window
	                                Args.Value[0]=OldSelector;
	                                Args.Value[1]=OldOffset;
	                                Args.Count=2;
	                                DisplayMemory(&Args);
                                }
                                // output window
                                else if(bShift)
                                {
                                    DPRINT((0,"DebuggerShell(): output window home\n"));
                                    if(ulLastLineDisplayedOffset != LinesInRingBuffer()-wWindow[OUTPUT_WINDOW].cy)
                                    {
                                        ulLastLineDisplayedOffset = LinesInRingBuffer()-wWindow[OUTPUT_WINDOW].cy+1;
                                        PrintRingBufferHome(wWindow[OUTPUT_WINDOW].cy-1);
                                    }
                                }
                                // source window home
                                else if(bControl)
                                {
                                    if(ulCurrentlyDisplayedLineNumber>0)
                                    {
                                        PICE_SYMBOLFILE_SOURCE* pSrc;

                                        if(ConvertTokenToSrcFile(szCurrentFile,(PULONG)&pSrc) )
                                        {
                                            ulCurrentlyDisplayedLineNumber = 1;

                                            DisplaySourceFile((LPSTR)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE),
                                                              (LPSTR)pSrc+pSrc->ulOffsetToNext,
                                                              1,-1);
                                        }
                                    }
                                }
                                else if(!bShift && !bControl && !bAlt)
                                {
                                }
                                break;
                            case SCANCODE_END: // end
                                DPRINT((0,"DebuggerShell(): END\n"));
                                // memory window
                                if(bAlt)
                                {
                                    DPRINT((0,"DebuggerShell(): data window end\n"));
                                    OldOffset=0xFFFFFFFF-0x10*4;
	                                // display data window
	                                Args.Value[0]=OldSelector;
	                                Args.Value[1]=OldOffset;
	                                Args.Count=2;
	                                DisplayMemory(&Args);
                                }
                                // output window
                                else if(bShift)
                                {
                                    DPRINT((0,"DebuggerShell(): output window end\n"));
                                    if(ulLastLineDisplayedOffset)
                                    {
                                        ulLastLineDisplayedOffset = 0;

                                        PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);
                                    }
                                }
                                else if(!bShift && !bControl && !bAlt)
                                {
                                }
                                break;
                            case SCANCODE_UP: // up
                                DPRINT((0,"DebuggerShell(): UP\n"));
                                // memory window
                                if(bAlt)
                                {
                                    DPRINT((0,"DebuggerShell(): data window up\n"));
                                    OldOffset-=0x10;
	                                // display data window
	                                Args.Value[0]=OldSelector;
	                                Args.Value[1]=OldOffset;
	                                Args.Count=2;
	                                DisplayMemory(&Args);
                                }
                                // output window
                                else if(bShift)
                                {
                                    DPRINT((0,"DebuggerShell(): output window up ulLastLineDisplayedOffset = %u\n",ulLastLineDisplayedOffset));

                                    if(ulLastLineDisplayedOffset+wWindow[OUTPUT_WINDOW].cy < LinesInRingBuffer())
                                    {
                                        ulLastLineDisplayedOffset += 1;

                                        PrintRingBufferOffset(wWindow[OUTPUT_WINDOW].cy-1,ulLastLineDisplayedOffset);
                                    }
                                }
                                // source window up
                                else if(bControl)
                                {
                                    if((ulCurrentlyDisplayedLineNumber-1)>0 && PICE_strlen(szCurrentFile) )
                                    {
                                        PICE_SYMBOLFILE_SOURCE* pSrc;

                                        if(ConvertTokenToSrcFile(szCurrentFile,(PULONG)&pSrc) )
                                        {
                                            ulCurrentlyDisplayedLineNumber--;
                                            DisplaySourceFile((LPSTR)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE),
                                                              (LPSTR)pSrc+pSrc->ulOffsetToNext,
                                                              ulCurrentlyDisplayedLineNumber,-1);
                                        }
                                    }
                                    else
                                    {
                                        UnassembleOneLineUp();
                                    }
                                }
                                // command line history
                                else if(!bShift && !bControl && !bAlt)
                                {
                                    LPSTR pCurrentCmd;
                                    ULONG len;

                                    DPRINT((0,"DebuggerShell(): command line up\n"));

                                    // only if anything in history
                                    if(GetLinesInCommandHistory())
                                    {
                                        // go to next entry in history
                                        if(ulCommandCurrentPos)
                                            ulCommandCurrentPos = (ulCommandCurrentPos-1)%GetLinesInCommandHistory();
                                        else
                                            ulCommandCurrentPos = GetLinesInCommandHistory()-1;
                                        DPRINT((0,"DebuggerShell(): current history pos = %u\n",ulCommandCurrentPos));
                                        // get this entry
                                        pCurrentCmd = GetFromCommandLineHistory(ulCommandCurrentPos);
                                        // if it has a string attached
                                        if((len = PICE_strlen(pCurrentCmd)))
                                        {
                                            // replace the current command line
                                            PICE_sprintf(tempShell,":");
                                            ReplaceRingBufferCurrent(tempShell);
                        					PICE_memset(&ucCommandBuffer,0,sizeof(ucCommandBuffer));
                                            PICE_strcpy(ucCommandBuffer,pCurrentCmd);
            							    usCurrentPosInInputBuffer=len;
                                            Print(OUTPUT_WINDOW,pCurrentCmd);
                                        }
                                    }
                                }
                                break;
                            case SCANCODE_DOWN: // down
                                DPRINT((0,"DebuggerShell(): DOWN\n"));
                                // memory window
                                if(bAlt)
                                {
                                    DPRINT((0,"DebuggerShell(): data window down\n"));
                                    OldOffset+=0x10;
	                                // display data window
	                                Args.Value[0]=OldSelector;
	                                Args.Value[1]=OldOffset;
	                                Args.Count=2;
	                                DisplayMemory(&Args);
                                }
                                // output window
                                else if(bShift)
                                {
                                    DPRINT((0,"DebuggerShell(): output window down ulLastLineDisplayedOffset = %u\n",ulLastLineDisplayedOffset));
                                    if(ulLastLineDisplayedOffset)
                                    {
                                        ulLastLineDisplayedOffset -= 1;

                                        if(!PrintRingBufferOffset(wWindow[OUTPUT_WINDOW].cy-1,ulLastLineDisplayedOffset))
                                        {
                                            ulLastLineDisplayedOffset = 0;
                                            PrintRingBuffer(wWindow[OUTPUT_WINDOW].cy-1);
                                        }
                                    }
                                }
                                // source window down
                                else if(bControl)
                                {
                                    if(ulCurrentlyDisplayedLineNumber>0 && PICE_strlen(szCurrentFile))
                                    {
                                        PICE_SYMBOLFILE_SOURCE* pSrc;

                                        if(ConvertTokenToSrcFile(szCurrentFile,(PULONG)&pSrc) )
                                        {
                                            ulCurrentlyDisplayedLineNumber++;
                                            DisplaySourceFile((LPSTR)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE),
                                                              (LPSTR)pSrc+pSrc->ulOffsetToNext,
                                                              ulCurrentlyDisplayedLineNumber,-1);
                                        }
                                    }
                                    else
                                    {
                                        UnassembleOneLineDown();
                                    }
                                }
                                // command line history
                                else if(!bShift && !bControl && !bAlt)
                                {
                                    LPSTR pCurrentCmd;
                                    ULONG len;

                                    DPRINT((0,"DebuggerShell(): command line down\n"));

                                    // only if anything in history
                                    if(GetLinesInCommandHistory())
                                    {
                                        // go to next entry in history
                                        ulCommandCurrentPos = (ulCommandCurrentPos+1)%(GetLinesInCommandHistory());
                                        DPRINT((0,"DebuggerShell(): current history pos = %u\n",ulCommandCurrentPos));
                                        // get this entry
                                        pCurrentCmd = GetFromCommandLineHistory(ulCommandCurrentPos);
                                        // if it has a string attached
                                        if((len = PICE_strlen(pCurrentCmd)))
                                        {
                                            // replace the current command line
                                            PICE_sprintf(tempShell,":");
                                            ReplaceRingBufferCurrent(tempShell);
                             				PICE_memset(&ucCommandBuffer,0,sizeof(ucCommandBuffer));
                                            PICE_strcpy(ucCommandBuffer,pCurrentCmd);
            							    usCurrentPosInInputBuffer=len;
                                            Print(OUTPUT_WINDOW,pCurrentCmd);
                                        }
                                    }
                                }
                                break;
                            case SCANCODE_LEFT: // left
                                DPRINT((0,"DebuggerShell(): LEFT\n"));
                                // memory window
                                if(bAlt)
                                {
                                    DPRINT((0,"DebuggerShell(): data window left\n"));

                                    OldOffset-=0x1;
	                                // display data window
	                                Args.Value[0]=OldSelector;
	                                Args.Value[1]=OldOffset;
	                                Args.Count=2;
	                                DisplayMemory(&Args);
                                }
                                else if(!bShift && !bControl && !bAlt)
                                {
                                }
                                else if(bControl)
                                {
                                    if(ulWindowOffset > 0)
                                        ulWindowOffset--;
                                    PICE_memset(&Args,0,sizeof(ARGS));
	                                Args.Count=0;
	                                Unassemble(&Args);
                                }
                                break;
                            case SCANCODE_RIGHT: // right
                                // memory window
                                if(bAlt)
                                {
                                    DPRINT((0,"DebuggerShell(): data window right\n"));

                                    OldOffset+=0x1;
	                                // display data window
	                                Args.Value[0]=OldSelector;
	                                Args.Value[1]=OldOffset;
	                                Args.Count=2;
	                                DisplayMemory(&Args);
                                }
                                else if(!bShift && !bControl && !bAlt)
                                {
                                }
                                else if(bControl)
                                {
                                    if(ulWindowOffset < 80)
                                        ulWindowOffset++;
                                    PICE_memset(&Args,0,sizeof(ARGS));
	                                Args.Count=0;
	                                Unassemble(&Args);
                                }
                                break;
                            case SCANCODE_PGUP: // page up
                                DPRINT((0,"DebuggerShell(): PAGEUP\n"));
                                // memory window
                                if(bAlt)
                                {
                                    OldOffset-=wWindow[DATA_WINDOW].cy*0x10;
	                                // display data window
	                                Args.Value[0]=OldSelector;
	                                Args.Value[1]=OldOffset;
	                                Args.Count=2;
	                                DisplayMemory(&Args);
                                }
                                // output window
                                else if(bShift)
                                {
                                    if(ulLastLineDisplayedOffset+2*(wWindow[OUTPUT_WINDOW].cy) < LinesInRingBuffer())
                                    {
                                        ulLastLineDisplayedOffset += (wWindow[OUTPUT_WINDOW].cy);

                                        PrintRingBufferOffset(wWindow[OUTPUT_WINDOW].cy-1,ulLastLineDisplayedOffset);
                                    }
                                    else
                                    {
                                        if(ulLastLineDisplayedOffset != LinesInRingBuffer()-wWindow[OUTPUT_WINDOW].cy)
                                        {
                                            ulLastLineDisplayedOffset = LinesInRingBuffer()-wWindow[OUTPUT_WINDOW].cy;
                                            PrintRingBufferOffset(wWindow[OUTPUT_WINDOW].cy-1,ulLastLineDisplayedOffset);
                                        }
                                    }
                                }
                                // source window page up
                                else if(bControl)
                                {
                                    if(PICE_strlen(szCurrentFile))
                                    {
                                        if((ulCurrentlyDisplayedLineNumber-wWindow[SOURCE_WINDOW].cy)>0)
                                        {
                                            PICE_SYMBOLFILE_SOURCE* pSrc;

                                            if(ConvertTokenToSrcFile(szCurrentFile,(PULONG)&pSrc) )
                                            {
                                                ulCurrentlyDisplayedLineNumber -= wWindow[SOURCE_WINDOW].cy;

                                                DisplaySourceFile((LPSTR)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE),
                                                                  (LPSTR)pSrc+pSrc->ulOffsetToNext,
                                                                  ulCurrentlyDisplayedLineNumber ,-1);
                                            }
                                        }
                                        else
                                        {
                                            PICE_SYMBOLFILE_SOURCE* pSrc;

                                            if(ConvertTokenToSrcFile(szCurrentFile,(PULONG)&pSrc) )
                                            {
                                                ulCurrentlyDisplayedLineNumber = 1;

                                                DisplaySourceFile((LPSTR)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE),
                                                                  (LPSTR)pSrc+pSrc->ulOffsetToNext,
                                                                  ulCurrentlyDisplayedLineNumber ,-1);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        UnassembleOnePageUp(wWindow[SOURCE_WINDOW].cy);
                                    }

                                }
                                else if(!bShift && !bControl && !bAlt)
                                {
                                }
                                break;
                            case SCANCODE_PGDN: // page down
                                DPRINT((0,"DebuggerShell(): PAGEDOWN\n"));
                                // memory window
                                if(bAlt)
                                {
                                    OldOffset+=wWindow[DATA_WINDOW].cy*0x10;
	                                // display data window
	                                Args.Value[0]=OldSelector;
	                                Args.Value[1]=OldOffset;
	                                Args.Count=2;
	                                DisplayMemory(&Args);
                                }
                                else if(bShift)
                                {
                                    if(ulLastLineDisplayedOffset>wWindow[OUTPUT_WINDOW].cy)
                                    {
                                        ulLastLineDisplayedOffset -= (wWindow[OUTPUT_WINDOW].cy);

                                        PrintRingBufferOffset(wWindow[OUTPUT_WINDOW].cy-1,ulLastLineDisplayedOffset);
                                    }
                                    else
                                    {
                                        if(ulLastLineDisplayedOffset)
                                        {
                                            ulLastLineDisplayedOffset = 0;
                                            PrintRingBufferOffset(wWindow[OUTPUT_WINDOW].cy-1,ulLastLineDisplayedOffset);
                                        }
                                    }
                                }
                                else if(bControl)
                                {
                                    if(PICE_strlen(szCurrentFile) )
                                    {
                                        if((ulCurrentlyDisplayedLineNumber+wWindow[SOURCE_WINDOW].cy)>0)
                                        {
                                            PICE_SYMBOLFILE_SOURCE* pSrc;

                                            if(ConvertTokenToSrcFile(szCurrentFile,(PULONG)&pSrc) )
                                            {
                                                ulCurrentlyDisplayedLineNumber += wWindow[SOURCE_WINDOW].cy;

                                                DisplaySourceFile((LPSTR)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE),
                                                                  (LPSTR)pSrc+pSrc->ulOffsetToNext,
                                                                  ulCurrentlyDisplayedLineNumber ,-1);
                                            }
                                        }
                                        else
                                        {
                                            PICE_SYMBOLFILE_SOURCE* pSrc;

                                            if(ConvertTokenToSrcFile(szCurrentFile,(PULONG)&pSrc) )
                                            {
                                                ulCurrentlyDisplayedLineNumber = 1;

                                                DisplaySourceFile((LPSTR)pSrc+sizeof(PICE_SYMBOLFILE_SOURCE),
                                                                  (LPSTR)pSrc+pSrc->ulOffsetToNext,
                                                                  ulCurrentlyDisplayedLineNumber ,-1);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        UnassembleOnePageDown(wWindow[SOURCE_WINDOW].cy);
                                    }
                                }
                                else if(!bShift && !bControl && !bAlt)
                                {
                                }
                                break;
                        }
                    }
				}
			}
			ucKeyPressedWhileIdle=0;
		}
	}

    SaveOldRegs();

    PrintLogo(TRUE);

    ShowRunningMsg();

    if(bRev)
		PrintCursor(TRUE);

	// hide the cursor
	HideCursor();

    FlushKeyboardQueue();

    RestoreGraphicsState();

	LEAVE_FUNC();
}

//*************************************************************************
// RealIsr()
//
//*************************************************************************
void RealIsr(ULONG dwReasonForBreak)
{
  BOOLEAN ReinstallPermanentBp = FALSE;

	DPRINT((0,"reason: %u#################################################################\n", dwReasonForBreak));
    ENTER_FUNC();

    // in handler
	bInDebuggerShell = TRUE;

    bStepping = FALSE;

	// don't assume we must call original handlers yet
    dwCallOldInt1Handler = dwCallOldInt3Handler = dwCallOldIntEHandler = dwCallOldGPFaultHandler = 0;
	bSkipMainLoop = FALSE;
    bEnterNow = FALSE;

    // reset trace flag (TF) on the stack
    CurrentEFL&=(~0x100);

    InstallPrintkHook();

    // control is not depressed
	bControl=FALSE;

    bIrqStateAtBreak = ((CurrentEFL&(1<<9))!=0);

	DPRINT((0,"\nbInDebuggerShell %x, dwReasonForBreak: %x, bIrqStateAtBreak: %d\n", bInDebuggerShell, dwReasonForBreak, bIrqStateAtBreak));
	DPRINT((0,"CurrentEIP: %x, CurrentESP: %x\n", CurrentEIP, CurrentESP));

    // came in because TF flag was set
	if(dwReasonForBreak == REASON_SINGLESTEP)
	{
		ULONG ulAddress,ulAddressCurrent;

        DPRINT((0,"REASON_SINGLESTEP: bSingleStep: %u\n", bSingleStep));

        if(!bSingleStep)
        {
            dwCallOldInt1Handler = 1;
            DPRINT((0,"no single step requested: %u!\n", dwCallOldInt1Handler));
            goto common_return_point;
        }

		ulAddress = GetLinearAddress(OldCS,OldEIP);
		ulAddressCurrent = GetLinearAddress(CurrentCS,CurrentEIP);

		// if we came in because we needed to skip past a permanent
		// INT3 hook, we need to put the INT3 back in place and
		// simply restart the system.
        if(NeedToReInstallSWBreakpoints(ulAddress,TRUE) )
        {
            DPRINT((0,"reinstalling INT3 @ %.4X:%.8X\n",OldCS,OldEIP));

            ReInstallSWBreakpoint(ulAddress);

            // previous command was go i.e. we did not single-step over a location
			// where a permanent breakpoint was installed (Printk() etc.) we simply restart
			// else we must stop the system.
            if(bPreviousCommandWasGo)
            {
                bPreviousCommandWasGo = FALSE;
				bInDebuggerShell = FALSE;

                if(bStepThroughSource)
                {
                    // set TF flag
                    CurrentEFL |= 0x100;
                }

            	LEAVE_FUNC();
				DPRINT((0,"singlestep-----------------------------------------------------------------\n"));
                return;
            }
            bPreviousCommandWasGo = FALSE;
        }

        if(IsSwBpAtAddressInstalled(ulAddressCurrent))
			DeInstallSWBreakpoint(ulAddressCurrent);

        // we came here while stepping through source code block
        if(bStepThroughSource)
        {
            ULONG ulLineNumber;
            LPSTR pSrc,pFileName;

            DPRINT((0,"RealIsr(): stepping through source!\n"));

            // look up the corresponding source line
            // if there isn't any or the source line number has changed
            // we break back into the debugger
			if(bShowSrc)
				pSrc = FindSourceLineForAddress(ulAddressCurrent,&ulLineNumber,NULL,NULL,&pFileName);
			else pSrc = NULL;

            DPRINT((0,"RealIsr(): line #%u pSrc=%x (old line #%u)\n",ulLineNumber,(ULONG)pSrc,g_ulLineNumberStart));

            // if we have found a source line there
            if(pSrc && ulLineNumber==g_ulLineNumberStart)
            {
                DPRINT((0,"RealIsr(): stepping through line #%u in file = %s!\n",ulLineNumber,pFileName));

                if(bStepInto)
                    StepInto(NULL);
                else
                    StepOver(NULL);

			    bInDebuggerShell = FALSE;
            	LEAVE_FUNC();
				DPRINT((0,"singstep-----------------------------------------------------------------\n"));
                return;
            }
            bStepThroughSource = FALSE;
            bNotifyToExit = FALSE;
            bSkipMainLoop = FALSE;
        }
	}
    // came in because hardware register triggered a breakpoint
	else if(dwReasonForBreak == REASON_HARDWARE_BP)
	{
        ULONG ulReason;

        DPRINT((0,"REASON_HARDWARE_BP\n"));

        // disable HW breakpoints
		__asm__("\n\t \
            movl %%dr6,%%eax\n\t \
            movl %%eax,%0\n\t \
			xorl %%eax,%%eax\n\t \
			movl %%eax,%%dr6\n\t \
			movl %%eax,%%dr7"
			:"=m" (ulReason)
            :
            :"eax"
			);

        DPRINT((0,"REASON_HARDWARE_BP: %x\n",(ulReason&0xF)));

        // HW breakpoint DR1 (skip: only used in init_module detection)
        if(ulReason&0x2)
        {
            CurrentEFL |=(1<<16); // set resume flag

            bSkipMainLoop = TRUE;

			TryToInstallVirtualSWBreakpoints();
        }
        // HW breakpoint DR0
        else if(ulReason&0x1)
        {
    		ULONG ulAddressCurrent;

    		ulAddressCurrent = GetLinearAddress(CurrentCS,CurrentEIP);

            // we came here while stepping through source code block
            if(bStepThroughSource)
            {
                ULONG ulLineNumber;
                LPSTR pSrc,pFileName;

                DPRINT((0,"RealIsr(): stepping through source! [2]\n"));

                // look up the corresponding source line
                // if there isn't any or the source line number has changed
                // we break back into the debugger
				if(bShowSrc)
		            pSrc = FindSourceLineForAddress(ulAddressCurrent,&ulLineNumber,NULL,NULL,&pFileName);
				else
					pSrc = NULL;

                DPRINT((0,"RealIsr(): line #%u pSrc=%x (old line #%u) [2]\n",ulLineNumber,(ULONG)pSrc,g_ulLineNumberStart));

                // if we have found a source line there
                if(pSrc && ulLineNumber==g_ulLineNumberStart)
                {
                    DPRINT((0,"RealIsr(): stepping through line #%u in file = %s! [2]\n",ulLineNumber,pFileName));

                    if(bStepInto)
                        StepInto(NULL);
                    else
                        StepOver(NULL);

			        bInDebuggerShell = FALSE;
                    LEAVE_FUNC();
					DPRINT((0,"rrr-----------------------------------------------------------------\n"));
                    return;
                }
                bNotifyToExit = FALSE;
                bSkipMainLoop = FALSE;
                bStepThroughSource = FALSE;
            }
        }
	}
	else if(dwReasonForBreak==REASON_INT3)
	{
		ULONG ulAddress;

        DPRINT((0,"REASON_INT3\n"));

		// must subtract one cause INT3s are generated after instructions execution
        CurrentEIP--;

        // make a flat address
		ulAddress = GetLinearAddress(CurrentCS,CurrentEIP);

        DPRINT((0,"INT3 @ %.8X\n",ulAddress));

        // if there's a breakpoint installed at current EIP remove it
        if(DeInstallSWBreakpoint(ulAddress) )
        {
            PSW_BP p;

			DPRINT((0,"INT3 @ %.8X removed\n",ulAddress));

            // if it's permanent (must be Printk() ) skip the DebuggerShell() and
            // do a callback
            if( (p = IsPermanentSWBreakpoint(ulAddress)) )
            {
    			DPRINT((0,"permanent breakpoint\n"));

                ReinstallPermanentBp = TRUE;

                OldCS = CurrentCS;
                OldEIP = CurrentEIP;

                bSkipMainLoop = TRUE;
				DPRINT((0,"callback at %x\n",p->Callback));
                if(p->Callback)
                    p->Callback();
            }
            else
            {
                LPSTR pFind;
                if(ScanExportsByAddress(&pFind,GetLinearAddress(CurrentCS,CurrentEIP)))
                {
			        PICE_sprintf(tempShell,"pICE: SW Breakpoint at %s (%.4X:%.8X)\n",pFind,CurrentCS,CurrentEIP);
                }
                else
                {
			        PICE_sprintf(tempShell,"pICE: SW Breakpoint at %.4X:%.8X\n",CurrentCS,CurrentEIP);
                }
			    Print(OUTPUT_WINDOW,tempShell);
            }
            CurrentEFL &= ~(1<<16); // clear resume flag
        }
        else
        {
            LPSTR pFind;
			PEPROCESS my_current = IoGetCurrentProcess();

			DPRINT((0,"can't deinstall, somebody else's breakpoint\n"));


            // if no other debugger is running on this process and the address is
            // above TASK_SIZE we assume this to be a hard embedded INT3
/*
#if REAL_LINUX_VERSION_CODE < 0x020400
            if(ulAddress<TASK_SIZE && !(my_current->flags & PF_PTRACED) )
#else
            if(ulAddress<TASK_SIZE && !(my_current->ptrace & PT_PTRACED) )
#endif
*/
			if( ulAddress )
            {
                if(ScanExportsByAddress(&pFind,GetLinearAddress(CurrentCS,CurrentEIP)))
                {
			        PICE_sprintf(tempShell,"pICE: break due to embedded INT 3 at %s (%.4X:%.8X)\n",pFind,CurrentCS,CurrentEIP);
                }
                else
                {
			        PICE_sprintf(tempShell,"pICE: break due to embedded INT 3 at user-mode address %.4X:%.8X\n",CurrentCS,CurrentEIP);
                }
			    Print(OUTPUT_WINDOW,tempShell);
	            CurrentEFL &= ~(1<<16); // clear resume flag
            }
            // well someone is already debugging this, we must pass the INT3 on to old handler
            // but only when it's a user-mode address
/*
            else
            {
                if(ulAddress<TASK_SIZE || !bInt3Here)
                {
			        DPRINT((0,"SW Breakpoint but debugged by other process at %.4X:%.8X\n",CurrentCS,CurrentEIP));
                    // call the old handler on return from RealIsr()
                    dwCallOldInt3Handler = 1;
                    // and skip DebuggerShell()
	                bSkipMainLoop = TRUE;
                }
                else
                {
                    if(ScanExportsByAddress(&pFind,GetLinearAddress(CurrentCS,CurrentEIP)))
                    {
	    		        PICE_sprintf(tempShell,"pICE: break due to embedded INT 3 at (%s) %.4X:%.8X\n",
                                     pFind,CurrentCS,CurrentEIP);
                    }
                    else
                    {
	    		        PICE_sprintf(tempShell,"pICE: break due to embedded INT 3 at kernel-mode address %.4X:%.8X\n",
                                     CurrentCS,CurrentEIP);
                    }
			        Print(OUTPUT_WINDOW,tempShell);
	                CurrentEFL &= ~(1<<16); // clear resume flag
                }
            }
*/
            // skip INT3
            CurrentEIP++;
        }
	}
	else if(dwReasonForBreak == REASON_PAGEFAULT)
	{
        LPSTR pSymbolName;

        DPRINT((0,"REASON_PAGEFAULT\n"));

        if( ScanExportsByAddress(&pSymbolName,GetLinearAddress(CurrentCS,CurrentEIP)) )
        {
		    PICE_sprintf(tempShell,"pICE: Breakpoint due to page fault at %.4X:%.8X (%s)\n",CurrentCS,CurrentEIP,pSymbolName);
        }
        else
        {
		    PICE_sprintf(tempShell,"pICE: Breakpoint due to page fault at %.4X:%.8X\n",CurrentCS,CurrentEIP);
        }
		Print(OUTPUT_WINDOW,tempShell);
		PICE_sprintf(tempShell,"pICE: memory referenced %x\n",CurrentCR2);
        Print(OUTPUT_WINDOW,tempShell);
        dwCallOldIntEHandler = 1;
	}
	else if(dwReasonForBreak == REASON_GP_FAULT)
	{
        LPSTR pSymbolName;

        DPRINT((0,"REASON_GPFAULT\n"));

        if( ScanExportsByAddress(&pSymbolName,GetLinearAddress(CurrentCS,CurrentEIP)) )
        {
    		PICE_sprintf(tempShell,"pICE: Breakpoint due to general protection fault at %.4X:%.8X (%s)\n",CurrentCS,CurrentEIP,pSymbolName);
        }
        else
        {
    		PICE_sprintf(tempShell,"pICE: Breakpoint due to general protection fault at %.4X:%.8X\n",CurrentCS,CurrentEIP);
        }
		Print(OUTPUT_WINDOW,tempShell);
        dwCallOldGPFaultHandler = 1;
	}
	else if(dwReasonForBreak == REASON_CTRLF)
	{
        DPRINT((0,"REASON_CTRLF\n"));
        // nothing to do
    }
    else if(dwReasonForBreak == REASON_DOUBLE_FAULT)
    {
        DPRINT((0,"REASON_DOUBLE_FAULT\n"));

        PICE_sprintf(tempShell,"pICE: Breakpoint due to double fault at %.4X:%.8X\n",CurrentCS,CurrentEIP);
		Print(OUTPUT_WINDOW,tempShell);
    }
    else if(dwReasonForBreak == REASON_INTERNAL_ERROR)
    {
        DPRINT((0,"REASON_INTERNAL_ERROR\n"));

        Print(OUTPUT_WINDOW,"pICE: Please report this error to klauspg@diamondmm.com!\n");
//        Print(OUTPUT_WINDOW,"pICE: !!! SYSTEM HALTED !!!\n");
//        __asm__ __volatile__("hlt");
    }
    else
    {
        DPRINT((0,"REASON_UNKNOWN\n"));

        PICE_sprintf(tempShell,"pICE: Breakpoint due to unknown reason at %.4X:%.8X (code %x)\n",CurrentCS,CurrentEIP,dwReasonForBreak);
		Print(OUTPUT_WINDOW,tempShell);
        Print(OUTPUT_WINDOW,"pICE: Please report this error to klauspg@diamondmm.com!\n");
        Print(OUTPUT_WINDOW,"pICE: !!! SYSTEM HALTED !!!\n");
        __asm__ __volatile__("hlt");
    }

    // we don't single-step yet
    DPRINT((0,"RealIsr(): not stepping yet\n"));
	bSingleStep=FALSE;

    // process commands
    if(bSkipMainLoop == FALSE)
	{
        DPRINT((0,"RealIsr(): saving registers\n"));
	    // save the extended regs
	    __asm__ __volatile__
	    ("\n\t \
            pushl %eax\n\t \
		    movw %es,%ax\n\t \
		    movw %ax,_CurrentES\n\t \
		    //movw %fs,%ax\n\t \
		    //movw %ax,_CurrentFS\n\t \
		    movw %gs,%ax\n\t \
		    movw %ax,_CurrentGS\n\t \
		    movl %dr0,%eax\n\t \
		    movl %eax,_CurrentDR0\n\t \
		    movl %dr1,%eax\n\t \
		    movl %eax,_CurrentDR1\n\t \
		    movl %dr2,%eax\n\t \
		    movl %eax,_CurrentDR2\n\t \
		    movl %dr3,%eax\n\t \
		    movl %eax,_CurrentDR3\n\t \
		    movl %dr6,%eax\n\t \
		    movl %eax,_CurrentDR6\n\t \
		    movl %dr7,%eax\n\t \
		    movl %eax,_CurrentDR7\n\t \
		    movl %cr0,%eax\n\t \
		    movl %eax,_CurrentCR0\n\t \
		    movl %cr2,%eax\n\t \
		    movl %eax,_CurrentCR2\n\t \
		    movl %cr3,%eax\n\t \
		    movl %eax,_CurrentCR3\n\t \
            popl %eax"
	    );

		CurrentFS = OLD_PCR;
        DPRINT((0,"RealIsr(): adding colon to output()\n"));
        Print(OUTPUT_WINDOW,":");

        DPRINT((0,"RealIsr(): calling DebuggerShell()\n"));
        DebuggerShell();
	}

	// if there was a SW breakpoint at CS:EIP
    if(NeedToReInstallSWBreakpoints(GetLinearAddress(CurrentCS,CurrentEIP),TRUE) || ReinstallPermanentBp)
    {
        DPRINT((0,"need to reinstall INT3\n"));
		// remember how we restarted last time
        bPreviousCommandWasGo = !bSingleStep;
        // do a single step to reinstall breakpoint
	    // modify trace flag
	    CurrentEFL|=0x100; // set trace flag (TF)

	    bSingleStep=TRUE;
	    bNotifyToExit=TRUE;
    }

common_return_point:

    // reset the global flags
    bNotifyToExit = FALSE;
    bSkipMainLoop = FALSE;

    // not in handler anymore
    bInDebuggerShell = FALSE;

    LEAVE_FUNC();
	DPRINT((0,"common return-----------------------------------------------------------------\n"));
}


__asm__(".global NewInt31Handler\n\t \
NewInt31Handler:\n\t \
	cli\n\t \
    cld\n\t \
\n\t \
	pushl %eax\n\t \
	pushl %ds\n\t \
\n\t \
	movw %ss,%ax\n\t \
	mov %ax,%ds\n\t \
\n\t \
	mov 0x4(%esp),%eax\n\t \
	movl %eax,_CurrentEAX\n\t \
	movl %ebx,_CurrentEBX\n\t \
	movl %ecx,_CurrentECX\n\t \
	movl %edx,_CurrentEDX\n\t \
	movl %esi,_CurrentESI\n\t \
	movl %edi,_CurrentEDI\n\t \
	movl %ebp,_CurrentEBP\n\t \
	movl (%esp),%eax\n\t \
	movw %ax,_CurrentDS\n\t \
\n\t \
    // test for V86 mode\n\t \
	testl $0x20000,5*4(%esp)\n\t \
	jz notV86\n\t \
\n\t \
	int $0x03\n\t \
\n\t \
notV86:\n\t \
    // test if stack switched (ring3->ring0 transition)\n\t \
    // stack is switched if orig. SS is not global kernel code segment\n\t \
    movl 4*4(%esp),%eax\n\t \
    cmpw $" STR(GLOBAL_CODE_SEGMENT) ",%ax\n\t \
	je notswitched\n\t \
\n\t \
	// switched stack\n\t \
	movl 6*4(%esp),%eax\n\t \
	mov %eax,_CurrentESP\n\t \
	mov 7*4(%esp),%eax\n\t \
	movzwl %ax,%eax\n\t \
	mov %ax,_CurrentSS\n\t \
	jmp afterswitch\n\t \
\n\t \
notswitched:\n\t \
    // didn't switch stack\n\t \
	movl %esp,_CurrentESP\n\t \
	addl $24,_CurrentESP\n\t \
	movw %ss,%ax\n\t \
	movzwl %ax,%eax\n\t \
	mov %ax,_CurrentSS\n\t \
\n\t \
afterswitch:\n\t \
    // save EIP\n\t \
	mov 3*4(%esp),%eax\n\t \
	mov %eax,_CurrentEIP\n\t \
    //save CS\n\t \
	mov 4*4(%esp),%eax\n\t \
	movzwl %ax,%eax\n\t \
	movw %ax,_CurrentCS\n\t \
    // save flags\n\t \
	movl 5*4(%esp),%eax\n\t \
	andl $0xFFFFFEFF,%eax\n\t \
	movl %eax,_CurrentEFL\n\t \
\n\t \
	pushal\n\t \
\n\t \
    // get reason code\n\t \
    mov 0x28(%esp),%ebx\n\t \
\n\t \
	/*\n\t \
	 * Load the PCR selector.\n\t \
	 */\n\t \
\n\t \
	movl 	%fs, %eax\n\t \
	movl	%eax, _OLD_PCR\n\t \
	movl	_PCR_SEL, %eax\n\t \
	movl	%eax, %fs\n\t \
\n\t \
    // setup a large work stack\n\t \
	movl %esp,%eax\n\t \
	movl %eax,_ulRealStackPtr\n\t \
\n\t \
    pushl %ebx\n\t \
	call _RealIsr\n\t \
    addl $4,%esp\n\t \
\n\t \
	pushl 	%eax\n\t \
	movl	_OLD_PCR, %eax\n\t \
	movl	%eax, %fs\n\t \
	popl	%eax\n\t \
\n\t \
	// restore all regs\n\t \
	popal\n\t \
\n\t \
	// do an EOI to IRQ controller (because we definitely pressed some key)\n\t \
	// TODO: SMP APIC support\n\t \
	movb $0x20,%al\n\t \
	outb %al,$0x20\n\t \
\n\t \
	popl %ds\n\t \
	popl %eax\n\t \
\n\t \
    // remove reason code\n\t \
    addl $4,%esp\n\t \
\n\t \
    // make EAX available\n\t \
	pushl %eax\n\t \
\n\t \
	// modify or restore EFLAGS\n\t \
	.byte 0x2e\n\t \
	mov _CurrentEFL,%eax\n\t \
	mov %eax,3*4(%esp)\n\t \
	.byte 0x2e\n\t \
	movzwl _CurrentCS,%eax\n\t \
	mov %eax,2*4(%esp)\n\t \
	.byte 0x2e\n\t \
	mov _CurrentEIP,%eax\n\t \
	mov %eax,1*4(%esp)\n\t \
\n\t \
    // restore EAX\n\t \
	popl %eax\n\t \
\n\t \
	// do we need to call old INT1 handler\n\t \
    .byte 0x2e\n\t \
     cmp $0,_dwCallOldInt1Handler\n\t \
     je do_iret2\n\t \
\n\t \
    // call INT3 handler\n\t \
    .byte 0x2e\n\t \
     jmp *_OldInt1Handler\n\t \
\n\t \
do_iret2:\n\t \
    // do we need to call old INT3 handler\n\t \
    .byte 0x2e\n\t \
    cmp $0,_dwCallOldInt3Handler\n\t \
    je do_iret1\n\t \
\n\t \
    // call INT3 handler\n\t \
    .byte 0x2e\n\t \
    jmp *_OldInt3Handler\n\t \
\n\t \
do_iret1:\n\t \
    // do we need to call old pagefault handler\n\t \
    .byte 0x2e\n\t \
    cmp $0,_dwCallOldIntEHandler\n\t \
    je do_iret3\n\t \
\n\t \
    // call old pagefault handler\n\t \
	.byte 0x2e\n\t \
    pushl _error_code\n\t \
	.byte 0x2e\n\t \
    jmp *_OldIntEHandler\n\t \
\n\t \
do_iret3:\n\t \
    // do we need to call old general protection fault handler\n\t \
    .byte 0x2e\n\t \
    cmp $0,_dwCallOldGPFaultHandler\n\t \
    je do_iret\n\t \
\n\t \
    // call old pagefault handler\n\t \
	.byte 0x2e\n\t \
    pushl _error_code\n\t \
	.byte 0x2e\n\t \
    jmp *_OldGPFaultHandler\n\t \
\n\t \
do_iret:\n\t \
	//ei\n\t \
	//int3\n\t \
	iretl ");

//
// stub for entering via CTRL-F
//
// IDTs keyboard IRQ points here
//
__asm__ ("\n\t \
NewGlobalInt31Handler:\n\t \
		.byte 0x2e\n\t \
		cmpb $0,_bEnterNow\n\t \
		jne dotheenter\n\t \
\n\t \
        // chain to old handler\n\t \
		.byte 0x2e\n\t \
		jmp *_OldGlobalInt31Handler\n\t \
\n\t \
dotheenter:\n\t \
        pushl $" STR(REASON_CTRLF) "\n\t \
        jmp NewInt31Handler "
);

void InstallGlobalKeyboardHook(void)
{
	ULONG LocalNewGlobalInt31Handler;

	ENTER_FUNC();

	MaskIrqs();
	if(!OldGlobalInt31Handler)
	{
		__asm__("mov $NewGlobalInt31Handler,%0"
			:"=r" (LocalNewGlobalInt31Handler)
			:
			:"eax");
		OldGlobalInt31Handler=SetGlobalInt(KeyboardIRQL,(ULONG)LocalNewGlobalInt31Handler);
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}

void DeInstallGlobalKeyboardHook(void)
{
    ENTER_FUNC();

	MaskIrqs();
	if(OldGlobalInt31Handler)
	{
		SetGlobalInt(KeyboardIRQL,(ULONG)OldGlobalInt31Handler);
		OldGlobalInt31Handler=0;
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}


