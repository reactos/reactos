#ifdef __cplusplus
extern "C" {
#endif

LONG APIENTRY SettingsCPlApplet(
    HWND  hwnd,
    WORD  message,
    DWORD wParam,
    LONG  lParam);

BOOL SettingsDllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL);


void DeleteDisplayDialogBox( LPARAM lParam );
DLGPROC NewDisplayDialogBox(HINSTANCE hmod, LPARAM *lplParam);
BOOL CALLBACK DisplayPageProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef enum {
    EXEC_NORMAL = 1,
    EXEC_SETUP,
    EXEC_DETECT,
    EXEC_INVALID_MODE,
} EXEC_MODE;

typedef enum {
    NOT_INVALID = 1,
    EXEC_INVALID_NEW_DRIVER,
    EXEC_INVALID_DEFAULT_DISPLAY_MODE,
    EXEC_INVALID_DISPLAY_DRIVER,
    EXEC_INVALID_OLD_DISPLAY_DRIVER,
    EXEC_INVALID_16COLOR_DISPLAY_MODE,
    EXEC_INVALID_DISPLAY_MODE,
    EXEC_INVALID_CONFIGURATION,
} EXEC_INVALID_SUBMODE;

extern EXEC_MODE gbExecMode;
extern EXEC_INVALID_SUBMODE gbInvalidMode;


#ifdef __cplusplus
}
#endif
