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
                        __asm__ __volatile__("
                            movl %2,%%eax
                            movl %%esp,%%ebx
                            mov  %%ebx,%0
                            leal _aulNewStack,%%ebx
                            addl $0x1FFF0,%%ebx
                            movl %%ebx,%%esp
                            pushl $0
                            pushl %%eax
                            call _Parse
                            movl %0,%%ebx
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
                            __asm__ __volatile__("
                                movl %2,%%eax
                                movl %%esp,%%ebx
                                mov  %%ebx,%0
                                leal _aulNewStack,%%ebx
                                addl $0x1FFF0,%%ebx
                                movl %%ebx,%%esp
                                pushl $1
                                pushl %%eax
                                call _Parse
                                movl %0,%%ebx
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
		__asm__("
            movl %%dr6,%%eax
            movl %%eax,%0
			xorl %%eax,%%eax
			movl %%eax,%%dr6
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
	    ("
            pushl %eax
		    movw %es,%ax
		    movw %ax,_CurrentES
		    //movw %fs,%ax
		    //movw %ax,_CurrentFS
		    movw %gs,%ax
		    movw %ax,_CurrentGS
		    movl %dr0,%eax
		    movl %eax,_CurrentDR0
		    movl %dr1,%eax
		    movl %eax,_CurrentDR1
		    movl %dr2,%eax
		    movl %eax,_CurrentDR2
		    movl %dr3,%eax
		    movl %eax,_CurrentDR3
		    movl %dr6,%eax
		    movl %eax,_CurrentDR6
		    movl %dr7,%eax
		    movl %eax,_CurrentDR7
		    movl %cr0,%eax
		    movl %eax,_CurrentCR0
		    movl %cr2,%eax
		    movl %eax,_CurrentCR2
		    movl %cr3,%eax
		    movl %eax,_CurrentCR3
            popl %eax"
	    );

		CurrentFS = OLD_PCR;
        DPRINT((0,"RealIsr(): adding colon to output()\n"));
        Print(OUTPUT_WINDOW,":");

        DPRINT((0,"RealIsr(): calling DebuggerShell()\n"));
        DebuggerShell();
	}

	// if there was a SW breakpoint at CS:EIP
    if(NeedToReInstallSWBreakpoints(GetLinearAddress(CurrentCS,CurrentEIP),TRUE))
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


__asm__(".global NewInt31Handler
NewInt31Handler:
	cli
    cld

	pushl %eax
	pushl %ds

	movw %ss,%ax
	mov %ax,%ds

	mov 0x4(%esp),%eax
	movl %eax,_CurrentEAX
	movl %ebx,_CurrentEBX
	movl %ecx,_CurrentECX
	movl %edx,_CurrentEDX
	movl %esi,_CurrentESI
	movl %edi,_CurrentEDI
	movl %ebp,_CurrentEBP
	movl (%esp),%eax
	movw %ax,_CurrentDS

    // test for V86 mode
	testl $0x20000,5*4(%esp)
	jz notV86

	int $0x03

notV86:
    // test if stack switched (ring3->ring0 transition)
    // stack is switched if orig. SS is not global kernel code segment
    movl 4*4(%esp),%eax
    cmpw $" STR(GLOBAL_CODE_SEGMENT) ",%ax
	je notswitched

	// switched stack
	movl 6*4(%esp),%eax
	mov %eax,_CurrentESP
	mov 7*4(%esp),%eax
	movzwl %ax,%eax
	mov %ax,_CurrentSS
	jmp afterswitch

notswitched:
    // didn't switch stack
	movl %esp,_CurrentESP
	addl $24,_CurrentESP
	movw %ss,%ax
	movzwl %ax,%eax
	mov %ax,_CurrentSS

afterswitch:
    // save EIP
	mov 3*4(%esp),%eax
	mov %eax,_CurrentEIP
    //save CS
	mov 4*4(%esp),%eax
	movzwl %ax,%eax
	movw %ax,_CurrentCS
    // save flags
	movl 5*4(%esp),%eax
	andl $0xFFFFFEFF,%eax
	movl %eax,_CurrentEFL

	pushal

    // get reason code
    mov 0x28(%esp),%ebx

	/*
	 * Load the PCR selector.
	 */

	movl 	%fs, %eax
	movl	%eax, _OLD_PCR
	movl	_PCR_SEL, %eax
	movl	%eax, %fs

    // setup a large work stack
	movl %esp,%eax
	movl %eax,_ulRealStackPtr

    pushl %ebx
	call _RealIsr
    addl $4,%esp

	pushl 	%eax
	movl	_OLD_PCR, %eax
	movl	%eax, %fs
	popl	%eax

	// restore all regs
	popal

	// do an EOI to IRQ controller (because we definitely pressed some key)
	// TODO: SMP APIC support
	movb $0x20,%al
	outb %al,$0x20

	popl %ds
	popl %eax

    // remove reason code
    addl $4,%esp

    // make EAX available
	pushl %eax

	// modify or restore EFLAGS
	.byte 0x2e
	mov _CurrentEFL,%eax
	mov %eax,3*4(%esp)
	.byte 0x2e
	movzwl _CurrentCS,%eax
	mov %eax,2*4(%esp)
	.byte 0x2e
	mov _CurrentEIP,%eax
	mov %eax,1*4(%esp)

    // restore EAX
	popl %eax

	// do we need to call old INT1 handler
    .byte 0x2e
     cmp $0,_dwCallOldInt1Handler
     je do_iret2

    // call INT3 handler
    .byte 0x2e
     jmp *_OldInt1Handler

do_iret2:
    // do we need to call old INT3 handler
    .byte 0x2e
    cmp $0,_dwCallOldInt3Handler
    je do_iret1

    // call INT3 handler
    .byte 0x2e
    jmp *_OldInt3Handler

do_iret1:
    // do we need to call old pagefault handler
    .byte 0x2e
    cmp $0,_dwCallOldIntEHandler
    je do_iret3

    // call old pagefault handler
	.byte 0x2e
    pushl _error_code
	.byte 0x2e
    jmp *_OldIntEHandler

do_iret3:
    // do we need to call old general protection fault handler
    .byte 0x2e
    cmp $0,_dwCallOldGPFaultHandler
    je do_iret

    // call old pagefault handler
	.byte 0x2e
    pushl _error_code
	.byte 0x2e
    jmp *_OldGPFaultHandler

do_iret:
	//ei
	//int3
	iretl ");

//
// stub for entering via CTRL-F
//
// IDTs keyboard IRQ points here
//
__asm__ ("
NewGlobalInt31Handler:
		.byte 0x2e
		cmpb $0,_bEnterNow
		jne dotheenter

        // chain to old handler
		.byte 0x2e
		jmp *_OldGlobalInt31Handler

dotheenter:
        pushl $" STR(REASON_CTRLF) "
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


