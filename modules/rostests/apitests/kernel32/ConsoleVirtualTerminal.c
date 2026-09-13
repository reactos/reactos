/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     End-to-end tests for the Win32 virtual terminal path
 */

#include "precomp.h"
#include <winuser.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#ifndef DISABLE_NEWLINE_AUTO_RETURN
#define DISABLE_NEWLINE_AUTO_RETURN 0x0008
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif

#ifndef CONSOLE_READ_NOWAIT
#define CONSOLE_READ_NOWAIT 0x0002
#endif

#define EDIT_INPUT_MODE \
    (ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_VIRTUAL_TERMINAL_INPUT)

#define EDIT_OUTPUT_MODE \
    (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | \
     ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN)

#define INPUT_RECORD_COUNT 4096

typedef BOOL
(WINAPI *PREAD_CONSOLE_INPUT_EX_W)(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead,
    WORD wFlags);

static BOOL
WriteVt(HANDLE Output,
        const char *Buffer,
        DWORD Length)
{
    BOOL Success;
    DWORD Written = 0;

    SetLastError(0xdeadbeef);
    Success = WriteFile(Output, Buffer, Length, &Written, NULL);
    ok(Success, "WriteFile failed with error %lu\n", GetLastError());
    ok(Written == Length, "WriteFile wrote %lu of %lu bytes\n", Written, Length);
    return Success && Written == Length;
}

#define WRITE_VT(Output, String) WriteVt((Output), (String), sizeof(String) - 1)

static BOOL
ClearScreen(HANDLE Output,
            CONSOLE_SCREEN_BUFFER_INFO *Info)
{
    COORD Origin = {0, 0};
    DWORD CellCount;
    DWORD Written;
    BOOL Success;

    Success = GetConsoleScreenBufferInfo(Output, Info);
    ok(Success, "GetConsoleScreenBufferInfo failed with error %lu\n", GetLastError());
    if (!Success)
        return FALSE;

    CellCount = (DWORD)Info->dwSize.X * Info->dwSize.Y;
    Success = FillConsoleOutputCharacterW(Output, L' ', CellCount, Origin, &Written);
    ok(Success, "FillConsoleOutputCharacterW failed with error %lu\n", GetLastError());
    ok(Written == CellCount, "Filled %lu of %lu characters\n", Written, CellCount);

    Success = FillConsoleOutputAttribute(Output, Info->wAttributes, CellCount, Origin, &Written);
    ok(Success, "FillConsoleOutputAttribute failed with error %lu\n", GetLastError());
    ok(Written == CellCount, "Filled %lu of %lu attributes\n", Written, CellCount);

    Success = SetConsoleCursorPosition(Output, Origin);
    ok(Success, "SetConsoleCursorPosition failed with error %lu\n", GetLastError());
    return Success;
}

static WCHAR
ReadCell(HANDLE Output,
         SHORT X,
         SHORT Y)
{
    COORD Position;
    WCHAR Character = 0;
    DWORD Read = 0;
    BOOL Success;

    Position.X = X;
    Position.Y = Y;
    Success = ReadConsoleOutputCharacterW(Output, &Character, 1, Position, &Read);
    ok(Success, "ReadConsoleOutputCharacterW(%d, %d) failed with error %lu\n",
       X, Y, GetLastError());
    ok(Read == 1, "ReadConsoleOutputCharacterW(%d, %d) read %lu characters\n",
       X, Y, Read);
    return Character;
}

static WORD
ReadCellAttribute(HANDLE Output,
                  SHORT X,
                  SHORT Y)
{
    COORD Position;
    WORD Attribute = 0;
    DWORD Read = 0;
    BOOL Success;

    Position.X = X;
    Position.Y = Y;
    Success = ReadConsoleOutputAttribute(Output, &Attribute, 1, Position, &Read);
    ok(Success, "ReadConsoleOutputAttribute(%d, %d) failed with error %lu\n",
       X, Y, GetLastError());
    ok(Read == 1, "ReadConsoleOutputAttribute(%d, %d) read %lu attributes\n",
       X, Y, Read);
    return Attribute;
}

