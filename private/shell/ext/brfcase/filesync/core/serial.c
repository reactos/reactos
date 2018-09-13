/*
 * serial.c - Access serialization routines module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "init.h"


/* Types
 ********/

/* process information */

typedef struct _processinfo
{
   HANDLE hModule;
}
PROCESSINFO;
DECLARE_STANDARD_TYPES(PROCESSINFO);

#ifdef DEBUG

/* debug flags */

typedef enum _serialdebugflags
{
   SERIAL_DFL_BREAK_ON_PROCESS_ATTACH  = 0x0001,

   SERIAL_DFL_BREAK_ON_THREAD_ATTACH   = 0x0002,

   ALL_SERIAL_DFLAGS                   = (SERIAL_DFL_BREAK_ON_PROCESS_ATTACH |
                                          SERIAL_DFL_BREAK_ON_THREAD_ATTACH)
}
SERIALDEBUGFLAGS;

#endif   /* DEBUG */


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_SHARED)

/*
 * RAIDRAID: (16273) The use of Mnrcs in a shared data section is broken under
 * NT.  To run under NT, this code should be changed to use a shared mutex
 * referenced by hMutex in Mpi.
 */

/* critical section used for access serialization */

PRIVATE_DATA NONREENTRANTCRITICALSECTION Mnrcs =
{
   { 0 },

#ifdef DEBUG
   INVALID_THREAD_ID,
#endif   /* DEBUG */

   FALSE
};

/* number of attached processes */

PRIVATE_DATA ULONG MulcProcesses = 0;

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* information about current process */

/*
 * Initialize Mpi so it is actually put in the .instanc section instead of the
 * .bss section.
 */

PRIVATE_DATA PROCESSINFO Mpi =
{
   NULL
};

#pragma data_seg()

#ifdef DEBUG

#pragma data_seg(DATA_SEG_SHARED)

/* debug flags */

PRIVATE_DATA DWORD MdwSerialModuleFlags = 0;

#pragma data_seg(DATA_SEG_READ_ONLY)

/* .ini file switch descriptions */

PRIVATE_DATA CBOOLINISWITCH cbisBreakOnProcessAttach =
{
   IST_BOOL,
  TEXT( "BreakOnProcessAttach"),
   &MdwSerialModuleFlags,
   SERIAL_DFL_BREAK_ON_PROCESS_ATTACH
};

PRIVATE_DATA CBOOLINISWITCH cbisBreakOnThreadAttach =
{
   IST_BOOL,
   TEXT("BreakOnThreadAttach"),
   &MdwSerialModuleFlags,
   SERIAL_DFL_BREAK_ON_THREAD_ATTACH
};

PRIVATE_DATA const PCVOID MrgcpcvisSerialModule[] =
{
   &cbisBreakOnProcessAttach,
   &cbisBreakOnThreadAttach
};

#pragma data_seg()

#endif   /* DEBUG */


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCSERIALCONTROL(PCSERIALCONTROL);
PRIVATE_CODE BOOL IsValidPCPROCESSINFO(PCPROCESSINFO);
PRIVATE_CODE BOOL IsValidPCCRITICAL_SECTION(PCCRITICAL_SECTION);
PRIVATE_CODE BOOL IsValidThreadId(DWORD);
PRIVATE_CODE BOOL IsValidPCNONREENTRANTCRITICALSECTION(PCNONREENTRANTCRITICALSECTION);

#endif


#ifdef DEBUG

/*
** IsValidPCSERIALCONTROL()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCSERIALCONTROL(PCSERIALCONTROL pcserctrl)
{
   return(IS_VALID_READ_PTR(pcserctrl, CSERIALCONTROL) &&
          (! pcserctrl->AttachProcess ||
           IS_VALID_CODE_PTR(pcserctrl->AttachProcess, AttachProcess)) &&
          (! pcserctrl->DetachProcess ||
           IS_VALID_CODE_PTR(pcserctrl->DetachProcess, DetachProcess)) &&
          (! pcserctrl->AttachThread ||
           IS_VALID_CODE_PTR(pcserctrl->AttachThread, AttachThread)) &&
          (! pcserctrl->DetachThread||
           IS_VALID_CODE_PTR(pcserctrl->DetachThread, DetachThread)));
}


/*
** IsValidPCPROCESSINFO()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCPROCESSINFO(PCPROCESSINFO pcpi)
{
   return(IS_VALID_READ_PTR(pcpi, CPROCESSINFO) &&
          IS_VALID_HANDLE(pcpi->hModule, MODULE));
}


/*
** IsValidPCCRITICAL_SECTION()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCCRITICAL_SECTION(PCCRITICAL_SECTION pccritsec)
{
   return(IS_VALID_READ_PTR(pccritsec, CCRITICAL_SECTION));
}


/*
** IsValidThreadId()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidThreadId(DWORD dwThreadId)
{
   return(dwThreadId != INVALID_THREAD_ID);
}


/*
** IsValidPCNONREENTRANTCRITICALSECTION()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCNONREENTRANTCRITICALSECTION(
                                          PCNONREENTRANTCRITICALSECTION pcnrcs)
{
   /* bEntered may be any value. */

   return(IS_VALID_READ_PTR(pcnrcs, CNONREENTRANTCRITICALSECTION) &&
          IS_VALID_STRUCT_PTR(&(pcnrcs->critsec), CCRITICAL_SECTION) &&
          EVAL(pcnrcs->dwOwnerThread == INVALID_THREAD_ID ||
               IsValidThreadId(pcnrcs->dwOwnerThread)));
}

