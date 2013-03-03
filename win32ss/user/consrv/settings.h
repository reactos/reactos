/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/settings.h
 * PURPOSE:         Consoles settings management
 * PROGRAMMERS:     Hermes Belusca - Maito
 *
 * NOTE: Adapted from existing code.
 */

#define PM_APPLY_CONSOLE_INFO (WM_APP + 100)

/* STRUCTURES *****************************************************************/

typedef struct _GUI_CONSOLE_INFO
{
    // FONTSIGNATURE FontSignature;
    WCHAR FaceName[LF_FACESIZE];
    UINT  FontFamily;
    DWORD FontSize;
    DWORD FontWeight;
    BOOL  UseRasterFonts;

    WORD  ShowWindow;
    BOOL  AutoPosition;
    POINT WindowOrigin;
} GUI_CONSOLE_INFO, *PGUI_CONSOLE_INFO;

typedef struct _CONSOLE_INFO
{
    ULONG   HistoryBufferSize;
    ULONG   NumberOfHistoryBuffers;
    BOOLEAN HistoryNoDup;

/* BOOLEAN */    ULONG FullScreen; // Give the type of console: GUI (windowed) or TUI (fullscreen)
    BOOLEAN QuickEdit;
    BOOLEAN InsertMode;
    ULONG InputBufferSize;
    COORD ScreenBufferSize;

/* SIZE */    COORD   ConsoleSize;  // This is really the size of the console at screen.

    BOOLEAN CursorBlinkOn;
    BOOLEAN ForceCursorOff;
    ULONG   CursorSize;

    USHORT ScreenAttrib; // CHAR_INFO ScreenFillAttrib
    USHORT PopupAttrib;

    // Color palette
    COLORREF Colors[16];

    ULONG CodePage;

    WCHAR ConsoleTitle[MAX_PATH + 1];

    union
    {
        GUI_CONSOLE_INFO GuiInfo;
        // TUI_CONSOLE_INFO TuiInfo;
    } u;
} CONSOLE_INFO, *PCONSOLE_INFO;

#define RGBFromAttrib(Console, Attribute)   ((Console)->Colors[(Attribute) & 0xF])
#define TextAttribFromAttrib(Attribute)     ((Attribute) & 0xF)
#define BkgdAttribFromAttrib(Attribute)     (((Attribute) >> 4) & 0xF)
#define MakeAttrib(TextAttrib, BkgdAttrib)  (DWORD)((((BkgdAttrib) & 0xF) << 4) | ((TextAttrib) & 0xF))


/* Used to communicate with console.dll */
typedef struct _CONSOLE_PROPS
{
    HWND hConsoleWindow;
    BOOL ShowDefaultParams;

    BOOLEAN AppliedConfig;
    DWORD   ActiveStaticControl;

    CONSOLE_INFO ci;
} CONSOLE_PROPS, *PCONSOLE_PROPS;

/* FUNCTIONS ******************************************************************/

BOOL ConSrvReadUserSettings(IN OUT PCONSOLE_INFO ConsoleInfo,
                            IN DWORD ProcessId);
BOOL ConSrvWriteUserSettings(IN PCONSOLE_INFO ConsoleInfo,
                             IN DWORD ProcessId);
VOID ConSrvGetDefaultSettings(IN OUT PCONSOLE_INFO ConsoleInfo,
                              IN DWORD ProcessId);

/* EOF */
