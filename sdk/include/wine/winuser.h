
#pragma once

#include <psdk/winuser.h>

/* Wine's <winuser.h> declares these publicly, while ReactOS keeps them in
   <reactos/undocuser.h>. put them here for less wine source mods */

#ifndef DCX_USESTYLE
#define DCX_USESTYLE     0x00010000
#endif

#ifndef DCX_NORECOMPUTE
#define DCX_NORECOMPUTE  0x00100000
#endif