#endif


/****************************** Public Functions *****************************/


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

#ifdef DEBUG

/*
** SetSerialModuleIniSwitches()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SetSerialModuleIniSwitches(void)
{
   BOOL bResult;

   bResult = SetIniSwitches(MrgcpcvisSerialModule,
                            ARRAY_ELEMENTS(MrgcpcvisSerialModule));

   ASSERT(FLAGS_ARE_VALID(MdwSerialModuleFlags, ALL_SERIAL_DFLAGS));

   return(bResult);
}

#endif


/*
** AttachProcess()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL AttachProcess(HMODULE hmod)
{
   BOOL bResult;

   ReinitializeNonReentrantCriticalSection(&Mnrcs);

   bResult = EnterNonReentrantCriticalSection(&Mnrcs);

   if (bResult)
   {

#ifdef DEBUG

      ASSERT(SetAllIniSwitches());

      TRACE_OUT((TEXT("AttachProcess(): Called for module %#lx."),
                 hmod));

      if (IS_FLAG_SET(MdwSerialModuleFlags, SERIAL_DFL_BREAK_ON_PROCESS_ATTACH))
      {
         WARNING_OUT((TEXT("AttachProcess(): Breaking on process attach, as requested.")));
         DebugBreak();
      }

#endif   /* DEBUG */

      Mpi.hModule = hmod;

      ASSERT(MulcProcesses < ULONG_MAX);

      if (! MulcProcesses++)
      {
         TRACE_OUT((TEXT("AttachProcess(): First process attached.  Calling InitializeDLL().")));

         bResult = InitializeDLL();
      }
      else
      {

#ifdef PRIVATE_HEAP

         bResult = TRUE;

#else
         /* 
          * Initialize the per-instance memory manager heap for 
          * subsequent processes.
          */

         bResult = InitMemoryManagerModule();

#endif

      }

      if (bResult)
      {
         ASSERT(IS_VALID_STRUCT_PTR(&g_cserctrl, CSERIALCONTROL));

         if (g_cserctrl.AttachProcess)
            bResult = g_cserctrl.AttachProcess(hmod);
      }

      TRACE_OUT((TEXT("AttachProcess(): There are now %lu processes attached."),
                 MulcProcesses));

      LeaveNonReentrantCriticalSection(&Mnrcs);
   }

   return(bResult);
}


/*
** DetachProcess()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL DetachProcess(HMODULE hmod)
{
   BOOL bResult;

   bResult = EnterNonReentrantCriticalSection(&Mnrcs);

   if (bResult)
   {
      ASSERT(hmod == Mpi.hModule);

      ASSERT(MulcProcesses > 0);

      TRACE_OUT((TEXT("DetachProcess(): Called for module %#lx."),
                 hmod));

      ASSERT(IS_VALID_STRUCT_PTR(&g_cserctrl, CSERIALCONTROL));

      if (g_cserctrl.DetachProcess)
         bResult = g_cserctrl.DetachProcess(hmod);

      if (--MulcProcesses)
      {
         bResult = TRUE;

#ifndef PRIVATE_HEAP

         /* 
          * Terminate the per-instance memory manager heap.
          */

         ExitMemoryManagerModule();

#endif
      }
      else
      {
         TRACE_OUT((TEXT("DetachProcess(): Last process detached.  Calling TerminateDLL().")));

         bResult = TerminateDLL();
      }

      TRACE_OUT((TEXT("DetachProcess(): There are now %lu processes attached."),
                 MulcProcesses));

      LeaveNonReentrantCriticalSection(&Mnrcs);
   }

   /*
    * Do not call DeleteCriticalSection(&(Mnrcs->critsec)) here, since doing so
    * at the right time would require unprotected access to shared data
    * MulcProcesses and Mnrcs->critsec.  Assume Kernel32 will clean up
    * Mnrcs->critsec for us at termination.
    */

   return(bResult);
}


