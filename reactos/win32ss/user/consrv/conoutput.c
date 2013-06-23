/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/conoutput.c
 * PURPOSE:         General Console Output Functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "console.h"
#include "include/conio.h"
#include "conio.h"
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

NTSTATUS FASTCALL
ConSrvCreateScreenBuffer(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
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

VOID FASTCALL
ConioSetActiveScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer)
{
    PCONSOLE Console = Buffer->Header.Console;
    Console->ActiveBuffer = Buffer;
    ConioResizeTerminal(Console);
    // ConioDrawConsole(Console);
}

PCONSOLE_SCREEN_BUFFER
ConDrvGetActiveScreenBuffer(IN PCONSOLE Console)
{
    return (Console ? Console->ActiveBuffer : NULL);
}

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvInvalidateBitMapRect)
{
    NTSTATUS Status;
    PCONSOLE_INVALIDATEDIBITS InvalidateDIBitsRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.InvalidateDIBitsRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    DPRINT("SrvInvalidateBitMapRect\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), InvalidateDIBitsRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    /* If the output buffer is the current one, redraw the correct portion of the screen */
    if (Buff == Console->ActiveBuffer)
        ConioDrawRegion(Console, &InvalidateDIBitsRequest->Region);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleCursorInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CursorInfoRequest;
    PCONSOLE_SCREEN_BUFFER Buff;

    DPRINT("SrvGetConsoleCursorInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), CursorInfoRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    CursorInfoRequest->Info.bVisible = Buff->CursorInfo.bVisible;
    CursorInfoRequest->Info.dwSize   = Buff->CursorInfo.dwSize;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleCursorInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CursorInfoRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    DWORD Size;
    BOOL Visible, Success = TRUE;

    DPRINT("SrvSetConsoleCursorInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), CursorInfoRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    Size    = CursorInfoRequest->Info.dwSize;
    Visible = CursorInfoRequest->Info.bVisible;
    if (Size < 1)   Size = 1;
    if (100 < Size) Size = 100;

    if ( (Size != Buff->CursorInfo.dwSize)         ||
         (Visible && !Buff->CursorInfo.bVisible)   ||
         (!Visible && Buff->CursorInfo.bVisible) )
    {
        Buff->CursorInfo.dwSize   = Size;
        Buff->CursorInfo.bVisible = Visible;

        Success = ConioSetCursorInfo(Console, Buff);
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

CSR_API(SrvSetConsoleCursorPosition)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_SETCURSORPOSITION SetCursorPositionRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetCursorPositionRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    SHORT OldCursorX, OldCursorY;
    SHORT NewCursorX, NewCursorY;

    DPRINT("SrvSetConsoleCursorPosition\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetCursorPositionRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    NewCursorX = SetCursorPositionRequest->Position.X;
    NewCursorY = SetCursorPositionRequest->Position.Y;
    if ( NewCursorX < 0 || NewCursorX >= Buff->ScreenBufferSize.X ||
         NewCursorY < 0 || NewCursorY >= Buff->ScreenBufferSize.Y )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quit;
    }
    OldCursorX = Buff->CursorPosition.X;
    OldCursorY = Buff->CursorPosition.Y;
    Buff->CursorPosition.X = NewCursorX;
    Buff->CursorPosition.Y = NewCursorY;
    if (Buff == Console->ActiveBuffer)
    {
        if (!ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Quit;
        }
    }

Quit:
    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return Status;
}

CSR_API(SrvCreateConsoleScreenBuffer)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PCONSOLE_CREATESCREENBUFFER CreateScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CreateScreenBufferRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    PVOID ScreenBufferInfo = NULL;
    TEXTMODE_BUFFER_INFO TextModeInfo = {{80, 25},
                                         DEFAULT_SCREEN_ATTRIB,
                                         DEFAULT_POPUP_ATTRIB ,
                                         TRUE,
                                         CSR_DEFAULT_CURSOR_SIZE};
    GRAPHICS_BUFFER_INFO GraphicsInfo;
    GraphicsInfo.Info = CreateScreenBufferRequest->GraphicsBufferInfo; // HACK for MSVC

    DPRINT("SrvCreateConsoleScreenBuffer\n");

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_TEXTMODE_BUFFER)
    {
        ScreenBufferInfo = &TextModeInfo;

        /*
        if (Console->ActiveBuffer)
        {
            TextModeInfo.ScreenBufferSize = Console->ActiveBuffer->ScreenBufferSize;
            if (TextModeInfo.ScreenBufferSize.X == 0) TextModeInfo.ScreenBufferSize.X = 80;
            if (TextModeInfo.ScreenBufferSize.Y == 0) TextModeInfo.ScreenBufferSize.Y = 25;

            TextModeInfo.ScreenAttrib = Console->ActiveBuffer->ScreenBuffer.TextBuffer.ScreenDefaultAttrib;
            TextModeInfo.PopupAttrib  = Console->ActiveBuffer->ScreenBuffer.TextBuffer.PopupDefaultAttrib;

            TextModeInfo.IsCursorVisible = Console->ActiveBuffer->CursorInfo.bVisible;
            TextModeInfo.CursorSize      = Console->ActiveBuffer->CursorInfo.dwSize;
        }
        */

        /*
         * This is Windows' behaviour
         */

        /* Use the current console size. Regularize it if needed. */
        TextModeInfo.ScreenBufferSize = Console->ConsoleSize;
        if (TextModeInfo.ScreenBufferSize.X == 0) TextModeInfo.ScreenBufferSize.X = 1;
        if (TextModeInfo.ScreenBufferSize.Y == 0) TextModeInfo.ScreenBufferSize.Y = 1;

        /* If we have an active screen buffer, use its attributes as the new ones */
        if (Console->ActiveBuffer && GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
        {
            PTEXTMODE_SCREEN_BUFFER Buffer = (PTEXTMODE_SCREEN_BUFFER)Console->ActiveBuffer;

            TextModeInfo.ScreenAttrib = Buffer->ScreenDefaultAttrib;
            TextModeInfo.PopupAttrib  = Buffer->PopupDefaultAttrib;

            TextModeInfo.IsCursorVisible = Buffer->CursorInfo.bVisible;
            TextModeInfo.CursorSize      = Buffer->CursorInfo.dwSize;
        }
    }
    else if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_GRAPHICS_BUFFER)
    {
        /* Get infos from the graphics buffer information structure */
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMapInfo,
                                      1,
                                      CreateScreenBufferRequest->GraphicsBufferInfo.dwBitMapInfoLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quit;
        }

        ScreenBufferInfo = &GraphicsInfo;

        /* Initialize shared variables */
        CreateScreenBufferRequest->GraphicsBufferInfo.hMutex   = GraphicsInfo.Info.hMutex   = INVALID_HANDLE_VALUE;
        CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMap = GraphicsInfo.Info.lpBitMap = NULL;

        /* A graphics screen buffer is never inheritable */
        CreateScreenBufferRequest->Inheritable = FALSE;
    }

    Status = ConSrvCreateScreenBuffer(&Buff,
                                      Console,
                                      CreateScreenBufferRequest->ScreenBufferType,
                                      ScreenBufferInfo);
    if (!NT_SUCCESS(Status)) goto Quit;

    /* Insert the new handle inside the process handles table */
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    Status = ConSrvInsertObject(ProcessData,
                                &CreateScreenBufferRequest->OutputHandle,
                                &Buff->Header,
                                CreateScreenBufferRequest->Access,
                                CreateScreenBufferRequest->Inheritable,
                                CreateScreenBufferRequest->ShareMode);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    if (!NT_SUCCESS(Status)) goto Quit;

    if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_GRAPHICS_BUFFER)
    {
        PGRAPHICS_SCREEN_BUFFER Buffer = (PGRAPHICS_SCREEN_BUFFER)Buff;
        /*
         * Initialize the graphics buffer information structure
         * and give it back to the client.
         */
        CreateScreenBufferRequest->GraphicsBufferInfo.hMutex   = Buffer->ClientMutex;
        CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMap = Buffer->ClientBitMap;
    }

Quit:
    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvSetConsoleActiveScreenBuffer)
{
    NTSTATUS Status;
    PCONSOLE_SETACTIVESCREENBUFFER SetScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetScreenBufferRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    DPRINT("SrvSetConsoleActiveScreenBuffer\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetScreenBufferRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    if (Buff == Console->ActiveBuffer) goto Quit;

    /* If old buffer has no handles, it's now unreferenced */
    if (Console->ActiveBuffer->Header.HandleCount == 0)
    {
        ConioDeleteScreenBuffer(Console->ActiveBuffer);
    }

    /* Tie console to new buffer */
    ConioSetActiveScreenBuffer(Buff);

Quit:
    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* EOF */