static BOOL
Contains(const WCHAR *Buffer,
         DWORD Length,
         const WCHAR *Needle)
{
    DWORD NeedleLength = (DWORD)wcslen(Needle);
    DWORD Index;

    if (NeedleLength == 0)
        return TRUE;
    if (NeedleLength > Length)
        return FALSE;

    for (Index = 0; Index <= Length - NeedleLength; ++Index)
    {
        if (memcmp(Buffer + Index, Needle, NeedleLength * sizeof(WCHAR)) == 0)
            return TRUE;
    }

    return FALSE;
}

static DWORD
CountOccurrences(const WCHAR *Buffer,
                 DWORD Length,
                 const WCHAR *Needle)
{
    DWORD NeedleLength = (DWORD)wcslen(Needle);
    DWORD Count = 0;
    DWORD Index;

    if (NeedleLength == 0 || NeedleLength > Length)
        return 0;

    for (Index = 0; Index <= Length - NeedleLength; ++Index)
    {
        if (memcmp(Buffer + Index, Needle, NeedleLength * sizeof(WCHAR)) == 0)
        {
            ++Count;
            Index += NeedleLength - 1;
        }
    }

    return Count;
}

static DWORD
ReadVtCharacters(PREAD_CONSOLE_INPUT_EX_W ReadConsoleInputExW_,
                 HANDLE Input,
                 WCHAR *Characters,
                 DWORD Capacity,
                 DWORD *RecordCount)
{
    PINPUT_RECORD Records;
    DWORD RecordsRead = 0;
    DWORD CharacterCount = 0;
    DWORD StoredCount;
    DWORD Index;
    BOOL CallCompleted = FALSE;
    BOOL Success;

    Records = HeapAlloc(GetProcessHeap(), 0, INPUT_RECORD_COUNT * sizeof(*Records));
    ok(Records != NULL, "Unable to allocate the input record buffer\n");
    if (Records == NULL)
        return 0;

    SetLastError(0xdeadbeef);
    _SEH2_TRY
    {
        Success = ReadConsoleInputExW_(Input,
                                       Records,
                                       INPUT_RECORD_COUNT,
                                       &RecordsRead,
                                       CONSOLE_READ_NOWAIT);
        CallCompleted = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Success = FALSE;
    }
    _SEH2_END;

    ok(CallCompleted, "ReadConsoleInputExW raised an exception\n");
    if (CallCompleted)
        ok(Success, "ReadConsoleInputExW failed with error %lu\n", GetLastError());

    if (CallCompleted && Success)
    {
        for (Index = 0; Index < RecordsRead; ++Index)
        {
            if (Records[Index].EventType == KEY_EVENT &&
                Records[Index].Event.KeyEvent.bKeyDown &&
                Records[Index].Event.KeyEvent.uChar.UnicodeChar != UNICODE_NULL)
            {
                if (CharacterCount < Capacity)
                    Characters[CharacterCount] = Records[Index].Event.KeyEvent.uChar.UnicodeChar;
                ++CharacterCount;
            }
        }
    }

    StoredCount = Capacity != 0 ? min(CharacterCount, Capacity - 1) : 0;
    if (Capacity != 0)
        Characters[StoredCount] = UNICODE_NULL;
    ok(CharacterCount < Capacity,
       "Input returned %lu characters for a %lu-character buffer\n",
       CharacterCount, Capacity);

    if (RecordCount != NULL)
        *RecordCount = RecordsRead;

    HeapFree(GetProcessHeap(), 0, Records);
    return StoredCount;
}

