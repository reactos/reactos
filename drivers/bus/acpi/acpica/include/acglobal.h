/******************************************************************************
 *
 * Name: acglobal.h - Declarations for global variables
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef __ACGLOBAL_H__
#define __ACGLOBAL_H__


/*
 * Ensure that the globals are actually defined and initialized only once.
 *
 * The use of these macros allows a single list of globals (here) in order
 * to simplify maintenance of the code.
 */
#ifdef DEFINE_ACPI_GLOBALS
#define ACPI_EXTERN
#define ACPI_INIT_GLOBAL(a,b) a=b
#else
#define ACPI_EXTERN extern
#define ACPI_INIT_GLOBAL(a,b) a
#endif


#ifdef DEFINE_ACPI_GLOBALS

/* Public globals, available from outside ACPICA subsystem */

/*****************************************************************************
 *
 * Runtime configuration (static defaults that can be overriden at runtime)
 *
 ****************************************************************************/

/*
 * Enable "slack" in the AML interpreter?  Default is FALSE, and the
 * interpreter strictly follows the ACPI specification.  Setting to TRUE
 * allows the interpreter to ignore certain errors and/or bad AML constructs.
 *
 * Currently, these features are enabled by this flag:
 *
 * 1) Allow "implicit return" of last value in a control method
 * 2) Allow access beyond the end of an operation region
 * 3) Allow access to uninitialized locals/args (auto-init to integer 0)
 * 4) Allow ANY object type to be a source operand for the Store() operator
 * 5) Allow unresolved references (invalid target name) in package objects
 * 6) Enable warning messages for behavior that is not ACPI spec compliant
 */
UINT8       ACPI_INIT_GLOBAL (AcpiGbl_EnableInterpreterSlack, FALSE);

/*
 * Automatically serialize ALL control methods? Default is FALSE, meaning
 * to use the Serialized/NotSerialized method flags on a per method basis.
 * Only change this if the ASL code is poorly written and cannot handle
 * reentrancy even though methods are marked "NotSerialized".
 */
UINT8       ACPI_INIT_GLOBAL (AcpiGbl_AllMethodsSerialized, FALSE);

/*
 * Create the predefined _OSI method in the namespace? Default is TRUE
 * because ACPI CA is fully compatible with other ACPI implementations.
 * Changing this will revert ACPI CA (and machine ASL) to pre-OSI behavior.
 */
UINT8       ACPI_INIT_GLOBAL (AcpiGbl_CreateOsiMethod, TRUE);

/*
 * Disable wakeup GPEs during runtime? Default is TRUE because WAKE and
 * RUNTIME GPEs should never be shared, and WAKE GPEs should typically only
 * be enabled just before going to sleep.
 */
UINT8       ACPI_INIT_GLOBAL (AcpiGbl_LeaveWakeGpesDisabled, TRUE);

/*
 * Optionally use default values for the ACPI register widths. Set this to
 * TRUE to use the defaults, if an FADT contains incorrect widths/lengths.
 */
UINT8       ACPI_INIT_GLOBAL (AcpiGbl_UseDefaultRegisterWidths, TRUE);


/* AcpiGbl_FADT is a local copy of the FADT, converted to a common format. */

ACPI_TABLE_FADT             AcpiGbl_FADT;
UINT32                      AcpiCurrentGpeCount;
UINT32                      AcpiGbl_TraceFlags;
ACPI_NAME                   AcpiGbl_TraceMethodName;

#endif

/*****************************************************************************
 *
 * ACPI Table globals
 *
 ****************************************************************************/

/*
 * AcpiGbl_RootTableList is the master list of ACPI tables found in the
 * RSDT/XSDT.
 *
 */
ACPI_EXTERN ACPI_INTERNAL_RSDT          AcpiGbl_RootTableList;
ACPI_EXTERN ACPI_TABLE_FACS            *AcpiGbl_FACS;

/* These addresses are calculated from the FADT Event Block addresses */

ACPI_EXTERN ACPI_GENERIC_ADDRESS        AcpiGbl_XPm1aStatus;
ACPI_EXTERN ACPI_GENERIC_ADDRESS        AcpiGbl_XPm1aEnable;

ACPI_EXTERN ACPI_GENERIC_ADDRESS        AcpiGbl_XPm1bStatus;
ACPI_EXTERN ACPI_GENERIC_ADDRESS        AcpiGbl_XPm1bEnable;

/*
 * Handle both ACPI 1.0 and ACPI 2.0 Integer widths. The integer width is
 * determined by the revision of the DSDT: If the DSDT revision is less than
 * 2, use only the lower 32 bits of the internal 64-bit Integer.
 */
