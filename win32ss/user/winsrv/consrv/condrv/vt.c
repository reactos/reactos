/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/condrv/vt.c
 * PURPOSE:         Virtual Terminal processing (phase 1 implementation)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>
#include <debug.h>
#include "../conoutput.h"
#include "../coninput.h"
#include "../include/vt.h"

#include <limits.h>

NTSTATUS NTAPI ConDrvScrollConsoleScreenBuffer(PCONSOLE Console,
                                             PTEXTMODE_SCREEN_BUFFER Buffer,
                                             BOOLEAN Unicode,
                                             PSMALL_RECT ScrollRectangle,
                                             BOOLEAN UseClipRectangle,
                                             PSMALL_RECT ClipRectangle OPTIONAL,
                                             PCOORD DestinationOrigin,
                                             CHAR_INFO FillChar);

NTSTATUS NTAPI ConDrvWriteConsoleInput(PCONSOLE Console,
                                       PCONSOLE_INPUT_BUFFER InputBuffer,
                                       BOOLEAN AppendToEnd,
                                       PINPUT_RECORD InputRecord,
                                       ULONG NumEventsToWrite,
                                       PULONG NumEventsWritten OPTIONAL);

NTSTATUS NTAPI ConDrvSetConsoleActiveScreenBuffer(PCONSOLE Console,
                                                  PCONSOLE_SCREEN_BUFFER Buffer);
NTSTATUS NTAPI ConDrvSetConsoleCursorInfo(PCONSOLE Console,
                                          PTEXTMODE_SCREEN_BUFFER Buffer,
                                          PCONSOLE_CURSOR_INFO CursorInfo);
NTSTATUS NTAPI ConDrvSetConsoleCursorPosition(PCONSOLE Console,
                                              PTEXTMODE_SCREEN_BUFFER Buffer,
                                              PCOORD Position);
NTSTATUS NTAPI ConDrvSetConsoleScreenBufferSize(PCONSOLE Console,
                                                PTEXTMODE_SCREEN_BUFFER Buffer,
                                                PCOORD Size);
NTSTATUS NTAPI ConDrvSetConsoleWindowInfo(PCONSOLE Console,
                                          PTEXTMODE_SCREEN_BUFFER Buffer,
                                          BOOLEAN Absolute,
                                          PSMALL_RECT WindowRect);

static UCHAR
VtXtermToConsoleIndex(UCHAR Index);

static VOID
VtFillKeyEvent(PINPUT_RECORD Destination, WCHAR Character, BOOLEAN KeyDown);

static VOID
VtFlushDirty(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer);

static VOID
VtMarkDirty(PTEXTMODE_SCREEN_BUFFER ScreenBuffer, SHORT Left, SHORT Top, SHORT Right, SHORT Bottom);

#define VT_MAX_PARAMS 16
#define VT_MAX_INPUT_SEQUENCE_CHARS 32

#define FG_ATTR_MASK (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define BG_ATTR_MASK (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY)

#define VT_DEFAULT_TAB_WIDTH 8


#define VT_CHARSET_SLOT_G0                   0
#define VT_CHARSET_SLOT_G1                   1
#define VT_CHARSET_SLOT_G2                   2
#define VT_CHARSET_SLOT_G3                   3
#define VT_CHARSET_SLOT_INVALID              0xFF

typedef enum _VT_CHARSET_ID
{
    VtCharsetAscii = 0,
    VtCharsetDecSpecial,
    VtCharsetUnitedKingdom,
    VtCharsetDutch,
    VtCharsetFinnish,
    VtCharsetFrench,
    VtCharsetFrenchCanadian,
    VtCharsetGerman,
    VtCharsetItalian,
    VtCharsetNorwegianDanish,
    VtCharsetSpanish,
    VtCharsetSwedish,
    VtCharsetSwiss,
    VtCharsetPortuguese,
    VtCharsetDecTechnical
} VT_CHARSET_ID;

#define VT_DEC_SPECIAL_START 0x20
#define VT_DEC_SPECIAL_COUNT 0x60

static const WCHAR VtDecSpecialMap[VT_DEC_SPECIAL_COUNT] =
{
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x2192, 0x2190, 0x2191, 0x2193, 0x002F,
    0x2588, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x00A0,
    0x25C6, 0x2592, 0x2409, 0x240C, 0x240D, 0x240A, 0x00B0, 0x00B1,
    0x2591, 0x240B, 0x2518, 0x2510, 0x250C, 0x2514, 0x253C, 0x23BA,
    0x23BB, 0x2500, 0x23BC, 0x23BD, 0x251C, 0x2524, 0x2534, 0x252C,
    0x2502, 0x2264, 0x2265, 0x03C0, 0x2260, 0x00A3, 0x00B7, 0x007F,
};

typedef struct _VT_CHARSET_OVERRIDE
{
    WCHAR Source;
    WCHAR Target;
} VT_CHARSET_OVERRIDE;

static const VT_CHARSET_OVERRIDE VtNrcsBritishOverrides[] =
{
    { L'#', 0x00A3 }
};

static const VT_CHARSET_OVERRIDE VtNrcsDutchOverrides[] =
{
    { L'#', 0x00A3 },
    { L'@', 0x00BE },
    { L'[', 0x0133 },
    { L'\\', 0x00BD },
    { L']', 0x007C },
    { L'{', 0x00A8 },
    { L'|', 0x0192 },
    { L'}', 0x00BC },
    { L'~', 0x00B4 }
};

static const VT_CHARSET_OVERRIDE VtNrcsFinnishOverrides[] =
{
    { L'[', 0x00C4 },
    { L'\\', 0x00D6 },
    { L']', 0x00C5 },
    { L'^', 0x00DC },
    { L'`', 0x00E9 },
    { L'{', 0x00E4 },
    { L'|', 0x00F6 },
    { L'}', 0x00E5 },
    { L'~', 0x00FC }
};

static const VT_CHARSET_OVERRIDE VtNrcsFrenchOverrides[] =
{
    { L'#', 0x00A3 },
    { L'@', 0x00E0 },
    { L'[', 0x00B0 },
    { L'\\', 0x00E7 },
    { L']', 0x00A7 },
    { L'{', 0x00E9 },
    { L'|', 0x00F9 },
    { L'}', 0x00E8 },
    { L'~', 0x00A8 }
};

static const VT_CHARSET_OVERRIDE VtNrcsFrenchCanadianOverrides[] =
{
    { L'@', 0x00E0 },
    { L'[', 0x00E2 },
    { L'\\', 0x00E7 },
    { L']', 0x00EA },
    { L'^', 0x00EE },
    { L'`', 0x00F4 },
    { L'{', 0x00E9 },
    { L'|', 0x00F9 },
    { L'}', 0x00E8 },
    { L'~', 0x00FB }
};

static const VT_CHARSET_OVERRIDE VtNrcsGermanOverrides[] =
{
    { L'@', 0x00A7 },
    { L'[', 0x00C4 },
    { L'\\', 0x00D6 },
    { L']', 0x00DC },
    { L'{', 0x00E4 },
    { L'|', 0x00F6 },
    { L'}', 0x00FC },
    { L'~', 0x00DF }
};

static const VT_CHARSET_OVERRIDE VtNrcsItalianOverrides[] =
{
    { L'#', 0x00A3 },
    { L'@', 0x00A7 },
    { L'[', 0x00B0 },
    { L'\\', 0x00E7 },
    { L']', 0x00E9 },
    { L'`', 0x00F9 },
    { L'{', 0x00E0 },
    { L'|', 0x00F2 },
    { L'}', 0x00E8 },
    { L'~', 0x00EC }
};

static const VT_CHARSET_OVERRIDE VtNrcsNorwegianDanishOverrides[] =
{
    { L'@', 0x00C4 },
    { L'[', 0x00C6 },
    { L'\\', 0x00D8 },
    { L']', 0x00C5 },
    { L'^', 0x00DC },
    { L'`', 0x00E4 },
    { L'{', 0x00E6 },
    { L'|', 0x00F8 },
    { L'}', 0x00E5 },
    { L'~', 0x00FC }
};

static const VT_CHARSET_OVERRIDE VtNrcsSpanishOverrides[] =
{
    { L'#', 0x00A3 },
    { L'@', 0x00A7 },
    { L'[', 0x00A1 },
    { L'\\', 0x00D1 },
    { L']', 0x00BF },
    { L'{', 0x00B0 },
    { L'|', 0x00F1 },
    { L'}', 0x00E7 }
};

static const VT_CHARSET_OVERRIDE VtNrcsSwedishOverrides[] =
{
    { L'@', 0x00C9 },
    { L'[', 0x00C4 },
    { L'\\', 0x00D6 },
    { L']', 0x00C5 },
    { L'^', 0x00DC },
    { L'`', 0x00E9 },
    { L'{', 0x00E4 },
    { L'|', 0x00F6 },
    { L'}', 0x00E5 },
    { L'~', 0x00FC }
};

static const VT_CHARSET_OVERRIDE VtNrcsSwissOverrides[] =
{
    { L'#', 0x00F9 },
    { L'@', 0x00E0 },
    { L'[', 0x00E9 },
    { L'\\', 0x00E7 },
    { L']', 0x00EA },
    { L'^', 0x00EE },
    { L'_', 0x00E8 },
    { L'`', 0x00F4 },
    { L'{', 0x00E4 },
    { L'|', 0x00F6 },
    { L'}', 0x00FC },
    { L'~', 0x00FB }
};

static const VT_CHARSET_OVERRIDE VtNrcsPortugueseOverrides[] =
{
    { L'[', 0x00C3 },
    { L'\\', 0x00C7 },
    { L']', 0x00D5 },
    { L'{', 0x00E3 },
    { L'|', 0x00E7 },
    { L'}', 0x00F5 }
};

static const VT_CHARSET_OVERRIDE VtDecTechnicalOverrides[] =
{
    { L'!', 0x23B7 },
    { L'"', 0x250C },
    { L'#', 0x2500 },
    { L'$', 0x2320 },
    { L'%', 0x2321 },
    { L'&', 0x2502 },
    { L'\'', 0x23A1 },
    { L'(', 0x23A3 },
    { L')', 0x23A4 },
    { L'*', 0x23A6 },
    { L'+', 0x239B },
    { L',', 0x239D },
    { L'-', 0x239E },
    { L'.', 0x23A0 },
    { L'/', 0x23A8 }
};


#ifdef DBG

#define VT_TRACE_PARAM_LIMIT 6
#define VT_TRACE_LIMIT       64

static VOID
VtTraceUnhandledCsi(WCHAR PrivateIndicator,
                    WCHAR Intermediate,
                    WCHAR Final,
                    const ULONG *Params,
                    ULONG Count)
{
    static ULONG Logged;

    if (Logged >= VT_TRACE_LIMIT)
        return;

    Logged++;

    DPRINT1("VT: Unhandled CSI sequence (priv='%lc', inter='%lc', final='%lc', count=%lu)\n",
            PrivateIndicator ? PrivateIndicator : L' ',
            Intermediate ? Intermediate : L' ',
            Final, Count);

    if (Count > 0 && Params)
    {
        ULONG i;
        ULONG Limit = (Count < VT_TRACE_PARAM_LIMIT) ? Count : VT_TRACE_PARAM_LIMIT;

        for (i = 0; i < Limit; ++i)
            DPRINT1("    param[%lu] = %lu\n", i, Params[i]);

        if (Count > Limit)
            DPRINT1("    ... %lu more params omitted\n", Count - Limit);
    }
}

static VOID
VtTraceUnhandledEscape(WCHAR Indicator)
{
    static ULONG Logged;

    if (Logged >= VT_TRACE_LIMIT)
        return;

    Logged++;

    DPRINT1("VT: Unhandled ESC sequence (indicator='%lc', code=0x%04x)\n",
            (Indicator >= L' ' && Indicator <= L'~') ? Indicator : L' ',
            Indicator);
}

static VOID
VtTraceUnhandledOsc(ULONG Parameter)
{
    static ULONG Logged;

    if (Logged >= VT_TRACE_LIMIT)
        return;

    Logged++;

    DPRINT1("VT: Unhandled OSC %lu\n", Parameter);
}

static VOID
VtTraceUnhandledDcs(VOID)
{
    static ULONG Logged;

    if (Logged >= VT_TRACE_LIMIT)
        return;

    Logged++;

    DPRINT1("VT: Unhandled DCS sequence\n");
}

static VOID
VtTracePolicyBlocked(const char *Feature)
{
    static ULONG Logged;

    if (Logged >= VT_TRACE_LIMIT)
        return;

    Logged++;
    DPRINT1("VT: %s suppressed by policy\n", Feature);
}

static VOID
VtTraceDcsReplayFailure(NTSTATUS Status)
{
    static ULONG Logged;

    if (Logged >= VT_TRACE_LIMIT)
        return;

    Logged++;
    DPRINT1("VT: DCS passthrough replay failed (Status 0x%08lx)\n", Status);
}

#else

#define VtTraceUnhandledCsi(Private, Intermediate, Final, Params, Count) ((void)0)
#define VtTraceUnhandledEscape(Indicator) ((void)0)
#define VtTraceUnhandledOsc(Parameter) ((void)0)
#define VtTraceUnhandledDcs()          ((void)0)
#define VtTracePolicyBlocked(Feature)  ((void)0)
#define VtTraceDcsReplayFailure(Status) ((void)0)

#endif

static BOOLEAN
VtEnsureTabStops(PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    USHORT Width;
    PUCHAR NewStops;
    USHORT CopyLength;

    if (!ScreenBuffer)
        return FALSE;

    Width = (ScreenBuffer->ScreenBufferSize.X > 0)
                ? (USHORT)ScreenBuffer->ScreenBufferSize.X
                : 1;

    if (ScreenBuffer->VtState.TabStops && ScreenBuffer->VtState.TabStopLength == Width)
        return TRUE;

    NewStops = (PUCHAR)ConsoleAllocHeap(0, Width * sizeof(UCHAR));
    if (!NewStops)
        return FALSE;

    RtlZeroMemory(NewStops, Width * sizeof(UCHAR));

    if (ScreenBuffer->VtState.TabStops && ScreenBuffer->VtState.TabStopLength > 0)
    {
        CopyLength = min(ScreenBuffer->VtState.TabStopLength, Width);
        RtlCopyMemory(NewStops,
                      ScreenBuffer->VtState.TabStops,
                      CopyLength * sizeof(UCHAR));
        ConsoleFreeHeap(ScreenBuffer->VtState.TabStops);

        if (Width > CopyLength)
        {
            for (USHORT Column = CopyLength; Column < Width; ++Column)
            {
                if ((Column % VT_DEFAULT_TAB_WIDTH) == 0 && Column != 0)
                    NewStops[Column] = 1;
            }
        }
    }
    else
    {
        for (USHORT Column = VT_DEFAULT_TAB_WIDTH; Column < Width; Column += VT_DEFAULT_TAB_WIDTH)
            NewStops[Column] = 1;
    }

    ScreenBuffer->VtState.TabStops = NewStops;
    ScreenBuffer->VtState.TabStopLength = Width;
    return TRUE;
}

VOID
NTAPI
VtResetTabStops(PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    if (!VtEnsureTabStops(ScreenBuffer))
        return;

    RtlZeroMemory(ScreenBuffer->VtState.TabStops,
                  ScreenBuffer->VtState.TabStopLength * sizeof(UCHAR));

    for (USHORT Column = VT_DEFAULT_TAB_WIDTH;
         Column < ScreenBuffer->VtState.TabStopLength;
         Column += VT_DEFAULT_TAB_WIDTH)
    {
        ScreenBuffer->VtState.TabStops[Column] = 1;
    }
}

VOID
NTAPI
VtSetTabStopAtCursor(PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    if (!ScreenBuffer)
        return;

    if (!VtEnsureTabStops(ScreenBuffer))
        return;

    if (ScreenBuffer->CursorPosition.X >= 0 &&
        ScreenBuffer->CursorPosition.X < ScreenBuffer->VtState.TabStopLength)
    {
        ScreenBuffer->VtState.TabStops[ScreenBuffer->CursorPosition.X] = 1;
    }
}

VOID
NTAPI
VtHandleTabClear(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                 ULONG Mode)
{
    if (!ScreenBuffer)
        return;

    if (!VtEnsureTabStops(ScreenBuffer))
        return;

    switch (Mode)
    {
        case 0:
        case 2:
        default:
            if (ScreenBuffer->CursorPosition.X >= 0 &&
                ScreenBuffer->CursorPosition.X < ScreenBuffer->VtState.TabStopLength)
            {
                ScreenBuffer->VtState.TabStops[ScreenBuffer->CursorPosition.X] = 0;
            }
            break;

        case 3:
            RtlZeroMemory(ScreenBuffer->VtState.TabStops,
                          ScreenBuffer->VtState.TabStopLength * sizeof(UCHAR));
            break;
    }
}

SHORT
NTAPI
VtFindNextTabStop(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                  SHORT StartColumn)
{
    SHORT Width;
    SHORT Column;

    if (!ScreenBuffer || ScreenBuffer->ScreenBufferSize.X <= 0)
        return 0;

    Width = ScreenBuffer->ScreenBufferSize.X;

    if (!VtEnsureTabStops(ScreenBuffer))
    {
        SHORT Fallback = ((StartColumn + 1 + (VT_DEFAULT_TAB_WIDTH - 1)) / VT_DEFAULT_TAB_WIDTH) * VT_DEFAULT_TAB_WIDTH;
        if (Fallback <= StartColumn)
            Fallback = StartColumn + 1;
        return (Fallback < Width) ? Fallback : Width;
    }

    if (StartColumn < -1)
        StartColumn = -1;

    for (Column = StartColumn + 1; Column < ScreenBuffer->VtState.TabStopLength; ++Column)
    {
        if (ScreenBuffer->VtState.TabStops[Column])
            return Column;
    }

    /* No configured tab stop found, fall back to the next multiple of the default width. */
    Column = ((StartColumn + 1 + (VT_DEFAULT_TAB_WIDTH - 1)) / VT_DEFAULT_TAB_WIDTH) * VT_DEFAULT_TAB_WIDTH;
    if (Column <= StartColumn)
        Column = StartColumn + 1;
    return (Column < Width) ? Column : Width;
}

/* Column of the tab stop at or before StartColumn, or 0 when there is none */
SHORT
NTAPI
VtFindPrevTabStop(PTEXTMODE_SCREEN_BUFFER ScreenBuffer, SHORT StartColumn)
{
    SHORT Column;

    if (!ScreenBuffer || ScreenBuffer->ScreenBufferSize.X <= 0 || StartColumn <= 0)
        return 0;

    if (StartColumn > ScreenBuffer->ScreenBufferSize.X)
        StartColumn = ScreenBuffer->ScreenBufferSize.X;

    if (VtEnsureTabStops(ScreenBuffer))
    {
        for (Column = min(StartColumn, (SHORT)ScreenBuffer->VtState.TabStopLength) - 1; Column > 0; --Column)
        {
            if (ScreenBuffer->VtState.TabStops[Column])
                return Column;
        }
        return 0;
    }

    /* No bitmap available: fall back to the previous multiple of the default width */
    Column = (SHORT)(((StartColumn - 1) / VT_DEFAULT_TAB_WIDTH) * VT_DEFAULT_TAB_WIDTH);
    return (Column > 0) ? Column : 0;
}

/* CHT - move the cursor forward over Count tab stops */
static VOID
VtTabForward(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer, ULONG Count)
{
    COORD Position;
    ULONG i;

    if (ScreenBuffer->ScreenBufferSize.X <= 0)
        return;

    Position = ScreenBuffer->CursorPosition;
    for (i = 0; i < Count; ++i)
    {
        SHORT Next = VtFindNextTabStop(ScreenBuffer, Position.X);

        if (Next >= ScreenBuffer->ScreenBufferSize.X)
        {
            Position.X = ScreenBuffer->ScreenBufferSize.X - 1;
            break;
        }
        Position.X = Next;
    }

    ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Position);
}

/* CBT - move the cursor back over Count tab stops */
static VOID
VtTabBackward(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer, ULONG Count)
{
    COORD Position;
    ULONG i;

    if (ScreenBuffer->ScreenBufferSize.X <= 0)
        return;

    Position = ScreenBuffer->CursorPosition;
    for (i = 0; i < Count && Position.X > 0; ++i)
        Position.X = VtFindPrevTabStop(ScreenBuffer, Position.X);

    ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Position);
}

static VOID
VtHandleDcsSequence(PCONSOLE Console,
                    PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                    const WCHAR *Sequence,
                    ULONG Length)
{
    PTEXTMODE_SCREEN_BUFFER Target;

    if (!Console)
        return;

    if (!Console->AllowVtDcsPassthrough)
    {
        VtTracePolicyBlocked("VT DCS passthrough");
        return;
    }

    if (!Sequence || Length == 0)
        return;

    /* tmux multiplexes escape sequences via DCS "tmux;" wrappers. */
    if (Length >= 5 &&
        Sequence[0] == L't' && Sequence[1] == L'm' &&
        Sequence[2] == L'u' && Sequence[3] == L'x' &&
        Sequence[4] == L';')
    {
        const WCHAR *Payload = Sequence + 5;
        ULONG PayloadLength = Length - 5;
        ULONG Processed = 0;
        BOOLEAN Handled = FALSE;
        NTSTATUS Status;

        if (PayloadLength == 0)
            return;

        Target = (PTEXTMODE_SCREEN_BUFFER)Console->ActiveBuffer;
        if (!Target)
            Target = ScreenBuffer;

        if (!Target)
            return;

        Status = ConDrvVtWriteConsole(Console,
                                      Target,
                                      Payload,
                                      PayloadLength,
                                      &Processed,
                                      &Handled);
        if (!NT_SUCCESS(Status))
            VtTraceDcsReplayFailure(Status);
        return;
    }

    VtTraceUnhandledDcs();
    return;
}