static VOID
TestEditStartup(HANDLE Input,
                HANDLE Output,
                PREAD_CONSOLE_INPUT_EX_W ReadConsoleInputExW_)
{
    static const char EditSetup[] =
        "\x1b[?1049h\x1b[?1002;1006;2004h\x1b[?1036h"
        "\x1b]4;0;?;1;?;2;?;3;?;4;?;5;?;6;?;7;?\x07"
        "\x1b]4;8;?;9;?;10;?;11;?;12;?;13;?;14;?;15;?\x07"
        "\x1b]10;?\x07\x1b]11;?\x07"
        "\r" "\xe2\x80\xa6" "\x1b[6n"
        "\x1b[c";
    static const char EditRestore[] =
        "\x1b[0 q\x1b[?25h\x1b]0;\x07"
        "\x1b[?1002;1006;2004l\x1b[?1049l";
    CONSOLE_SCREEN_BUFFER_INFO Info;
    WCHAR Responses[1024];
    COORD MarkerPosition = {7, 3};
    DWORD CharacterCount;
    DWORD RecordsRead;
    BOOL Success;

    if (!ClearScreen(Output, &Info))
        return;
    if (Info.dwSize.X <= MarkerPosition.X || Info.dwSize.Y <= MarkerPosition.Y)
    {
        skip("Console screen buffer is too small for the alternate-screen test\n");
        return;
    }

    Success = SetConsoleCursorPosition(Output, MarkerPosition);
    ok(Success, "Unable to position the primary-screen marker, error %lu\n", GetLastError());
    if (!Success)
        return;
    WRITE_VT(Output, "P");

    Success = FlushConsoleInputBuffer(Input);
    ok(Success, "FlushConsoleInputBuffer failed with error %lu\n", GetLastError());

    WriteVt(Output, EditSetup, sizeof(EditSetup) - 1);

    ok(ReadCell(Output, MarkerPosition.X, MarkerPosition.Y) == L' ',
       "Primary-screen marker remained visible on the alternate screen\n");
    ok(ReadCell(Output, 0, 0) == 0x2026,
       "Expected the UTF-8 ellipsis at the alternate-screen origin\n");

    RecordsRead = 0;
    CharacterCount = ReadVtCharacters(ReadConsoleInputExW_,
                                      Input,
                                      Responses,
                                      ARRAYSIZE(Responses),
                                      &RecordsRead);
    trace("Edit startup returned %lu input records\n", RecordsRead);
    ok(CountOccurrences(Responses, CharacterCount, L"\x1b]4;") == 16,
       "Expected 16 OSC 4 palette replies\n");
    ok(Contains(Responses, CharacterCount, L"\x1b]10;rgb:"),
       "Missing OSC 10 foreground-colour reply\n");
    ok(Contains(Responses, CharacterCount, L"\x1b]11;rgb:"),
       "Missing OSC 11 background-colour reply\n");
    ok(Contains(Responses, CharacterCount, L"\x1b[1;2R") ||
       Contains(Responses, CharacterCount, L"\x1b[1;3R"),
       "Missing cursor-position reply for the ambiguous-width probe\n");
    ok(Contains(Responses, CharacterCount, L"\x1b[?1;0c"),
       "Missing primary device-attributes reply\n");

    WriteVt(Output, EditRestore, sizeof(EditRestore) - 1);
    ok(ReadCell(Output, MarkerPosition.X, MarkerPosition.Y) == L'P',
       "Primary-screen contents were not restored\n");

    Success = GetConsoleScreenBufferInfo(Output, &Info);
    ok(Success, "GetConsoleScreenBufferInfo failed with error %lu\n", GetLastError());
    if (Success)
    {
        ok(Info.dwCursorPosition.X == MarkerPosition.X + 1 &&
           Info.dwCursorPosition.Y == MarkerPosition.Y,
           "Primary cursor restored to (%d, %d), expected (%d, %d)\n",
           Info.dwCursorPosition.X,
           Info.dwCursorPosition.Y,
           MarkerPosition.X + 1,
           MarkerPosition.Y);
    }
}

