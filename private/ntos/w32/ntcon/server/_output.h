/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    _output.h

Abstract:

    Performance critical routine for Single Binary

    Each function will be created with two flavors FE and non FE

Author:

    KazuM Jun.11.1997

Revision History:

--*/

#define WWSB_NEUTRAL_FILE 1

#if !defined(FE_SB)
#error This header file should be included with FE_SB
#endif

#if !defined(WWSB_FE) && !defined(WWSB_NOFE)
#error Either WWSB_FE and WWSB_NOFE must be defined.
#endif

#if defined(WWSB_FE) && defined(WWSB_NOFE)
#error Both WWSB_FE and WWSB_NOFE defined.
#endif

#include "dispatch.h"

#if defined(WWSB_FE)
#pragma alloc_text(FE_TEXT, FE_StreamWriteToScreenBuffer)
#pragma alloc_text(FE_TEXT, FE_WriteRectToScreenBuffer)
#pragma alloc_text(FE_TEXT, FE_WriteRegionToScreen)
#pragma alloc_text(FE_TEXT, FE_WriteToScreen)
#pragma alloc_text(FE_TEXT, FE_WriteOutputString)
#pragma alloc_text(FE_TEXT, FE_FillOutput)
#pragma alloc_text(FE_TEXT, FE_FillRectangle)
#pragma alloc_text(FE_TEXT, FE_PolyTextOutCandidate)
#pragma alloc_text(FE_TEXT, FE_ConsolePolyTextOut)
#endif


#if defined(WWSB_NOFE)
VOID
SB_StreamWriteToScreenBuffer(
    IN PWCHAR String,
    IN SHORT StringLength,
    IN PSCREEN_INFORMATION ScreenInfo
    )
#else
VOID
FE_StreamWriteToScreenBuffer(
    IN PWCHAR String,
    IN SHORT StringLength,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PCHAR StringA
    )
#endif
{
    SHORT RowIndex;
    PROW Row;
    PWCHAR Char;
    COORD TargetPoint;

    DBGOUTPUT(("StreamWriteToScreenBuffer\n"));
#ifdef WWSB_FE
    ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
#endif
    ScreenInfo->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;
    TargetPoint = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+TargetPoint.Y) % ScreenInfo->ScreenBufferSize.Y;
    Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
    DBGOUTPUT(("RowIndex = %x, Row = %x, TargetPoint = (%x,%x)\n",
            RowIndex, Row, TargetPoint.X, TargetPoint.Y));

    //
    // copy chars
    //
#ifdef WWSB_FE
    BisectWrite(StringLength,TargetPoint,ScreenInfo);
    if (TargetPoint.Y == ScreenInfo->ScreenBufferSize.Y-1 &&
        TargetPoint.X+StringLength >= ScreenInfo->ScreenBufferSize.X &&
        *(StringA+ScreenInfo->ScreenBufferSize.X-TargetPoint.X-1) & ATTR_LEADING_BYTE
       ) {
        *(String+ScreenInfo->ScreenBufferSize.X-TargetPoint.X-1) = UNICODE_SPACE;
        *(StringA+ScreenInfo->ScreenBufferSize.X-TargetPoint.X-1) = 0;
        if (StringLength > ScreenInfo->ScreenBufferSize.X-TargetPoint.X-1) {
            *(String+ScreenInfo->ScreenBufferSize.X-TargetPoint.X) = UNICODE_SPACE;
            *(StringA+ScreenInfo->ScreenBufferSize.X-TargetPoint.X) = 0;
        }
    }

    RtlCopyMemory(&Row->CharRow.KAttrs[TargetPoint.X],StringA,StringLength*sizeof(CHAR));
#endif

    RtlCopyMemory(&Row->CharRow.Chars[TargetPoint.X],String,StringLength*sizeof(WCHAR));

    // recalculate first and last non-space char

    Row->CharRow.OldLeft = Row->CharRow.Left;
    if (TargetPoint.X < Row->CharRow.Left) {
        PWCHAR LastChar = &Row->CharRow.Chars[ScreenInfo->ScreenBufferSize.X];

        for (Char=&Row->CharRow.Chars[TargetPoint.X];Char < LastChar && *Char==(WCHAR)' ';Char++)
            ;
        Row->CharRow.Left = (SHORT)(Char-Row->CharRow.Chars);
    }

    Row->CharRow.OldRight = Row->CharRow.Right;
    if ((TargetPoint.X+StringLength) >= Row->CharRow.Right) {
        PWCHAR FirstChar = Row->CharRow.Chars;

        for (Char=&Row->CharRow.Chars[TargetPoint.X+StringLength-1];*Char==(WCHAR)' ' && Char >= FirstChar;Char--)
            ;
        Row->CharRow.Right = (SHORT)(Char+1-FirstChar);
    }

    //
    // see if attr string is different.  if so, allocate a new
    // attr buffer and merge the two strings.
    //

    if (Row->AttrRow.Length != 1 ||
        Row->AttrRow.Attrs->Attr != ScreenInfo->Attributes) {
        PATTR_PAIR NewAttrs;
        WORD NewAttrsLength;
        ATTR_PAIR Attrs;

        Attrs.Length = StringLength;
        Attrs.Attr = ScreenInfo->Attributes;
        if (!NT_SUCCESS(MergeAttrStrings(Row->AttrRow.Attrs,
                         Row->AttrRow.Length,
                         &Attrs,
                         1,
                         &NewAttrs,
                         &NewAttrsLength,
                         TargetPoint.X,
                         (SHORT)(TargetPoint.X+StringLength-1),
                         Row,
                         ScreenInfo
                        ))) {
            return;
        }
        if (Row->AttrRow.Length > 1) {
            ConsoleHeapFree(Row->AttrRow.Attrs);
        }
        else {
            ASSERT(Row->AttrRow.Attrs == &Row->AttrRow.AttrPair);
        }
        Row->AttrRow.Attrs = NewAttrs;
        Row->AttrRow.Length = NewAttrsLength;
        Row->CharRow.OldLeft = INVALID_OLD_LENGTH;
        Row->CharRow.OldRight = INVALID_OLD_LENGTH;
    }
    ResetTextFlags(ScreenInfo,TargetPoint.Y,TargetPoint.Y);
}


#define CHAR_OF_PCI(p)  (((PCHAR_INFO)(p))->Char.AsciiChar)
#define WCHAR_OF_PCI(p) (((PCHAR_INFO)(p))->Char.UnicodeChar)
#define ATTR_OF_PCI(p)  (((PCHAR_INFO)(p))->Attributes)
#define SIZEOF_CI_CELL  sizeof(CHAR_INFO)

#define CHAR_OF_VGA(p)  (p[0])
#define ATTR_OF_VGA(p)  (p[1])
#ifdef i386
#define SIZEOF_VGA_CELL 2
#else // risc
#define SIZEOF_VGA_CELL 4
#endif


#define COMMON_LVB_MASK        0x33
#define ATTR_OF_COMMON_LVB(p)  (ATTR_OF_VGA(p) + (((p[2] & ~COMMON_LVB_MASK)) << 8))
#define SIZEOF_COMMON_LVB_CELL 4

VOID
WWSB_WriteRectToScreenBuffer(
    PBYTE Source,
    COORD SourceSize,
    PSMALL_RECT SourceRect,
    PSCREEN_INFORMATION ScreenInfo,
    COORD TargetPoint,
    IN UINT Codepage
    )

/*++

Routine Description:

    This routine copies a rectangular region to the screen buffer.
    no clipping is done.

    The source should contain Unicode or UnicodeOem chars.

Arguments:

    Source - pointer to source buffer (a real VGA buffer or CHAR_INFO[])

    SourceSize - dimensions of source buffer

    SourceRect - rectangle in source buffer to copy

    ScreenInfo - pointer to screen info

    TargetPoint - upper left coordinates of target rectangle

    Codepage - codepage to translate real VGA buffer from,
               0xFFFFFFF if Source is CHAR_INFO[] (not requiring translation)
Return Value:

    none.

--*/

