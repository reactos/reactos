/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    dbcs.c

Abstract:

Author:

    KazuM Mar.05.1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#pragma alloc_text(FE_TEXT, CheckBisectStringA)
#pragma alloc_text(FE_TEXT, BisectWrite)
#pragma alloc_text(FE_TEXT, BisectClipbrd)
#pragma alloc_text(FE_TEXT, BisectWriteAttr)
#pragma alloc_text(FE_TEXT, IsDBCSLeadByteConsole)
#pragma alloc_text(FE_TEXT, TextOutEverything)
#pragma alloc_text(FE_TEXT, TextOutCommonLVB)
#ifdef i386
#pragma alloc_text(FE_TEXT, RealUnicodeToNEC_OS2_Unicode)
#pragma alloc_text(FE_TEXT, InitializeNEC_OS2_CP)
#endif
#pragma alloc_text(FE_TEXT, ProcessCreateConsoleIME)
#pragma alloc_text(FE_TEXT, InitConsoleIMEStuff)
#pragma alloc_text(FE_TEXT, WaitConsoleIMEStuff)
#pragma alloc_text(FE_TEXT, ConSrvRegisterConsoleIME)
#pragma alloc_text(FE_TEXT, RemoveConsoleIME)
#pragma alloc_text(FE_TEXT, ConsoleImeMessagePump)
#pragma alloc_text(FE_TEXT, RegisterKeisenOfTTFont)
#pragma alloc_text(FE_TEXT, ImmConversionToConsole)
#pragma alloc_text(FE_TEXT, ImmConversionFromConsole)
#pragma alloc_text(FE_TEXT, TranslateUnicodeToOem)


#if defined(FE_SB)

SINGLE_LIST_ENTRY gTTFontList;    // This list contain TTFONTLIST data.

#if defined(i386)
ULONG  gdwMachineId;
#endif

LPTHREAD_START_ROUTINE ConsoleIMERoutine;  // client side console IME routine
CRITICAL_SECTION ConIMEInitWindowsLock;

#if defined(i386)
/*
 * NEC PC-98 OS/2 OEM character set
 * When FormatID is 0 or 80, Convert SBCS (00h-1Fh) font.
 */
PCPTABLEINFO pGlyph_NEC_OS2_CP;
PUSHORT pGlyph_NEC_OS2_Table;
#endif // i386



#if defined(FE_IME)


#if defined(i386)
NTSTATUS
ImeWmFullScreen(
    IN BOOL Foreground,
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if(Foreground) {
        ULONG ModeIndex;
        PCONVERSIONAREA_INFORMATION ConvAreaInfo;

        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                              CONIME_SETFOCUS,
                              (WPARAM)Console->ConsoleHandle,
                              (LPARAM)Console->hklActive
                             ))) {
            return STATUS_INVALID_HANDLE;
        }

        if (ConvAreaInfo = Console->ConsoleIme.ConvAreaRoot) {
            if (!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER))
                ModeIndex = ScreenInfo->BufferInfo.TextInfo.ModeIndex;
            else if (!(Console->CurrentScreenBuffer->Flags & CONSOLE_GRAPHICS_BUFFER))
                ModeIndex = Console->CurrentScreenBuffer->BufferInfo.TextInfo.ModeIndex;
            else
                ModeIndex = 0;
            do {
#ifdef FE_SB
                // Check code for must CONSOLE_TEXTMODE_BUFFER !!
                if (! (ConvAreaInfo->ScreenBuffer->Flags & CONSOLE_GRAPHICS_BUFFER))
                    ConvAreaInfo->ScreenBuffer->BufferInfo.TextInfo.ModeIndex = ModeIndex;
                else
                    ASSERT(FALSE);
#else
                ConvAreaInfo->ScreenBuffer->BufferInfo.TextInfo.ModeIndex = ModeIndex;
#endif
            } while (ConvAreaInfo = ConvAreaInfo->ConvAreaNext);
        }
    }
    else
    {
        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                              CONIME_KILLFOCUS,
                              (WPARAM)Console->ConsoleHandle,
                              (LPARAM)Console->hklActive
                             ))) {
            return STATUS_INVALID_HANDLE;
        }
    }

    return Status;
}
#endif // i386




NTSTATUS
GetImeKeyState(
    IN PCONSOLE_INFORMATION Console,
    IN PDWORD pdwConversion
    )

/*++

Routine Description:

    This routine get IME mode for KEY_EVENT_RECORD.

Arguments:

    ConsoleInfo - Pointer to console information structure.

Return Value:

--*/

{
    DWORD dwDummy;

    /*
     * If pdwConversion is NULL, the caller doensn't want
     * the result --- but for code efficiency, let it
     * point to the dummy dword variable, so that
     * we don't have to care from here.
     */
    if (pdwConversion == NULL) {
        pdwConversion = &dwDummy;
    }

    if (Console->InputBuffer.ImeMode.Disable)
    {
        *pdwConversion = 0;
    }
    else
    {
        PINPUT_THREAD_INFO InputThreadInfo;     // console thread info

        InputThreadInfo = TlsGetValue(InputThreadTlsIndex);

        if (InputThreadInfo != NULL)
        {
            LRESULT lResult;

            /*
             * This thread is in InputThread()
             * We can try message pump.
             */

            if (!NT_SUCCESS(ConsoleImeMessagePumpWorker(Console,
                    CONIME_GET_NLSMODE,
                    (WPARAM)Console->ConsoleHandle,
                    (LPARAM)0,
                    &lResult))) {

                *pdwConversion = IME_CMODE_DISABLE;
                return STATUS_INVALID_HANDLE;
            }


            *pdwConversion = (DWORD)lResult;

            if (Console->InputBuffer.ImeMode.ReadyConversion == FALSE)
                Console->InputBuffer.ImeMode.ReadyConversion = TRUE;
        }
        else
        {
            /*
             * This thread is in CsrApiRequestThread()
             * We can not try message pump.
             */
            if (Console->InputBuffer.ImeMode.ReadyConversion == FALSE) {
                *pdwConversion = 0;
                return STATUS_SUCCESS;
            }

            *pdwConversion = Console->InputBuffer.ImeMode.Conversion;
        }


        if (*pdwConversion & IME_CMODE_OPEN)
            Console->InputBuffer.ImeMode.Open = TRUE;
        else
            Console->InputBuffer.ImeMode.Open = FALSE;
        if (*pdwConversion & IME_CMODE_DISABLE)
            Console->InputBuffer.ImeMode.Disable = TRUE;
        else
            Console->InputBuffer.ImeMode.Disable = FALSE;

        Console->InputBuffer.ImeMode.Conversion = *pdwConversion;

    }

    return STATUS_SUCCESS;
}




NTSTATUS
SetImeKeyState(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD fdwConversion
    )

/*++

Routine Description:

    This routine get IME mode for KEY_EVENT_RECORD.

Arguments:

    Console - Pointer to console information structure.

    fdwConversion - IME conversion status.

Return Value:

--*/

