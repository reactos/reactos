
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shell2.h>
#include <shlobj.h>
#include <prsht.h>

#include "id.h"



BOOL LoadComputerObjectAlertPage(HWND hwnd);


//
//  The following is used to display the alert page for the computer object.
//

typedef void (*PASLOADCOMPUTEROBJECTALERTPAGE)(HWND);