{

    PBYTE SourcePtr;
    SHORT i,j;
    SHORT XSize,YSize;
    BOOLEAN WholeSource;
    SHORT RowIndex;
    PROW Row;
    PWCHAR Char;
    ATTR_PAIR Attrs[80];
    PATTR_PAIR AttrBuf;
    PATTR_PAIR Attr;
    SHORT AttrLength;
    BOOL bVGABuffer;
    ULONG ulCellSize;
#ifdef WWSB_FE
    PCHAR AttrP;
#endif

    DBGOUTPUT(("WriteRectToScreenBuffer\n"));
#ifdef WWSB_FE
    ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
#endif

    ScreenInfo->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;
    XSize = (SHORT)(SourceRect->Right - SourceRect->Left + 1);
    YSize = (SHORT)(SourceRect->Bottom - SourceRect->Top + 1);


    AttrBuf = Attrs;
    if (XSize > 80) {
        AttrBuf = (PATTR_PAIR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),XSize * sizeof(ATTR_PAIR));
        if (AttrBuf == NULL)
            return;
    }

    bVGABuffer = (Codepage != 0xFFFFFFFF);
    if (bVGABuffer) {
#ifdef WWSB_FE
        ulCellSize = (ScreenInfo->Console->fVDMVideoMode) ? SIZEOF_COMMON_LVB_CELL : SIZEOF_VGA_CELL;
#else
        ulCellSize = SIZEOF_VGA_CELL;
#endif
    } else {
        ulCellSize = SIZEOF_CI_CELL;
    }

    SourcePtr = Source;

    WholeSource = FALSE;
    if (XSize == SourceSize.X) {
        ASSERT (SourceRect->Left == 0);
        if (SourceRect->Top != 0) {
            SourcePtr += SCREEN_BUFFER_POINTER(SourceRect->Left,
                                               SourceRect->Top,
                                               SourceSize.X,
                                               ulCellSize);
        }
        WholeSource = TRUE;
    }
    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+TargetPoint.Y) % ScreenInfo->ScreenBufferSize.Y;
    for (i=0;i<YSize;i++) {
        if (!WholeSource) {
            SourcePtr = Source + SCREEN_BUFFER_POINTER(SourceRect->Left,
                                                       SourceRect->Top+i,
                                                       SourceSize.X,
                                                       ulCellSize);
        }

        //
        // copy the chars and attrs into their respective arrays
        //

#ifdef WWSB_FE
        if (! bVGABuffer) {
            COORD TPoint;

            TPoint.X = TargetPoint.X;
            TPoint.Y = TargetPoint.Y + i;
            BisectWrite(XSize,TPoint,ScreenInfo);
            if (TPoint.Y == ScreenInfo->ScreenBufferSize.Y-1 &&
                TPoint.X+XSize-1 >= ScreenInfo->ScreenBufferSize.X &&
                ATTR_OF_PCI(SourcePtr+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) & COMMON_LVB_LEADING_BYTE)
            {
                WCHAR_OF_PCI(SourcePtr+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) = UNICODE_SPACE;
                ATTR_OF_PCI(SourcePtr+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) &= ~COMMON_LVB_SBCSDBCS;
                if (XSize-1 > ScreenInfo->ScreenBufferSize.X-TPoint.X-1) {
                    WCHAR_OF_PCI(SourcePtr+ScreenInfo->ScreenBufferSize.X-TPoint.X) = UNICODE_SPACE;
                    ATTR_OF_PCI(SourcePtr+ScreenInfo->ScreenBufferSize.X-TPoint.X) &= ~COMMON_LVB_SBCSDBCS;
                }
            }
        }
#endif

        Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
        Char = &Row->CharRow.Chars[TargetPoint.X];
#ifdef WWSB_FE
        AttrP = &Row->CharRow.KAttrs[TargetPoint.X];
#endif
        Attr = AttrBuf;
        Attr->Length = 0;
        AttrLength = 1;

        /*
         * Two version of the following loop to keep it fast:
         * one for VGA buffers, one for CHAR_INFO buffers.
         */
        if (bVGABuffer) {
#ifdef WWSB_FE
            Attr->Attr = (ScreenInfo->Console->fVDMVideoMode) ? ATTR_OF_COMMON_LVB(SourcePtr) : ATTR_OF_VGA(SourcePtr);
#else
            Attr->Attr = ATTR_OF_VGA(SourcePtr);
#endif
            for (j = SourceRect->Left;
                    j <= SourceRect->Right;
                    j++,
#ifdef WWSB_FE
                    SourcePtr += (ScreenInfo->Console->fVDMVideoMode) ? SIZEOF_COMMON_LVB_CELL : SIZEOF_VGA_CELL
#else
                    SourcePtr += SIZEOF_VGA_CELL
#endif
                ) {

#ifdef WWSB_FE
                UCHAR TmpBuff[2];

                if (IsDBCSLeadByteConsole(CHAR_OF_VGA(SourcePtr),&ScreenInfo->Console->OutputCPInfo)) {
                    if (j+1 > SourceRect->Right) {
                        *Char = UNICODE_SPACE;
                        *AttrP = 0;
                    }
                    else {
                        TmpBuff[0] = CHAR_OF_VGA(SourcePtr);
                        TmpBuff[1] = CHAR_OF_VGA((SourcePtr + ((ScreenInfo->Console->fVDMVideoMode) ? SIZEOF_COMMON_LVB_CELL : SIZEOF_VGA_CELL)));
                        ConvertOutputToUnicode(Codepage,
                                               TmpBuff,
                                               2,
                                               Char,
                                               2);
                        Char++;
                        j++;
                        *AttrP++ = ATTR_LEADING_BYTE;
                        *Char++ = *(Char-1);
                        *AttrP++ = ATTR_TRAILING_BYTE;

                        if (ScreenInfo->Console->fVDMVideoMode) {
                            if (Attr->Attr == ATTR_OF_COMMON_LVB(SourcePtr)) {
                                Attr->Length += 1;
                            }
                            else {
                                Attr++;
                                Attr->Length = 1;
                                Attr->Attr = ATTR_OF_COMMON_LVB(SourcePtr);
                                AttrLength += 1;
                            }
                        }
                        else
                        {
                            if (Attr->Attr == ATTR_OF_VGA(SourcePtr)) {
                                Attr->Length += 1;
                            }
                            else {
                                Attr++;
                                Attr->Length = 1;
                                Attr->Attr = ATTR_OF_VGA(SourcePtr);
                                AttrLength += 1;
                            }
                        }

                        SourcePtr += (ScreenInfo->Console->fVDMVideoMode) ? SIZEOF_COMMON_LVB_CELL : SIZEOF_VGA_CELL;
                    }
                }
                else {
                    ConvertOutputToUnicode(Codepage,
                                           &CHAR_OF_VGA(SourcePtr),
                                           1,
                                           Char,
                                           1);
                    Char++;
                    *AttrP++ = 0;
                }
#else
                *Char++ = SB_CharToWcharGlyph(Codepage, CHAR_OF_VGA(SourcePtr));
#endif

#ifdef WWSB_FE
                if (ScreenInfo->Console->fVDMVideoMode) {
                    if (Attr->Attr == ATTR_OF_COMMON_LVB(SourcePtr)) {
                        Attr->Length += 1;
                    }
                    else {
                        Attr++;
                        Attr->Length = 1;
                        Attr->Attr = ATTR_OF_COMMON_LVB(SourcePtr);
                        AttrLength += 1;
                    }
                }
                else
#endif
                if (Attr->Attr == ATTR_OF_VGA(SourcePtr)) {
                    Attr->Length += 1;
                }
                else {
                    Attr++;
                    Attr->Length = 1;
                    Attr->Attr = ATTR_OF_VGA(SourcePtr);
                    AttrLength += 1;
                }
            }
        } else {
#ifdef WWSB_FE
            Attr->Attr = ATTR_OF_PCI(SourcePtr) & ~COMMON_LVB_SBCSDBCS;
#else
            Attr->Attr = ATTR_OF_PCI(SourcePtr);
#endif
            for (j = SourceRect->Left;
                    j <= SourceRect->Right;
                    j++, SourcePtr += SIZEOF_CI_CELL) {

                *Char++ = WCHAR_OF_PCI(SourcePtr);
#ifdef WWSB_FE
                // MSKK Apr.02.1993 V-HirotS For KAttr
                *AttrP++ = (CHAR)((ATTR_OF_PCI(SourcePtr) & COMMON_LVB_SBCSDBCS) >>8);
#endif

#ifdef WWSB_FE
                if (Attr->Attr == (ATTR_OF_PCI(SourcePtr) & ~COMMON_LVB_SBCSDBCS))
#else
                if (Attr->Attr == ATTR_OF_PCI(SourcePtr))
#endif
                {
                    Attr->Length += 1;
                }
                else {
                    Attr++;
                    Attr->Length = 1;
#ifdef WWSB_FE
                    // MSKK Apr.02.1993 V-HirotS For KAttr
                    Attr->Attr = ATTR_OF_PCI(SourcePtr) & ~COMMON_LVB_SBCSDBCS;
#else
                    Attr->Attr = ATTR_OF_PCI(SourcePtr);
#endif
                    AttrLength += 1;
                }
            }
        }

        // recalculate first and last non-space char

        Row->CharRow.OldLeft = Row->CharRow.Left;
        if (TargetPoint.X < Row->CharRow.Left) {
            PWCHAR LastChar = &Row->CharRow.Chars[ScreenInfo->ScreenBufferSize.X];

            for (Char=&Row->CharRow.Chars[TargetPoint.X];Char < LastChar && *Char==(WCHAR)' ';Char++)
                ;
            Row->CharRow.Left = (SHORT)(Char-Row->CharRow.Chars);
        }

        Row->CharRow.OldRight = Row->CharRow.Right;
        if ((TargetPoint.X+XSize) >= Row->CharRow.Right) {
            SHORT LastNonSpace;
            PWCHAR FirstChar = Row->CharRow.Chars;

            LastNonSpace = (SHORT)(TargetPoint.X+XSize-1);
            for (Char=&Row->CharRow.Chars[(TargetPoint.X+XSize-1)];*Char==(WCHAR)' ' && Char >= FirstChar;Char--)
                LastNonSpace--;

            //
            // if the attributes change after the last non-space, make the
            // index of the last attribute change + 1 the length.  otherwise
            // make the length one more than the last non-space.
            //

            Row->CharRow.Right = (SHORT)(LastNonSpace+1);
        }

        //
        // see if attr string is different.  if so, allocate a new
        // attr buffer and merge the two strings.
        //


        if (AttrLength != Row->AttrRow.Length ||
            memcmp(Row->AttrRow.Attrs,AttrBuf,AttrLength*sizeof(*Attr))) {
            PATTR_PAIR NewAttrs;
            WORD NewAttrsLength;

            if (!NT_SUCCESS(MergeAttrStrings(Row->AttrRow.Attrs,
                             Row->AttrRow.Length,
                             AttrBuf,
                             AttrLength,
                             &NewAttrs,
                             &NewAttrsLength,
                             TargetPoint.X,
                             (SHORT)(TargetPoint.X+XSize-1),
                             Row,
                             ScreenInfo
                            ))) {
                if (XSize > 80) {
                    ConsoleHeapFree(AttrBuf);
                }
                ResetTextFlags(ScreenInfo,TargetPoint.Y,(SHORT)(TargetPoint.Y+YSize-1));
                return;
            }
            if (Row->AttrRow.Length > 1) {
                ConsoleHeapFree(Row->AttrRow.Attrs);
            }
            else {
                ASSERT(Row->AttrRow.Attrs == &Row->AttrRow.AttrPair);
            }
            Row->AttrRow.Attrs = NewAttrs;
            Row->AttrRow.Length = NewAttrsLength;
            Row->CharRow.OldLeft = INVALID_OLD_LENGTH;
            Row->CharRow.OldRight = INVALID_OLD_LENGTH;
        }
        if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
            RowIndex = 0;
        }
    }
    ResetTextFlags(ScreenInfo,TargetPoint.Y,(SHORT)(TargetPoint.Y+YSize-1));

    if (XSize > 80) {
        ConsoleHeapFree(AttrBuf);
    }
}

