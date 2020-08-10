#include <windows.h>
#include <stdio.h>

static char * messageFromFunction = "InitOnceFunction did not run";
static BOOL CALLBACK InitOnceFunction(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
    printf("Inside InitOnceFunction\r\n");
    (*(char **)lpContext) = "InitOnceFunction ran successfully";
    return TRUE;
}

static void run_test(void)
{
    static INIT_ONCE s_InitOnce = INIT_ONCE_STATIC_INIT;
    BOOL bStatus = InitOnceExecuteOnce(&s_InitOnce, InitOnceFunction, NULL, (LPVOID *)&messageFromFunction);
    if (bStatus == FALSE)
    {
        printf("InitOnceExecuteOnce() failed\r\n");
        return;
    }

    printf("%s\r\n", messageFromFunction);
}

int main(int argc, const char *argv[])
{
    // Run the test twice, to ensure that InitOnceFunction() is only run once.
    run_test();
    run_test();

    return 0;
}