ACPI_EXTERN UINT8                       AcpiGbl_IntegerBitWidth;
ACPI_EXTERN UINT8                       AcpiGbl_IntegerByteWidth;
ACPI_EXTERN UINT8                       AcpiGbl_IntegerNybbleWidth;


/*****************************************************************************
 *
 * Mutual exlusion within ACPICA subsystem
 *
 ****************************************************************************/

/*
 * Predefined mutex objects. This array contains the
 * actual OS mutex handles, indexed by the local ACPI_MUTEX_HANDLEs.
 * (The table maps local handles to the real OS handles)
 */
ACPI_EXTERN ACPI_MUTEX_INFO             AcpiGbl_MutexInfo[ACPI_NUM_MUTEX];

/*
 * Global lock mutex is an actual AML mutex object
 * Global lock semaphore works in conjunction with the HW global lock
 */
ACPI_EXTERN ACPI_OPERAND_OBJECT        *AcpiGbl_GlobalLockMutex;
ACPI_EXTERN ACPI_SEMAPHORE              AcpiGbl_GlobalLockSemaphore;
ACPI_EXTERN UINT16                      AcpiGbl_GlobalLockHandle;
ACPI_EXTERN BOOLEAN                     AcpiGbl_GlobalLockAcquired;
ACPI_EXTERN BOOLEAN                     AcpiGbl_GlobalLockPresent;

/*
 * Spinlocks are used for interfaces that can be possibly called at
 * interrupt level
 */
ACPI_EXTERN ACPI_SPINLOCK               AcpiGbl_GpeLock;      /* For GPE data structs and registers */
ACPI_EXTERN ACPI_SPINLOCK               AcpiGbl_HardwareLock; /* For ACPI H/W except GPE registers */

/* Reader/Writer lock is used for namespace walk and dynamic table unload */

ACPI_EXTERN ACPI_RW_LOCK                AcpiGbl_NamespaceRwLock;


/*****************************************************************************
 *
 * Miscellaneous globals
 *
 ****************************************************************************/

/* Object caches */

ACPI_EXTERN ACPI_CACHE_T               *AcpiGbl_NamespaceCache;
ACPI_EXTERN ACPI_CACHE_T               *AcpiGbl_StateCache;
ACPI_EXTERN ACPI_CACHE_T               *AcpiGbl_PsNodeCache;
ACPI_EXTERN ACPI_CACHE_T               *AcpiGbl_PsNodeExtCache;
ACPI_EXTERN ACPI_CACHE_T               *AcpiGbl_OperandCache;

/* Global handlers */

ACPI_EXTERN ACPI_OBJECT_NOTIFY_HANDLER  AcpiGbl_DeviceNotify;
ACPI_EXTERN ACPI_OBJECT_NOTIFY_HANDLER  AcpiGbl_SystemNotify;
ACPI_EXTERN ACPI_EXCEPTION_HANDLER      AcpiGbl_ExceptionHandler;
ACPI_EXTERN ACPI_INIT_HANDLER           AcpiGbl_InitHandler;
ACPI_EXTERN ACPI_TABLE_HANDLER          AcpiGbl_TableHandler;
ACPI_EXTERN void                       *AcpiGbl_TableHandlerContext;
ACPI_EXTERN ACPI_WALK_STATE            *AcpiGbl_BreakpointWalk;


/* Owner ID support */

ACPI_EXTERN UINT32                      AcpiGbl_OwnerIdMask[ACPI_NUM_OWNERID_MASKS];
ACPI_EXTERN UINT8                       AcpiGbl_LastOwnerIdIndex;
ACPI_EXTERN UINT8                       AcpiGbl_NextOwnerIdOffset;

/* Misc */

ACPI_EXTERN UINT32                      AcpiGbl_OriginalMode;
ACPI_EXTERN UINT32                      AcpiGbl_RsdpOriginalLocation;
ACPI_EXTERN UINT32                      AcpiGbl_NsLookupCount;
ACPI_EXTERN UINT32                      AcpiGbl_PsFindCount;
ACPI_EXTERN UINT16                      AcpiGbl_Pm1EnableRegisterSave;
ACPI_EXTERN UINT8                       AcpiGbl_DebuggerConfiguration;
ACPI_EXTERN BOOLEAN                     AcpiGbl_StepToNextCall;
ACPI_EXTERN BOOLEAN                     AcpiGbl_AcpiHardwarePresent;
ACPI_EXTERN BOOLEAN                     AcpiGbl_EventsInitialized;
ACPI_EXTERN BOOLEAN                     AcpiGbl_SystemAwakeAndRunning;
ACPI_EXTERN UINT8                       AcpiGbl_OsiData;


