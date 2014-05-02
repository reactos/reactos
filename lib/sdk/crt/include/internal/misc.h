

extern int msvcrt_error_mode;
extern int __app_type;
#define _UNKNOWN_APP 0
#define _CONSOLE_APP 1
#define _GUI_APP 2

int
__cdecl
__crt_MessageBoxA (
    _In_opt_ const char *pszText,
    _In_ unsigned int uType);