{
    PCONVERSIONAREA_INFORMATION ConvAreaInfo;

    if ( (fdwConversion & IME_CMODE_DISABLE) && (! Console->InputBuffer.ImeMode.Disable) ) {
        Console->InputBuffer.ImeMode.Disable = TRUE;
        if ( Console->InputBuffer.ImeMode.Open ) {
            ConvAreaInfo = Console->ConsoleIme.ConvAreaMode;
            if (ConvAreaInfo)
                ConvAreaInfo->ConversionAreaMode |= CA_HIDDEN;
            ConvAreaInfo = Console->ConsoleIme.ConvAreaSystem;
            if (ConvAreaInfo)
                ConvAreaInfo->ConversionAreaMode |= CA_HIDDEN;
            if (Console->InputBuffer.ImeMode.Open && CONSOLE_IS_DBCS_OUTPUTCP(Console))
                ConsoleImePaint(Console, Console->ConsoleIme.ConvAreaRoot);
        }
    }
    else if ( (! (fdwConversion & IME_CMODE_DISABLE)) && Console->InputBuffer.ImeMode.Disable) {
        Console->InputBuffer.ImeMode.Disable = FALSE;
        if ( fdwConversion & IME_CMODE_OPEN ) {
            ConvAreaInfo = Console->ConsoleIme.ConvAreaMode;
            if (ConvAreaInfo)
                ConvAreaInfo->ConversionAreaMode &= ~CA_HIDDEN;
            ConvAreaInfo = Console->ConsoleIme.ConvAreaSystem;
            if (ConvAreaInfo)
                ConvAreaInfo->ConversionAreaMode &= ~CA_HIDDEN;
            if (Console->InputBuffer.ImeMode.Open && CONSOLE_IS_DBCS_OUTPUTCP(Console))
                ConsoleImePaint(Console, Console->ConsoleIme.ConvAreaRoot);
        }
    }
    else if ( (fdwConversion & IME_CMODE_DISABLE) && (Console->InputBuffer.ImeMode.Disable) ) {
        return STATUS_SUCCESS;
    }

    if ( (fdwConversion & IME_CMODE_OPEN) && (! Console->InputBuffer.ImeMode.Open)) {
        Console->InputBuffer.ImeMode.Open = TRUE;
    }
    else if ( (! (fdwConversion & IME_CMODE_OPEN)) && Console->InputBuffer.ImeMode.Open) {
        Console->InputBuffer.ImeMode.Open = FALSE;
    }

    Console->InputBuffer.ImeMode.Conversion = fdwConversion;

    if (Console->InputBuffer.ImeMode.ReadyConversion == FALSE)
        Console->InputBuffer.ImeMode.ReadyConversion = TRUE;

    if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                          CONIME_SET_NLSMODE,
                          (WPARAM)Console->ConsoleHandle,
                          (LPARAM)fdwConversion
                         ))) {
        return STATUS_INVALID_HANDLE;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SetImeCodePage(
    IN PCONSOLE_INFORMATION Console
    )
{
    DWORD CodePage = Console->OutputCP;
    DWORD fdwConversion;

    if (!CONSOLE_IS_DBCS_CP(Console))
    {
        if (!NT_SUCCESS(GetImeKeyState(Console, &fdwConversion))) {
            return STATUS_INVALID_HANDLE;
        }

        fdwConversion |= IME_CMODE_DISABLE;

    }
    else {
        fdwConversion = Console->InputBuffer.ImeMode.Conversion & ~IME_CMODE_DISABLE;
    }

    if (!NT_SUCCESS(SetImeKeyState(Console, fdwConversion))) {
        return STATUS_INVALID_HANDLE;
    }

    if (CONSOLE_IS_IME_ENABLED()) {
        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                              CONIME_NOTIFY_CODEPAGE,
                              (WPARAM)Console->ConsoleHandle,
                              (LPARAM)MAKELPARAM(FALSE, CodePage)
                             ))) {
            return STATUS_INVALID_HANDLE;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SetImeOutputCodePage(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN DWORD PrevCodePage
    )
{
    DWORD CodePage = Console->OutputCP;

    // Output code page
    if ((ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) &&
        (IsAvailableFarEastCodePage(CodePage) || IsAvailableFarEastCodePage(PrevCodePage)))
    {
        ConvertToCodePage(Console, PrevCodePage);
        AdjustFont(Console, CodePage);
    }
    // load special ROM font, if necessary
#ifdef i386
    if ( (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) &&
         !(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER))
    {
        SetROMFontCodePage(CodePage,
                           ScreenInfo->BufferInfo.TextInfo.ModeIndex);
        SetCursorInformationHW(ScreenInfo,
                        ScreenInfo->BufferInfo.TextInfo.CursorSize,
                        ScreenInfo->BufferInfo.TextInfo.CursorVisible);
        WriteRegionToScreenHW(ScreenInfo,
                &ScreenInfo->Window);
    }
#endif

    if (CONSOLE_IS_IME_ENABLED()) {
        if (!NT_SUCCESS(ConsoleImeMessagePump(Console,
                              CONIME_NOTIFY_CODEPAGE,
                              (WPARAM)Console->ConsoleHandle,
                              (LPARAM)MAKELPARAM(TRUE, CodePage)
                             ))) {
            return STATUS_INVALID_HANDLE;
        }
    }

    return STATUS_SUCCESS;
}
#endif // FE_IME

















VOID
SetLineChar(
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine setup of line character code.

Arguments:

    ScreenInfo - Pointer to screen information structure.

Return Value:

    none.

--*/

{
    if (CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console))
    {
        if (OEMCP == JAPAN_CP || OEMCP == KOREAN_CP)
        {
            /*
             * This is Japanese/Korean case,
             * These characters maps grid of half width.
             * so, same as U+2500.
             */
            ScreenInfo->LineChar[UPPER_LEFT_CORNER]   = 0x0001;
            ScreenInfo->LineChar[UPPER_RIGHT_CORNER]  = 0x0002;
            ScreenInfo->LineChar[HORIZONTAL_LINE]     = 0x0006;
            ScreenInfo->LineChar[VERTICAL_LINE]       = 0x0005;
            ScreenInfo->LineChar[BOTTOM_LEFT_CORNER]  = 0x0003;
            ScreenInfo->LineChar[BOTTOM_RIGHT_CORNER] = 0x0004;
        }
        else
        {
            /*
             * This is FE case,
             * FE don't uses U+2500 because these grid characters
             * maps to full width.
             */
            ScreenInfo->LineChar[UPPER_LEFT_CORNER]   = L'+';
            ScreenInfo->LineChar[UPPER_RIGHT_CORNER]  = L'+';
            ScreenInfo->LineChar[HORIZONTAL_LINE]     = L'-';
            ScreenInfo->LineChar[VERTICAL_LINE]       = L'|';
            ScreenInfo->LineChar[BOTTOM_LEFT_CORNER]  = L'+';
            ScreenInfo->LineChar[BOTTOM_RIGHT_CORNER] = L'+';
        }
    }
    else {
        ScreenInfo->LineChar[UPPER_LEFT_CORNER]   = 0x250c;
        ScreenInfo->LineChar[UPPER_RIGHT_CORNER]  = 0x2510;
        ScreenInfo->LineChar[HORIZONTAL_LINE]     = 0x2500;
        ScreenInfo->LineChar[VERTICAL_LINE]       = 0x2502;
        ScreenInfo->LineChar[BOTTOM_LEFT_CORNER]  = 0x2514;
        ScreenInfo->LineChar[BOTTOM_RIGHT_CORNER] = 0x2518;
    }
}

BOOL
CheckBisectStringA(
    IN DWORD CodePage,
    IN PCHAR Buffer,
    IN DWORD NumBytes,
    IN LPCPINFO lpCPInfo
    )

/*++

Routine Description:

    This routine check bisected on Ascii string end.

Arguments:

    CodePage - Value of code page.

    Buffer - Pointer to Ascii string buffer.

    NumBytes - Number of Ascii string.

Return Value:

    TRUE - Bisected character.

    FALSE - Correctly.

--*/

{
    UNREFERENCED_PARAMETER(CodePage);

    while(NumBytes) {
        if (IsDBCSLeadByteConsole(*Buffer,lpCPInfo)) {
            if (NumBytes <= 1)
                return TRUE;
            else {
                Buffer += 2;
                NumBytes -= 2;
            }
        }
        else {
            Buffer++;
            NumBytes--;
        }
    }
    return FALSE;
}



VOID
BisectWrite(
    IN SHORT StringLength,
    IN COORD TargetPoint,
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine write buffer with bisect.

Arguments:

Return Value:

--*/

{
    SHORT RowIndex;
    PROW Row;
    PROW RowPrev;
    PROW RowNext;

#if defined(DBG) && defined(DBG_KATTR)
    BeginKAttrCheck(ScreenInfo);
#endif

#ifdef FE_SB
    // Check code for must CONSOLE_TEXTMODE_BUFFER !!
    ASSERT(!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER));
#endif

    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+TargetPoint.Y) % ScreenInfo->ScreenBufferSize.Y;
    Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];

    if (RowIndex > 0) {
        RowPrev = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex-1];
    } else {
        RowPrev = &ScreenInfo->BufferInfo.TextInfo.Rows[ScreenInfo->ScreenBufferSize.Y-1];
    }

    if (RowIndex+1 < ScreenInfo->ScreenBufferSize.Y) {
        RowNext = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex+1];
    } else {
        RowNext = &ScreenInfo->BufferInfo.TextInfo.Rows[0];
    }

    //
    // Check start position of strings
    //
    if (Row->CharRow.KAttrs[TargetPoint.X] & ATTR_TRAILING_BYTE)
    {
        if (TargetPoint.X == 0) {
            RowPrev->CharRow.Chars[ScreenInfo->ScreenBufferSize.X-1] = UNICODE_SPACE;
            RowPrev->CharRow.KAttrs[ScreenInfo->ScreenBufferSize.X-1] = 0;
            ScreenInfo->BisectFlag |= BISECT_TOP;
        }
        else {
            Row->CharRow.Chars[TargetPoint.X-1] = UNICODE_SPACE;
            Row->CharRow.KAttrs[TargetPoint.X-1] = 0;
            ScreenInfo->BisectFlag |= BISECT_LEFT;
        }
    }

    //
    // Check end position of strings
    //
    if (TargetPoint.X+StringLength < ScreenInfo->ScreenBufferSize.X) {
        if (Row->CharRow.KAttrs[TargetPoint.X+StringLength] & ATTR_TRAILING_BYTE)
          {
            Row->CharRow.Chars[TargetPoint.X+StringLength] = UNICODE_SPACE;
            Row->CharRow.KAttrs[TargetPoint.X+StringLength] = 0;
            ScreenInfo->BisectFlag |= BISECT_RIGHT;
        }
    }
    else if (TargetPoint.Y+1 < ScreenInfo->ScreenBufferSize.Y) {
        if (RowNext->CharRow.KAttrs[0] & ATTR_TRAILING_BYTE)
        {
            RowNext->CharRow.Chars[0] = UNICODE_SPACE;
            RowNext->CharRow.KAttrs[0] = 0;
            ScreenInfo->BisectFlag |= BISECT_BOTTOM;
        }
    }
}

VOID
BisectClipbrd(
    IN SHORT StringLength,
    IN COORD TargetPoint,
    IN PSCREEN_INFORMATION ScreenInfo,
    OUT PSMALL_RECT SmallRect
    )

/*++

Routine Description:

    This routine check bisect for clipboard process.

Arguments:

Return Value:

--*/

{
    SHORT RowIndex;
    PROW Row;
    PROW RowNext;

#if defined(DBG) && defined(DBG_KATTR)
//    BeginKAttrCheck(ScreenInfo);
#endif

#ifdef FE_SB
    // Check code for must CONSOLE_TEXTMODE_BUFFER !!
    ASSERT(!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER));
#endif

    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+TargetPoint.Y) % ScreenInfo->ScreenBufferSize.Y;
    Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];

    if (RowIndex+1 < ScreenInfo->ScreenBufferSize.Y) {
        RowNext = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex+1];
    } else {
        RowNext = &ScreenInfo->BufferInfo.TextInfo.Rows[0];
    }

    //
    // Check start position of strings
    //
    ASSERT(CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console));
    if (Row->CharRow.KAttrs[TargetPoint.X] & ATTR_TRAILING_BYTE)
    {
        if (TargetPoint.X == 0) {
            SmallRect->Left++;
        }
        else {
            SmallRect->Left--;
        }
    }
    //
    // Check end position of strings
    //
    if (TargetPoint.X+StringLength < ScreenInfo->ScreenBufferSize.X) {
        if (Row->CharRow.KAttrs[TargetPoint.X+StringLength] & ATTR_TRAILING_BYTE)
        {
            SmallRect->Right++;
        }
    }
    else if (TargetPoint.Y+1 < ScreenInfo->ScreenBufferSize.Y) {
        if (RowNext->CharRow.KAttrs[0] & ATTR_TRAILING_BYTE)
        {
            SmallRect->Right--;
        }
    }
}


VOID
BisectWriteAttr(
    IN SHORT StringLength,
    IN COORD TargetPoint,
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine write buffer with bisect.

Arguments:

Return Value:

--*/

{
    SHORT RowIndex;
    PROW Row;
    PROW RowNext;

#if defined(DBG) && defined(DBG_KATTR)
    BeginKAttrCheck(ScreenInfo);
#endif

#ifdef FE_SB
    // Check code for must CONSOLE_TEXTMODE_BUFFER !!
    ASSERT(!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER));
#endif

    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+TargetPoint.Y) % ScreenInfo->ScreenBufferSize.Y;
    Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];

    if (RowIndex+1 < ScreenInfo->ScreenBufferSize.Y) {
        RowNext = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex+1];
    } else {
        RowNext = &ScreenInfo->BufferInfo.TextInfo.Rows[0];
    }

    //
    // Check start position of strings
    //
    if (Row->CharRow.KAttrs[TargetPoint.X] & ATTR_TRAILING_BYTE){
        if (TargetPoint.X == 0) {
            ScreenInfo->BisectFlag |= BISECT_TOP;
        }
        else {
            ScreenInfo->BisectFlag |= BISECT_LEFT;
        }
    }

    //
    // Check end position of strings
    //
    if (TargetPoint.X+StringLength < ScreenInfo->ScreenBufferSize.X) {
        if (Row->CharRow.KAttrs[TargetPoint.X+StringLength] & ATTR_TRAILING_BYTE){
            ScreenInfo->BisectFlag |= BISECT_RIGHT;
        }
    }
    else if (TargetPoint.Y+1 < ScreenInfo->ScreenBufferSize.Y) {
        if (RowNext->CharRow.KAttrs[0] & ATTR_TRAILING_BYTE){
            ScreenInfo->BisectFlag |= BISECT_BOTTOM;
        }
    }
}



/***************************************************************************\
* BOOL IsConsoleFullWidth(HDC hDC,DWORD CodePage,WCHAR wch)
*
* Determine if the given Unicode char is fullwidth or not.
*
* Return:
*     FASLE : half width. Uses 1 column per one character
*     TRUE  : full width. Uses 2 columns per one character
*
* History:
* 04-08-92 ShunK       Created.
* Jul-27-1992 KazuM    Added Screen Information and Code Page Information.
* Jan-29-1992 V-Hirots Substruct Screen Information.
* Oct-06-1996 KazuM    Not use RtlUnicodeToMultiByteSize and WideCharToMultiByte
*                      Because 950 only defined 13500 chars,
*                      and unicode defined almost 18000 chars.
*                      So there are almost 4000 chars can not be mapped to big5 code.
\***************************************************************************/

