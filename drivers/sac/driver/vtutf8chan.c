/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/vtutf8chan.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

CHAR IncomingUtf8ConversionBuffer[4];
WCHAR IncomingUnicodeValue;

SAC_STATIC_ESCAPE_STRING SacStaticEscapeStrings [] =
{
    { VT_ANSI_CURSOR_UP, 2, SacCursorUp },
    { VT_ANSI_CURSOR_DOWN, 2, SacCursorDown },
    { VT_ANSI_CURSOR_RIGHT, 2, SacCursorRight },
    { VT_ANSI_CURSOR_LEFT, 2, SacCursorLeft },
    { VT_220_BACKTAB, 3, SacBackTab },
    { VT_ANSI_ERASE_END_LINE, 2, SacEraseEndOfLine },
    { VT_ANSI_ERASE_START_LINE, 3, SacEraseStartOfLine },
    { VT_ANSI_ERASE_ENTIRE_LINE, 3, SacEraseLine },
    { VT_ANSI_ERASE_DOWN_SCREEN, 2, SacEraseEndOfScreen },
    { VT_ANSI_ERASE_UP_SCREEN, 3, SacEraseStartOfScreen },
    { VT_ANSI_ERASE_ENTIRE_SCREEN, 3, SacEraseScreen },
};

/* FUNCTIONS ******************************************************************/

FORCEINLINE
VOID
VTUTF8ChannelAssertCursor(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel->CursorRow < SAC_VTUTF8_ROW_HEIGHT);
    ASSERT(Channel->CursorCol < SAC_VTUTF8_COL_WIDTH);
}

BOOLEAN
NTAPI
VTUTF8ChannelScanForNumber(IN PWCHAR String,
                           OUT PULONG Number)
{
    /* If the first character is invalid, fail early */
    if ((*String < L'0') || (*String > L'9')) return FALSE;

    /* Otherwise, initialize the output and loop the string */
    *Number = 0;
    while ((*String >= L'0') && (*String <= L'9'))
    {
        /* Save the first decimal */
        *Number = 10 * *Number;

        /* Compute and add the second one */
        *Number += *++String - L'0';
    }

    /* All done */
    return TRUE;
}

NTSTATUS
NTAPI
VTUTF8ChannelAnsiDispatch(IN PSAC_CHANNEL Channel,
                          IN SAC_ANSI_DISPATCH AnsiCode,
                          IN INT* Data,
                          IN ULONG Length)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCHAR LocalBuffer = NULL, Tmp;
    INT l;
    CHECK_PARAMETER1(Channel);

    /* Check which ANSI sequence we should output */
    switch (AnsiCode)
    {
        /* Send the [2J (Clear Screen and Reset Cursor) */
        case SacAnsiClearScreen:
            Tmp = "\x1B[2J";
            break;

        /* Send the [0J (Clear From Position Till End Of Screen) */
        case SacAnsiClearEndOfScreen:
            Tmp = "\x1B[0J";
            break;

        /* Send the [0K (Clear from Position Till End Of Line) */
        case SacAnsiClearEndOfLine:
            Tmp = "\x1B[0K";
            break;

        /* Send a combination of two [#m attribute changes */
        case SacAnsiSetColors:

            /* Allocate a small local buffer for it */
            LocalBuffer = SacAllocatePool(SAC_VTUTF8_COL_WIDTH, GLOBAL_BLOCK_TAG);
            CHECK_ALLOCATION(LocalBuffer);

            /* Caller should have sent two colors as two integers */
            if (!(Data) || (Length != 8))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Create the escape sequence string */
            l = sprintf(LocalBuffer, "\x1B[%dm\x1B[%dm", Data[1], Data[0]);
            ASSERT((l + 1)*sizeof(UCHAR) < SAC_VTUTF8_COL_WIDTH);
            ASSERT(LocalBuffer);
            Tmp = LocalBuffer;
            break;

        /* Send the [#;#H (Cursor Position) sequence */
        case SacAnsiSetPosition:

            /* Allocate a small local buffer for it */
            LocalBuffer = SacAllocatePool(SAC_VTUTF8_COL_WIDTH, GLOBAL_BLOCK_TAG);
            CHECK_ALLOCATION(LocalBuffer);

            /* Caller should have sent the position as two integers */
            if (!(Data) || (Length != 8))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Create the escape sequence string */
            l = sprintf(LocalBuffer, "\x1B[%d;%dH", Data[1] + 1, Data[0] + 1);
            ASSERT((l + 1)*sizeof(UCHAR) < SAC_VTUTF8_COL_WIDTH);
            ASSERT(LocalBuffer);
            Tmp = LocalBuffer;
            break;

        /* Send the [0m sequence (Set Attribute 0) */
        case SacAnsiClearAttributes:
            Tmp = "\x1B[0m";
            break;

        /* Send the [7m sequence (Set Attribute 7) */
        case SacAnsiSetInverseAttribute:
            Tmp = "\x1B[7m";
            break;

        /* Send the [27m sequence (Set Attribute 27) */
        case SacAnsiClearInverseAttribute:
            Tmp = "\x1B[27m";
            break;

        /* Send the [5m sequence (Set Attribute 5) */
        case SacAnsiSetBlinkAttribute:
            Tmp = "\x1B[5m";
            break;

        /* Send the [25m sequence (Set Attribute 25) */
        case SacAnsiClearBlinkAttribute:
            Tmp = "\x1B[25m";
            break;

        /* Send the [1m sequence (Set Attribute 1) */
        case SacAnsiSetBoldAttribute:
            Tmp = "\x1B[1m";
            break;

        /* Send the [22m sequence (Set Attribute 22) */
        case SacAnsiClearBoldAttribute:
            Tmp = "\x1B[22m";
            break;

        /* We don't recognize it */
        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    /* Did everything work above? */
    if (NT_SUCCESS(Status))
    {
        /* Go write out the sequence */
        Status = ConMgrWriteData(Channel, Tmp, strlen(Tmp));
        if (NT_SUCCESS(Status))
        {
            /* Now flush it */
            Status = ConMgrFlushData(Channel);
        }
    }

    /* Free the temporary buffer, if any, and return the status */
    if (LocalBuffer) SacFreePool(LocalBuffer);
    return Status;
}

