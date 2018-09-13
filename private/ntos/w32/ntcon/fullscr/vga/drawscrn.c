/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    drawscrn.c

Abstract:

    This is the console fullscreen driver for the VGA card.

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "ntddk.h"
#include "fsvga.h"
#include "fsvgalog.h"

#define COMMON_LVB_MASK (COMMON_LVB_GRID_HORIZONTAL |  \
                         COMMON_LVB_GRID_LVERTICAL  |  \
                         COMMON_LVB_GRID_RVERTICAL  |  \
                         COMMON_LVB_REVERSE_VIDEO   |  \
                         COMMON_LVB_UNDERSCORE      )

/* ----- Macros -----*/
/*++
    Macro Description:
        This Macro calcurate a scan line in graphics buffer.

    Arguments:
        WindowY        - Coord to Y.
        FontSizeY      - Font size of Y.

    Return Value:
        Returen to graphics buffer offset.
--*/
#define CalcGRAMScanLine(WindowY,FontSizeY) \
    (WindowY * FontSizeY)

/*++
    Macro Description:
        This Macro calcurate a graphics buffer offset.

    Arguments:
        WindowSize - Coord of window size.
        DeviceExtension - Pointer to the miniport driver's device extension.

    Return Value:
        Returen to graphics buffer offset.
--*/
#define CalcGRAMOffs(WindowSize,DeviceExtension) \
    (DeviceExtension->EmulateInfo.StartAddress + \
     CalcGRAMSize(WindowSize,DeviceExtension) \
    )

/*++
    Macro Description:
        This Macro gets the byte per one scan line.

    Arguments:
        EmulateInfo - Pointer to screen emualte information structure.

    Return Value:
        Byte pre line number.
--*/
#define GetBytePerLine(DeviceExtension) \
    ((DeviceExtension->Configuration.HardwareScroll & OFFSET_128_TO_NEXT_SLICE) ? \
     (1024 / 8) : \
     (640 / 8) \
    )



