/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    precomp.h

Abstract:

    precompiled headers

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
#define FRAMEBUFFER_SIZE (0x8000)
#define LINES_IN_BUFFER (2048)

#include "retypes.h"
//#include <asm/segment.h>
#include "../shared/shared.h"
#include "debug.h"
#include "hardware.h" 
#include "utils.h" 
#include "init.h" 
#include "shell.h" 
#include "trace.h" 
#include "hooks.h" 
#include "patch.h"		// patch the keyboard driver 
#include "symbols.h"
#include "parse.h" 
#include "syscall.h"
#include "bp.h"
#include "scancodes.h"
#include "output.h"
#include "dblflt.h"
#include "pgflt.h"
#include "gpfault.h"
#include "serial.h"
#include "hercules.h"
#include "vga.h"
#include "pice_ver.h"