BOOL IsConsoleFullWidth(
    IN HDC hDC,
    IN DWORD CodePage,
    IN WCHAR wch
    )
{
    INT Width;
    TEXTMETRIC tmi;

    if (! IsAvailableFarEastCodePage(CodePage))
        return FALSE;

    if (0x20 <= wch && wch <= 0x7e)
        /* ASCII */
        return FALSE;
    else if (0x3041 <= wch && wch <= 0x3094)
        /* Hiragana */
        return TRUE;
    else if (0x30a1 <= wch && wch <= 0x30f6)
        /* Katakana */
        return TRUE;
    else if (0x3105 <= wch && wch <= 0x312c)
        /* Bopomofo */
        return TRUE;
    else if (0x3131 <= wch && wch <= 0x318e)
        /* Hangul Elements */
        return TRUE;
    else if (0xac00 <= wch && wch <= 0xd7a3)
        /* Korean Hangul Syllables */
        return TRUE;
    else if (0xff01 <= wch && wch <= 0xff5e)
        /* Fullwidth ASCII variants */
        return TRUE;
    else if (0xff61 <= wch && wch <= 0xff9f)
        /* Halfwidth Katakana variants */
        return FALSE;
    else if ( (0xffa0 <= wch && wch <= 0xffbe) ||
              (0xffc2 <= wch && wch <= 0xffc7) ||
              (0xffca <= wch && wch <= 0xffcf) ||
              (0xffd2 <= wch && wch <= 0xffd7) ||
              (0xffda <= wch && wch <= 0xffdc)   )
        /* Halfwidth Hangule variants */
        return FALSE;
    else if (0xffe0 <= wch && wch <= 0xffe6)
        /* Fullwidth symbol variants */
        return TRUE;
    else if (0x4e00 <= wch && wch <= 0x9fa5)
        /* Han Ideographic */
        return TRUE;
    else if (0xf900 <= wch && wch <= 0xfa2d)
        /* Han Compatibility Ideographs */
        return TRUE;
    else
    {
        BOOL ret;

        /* Unknown character */

        ret = GetTextMetricsW(hDC, &tmi);
        if (! ret) {
            KdPrint(("CONSRV: IsConsoleFullWidth: GetTextMetricsW failed 0x%x\n",GetLastError()));
            return FALSE;
        }
        if (IS_ANY_DBCS_CHARSET(tmi.tmCharSet))
            tmi.tmMaxCharWidth /= 2;

        ret = GetCharWidth32(hDC, wch, wch, &Width);
        if (! ret) {
            KdPrint(("CONSRV: IsConsoleFullWidth: GetCharWidth32 failed 0x%x\n",GetLastError()));
            return FALSE;
        }
        if (Width == tmi.tmMaxCharWidth)
            return FALSE;
        else if (Width == tmi.tmMaxCharWidth*2)
            return TRUE;
    }
    ASSERT(FALSE);
    return FALSE;
}


/*++

Routine Description:

    This routine remove DBCS padding code.

Arguments:

    Dst - Pointer to destination.

    Src - Pointer to source.

    NumBytes - Number of string.

    OS2OemFormat -

Return Value:

--*/

DWORD
RemoveDbcsMark(
    IN PWCHAR Dst,
    IN PWCHAR Src,
    IN DWORD NumBytes,
    IN PCHAR SrcA,
    IN BOOL OS2OemFormat
    )
{
    PWCHAR Tmp = Dst;

    if (NumBytes == 0 || NumBytes >= 0xffffffff)
        return( 0 );

#if defined(i386)
    if (OS2OemFormat) {
        RealUnicodeToNEC_OS2_Unicode(Src, NumBytes);
    }
#endif

    if (SrcA) {
        while (NumBytes--)
        {
            if (!(*SrcA++ & ATTR_TRAILING_BYTE))
                *Dst++ = *Src;
            Src++;
        }
        return (ULONG)(Dst - Tmp);
    }
    else {
        RtlCopyMemory(Dst,Src,NumBytes * sizeof(WCHAR)) ;
        return(NumBytes) ;
    }
#if !defined(i386)
    UNREFERENCED_PARAMETER(OS2OemFormat);
#endif
}

/*++

Routine Description:

    This routine remove DBCS padding code for cell format.

Arguments:

    Dst - Pointer to destination.

    Src - Pointer to source.

    NumBytes - Number of string.

Return Value:

--*/

DWORD
RemoveDbcsMarkCell(
    IN PCHAR_INFO Dst,
    IN PCHAR_INFO Src,
    IN DWORD NumBytes
    )
{
    PCHAR_INFO Tmp = Dst;
    DWORD TmpByte;

    TmpByte = NumBytes;
    while (NumBytes--) {
        if (!(Src->Attributes & COMMON_LVB_TRAILING_BYTE)){
            *Dst = *Src;
            Dst->Attributes &= ~COMMON_LVB_SBCSDBCS;
            Dst++;
        }
        Src++;
    }
    NumBytes = (ULONG)(TmpByte - (Dst - Tmp));
    RtlZeroMemory(Dst, NumBytes * sizeof(CHAR_INFO));
    Dst += NumBytes;

    return (ULONG)(Dst - Tmp);
}

DWORD
RemoveDbcsMarkAll(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PROW Row,
    IN PSHORT LeftChar,
    IN PRECT TextRect,
    IN int *TextLeft,
    IN PWCHAR Buffer,
    IN SHORT NumberOfChars
    )
{
    BOOL OS2OemFormat = FALSE;

#if defined(i386)
    if ((ScreenInfo->Console->Flags & CONSOLE_OS2_REGISTERED) &&
        (ScreenInfo->Console->Flags & CONSOLE_OS2_OEM_FORMAT) &&
        (ScreenInfo->Console->OutputCP == OEMCP)) {
        OS2OemFormat = TRUE;
    }
#endif // i386

    if (NumberOfChars <= 0)
        return NumberOfChars;

    if ( !CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console))
    {
        return RemoveDbcsMark(Buffer,
                              &Row->CharRow.Chars[*LeftChar],
                              NumberOfChars,
                              NULL,
                              OS2OemFormat
                             );
    }
    else if ( *LeftChar > ScreenInfo->Window.Left &&  Row->CharRow.KAttrs[*LeftChar] & ATTR_TRAILING_BYTE)
    {
        TextRect->left -= SCR_FONTSIZE(ScreenInfo).X;
        --*LeftChar;
        if (TextLeft)
            *TextLeft = TextRect->left;
        return RemoveDbcsMark(Buffer,
                              &Row->CharRow.Chars[*LeftChar],
                              NumberOfChars+1,
                              &Row->CharRow.KAttrs[*LeftChar],
                              OS2OemFormat
                             );
    }
    else if (*LeftChar == ScreenInfo->Window.Left && Row->CharRow.KAttrs[*LeftChar] & ATTR_TRAILING_BYTE)
    {
        *Buffer = UNICODE_SPACE;
        return RemoveDbcsMark(Buffer+1,
                              &Row->CharRow.Chars[*LeftChar+1],
                              NumberOfChars-1,
                              &Row->CharRow.KAttrs[*LeftChar+1],
                              OS2OemFormat
                             ) + 1;
    }
    else
    {
        return RemoveDbcsMark(Buffer,
                              &Row->CharRow.Chars[*LeftChar],
                              NumberOfChars,
                              &Row->CharRow.KAttrs[*LeftChar],
                              OS2OemFormat
                             );
    }
}


BOOL
IsDBCSLeadByteConsole(
    IN BYTE AsciiChar,
    IN LPCPINFO lpCPInfo
    )
{
    int i;

    i = 0;
    while (lpCPInfo->LeadByte[i]) {
        if (lpCPInfo->LeadByte[i] <= AsciiChar && AsciiChar <= lpCPInfo->LeadByte[i+1])
            return TRUE;
        i += 2;
    }
    return FALSE;
}


