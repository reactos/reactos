#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <limits.h>
#include <process.h>

#define INC_OLE2

#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <commctrl.h>
#include <shlobj.h>         // Windows Shell API
#include <shellapi.h>
#include <shlwapi.h>
#include <setupapi.h>
#include <commdlg.h>


// Standard function to prompt an Assertion False
void ShowAssert(LPSTR condition, UINT line, LPSTR file);


// First, a sanity check
#ifdef Assert
#undef Assert
#endif

#define Assert(condition)   \
    {                                   \
        if (!(condition))       \
        {                       \
            static char szFileName[] = __FILE__; \
            static char szExp[] = #condition;  \
            ShowAssert(szExp,__LINE__,szFileName);  \
        }   \
    }







// workspace code
#include "ws_comon.h"
#include "tsgllist.h"
#include "tlist.h"
#include "ws_misc.h"


#include "resource.h"

#include "dbgwiz.h"
#include "cfgdata.h"
#include "pagedefs.h"
#include "pagedefs.hxx"

#include "wizmisc.h"

#include "copydlg.h"
#include "cfgfile.h"
