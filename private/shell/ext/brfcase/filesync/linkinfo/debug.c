/*
 * debug.c - Debug functions module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Constants
 ************/

#ifdef DEBUG

#define LOG_FILE_NAME               TEXT("debug.log")

#endif


/* Types
 ********/

#ifdef DEBUG

/* debug flags */

typedef enum _debugdebugflags
{
   DEBUG_DFL_ENABLE_TRACE_MESSAGES  = 0x0001,

   DEBUG_DFL_LOG_TRACE_MESSAGES     = 0x0002,

   DEBUG_DFL_DUMP_THREAD_ID         = 0x0004,

   DEBUG_DFL_DUMP_LAST_ERROR        = 0x0008,

   ALL_DEBUG_DFLAGS                 = (DEBUG_DFL_ENABLE_TRACE_MESSAGES |
                                       DEBUG_DFL_LOG_TRACE_MESSAGES |
                                       DEBUG_DFL_DUMP_THREAD_ID |
                                       DEBUG_DFL_DUMP_LAST_ERROR)
}
DEBUGDEBUGFLAGS;

#endif


/* Global Variables
 *******************/

#ifdef DEBUG

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* parameters used by SpewOut() */

PUBLIC_DATA DWORD GdwSpewFlags = 0;
PUBLIC_DATA UINT GuSpewSev = 0;
PUBLIC_DATA UINT GuSpewLine = 0;
PUBLIC_DATA LPCTSTR GpcszSpewFile = NULL;

#pragma data_seg()

#endif   /* DEBUG */


/* Module Variables
 *******************/

#ifdef DEBUG

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* TLS slot used to store stack depth for SpewOut() indentation */

PRIVATE_DATA DWORD MdwStackDepthSlot = TLS_OUT_OF_INDEXES;

/* hack stack depth counter used until MdwStackDepthSlot is not available */

PRIVATE_DATA ULONG MulcHackStackDepth = 0;

#pragma data_seg(DATA_SEG_SHARED)

/* debug flags */

PRIVATE_DATA DWORD MdwDebugModuleFlags = 0;

#pragma data_seg(DATA_SEG_READ_ONLY)

/* .ini file switch descriptions */

PRIVATE_DATA CBOOLINISWITCH cbisEnableTraceMessages =
{
   IST_BOOL,
   TEXT("EnableTraceMessages"),
   &MdwDebugModuleFlags,
   DEBUG_DFL_ENABLE_TRACE_MESSAGES
};

PRIVATE_DATA CBOOLINISWITCH cbisLogTraceMessages =
{
   IST_BOOL,
   TEXT("LogTraceMessages"),
   &MdwDebugModuleFlags,
   DEBUG_DFL_LOG_TRACE_MESSAGES
};

PRIVATE_DATA CBOOLINISWITCH cbisDumpThreadID =
{
   IST_BOOL,
   TEXT("DumpThreadID"),
   &MdwDebugModuleFlags,
   DEBUG_DFL_DUMP_THREAD_ID
};

PRIVATE_DATA CBOOLINISWITCH cbisDumpLastError =
{
   IST_BOOL,
   TEXT("DumpLastError"),
   &MdwDebugModuleFlags,
   DEBUG_DFL_DUMP_LAST_ERROR
};

PRIVATE_DATA const PCVOID MrgcpcvisDebugModule[] =
{
   &cbisLogTraceMessages,
   &cbisEnableTraceMessages,
   &cbisDumpThreadID,
   &cbisDumpLastError
};

#pragma data_seg()

#endif   /* DEBUG */


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

#ifdef DEBUG

PRIVATE_CODE BOOL LogOutputDebugString(LPCTSTR);
PRIVATE_CODE BOOL IsValidSpewSev(UINT);

#endif   /* DEBUG */


#ifdef DEBUG

/*
** LogOutputDebugString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL LogOutputDebugString(LPCTSTR pcsz)
{
   BOOL bResult = FALSE;
   UINT ucb;
   TCHAR rgchLogFile[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   ucb = GetWindowsDirectory(rgchLogFile, ARRAYSIZE(rgchLogFile));

   if (ucb > 0 && ucb < ARRAYSIZE(rgchLogFile))
   {
      HANDLE hfLog;

      lstrcat(rgchLogFile, TEXT("\\"));
      lstrcat(rgchLogFile, LOG_FILE_NAME);

      hfLog = CreateFile(rgchLogFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                         FILE_FLAG_WRITE_THROUGH, NULL);

      if (hfLog != INVALID_HANDLE_VALUE)
      {
         if (SetFilePointer(hfLog, 0, NULL, FILE_END) != INVALID_SEEK_POSITION)
         {
            DWORD dwcbWritten;

            bResult = WriteFile(hfLog, pcsz, lstrlen(pcsz)*SIZEOF(TCHAR), &dwcbWritten, NULL);

            if (! CloseHandle(hfLog) && bResult)
               bResult = FALSE;
         }
      }
   }

   return(bResult);
}


/*
** IsValidSpewSev()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidSpewSev(UINT uSpewSev)
{
   BOOL bResult;

   switch (uSpewSev)
   {
      case SPEW_TRACE:
      case SPEW_WARNING:
      case SPEW_ERROR:
      case SPEW_FATAL:
         bResult = TRUE;
         break;

      default:
         ERROR_OUT((TEXT("IsValidSpewSev(): Invalid debug spew severity %u."),
                    uSpewSev));
         bResult = FALSE;
         break;
   }

   return(bResult);
}

#endif   /* DEBUG */


