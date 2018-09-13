/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    bitmap.c

Abstract:

        This file implements bitmap video buffer management.

Author:

    Therese Stowell (thereses) 4-Sept-1991

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

NTSTATUS
CreateConsoleBitmap(
    IN OUT PCONSOLE_GRAPHICS_BUFFER_INFO GraphicsInfo,
    IN OUT PSCREEN_INFORMATION ScreenInfo,
    OUT PVOID *lpBitmap,
    OUT HANDLE *hMutex
    )
{
    NTSTATUS Status;
    LARGE_INTEGER MaximumSize;
    SIZE_T ViewSize;

    //
    // adjust bitmap info
    //


    if (GraphicsInfo->lpBitMapInfo->bmiHeader.biHeight > 0)
    {
#if DBG
        DbgPrint("*************** Negating biHeight\n");
#endif
        GraphicsInfo->lpBitMapInfo->bmiHeader.biHeight =
            -GraphicsInfo->lpBitMapInfo->bmiHeader.biHeight;
    }

    if (GraphicsInfo->lpBitMapInfo->bmiHeader.biCompression != BI_RGB)
    {
#if DBG
        DbgPrint("*************** setting Compression to BI_RGB)\n");
#endif
        GraphicsInfo->lpBitMapInfo->bmiHeader.biCompression = BI_RGB;
    }

    //
    // allocate screeninfo buffer data and copy it
    //

    ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo = (LPBITMAPINFO)ConsoleHeapAlloc(MAKE_TAG( BMP_TAG ),GraphicsInfo->dwBitMapInfoLength);
    if (ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo == NULL) {
        return STATUS_NO_MEMORY;
    }
    ScreenInfo->BufferInfo.GraphicsInfo.BitMapInfoLength = GraphicsInfo->dwBitMapInfoLength;
    RtlCopyMemory(ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo,
           GraphicsInfo->lpBitMapInfo,
           GraphicsInfo->dwBitMapInfoLength
          );
    ASSERT((GraphicsInfo->lpBitMapInfo->bmiHeader.biWidth *
            -GraphicsInfo->lpBitMapInfo->bmiHeader.biHeight / 8 *
            GraphicsInfo->lpBitMapInfo->bmiHeader.biBitCount) ==
           (LONG)GraphicsInfo->lpBitMapInfo->bmiHeader.biSizeImage);

    //
    // create bitmap section
    //

    MaximumSize.QuadPart = GraphicsInfo->lpBitMapInfo->bmiHeader.biSizeImage;
    Status = NtCreateSection(&ScreenInfo->BufferInfo.GraphicsInfo.hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL
                            );
    if (!NT_SUCCESS(Status)) {
        ConsoleHeapFree(ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo);
        return Status;
    }

    //
    // map server view of section
    //

    ViewSize = GraphicsInfo->lpBitMapInfo->bmiHeader.biSizeImage;
    ScreenInfo->BufferInfo.GraphicsInfo.BitMap = 0;
    Status = NtMapViewOfSection(ScreenInfo->BufferInfo.GraphicsInfo.hSection,
                                NtCurrentProcess(),
                                &ScreenInfo->BufferInfo.GraphicsInfo.BitMap,
                                0L,
                                GraphicsInfo->lpBitMapInfo->bmiHeader.biSizeImage,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0L,
                                PAGE_READWRITE
                               );
    if (!NT_SUCCESS(Status)) {
        ConsoleHeapFree(ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo);
        NtClose(ScreenInfo->BufferInfo.GraphicsInfo.hSection);
        return Status;
    }

    //
    // map client view of section
    //

    ViewSize = GraphicsInfo->lpBitMapInfo->bmiHeader.biSizeImage;
    *lpBitmap = 0;
    Status = NtMapViewOfSection(ScreenInfo->BufferInfo.GraphicsInfo.hSection,
                                CONSOLE_CLIENTPROCESSHANDLE(),
                                lpBitmap,
                                0L,
                                GraphicsInfo->lpBitMapInfo->bmiHeader.biSizeImage,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0L,
                                PAGE_READWRITE
                               );
    if (!NT_SUCCESS(Status)) {
        ConsoleHeapFree(ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo);
        NtUnmapViewOfSection(NtCurrentProcess(),ScreenInfo->BufferInfo.GraphicsInfo.BitMap);
        NtClose(ScreenInfo->BufferInfo.GraphicsInfo.hSection);
        return Status;
    }
    ScreenInfo->BufferInfo.GraphicsInfo.ClientProcess = CONSOLE_CLIENTPROCESSHANDLE();
    ScreenInfo->BufferInfo.GraphicsInfo.ClientBitMap = *lpBitmap;

    //
    // create mutex to serialize access to bitmap, then map handle to mutex to client side
    //

    NtCreateMutant(&ScreenInfo->BufferInfo.GraphicsInfo.hMutex,
                   MUTANT_ALL_ACCESS, NULL, FALSE);
    MapHandle(CONSOLE_CLIENTPROCESSHANDLE(),
              ScreenInfo->BufferInfo.GraphicsInfo.hMutex,
              hMutex
             );

    ScreenInfo->BufferInfo.GraphicsInfo.dwUsage = GraphicsInfo->dwUsage;
    ScreenInfo->ScreenBufferSize.X = (WORD)GraphicsInfo->lpBitMapInfo->bmiHeader.biWidth;
    ScreenInfo->ScreenBufferSize.Y = (WORD)-GraphicsInfo->lpBitMapInfo->bmiHeader.biHeight;
    ScreenInfo->Window.Left = 0;
    ScreenInfo->Window.Top = 0;
    ScreenInfo->Window.Right = (SHORT)(ScreenInfo->Window.Left+ScreenInfo->ScreenBufferSize.X-1);
    ScreenInfo->Window.Bottom = (SHORT)(ScreenInfo->Window.Top+ScreenInfo->ScreenBufferSize.Y-1);
    return STATUS_SUCCESS;
}


