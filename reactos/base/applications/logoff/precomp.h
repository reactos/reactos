#ifndef __SHUTDOWN_PRECOMP_H
#define __SHUTDOWN_PRECOMP_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include <stdio.h>
//#include <stdlib.h>
#include <tchar.h>
//#include <reason.h> //shutdown codes

#include "resource.h"

// misc.c
INT AllocAndLoadString(OUT LPTSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT uID);

#endif