/*
** AttachThread()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL AttachThread(HMODULE hmod)
{
   BOOL bResult;

   bResult = EnterNonReentrantCriticalSection(&Mnrcs);

   if (bResult)
   {

#ifdef DEBUG

      ASSERT(SetAllIniSwitches());

      TRACE_OUT((TEXT("AttachThread() called for module %#lx, thread ID %#lx."),
                 hmod,
                 GetCurrentThreadId()));

      if (IS_FLAG_SET(MdwSerialModuleFlags, SERIAL_DFL_BREAK_ON_THREAD_ATTACH))
      {
         WARNING_OUT((TEXT("AttachThread(): Breaking on thread attach, as requested.")));
         DebugBreak();
      }

#endif

      ASSERT(IS_VALID_STRUCT_PTR(&g_cserctrl, CSERIALCONTROL));

      if (g_cserctrl.AttachThread)
         bResult = g_cserctrl.AttachThread(hmod);
      else
         bResult = TRUE;

      LeaveNonReentrantCriticalSection(&Mnrcs);
   }

   return(bResult);
}


/*
** DetachThread()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL DetachThread(HMODULE hmod)
{
   BOOL bResult;

   bResult = EnterNonReentrantCriticalSection(&Mnrcs);

   if (bResult)
   {
      TRACE_OUT((TEXT("DetachThread() called for module %#lx, thread ID %#lx."),
                 hmod,
                 GetCurrentThreadId()));

      ASSERT(IS_VALID_STRUCT_PTR(&g_cserctrl, CSERIALCONTROL));

      if (g_cserctrl.DetachThread)
         bResult = g_cserctrl.DetachThread(hmod);
      else
         bResult = TRUE;

      LeaveNonReentrantCriticalSection(&Mnrcs);
   }

   return(bResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** ReinitializeNonReentrantCriticalSection()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ReinitializeNonReentrantCriticalSection(
                                          PNONREENTRANTCRITICALSECTION pnrcs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pnrcs, CNONREENTRANTCRITICALSECTION));

   InitializeCriticalSection(&(pnrcs->critsec));

   return;
}


/*
** EnterNonReentrantCriticalSection()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL EnterNonReentrantCriticalSection(
                                          PNONREENTRANTCRITICALSECTION pnrcs)
{
   BOOL bEntered;

#ifdef DEBUG

   BOOL bBlocked;

   ASSERT(IS_VALID_STRUCT_PTR(pnrcs, CNONREENTRANTCRITICALSECTION));

   /* Is the critical section already owned by another thread? */

   /* Use pnrcs->bEntered and pnrcs->dwOwnerThread unprotected here. */

   bBlocked = (pnrcs->bEntered &&
               GetCurrentThreadId() != pnrcs->dwOwnerThread);

   if (bBlocked)
      WARNING_OUT((TEXT("EnterNonReentrantCriticalSection(): Blocking thread %lx.  Critical section is already owned by thread %#lx."),
                   GetCurrentThreadId(),
                   pnrcs->dwOwnerThread));

#endif

   EnterCriticalSection(&(pnrcs->critsec));

   bEntered = (! pnrcs->bEntered);

   if (bEntered)
   {
      pnrcs->bEntered = TRUE;

#ifdef DEBUG

      pnrcs->dwOwnerThread = GetCurrentThreadId();

      if (bBlocked)
         WARNING_OUT((TEXT("EnterNonReentrantCriticalSection(): Unblocking thread %lx.  Critical section is now owned by this thread."),
                      pnrcs->dwOwnerThread));
#endif

   }
   else
   {
      LeaveCriticalSection(&(pnrcs->critsec));

      ERROR_OUT((TEXT("EnterNonReentrantCriticalSection(): Thread %#lx attempted to reenter non-reentrant code."),
                 GetCurrentThreadId()));
   }

   return(bEntered);
}


/*
** LeaveNonReentrantCriticalSection()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void LeaveNonReentrantCriticalSection(
                                          PNONREENTRANTCRITICALSECTION pnrcs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pnrcs, CNONREENTRANTCRITICALSECTION));

   if (EVAL(pnrcs->bEntered))
   {
      pnrcs->bEntered = FALSE;
#ifdef DEBUG
      pnrcs->dwOwnerThread = INVALID_THREAD_ID;
#endif

      LeaveCriticalSection(&(pnrcs->critsec));
   }

   return;
}


#ifdef DEBUG

/*
** NonReentrantCriticalSectionIsOwned()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL NonReentrantCriticalSectionIsOwned(
                                          PCNONREENTRANTCRITICALSECTION pcnrcs)
{
   return(pcnrcs->bEntered);
}

#endif


/*
** BeginExclusiveAccess()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL BeginExclusiveAccess(void)
{
   return(EnterNonReentrantCriticalSection(&Mnrcs));
}


/*
** EndExclusiveAccess()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void EndExclusiveAccess(void)
{
   LeaveNonReentrantCriticalSection(&Mnrcs);

   return;
}


#ifdef DEBUG

/*
** AccessIsExclusive()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL AccessIsExclusive(void)
{
   return(NonReentrantCriticalSectionIsOwned(&Mnrcs));
}

#endif   /* DEBUG */


/*
** GetThisModulesHandle()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HMODULE GetThisModulesHandle(void)
{
   ASSERT(IS_VALID_STRUCT_PTR((PCPROCESSINFO)&Mpi, CPROCESSINFO));

   return(Mpi.hModule);
}