static VOID
TestEditRendering(HANDLE Output)
{
    CONSOLE_SCREEN_BUFFER_INFO Info;
    CONSOLE_CURSOR_INFO DefaultCursor;
    CONSOLE_CURSOR_INFO Cursor;
    COORD Position;
    WCHAR Character;
    WORD Attribute;
    BOOL Success;

    if (!ClearScreen(Output, &Info))
        return;
    if (Info.dwSize.X < 8 || Info.dwSize.Y < 4)
    {
        skip("Console screen buffer is too small for the rendering tests\n");
        return;
    }

    WRITE_VT(Output, "\x1b[3;");
    ok(ReadCell(Output, 0, 0) == L' ',
       "An incomplete CSI sequence was rendered as text\n");

    WRITE_VT(Output, "5H");
    WRITE_VT(Output, "Z");
    ok(ReadCell(Output, 4, 2) == L'Z',
       "Split CUP sequence did not place text at column 5, row 3\n");

    Success = GetConsoleScreenBufferInfo(Output, &Info);
    ok(Success, "GetConsoleScreenBufferInfo failed with error %lu\n", GetLastError());
    if (Success)
    {
        ok(Info.dwCursorPosition.X == 5 && Info.dwCursorPosition.Y == 2,
           "Cursor is at (%d, %d), expected (5, 2)\n",
           Info.dwCursorPosition.X, Info.dwCursorPosition.Y);
    }

    WRITE_VT(Output,
             "\x1b[m\x1b[2;3H"
             "\x1b[38;2;17;34;51m\x1b[48;2;68;85;102m"
             "\x1b[3m\x1b[4mE"
             "\x1b[23m\x1b[24m\x1b[39m\x1b[49mF");
    ok(ReadCell(Output, 2, 1) == L'E',
       "True-colour SGR sequence did not render its text at column 3, row 2\n");
    Attribute = ReadCellAttribute(Output, 2, 1);
    ok((Attribute & COMMON_LVB_UNDERSCORE) != 0,
       "SGR 4 did not apply the underline attribute\n");
    ok(ReadCell(Output, 3, 1) == L'F',
       "SGR reset sequence did not render the following character\n");
    Attribute = ReadCellAttribute(Output, 3, 1);
    ok((Attribute & COMMON_LVB_UNDERSCORE) == 0,
       "SGR 24 did not clear the underline attribute\n");

    Success = GetConsoleCursorInfo(Output, &DefaultCursor);
    ok(Success, "GetConsoleCursorInfo failed with error %lu\n", GetLastError());
    if (Success)
    {
        WRITE_VT(Output, "\x1b[1 q");
        Success = GetConsoleCursorInfo(Output, &Cursor);
        ok(Success, "GetConsoleCursorInfo failed with error %lu\n", GetLastError());
        if (Success)
            ok(Cursor.dwSize == 100, "Block cursor size is %lu, expected 100\n", Cursor.dwSize);

        WRITE_VT(Output, "\x1b[5 q");
        Success = GetConsoleCursorInfo(Output, &Cursor);
        ok(Success, "GetConsoleCursorInfo failed with error %lu\n", GetLastError());
        if (Success)
            ok(Cursor.dwSize == 25, "Bar cursor size is %lu, expected 25\n", Cursor.dwSize);

        WRITE_VT(Output, "\x1b[?25l");
        Success = GetConsoleCursorInfo(Output, &Cursor);
        ok(Success, "GetConsoleCursorInfo failed with error %lu\n", GetLastError());
        if (Success)
            ok(!Cursor.bVisible, "DECTCEM did not hide the cursor\n");

        WRITE_VT(Output, "\x1b[0 q\x1b[?25h");
        Success = GetConsoleCursorInfo(Output, &Cursor);
        ok(Success, "GetConsoleCursorInfo failed with error %lu\n", GetLastError());
        if (Success)
        {
            ok(Cursor.dwSize == DefaultCursor.dwSize,
               "Default cursor size is %lu, expected %lu\n",
               Cursor.dwSize, DefaultCursor.dwSize);
            ok(Cursor.bVisible, "DECTCEM did not show the cursor\n");
        }
    }

    if (!ClearScreen(Output, &Info))
        return;

    Position.X = Info.dwSize.X - 1;
    Position.Y = 1;
    Success = SetConsoleCursorPosition(Output, Position);
    ok(Success, "SetConsoleCursorPosition failed with error %lu\n", GetLastError());
    if (!Success)
        return;

    WRITE_VT(Output, "A");
    Success = GetConsoleScreenBufferInfo(Output, &Info);
    ok(Success, "GetConsoleScreenBufferInfo failed with error %lu\n", GetLastError());
    if (Success)
    {
        ok(Info.dwCursorPosition.X == Position.X &&
           Info.dwCursorPosition.Y == Position.Y,
           "Delayed wrap moved the cursor to (%d, %d)\n",
           Info.dwCursorPosition.X, Info.dwCursorPosition.Y);
    }

    WRITE_VT(Output, "B");
    Success = GetConsoleScreenBufferInfo(Output, &Info);
    ok(Success, "GetConsoleScreenBufferInfo failed with error %lu\n", GetLastError());
    if (Success)
    {
        ok(Info.dwCursorPosition.X == 1 &&
           Info.dwCursorPosition.Y == Position.Y + 1,
           "Delayed wrap continued at (%d, %d), expected (1, %d)\n",
           Info.dwCursorPosition.X,
           Info.dwCursorPosition.Y,
           Position.Y + 1);
    }

    Character = ReadCell(Output, Position.X, Position.Y);
    ok(Character == L'A', "Last-column character is %#x, expected 'A'\n", Character);
    Character = ReadCell(Output, 0, Position.Y + 1);
    ok(Character == L'B', "Wrapped character is %#x, expected 'B'\n", Character);
}

