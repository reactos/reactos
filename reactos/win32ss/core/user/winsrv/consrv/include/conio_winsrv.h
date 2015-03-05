/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/include/conio_winsrv.h
 * PURPOSE:         Public Console I/O Interface - Offers wrap-up structures
 *                  over the console objects exposed by the console driver.
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#include "rect.h"

// This is ALMOST a HACK!!!!!!!
// Helpers for code refactoring
#ifdef USE_NEW_CONSOLE_WAY

#define _CONSRV_CONSOLE  _WINSRV_CONSOLE
#define  CONSRV_CONSOLE   WINSRV_CONSOLE
#define PCONSRV_CONSOLE  PWINSRV_CONSOLE

#else

#define _CONSRV_CONSOLE  _CONSOLE
#define  CONSRV_CONSOLE   CONSOLE
#define PCONSRV_CONSOLE  PCONSOLE

#endif

#define CSR_DEFAULT_CURSOR_SIZE 25

/* VGA character cell */
typedef struct _CHAR_CELL
{
    CHAR Char;
    BYTE Attributes;
} CHAR_CELL, *PCHAR_CELL;
C_ASSERT(sizeof(CHAR_CELL) == 2);

// HACK!!
struct _WINSRV_CONSOLE;
/* HACK: */ typedef struct _WINSRV_CONSOLE *PWINSRV_CONSOLE;
#ifdef USE_NEW_CONSOLE_WAY
#include "conio.h"
#endif

