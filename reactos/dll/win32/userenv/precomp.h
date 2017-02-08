#include <stdio.h>
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
#define NTOS_MODE_USER
#include <ndk/sefuncs.h>
#include <ndk/rtlfuncs.h>
#include <userenv.h>
#include <sddl.h>
#include <shlobj.h>

#include "internal.h"
#include "resources.h"
