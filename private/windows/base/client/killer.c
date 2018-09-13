#include <stdio.h>
#include <windows.h>


Spin()
{
    int i;
    for (i=0;1;i++) {
        Sleep(i*7500);
        }
}

void
main(void)
{
    DWORD ThreadId;
    HANDLE Thread;
    int i;
    int failcount;

    failcount = 0;
    for (i = 0;; i++) {
        Thread = CreateThread(NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)Spin,
                    NULL,
                    0,
                    &ThreadId
                    );
        if ( (i/50)*50 == i ) {
            printf("%d threads created\n", i);
            }

        if (!Thread) {
            failcount++;
            printf("%d threads created before %d failure\n", i,failcount);
            Sleep(5000);
            if ( failcount < 10 ) {
                i--;
                goto again;
                }
            break;
            }
        else {
            CloseHandle(Thread);
            }
again:;
        }
}
