#include <Windows.h>

static LPWSTR messageFromFunction = L"did not run";
static BOOL CALLBACK InitOnceFunction(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
    MessageBoxW(NULL, L"Hello from InitOnceFunction", L"Versioning Test", MB_OK);
    (*(LPWSTR *)lpContext) = L"Ran successfully!";
    return TRUE;
}

static void run_test(void)
{
    static INIT_ONCE s_InitOnce = INIT_ONCE_STATIC_INIT;
    BOOL bStatus = InitOnceExecuteOnce(&s_InitOnce, InitOnceFunction, NULL, (LPVOID *)&messageFromFunction);
    if (bStatus == FALSE)
    {
        MessageBoxW(NULL, L"InitOnceExecuteOnce() failed", L"Versioning Test", MB_OK);
        return;
    }

    MessageBoxW(NULL, messageFromFunction, L"Versioning Test", MB_OK);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR cmdLine, int nCmdShow)
{
    // Run the test twice, to ensure that InitOnceFunction() is only run once.
    run_test();
    run_test();

    return 0;
}
