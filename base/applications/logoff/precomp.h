#ifndef __SHUTDOWN_PRECOMP_H
#define __SHUTDOWN_PRECOMP_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include "resource.h"

/* misc.c */
INT AllocAndLoadString(OUT LPTSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT uID);

#endif /* __SHUTDOWN_PRECOMP_H */