NTSTATUS
AdjustFont(
    IN PCONSOLE_INFORMATION Console,
    IN UINT CodePage
    )
{
    PSCREEN_INFORMATION ScreenInfo = Console->CurrentScreenBuffer;
    ULONG FontIndex;
    static const COORD NullCoord = {0, 0};
    TEXT_BUFFER_FONT_INFO TextFontInfo;
    NTSTATUS Status;

    Status = FindTextBufferFontInfo(ScreenInfo,
                                    CodePage,
                                    &TextFontInfo);
    if (NT_SUCCESS(Status)) {
        FontIndex = FindCreateFont(TextFontInfo.Family,
                                   TextFontInfo.FaceName,
                                   TextFontInfo.FontSize,
                                   TextFontInfo.Weight,
                                   CodePage);
    }
    else {
        FontIndex = FindCreateFont(0,
                                   SCR_FACENAME(ScreenInfo),
                                   NullCoord,                  // sets new font by FontSize=0
                                   0,
                                   CodePage);
    }
#ifdef i386
    if (! (Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
        SetScreenBufferFont(Console->CurrentScreenBuffer,FontIndex, CodePage);
    }
    else {
        BOOL fChange = FALSE;

        if ((Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) &&
            (GetForegroundWindow() == Console->hWnd)                    )
        {
            ChangeDispSettings(Console, Console->hWnd, 0);
            fChange = TRUE;
        }
        SetScreenBufferFont(Console->CurrentScreenBuffer,FontIndex, CodePage);
        ConvertToFullScreen(Console);
        if (fChange &&
            (GetForegroundWindow() == Console->hWnd))
            ChangeDispSettings(Console, Console->hWnd, CDS_FULLSCREEN);
    }
#else
    SetScreenBufferFont(Console->CurrentScreenBuffer,FontIndex, CodePage);
#endif
    return STATUS_SUCCESS;
}


NTSTATUS
ConvertToCodePage(
    IN PCONSOLE_INFORMATION Console,
    IN UINT PrevCodePage
    )
{
    PSCREEN_INFORMATION Cur;

    if (Console->OutputCP != OEMCP && PrevCodePage == OEMCP)
    {

        for (Cur=Console->ScreenBuffers;Cur!=NULL;Cur=Cur->Next) {

            if (Cur->Flags & CONSOLE_GRAPHICS_BUFFER) {
                continue;
            }

            ConvertOutputOemToNonOemUnicode(
                Cur->BufferInfo.TextInfo.TextRows,
                Cur->BufferInfo.TextInfo.DbcsScreenBuffer.KAttrRows,
                Cur->ScreenBufferSize.X * Cur->ScreenBufferSize.Y,
                Console->OutputCP);

            if ((Cur->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
                RealUnicodeToFalseUnicode(
                    Cur->BufferInfo.TextInfo.TextRows,
                    Cur->ScreenBufferSize.X * Cur->ScreenBufferSize.Y,
                    Console->OutputCP);
            }
        }

        if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
            PCONVERSIONAREA_INFORMATION ConvAreaInfo;
            ConvAreaInfo = Console->ConsoleIme.ConvAreaRoot;
            while (ConvAreaInfo) {
                Cur = ConvAreaInfo->ScreenBuffer;

                if (!(Cur->Flags & CONSOLE_GRAPHICS_BUFFER)) {

                    ConvertOutputOemToNonOemUnicode(
                        Cur->BufferInfo.TextInfo.TextRows,
                        Cur->BufferInfo.TextInfo.DbcsScreenBuffer.KAttrRows,
                        Cur->ScreenBufferSize.X * Cur->ScreenBufferSize.Y,
                        Console->OutputCP);

                    if ((Cur->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                        ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
                        RealUnicodeToFalseUnicode(
                            Cur->BufferInfo.TextInfo.TextRows,
                            Cur->ScreenBufferSize.X * Cur->ScreenBufferSize.Y,
                            Console->OutputCP);
                    }
                }

                ConvAreaInfo = ConvAreaInfo->ConvAreaNext;
            }

            Console->CurrentScreenBuffer->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
        }

#ifdef FE_SB
        else
        {
            // Check code for must CONSOLE_TEXTMODE_BUFFER !!
            ASSERT(FALSE);
        }
#endif

        SetWindowSize(Console->CurrentScreenBuffer);
        WriteToScreen(Console->CurrentScreenBuffer,&Console->CurrentScreenBuffer->Window);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ConvertOutputOemToNonOemUnicode(
    IN OUT LPWSTR Source,
    IN OUT PBYTE KAttrRows,
    IN int SourceLength, // in chars
    IN UINT Codepage
    )
{
    NTSTATUS Status;
    LPSTR  pTemp;
    LPWSTR pwTemp;
    ULONG TempLength;
    ULONG Length;
    BOOL NormalChars;
    int i;

    if (SourceLength == 0 )
        return STATUS_SUCCESS;

    NormalChars = TRUE;
    for (i=0;i<SourceLength;i++) {
        if (Source[i] > 0x7f) {
            NormalChars = FALSE;
            break;
        }
    }
    if (NormalChars) {
        return STATUS_SUCCESS;
    }

    pTemp = (LPSTR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),SourceLength);
    if (pTemp == NULL) {
        return STATUS_NO_MEMORY;
    }

    pwTemp = (LPWSTR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),SourceLength * sizeof(WCHAR));
    if (pwTemp == NULL) {
        ConsoleHeapFree(pTemp);
        return STATUS_NO_MEMORY;
    }

    TempLength = RemoveDbcsMark(pwTemp,
                                Source,
                                SourceLength,
                                KAttrRows,
                                FALSE);

    Status = RtlUnicodeToOemN(pTemp,
                              (ULONG)ConsoleHeapSize(pTemp),
                              &Length,
                              pwTemp,
                              TempLength * sizeof(WCHAR)
                             );
    if (!NT_SUCCESS(Status)) {
        ConsoleHeapFree(pTemp);
        ConsoleHeapFree(pwTemp);
        return Status;
    }

    MultiByteToWideChar(Codepage,
                        0,
                        pTemp,
                        Length,
                        Source,
                        SourceLength
                       );
    ConsoleHeapFree(pTemp);
    ConsoleHeapFree(pwTemp);

    if (!NT_SUCCESS(Status)) {
        return Status;
    } else {
        if (KAttrRows) {
            RtlZeroMemory(KAttrRows, SourceLength);
        }
        return STATUS_SUCCESS;
    }
}






VOID
TextOutEverything(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN SHORT LeftWindowPos,
    IN OUT PSHORT RightWindowPos,
    IN OUT PSHORT CountOfAttr,
    IN SHORT CountOfAttrOriginal,
    IN OUT PBOOL DoubleColorDBCS,
    IN BOOL LocalEUDCFlag,
    IN PROW Row,
    IN PATTR_PAIR Attr,
    IN SHORT LeftTextPos,
    IN SHORT RightTextPos,
    IN int WindowRectLeft,
    IN RECT WindowRect,
    IN SHORT NumberOfChars
    )

/*++

Routine Description:

    This routine text out everything.

Arguments:

Return Value:

--*/

{
    int   j = LeftWindowPos;
    int   TextLeft = WindowRectLeft;
    RECT TextRect = WindowRect;
    SHORT LeftChar = LeftTextPos;
    SHORT RightChar = RightTextPos;
    BOOL  DoubleColorDBCSBefore;
    BOOL  LocalEUDCFlagBefore;
    PEUDC_INFORMATION EudcInfo;

    int   RightPos  = j + *CountOfAttr - 1;
    int   RightText = LeftChar + *CountOfAttr - 1;
    BOOL  OS2OemFormat = FALSE;

#ifdef FE_SB
    // Check code for must CONSOLE_TEXTMODE_BUFFER !!
    ASSERT(!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER));
#endif

#if defined(i386)
    if ((ScreenInfo->Console->Flags & CONSOLE_OS2_REGISTERED) &&
        (ScreenInfo->Console->Flags & CONSOLE_OS2_OEM_FORMAT) &&
        (ScreenInfo->Console->OutputCP == OEMCP)) {
        OS2OemFormat = TRUE;
    }
#endif // i386

#if defined(DBG) && defined(DBG_KATTR)
    BeginKAttrCheck(ScreenInfo);
#endif

    RightText = min(RightText,(ScreenInfo->ScreenBufferSize.X-1));

    LocalEUDCFlagBefore = LocalEUDCFlag ;
    EudcInfo = (PEUDC_INFORMATION)Console->EudcInformation;

    DoubleColorDBCSBefore = *DoubleColorDBCS ;
    if (DoubleColorDBCSBefore){
        RECT TmpRect;

        if (Console->FonthDC == NULL) {
            Console->FonthDC = CreateCompatibleDC(Console->hDC);
            Console->hBitmap = CreateBitmap(DEFAULT_FONTSIZE, DEFAULT_FONTSIZE, BITMAP_PLANES, BITMAP_BITS_PIXEL, NULL);
            SelectObject(Console->FonthDC, Console->hBitmap);
        }

        if (LocalEUDCFlagBefore){
            if (EudcInfo->hDCLocalEudc == NULL) {
                EudcInfo->hDCLocalEudc = CreateCompatibleDC(Console->hDC);
                EudcInfo->hBmpLocalEudc = CreateBitmap(EudcInfo->LocalEudcSize.X,
                                                       EudcInfo->LocalEudcSize.Y,
                                                       BITMAP_PLANES, BITMAP_BITS_PIXEL, NULL);
                SelectObject(EudcInfo->hDCLocalEudc, EudcInfo->hBmpLocalEudc);
            }
            GetFitLocalEUDCFont(Console,
                                Row->CharRow.Chars[LeftChar-1]);
            BitBlt(Console->hDC,
                   TextRect.left,
                   TextRect.top,
                   SCR_FONTSIZE(ScreenInfo).X,
                   SCR_FONTSIZE(ScreenInfo).Y,
                   EudcInfo->hDCLocalEudc,
                   SCR_FONTSIZE(ScreenInfo).X,
                   0,
                   SRCCOPY
                  );
            TextRect.left += SCR_FONTSIZE(ScreenInfo).X;
            TextLeft +=  SCR_FONTSIZE(ScreenInfo).X;
            TextRect.right += SCR_FONTSIZE(ScreenInfo).X;
            (*CountOfAttr)++;
            NumberOfChars = 0;
        }
        else{
            TmpRect.left = 0;
            TmpRect.top = 0;
            TmpRect.right = SCR_FONTSIZE(ScreenInfo).X;
            TmpRect.bottom = SCR_FONTSIZE(ScreenInfo).Y;

            SelectObject(Console->FonthDC,
                         FontInfo[SCR_FONTNUMBER(ScreenInfo)].hFont
                        );

            ExtTextOutW(Console->FonthDC,
                        0,
                        0,
                        ETO_OPAQUE,
                        &TmpRect,
                        &Row->CharRow.Chars[LeftChar-1],
                        1,
                        NULL
                       );
            BitBlt(Console->hDC,
                   TextRect.left,
                   TextRect.top,
                   SCR_FONTSIZE(ScreenInfo).X,
                   SCR_FONTSIZE(ScreenInfo).Y,
                   Console->FonthDC,
                   SCR_FONTSIZE(ScreenInfo).X,
                   0,
                   SRCCOPY
                  );
            TextRect.left += SCR_FONTSIZE(ScreenInfo).X;
            TextLeft += SCR_FONTSIZE(ScreenInfo).X;
            NumberOfChars = (SHORT)RemoveDbcsMark(ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter,
                                                  &Row->CharRow.Chars[LeftChar+1],
                                                  NumberOfChars-1,
                                                  &Row->CharRow.KAttrs[LeftChar+1],
                                                  OS2OemFormat);
        }

    }
    else {
        NumberOfChars = (SHORT)RemoveDbcsMarkAll(ScreenInfo,
                                                 Row,
                                                 &LeftChar,
                                                 &TextRect,
                                                 &TextLeft,
                                                 ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter,
                                                 NumberOfChars);
    }


    *DoubleColorDBCS = FALSE ;
    if ((NumberOfChars != 0) && (Row->CharRow.KAttrs[RightText] & ATTR_LEADING_BYTE)){
        if (RightPos >= ScreenInfo->Window.Right)
            *(ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter+NumberOfChars-1) = UNICODE_SPACE;
        else if(TextRect.right <= ScreenInfo->Window.Right * SCR_FONTSIZE(ScreenInfo).X) {
            *DoubleColorDBCS = TRUE;
            TextRect.right += SCR_FONTSIZE(ScreenInfo).X;
            if((j == *RightWindowPos)&&
               (*RightWindowPos < ScreenInfo->Window.Right))
            *RightWindowPos++;
        }
    }

    if( TextRect.left < TextRect.right){
        ExtTextOutW(Console->hDC,
                    TextLeft,
                    TextRect.top,
                    ETO_OPAQUE,
                    &TextRect,
                    ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter,
                    NumberOfChars,
                    NULL
                   );
    }
    if (LocalEUDCFlagBefore){
        DWORD dwFullWidth = (IsConsoleFullWidth(Console->hDC,
                                                Console->OutputCP,
                                                Row->CharRow.Chars[RightText+1]) ? 2 : 1);

        if (EudcInfo->hDCLocalEudc == NULL) {
            EudcInfo->hDCLocalEudc = CreateCompatibleDC(Console->hDC);
            EudcInfo->hBmpLocalEudc = CreateBitmap(EudcInfo->LocalEudcSize.X,
                                                   EudcInfo->LocalEudcSize.Y,
                                                   BITMAP_PLANES, BITMAP_BITS_PIXEL, NULL);
            SelectObject(EudcInfo->hDCLocalEudc, EudcInfo->hBmpLocalEudc);
        }
        GetFitLocalEUDCFont(Console,
                            Row->CharRow.Chars[RightText+1]);
        BitBlt(Console->hDC,                      // hdcDest
               TextRect.right,                    // nXDest
               TextRect.top,                      // nYDest
               SCR_FONTSIZE(ScreenInfo).X * dwFullWidth, // nWidth
               SCR_FONTSIZE(ScreenInfo).Y,    // nHeight
               EudcInfo->hDCLocalEudc,            // hdcSrc
               0,                                 // nXSrc
               0,                                 // nYSrc
               SRCCOPY
              );

        TextRect.right += (SCR_FONTSIZE(ScreenInfo).X * dwFullWidth);
        (*CountOfAttr) += (SHORT)dwFullWidth;
        if (CountOfAttrOriginal < *CountOfAttr ){
            *DoubleColorDBCS = TRUE ;
            (*CountOfAttr)--;
            TextRect.right -= SCR_FONTSIZE(ScreenInfo).X;
        }
    }
    if (DoubleColorDBCSBefore){
        TextRect.left -= SCR_FONTSIZE(ScreenInfo).X;
    }

    TextOutCommonLVB(Console, Attr->Attr, TextRect);

}

VOID
TextOutCommonLVB(
    IN PCONSOLE_INFORMATION Console,
    IN WORD Attributes,
    IN RECT CommonLVBRect
    )
{
    HBRUSH hbrSave;
    HGDIOBJ hbr;
    int GridX;

    if (Attributes & (COMMON_LVB_GRID_HORIZONTAL |
                      COMMON_LVB_GRID_LVERTICAL  |
                      COMMON_LVB_GRID_RVERTICAL  |
                      COMMON_LVB_UNDERSCORE       )
       )
    {
        if(Attributes & COMMON_LVB_UNDERSCORE){
            if(Attributes & COMMON_LVB_REVERSE_VIDEO)
                hbr = CreateSolidBrush(ConvertAttrToRGB(Console, LOBYTE(Attributes >> 4)));
            else
                hbr = CreateSolidBrush(ConvertAttrToRGB(Console, LOBYTE(Attributes)));
            hbrSave = SelectObject(Console->hDC, hbr);
            PatBlt(Console->hDC,
                   CommonLVBRect.left,
                   CommonLVBRect.bottom-1,
                   CommonLVBRect.right-CommonLVBRect.left,
                   1,
                   PATCOPY
                  );
            SelectObject(Console->hDC, hbrSave);
            DeleteObject(hbr);
        }

        if(Attributes & (COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_LVERTICAL | COMMON_LVB_GRID_RVERTICAL)){
            hbr = CreateSolidBrush(ConvertAttrToRGB(Console, 0x0007));
            hbrSave = SelectObject(Console->hDC, hbr);

            if(Attributes & COMMON_LVB_GRID_HORIZONTAL){
                PatBlt(Console->hDC,
                       CommonLVBRect.left,
                       CommonLVBRect.top,
                       CommonLVBRect.right-CommonLVBRect.left,
                       1,
                       PATCOPY
                      );
            }
            if(Attributes & COMMON_LVB_GRID_LVERTICAL){
                for ( GridX = CommonLVBRect.left ;
                      GridX < CommonLVBRect.right ;
                      GridX += CON_FONTSIZE(Console).X){
                    PatBlt(Console->hDC,
                           GridX,
                           CommonLVBRect.top,
                           1,
                           CON_FONTSIZE(Console).Y,
                           PATCOPY
                          );
                }
            }
            if(Attributes & COMMON_LVB_GRID_RVERTICAL){
                for ( GridX = CommonLVBRect.left + CON_FONTSIZE(Console).X-1 ;
                      GridX < CommonLVBRect.right ;
                      GridX += CON_FONTSIZE(Console).X){
                    PatBlt(Console->hDC,
                           GridX,
                           CommonLVBRect.top,
                           1,
                           CON_FONTSIZE(Console).Y,
                           PATCOPY
                          );
                }
            }
            SelectObject(Console->hDC, hbrSave);
            DeleteObject(hbr);
        }
    }
}

NTSTATUS
MakeAltRasterFont(
    UINT CodePage,
    COORD DefaultFontSize,
    COORD *AltFontSize,
    BYTE  *AltFontFamily,
    ULONG *AltFontIndex,
    LPWSTR AltFaceName
    )
{
    DWORD i;
    DWORD Find;
    ULONG FontIndex;
    COORD FontSize = DefaultFontSize;
    COORD FontDelta;
    BOOL  fDbcsCharSet = IsAvailableFarEastCodePage(CodePage);

    FontIndex = 0;
    Find = (DWORD)-1;
    for (i=0; i < NumberOfFonts; i++)
    {
        if (!TM_IS_TT_FONT(FontInfo[i].Family) &&
            IS_ANY_DBCS_CHARSET(FontInfo[i].tmCharSet) == fDbcsCharSet
           )
        {
            FontDelta.X = abs(FontSize.X - FontInfo[i].Size.X);
            FontDelta.Y = abs(FontSize.Y - FontInfo[i].Size.Y);
            if (Find > (DWORD)(FontDelta.X + FontDelta.Y))
            {
                Find = (DWORD)(FontDelta.X + FontDelta.Y);
                FontIndex = i;
            }
        }
    }

    *AltFontIndex = FontIndex;
    wcscpy(AltFaceName, FontInfo[*AltFontIndex].FaceName);
    *AltFontSize = FontInfo[*AltFontIndex].Size;
    *AltFontFamily = FontInfo[*AltFontIndex].Family;

    DBGFONTS(("MakeAltRasterFont : AltFontIndex = %ld\n", *AltFontIndex));

    return STATUS_SUCCESS;
}


NTSTATUS
InitializeDbcsMisc(
    VOID
    )
{
    HANDLE hkRegistry = NULL;
    NTSTATUS Status;
    WCHAR awchValue[ 512 ];
    WCHAR awchData[ 512 ];
    BYTE  Buffer[ 512 ];
    DWORD Length;
    DWORD dwIndex;
    LPWSTR pwsz;

    ASSERT(gTTFontList.Next==NULL);
    ASSERT(gRegFullScreenCodePage.Next==NULL);

    gTTFontList.Next = NULL;
    gRegFullScreenCodePage.Next = NULL;

    /*
     * Get TrueType Font Face name from registry.
     */
    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_CONSOLE_TTFONT,
                          &hkRegistry);
    if (!NT_SUCCESS( Status )) {
        DBGPRINT(("CONSRV: NtOpenKey failed %ws\n", MACHINE_REGISTRY_CONSOLE_TTFONT));
    }
    else {
        LPTTFONTLIST pTTFontList;

        for( dwIndex = 0; ; dwIndex++) {
            Status = MyRegEnumValue(hkRegistry,
                                    dwIndex,
                                    sizeof(awchValue), (LPWSTR)&awchValue,
                                    sizeof(awchData),  (PBYTE)&awchData);
            if (!NT_SUCCESS( Status )) {
                break;
            }

            pTTFontList = ConsoleHeapAlloc(
                                    MAKE_TAG( SCREEN_DBCS_TAG ),
                                    sizeof(TTFONTLIST));
            if (pTTFontList == NULL) {
                break;
            }

            pTTFontList->List.Next = NULL;
            pTTFontList->CodePage = ConvertStringToDec(awchValue, NULL);
            pwsz = awchData;
            if (*pwsz == BOLD_MARK) {
                pTTFontList->fDisableBold = TRUE;
                pwsz++;
            }
            else
                pTTFontList->fDisableBold = FALSE;
            wcscpy(pTTFontList->FaceName1, pwsz);

            pwsz += wcslen(pwsz) + 1;
            if (*pwsz == BOLD_MARK) {
                pTTFontList->fDisableBold = TRUE;
                pwsz++;
            }
            wcscpy(pTTFontList->FaceName2, pwsz);

            PushEntryList(&gTTFontList, &(pTTFontList->List));
        }

        NtClose(hkRegistry);
    }

    /*
     * Get Full Screen from registry.
     */
    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_CONSOLE_FULLSCREEN,
                          &hkRegistry);
    if (!NT_SUCCESS( Status )) {
        DBGPRINT(("CONSRV: NtOpenKey failed %ws\n", MACHINE_REGISTRY_CONSOLE_FULLSCREEN));
    }
    else {
        /*
         * InitialPalette
         */
        Status = MyRegQueryValueEx(hkRegistry,
                                   MACHINE_REGISTRY_INITIAL_PALETTE,
                                   sizeof( Buffer ), Buffer, &Length);
        if (NT_SUCCESS( Status ) && Length > sizeof(DWORD)) {
            DWORD PaletteLength = ((LPDWORD)Buffer)[0];
            PUSHORT Palette;

            if (PaletteLength * sizeof(USHORT) >= (Length - sizeof(DWORD)))
            {
                Palette = ConsoleHeapAlloc(
                                    MAKE_TAG( BUFFER_TAG ),
                                    Length);
                if (Palette != NULL)
                {
                    RtlCopyMemory(Palette, Buffer, Length);
                    RegInitialPalette = Palette;
                }
            }
        }

        /*
         * ColorBuffer
         */
        Status = MyRegQueryValueEx(hkRegistry,
                                   MACHINE_REGISTRY_COLOR_BUFFER,
                                   sizeof( Buffer ), Buffer, &Length);
        if (NT_SUCCESS( Status ) && Length > sizeof(DWORD)) {
            DWORD ColorBufferLength = ((LPDWORD)Buffer)[0];
            PUCHAR Color;

            if (ColorBufferLength * sizeof(DWORD) >= (Length - sizeof(DWORD)))
            {
                Color = ConsoleHeapAlloc(
                                  MAKE_TAG( BUFFER_TAG ),
                                  Length);
                if (Color != NULL)
                {
                    RtlCopyMemory(Color, Buffer, Length);
                    RegColorBuffer = Color;
                }
            }
        }

        /*
         * ColorBufferNoTranslate
         */
        Status = MyRegQueryValueEx(hkRegistry,
                                   MACHINE_REGISTRY_COLOR_BUFFER_NO_TRANSLATE,
                                   sizeof( Buffer ), Buffer, &Length);
        if (NT_SUCCESS( Status ) && Length > sizeof(DWORD)) {
            DWORD ColorBufferLength = ((LPDWORD)Buffer)[0];
            PUCHAR Color;

            if (ColorBufferLength * sizeof(DWORD) >= (Length - sizeof(DWORD)))
            {
                Color = ConsoleHeapAlloc(
                                  MAKE_TAG( BUFFER_TAG ),
                                  Length);
                if (Color != NULL)
                {
                    RtlCopyMemory(Color, Buffer, Length);
                    RegColorBufferNoTranslate = Color;
                }
            }
        }

        /*
         * ModeFontPairs
         */
        Status = MyRegQueryValueEx(hkRegistry,
                                   MACHINE_REGISTRY_MODE_FONT_PAIRS,
                                   sizeof( Buffer ), Buffer, &Length);
        if (NT_SUCCESS( Status ) && Length > sizeof(DWORD)) {
            DWORD NumOfEntries = ((LPDWORD)Buffer)[0];
            PMODE_FONT_PAIR ModeFont;

            if (NumOfEntries * sizeof(MODE_FONT_PAIR) >= (Length - sizeof(DWORD)))
            {
                ModeFont = ConsoleHeapAlloc(
                                     MAKE_TAG( BUFFER_TAG ),
                                     Length);
                if (ModeFont != NULL)
                {
                    Length -= sizeof(DWORD);
                    RtlCopyMemory(ModeFont, &Buffer[sizeof(DWORD)], Length);
                    RegModeFontPairs = ModeFont;
                    NUMBER_OF_MODE_FONT_PAIRS = NumOfEntries;
                }
            }
        }

        /*
         * FullScreen\CodePage
         */
        {
            HANDLE hkRegCP = NULL;

            Status = MyRegOpenKey(hkRegistry,
                                  MACHINE_REGISTRY_FS_CODEPAGE,
                                  &hkRegCP);
            if (!NT_SUCCESS( Status )) {
                DBGPRINT(("CONSRV: NtOpenKey failed %ws\n", MACHINE_REGISTRY_FS_CODEPAGE));
            }
            else {
                PFS_CODEPAGE pFsCodePage;

                for( dwIndex = 0; ; dwIndex++) {
                    Status = MyRegEnumValue(hkRegCP,
                                            dwIndex,
                                            sizeof(awchValue), (LPWSTR)&awchValue,
                                            sizeof(awchData),  (PBYTE)&awchData);
                    if (!NT_SUCCESS( Status )) {
                        break;
                    }

                    pFsCodePage = ConsoleHeapAlloc(
                                            MAKE_TAG( BUFFER_TAG ),
                                            sizeof(FS_CODEPAGE));
                    if (pFsCodePage == NULL) {
                        break;
                    }

                    pFsCodePage->List.Next = NULL;
                    pFsCodePage->CodePage = ConvertStringToDec(awchValue, NULL);

                    PushEntryList(&gRegFullScreenCodePage, &(pFsCodePage->List));
                }

                NtClose(hkRegCP);
            }
        }


        NtClose(hkRegistry);
    }


