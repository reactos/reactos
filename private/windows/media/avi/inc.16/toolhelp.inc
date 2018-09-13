;**************************************************************************
;*  TOOLHELP.INC - toolhelp.dll functions, types, and definitions
;*
;*	Assembly-language public include file for TOOLHELP.DLL
;*
;*	Version 1.0								      *
;*
;* Copyright (c) 1992-1994, Microsoft Corp.	All rights reserved.
;**************************************************************************

;** General symbol values
MAX_DATA        EQU     11
MAX_PATH16      EQU     255
MAX_MODULE_NAME EQU     8 + 1           ;Leave room for the zero and padding
MAX_CLASSNAME   EQU     255

; ----- Global heap walking -----

GLOBALENTRY STRUC
ge_dwSize       DD      ?               ;Structure version
ge_dwAddress    DD      ?               ;This block's address
ge_dwBlockSize  DD      ?               ;This block's size
ge_hBlock       DW      ?               ;This block's handle
ge_wcLock       DW      ?               ;Lock count
ge_wcPageLock   DW      ?               ;Page lock count
ge_wFlags       DW      ?               ;Block flags
ge_wHeapPresent DW      ?               ;Size of available local heap
ge_hOwner       DW      ?               ;Owner handle
ge_wType        DW      ?               ;Block type
ge_wData        DW      ?               ;Type-specific data
ge_dwNext       DD      ?               ;Pointer to next block
ge_dwNextAlt    DD      ?               ;Next free/LRU block in chain
GLOBALENTRY ENDS

; ** GlobalFirst()/GlobalNext() flags
GLOBAL_ALL      EQU     0
GLOBAL_LRU      EQU     1
GLOBAL_FREE     EQU     2

; ** GLOBALENTRY.ge_wType entries
GT_UNKNOWN      EQU     0
GT_DGROUP       EQU     1
GT_DATA         EQU     2
GT_CODE         EQU     3
GT_TASK         EQU     4
GT_RESOURCE     EQU     5
GT_MODULE       EQU     6
GT_FREE         EQU     7
GT_INTERNAL     EQU     8
GT_SENTINEL     EQU     9
GT_BURGERMASTER EQU     10

; ** if GLOBALENTRY.wType==GT_RESOURCE, the following is GLOBALENTRY.wData:
GD_USERDEFINED          EQU     0
GD_CURSORCOMPONENT      EQU     1
GD_BITMAP               EQU     2
GD_ICONCOMPONENT        EQU     3
GD_MENU                 EQU     4
GD_DIALOG               EQU     5
GD_STRING               EQU     6
GD_FONTDIR              EQU     7
GD_FONT                 EQU     8
GD_ACCELERATORS         EQU     9
GD_RCDATA               EQU     10
GD_ERRTABLE             EQU     11
GD_CURSOR               EQU     12
GD_ICON                 EQU     14
GD_NAMETABLE            EQU     15
GD_MAX_RESOURCE         EQU     15

;** GLOBALENTRY.wFlags
GF_PDB_OWNER    EQU     100h            ;Low byte is KERNEL flags

; ----- Local heap walking -----

LOCALENTRY STRUC
le_dwSize       DD      ?
le_hHandle      DW      ?
le_wAddress     DW      ?
le_wSize        DW      ?
le_wFlags       DW      ?
le_wcLock       DW      ?
le_wType        DW      ?
le_hHeap        DW      ?
le_wHeapType    DW      ?
le_wNext        DW      ?
LOCALENTRY ENDS

; ** LOCALENTRY.wHeapType flags
NORMAL_HEAP     EQU     0
USER_HEAP       EQU     1
GDI_HEAP        EQU     2

;** LOCALENTRY.wFlags
LF_FIXED                EQU     1
LF_FREE                 EQU     2
LF_MOVEABLE             EQU     4