static WCHAR
VtTranslateUsingTable(WCHAR Ch,
                      const VT_CHARSET_OVERRIDE *Table,
                      size_t Count)
{
    if (Ch < 0x20 || Ch > 0x7F || Table == NULL || Count == 0)
        return Ch;

    for (size_t i = 0; i < Count; ++i)
    {
        if (Table[i].Source == Ch)
            return Table[i].Target;
    }

    return Ch;
}

static VT_CHARSET_ID
VtGetCharsetSlot(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                 UCHAR Slot)
{
    if (Slot >= ARRAYSIZE(ScreenBuffer->VtState.Charsets))
        return VtCharsetAscii;

    return (VT_CHARSET_ID)ScreenBuffer->VtState.Charsets[Slot];
}

static VT_CHARSET_ID
VtGetActiveCharset(PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    return VtGetCharsetSlot(ScreenBuffer, ScreenBuffer->VtState.ActiveCharset);
}

static VOID
VtSetActiveCharset(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                   UCHAR Slot)
{
    if (Slot > VT_CHARSET_SLOT_G3)
        Slot = VT_CHARSET_SLOT_G0;

    ScreenBuffer->VtState.ActiveCharset = Slot;
}

static BOOLEAN
VtDesignateCharset(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                   UCHAR Slot,
                   WCHAR Intermediate,
                   WCHAR Designator)
{
    VT_CHARSET_ID Charset = VtCharsetAscii;
    BOOLEAN Logged = FALSE;

    if (Intermediate == L'%')
    {
        switch (Designator)
        {
            case L'6':
                Charset = VtCharsetPortuguese;
                break;

            default:
                Logged = TRUE;
                Charset = VtCharsetAscii;
                break;
        }
    }
    else
    {
        switch (Designator)
        {
            case L'0':
                Charset = VtCharsetDecSpecial;
                break;

            case L'A':
                Charset = VtCharsetUnitedKingdom;
                break;

            case L'4':
                Charset = VtCharsetDutch;
                break;

            case L'5':
            case L'C':
                Charset = VtCharsetFinnish;
                break;

            case L'R':
            case L'f':
                Charset = VtCharsetFrench;
                break;

            case L'Q':
            case L'9':
                Charset = VtCharsetFrenchCanadian;
                break;

            case L'K':
                Charset = VtCharsetGerman;
                break;

            case L'Y':
                Charset = VtCharsetItalian;
                break;

            case L'E':
            case L'6':
            case 0x0060: /* ` */
                Charset = VtCharsetNorwegianDanish;
                break;

            case L'Z':
                Charset = VtCharsetSpanish;
                break;

            case L'7':
            case L'H':
                Charset = VtCharsetSwedish;
                break;

            case L'=':
                Charset = VtCharsetSwiss;
                break;

            case L'>':
                Charset = VtCharsetDecTechnical;
                break;

            case L'B':
            case L'U':
            default:
                if (Designator != L'B' && Designator != L'U')
                    Logged = TRUE;
                Charset = VtCharsetAscii;
                break;
        }
    }

#ifdef DBG
    if (Logged)
    {
        static ULONG LoggedCount;
        if (LoggedCount < VT_TRACE_LIMIT)
        {
            LoggedCount++;
            DPRINT1("VT: Unsupported charset designator ESC %c %lc (slot %u)\n",
                    (Intermediate ? Intermediate : L'(' + Slot), Designator, Slot);
        }
    }
#endif

    if (Slot >= ARRAYSIZE(ScreenBuffer->VtState.Charsets))
        return FALSE;

    ScreenBuffer->VtState.Charsets[Slot] = (UCHAR)Charset;
    return TRUE;
}

static WCHAR
VtTranslateGlyphSlot(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                     UCHAR Slot,
                     WCHAR Ch)
{
    VT_CHARSET_ID Charset = VtGetCharsetSlot(ScreenBuffer, Slot);

    if (Charset == VtCharsetDecSpecial)
    {
        if (Ch < VT_DEC_SPECIAL_START ||
            Ch >= (VT_DEC_SPECIAL_START + VT_DEC_SPECIAL_COUNT))
        {
            return Ch;
        }

        return VtDecSpecialMap[Ch - VT_DEC_SPECIAL_START];
    }

    if (Ch < 0x20 || Ch > 0x7F)
        return Ch;

    /*
     * The NRCS tables are indexed by VT_CHARSET_ID, so adding a character set
     * means adding its table to this one array - not editing a switch as well.
     */
    static const struct
    {
        const VT_CHARSET_OVERRIDE *Table;
        USHORT Count;
    } NrcsTables[] =
    {
        {NULL, 0},                                                                  /* VtCharsetAscii */
        {NULL, 0},                                                                  /* VtCharsetDecSpecial, handled above */
        {VtNrcsBritishOverrides, RTL_NUMBER_OF_V1(VtNrcsBritishOverrides)},
        {VtNrcsDutchOverrides, RTL_NUMBER_OF_V1(VtNrcsDutchOverrides)},
        {VtNrcsFinnishOverrides, RTL_NUMBER_OF_V1(VtNrcsFinnishOverrides)},
        {VtNrcsFrenchOverrides, RTL_NUMBER_OF_V1(VtNrcsFrenchOverrides)},
        {VtNrcsFrenchCanadianOverrides, RTL_NUMBER_OF_V1(VtNrcsFrenchCanadianOverrides)},
        {VtNrcsGermanOverrides, RTL_NUMBER_OF_V1(VtNrcsGermanOverrides)},
        {VtNrcsItalianOverrides, RTL_NUMBER_OF_V1(VtNrcsItalianOverrides)},
        {VtNrcsNorwegianDanishOverrides, RTL_NUMBER_OF_V1(VtNrcsNorwegianDanishOverrides)},
        {VtNrcsSpanishOverrides, RTL_NUMBER_OF_V1(VtNrcsSpanishOverrides)},
        {VtNrcsSwedishOverrides, RTL_NUMBER_OF_V1(VtNrcsSwedishOverrides)},
        {VtNrcsSwissOverrides, RTL_NUMBER_OF_V1(VtNrcsSwissOverrides)},
        {VtNrcsPortugueseOverrides, RTL_NUMBER_OF_V1(VtNrcsPortugueseOverrides)},
        {VtDecTechnicalOverrides, RTL_NUMBER_OF_V1(VtDecTechnicalOverrides)},
    };

    if ((ULONG)Charset >= ARRAYSIZE(NrcsTables) || NrcsTables[Charset].Table == NULL)
        return Ch;

    return VtTranslateUsingTable(Ch, NrcsTables[Charset].Table, NrcsTables[Charset].Count);
}

static VOID
VtSaveCursorState(PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    ScreenBuffer->VtState.SavedCursorPos = ScreenBuffer->CursorPosition;
    ScreenBuffer->VtState.SavedAttributes = ScreenBuffer->VtState.CurrentAttributes;
    ScreenBuffer->VtState.SavedFgColor = ScreenBuffer->VtState.CurrentFgColor;
    ScreenBuffer->VtState.SavedBgColor = ScreenBuffer->VtState.CurrentBgColor;
    ScreenBuffer->VtState.SavedUseRgbForeground = ScreenBuffer->VtState.UseRgbForeground;
    ScreenBuffer->VtState.SavedUseRgbBackground = ScreenBuffer->VtState.UseRgbBackground;
    RtlCopyMemory(ScreenBuffer->VtState.SavedCharsets, ScreenBuffer->VtState.Charsets, sizeof(ScreenBuffer->VtState.Charsets));
    ScreenBuffer->VtState.SavedActiveCharset = ScreenBuffer->VtState.ActiveCharset;
    ScreenBuffer->VtState.LastCharValid = FALSE;
    ScreenBuffer->VtState.LastWrittenChar = L' ';
    ScreenBuffer->VtState.CursorSaved = TRUE;
}

static VOID
VtRestoreCursorState(PCONSOLE Console,
                     PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    if (!ScreenBuffer->VtState.CursorSaved)
        return;

    ConDrvSetConsoleCursorPosition(Console,
                                   ScreenBuffer,
                                   &ScreenBuffer->VtState.SavedCursorPos);
    ScreenBuffer->VtState.CurrentAttributes = ScreenBuffer->VtState.SavedAttributes;
    ScreenBuffer->VtState.CurrentFgColor = ScreenBuffer->VtState.SavedFgColor;
    ScreenBuffer->VtState.CurrentBgColor = ScreenBuffer->VtState.SavedBgColor;
    ScreenBuffer->VtState.UseRgbForeground = ScreenBuffer->VtState.SavedUseRgbForeground;
    ScreenBuffer->VtState.UseRgbBackground = ScreenBuffer->VtState.SavedUseRgbBackground;
    RtlCopyMemory(ScreenBuffer->VtState.Charsets, ScreenBuffer->VtState.SavedCharsets, sizeof(ScreenBuffer->VtState.Charsets));
    VtSetActiveCharset(ScreenBuffer, ScreenBuffer->VtState.SavedActiveCharset);
    ScreenBuffer->VtState.PendingSingleShift = VT_CHARSET_SLOT_INVALID;
}

/*
 * DECSET/DECRST 1047 and 1049 - the alternate screen.
 *
 * Implemented as a content swap inside this same screen-buffer object: the
 * primary's cell array, extended colours, cursor, viewport and margins are
 * parked in VtState and a blank set is installed in their place. Nothing about
 * the object's identity changes, so Console->ActiveBuffer, every client handle
 * and the reference the current write holds all stay valid - which a second
 * screen-buffer object plus ConDrvSetConsoleActiveScreenBuffer did not.
 *
 * The dimensions are deliberately left alone. xterm gives the alternate screen
 * the visible area only; keeping the buffer size means the rows below the
 * viewport simply stay blank, and avoids a reallocation that would invalidate
 * the very pointers being parked.
 */
static BOOLEAN
VtEnableAlternateScreen(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    SIZE_T CellCount;
    PCHAR_INFO Blank;
    SHORT X, Y;

    if (ScreenBuffer->VtState.AlternateActive)
        return TRUE;

    CellCount = (SIZE_T)ScreenBuffer->ScreenBufferSize.X * ScreenBuffer->ScreenBufferSize.Y;
    if (CellCount == 0)
        return FALSE;

    Blank = ConsoleAllocHeap(0, CellCount * sizeof(CHAR_INFO));
    if (!Blank)
        return FALSE;

    /* Park the primary screen */
    ScreenBuffer->VtState.SavedBuffer = ScreenBuffer->Buffer;
    ScreenBuffer->VtState.SavedCellRgb = ScreenBuffer->CellRgb;
    ScreenBuffer->VtState.SavedVirtualY = ScreenBuffer->VirtualY;
    ScreenBuffer->VtState.SavedScreenCursorPos = ScreenBuffer->CursorPosition;
    ScreenBuffer->VtState.SavedScreenCursorInfo = ScreenBuffer->CursorInfo;
    ScreenBuffer->VtState.SavedViewOrigin = ScreenBuffer->ViewOrigin;
    ScreenBuffer->VtState.SavedScrollTop = ScreenBuffer->VtState.ScrollTop;
    ScreenBuffer->VtState.SavedScrollBottom = ScreenBuffer->VtState.ScrollBottom;

    /* Install a blank screen. The colour array is dropped rather than copied:
     * it is rebuilt lazily the moment a 24-bit colour is written. */
    ScreenBuffer->Buffer = Blank;
    ScreenBuffer->CellRgb = NULL;
    ScreenBuffer->VirtualY = 0;
    ScreenBuffer->ViewOrigin.X = ScreenBuffer->ViewOrigin.Y = 0;
    ScreenBuffer->VtState.ScrollTop = 0;
    ScreenBuffer->VtState.ScrollBottom = max(0, ScreenBuffer->ScreenBufferSize.Y - 1);

    for (Y = 0; Y < ScreenBuffer->ScreenBufferSize.Y; ++Y)
    {
        PCHAR_INFO Row = ConioCoordToPointer(ScreenBuffer, 0, Y);

        for (X = 0; X < ScreenBuffer->ScreenBufferSize.X; ++X, ++Row)
        {
            Row->Char.UnicodeChar = L' ';
            Row->Attributes = ScreenBuffer->VtState.CurrentAttributes;
        }
    }

    ScreenBuffer->VtState.AlternateActive = TRUE;
    ScreenBuffer->VtState.PrivateModes |= VT_PRIVMODE_ALTERNATE_SCREEN_BUFFER;

    VtMarkDirty(ScreenBuffer, 0, 0, (SHORT)(ScreenBuffer->ScreenBufferSize.X - 1), (SHORT)(ScreenBuffer->ScreenBufferSize.Y - 1));

    {
        COORD Home = {0, 0};
        ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Home);
    }

    return TRUE;
}

static BOOLEAN
VtDisableAlternateScreen(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    if (!ScreenBuffer->VtState.AlternateActive)
        return TRUE;

    /* Discard the alternate screen and put the primary's contents back */
    ConsoleFreeHeap(ScreenBuffer->Buffer);
    if (ScreenBuffer->CellRgb)
        ConsoleFreeHeap(ScreenBuffer->CellRgb);

    ScreenBuffer->Buffer = ScreenBuffer->VtState.SavedBuffer;
    ScreenBuffer->CellRgb = ScreenBuffer->VtState.SavedCellRgb;
    ScreenBuffer->VirtualY = ScreenBuffer->VtState.SavedVirtualY;
    ScreenBuffer->ViewOrigin = ScreenBuffer->VtState.SavedViewOrigin;
    ScreenBuffer->CursorInfo = ScreenBuffer->VtState.SavedScreenCursorInfo;
    ScreenBuffer->VtState.ScrollTop = ScreenBuffer->VtState.SavedScrollTop;
    ScreenBuffer->VtState.ScrollBottom = ScreenBuffer->VtState.SavedScrollBottom;

    ScreenBuffer->VtState.SavedBuffer = NULL;
    ScreenBuffer->VtState.SavedCellRgb = NULL;
    ScreenBuffer->VtState.AlternateActive = FALSE;
    ScreenBuffer->VtState.PrivateModes &= ~VT_PRIVMODE_ALTERNATE_SCREEN_BUFFER;

    VtMarkDirty(ScreenBuffer, 0, 0, (SHORT)(ScreenBuffer->ScreenBufferSize.X - 1), (SHORT)(ScreenBuffer->ScreenBufferSize.Y - 1));

    ConDrvSetConsoleCursorInfo(Console, ScreenBuffer, &ScreenBuffer->CursorInfo);
    ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &ScreenBuffer->VtState.SavedScreenCursorPos);

    return TRUE;
}

/*
 * The 16-colour table belongs to the server layer, so reach it through the
 * TERMINAL vtable rather than casting Console to PCONSRV_CONSOLE.
 */
#define VT_PALETTE_SIZE 16

static BOOLEAN
VtReadPalette(PCONSOLE Console, COLORREF *Colors)
{
    if (!Console) return FALSE;
    return !!TermGetColorTable(Console, Colors, VT_PALETTE_SIZE);
}

static COLORREF
VtGetPaletteColor(PCONSOLE Console, UCHAR Index)
{
    COLORREF Colors[VT_PALETTE_SIZE];

    if (!VtReadPalette(Console, Colors)) return 0;
    return Colors[Index & 0x0F];
}

static VOID
VtClearHyperlink(PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    if (!ScreenBuffer)
        return;

    if (ScreenBuffer->VtState.HyperlinkUri.Buffer)
    {
        ConsoleFreeHeap(ScreenBuffer->VtState.HyperlinkUri.Buffer);
        ScreenBuffer->VtState.HyperlinkUri.Buffer = NULL;
    }

    ScreenBuffer->VtState.HyperlinkUri.Length = 0;
    ScreenBuffer->VtState.HyperlinkUri.MaximumLength = 0;
    ScreenBuffer->VtState.HyperlinkActive = FALSE;
}

VOID
NTAPI
ConDrvVtInvalidateBufferRgb(PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    SIZE_T CellCount;
    PCONSOLE Console;

    if (!ScreenBuffer)
        return;

    CellCount = (SIZE_T)ScreenBuffer->ScreenBufferSize.X * ScreenBuffer->ScreenBufferSize.Y;

    if (ScreenBuffer->CellRgb)
        RtlFillMemory(ScreenBuffer->CellRgb, CellCount * sizeof(CELL_RGB), 0xFF);

    ScreenBuffer->VtState.CursorSaved = FALSE;
    ScreenBuffer->VtState.UseRgbForeground = FALSE;
    ScreenBuffer->VtState.UseRgbBackground = FALSE;
    ScreenBuffer->VtState.SavedUseRgbForeground = FALSE;
    ScreenBuffer->VtState.SavedUseRgbBackground = FALSE;
    ScreenBuffer->VtState.PrivateModes = 0;
    ScreenBuffer->VtState.ScrollTop = 0;
    ScreenBuffer->VtState.ScrollBottom = max(0, ScreenBuffer->ScreenBufferSize.Y - 1);
    ScreenBuffer->VtState.PendingSequenceLength = 0;
    VtClearHyperlink(ScreenBuffer);

    Console = ScreenBuffer->Header.Console;
    if (Console)
    {
        COLORREF fg = VtGetPaletteColor(Console, ScreenBuffer->VtState.CurrentAttributes & 0x0F);
        COLORREF bg = VtGetPaletteColor(Console, (ScreenBuffer->VtState.CurrentAttributes >> 4) & 0x0F);
        ScreenBuffer->VtState.CurrentFgColor = fg;
        ScreenBuffer->VtState.CurrentBgColor = bg;
        ScreenBuffer->VtState.SavedFgColor = fg;
        ScreenBuffer->VtState.SavedBgColor = bg;
    }
}

static UCHAR
VtFindNearestPaletteIndex(PCONSOLE Console, COLORREF Color)
{
    COLORREF Colors[VT_PALETTE_SIZE];
    UCHAR best = 0;
    ULONG bestDiff = ULONG_MAX;

    if (!VtReadPalette(Console, Colors)) return 0;
    for (UCHAR idx = 0; idx < VT_PALETTE_SIZE; ++idx)
    {
        COLORREF ref = Colors[idx];
        LONG dr = (LONG)GetRValue(Color) - (LONG)GetRValue(ref);
        LONG dg = (LONG)GetGValue(Color) - (LONG)GetGValue(ref);
        LONG db = (LONG)GetBValue(Color) - (LONG)GetBValue(ref);
        ULONG diff = (ULONG)(dr * dr + dg * dg + db * db);
        if (diff < bestDiff)
        {
            bestDiff = diff;
            best = idx;
        }
    }
    return best;
}

static COLORREF
VtColorFromXtermIndex(ULONG Index)
{
    /* Indices below 16 are palette entries and never reach here - the caller
     * resolves those through the console attribute directly */
    if (Index >= 16 && Index <= 231)
    {
        Index -= 16;
        ULONG r = (Index / 36) % 6;
        ULONG g = (Index / 6) % 6;
        ULONG b = Index % 6;
        r = r ? r * 40 + 55 : 0;
        g = g ? g * 40 + 55 : 0;
        b = b ? b * 40 + 55 : 0;
        return RGB((BYTE)r, (BYTE)g, (BYTE)b);
    }

    if (Index >= 232 && Index <= 255)
    {
        BYTE shade = (BYTE)((Index - 232) * 10 + 8);
        return RGB(shade, shade, shade);
    }

    return 0;
}

static SIZE_T
VtAppendNumber(WCHAR *Buffer, SIZE_T BufferLength, ULONG Number)
{
    WCHAR Temp[12];
    SIZE_T Count = 0;
    SIZE_T Written = 0;

    if (BufferLength == 0)
        return 0;

    do
    {
        Temp[Count++] = (WCHAR)(L'0' + (Number % 10));
        Number /= 10;
    } while (Number != 0 && Count < ARRAYSIZE(Temp));

    while (Count > 0 && Written < BufferLength)
    {
        Buffer[Written++] = Temp[--Count];
    }

    return Written;
}

