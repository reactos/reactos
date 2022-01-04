
#include <precomp.h>

extern int BlockEnvToEnvironA(void);
extern int BlockEnvToEnvironW(void);
extern void FreeEnvironment(char **environment);

extern void msvcrt_init_mt_locks(void);
extern void msvcrt_init_io(void);

extern char* _acmdln;        /* pointer to ascii command line */
extern wchar_t* _wcmdln;     /* pointer to wide character command line */
#undef _environ
extern char** _environ;      /* pointer to environment block */
extern char** __initenv;     /* pointer to initial environment block */
extern wchar_t** _wenviron;  /* pointer to environment block */
extern wchar_t** __winitenv; /* pointer to initial environment block */

BOOL
crt_process_init(void)
{
    OSVERSIONINFOW osvi;

    /* initialize version info */
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    GetVersionExW(&osvi);
    _winver     = (osvi.dwMajorVersion << 8) | osvi.dwMinorVersion;
    _winmajor   = osvi.dwMajorVersion;
    _winminor   = osvi.dwMinorVersion;
    _osplatform = osvi.dwPlatformId;
    _osver      = osvi.dwBuildNumber;

    /* create tls stuff */
    if (!msvcrt_init_tls())
        return FALSE;

    if (BlockEnvToEnvironA() < 0)
        return FALSE;

    if (BlockEnvToEnvironW() < 0)
    {
        FreeEnvironment(_environ);
        return FALSE;
    }

    _acmdln = _strdup(GetCommandLineA());
    _wcmdln = _wcsdup(GetCommandLineW());

    /* Initialization of the WINE code */
    msvcrt_init_mt_locks();

    //msvcrt_init_math();
    msvcrt_init_io();
    //msvcrt_init_console();
    //msvcrt_init_args();
    //msvcrt_init_signals();

    return TRUE;
}