;** LOCALENTRY.wType
LT_NORMAL               EQU     0
LT_FREE                 EQU     0ffh
LT_GDI_PEN              EQU     1
LT_GDI_BRUSH            EQU     2
LT_GDI_FONT             EQU     3
LT_GDI_PALETTE          EQU     4
LT_GDI_BITMAP           EQU     5
LT_GDI_RGN              EQU     6
LT_GDI_DC               EQU     7
LT_GDI_DISABLED_DC      EQU     8
LT_GDI_METADC           EQU     9
LT_GDI_METAFILE         EQU     10
LT_GDI_MAX              EQU     LT_GDI_METAFILE
LT_USER_CLASS           EQU     1
LT_USER_WND             EQU     2
LT_USER_STRING          EQU     3
LT_USER_MENU            EQU     4
LT_USER_CLIP            EQU     5
LT_USER_CBOX            EQU     6
LT_USER_PALETTE         EQU     7
LT_USER_ED              EQU     8
LT_USER_BWL             EQU     9
LT_USER_OWNERDRAW       EQU     10
LT_USER_SPB             EQU     11
LT_USER_CHECKPOINT      EQU     12
LT_USER_DCE             EQU     13
LT_USER_MWP             EQU     14
LT_USER_PROP            EQU     15
LT_USER_LBIV            EQU     16
LT_USER_MISC            EQU     17
LT_USER_ATOMS           EQU     18
LT_USER_LOCKINPUTSTATE  EQU     19
LT_USER_HOOKLIST        EQU     20
LT_USER_USERSEEUSERDOALLOC EQU  21
LT_USER_HOTKEYLIST      EQU     22
LT_USER_POPUPMENU       EQU     23
LT_USER_ICON            EQU     24
LT_USER_QMSG            EQU     26
LT_USER_VWININFO        EQU     27
LT_USER_SMS             EQU     28
LT_USER_PROCESSINFO     EQU     29
LT_USER_ATTACHINFO      EQU     30
LT_USER_SYSCOLORTEMP    EQU     31
LT_USER_HANDLETABLE     EQU     32
LT_USER_SWITCHWNDINFO   EQU     33
LT_USER_HOOKMSG         EQU     34
LT_USER_MAX             EQU     34

; ----- Local32 heap walking -----

LOCAL32INFO     STRUC
li32_dwSize                     DD      ?
li32_dwMemReserved              DD      ?
li32_dwMemCommitted             DD      ?
li32_dwTotalFree                DD      ?
li32_dwLargestFreeBlock         DD      ?
li32_dwcFreeHandles             DD      ?
LOCAL32INFO     ENDS

LOCAL32ENTRY    STRUC
le32_dwSize          DD      ?
le32_hHandle         DW      ?
le32_dwAddress       DD      ?
le32_dwSizeBlock     DD      ?
le32_wFlags          DW      ?
le32_wType           DW      ?
le32_hHeap           DW      ?
le32_wHeapType       DW      ?
le32_dwNext          DD      ?
le32_dwNextAlt       DD      ?
LOCAL32ENTRY    ENDS

; ----- Stack tracing -----

STACKTRACEENTRY STRUC
st_dwSize       DD      ?
st_hTask        DW      ?
st_wSS          DW      ?
st_wBP          DW      ?
st_wCS          DW      ?
st_wIP          DW      ?
st_hModule      DW      ?
st_wSegment     DW      ?
st_wFlags       DW      ?
STACKTRACEENTRY ENDS

;** STACKTRACEENTRY.wFlags values
FRAME_FAR       EQU     0
FRAME_NEAR      EQU     1

; ----- Module list walking -----

MODULEENTRY STRUC
me_dwSize       DD      ?
me_szModule     DB      MAX_MODULE_NAME + 1 DUP(?)
me_hModule      DW      ?
me_wcUsage      DW      ?
me_szExePath    DB      MAX_PATH16 + 1 + 1 DUP(?)
me_wNext        DW      ?
MODULEENTRY ENDS

; ----- Task list walking -----

TASKENTRY STRUC
te_dwSize               DD      ?
te_hTask                DW      ?
te_hTaskParent          DW      ?
te_hInst                DW      ?
te_hModule              DW      ?
te_wSS                  DW      ?
te_wSP                  DW      ?
te_wStackTop            DW      ?
te_wStackMinimum        DW      ?
te_wStackBottom         DW      ?
te_wcEvents             DW      ?
te_hQueue               DW      ?
te_szModule             DB      MAX_MODULE_NAME + 1 DUP(?)
te_wPSPOffset           DW      ?
te_hNext                DW      ?
TASKENTRY ENDS

;** TaskSwitch() return values
TS_ERROR    EQU         0
TS_NEW      EQU         1
TS_OLD      EQU         2

; ----- Window Class enumeration -----

CLASSENTRY STRUC
ce_dwSize               DD      ?
ce_hInst                DW      ?
ce_szClassName          DB      MAX_CLASSNAME + 1 DUP(?)
ce_wNext                DW      ?
CLASSENTRY ENDS

; ----- Information functions -----