static USHORT
VtGetModifierParameter(const KEY_EVENT_RECORD *KeyEvent)
{
    USHORT Parameter = 1;

    if (KeyEvent->dwControlKeyState & SHIFT_PRESSED)
        Parameter += 1;
    if (KeyEvent->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
        Parameter += 2;
    if (KeyEvent->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
        Parameter += 4;

    return Parameter;
}

static BOOLEAN
VtComposeCsiTildeSequence(ULONG Parameter,
                          USHORT ModifierParam,
                          WCHAR Final,
                          WCHAR *Buffer,
                          SIZE_T Capacity,
                          SIZE_T *Length)
{
    SIZE_T Written = 0;

    if (Capacity < 3)
        return FALSE;

    Buffer[Written++] = L'\x1b';
    Buffer[Written++] = L'[';
    Written += VtAppendNumber(Buffer + Written,
                              Capacity - Written,
                              Parameter);

    if (ModifierParam > 1)
    {
        if (Written >= Capacity)
            return FALSE;
        Buffer[Written++] = L';';
        Written += VtAppendNumber(Buffer + Written,
                                  Capacity - Written,
                                  ModifierParam);
    }

    if (Written >= Capacity)
        return FALSE;

    Buffer[Written++] = Final;
    *Length = Written;
    return TRUE;
}

static BOOLEAN
VtComposeCursorKeySequence(BOOLEAN ApplicationMode,
                           const KEY_EVENT_RECORD *KeyEvent,
                           WCHAR Final,
                           WCHAR *Buffer,
                           SIZE_T Capacity,
                           SIZE_T *Length)
{
    SIZE_T Written = 0;
    USHORT ModifierParam = VtGetModifierParameter(KeyEvent);

    if (ApplicationMode && ModifierParam == 1)
    {
        if (Capacity < 3)
            return FALSE;
        Buffer[Written++] = L'\x1b';
        Buffer[Written++] = L'O';
        Buffer[Written++] = Final;
        *Length = Written;
        return TRUE;
    }

    if (Capacity < 3)
        return FALSE;

    Buffer[Written++] = L'\x1b';
    Buffer[Written++] = L'[';

    if (ModifierParam > 1)
    {
        Written += VtAppendNumber(Buffer + Written,
                                  Capacity - Written,
                                  1);
        if (Written >= Capacity)
            return FALSE;
        Buffer[Written++] = L';';
        Written += VtAppendNumber(Buffer + Written,
                                  Capacity - Written,
                                  ModifierParam);
    }

    if (Written >= Capacity)
        return FALSE;

    Buffer[Written++] = Final;
    *Length = Written;
    return TRUE;
}

static USHORT
VtMouseModifierBits(DWORD ControlKeyState)
{
    USHORT Bits = 0;

    if (ControlKeyState & SHIFT_PRESSED)
        Bits |= 4;
    if (ControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
        Bits |= 8;
    if (ControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
        Bits |= 16;

    return Bits;
}

static INT
VtFirstMouseButton(ULONG Mask)
{
    if (Mask & FROM_LEFT_1ST_BUTTON_PRESSED) return 0;
    if (Mask & FROM_LEFT_2ND_BUTTON_PRESSED) return 1;
    if (Mask & RIGHTMOST_BUTTON_PRESSED) return 2;
    if (Mask & FROM_LEFT_3RD_BUTTON_PRESSED) return 3;
    if (Mask & FROM_LEFT_4TH_BUTTON_PRESSED) return 4;
    return -1;
}

static BOOLEAN
VtComposeMouseSgrSequence(ULONG ButtonCode,
                          ULONG X,
                          ULONG Y,
                          BOOLEAN Released,
                          WCHAR *Buffer,
                          SIZE_T Capacity,
                          SIZE_T *Length)
{
    SIZE_T Written = 0;

    if (Capacity < 7)
        return FALSE;

    if (X == 0) X = 1;
    if (Y == 0) Y = 1;

    Buffer[Written++] = L'\x1b';
    Buffer[Written++] = L'[';
    Buffer[Written++] = L'<';
    Written += VtAppendNumber(Buffer + Written,
                              Capacity - Written,
                              ButtonCode);
    if (Written >= Capacity)
        return FALSE;
    Buffer[Written++] = L';';
    Written += VtAppendNumber(Buffer + Written,
                              Capacity - Written,
                              X);
    if (Written >= Capacity)
        return FALSE;
    Buffer[Written++] = L';';
    Written += VtAppendNumber(Buffer + Written,
                              Capacity - Written,
                              Y);
    if (Written >= Capacity)
        return FALSE;
    Buffer[Written++] = Released ? L'm' : L'M';
    *Length = Written;
    return TRUE;
}

static BOOLEAN
VtComposeMouseLegacySequence(ULONG ButtonCode,
                             ULONG X,
                             ULONG Y,
                             WCHAR *Buffer,
                             SIZE_T Capacity,
                             SIZE_T *Length)
{
    ULONG AdjustedX;
    ULONG AdjustedY;

    if (Capacity < 6)
        return FALSE;

    if (X < 1) X = 1;
    if (Y < 1) Y = 1;

    AdjustedX = min(223u, X);
    AdjustedY = min(223u, Y);

    Buffer[0] = L'\x1b';
    Buffer[1] = L'[';
    Buffer[2] = L'M';
    Buffer[3] = (WCHAR)((min(ButtonCode, 255u)) + 32u);
    Buffer[4] = (WCHAR)(AdjustedX + 32u);
    Buffer[5] = (WCHAR)(AdjustedY + 32u);
    *Length = 6;
    return TRUE;
}

static VOID
VtSendInputResponse(PCONSOLE Console,
                    const WCHAR *Response,
                    SIZE_T Length)
{
    /* Every caller replies out of a WCHAR[VT_MAX_INPUT_SEQUENCE_CHARS], so the
     * stack covers all of them and the heap path is only a safety net */
    INPUT_RECORD Stack[VT_MAX_INPUT_SEQUENCE_CHARS * 2];
    PINPUT_RECORD Records = Stack;
    SIZE_T Index;
    ULONG Total;

    if (Console == NULL || Response == NULL || Length == 0)
        return;

    Total = (ULONG)(Length * 2);
    if (Total > ARRAYSIZE(Stack))
    {
        Records = ConsoleAllocHeap(0, Total * sizeof(INPUT_RECORD));
        if (!Records)
            return;
    }

    for (Index = 0; Index < Length; ++Index)
    {
        VtFillKeyEvent(&Records[Index * 2], Response[Index], TRUE);
        VtFillKeyEvent(&Records[Index * 2 + 1], Response[Index], FALSE);
    }

    ConDrvWriteConsoleInput(Console,
                            &Console->InputBuffer,
                            TRUE,
                            Records,
                            Total,
                            NULL);

    if (Records != Stack) ConsoleFreeHeap(Records);
}

static BOOLEAN
VtStorePendingSequence(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                       const WCHAR *Sequence,
                       ULONG Length)
{
    if (!ScreenBuffer)
        return FALSE;

    if (!Sequence || Length == 0)
    {
        ScreenBuffer->VtState.PendingSequenceLength = 0;
        return TRUE;
    }

    if (Length > ARRAYSIZE(ScreenBuffer->VtState.PendingSequence))
        return FALSE;

    RtlCopyMemory(ScreenBuffer->VtState.PendingSequence,
                  Sequence,
                  Length * sizeof(WCHAR));
    ScreenBuffer->VtState.PendingSequenceLength = (USHORT)Length;
    return TRUE;
}

static SIZE_T
VtAppendHex(WCHAR *Buffer, SIZE_T BufferLength, ULONG Value, ULONG Digits)
{
    SIZE_T Written = 0;
    LONG Shift;

    if (Digits == 0 || BufferLength == 0)
        return 0;

    for (Shift = (LONG)(Digits - 1); Shift >= 0 && Written < BufferLength; --Shift)
    {
        ULONG Nibble = (Value >> (Shift * 4)) & 0xF;
        Buffer[Written++] = (WCHAR)(Nibble < 10 ? (L'0' + Nibble) : (L'A' + (Nibble - 10)));
    }

    return Written;
}

static SIZE_T
VtAppendOscRgb(WCHAR *Buffer, SIZE_T BufferLength, COLORREF Color)
{
    SIZE_T Written = 0;
    ULONG Components[3];
    ULONG Index;

    if (BufferLength < 4)
        return 0;

    Buffer[Written++] = L'r';
    Buffer[Written++] = L'g';
    Buffer[Written++] = L'b';
    Buffer[Written++] = L':';

    Components[0] = (ULONG)GetRValue(Color) * 257u;
    Components[1] = (ULONG)GetGValue(Color) * 257u;
    Components[2] = (ULONG)GetBValue(Color) * 257u;

    for (Index = 0; Index < ARRAYSIZE(Components) && Written < BufferLength; ++Index)
    {
        Written += VtAppendHex(Buffer + Written,
                               BufferLength - Written,
                               Components[Index],
                               4);

        if (Index + 1 < ARRAYSIZE(Components) && Written < BufferLength)
            Buffer[Written++] = L'/';
    }

    return Written;
}

static INT
VtBase64Value(WCHAR Ch)
{
    if (Ch >= L'A' && Ch <= L'Z') return (INT)(Ch - L'A');
    if (Ch >= L'a' && Ch <= L'z') return (INT)(Ch - L'a') + 26;
    if (Ch >= L'0' && Ch <= L'9') return (INT)(Ch - L'0') + 52;
    if (Ch == L'+') return 62;
    if (Ch == L'/') return 63;
    return -1;
}

static BOOLEAN
VtDecodeBase64(const WCHAR *Input,
               ULONG Length,
               PBYTE *Output,
               PULONG OutputSize)
{
    ULONG Estimate;
    PBYTE Buffer;
    ULONG OutIndex = 0;
    ULONG Quad[4];
    ULONG QuadCount = 0;
    ULONG PadCount = 0;

    if (!Output || !OutputSize)
        return FALSE;

    Estimate = ((Length + 3) / 4) * 3;
    Buffer = ConsoleAllocHeap(0, Estimate ? Estimate : 1);
    if (!Buffer)
        return FALSE;

    for (ULONG i = 0; i < Length; ++i)
    {
        WCHAR Ch = Input[i];

        if (Ch == L'\r' || Ch == L'\n' || Ch == L'	' || Ch == L' ')
            continue;

        if (Ch == L'=')
        {
            Quad[QuadCount++] = 0;
            PadCount++;
        }
        else
        {
            INT Value = VtBase64Value(Ch);
            if (Value < 0)
            {
                ConsoleFreeHeap(Buffer);
                return FALSE;
            }

            Quad[QuadCount++] = (ULONG)Value;
        }

        if (QuadCount == 4)
        {
            Buffer[OutIndex++] = (BYTE)((Quad[0] << 2) | (Quad[1] >> 4));
            if (PadCount < 2)
                Buffer[OutIndex++] = (BYTE)(((Quad[1] & 0x0F) << 4) | (Quad[2] >> 2));
            if (PadCount < 1)
                Buffer[OutIndex++] = (BYTE)(((Quad[2] & 0x03) << 6) | Quad[3]);

            QuadCount = 0;
            PadCount = 0;
        }
    }

    if (QuadCount != 0)
    {
        ConsoleFreeHeap(Buffer);
        return FALSE;
    }

    *Output = Buffer;
    *OutputSize = OutIndex;
    return TRUE;
}

static BOOLEAN
VtEncodeBase64(const BYTE *Input,
               ULONG Length,
               PWSTR *Output,
               PULONG OutputChars)
{
    static const WCHAR Table[] =
        L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    ULONG OutLen;
    PWSTR Buffer;
    ULONG Index = 0;

    if (!Output || !OutputChars)
        return FALSE;

    OutLen = ((Length + 2) / 3) * 4;
    Buffer = ConsoleAllocHeap(0, (OutLen + 1) * sizeof(WCHAR));
    if (!Buffer)
        return FALSE;

    for (ULONG i = 0; i < Length; i += 3)
    {
        BYTE b0 = Input[i];
        BYTE b1 = (i + 1 < Length) ? Input[i + 1] : 0;
        BYTE b2 = (i + 2 < Length) ? Input[i + 2] : 0;

        Buffer[Index++] = Table[(b0 >> 2) & 0x3F];
        Buffer[Index++] = Table[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)];
        Buffer[Index++] = (i + 1 < Length) ? Table[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : L'=';
        Buffer[Index++] = (i + 2 < Length) ? Table[b2 & 0x3F] : L'=';
    }

    Buffer[Index] = UNICODE_NULL;
    *Output = Buffer;
    *OutputChars = Index;
    return TRUE;
}

static VOID
VtSendOscClipboardResponse(PCONSOLE Console,
                           const WCHAR *TargetStart,
                           SIZE_T TargetLength,
                           const WCHAR *Payload)
{
    WCHAR StaticTarget = L'c';
    const WCHAR *Target = TargetStart;
    SIZE_T Length = TargetLength;
    SIZE_T PayloadLength = Payload ? wcslen(Payload) : 0;
    PWSTR Response;
    SIZE_T Pos = 0;

    if (!Target || Length == 0)
    {
        Target = &StaticTarget;
        Length = 1;
    }

    Response = ConsoleAllocHeap(0, (5 + Length + PayloadLength + 2) * sizeof(WCHAR));
    if (!Response)
        return;

    Response[Pos++] = L'';
    Response[Pos++] = L']';
    Response[Pos++] = L'5';
    Response[Pos++] = L'2';
    Response[Pos++] = L';';

    for (SIZE_T i = 0; i < Length; ++i)
        Response[Pos++] = Target[i];

    Response[Pos++] = L';';

    for (SIZE_T i = 0; i < PayloadLength; ++i)
        Response[Pos++] = Payload[i];

    Response[Pos++] = L'';

    VtSendInputResponse(Console, Response, Pos);

    ConsoleFreeHeap(Response);
}


/*
 * OSC colour report: ESC ] <Ps> [ ; <Index> ] ; <rgb> BEL
 *
 * OSC 4 reports a palette entry and carries its index as a second parameter;
 * OSC 10/11 report the default fore/background and carry none.
 */
static VOID
VtSendOscColorReply(PCONSOLE Console, ULONG Ps, BOOLEAN HasIndex, ULONG Index, COLORREF Color)
{
    WCHAR Response[80];
    SIZE_T Len = 0;

    Response[Len++] = L'';
    Response[Len++] = L']';
    Len += VtAppendNumber(Response + Len, ARRAYSIZE(Response) - Len, Ps);

    if (HasIndex)
    {
        if (Len < ARRAYSIZE(Response))
            Response[Len++] = L';';
        Len += VtAppendNumber(Response + Len, ARRAYSIZE(Response) - Len, Index);
    }

    if (Len < ARRAYSIZE(Response))
        Response[Len++] = L';';
    Len += VtAppendOscRgb(Response + Len, ARRAYSIZE(Response) - Len, Color);
    if (Len < ARRAYSIZE(Response))
        Response[Len++] = L'';

    VtSendInputResponse(Console, Response, Len);
}


/*
 * Invalidation accumulator.
 *
 * Every VT operation used to call TermDrawRegion itself, so a single frame from
 * a full-screen application cost one win32k InvalidateRect per escape sequence.
 * Repainting is asynchronous and always reads the buffer as it stands when the
 * paint happens, so the individual regions can be unioned and issued once.
 *
 * The rectangle lives on the screen buffer, so it must be flushed before the
 * console switches active buffer - otherwise it would be left pending on a
 * buffer nobody is painting.
 */
static VOID
VtMarkDirty(PTEXTMODE_SCREEN_BUFFER ScreenBuffer, SHORT Left, SHORT Top, SHORT Right, SHORT Bottom)
{
    PSMALL_RECT Dirty = &ScreenBuffer->VtState.DirtyRect;

    if (Left > Right || Top > Bottom)
        return;

    if (!ScreenBuffer->VtState.DirtyValid)
    {
        Dirty->Left = Left;
        Dirty->Top = Top;
        Dirty->Right = Right;
        Dirty->Bottom = Bottom;
        ScreenBuffer->VtState.DirtyValid = TRUE;
        return;
    }

    Dirty->Left = min(Dirty->Left, Left);
    Dirty->Top = min(Dirty->Top, Top);
    Dirty->Right = max(Dirty->Right, Right);
    Dirty->Bottom = max(Dirty->Bottom, Bottom);
}

static VOID
VtMarkDirtyRect(PTEXTMODE_SCREEN_BUFFER ScreenBuffer, const SMALL_RECT *Region)
{
    VtMarkDirty(ScreenBuffer, Region->Left, Region->Top, Region->Right, Region->Bottom);
}

static VOID
VtFlushDirty(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    if (!ScreenBuffer || !ScreenBuffer->VtState.DirtyValid)
        return;

    ScreenBuffer->VtState.DirtyValid = FALSE;
    TermDrawRegion(Console, &ScreenBuffer->VtState.DirtyRect);
}

/*
 * Apply the current SGR state to a rectangle, clipped to the buffer.
 *
 * FillChar == 0 refreshes only the extended colours, leaving the characters and
 * attributes alone - that is what the scrolling paths need after cells have been
 * moved about. Any other value also overwrites every cell's glyph and attribute.
 */
static VOID
VtFillRect(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
           SHORT Left,
           SHORT Top,
           SHORT Right,
           SHORT Bottom,
           WCHAR FillChar)
{
    SHORT Width = ScreenBuffer->ScreenBufferSize.X;
    SHORT Height = ScreenBuffer->ScreenBufferSize.Y;
    SHORT X, Y;
    USHORT Attr = ScreenBuffer->VtState.CurrentAttributes;
    COLORREF Fg = ScreenBuffer->VtState.UseRgbForeground ? ScreenBuffer->VtState.CurrentFgColor : CLR_INVALID;
    COLORREF Bg = ScreenBuffer->VtState.UseRgbBackground ? ScreenBuffer->VtState.CurrentBgColor : CLR_INVALID;

    if (Width <= 0 || Height <= 0)
        return;

    Left = max(Left, 0);
    Top = max(Top, 0);
    Right = min(Right, (SHORT)(Width - 1));
    Bottom = min(Bottom, (SHORT)(Height - 1));

    if (Left > Right || Top > Bottom)
        return;

    if (Fg != CLR_INVALID || Bg != CLR_INVALID)
        ConioEnsureCellColors(ScreenBuffer);

    for (Y = Top; Y <= Bottom; ++Y)
    {
        if (FillChar)
        {
            PCHAR_INFO Ptr = ConioCoordToPointer(ScreenBuffer, Left, Y);
            for (X = Left; X <= Right; ++X, ++Ptr)
            {
                Ptr->Char.UnicodeChar = FillChar;
                Ptr->Attributes = Attr;
            }
        }
        ConioFillCellColors(ScreenBuffer, Left, Y, Right + 1 - Left, Fg, Bg);
    }
}

/*
 * ICH / DCH - shift the tail of the cursor's row.
 *
 * Delta > 0 inserts that many blanks at the cursor, pushing the tail right and
 * dropping whatever falls off the end. Delta < 0 deletes that many cells,
 * pulling the tail left and blanking the columns vacated at the right edge.
 * They are one operation with source and destination swapped and the fill at
 * the other end.
 */
static VOID
VtShiftRow(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer, LONG Delta)
{
    SHORT Width = ScreenBuffer->ScreenBufferSize.X;
    SHORT Y = ScreenBuffer->CursorPosition.Y;
    SHORT X = ScreenBuffer->CursorPosition.X;
    BOOLEAN Insert = (Delta > 0);
    ULONG Available;
    ULONG Count;
    ULONG CellsToMove;
    SMALL_RECT Region;

    if (Delta == 0 || Width <= 0)
        return;

    if (Y < 0 || Y >= ScreenBuffer->ScreenBufferSize.Y)
        return;

    if (X < 0 || X >= Width)
        return;

    /* X < Width, so at least one cell is available */
    Available = (ULONG)(Width - X);
    Count = (ULONG)(Insert ? Delta : -Delta);
    if (Count > Available)
        Count = Available;

    CellsToMove = Available - Count;
    if (CellsToMove > 0)
    {
        PCHAR_INFO Row = ConioCoordToPointer(ScreenBuffer, 0, Y);
        ULONG RowIndex = ConioCoordToIndex(ScreenBuffer, 0, Y);
        ULONG Dst = Insert ? (ULONG)X + Count : (ULONG)X;
        ULONG Src = Insert ? (ULONG)X : (ULONG)X + Count;

        RtlMoveMemory(Row + Dst, Row + Src, CellsToMove * sizeof(CHAR_INFO));
        ConioMoveCellColorsByIndex(ScreenBuffer, RowIndex + Dst, RowIndex + Src, CellsToMove);
    }

    if (Insert)
        VtFillRect(ScreenBuffer, X, Y, (SHORT)(X + Count - 1), Y, L' ');
    else
        VtFillRect(ScreenBuffer, (SHORT)(Width - Count), Y, (SHORT)(Width - 1), Y, L' ');

    Region.Left = X;
    Region.Top = Y;
    Region.Right = Width - 1;
    Region.Bottom = Y;
    VtMarkDirtyRect(ScreenBuffer, &Region);
}

static VOID
VtEraseCharacters(PCONSOLE Console,
                  PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                  ULONG Count)
{
    SHORT Width = ScreenBuffer->ScreenBufferSize.X;
    SHORT Y = ScreenBuffer->CursorPosition.Y;
    SHORT X = ScreenBuffer->CursorPosition.X;

    if (Count == 0)
        Count = 1;

    if (Width <= 0)
        return;

    if (Y < 0 || Y >= ScreenBuffer->ScreenBufferSize.Y)
        return;

    if (X < 0 || X >= Width)
        return;

    if ((ULONG)(Width - X) < Count)
        Count = (ULONG)(Width - X);

    if (Count == 0)
        return;

    {
        SHORT EraseCount = (SHORT)Count;
        VtFillRect(ScreenBuffer, X, Y, (SHORT)(X + EraseCount - 1), Y, L' ');

        {
            SMALL_RECT Region;
            Region.Left = X;
            Region.Top = Y;
            Region.Right = (SHORT)(X + EraseCount - 1);
            Region.Bottom = Y;
            VtMarkDirtyRect(ScreenBuffer, &Region);
        }
    }
}

static NTSTATUS
VtFlushText(PCONSOLE Console,
           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
           PCWSTR Text,
           ULONG Length);

static VOID
VtRepeatCharacter(PCONSOLE Console,
                  PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                  ULONG Count)
{
    WCHAR Chunk[64];
    ULONG Remaining;

    if (!Console || !ScreenBuffer)
        return;

    if (!ScreenBuffer->VtState.LastCharValid)
        return;

    if (Count == 0)
        Count = 1;

    Remaining = Count;

    while (Remaining > 0)
    {
        ULONG Emit = (Remaining > ARRAYSIZE(Chunk)) ? ARRAYSIZE(Chunk) : Remaining;
        ULONG i;

        for (i = 0; i < Emit; ++i)
            Chunk[i] = ScreenBuffer->VtState.LastWrittenChar;

        if (!NT_SUCCESS(VtFlushText(Console, ScreenBuffer, Chunk, Emit)))
            break;

        Remaining -= Emit;
    }
}

/*
 * IL / DL - shift lines within the scrolling region, from the cursor's line to
 * the bottom margin.
 *
 * Delta > 0 opens that many blank lines at the cursor, pushing the rest down and
 * off the bottom margin. Delta < 0 removes that many lines, pulling the rest up
 * and blanking the lines vacated at the bottom. One operation, opposite
 * direction: the copy walks the rows backwards when opening a gap and forwards
 * when closing one, so overlapping rows are never overwritten before use.
 */
static VOID
VtShiftLines(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer, LONG Delta)
{
    SHORT Width = ScreenBuffer->ScreenBufferSize.X;
    SHORT CursorY = ScreenBuffer->CursorPosition.Y;
    SHORT Top = ScreenBuffer->VtState.ScrollTop;
    SHORT Bottom = ScreenBuffer->VtState.ScrollBottom;
    BOOLEAN Insert = (Delta > 0);
    SHORT LinesAvailable;
    SHORT Shift;
    SHORT Line;
    SMALL_RECT Region;

    if (Delta == 0 || Width <= 0)
        return;

    if (Top < 0)
        Top = 0;
    if (Bottom >= ScreenBuffer->ScreenBufferSize.Y)
        Bottom = ScreenBuffer->ScreenBufferSize.Y - 1;

    if (CursorY < Top || CursorY > Bottom)
        return;

    LinesAvailable = Bottom - CursorY + 1;
    if (LinesAvailable <= 0)
        return;

    Shift = (SHORT)(Insert ? Delta : -Delta);
    if (Shift > LinesAvailable)
        Shift = LinesAvailable;

    if (Shift < LinesAvailable)
    {
        if (Insert)
        {
            for (Line = Bottom - Shift; Line >= CursorY; --Line)
            {
                PCHAR_INFO Src = ConioCoordToPointer(ScreenBuffer, 0, Line);
                PCHAR_INFO Dst = ConioCoordToPointer(ScreenBuffer, 0, Line + Shift);

                RtlMoveMemory(Dst, Src, Width * sizeof(CHAR_INFO));
                ConioMoveCellColors(ScreenBuffer, 0, Line + Shift, 0, Line, Width);
            }
        }
        else
        {
            for (Line = CursorY; Line <= Bottom - Shift; ++Line)
            {
                PCHAR_INFO Src = ConioCoordToPointer(ScreenBuffer, 0, Line + Shift);
                PCHAR_INFO Dst = ConioCoordToPointer(ScreenBuffer, 0, Line);

                RtlMoveMemory(Dst, Src, Width * sizeof(CHAR_INFO));
                ConioMoveCellColors(ScreenBuffer, 0, Line, 0, Line + Shift, Width);
            }
        }
    }

    /* Blank the lines the shift vacated: at the cursor, or at the bottom margin */
    if (Insert)
        VtFillRect(ScreenBuffer, 0, CursorY, (SHORT)(Width - 1), (SHORT)(CursorY + Shift - 1), L' ');
    else
        VtFillRect(ScreenBuffer, 0, (SHORT)(Bottom - Shift + 1), (SHORT)(Width - 1), Bottom, L' ');

    Region.Left = 0;
    Region.Right = Width - 1;
    Region.Top = CursorY;
    Region.Bottom = Bottom;
    VtMarkDirtyRect(ScreenBuffer, &Region);
}

/*
 * SU / SD - scroll the whole scrolling region. Delta > 0 scrolls content up (the
 * top lines leave, blanks appear at the bottom margin); Delta < 0 scrolls it
 * down. ConDrvScrollConsoleScreenBuffer performs the move and the attribute
 * fill; the shared fill path leaves extended colours at CLR_INVALID, so
 * VtFillRect then re-applies the current SGR colours to the vacated band.
 */
static VOID
VtScrollRegion(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer, LONG Delta)
{
    SHORT Top = ScreenBuffer->VtState.ScrollTop;
    SHORT Bottom = ScreenBuffer->VtState.ScrollBottom;
    SHORT Height = Bottom - Top + 1;
    SHORT Right = ScreenBuffer->ScreenBufferSize.X - 1;
    BOOLEAN Up = (Delta > 0);
    SHORT Lines;
    SMALL_RECT ScrollRect;
    COORD DestOrigin;
    CHAR_INFO FillChar;

    if (Delta == 0 || Height <= 0)
        return;

    Lines = (SHORT)(Up ? Delta : -Delta);
    if (Lines > Height)
        Lines = Height;

    ScrollRect.Left = 0;
    ScrollRect.Top = Top;
    ScrollRect.Right = Right;
    ScrollRect.Bottom = Bottom;

    DestOrigin.X = 0;
    DestOrigin.Y = (SHORT)(Up ? Top - Lines : Top + Lines);

    FillChar.Char.UnicodeChar = L' ';
    FillChar.Attributes = ScreenBuffer->VtState.CurrentAttributes;

    if (!NT_SUCCESS(ConDrvScrollConsoleScreenBuffer(Console, ScreenBuffer, TRUE, &ScrollRect, FALSE, NULL, &DestOrigin, FillChar)))
        return;

    if (Up)
        VtFillRect(ScreenBuffer, 0, (SHORT)(Bottom - Lines + 1), Right, Bottom, 0);
    else
        VtFillRect(ScreenBuffer, 0, Top, Right, (SHORT)(Top + Lines - 1), 0);
}

VOID
NTAPI
ConDrvVtAdvanceLine(PCONSOLE Console,
                    PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    SHORT Bottom;

    if (!Console || !ScreenBuffer || ScreenBuffer->ScreenBufferSize.Y <= 0)
        return;

    Bottom = ScreenBuffer->ScreenBufferSize.Y - 1;

    if (ScreenBuffer->VtState.ScrollTop >= 0 &&
        ScreenBuffer->VtState.ScrollBottom >= ScreenBuffer->VtState.ScrollTop)
    {
        Bottom = min(Bottom, ScreenBuffer->VtState.ScrollBottom);
    }

    if (ScreenBuffer->CursorPosition.Y >= ScreenBuffer->VtState.ScrollTop &&
        ScreenBuffer->CursorPosition.Y == Bottom)
    {
        VtScrollRegion(Console, ScreenBuffer, 1);
        return;
    }

    if (ScreenBuffer->CursorPosition.Y < ScreenBuffer->ScreenBufferSize.Y - 1)
        ScreenBuffer->CursorPosition.Y++;
}

/*
 * SL / SR - shift every line of the scrolling region sideways. Delta > 0 shifts
 * content left (columns vacate at the right edge); Delta < 0 shifts it right
 * (columns vacate at the left edge). A shift at or beyond the buffer width
 * clears the line outright.
 */
static VOID
VtScrollColumns(PCONSOLE Console, PTEXTMODE_SCREEN_BUFFER ScreenBuffer, LONG Delta)
{
    SHORT Top = ScreenBuffer->VtState.ScrollTop;
    SHORT Bottom = ScreenBuffer->VtState.ScrollBottom;
    SHORT Width = ScreenBuffer->ScreenBufferSize.X;
    BOOLEAN Left = (Delta > 0);
    SHORT Shift;
    SHORT Line;
    SMALL_RECT Region;

    if (Delta == 0 || Width <= 0)
        return;

    Shift = (SHORT)(Left ? Delta : -Delta);
    if (Shift > Width)
        Shift = Width;

    if (Top < 0)
        Top = 0;
    if (Bottom >= ScreenBuffer->ScreenBufferSize.Y)
        Bottom = ScreenBuffer->ScreenBufferSize.Y - 1;
    if (Bottom < Top)
        return;

    for (Line = Top; Line <= Bottom; ++Line)
    {
        PCHAR_INFO Row = ConioCoordToPointer(ScreenBuffer, 0, Line);
        ULONG RowIndex = ConioCoordToIndex(ScreenBuffer, 0, Line);
        ULONG Keep = (ULONG)(Width - Shift);

        if (Shift >= Width)
        {
            VtFillRect(ScreenBuffer, 0, Line, (SHORT)(Width - 1), Line, L' ');
            continue;
        }

        if (Left)
        {
            RtlMoveMemory(Row, Row + Shift, Keep * sizeof(CHAR_INFO));
            ConioMoveCellColorsByIndex(ScreenBuffer, RowIndex, RowIndex + Shift, Keep);
            VtFillRect(ScreenBuffer, (SHORT)(Width - Shift), Line, (SHORT)(Width - 1), Line, L' ');
        }
        else
        {
            RtlMoveMemory(Row + Shift, Row, Keep * sizeof(CHAR_INFO));
            ConioMoveCellColorsByIndex(ScreenBuffer, RowIndex + Shift, RowIndex, Keep);
            VtFillRect(ScreenBuffer, 0, Line, (SHORT)(Shift - 1), Line, L' ');
        }
    }

    Region.Left = 0;
    Region.Right = Width - 1;
    Region.Top = Top;
    Region.Bottom = Bottom;
    VtMarkDirtyRect(ScreenBuffer, &Region);
}

static BOOLEAN
VtSetConsoleTitle(PCONSOLE Console,
                  const WCHAR *Title,
                  ULONG Length)
{
    if (!Console || !Title)
        return FALSE;

    /* Clamp to what the UNICODE_STRING behind the title can represent */
    if (Length > (ULONG)((MAXUSHORT / sizeof(WCHAR)) - 1))
        Length = (ULONG)((MAXUSHORT / sizeof(WCHAR)) - 1);

    /* The title is server-layer state, including who frees the old buffer */
    return !!TermSetTitle(Console, Title, Length * sizeof(WCHAR));
}

static BOOLEAN
VtReportWindowSize(PCONSOLE Console,
                   PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                   ULONG ReplyPrefix)
{
    WCHAR Response[32];
    SIZE_T Len = 0;
    SHORT Rows;
    SHORT Cols;

    if (!Console || !ScreenBuffer)
        return FALSE;

    Rows = ScreenBuffer->ViewSize.Y;
    Cols = ScreenBuffer->ViewSize.X;
    if (Rows <= 0)
        Rows = ScreenBuffer->ScreenBufferSize.Y;
    if (Cols <= 0)
        Cols = ScreenBuffer->ScreenBufferSize.X;
    if (Rows <= 0) Rows = 1;
    if (Cols <= 0) Cols = 1;

    Response[Len++] = L'\x1b';
    Response[Len++] = L'[';
    Len += VtAppendNumber(Response + Len,
                          ARRAYSIZE(Response) - Len,
                          ReplyPrefix);
    if (Len < ARRAYSIZE(Response))
        Response[Len++] = L';';
    Len += VtAppendNumber(Response + Len,
                          ARRAYSIZE(Response) - Len,
                          (ULONG)Rows);
    if (Len < ARRAYSIZE(Response))
        Response[Len++] = L';';
    Len += VtAppendNumber(Response + Len,
                          ARRAYSIZE(Response) - Len,
                          (ULONG)Cols);
    if (Len < ARRAYSIZE(Response))
        Response[Len++] = L't';

    VtSendInputResponse(Console, Response, Len);
    return TRUE;
}

static BOOLEAN
VtResizeWindow(PCONSOLE Console,
               PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
               ULONG Rows,
               ULONG Cols)
{
    SMALL_RECT WindowRect;
    COORD Size;
    NTSTATUS Status;

    if (!Console || !ScreenBuffer)
        return FALSE;

    if (Rows == 0 || Cols == 0)
        return TRUE;

    if (Rows > SHRT_MAX) Rows = SHRT_MAX;
    if (Cols > SHRT_MAX) Cols = SHRT_MAX;

    Size.X = (SHORT)max(1UL, Cols);
    Size.Y = (SHORT)max(1UL, Rows);

    if (Size.X != ScreenBuffer->ScreenBufferSize.X ||
        Size.Y != ScreenBuffer->ScreenBufferSize.Y)
    {
        Status = ConDrvSetConsoleScreenBufferSize(Console, ScreenBuffer, &Size);
        if (!NT_SUCCESS(Status))
            return FALSE;
    }

    WindowRect.Left = 0;
    WindowRect.Top = 0;
    WindowRect.Right = (SHORT)(Size.X - 1);
    WindowRect.Bottom = (SHORT)(Size.Y - 1);

    Status = ConDrvSetConsoleWindowInfo(Console, ScreenBuffer, TRUE, &WindowRect);
    return NT_SUCCESS(Status);
}

static BOOLEAN
VtParseHexComponent(const WCHAR *Start,
                    const WCHAR *End,
                    ULONG *Value,
                    ULONG *Digits)
{
    ULONG Result = 0;
    ULONG Count = 0;

    if (!Value || !Digits)
        return FALSE;

    while (Start < End)
    {
        WCHAR Ch = *Start++;
        ULONG Nibble;

        if (Ch >= L'0' && Ch <= L'9')
            Nibble = Ch - L'0';
        else if (Ch >= L'a' && Ch <= L'f')
            Nibble = Ch - L'a' + 10;
        else if (Ch >= L'A' && Ch <= L'F')
            Nibble = Ch - L'A' + 10;
        else
            return FALSE;

        Result = (Result << 4) | Nibble;
        if (++Count > 4)
            return FALSE;
    }

    if (Count == 0)
        return FALSE;

    *Value = Result;
    *Digits = Count;
    return TRUE;
}

static BYTE
VtScaleComponent(ULONG Value, ULONG Digits)
{
    ULONG MaxValue;

    if (Digits == 0)
        return 0;

    MaxValue = (1u << (Digits * 4)) - 1u;
    if (MaxValue == 0)
        return 0;

    return (BYTE)((Value * 255u + MaxValue / 2u) / MaxValue);
}

static BOOLEAN
VtOscParseRgb(const WCHAR *Value,
              ULONG Length,
              COLORREF *Color)
{
    const WCHAR *Start = Value;
    const WCHAR *End = Value + Length;

    if (!Value || !Color)
        return FALSE;

    /* Trim surrounding whitespace */
    while (Start < End && (*Start == L' ' || *Start == L'\t'))
        ++Start;
    while (Start < End && (End[-1] == L' ' || End[-1] == L'\t'))
        --End;

    if (Start >= End)
        return FALSE;

    if (*Start == L'#')
    {
        ULONG DigitsPerComponent;
        ULONG ComponentLength = (ULONG)(End - Start - 1);
        ULONG RValue, GValue, BValue;
        ULONG Digits;
        const WCHAR *Ptr;

        if (ComponentLength == 0 || (ComponentLength % 3) != 0)
            return FALSE;

        DigitsPerComponent = ComponentLength / 3;
        Ptr = Start + 1;

        if (!VtParseHexComponent(Ptr, Ptr + DigitsPerComponent, &RValue, &Digits))
            return FALSE;
        Ptr += DigitsPerComponent;

        if (!VtParseHexComponent(Ptr, Ptr + DigitsPerComponent, &GValue, &Digits))
            return FALSE;
        Ptr += DigitsPerComponent;

        if (!VtParseHexComponent(Ptr, Ptr + DigitsPerComponent, &BValue, &Digits))
            return FALSE;

        *Color = RGB(VtScaleComponent(RValue, DigitsPerComponent),
                     VtScaleComponent(GValue, DigitsPerComponent),
                     VtScaleComponent(BValue, DigitsPerComponent));
        return TRUE;
    }

    if ((End - Start) > 4 && Start[0] == L'r' && Start[1] == L'g' && Start[2] == L'b' && Start[3] == L':')
    {
        const WCHAR *Ptr = Start + 4;
        ULONG Values[3];
        ULONG Digits;
        ULONG Index = 0;
        const WCHAR *SegmentStart;

        while (Index < 3 && Ptr <= End)
        {
            SegmentStart = Ptr;
            while (Ptr < End && *Ptr != L'/')
                ++Ptr;

            if (!VtParseHexComponent(SegmentStart, Ptr, &Values[Index], &Digits))
                return FALSE;

            Values[Index] = VtScaleComponent(Values[Index], Digits);
            ++Index;

            if (Ptr < End)
                ++Ptr; /* Skip '/' */
        }

        if (Index != 3)
            return FALSE;

        *Color = RGB((BYTE)Values[0], (BYTE)Values[1], (BYTE)Values[2]);
        return TRUE;
    }

    return FALSE;
}

static VOID
VtUpdatePaletteHandle(PCONSOLE Console,
                      PTEXTMODE_SCREEN_BUFFER Buffer)
{
    COLORREF Colors[VT_PALETTE_SIZE];
    PALETTEENTRY Entries[VT_PALETTE_SIZE];
    UINT Index;

    if (!Buffer || !Buffer->PaletteHandle)
        return;
    if (!VtReadPalette(Console, Colors))
        return;

    for (Index = 0; Index < ARRAYSIZE(Entries); ++Index)
    {
        COLORREF Color = Colors[Index];
        Entries[Index].peRed   = GetRValue(Color);
        Entries[Index].peGreen = GetGValue(Color);
        Entries[Index].peBlue  = GetBValue(Color);
        Entries[Index].peFlags = 0;
    }

    SetPaletteEntries(Buffer->PaletteHandle, 0, ARRAYSIZE(Entries), Entries);
}

static VOID
VtRefreshPaletteForBuffer(PCONSOLE Console,
                          PTEXTMODE_SCREEN_BUFFER Buffer)
{
    if (!Console || !Buffer)
        return;

    if (!Buffer->VtState.UseRgbForeground)
    {
        Buffer->VtState.CurrentFgColor = VtGetPaletteColor(Console, Buffer->VtState.CurrentAttributes & 0x0F);
    }

    if (!Buffer->VtState.UseRgbBackground)
    {
        Buffer->VtState.CurrentBgColor = VtGetPaletteColor(Console, (Buffer->VtState.CurrentAttributes >> 4) & 0x0F);
    }

    if (!Buffer->VtState.SavedUseRgbForeground)
    {
        Buffer->VtState.SavedFgColor = VtGetPaletteColor(Console, Buffer->VtState.SavedAttributes & 0x0F);
    }

    if (!Buffer->VtState.SavedUseRgbBackground)
    {
        Buffer->VtState.SavedBgColor = VtGetPaletteColor(Console, (Buffer->VtState.SavedAttributes >> 4) & 0x0F);
    }

    VtUpdatePaletteHandle(Console, Buffer);
}

/*
 * Make the console's colour table visible on screen.
 *
 * This is the one place that knows how a Colors[] change propagates: every
 * text-mode buffer's cached VT colours and HPALETTE are refreshed, then the
 * active buffer's palette is handed to the frontend and repainted. Walking
 * BufferList rather than naming the active/primary/alternate buffers means a
 * buffer created by some other path is not silently left with a stale palette.
 */
VOID
NTAPI
ConDrvVtRefreshPalette(PCONSOLE Console)
{
    PCONSOLE_SCREEN_BUFFER Active;
    PLIST_ENTRY Entry;

    if (!Console)
        return;

    for (Entry = Console->BufferList.Flink; Entry != &Console->BufferList; Entry = Entry->Flink)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = CONTAINING_RECORD(Entry, CONSOLE_SCREEN_BUFFER, ListEntry);

        if (GetType(Buffer) == TEXTMODE_BUFFER)
            VtRefreshPaletteForBuffer(Console, (PTEXTMODE_SCREEN_BUFFER)Buffer);
    }

    Active = Console->ActiveBuffer;
    if (Active && GetType(Active) == TEXTMODE_BUFFER)
    {
        PTEXTMODE_SCREEN_BUFFER TextActive = (PTEXTMODE_SCREEN_BUFFER)Active;
        SMALL_RECT Region;

        TermSetPalette(Console, TextActive->PaletteHandle, TextActive->PaletteUsage);

        if (TextActive->ScreenBufferSize.X > 0 && TextActive->ScreenBufferSize.Y > 0)
        {
            Region.Left = 0;
            Region.Top = 0;
            Region.Right = TextActive->ScreenBufferSize.X - 1;
            Region.Bottom = TextActive->ScreenBufferSize.Y - 1;
            /*
             * Flush straight away rather than accumulating: this marks the
             * console's active buffer, which need not be the one the current
             * write is addressed to, so the rectangle could otherwise sit
             * unpainted until that buffer is next written. A palette change is
             * a whole-screen repaint and is rare, so there is nothing to save.
             */
            VtMarkDirtyRect(TextActive, &Region);
            VtFlushDirty(Console, TextActive);
        }
    }
}

static BOOLEAN
VtHandleOscPalette(PCONSOLE Console,
                   PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                   const WCHAR *Parameters,
                   ULONG Length)
{
    const WCHAR *Ptr = Parameters;
    const WCHAR *End = Parameters + Length;
    BOOLEAN Handled = FALSE;
    BOOLEAN Updated = FALSE;
    COLORREF Colors[VT_PALETTE_SIZE];

    if (!VtReadPalette(Console, Colors))
        return TRUE;

    while (Ptr < End)
    {
        ULONG Index = 0;
        BOOLEAN HaveDigits = FALSE;

        while (Ptr < End && (*Ptr == L';' || *Ptr == L' ' || *Ptr == L'\t'))
            ++Ptr;

        if (Ptr >= End)
            break;

        while (Ptr < End && *Ptr != L';')
        {
            if (*Ptr >= L'0' && *Ptr <= L'9')
            {
                Index = Index * 10 + (ULONG)(*Ptr - L'0');
                HaveDigits = TRUE;
            }
            else if (*Ptr != L' ' && *Ptr != L'\t')
            {
                HaveDigits = FALSE;
            }
            ++Ptr;
        }

        if (!HaveDigits)
        {
            if (Ptr < End)
                ++Ptr;
            continue;
        }

        if (Ptr >= End)
            break;

        ++Ptr; /* Skip ';' */
        const WCHAR *ColorStart = Ptr;
        while (Ptr < End && *Ptr != L';')
            ++Ptr;

        if (ColorStart < Ptr && Index < 16)
        {
            COLORREF Parsed;
            ULONG ColorLength = (ULONG)(Ptr - ColorStart);
            UCHAR PaletteIndex = VtXtermToConsoleIndex((UCHAR)Index);

            if (ColorLength == 1 && *ColorStart == L'?')
            {
                VtSendOscColorReply(Console, 4, TRUE, Index, Colors[PaletteIndex]);
                Handled = TRUE;
            }
            else if (VtOscParseRgb(ColorStart, ColorLength, &Parsed))
            {
                Colors[PaletteIndex] = Parsed;
                Handled = TRUE;
                Updated = TRUE;
            }
        }
    }

    if (Updated)
    {
        TermSetColorTable(Console, Colors, VT_PALETTE_SIZE);

        ConDrvVtRefreshPalette(Console);
    }

    return Handled;
}

static BOOLEAN
VtHandleOscDefaultColor(PCONSOLE Console,
                        PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                        ULONG Parameter,
                        const WCHAR *Value,
                        ULONG Length)
{
    COLORREF Colors[VT_PALETTE_SIZE];
    UCHAR Index;
    COLORREF Parsed;

    if (!ScreenBuffer || !Value)
        return FALSE;
    if (!VtReadPalette(Console, Colors))
        return FALSE;

    while (Length > 0 && (*Value == L' ' || *Value == L'\t'))
    {
        ++Value;
        --Length;
    }

    if (Parameter == 10)
        Index = (UCHAR)(ScreenBuffer->ScreenDefaultAttrib & 0x0F);
    else if (Parameter == 11)
        Index = (UCHAR)((ScreenBuffer->ScreenDefaultAttrib >> 4) & 0x0F);
    else
        return FALSE;

    if (Length > 0 && *Value == L'?')
    {
        VtSendOscColorReply(Console, Parameter, FALSE, 0, Colors[Index]);
        return TRUE;
    }

    if (!VtOscParseRgb(Value, Length, &Parsed))
        return FALSE;

    Colors[Index] = Parsed;
    return !!TermSetColorTable(Console, Colors, VT_PALETTE_SIZE);
}

static VOID
VtHandleOscHyperlink(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                     const WCHAR *Parameters,
                     ULONG Length)
{
    PCONSOLE Console = ScreenBuffer ? ScreenBuffer->Header.Console : NULL;
    const WCHAR *Ptr = Parameters;
    const WCHAR *End = Parameters + Length;
    const WCHAR *UriStart;
    ULONG UriLength = 0;

    if (!ScreenBuffer)
        return;

    if (Console && !Console->AllowVtOscHyperlinks)
    {
        VtTracePolicyBlocked("OSC 8 hyperlink");
        VtClearHyperlink(ScreenBuffer);
        return;
    }

    while (Ptr < End && *Ptr != L';')
        ++Ptr;

    if (Ptr < End)
        ++Ptr;
    else
        Ptr = End;

    UriStart = Ptr;
    if (UriStart < End)
        UriLength = (ULONG)(End - UriStart);

    VtClearHyperlink(ScreenBuffer);

    if (UriLength == 0)
        return;

    if (UriLength >= (ULONG)((USHRT_MAX / sizeof(WCHAR)) - 1))
        return;

    PWCHAR Buffer = ConsoleAllocHeap(0, (UriLength + 1) * sizeof(WCHAR));
    if (!Buffer)
        return;

    RtlCopyMemory(Buffer, UriStart, UriLength * sizeof(WCHAR));
    Buffer[UriLength] = UNICODE_NULL;

    ScreenBuffer->VtState.HyperlinkUri.Buffer = Buffer;
    ScreenBuffer->VtState.HyperlinkUri.Length = (USHORT)(UriLength * sizeof(WCHAR));
    ScreenBuffer->VtState.HyperlinkUri.MaximumLength = (USHORT)((UriLength + 1) * sizeof(WCHAR));
    ScreenBuffer->VtState.HyperlinkActive = TRUE;
    return;
}


static VOID
VtHandleOscClipboard(PCONSOLE Console,
                     const WCHAR *Parameters,
                     ULONG Length)
{
    const WCHAR *End = Parameters + Length;
    const WCHAR *TargetStart = Parameters;
    const WCHAR *TargetEnd = TargetStart;
    const WCHAR *DataStart;
    SIZE_T TargetLength;

    if (!Console)
        return;

    while (TargetEnd < End && *TargetEnd != L';')
        ++TargetEnd;

    DataStart = (TargetEnd < End) ? TargetEnd + 1 : End;
    TargetLength = (SIZE_T)(TargetEnd - TargetStart);

    while (DataStart < End && (*DataStart == L' ' || *DataStart == L'	'))
        ++DataStart;
    while (End > DataStart && (End[-1] == L' ' || End[-1] == L'	'))
        --End;

    if (DataStart >= End)
        return;

    if (*DataStart == L'?')
    {
        PWSTR Encoded = NULL;
        BYTE *Utf8Buffer = NULL;
        PWCHAR ClipText = NULL;
        ULONG ClipChars = 0;
        BOOL Success = FALSE;

        if (!Console->AllowVtOscClipboard)
        {
            VtSendOscClipboardResponse(Console, TargetStart, TargetLength, L"");
            return;
        }

        /* The clipboard belongs to the frontend's window station, not to us */
        if (TermGetClipboardText(Console, &ClipText, &ClipChars) && ClipChars > 0)
        {
            int Required = WideCharToMultiByte(CP_UTF8, 0, ClipText, (int)ClipChars, NULL, 0, NULL, NULL);
            if (Required > 0)
            {
                Utf8Buffer = ConsoleAllocHeap(0, (ULONG)Required);
                if (Utf8Buffer)
                {
                    if (WideCharToMultiByte(CP_UTF8, 0, ClipText, (int)ClipChars, (LPSTR)Utf8Buffer, Required, NULL, NULL) == Required)
                    {
                        ULONG EncodedChars = 0;
                        if (VtEncodeBase64(Utf8Buffer, (ULONG)Required, &Encoded, &EncodedChars))
                            Success = TRUE;
                    }
                    ConsoleFreeHeap(Utf8Buffer);
                }
            }
        }
        if (ClipText) ConsoleFreeHeap(ClipText);

        if (Success && Encoded)
        {
            VtSendOscClipboardResponse(Console, TargetStart, TargetLength, Encoded);
            ConsoleFreeHeap(Encoded);
        }
        else
        {
            VtSendOscClipboardResponse(Console, TargetStart, TargetLength, L"");
            ConsoleFreeHeap(Encoded);
        }

        return;
    }

    if (!Console->AllowVtOscClipboard)
        return;

    {
        ULONG DataLength = (ULONG)(End - DataStart);
        BYTE *Decoded = NULL;
        ULONG DecodedLength = 0;

        if (!VtDecodeBase64(DataStart, DataLength, &Decoded, &DecodedLength))
            return;

        if (DecodedLength > 0)
        {
            int WideChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)Decoded, (int)DecodedLength, NULL, 0);
            if (WideChars > 0)
            {
                PWCHAR Wide = ConsoleAllocHeap(0, ((ULONG)WideChars + 1) * sizeof(WCHAR));
                if (Wide)
                {
                    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)Decoded, (int)DecodedLength, Wide, WideChars) == WideChars)
                    {
                        Wide[WideChars] = UNICODE_NULL;
                        /* Publishing to the clipboard is the frontend's job */
                        TermSetClipboardText(Console, Wide, (ULONG)WideChars);
                    }
                    ConsoleFreeHeap(Wide);
                }
            }
        }

        ConsoleFreeHeap(Decoded);
    }

    return;
}

