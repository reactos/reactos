/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    hardwar.h

Abstract:

    HEADER for hardware.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
typedef struct tagWindow
{
	USHORT y,cy;
	USHORT usCurX,usCurY;
	BOOLEAN bScrollDisabled;
}WINDOW,*PWINDOW;

// pointer indirection table for output functions
typedef struct _OUTPUT_HANDLERS
{
    void    (*CopyLineTo)               (USHORT dest,USHORT src);
    void    (*PrintGraf)                (ULONG x,ULONG y,UCHAR c);
    void    (*Flush)                    (void);
    void    (*ClrLine)                  (ULONG line);
    void    (*InvertLine)               (ULONG line);
    void    (*HatchLine)                (ULONG line);
    void    (*PrintLogo)                (BOOLEAN bShow);
    void    (*PrintCursor)              (BOOLEAN bForce);
    void    (*SaveGraphicsState)        (void);
    void    (*RestoreGraphicsState)     (void);
    void    (*ShowCursor)               (void);
    void    (*HideCursor)               (void);
    void    (*SetForegroundColor)       (ECOLORS);
    void    (*SetBackgroundColor)       (ECOLORS);
}OUTPUT_HANDLERS,*POUTPUT_HANDLERS;

// pointer indirection table for input functions
typedef struct _INPUT_HANDLERS
{
    UCHAR   (*GetKeyPolled)             (void);
    void    (*FlushKeyboardQueue)       (void);
}INPUT_HANDLERS,*PINPUT_HANDLERS;

extern OUTPUT_HANDLERS ohandlers;
extern INPUT_HANDLERS ihandlers;

enum
{
    REGISTER_WINDOW = 0 ,
    DATA_WINDOW ,
    SOURCE_WINDOW ,
    OUTPUT_WINDOW ,
    OUTPUT_WINDOW_UNBUFFERED
};

typedef enum _ETERMINALMODE
{
    TERMINAL_MODE_HERCULES_GRAPHICS = 0 ,
    TERMINAL_MODE_HERCULES_TEXT,
    TERMINAL_MODE_VGA_TEXT,
    TERMINAL_MODE_SERIAL,
    TERMINAL_MODE_NONE
}ETERMINALMODE;

extern ETERMINALMODE eTerminalMode;

extern WINDOW wWindow[];
extern BOOLEAN bRev;
extern BOOLEAN bGrayed;
extern BOOLEAN bCursorEnabled;

// install and remove handler
BOOLEAN ConsoleInit(void);
void ConsoleShutdown(void);

// OUTPUT handler
void Print(USHORT Window,LPSTR p);
void SetBackgroundColor(ECOLORS c);
void SetForegroundColor(ECOLORS c);
void Clear(USHORT window);
void PutChar(LPSTR p,ULONG x,ULONG y);
void ClrLine(ULONG line);
void ShowCursor(void);
void HideCursor(void);
void EnableScroll(USHORT Window);
void DisableScroll(USHORT Window);
void CopyLineTo(USHORT dest,USHORT src);
void PrintLogo(BOOLEAN bShow);
void PrintCursor(BOOLEAN bForce);
void PrintGraf(ULONG x,ULONG y,UCHAR c);
void ScrollUp(USHORT Window);
void Home(USHORT Window);
void InvertLine(ULONG line);
void FillLine(ULONG line,UCHAR c);
void PrintTemplate(void);
void PrintCaption(void);
void ClrLineToEnd(USHORT Window,ULONG line,ULONG x);
void SuspendPrintRingBuffer(BOOLEAN bSuspend);
void HatchLine(ULONG line);
void SaveGraphicsState(void);
void RestoreGraphicsState(void);
void SetWindowGeometry(PVOID pWindow);

// INPUT handler
UCHAR GetKeyPolled(void);
void FlushKeyboardQueue(void);


BOOLEAN PrintRingBufferOffset(ULONG ulLines,ULONG ulOffset);
BOOLEAN PrintRingBufferHome(ULONG ulLines);
void PrintRingBuffer(ULONG ulLines);
ULONG LinesInRingBuffer(void);
void ReplaceRingBufferCurrent(LPSTR s);
void EmptyRingBuffer(void);
void CheckRingBuffer(void);
BOOLEAN AddToRingBuffer(LPSTR p);
void ResetColor(void);

extern ULONG GLOBAL_SCREEN_WIDTH;
extern ULONG GLOBAL_SCREEN_HEIGHT;

extern ULONG ulOutputLock;

#define Acquire_Output_Lock()       \
{                                   \
    save_flags(ulOutputLock);       \
    cli();                          \
}

#define Release_Output_Lock()       \
    restore_flags(ulOutputLock);

#define NOT_IMPLEMENTED()

extern USHORT usCaptionColor;
#define COLOR_CAPTION usCaptionColor
extern USHORT usCaptionText;
#define COLOR_TEXT usCaptionText
extern USHORT usForegroundColor;
#define COLOR_FOREGROUND usForegroundColor
extern USHORT usBackgroundColor;

#undef COLOR_BACKGROUND
#define COLOR_BACKGROUND usBackgroundColor
extern USHORT usHiLiteColor;
#define COLOR_HILITE usHiLiteColor