NTSTATUS
NTAPI
VTUTF8ChannelProcessAttributes(IN PSAC_CHANNEL Channel,
                               IN UCHAR Attribute)
{
    NTSTATUS Status;
    CHECK_PARAMETER(Channel);

    /* Set bold if needed */
    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                       Attribute & SAC_CELL_FLAG_BOLD ?
                                       SacAnsiSetBoldAttribute :
                                       SacAnsiClearBoldAttribute,
                                       NULL,
                                       0);
    if (!NT_SUCCESS(Status)) return Status;
    
    /* Set blink if needed */
    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                       Attribute & SAC_CELL_FLAG_BLINK ?
                                       SacAnsiSetBlinkAttribute :
                                       SacAnsiClearBlinkAttribute,
                                       NULL,
                                       0);
    if (!NT_SUCCESS(Status)) return Status;

    /* Set inverse if needed */
    return VTUTF8ChannelAnsiDispatch(Channel,
                                     Attribute & SAC_CELL_FLAG_INVERTED ?
                                     SacAnsiSetInverseAttribute :
                                     SacAnsiClearInverseAttribute,
                                     NULL,
                                     0);
}

//
// This function is the guts of the sequences that SAC supports.
//
// It is written to conform to the way that Microsoft's SAC driver interprets
// the ANSI standard. If you want to extend and/or "fix" it, please use a flag
// that can be set in the Registry to enable "extended" ANSI support or etc...
//
// Hermes, I'm looking at you, buddy.
//
ULONG
NTAPI
VTUTF8ChannelConsumeEscapeSequence(IN PSAC_CHANNEL Channel,
                                   IN PWCHAR String)
{
    ULONG Number, Number2, Number3, i, Action, Result;
    PWCHAR Sequence;
    PSAC_VTUTF8_SCREEN Screen;
    ASSERT(String[0] == VT_ANSI_ESCAPE);

    /* Microsoft's driver does this after the O(n) check below. Be smarter. */
    if (String[1] != VT_ANSI_COMMAND) return 0;

    /* Now that we know it's a valid command, look through the common cases */
    for (i = 0; i < RTL_NUMBER_OF(SacStaticEscapeStrings); i++)
    {
        /* Check if an optimized sequence was detected */
        if (!wcsncmp(String + 1,
                     SacStaticEscapeStrings[i].Sequence,
                     SacStaticEscapeStrings[i].Size))
        {
            /* Yep, return the right action, length, and set optionals to 1 */
            Action = SacStaticEscapeStrings[i].Action;
            Result = SacStaticEscapeStrings[i].Size + 1;
            Number = Number2 = Number3 = 1;
            goto ProcessString;
        }
    }

    /* It's a more complex sequence, start parsing it */
    Result = 0;
    Sequence = String + 2;

    /* First, check for the cursor sequences. This is useless due to above. */
    switch (*Sequence)
    {
        case VT_ANSI_CURSOR_UP_CHAR:
            Action = SacCursorUp;
            goto ProcessString;

        case VT_ANSI_CURSOR_DOWN_CHAR:
            Action = SacCursorDown;
            goto ProcessString;

        case VT_ANSI_CURSOR_RIGHT_CHAR:
            Action = SacCursorLeft; //bug
            goto ProcessString;

        case VT_ANSI_CURSOR_LEFT_CHAR:
            Action = SacCursorRight; //bug
            goto ProcessString;

        case VT_ANSI_ERASE_LINE_CHAR:
            Action = SacEraseEndOfLine;
            goto ProcessString;

        default:
            break;
    }

    /* This must be a sequence starting with ESC[# */
    if (!VTUTF8ChannelScanForNumber(Sequence, &Number)) return 0;
    while ((*Sequence >= L'0') && (*Sequence <= L'9')) Sequence++;

    /* Check if this is ESC[#m */
    if (*Sequence == VT_ANSI_SET_ATTRIBUTE_CHAR)
    {
        /* Some attribute is being set, go over the ones we support */
        switch (Number)
        {
            /* Make the font standard */
            case Normal:
                Action = SacFontNormal;
                break;

            /* Make the font bold */
            case Bold:
                Action = SacFontBold;
                break;

            /* Make the font blink */
            case SlowBlink:
                Action = SacFontBlink;
                break;

            /* Make the font colors inverted */
            case Inverse:
                Action = SacFontInverse;
                break;

            /* Make the font standard intensity */
            case BoldOff:
                Action = SacFontBoldOff;
                break;

            /* Turn off blinking */
            case BlinkOff:
                Action = SacFontBlinkOff;
                break;

            /* Turn off inverted colors */
            case InverseOff:
                Action = SacFontInverseOff;
                break;

            /* Something else... */
            default:

                /* Is a background color being set? */
                if ((Number < SetBackColorStart) || (Number > SetBackColorMax))
                {
                    /* Nope... is it the foreground color? */
                    if ((Number < SetColorStart) || (Number > SetColorMax))
                    {
                        /* Nope. SAC expects no other attributes so bail out */
                        ASSERT(FALSE);
                        return 0;
                    }

                    /* Yep -- the number will tell us which */
                    Action = SacSetFontColor;
                }
                else
                {
                    /* Yep -- the number will tell us which */
                    Action = SacSetBackgroundColor;
                }
                break;
        }

        /* In all cases, we're done here */
        goto ProcessString;
    }

    /* The only allowed possibility now is ESC[#;# */
    if (*Sequence != VT_ANSI_SEPARATOR_CHAR) return 0;
    Sequence++;
    if (!VTUTF8ChannelScanForNumber(Sequence, &Number2)) return 0;
    while ((*Sequence >= L'0') && (*Sequence <= L'9')) Sequence++;

    /* There's two valid forms accepted at this point: ESC[#;#m and ESC[#;#H */
    switch (*Sequence)
    {
        /* This is ESC[#;#m -- used to set both colors at once */
        case VT_ANSI_SET_ATTRIBUTE_CHAR:
            Action = SacSetColors;
            goto ProcessString;

        /* This is ESC[#;#H -- used to set the cursor position */
        case VT_ANSI_CUP_CURSOR_CHAR:
            Action = SacSetCursorPosition;
            goto ProcessString;

        /* Finally, this *could* be ESC[#;#; -- we'll keep parsing */
        case VT_ANSI_SEPARATOR_CHAR:
            Sequence++;
            break;

        /* Abandon anything else */
        default:
            return 0;
    }

    /* The SAC seems to accept a few more possibilities if a ';' follows... */
    switch (*Sequence)
    {
        /* Both ESC[#;#;H and ESC[#;#;f are really the same command */
        case VT_ANSI_CUP_CURSOR_CHAR:
        case VT_ANSI_HVP_CURSOR_CHAR:
            /* It's unclear why MS doesn't allow the HVP sequence on its own */
            Action = SacSetCursorPosition;
            goto ProcessString;

        /* And this is the ESC[#;#;r command to set the scroll region... */
        case VT_ANSI_SCROLL_CHAR:
            /* Again, not clear why ESC[#;#r isn't supported */
            Action = SacSetScrollRegion;
            goto ProcessString;

        /* Anything else must be ESC[#;#;# */
        default:
            break;
    }

    /* Get the last "#" */
    if (!VTUTF8ChannelScanForNumber(Sequence, &Number3)) return 0;
    while ((*Sequence >= L'0') && (*Sequence <= L'9')) Sequence++;

    /* And now the only remaining possibility is ESC[#;#;#;m */
    if (*Sequence == VT_ANSI_SET_ATTRIBUTE_CHAR)
    {
        /* Which sets both color and attributes in one command */
        Action = SacSetColorsAndAttributes;
        goto ProcessString;
    }

    /* No other sequences supported */
    return 0;

ProcessString:
    /* Unless we got here from the optimized path, calculate the length */
    if (!Result) Result = Sequence - String + 1;

    /* Get the current cell buffer */
    Screen = (PSAC_VTUTF8_SCREEN)Channel->OBuffer;
    VTUTF8ChannelAssertCursor(Channel);

    /* Handle all the supported SAC ANSI commands */
    switch (Action)
    {
        case SacCursorUp:
            /* Check if we are scrolling too high */
            if (Channel->CursorRow < Number)
            {
                /* Reset the row to the top */
                Channel->CursorRow = 0;
            }
            else
            {
                /* We're fine -- scroll up by that much */
                Channel->CursorRow -= Number;
            }

            /* All done */
            VTUTF8ChannelAssertCursor(Channel);
            break;

        case SacCursorDown:
            /* Check if we are scrolling too low */
            if (Channel->CursorRow >= SAC_VTUTF8_ROW_HEIGHT)
            {
                /* Reset the row to the bottom */
                Channel->CursorRow = SAC_VTUTF8_ROW_HEIGHT;
            }
            else
            {
                /* We're fine -- scroll down by that much */
                Channel->CursorRow += Number;
            }

            /* All done */
            VTUTF8ChannelAssertCursor(Channel);
            break;

        case SacCursorLeft:
            /* Check if we're scrolling too much to the left */
            if (Channel->CursorCol < Number)
            {
                /* Reset the column to the left-most margin */
                Channel->CursorCol = 0;
            }
            else
            {
                /* We're fine -- scroll left by that much */
                Channel->CursorCol -= Number;
            }

            /* All done */
            VTUTF8ChannelAssertCursor(Channel);
            break;

        case SacCursorRight:
            /* Check if we're scrolling too much to the right */
            if (Channel->CursorCol >= SAC_VTUTF8_COL_WIDTH)
            {
                /* Reset the column to the right-most margin */
                Channel->CursorCol = SAC_VTUTF8_COL_WIDTH;
            }
            else
            {
                /* We're fine -- scroll right by that much */
                Channel->CursorCol += Number;
            }

            /* All done */
            VTUTF8ChannelAssertCursor(Channel);
            break;

        case SacFontNormal:
            /* Reset the cell attributes */
            Channel->CellFlags = 0;
            Channel->CellBackColor = SetBackColorBlack;
            Channel->CellForeColor = SetColorWhite;
            break;

        case SacFontBlink:
            /* Set the appropriate flag */
            Channel->CellFlags |= SAC_CELL_FLAG_BLINK;
            break;

        case SacFontBlinkOff:
            /* Clear the appropriate flag */
            Channel->CellFlags &= ~SAC_CELL_FLAG_BLINK;
            break;

        case SacFontBold:
            /* Set the appropriate flag */
            Channel->CellFlags |= SAC_CELL_FLAG_BOLD;
            break;

        case SacFontBoldOff:
            /* Clear the appropriate flag */
            Channel->CellFlags &= ~SAC_CELL_FLAG_BOLD;
            break;

        case SacFontInverse:
            /* Set the appropriate flag */
            Channel->CellFlags |= SAC_CELL_FLAG_INVERTED;
            break;

        case SacFontInverseOff:
            /* Clear the appropriate flag */
            Channel->CellFlags &= ~SAC_CELL_FLAG_INVERTED;
            break;

        case SacEraseEndOfLine:
            /* Loop all columns in this line past the current position */
            for (i = Channel->CursorCol; i < SAC_VTUTF8_COL_WIDTH; i++)
            {
                /* Replace everything after the current position with blanks */
                Screen->Cell[Channel->CursorRow][i].CellFlags = Channel->CellFlags;
                Screen->Cell[Channel->CursorRow][i].CellBackColor = Channel->CellForeColor;
                Screen->Cell[Channel->CursorRow][i].CellForeColor = Channel->CellBackColor;
                Screen->Cell[Channel->CursorRow][i].Char = L' ';
            }
            break;

        case SacEraseStartOfLine:
            /* Loop all columns in this line, before the current position */
            for (i = 0; i < (Channel->CursorCol + 1); i++)
            {
                /* Replace everything after the current position with blanks */
                Screen->Cell[Channel->CursorRow][i].CellFlags = Channel->CellFlags;
                Screen->Cell[Channel->CursorRow][i].CellBackColor = Channel->CellForeColor;
                Screen->Cell[Channel->CursorRow][i].CellForeColor = Channel->CellBackColor;
                Screen->Cell[Channel->CursorRow][i].Char = L' ';
            }
            break;

        case SacEraseLine:
            /* Loop all the columns in this line */
            for (i = 0; i < SAC_VTUTF8_COL_WIDTH; i++)
            {
                /* Replace them all with blanks */
                Screen->Cell[Channel->CursorRow][i].CellFlags = Channel->CellFlags;
                Screen->Cell[Channel->CursorRow][i].CellBackColor = Channel->CellForeColor;
                Screen->Cell[Channel->CursorRow][i].CellForeColor = Channel->CellBackColor;
                Screen->Cell[Channel->CursorRow][i].Char = L' ';
            }
            break;

        case SacEraseEndOfScreen:
            ASSERT(FALSE); // todo
            break;

        case SacEraseStartOfScreen:
            ASSERT(FALSE); // todo
            break;

        case SacEraseScreen:
            ASSERT(FALSE); // todo
            break;

        case SacSetCursorPosition:
            ASSERT(FALSE); // todo
            break;

        case SacSetScrollRegion:
            ASSERT(FALSE); // todo
            break;

        case SacSetColors:
            /* Set the cell colors */
            Channel->CellForeColor = Number;
            Channel->CellBackColor = Number2;
            break;

        case SacSetBackgroundColor:
            /* Set the cell back color */
            Channel->CellBackColor = Number;
            break;

        case SacSetFontColor:
            /* Set the cell text color */
            Channel->CellForeColor = Number;
            break;

        case SacSetColorsAndAttributes:
            /* Set the cell flag and colors */
            Channel->CellFlags = Number;
            Channel->CellForeColor = Number2;
            Channel->CellBackColor = Number3;
            break;

        default:
            /* Unknown, do nothing */
            break;
    }

    /* Return the length of the sequence */
    return Result;
}