ULONG
CalcGRAMSize(
    IN COORD WindowSize,
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This Macro calcurate a graphics buffer size.

Arguments:

    WindowSize - Coord of window size.

    DeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    Returen to graphics buffer offset.

--*/

{
    return WindowSize.X +
           CalcGRAMScanLine(WindowSize.Y, DeviceExtension->ScreenAndFont.FontSize.Y) *
           DeviceExtension->EmulateInfo.BytePerLine;
}


PUCHAR
CalcGRAMAddress(
    IN COORD WindowSize,
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine calcurate a graphics buffer address.

Arguments:

    WindowSize - Coord of window size.

    DeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    Returen to graphics buffer address.

--*/
{
    PUCHAR BufPtr = (PUCHAR)DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase;

    BufPtr += CalcGRAMOffs(WindowSize, DeviceExtension);
    if ((DWORD)(BufPtr -
                (PUCHAR)DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase)
           >= DeviceExtension->EmulateInfo.LimitGRAM)
        return (BufPtr - DeviceExtension->EmulateInfo.LimitGRAM);
    else
        return BufPtr;
}


BOOL
IsGRAMRowOver(
    PUCHAR BufPtr,
    BOOL fDbcs,
    PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This Routine check ROW overflow as GRAM limit line.

Arguments:

    BufPtr - Pointer to graphics buffer.

    fDbcs - Flag of DBCS(true) or SBCS(false).

Return Value:
    TRUE:  if font box is overflow as GRAMlimit line.
    FALSE: not overflow.

--*/

{
    if (fDbcs)
    {
        if ((DWORD)(BufPtr + 1 +
                    DeviceExtension->EmulateInfo.DeltaNextFontRow -
                    (PUCHAR)DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase)
             >= DeviceExtension->EmulateInfo.LimitGRAM)
            return TRUE;
        else
            return FALSE;
    }
    else
    {
        if ((DWORD)(BufPtr +
                    DeviceExtension->EmulateInfo.DeltaNextFontRow -
                    (PUCHAR)DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase)
             >= DeviceExtension->EmulateInfo.LimitGRAM)
            return TRUE;
        else
            return FALSE;
    }
}

PUCHAR
NextGRAMRow(
    PUCHAR BufPtr,
    PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:
    This Routine add next row a graphics buffer address.

Arguments:

    BufPtr - Pointer to graphics buffer.

Return Value:

    Returen to graphics buffer address.

--*/

{
    if ((DWORD)(BufPtr +
                DeviceExtension->EmulateInfo.BytePerLine -
                (PUCHAR)DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase)
           >= DeviceExtension->EmulateInfo.LimitGRAM)
        return (BufPtr +
                DeviceExtension->EmulateInfo.BytePerLine -
                DeviceExtension->EmulateInfo.LimitGRAM);
    else
        return (BufPtr + DeviceExtension->EmulateInfo.BytePerLine);
}

VOID
memcpyGRAM(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is a memory copy for byte order.

Arguments:

    TargetPtr - Pointer to target graphics buffer.

    SourcePtr - Pointer to source graphics buffer.

    Length - Fill length.

Return Value:

--*/

{
    while (Length--)
        *TargetPtr++ = *SourcePtr++;
}

VOID
memcpyGRAMOver(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length,
    IN PUCHAR FrameBufPtr,
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine move a graphics buffer.

Arguments:

    TargetPtr - Pointer to target graphics buffer.

    SourcePtr - Pointer to source graphics buffer.

    Length - Fill length.

Return Value:

--*/

{
    ULONG tmpLen;

    if ((DWORD)(SourcePtr + Length - FrameBufPtr) >= DeviceExtension->EmulateInfo.LimitGRAM) {
        tmpLen = DeviceExtension->EmulateInfo.LimitGRAM - (SourcePtr - FrameBufPtr);
        memcpyGRAM(TargetPtr, SourcePtr, tmpLen);
        TargetPtr += tmpLen;
        Length -= tmpLen;
        SourcePtr = FrameBufPtr;
    }

    if ((DWORD)(TargetPtr + Length - FrameBufPtr) >= DeviceExtension->EmulateInfo.LimitGRAM) {
        tmpLen = DeviceExtension->EmulateInfo.LimitGRAM - (TargetPtr - FrameBufPtr);
        memcpyGRAM(TargetPtr, SourcePtr, tmpLen);
        SourcePtr += tmpLen;
        Length -= tmpLen;
        TargetPtr = FrameBufPtr;
    }

    if (Length) {
        memcpyGRAM(TargetPtr, SourcePtr, Length);
    }
}

VOID
MoveGRAM(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length,
    IN PUCHAR FrameBufPtr,
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine move a graphics buffer.

Arguments:

    TargetPtr - Pointer to target graphics buffer.

    SourcePtr - Pointer to source graphics buffer.

    Length - Fill length.

Return Value:

    none.

--*/
{
    PCHAR tmpSrc;
    PCHAR tmpTrg;
    ULONG tmpLen;

    //
    // Set copy mode of graphics register
    //

    SetGRAMCopyMode(DeviceExtension);

    if ((DWORD)(SourcePtr + Length - FrameBufPtr) >= DeviceExtension->EmulateInfo.LimitGRAM ||
        (DWORD)(TargetPtr + Length - FrameBufPtr) >= DeviceExtension->EmulateInfo.LimitGRAM    ) {
        if (SourcePtr > TargetPtr) {
            memcpyGRAMOver(TargetPtr,SourcePtr,Length,FrameBufPtr,DeviceExtension);
        }
        else if ((ULONG)(TargetPtr - SourcePtr) >= Length) {
            memcpyGRAMOver(TargetPtr,SourcePtr,Length,FrameBufPtr,DeviceExtension);
        }
        else {
            if ((DWORD)(SourcePtr + Length - FrameBufPtr) >= DeviceExtension->EmulateInfo.LimitGRAM) {
                tmpLen = SourcePtr + Length - FrameBufPtr - DeviceExtension->EmulateInfo.LimitGRAM;
                tmpTrg = TargetPtr + Length - tmpLen - DeviceExtension->EmulateInfo.LimitGRAM;
                memcpyGRAM(tmpTrg, FrameBufPtr, tmpLen);
                Length -= tmpLen;
            }
            if ((DWORD)(TargetPtr + Length - FrameBufPtr) >= DeviceExtension->EmulateInfo.LimitGRAM) {
                tmpLen = TargetPtr + Length - FrameBufPtr - DeviceExtension->EmulateInfo.LimitGRAM;
                tmpSrc = SourcePtr + Length - tmpLen;
                memcpyGRAM(FrameBufPtr, tmpSrc, tmpLen);
                Length -= tmpLen;
            }
            if (Length) {
                memcpyGRAM(TargetPtr, SourcePtr, Length);
            }
        }
    }
    else {
        memcpyGRAM(TargetPtr, SourcePtr, Length);
    }

    SetGRAMWriteMode(DeviceExtension);
}


NTSTATUS
FsgVgaInitializeHWFlags(
    PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine initialize the hardware scrolls flag and any values.

Arguments:

    EmulateInfo - Pointer to screen emulate information structure.

Return Value:

--*/

{
    ULONG Index;

    GetHardwareScrollReg(DeviceExtension);
    DeviceExtension->EmulateInfo.BytePerLine =
        (WORD)GetBytePerLine(DeviceExtension);
    DeviceExtension->EmulateInfo.MaxScanLine =
        (WORD)CalcGRAMScanLine(DeviceExtension->ScreenAndFont.ScreenSize.Y,
                               DeviceExtension->ScreenAndFont.FontSize.Y);
    DeviceExtension->EmulateInfo.DeltaNextFontRow =
        DeviceExtension->EmulateInfo.BytePerLine * DeviceExtension->ScreenAndFont.FontSize.Y;

    if (DeviceExtension->Configuration.HardwareScroll & USE_LINE_COMPARE) {
        DeviceExtension->EmulateInfo.LimitGRAM =
            DeviceExtension->EmulateInfo.MaxScanLine * DeviceExtension->EmulateInfo.BytePerLine;
    }
    else {
        DeviceExtension->EmulateInfo.LimitGRAM = LIMIT_64K;
    }

    DeviceExtension->EmulateInfo.ColorFg = (BYTE)-1;
    DeviceExtension->EmulateInfo.ColorBg = (BYTE)-1;

    DeviceExtension->EmulateInfo.CursorAttributes.Enable = 0;
    DeviceExtension->EmulateInfo.ShowCursor = FALSE;

    SetGRAMWriteMode(DeviceExtension);

    return STATUS_SUCCESS;
}

NTSTATUS
FsgCopyFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_COPY_FRAME_BUFFER CopyFrameBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine copy the frame buffer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    CopyFrameBuffer - Pointer to the structure containing the information about the copy frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    PUCHAR SourcePtr, TargetPtr;

    FsgInvertCursor(DeviceExtension,FALSE);

    SourcePtr = CalcGRAMAddress (CopyFrameBuffer->SrcScreen.Position,
                                 DeviceExtension);
    TargetPtr = CalcGRAMAddress (CopyFrameBuffer->DestScreen.Position,
                                 DeviceExtension);
    MoveGRAM (TargetPtr,
              SourcePtr,
              CopyFrameBuffer->SrcScreen.nNumberOfChars *
                  DeviceExtension->ScreenAndFont.FontSize.Y,
              DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase,
              DeviceExtension
             );

    FsgInvertCursor(DeviceExtension,TRUE);

    return STATUS_SUCCESS;
}

NTSTATUS
FsgWriteToFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_WRITE_TO_FRAME_BUFFER WriteFrameBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine write the frame buffer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    WriteFrameBuffer - Pointer to the structure containing the information about the write frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    PCHAR_IMAGE_INFO pCharInfoUni = WriteFrameBuffer->SrcBuffer;
    PUCHAR TargetPtr;
    COORD Position = WriteFrameBuffer->DestScreen.Position;
    DWORD Length = WriteFrameBuffer->DestScreen.nNumberOfChars;
    COORD FontSize1 = DeviceExtension->ScreenAndFont.FontSize;
    COORD FontSize2;
    PVOID pCapBuffer = NULL;
    ULONG cCapBuffer = 0;
    BOOL  fDbcs;
    NTSTATUS Status;

    FsgInvertCursor(DeviceExtension,FALSE);

    DeviceExtension->EmulateInfo.ColorFg = (BYTE)-1;
    DeviceExtension->EmulateInfo.ColorBg = (BYTE)-1;

    FontSize2 = FontSize1;
    FontSize2.X *= 2;
    cCapBuffer = CalcBitmapBufferSize(FontSize2,BYTE_ALIGN);
    pCapBuffer = ExAllocatePool(PagedPool, cCapBuffer);

    while (Length--)
    {
        if (pCharInfoUni->FontImageInfo.ImageBits != NULL)
        {
            try
            {
                fDbcs = pCharInfoUni->CharInfo.Attributes & COMMON_LVB_SBCSDBCS;
                AlignCopyMemory((PVOID)pCapBuffer,                    // pDestBits
                                BYTE_ALIGN,                           // dwDestAlign
                                pCharInfoUni->FontImageInfo.ImageBits,// pSrcBits
                                WORD_ALIGN,                           // dwSrcAlign
                                fDbcs ? FontSize2 : FontSize1);

                TargetPtr = CalcGRAMAddress (Position,
                                             DeviceExtension);
                if (fDbcs)
                {
                    if (Length)
                    {
                        FsgWriteToScreen(TargetPtr, pCapBuffer, 2, fDbcs,
                                         pCharInfoUni->CharInfo.Attributes,
                                         (pCharInfoUni+1)->CharInfo.Attributes,
                                         DeviceExtension);
                    }
                    else
                    {
                        FsgWriteToScreen(TargetPtr, pCapBuffer, 2, FALSE,
                                         pCharInfoUni->CharInfo.Attributes,
                                         (WORD)-1,
                                         DeviceExtension);
                    }
                }
                else
                {
                    FsgWriteToScreen(TargetPtr, pCapBuffer, 1, fDbcs,
                                     pCharInfoUni->CharInfo.Attributes,
                                     (WORD)-1,
                                     DeviceExtension);
                }

            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
            }

        }

        if (fDbcs && Length)
        {
            Length--;
            Position.X += 2;
            pCharInfoUni += 2;
        }
        else
        {
            Position.X++;
            pCharInfoUni++;
        }
    }

    FsgInvertCursor(DeviceExtension,TRUE);

    if (pCapBuffer != NULL)
        ExFreePool(pCapBuffer);

    return STATUS_SUCCESS;
}

NTSTATUS
FsgReverseMousePointer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_REVERSE_MOUSE_POINTER MouseBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine reverse the frame buffer for mouse pointer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    MouseBuffer - Pointer to the structure containing the information about the mouse frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{

    PUCHAR CurFrameBufPtr;
    COORD  CursorPosition;
    COORD  FontSize;
    SHORT  i;
    BOOL   fOneMore = FALSE;

    FsgInvertCursor(DeviceExtension,FALSE);

    FontSize = DeviceExtension->ScreenAndFont.FontSize;

    CursorPosition.X = MouseBuffer->Screen.Position.X;
    CursorPosition.Y = MouseBuffer->Screen.Position.Y;
    if ( (0 <= CursorPosition.X &&
               CursorPosition.X < MouseBuffer->Screen.ScreenSize.X) &&
         (0 <= CursorPosition.Y &&
               CursorPosition.Y < MouseBuffer->Screen.ScreenSize.Y)    )
    {
        switch (MouseBuffer->dwType)
        {
            case CHAR_TYPE_LEADING:
                if (CursorPosition.X != MouseBuffer->Screen.ScreenSize.X-1)
                {
                    fOneMore = TRUE;
                }
                break;
            case CHAR_TYPE_TRAILING:
                if (CursorPosition.X != 0)
                {
                    fOneMore = TRUE;
                    CursorPosition.X--;
                }
                break;
        }

        CurFrameBufPtr = CalcGRAMAddress (CursorPosition,
                                          DeviceExtension);

        //
        // Set invert mode of graphics register
        //
        SetGRAMInvertMode(DeviceExtension);

        /*
         * CursorAttributes.Width is bottom scan lines.
         */
        for (i=0 ; i < FontSize.Y; i++)
        {
            AccessGRAM_AND(CurFrameBufPtr, (BYTE)-1);
            if (fOneMore)
                AccessGRAM_AND(CurFrameBufPtr+1, (BYTE)-1);
            CurFrameBufPtr = NextGRAMRow(CurFrameBufPtr,DeviceExtension);
        }

        SetGRAMWriteMode(DeviceExtension);
    }

    FsgInvertCursor(DeviceExtension,TRUE);

    return STATUS_SUCCESS;
}

NTSTATUS
FsgInvertCursor(
    PDEVICE_EXTENSION DeviceExtension,
    BOOL Invert
    )

/*++

Routine Description:

    This routine inverts the cursor.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    Invert - 

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    PUCHAR CurFrameBufPtr;
    COORD  CursorPosition;
    COORD  FontSize;
    SHORT  i;
    SHORT  TopScanLine;
    BOOL   fOneMore = FALSE;

    if (DeviceExtension->EmulateInfo.ShowCursor == Invert)
        return STATUS_SUCCESS;

    DeviceExtension->EmulateInfo.ShowCursor = Invert;

    if (!(DeviceExtension->EmulateInfo.CursorAttributes.Enable))
        return STATUS_SUCCESS;

    FontSize = DeviceExtension->ScreenAndFont.FontSize;
    if (DeviceExtension->ScreenAndFont.ScreenSize.Y > 25)
    {
        TopScanLine = ((DeviceExtension->EmulateInfo.CursorAttributes.Height + 8) * 100 / 8) - 100;
    }
    else
    {
        TopScanLine = ((DeviceExtension->EmulateInfo.CursorAttributes.Height + 16) * 100 / 16) - 100;
    }
    TopScanLine = (FontSize.Y * TopScanLine) / 100;

    CursorPosition.X = DeviceExtension->EmulateInfo.CursorPosition.Coord.Column;
    CursorPosition.Y = DeviceExtension->EmulateInfo.CursorPosition.Coord.Row;
    if ( (0 <= CursorPosition.X &&
               CursorPosition.X < DeviceExtension->ScreenAndFont.ScreenSize.X) &&
         (0 <= CursorPosition.Y &&
               CursorPosition.Y < DeviceExtension->ScreenAndFont.ScreenSize.Y)    )
    {
        switch (DeviceExtension->EmulateInfo.CursorPosition.dwType)
        {
            case CHAR_TYPE_LEADING:
                if (CursorPosition.X != DeviceExtension->ScreenAndFont.ScreenSize.X-1)
                {
                    fOneMore = TRUE;
                }
                break;
            case CHAR_TYPE_TRAILING:
                if (CursorPosition.X != 0)
                {
                    fOneMore = TRUE;
                    CursorPosition.X--;
                }
                break;
        }

        CurFrameBufPtr = CalcGRAMAddress (CursorPosition,
                                          DeviceExtension);

        /*
         * CursorAttributes.Height is top scan lines.
         */
        for (i = 0; i < TopScanLine; i++)
        {
            CurFrameBufPtr = NextGRAMRow(CurFrameBufPtr,DeviceExtension);
        }

        //
        // Set invert mode of graphics register
        //
        SetGRAMInvertMode(DeviceExtension);

        /*
         * CursorAttributes.Width is bottom scan lines.
         */
        for ( ; i < FontSize.Y; i++)
        {
            AccessGRAM_AND(CurFrameBufPtr, (BYTE)-1);
            if (fOneMore)
                AccessGRAM_AND(CurFrameBufPtr+1, (BYTE)-1);
            CurFrameBufPtr = NextGRAMRow(CurFrameBufPtr,DeviceExtension);
        }

        SetGRAMWriteMode(DeviceExtension);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FsgWriteToScreen(
    PUCHAR FrameBuffer,
    PUCHAR BitmapBuffer,
    DWORD cjBytes,
    BOOL fDbcs,
    WORD Attributes1,
    WORD Attributes2,
    PDEVICE_EXTENSION DeviceExtension
    )
{
    WORD  Index;
    PCHAR CurFrameBufPtrTmp;
    PCHAR CurFrameBufPtr2nd;
    PWORD CurFrameBufPtrWord;
    PUCHAR BitmapBufferTmp;

#ifdef LATER_HIGH_SPPED_VRAM_ACCESS  // kazum
    if (! IsGRAMRowOver(FrameBuffer,fDBCS,DeviceExtension)) {
        if (!fDbcs) {
            if (cjBytes == 2)
                BitmapBuffer++;
            (*WriteGramInfo->pfnWriteFontToByteGRAM)(WriteGramInfo);
        }
        else if (cjBytes == 2 && fDBCS) {
            if (DeviceExtension->Configuration.EmulationMode & ENABLE_WORD_WRITE_VRAM) {
                (*WriteGramInfo->pfnWriteFontToFirstWordGRAM)(WriteGramInfo);
            }
            else {
               (*WriteGramInfo->pfnWriteFontToWordGRAM)(WriteGramInfo);
            }
        }
    }
    else
#endif // LATER_HIGH_SPPED_VRAM_ACCESS  // kazum
    try
    {
        set_opaque_bkgnd_proc(DeviceExtension,FrameBuffer,Attributes1);

        if (!fDbcs) {
            CurFrameBufPtrTmp = FrameBuffer;
            if (cjBytes == 2)
                BitmapBuffer++;
            for (Index=0; Index < DeviceExtension->ScreenAndFont.FontSize.Y; Index++) {
                *CurFrameBufPtrTmp = *BitmapBuffer;
                BitmapBuffer += cjBytes;
                CurFrameBufPtrTmp=NextGRAMRow(CurFrameBufPtrTmp,DeviceExtension);
            }
        }
        else if (cjBytes == 2 && fDbcs) {
            if ((DeviceExtension->Configuration.EmulationMode & ENABLE_WORD_WRITE_VRAM) &&
                !((ULONG)FrameBuffer & 1) &&
                (Attributes2 != -1) &&
                (Attributes1 == Attributes2)
               ) {
                CurFrameBufPtrWord = (PWORD)FrameBuffer;
                for (Index=0; Index < DeviceExtension->ScreenAndFont.FontSize.Y; Index++) {
                    *CurFrameBufPtrWord = *((PWORD)BitmapBuffer);
                    BitmapBuffer += cjBytes;
                    CurFrameBufPtrWord=(PWORD)NextGRAMRow((PCHAR)CurFrameBufPtrWord,DeviceExtension);
                }
            }
            else {
                CurFrameBufPtrTmp = FrameBuffer;
                CurFrameBufPtr2nd = FrameBuffer + 1;
                BitmapBufferTmp = BitmapBuffer + 1;
                for (Index=0; Index < DeviceExtension->ScreenAndFont.FontSize.Y; Index++) {
                    *CurFrameBufPtrTmp = *BitmapBuffer;
                    BitmapBuffer += cjBytes;
                    CurFrameBufPtrTmp=NextGRAMRow(CurFrameBufPtrTmp,DeviceExtension);
                }
                if (Attributes2 != -1 &&
                    Attributes1 != Attributes2)
                    set_opaque_bkgnd_proc(DeviceExtension,FrameBuffer,Attributes2);
                for (Index=0; Index < DeviceExtension->ScreenAndFont.FontSize.Y; Index++) {
                    *CurFrameBufPtr2nd = *BitmapBufferTmp;
                    BitmapBufferTmp += cjBytes;
                    CurFrameBufPtr2nd=NextGRAMRow(CurFrameBufPtr2nd,DeviceExtension);
                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (Attributes1 & COMMON_LVB_MASK)
    {
        FsgWriteToScreenCommonLVB(FrameBuffer,
                                  Attributes1,
                                  DeviceExtension);
    }
    if ((Attributes2 != (WORD)-1) && (Attributes2 & COMMON_LVB_MASK))
    {
        FsgWriteToScreenCommonLVB(FrameBuffer+1,
                                  Attributes2,
                                  DeviceExtension);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FsgWriteToScreenCommonLVB(
    PUCHAR FrameBuffer,
    WORD Attributes,
    PDEVICE_EXTENSION DeviceExtension
    )
{
    WORD  Index;
    PUCHAR CurFrameBufPtrTmp;

    try
    {
        if (Attributes & COMMON_LVB_UNDERSCORE)
        {
            set_opaque_bkgnd_proc(DeviceExtension,FrameBuffer,Attributes);
            CurFrameBufPtrTmp = FrameBuffer;
            for (Index=0; Index < DeviceExtension->ScreenAndFont.FontSize.Y - 1; Index++)
                CurFrameBufPtrTmp=NextGRAMRow(CurFrameBufPtrTmp,DeviceExtension);
            *CurFrameBufPtrTmp = 0xff;
        }

        if (Attributes & COMMON_LVB_GRID_HORIZONTAL)
        {
            ColorSetDirect(DeviceExtension, FrameBuffer,
                           FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
                           0);
            *FrameBuffer = 0xff;
        }

        if ( (Attributes & COMMON_LVB_GRID_LVERTICAL) ||
             (Attributes & COMMON_LVB_GRID_RVERTICAL)   )
        {
            BYTE mask = ((Attributes & COMMON_LVB_GRID_LVERTICAL) ? 0x80 : 0) +
                        ((Attributes & COMMON_LVB_GRID_RVERTICAL) ? 0x01 : 0);
            ColorSetGridMask(DeviceExtension,
                             mask
                            );
            CurFrameBufPtrTmp = FrameBuffer;
            for (Index=0; Index < DeviceExtension->ScreenAndFont.FontSize.Y; Index++) {
                AccessGRAM_RW(CurFrameBufPtrTmp, mask);
                CurFrameBufPtrTmp=NextGRAMRow(CurFrameBufPtrTmp,DeviceExtension);
            }

            SetGRAMWriteMode(DeviceExtension);
        }

        DeviceExtension->EmulateInfo.ColorFg = (BYTE)-1;
        DeviceExtension->EmulateInfo.ColorBg = (BYTE)-1;

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return STATUS_SUCCESS;
}

#pragma optimize("",off)
    /*
     * because, frame buffer memory access is need by write/read.
     */
UCHAR
AccessGRAM_WR(
    PUCHAR FrameBuffer,
    UCHAR  write
    )
{
    *FrameBuffer = write;
    return *FrameBuffer;
}

UCHAR
AccessGRAM_RW(
    PUCHAR FrameBuffer,
    UCHAR  write
    )
{
    UCHAR tmp;
    tmp = *FrameBuffer;
    *FrameBuffer = write;
    return tmp;
}

UCHAR
AccessGRAM_AND(
    PUCHAR FrameBuffer,
    UCHAR  write
    )
{
    return *FrameBuffer &= write;
}
#pragma optimize("",on)
