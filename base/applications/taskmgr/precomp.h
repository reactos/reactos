#ifndef __PRECOMP_H
#define __PRECOMP_H

#ifndef UNICODE
#error Task-Manager uses NDK functions, so it can only be compiled with Unicode support enabled!
#endif

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <aclapi.h>
#include <commctrl.h>
#include <shellapi.h>

#include "column.h"
#include "taskmgr.h"
#include "perfdata.h"
#include "perfpage.h"
#include "about.h"
#include "procpage.h"
#include "proclist.h"
#include "affinity.h"
#include "applpage.h"
#include "dbgchnl.h"
#include "debug.h"
#include "endproc.h"
#include "graph.h"
#include "graphctl.h"
#include "optnmenu.h"
#include "priority.h"
#include "run.h"
#include "trayicon.h"

#endif /* __PRECOMP_H */