NTSTATUS
NTAPI
VTUTF8ChannelOInit(IN PSAC_CHANNEL Channel)
{
    PSAC_VTUTF8_SCREEN Screen;
    ULONG R, C;
    CHECK_PARAMETER(Channel);

    /* Set the current channel cell parameters */
    Channel->CellFlags = 0;
    Channel->CellBackColor = SetBackColorBlack;
    Channel->CellForeColor = SetColorWhite;

    /* Set the cell buffer position */
    Screen = (PSAC_VTUTF8_SCREEN)Channel->OBuffer;

    /* Loop the output buffer height by width */
    for (R = 0; R < SAC_VTUTF8_ROW_HEIGHT; R++)
    {
        for (C = 0; C < SAC_VTUTF8_COL_WIDTH; C++)
        {
            /* For every character, set the defaults */
            Screen->Cell[R][C].Char = L' ';
            Screen->Cell[R][C].CellBackColor = SetBackColorBlack;
            Screen->Cell[R][C].CellForeColor = SetColorWhite;
        }
    }

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelCreate(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;
    CHECK_PARAMETER(Channel);

    /* Allocate the output buffer */
    Channel->OBuffer = SacAllocatePool(SAC_VTUTF8_OBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->OBuffer);

    /* Allocate the input buffer */
    Channel->IBuffer = SacAllocatePool(SAC_VTUTF8_IBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->IBuffer);

    /* Initialize the output stream */
    Status = VTUTF8ChannelOInit(Channel);
    if (NT_SUCCESS(Status)) return Status;

    /* Reset all flags and return success */
    _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 0);
    _InterlockedExchange(&Channel->ChannelHasNewIBufferData, 0);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelDestroy(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER(Channel);

    /* Free the buffer and then destroy the channel */
    if (Channel->OBuffer) SacFreePool(Channel->OBuffer);
    if (Channel->IBuffer) SacFreePool(Channel->IBuffer);
    return ChannelDestroy(Channel);
}

NTSTATUS
NTAPI
VTUTF8ChannelORead(IN PSAC_CHANNEL Channel,
                   IN PCHAR Buffer,
                   IN ULONG BufferSize,
                   OUT PULONG ByteCount)
{
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelOFlush(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;
    PSAC_VTUTF8_SCREEN Screen;
    INT Color[2], Position[2];
    ULONG Utf8ProcessedCount, Utf8Count, R, C, ForeColor, BackColor, Attribute;
    PWCHAR TmpBuffer;
    BOOLEAN Overflow = FALSE;
    CHECK_PARAMETER(Channel);

    /* Set the cell buffer position */
    Screen = (PSAC_VTUTF8_SCREEN)Channel->OBuffer;

    /* Allocate a temporary buffer */
    TmpBuffer = SacAllocatePool(40, GLOBAL_BLOCK_TAG);
    if (!TmpBuffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* First, clear the screen */
    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                       SacAnsiClearScreen,
                                       NULL,
                                       0);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Next, reset the cursor position */
    Position[1] = 0;
    Position[0] = 0;
    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                       SacAnsiSetPosition,
                                       Position,
                                       sizeof(Position));
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Finally, reset the attributes */
    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                       SacAnsiClearAttributes,
                                       NULL,
                                       0);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Now set the current cell attributes */
    Attribute = Channel->CellFlags;
    Status = VTUTF8ChannelProcessAttributes(Channel, Attribute);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* And set the current cell colors */
    ForeColor = Channel->CellForeColor;
    BackColor = Channel->CellBackColor;
    Color[1] = BackColor;
    Color[0] = ForeColor;
    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                       SacAnsiSetColors,
                                       Color,
                                       sizeof(Color));
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Now loop all the characters in the cell buffer */
    for (R = 0; R < SAC_VTUTF8_ROW_HEIGHT; R++)
    {
        /* Across every row */
        for (C = 0; C < SAC_VTUTF8_COL_WIDTH; C++)
        {
            /* Check if there's been a change in colors */
            if ((Screen->Cell[R][C].CellBackColor != BackColor) ||
                (Screen->Cell[R][C].CellForeColor != ForeColor))
            {
                /* New colors are being drawn -- are we also on a new row now? */
                if (Overflow)
                {
                    /* Reposition the cursor correctly */
                    Position[1] = R;
                    Position[0] = C;
                    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                                       SacAnsiSetPosition,
                                                       Position,
                                                       sizeof(Position));
                    if (!NT_SUCCESS(Status)) goto Quickie;
                    Overflow = FALSE;
                }

                /* Cache the new colors */
                ForeColor = Screen->Cell[R][C].CellForeColor;
                BackColor = Screen->Cell[R][C].CellBackColor;

                /* Set them on the screen */
                Color[1] = BackColor;
                Color[0] = ForeColor;
                Status = VTUTF8ChannelAnsiDispatch(Channel,
                                                   SacAnsiSetColors,
                                                   Color,
                                                   sizeof(Color));
                if (!NT_SUCCESS(Status)) goto Quickie;
            }

            /* Check if there's been a change in attributes */
            if (Screen->Cell[R][C].CellFlags != Attribute)
            {
                /* Yep! Are we also on a new row now? */
                if (Overflow)
                {
                    /* Reposition the cursor correctly */
                    Position[1] = R;
                    Position[0] = C;
                    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                                       SacAnsiSetPosition,
                                                       Position,
                                                       sizeof(Position));
                    if (!NT_SUCCESS(Status)) goto Quickie;
                    Overflow = FALSE;
                }

                /* Set the new attributes on screen */
                Attribute = Screen->Cell[R][C].CellFlags;
                Status = VTUTF8ChannelProcessAttributes(Channel, Attribute);
                if (!NT_SUCCESS(Status)) goto Quickie;
            }

            /* Time to write the character -- are we on a new row now? */
            if (Overflow)
            {
                /* Reposition the cursor correctly */
                Position[1] = R;
                Position[0] = C;
                Status = VTUTF8ChannelAnsiDispatch(Channel,
                                                   SacAnsiSetPosition,
                                                   Position,
                                                   sizeof(Position));
                if (!NT_SUCCESS(Status)) goto Quickie;
                Overflow = FALSE;
            }

            /* Write the character into our temporary buffer */
            *TmpBuffer = Screen->Cell[R][C].Char;
            TmpBuffer[1] = UNICODE_NULL;

            /* Convert it to UTF-8 */
            if (!SacTranslateUnicodeToUtf8(TmpBuffer,
                                           1,
                                           Utf8ConversionBuffer,
                                           Utf8ConversionBufferSize,
                                           &Utf8Count,
                                           &Utf8ProcessedCount))
            {
                /* Bail out if this failed */
                Status = STATUS_UNSUCCESSFUL;
                goto Quickie;
            }

            /* Make sure we have a remaining valid character */
            if (Utf8Count)
            {
                /* Write it out on the wire */
                Status = ConMgrWriteData(Channel, Utf8ConversionBuffer, Utf8Count);
                if (!NT_SUCCESS(Status)) goto Quickie;
            }
        }

        /* All the characters on the row are done, indicate we need a reset */
        Overflow = TRUE;
    }

    /* Everything is done, set the positition one last time */
    Position[1] = Channel->CursorRow;
    Position[0] = Channel->CursorCol;
    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                       SacAnsiSetPosition,
                                       Position,
                                       sizeof(Position));
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Set the current attribute one last time */
    Status = VTUTF8ChannelProcessAttributes(Channel, Channel->CellFlags);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Set the current colors one last time */
    Color[1] = Channel->CellBackColor;
    Color[0] = Channel->CellForeColor;
    Status = VTUTF8ChannelAnsiDispatch(Channel,
                                       SacAnsiSetColors,
                                       Color,
                                       sizeof(Color));
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Flush all the data out on the wire */
    Status = ConMgrFlushData(Channel);

