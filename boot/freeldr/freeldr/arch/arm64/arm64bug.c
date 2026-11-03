/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 bugcheck screen
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <freeldr.h>

#include <debug.h>

#define SCREEN_ATTR 0x1F    // Bright white on blue background

static ULONG ScreenPosX;
static ULONG ScreenPosY;

static
VOID
PrintString(
    _In_ PCSTR Text)
{
    ULONG Width, Unused;

    MachVideoGetDisplaySize(&Width, &Unused, &Unused);

    for (; *Text != ANSI_NULL; Text++)
    {
        if (*Text == '\n' || ScreenPosX >= Width)
        {
            ScreenPosX = 0;
            ScreenPosY++;
            if (*Text == '\n')
                continue;
        }

        MachVideoPutChar(*Text, SCREEN_ATTR, ScreenPosX++, ScreenPosY);
    }
}

static
VOID
PrintTextV(
    _In_ PCSTR Format,
    _In_ va_list Args)
{
    CHAR Buffer[512];

    _vsnprintf(Buffer, sizeof(Buffer), Format, Args);
    Buffer[sizeof(Buffer) - 1] = ANSI_NULL;

    DbgPrint("%s", Buffer);
    PrintString(Buffer);
}

static
VOID
PrintText(
    _In_ PCSTR Format,
    ...)
{
    va_list Args;

    va_start(Args, Format);
    PrintTextV(Format, Args);
    va_end(Args);
}

static
VOID
PrintHeader(
    _In_ ULONG BugCode,
    _In_opt_ PCHAR File,
    _In_ ULONG Line)
{
    MachVideoHideShowTextCursor(FALSE);
    MachVideoClearScreen(SCREEN_ATTR);
    ScreenPosX = 0;
    ScreenPosY = 0;

    PrintText("A problem has been detected and FreeLoader boot has been aborted.\n\n");
    PrintText("%ld: %s\n\n", BugCode, BugCodeStrings[BugCode]);

    if (File)
        PrintText("Location: %s:%ld\n\n", File, Line);
}

static
DECLSPEC_NORETURN
VOID
Halt(VOID)
{
    _disable();
    for (;;)
        __wfi();
}

DECLSPEC_NORETURN
VOID
FrLdrBugCheckWithMessage(
    _In_ ULONG BugCode,
    _In_opt_ PCHAR File,
    _In_ ULONG Line,
    _In_ PCSTR Format,
    ...)
{
    va_list Args;

    PrintHeader(BugCode, File, Line);

    va_start(Args, Format);
    PrintTextV(Format, Args);
    va_end(Args);

    Halt();
}

DECLSPEC_NORETURN
VOID
NTAPI
FrLdrBugCheck(
    _In_ ULONG BugCode)
{
    PrintHeader(BugCode, NULL, 0);

    PrintText("Bug Information:\n    %p\n    %p\n    %p\n    %p\n    %p\n\n",
              BugCheckInfo[0], BugCheckInfo[1], BugCheckInfo[2], BugCheckInfo[3], BugCheckInfo[4]);

    Halt();
}
