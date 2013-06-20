
/******************************************************************************
 *
 * Name: acpixf.h - External interfaces to the ACPI subsystem
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2011, Intel Corp.
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


#ifndef __ACXFACE_H__
#define __ACXFACE_H__

/* Current ACPICA subsystem version in YYYYMMDD format */

#define ACPI_CA_VERSION                 0x20110922

#include "actypes.h"
#include "actbl.h"

/*
 * Globals that are publically available
 */
extern UINT32               AcpiCurrentGpeCount;
extern ACPI_TABLE_FADT      AcpiGbl_FADT;
extern BOOLEAN              AcpiGbl_SystemAwakeAndRunning;

/* Runtime configuration of debug print levels */

extern UINT32               AcpiDbgLevel;
extern UINT32               AcpiDbgLayer;

/* ACPICA runtime options */

extern UINT8                AcpiGbl_EnableInterpreterSlack;
extern UINT8                AcpiGbl_AllMethodsSerialized;
extern UINT8                AcpiGbl_CreateOsiMethod;
extern UINT8                AcpiGbl_UseDefaultRegisterWidths;
extern ACPI_NAME            AcpiGbl_TraceMethodName;
extern UINT32               AcpiGbl_TraceFlags;
extern UINT8                AcpiGbl_EnableAmlDebugObject;
extern UINT8                AcpiGbl_CopyDsdtLocally;
extern UINT8                AcpiGbl_TruncateIoAddresses;
extern UINT8                AcpiGbl_DisableAutoRepair;


/*
 * Initialization
 */
ACPI_STATUS
AcpiInitializeTables (
    ACPI_TABLE_DESC         *InitialStorage,
    UINT32                  InitialTableCount,
    BOOLEAN                 AllowResize);

ACPI_STATUS
AcpiInitializeSubsystem (
    void);

ACPI_STATUS
AcpiEnableSubsystem (
    UINT32                  Flags);

ACPI_STATUS
AcpiInitializeObjects (
    UINT32                  Flags);

ACPI_STATUS
AcpiTerminate (
    void);


/*
 * Miscellaneous global interfaces
 */
ACPI_STATUS
AcpiEnable (
    void);

ACPI_STATUS
AcpiDisable (
    void);

ACPI_STATUS
AcpiSubsystemStatus (
    void);

