/*++

Module Name:

    ktrace.c

Abstract:

    This module implements a tracing facility for use by all kernel mode
    modules in Windows NT.

Author:

    Roy D'Souza (rdsouza@gomez) 22-May-1996

Environment:

    User or Kernel mode.

Revision History:

--*/

#include "ki.h"
#if DBG && IA64_KTRACE

#include "ktrace.h"
#include "ktracep.h"


/***********************************************************************
Format of KTrace record (144 bytes)
***********************************************************************/
typedef struct _KTRACE_RECORD_ {
    ULONG              ModuleID;
    USHORT         MessageType;
    USHORT         MessageIndex;
    LARGE_INTEGER  SystemTime;
    ULONGLONG      Arg1;
    ULONGLONG      Arg2;
    ULONGLONG      Arg3;
    ULONGLONG      Arg4;
    ULONGLONG      Arg5;
    LARGE_INTEGER   HiResTimeStamp;
} KTRACE_RECORD, *PKTRACE_RECORD;

/* IF YOU MAKE ANY CHANGE TO THE ABOVE TYPEDEF YOU
ALSO NEED TO UPDATE THE CONSTANT RECORD_SIZE_IN_BYTES
IN FILE KTRACEP.H, SO THAT THE SIMDB MACROS WILL CONTINUE
TO WORK CORRECTLY
*/

/***********************************************************************
KTrace Private Data
***********************************************************************/


KTRACE_RECORD KTrace[MAXIMUM_PROCESSORS][KTRACE_LOG_SIZE];

//
// Current queue head
//
static ULONG KTraceQueueHead = 0;

//
// By default the mask is on for all:
//
static ULONG ModuleIDMask = 0xFFFFFFFF;

/***********************************************************************
KeEnableKTrace - selectively enable/disable client's rights to trace
***********************************************************************/
VOID
NTAPI
KeEnableKTrace (
        ULONG IDMask
           )
/*++

Routine Description:

    Selectively enable or disable individual module's abilities to
    write to the KTrace.

    By default all modules are permitted to write traces.

    Any kernel mode modules can call this routine to toggle write
    write permissions.
    
Arguments:

    IDMask

       A bit pattern which specifies the modules that are to be enabled.

Return Value:

    None.

--*/

{
    ModuleIDMask = IDMask;

    return;

} // KeEnableKTrace

/***********************************************************************
CurrentKTraceEntry -- return address of current entry and update trace index
***********************************************************************/
PKTRACE_RECORD CurrentEntry = 0;

PKTRACE_RECORD
NTAPI
KiCurrentKTraceEntry (
    )
/*++

Routine Description:

    Provide pointer to next trace entry for use by assembly code that updates trace
    table. 

Arguments:

    None.

Return Value:

    Pointer to next entry.
    
--*/
{   
   ULONG          CurrentProcessor;

   CurrentProcessor = KeGetCurrentProcessorNumber();
   if (CurrentProcessor > MAXIMUM_PROCESSORS) {
      DbgPrint("KTrace:CurrentKTraceEntry:KeGetCurrentProcessorNumber invalid\n");
      return NULL;
   }

   CurrentEntry = &KTrace[CurrentProcessor][KTraceQueueHead];
   KTraceQueueHead = (KTraceQueueHead + 1) % KTRACE_LOG_SIZE;

   return CurrentEntry;

} // CurrentKTraceEntry

/***********************************************************************
AddTrace - add an entry into the trace.
***********************************************************************/

NTSTATUS
NTAPI
KeAddKTrace (
    ULONG     ModuleID,
    USHORT    MessageType,
    USHORT    MessageIndex,
    ULONGLONG Arg1,
    ULONGLONG Arg2,
    ULONGLONG Arg3,
    ULONGLONG Arg4
    )