#ifndef DEFINE_ACPI_GLOBALS

/* Exception codes */

extern char const                       *AcpiGbl_ExceptionNames_Env[];
extern char const                       *AcpiGbl_ExceptionNames_Pgm[];
extern char const                       *AcpiGbl_ExceptionNames_Tbl[];
extern char const                       *AcpiGbl_ExceptionNames_Aml[];
extern char const                       *AcpiGbl_ExceptionNames_Ctrl[];

/* Other miscellaneous */

extern BOOLEAN                          AcpiGbl_Shutdown;
extern UINT32                           AcpiGbl_StartupFlags;
extern const char                      *AcpiGbl_SleepStateNames[ACPI_S_STATE_COUNT];
extern const char                      *AcpiGbl_LowestDstateNames[ACPI_NUM_SxW_METHODS];
extern const char                      *AcpiGbl_HighestDstateNames[ACPI_NUM_SxD_METHODS];
extern const ACPI_OPCODE_INFO           AcpiGbl_AmlOpInfo[AML_NUM_OPCODES];
extern const char                      *AcpiGbl_RegionTypes[ACPI_NUM_PREDEFINED_REGIONS];
#endif


#ifdef ACPI_DBG_TRACK_ALLOCATIONS

/* Lists for tracking memory allocations */

ACPI_EXTERN ACPI_MEMORY_LIST           *AcpiGbl_GlobalList;
ACPI_EXTERN ACPI_MEMORY_LIST           *AcpiGbl_NsNodeList;
ACPI_EXTERN BOOLEAN                     AcpiGbl_DisplayFinalMemStats;
#endif


/*****************************************************************************
 *
 * Namespace globals
 *
 ****************************************************************************/

#if !defined (ACPI_NO_METHOD_EXECUTION) || defined (ACPI_CONSTANT_EVAL_ONLY)
#define NUM_PREDEFINED_NAMES            10
#else
#define NUM_PREDEFINED_NAMES            9
#endif

ACPI_EXTERN ACPI_NAMESPACE_NODE         AcpiGbl_RootNodeStruct;
ACPI_EXTERN ACPI_NAMESPACE_NODE        *AcpiGbl_RootNode;
ACPI_EXTERN ACPI_NAMESPACE_NODE        *AcpiGbl_FadtGpeDevice;
ACPI_EXTERN ACPI_OPERAND_OBJECT        *AcpiGbl_ModuleCodeList;


extern const UINT8                      AcpiGbl_NsProperties [ACPI_NUM_NS_TYPES];
extern const ACPI_PREDEFINED_NAMES      AcpiGbl_PreDefinedNames [NUM_PREDEFINED_NAMES];

#ifdef ACPI_DEBUG_OUTPUT
ACPI_EXTERN UINT32                      AcpiGbl_CurrentNodeCount;
ACPI_EXTERN UINT32                      AcpiGbl_CurrentNodeSize;
ACPI_EXTERN UINT32                      AcpiGbl_MaxConcurrentNodeCount;
ACPI_EXTERN ACPI_SIZE                  *AcpiGbl_EntryStackPointer;
ACPI_EXTERN ACPI_SIZE                  *AcpiGbl_LowestStackPointer;
ACPI_EXTERN UINT32                      AcpiGbl_DeepestNesting;
#endif


/*****************************************************************************
 *
 * Interpreter globals
 *
 ****************************************************************************/


ACPI_EXTERN ACPI_THREAD_STATE          *AcpiGbl_CurrentWalkList;

/* Control method single step flag */

ACPI_EXTERN UINT8                       AcpiGbl_CmSingleStep;


/*****************************************************************************
 *
 * Hardware globals
 *
 ****************************************************************************/

extern      ACPI_BIT_REGISTER_INFO      AcpiGbl_BitRegisterInfo[ACPI_NUM_BITREG];
ACPI_EXTERN UINT8                       AcpiGbl_SleepTypeA;
ACPI_EXTERN UINT8                       AcpiGbl_SleepTypeB;


/*****************************************************************************
 *
 * Event and GPE globals
 *
 ****************************************************************************/

extern      ACPI_FIXED_EVENT_INFO       AcpiGbl_FixedEventInfo[ACPI_NUM_FIXED_EVENTS];
ACPI_EXTERN ACPI_FIXED_EVENT_HANDLER    AcpiGbl_FixedEventHandlers[ACPI_NUM_FIXED_EVENTS];
ACPI_EXTERN ACPI_GPE_XRUPT_INFO        *AcpiGbl_GpeXruptListHead;
ACPI_EXTERN ACPI_GPE_BLOCK_INFO        *AcpiGbl_GpeFadtBlocks[ACPI_MAX_GPE_BLOCKS];