/****************************** Public Functions *****************************/


#ifdef DEBUG

/*
** SetDebugModuleIniSwitches()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SetDebugModuleIniSwitches(void)
{
   BOOL bResult;

   bResult = SetIniSwitches(MrgcpcvisDebugModule,
                            ARRAY_ELEMENTS(MrgcpcvisDebugModule));

   ASSERT(FLAGS_ARE_VALID(MdwDebugModuleFlags, ALL_DEBUG_DFLAGS));

   return(bResult);
}


/*
** InitDebugModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL InitDebugModule(void)
{
   ASSERT(MdwStackDepthSlot == TLS_OUT_OF_INDEXES);

   MdwStackDepthSlot = TlsAlloc();

   if (MdwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      EVAL(TlsSetValue(MdwStackDepthSlot, (PVOID)MulcHackStackDepth));

      TRACE_OUT((TEXT("InitDebugModule(): Using thread local storage slot %lu for debug stack depth counter."),
                 MdwStackDepthSlot));
   }
   else
      WARNING_OUT((TEXT("InitDebugModule(): TlsAlloc() failed to allocate thread local storage for debug stack depth counter.")));

   return(TRUE);
}


/*
** ExitDebugModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ExitDebugModule(void)
{
   if (MdwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      MulcHackStackDepth = PtrToUlong(TlsGetValue(MdwStackDepthSlot));

      /* Leave MulcHackStackDepth == 0 if TlsGetValue() fails. */

      EVAL(TlsFree(MdwStackDepthSlot));
      MdwStackDepthSlot = TLS_OUT_OF_INDEXES;
   }

   return;
}