/*++

Routine Description:

    Add a record to the KTrace log. Can be called by anybody in the
    kernel. (Need to figure out a way to export this through ntoskrnl.lib
    so that drivers can access it.)

Arguments:

    ModuleID    identifies the client making this call. See ktrace.h
                for definitions.

    MessageType specifies the type/severity of the message (i.e. warning,
                error, checkpoint...) See ktrace.h

    Arg1-4      for unrestricted use by the client.

Return Value:

    Status

               STATUS_SUCCESS if successful.
               STATUS_UNSUCCESSFUL if failure.
--*/
{   
   ULONG           CurrentProcessor;
   LARGE_INTEGER  PerfCtr;
   NTSTATUS       Status  = STATUS_SUCCESS;
   LARGE_INTEGER  SystemTime;

   if (!(ModuleID & ModuleIDMask))
    return STATUS_UNSUCCESSFUL;

   CurrentProcessor = KeGetCurrentProcessorNumber();
   if (CurrentProcessor > MAXIMUM_PROCESSORS) {
      DbgPrint("KTrace:AddTrace:KeGetCurrentProcessorNumber invalid\n");
      return STATUS_UNSUCCESSFUL;
   }

   KTrace[CurrentProcessor][KTraceQueueHead].ModuleID     = ModuleID;
   KTrace[CurrentProcessor][KTraceQueueHead].MessageType  = MessageType;
   KTrace[CurrentProcessor][KTraceQueueHead].MessageIndex = MessageIndex;
   KTrace[CurrentProcessor][KTraceQueueHead].Arg1         = Arg1;
   KTrace[CurrentProcessor][KTraceQueueHead].Arg2         = Arg2;
   KTrace[CurrentProcessor][KTraceQueueHead].Arg3         = Arg3;
   KTrace[CurrentProcessor][KTraceQueueHead].Arg4         = Arg4;

   KeQuerySystemTime(&SystemTime);
   KTrace[CurrentProcessor][KTraceQueueHead].SystemTime = SystemTime;
#if 0
   Status = NtQueryPerformanceCounter(&PerfCtr, NULL);
   if (!NT_SUCCESS(Status)) {
      DbgPrint("NtQueryPerformanceCounter failed with %x\n", Status);
      return Status;
   }

   KTrace[CurrentProcessor][KTraceQueueHead].HiResTimeStamp = PerfCtr;
#endif
   KTraceQueueHead = (KTraceQueueHead + 1) % KTRACE_LOG_SIZE;

   return Status;

} // AddTrace

/***********************************************************************
QueryDumpKTraceBuffer - API query: selectively dump trace
***********************************************************************/
LONG
NTAPI
KeQueryDumpKTrace (
    ULONG       Processor,
    ULONG       StartEntry,
    ULONG       NumberOfEntries,
    ULONG       ModuleFilter,
    ULONG       MessageFilter,
    BOOLEAN     Sort)

/*++

Routine Description:

Arguments:
    
    ProcessorMask

      Specify the particular processor of interest.

    StartEntry

      The offset from the start of the queue to start dumping records.
      If this entry is 0 then the dumping starts from the head of the
      queue.
      If this argument is specified to be larger than the size of the
      KTrace buffer, then the dump wraps around appropriately.

    NumberOfEntries

      The number of log entries to dump starting from StartEntry.

    ModuleFilter

      A bit pattern specifying the modules of interest. See ktrace.h
      for a definition of modules. Only the logs of these modules are
      included in the dump. Bits representing undefined modules are
      ignored.

    MessageFilter

      A bit pattern specifying the subset of message types to dump. See
      ktrace.h for definitions of all message types.

    Sort

      Sort the output before dumping. Base the sort on the global NT
      timestamp. Most recent entries are dumped first. (NOT IMPLEMENTED
      YET).

Return Value:

    The number of records that were dumped.
    Returns -1 if error.
    
--*/

