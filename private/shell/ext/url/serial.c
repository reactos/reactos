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

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* critical section used for access serialization */

PRIVATE_DATA NONREENTRANTCRITICALSECTION s_nrcs =
{
   { 0 },

#ifdef DEBUG
   INVALID_THREAD_ID,
#endif   /* DEBUG */

   FALSE
};

/* information about current process */

/*
 * Initialize s_pi so it is actually put in the .instanc section instead of the
 * .bss section.
 */

PRIVATE_DATA PROCESSINFO s_pi =
{
   NULL
};

#pragma data_seg()

#ifdef DEBUG

#pragma data_seg(DATA_SEG_SHARED)

/* debug flags */

PRIVATE_DATA DWORD s_dwSerialModuleFlags = 0;

#pragma data_seg(DATA_SEG_READ_ONLY)

/* .ini file switch descriptions */

PRIVATE_DATA CBOOLINISWITCH s_cbisBreakOnProcessAttach =
{
   IST_BOOL,
   "BreakOnProcessAttach",
   &s_dwSerialModuleFlags,
   SERIAL_DFL_BREAK_ON_PROCESS_ATTACH
};

PRIVATE_DATA CBOOLINISWITCH s_cbisBreakOnThreadAttach =
{
   IST_BOOL,
   "BreakOnThreadAttach",
   &s_dwSerialModuleFlags,
   SERIAL_DFL_BREAK_ON_THREAD_ATTACH
};

PRIVATE_DATA const PCVOID s_rgcpcvisSerialModule[] =
{
   &s_cbisBreakOnProcessAttach,
   &s_cbisBreakOnThreadAttach
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


PRIVATE_CODE BOOL IsValidPCPROCESSINFO(PCPROCESSINFO pcpi)
{
   return(IS_VALID_READ_PTR(pcpi, CPROCESSINFO) &&
          IS_VALID_HANDLE(pcpi->hModule, MODULE));
}


PRIVATE_CODE BOOL IsValidPCCRITICAL_SECTION(PCCRITICAL_SECTION pccritsec)
{
   return(IS_VALID_READ_PTR(pccritsec, CCRITICAL_SECTION));
}


PRIVATE_CODE BOOL IsValidThreadId(DWORD dwThreadId)
{
   return(dwThreadId != INVALID_THREAD_ID);
}


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

PUBLIC_CODE BOOL SetSerialModuleIniSwitches(void)
{
   BOOL bResult;

   bResult = SetIniSwitches(s_rgcpcvisSerialModule,
                            ARRAY_ELEMENTS(s_rgcpcvisSerialModule));

   ASSERT(FLAGS_ARE_VALID(s_dwSerialModuleFlags, ALL_SERIAL_DFLAGS));

   return(bResult);
}

#endif


PUBLIC_CODE BOOL AttachProcess(HMODULE hmod)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hmod, MODULE));

   InitializeNonReentrantCriticalSection(&s_nrcs);

   bResult = EnterNonReentrantCriticalSection(&s_nrcs);

   if (bResult)
   {

#ifdef DEBUG

      ASSERT(SetAllIniSwitches());

      TRACE_OUT(("AttachProcess(): Called for module %#lx.",
                 hmod));

      if (IS_FLAG_SET(s_dwSerialModuleFlags, SERIAL_DFL_BREAK_ON_PROCESS_ATTACH))
      {
         WARNING_OUT(("AttachProcess(): Breaking on process attach, as requested."));
#ifndef MAINWIN
         DebugBreak();
#endif
      }

#endif   /* DEBUG */

      s_pi.hModule = hmod;

      ASSERT(IS_VALID_STRUCT_PTR(&g_cserctrl, CSERIALCONTROL));

      if (g_cserctrl.AttachProcess)
         bResult = g_cserctrl.AttachProcess(hmod);

      LeaveNonReentrantCriticalSection(&s_nrcs);
   }

   return(bResult);
}


PUBLIC_CODE BOOL DetachProcess(HMODULE hmod)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hmod, MODULE));

   bResult = EnterNonReentrantCriticalSection(&s_nrcs);

   if (bResult)
   {
      ASSERT(hmod == s_pi.hModule);

      TRACE_OUT(("DetachProcess(): Called for module %#lx.",
                 hmod));

      ASSERT(IS_VALID_STRUCT_PTR(&g_cserctrl, CSERIALCONTROL));

      if (g_cserctrl.DetachProcess)
         bResult = g_cserctrl.DetachProcess(hmod);

      LeaveNonReentrantCriticalSection(&s_nrcs);

      DeleteNonReentrantCriticalSection(&s_nrcs);
   }

   return(bResult);
}


