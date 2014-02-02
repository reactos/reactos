/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            win32ss/user/winsrv/consrv/condrv/conoutput.c
 * PURPOSE:         General Console Output Functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "console.h"
#include "include/conio.h"
#include "include/conio2.h"
#include "conoutput.h"
#include "handle.h"

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
TEXTMODE_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                           IN OUT PCONSOLE Console,
                           IN PTEXTMODE_BUFFER_INFO TextModeInfo);
NTSTATUS
GRAPHICS_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                           IN OUT PCONSOLE Console,
                           IN PGRAPHICS_BUFFER_INFO GraphicsInfo);

VOID
TEXTMODE_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer);
VOID
GRAPHICS_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer);


NTSTATUS
CONSOLE_SCREEN_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                                 IN OUT PCONSOLE Console,
                                 IN SIZE_T Size)
{
    if (Buffer == NULL || Console == NULL)
        return STATUS_INVALID_PARAMETER;

    *Buffer = ConsoleAllocHeap(HEAP_ZERO_MEMORY, max(sizeof(CONSOLE_SCREEN_BUFFER), Size));
    if (*Buffer == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize the header with the default type */
    ConSrvInitObject(&(*Buffer)->Header, SCREEN_BUFFER, Console);
    (*Buffer)->Vtbl = NULL;
    return STATUS_SUCCESS;
}

VOID
CONSOLE_SCREEN_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer)
{
    if (Buffer->Header.Type == TEXTMODE_BUFFER)
        TEXTMODE_BUFFER_Destroy(Buffer);
    else if (Buffer->Header.Type == GRAPHICS_BUFFER)
        GRAPHICS_BUFFER_Destroy(Buffer);
    else if (Buffer->Header.Type == SCREEN_BUFFER)
        ConsoleFreeHeap(Buffer);
    // else
    //     do_nothing;
}

// ConDrvCreateConsoleScreenBuffer
NTSTATUS FASTCALL
ConDrvCreateScreenBuffer(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                         IN OUT PCONSOLE Console,
                         IN ULONG BufferType,
                         IN PVOID ScreenBufferInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( Console == NULL || Buffer == NULL ||
        (BufferType != CONSOLE_TEXTMODE_BUFFER && BufferType != CONSOLE_GRAPHICS_BUFFER) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (BufferType == CONSOLE_TEXTMODE_BUFFER)
    {
        Status = TEXTMODE_BUFFER_Initialize(Buffer,
                                            Console,
                                            (PTEXTMODE_BUFFER_INFO)ScreenBufferInfo);
    }
    else if (BufferType == CONSOLE_GRAPHICS_BUFFER)
    {
        Status = GRAPHICS_BUFFER_Initialize(Buffer,
                                            Console,
                                            (PGRAPHICS_BUFFER_INFO)ScreenBufferInfo);
    }
    else
    {
        /* Never ever go there!! */
        ASSERT(FALSE);
    }

    /* Insert the newly created screen buffer into the list, if succeeded */
    if (NT_SUCCESS(Status)) InsertHeadList(&Console->BufferList, &(*Buffer)->ListEntry);

    return Status;
}

static VOID
ConioSetActiveScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer);

VOID WINAPI
ConioDeleteScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer)
{
    PCONSOLE Console = Buffer->Header.Console;
    PCONSOLE_SCREEN_BUFFER NewBuffer;

    RemoveEntryList(&Buffer->ListEntry);
    if (Buffer == Console->ActiveBuffer)
    {
        /* Delete active buffer; switch to most recently created */
        Console->ActiveBuffer = NULL;
        if (!IsListEmpty(&Console->BufferList))
        {
            NewBuffer = CONTAINING_RECORD(Console->BufferList.Flink,
                                          CONSOLE_SCREEN_BUFFER,
                                          ListEntry);
            ConioSetActiveScreenBuffer(NewBuffer);
        }
    }

    CONSOLE_SCREEN_BUFFER_Destroy(Buffer);
}