VOID
WWSB_WriteRegionToScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    )
{
    COORD Window;
    int i,j;
    PATTR_PAIR Attr;
    RECT TextRect;
    SHORT RowIndex;
    SHORT CountOfAttr;
    PROW Row;
    BOOL OneLine, SimpleWrite;  // one line && one attribute per line
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;
    PWCHAR TransBufferCharacter = NULL ;
#ifdef WWSB_FE
    BOOL  DoubleColorDbcs;
    SHORT CountOfAttrOriginal;
    SHORT RegionRight;
    BOOL  LocalEUDCFlag;
    SMALL_RECT CaTextRect;
    PCONVERSIONAREA_INFORMATION ConvAreaInfo = ScreenInfo->ConvScreenInfo;
#endif

    DBGOUTPUT(("WriteRegionToScreen\n"));

#ifdef WWSB_FE
    if (ConvAreaInfo) {
        CaTextRect.Left = Region->Left - ScreenInfo->Console->CurrentScreenBuffer->Window.Left - ConvAreaInfo->CaInfo.coordConView.X;
        CaTextRect.Right = CaTextRect.Left + (Region->Right - Region->Left);
        CaTextRect.Top   = Region->Top - ScreenInfo->Console->CurrentScreenBuffer->Window.Top - ConvAreaInfo->CaInfo.coordConView.Y;
        CaTextRect.Bottom = CaTextRect.Top + (Region->Bottom - Region->Top);
    }

    if (Region->Left && (ScreenInfo->BisectFlag & BISECT_LEFT)) {
        Region->Left--;
    }
    if (Region->Right+1 < ScreenInfo->ScreenBufferSize.X && (ScreenInfo->BisectFlag & BISECT_RIGHT)) {
        Region->Right++;
    }
    ScreenInfo->BisectFlag &= ~(BISECT_LEFT | BISECT_RIGHT);
    Console->ConsoleIme.ScrollWaitCountDown = Console->ConsoleIme.ScrollWaitTimeout;
#endif

    if (Console->FullScreenFlags == 0) {

        //
        // if we have a selection, turn it off.
        //

        InvertSelection(Console, TRUE);

        ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
        if (WWSB_PolyTextOutCandidate(ScreenInfo,Region)) {
            WWSB_ConsolePolyTextOut(ScreenInfo,Region);
        }
        else {

#ifdef WWSB_FE
            if (ConvAreaInfo) {
                Window.Y = Region->Top - Console->CurrentScreenBuffer->Window.Top;
                Window.X = Region->Left - Console->CurrentScreenBuffer->Window.Left;
            }
            else {
#endif
                Window.Y = Region->Top - ScreenInfo->Window.Top;
                Window.X = Region->Left - ScreenInfo->Window.Left;
#ifdef WWSB_FE
            }
#endif

#ifdef WWSB_FE
            RowIndex = (ConvAreaInfo ? CaTextRect.Top :
                                       (ScreenInfo->BufferInfo.TextInfo.FirstRow+Region->Top) % ScreenInfo->ScreenBufferSize.Y
                       );
            RegionRight = Region->Right;
#else
            RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+Region->Top) % ScreenInfo->ScreenBufferSize.Y;
#endif
            OneLine = (Region->Top==Region->Bottom);

            TransBufferCharacter = (PWCHAR)ConsoleHeapAlloc(
                                                     MAKE_TAG( TMP_DBCS_TAG ),
                                                     (ScreenInfo->ScreenBufferSize.X*sizeof(WCHAR))+sizeof(WCHAR));
            if (TransBufferCharacter == NULL)
            {
                KdPrint(("CONSRV: WriteRegionToScreen cannot allocate memory\n"));
                return ;
            }

            for (i=Region->Top;i<=Region->Bottom;i++,Window.Y++) {
#ifdef WWSB_FE
                DoubleColorDbcs = FALSE;
                Region->Right = RegionRight;
#endif

                //
                // copy the chars and attrs from their respective arrays
                //

                Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];

                if (Row->AttrRow.Length == 1) {
                    Attr = Row->AttrRow.Attrs;
                    CountOfAttr = ScreenInfo->ScreenBufferSize.X;
                    SimpleWrite = TRUE;
                } else {
                    SimpleWrite = FALSE;
                    FindAttrIndex(Row->AttrRow.Attrs,
#ifdef WWSB_FE
                                  (SHORT)(ConvAreaInfo ? CaTextRect.Left : Region->Left),
#else
                                  Region->Left,
#endif
                                  &Attr,
                                  &CountOfAttr
                                 );
                }
                if (Console->LastAttributes != Attr->Attr) {
                    TEXTCOLOR_CALL;
#ifdef WWSB_FE
                    if (Attr->Attr & COMMON_LVB_REVERSE_VIDEO)
                    {
                        SetBkColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr->Attr)));
                        SetTextColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr->Attr >> 4)));
                    }
                    else{
#endif
                        SetTextColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr->Attr)));
                        SetBkColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr->Attr >> 4)));
#ifdef WWSB_FE
                    }
#endif
                    Console->LastAttributes = Attr->Attr;
                }
                TextRect.top = Window.Y*SCR_FONTSIZE(ScreenInfo).Y;
                TextRect.bottom = TextRect.top + SCR_FONTSIZE(ScreenInfo).Y;
                for (j=Region->Left;j<=Region->Right;) {
                    SHORT NumberOfChars;
                    int TextLeft;
                    SHORT LeftChar,RightChar;

                    if (CountOfAttr > (SHORT)(Region->Right - j + 1)) {
                        CountOfAttr = (SHORT)(Region->Right - j + 1);
                    }

#ifdef WWSB_FE
                    CountOfAttrOriginal = CountOfAttr;


                    LocalEUDCFlag = FALSE;
                    if((ScreenInfo->Console->Flags & CONSOLE_VDM_REGISTERED &&
                        ((PEUDC_INFORMATION)(ScreenInfo->Console->EudcInformation))->LocalVDMEudcMode)){
                        LocalEUDCFlag = CheckEudcRangeInString(
                                            Console,
                                            &Row->CharRow.Chars[ConvAreaInfo ?
                                                                CaTextRect.Left + (j-Region->Left) : j],
                                            CountOfAttr,
                                            &CountOfAttr);
                    }
                    if (!(ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                        !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN) &&
                        ((PEUDC_INFORMATION)(ScreenInfo->Console->EudcInformation))->LocalKeisenEudcMode
                       ) {
                        SHORT k;
                        PWCHAR Char2;
                        Char2 = &Row->CharRow.Chars[ConvAreaInfo ? CaTextRect.Left + (j-Region->Left) : j];
                        for ( k = 0 ; k < CountOfAttr ; k++,Char2++){
                            if (*Char2 < UNICODE_SPACE){
                                CountOfAttr = k ;
                                LocalEUDCFlag = TRUE;
                                break;
                            }
                        }
                    }
#endif

                    //
                    // make the bounding rect smaller, if we can.  the TEXT_VALID_HINT
                    // flag gets set each time we write to the screen buffer.  it gets
                    // turned off any time we get asked to redraw the screen
                    // and we don't know exactly what needs to be redrawn
                    // (i.e. paint messages).
                    //
                    // we have the left and right bounds of the text on the
                    // line.  the opaqueing rectangle and the number of
                    // chars get set according to those values.
                    //
                    // if there's more than one attr per line (!SimpleWrite)
                    // we bail on the opaqueing rect.
                    //

                    if (ScreenInfo->BufferInfo.TextInfo.Flags & TEXT_VALID_HINT && SimpleWrite) {
                        if (Row->CharRow.OldLeft != INVALID_OLD_LENGTH) {
                            TextRect.left = (max(min(Row->CharRow.Left,Row->CharRow.OldLeft),j)-ScreenInfo->Window.Left) *
                                            SCR_FONTSIZE(ScreenInfo).X;
                        } else {
                            TextRect.left = Window.X*SCR_FONTSIZE(ScreenInfo).X;
                        }

                        if (Row->CharRow.OldRight != INVALID_OLD_LENGTH) {
                            TextRect.right = (min(max(Row->CharRow.Right,Row->CharRow.OldRight),j+CountOfAttr)-ScreenInfo->Window.Left) *
                                             SCR_FONTSIZE(ScreenInfo).X;
                        } else {
                            TextRect.right = TextRect.left + CountOfAttr*SCR_FONTSIZE(ScreenInfo).X;
                        }
                        LeftChar = max(Row->CharRow.Left,j);
                        RightChar = min(Row->CharRow.Right,j+CountOfAttr);
                        NumberOfChars = RightChar - LeftChar;
                        TextLeft = (LeftChar-ScreenInfo->Window.Left)*SCR_FONTSIZE(ScreenInfo).X;
                    } else {
#ifdef WWSB_FE
                        LeftChar = ConvAreaInfo ? CaTextRect.Left + (j-Region->Left) : j;
#else
                        LeftChar = (SHORT)j;
#endif
                        TextRect.left = Window.X*SCR_FONTSIZE(ScreenInfo).X;
                        TextRect.right = TextRect.left + CountOfAttr*SCR_FONTSIZE(ScreenInfo).X;
#ifdef WWSB_FE
                        if (ConvAreaInfo)
                            NumberOfChars = (Row->CharRow.Right > (SHORT)((CaTextRect.Left+(j-Region->Left)) + CountOfAttr)) ?
                                (CountOfAttr) : (SHORT)(Row->CharRow.Right-(CaTextRect.Left+(j-Region->Left)));
                        else
#endif
                            NumberOfChars = (Row->CharRow.Right > (SHORT)(j + CountOfAttr)) ? (CountOfAttr) : (SHORT)(Row->CharRow.Right-j);
                        TextLeft = TextRect.left;
                    }

                    if (NumberOfChars < 0)
                    {
                        NumberOfChars = 0;
#ifdef WWSB_FE
                        TextRect.left = Window.X*SCR_FONTSIZE(ScreenInfo).X;
                        TextRect.right = TextRect.left + CountOfAttr*SCR_FONTSIZE(ScreenInfo).X;
#endif
                    }
                    TEXTOUT_CALL;
#ifdef WWSB_FE
                    /*
                     * Text out everything (i.e. SBCS/DBCS, Common LVB attribute, Local EUDC)
                     */
                    TextOutEverything(Console,
                                      ScreenInfo,
                                      (SHORT)j,
                                      &Region->Right,
                                      &CountOfAttr,
                                      CountOfAttrOriginal,
                                      &DoubleColorDbcs,
                                      LocalEUDCFlag,
                                      Row,
                                      Attr,
                                      LeftChar,
                                      RightChar,
                                      TextLeft,
                                      TextRect,
                                      NumberOfChars);
#else
                    NumberOfChars =
                        (SHORT)RemoveDbcsMarkAll(ScreenInfo,
                                                 Row,
                                                 &LeftChar,
                                                 &TextRect,
                                                 &TextLeft,
                                                 TransBufferCharacter,
                                                 NumberOfChars);
                    ExtTextOutW(Console->hDC,
                               TextLeft,
                               TextRect.top,
                               ETO_OPAQUE,
                               &TextRect,
                               TransBufferCharacter,
                               NumberOfChars,
                               NULL
                              );
#endif
                    if (OneLine && SimpleWrite) {
                        break;
                    }
                    j+=CountOfAttr;
                    if (j <= Region->Right) {
                        Window.X += CountOfAttr;
#ifdef WWSB_FE
                        if (CountOfAttr < CountOfAttrOriginal){
                            CountOfAttr = CountOfAttrOriginal - CountOfAttr;
                        }
                        else {
#endif
                            Attr++;
                            CountOfAttr = Attr->Length;
#ifdef WWSB_FE
                        }
#endif
#ifdef WWSB_FE
                        if (Attr->Attr & COMMON_LVB_REVERSE_VIDEO)
                        {
                            SetBkColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr->Attr)));
                            SetTextColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr->Attr >> 4)));
                        }
                        else{
#endif
                            SetTextColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr->Attr)));
                            SetBkColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr->Attr >> 4)));
