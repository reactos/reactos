/*++ BUILD Version: 0001

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

      userdata.h

Abstract:

    Header file for the USER Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    win32k.sys is placed into the structures shown here.

Revisions:
    Sept 15 1997    MCostea     Added CriticalSection object

--*/

#ifndef _USERDATA_H_
#define _USERDATA_H_

//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundaries. Alpha support may
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following define when adding an object type.

#define USER_NUM_PERF_OBJECT_TYPES 2

//
// Create a section like this for each performance object you add
//

//----------------------------------------------------------------------------
//
//  USER Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

//
// If adding more counters please preserve NumTotals as the first one and
// NumInputContexts as the last one.  The counters are sorted in perfmon
//
#define NUM_TOTALS_OFFSET         sizeof(DWORD)
#define NUM_FREEONES_OFFSET       NUM_TOTALS_OFFSET     + sizeof(DWORD)
#define NUM_WINDOWS_OFFSET        NUM_FREEONES_OFFSET       + sizeof(DWORD)
#define NUM_MENUS_OFFSET          NUM_WINDOWS_OFFSET    + sizeof(DWORD)
#define NUM_CURSORS_OFFSET        NUM_MENUS_OFFSET      + sizeof(DWORD)
#define NUM_SETWINDOWPOS_OFFSET   NUM_CURSORS_OFFSET    + sizeof(DWORD)
#define NUM_HOOKS_OFFSET          NUM_SETWINDOWPOS_OFFSET   + sizeof(DWORD)
#define NUM_CLIPDATAS_OFFSET      NUM_HOOKS_OFFSET      + sizeof(DWORD)
#define NUM_CALLPROCS_OFFSET      NUM_CLIPDATAS_OFFSET  + sizeof(DWORD)
#define NUM_ACCELTABLES_OFFSET    NUM_CALLPROCS_OFFSET  + sizeof(DWORD)
#define NUM_DDEACCESS_OFFSET      NUM_ACCELTABLES_OFFSET    + sizeof(DWORD)
#define NUM_DDECONVS_OFFSET       NUM_DDEACCESS_OFFSET  + sizeof(DWORD)
#define NUM_DDEXACTS_OFFSET       NUM_DDECONVS_OFFSET   + sizeof(DWORD)
#define NUM_MONITORS_OFFSET       NUM_DDEXACTS_OFFSET   + sizeof(DWORD)
#define NUM_KBDLAYOUTS_OFFSET     NUM_MONITORS_OFFSET   + sizeof(DWORD)
#define NUM_KBDFILES_OFFSET       NUM_KBDLAYOUTS_OFFSET + sizeof(DWORD)
#define NUM_WINEVENTHOOKS_OFFSET  NUM_KBDFILES_OFFSET   + sizeof(DWORD)
#define NUM_TIMERS_OFFSET         NUM_WINEVENTHOOKS_OFFSET  + sizeof(DWORD)
#define NUM_INPUTCONTEXTS_OFFSET  NUM_TIMERS_OFFSET     + sizeof(DWORD)

#define SIZE_OF_USER_PERFORMANCE_DATA \
                    NUM_INPUTCONTEXTS_OFFSET + sizeof(DWORD)

typedef struct _USER_DATA_DEFINITION {
    PERF_OBJECT_TYPE           UserObjectType;
    PERF_COUNTER_DEFINITION    NumTotals;
    PERF_COUNTER_DEFINITION    NumFreeOnes;
    PERF_COUNTER_DEFINITION    NumWindows;
    PERF_COUNTER_DEFINITION    NumMenus;
    PERF_COUNTER_DEFINITION    NumCursors;
    PERF_COUNTER_DEFINITION    NumSetwindowPos;
    PERF_COUNTER_DEFINITION    NumHooks;
    PERF_COUNTER_DEFINITION    NumClipdatas;
    PERF_COUNTER_DEFINITION    NumCallProcs;
    PERF_COUNTER_DEFINITION    NumAccelTables;
    PERF_COUNTER_DEFINITION    NumDDEAccess;
    PERF_COUNTER_DEFINITION    NumDDEConvs;
    PERF_COUNTER_DEFINITION    NumDDEXActs;
    PERF_COUNTER_DEFINITION    NumMonitors;
    PERF_COUNTER_DEFINITION    NumKBDLayouts;
    PERF_COUNTER_DEFINITION    NumKBDFiles;
    PERF_COUNTER_DEFINITION    NumWinEventHooks;
    PERF_COUNTER_DEFINITION    NumTimers;
    PERF_COUNTER_DEFINITION    NumInputContexts;
} USER_DATA_DEFINITION;
#define NUM_USER_COUNTERS    (sizeof(USER_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/sizeof(PERF_COUNTER_DEFINITION)

//----------------------------------------------------------------------------
//
//  Critical Section object definition
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define CS_EXENTER_OFFSET         sizeof(DWORD)
#define CS_SHENTER_OFFSET         CS_EXENTER_OFFSET + sizeof(DWORD)
#define CS_EXTIME_OFFSET          CS_SHENTER_OFFSET + sizeof(DWORD)

#define SIZE_OF_CS_PERFORMANCE_DATA \
                    CS_EXTIME_OFFSET + sizeof(DWORD)

typedef struct _CS_DATA_DEFINITION {
    PERF_OBJECT_TYPE           CSObjectType;
    PERF_COUNTER_DEFINITION    CSExEnter;
    PERF_COUNTER_DEFINITION    CSShEnter;
    PERF_COUNTER_DEFINITION    CSExTime;
} CS_DATA_DEFINITION;
#define NUM_CS_COUNTERS    (sizeof(CS_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/sizeof(PERF_COUNTER_DEFINITION)

#pragma pack ()

#endif //_USERDATA_H_

