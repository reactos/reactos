/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ContxtHlp.h

Abstract:

    This module contains the #defines for context identifiers, plus the array
    of control identifier/context identifier pairs for context-sensitive help.

Author:

    Julie Solon (julieso) 22-December-1997

Environment:

    Win32, User Mode

--*/

// Memory Options

#define IDH_MEMADDR 1000
#define IDH_MEMFMT 1001
#define IDH_FILL 1002
#define IDH_LIVE 1003

// Exceptions

#define IDH_ENUMBER 1100
#define IDH_ENAME 1101
#define IDH_ACTION 1102
#define IDH_CMD1 1103
#define IDH_CMD2 1104
#define IDH_ELIST 1105
#define IDH_EXCEPTADD 1106
#define IDH_EXCEPTDEL 1107
#define IDH_EXCEPTIONS 1108

// Find

#define IDH_WHAT 1200
#define IDH_WHOLE 1201
#define IDH_CASE 1202
#define IDH_REGEXPR 1203
#define IDH_UP 1204
#define IDH_DOWN 1205
#define IDH_FINDNEXT 1206
#define IDH_TAGALL 1207

// Go to Address

#define IDH_ADDR 1300

// Go to Line

#define IDH_LINE 1400

// Messages

#define IDH_SINGLE 1500
#define IDH_CLASS 1501
#define IDH_MSG 1502
#define IDH_MSGCLASS 1503

// Processes

#define IDH_SETPROCESS 1600
#define IDH_SELECTPROCESS 1601

// Breakpoints

#define IDH_BP 1700
#define IDH_LOCATION 1701
#define IDH_WNDPROC 1702
#define IDH_EXPR 1703
#define IDH_LEN 1704
#define IDH_BPMSG 1705
#define IDH_COUNT 1706
#define IDH_LEFT 1707
#define IDH_PROC 1708
#define IDH_THRD 1709
#define IDH_CMD 1710
#define IDH_BREAKPTLIST 1711
#define IDH_BPADD 1712
#define IDH_BPCLEAR 1713
#define IDH_BPCLEARALL 1714
#define IDH_BPENABLE 1715
#define IDH_BPDISABLE 1716
#define IDH_BPMODIFY 1717
#define IDH_BREAKPNTS 1718

#define IDH_BPRESOLVE_LIST 1719
#define IDH_BPRESOLVE_USE 1720

#define IDH_UNRESOLVED_CLEAR 1721
#define IDH_UNRESOLVED_DISABLE 1722
#define IDH_UNRESOLVED_DEFER 1723
#define IDH_UNRESOLVED_QUIET 1724
#define IDH_UNRESOLVED_BREAKPOINTS 1725

#define IDH_FSRESOLVE_LIST 1726
#define IDH_FSRESOLVE_USE 1727
#define IDH_FSRESOLVE_ADD 1728

// Threads

#define IDH_SETTHRD 1800
#define IDH_SELECTTHREAD 1801
#define IDH_FREEZE 1802
#define IDH_FREEZEALL 1804
#define IDH_THAWALL 1805


// Workspaces

#define IDH_PROGRAM 1900
#define IDH_WORKSPACE 1901
#define IDH_NEW 1902
#define IDH_OPEN_WKSP 1903
#define IDH_WKSPNAME 1904
#define IDH_DEFWKSP 1905
#define IDH_DELETE 1906
#define IDH_DEL_WKSP 1907

// Font

#define IDH_FONT 2000
#define IDH_FONTSTYLE 2001
#define IDH_FONTSIZE 2002
#define IDH_DEFFONT 2003

// Quickwatch

#define IDH_QWEXPR 2100
#define IDH_EVALUATE 2101
#define IDH_ADDWATCH 2102
#define IDH_REMOVELAST 2103
#define IDH_PANE 2104
#define IDH_QUICKWATCH 2105

// Add/Edit Transport Layer

#define IDH_DISPNAME 2200
#define IDH_DESC 2201
#define IDH_PATH 2202
#define IDH_PARAMETERS 2203
#define IDH_EDITTRANSPORT 2204

// Locals and Watch window options

