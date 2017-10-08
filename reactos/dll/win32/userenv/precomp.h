#ifndef _USERENV_PCH_
#define _USERENV_PCH_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
#include <objbase.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <userenv.h>
#include <strsafe.h>

#include "internal.h"

#endif /* _USERENV_PCH_ */