{
   ULONG Index = StartEntry;
   ULONG RecordCount = 0;

   //
   // Verify that we have a valid processor number:
   //
#if !defined(NT_UP)
   if (Processor > KeRegisteredProcessors) {
      DbgPrint("KTrace error: attempt to access invalid processor"
               "%d on a system with %d processors\n",
               Processor,
               KeRegisteredProcessors);
      return 0L;
   }
#else
   if (Processor > 0) {
      DbgPrint("KTrace error: attempted to access invalid processor"
               "%d on a uni-processor system\n",
               Processor);
   }
#endif

   //
   // Loop through the entire KTrace
   //
   while (NumberOfEntries > 0) {

      //
      // See if the current record matches the query criteria
      //
      if ((ModuleFilter  & KTrace[Processor][Index].ModuleID) &&
          (MessageFilter & KTrace[Processor][Index].MessageType)) {

         DumpRecord(Processor, Index);
         RecordCount++;
      }

      NumberOfEntries = NumberOfEntries - 1;
      Index = Index > 0 ? Index - 1 : KTRACE_LOG_SIZE - 1;
   }
   return RecordCount;

} // QueryDumpKTraceBuffer

/***********************************************************************
KePurgeKTrace - delete all records from the trace.
***********************************************************************/

VOID
NTAPI
KePurgeKTrace (
            )
{

   ULONG Index1, Index2;

   for (Index1 = 0; Index1 < KTRACE_LOG_SIZE; Index1++) {
      for (Index2 = 0; Index2 < MAXIMUM_PROCESSORS; Index2++) {
         KTrace[Index2][Index1].ModuleID       = 0;
     KTrace[Index2][Index1].MessageType    = 0;
     KTrace[Index2][Index1].MessageIndex   = 0;
         KTrace[Index2][Index1].SystemTime.HighPart = 0;
         KTrace[Index2][Index1].SystemTime.LowPart  = 0;
     KTrace[Index2][Index1].Arg1           = 0;
     KTrace[Index2][Index1].Arg2           = 0;
     KTrace[Index2][Index1].Arg3           = 0;
     KTrace[Index2][Index1].Arg4           = 0;
     KTrace[Index2][Index1].HiResTimeStamp.HighPart = 0;
     KTrace[Index2][Index1].HiResTimeStamp.LowPart = 0;
      }
   }
}

/***********************************************************************
DumpRecord - dump a single record specified by processor number & index.
***********************************************************************/
VOID
NTAPI
DumpRecord (IN ULONG ProcessorNumber,
            IN ULONG Index)
/*++

Routine Description:

   Dumps out the specified record in the trace associated with the
   specified processor out to the remote debugger console.

Arguments:

   ProcessorNumber

      The processor whose associated trace log is to be accessed.

   Index

      The offset of the record in the trace to be dumped.

Return Value:

   None.

--*/
{

#if !defined(NT_UP)
   if (ProcessorNumber > KeRegisteredProcessors) {
      DbgPrint("KTrace:DumpRecord:"
               "illegal processor number %x in a %x-processor system\n",
               ProcessorNumber, KeRegisteredProcessors);
      return;
   }
#else
   if (ProcessorNumber > 0) {
      DbgPrint("KTrace:DumpRecord:"
               "illegal processor %x in a uni-processor system\n",
               ProcessorNumber);
   }
#endif

   DbgPrint("Dumping Record Index [%ld], Processor = [%ld]\n",
        Index);
   DbgPrint("\tModuleID = [%lx]\n",
        KTrace[ProcessorNumber][Index].ModuleID);
   DbgPrint("\tMessageType = [%lx]\n",
        KTrace[ProcessorNumber][Index].MessageType);
   DbgPrint("\tMessageIndex = [%lx]\n",
        KTrace[ProcessorNumber][Index].MessageIndex);
   DbgPrint("\tArg1= [%lx%LX]\n",
        KTrace[ProcessorNumber][Index].Arg1,
        KTrace[ProcessorNumber][Index].Arg1);
   DbgPrint("\tArg2= [%lx%LX]\n",
        KTrace[ProcessorNumber][Index].Arg2,
        KTrace[ProcessorNumber][Index].Arg2);
   DbgPrint("\tArg3= [%lx%LX]\n",
        KTrace[ProcessorNumber][Index].Arg3,
        KTrace[ProcessorNumber][Index].Arg3);
   DbgPrint("\tArg4= [%lx%LX]\n",
        KTrace[ProcessorNumber][Index].Arg4,
        KTrace[ProcessorNumber][Index].Arg4);
} // DumpRecord


#endif // DBG

// end ktrace.c

