/*****************************************************************************

  Fraginator

*****************************************************************************/

#define NDEBUG

#include "Fraginator.h"
#include "Mutex.h"
#include "DriveVolume.h"
#include "Defragment.h"
#include "MainDialog.h"
#include "resource.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

HINSTANCE   GlobalHInstance = NULL;
Defragment *Defrag = NULL;

INT WINAPI
wWinMain(HINSTANCE HInstance,
         HINSTANCE hPrev,
         LPWSTR Cmd,
         int iCmd)
{
    INITCOMMONCONTROLSEX InitControls;

    // debugging crap
#ifndef NDEBUG
    _CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag (_CRTDBG_REPORT_FLAG));
    _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode (_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile (_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

    GlobalHInstance = HInstance;

    // We want our progress bar! NOW!
    InitControls.dwSize = sizeof (InitControls);
    InitControls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx (&InitControls);

    if (!CheckWinVer())
    {
        MessageBox (GetDesktopWindow(), L"Sorry, this program requires Windows 2000.", L"Error", MB_OK);
        return (0);
    }

    DialogBox (HInstance, MAKEINTRESOURCE (IDD_MAIN), GetDesktopWindow(), MainDialogProc);

#if 0
    AllocConsole ();
    if (_CrtDumpMemoryLeaks ())
        MessageBox (NULL, L"Click OK to quit", L"Leaks", MB_OK);
#endif

    return (0);
}