#if defined(i386)
    Status = NtGetMachineIdentifierValue(&gdwMachineId);
    if (!NT_SUCCESS(Status)) {
        gdwMachineId = MACHINEID_MS_PCAT;
    }
#endif

    Status = RtlInitializeCriticalSectionAndSpinCount(&ConIMEInitWindowsLock,
                                                      0x80000000);

    return Status;
}

#if defined(i386)
NTSTATUS
RealUnicodeToNEC_OS2_Unicode(
    IN OUT LPWSTR Source,
    IN int SourceLength      // in chars
    )

/*

    this routine converts a unicode string from the real unicode characters
    to the NEC OS/2 unicode characters.

*/

{
    NTSTATUS Status;
    LPSTR Temp;
    ULONG TempLength;
    ULONG Length;
    CHAR StackBuffer[STACK_BUFFER_SIZE];
    BOOL NormalChars;
    int i;

    DBGCHARS(("RealUnicodeToNEC_OS2_Unicode U->ACP:???->U %.*ls\n",
            SourceLength > 10 ? 10 : SourceLength, Source));
    NormalChars = TRUE;

    if (pGlyph_NEC_OS2_CP == NULL || pGlyph_NEC_OS2_CP->MultiByteTable == NULL) {
        DBGCHARS(("RealUnicodeToNEC_OS2_Unicode  xfer buffer null\n"));
        return STATUS_SUCCESS;  // there's nothing we can do
    }

    /*
     * Test for characters < 0x20.  If none are found, we don't have
     * any conversion to do!
     */
    for (i=0;i<SourceLength;i++) {
        if ((USHORT)(Source[i]) < 0x20) {
            NormalChars = FALSE;
            break;
        }
    }
    if (NormalChars) {
        return STATUS_SUCCESS;
    }

    TempLength = SourceLength;
    if (TempLength > STACK_BUFFER_SIZE) {
        Temp = (LPSTR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),TempLength);
        if (Temp == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else {
        Temp = StackBuffer;
    }
    Status = RtlUnicodeToMultiByteN(Temp,
                                    TempLength,
                                    &Length,
                                    Source,
                                    SourceLength * sizeof(WCHAR)
                                   );
    if (!NT_SUCCESS(Status)) {
        if (TempLength > STACK_BUFFER_SIZE) {
            ConsoleHeapFree(Temp);
        }
        return Status;
    }
    ASSERT(pGlyph_NEC_OS2_CP != NULL && pGlyph_NEC_OS2_CP->MultiByteTable != NULL);
    Status = RtlCustomCPToUnicodeN(pGlyph_NEC_OS2_CP,
                                   Source,
                                   SourceLength * sizeof(WCHAR),
                                   &Length,
                                   Temp,
                                   TempLength
                                  );
    if (TempLength > STACK_BUFFER_SIZE) {
        ConsoleHeapFree(Temp);
    }
    if (!NT_SUCCESS(Status)) {
        return Status;
    } else {
        return STATUS_SUCCESS;
    }
}

BOOL
InitializeNEC_OS2_CP(
    VOID
    )
{
    PPEB pPeb;

    pPeb = NtCurrentPeb();
    if ((pPeb == NULL) || (pPeb->OemCodePageData == NULL)) {
        return FALSE;
    }

    /*
     * Fill in the CPTABLEINFO struct
     */
    if (pGlyph_NEC_OS2_CP == NULL) {
        pGlyph_NEC_OS2_CP = (PCPTABLEINFO)ConsoleHeapAlloc( MAKE_TAG(SCREEN_DBCS_TAG), sizeof(CPTABLEINFO));
        if (pGlyph_NEC_OS2_CP == NULL) {
            return FALSE;
        }
    }
    RtlInitCodePageTable(pPeb->OemCodePageData, pGlyph_NEC_OS2_CP);

    /*
     * Make a copy of the MultiByteToWideChar table
     */
    if (pGlyph_NEC_OS2_Table == NULL) {
        pGlyph_NEC_OS2_Table = (PUSHORT)ConsoleHeapAlloc( MAKE_TAG(SCREEN_DBCS_TAG), 256 * sizeof(USHORT));
        if (pGlyph_NEC_OS2_Table == NULL) {
            return FALSE;
        }
    }
    RtlCopyMemory(pGlyph_NEC_OS2_Table, pGlyph_NEC_OS2_CP->MultiByteTable, 256 * sizeof(USHORT));

    /*
     * Modify the first 0x20 bytes so that they are glyphs.
     */
    MultiByteToWideChar(CP_OEMCP, MB_USEGLYPHCHARS,
            "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
            "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x1E\x1F\x1C\x07",
            0x20, pGlyph_NEC_OS2_Table, 0x20);


    /*
     * Point the Custom CP at the glyph table
     */
    pGlyph_NEC_OS2_CP->MultiByteTable = pGlyph_NEC_OS2_Table;

    return TRUE;
}
#endif // i386

BYTE
CodePageToCharSet(
    UINT CodePage
    )
{
    CHARSETINFO csi;

    if (!TranslateCharsetInfo((DWORD *)CodePage, &csi, TCI_SRCCODEPAGE))
        csi.ciCharset = OEM_CHARSET;

    return (BYTE)csi.ciCharset;
}

BOOL
IsAvailableFarEastCodePage(
    UINT CodePage
    )
{
    BYTE CharSet = CodePageToCharSet(CodePage);

    return IS_ANY_DBCS_CHARSET(CharSet);
}

LPTTFONTLIST
SearchTTFont(
    LPWSTR pwszFace,
    BOOL   fCodePage,
    UINT   CodePage
    )
{
    PSINGLE_LIST_ENTRY pTemp = gTTFontList.Next;

    if (pwszFace) {
        while (pTemp != NULL) {
            LPTTFONTLIST pTTFontList = (LPTTFONTLIST)pTemp;

            if (wcscmp(pwszFace, pTTFontList->FaceName1) == 0 ||
                wcscmp(pwszFace, pTTFontList->FaceName2) == 0    ) {
                if (fCodePage)
                    if (pTTFontList->CodePage == CodePage )
                        return pTTFontList;
                    else
                        return NULL;
                else
                    return pTTFontList;
            }

            pTemp = pTemp->Next;
        }
    }

    return NULL;
}

BOOL
IsAvailableTTFont(
    LPWSTR pwszFace
    )
{
    if (SearchTTFont(pwszFace, FALSE, 0))
        return TRUE;
    else
        return FALSE;
}

BOOL
IsAvailableTTFontCP(
    LPWSTR pwszFace,
    UINT CodePage
    )
{
    if (SearchTTFont(pwszFace, TRUE, CodePage))
        return TRUE;
    else
        return FALSE;
}

LPWSTR
GetAltFaceName(
    LPWSTR pwszFace
    )
{
    LPTTFONTLIST pTTFontList;

    pTTFontList = SearchTTFont(pwszFace, FALSE, 0);
    if (pTTFontList != NULL) {
        if (wcscmp(pwszFace, pTTFontList->FaceName1) == 0) {
            return pTTFontList->FaceName2;
        }
        if (wcscmp(pwszFace, pTTFontList->FaceName2) == 0) {
            return pTTFontList->FaceName1;
        }
        return NULL;
    }
    else
        return NULL;
}

BOOL
IsAvailableFsCodePage(
    UINT CodePage
    )
{
    PSINGLE_LIST_ENTRY pTemp = gRegFullScreenCodePage.Next;

    while (pTemp != NULL) {
        PFS_CODEPAGE pFsCodePage = (PFS_CODEPAGE)pTemp;

        if (pFsCodePage->CodePage == CodePage) {
            return TRUE;
        }

        pTemp = pTemp->Next;
    }

    return FALSE;
}


#if defined(FE_IME)
/*
 * Console IME executing logic.
 *
 * KERNEL32:ConDllInitialize
 *            If Reason is DLL_PROCESS_ATTACH
 *            |
 *            V
 * WINSRV:ConsoleClientConnectRoutine
 *          |
 *          V
 *          SetUpConsole
 *            |
 *            V
 *            AllocateConsole
 *              PostThreadMessage(CM_CREATE_CONSOLE_WINDOW)
 *          |
 *          V
 *          UnlockConsoleHandleTable
 *          InitConsoleIMEStuff
 *            |
 *            V
 *            If never register console IME
 *              hThread = InternalCreateCallbackThread(ConsoleIMERoutine)
 *              QueueThreadMessage(CM_WAIT_CONIME_PROCESS)
 *            |
 *            V
 *            QueueThreadMessage(CM_CONIME_CREATE)
 *          |
 *          V
 * KERNEL32:NtWaitForMultipleObjects(InitEvents)
 *
 *
 * WINSRV:InputThread
 *          |
 *          V
 *          GetMessage
 *            Receive CM_CREATE_CONSOLE_WINDOW
 *              |
 *              V
 *              ProcessCreateConsoleWindow
 *                |
 *                V
 *                CreateWindowsWindow
 *                  |
 *                  V
 *                  CreateWindowEx
 *                  NtSetEvent(InitEvents)
 *          |
 *          V
 *          GetMessage
 *            Receive CM_WAIT_CONIME_PROCESS (this case is never register console IME)
 *              |
 *              V
 *              WaitConsoleIMEStuff
 *                If never register console IME
 *                  NtWaitForSingleObject(hThread, 20 sec)
 *
 *
 * KERNEL32:ConsoleIMERoutine
 *            |
 *            V
 *            hEvent = CreateEvent(CONSOLEIME_EVENT)
 *            If not exist named event
 *              CreateProcess(conime.exe)
 *              WaitForSingleObject(hEvent, 10 sec)
 *              If WAIT_TIMEOUT
 *                TerminateProcess
 *            |
 *            V
 *            TerminateThread(hThread)
 *
 *
 * CONIME:WinMain
 *          |
 *          V
 *          CreateWindow
 *          RegisterConsoleIME
 *            |
 *            V
 *            WINSRV:ConSrvRegisterConsoleIME
 *                     |
 *                     V
 *                     QueueThreadMessage(CM_SET_CONSOLEIME_WINDOW)
 *          |
 *          V
 *          AttachThreadInput
 *          SetEvent(CONSOLEIME_EVENT)
 *
 *
 * WINSRV:InputThread
 *          |
 *          V
 *          GetMessage
 *            Receive CM_CONIME_CREATE
 *              |
 *              V
 *              ProcessCreateConsoleIME
 *                If available hWndConsoleIME
 *                  hIMC = SendMessage(console IME, CONIME_CREATE)
 *                Else
 *                  PostMessage(CM_CONIME_CREATE)
 *          |
 *          V
 *          GetMessage
 *            Receive CM_SET_CONSOLEIME_WINDOW
 *              TlsGetValue()->hWndConsoleIME = wParam
 *
 *
 * TerminateProcess of Console IME
 *   WINSRV:ConsoleClientDisconnectRoutine
 *            |
 *            V
 *            RemoveConsoleIME
 */

VOID
ProcessCreateConsoleIME(
    IN LPMSG lpMsg,
    DWORD dwConsoleThreadId
    )
{
    NTSTATUS Status;
    HANDLE ConsoleHandle = (HANDLE)lpMsg->wParam;
    PCONSOLE_INFORMATION pConsole;
    HWND hwndConIme;

    Status = RevalidateConsole(ConsoleHandle, &pConsole);
    if (!NT_SUCCESS(Status)) {
        return;  // STATUS_PROCESS_IS_TERMINATING
    }

    hwndConIme = pConsole->InputThreadInfo->hWndConsoleIME;

    if (pConsole->InputThreadInfo->hWndConsoleIME != NULL) {
        LRESULT lResult;

        if (!NT_SUCCESS(ConsoleImeMessagePumpWorker(pConsole,
                                  CONIME_CREATE,
                                  (WPARAM)pConsole->ConsoleHandle,
                                  (LPARAM)pConsole->hWnd,
                                  &lResult
                                 ))) {
            goto TerminateConsoleIme;
        }

        if (lResult)
        {
            DBGPRINT(("ConsoleIME connected(Create)\n")) ;

            if (!CONSOLE_IS_DBCS_CP(pConsole))
                pConsole->InputBuffer.ImeMode.Disable = TRUE;

            CreateConvAreaModeSystem(pConsole);

            if ((pConsole->Flags & CONSOLE_HAS_FOCUS) ||
                (pConsole->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE))
            {
                if (!NT_SUCCESS(ConsoleImeMessagePump(pConsole,
                                      CONIME_SETFOCUS,
                                      (WPARAM)pConsole->ConsoleHandle,
                                      (LPARAM)pConsole->hklActive
                                     ))) {
                    goto TerminateConsoleIme;
                }
            }

            if (pConsole->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER)
            {
                if (!NT_SUCCESS(ConsoleImeMessagePump(pConsole,
                                      CONIME_NOTIFY_SCREENBUFFERSIZE,
                                      (WPARAM)pConsole->ConsoleHandle,
                                      (LPARAM)MAKELPARAM(pConsole->CurrentScreenBuffer->ScreenBufferSize.X,
                                                         pConsole->CurrentScreenBuffer->ScreenBufferSize.Y)
                                     ))) {
                    goto TerminateConsoleIme;
                }
            }
            if (!NT_SUCCESS(ConsoleImeMessagePump(pConsole,
                                  CONIME_NOTIFY_CODEPAGE,
                                  (WPARAM)pConsole->ConsoleHandle,
                                  (LPARAM)MAKELPARAM(FALSE, pConsole->CP)
                                 ))) {
                goto TerminateConsoleIme;
            }
            if (!NT_SUCCESS(ConsoleImeMessagePump(pConsole,
                                  CONIME_NOTIFY_CODEPAGE,
                                  (WPARAM)pConsole->ConsoleHandle,
                                  (LPARAM)MAKELPARAM(TRUE, pConsole->OutputCP)
                                 ))) {
                goto TerminateConsoleIme;
            }

            if (!NT_SUCCESS(GetImeKeyState(pConsole, NULL))) {
                goto TerminateConsoleIme;
            }
        }
    }
    else if (lpMsg->lParam) {
        /*
         * This case, First = TRUE
         * Again post message of CM_CONIME_CREATE.
         * Becase hWndConsoleIME available when CM_SET_CONSOLEIME_WINDOW message
         * and it message will be run after this.
         */
        Status = QueueThreadMessage(dwConsoleThreadId,
                                    CM_CONIME_CREATE,
                                    (WPARAM)ConsoleHandle,
                                    (LPARAM)FALSE
                                    );

        if (!NT_SUCCESS(Status)) {
                RIPMSG1(RIP_WARNING, "QueueThreadMessage(CM_CONIME_CREATE) failed (%08x)\n", Status);
        }
    }
    UnlockConsole(pConsole);

    return;

TerminateConsoleIme:
    if (IsWindow(hwndConIme)) {
        PostMessage(hwndConIme, CONIME_DESTROY, (WPARAM)ConsoleHandle, (LPARAM)NULL);
    }
}

NTSTATUS
InitConsoleIMEStuff(
    HDESK hDesktop,
    DWORD dwConsoleThreadId,
    PCONSOLE_INFORMATION Console
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    CONSOLE_REGISTER_CONSOLEIME RegConIMEInfo;
    HANDLE hThread = NULL;
    BOOL First = FALSE;

    if (!gfLoadConIme) {
        KdPrint(("CONSRV: InitConsoleIMEStuff is skipping conime loading\n"));
        return STATUS_UNSUCCESSFUL; // the return value does not really matter...
    }

    RtlEnterCriticalSection(&ConIMEInitWindowsLock);

    RegConIMEInfo.hdesk      = hDesktop;
    RegConIMEInfo.dwThreadId = 0;
    RegConIMEInfo.dwAction   = REGCONIME_QUERY;
    NtUserConsoleControl(ConsoleRegisterConsoleIME, &RegConIMEInfo, sizeof(RegConIMEInfo));
    if (RegConIMEInfo.dwThreadId == 0)
    {
        /*
         * Create a Remote Thread on client side.
         * This remote thread do create a console IME process.
         */
        hThread = InternalCreateCallbackThread(CONSOLE_CLIENTPROCESSHANDLE(),
                                               (ULONG_PTR)ConsoleIMERoutine,
                                               (ULONG_PTR)0);
        if (hThread == NULL) {
            KdPrint(("CONSRV: CreateRemoteThread failed %x\n",GetLastError()));
        }
        else
        {
            /*
             * CM_WAIT_CONIME_PROCESS
             * This message wait for ready to go console IME process.
             */
            Status = QueueThreadMessage(dwConsoleThreadId,
                                        CM_WAIT_CONIME_PROCESS,
                                        (WPARAM)hDesktop,
                                        (LPARAM)hThread
                                        );
            if (!NT_SUCCESS(Status)) {
                RIPMSG1(RIP_WARNING, "QueueThreadMessage(CM_WAIT_CONIME_PROCESS) failed (%08x)\n", Status);
            } else {
                First = TRUE;
            }
        }
    }

    Status = QueueThreadMessage(dwConsoleThreadId,
                                CM_CONIME_CREATE,
                                (WPARAM)Console->ConsoleHandle,
                                (LPARAM)First
                                );
    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "InitConsoleIMEStuff: QueueThreadMessage(CM_CONIME_CREATE) failed (%08x)\n", Status);
    }

    RtlLeaveCriticalSection(&ConIMEInitWindowsLock);

    return Status;
}