static BOOL
SendConsoleMessage(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    DWORD_PTR Result = 0;
    BOOL Success;

    SetLastError(0xdeadbeef);
    Success = SendMessageTimeoutW(Window,
                                  Message,
                                  WParam,
                                  LParam,
                                  SMTO_ABORTIFHUNG,
                                  2000,
                                  &Result) != 0;
    ok(Success, "SendMessageTimeoutW(%#x) failed with error %lu\n",
       Message, GetLastError());
    return Success;
}

static VOID
TestEditFrontendInput(HANDLE Input,
                      HANDLE Output,
                      PREAD_CONSOLE_INPUT_EX_W ReadConsoleInputExW_)
{
    WCHAR Characters[128];
    HWND Window;
    UINT ScanCode;
    LPARAM KeyDown;
    LPARAM KeyUp;
    RECT Client;
    LPARAM MousePosition;
    DWORD CharacterCount;
    BOOL Success;

    Window = GetConsoleWindow();
    if (Window == NULL || !IsWindow(Window))
    {
        skip("No GUI console window is available for frontend input tests\n");
        return;
    }

    WRITE_VT(Output, "\x1b[?1002;1006;2004h\x1b[?1036h");

    Success = FlushConsoleInputBuffer(Input);
    ok(Success, "FlushConsoleInputBuffer failed with error %lu\n", GetLastError());

    ScanCode = MapVirtualKeyW(VK_UP, MAPVK_VK_TO_VSC);
    KeyDown = 1 | ((LPARAM)ScanCode << 16) | ((LPARAM)1 << 24);
    KeyUp = KeyDown | ((LPARAM)1 << 30) | ((LPARAM)1 << 31);

    if (SendConsoleMessage(Window, WM_KEYDOWN, VK_UP, KeyDown) &&
        SendConsoleMessage(Window, WM_KEYUP, VK_UP, KeyUp))
    {
        CharacterCount = ReadVtCharacters(ReadConsoleInputExW_,
                                          Input,
                                          Characters,
                                          ARRAYSIZE(Characters),
                                          NULL);
        ok(CharacterCount == 3,
           "Up arrow produced %lu VT characters, expected 3\n", CharacterCount);
        ok(CharacterCount >= 3 &&
           Characters[0] == L'\x1b' &&
           Characters[1] == L'[' &&
           Characters[2] == L'A',
           "Up arrow did not produce CSI A\n");
    }

    Success = FlushConsoleInputBuffer(Input);
    ok(Success, "FlushConsoleInputBuffer failed with error %lu\n", GetLastError());

    Success = GetClientRect(Window, &Client);
    ok(Success, "GetClientRect failed with error %lu\n", GetLastError());
    if (Success && Client.right > 2 && Client.bottom > 2)
    {
        MousePosition = MAKELPARAM(Client.right / 2, Client.bottom / 2);
        if (SendConsoleMessage(Window, WM_LBUTTONDOWN, MK_LBUTTON, MousePosition) &&
            SendConsoleMessage(Window, WM_LBUTTONUP, 0, MousePosition))
        {
            CharacterCount = ReadVtCharacters(ReadConsoleInputExW_,
                                              Input,
                                              Characters,
                                              ARRAYSIZE(Characters),
                                              NULL);
            ok(CountOccurrences(Characters, CharacterCount, L"\x1b[<0;") == 2,
               "Mouse press/release did not produce two SGR mouse sequences\n");
            ok(CharacterCount != 0 && Characters[CharacterCount - 1] == L'm',
               "Mouse release did not end in the SGR release marker\n");
        }
    }

    WRITE_VT(Output, "\x1b[?1002;1006;2004l");
    FlushConsoleInputBuffer(Input);
}