#ifdef WWSB_FE
                        }
#endif
                        Console->LastAttributes = Attr->Attr;
                    }
                }
                Window.X = Region->Left - ScreenInfo->Window.Left;
                if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
                    RowIndex = 0;
                }
            }
            GdiFlush();
            ConsoleHeapFree(TransBufferCharacter);
        }

        //
        // if we have a selection, turn it on.
        //

        InvertSelection(Console, FALSE);
    }
#ifdef i386
    else if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
#ifdef WWSB_FE
        if (! ScreenInfo->ConvScreenInfo) {
            if (ScreenInfo->Console->CurrentScreenBuffer == ScreenInfo) {
                WWSB_WriteRegionToScreenHW(ScreenInfo,Region);
            }
        }
        else if (ScreenInfo->Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER)
#endif
            WWSB_WriteRegionToScreenHW(ScreenInfo,Region);
    }
#endif

#ifdef WWSB_FE
    {
        SMALL_RECT TmpRegion;

        if (ScreenInfo->BisectFlag & BISECT_TOP) {
            ScreenInfo->BisectFlag &= ~BISECT_TOP;
            if (Region->Top) {
                TmpRegion.Top = Region->Top-1;
                TmpRegion.Bottom = Region->Top-1;
                TmpRegion.Left = ScreenInfo->ScreenBufferSize.X-1;
                TmpRegion.Right = ScreenInfo->ScreenBufferSize.X-1;
                WWSB_WriteRegionToScreen(ScreenInfo,&TmpRegion);
            }
        }
        if (ScreenInfo->BisectFlag & BISECT_BOTTOM) {
            ScreenInfo->BisectFlag &= ~BISECT_BOTTOM;
            if (Region->Bottom+1 < ScreenInfo->ScreenBufferSize.Y) {
                TmpRegion.Top = Region->Bottom+1;
                TmpRegion.Bottom = Region->Bottom+1;
                TmpRegion.Left = 0;
                TmpRegion.Right = 0;
                WWSB_WriteRegionToScreen(ScreenInfo,&TmpRegion);
            }
        }
    }
#endif
}

VOID
WWSB_WriteToScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    )
/*++

Routine Description:

    This routine writes a screen buffer region to the screen.

Arguments:

    ScreenInfo - Pointer to screen buffer information.

    Region - Region to write in screen buffer coordinates.  Region is
    inclusive

Return Value:

    none.

--*/

{
    SMALL_RECT ClippedRegion;

    DBGOUTPUT(("WriteToScreen\n"));
    //
    // update to screen, if we're not iconic.  we're marked as
    // iconic if we're fullscreen, so check for fullscreen.
    //

    if (!ACTIVE_SCREEN_BUFFER(ScreenInfo) ||
        (ScreenInfo->Console->Flags & (CONSOLE_IS_ICONIC | CONSOLE_NO_WINDOW) &&
         ScreenInfo->Console->FullScreenFlags == 0)) {
        return;
    }

    // clip region

    ClippedRegion.Left = max(Region->Left, ScreenInfo->Window.Left);
    ClippedRegion.Top = max(Region->Top, ScreenInfo->Window.Top);
    ClippedRegion.Right = min(Region->Right, ScreenInfo->Window.Right);
    ClippedRegion.Bottom = min(Region->Bottom, ScreenInfo->Window.Bottom);
    if (ClippedRegion.Right < ClippedRegion.Left ||
        ClippedRegion.Bottom < ClippedRegion.Top) {
        return;
    }

    if (ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER) {
        if (ScreenInfo->Console->FullScreenFlags == 0) {
            WriteRegionToScreenBitMap(ScreenInfo, &ClippedRegion);
        }
    } else {
        ConsoleHideCursor(ScreenInfo);
        WWSB_WriteRegionToScreen(ScreenInfo, &ClippedRegion);
#ifdef WWSB_FE
        if (!(ScreenInfo->Console->ConsoleIme.ScrollFlag & HIDE_FOR_SCROLL))
        {
            PCONVERSIONAREA_INFORMATION ConvAreaInfo;

            if (! ScreenInfo->Console->CurrentScreenBuffer->ConvScreenInfo) {
                WriteConvRegionToScreen(ScreenInfo,
                                        ScreenInfo->Console->ConsoleIme.ConvAreaRoot,
                                        Region);
            }
            else if (ConvAreaInfo = ScreenInfo->Console->ConsoleIme.ConvAreaRoot) {
                do {
                    if (ConvAreaInfo->ScreenBuffer == ScreenInfo)
                        break;
                } while (ConvAreaInfo = ConvAreaInfo->ConvAreaNext);
                if (ConvAreaInfo) {
                    WriteConvRegionToScreen(ScreenInfo,
                                            ConvAreaInfo->ConvAreaNext,
                                            Region);
                }
            }
        }
#endif
        ConsoleShowCursor(ScreenInfo);
    }
}

NTSTATUS
WWSB_WriteOutputString(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PVOID Buffer,
    IN COORD WriteCoord,
    IN ULONG StringType,
    IN OUT PULONG NumRecords, // this value is valid even for error cases
    OUT PULONG NumColumns OPTIONAL
    )

/*++

Routine Description:

    This routine writes a string of characters or attributes to the
    screen buffer.

Arguments:

    ScreenInfo - Pointer to screen buffer information.

    Buffer - Buffer to write from.

    WriteCoord - Screen buffer coordinate to begin writing to.

    StringType
      One of the following:
        CONSOLE_ASCII          - write a string of ascii characters.
        CONSOLE_REAL_UNICODE   - write a string of real unicode characters.
        CONSOLE_FALSE_UNICODE  - write a string of false unicode characters.
        CONSOLE_ATTRIBUTE      - write a string of attributes.

    NumRecords - On input, the number of elements to write.  On output,
    the number of elements written.

    NumColumns - receives the number of columns output, which could be more
                 than NumRecords (FE fullwidth chars)
Return Value:


--*/

{
    ULONG NumWritten;
    SHORT X,Y,LeftX;
    SMALL_RECT WriteRegion;
    PROW Row;
    PWCHAR Char;
    SHORT RowIndex;
    SHORT j;
    PWCHAR TransBuffer;
#ifdef WWSB_NOFE
    WCHAR SingleChar;
#endif
    UINT Codepage;
#ifdef WWSB_FE
    PBYTE AttrP;
    PBYTE TransBufferA;
    PBYTE BufferA;
    ULONG NumRecordsSavedForUnicode;
    BOOL  fLocalHeap = FALSE;
#endif

    DBGOUTPUT(("WriteOutputString\n"));
#ifdef WWSB_FE
    ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
#endif

    if (*NumRecords == 0)
        return STATUS_SUCCESS;

    NumWritten = 0;
    X=WriteCoord.X;
    Y=WriteCoord.Y;
    if (X>=ScreenInfo->ScreenBufferSize.X ||
        X<0 ||
        Y>=ScreenInfo->ScreenBufferSize.Y ||
        Y<0) {
        *NumRecords = 0;
        return STATUS_SUCCESS;
    }

    ScreenInfo->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;
    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+WriteCoord.Y) % ScreenInfo->ScreenBufferSize.Y;

    if (StringType == CONSOLE_ASCII) {
#ifdef WWSB_FE
        PCHAR TmpBuf;
        PWCHAR TmpTrans;
        ULONG i;
        PCHAR TmpTransA;
#endif

        if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
            if (ScreenInfo->Console->OutputCP != WINDOWSCP)
                Codepage = USACP;
            else
                Codepage = WINDOWSCP;
        } else {
            Codepage = ScreenInfo->Console->OutputCP;
        }

#ifdef WWSB_FE
        if (*NumRecords > (ULONG)(ScreenInfo->ScreenBufferSize.X * ScreenInfo->ScreenBufferSize.Y)) {

            TransBuffer = ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),*NumRecords * 2 * sizeof(WCHAR));
            if (TransBuffer == NULL) {
                return STATUS_NO_MEMORY;
            }
            TransBufferA = ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),*NumRecords * 2 * sizeof(CHAR));
            if (TransBufferA == NULL) {
                ConsoleHeapFree(TransBuffer);
                return STATUS_NO_MEMORY;
            }

            fLocalHeap = TRUE;
        }
        else {
            TransBuffer  = ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter;
            TransBufferA = ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferAttribute;
        }

        TmpBuf = Buffer;
        TmpTrans = TransBuffer;
        TmpTransA = TransBufferA;      // MSKK Apr.02.1993 V-HirotS For KAttr
        for (i=0; i < *NumRecords;) {
            if (IsDBCSLeadByteConsole(*TmpBuf,&ScreenInfo->Console->OutputCPInfo)) {
                if (i+1 >= *NumRecords) {
                    *TmpTrans = UNICODE_SPACE;
                    *TmpTransA = 0;
                    i++;
                }
                else {
                    ConvertOutputToUnicode(Codepage,
                                           TmpBuf,
                                           2,
                                           TmpTrans,
                                           2);
                    *(TmpTrans+1) = *TmpTrans;
                    TmpTrans += 2;
                    TmpBuf += 2;
                    *TmpTransA++ = ATTR_LEADING_BYTE;
                    *TmpTransA++ = ATTR_TRAILING_BYTE;
                    i += 2;
                }
            }
            else {
                ConvertOutputToUnicode(Codepage,
                                       TmpBuf,
                                       1,
                                       TmpTrans,
                                       1);
                TmpTrans++;
                TmpBuf++;
                *TmpTransA++ = 0;               // MSKK APr.02.1993 V-HirotS For KAttr
                i++;
            }
        }
        BufferA = TransBufferA;
        Buffer = TransBuffer;
#else
        if (*NumRecords == 1) {
            TransBuffer = NULL;
            SingleChar = SB_CharToWcharGlyph(Codepage, *((char *)Buffer));
            Buffer = &SingleChar;
        } else {
            TransBuffer = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),*NumRecords * sizeof(WCHAR));
            if (TransBuffer == NULL) {
                return STATUS_NO_MEMORY;
            }
            ConvertOutputToUnicode(Codepage, Buffer, *NumRecords,
                    TransBuffer, *NumRecords);
            Buffer = TransBuffer;
        }
