/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Helpid.h

Abstract:

    This module contains the # defines for helpid's and identifiers for
    Windbg's menus and menu items.

Author:

    Griffith Wm. Kadnier (v-griffk) 14-August-1992

Environment:

    Win32, User Mode

--*/



// Menu Resource Signature

#define MENU_SIGNATURE              0x4000


// Accelerator IDs

#define IDA_BASE                    10000
#define IDA_FINDNEXT                10001



//Base menu ID

#define IDM_BASE                    100

//
// File
//

#define IDM_FILE                    16484
#define IDM_FILE_OPEN               16486
#define IDM_FILE_CLOSE              16488
#define IDM_FILE_EXIT               16492
#define IDM_FILE_FIRST              16484
#define IDM_FILE_LAST               16492

//
// Edit
//

#define IDM_EDIT                    16584
#define IDM_EDIT_REDO               16586
#define IDM_EDIT_CUT                16587
#define IDM_EDIT_COPY               16588
#define IDM_EDIT_PASTE              16589
#define IDM_EDIT_DELETE             16590
#define IDM_EDIT_FIND               16591
#define IDM_EDIT_REPLACE            16592
#define IDM_EDIT_READONLY           16593
#define IDM_EDIT_PROPERTIES         16594
#define IDM_EDIT_FIRST              16584
#define IDM_EDIT_LAST               16594

//
// View
//

#define IDM_VIEW                    16684
#define IDM_VIEW_LINE               16685
#define IDM_VIEW_FUNCTION           16686
#define IDM_VIEW_TOGGLETAG          16687
#define IDM_VIEW_NEXTTAG            16688
#define IDM_VIEW_PREVIOUSTAG        16689
#define IDM_VIEW_CLEARALLTAGS       16690
#define IDM_VIEW_TOOLBAR            16691
#define IDM_VIEW_STATUS             16692
#define IDM_VIEW_FIRST              16684
#define IDM_VIEW_LAST               16692

//
// Program
//

#define IDM_PROGRAM                 16784
#define IDM_PROGRAM_OPEN            16785
#define IDM_PROGRAM_CLOSE           16786
#define IDM_PROGRAM_SAVE            16787
#define IDM_PROGRAM_SAVEAS          16788
#define IDM_PROGRAM_DELETE          16789
#define IDM_PROGRAM_SAVE_DEFAULTS   16790
#define IDM_PROGRAM_FIRST           16784
#define IDM_PROGRAM_LAST            16790

//
// Run
//

#define IDM_RUN                     16884
#define IDM_RUN_RESTART             16885
#define IDM_RUN_STOPDEBUGGING       16886
#define IDM_RUN_GO                  16887
#define IDM_RUN_TOCURSOR            16888
#define IDM_RUN_TRACEINTO           16889
#define IDM_RUN_STEPOVER            16890
#define IDM_RUN_HALT                16891
#define IDM_RUN_SET_THREAD          16892
#define IDM_RUN_SET_PROCESS         16893
#define IDM_RUN_SOURCE_MODE         16894
#define IDM_RUN_ATTACH              16895
#define IDM_RUN_GO_HANDLED          16896
#define IDM_RUN_GO_UNHANDLED        16897
#define IDM_RUN_FIRST               16884
#define IDM_RUN_LAST                16897

//
// Debug
//

#define IDM_DEBUG                   16984
#define IDM_DEBUG_CALLS             16985
#define IDM_DEBUG_SETBREAK          16986
#define IDM_DEBUG_QUICKWATCH        16987
//#define IDM_DEBUG_WATCH             16988
#define IDM_DEBUG_MODIFY            16989
#define IDM_DEBUG_FIRST             16984
#define IDM_DEBUG_LAST              16989

//
// Options
//

#define IDM_OPTIONS                 17084
#define IDM_OPTIONS_RUN             17085
#define IDM_OPTIONS_DEBUG           17086
#define IDM_OPTIONS_MEMORY          17087
#define IDM_OPTIONS_WATCH           17088
#define IDM_OPTIONS_DISASSEMBLY     17089
#define IDM_OPTIONS_ENVIRON         17090
#define IDM_OPTIONS_WORKSPACE       17091
#define IDM_OPTIONS_COLOR           17092
#define IDM_OPTIONS_FONTS           17093
#define IDM_OPTIONS_LOCAL           17094
#define IDM_OPTIONS_CPU             17095
#define IDM_OPTIONS_FLOAT           17096
#define IDM_OPTIONS_KD              17097
#define IDM_OPTIONS_CALLS           17098
#define IDM_OPTIONS_USERDLL         17099
#define IDM_OPTIONS_DBGDLL          17100
#define IDM_OPTIONS_EXCEPTIONS      17101

#define IDM_OPTIONS_FIRST           17084
#define IDM_OPTIONS_LAST            17101


//
// Window
//

#define IDM_WINDOW                  17184
#define IDM_WINDOW_NEWWINDOW        17185
#define IDM_WINDOW_CASCADE          17186
#define IDM_WINDOW_TILE             17187
#define IDM_WINDOW_ARRANGE          17188
#define IDM_WINDOW_ARRANGE_ICONS    17189
#define IDM_WINDOW_SOURCE_OVERLAY   17190
#define IDM_WINDOW_WATCH            17191
#define IDM_WINDOW_LOCALS           17192
#define IDM_WINDOW_CPU              17193
#define IDM_WINDOW_DISASM           17194
#define IDM_WINDOW_COMMAND          17195
#define IDM_WINDOW_FLOAT            17196
#define IDM_WINDOW_MEMORY           17197
#define IDM_WINDOW_CALLS            17198
#define IDM_WINDOWCHILD             17199
#define IDM_WINDOW_FIRST            17184
#define IDM_WINDOW_LAST             17198

//
// Help
//

#define IDM_HELP                    17284
#define IDM_HELP_CONTENTS           17285
#define IDM_HELP_SEARCH             17286
#define IDM_HELP_ABOUT              17287
#define IDM_HELP_FIRST              17284
#define IDM_HELP_LAST               17287