/*
** StackEnter()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void StackEnter(void)
{
   if (MdwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      ULONG ulcDepth;

      ulcDepth = PtrToUlong(TlsGetValue(MdwStackDepthSlot));

      ASSERT(ulcDepth < ULONG_MAX);

      EVAL(TlsSetValue(MdwStackDepthSlot, (PVOID)(ulcDepth + 1)));
   }
   else
   {
      ASSERT(MulcHackStackDepth < ULONG_MAX);
      InterlockedIncrement(&MulcHackStackDepth);
   }

   return;
}


/*
** StackLeave()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void StackLeave(void)
{
   if (MdwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      ULONG ulcDepth;

      ulcDepth = PtrToUlong(TlsGetValue(MdwStackDepthSlot));

      if (EVAL(ulcDepth > 0))
         EVAL(TlsSetValue(MdwStackDepthSlot, (PVOID)(ulcDepth - 1)));
   }
   else
   {
      if (EVAL(MulcHackStackDepth > 0))
         InterlockedDecrement(&MulcHackStackDepth);
   }

   return;
}


/*
** GetStackDepth()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE ULONG GetStackDepth(void)
{
   ULONG ulcDepth;

   if (MdwStackDepthSlot != TLS_OUT_OF_INDEXES)
      ulcDepth = PtrToUlong(TlsGetValue(MdwStackDepthSlot));
   else
      ulcDepth = MulcHackStackDepth;

   return(ulcDepth);
}


/*
** SpewOut()
**
** Spews out a formatted message to the debug terminal.
**
** Arguments:     pcszFormat - pointer to wvsprintf() format string
**                ... - formatting arguments ala wvsprintf()
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., this function assumes the global variables GdwSpewFlags, GuSpewSev,
** GpcszSpewModule, GpcszSpewFile, and GpcszSpewLine are filled in.
**
** SpewOut() uses global variables to set the message parameters in order to
** permit printf()-style macro expansion.
*/
PUBLIC_CODE void __cdecl SpewOut(LPCTSTR pcszFormat, ...)
{
   va_list  arglist;

   ASSERT(IS_VALID_STRING_PTR(pcszFormat, CSTR));

   ASSERT(FLAGS_ARE_VALID(GdwSpewFlags, ALL_SPEW_FLAGS));
   ASSERT(IsValidSpewSev(GuSpewSev));
   ASSERT(IS_FLAG_CLEAR(GdwSpewFlags, SPEW_FL_SPEW_LOCATION) ||
          (IS_VALID_STRING_PTR(GpcszSpewFile, CSTR) &&
           IS_VALID_STRING_PTR(GpcszSpewModule, CSTR)));

   if (GuSpewSev != SPEW_TRACE || IS_FLAG_SET(MdwDebugModuleFlags, DEBUG_DFL_ENABLE_TRACE_MESSAGES))
   {
      int nMsgLen;
      TCHAR rgchMsg[1024];

      if (IS_FLAG_SET(GdwSpewFlags, SPEW_FL_SPEW_PREFIX))
      {
#pragma data_seg(DATA_SEG_SHARED)
         static TCHAR SrgchSpewLeader[] = TEXT("                                                                                ");
#pragma data_seg()
         ULONG ulcStackDepth;
         TCHAR chReplaced;
         LPTSTR pszSpewLeaderEnd;
         LPCTSTR pcszSpewPrefix;

         /* Build spew message space leader string. */

         ulcStackDepth = GetStackDepth();

         if (ulcStackDepth < ARRAYSIZE(SrgchSpewLeader))
            pszSpewLeaderEnd = SrgchSpewLeader + ulcStackDepth;
         else
            pszSpewLeaderEnd = SrgchSpewLeader + ARRAYSIZE(SrgchSpewLeader) - 1;

         chReplaced = *pszSpewLeaderEnd;
         *pszSpewLeaderEnd = TEXT('\0');

         /* Determine spew prefix. */

         switch (GuSpewSev)
         {
            case SPEW_TRACE:
               pcszSpewPrefix = TEXT("t");
               break;

            case SPEW_WARNING:
               pcszSpewPrefix = TEXT("w");
               break;

            case SPEW_ERROR:
               pcszSpewPrefix = TEXT("e");
               break;

            case SPEW_FATAL:
               pcszSpewPrefix = TEXT("f");
               break;

            default:
               pcszSpewPrefix = TEXT("u");
               ERROR_OUT((TEXT("SpewOut(): Invalid GuSpewSev %u."),
                          GuSpewSev));
               break;
         }

         nMsgLen = wsprintf(rgchMsg, TEXT("%s%s %s "), SrgchSpewLeader, pcszSpewPrefix, GpcszSpewModule);

         /* Restore spew leader. */

         *pszSpewLeaderEnd = chReplaced;

         ASSERT(nMsgLen < ARRAYSIZE(rgchMsg));
      }
      else
         nMsgLen = 0;

      /* Append thread ID. */

      if (IS_FLAG_SET(MdwDebugModuleFlags, DEBUG_DFL_DUMP_THREAD_ID))
      {
         nMsgLen += wsprintf(rgchMsg + nMsgLen, TEXT("%#lx "), GetCurrentThreadId());

         ASSERT(nMsgLen < ARRAYSIZE(rgchMsg));
      }

      /* Build position string. */

      if (IS_FLAG_SET(GdwSpewFlags, SPEW_FL_SPEW_LOCATION))
      {
         nMsgLen += wsprintf(rgchMsg + nMsgLen, TEXT("(%s line %u): "), GpcszSpewFile, GuSpewLine);

         ASSERT(nMsgLen < ARRAYSIZE(rgchMsg));
      }

      /* Append message string. */

      va_start(arglist,pcszFormat);
      nMsgLen += wvsprintf(rgchMsg + nMsgLen, pcszFormat, arglist);
      va_end(arglist);

      ASSERT(nMsgLen < ARRAYSIZE(rgchMsg));

      if (IS_FLAG_SET(GdwSpewFlags, DEBUG_DFL_DUMP_THREAD_ID))
      {
         if (GuSpewSev == SPEW_ERROR ||
             GuSpewSev == SPEW_FATAL)
         {
            nMsgLen += wsprintf(rgchMsg + nMsgLen, TEXT(" (GetLastError() == %lu)"), GetLastError());

            ASSERT(nMsgLen < ARRAYSIZE(rgchMsg));
         }
      }

      nMsgLen += wsprintf(rgchMsg + nMsgLen, TEXT("\r\n"));

      ASSERT(nMsgLen < ARRAYSIZE(rgchMsg));

      OutputDebugString(rgchMsg);

      if (IS_FLAG_SET(MdwDebugModuleFlags, DEBUG_DFL_LOG_TRACE_MESSAGES))
      {
         LogOutputDebugString(rgchMsg);
         LogOutputDebugString(TEXT("\r\n"));
      }
   }

   /* Break here on errors and fatal errors. */

   if (GuSpewSev == SPEW_ERROR || GuSpewSev == SPEW_FATAL)
      DebugBreak();

   return;
}

#endif   /* DEBUG */