ULONG
SrvInvalidateBitMapRect(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine is called to indicate that the application has modified a region
    in the bitmap.  We update the region to the screen.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    PCONSOLE_INVALIDATERECT_MSG a = (PCONSOLE_INVALIDATERECT_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    UINT Codepage;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE | CONSOLE_GRAPHICS_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        UnlockConsole(Console);
        return((ULONG) Status);
    }
    if (HandleData->HandleType & CONSOLE_OUTPUT_HANDLE) {
        //ASSERT(Console->Flags & CONSOLE_VDM_REGISTERED);
        //ASSERT(!(Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE));
        ASSERT(Console->VDMBuffer != NULL);
        if (Console->VDMBuffer != NULL) {
            //ASSERT(HandleData->Buffer.ScreenBuffer->ScreenBufferSize.X <= Console->VDMBufferSize.X);
            //ASSERT(HandleData->Buffer.ScreenBuffer->ScreenBufferSize.Y <= Console->VDMBufferSize.Y);
            if (HandleData->Buffer.ScreenBuffer->ScreenBufferSize.X <= Console->VDMBufferSize.X &&
                HandleData->Buffer.ScreenBuffer->ScreenBufferSize.Y <= Console->VDMBufferSize.Y) {
                COORD TargetPoint;

                TargetPoint.X = a->Rect.Left;
                TargetPoint.Y = a->Rect.Top;
                // VDM can sometimes get out of sync with window size
                //ASSERT(a->Rect.Left >= 0);
                //ASSERT(a->Rect.Top >= 0);
                //ASSERT(a->Rect.Right < HandleData->Buffer.ScreenBuffer->ScreenBufferSize.X);
                //ASSERT(a->Rect.Bottom < HandleData->Buffer.ScreenBuffer->ScreenBufferSize.Y);
                //ASSERT(a->Rect.Left <= a->Rect.Right);
                //ASSERT(a->Rect.Top <= a->Rect.Bottom);
                if ((a->Rect.Left >= 0) &&
                    (a->Rect.Top >= 0) &&
                    (a->Rect.Right < HandleData->Buffer.ScreenBuffer->ScreenBufferSize.X) &&
                    (a->Rect.Bottom < HandleData->Buffer.ScreenBuffer->ScreenBufferSize.Y) &&
                    (a->Rect.Left <= a->Rect.Right) &&
                    (a->Rect.Top <= a->Rect.Bottom) ) {

                    if ((Console->CurrentScreenBuffer->Flags & CONSOLE_OEMFONT_DISPLAY) && ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
#if defined(FE_SB)
                        if (CONSOLE_IS_DBCS_ENABLED() &&
                            Console->OutputCP != WINDOWSCP )
                        {
                            Codepage = USACP;
                        }
                        else

#endif
                        // we want UnicodeOem characters
                        Codepage = WINDOWSCP;
                    } else {
#if defined(FE_SB)
                        if (CONSOLE_IS_DBCS_ENABLED()) {
                            Codepage = Console->OutputCP;
                        }
                        else
#endif
                        // we want real Unicode characters
                        Codepage = Console->CP;
                    }

                    WriteRectToScreenBuffer((PBYTE)Console->VDMBuffer,
                            Console->VDMBufferSize, &a->Rect,
                            HandleData->Buffer.ScreenBuffer, TargetPoint,
                            Codepage);
                    WriteToScreen(HandleData->Buffer.ScreenBuffer,&a->Rect);
                } else {
                    Status = STATUS_INVALID_PARAMETER;
                }
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } else {

        //
        // write data to screen
        //

        WriteToScreen(HandleData->Buffer.ScreenBuffer,&a->Rect);
    }

    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);
}

NTSTATUS
WriteRegionToScreenBitMap(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    )
{
    DWORD NumScanLines;
    int   Height;
    //
    // if we have a selection, turn it off.
    //

    InvertSelection(ScreenInfo->Console,TRUE);

    NtWaitForSingleObject(ScreenInfo->BufferInfo.GraphicsInfo.hMutex,
                          FALSE, NULL);

    // The origin of (xSrc, ySrc) passed to SetDIBitsToDevice is located
    // at the DIB's bottom-left corner no matter if the DIB is
    // a top-down or bottom-up. Thus, if the DIB is a top-down, we have
    // to translate ySrc accordingly:
    // if (height < 0) {        // top-down
    //      ySrc = abs(height) - rect.Bottom -1;
    //
    // else
    //      ySrc = rect.Bottom;
    //
    Height = ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo->bmiHeader.biHeight;

    NumScanLines = SetDIBitsToDevice(ScreenInfo->Console->hDC,
                      Region->Left - ScreenInfo->Window.Left,
                      Region->Top - ScreenInfo->Window.Top,
                      Region->Right - Region->Left + 1,
                      Region->Bottom - Region->Top + 1,
                      Region->Left,
              Height < 0 ? -Height - Region->Bottom - 1 : Region->Bottom,
                      0,
                      ScreenInfo->ScreenBufferSize.Y,
                      ScreenInfo->BufferInfo.GraphicsInfo.BitMap,
                      ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo,
                      ScreenInfo->BufferInfo.GraphicsInfo.dwUsage
                     );

    NtReleaseMutant(ScreenInfo->BufferInfo.GraphicsInfo.hMutex, NULL);

    //
    // if we have a selection, turn it on.
    //

    InvertSelection(ScreenInfo->Console,FALSE);

    if (NumScanLines == 0) {
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}




VOID
FreeConsoleBitmap(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    NtUnmapViewOfSection(NtCurrentProcess(),
                         ScreenInfo->BufferInfo.GraphicsInfo.BitMap);
    NtUnmapViewOfSection(ScreenInfo->BufferInfo.GraphicsInfo.ClientProcess,
                         ScreenInfo->BufferInfo.GraphicsInfo.ClientBitMap);
    NtClose(ScreenInfo->BufferInfo.GraphicsInfo.hSection);
    NtClose(ScreenInfo->BufferInfo.GraphicsInfo.hMutex);
    ConsoleHeapFree(ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo);
}
