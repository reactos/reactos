#include <stdio.h>
#include <tchar.h>
#include <windows.h>

typedef BOOL (WINAPI * PTHEMESHOOKINSTALL) ();

int _tmain(int argc, _TCHAR* argv[])
{
    HMODULE hmodRosTheme;
    PTHEMESHOOKINSTALL fnInstall, fnRemove;
    BOOL ret;
    DWORD info = BSM_APPLICATIONS;

    if ( argc != 2 )
    {
        return 0;
    }

    hmodRosTheme = LoadLibrary(argv[1]);
    if(!hmodRosTheme)
    {
        printf("failed to load rostheme\n");
        return 0;
    }

    fnInstall = GetProcAddress(hmodRosTheme, (LPCSTR)34);
    fnRemove = GetProcAddress(hmodRosTheme, (LPCSTR)35);

    if(!fnInstall)
    {
        printf("failed to get ThemeHooksInstall\n");
        return 0;
    }

    if(!fnRemove)
    {
        printf("failed to get ThemeHooksRemove\n");
        return 0;
    }

    ret = fnInstall();
    printf("ThemeHooksInstall returned %d\n", ret);
    
    BroadcastSystemMessage(BSF_POSTMESSAGE, &info,WM_SETFOCUS,0,0);

    system("pause");

    ret = fnRemove();
    printf("ThemeHooksRemove returned %d\n", ret);

    BroadcastSystemMessage(BSF_POSTMESSAGE, &info,WM_SETFOCUS,0,0);

	return 0;
}