#endif
    } else if (StringType == CONSOLE_REAL_UNICODE &&
            (ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
        RealUnicodeToFalseUnicode(Buffer,
                                *NumRecords,
                                ScreenInfo->Console->OutputCP
                                );
    }

#ifdef WWSB_FE
    if ((StringType == CONSOLE_REAL_UNICODE) || (StringType == CONSOLE_FALSE_UNICODE)) {
        PWCHAR TmpBuf;
        PWCHAR TmpTrans;
        PCHAR TmpTransA;
        ULONG i,j;
        WCHAR c;

        /* Avoid overflow into TransBufferCharacter , TransBufferAttribute
         * because, if hit by IsConsoleFullWidth()
         * then one unicde character needs two spaces on TransBuffer.
         */
        if ((*NumRecords*2) > (ULONG)(ScreenInfo->ScreenBufferSize.X * ScreenInfo->ScreenBufferSize.Y)) {

            TransBuffer = ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),*NumRecords * 2 * sizeof(WCHAR));
            if (TransBuffer == NULL) {
                return STATUS_NO_MEMORY;
            }
            TransBufferA = ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),*NumRecords * 2 * sizeof(CHAR));
            if (TransBufferA == NULL) {
                ConsoleHeapFree(TransBuffer);
                return STATUS_NO_MEMORY;
            }

            fLocalHeap = TRUE;
        }
        else {
            TransBuffer  = ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter;
            TransBufferA = ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferAttribute;
        }

        TmpBuf = Buffer;
        TmpTrans = TransBuffer;
        TmpTransA = TransBufferA;
        for (i=0,j=0; i < *NumRecords; i++,j++) {
            *TmpTrans++ = c = *TmpBuf++;
            *TmpTransA = 0;
            if (IsConsoleFullWidth(ScreenInfo->Console->hDC,
                                   ScreenInfo->Console->OutputCP,c)) {
                *TmpTransA++ = ATTR_LEADING_BYTE;
                *TmpTrans++ = c;
                *TmpTransA = ATTR_TRAILING_BYTE;
                j++;
            }
            TmpTransA++;
        }
        NumRecordsSavedForUnicode = *NumRecords;
        *NumRecords = j;
        Buffer = TransBuffer;
        BufferA = TransBufferA;
    }
#endif

    if ((StringType == CONSOLE_REAL_UNICODE) ||
            (StringType == CONSOLE_FALSE_UNICODE) ||
            (StringType == CONSOLE_ASCII)) {
        while (TRUE) {

            LeftX = X;

            //
            // copy the chars into their arrays
            //

            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
            Char = &Row->CharRow.Chars[X];
#ifdef WWSB_FE
            AttrP = &Row->CharRow.KAttrs[X];
#endif
            if ((ULONG)(ScreenInfo->ScreenBufferSize.X - X) >= (*NumRecords - NumWritten)) {
                /*
                 * The text will not hit the right hand edge, copy it all
                 */
#ifdef WWSB_FE
                COORD TPoint;

                TPoint.X = X;
                TPoint.Y = Y;
                BisectWrite((SHORT)(*NumRecords-NumWritten),TPoint,ScreenInfo);
                if (TPoint.Y == ScreenInfo->ScreenBufferSize.Y-1 &&
                    (SHORT)(TPoint.X+*NumRecords-NumWritten) >= ScreenInfo->ScreenBufferSize.X &&
                    *((PCHAR)BufferA+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) & ATTR_LEADING_BYTE
                   ) {
                    *((PWCHAR)Buffer+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) = UNICODE_SPACE;
                    *((PCHAR)BufferA+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) = 0;
                    if ((SHORT)(*NumRecords-NumWritten) > (SHORT)(ScreenInfo->ScreenBufferSize.X-TPoint.X-1)) {
                        *((PWCHAR)Buffer+ScreenInfo->ScreenBufferSize.X-TPoint.X) = UNICODE_SPACE;
                        *((PCHAR)BufferA+ScreenInfo->ScreenBufferSize.X-TPoint.X) = 0;
                    }
                }
                RtlCopyMemory(AttrP,BufferA,(*NumRecords - NumWritten) * sizeof(CHAR));
#endif
                RtlCopyMemory(Char,Buffer,(*NumRecords - NumWritten) * sizeof(WCHAR));
                X=(SHORT)(X+*NumRecords - NumWritten-1);
                NumWritten = *NumRecords;
            }
            else {
                /*
                 * The text will hit the right hand edge, copy only that much
                 */
#ifdef WWSB_FE
                COORD TPoint;

                TPoint.X = X;
                TPoint.Y = Y;
                BisectWrite((SHORT)(ScreenInfo->ScreenBufferSize.X-X),TPoint,ScreenInfo);
                if (TPoint.Y == ScreenInfo->ScreenBufferSize.Y-1 &&
                    TPoint.X+ScreenInfo->ScreenBufferSize.X-X >= ScreenInfo->ScreenBufferSize.X &&
                    *((PCHAR)BufferA+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) & ATTR_LEADING_BYTE
                   ) {
                    *((PWCHAR)Buffer+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) = UNICODE_SPACE;
                    *((PCHAR)BufferA+ScreenInfo->ScreenBufferSize.X-TPoint.X-1) = 0;
                    if (ScreenInfo->ScreenBufferSize.X-X > ScreenInfo->ScreenBufferSize.X-TPoint.X-1) {
                        *((PWCHAR)Buffer+ScreenInfo->ScreenBufferSize.X-TPoint.X) = UNICODE_SPACE;
                        *((PCHAR)BufferA+ScreenInfo->ScreenBufferSize.X-TPoint.X) = 0;
                    }
                }
                RtlCopyMemory(AttrP,BufferA,(ScreenInfo->ScreenBufferSize.X - X) * sizeof(CHAR));
                BufferA = (PVOID)((PBYTE)BufferA + ((ScreenInfo->ScreenBufferSize.X - X) * sizeof(CHAR)));
#endif
                RtlCopyMemory(Char,Buffer,(ScreenInfo->ScreenBufferSize.X - X) * sizeof(WCHAR));
                Buffer = (PVOID)((PBYTE)Buffer + ((ScreenInfo->ScreenBufferSize.X - X) * sizeof(WCHAR)));
                NumWritten += ScreenInfo->ScreenBufferSize.X - X;
                X = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
            }

            // recalculate first and last non-space char

            Row->CharRow.OldLeft = Row->CharRow.Left;
            if (LeftX < Row->CharRow.Left) {
                PWCHAR LastChar = &Row->CharRow.Chars[ScreenInfo->ScreenBufferSize.X];

                for (Char=&Row->CharRow.Chars[LeftX];Char < LastChar && *Char==(WCHAR)' ';Char++)
                    ;
                Row->CharRow.Left = (SHORT)(Char-Row->CharRow.Chars);
            }

            Row->CharRow.OldRight = Row->CharRow.Right;
            if ((X+1) >= Row->CharRow.Right) {
                WORD LastNonSpace;
                PWCHAR FirstChar = Row->CharRow.Chars;

                LastNonSpace = X;
                for (Char=&Row->CharRow.Chars[X];*Char==(WCHAR)' ' && Char >= FirstChar;Char--)
                    LastNonSpace--;
                Row->CharRow.Right = (SHORT)(LastNonSpace+1);
            }
            if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
                RowIndex = 0;
            }
            if (NumWritten < *NumRecords) {
                /*
                 * The string hit the right hand edge, so wrap around to the
                 * next line by going back round the while loop, unless we
                 * are at the end of the buffer - in which case we simply
                 * abandon the remainder of the output string!
                 */
                X = 0;
                Y++;
                if (Y >= ScreenInfo->ScreenBufferSize.Y) {
                    break; // abandon output, string is truncated
                }
            } else {
                break;
            }
        }
    } else if (StringType == CONSOLE_ATTRIBUTE) {
        PWORD SourcePtr=Buffer;
        PATTR_PAIR AttrBuf;
        ATTR_PAIR Attrs[80];
        PATTR_PAIR Attr;
        SHORT AttrLength;

        AttrBuf = Attrs;
        if (ScreenInfo->ScreenBufferSize.X > 80) {
            AttrBuf = (PATTR_PAIR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),ScreenInfo->ScreenBufferSize.X * sizeof(ATTR_PAIR));
            if (AttrBuf == NULL)
                return STATUS_NO_MEMORY;
        }
#ifdef WWSB_FE
        {
            COORD TPoint;
            TPoint.X = X;
            TPoint.Y = Y;
            if ((ULONG)(ScreenInfo->ScreenBufferSize.X - X) >= (*NumRecords - NumWritten)) {
                BisectWriteAttr((SHORT)(*NumRecords-NumWritten),TPoint,ScreenInfo);
            }
            else{
                BisectWriteAttr((SHORT)(ScreenInfo->ScreenBufferSize.X-X),TPoint,ScreenInfo);
            }
        }
#endif
        while (TRUE) {

            //
            // copy the attrs into the screen buffer arrays
            //

            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
            Attr = AttrBuf;
            Attr->Length = 0;
#ifdef WWSB_FE
            Attr->Attr = *SourcePtr & ~COMMON_LVB_SBCSDBCS;
#else
            Attr->Attr = *SourcePtr;
#endif
            AttrLength = 1;
            for (j=X;j<ScreenInfo->ScreenBufferSize.X;j++,SourcePtr++) {
#ifdef WWSB_FE
                if (Attr->Attr == (*SourcePtr & ~COMMON_LVB_SBCSDBCS))
#else
                if (Attr->Attr == *SourcePtr)
#endif
                {
                    Attr->Length += 1;
                }
                else {
                    Attr++;
                    Attr->Length = 1;
#ifdef WWSB_FE
                    Attr->Attr = *SourcePtr & ~COMMON_LVB_SBCSDBCS;
#else
                    Attr->Attr = *SourcePtr;
#endif
                    AttrLength += 1;
                }
                NumWritten++;
                X++;
                if (NumWritten == *NumRecords) {
                    break;
                }
            }
            X--;

            // recalculate last non-space char

            //
            // see if attr string is different.  if so, allocate a new
            // attr buffer and merge the two strings.
            //

            if (AttrLength != Row->AttrRow.Length ||
                memcmp(Row->AttrRow.Attrs,AttrBuf,AttrLength*sizeof(*Attr))) {
                PATTR_PAIR NewAttrs;
                WORD NewAttrsLength;

                if (!NT_SUCCESS(MergeAttrStrings(Row->AttrRow.Attrs,
                                 Row->AttrRow.Length,
                                 AttrBuf,
                                 AttrLength,
                                 &NewAttrs,
                                 &NewAttrsLength,
                                 (SHORT)((Y == WriteCoord.Y) ? WriteCoord.X : 0),
                                 X,
                                 Row,
                                 ScreenInfo
                                ))) {
                    if (ScreenInfo->ScreenBufferSize.X > 80) {
                        ConsoleHeapFree(AttrBuf);
                    }
                    ResetTextFlags(ScreenInfo,WriteCoord.Y,Y);
                    return STATUS_NO_MEMORY;
                }
                if (Row->AttrRow.Length > 1) {
                    ConsoleHeapFree(Row->AttrRow.Attrs);
                }
                else {
                    ASSERT(Row->AttrRow.Attrs == &Row->AttrRow.AttrPair);
                }
                Row->AttrRow.Attrs = NewAttrs;
                Row->AttrRow.Length = NewAttrsLength;
                Row->CharRow.OldLeft = INVALID_OLD_LENGTH;
                Row->CharRow.OldRight = INVALID_OLD_LENGTH;
            }

            if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
                RowIndex = 0;
            }
            if (NumWritten < *NumRecords) {
                X = 0;
                Y++;
                if (Y>=ScreenInfo->ScreenBufferSize.Y) {
                    break;
                }
            } else {
                break;
            }
        }
        ResetTextFlags(ScreenInfo,WriteCoord.Y,Y);
        if (ScreenInfo->ScreenBufferSize.X > 80) {
            ConsoleHeapFree(AttrBuf);
        }
    } else {
        *NumRecords = 0;
        return STATUS_INVALID_PARAMETER;
    }
    if ((StringType == CONSOLE_ASCII) && (TransBuffer != NULL)) {
#ifdef WWSB_FE
        if (fLocalHeap) {
            ConsoleHeapFree(TransBuffer);
            ConsoleHeapFree(TransBufferA);
        }
#else
        ConsoleHeapFree(TransBuffer);
#endif
    }