Quickie:
    /* We're done, free the temporary buffer */
    if (TmpBuffer) SacFreePool(TmpBuffer);

    /* Indicate that all new data has been flushed now */
    if (NT_SUCCESS(Status))
    {
        _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 0);
    }

    /* Return the result */
    return Status;
}

NTSTATUS
NTAPI
VTUTF8ChannelOWrite2(IN PSAC_CHANNEL Channel,
                     IN PWCHAR String,
                     IN ULONG Size)
{
    PSAC_VTUTF8_SCREEN Screen;
    ULONG i, EscapeSize, R, C;
    PWSTR pwch;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);
    VTUTF8ChannelAssertCursor(Channel);

    /* Loop every character */
    Screen = (PSAC_VTUTF8_SCREEN)Channel->OBuffer;
    for (i = 0; i < Size; i++)
    {
        /* Check what the character is */
        pwch = &String[i];
        switch (*pwch)
        {
            /* It's an escape sequence... */
            case L'\x1B':

                /* Send it to the parser, see how long the sequence was */
                EscapeSize = VTUTF8ChannelConsumeEscapeSequence(Channel, pwch);
                if (EscapeSize)
                {
                    /* Consume that many characters for next time*/
                    i += EscapeSize - 1;
                }
                else
                {
                    /* Invalid escape sequence, skip just the ESC character */
                    i++;
                }

                /* Keep going*/
                break;

            /* It's a line feed */
            case L'\n':

                /* Simply reset the column to zero on the current line */
                Channel->CursorCol = 0;
                break;

            /* It's a carriage feed */
            case L'\r':

                /* Move to the next row */
                Channel->CursorRow++;

                /* Check if we hit the last row on the screen */
                if (Channel->CursorRow >= SAC_VTUTF8_ROW_HEIGHT)
                {
                    /* Go over every row before the last one */
                    for (R = 0; R < (SAC_VTUTF8_ROW_HEIGHT - 1); R++)
                    {
                        /* Sanity check, since we always copy one row below */
                        ASSERT((R + 1) < SAC_VTUTF8_ROW_HEIGHT);

                        /* Loop every character on the row */
                        for (C = 0; C < SAC_VTUTF8_COL_WIDTH; C++)
                        {
                            /* And replace it with one from the row below */
                            Screen->Cell[R][C] = Screen->Cell[R + 1][C];
                        }
                    }

                    /* Now we're left with the before-last row, zero it out */
                    ASSERT(R == (SAC_VTUTF8_ROW_HEIGHT - 1));
                    RtlZeroMemory(&Screen->Cell[R], sizeof(Screen->Cell[R]));

                    /* Reset the row back by one */
                    Channel->CursorRow--;
                    VTUTF8ChannelAssertCursor(Channel);
                }
                break;

            /* It's a TAB character */
            case L'\t':

                /* Loop the remaining characters until a multiple of 4 */
                VTUTF8ChannelAssertCursor(Channel);
                for (C = (4 - Channel->CursorCol % 4); C; C--)
                {
                    /* Fill each remaining character with a space */
                    VTUTF8ChannelAssertCursor(Channel);
                    Screen->Cell[Channel->CursorRow][Channel->CursorCol].CellFlags = Channel->CellFlags;
                    Screen->Cell[Channel->CursorRow][Channel->CursorCol].CellBackColor = Channel->CellBackColor;
                    Screen->Cell[Channel->CursorRow][Channel->CursorCol].CellForeColor = Channel->CellForeColor;
                    Screen->Cell[Channel->CursorRow][Channel->CursorCol].Char = L' ';

                    /* Move to the next character position, but don't overflow */
                    Channel->CursorCol++;
                    if (Channel->CursorCol >= SAC_VTUTF8_COL_WIDTH)
                    {
                        Channel->CursorCol = SAC_VTUTF8_COL_WIDTH - 1;
                    }
                }

                /* All done, move to the next one */
                VTUTF8ChannelAssertCursor(Channel);
                break;

            /* It's a backspace or delete character */
            case L'\b':
            case L'\x7F':

                /* Move back one character, unless we had nothing typed */
                if (Channel->CursorCol) Channel->CursorCol--;
                VTUTF8ChannelAssertCursor(Channel);
                break;

            /* It's some other character */
            default:

                /* Is it non-printable? Ignore it and keep parsing */
                if (*pwch < L' ') continue;

                /* Otherwise, print it out with the current attributes */
                VTUTF8ChannelAssertCursor(Channel);
                Screen->Cell[Channel->CursorRow][Channel->CursorCol].CellFlags = Channel->CellFlags;
                Screen->Cell[Channel->CursorRow][Channel->CursorCol].CellBackColor = Channel->CellBackColor;
                Screen->Cell[Channel->CursorRow][Channel->CursorCol].CellForeColor = Channel->CellForeColor;
                Screen->Cell[Channel->CursorRow][Channel->CursorCol].Char = *pwch;

                /* Move forward one character, but make sure not to overflow */
                Channel->CursorCol++;
                if (Channel->CursorCol == SAC_VTUTF8_COL_WIDTH)
                {
                    Channel->CursorCol = SAC_VTUTF8_COL_WIDTH - 1;
                }

                /* All done, move to the next one */
                VTUTF8ChannelAssertCursor(Channel);
                break;
            }
    }

    /* Parsing of the input string completed -- string was written */
    VTUTF8ChannelAssertCursor(Channel);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelOEcho(IN PSAC_CHANNEL Channel,
                   IN PCHAR String,
                   IN ULONG Size)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwch;
    ULONG i, k, TranslatedCount, UTF8TranslationSize;
    BOOLEAN Result;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);

    /* Return success if there's nothing to echo */
    if (!(Size / sizeof(WCHAR))) return Status;

    /* Start with the input string */
    pwch = (PWCHAR)String;

    /* First, figure out how much is outside of the block length alignment */
    k = (Size / sizeof(WCHAR)) % MAX_UTF8_ENCODE_BLOCK_LENGTH;
    if (k)
    {
        /* Translate the misaligned portion */
        Result = SacTranslateUnicodeToUtf8(pwch,
                                           k,
                                           Utf8ConversionBuffer,
                                           Utf8ConversionBufferSize,
                                           &UTF8TranslationSize,
                                           &TranslatedCount);
        ASSERT(k == TranslatedCount);
        if (!Result)
        {
            /* If we couldn't translate, write failure to break out below */
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            /* Write the misaligned portion into the buffer */
            Status = ConMgrWriteData(Channel,
                                     Utf8ConversionBuffer,
                                     UTF8TranslationSize);
        }

        /* If translation or write failed, bail out */
        if (!NT_SUCCESS(Status)) goto Return;
    }

    /* Push the string to its new location (this could be a noop if aligned) */
    pwch += k;

    /* Now figure out how many aligned blocks we have, and loop each one */
    k = (Size / sizeof(WCHAR)) / MAX_UTF8_ENCODE_BLOCK_LENGTH;
    for (i = 0; i < k; i++)
    {
        /* Translate the aligned block */
        Result = SacTranslateUnicodeToUtf8(pwch,
                                           MAX_UTF8_ENCODE_BLOCK_LENGTH,
                                           Utf8ConversionBuffer,
                                           Utf8ConversionBufferSize,
                                           &UTF8TranslationSize,
                                           &TranslatedCount);
        ASSERT(MAX_UTF8_ENCODE_BLOCK_LENGTH == TranslatedCount);
        ASSERT(UTF8TranslationSize > 0);
        if (!Result)
        {
            /* Set failure here, we'll break out below */
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            /* Move the string location to the next aligned block */
            pwch += MAX_UTF8_ENCODE_BLOCK_LENGTH;

            /* Write the aligned block into the buffer */
            Status = ConMgrWriteData(Channel,
                                     Utf8ConversionBuffer,
                                     UTF8TranslationSize);
        }

        /* If translation or write failed, bail out */
        if (!NT_SUCCESS(Status)) break;
    }

