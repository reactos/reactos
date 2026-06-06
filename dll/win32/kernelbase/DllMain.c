
#include <windows.h>

void init_locale( HMODULE module );

// Used by wine/locale.c
char system_dir[MAX_PATH];

static
void
InitSystemDir(void)
{
    GetSystemDirectoryA(system_dir, MAX_PATH);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            InitSystemDir();
            //init_locale(hinstDLL);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