#ifdef WWSB_FE
    else if ((StringType == CONSOLE_FALSE_UNICODE) || (StringType == CONSOLE_REAL_UNICODE)) {
        if (fLocalHeap) {
            ConsoleHeapFree(TransBuffer);
            ConsoleHeapFree(TransBufferA);
        }
        NumWritten = NumRecordsSavedForUnicode - (*NumRecords - NumWritten);
    }
#endif

    //
    // determine write region.  if we're still on the same line we started
    // on, left X is the X we started with and right X is the one we're on
    // now.  otherwise, left X is 0 and right X is the rightmost column of
    // the screen buffer.
    //
    // then update the screen.
    //

    WriteRegion.Top = WriteCoord.Y;
    WriteRegion.Bottom = Y;
    if (Y != WriteCoord.Y) {
        WriteRegion.Left = 0;
        WriteRegion.Right = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
    }
    else {
        WriteRegion.Left = WriteCoord.X;
        WriteRegion.Right = X;
    }
    WWSB_WriteToScreen(ScreenInfo,&WriteRegion);
    if (NumColumns) {
        *NumColumns = X + (WriteCoord.Y - Y) * ScreenInfo->ScreenBufferSize.X - WriteCoord.X + 1;
    }
    *NumRecords = NumWritten;
    return STATUS_SUCCESS;
}

NTSTATUS
WWSB_FillOutput(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN WORD Element,
    IN COORD WriteCoord,
    IN ULONG ElementType,
    IN OUT PULONG Length // this value is valid even for error cases
    )

/*++

Routine Description:

    This routine fills the screen buffer with the specified character or
    attribute.

Arguments:

    ScreenInfo - Pointer to screen buffer information.

    Element - Element to write.

    WriteCoord - Screen buffer coordinate to begin writing to.

    ElementType

        CONSOLE_ASCII         - element is an ascii character.
        CONSOLE_REAL_UNICODE  - element is a real unicode character. These will
                                get converted to False Unicode as required.
        CONSOLE_FALSE_UNICODE - element is a False Unicode character.
        CONSOLE_ATTRIBUTE     - element is an attribute.

    Length - On input, the number of elements to write.  On output,
    the number of elements written.

Return Value:


--*/

{
    ULONG NumWritten;
    SHORT X,Y,LeftX;
    SMALL_RECT WriteRegion;
    PROW Row;
    PWCHAR Char;
    SHORT RowIndex;
    SHORT j;
#ifdef WWSB_FE
    PCHAR AttrP;
#endif

    DBGOUTPUT(("FillOutput\n"));
    if (*Length == 0)
        return STATUS_SUCCESS;
    NumWritten = 0;
    X=WriteCoord.X;
    Y=WriteCoord.Y;
    if (X>=ScreenInfo->ScreenBufferSize.X ||
        X<0 ||
        Y>=ScreenInfo->ScreenBufferSize.Y ||
        Y<0) {
        *Length = 0;
        return STATUS_SUCCESS;
    }

#ifdef WWSB_FE
    ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
#endif

    ScreenInfo->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;
    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+WriteCoord.Y) % ScreenInfo->ScreenBufferSize.Y;

    if (ElementType == CONSOLE_ASCII) {
        UINT Codepage;
        if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                ((ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
            if (ScreenInfo->Console->OutputCP != WINDOWSCP)
                Codepage = USACP;
            else
                Codepage = WINDOWSCP;
        } else {
            Codepage = ScreenInfo->Console->OutputCP;
        }
#ifdef WWSB_FE
        if (ScreenInfo->FillOutDbcsLeadChar == 0){
            if (IsDBCSLeadByteConsole((CHAR)Element,&ScreenInfo->Console->OutputCPInfo)) {
                ScreenInfo->FillOutDbcsLeadChar = (CHAR)Element;
                *Length = 0;
                return STATUS_SUCCESS;
            }else{
                CHAR Char=(CHAR)Element;
                ConvertOutputToUnicode(Codepage,
                          &Char,
                          1,
                          &Element,
                          1);
            }
        }else{
            CHAR Char[2];
            Char[0]=ScreenInfo->FillOutDbcsLeadChar;
            Char[1]=(BYTE)Element;
            ScreenInfo->FillOutDbcsLeadChar = 0;
            ConvertOutputToUnicode(Codepage,
                      Char,
                      2,
                      &Element,
                      2);
        }
#else
        Element = SB_CharToWchar(Codepage, (CHAR)Element);
#endif
    } else if (ElementType == CONSOLE_REAL_UNICODE &&
            (ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
        RealUnicodeToFalseUnicode(&Element,
                                1,
                                ScreenInfo->Console->OutputCP
                                );
    }

    if ((ElementType == CONSOLE_ASCII) ||
            (ElementType == CONSOLE_REAL_UNICODE) ||
            (ElementType == CONSOLE_FALSE_UNICODE)) {
#ifdef WWSB_FE
        DWORD StartPosFlag ;
        StartPosFlag = 0;
#endif
        while (TRUE) {

            //
            // copy the chars into their arrays
            //

            LeftX = X;
            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
            Char = &Row->CharRow.Chars[X];
#ifdef WWSB_FE
            AttrP = &Row->CharRow.KAttrs[X];
#endif
            if ((ULONG)(ScreenInfo->ScreenBufferSize.X - X) >= (*Length - NumWritten)) {
#ifdef WWSB_FE
                {
                    COORD TPoint;

                    TPoint.X = X;
                    TPoint.Y = Y;
                    BisectWrite((SHORT)(*Length-NumWritten),TPoint,ScreenInfo);
                }
#endif
#ifdef WWSB_FE
                if (IsConsoleFullWidth(ScreenInfo->Console->hDC,
                                       ScreenInfo->Console->OutputCP,(WCHAR)Element)) {
                    for (j=0;j<(SHORT)(*Length - NumWritten);j++) {
                        *Char++ = (WCHAR)Element;
                        *AttrP &= ~ATTR_DBCSSBCS_BYTE;
                        if(StartPosFlag++ & 1)
                            *AttrP++ |= ATTR_TRAILING_BYTE;
                        else
                            *AttrP++ |= ATTR_LEADING_BYTE;
                    }
                    if(StartPosFlag & 1){
                        *(Char-1) = UNICODE_SPACE;
                        *(AttrP-1) &= ~ATTR_DBCSSBCS_BYTE;
                    }
                }
                else {
#endif
                    for (j=0;j<(SHORT)(*Length - NumWritten);j++) {
                        *Char++ = (WCHAR)Element;
#ifdef WWSB_FE
                        *AttrP++ &= ~ATTR_DBCSSBCS_BYTE;
#endif
                    }
#ifdef WWSB_FE
                }
#endif
                X=(SHORT)(X+*Length - NumWritten - 1);
                NumWritten = *Length;
            }
            else {
#ifdef WWSB_FE
                {
                    COORD TPoint;

                    TPoint.X = X;
                    TPoint.Y = Y;
                    BisectWrite((SHORT)(ScreenInfo->ScreenBufferSize.X-X),TPoint,ScreenInfo);
                }
#endif
#ifdef WWSB_FE
                if (IsConsoleFullWidth(ScreenInfo->Console->hDC,
                                       ScreenInfo->Console->OutputCP,(WCHAR)Element)) {
                    for (j=0;j<ScreenInfo->ScreenBufferSize.X - X;j++) {
                        *Char++ = (WCHAR)Element;
                        *AttrP &= ~ATTR_DBCSSBCS_BYTE;
                        if(StartPosFlag++ & 1)
                            *AttrP++ |= ATTR_TRAILING_BYTE;
                        else
                            *AttrP++ |= ATTR_LEADING_BYTE;
                    }
                }
                else {
#endif
                    for (j=0;j<ScreenInfo->ScreenBufferSize.X - X;j++) {
                        *Char++ = (WCHAR)Element;
#ifdef WWSB_FE
                        *AttrP++ &= ~ATTR_DBCSSBCS_BYTE;
#endif
                    }
#ifdef WWSB_FE
                }
#endif
                NumWritten += ScreenInfo->ScreenBufferSize.X - X;
                X = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
            }

            // recalculate first and last non-space char

            Row->CharRow.OldLeft = Row->CharRow.Left;
            if (LeftX < Row->CharRow.Left) {
                if (Element == UNICODE_SPACE) {
                    Row->CharRow.Left = X+1;
                } else {
                    Row->CharRow.Left = LeftX;
                }
            }
            Row->CharRow.OldRight = Row->CharRow.Right;
            if ((X+1) >= Row->CharRow.Right) {
                if (Element == UNICODE_SPACE) {
                    Row->CharRow.Right = LeftX;
                } else {
                    Row->CharRow.Right = X+1;
                }
            }
            if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
                RowIndex = 0;
            }
            if (NumWritten < *Length) {
                X = 0;
                Y++;
                if (Y>=ScreenInfo->ScreenBufferSize.Y) {
                    break;
                }
            } else {
                break;
            }
        }
    } else if (ElementType == CONSOLE_ATTRIBUTE) {
        ATTR_PAIR Attr;

#ifdef WWSB_FE
        COORD TPoint;
        TPoint.X = X;
        TPoint.Y = Y;

        if ((ULONG)(ScreenInfo->ScreenBufferSize.X - X) >= (*Length - NumWritten)) {
            BisectWriteAttr((SHORT)(*Length-NumWritten),TPoint,ScreenInfo);
        }
        else{
            BisectWriteAttr((SHORT)(ScreenInfo->ScreenBufferSize.X-X),TPoint,ScreenInfo);
        }
#endif

        while (TRUE) {

            //
            // copy the attrs into the screen buffer arrays
            //

            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
            if ((ULONG)(ScreenInfo->ScreenBufferSize.X - X) >= (*Length - NumWritten)) {
                X=(SHORT)(X+*Length - NumWritten - 1);
                NumWritten = *Length;
            }
            else {
                NumWritten += ScreenInfo->ScreenBufferSize.X - X;
                X = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
            }

            // recalculate last non-space char

            //
            //  merge the two attribute strings.
            //

            Attr.Length = (SHORT)((Y == WriteCoord.Y) ? (X-WriteCoord.X+1) : (X+1));
#ifdef WWSB_FE
            Attr.Attr = Element & ~COMMON_LVB_SBCSDBCS;
#else
            Attr.Attr = Element;
#endif
            if (1 != Row->AttrRow.Length ||
                memcmp(Row->AttrRow.Attrs,&Attr,sizeof(Attr))) {
                PATTR_PAIR NewAttrs;
                WORD NewAttrsLength;

                if (!NT_SUCCESS(MergeAttrStrings(Row->AttrRow.Attrs,
                                 Row->AttrRow.Length,
                                 &Attr,
                                 1,
                                 &NewAttrs,
                                 &NewAttrsLength,
                                 (SHORT)(X-Attr.Length+1),
                                 X,
                                 Row,
                                 ScreenInfo
                                ))) {
                    ResetTextFlags(ScreenInfo,WriteCoord.Y,Y);
                    return STATUS_NO_MEMORY;
                }
                if (Row->AttrRow.Length > 1) {
                    ConsoleHeapFree(Row->AttrRow.Attrs);
                }
                else {
                    ASSERT(Row->AttrRow.Attrs == &Row->AttrRow.AttrPair);
                }
                Row->AttrRow.Attrs = NewAttrs;
                Row->AttrRow.Length = NewAttrsLength;
                Row->CharRow.OldLeft = INVALID_OLD_LENGTH;
                Row->CharRow.OldRight = INVALID_OLD_LENGTH;
            }

            if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
                RowIndex = 0;
            }
            if (NumWritten < *Length) {
                X = 0;
                Y++;
                if (Y>=ScreenInfo->ScreenBufferSize.Y) {
                    break;
                }
            } else {
                break;
            }
        }
        ResetTextFlags(ScreenInfo,WriteCoord.Y,Y);
    } else {
        *Length = 0;
        return STATUS_INVALID_PARAMETER;
    }

    //
    // determine write region.  if we're still on the same line we started
    // on, left X is the X we started with and right X is the one we're on
    // now.  otherwise, left X is 0 and right X is the rightmost column of
    // the screen buffer.
    //
    // then update the screen.
    //

