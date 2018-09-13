#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

int
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    int i=0;
    SMALL_RECT Window;

    Window.Left = 0;
    Window.Top = 0;
    Window.Right = 10;
    Window.Bottom = 5;

    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                         TRUE,
                         &Window
                        );


    while ( TRUE ) {
        printf("%d\n",++i);
    }

}