VOID FASTCALL
ConioDrawConsole(PCONSOLE Console)
{
    SMALL_RECT Region;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = Console->ActiveBuffer;

    if (ActiveBuffer)
    {
        ConioInitRect(&Region, 0, 0, ActiveBuffer->ViewSize.Y - 1, ActiveBuffer->ViewSize.X - 1);
        ConioDrawRegion(Console, &Region);
    }
}

static VOID
ConioSetActiveScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer)
{
    PCONSOLE Console = Buffer->Header.Console;
    Console->ActiveBuffer = Buffer;
    ConioResizeTerminal(Console);
    // ConioDrawConsole(Console);
}

NTSTATUS NTAPI
ConDrvSetConsoleActiveScreenBuffer(IN PCONSOLE Console,
                                   IN PCONSOLE_SCREEN_BUFFER Buffer)
{
    if (Console == NULL || Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    if (Buffer == Console->ActiveBuffer) return STATUS_SUCCESS;

    /* If old buffer has no handles, it's now unreferenced */
    if (Console->ActiveBuffer->Header.HandleCount == 0)
    {
        ConioDeleteScreenBuffer(Console->ActiveBuffer);
    }

    /* Tie console to new buffer */
    ConioSetActiveScreenBuffer(Buffer);

    return STATUS_SUCCESS;
}

PCONSOLE_SCREEN_BUFFER
ConDrvGetActiveScreenBuffer(IN PCONSOLE Console)
{
    return (Console ? Console->ActiveBuffer : NULL);
}

/* PUBLIC DRIVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvInvalidateBitMapRect(IN PCONSOLE Console,
                           IN PCONSOLE_SCREEN_BUFFER Buffer,
                           IN PSMALL_RECT Region)
{
    if (Console == NULL || Buffer == NULL || Region == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    /* If the output buffer is the current one, redraw the correct portion of the screen */
    if (Buffer == Console->ActiveBuffer) ConioDrawRegion(Console, Region);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleCursorInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           OUT PCONSOLE_CURSOR_INFO CursorInfo)
{
    if (Console == NULL || Buffer == NULL || CursorInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    *CursorInfo = Buffer->CursorInfo;
    // CursorInfo->bVisible = Buffer->CursorInfo.bVisible;
    // CursorInfo->dwSize   = Buffer->CursorInfo.dwSize;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsoleCursorInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           IN PCONSOLE_CURSOR_INFO CursorInfo)
{
    ULONG Size;
    BOOLEAN Visible, Success = TRUE;

    if (Console == NULL || Buffer == NULL || CursorInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    Size    = min(max(CursorInfo->dwSize, 1), 100);
    Visible = CursorInfo->bVisible;

    if ( (Size != Buffer->CursorInfo.dwSize)         ||
         (Visible && !Buffer->CursorInfo.bVisible)   ||
         (!Visible && Buffer->CursorInfo.bVisible) )
    {
        Buffer->CursorInfo.dwSize   = Size;
        Buffer->CursorInfo.bVisible = Visible;

        Success = ConioSetCursorInfo(Console, (PCONSOLE_SCREEN_BUFFER)Buffer);
    }

    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

NTSTATUS NTAPI
ConDrvSetConsoleCursorPosition(IN PCONSOLE Console,
                               IN PTEXTMODE_SCREEN_BUFFER Buffer,
                               IN PCOORD Position)
{
    SHORT OldCursorX, OldCursorY;

    if (Console == NULL || Buffer == NULL || Position == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    if ( Position->X < 0 || Position->X >= Buffer->ScreenBufferSize.X ||
         Position->Y < 0 || Position->Y >= Buffer->ScreenBufferSize.Y )
    {
        return STATUS_INVALID_PARAMETER;
    }

    OldCursorX = Buffer->CursorPosition.X;
    OldCursorY = Buffer->CursorPosition.Y;
    Buffer->CursorPosition = *Position;
    // Buffer->CursorPosition.X = Position->X;
    // Buffer->CursorPosition.Y = Position->Y;
    if ( ((PCONSOLE_SCREEN_BUFFER)Buffer == Console->ActiveBuffer) &&
         (!ConioSetScreenInfo(Console, (PCONSOLE_SCREEN_BUFFER)Buffer, OldCursorX, OldCursorY)) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

/* EOF */