Return:
    ASSERT(pwch == (PWSTR)(String + Size));
    if (NT_SUCCESS(Status)) Status = ConMgrFlushData(Channel);
    return Status;
}

NTSTATUS
NTAPI
VTUTF8ChannelOWrite(IN PSAC_CHANNEL Channel,
                    IN PCHAR String,
                    IN ULONG Length)
{
    NTSTATUS Status;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);

    /* Call the lower level function */
    Status = VTUTF8ChannelOWrite2(Channel, (PWCHAR)String, Length / sizeof(WCHAR));
    if (NT_SUCCESS(Status))
    {
        /* Is the channel enabled for output? */
        if ((ConMgrIsWriteEnabled(Channel)) && (Channel->WriteEnabled))
        {
            /* Go ahead and output it */
            Status = VTUTF8ChannelOEcho(Channel, String, Length);
        }
        else
        {
            /* Otherwise, just remember that we have new data */
            _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 1);
        }
    }

    /* We're done */
    return Status;
}

ULONG
NTAPI
VTUTF8ChannelGetIBufferIndex(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);
    ASSERT((Channel->IBufferIndex % sizeof(WCHAR)) == 0);
    ASSERT(Channel->IBufferIndex < SAC_VTUTF8_IBUFFER_SIZE);

    /* Return the current buffer index */
    return Channel->IBufferIndex;
}