ACPI_STATUS
AcpiGetSystemInfo (
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiGetStatistics (
    ACPI_STATISTICS         *Stats);

const char *
AcpiFormatException (
    ACPI_STATUS             Exception);

ACPI_STATUS
AcpiPurgeCachedObjects (
    void);

ACPI_STATUS
AcpiInstallInterface (
    ACPI_STRING             InterfaceName);

ACPI_STATUS
AcpiRemoveInterface (
    ACPI_STRING             InterfaceName);


/*
 * ACPI Memory management
 */
void *
AcpiAllocate (
    UINT32                  Size);

void *
AcpiCallocate (
    UINT32                  Size);

void
AcpiFree (
    void                    *Address);


/*
 * ACPI table manipulation interfaces
 */
ACPI_STATUS
AcpiReallocateRootTable (
    void);

ACPI_STATUS
AcpiFindRootPointer (
    ACPI_SIZE               *RsdpAddress);

ACPI_STATUS
AcpiLoadTables (
    void);

ACPI_STATUS
AcpiGetTableHeader (
    ACPI_STRING             Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       *OutTableHeader);

ACPI_STATUS
AcpiGetTable (
    ACPI_STRING             Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **OutTable);

ACPI_STATUS
AcpiGetTableByIndex (
    UINT32                  TableIndex,
    ACPI_TABLE_HEADER       **OutTable);

ACPI_STATUS
AcpiInstallTableHandler (
    ACPI_TABLE_HANDLER      Handler,
    void                    *Context);

ACPI_STATUS
AcpiRemoveTableHandler (
    ACPI_TABLE_HANDLER      Handler);


/*
 * Namespace and name interfaces
 */
ACPI_STATUS
AcpiWalkNamespace (
    ACPI_OBJECT_TYPE        Type,
    ACPI_HANDLE             StartObject,
    UINT32                  MaxDepth,
    ACPI_WALK_CALLBACK      PreOrderVisit,
    ACPI_WALK_CALLBACK      PostOrderVisit,
    void                    *Context,
    void                    **ReturnValue);

ACPI_STATUS
AcpiGetDevices (
    char                    *HID,
    ACPI_WALK_CALLBACK      UserFunction,
    void                    *Context,
    void                    **ReturnValue);

ACPI_STATUS
AcpiGetName (
    ACPI_HANDLE             Object,
    UINT32                  NameType,
    ACPI_BUFFER             *RetPathPtr);

ACPI_STATUS
AcpiGetHandle (
    ACPI_HANDLE             Parent,
    ACPI_STRING             Pathname,
    ACPI_HANDLE             *RetHandle);

ACPI_STATUS
AcpiAttachData (
    ACPI_HANDLE             Object,
    ACPI_OBJECT_HANDLER     Handler,
    void                    *Data);

ACPI_STATUS
AcpiDetachData (
    ACPI_HANDLE             Object,
    ACPI_OBJECT_HANDLER     Handler);

ACPI_STATUS
AcpiGetData (
    ACPI_HANDLE             Object,
    ACPI_OBJECT_HANDLER     Handler,
    void                    **Data);

ACPI_STATUS
AcpiDebugTrace (
    char                    *Name,
    UINT32                  DebugLevel,
    UINT32                  DebugLayer,
    UINT32                  Flags);


/*
 * Object manipulation and enumeration
 */
ACPI_STATUS
AcpiEvaluateObject (
    ACPI_HANDLE             Object,
    ACPI_STRING             Pathname,
    ACPI_OBJECT_LIST        *ParameterObjects,
    ACPI_BUFFER             *ReturnObjectBuffer);

ACPI_STATUS
AcpiEvaluateObjectTyped (
    ACPI_HANDLE             Object,
    ACPI_STRING             Pathname,
    ACPI_OBJECT_LIST        *ExternalParams,
    ACPI_BUFFER             *ReturnBuffer,
    ACPI_OBJECT_TYPE        ReturnType);

ACPI_STATUS
AcpiGetObjectInfo (
    ACPI_HANDLE             Object,
    ACPI_DEVICE_INFO        **ReturnBuffer);

ACPI_STATUS
AcpiInstallMethod (
    UINT8                   *Buffer);

ACPI_STATUS
AcpiGetNextObject (
    ACPI_OBJECT_TYPE        Type,
    ACPI_HANDLE             Parent,
    ACPI_HANDLE             Child,
    ACPI_HANDLE             *OutHandle);

ACPI_STATUS
AcpiGetType (
    ACPI_HANDLE             Object,
    ACPI_OBJECT_TYPE        *OutType);

ACPI_STATUS
AcpiGetParent (
    ACPI_HANDLE             Object,
    ACPI_HANDLE             *OutHandle);


/*
 * Handler interfaces
 */
ACPI_STATUS
AcpiInstallInitializationHandler (
    ACPI_INIT_HANDLER       Handler,
    UINT32                  Function);

ACPI_STATUS
AcpiInstallGlobalEventHandler (
    ACPI_GBL_EVENT_HANDLER  Handler,
    void                    *Context);

ACPI_STATUS
AcpiInstallFixedEventHandler (
    UINT32                  AcpiEvent,
    ACPI_EVENT_HANDLER      Handler,
    void                    *Context);

ACPI_STATUS
AcpiRemoveFixedEventHandler (
    UINT32                  AcpiEvent,
    ACPI_EVENT_HANDLER      Handler);

ACPI_STATUS
AcpiInstallGpeHandler (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Type,
    ACPI_GPE_HANDLER        Address,
    void                    *Context);

ACPI_STATUS
AcpiRemoveGpeHandler (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    ACPI_GPE_HANDLER        Address);

ACPI_STATUS
AcpiInstallNotifyHandler (
    ACPI_HANDLE             Device,
    UINT32                  HandlerType,
    ACPI_NOTIFY_HANDLER     Handler,
    void                    *Context);

ACPI_STATUS
AcpiRemoveNotifyHandler (
    ACPI_HANDLE             Device,
    UINT32                  HandlerType,
    ACPI_NOTIFY_HANDLER     Handler);

ACPI_STATUS
AcpiInstallAddressSpaceHandler (
    ACPI_HANDLE             Device,
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_ADR_SPACE_HANDLER  Handler,
    ACPI_ADR_SPACE_SETUP    Setup,
    void                    *Context);

ACPI_STATUS
AcpiRemoveAddressSpaceHandler (
    ACPI_HANDLE             Device,
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_ADR_SPACE_HANDLER  Handler);

ACPI_STATUS
AcpiInstallExceptionHandler (
    ACPI_EXCEPTION_HANDLER  Handler);

ACPI_STATUS
AcpiInstallInterfaceHandler (
    ACPI_INTERFACE_HANDLER  Handler);


/*
 * Global Lock interfaces
 */
ACPI_STATUS
AcpiAcquireGlobalLock (
    UINT16                  Timeout,
    UINT32                  *Handle);

ACPI_STATUS
AcpiReleaseGlobalLock (
    UINT32                  Handle);


/*
 * Fixed Event interfaces
 */
ACPI_STATUS
AcpiEnableEvent (
    UINT32                  Event,
    UINT32                  Flags);

ACPI_STATUS
AcpiDisableEvent (
    UINT32                  Event,
    UINT32                  Flags);

ACPI_STATUS
AcpiClearEvent (
    UINT32                  Event);

ACPI_STATUS
AcpiGetEventStatus (
    UINT32                  Event,
    ACPI_EVENT_STATUS       *EventStatus);


/*
 * General Purpose Event (GPE) Interfaces
 */
ACPI_STATUS
AcpiUpdateAllGpes (
    void);

ACPI_STATUS
AcpiEnableGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber);

ACPI_STATUS
AcpiDisableGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber);

ACPI_STATUS
AcpiClearGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber);

