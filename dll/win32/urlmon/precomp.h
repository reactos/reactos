
#ifndef _URLMON_PRECOMP_H
#define _URLMON_PRECOMP_H

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "urlmon_main.h"

#include <winreg.h>
#include <advpub.h>
#define NO_SHLWAPI_REG
#include <shlwapi.h>

#include <wine/debug.h>

#endif /* !_URLMON_PRECOMP_H */
