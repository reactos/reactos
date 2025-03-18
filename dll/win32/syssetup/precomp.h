#ifndef _SYSSETUP_PCH_
#define _SYSSETUP_PCH_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <setupapi.h>
#include <syssetup/syssetup.h>
#include <pseh/pseh2.h>
#include <cfgmgr32.h>

#include <setupapi_undoc.h>

#include "globals.h"
#include "resource.h"

#endif /* _SYSSETUP_PCH_ */
