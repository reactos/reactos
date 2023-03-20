#ifndef _REGEDIT_H
#define _REGEDIT_H

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdio.h>
#include <aclapi.h>
#include <shellapi.h>
#include <strsafe.h>
#include <stdlib.h>
#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include "main.h"
#include "hexedit.h"
#include "security.h"
#include "wine/debug.h"

#endif /* _REGEDIT_H */
