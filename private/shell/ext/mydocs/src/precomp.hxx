#ifndef _pch_h
#define _pch_h

#include "windows.h"
#include "windowsx.h"
#include "commctrl.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "shellapi.h"

#include "ccstock.h"
#include "comctrlp.h"
#include "winuserp.h"
#include "shsemip.h"
#include "shlwapip.h"
#include "shlobjp.h"
#include "shell2.h"
#include "shellp.h"
#include "shdocvw.h"
#include "exdisp.h"
#include "shlapip.h"
#undef TerminateThread
#define TerminateThread TerminateThread
#include "advpub.h"

#ifdef DBG
#define DEBUG 1
#endif

#include "common.h"
#include "mdguids.h"

#include "resource.h"
#include "cstrings.h"
#include "idlist.h"

#include "dataobj.h"
#include "dll.h"
#include "enum.h"
//#define DO_OTHER_PAGES 1

#define SHOW_PATHS 1
//#define SHOW_ATTRIBUTES 1



#include "factory.h"
#include "folder.h"
#include "icon.h"
#include "util.h"
#include "prop.h"
#include "wcthk.h"
#include "copyhook.h"
#include "viewcb.h"

// Magic debug flags

#define TRACE_CORE          0x00000001
#define TRACE_FOLDER        0x00000002
#define TRACE_ENUM          0x00000004
#define TRACE_ICON          0x00000008
#define TRACE_DATAOBJ       0x00000010
#define TRACE_IDLIST        0x00000020
#define TRACE_CALLBACKS     0x00000040
#define TRACE_COMPAREIDS    0x00000080
#define TRACE_DETAILS       0x00000100
#define TRACE_QI            0x00000200
#define TRACE_EXT           0x00000400
#define TRACE_UTIL          0x00000800
#define TRACE_SETUP         0x00001000
#define TRACE_PROPS         0x00002000
#define TRACE_COPYHOOK      0x00004000
#define TRACE_FACTORY       0x00008000


#define TRACE_ALWAYS        0xffffffff          // use with caution


#endif