#ifdef WWSB_FE
    if (ScreenInfo->ConvScreenInfo) {
        WriteRegion.Top = WriteCoord.Y + ScreenInfo->Window.Left + ScreenInfo->ConvScreenInfo->CaInfo.coordConView.Y;
        WriteRegion.Bottom = Y + ScreenInfo->Window.Left + ScreenInfo->ConvScreenInfo->CaInfo.coordConView.Y;
        if (Y != WriteCoord.Y) {
            WriteRegion.Left = 0;
            WriteRegion.Right = (SHORT)(ScreenInfo->Console->CurrentScreenBuffer->ScreenBufferSize.X-1);
        }
        else {
            WriteRegion.Left = WriteCoord.X + ScreenInfo->Window.Top + ScreenInfo->ConvScreenInfo->CaInfo.coordConView.X;
            WriteRegion.Right = X + ScreenInfo->Window.Top + ScreenInfo->ConvScreenInfo->CaInfo.coordConView.X;
        }
        WriteConvRegionToScreen(ScreenInfo->Console->CurrentScreenBuffer,
                                ScreenInfo->ConvScreenInfo,
                                &WriteRegion
                               );
        ScreenInfo->BisectFlag &= ~(BISECT_LEFT | BISECT_RIGHT | BISECT_TOP | BISECT_BOTTOM);
        *Length = NumWritten;
        return STATUS_SUCCESS;
    }
#endif

    WriteRegion.Top = WriteCoord.Y;
    WriteRegion.Bottom = Y;
    if (Y != WriteCoord.Y) {
        WriteRegion.Left = 0;
        WriteRegion.Right = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
    }
    else {
        WriteRegion.Left = WriteCoord.X;
        WriteRegion.Right = X;
    }
    WWSB_WriteToScreen(ScreenInfo,&WriteRegion);
    *Length = NumWritten;
    return STATUS_SUCCESS;
}

VOID
WWSB_FillRectangle(
    IN CHAR_INFO Fill,
    IN OUT PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT TargetRect
    )

/*++

Routine Description:

    This routine fills a rectangular region in the screen
    buffer.  no clipping is done.

Arguments:

    Fill - element to copy to each element in target rect

    ScreenInfo - pointer to screen info

    TargetRect - rectangle in screen buffer to fill

Return Value:

--*/

{
    SHORT i,j;
    SHORT XSize;
    SHORT RowIndex;
    PROW Row;
    PWCHAR Char;
    ATTR_PAIR Attr;
#ifdef WWSB_FE
    PCHAR AttrP;
    BOOL Width;
#endif
    DBGOUTPUT(("FillRectangle\n"));
#ifdef WWFE_SB
    ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
#endif

    XSize = (SHORT)(TargetRect->Right - TargetRect->Left + 1);

    ScreenInfo->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;
    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+TargetRect->Top) % ScreenInfo->ScreenBufferSize.Y;
    for (i=TargetRect->Top;i<=TargetRect->Bottom;i++) {

        //
        // copy the chars and attrs into their respective arrays
        //

#ifdef WWSB_FE
        {
            COORD TPoint;

            TPoint.X = TargetRect->Left;
            TPoint.Y = i;
            BisectWrite(XSize,TPoint,ScreenInfo);
            Width = IsConsoleFullWidth(ScreenInfo->Console->hDC,
                                       ScreenInfo->Console->OutputCP,Fill.Char.UnicodeChar);
        }
#endif

        Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
        Char = &Row->CharRow.Chars[TargetRect->Left];
#ifdef WWSB_FE
        AttrP = &Row->CharRow.KAttrs[TargetRect->Left];
#endif
        for (j=0;j<XSize;j++) {
#ifdef WWSB_FE
            if (Width){
                if (j < XSize-1){
                    *Char++ = Fill.Char.UnicodeChar;
                    *Char++ = Fill.Char.UnicodeChar;
                    *AttrP++ = ATTR_LEADING_BYTE;
                    *AttrP++ = ATTR_TRAILING_BYTE;
                    j++;
                }
                else{
                    *Char++ = UNICODE_SPACE;
                    *AttrP++ = 0 ;
                }
            }
            else{
#endif
                *Char++ = Fill.Char.UnicodeChar;
#ifdef WWSB_FE
                *AttrP++ = 0 ;
            }
#endif
        }

        // recalculate first and last non-space char

        Row->CharRow.OldLeft = Row->CharRow.Left;
        if (TargetRect->Left < Row->CharRow.Left) {
            if (Fill.Char.UnicodeChar == UNICODE_SPACE) {
                Row->CharRow.Left = (SHORT)(TargetRect->Right+1);
            }
            else {
                Row->CharRow.Left = (SHORT)(TargetRect->Left);
            }
        }

        Row->CharRow.OldRight = Row->CharRow.Right;
        if (TargetRect->Right >= Row->CharRow.Right) {
            if (Fill.Char.UnicodeChar == UNICODE_SPACE) {
                Row->CharRow.Right = (SHORT)(TargetRect->Left);
            }
            else {
                Row->CharRow.Right = (SHORT)(TargetRect->Right+1);
            }
        }

        Attr.Length = XSize;
        Attr.Attr = Fill.Attributes;

        //
        //  merge the two attribute strings.
        //

        if (1 != Row->AttrRow.Length ||
            memcmp(Row->AttrRow.Attrs,&Attr,sizeof(Attr))) {
            PATTR_PAIR NewAttrs;
            WORD NewAttrsLength;

            if (!NT_SUCCESS(MergeAttrStrings(Row->AttrRow.Attrs,
                             Row->AttrRow.Length,
                             &Attr,
                             1,
                             &NewAttrs,
                             &NewAttrsLength,
                             TargetRect->Left,
                             TargetRect->Right,
                             Row,
                             ScreenInfo
                            ))) {
                ResetTextFlags(ScreenInfo,TargetRect->Top,TargetRect->Bottom);
                return;
            }
            if (Row->AttrRow.Length > 1) {
                ConsoleHeapFree(Row->AttrRow.Attrs);
            }
            else {
                ASSERT(Row->AttrRow.Attrs == &Row->AttrRow.AttrPair);
            }
            Row->AttrRow.Attrs = NewAttrs;
            Row->AttrRow.Length = NewAttrsLength;
            Row->CharRow.OldLeft = INVALID_OLD_LENGTH;
            Row->CharRow.OldRight = INVALID_OLD_LENGTH;
        }
        if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
            RowIndex = 0;
        }
    }
    ResetTextFlags(ScreenInfo,TargetRect->Top,TargetRect->Bottom);
}

BOOL
WWSB_PolyTextOutCandidate(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    )

/*

    This function returns TRUE if the input region is reasonable to
    pass to ConsolePolyTextOut.  The criteria are that there is only
    one attribute per line.

*/

{
    SHORT RowIndex;
    PROW Row;
    SHORT i;

#ifdef WWSB_FE
    if((ScreenInfo->Console->Flags & CONSOLE_VDM_REGISTERED &&
        ((PEUDC_INFORMATION)(ScreenInfo->Console->EudcInformation))->LocalVDMEudcMode)){
        return FALSE;
    }
    if (!(ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
        !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN) &&
        ((PEUDC_INFORMATION)(ScreenInfo->Console->EudcInformation))->LocalKeisenEudcMode
       ) {
        return FALSE;
    }
    ASSERT(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER);
    if (ScreenInfo->BufferInfo.TextInfo.Flags & CONSOLE_CONVERSION_AREA_REDRAW) {
        return FALSE;
    }
#endif

    if (ScreenInfo->BufferInfo.TextInfo.Flags & SINGLE_ATTRIBUTES_PER_LINE) {
        return TRUE;
    }

    //
    // make sure there is only one attr per line.
    //

    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+Region->Top) % ScreenInfo->ScreenBufferSize.Y;
    for (i=Region->Top;i<=Region->Bottom;i++) {
        Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
        if (Row->AttrRow.Length != 1) {
            return FALSE;
        }
        if (++RowIndex == ScreenInfo->ScreenBufferSize.Y) {
            RowIndex = 0;
        }
    }
    return TRUE;
}