NTSTATUS
WaitConsoleIMEStuff(
    HDESK hDesktop,
    HANDLE hThread
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    CONSOLE_REGISTER_CONSOLEIME RegConIMEInfo;

    RtlEnterCriticalSection(&ConIMEInitWindowsLock);

    RegConIMEInfo.hdesk      = hDesktop;
    RegConIMEInfo.dwThreadId = 0;
    RegConIMEInfo.dwAction   = REGCONIME_QUERY;
    NtUserConsoleControl(ConsoleRegisterConsoleIME, &RegConIMEInfo, sizeof(RegConIMEInfo));

    RtlLeaveCriticalSection(&ConIMEInitWindowsLock);

    if (RegConIMEInfo.dwThreadId == 0)
    {
        int cLoops;
        LARGE_INTEGER li;

        /*
         * Do wait for ready to go console IME process.
         *
         * This wait code should after the CreateWindowsWindow
         * because doesn't finish DLL attach on client side
         * for wait a Console->InitEvents.
         */
        cLoops = 80;
        li.QuadPart = (LONGLONG)-10000 * 250;
        while (cLoops--)
        {
            /*
             * Sleep for a second.
             */
            Status = NtWaitForSingleObject(hThread, FALSE, &li);
            if (Status != STATUS_TIMEOUT) {
                break;
            }

            RtlEnterCriticalSection(&ConIMEInitWindowsLock);
            RegConIMEInfo.hdesk      = hDesktop;
            RegConIMEInfo.dwThreadId = 0;
            RegConIMEInfo.dwAction   = REGCONIME_QUERY;
            NtUserConsoleControl(ConsoleRegisterConsoleIME, &RegConIMEInfo, sizeof(RegConIMEInfo));
            RtlLeaveCriticalSection(&ConIMEInitWindowsLock);
            if (RegConIMEInfo.dwThreadId != 0) {
                break;
            }
        }
    }

    NtClose(hThread);

    return Status;
}

