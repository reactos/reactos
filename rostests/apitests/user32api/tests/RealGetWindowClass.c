#include "../user32api.h"

typedef struct _TestData
{
    BOOL    OverrideWndProc;            /* TRUE if lpfnWndProc should be overridden */
    LPCSTR  ClassName;                  /* Name of the new class to register */
    DWORD   WndExtra;                   /* Remove these WNDCLASS::cbWndExtra flags */
    BOOL    ExpectsHwnd;                /* TRUE if a HWND should be created to run tests on */
    LPCSTR  ExpectedClassNameBefore;    /* Expected class name before any dialog function is called */
    LPCSTR  ExpectedClassNameAfter;     /* Expected class name after any dialog function is called */
} TestData;

static TestData RealClassTestData[] =
{
    {
        TRUE,
        "OverrideWndProc_with_DLGWINDOWEXTRA_TRUE",
        0,
        TRUE,
        "OverrideWndProc_with_DLGWINDOWEXTRA_TRUE",
        "#32770",
    },
    {
        TRUE,
        "OverrideWndProc_without_DLGWINDOWEXTRA_TRUE",
        DLGWINDOWEXTRA,
        TRUE,
        "OverrideWndProc_without_DLGWINDOWEXTRA_TRUE",
        "OverrideWndProc_without_DLGWINDOWEXTRA_TRUE",
    },
    {
        FALSE,
        "DefaultWndProc_with_DLGWINDOWEXTRA_FALSE",
        0,
        TRUE,
        "#32770",
        "#32770",
    },
    {
        FALSE,
        "DefaultWndProc_without_DLGWINDOWEXTRA_FALSE",
        DLGWINDOWEXTRA,
        FALSE,
        "N/A",
        "N/A",
    },
};

INT
Test_RealGetWindowClass(PTESTINFO pti)
{
    int testNo;
    UINT Result;
    CHAR Buffer[1024];

    Result = RealGetWindowClass( NULL, Buffer, ARRAY_SIZE(Buffer) );
    TEST(Result == 0);
    TEST(GetLastError() == ERROR_INVALID_WINDOW_HANDLE);

    for (testNo = 0; testNo < ARRAY_SIZE(RealClassTestData); testNo++)
    {
        ATOM atom;
        WNDCLASSA cls;
        HWND hWnd;

        /* Register classes, "derived" from built-in dialog, with and without the DLGWINDOWEXTRA flag set */
        GetClassInfoA(0, "#32770", &cls);
        if (RealClassTestData[testNo].OverrideWndProc)
            cls.lpfnWndProc = DefWindowProcA;
        cls.lpszClassName = RealClassTestData[testNo].ClassName;
        cls.cbWndExtra &= ~RealClassTestData[testNo].WndExtra;
        atom = RegisterClassA (&cls);
        ASSERT(atom != 0);

        /* Create a window */
        hWnd = CreateWindowEx( WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR |
                               WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT | WS_EX_APPWINDOW,
                               RealClassTestData[testNo].ClassName,
                               RealClassTestData[testNo].ClassName,
                               WS_POPUPWINDOW | WS_CLIPSIBLINGS | WS_DLGFRAME | WS_OVERLAPPED |
                               WS_MINIMIZEBOX | WS_MAXIMIZEBOX | DS_3DLOOK | DS_SETFONT | DS_MODALFRAME,
                               CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                               NULL, NULL, g_hInstance, 0);

        /* Do we expect a HWND at all? */
        if (RealClassTestData[testNo].ExpectsHwnd)
        {
            TEST(hWnd != NULL);

            if (hWnd != NULL)
            {
                /* Get the "real" class name */
                Result = RealGetWindowClass( hWnd, Buffer, ARRAY_SIZE(Buffer) );
                printf("Buffer: %s\nExpectedClassNameBefore(%d): %s\n", Buffer, testNo, RealClassTestData[testNo].ExpectedClassNameBefore);
                TEST( Result != 0 );
                TEST( strcmp( Buffer, RealClassTestData[testNo].ExpectedClassNameBefore ) == 0 );

                /* Call a function that requires a dialog window */
                DefDlgProcA( hWnd, DM_SETDEFID, IDCANCEL, 0 );

                /* Get the "real" class name again */
                Result = RealGetWindowClass( hWnd, Buffer, ARRAY_SIZE(Buffer) );
                printf("Buffer: %s\nExpectedClassNameAfter(%d): %s\n", Buffer, testNo, RealClassTestData[testNo].ExpectedClassNameAfter);
                TEST( Result != 0 );
                TEST( strcmp( Buffer, RealClassTestData[testNo].ExpectedClassNameAfter ) == 0 );
            }
        }
        else
        {
            TEST(hWnd == NULL);
        }

        /* Cleanup */
        DestroyWindow(hWnd);
        UnregisterClass(RealClassTestData[testNo].ClassName, g_hInstance);
    }

    return APISTATUS_NORMAL;
}