/*****************************************************************************
 *
 * Debug support
 *
 ****************************************************************************/

/* Procedure nesting level for debug output */

extern      UINT32                      AcpiGbl_NestingLevel;

/* Event counters */

ACPI_EXTERN UINT32                      AcpiMethodCount;
ACPI_EXTERN UINT32                      AcpiGpeCount;
ACPI_EXTERN UINT32                      AcpiSciCount;
ACPI_EXTERN UINT32                      AcpiFixedEventCount[ACPI_NUM_FIXED_EVENTS];

/* Support for dynamic control method tracing mechanism */

ACPI_EXTERN UINT32                      AcpiGbl_OriginalDbgLevel;
ACPI_EXTERN UINT32                      AcpiGbl_OriginalDbgLayer;
ACPI_EXTERN UINT32                      AcpiGbl_TraceDbgLevel;
ACPI_EXTERN UINT32                      AcpiGbl_TraceDbgLayer;


/*****************************************************************************
 *
 * Debugger globals
 *
 ****************************************************************************/

ACPI_EXTERN UINT8                       AcpiGbl_DbOutputFlags;

#ifdef ACPI_DISASSEMBLER

ACPI_EXTERN BOOLEAN                     AcpiGbl_DbOpt_disasm;
ACPI_EXTERN BOOLEAN                     AcpiGbl_DbOpt_verbose;
ACPI_EXTERN ACPI_EXTERNAL_LIST         *AcpiGbl_ExternalList;
#endif


#ifdef ACPI_DEBUGGER

extern      BOOLEAN                     AcpiGbl_MethodExecuting;
extern      BOOLEAN                     AcpiGbl_AbortMethod;
extern      BOOLEAN                     AcpiGbl_DbTerminateThreads;

ACPI_EXTERN BOOLEAN                     AcpiGbl_DbOpt_tables;
ACPI_EXTERN BOOLEAN                     AcpiGbl_DbOpt_stats;
ACPI_EXTERN BOOLEAN                     AcpiGbl_DbOpt_ini_methods;
ACPI_EXTERN BOOLEAN                     AcpiGbl_DbOpt_NoRegionSupport;

ACPI_EXTERN char                       *AcpiGbl_DbArgs[ACPI_DEBUGGER_MAX_ARGS];
ACPI_EXTERN char                        AcpiGbl_DbLineBuf[80];
ACPI_EXTERN char                        AcpiGbl_DbParsedBuf[80];
ACPI_EXTERN char                        AcpiGbl_DbScopeBuf[40];
ACPI_EXTERN char                        AcpiGbl_DbDebugFilename[40];
ACPI_EXTERN BOOLEAN                     AcpiGbl_DbOutputToFile;
ACPI_EXTERN char                       *AcpiGbl_DbBuffer;
ACPI_EXTERN char                       *AcpiGbl_DbFilename;
ACPI_EXTERN UINT32                      AcpiGbl_DbDebugLevel;
ACPI_EXTERN UINT32                      AcpiGbl_DbConsoleDebugLevel;
ACPI_EXTERN ACPI_NAMESPACE_NODE        *AcpiGbl_DbScopeNode;

/*
 * Statistic globals
 */
ACPI_EXTERN UINT16                      AcpiGbl_ObjTypeCount[ACPI_TYPE_NS_NODE_MAX+1];
ACPI_EXTERN UINT16                      AcpiGbl_NodeTypeCount[ACPI_TYPE_NS_NODE_MAX+1];
ACPI_EXTERN UINT16                      AcpiGbl_ObjTypeCountMisc;
ACPI_EXTERN UINT16                      AcpiGbl_NodeTypeCountMisc;
ACPI_EXTERN UINT32                      AcpiGbl_NumNodes;
ACPI_EXTERN UINT32                      AcpiGbl_NumObjects;


ACPI_EXTERN UINT32                      AcpiGbl_SizeOfParseTree;
ACPI_EXTERN UINT32                      AcpiGbl_SizeOfMethodTrees;
ACPI_EXTERN UINT32                      AcpiGbl_SizeOfNodeEntries;
ACPI_EXTERN UINT32                      AcpiGbl_SizeOfAcpiObjects;

#endif /* ACPI_DEBUGGER */

#endif /* __ACGLOBAL_H__ */