VMEMMANINFO STRUC
vmm_dwSize              DD      ?
vmm_dwLargestFreeBlock  DD      ?
vmm_dwMaxPagesAvailable DD      ?
vmm_dwMaxPagesLockable  DD      ?
vmm_dwTotalLinearSpace  DD      ?
vmm_dwTotalUnlockedPages DD     ?
vmm_dwFreePages         DD      ?
vmm_dwTotalPages        DD      ?
vmm_dwFreeLinearSpace   DD      ?
vmm_dwSwapFilePages     DD      ?
vmm_wPageSize           DW      ?
VMEMMANINFO ENDS

SYSHEAPINFO STRUC
si_dwSize               DD      ?
si_wUserFreePercent     DW      ?
si_wGDIFreePercent      DW      ?
si_hUserSegment         DW      ?
si_hGDISegment          DW      ?
si_hWndSegment          DW      ?
si_hMenuSegment         DW      ?
si_hGDI32Segment        DW      ?
SYSHEAPINFO ENDS

; ----- Notifications -----

;* NFY_UNKNOWN:  An unknown notification has been returned from KERNEL.  Apps
;*      should ignore these.
;*
NFY_UNKNOWN     EQU     0

;** NFY_LOADSEG:  dwData points to a NFYLOADSEG structure
NFY_LOADSEG     EQU     1
NFYLOADSEG STRUC
nls_dwSize              DD      ?
nls_wSelector           DW      ?
nls_wSegNum             DW      ?
nls_wType               DW      ?       ;Low bit set for DATA, clear for CODE
nls_wcInstance          DW      ?       ;Instance count ONLY VALID FOR DATA!
nls_lpstrModuleName     DD      ?
NFYLOADSEG ENDS

;** NFY_FREESEG:  LOWORD(dwData) is the selector of the segment being freed
NFY_FREESEG     EQU     2

;** NFY_STARTDLL:  dwData points to a NFYSTARTDLL structure
NFY_STARTDLL    EQU     3
NFYSTARTDLL STRUC
nsd_dwSize              DD      ?
nsd_hModule             DW      ?
nsd_wCS                 DW      ?
nsd_wIP                 DW      ?
NFYSTARTDLL ENDS

;** NFY_STARTTASK:  dwData is the CS:IP of the task start address
NFY_STARTTASK   EQU     4

;** NFY_EXITTASK:  The low byte of dwData contains the program exit code
NFY_EXITTASK    EQU     5

;** NFY_DELMODULE:  LOWORD(dwData) is the handle of the module to be freed
NFY_DELMODULE   EQU     6

;** NFY_RIP:  LOWORD(dwData) is the fatal exit code
;** NFY_RIP:  dwData points to a NFYRIP structure
NFY_RIP         EQU     7
NFYRIP STRUC
nrp_dwSize      DD      ?
nrp_wIP         DW      ?
nrp_wCS         DW      ?
nrp_wSS         DW      ?
nrp_wBP         DW      ?
nrp_wExitCode   DW      ?
NFYRIP ENDS

;** NFY_TASKIN:  No data.  Callback should do GetCurrentTask()
NFY_TASKIN      EQU     8

;** NFY_TASKOUT:  No data.  Callback should do GetCurrentTask()
NFY_TASKOUT     EQU     9

;** NFY_INCHAR:  Return value of callback is used.  If FALSE, mapped to 'i'
NFY_INCHAR      EQU     10

;** NFY_OUTSTR:  dwData points to the string to be displayed
NFY_OUTSTR      EQU     11

;** NFY_LOGERROR:  dwData points to a NFYLOGERROR struct
NFY_LOGERROR    EQU     12
NFYLOGERROR STRUC
nle_dwSize      DD      ?
nle_wErrCode    DW      ?
nle_lpInfo      DD      ?       ;Error-code dependent
NFYLOGERROR ENDS

;** NFY_LOGPARAMERROR:  dwData points to a NFYLOGPARAMERROR struct
NFY_LOGPARAMERROR EQU   13
NFYLOGPARAMERROR STRUC
nlp_dwSize              DD      ?
nlp_wErrCode            DW      ?
nlp_lpfnErrorAddr       DD      ?
nlp_lpBadParam          DD      ?
NFYLOGPARAMERROR ENDS

;** NotifyRegister() flags
NF_NORMAL       EQU     0
NF_TASKSWITCH   EQU     1
NF_RIP          EQU     2

;** TerminateApp() flag values
UAE_BOX         EQU     0
NO_UAE_BOX      EQU     1

TIMERINFO STRUC
ti_dwSize               DD      ?
ti_dwmsSinceStart       DD      ?
ti_dwmsThisVM           DD      ?
TIMERINFO ENDS
