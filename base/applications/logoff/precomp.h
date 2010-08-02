#ifndef __SHUTDOWN_PRECOMP_H
#define __SHUTDOWN_PRECOMP_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <reason.h> //shutdown codes
#include "resource.h"

// misc.c
INT AllocAndLoadString(OUT LPTSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT uID);

#endif
