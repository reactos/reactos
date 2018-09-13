#ifndef I_PRIVCID_H_
#define I_PRIVCID_H_
#ifndef RC_INVOKED
#pragma INCMSG("--- Beg 'privcid.h'")
#endif

#ifndef X_MSHTMCID_H_
#define X_MSHTMCID_H_
#include "mshtmcid.h"
#endif

//----------------------------------------------------------------------------
//
// Private Command IDs.
//
//----------------------------------------------------------------------------

#define IDM_TABKEY                  6000
#define IDM_SHTABKEY                6001
#define IDM_RETURNKEY               6002
#define IDM_ESCKEY                  6003

#if DBG == 1
// dependencies - shdocvw\resource.h
#define IDM_DEBUG_TRACETAGS         6004
#define IDM_DEBUG_RESFAIL           6005
#define IDM_DEBUG_DUMPOTRACK        6006
#define IDM_DEBUG_BREAK             6007
#define IDM_DEBUG_VIEW              6008
#define IDM_DEBUG_DUMPTREE          6009
#define IDM_DEBUG_DUMPLINES         6010
#define IDM_DEBUG_LOADHTML          6011
#define IDM_DEBUG_SAVEHTML          6012
#define IDM_DEBUG_MEMMON            6013
#define IDM_DEBUG_METERS            6014
#define IDM_DEBUG_DUMPDISPLAYTREE   6015
#define IDM_DEBUG_DUMPFORMATCACHES  6016
#endif

// IE4 Shdocvw Messages

#define IDM_SHDV_FINALTITLEAVAIL         6020
#define IDM_SHDV_MIMECSETMENUOPEN        6021
#define IDM_SHDV_PRINTFRAME              6022
#define IDM_SHDV_PUTOFFLINE              6022
#define IDM_SHDV_GOBACK                  6024   // different from IDM_GOBACK
#define IDM_SHDV_GOFORWARD               6025   // different from ISM_GOFORWARD
#define IDM_SHDV_CANGOBACK               6026
#define IDM_SHDV_CANGOFORWARD            6027
#define IDM_SHDV_CANSUPPORTPICS          6028
#define IDM_SHDV_CANDEACTIVATENOW        6029
#define IDM_SHDV_DEACTIVATEMENOW         6030
#define IDM_SHDV_NODEACTIVATENOW         6031
#define IDM_SHDV_SETPENDINGURL           6032
#define IDM_SHDV_ISDRAGSOURCE            6033
#define IDM_SHDV_DOCFAMILYCHARSET        6034
#define IDM_SHDV_DOCCHARSET              6035
#define IDM_SHDV_GETMIMECSETMENU         6036
#define IDM_SHDV_GETFRAMEZONE            6037
#define IDM_SHDV_CANDOCOLORSCHANGE       6038
#define IDM_SHDV_ONCOLORSCHANGE			 6039

// Flavors of refresh

#define IDM_REFRESH_TOP                  6041   // Normal refresh, topmost doc
#define IDM_REFRESH_THIS                 6042   // Normal refresh, nearest doc
#define IDM_REFRESH_TOP_FULL             6043   // Full refresh, topmost doc
#define IDM_REFRESH_THIS_FULL            6044   // Full refresh, nearest doc

#define IDM_DEFAULTBLOCK                 6046

// placeholder for context menu extensions
#define IDM_MENUEXT_PLACEHOLDER          6047

// IE5 Webcheck messages
#define IDM_DWNH_SETDOWNLOAD             6048

// Undo hack command for VID to force preservation of the undo stack across
// arbitrary operations. Arye.
#define IDM_PRESERVEUNDOALWAYS           6049

// IE5 Shdocvw messages
#define IDM_ONPERSISTSHORTCUT            6050
#define IDM_SHDV_GETFONTMENU             6051
#define IDM_SHDV_FONTMENUOPEN            6052
#define IDM_SAVEASTHICKET                6053
#define IDM_SHDV_GETDOCDIRMENU           6054
#define IDM_SHDV_ADDMENUEXTENSIONS       6055
#define IDM_SHDV_PAGEFROMPOSTDATA        6056

#define IDM_GETSWITCHTIMERS              6998   // Used by MSHTMPAD for perf timings
#define IDM_WAITFORRECALC                6999   // Used by MSHTMPAD for perf timings

// JuliaC -- This is hack for InfoViewer's "Font Size" toolbar button
// For details, please see bug 45627
#define IDM_INFOVIEW_ZOOM                7000
#define IDM_INFOVIEW_GETZOOMRANGE        7001

// IOleCommandTarget IDs
#define IDM_GETPUNKCONTROL               6048

#ifndef RC_INVOKED
#pragma INCMSG("--- End 'privcid.h'")
#endif
#else
#ifndef RC_INVOKED
#pragma INCMSG("*** Dup 'privcid.h'")
#endif
#endif