VOID
NTAPI
VTUTF8ChannelSetIBufferIndex(IN PSAC_CHANNEL Channel,
                             IN ULONG BufferIndex)
{
    NTSTATUS Status;
    ASSERT(Channel);
    ASSERT((Channel->IBufferIndex % sizeof(WCHAR)) == 0);
    ASSERT(Channel->IBufferIndex < SAC_VTUTF8_IBUFFER_SIZE);

    /* Set the new index, and if it's not zero, it means we have data */
    Channel->IBufferIndex = BufferIndex;
    _InterlockedExchange(&Channel->ChannelHasNewIBufferData, BufferIndex != 0);

    /* If we have new data, and an event has been registered... */
    if (!(Channel->IBufferIndex) &&
        (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT))
    {
        /* Go ahead and signal it */
        ChannelClearEvent(Channel, HasNewDataEvent);
        UNREFERENCED_PARAMETER(Status);
    }
}

NTSTATUS
NTAPI
VTUTF8ChannelIRead(IN PSAC_CHANNEL Channel,
                   IN PCHAR Buffer,
                   IN ULONG BufferSize,
                   IN PULONG ReturnBufferSize)
{
    ULONG CopyChars, ReadLength;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Buffer);
    CHECK_PARAMETER_WITH_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    /* Assume failure */
    *ReturnBufferSize = 0;

    /* Check how many bytes are in the buffer */
    if (Channel->ChannelInputBufferLength(Channel) == 0)
    {
        /* Apparently nothing. Make sure the flag indicates so too */
        ASSERT(ChannelHasNewIBufferData(Channel) == FALSE);
    }
    else
    {
        /* Use the smallest number of bytes either in the buffer or requested */
        ReadLength = min(Channel->ChannelInputBufferLength(Channel) * sizeof(WCHAR),
                         BufferSize);

        /* Do some cheezy buffer alignment */
        CopyChars = ReadLength / sizeof(WCHAR);
        ReadLength = CopyChars * sizeof(WCHAR);
        ASSERT(CopyChars <= Channel->ChannelInputBufferLength(Channel));

        /* Copy them into the caller's buffer */
        RtlCopyMemory(Buffer, Channel->IBuffer, ReadLength);

        /* Update the channel's index past the copied (read) bytes */
        VTUTF8ChannelSetIBufferIndex(Channel,
            VTUTF8ChannelGetIBufferIndex(Channel) - ReadLength);

        /* Are there still bytes that haven't been read yet? */
        if (Channel->ChannelInputBufferLength(Channel))
        {
            /* Shift them up in the buffer */
            RtlMoveMemory(Channel->IBuffer,
                          &Channel->IBuffer[ReadLength],
                          Channel->ChannelInputBufferLength(Channel) *
                          sizeof(WCHAR));
        }

        /* Return the number of bytes we actually copied */
        *ReturnBufferSize = ReadLength;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelIBufferIsFull(IN PSAC_CHANNEL Channel,
                           OUT PBOOLEAN BufferStatus)
{
    CHECK_PARAMETER1(Channel);

    /* If the index is beyond the length, the buffer must be full */
    *BufferStatus = VTUTF8ChannelGetIBufferIndex(Channel) > SAC_VTUTF8_IBUFFER_SIZE;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
VTUTF8ChannelIBufferLength(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);

    /* The index is the length, so divide by two to get character count */
    return VTUTF8ChannelGetIBufferIndex(Channel) / sizeof(WCHAR);
}

WCHAR
NTAPI
VTUTF8ChannelIReadLast(IN PSAC_CHANNEL Channel)
{
    PWCHAR LastCharLocation;
    WCHAR LastChar = 0;
    ASSERT(Channel);

    /* Check if there's anything to read in the buffer */
    if (Channel->ChannelInputBufferLength(Channel))
    {
        /* Go back one character */
        VTUTF8ChannelSetIBufferIndex(Channel,
                                     VTUTF8ChannelGetIBufferIndex(Channel) -
                                     sizeof(WCHAR));

        /* Read it, and clear its current value */
        LastCharLocation = (PWCHAR)&Channel->IBuffer[VTUTF8ChannelGetIBufferIndex(Channel)];
        LastChar = *LastCharLocation;
        *LastCharLocation = UNICODE_NULL;
    }

    /* Return the last character */
    return LastChar;
}

NTSTATUS
NTAPI
VTUTF8ChannelIWrite(IN PSAC_CHANNEL Channel,
                    IN PCHAR Buffer,
                    IN ULONG BufferSize)
{
    NTSTATUS Status;
    BOOLEAN IsFull;
    ULONG Index, i;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Buffer);
    CHECK_PARAMETER_WITH_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    /* First, check if the input buffer still has space */
    Status = VTUTF8ChannelIBufferIsFull(Channel, &IsFull);
    if (!NT_SUCCESS(Status)) return Status;
    if (IsFull) return STATUS_UNSUCCESSFUL;

    /* Get the current buffer index */
    Index = VTUTF8ChannelGetIBufferIndex(Channel);
    if ((SAC_VTUTF8_IBUFFER_SIZE - Index) < BufferSize)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the new data */
    for (i = 0; i < BufferSize; i++)
    {
        /* Convert the character */
        if (SacTranslateUtf8ToUnicode(Buffer[i],
                                      IncomingUtf8ConversionBuffer,
                                      &IncomingUnicodeValue))
        {
            /* Write it into the buffer */
            *(PWCHAR)&Channel->IBuffer[VTUTF8ChannelGetIBufferIndex(Channel)] =
                IncomingUnicodeValue;

            /* Update the index */
            Index = VTUTF8ChannelGetIBufferIndex(Channel);
            VTUTF8ChannelSetIBufferIndex(Channel, Index + sizeof(WCHAR));
        }
    }

    /* Signal the event, if one was set */
    if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT)
    {
        ChannelSetEvent(Channel, HasNewDataEvent);
    }

    /* All done */
    return STATUS_SUCCESS;
}
