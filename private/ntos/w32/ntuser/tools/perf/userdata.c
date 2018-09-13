/*++ BUILD Version: 0001

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    userdata.c

Abstract:

    A file containing the constant data structures used by the Performance
    Monitor data for the USER Extensible Objects.

Revision History:

    Sept 97 MCostea Created
    Oct. 97 MCostea Added Critical Section Object

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include "userctrnm.h"
#include "userdata.h"

//
//  Constant structure initializations
//      defined in userdata.h
//

USER_DATA_DEFINITION UserDataDefinition = {

    {
        0,
        sizeof(UserDataDefinition),
        sizeof(PERF_OBJECT_TYPE),
        USEROBJ,
        NULL,
        USEROBJ,
        NULL,
        PERF_DETAIL_NOVICE,
        NUM_USER_COUNTERS,
        0,
        0,
        0
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        TOTALS,
        NULL,
        TOTALS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_TOTALS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        FREEONES,
        NULL,
        FREEONES,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_FREEONES_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        WINDOWS,
        NULL,
        WINDOWS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_WINDOWS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        MENUS,
        NULL,
        MENUS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_MENUS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        CURSORS,
        NULL,
        CURSORS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_CURSORS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        SETWINDOWPOS,
        NULL,
        SETWINDOWPOS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_SETWINDOWPOS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        HOOKS,
        NULL,
        HOOKS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_HOOKS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        CLIPDATAS,
        NULL,
        CLIPDATAS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_CLIPDATAS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        CALLPROCS,
        NULL,
        CALLPROCS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_CALLPROCS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        ACCELTABLES,
        NULL,
        ACCELTABLES,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_ACCELTABLES_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        DDEACCESS,
        NULL,
        DDEACCESS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DDEACCESS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        DDECONVS,
        NULL,
        DDECONVS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DDECONVS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        DDEXACTS,
        NULL,
        DDEXACTS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DDEXACTS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        MONITORS,
        NULL,
        MONITORS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_MONITORS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        KBDLAYOUTS,
        NULL,
        KBDLAYOUTS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_KBDLAYOUTS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        KBDFILES,
        NULL,
        KBDFILES,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_KBDFILES_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        WINEVENTHOOKS,
        NULL,
        WINEVENTHOOKS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_WINEVENTHOOKS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        TIMERS,
        NULL,
        TIMERS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_TIMERS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        INPUTCONTEXTS,
        NULL,
        INPUTCONTEXTS,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_INPUTCONTEXTS_OFFSET
    }

};



CS_DATA_DEFINITION CSDataDefinition = {

    {
        sizeof(CS_DATA_DEFINITION) + SIZE_OF_CS_PERFORMANCE_DATA,
        sizeof(CS_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        CSOBJ,
        NULL,
        CSOBJ,
        NULL,
        PERF_DETAIL_NOVICE,
        NUM_CS_COUNTERS,
        0,
        PERF_NO_INSTANCES,
        0
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        EXENTER,
        NULL,
        EXENTER,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_VALUE | PERF_SIZE_DWORD,
        sizeof(DWORD),
        CS_EXENTER_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        SHENTER,
        NULL,
        SHENTER,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_VALUE | PERF_SIZE_DWORD,
        sizeof(DWORD),
        CS_SHENTER_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        EXTIME,
        NULL,
        EXTIME,
        NULL,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        CS_EXTIME_OFFSET
    }
};