/*
 * An OSC sequence is always consumed, recognised or not: a terminal swallows
 * what it does not understand rather than printing the raw payload on screen.
 */
static VOID
VtHandleOscSequence(PCONSOLE Console,
                    PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                    const WCHAR *Sequence,
                    ULONG Length)
{
    const WCHAR *End = Sequence + Length;
    const WCHAR *ParamEnd = Sequence;
    ULONG Parameter = 0;
    BOOLEAN HaveParameter = FALSE;

    while (ParamEnd < End && *ParamEnd != L';')
        ++ParamEnd;

    for (const WCHAR *Ptr = Sequence; Ptr < ParamEnd; ++Ptr)
    {
        if (*Ptr >= L'0' && *Ptr <= L'9')
        {
            Parameter = Parameter * 10 + (ULONG)(*Ptr - L'0');
            HaveParameter = TRUE;
        }
        else if (*Ptr != L' ' && *Ptr != L'\t')
        {
            /* Not a numeric parameter - nothing we can dispatch on */
            return;
        }
    }

    if (!HaveParameter)
        Parameter = 0;

    if (ParamEnd < End)
        ++ParamEnd;

    switch (Parameter)
    {
        case 0:
        case 1:
        case 2:
            if (ParamEnd <= End)
            {
                VtSetConsoleTitle(Console, ParamEnd, (ULONG)(End - ParamEnd));
            }
            return;

        case 4:
            VtHandleOscPalette(Console, ScreenBuffer, ParamEnd, (ULONG)(End - ParamEnd));
            return;

        case 10:
        case 11:
        {
            BOOLEAN Updated = FALSE;

            if (ParamEnd <= End)
            {
                Updated = VtHandleOscDefaultColor(Console,
                                                   ScreenBuffer,
                                                   Parameter,
                                                   ParamEnd,
                                                   (ULONG)(End - ParamEnd));
            }

            if (Updated)
            {
                ConDrvVtRefreshPalette(Console);
            }

            return;
        }

        case 8:
            VtHandleOscHyperlink(ScreenBuffer, ParamEnd, (ULONG)(End - ParamEnd));
            return;

        case 52:
            VtHandleOscClipboard(Console, ParamEnd, (ULONG)(End - ParamEnd));
            return;

        case 133: /* iTerm2 prompt markers */
        case 134:
        case 633:
            return;

        default:
            VtTraceUnhandledOsc(Parameter);
            return;
    }
}