ACPI_STATUS
AcpiSetGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT8                   Action);

ACPI_STATUS
AcpiFinishGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber);

ACPI_STATUS
AcpiSetupGpeForWake (
    ACPI_HANDLE             ParentDevice,
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber);

ACPI_STATUS
AcpiSetGpeWakeMask (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT8                   Action);

ACPI_STATUS
AcpiGetGpeStatus (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    ACPI_EVENT_STATUS       *EventStatus);

ACPI_STATUS
AcpiDisableAllGpes (
    void);

ACPI_STATUS
AcpiEnableAllRuntimeGpes (
    void);

ACPI_STATUS
AcpiGetGpeDevice (
    UINT32                  GpeIndex,
    ACPI_HANDLE             *GpeDevice);

ACPI_STATUS
AcpiInstallGpeBlock (
    ACPI_HANDLE             GpeDevice,
    ACPI_GENERIC_ADDRESS    *GpeBlockAddress,
    UINT32                  RegisterCount,
    UINT32                  InterruptNumber);

ACPI_STATUS
AcpiRemoveGpeBlock (
    ACPI_HANDLE             GpeDevice);


/*
 * Resource interfaces
 */
typedef
ACPI_STATUS (*ACPI_WALK_RESOURCE_CALLBACK) (
    ACPI_RESOURCE           *Resource,
    void                    *Context);

ACPI_STATUS
AcpiGetVendorResource (
    ACPI_HANDLE             Device,
    char                    *Name,
    ACPI_VENDOR_UUID        *Uuid,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiGetCurrentResources (
    ACPI_HANDLE             Device,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiGetPossibleResources (
    ACPI_HANDLE             Device,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiWalkResources (
    ACPI_HANDLE                 Device,
    char                        *Name,
    ACPI_WALK_RESOURCE_CALLBACK UserFunction,
    void                        *Context);

ACPI_STATUS
AcpiSetCurrentResources (
    ACPI_HANDLE             Device,
    ACPI_BUFFER             *InBuffer);

ACPI_STATUS
AcpiGetIrqRoutingTable (
    ACPI_HANDLE             Device,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiResourceToAddress64 (
    ACPI_RESOURCE           *Resource,
    ACPI_RESOURCE_ADDRESS64 *Out);


/*
 * Hardware (ACPI device) interfaces
 */
ACPI_STATUS
AcpiReset (
    void);

ACPI_STATUS
AcpiRead (
    UINT64                  *Value,
    ACPI_GENERIC_ADDRESS    *Reg);

ACPI_STATUS
AcpiWrite (
    UINT64                  Value,
    ACPI_GENERIC_ADDRESS    *Reg);

ACPI_STATUS
AcpiReadBitRegister (
    UINT32                  RegisterId,
    UINT32                  *ReturnValue);

ACPI_STATUS
AcpiWriteBitRegister (
    UINT32                  RegisterId,
    UINT32                  Value);

ACPI_STATUS
AcpiGetSleepTypeData (
    UINT8                   SleepState,
    UINT8                   *Slp_TypA,
    UINT8                   *Slp_TypB);

ACPI_STATUS
AcpiEnterSleepStatePrep (
    UINT8                   SleepState);

ACPI_STATUS
AcpiEnterSleepState (
    UINT8                   SleepState);

ACPI_STATUS
AcpiEnterSleepStateS4bios (
    void);

ACPI_STATUS
AcpiLeaveSleepState (
    UINT8                   SleepState)
    ;
ACPI_STATUS
AcpiSetFirmwareWakingVector (
    UINT32                  PhysicalAddress);

#if ACPI_MACHINE_WIDTH == 64
ACPI_STATUS
AcpiSetFirmwareWakingVector64 (
    UINT64                  PhysicalAddress);
#endif


/*
 * Error/Warning output
 */
void ACPI_INTERNAL_VAR_XFACE
AcpiError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Format,
    ...) ACPI_PRINTF_LIKE(3);

void  ACPI_INTERNAL_VAR_XFACE
AcpiException (
    const char              *ModuleName,
    UINT32                  LineNumber,
    ACPI_STATUS             Status,
    const char              *Format,
    ...) ACPI_PRINTF_LIKE(4);

void ACPI_INTERNAL_VAR_XFACE
AcpiWarning (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Format,
    ...) ACPI_PRINTF_LIKE(3);

void ACPI_INTERNAL_VAR_XFACE
AcpiInfo (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Format,
    ...) ACPI_PRINTF_LIKE(3);


/*
 * Debug output
 */
#ifdef ACPI_DEBUG_OUTPUT

void ACPI_INTERNAL_VAR_XFACE
AcpiDebugPrint (
    UINT32                  RequestedDebugLevel,
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    const char              *Format,
    ...) ACPI_PRINTF_LIKE(6);

void ACPI_INTERNAL_VAR_XFACE
AcpiDebugPrintRaw (
    UINT32                  RequestedDebugLevel,
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    const char              *Format,
    ...) ACPI_PRINTF_LIKE(6);
#endif

#endif /* __ACXFACE_H__ */