NTSTATUS
ConSrvRegisterConsoleIME(
    PCSR_PROCESS Process,
    HDESK hDesktop,
    HWINSTA hWinSta,
    HWND  hWndConsoleIME,
    DWORD dwConsoleIMEThreadId,
    DWORD dwAction,
    DWORD *dwConsoleThreadId
    )
{
    NTSTATUS Status;
    CONSOLE_REGISTER_CONSOLEIME RegConIMEInfo;
    PCONSOLE_PER_PROCESS_DATA ProcessData;

    RtlEnterCriticalSection(&ConIMEInitWindowsLock);

    ProcessData = CONSOLE_FROMPROCESSPERPROCESSDATA(Process);

    RegConIMEInfo.hdesk      = hDesktop;
    RegConIMEInfo.dwThreadId = 0;
    RegConIMEInfo.dwAction   = REGCONIME_QUERY;
    NtUserConsoleControl(ConsoleRegisterConsoleIME, &RegConIMEInfo, sizeof(RegConIMEInfo));
    if (RegConIMEInfo.dwConsoleInputThreadId == 0)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto ErrorExit;
    }

    if (RegConIMEInfo.dwThreadId == 0)
    {
        /*
         * never registered console ime thread.
         */
        if (dwAction == REGCONIME_REGISTER)
        {
            /*
             * register
             */
            RegConIMEInfo.hdesk      = hDesktop;
            RegConIMEInfo.dwThreadId = dwConsoleIMEThreadId;
            RegConIMEInfo.dwAction   = dwAction;
            Status = NtUserConsoleControl(ConsoleRegisterConsoleIME, &RegConIMEInfo, sizeof(RegConIMEInfo));
            if (NT_SUCCESS(Status)) {
                Status = QueueThreadMessage(RegConIMEInfo.dwConsoleInputThreadId,
                                            CM_SET_CONSOLEIME_WINDOW,
                                            (WPARAM)hWndConsoleIME,
                                            0
                                            );
                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_WARNING, "ConSrvRegisterConsoleIME: QueueThreadMessage failed (%08x)\n", Status);
                    Status = STATUS_UNSUCCESSFUL;
                    goto ErrorExit;
                }

                ProcessData->hDesk = hDesktop;
                ProcessData->hWinSta = hWinSta;

                if (dwConsoleThreadId)
                    *dwConsoleThreadId = RegConIMEInfo.dwConsoleInputThreadId;
            }
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
            goto ErrorExit;
        }
    }
    else
    {
        /*
         * do registered console ime thread.
         */
        if ( (dwAction == REGCONIME_UNREGISTER) ||
             (dwAction == REGCONIME_TERMINATE)     )
        {
            /*
             * unregister
             */
            RegConIMEInfo.hdesk      = hDesktop;
            RegConIMEInfo.dwThreadId = dwConsoleIMEThreadId;
            RegConIMEInfo.dwAction   = dwAction;
            Status = NtUserConsoleControl(ConsoleRegisterConsoleIME, &RegConIMEInfo, sizeof(RegConIMEInfo));
            if (NT_SUCCESS(Status)) {
                Status = QueueThreadMessage(RegConIMEInfo.dwConsoleInputThreadId,
                                            CM_SET_CONSOLEIME_WINDOW,
                                            (WPARAM)NULL,
                                            0
                                            );
                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_WARNING, "ConSrvRegisterConsoleIME: QueueThreadMessage failed (%08x)\n", Status);
                    Status = STATUS_UNSUCCESSFUL;
                    goto ErrorExit;
                }

                CloseDesktop(ProcessData->hDesk);
                CloseWindowStation(ProcessData->hWinSta);

                ProcessData->hDesk = NULL;
                ProcessData->hWinSta = NULL;

                if (dwConsoleThreadId)
                    *dwConsoleThreadId = RegConIMEInfo.dwConsoleInputThreadId;
            }
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
            goto ErrorExit;
        }
    }

