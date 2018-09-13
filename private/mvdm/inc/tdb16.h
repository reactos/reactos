/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  TDB.H
 *  16-bit Kernel Task Data Block
 *
 *  History:
 *  Created 11-Feb-1992 by Matt Felton (mattfe) - from 16 bit tdb.inc
 *       7-apr-1992 mattfe updated to be win 3.1 compatible
 *
--*/

/*
 * NewExeHdr struct offsets. WOW32 uses these for getting expected winversion
 * directly from the exehdr.
 *
 */

#define NE_LOWINVER_OFFSET 0x3e
#define NE_HIWINVER_OFFSET 0x0c
#define FLAG_NE_PROPFONT   0x2000

/*
 * Task Data Block - 16 Bit Kernel Data Structure
 *
 *   Contains all 16 bit task specific data.
 *
 */

#define numTaskInts 7
#define THUNKELEM   8   // (62*8) = 512-16 (low arena overhead)
#define THUNKSIZE   8

/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct TDB  {       /* tdb16 */

     WORD TDB_next    ;     // next task in dispatch queue
     WORD TDB_taskSP      ;     // Saved SS:SP for this task
     WORD TDB_taskSS      ;     //
     WORD TDB_nEvents     ;     // Task event counter
     BYTE TDB_priority    ;     // Task priority (0 is highest)
     BYTE TDB_thread_ordinal  ;     // ordinal number of this thread
     WORD TDB_thread_next   ;       // next thread
     WORD TDB_thread_tdb      ; // the real TDB for this task
     WORD TDB_thread_list   ;       // list of allocated thread structures
     WORD TDB_thread_free   ;       // free list of availble thread structures
     WORD TDB_thread_count  ;       // total count of tread structures
     WORD TDB_FCW         ; // Floating point control word
     BYTE TDB_flags   ;     // Task flags
     BYTE TDB_filler      ;     // keep word aligned
     WORD TDB_ErrMode     ;     // Error mode for this task
     WORD TDB_ExpWinVer   ;     // Expected Windows version for this task
     WORD TDB_Module      ;     // Task module handle to free in killtask
     WORD TDB_pModule     ;     // Pointer to the module database.
     WORD TDB_Queue   ;     // Task Event Queue pointer
     WORD TDB_Parent      ;     // TDB of the task that started this up
     WORD TDB_SigAction   ;     // Action for app task signal
     DWORD TDB_ASignalProc   ;      // App's Task Signal procedure address
     DWORD TDB_USignalProc   ;      // User's Task Signal procedure address
     DWORD TDB_GNotifyProc    ; // Task global discard notify proc.
     DWORD TDB_INTVECS[numTaskInts] ;   // Task specfic harare interrupts
     WORD TDB_CompatFlags ;     // Compatibility flags
     WORD TDB_CompatFlags2 ;        // Upper 16 bits
     WORD TDB_CompatHandle ;    // for dBase bug
     WORD TDB_WOWCompatFlagsEx ;     // More WOW Compatibility flags
     WORD TDB_WOWCompatFlagsEx2 ;        // Upper 16 bits  
     BYTE TDB_Free[3] ;         // Filler to keep TDB size unchanged
     BYTE TDB_cLibrary    ;     // tracks  add/del of ALL libs in system EMS
     DWORD TDB_PHT        ; // (HANDLE:OFFSET) to private handle table
     WORD TDB_PDB         ; // MSDOS Process Data Block (PDB)
     DWORD TDB_DTA        ; // MSDOS Disk Transfer Address
     BYTE TDB_Drive  ;      // MSDOS current drive
     BYTE TDB_Directory[65] ;       // *** not used starting with win95
     WORD TDB_Validity    ;     // initial AX to be passed to a task
     WORD TDB_Yield_to    ;     // DirectedYield arg stored here
     WORD TDB_LibInitSeg      ; // segment address of libraries to init
     WORD TDB_LibInitOff      ; // MakeProcInstance thunks live here.
     WORD TDB_MPI_Sel     ;     // Code selector for thunks
     WORD TDB_MPI_Thunks[((THUNKELEM*THUNKSIZE)/2)]; //
     BYTE TDB_ModName[8] ;      // Name of Module.
     WORD TDB_sig         ; // Signature word to detect bogus code
     DWORD TDB_ThreadID   ;     // 32-Bit Thread ID for this Task (use TDB_Filler Above)
     DWORD TDB_hThread	  ;	// 32-bit Thread Handle for this task
     WORD  TDB_WOWCompatFlags;  // WOW Compatibility flags
     WORD  TDB_WOWCompatFlags2; // WOW Compatibility flags
#ifdef FE_SB
     WORD  TDB_WOWCompatFlagsJPN;  // WOW Compatibility flags for JAPAN
     WORD  TDB_WOWCompatFlagsJPN2; // WOW Compatibility flags for JAPAN
#endif // FE_SB
     DWORD TDB_vpfnAbortProc;   // printer AbortProc
     BYTE TDB_LFNDirectory[260]; // Long directory name

} TDB;
typedef TDB UNALIGNED *PTDB;

// This bit is defined for the TDB_Drive field
#define TDB_DIR_VALID 0x80
#define TDB_SIGNATURE 0x4454


// NOTE TDB_ThreadID MUST be DWORD aligned or else it will fail on MIPS

/* XLATOFF */
#pragma pack()
/* XLATON */
