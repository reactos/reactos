/*
 * PROJECT:         ReactOS Utility Manager (Accessibility)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Pre-compiled header file
 * COPYRIGHT:       Copyright 2019-2020 George Bișoc (george.bisoc@reactos.org)
 */

#ifndef _UTILMAN_H
#define _UTILMAN_H

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <strsafe.h>

#include "resource.h"

/* DEFINES ********************************************************************/

#define MAX_BUFFER 256

/* TYPES **********************************************************************/

typedef BOOL (WINAPI *EXECDLGROUTINE)(VOID);

#endif /* _UTILMAN_H */

/* EOF */