#define IDH_FMT 2300
#define IDH_EXPAND 2301
#define IDH_NOTEXPAND 2302

// Attach to a Process

#define IDH_PROCESSLIST 2400

// Workspace (Options)

#define IDH_AUTOSAVE 2498
#define IDH_MANUALSAVE 2499
#define IDH_PROMPT 2500
#define IDH_AUTOOPEN 2501
#define IDH_APPENDLOG 2502
#define IDH_STARTLOG 2503
#define IDH_STOPLOG 2504
#define IDH_LOGFILE 2505
#define IDH_BROWSELOG 2506

// Symbols (Options)

#define IDH_LOAD 2507
#define IDH_DEFER 2508
#define IDH_ANSI 2509
#define IDH_UNICODE 2510
#define IDH_NOSUFFIX 2511
#define IDH_BADSYM_NOLOAD 2512
#define IDH_BADSYM_PROMPT 2513
#define IDH_BADSYM_IGNORE 2514
#define IDH_SYMPATH 2515

// Program (Options)

#define IDH_EXE 2516
#define IDH_ARGS 2517
#define IDH_SPATH 2518
#define IDH_CRASHDUMP 2519

// Debugger (Options)

#define IDH_IGNORECASE 2520
#define IDH_CHILD 2521
#define IDH_GOATTACH 2522
#define IDH_CMDREP 2523
#define IDH_DISCONNECT 2524
#define IDH_VERBOSE 2525
#define IDH_SEGMENT 2526
#define IDH_GTERM 2527
#define IDH_WOW 2528
#define IDH_GCREATE 2529
#define IDH_CONTEXT 2530
#define IDH_MASMEVAL 2531
#define IDH_REG 2532
#define IDH_RADIX 2533

// Disassembler (Options)

#define IDH_DRAW 2534
#define IDH_DSYM 2535
#define IDH_DUC 2536
#define IDH_DISASM 2537

// Source Files (Options)

#define IDH_TABSTOP 2538
#define IDH_SCROLLBAR 2539
#define IDH_SEARCHORDER 2540

// Kernel Debugger (Options)

#define IDH_BP1 2541
#define IDH_KD 2542
#define IDH_GEXIT 2543
#define IDH_BAUD 2544
#define IDH_PORT 2545
#define IDH_CACHE 2546
#define IDH_PLATFORM 2547

// Call Stack (Options)

#define IDH_FP 2548
#define IDH_FUNC 2549
#define IDH_PARAM 2550
#define IDH_MODULE 2551
#define IDH_4DWORDS 2552
#define IDH_RETADDR 2553
#define IDH_DISPLACE 2554
#define IDH_SRC 2555
#define IDH_RUNTIME 2556
#define IDH_MAXFRAMES 2557

// Transport Layer (Options)

#define IDH_TRANSPORT 2558
#define IDH_TLSELECT 2559
#define IDH_TLADD 2560
#define IDH_TLEDIT 2561
#define IDH_TLDEL 2562
#define IDH_TRANSPORT_DISABLED 2563

// Source Code Search Order

#define IDH_MAP 2564
#define IDH_ADD 2565
#define IDH_EDIT 2566
#define IDH_DEL 2567
#define IDH_MOVEUP 2568
#define IDH_MOVEDOWN 2569
#define IDH_DEFPATH 2570
#define IDH_SEARCHPATH 2571

// Source Root Mapping

#define IDH_EDIT_SRCROOT 2572
#define IDH_EDIT_TARGETROOT 2573

// Colors

#define IDH_BASICCOLOR 2600
#define IDH_CUSTOMCOLOR 2601
#define IDH_COLORITEMS 2602
#define IDH_HUE 2603
#define IDH_SAT 2604
#define IDH_LUM 2605
#define IDH_RED 2606
#define IDH_GREEN 2607
#define IDH_BLUE 2608
#define IDH_ADDCOLOR 2609
#define IDH_FORECOLOR 2610
#define IDH_DEFCOLOR 2611
#define IDH_BACKCOLOR 2612
#define IDH_SELECTALL 2613

// Additional topics

#define _Setting_Up_a_User_Mode_Remote_Debugging_Session 2701
#define _WinDbg_Command_Line 2702