START_TEST(ConsoleVirtualTerminal)
{
    PREAD_CONSOLE_INPUT_EX_W ReadConsoleInputExW_;
    CONSOLE_SCREEN_BUFFER_INFOEX InfoEx;
    HANDLE OriginalOutput = INVALID_HANDLE_VALUE;
    HANDLE Input = INVALID_HANDLE_VALUE;
    HANDLE Output = INVALID_HANDLE_VALUE;
    DWORD OriginalInputMode = 0;
    DWORD OriginalOutputMode = 0;
    DWORD Mode;
    DWORD Error;
    UINT OriginalInputCodePage = 0;
    UINT OriginalOutputCodePage = 0;
    WCHAR OriginalTitle[256];
    BOOL HaveInputMode = FALSE;
    BOOL HaveTitle = FALSE;
    BOOL AllocatedConsole = FALSE;
    BOOL TestBufferActive = FALSE;
    BOOL RunningOnReactOS;
    BOOL NativeNt63;
    BOOL Success;
    ULONG NtVersion;

    NtVersion = GetNTVersion();
    RunningOnReactOS = is_reactos();
    NativeNt63 = !RunningOnReactOS && NtVersion == _WIN32_WINNT_WINBLUE;
    trace("Running Edit VT tests on NT %lu.%lu\n", NtVersion >> 8, NtVersion & 0xff);
    /* ReactOS exposes this VT path independently of its reported NT version. */
    if (!RunningOnReactOS && NtVersion < _WIN32_WINNT_WINBLUE)
    {
        skip("The Edit VT baseline requires native NT 6.3 or newer\n");
        return;
    }

    ReadConsoleInputExW_ = (PREAD_CONSOLE_INPUT_EX_W)GetProcAddress(
        GetModuleHandleW(L"kernel32.dll"), "ReadConsoleInputExW");
    ok(ReadConsoleInputExW_ != NULL, "ReadConsoleInputExW is not exported\n");

    OriginalOutput = CreateFileW(L"CONOUT$",
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);
    if (OriginalOutput == INVALID_HANDLE_VALUE && AllocConsole())
    {
        AllocatedConsole = TRUE;
        OriginalOutput = CreateFileW(L"CONOUT$",
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL);
    }
    ok(OriginalOutput != INVALID_HANDLE_VALUE,
       "Opening CONOUT$ failed with error %lu\n", GetLastError());
    if (OriginalOutput == INVALID_HANDLE_VALUE)
        goto Cleanup;

    Input = CreateFileW(L"CONIN$",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
    ok(Input != INVALID_HANDLE_VALUE,
       "Opening CONIN$ failed with error %lu\n", GetLastError());
    if (Input == INVALID_HANDLE_VALUE)
        goto Cleanup;

    Output = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL,
                                       CONSOLE_TEXTMODE_BUFFER,
                                       NULL);
    ok(Output != INVALID_HANDLE_VALUE,
       "CreateConsoleScreenBuffer failed with error %lu\n", GetLastError());
    if (Output == INVALID_HANDLE_VALUE)
        goto Cleanup;

    Success = GetConsoleMode(Input, &OriginalInputMode);
    ok(Success, "GetConsoleMode(CONIN$) failed with error %lu\n", GetLastError());
    if (!Success)
        goto Cleanup;
    HaveInputMode = TRUE;

    OriginalInputCodePage = GetConsoleCP();
    OriginalOutputCodePage = GetConsoleOutputCP();
    OriginalTitle[0] = UNICODE_NULL;
    GetConsoleTitleW(OriginalTitle, ARRAYSIZE(OriginalTitle));
    HaveTitle = TRUE;

    Success = SetConsoleActiveScreenBuffer(Output);
    ok(Success, "SetConsoleActiveScreenBuffer failed with error %lu\n", GetLastError());
    if (!Success)
        goto Cleanup;
    TestBufferActive = TRUE;

    Success = SetConsoleCP(CP_UTF8);
    ok(Success, "SetConsoleCP(CP_UTF8) failed with error %lu\n", GetLastError());
    Success = SetConsoleOutputCP(CP_UTF8);
    ok(Success, "SetConsoleOutputCP(CP_UTF8) failed with error %lu\n", GetLastError());

    Success = GetConsoleMode(Output, &OriginalOutputMode);
    ok(Success, "GetConsoleMode(CONOUT$) failed with error %lu\n", GetLastError());
    if (!Success)
        goto Cleanup;

    SetLastError(0xdeadbeef);
    Success = SetConsoleMode(Input, EDIT_INPUT_MODE);
    Error = GetLastError();
    if (NativeNt63)
    {
        ok(!Success, "SetConsoleMode(Edit input mode) unexpectedly succeeded\n");
        ok(Error == ERROR_INVALID_PARAMETER,
           "SetConsoleMode(Edit input mode) failed with error %lu, expected %lu\n",
           Error, (DWORD)ERROR_INVALID_PARAMETER);
    }
    else
    {
        ok(Success, "SetConsoleMode(Edit input mode) failed with error %lu\n", Error);
    }
    Success = GetConsoleMode(Input, &Mode);
    ok(Success, "GetConsoleMode(CONIN$) failed with error %lu\n", GetLastError());
    if (Success)
    {
        if (NativeNt63)
        {
            ok(Mode == OriginalInputMode,
               "Rejected Edit input mode changed the mode from %#lx to %#lx\n",
               OriginalInputMode, Mode);
        }
        else
        {
            /*
             * ENABLE_EXTENDED_FLAGS controls the Quick Edit setting passed to
             * SetConsoleMode; it is not an independent input behavior. Console
             * hosts may omit it when both Quick Edit and Insert mode are disabled.
             */
            ok((Mode & ~ENABLE_EXTENDED_FLAGS) == (EDIT_INPUT_MODE & ~ENABLE_EXTENDED_FLAGS),
               "Input mode is %#lx, expected Edit's operational mode %#lx\n",
               Mode,
               (DWORD)(EDIT_INPUT_MODE & ~ENABLE_EXTENDED_FLAGS));
        }
    }

    SetLastError(0xdeadbeef);
    Success = SetConsoleMode(Output, EDIT_OUTPUT_MODE);
    Error = GetLastError();
    if (NativeNt63)
    {
        ok(!Success, "SetConsoleMode(Edit output mode) unexpectedly succeeded\n");
        ok(Error == ERROR_INVALID_PARAMETER,
           "SetConsoleMode(Edit output mode) failed with error %lu, expected %lu\n",
           Error, (DWORD)ERROR_INVALID_PARAMETER);
    }
    else
    {
        ok(Success, "SetConsoleMode(Edit output mode) failed with error %lu\n", Error);
    }
    Success = GetConsoleMode(Output, &Mode);
    ok(Success, "GetConsoleMode(CONOUT$) failed with error %lu\n", GetLastError());
    if (Success)
    {
        if (NativeNt63)
            ok(Mode == OriginalOutputMode,
               "Rejected Edit output mode changed the mode from %#lx to %#lx\n",
               OriginalOutputMode, Mode);
        else
            ok(Mode == EDIT_OUTPUT_MODE, "Output mode is %#lx, expected %#lx\n", Mode, (DWORD)EDIT_OUTPUT_MODE);
    }

    ZeroMemory(&InfoEx, sizeof(InfoEx));
    InfoEx.cbSize = sizeof(InfoEx);
    Success = GetConsoleScreenBufferInfoEx(Output, &InfoEx);
    ok(Success, "GetConsoleScreenBufferInfoEx failed with error %lu\n", GetLastError());
    if (Success)
    {
        ok(InfoEx.dwSize.X > 0 && InfoEx.dwSize.Y > 0,
           "Invalid screen-buffer size %dx%d\n", InfoEx.dwSize.X, InfoEx.dwSize.Y);
        ok(InfoEx.srWindow.Right >= InfoEx.srWindow.Left &&
           InfoEx.srWindow.Bottom >= InfoEx.srWindow.Top,
           "Invalid console window (%d,%d)-(%d,%d)\n",
           InfoEx.srWindow.Left,
           InfoEx.srWindow.Top,
           InfoEx.srWindow.Right,
           InfoEx.srWindow.Bottom);
    }

    if (ReadConsoleInputExW_ != NULL)
    {
        Success = FlushConsoleInputBuffer(Input);
        ok(Success, "FlushConsoleInputBuffer failed with error %lu\n", GetLastError());
        if (Success)
        {
            WCHAR Empty[1];
            DWORD RecordsRead = 0;
            DWORD CharacterCount;

            CharacterCount = ReadVtCharacters(ReadConsoleInputExW_,
                                              Input,
                                              Empty,
                                              ARRAYSIZE(Empty),
                                              &RecordsRead);
            ok(RecordsRead == 0, "Empty nonblocking read returned %lu records\n", RecordsRead);
            ok(CharacterCount == 0, "Empty nonblocking read returned %lu characters\n",
               CharacterCount);
        }

        if (!NativeNt63)
            TestEditStartup(Input, Output, ReadConsoleInputExW_);
    }
    else
    {
        skip("ReadConsoleInputExW is unavailable; skipping Edit startup replies\n");
    }
    if (NativeNt63)
    {
        skip("Native NT 6.3 does not support VT processing; skipping Edit VT parser tests\n");
    }
    else
    {
        TestEditRendering(Output);
        if (ReadConsoleInputExW_ != NULL)
            TestEditFrontendInput(Input, Output, ReadConsoleInputExW_);
    }

Cleanup:
    if (!NativeNt63 && Output != INVALID_HANDLE_VALUE)
        WRITE_VT(Output, "\x1b[?1002;1006;2004l\x1b[?1049l\x1b[0 q\x1b[?25h");

    if (TestBufferActive && OriginalOutput != INVALID_HANDLE_VALUE)
    {
        Success = SetConsoleActiveScreenBuffer(OriginalOutput);
        ok(Success, "Restoring the original screen buffer failed with error %lu\n",
           GetLastError());
    }

    if (HaveInputMode)
        SetConsoleMode(Input, OriginalInputMode);
    if (Input != INVALID_HANDLE_VALUE)
        FlushConsoleInputBuffer(Input);

    if (OriginalInputCodePage != 0)
        SetConsoleCP(OriginalInputCodePage);
    if (OriginalOutputCodePage != 0)
        SetConsoleOutputCP(OriginalOutputCodePage);
    if (HaveTitle)
        SetConsoleTitleW(OriginalTitle);

    if (Output != INVALID_HANDLE_VALUE)
        CloseHandle(Output);
    if (Input != INVALID_HANDLE_VALUE)
        CloseHandle(Input);
    if (OriginalOutput != INVALID_HANDLE_VALUE)
        CloseHandle(OriginalOutput);
    if (AllocatedConsole)
        FreeConsole();
}
