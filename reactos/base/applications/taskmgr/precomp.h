#ifndef __PRECOMP_H
#define __PRECOMP_H

#ifndef UNICODE
#error Task-Manager uses NDK functions, so it can only be compiled with Unicode support enabled!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winreg.h>
#include <commctrl.h>
#include <shellapi.h>

#include "column.h"
#include "taskmgr.h"
#include "perfdata.h"
#include "procpage.h"
#include "applpage.h"
#include "dbgchnl.h"
#include "endproc.h"
#include "graph.h"
#include "graphctl.h"
#include "optnmenu.h"
#include "run.h"
#include "trayicon.h"
#include "shutdown.h"

#endif /* __PRECOMP_H */