static VOID
VtUpdateLastWrittenChar(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                        PCWSTR Text,
                        ULONG Length)
{
    ULONG i;

    if (!ScreenBuffer || !Text || Length == 0)
        return;

    for (i = Length; i > 0; --i)
    {
        WCHAR Ch = Text[i - 1];
        if (Ch >= L' ' && Ch != 0x7F)
        {
            ScreenBuffer->VtState.LastWrittenChar = Ch;
            ScreenBuffer->VtState.LastCharValid = TRUE;
            break;
        }
    }
}

static NTSTATUS
VtFlushText(PCONSOLE Console,
           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
           PCWSTR Text,
           ULONG Length)
{
    NTSTATUS Status;
    USHORT OriginalAttrib;
    WCHAR StackBuffer[128];
    WCHAR *Translated;
    UCHAR pending;

    if (Length == 0) return STATUS_SUCCESS;

    OriginalAttrib = ScreenBuffer->ScreenDefaultAttrib;
    ScreenBuffer->ScreenDefaultAttrib = ScreenBuffer->VtState.CurrentAttributes;

    VT_CHARSET_ID ActiveCharset = VtGetActiveCharset(ScreenBuffer);

    if ((ScreenBuffer->VtState.PendingSingleShift == VT_CHARSET_SLOT_INVALID) &&
        (ActiveCharset == VtCharsetAscii))
    {
        Status = TermWriteStream(Console,
                                 ScreenBuffer,
                                 (PWCHAR)Text,
                                 Length,
                                 TRUE);
        if (NT_SUCCESS(Status))
            VtUpdateLastWrittenChar(ScreenBuffer, Text, Length);
        ScreenBuffer->ScreenDefaultAttrib = OriginalAttrib;
        return Status;
    }

    Translated = StackBuffer;

    if (Length > ARRAYSIZE(StackBuffer))
    {
        Translated = ConsoleAllocHeap(0, Length * sizeof(WCHAR));
        if (!Translated)
        {
            ScreenBuffer->ScreenDefaultAttrib = OriginalAttrib;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    pending = ScreenBuffer->VtState.PendingSingleShift;

    for (ULONG i = 0; i < Length; ++i)
    {
        UCHAR slot = (pending != VT_CHARSET_SLOT_INVALID) ? pending
                                                          : ScreenBuffer->VtState.ActiveCharset;
        Translated[i] = VtTranslateGlyphSlot(ScreenBuffer, slot, Text[i]);

        if (pending != VT_CHARSET_SLOT_INVALID)
            pending = VT_CHARSET_SLOT_INVALID;
    }

    ScreenBuffer->VtState.PendingSingleShift = pending;

    Status = TermWriteStream(Console,
                             ScreenBuffer,
                             Translated,
                             Length,
                             TRUE);

    if (NT_SUCCESS(Status))
        VtUpdateLastWrittenChar(ScreenBuffer, Translated, Length);

    if (Translated != StackBuffer)
        ConsoleFreeHeap(Translated);

    ScreenBuffer->ScreenDefaultAttrib = OriginalAttrib;
    return Status;
}

static UCHAR
VtXtermToConsoleIndex(UCHAR Index)
{
    Index &= 0x0F;
    return (UCHAR)((Index & 0x8) |
                   ((Index & 0x1) << 2) |
                   (Index & 0x2) |
                   ((Index & 0x4) >> 2));
}

static VOID
VtApplySgr(PCONSOLE Console,
           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
           const ULONG *Params,
           ULONG Count)
{
    USHORT Attr = ScreenBuffer->VtState.CurrentAttributes;
    /*
     * What SGR 0 / 39 / 49 reset to is the screen buffer's default attribute,
     * not whatever DECSC last parked in SavedAttributes - those are unrelated
     * concepts, and reading the DECSC slot here meant an SGR 0 after a DECSC
     * reset to the attributes in force at the DECSC rather than to the default.
     *
     * Safe to read directly: VtFlushText's temporary override of
     * ScreenDefaultAttrib is confined to that function and is restored before
     * any SGR sequence is dispatched.
     */
    const USHORT DefaultAttr = ScreenBuffer->ScreenDefaultAttrib;

    if (Count == 0)
    {
        Attr = DefaultAttr;
        ScreenBuffer->VtState.UseRgbForeground = FALSE;
        ScreenBuffer->VtState.UseRgbBackground = FALSE;
        ScreenBuffer->VtState.CurrentFgColor = VtGetPaletteColor(Console, Attr & 0x0F);
        ScreenBuffer->VtState.CurrentBgColor = VtGetPaletteColor(Console, (Attr >> 4) & 0x0F);
        ScreenBuffer->VtState.CurrentAttributes = Attr;
        return;
    }

    for (ULONG i = 0; i < Count; ++i)
    {
        ULONG Code = Params[i];
        switch (Code)
        {
            case 0:
                Attr = DefaultAttr;
                ScreenBuffer->VtState.UseRgbForeground = FALSE;
                ScreenBuffer->VtState.UseRgbBackground = FALSE;
                break;

            case 1:
                Attr |= FOREGROUND_INTENSITY;
                break;

            case 2:
                /* SGR 2 (Dim/Faint): Not supported by the console subsystem;
                 * recognized to avoid unknown-sequence warnings. */
                break;

            case 3:
                /* SGR 3 (Italic): Not supported by the console subsystem;
                 * recognized to avoid unknown-sequence warnings. */
                break;

            case 4:
                Attr |= COMMON_LVB_UNDERSCORE;
                break;

            case 7:
                Attr |= COMMON_LVB_REVERSE_VIDEO;
                break;

            case 9:
                /* SGR 9 (Strikethrough): Not supported by the console subsystem;
                 * recognized to avoid unknown-sequence warnings. */
                break;

            case 22:
                Attr &= ~FOREGROUND_INTENSITY;
                break;

            case 23:
                /* SGR 23 (Italic off): Not supported by the console subsystem;
                 * recognized to avoid unknown-sequence warnings. */
                break;

            case 24:
                Attr &= ~COMMON_LVB_UNDERSCORE;
                break;

            case 27:
                Attr &= ~COMMON_LVB_REVERSE_VIDEO;
                break;

            case 29:
                /* SGR 29 (Strikethrough off): Not supported by the console subsystem;
                 * recognized to avoid unknown-sequence warnings. */
                break;

            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
            {
                UCHAR idx = (UCHAR)(Code - 30);
                Attr &= ~FG_ATTR_MASK;
                Attr |= VtXtermToConsoleIndex(idx);
                ScreenBuffer->VtState.UseRgbForeground = FALSE;
                break;
            }

            case 39:
                Attr &= ~FG_ATTR_MASK;
                Attr |= (DefaultAttr & FG_ATTR_MASK);
                ScreenBuffer->VtState.UseRgbForeground = FALSE;
                break;

            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
            {
                UCHAR idx = (UCHAR)(Code - 40);
                Attr &= ~BG_ATTR_MASK;
                Attr |= (USHORT)(VtXtermToConsoleIndex(idx) << 4);
                ScreenBuffer->VtState.UseRgbBackground = FALSE;
                break;
            }

            case 49:
                Attr &= ~BG_ATTR_MASK;
                Attr |= (DefaultAttr & BG_ATTR_MASK);
                ScreenBuffer->VtState.UseRgbBackground = FALSE;
                break;

            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
            {
                UCHAR idx = (UCHAR)((Code - 90) | 0x08);
                Attr &= ~FG_ATTR_MASK;
                Attr |= VtXtermToConsoleIndex(idx);
                ScreenBuffer->VtState.UseRgbForeground = FALSE;
                break;
            }

            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
            {
                UCHAR idx = (UCHAR)((Code - 100) | 0x08);
                Attr &= ~BG_ATTR_MASK;
                Attr |= (USHORT)(VtXtermToConsoleIndex(idx) << 4);
                ScreenBuffer->VtState.UseRgbBackground = FALSE;
                break;
            }

            case 38:
            case 48:
            {
                BOOL Foreground = (Code == 38);
                if (i + 1 < Count)
                {
                    ULONG mode = Params[++i];
                    if (mode == 2 && (i + 3) < Count)
                    {
                        ULONG r = Params[++i] & 0xFF;
                        ULONG g = Params[++i] & 0xFF;
                        ULONG b = Params[++i] & 0xFF;
                        COLORREF color = RGB((BYTE)r, (BYTE)g, (BYTE)b);
                        UCHAR nearest = VtFindNearestPaletteIndex(Console, color);
                        if (Foreground)
                        {
                            Attr &= ~FG_ATTR_MASK;
                            Attr |= (USHORT)(nearest & 0x0F);
                            ScreenBuffer->VtState.UseRgbForeground = TRUE;
                            ScreenBuffer->VtState.CurrentFgColor = color;
                        }
                        else
                        {
                            Attr &= ~BG_ATTR_MASK;
                            Attr |= (USHORT)((nearest & 0x0F) << 4);
                            ScreenBuffer->VtState.UseRgbBackground = TRUE;
                            ScreenBuffer->VtState.CurrentBgColor = color;
                        }
                        continue;
                    }
                    else if (mode == 5 && (i + 1) < Count)
                    {
                        ULONG idx = Params[++i];
                        if (idx < 16)
                        {
                            if (Foreground)
                            {
                                Attr &= ~FG_ATTR_MASK;
                                Attr |= VtXtermToConsoleIndex((UCHAR)idx);
                                ScreenBuffer->VtState.UseRgbForeground = FALSE;
                            }
                            else
                            {
                                Attr &= ~BG_ATTR_MASK;
                                Attr |= (USHORT)(VtXtermToConsoleIndex((UCHAR)idx) << 4);
                                ScreenBuffer->VtState.UseRgbBackground = FALSE;
                            }
                        }
                        else
                        {
                            /* Only the 6x6x6 cube and the greyscale ramp need an RGB value */
                            COLORREF color = VtColorFromXtermIndex(idx);
                            UCHAR nearest = VtFindNearestPaletteIndex(Console, color);
                            if (Foreground)
                            {
                                Attr &= ~FG_ATTR_MASK;
                                Attr |= (USHORT)(nearest & 0x0F);
                                ScreenBuffer->VtState.UseRgbForeground = TRUE;
                                ScreenBuffer->VtState.CurrentFgColor = color;
                            }
                            else
                            {
                                Attr &= ~BG_ATTR_MASK;
                                Attr |= (USHORT)((nearest & 0x0F) << 4);
                                ScreenBuffer->VtState.UseRgbBackground = TRUE;
                                ScreenBuffer->VtState.CurrentBgColor = color;
                            }
                        }
                        continue;
                    }
                }

                if (Foreground)
                    ScreenBuffer->VtState.UseRgbForeground = FALSE;
                else
                    ScreenBuffer->VtState.UseRgbBackground = FALSE;
                break;
            }

            default:
                break;
        }
    }

    ScreenBuffer->VtState.CurrentAttributes = Attr;

    if (!ScreenBuffer->VtState.UseRgbForeground)
        ScreenBuffer->VtState.CurrentFgColor = VtGetPaletteColor(Console, Attr & 0x0F);
    if (!ScreenBuffer->VtState.UseRgbBackground)
        ScreenBuffer->VtState.CurrentBgColor = VtGetPaletteColor(Console, (Attr >> 4) & 0x0F);
}


static VOID
VtEraseLine(PCONSOLE Console,
            PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
            ULONG Mode)
{
    SHORT Y = ScreenBuffer->CursorPosition.Y;
    SMALL_RECT Region;
    SHORT StartX, EndX;

    if (Y < 0 || Y >= ScreenBuffer->ScreenBufferSize.Y)
        return;

    switch (Mode)
    {
        case 1:
            StartX = 0;
            EndX = ScreenBuffer->CursorPosition.X;
            break;

        case 2:
            StartX = 0;
            EndX = ScreenBuffer->ScreenBufferSize.X - 1;
            break;

        case 0:
        default:
            StartX = ScreenBuffer->CursorPosition.X;
            EndX = ScreenBuffer->ScreenBufferSize.X - 1;
            break;
    }

    StartX = max(StartX, 0);
    EndX = min(EndX, (SHORT)(ScreenBuffer->ScreenBufferSize.X - 1));
    if (EndX < StartX)
        return;

    VtFillRect(ScreenBuffer, StartX, Y, EndX, Y, L' ');

    Region.Left   = StartX;
    Region.Right  = EndX;
    Region.Top    = Y;
    Region.Bottom = Y;
    VtMarkDirtyRect(ScreenBuffer, &Region);
}

static VOID
VtDecaln(PCONSOLE Console,
         PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    SMALL_RECT Region;
    COORD Origin;

    if (!Console || !ScreenBuffer)
        return;

    /* DECALN fills the whole screen with 'E' using the current attributes */
    VtFillRect(ScreenBuffer, 0, 0, (SHORT)(ScreenBuffer->ScreenBufferSize.X - 1), (SHORT)(ScreenBuffer->ScreenBufferSize.Y - 1), L'E');

    Region.Left   = 0;
    Region.Top    = 0;
    Region.Right  = ScreenBuffer->ScreenBufferSize.X > 0 ? ScreenBuffer->ScreenBufferSize.X - 1 : 0;
    Region.Bottom = ScreenBuffer->ScreenBufferSize.Y > 0 ? ScreenBuffer->ScreenBufferSize.Y - 1 : 0;
    VtMarkDirtyRect(ScreenBuffer, &Region);

    Origin.X = 0;
    Origin.Y = 0;
    ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Origin);
}

static VOID
VtEraseDisplay(PCONSOLE Console,
               PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
               ULONG Mode)
{
    SHORT Y;
    SMALL_RECT Region;
    COORD Origin;

    switch (Mode)
    {
        case 1:
        case 0:
        {
            SHORT StartY, EndY;
            SHORT StartX = 0;
            SHORT EndX = ScreenBuffer->ScreenBufferSize.X - 1;

            if (Mode == 0)
            {
                StartY = ScreenBuffer->CursorPosition.Y;
                EndY = ScreenBuffer->ScreenBufferSize.Y - 1;
            }
            else
            {
                StartY = 0;
                EndY = ScreenBuffer->CursorPosition.Y;
            }

            StartY = max(StartY, 0);
            EndY = min(EndY, (SHORT)(ScreenBuffer->ScreenBufferSize.Y - 1));
            if (EndY < StartY)
                return;

            for (Y = StartY; Y <= EndY; ++Y)
            {
                SHORT LocalStartX = StartX;
                SHORT LocalEndX = EndX;

                if (Mode == 0 && Y == StartY)
                    LocalStartX = max(ScreenBuffer->CursorPosition.X, 0);
                else if (Mode == 1 && Y == EndY)
                    LocalEndX = min(ScreenBuffer->CursorPosition.X, (SHORT)(ScreenBuffer->ScreenBufferSize.X - 1));

                VtFillRect(ScreenBuffer, LocalStartX, Y, LocalEndX, Y, L' ');
            }

            Region.Left   = 0;
            Region.Right  = ScreenBuffer->ScreenBufferSize.X - 1;
            Region.Top    = StartY;
            Region.Bottom = EndY;
            VtMarkDirtyRect(ScreenBuffer, &Region);
            break;
        }

        case 2:
        case 3:
            VtFillRect(ScreenBuffer, 0, 0, (SHORT)(ScreenBuffer->ScreenBufferSize.X - 1), (SHORT)(ScreenBuffer->ScreenBufferSize.Y - 1), L' ');

            Region.Left   = 0;
            Region.Top    = 0;
            Region.Right  = ScreenBuffer->ScreenBufferSize.X - 1;
            Region.Bottom = ScreenBuffer->ScreenBufferSize.Y - 1;
            VtMarkDirtyRect(ScreenBuffer, &Region);

            Origin.X = Origin.Y = 0;
            ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Origin);

            if (Mode == 3)
            {
                ScreenBuffer->VirtualY = 0;
                ScreenBuffer->ViewOrigin.Y = 0;
            }
            break;
    }
}

