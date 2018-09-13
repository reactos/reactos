/** SVC Defines
 *
 *  Revision history:
 *
 *  bobday 13-Jan-1992 Created
 */


/* DBGSVC - DBG SVC calls.
 *
 *   This macro is used by Nlddebug.asm (where N=nothing or 2)
 *
 */

/* ASM
include bop.inc

dbgsvc  macro   func
    BOP BOP_DEBUGGER
    db  func
    endm
*/

//
// Debug event codes
// These need to match the public definition in sdk\inc\vdmdbg.h
//

#define DBG_SEGLOAD     0
#define DBG_SEGMOVE     1
#define DBG_SEGFREE     2
#define DBG_MODLOAD     3
#define DBG_MODFREE     4
#define DBG_SINGLESTEP  5
#define DBG_BREAK       6
#define DBG_GPFAULT     7
#define DBG_DIVOVERFLOW 8
#define DBG_INSTRFAULT  9
#define DBG_TASKSTART   10
#define DBG_TASKSTOP    11
#define DBG_DLLSTART    12
#define DBG_DLLSTOP     13
#define DBG_ATTACH      14
#define DBG_TOOLHELP    15
#define DBG_STACKFAULT  16
#define DBG_WOWINIT     17
#define DBG_TEMPBP      18
#define DBG_MODMOVE     19
#define DBG_INIT        20
#define DBG_GPFAULT2    21

//
// Flags used by DemIsDebug
//
#define ISDBG_DEBUGGEE 1
#define ISDBG_SHOWSVC  2


void
DbgSegmentNotice(
    WORD wType,
    WORD  wModuleSeg,
    WORD  wLoadSeg,
    WORD  wNewSeg,
    LPSTR lpModuleName,
    LPSTR lpModulePath,
    DWORD dwImageLen
    );

BOOL
DbgIsDebuggee(
    void
    );

BOOL
DbgInit(
    void
    );

VOID DbgDosAppStart(WORD, WORD);
BOOL DbgBPInt(VOID);
BOOL DbgTraceInt(VOID);
BOOL DbgFault(ULONG);
VOID VdmPrompt(ULONG, ULONG);
VOID VdmTraceEvent(USHORT, USHORT, ULONG);

/* XLATOFF */
#if DBG

#define DBGTRACE(Type, wData, lData) VdmTraceEvent(Type, wData, lData)

#else

#define DBGTRACE(Type, wData, lData) {}

#endif

/* XLATON */

//
// Events for DbgPrompt()
//
#define DBG_EVENT_HW_INT    1
#define DBG_EVENT_SW_INT    2

//
// Definitions for VdmTraceEvent major code types
//

#define VDMTR_TYPE_KERNEL   0
#define VDMTR_TYPE_DPMI     0x100
#define VDMTR_TYPE_DPMI_SF  0x200
#define VDMTR_TYPE_DPMI_SI  0x300
#define VDMTR_TYPE_DEM      0x400
#define VDMTR_TYPE_WOW      0x500
#define VDMTR_TYPE_VSBD     0x600
#define VDMTR_TYPE_DBG      0x700
#define VDMTR_TYPE_MONITOR  0x800


//
// Definitions for VdmTraceEvent minor code types
//

#define DPMI_SWITCH_STACKS          1
#define DPMI_GENERIC                2
#define DPMI_DISPATCH_INT           3
#define DPMI_HW_INT                 4
#define DPMI_SW_INT                 5
#define DPMI_INT_IRET16             6
#define DPMI_INT_IRET32             7
#define DPMI_FAULT                  8
#define DPMI_DISPATCH_FAULT         9
#define DPMI_FAULT_IRET             10
#define DPMI_OP_EMULATION           11
#define DPMI_DISPATCH_ENTRY         12
#define DPMI_IN_PM                  13
#define DPMI_IN_V86                 14
#define DPMI_DISPATCH_EXIT          15
#define DPMI_REFLECT_TO_V86         16
#define DPMI_REFLECT_TO_PM          17

#define DEM_EVENT_DISPATCH          1
#define DEM_EVENT_DISPATCH_EXIT     2

#define MONITOR_EVENT_IO            1
#define MONITOR_EVENT_STRING_IO     2
#define MONITOR_EVENT_MEM_ACCESS    3
#define MONITOR_EVENT_INT_ACK       4
#define MONITOR_EVENT_BOP           5
#define MONITOR_EVENT_ERROR         6
#define MONITOR_EVENT_IRQ_13        7
#define MONITOR_CPU_SIMULATE        8
#define MONITOR_CPU_UNSIMULATE      9
