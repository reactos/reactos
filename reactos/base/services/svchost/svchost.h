/*
 * PROJECT:         ReactOS SvcHost
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            /base/services/svchost/svchost.h
 * PURPOSE:         Provide dll service loader
 * PROGRAMMERS:     Gregor Brunmar (gregor.brunmar@home.se)
 */

#pragma once

/* INCLUDES ******************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>
#include <strsafe.h>

/* DEFINES *******************************************************************/

typedef struct _SERVICE {
    PWSTR Name;
    HINSTANCE hServiceDll;
    LPSERVICE_MAIN_FUNCTION ServiceMainFunc;
    struct _SERVICE *Next;
} SERVICE, *PSERVICE;

/* FUNCTIONS *****************************************************************/

/* EOF */
