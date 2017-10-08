/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for RealGetWindowClass
 * PROGRAMMERS:     Gregor Gullwi
 */

#include <apitest.h>

#include <stdio.h>
#include <winuser.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static PWSTR ControlsList[] =
{
  L"Button\0",
  L"Edit\0",
  L"Static\0",
  L"ListBox\0",
  L"ScrollBar\0",
  L"ComboBox\0",
  L"MDIClient\0",
  L"ComboLBox\0",
  L"DDEMLEvent\0",
  L"DDEMLMom\0",
  L"DMGClass\0",
  L"DDEMLAnsiClient\0",
  L"DDEMLUnicodeClient\0",
  L"DDEMLAnsiServer\0",
  L"DDEMLUnicodeServer\0",
  L"IME\0",
  L"\0",//L"Ghost\0", // Vista+, Ghost is registered after "Message" in XP/W2k3 Win32k.sys.
};

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

//
// The Control Class Atoms are consecitively issued.
//

void Test_ClassAtoms()
{
    ATOM atom;
    int i;
    WNDCLASSW cls;

    // Initialize starting Atom number.
    atom = GetClassInfoW(NULL, ControlsList[0], &cls);
    if (!atom)
    {
       skip( "No reference Atom\n" );
       return;
    }

    trace("First Control Class Atom 0x%x\n",atom);

    for (i = 0; i < 17; atom++ ,i++)
    {
        if (lstrcmpW(ControlsList[i],L"\0"))
        {
           ATOM test_atom = GetClassInfoW(NULL, ControlsList[i], &cls);
           // Skip unregistered Classes.
           if (test_atom)
           {
              ok(test_atom == atom, "%S class atom did not match %x:%x\n",ControlsList[i],test_atom,atom);
              ok(!lstrcmpW(cls.lpszClassName,ControlsList[i]),"GetClassName returned incorrect name\n");
           }
        }
    }
}

void Test_RealGetWindowClass()
{
    int testNo;
    UINT Result;
    CHAR Buffer[1024];

    Result = RealGetWindowClass( NULL, Buffer, ARRAY_SIZE(Buffer) );
    ok(Result == 0, "Result = %d\n", Result);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "GetLastError() = %ld\n", GetLastError());

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
        if (atom == 0) return;

        /* Create a window */
        hWnd = CreateWindowEx( WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR |
                               WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT | WS_EX_APPWINDOW,
                               RealClassTestData[testNo].ClassName,
                               RealClassTestData[testNo].ClassName,
                               WS_POPUPWINDOW | WS_CLIPSIBLINGS | WS_DLGFRAME | WS_OVERLAPPED |
                               WS_MINIMIZEBOX | WS_MAXIMIZEBOX | DS_3DLOOK | DS_SETFONT | DS_MODALFRAME,
                               CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                               NULL, NULL, 0, 0);

        /* Do we expect a HWND at all? */
        if (RealClassTestData[testNo].ExpectsHwnd)
        {
            ok(hWnd != NULL, "\n");

            if (hWnd != NULL)
            {
                /* Get the "real" class name */
                Result = RealGetWindowClass( hWnd, Buffer, ARRAY_SIZE(Buffer) );
                printf("Buffer: %s\nExpectedClassNameBefore(%d): %s\n", Buffer, testNo, RealClassTestData[testNo].ExpectedClassNameBefore);
                ok( Result != 0, "\n" );
                ok( strcmp( Buffer, RealClassTestData[testNo].ExpectedClassNameBefore ) == 0, "\n" );

                /* Call a function that requires a dialog window */
                DefDlgProcA( hWnd, DM_SETDEFID, IDCANCEL, 0 );

                /* Get the "real" class name again */
                Result = RealGetWindowClass( hWnd, Buffer, ARRAY_SIZE(Buffer) );
                printf("Buffer: %s\nExpectedClassNameAfter(%d): %s\n", Buffer, testNo, RealClassTestData[testNo].ExpectedClassNameAfter);
                ok( Result != 0, "\n" );
                ok( strcmp( Buffer, RealClassTestData[testNo].ExpectedClassNameAfter ) == 0, "\n" );
            }
        }
        else
        {
            ok(hWnd == NULL, "\n");
        }

        /* Cleanup */
        DestroyWindow(hWnd);
        UnregisterClass(RealClassTestData[testNo].ClassName, 0);
    }

}

START_TEST(RealGetWindowClass)
{
    Test_ClassAtoms();
    Test_RealGetWindowClass();
}

