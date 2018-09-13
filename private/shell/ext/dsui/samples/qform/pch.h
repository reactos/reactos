#ifndef _pch_h
#define _pch_h

//
// NB: Query forms are currently UNICODE only, support for ANSI and Win9x will
//     come post NT5 beta 1
//

#ifndef UNICODE
#error "Query forms only build UNICODE currently"
#endif

#include "windows.h"
#include "windowsx.h"
#include "commctrl.h"
#include "shellapi.h"
#include "shlobj.h"
#include "cmnquery.h"
#include "dsquery.h"

#include "common.h"
#include "dll.h"
#include "unknown.h"

#endif
