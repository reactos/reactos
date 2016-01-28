
/* FUNCTIONS ******************************************************************/

VOID ScreenEventHandler(PWINDOW_BUFFER_SIZE_RECORD ScreenEvent);
BOOLEAN VgaGetDoubleVisionState(PBOOLEAN Horizontal, PBOOLEAN Vertical);
BOOL VgaAttachToConsole(VOID);
VOID VgaDetachFromConsole(VOID);


VOID
VgaConsoleUpdateTextCursor(BOOL CursorVisible,
                           BYTE CursorStart,
                           BYTE CursorEnd,
                           BYTE TextSize,
                           DWORD ScanlineSize,
                           WORD Location);

BOOL
VgaConsoleCreateGraphicsScreen(// OUT PBYTE* GraphicsFramebuffer,
                               IN PCOORD Resolution,
                               IN HANDLE PaletteHandle);

VOID VgaConsoleDestroyGraphicsScreen(VOID);

BOOL
VgaConsoleCreateTextScreen(// OUT PCHAR_CELL* TextFramebuffer,
                           IN PCOORD Resolution,
                           IN HANDLE PaletteHandle);

VOID VgaConsoleDestroyTextScreen(VOID);

VOID VgaConsoleRepaintScreen(PSMALL_RECT Rect);

BOOLEAN VgaConsoleInitialize(HANDLE TextHandle);
VOID VgaConsoleCleanup(VOID);