ErrorExit:
    if (! NT_SUCCESS(Status))
    {
        CloseDesktop(hDesktop);
        CloseWindowStation(hWinSta);
    }

    RtlLeaveCriticalSection(&ConIMEInitWindowsLock);

    return Status;
}


VOID
RemoveConsoleIME(
    PCSR_PROCESS Process,
    DWORD dwConsoleIMEThreadId
    )
{
    NTSTATUS Status;
    CONSOLE_REGISTER_CONSOLEIME RegConIMEInfo;
    PCONSOLE_PER_PROCESS_DATA ProcessData;

    ProcessData = CONSOLE_FROMPROCESSPERPROCESSDATA(Process);

    //
    // This is console IME process
    //
    RtlEnterCriticalSection(&ConIMEInitWindowsLock);

    RegConIMEInfo.hdesk      = ProcessData->hDesk;
    RegConIMEInfo.dwThreadId = 0;
    RegConIMEInfo.dwAction   = REGCONIME_QUERY;
    NtUserConsoleControl(ConsoleRegisterConsoleIME, &RegConIMEInfo, sizeof(RegConIMEInfo));
    if (RegConIMEInfo.dwConsoleInputThreadId == 0)
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    if (dwConsoleIMEThreadId == RegConIMEInfo.dwThreadId)
    {
        /*
         * Unregister console IME
         */
        Status = ConSrvRegisterConsoleIME(Process,
                                          ProcessData->hDesk,
                                          ProcessData->hWinSta,
                                          NULL,
                                          dwConsoleIMEThreadId,
                                          REGCONIME_TERMINATE,
                                          NULL
                                         );
    }
    RtlLeaveCriticalSection(&ConIMEInitWindowsLock);

    return;
}


/*
 * Console IME message pump.
 *
 * Note for NT5 --- this function is build on bogus assumptions
 * (also has some nasty workaround for sloppy conime).
 * There's a chance that pConsole goes away while sendmessage
 * is processed by conime.
 * Keep in mind, anybody who calls this function should validate
 * the return status as appropriate.
 */

NTSTATUS
ConsoleImeMessagePumpWorker(
    PCONSOLE_INFORMATION Console,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam,
    LRESULT* lplResult)
{
    HWND    hWndConsoleIME = Console->InputThreadInfo->hWndConsoleIME;
    LRESULT fNoTimeout;
    PINPUT_THREAD_INFO InputThreadInfo;     // console thread info

    *lplResult = 0;

    if (hWndConsoleIME == NULL)
    {
        return STATUS_SUCCESS;
    }

    InputThreadInfo = TlsGetValue(InputThreadTlsIndex);

    if (InputThreadInfo != NULL)
    {
        HWND hWnd = Console->hWnd;

        /*
         * This thread is in InputThread()
         * We can try message pump.
         */

        fNoTimeout = SendMessageTimeout(hWndConsoleIME,
                                        Message,
                                        wParam,
                                        lParam,
                                        SMTO_ABORTIFHUNG | SMTO_NORMAL,
                                        CONIME_SENDMSG_TIMEOUT,
                                        lplResult);
        if (fNoTimeout)
        {
            return STATUS_SUCCESS;
        }

        if ((Console = GetWindowConsole(hWnd)) == NULL ||
                (Console->Flags & CONSOLE_TERMINATING)) {

            // This console is terminated.
            // ConsoleImeMessagePump gives up SendMessage to conime.

            return STATUS_INVALID_HANDLE;
        }

        /*
         * Timeout return from SendMessageTimeout
         * Or, Hung hWndConsoleIME.
         */
    }

    /*
     * This thread is in CsrApiRequestThread()
     * We can not try message pump.
     */

    DBGPRINT(("ConsoleImeMessagePumpWorker::PostMessage(0x%x)\n",Message));
    PostMessage(hWndConsoleIME,
                Message,
                wParam,
                lParam);

    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleImeMessagePump(
    PCONSOLE_INFORMATION Console,
    UINT   Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LRESULT lResultDummy;

    return ConsoleImeMessagePumpWorker(Console, Message, wParam, lParam, &lResultDummy);
}

#endif // FE_IME




BOOL
RegisterKeisenOfTTFont(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    NTSTATUS Status;
    COORD FontSize;
    DWORD BuffSize;
    LPSTRINGBITMAP StringBitmap;
    WCHAR wChar;
    WCHAR wCharBuf[2];
    ULONG ulNumFonts;
    DWORD dwFonts;
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    GetNumFonts(&ulNumFonts);
    for (dwFonts=0; dwFonts < ulNumFonts; dwFonts++) {
        if (!TM_IS_TT_FONT(FontInfo[dwFonts].Family) &&
            IS_ANY_DBCS_CHARSET(FontInfo[dwFonts].tmCharSet)
           ) {
            GetFontSize(dwFonts, &FontSize);
            BuffSize = CalcBitmapBufferSize(FontSize,BYTE_ALIGN);
            StringBitmap = ConsoleHeapAlloc( MAKE_TAG( TMP_DBCS_TAG ), sizeof(STRINGBITMAP) + BuffSize);
            if (StringBitmap == NULL) {
                RIPMSG1(RIP_WARNING, "RegisterKeisenOfTTFont: cannot allocate memory (%d bytes)",
                          sizeof(STRINGBITMAP) + BuffSize);
                return FALSE;
            }

            if (SelectObject(Console->hDC,FontInfo[dwFonts].hFont)==0) {
                goto error_return;
            }

            for (wChar=0; wChar < UNICODE_SPACE; wChar++) {
                wCharBuf[0] = wChar;
                wCharBuf[1] = TEXT('\0');;
                if (GetStringBitmapW(Console->hDC,
                                     wCharBuf,
                                     1,
                                     sizeof(STRINGBITMAP) + BuffSize,
                                     (BYTE*)StringBitmap
                                    ) == 0) {
                    goto error_return;
                }
                FontSize.X = (WORD)StringBitmap->uiWidth;
                FontSize.Y = (WORD)StringBitmap->uiHeight;
                Status = RegisterLocalEUDC(Console,wChar,FontSize,StringBitmap->ajBits);
                if (! NT_SUCCESS(Status)) {
error_return:
                    ConsoleHeapFree(StringBitmap);
                    return FALSE;
                }
            }

            ConsoleHeapFree(StringBitmap);
        }
        ((PEUDC_INFORMATION)(Console->EudcInformation))->LocalKeisenEudcMode = TRUE ;
    }
    return TRUE;
}

ULONG
TranslateUnicodeToOem(
    IN PCONSOLE_INFORMATION Console,
    IN PWCHAR UnicodeBuffer,
    IN ULONG UnicodeCharCount,
    OUT PCHAR AnsiBuffer,
    IN ULONG AnsiByteCount,
    OUT PINPUT_RECORD DbcsLeadInpRec
    )
{
    ULONG i,j;
    PWCHAR TmpUni;
    BYTE AsciiDbcs[2];
    ULONG NumBytes;

    TmpUni = ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),UnicodeCharCount*sizeof(WCHAR));
    if (TmpUni == NULL)
        return 0;

    memcpy(TmpUni,UnicodeBuffer,UnicodeCharCount*sizeof(WCHAR));
    AsciiDbcs[1] = 0;
    for (i=0,j=0; i<UnicodeCharCount; i++,j++) {
        if (IsConsoleFullWidth(Console->hDC,Console->CP,TmpUni[i])) {
            NumBytes = sizeof(AsciiDbcs);
            ConvertToOem(Console->CP,
                   &TmpUni[i],
                   1,
                   &AsciiDbcs[0],
                   NumBytes
                   );
            if (IsDBCSLeadByteConsole(AsciiDbcs[0],&Console->CPInfo)) {
                if (j < AnsiByteCount-1) {  // -1 is safe DBCS in buffer
                    AnsiBuffer[j] = AsciiDbcs[0];
                    j++;
                    AnsiBuffer[j] = AsciiDbcs[1];
                    AsciiDbcs[1] = 0;
                }
                else if (j == AnsiByteCount-1) {
                    AnsiBuffer[j] = AsciiDbcs[0];
                    j++;
                    break;
                }
                else {
                    AsciiDbcs[1] = 0;
                    break;
                }
            }
            else {
                AnsiBuffer[j] = AsciiDbcs[0];
                AsciiDbcs[1] = 0;
            }
        }
        else {
            ConvertToOem(Console->CP,
                   &TmpUni[i],
                   1,
                   &AnsiBuffer[j],
                   1
                   );
        }
    }
    if (DbcsLeadInpRec) {
        if (AsciiDbcs[1]) {
            DbcsLeadInpRec->EventType = KEY_EVENT;
            DbcsLeadInpRec->Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[1];
        }
        else {
            RtlZeroMemory(DbcsLeadInpRec,sizeof(INPUT_RECORD));
        }
    }
    ConsoleHeapFree(TmpUni);
    return j;
}


DWORD
ImmConversionToConsole(
    DWORD fdwConversion
    )
{
    DWORD dwNlsMode;

    if (GetKeyState(VK_KANA) & KEY_TOGGLED) {
        fdwConversion = (fdwConversion & ~IME_CMODE_LANGUAGE) | (IME_CMODE_NATIVE | IME_CMODE_KATAKANA);
    }

    dwNlsMode = 0;
    if (fdwConversion & IME_CMODE_NATIVE) {
        if (fdwConversion & IME_CMODE_KATAKANA)
            dwNlsMode |= NLS_KATAKANA;
        else
            dwNlsMode |= NLS_HIRAGANA;
    }
    else {
        dwNlsMode |= NLS_ALPHANUMERIC;
    }

    if (fdwConversion & IME_CMODE_FULLSHAPE)
        dwNlsMode |= NLS_DBCSCHAR;

    if (fdwConversion & IME_CMODE_ROMAN)
        dwNlsMode |= NLS_ROMAN;

    if (fdwConversion & IME_CMODE_OPEN)
        dwNlsMode |= NLS_IME_CONVERSION;

    if (fdwConversion & IME_CMODE_DISABLE)
        dwNlsMode |= NLS_IME_DISABLE;

    return dwNlsMode;
}

DWORD
ImmConversionFromConsole(
    DWORD dwNlsMode
    )
{
    DWORD fdwConversion;

    fdwConversion = 0;
    if (dwNlsMode & (NLS_KATAKANA | NLS_HIRAGANA)) {
        fdwConversion |= IME_CMODE_NATIVE;
        if (dwNlsMode & NLS_KATAKANA)
            fdwConversion |= IME_CMODE_KATAKANA;
    }

    if (dwNlsMode & NLS_DBCSCHAR)
        fdwConversion |= IME_CMODE_FULLSHAPE;

    if (dwNlsMode & NLS_ROMAN)
        fdwConversion |= IME_CMODE_ROMAN;

    if (dwNlsMode & NLS_IME_CONVERSION)
        fdwConversion |= IME_CMODE_OPEN;

    if (dwNlsMode & NLS_IME_DISABLE)
        fdwConversion |= IME_CMODE_DISABLE;

    return fdwConversion;
}












#if defined(DBG) && defined(DBG_KATTR)
VOID
BeginKAttrCheck(
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    SHORT RowIndex;
    PROW Row;
    SHORT i;

    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow) % ScreenInfo->ScreenBufferSize.Y;
    Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];

    for (i=0;i<ScreenInfo->ScreenBufferSize.Y;i++) {
        ASSERT (Row->CharRow.KAttrs);
        if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
            RowIndex = 0;
        }
    }
}
#endif // DBG && DBG_KATTR

#endif