#define MAX_POLY_LINES 80
#define VERY_BIG_NUMBER 0x0FFFFFFF

#ifdef WWSB_FE
typedef struct _KEISEN_INFORMATION {
    COORD Coord;
    WORD n;
} KEISEN_INFORMATION, *PKEISEN_INFORMATION;
#endif

VOID
WWSB_ConsolePolyTextOut(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    )

/*

    This function calls PolyTextOut.  The only restriction is that
    there can't be more than one attribute per line in the region.

*/

{
    PROW  Row,LastRow;
    SHORT i,k;
    WORD Attr;
    POLYTEXTW TextInfo[MAX_POLY_LINES];
    RECT  TextRect;
    RECTL BoundingRect;
    int   xSize = SCR_FONTSIZE(ScreenInfo).X;
    int   ySize = SCR_FONTSIZE(ScreenInfo).Y;
    ULONG Flags = ScreenInfo->BufferInfo.TextInfo.Flags;
    int   WindowLeft = ScreenInfo->Window.Left;
    int   RegionLeft = Region->Left;
    int   RegionRight = Region->Right + 1;
    int   DefaultLeft  = (RegionLeft - WindowLeft) * xSize;
    int   DefaultRight = (RegionRight - WindowLeft) * xSize;
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;
    PWCHAR TransPolyTextOut = NULL ;
#ifdef WWSB_FE
    KEISEN_INFORMATION KeisenInfo[MAX_POLY_LINES];
    SHORT j;
    WORD OldAttr;
#endif

    //
    // initialize the text rect and window position.
    //

    TextRect.top = (Region->Top - ScreenInfo->Window.Top) * ySize;
    // TextRect.bottom is invalid.
    BoundingRect.top = TextRect.top;
    BoundingRect.left = VERY_BIG_NUMBER;
    BoundingRect.right = 0;

    //
    // copy the chars and attrs from their respective arrays
    //

    Row = &ScreenInfo->BufferInfo.TextInfo.Rows
           [ScreenInfo->BufferInfo.TextInfo.FirstRow+Region->Top];
    LastRow = &ScreenInfo->BufferInfo.TextInfo.Rows[ScreenInfo->ScreenBufferSize.Y];
    if (Row >= LastRow)
        Row -= ScreenInfo->ScreenBufferSize.Y;

    Attr = Row->AttrRow.AttrPair.Attr;
    if (Console->LastAttributes != Attr) {
#ifdef WWSB_FE
        if (Attr & COMMON_LVB_REVERSE_VIDEO)
        {
            SetBkColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr)));
            SetTextColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr >> 4)));
        }
        else{
#endif
            SetTextColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr)));
            SetBkColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr >> 4)));
#ifdef WWSB_FE
        }
#endif
        Console->LastAttributes = Attr;
    }

    /*
     * Temporarly use the process heap to isolate the heap trash problem
     * TransPolyTextOut = (PWCHAR)ConsoleHeapAlloc(
     *                                    MAKE_TAG( TMP_DBCS_TAG ),
     *                                    ScreenInfo->ScreenBufferSize.X*MAX_POLY_LINES*sizeof(WCHAR));
     */
    TransPolyTextOut = (PWCHAR)LocalAlloc(LPTR, ScreenInfo->ScreenBufferSize.X*MAX_POLY_LINES*sizeof(WCHAR));
    if (TransPolyTextOut == NULL)
    {
        KdPrint(("CONSRV: ConsoleTextOut cannot allocate memory\n"));
        return ;
    }

    for (i=Region->Top;i<=Region->Bottom;) {
        PWCHAR TmpChar;
        TmpChar = TransPolyTextOut;
        for(k=0;i<=Region->Bottom&&k<MAX_POLY_LINES;i++) {
            SHORT NumberOfChars;
            SHORT LeftChar,RightChar;

            //
            // make the bounding rect smaller, if we can.  the TEXT_VALID_HINT
            // flag gets set each time we write to the screen buffer.  it gets
            // turned off any time we get asked to redraw the screen
            // and we don't know exactly what needs to be redrawn
            // (i.e. paint messages).
            //
            // we have the left and right bounds of the text on the
            // line.  the opaqueing rectangle and the number of
            // chars get set according to those values.
            //

            TextRect.left  = DefaultLeft;
            TextRect.right = DefaultRight;

            if (Flags & TEXT_VALID_HINT)
            {
            // We compute an opaquing interval.  If A is the old interval of text,
            // B is the new interval, and R is the Region, then the opaquing interval
            // must be R*(A+B), where * represents intersection and + represents union.

                if (Row->CharRow.OldLeft != INVALID_OLD_LENGTH)
                {
                // The min determines the left of (A+B).  The max intersects that with
                // the left of the region.

                    TextRect.left = (
                                      max
                                      (
                                        min
                                        (
                                          Row->CharRow.Left,
                                          Row->CharRow.OldLeft
                                        ),
                                        RegionLeft
                                      )
                                      -WindowLeft
                                    ) * xSize;
                }

                if (Row->CharRow.OldRight != INVALID_OLD_LENGTH)
                {
                // The max determines the right of (A+B).  The min intersects that with
                // the right of the region.

                    TextRect.right = (
                                       min
                                       (
                                         max
                                         (
                                           Row->CharRow.Right,
                                           Row->CharRow.OldRight
                                         ),
                                         RegionRight
                                       )
                                       -WindowLeft
                                     ) * xSize;
                }
            }

            //
            // We've got to draw any new text that appears in the region, so we just
            // intersect the new text interval with the region.
            //

            LeftChar = max(Row->CharRow.Left,RegionLeft);
            RightChar = min(Row->CharRow.Right,RegionRight);
            NumberOfChars = RightChar - LeftChar;
#ifdef WWSB_FE
            if (Row->CharRow.KAttrs[RightChar-1] & ATTR_LEADING_BYTE){
                if(TextRect.right <= ScreenInfo->Window.Right*xSize) {
                    TextRect.right += xSize;
                }
            }
#endif

            //
            // Empty rows are represented by CharRow.Right=0, CharRow.Left=MAX, so we
            // may have NumberOfChars<0 at this point if there is no text that needs
            // drawing.  (I.e. the intersection was empty.)
            //

            if (NumberOfChars < 0) {
                NumberOfChars = 0;
                LeftChar = 0;
                RightChar = 0;
            }

            //
            // We may also have TextRect.right<TextRect.left if the screen
            // is already cleared, and we really don't need to do anything at all.
            //

            if (TextRect.right > TextRect.left)
            {
                NumberOfChars = (SHORT)RemoveDbcsMarkAll(ScreenInfo,Row,&LeftChar,&TextRect,NULL,TmpChar,NumberOfChars);
                TextInfo[k].x = (LeftChar-WindowLeft) * xSize;
                TextInfo[k].y = TextRect.top;
                TextRect.bottom =  TextRect.top + ySize;
                TextInfo[k].n = NumberOfChars;
                TextInfo[k].lpstr = TmpChar;
#ifdef WWSB_FE
                if (CheckBisectStringW(ScreenInfo,
                                       Console->OutputCP,
                                       TmpChar,
                                       NumberOfChars,
                                       (TextRect.right-max(TextRect.left,TextInfo[k].x))/xSize
                                      )
                   ) {
                    TextRect.right += xSize;
                }
#endif
                TmpChar += NumberOfChars;
                TextInfo[k].rcl = TextRect;
                TextInfo[k].pdx = NULL;
                TextInfo[k].uiFlags = ETO_OPAQUE;
#ifdef WWSB_FE
                KeisenInfo[k].n = DefaultRight-DefaultLeft ;
                KeisenInfo[k].Coord.Y = (WORD)TextRect.top;
                KeisenInfo[k].Coord.X = (WORD)DefaultLeft;
#endif
                k++;

                if (BoundingRect.left > TextRect.left) {
                    BoundingRect.left = TextRect.left;
                }
                if (BoundingRect.right < TextRect.right) {
                    BoundingRect.right = TextRect.right;
                }
            }

            // Advance the high res bounds.

            TextRect.top += ySize;

            // Advance the row pointer.

            if (++Row >= LastRow)
                Row = ScreenInfo->BufferInfo.TextInfo.Rows;

            // Draw now if the attributes are about to change.

#ifdef WWSB_FE
            OldAttr = Attr ;
#endif
            if (Attr != Row->AttrRow.AttrPair.Attr) {
                Attr = Row->AttrRow.AttrPair.Attr;
                i++;
                break;
            }
        }

        if (k)
        {
            BoundingRect.bottom = TextRect.top;
            ASSERT(BoundingRect.left != VERY_BIG_NUMBER);
            ASSERT(BoundingRect.left <= BoundingRect.right);
            ASSERT(BoundingRect.top <= BoundingRect.bottom);
            GdiConsoleTextOut(Console->hDC,
                              TextInfo,
                              k,
                              &BoundingRect);
#ifdef WWSB_FE
            for ( j = 0 ; j < k ; j++){
                RECT TextRect;

                TextRect.left   = KeisenInfo[j].Coord.X;
                TextRect.top    = KeisenInfo[j].Coord.Y;
                TextRect.right  = KeisenInfo[j].n + TextRect.left;
                TextRect.bottom = KeisenInfo[j].Coord.Y + ySize;
                TextOutCommonLVB(ScreenInfo->Console, OldAttr, TextRect);
            }
#endif
        }
        if (Console->LastAttributes != Attr) {
#ifdef WWSB_FE
            if (Attr & COMMON_LVB_REVERSE_VIDEO)
            {
                SetBkColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr)));
                SetTextColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr >> 4)));
            }
            else{
#endif
                SetTextColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr)));
                SetBkColor(Console->hDC, ConvertAttrToRGB(Console, LOBYTE(Attr >> 4)));
#ifdef WWSB_FE
            }
#endif
            Console->LastAttributes = Attr;
            BoundingRect.top = TextRect.top;
            BoundingRect.left = VERY_BIG_NUMBER;
            BoundingRect.right = 0;
        }
    }
    GdiFlush();
    /*
     * ConsoleHeapFree(TransPolyTextOut);
     */
    LocalFree(TransPolyTextOut);
}
