

#include <freeldr.h>
#include <debug.h>
/* Used to store the current X and Y position on the screen */
static ULONG i386_ScreenPosX = 0;
static ULONG i386_ScreenPosY = 0;
#define SCREEN_ATTR 0x1F    // Bright white on blue background

static void
i386PrintText(CHAR *pszText)
{
    ULONG Width, Unused;

    MachVideoGetDisplaySize(&Width, &Unused, &Unused);

    for (; *pszText != ANSI_NULL; ++pszText)
    {
        if (*pszText == '\n')
        {
            i386_ScreenPosX = 0;
            ++i386_ScreenPosY;
            continue;
        }

        MachVideoPutChar(*pszText, SCREEN_ATTR, i386_ScreenPosX, i386_ScreenPosY);
        if (++i386_ScreenPosX >= Width)
        {
            i386_ScreenPosX = 0;
            ++i386_ScreenPosY;
        }
    // FIXME: Implement vertical screen scrolling if we are at the end of the screen.
    }
}

static void
PrintTextV(const CHAR *Format, va_list args)
{
    CHAR Buffer[512];

    _vsnprintf(Buffer, sizeof(Buffer), Format, args);
    Buffer[sizeof(Buffer) - 1] = ANSI_NULL;

    i386PrintText(Buffer);
}

static void
PrintText(const CHAR *Format, ...)
{
    va_list argptr;

    va_start(argptr, Format);
    PrintTextV(Format, argptr);
    va_end(argptr);
}

VOID
FrLdrBugCheckWithMessage(
    ULONG BugCode,
    PCHAR File,
    ULONG Line,
    PSTR Format,
    ...)
{
    va_list argptr;

    MachVideoHideShowTextCursor(FALSE);
    MachVideoClearScreen(SCREEN_ATTR);
    i386_ScreenPosX = 0;
    i386_ScreenPosY = 0;

    PrintText("A problem has been detected and FreeLoader boot has been aborted.\n\n");

    PrintText("%ld: %s\n\n", BugCode, BugCodeStrings[BugCode]);

    if (File)
    {
        PrintText("Location: %s:%ld\n\n", File, Line);
    }

    va_start(argptr, Format);
    PrintTextV(Format, argptr);
    va_end(argptr);
    for (;;);
}