static VOID
VtSoftReset(PCONSOLE Console,
            PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    PTEXTMODE_SCREEN_BUFFER Target;
    COORD Origin;

    if (!Console || !ScreenBuffer)
        return;

    if (ScreenBuffer->VtState.AlternateActive)
    {
        /* The swap happens in place, so the target never changes */
        VtDisableAlternateScreen(Console, ScreenBuffer);
        Target = ScreenBuffer;
    }
    else
    {
        Target = ScreenBuffer;
    }

    if (!Target)
        return;

    VtClearHyperlink(Target);
    ConDrvVtInitializeBuffer(Target);

    VtEraseDisplay(Console, Target, 2);

    Origin.X = 0;
    Origin.Y = 0;
    ConDrvSetConsoleCursorPosition(Console, Target, &Origin);
    ConDrvSetConsoleCursorInfo(Console, Target, &Target->CursorInfo);
    Target->VtState.LastCharValid = FALSE;
}

VOID
NTAPI
ConDrvVtInitializeBuffer(PTEXTMODE_SCREEN_BUFFER ScreenBuffer)
{
    COORD Zero = {0, 0};

    if (!ScreenBuffer) return;

    ScreenBuffer->VtState.CursorSaved = FALSE;
    ScreenBuffer->VtState.SavedCursorPos = Zero;
    /* DECSC slot: starts out matching the buffer default until a DECSC happens */
    ScreenBuffer->VtState.SavedAttributes = ScreenBuffer->ScreenDefaultAttrib;
    ScreenBuffer->VtState.CurrentAttributes = ScreenBuffer->ScreenDefaultAttrib;
    ScreenBuffer->VtState.UseRgbForeground = FALSE;
    ScreenBuffer->VtState.UseRgbBackground = FALSE;
    ScreenBuffer->VtState.CurrentFgColor = VtGetPaletteColor(ScreenBuffer->Header.Console, ScreenBuffer->ScreenDefaultAttrib & 0x0F);
    ScreenBuffer->VtState.CurrentBgColor = VtGetPaletteColor(ScreenBuffer->Header.Console, (ScreenBuffer->ScreenDefaultAttrib >> 4) & 0x0F);
    ScreenBuffer->VtState.SavedFgColor = ScreenBuffer->VtState.CurrentFgColor;
    ScreenBuffer->VtState.SavedBgColor = ScreenBuffer->VtState.CurrentBgColor;
    ScreenBuffer->VtState.SavedUseRgbForeground = FALSE;
    ScreenBuffer->VtState.SavedUseRgbBackground = FALSE;
    ScreenBuffer->VtState.PrivateModes = 0;
    ScreenBuffer->VtState.ScrollTop = 0;
    ScreenBuffer->VtState.ScrollBottom = max(0, ScreenBuffer->ScreenBufferSize.Y - 1);
    ScreenBuffer->VtState.AlternateActive = FALSE;
    ScreenBuffer->VtState.SavedBuffer = NULL;
    ScreenBuffer->VtState.SavedCellRgb = NULL;
    ScreenBuffer->VtState.SavedScreenCursorInfo = ScreenBuffer->CursorInfo;
    ScreenBuffer->VtState.SavedScreenCursorPos = Zero;
    ScreenBuffer->VtState.SavedViewOrigin = Zero;
    ScreenBuffer->VtState.SavedVirtualY = 0;
    ScreenBuffer->VtState.DefaultCursorInfo = ScreenBuffer->CursorInfo;
    ScreenBuffer->VtState.HyperlinkActive = FALSE;
    ScreenBuffer->VtState.HyperlinkUri.Buffer = NULL;
    ScreenBuffer->VtState.HyperlinkUri.Length = 0;
    ScreenBuffer->VtState.HyperlinkUri.MaximumLength = 0;
    ScreenBuffer->VtState.Charsets[VT_CHARSET_SLOT_G0] = VtCharsetAscii;
    ScreenBuffer->VtState.Charsets[VT_CHARSET_SLOT_G1] = VtCharsetDecSpecial;
    ScreenBuffer->VtState.Charsets[VT_CHARSET_SLOT_G2] = VtCharsetAscii;
    ScreenBuffer->VtState.Charsets[VT_CHARSET_SLOT_G3] = VtCharsetAscii;
    ScreenBuffer->VtState.ActiveCharset = VT_CHARSET_SLOT_G0;
    ScreenBuffer->VtState.PendingSingleShift = VT_CHARSET_SLOT_INVALID;
    RtlCopyMemory(ScreenBuffer->VtState.SavedCharsets, ScreenBuffer->VtState.Charsets, sizeof(ScreenBuffer->VtState.Charsets));
    ScreenBuffer->VtState.SavedActiveCharset = ScreenBuffer->VtState.ActiveCharset;
    ScreenBuffer->VtState.MouseButtonState = 0;
    ScreenBuffer->VtState.PendingSequenceLength = 0;

    /* VtEnsureTabStops reuses an existing bitmap of the right width, so there is
     * no need to free and immediately re-allocate one here */
    VtResetTabStops(ScreenBuffer);
}

static VOID
VtHandleCursorPosition(PCONSOLE Console,
                       PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                       const ULONG *Params,
                       ULONG Count)
{
    COORD Position;
    SHORT Row, Col;

    Row = (SHORT)max(1UL, Params[0]);
    Col = (Count >= 2) ? (SHORT)max(1UL, Params[1]) : 1;

    if (ScreenBuffer->VtState.PrivateModes & VT_PRIVMODE_ORIGIN_MODE)
    {
        SHORT Top = ScreenBuffer->VtState.ScrollTop;
        SHORT Bottom = ScreenBuffer->VtState.ScrollBottom;
        SHORT Height = Bottom - Top + 1;

        if (Height <= 0 || Height > ScreenBuffer->ScreenBufferSize.Y)
            Height = ScreenBuffer->ScreenBufferSize.Y;
        if (Height <= 0)
            Height = 1;

        if (Row > Height)
            Row = Height;

        Position.Y = Top + Row - 1;
    }
    else
    {
        if (Row > ScreenBuffer->ScreenBufferSize.Y)
            Row = ScreenBuffer->ScreenBufferSize.Y;

        Position.Y = Row - 1;
    }

    if (Col > ScreenBuffer->ScreenBufferSize.X)
        Col = ScreenBuffer->ScreenBufferSize.X;

    Position.X = Col - 1;

    ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Position);
}

static VOID
VtMoveCursorRelative(PCONSOLE Console,
                     PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                     LONG DeltaX,
                     LONG DeltaY)
{
    COORD Position;
    LONG MinY = 0;
    LONG MaxY;
    LONG MaxX;

    if (!Console || !ScreenBuffer)
        return;

    if (ScreenBuffer->ScreenBufferSize.X <= 0 || ScreenBuffer->ScreenBufferSize.Y <= 0)
        return;

    Position = ScreenBuffer->CursorPosition;
    MaxX = ScreenBuffer->ScreenBufferSize.X - 1;
    MaxY = ScreenBuffer->ScreenBufferSize.Y - 1;

    if (ScreenBuffer->VtState.PrivateModes & VT_PRIVMODE_ORIGIN_MODE)
    {
        MinY = max(0, ScreenBuffer->VtState.ScrollTop);
        MaxY = min(MaxY, ScreenBuffer->VtState.ScrollBottom);
        if (MaxY < MinY)
        {
            MinY = 0;
            MaxY = ScreenBuffer->ScreenBufferSize.Y - 1;
        }
    }

    Position.X = (SHORT)min(max((LONG)Position.X + DeltaX, 0), MaxX);
    Position.Y = (SHORT)min(max((LONG)Position.Y + DeltaY, MinY), MaxY);
    ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Position);
}

static BOOLEAN
VtApplyCursorStyle(PCONSOLE Console,
                   PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                   ULONG Style)
{
    CONSOLE_CURSOR_INFO CursorInfo;

    if (Style == 0)
    {
        CursorInfo = ScreenBuffer->VtState.DefaultCursorInfo;
    }
    else
    {
        CursorInfo = ScreenBuffer->CursorInfo;

        switch (Style)
        {
            case 1: /* blinking block (default) */
            case 2: /* steady block */
                CursorInfo.dwSize = 100;
                break;

            case 3: /* blinking underline */
            case 4: /* steady underline */
                CursorInfo.dwSize = 15;
                break;

            case 5: /* blinking bar */
            case 6: /* steady bar */
                CursorInfo.dwSize = 25;
                break;

            default:
                return FALSE;
        }
    }

    if (!NT_SUCCESS(ConDrvSetConsoleCursorInfo(Console,
                                               ScreenBuffer,
                                               &CursorInfo)))
    {
        return FALSE;
    }


    return TRUE;
}

static BOOLEAN
VtHandleDecPrivateMode(PCONSOLE Console,
                       PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                       ULONG Mode,
                       BOOLEAN Enable)
{
    ULONG Mask = 0;
    BOOLEAN RequiresWindowResize = FALSE;
    ULONG NewColumns = 0;
    BOOLEAN Handled = FALSE;

    switch (Mode)
    {
        case 25: /* DECTCEM – show/hide the text cursor */
        {
            CONSOLE_CURSOR_INFO CursorInfo = ScreenBuffer->CursorInfo;
            CursorInfo.bVisible = Enable ? TRUE : FALSE;

            return NT_SUCCESS(ConDrvSetConsoleCursorInfo(Console,
                                                         ScreenBuffer,
                                                         &CursorInfo));
        }

        case 1:  /* DECCKM – cursor keys send application sequences */
            Mask = VT_PRIVMODE_CURSOR_KEYS_APPLICATION;
            Handled = TRUE;
            break;

        case 3:  /* DECCOLM – 80/132 column mode */
            Mask = VT_PRIVMODE_COLUMN_132;
            Handled = TRUE;
            RequiresWindowResize = TRUE;
            NewColumns = Enable ? 132 : 80;
            break;

        case 6:  /* DECOM – origin mode */
        {
            if (Enable)
                ScreenBuffer->VtState.PrivateModes |= VT_PRIVMODE_ORIGIN_MODE;
            else
                ScreenBuffer->VtState.PrivateModes &= ~VT_PRIVMODE_ORIGIN_MODE;

            {
                COORD Home;
                Home.X = 0;
                Home.Y = Enable ? ScreenBuffer->VtState.ScrollTop : 0;
                ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Home);
            }

            return TRUE;
        }

        case 1000:
            Mask = VT_PRIVMODE_MOUSE_X10;
            break;

        case 1003:
            Mask = VT_PRIVMODE_MOUSE_ANY_EVENT;
            break;

        case 1002:
            Mask = VT_PRIVMODE_MOUSE_BUTTON_TRACKING;
            break;

        case 1006:
            Mask = VT_PRIVMODE_MOUSE_SGR_EXTENDED;
            break;

        case 1004:
            Mask = VT_PRIVMODE_FOCUS_EVENT;
            break;

        case 2004:
            Mask = VT_PRIVMODE_BRACKETED_PASTE;
            break;

        case 1036:
            Mask = VT_PRIVMODE_META_SENDS_ESCAPE;
            break;

        case 66: /* DECNKM – keypad application mode */
            Mask = VT_PRIVMODE_KEYPAD_APPLICATION;
            Handled = TRUE;
            break;

        case 1049:
            if (Enable)
                return VtEnableAlternateScreen(Console, ScreenBuffer);
            return VtDisableAlternateScreen(Console, ScreenBuffer);

        default:
            return FALSE;
    }

    if (RequiresWindowResize)
    {
        if (NewColumns == 0)
            NewColumns = ScreenBuffer->ScreenBufferSize.X;

        if (NewColumns < 1)
            NewColumns = 1;

        if (ScreenBuffer->ScreenBufferSize.X != (SHORT)NewColumns)
        {
            ULONG Rows = (ULONG)max(1, ScreenBuffer->ScreenBufferSize.Y);
            if (!VtResizeWindow(Console, ScreenBuffer, Rows, NewColumns))
                DPRINT1("VT: Failed to resize console to %lu columns for DECCOLM\n", NewColumns);
        }

        ScreenBuffer->VtState.ScrollTop = 0;
        ScreenBuffer->VtState.ScrollBottom = max(0, ScreenBuffer->ScreenBufferSize.Y - 1);
        VtEraseDisplay(Console, ScreenBuffer, 2);
        VtResetTabStops(ScreenBuffer);

        {
            COORD Home = {0, 0};
            ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Home);
        }
    }

    if (Enable)
        ScreenBuffer->VtState.PrivateModes |= Mask;
    else
        ScreenBuffer->VtState.PrivateModes &= ~Mask;

    return Handled || (Mask != 0);
}

static BOOLEAN
VtHandleDeviceAttributes(PCONSOLE Console,
                         WCHAR PrivateIndicator)
{
    WCHAR Response[32];
    SIZE_T Len = 0;

    if (!Console)
        return FALSE;

    Response[Len++] = L'\x1b';
    Response[Len++] = L'[';

    switch (PrivateIndicator)
    {
        case 0:
            if (Len >= ARRAYSIZE(Response))
                return FALSE;
            Response[Len++] = L'?';
            Len += VtAppendNumber(Response + Len,
                                  ARRAYSIZE(Response) - Len,
                                  1);
            if (Len >= ARRAYSIZE(Response))
                return FALSE;
            Response[Len++] = L';';
            Len += VtAppendNumber(Response + Len,
                                  ARRAYSIZE(Response) - Len,
                                  0);
            break;

        case L'>':
            if (Len >= ARRAYSIZE(Response))
                return FALSE;
            Response[Len++] = L'>';
            Len += VtAppendNumber(Response + Len,
                                  ARRAYSIZE(Response) - Len,
                                  0);
            if (Len >= ARRAYSIZE(Response))
                return FALSE;
            Response[Len++] = L';';
            Len += VtAppendNumber(Response + Len,
                                  ARRAYSIZE(Response) - Len,
                                  136);
            if (Len >= ARRAYSIZE(Response))
                return FALSE;
            Response[Len++] = L';';
            Len += VtAppendNumber(Response + Len,
                                  ARRAYSIZE(Response) - Len,
                                  0);
            break;

        case L'?':
            if (Len >= ARRAYSIZE(Response))
                return FALSE;
            Response[Len++] = L'>';
            Len += VtAppendNumber(Response + Len,
                                  ARRAYSIZE(Response) - Len,
                                  83);
            if (Len >= ARRAYSIZE(Response))
                return FALSE;
            Response[Len++] = L';';
            Len += VtAppendNumber(Response + Len,
                                  ARRAYSIZE(Response) - Len,
                                  40003);
            if (Len >= ARRAYSIZE(Response))
                return FALSE;
            Response[Len++] = L';';
            Len += VtAppendNumber(Response + Len,
                                  ARRAYSIZE(Response) - Len,
                                  0);
            break;

        default:
            return FALSE;
    }

    if (Len >= ARRAYSIZE(Response))
        return FALSE;

    Response[Len++] = L'c';

    VtSendInputResponse(Console, Response, Len);
    return TRUE;
}

static BOOLEAN
VtHandlePrivateCsiSequence(PCONSOLE Console,
                           PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                           WCHAR PrivateIndicator,
                           WCHAR Final,
                           WCHAR Intermediate,
                           const ULONG *Params,
                           ULONG Count)
{
    UNREFERENCED_PARAMETER(Intermediate);

    switch (Final)
    {
        case L'h':
        case L'l':
        {
            BOOLEAN Enable = (Final == L'h');
            BOOLEAN Handled = FALSE;
            ULONG i;

            for (i = 0; i < Count; ++i)
            {
                if (VtHandleDecPrivateMode(Console, ScreenBuffer, Params[i], Enable))
                    Handled = TRUE;
            }

            return Handled;
        }

        case L'c':
            return VtHandleDeviceAttributes(Console, PrivateIndicator);

        default:
            return FALSE;
    }
}

