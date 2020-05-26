
#ifndef _WINTRUST_PRECOMP_H
#define _WINTRUST_PRECOMP_H

#include <stdio.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <winnls.h>
#include <winternl.h>
#include <softpub.h>
#include <mscat.h>

#include <wine/debug.h>

#include "wintrust_priv.h"

#endif /* !_WINTRUST_PRECOMP_H */
