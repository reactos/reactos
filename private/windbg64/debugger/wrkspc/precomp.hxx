#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <typeinfo.h>


#include "od.h"


// windbg
#include "asertdbg.h"


//#include "ws_resrc.h"


#include "tlist.h"


/****************************************************************************

    GLOBAL LIMITS CONSTANTS:

****************************************************************************/
#define MAX_MSG_TXT         4096    //Max text width in message boxes
#define MAX_VAR_MSG_TXT     8192    //Max size of a message built at run-time


// wrkspc
#include "ws_misc.h"
#include "ws_comon.h"

#include "wdbg_def.h"

#include "ws_items.h"

#include "ws_defs.h"
#include "ws_impl.h"


#include "ws_items.inl"
#include "ws_defs.inl"