static BOOLEAN
VtHandleCsiSequence(PCONSOLE Console,
                    PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                    WCHAR PrivateIndicator,
                    WCHAR Final,
                    WCHAR Intermediate,
                    const ULONG *Params,
                    ULONG Count)
{
    switch (Final)
    {
        case L'm':
            VtApplySgr(Console, ScreenBuffer, Params, Count);
            return TRUE;

        case L'p':
            if (Intermediate == L'!')
            {
                VtSoftReset(Console, ScreenBuffer);
                return TRUE;
            }
            VtTraceUnhandledCsi(PrivateIndicator, Intermediate, Final, Params, Count);
            return FALSE;

        case L'H':
        case L'f':
            VtHandleCursorPosition(Console, ScreenBuffer, Params, Count);
            return TRUE;

        case L'J':
            VtEraseDisplay(Console, ScreenBuffer, Params[0]);
            return TRUE;

        case L'K':
            VtEraseLine(Console, ScreenBuffer, Params[0]);
            return TRUE;

        case L'@':
            if (Intermediate == L' ')
            {
                ULONG Columns = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
                VtScrollColumns(Console, ScreenBuffer, (LONG)Columns);
                return TRUE;
            }
            VtShiftRow(Console, ScreenBuffer, (Count >= 1 && Params[0] != 0) ? (LONG)Params[0] : 1);
            return TRUE;
        case L'A':
            if (Intermediate == L' ')
            {
                ULONG Columns = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
                VtScrollColumns(Console, ScreenBuffer, -(LONG)Columns);
                return TRUE;
            }
            {
                ULONG Raw = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
                LONG Distance = (LONG)min(Raw, (ULONG)LONG_MAX);
                VtMoveCursorRelative(Console, ScreenBuffer, 0, -Distance);
            }
            return TRUE;

        case L'B':
        {
            ULONG Raw = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
            LONG Distance = (LONG)min(Raw, (ULONG)LONG_MAX);
            VtMoveCursorRelative(Console, ScreenBuffer, 0, Distance);
        }
            return TRUE;

        case L'C':
        {
            ULONG Raw = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
            LONG Distance = (LONG)min(Raw, (ULONG)LONG_MAX);
            VtMoveCursorRelative(Console, ScreenBuffer, Distance, 0);
        }
            return TRUE;

        case L'D':
        {
            ULONG Raw = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
            LONG Distance = (LONG)min(Raw, (ULONG)LONG_MAX);
            VtMoveCursorRelative(Console, ScreenBuffer, -Distance, 0);
        }
            return TRUE;

        case L'P':
            VtShiftRow(Console, ScreenBuffer, -((Count >= 1 && Params[0] != 0) ? (LONG)Params[0] : 1));
            return TRUE;

        case L'X':
            VtEraseCharacters(Console, ScreenBuffer, Params[0]);
            return TRUE;

        case L'b':
        {
            ULONG Repeat = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
            VtRepeatCharacter(Console, ScreenBuffer, Repeat);
            return TRUE;
        }

        case L'L':
            VtShiftLines(Console, ScreenBuffer, (Count >= 1 && Params[0] != 0) ? (LONG)Params[0] : 1);
            return TRUE;

        case L'M':
            VtShiftLines(Console, ScreenBuffer, -((Count >= 1 && Params[0] != 0) ? (LONG)Params[0] : 1));
            return TRUE;

        case L'c':
            if (Intermediate == 0)
                return VtHandleDeviceAttributes(Console, 0);
            VtTraceUnhandledCsi(PrivateIndicator, Intermediate, Final, Params, Count);
            return FALSE;

        case L't':
        {
            ULONG Action = Params[0];

            switch (Action)
            {
                case 8:
                    if (Count >= 3)
                    {
                        ULONG Rows = Params[1];
                        ULONG Cols = Params[2];
                        VtResizeWindow(Console, ScreenBuffer, Rows, Cols);
                        return TRUE;
                    }
                    return TRUE;

                case 18:
                    return VtReportWindowSize(Console, ScreenBuffer, 8);

                case 19:
                    return VtReportWindowSize(Console, ScreenBuffer, 4);

                default:
                    VtTraceUnhandledCsi(PrivateIndicator, Intermediate, Final, Params, Count);
                    return FALSE;
            }
        }

        case L'n':
        {
            ULONG Query = Params[0];
            WCHAR Response[32];
            SIZE_T Len = 0;

            switch (Query)
            {
                case 5:
                    Response[Len++] = L'\x1b';
                    Response[Len++] = L'[';
                    Response[Len++] = L'0';
                    Response[Len++] = L'n';
                    break;

                case 6:
                {
                    SHORT Row = ScreenBuffer->CursorPosition.Y - ScreenBuffer->ViewOrigin.Y;
                    SHORT Col = ScreenBuffer->CursorPosition.X - ScreenBuffer->ViewOrigin.X;

                    if (Row < 0) Row = 0;
                    if (Col < 0) Col = 0;

                    Response[Len++] = L'\x1b';
                    Response[Len++] = L'[';
                    Len += VtAppendNumber(Response + Len,
                                          ARRAYSIZE(Response) - Len,
                                          (ULONG)Row + 1);
                    if (Len < ARRAYSIZE(Response))
                        Response[Len++] = L';';
                    Len += VtAppendNumber(Response + Len,
                                          ARRAYSIZE(Response) - Len,
                                          (ULONG)Col + 1);
                    if (Len < ARRAYSIZE(Response))
                        Response[Len++] = L'R';
                    break;
                }

                default:
                    VtTraceUnhandledCsi(PrivateIndicator, Intermediate, Final, Params, Count);
                    return FALSE;
            }

            VtSendInputResponse(Console, Response, Len);
            return TRUE;
        }

        case L'g':
        {
            /* TBC - Tab Clear (0 = at cursor, 3 = all) */
            VtHandleTabClear(ScreenBuffer, Count >= 1 ? Params[0] : 0);
            return TRUE;
        }

        case L'I':
        {
            /* CHT - Cursor Forward Tabulation */
            ULONG Count2 = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
            VtTabForward(Console, ScreenBuffer, Count2);
            return TRUE;
        }

        case L'Z':
        {
            /* CBT - Cursor Backward Tabulation */
            ULONG Count2 = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
            VtTabBackward(Console, ScreenBuffer, Count2);
            return TRUE;
        }

        case L'S':
        {
            ULONG Lines = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
            VtScrollRegion(Console, ScreenBuffer, (LONG)Lines);
            return TRUE;
        }

        case L'T':
        {
            ULONG Lines = (Count >= 1 && Params[0] != 0) ? Params[0] : 1;
            VtScrollRegion(Console, ScreenBuffer, -(LONG)Lines);
            return TRUE;
        }

        case L'r':
        {
            SHORT Top = 0;
            SHORT Bottom = ScreenBuffer->ScreenBufferSize.Y > 0 ? ScreenBuffer->ScreenBufferSize.Y - 1 : 0;

            if (Count >= 1 && Params[0] != 0)
                Top = (SHORT)max(0L, (LONG)Params[0] - 1);
            if (Count >= 2 && Params[1] != 0)
                Bottom = (SHORT)min((LONG)Bottom, (LONG)Params[1] - 1);

            if (Top < 0)
                Top = 0;
            if (Bottom >= ScreenBuffer->ScreenBufferSize.Y)
                Bottom = ScreenBuffer->ScreenBufferSize.Y - 1;

            if (Bottom < Top)
            {
                Top = 0;
                Bottom = ScreenBuffer->ScreenBufferSize.Y > 0 ? ScreenBuffer->ScreenBufferSize.Y - 1 : 0;
            }

            ScreenBuffer->VtState.ScrollTop = Top;
            ScreenBuffer->VtState.ScrollBottom = Bottom;

            {
                COORD Home;
                Home.X = 0;
                Home.Y = Top;
                ConDrvSetConsoleCursorPosition(Console, ScreenBuffer, &Home);
            }
            return TRUE;
        }

        case L'q':
            if (Intermediate == L' ')
            {
                ULONG Style = Params[0];
                if (VtApplyCursorStyle(Console, ScreenBuffer, Style))
                    return TRUE;
            }
            VtTraceUnhandledCsi(PrivateIndicator, Intermediate, Final, Params, Count);
            return FALSE;

        default:
            VtTraceUnhandledCsi(PrivateIndicator, Intermediate, Final, Params, Count);
            return FALSE;
    }
}

static BOOLEAN
VtHandleEscapeSequence(PCONSOLE Console,
                       PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                       WCHAR Indicator,
                       PCWSTR Buffer,
                       ULONG Length,
                       PULONG Position)
{
    switch (Indicator)
    {
        case L's':
        case L'7':
            VtSaveCursorState(ScreenBuffer);
            return TRUE;

        case L'u':
        case L'8':
            VtRestoreCursorState(Console, ScreenBuffer);
            return TRUE;

        case L'c':
            VtSoftReset(Console, ScreenBuffer);
            return TRUE;

        case L'H':
            /* HTS - Horizontal Tab Set at the current column */
            VtSetTabStopAtCursor(ScreenBuffer);
            return TRUE;

        case L'#':
        {
            WCHAR Selector;

            if (!Position || *Position >= Length)
                return FALSE;

            Selector = Buffer[*Position];
            if (Selector == L'8')
            {
                (*Position)++;
                VtDecaln(Console, ScreenBuffer);
                return TRUE;
            }

            return FALSE;
        }

        case L'(':
        case L')':
        case L'*':
        case L'+':
        case L'-':
        case L'.':
        case L'/':
        {
            UCHAR Slot;
            WCHAR Intermediate;
            WCHAR Final;

            if (!Position || *Position >= Length)
                return FALSE;

            switch (Indicator)
            {
                case L'(': Slot = VT_CHARSET_SLOT_G0; break;
                case L')':
                case L'-': Slot = VT_CHARSET_SLOT_G1; break;
                case L'*':
                case L'.': Slot = VT_CHARSET_SLOT_G2; break;
                case L'+':
                case L'/': Slot = VT_CHARSET_SLOT_G3; break;
                default: return FALSE;
            }

            Final = Buffer[*Position];
            Intermediate = 0;

            while (Final >= L' ' && Final <= L'/')
            {
                Intermediate = Final;
                (*Position)++;
                if (!Position || *Position >= Length)
                    return FALSE;
                Final = Buffer[*Position];
            }

            if (VtDesignateCharset(ScreenBuffer, Slot, Intermediate, Final))
            {
                (*Position)++;
                return TRUE;
            }

            VtTraceUnhandledEscape(Indicator);
            return FALSE;
        }

        case L'N':
            ScreenBuffer->VtState.PendingSingleShift = VT_CHARSET_SLOT_G2;
            return TRUE;

        case L'O':
            ScreenBuffer->VtState.PendingSingleShift = VT_CHARSET_SLOT_G3;
            return TRUE;

        case L'n':
            VtSetActiveCharset(ScreenBuffer, VT_CHARSET_SLOT_G2);
            return TRUE;

        case L'o':
            VtSetActiveCharset(ScreenBuffer, VT_CHARSET_SLOT_G3);
            return TRUE;

        default:
            return FALSE;
    }
}

NTSTATUS
NTAPI
ConDrvVtWriteConsole(PCONSOLE Console,
                     PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                     PCWSTR Buffer,
                     ULONG Length,
                     PULONG NumCharsProcessed,
                     PBOOLEAN Handled)
{
    PWSTR CombinedBuffer = NULL;
    PCWSTR WorkingBuffer = Buffer;
    ULONG WorkingLength = Length;
    ULONG OriginalInputLength = Length;
    ULONG Pos = 0;
    ULONG SegmentStart = 0;
    ULONG Processed = 0;
    ULONG Params[VT_MAX_PARAMS];
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AnyHandled = FALSE;
    BOOLEAN HoldTail = FALSE;
    ULONG TailStart = 0;

    if (!Console || !ScreenBuffer || !Buffer)
    {
        if (NumCharsProcessed) *NumCharsProcessed = 0;
        if (Handled) *Handled = FALSE;
        return STATUS_INVALID_PARAMETER;
    }

    if (ScreenBuffer->VtState.PendingSequenceLength > 0)
    {
        ULONG PendingLength = ScreenBuffer->VtState.PendingSequenceLength;
        ULONGLONG TotalLength64 = (ULONGLONG)PendingLength + (ULONGLONG)WorkingLength;

        if (TotalLength64 > ULONG_MAX)
        {
            Status = STATUS_INTEGER_OVERFLOW;
            goto Cleanup;
        }

        CombinedBuffer = ConsoleAllocHeap(0, ((ULONG)TotalLength64 + 1) * sizeof(WCHAR));
        if (!CombinedBuffer)
        {
            Status = VtFlushText(Console,
                                 ScreenBuffer,
                                 ScreenBuffer->VtState.PendingSequence,
                                 PendingLength);
            ScreenBuffer->VtState.PendingSequenceLength = 0;
            if (!NT_SUCCESS(Status))
                goto Cleanup;
        }
        else
        {
            if (PendingLength > 0)
            {
                RtlCopyMemory(CombinedBuffer,
                              ScreenBuffer->VtState.PendingSequence,
                              PendingLength * sizeof(WCHAR));
            }

            if (WorkingLength > 0)
            {
                RtlCopyMemory(CombinedBuffer + PendingLength,
                              WorkingBuffer,
                              WorkingLength * sizeof(WCHAR));
            }

            CombinedBuffer[(ULONG)TotalLength64] = UNICODE_NULL;
            WorkingBuffer = CombinedBuffer;
            WorkingLength = (ULONG)TotalLength64;
            ScreenBuffer->VtState.PendingSequenceLength = 0;
        }
    }

    while (Pos < WorkingLength && NT_SUCCESS(Status))
    {
        WCHAR Ch = WorkingBuffer[Pos];

        if (Ch == L'' || Ch == L'')
        {
            Status = VtFlushText(Console,
                                 ScreenBuffer,
                                 WorkingBuffer + SegmentStart,
                                 Pos - SegmentStart);
            if (!NT_SUCCESS(Status))
                break;

            VtSetActiveCharset(ScreenBuffer,
                               (Ch == L'\x0e') ? VT_CHARSET_SLOT_G1
                                                 : VT_CHARSET_SLOT_G0);
            Pos++;
            SegmentStart = Pos;
            AnyHandled = TRUE;
            continue;
        }

        /*
         * An escape sequence introducer, either as ESC + a final character or as
         * the equivalent single C1 control (0x9B == CSI == ESC [, 0x9D == OSC ==
         * ESC ], 0x90 == DCS == ESC P, ...). A C1 code point is exactly its
         * two-character form with 0x40 subtracted, so both feed one dispatcher
         * and no pre-pass has to rewrite the input.
         */
        if (Ch == L'' || (Ch >= 0x90 && Ch <= 0x9F))
        {
            ULONG EscStart = Pos;
            BOOLEAN IsC1 = (Ch != L'');

            /* A bare trailing ESC may still be completed by the next write; a
             * single C1 code point is already complete. */
            if (!IsC1 && Pos + 1 >= WorkingLength)
            {
                HoldTail = TRUE;
                TailStart = EscStart;
                break;
            }

            Status = VtFlushText(Console,
                                 ScreenBuffer,
                                 WorkingBuffer + SegmentStart,
                                 EscStart - SegmentStart);
            if (!NT_SUCCESS(Status))
                break;

            if (IsC1)
            {
                Ch = (WCHAR)(Ch - 0x40);
                Pos++;
            }
            else
            {
                Pos++;
                Ch = WorkingBuffer[Pos++];
            }

            if (Ch == L'[')
            {
                WCHAR Final = 0;
                ULONG Count = 0;
                ULONG Current = 0;
                BOOLEAN HaveCurrent = FALSE;
                WCHAR PrivateIndicator = 0;
                WCHAR Intermediate = 0;

                if (Pos < WorkingLength &&
                    (WorkingBuffer[Pos] == L'?' || WorkingBuffer[Pos] == L'>' ||
                     WorkingBuffer[Pos] == L'<' || WorkingBuffer[Pos] == L'='))
                {
                    PrivateIndicator = WorkingBuffer[Pos];
                    Pos++;
                }

                while (Pos < WorkingLength)
                {
                    WCHAR C = WorkingBuffer[Pos];
                    if (C >= L'0' && C <= L'9')
                    {
                        ULONG Digit = (ULONG)(C - L'0');
                        HaveCurrent = TRUE;
                        if (Current > (ULONG_MAX - Digit) / 10)
                            Current = ULONG_MAX;
                        else
                            Current = Current * 10 + Digit;
                        Pos++;
                        continue;
                    }
                    if (C == L';')
                    {
                        if (Count < VT_MAX_PARAMS)
                            Params[Count++] = HaveCurrent ? Current : 0;
                        Current = 0;
                        HaveCurrent = FALSE;
                        Pos++;
                        continue;
                    }

                    if (C >= L' ' && C <= L'/')
                    {
                        Intermediate = C;
                        Pos++;
                        continue;
                    }

                    Final = C;
                    Pos++;
                    break;
                }

                if (Final == 0)
                {
                    HoldTail = TRUE;
                    TailStart = EscStart;
                    break;
                }

                /*
                 * Always leaves at least one parameter: an empty parameter list
                 * yields a single 0. Handlers below can therefore read Params[0]
                 * without testing Count.
                 */
                if (HaveCurrent || Count == 0)
                {
                    if (Count < VT_MAX_PARAMS)
                        Params[Count++] = HaveCurrent ? Current : 0;
                }

                if ((PrivateIndicator != 0 &&
                     VtHandlePrivateCsiSequence(Console, ScreenBuffer, PrivateIndicator, Final, Intermediate, Params, Count)) ||
                    (PrivateIndicator == 0 &&
                     VtHandleCsiSequence(Console, ScreenBuffer, PrivateIndicator, Final, Intermediate, Params, Count)))
                {
                    AnyHandled = TRUE;
                    SegmentStart = Pos;
                    continue;
                }

                /* Unsupported CSI sequence, emit literally */
                Status = VtFlushText(Console,
                                     ScreenBuffer,
                                     WorkingBuffer + EscStart,
                                     Pos - EscStart);
                SegmentStart = Pos;
                continue;
            }
            else if (Ch == L']')
            {
                ULONG OscStart = Pos;
                ULONG SearchPos = Pos;
                ULONG TerminatorLen = 0;

                while (SearchPos < WorkingLength)
                {
                    WCHAR C = WorkingBuffer[SearchPos];
                    if (C == L'\a')
                    {
                        TerminatorLen = 1;
                        break;
                    }
                    if (C == L'\x1b' && (SearchPos + 1) < WorkingLength && WorkingBuffer[SearchPos + 1] == L'\\')
                    {
                        TerminatorLen = 2;
                        break;
                    }
                    SearchPos++;
                }

                if (TerminatorLen == 0)
                {
                    HoldTail = TRUE;
                    TailStart = EscStart;
                    break;
                }

                VtHandleOscSequence(Console, ScreenBuffer, WorkingBuffer + OscStart, SearchPos - OscStart);

                AnyHandled = TRUE;
                Pos = SearchPos + TerminatorLen;
                SegmentStart = Pos;
                continue;
            }
            else if (Ch == L'P')
            {
                ULONG SearchPos = Pos;
                ULONG TerminatorLen = 0;

                while (SearchPos < WorkingLength)
                {
                    WCHAR C = WorkingBuffer[SearchPos];
                    if (C == L'\x1b' && (SearchPos + 1) < WorkingLength && WorkingBuffer[SearchPos + 1] == L'\\')
                    {
                        TerminatorLen = 2;
                        break;
                    }
                    SearchPos++;
                }

                if (TerminatorLen == 0)
                {
                    HoldTail = TRUE;
                    TailStart = EscStart;
                    break;
                }

                VtHandleDcsSequence(Console, ScreenBuffer, WorkingBuffer + Pos, SearchPos - Pos);

                AnyHandled = TRUE;
                Pos = SearchPos + TerminatorLen;
                SegmentStart = Pos;
                continue;
            }
            else
            {
                ULONG NewPos = Pos;
                if (VtHandleEscapeSequence(Console,
                                           ScreenBuffer,
                                           Ch,
                                           WorkingBuffer,
                                           WorkingLength,
                                           &NewPos))
                {
                    AnyHandled = TRUE;
                    Pos = NewPos;
                    SegmentStart = Pos;
                    continue;
                }

                if ((Ch == L'#' ||
                     Ch == L'(' || Ch == L')' || Ch == L'*' || Ch == L'+' ||
                     Ch == L'-' || Ch == L'.' || Ch == L'/') &&
                    Pos >= WorkingLength)
                {
                    HoldTail = TRUE;
                    TailStart = EscStart;
                    break;
                }

                /* Unsupported ESC sequence, emit literally */
                Status = VtFlushText(Console,
                                     ScreenBuffer,
                                     WorkingBuffer + EscStart,
                                     Pos - EscStart);
                SegmentStart = Pos;
                continue;
            }
        }

        Pos++;
    }

    if (NT_SUCCESS(Status))
    {
        if (HoldTail)
        {
            if (TailStart > SegmentStart)
            {
                Status = VtFlushText(Console,
                                     ScreenBuffer,
                                     WorkingBuffer + SegmentStart,
                                     TailStart - SegmentStart);
            }

            if (NT_SUCCESS(Status))
            {
                ULONG TailLength = WorkingLength - TailStart;

                if (!VtStorePendingSequence(ScreenBuffer,
                                            WorkingBuffer + TailStart,
                                            TailLength))
                {
                    Status = VtFlushText(Console,
                                         ScreenBuffer,
                                         WorkingBuffer + TailStart,
                                         TailLength);
                    ScreenBuffer->VtState.PendingSequenceLength = 0;
                }
            }
        }
        else
        {
            Status = VtFlushText(Console,
                                 ScreenBuffer,
                                 WorkingBuffer + SegmentStart,
                                 WorkingLength - SegmentStart);
        }

        if (NT_SUCCESS(Status))
        {
            Processed = OriginalInputLength;
            AnyHandled = TRUE;
        }
    }

    if (!NT_SUCCESS(Status))
        AnyHandled = FALSE;

Cleanup:
    /* One invalidation for everything this write touched */
    VtFlushDirty(Console, ScreenBuffer);

    if (CombinedBuffer)
        ConsoleFreeHeap(CombinedBuffer);

    if (NumCharsProcessed) *NumCharsProcessed = Processed;
    if (Handled) *Handled = AnyHandled;

    return Status;
}

typedef enum _VT_INPUT_ACTION
{
    VtInputPassThrough = 0,
    VtInputGenerateSequence,
    VtInputDrop
} VT_INPUT_ACTION;

typedef struct _VT_INPUT_TRANSLATION
{
    VT_INPUT_ACTION Action;
    USHORT RepeatCount;
    SIZE_T SequenceLength;
    WCHAR Sequence[VT_MAX_INPUT_SEQUENCE_CHARS];
} VT_INPUT_TRANSLATION, *PVT_INPUT_TRANSLATION;

static BOOLEAN
VtShouldHandleKeyUp(USHORT VirtualKey)
{
    switch (VirtualKey)
    {
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_HOME:
        case VK_END:
        case VK_INSERT:
        case VK_DELETE:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_F1:
        case VK_F2:
        case VK_F3:
        case VK_F4:
        case VK_F5:
        case VK_F6:
        case VK_F7:
        case VK_F8:
        case VK_F9:
        case VK_F10:
        case VK_F11:
        case VK_F12:
            return TRUE;
    }

    return FALSE;
}

