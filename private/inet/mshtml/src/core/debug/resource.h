//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       resource.h
//
//  History:    10-06-93   ErikGav   Created
//
//----------------------------------------------------------------------------


#define tmcOk                   IDOK
#define tmcCancel               IDCANCEL
#define tmcStatic               -1
#define tmcListbox              101
#define tmcEnabled              102
#define tmcDisk                 103
#define tmcBreak                105
#define tmcEnableAll            107
#define tmcDisableAll           108

// Simulate failure dialog resources
#define IDD_SIMFAIL                 200
#define ID_LBLFAIL                  201
#define ID_TXTFAIL                  202
#define ID_LBLINTERVAL              203
#define ID_TXTINTERVAL              204
#define ID_LBLCOUNT                 205
#define ID_TXTCOUNT                 206
#define ID_BTNRESET                 207
#define ID_BTNNEVER                 208
#define ID_BTNUPDATE                209

/* Control Ids for the two dialogs - HeapMon & BlockEdit */
 
#define IDD_HEAPMON                 1000
#define IDG_HEADER                  1010
#define IDT_ALLOCNUM                1020
#define IDT_LIVEBLOCK               1030
#define IDT_FRAGIDX                 1040
#define IDE_ALLOCNUM                1050
#define IDE_LIVEBLOCK               1060
#define IDE_FRAGIDX                 1070
#define IDT_VIRTUAL                 1080
#define IDT_FILL                    1090
#define IDE_BLOCKS                  1100
#define IDT_BLOCKS                  1110
#define IDC_REFRESH                 1120
#define IDC_CLOSE                   1130
#define IDC_FILEDUMP                1140
#define IDE_FILEDUMP                1150
#define IDC_BLOCKLIST               1160
#define IDE_LIVEMEMORY              1200
#define IDG_SORTBY                  1210
#define IDC_SORTCFLAG               1220
#define IDC_SORTTYPE                1221
#define IDC_SORTNAME                1222
#define IDC_SORTADDRESS             1230
#define IDC_SORTALLOC               1240
#define IDC_SORTSIZE                1250
#define IDT_LIVEMEMORY              1260
#define IDT_RANGELOW                1280
#define IDE_RANGELOW                1270
#define IDT_RANGEHIGH               1310
#define IDE_RANGEHIGH               1290
#define IDT_RANGEEXTENT             1330
#define IDE_RANGEEXTENT             1300
#define IDC_SUMSEL                  1190
#define IDE_SUMSEL                  1320
#define IDC_HEAPLIST                1170
#define IDT_HEAPNAME                1180
#define IDC_VIRTUAL					1400
#define IDC_LOAD_SYMBOLS			1410
#define IDC_DUMPHEAPS               1411

#define IDD_BLOCKEDIT               2000
#define IDT_BE_ADDRESS              2010
#define IDT_BE_ALLOCNUM             2030
#define IDT_BE_SIZE                 2040
#define IDE_BE_ADDRESS              2050
#define IDE_BE_ALLOCNUM             2070
#define IDE_BE_SIZE                 2080
#define IDC_BE_MEMORY               2100
#define IDG_DATATYPE                2090
#define IDC_DWORD                   2110
#define IDC_WORD                    2120
#define IDC_BYTE                    2130
#define IDT_CALLSTACK               2170
#define IDC_CALLSTACK               2180

#define IDB_ICONIC					3000
#define IIC_ICONIC					3010

#define BROWSE_DLG					3000
#define IDC_TRACE_LIST				3001
#define ID_ADD						3002
#define ID_VIEW						3003
#define ID_REFRESH					3004
#define IDC_TOTALMEM                1000
#define IDC_CODEMEM                 1001
#define IDC_DATAMEM                 1002
#define IDC_RESERVEMEM              1003
#define IDC_READONLY                1004
#define IDC_READWRITE               1005
#define IDC_STATIC                  -1
#define ID_DUMP						3005

/* Perftags */

#define IDD_PERFTAGS                9700
#define IDC_TAGLIST                 9702
#define IDC_ENABLEALL               9703
#define IDC_DISABLEALL              9704
#define IDC_CLEARLOG                9705
#define IDC_DUMPLOG                 9706
#define IDC_DELETELOG               9707
//was:  IDC_REFRESH                 9708
#define IDC_STARTSTOP               9709
#define IDC_CLEARMETER              9710

#define IDD_PERFMETER               9800

#define IDC_SHOWMETERS              9802