PUBLIC_CODE BOOL AttachThread(HMODULE hmod)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hmod, MODULE));

   bResult = EnterNonReentrantCriticalSection(&s_nrcs);

   if (bResult)
   {

#ifdef DEBUG

      ASSERT(SetAllIniSwitches());

      TRACE_OUT(("AttachThread() called for module %#lx, thread ID %#lx.",
                 hmod,
                 GetCurrentThreadId()));

      if (IS_FLAG_SET(s_dwSerialModuleFlags, SERIAL_DFL_BREAK_ON_THREAD_ATTACH))
      {
         WARNING_OUT(("AttachThread(): Breaking on thread attach, as requested."));
#ifndef MAINWIN
         DebugBreak();
#endif 
      }

#endif

      ASSERT(IS_VALID_STRUCT_PTR(&g_cserctrl, CSERIALCONTROL));

      if (g_cserctrl.AttachThread)
         bResult = g_cserctrl.AttachThread(hmod);
      else
         bResult = TRUE;

      LeaveNonReentrantCriticalSection(&s_nrcs);
   }

   return(bResult);
}


PUBLIC_CODE BOOL DetachThread(HMODULE hmod)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hmod, MODULE));

   bResult = EnterNonReentrantCriticalSection(&s_nrcs);

   if (bResult)
   {
      TRACE_OUT(("DetachThread() called for module %#lx, thread ID %#lx.",
                 hmod,
                 GetCurrentThreadId()));

      ASSERT(IS_VALID_STRUCT_PTR(&g_cserctrl, CSERIALCONTROL));

      if (g_cserctrl.DetachThread)
         bResult = g_cserctrl.DetachThread(hmod);
      else
         bResult = TRUE;

      LeaveNonReentrantCriticalSection(&s_nrcs);
   }

   return(bResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


PUBLIC_CODE void InitializeNonReentrantCriticalSection(
                                          PNONREENTRANTCRITICALSECTION pnrcs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pnrcs, CNONREENTRANTCRITICALSECTION));

   InitializeCriticalSection(&(pnrcs->critsec));

   pnrcs->bEntered = FALSE;
#ifdef DEBUG
   pnrcs->dwOwnerThread = INVALID_THREAD_ID;
#endif

   return;
}


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
      TRACE_OUT(("EnterNonReentrantCriticalSection(): Blocking thread %lx.  Critical section is already owned by thread %#lx.",
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
         TRACE_OUT(("EnterNonReentrantCriticalSection(): Unblocking thread %lx.  Critical section is now owned by this thread.",
                    pnrcs->dwOwnerThread));
#endif

   }
   else
   {
      LeaveCriticalSection(&(pnrcs->critsec));

      ERROR_OUT(("EnterNonReentrantCriticalSection(): Thread %#lx attempted to reenter non-reentrant code.",
                 GetCurrentThreadId()));
   }

   return(bEntered);
}


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


PUBLIC_CODE void DeleteNonReentrantCriticalSection(
                                          PNONREENTRANTCRITICALSECTION pnrcs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pnrcs, CNONREENTRANTCRITICALSECTION));

   ASSERT(! pnrcs->bEntered);
   ASSERT(pnrcs->dwOwnerThread == INVALID_THREAD_ID);

   DeleteCriticalSection(&(pnrcs->critsec));

   return;
}


#ifdef DEBUG

PUBLIC_CODE BOOL NonReentrantCriticalSectionIsOwned(
                                          PCNONREENTRANTCRITICALSECTION pcnrcs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcnrcs, CNONREENTRANTCRITICALSECTION));

   return(pcnrcs->bEntered);
}


PUBLIC_CODE DWORD GetNonReentrantCriticalSectionOwner(
                                          PCNONREENTRANTCRITICALSECTION pcnrcs)

{
   ASSERT(IS_VALID_STRUCT_PTR(pcnrcs, CNONREENTRANTCRITICALSECTION));

   return(pcnrcs->dwOwnerThread);
}

#endif


PUBLIC_CODE BOOL BeginExclusiveAccess(void)
{
   return(EnterNonReentrantCriticalSection(&s_nrcs));
}


PUBLIC_CODE void EndExclusiveAccess(void)
{
   LeaveNonReentrantCriticalSection(&s_nrcs);

   return;
}


#ifdef DEBUG

PUBLIC_CODE BOOL AccessIsExclusive(void)
{
   return(NonReentrantCriticalSectionIsOwned(&s_nrcs) &&
          GetNonReentrantCriticalSectionOwner(&s_nrcs) == GetCurrentThreadId());
}

#endif   /* DEBUG */


PUBLIC_CODE HMODULE GetThisModulesHandle(void)
{
   ASSERT(IS_VALID_STRUCT_PTR((PCPROCESSINFO)&s_pi, CPROCESSINFO));

   return(s_pi.hModule);
}