static BOOLEAN
VtTranslateKeyEvent(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                    const KEY_EVENT_RECORD *KeyEvent,
                    PVT_INPUT_TRANSLATION Translation)
{
    ULONG PrivateModes = ScreenBuffer ? ScreenBuffer->VtState.PrivateModes : 0;
    SIZE_T Length = 0;
    USHORT Repeat = (KeyEvent->wRepeatCount != 0) ? KeyEvent->wRepeatCount : 1;

    if (!KeyEvent->bKeyDown)
    {
        if (VtShouldHandleKeyUp(KeyEvent->wVirtualKeyCode))
        {
            Translation->Action = VtInputDrop;
            Translation->RepeatCount = 0;
            Translation->SequenceLength = 0;
            return TRUE;
        }

        return FALSE;
    }

    if (KeyEvent->wVirtualKeyCode == 0 && KeyEvent->uChar.UnicodeChar != 0)
        return FALSE;

    /* Shift+Tab generates CSI Z (backtab) */
    if (KeyEvent->wVirtualKeyCode == VK_TAB &&
        (KeyEvent->dwControlKeyState & SHIFT_PRESSED))
    {
        Translation->Action = VtInputGenerateSequence;
        Translation->RepeatCount = Repeat;
        Translation->Sequence[0] = L'\x1b';
        Translation->Sequence[1] = L'[';
        Translation->Sequence[2] = L'Z';
        Translation->SequenceLength = 3;
        return TRUE;
    }

    /* VT terminals send DEL (0x7f) for Backspace */
    if (KeyEvent->wVirtualKeyCode == VK_BACK)
    {
        if ((PrivateModes & VT_PRIVMODE_META_SENDS_ESCAPE) &&
            (KeyEvent->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))
        {
            Translation->Action = VtInputGenerateSequence;
            Translation->RepeatCount = Repeat;
            Translation->Sequence[0] = L'\x1b';
            Translation->Sequence[1] = L'\x7f';
            Translation->SequenceLength = 2;
            return TRUE;
        }

        Translation->Action = VtInputGenerateSequence;
        Translation->RepeatCount = Repeat;
        Translation->Sequence[0] = L'\x7f';
        Translation->SequenceLength = 1;
        return TRUE;
    }

    if ((PrivateModes & VT_PRIVMODE_META_SENDS_ESCAPE) &&
        (KeyEvent->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) &&
        KeyEvent->uChar.UnicodeChar != 0)
    {
        Translation->Action = VtInputGenerateSequence;
        Translation->RepeatCount = Repeat;
        Translation->Sequence[0] = L'\x1b';
        Translation->Sequence[1] = KeyEvent->uChar.UnicodeChar;
        Translation->SequenceLength = 2;
        return TRUE;
    }

    if (KeyEvent->uChar.UnicodeChar != 0)
        return FALSE;

    /*
     * Every remaining key is a table lookup plus one compose call. The three
     * groups differ only in which encoding they use:
     *   - cursor/SS3 keys: ESC O <final> in application mode, CSI <final> otherwise
     *   - editing and F5-F12 keys: CSI <param> ~
     *   - the numeric keypad in application mode: ESC O <final>
     */
    {
        static const struct
        {
            USHORT Vk;
            WCHAR Final;
            ULONG AppModeMask;  /* 0 = always use the application (SS3) form */
        }
        CursorKeys[] =
        {
            {VK_UP,    L'A', VT_PRIVMODE_CURSOR_KEYS_APPLICATION},
            {VK_DOWN,  L'B', VT_PRIVMODE_CURSOR_KEYS_APPLICATION},
            {VK_RIGHT, L'C', VT_PRIVMODE_CURSOR_KEYS_APPLICATION},
            {VK_LEFT,  L'D', VT_PRIVMODE_CURSOR_KEYS_APPLICATION},
            {VK_HOME,  L'H', VT_PRIVMODE_KEYPAD_APPLICATION},
            {VK_END,   L'F', VT_PRIVMODE_KEYPAD_APPLICATION},
            {VK_F1,    L'P', 0},
            {VK_F2,    L'Q', 0},
            {VK_F3,    L'R', 0},
            {VK_F4,    L'S', 0},
        };

        static const struct
        {
            USHORT Vk;
            ULONG Parameter;
        }
        TildeKeys[] =
        {
            {VK_INSERT, 2},
            {VK_DELETE, 3},
            {VK_PRIOR,  5},   /* Page Up */
            {VK_NEXT,   6},   /* Page Down */
            {VK_F5,  15}, {VK_F6,  17}, {VK_F7,  18}, {VK_F8,  19},
            {VK_F9,  20}, {VK_F10, 21}, {VK_F11, 23}, {VK_F12, 24},
        };

        static const struct
        {
            USHORT Vk;
            WCHAR Final;
        }
        KeypadKeys[] =
        {
            {VK_NUMPAD0, L'p'}, {VK_NUMPAD1, L'q'}, {VK_NUMPAD2, L'r'},
            {VK_NUMPAD3, L's'}, {VK_NUMPAD4, L't'}, {VK_NUMPAD5, L'u'},
            {VK_NUMPAD6, L'v'}, {VK_NUMPAD7, L'w'}, {VK_NUMPAD8, L'x'},
            {VK_NUMPAD9, L'y'}, {VK_DECIMAL, L'n'}, {VK_ADD,     L'k'},
            {VK_SUBTRACT, L'm'}, {VK_MULTIPLY, L'j'}, {VK_DIVIDE, L'o'},
            {VK_RETURN,  L'M'},
        };

        USHORT Vk = KeyEvent->wVirtualKeyCode;
        ULONG i;
        BOOLEAN Matched = FALSE;

        for (i = 0; i < ARRAYSIZE(CursorKeys) && !Matched; ++i)
        {
            if (CursorKeys[i].Vk != Vk) continue;

            if (!VtComposeCursorKeySequence(CursorKeys[i].AppModeMask == 0 || (PrivateModes & CursorKeys[i].AppModeMask) != 0,
                                            KeyEvent,
                                            CursorKeys[i].Final,
                                            Translation->Sequence,
                                            ARRAYSIZE(Translation->Sequence),
                                            &Length))
            {
                return FALSE;
            }
            Matched = TRUE;
        }

        for (i = 0; i < ARRAYSIZE(TildeKeys) && !Matched; ++i)
        {
            if (TildeKeys[i].Vk != Vk) continue;

            if (!VtComposeCsiTildeSequence(TildeKeys[i].Parameter,
                                           VtGetModifierParameter(KeyEvent),
                                           L'~',
                                           Translation->Sequence,
                                           ARRAYSIZE(Translation->Sequence),
                                           &Length))
            {
                return FALSE;
            }
            Matched = TRUE;
        }

        for (i = 0; i < ARRAYSIZE(KeypadKeys) && !Matched; ++i)
        {
            if (KeypadKeys[i].Vk != Vk) continue;

            /* The keypad only differs from ordinary keys in application mode,
             * carries no modifier encoding, and Enter must be the keypad one */
            if (!(PrivateModes & VT_PRIVMODE_KEYPAD_APPLICATION))
                return FALSE;
            if (VtGetModifierParameter(KeyEvent) != 1)
                return FALSE;
            if (Vk == VK_RETURN && !(KeyEvent->dwControlKeyState & ENHANCED_KEY))
                return FALSE;
            if (ARRAYSIZE(Translation->Sequence) < 3)
                return FALSE;

            Translation->Sequence[0] = L'';
            Translation->Sequence[1] = L'O';
            Translation->Sequence[2] = KeypadKeys[i].Final;
            Length = 3;
            Matched = TRUE;
        }

        if (!Matched)
            return FALSE;
    }

    Translation->Action = VtInputGenerateSequence;
    Translation->RepeatCount = Repeat;
    Translation->SequenceLength = Length;
    return TRUE;
}

static BOOLEAN
VtTranslateMouseEvent(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                      const MOUSE_EVENT_RECORD *MouseEvent,
                      PVT_INPUT_TRANSLATION Translation)
{
    ULONG PrivateModes;
    ULONG PreviousState;
    ULONG CurrentState;
    ULONG ButtonCode = 0;
    ULONG X;
    ULONG Y;
    SIZE_T Length = 0;
    BOOLEAN Released = FALSE;
    BOOLEAN UseSgr;
    ULONG Modifiers;
    BOOLEAN TrackX10;
    BOOLEAN TrackButtons;
    BOOLEAN TrackAny;

    if (!ScreenBuffer)
        return FALSE;

    PrivateModes = ScreenBuffer->VtState.PrivateModes;
    TrackX10 = (PrivateModes & VT_PRIVMODE_MOUSE_X10) != 0;
    TrackButtons = (PrivateModes & VT_PRIVMODE_MOUSE_BUTTON_TRACKING) != 0;
    TrackAny = (PrivateModes & VT_PRIVMODE_MOUSE_ANY_EVENT) != 0;

    if (!TrackX10 && !TrackButtons && !TrackAny)
        return FALSE;

    PreviousState = ScreenBuffer->VtState.MouseButtonState;
    CurrentState = MouseEvent->dwButtonState & 0xFFFF;
    X = (ULONG)MouseEvent->dwMousePosition.X + 1; /* Convert to 1-based */
    Y = (ULONG)MouseEvent->dwMousePosition.Y + 1;
    UseSgr = (PrivateModes & VT_PRIVMODE_MOUSE_SGR_EXTENDED) != 0;
    Modifiers = VtMouseModifierBits(MouseEvent->dwControlKeyState);

    if (MouseEvent->dwEventFlags == MOUSE_WHEELED ||
        MouseEvent->dwEventFlags == MOUSE_HWHEELED)
    {
        SHORT Delta = (SHORT)HIWORD(MouseEvent->dwButtonState);

        if (Delta == 0)
            return FALSE;

        if (MouseEvent->dwEventFlags == MOUSE_WHEELED)
            ButtonCode = (Delta > 0) ? 64 : 65;
        else
            ButtonCode = (Delta > 0) ? 66 : 67;

        ButtonCode += Modifiers;

        if (UseSgr)
        {
            if (!VtComposeMouseSgrSequence(ButtonCode, X, Y, FALSE,
                                           Translation->Sequence,
                                           ARRAYSIZE(Translation->Sequence),
                                           &Length))
                return FALSE;
        }
        else
        {
            if (!VtComposeMouseLegacySequence(ButtonCode, X, Y,
                                              Translation->Sequence,
                                              ARRAYSIZE(Translation->Sequence),
                                              &Length))
                return FALSE;
        }

        Translation->Action = VtInputGenerateSequence;
        Translation->RepeatCount = 1;
        Translation->SequenceLength = Length;
        ScreenBuffer->VtState.MouseButtonState = CurrentState;
        return TRUE;
    }

    if (MouseEvent->dwEventFlags == MOUSE_MOVED)
    {
        if (!TrackAny && !(TrackButtons && CurrentState != 0))
            return FALSE;

        if (CurrentState != 0)
        {
            INT ButtonIndex = VtFirstMouseButton(CurrentState);
            if (ButtonIndex < 0)
                return FALSE;

            ButtonCode = (ULONG)ButtonIndex;
        }
        else
        {
            ButtonCode = 3; /* Motion with no buttons */
        }
    }
    else
    {
        ULONG Changed = PreviousState ^ CurrentState;
        ULONG Pressed = Changed & CurrentState;
        ULONG ReleasedMask = Changed & ~CurrentState;

        if (Pressed != 0)
        {
            INT ButtonIndex = VtFirstMouseButton(Pressed);
            if (ButtonIndex < 0)
                ButtonIndex = VtFirstMouseButton(CurrentState);
            if (ButtonIndex < 0)
                return FALSE;

            ButtonCode = (ULONG)ButtonIndex;
        }
        else if (ReleasedMask != 0)
        {
            INT ButtonIndex = VtFirstMouseButton(ReleasedMask);
            if (ButtonIndex < 0)
                ButtonIndex = VtFirstMouseButton(PreviousState);
            if (ButtonIndex < 0)
                return FALSE;

            ButtonCode = (ULONG)ButtonIndex;
            Released = TRUE;
        }
        else
        {
            if (MouseEvent->dwEventFlags == DOUBLE_CLICK)
            {
                INT ButtonIndex = VtFirstMouseButton(CurrentState);
                if (ButtonIndex < 0)
                    return FALSE;

                ButtonCode = (ULONG)ButtonIndex;
            }
            else
            {
                return FALSE;
            }
        }
    }

    ButtonCode += Modifiers;

    if (UseSgr)
    {
        ULONG Code = ButtonCode;

        if (MouseEvent->dwEventFlags == MOUSE_MOVED)
            Code |= 32;

        if (!VtComposeMouseSgrSequence(Code, X, Y, Released,
                                       Translation->Sequence,
                                       ARRAYSIZE(Translation->Sequence),
                                       &Length))
            return FALSE;
    }
    else
    {
        ULONG Code = ButtonCode;

        if (Released)
            Code = 3 + Modifiers;
        else if (MouseEvent->dwEventFlags == MOUSE_MOVED)
            Code += 32;

        if (!VtComposeMouseLegacySequence(Code, X, Y,
                                          Translation->Sequence,
                                          ARRAYSIZE(Translation->Sequence),
                                          &Length))
            return FALSE;
    }

    ScreenBuffer->VtState.MouseButtonState = CurrentState;

    Translation->Action = VtInputGenerateSequence;
    Translation->RepeatCount = 1;
    Translation->SequenceLength = Length;
    return TRUE;
}

static BOOLEAN
VtTranslateFocusEvent(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                      const FOCUS_EVENT_RECORD *FocusEvent,
                      PVT_INPUT_TRANSLATION Translation)
{
    if (!ScreenBuffer)
        return FALSE;

    if (!(ScreenBuffer->VtState.PrivateModes & VT_PRIVMODE_FOCUS_EVENT))
        return FALSE;

    if (ARRAYSIZE(Translation->Sequence) < 3)
        return FALSE;

    Translation->Sequence[0] = L'\x1b';
    Translation->Sequence[1] = L'[';
    Translation->Sequence[2] = FocusEvent->bSetFocus ? L'I' : L'O';
    Translation->Action = VtInputGenerateSequence;
    Translation->RepeatCount = 1;
    Translation->SequenceLength = 3;
    return TRUE;
}

static VOID
VtFillKeyEvent(PINPUT_RECORD Destination,
               WCHAR Character,
               BOOLEAN KeyDown)
{
    RtlZeroMemory(Destination, sizeof(*Destination));
    Destination->EventType = KEY_EVENT;
    Destination->Event.KeyEvent.bKeyDown = KeyDown;
    Destination->Event.KeyEvent.wRepeatCount = 1;
    Destination->Event.KeyEvent.uChar.UnicodeChar = Character;
}

BOOLEAN
NTAPI
ConDrvVtIsMouseTrackingEnabled(PCONSOLE Console)
{
    PTEXTMODE_SCREEN_BUFFER ScreenBuffer;
    ULONG PrivateModes;

    if (!Console ||
        !(Console->InputBuffer.Mode & ENABLE_VIRTUAL_TERMINAL_INPUT) ||
        !Console->ActiveBuffer ||
        GetType(Console->ActiveBuffer) != TEXTMODE_BUFFER)
    {
        return FALSE;
    }

    ScreenBuffer = (PTEXTMODE_SCREEN_BUFFER)Console->ActiveBuffer;
    PrivateModes = ScreenBuffer->VtState.PrivateModes;
    return !!(PrivateModes & (VT_PRIVMODE_MOUSE_X10 |
                              VT_PRIVMODE_MOUSE_BUTTON_TRACKING |
                              VT_PRIVMODE_MOUSE_ANY_EVENT));
}

NTSTATUS
NTAPI
ConDrvVtTranslateInput(PCONSOLE Console,
                       PINPUT_RECORD InputRecords,
                       ULONG NumRecords,
                       PINPUT_RECORD *TranslatedRecords,
                       PULONG TranslatedCount,
                       PBOOLEAN AllocatedBuffer)
{
    PTEXTMODE_SCREEN_BUFFER ScreenBuffer;
    PVT_INPUT_TRANSLATION TranslationInfo = NULL;
    PINPUT_RECORD OutputRecords = NULL;
    ULONGLONG OutputCount64 = 0;
    ULONG Index;
    BOOLEAN AnyTranslated = FALSE;

    if (!TranslatedRecords || !TranslatedCount || !AllocatedBuffer)
        return STATUS_INVALID_PARAMETER;

    *TranslatedRecords = InputRecords;
    *TranslatedCount = NumRecords;
    *AllocatedBuffer = FALSE;

    if (Console == NULL || InputRecords == NULL || NumRecords == 0)
        return STATUS_SUCCESS;

    if (!(Console->InputBuffer.Mode & ENABLE_VIRTUAL_TERMINAL_INPUT))
        return STATUS_SUCCESS;

    ScreenBuffer = (Console->ActiveBuffer &&
                    GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
                       ? (PTEXTMODE_SCREEN_BUFFER)Console->ActiveBuffer
                       : NULL;

    TranslationInfo = ConsoleAllocHeap(0, sizeof(VT_INPUT_TRANSLATION) * NumRecords);
    if (!TranslationInfo)
        return STATUS_NO_MEMORY;

    for (Index = 0; Index < NumRecords; ++Index)
    {
        const INPUT_RECORD *Record = &InputRecords[Index];
        PVT_INPUT_TRANSLATION Info = &TranslationInfo[Index];

        Info->Action = VtInputPassThrough;
        Info->RepeatCount = 0;
        Info->SequenceLength = 0;

        switch (Record->EventType)
        {
            case KEY_EVENT:
                if (VtTranslateKeyEvent(ScreenBuffer, &Record->Event.KeyEvent, Info))
                {
                    AnyTranslated = TRUE;

                    if (Info->Action == VtInputGenerateSequence)
                    {
                        ULONGLONG Repeat = Info->RepeatCount ? Info->RepeatCount : 1;
                        ULONGLONG SequenceLength = Info->SequenceLength;
                        OutputCount64 += SequenceLength * 2ull * Repeat;
                    }
                    else if (Info->Action == VtInputPassThrough)
                    {
                        OutputCount64 += 1;
                    }
                    /* Drop case contributes nothing */
                }
                else
                {
                    OutputCount64 += 1;
                }
                break;

            case MOUSE_EVENT:
                if (VtTranslateMouseEvent(ScreenBuffer, &Record->Event.MouseEvent, Info))
                {
                    AnyTranslated = TRUE;

                    if (Info->Action == VtInputGenerateSequence)
                    {
                        ULONGLONG SequenceLength = Info->SequenceLength;
                        OutputCount64 += SequenceLength * 2ull;
                    }
                    else if (Info->Action == VtInputPassThrough)
                    {
                        OutputCount64 += 1;
                    }
                }
                else
                {
                    OutputCount64 += 1;
                }
                break;

            case FOCUS_EVENT:
                if (VtTranslateFocusEvent(ScreenBuffer, &Record->Event.FocusEvent, Info))
                {
                    AnyTranslated = TRUE;

                    if (Info->Action == VtInputGenerateSequence)
                    {
                        ULONGLONG SequenceLength = Info->SequenceLength;
                        OutputCount64 += SequenceLength * 2ull;
                    }
                    else if (Info->Action == VtInputPassThrough)
                    {
                        OutputCount64 += 1;
                    }
                }
                else
                {
                    OutputCount64 += 1;
                }
                break;

            default:
                OutputCount64 += 1;
                break;
        }
    }

    if (!AnyTranslated)
    {
        ConsoleFreeHeap(TranslationInfo);
        return STATUS_SUCCESS;
    }

    if (OutputCount64 > ULONG_MAX)
    {
        ConsoleFreeHeap(TranslationInfo);
        return STATUS_INTEGER_OVERFLOW;
    }

    if (OutputCount64 == 0)
    {
        ConsoleFreeHeap(TranslationInfo);
        *TranslatedRecords = NULL;
        *TranslatedCount = 0;
        *AllocatedBuffer = FALSE;
        return STATUS_SUCCESS;
    }

    OutputRecords = ConsoleAllocHeap(0, sizeof(INPUT_RECORD) * (ULONG)OutputCount64);
    if (!OutputRecords)
    {
        ConsoleFreeHeap(TranslationInfo);
        return STATUS_NO_MEMORY;
    }

    {
        ULONG OutIndex = 0;

        for (Index = 0; Index < NumRecords; ++Index)
        {
            const INPUT_RECORD *Record = &InputRecords[Index];
            PVT_INPUT_TRANSLATION Info = &TranslationInfo[Index];

            if (Info->Action == VtInputPassThrough)
            {
                OutputRecords[OutIndex++] = *Record;
                continue;
            }

            if (Info->Action == VtInputDrop)
                continue;

            if (Info->Action == VtInputGenerateSequence && Info->SequenceLength > 0)
            {
                ULONG Repeat;

                for (Repeat = 0; Repeat < (Info->RepeatCount ? Info->RepeatCount : 1); ++Repeat)
                {
                    SIZE_T SeqIndex;

                    for (SeqIndex = 0; SeqIndex < Info->SequenceLength; ++SeqIndex)
                    {
                        WCHAR Ch = Info->Sequence[SeqIndex];
                        VtFillKeyEvent(&OutputRecords[OutIndex++], Ch, TRUE);
                        VtFillKeyEvent(&OutputRecords[OutIndex++], Ch, FALSE);
                    }
                }
                continue;
            }

            OutputRecords[OutIndex++] = *Record;
        }
    }

    ConsoleFreeHeap(TranslationInfo);

    *TranslatedRecords = OutputRecords;
    *TranslatedCount = (ULONG)OutputCount64;
    *AllocatedBuffer = TRUE;
    return STATUS_SUCCESS;
}