typedef struct _FRONTEND FRONTEND, *PFRONTEND;
/* HACK: */ typedef struct _CONSOLE_INFO *PCONSOLE_INFO;
typedef struct _FRONTEND_VTBL
{
    // NTSTATUS (NTAPI *UnloadFrontEnd)(IN OUT PFRONTEND This);

    /*
     * Internal interface (functions called by the console server only)
     */
    NTSTATUS (NTAPI *InitFrontEnd)(IN OUT PFRONTEND This,
                                   IN struct _CONSRV_CONSOLE* Console);
    VOID (NTAPI *DeinitFrontEnd)(IN OUT PFRONTEND This);

    /* Interface used for both text-mode and graphics screen buffers */
    VOID (NTAPI *DrawRegion)(IN OUT PFRONTEND This,
                             SMALL_RECT* Region);
    /* Interface used only for text-mode screen buffers */
    VOID (NTAPI *WriteStream)(IN OUT PFRONTEND This,
                              SMALL_RECT* Region,
                              SHORT CursorStartX,
                              SHORT CursorStartY,
                              UINT ScrolledLines,
                              PWCHAR Buffer,
                              UINT Length);
    VOID (NTAPI *RingBell)(IN OUT PFRONTEND This);
    BOOL (NTAPI *SetCursorInfo)(IN OUT PFRONTEND This,
                                PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    BOOL (NTAPI *SetScreenInfo)(IN OUT PFRONTEND This,
                                PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                                SHORT OldCursorX,
                                SHORT OldCursorY);
    VOID (NTAPI *ResizeTerminal)(IN OUT PFRONTEND This);
    VOID (NTAPI *SetActiveScreenBuffer)(IN OUT PFRONTEND This);
    VOID (NTAPI *ReleaseScreenBuffer)(IN OUT PFRONTEND This,
                                      IN PCONSOLE_SCREEN_BUFFER ScreenBuffer);
    VOID (NTAPI *RefreshInternalInfo)(IN OUT PFRONTEND This);

    /*
     * External interface (functions corresponding to the Console API)
     */
    VOID (NTAPI *ChangeTitle)(IN OUT PFRONTEND This);
    BOOL (NTAPI *ChangeIcon)(IN OUT PFRONTEND This,
                             HICON IconHandle);
    HWND (NTAPI *GetConsoleWindowHandle)(IN OUT PFRONTEND This);
    VOID (NTAPI *GetLargestConsoleWindowSize)(IN OUT PFRONTEND This,
                                              PCOORD pSize);
    BOOL (NTAPI *GetSelectionInfo)(IN OUT PFRONTEND This,
                                   PCONSOLE_SELECTION_INFO pSelectionInfo);
    BOOL (NTAPI *SetPalette)(IN OUT PFRONTEND This,
                             HPALETTE PaletteHandle,
                             UINT PaletteUsage);
    ULONG (NTAPI *GetDisplayMode)(IN OUT PFRONTEND This);
    BOOL  (NTAPI *SetDisplayMode)(IN OUT PFRONTEND This,
                                  ULONG NewMode);
    INT   (NTAPI *ShowMouseCursor)(IN OUT PFRONTEND This,
                                   BOOL Show);
    BOOL  (NTAPI *SetMouseCursor)(IN OUT PFRONTEND This,
                                  HCURSOR CursorHandle);
    HMENU (NTAPI *MenuControl)(IN OUT PFRONTEND This,
                               UINT CmdIdLow,
                               UINT CmdIdHigh);
    BOOL  (NTAPI *SetMenuClose)(IN OUT PFRONTEND This,
                                BOOL Enable);
} FRONTEND_VTBL, *PFRONTEND_VTBL;

struct _FRONTEND
{
    PFRONTEND_VTBL Vtbl;        /* Virtual table */
    NTSTATUS (NTAPI *UnloadFrontEnd)(IN OUT PFRONTEND This);

    struct _CONSRV_CONSOLE* Console;   /* Console to which the frontend is attached to */
    PVOID Context;              /* Private context */
    PVOID Context2;             /* Private context */
};

/* PauseFlags values (internal only) */
#define PAUSED_FROM_KEYBOARD  0x1
#define PAUSED_FROM_SCROLLBAR 0x2
#define PAUSED_FROM_SELECTION 0x4

typedef struct _WINSRV_CONSOLE
{
/******************************* Console Set-up *******************************/
    /* This **MUST** be FIRST!! */
#ifdef USE_NEW_CONSOLE_WAY
    CONSOLE;
    // CONSOLE Console;
    // // PCONSOLE Console;
#endif

    // LONG ReferenceCount;                    /* Is incremented each time a handle to something in the console (a screen-buffer or the input buffer of this console) gets referenced */
    // CRITICAL_SECTION Lock;
    // CONSOLE_STATE State;                    /* State of the console */

    HANDLE InitEvents[MAX_INIT_EVENTS];         /* Initialization events */

    FRONTEND FrontEndIFace;                     /* Frontend-specific interface */

/******************************* Process support ******************************/
    LIST_ENTRY ProcessList;         /* List of processes owning the console. The first one is the so-called "Console Leader Process" */
    PCONSOLE_PROCESS_DATA NotifiedLastCloseProcess; /* Pointer to the unique process that needs to be notified when the console leader process is killed */
    BOOLEAN NotifyLastClose;        /* TRUE if the console should send a control event when the console leader process is killed */

/******************************* Pausing support ******************************/
    BYTE PauseFlags;
    LIST_ENTRY  ReadWaitQueue;      /* List head for the queue of unique input buffer read wait blocks */
    LIST_ENTRY WriteWaitQueue;      /* List head for the queue of current screen-buffer write wait blocks */

/**************************** Aliases and Histories ***************************/
    struct _ALIAS_HEADER *Aliases;
    LIST_ENTRY HistoryBuffers;
    ULONG HistoryBufferSize;                /* Size for newly created history buffers */
    ULONG NumberOfHistoryBuffers;           /* Maximum number of history buffers allowed */
    BOOLEAN HistoryNoDup;                   /* Remove old duplicate history entries */

/**************************** Input Line Discipline ***************************/
    PWCHAR  LineBuffer;                     /* Current line being input, in line buffered mode */
    ULONG   LineMaxSize;                    /* Maximum size of line in characters (including CR+LF) */
    ULONG   LineSize;                       /* Current size of line */
    ULONG   LinePos;                        /* Current position within line */
    BOOLEAN LineComplete;                   /* User pressed enter, ready to send back to client */
    BOOLEAN LineUpPressed;
    BOOLEAN LineInsertToggle;               /* Replace character over cursor instead of inserting */
    ULONG   LineWakeupMask;                 /* Bitmap of which control characters will end line input */

    BOOLEAN InsertMode;
    BOOLEAN QuickEdit;

/************************ Virtual DOS Machine support *************************/
    COORD   VDMBufferSize;             /* Real size of the VDM buffer, in units of ??? */
    HANDLE  VDMBufferSection;          /* Handle to the memory shared section for the VDM buffer */
    PVOID   VDMBuffer;                 /* Our VDM buffer */
    PVOID   ClientVDMBuffer;           /* A copy of the client view of our VDM buffer */
    HANDLE  VDMClientProcess;          /* Handle to the client process who opened the buffer, to unmap the view */

    HANDLE StartHardwareEvent;
    HANDLE EndHardwareEvent;
    HANDLE ErrorHardwareEvent;

/****************************** Other properties ******************************/
    LIST_ENTRY PopupWindows;                /* List of popup windows */
    UNICODE_STRING OriginalTitle;           /* Original title of console, the one defined when the console leader is launched; it never changes. Always NULL-terminated */
    UNICODE_STRING Title;                   /* Title of console. Always NULL-terminated */
    COLORREF   Colors[16];                  /* Colour palette */

} WINSRV_CONSOLE; // , *PWINSRV_CONSOLE;

/* console.c */
VOID ConioPause(PCONSRV_CONSOLE Console, UINT Flags);
VOID ConioUnpause(PCONSRV_CONSOLE Console, UINT Flags);

PCONSOLE_PROCESS_DATA NTAPI
ConSrvGetConsoleLeaderProcess(IN PCONSRV_CONSOLE Console);
NTSTATUS
ConSrvConsoleCtrlEvent(IN ULONG CtrlEvent,
                       IN PCONSOLE_PROCESS_DATA ProcessData);
NTSTATUS NTAPI
ConSrvConsoleProcessCtrlEvent(IN PCONSRV_CONSOLE Console,
                              IN ULONG ProcessGroupId,
                              IN ULONG CtrlEvent);

/* coninput.c */
VOID NTAPI ConioProcessKey(PCONSRV_CONSOLE Console, MSG* msg);
DWORD ConioEffectiveCursorSize(PCONSRV_CONSOLE Console,
                               DWORD Scale);

NTSTATUS
ConioProcessInputEvent(PCONSRV_CONSOLE Console,
                       PINPUT_RECORD InputEvent);

/* conoutput.c */
PCHAR_INFO ConioCoordToPointer(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y);
NTSTATUS ConioResizeBuffer(PCONSOLE /*PCONSRV_CONSOLE*/ Console,
                           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                           COORD Size);

/* terminal.c */
VOID ConioDrawConsole(PCONSRV_CONSOLE Console);

/* EOF */
