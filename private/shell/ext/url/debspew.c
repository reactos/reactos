/*
 * debspew.c - Debug spew functions module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#ifdef DEBUG


/* Types
 ********/


/* debug flags */

typedef enum _debugdebugflags
{
   DEBUG_DFL_ENABLE_TRACE_MESSAGES  = 0x0001,

   DEBUG_DFL_LOG_TRACE_MESSAGES     = 0x0002,

   DEBUG_DFL_DUMP_THREAD_ID         = 0x0004,

   ALL_DEBUG_DFLAGS                 = (DEBUG_DFL_ENABLE_TRACE_MESSAGES |
                                       DEBUG_DFL_LOG_TRACE_MESSAGES |
                                       DEBUG_DFL_DUMP_THREAD_ID)
}
DEBUGDEBUGFLAGS;



/* Module Constants
 *******************/


#pragma data_seg(DATA_SEG_READ_ONLY)

/* debug message output log file */

PRIVATE_DATA CCHAR s_cszLogFile[]   = "debug.log";

#pragma data_seg()



/* Global Variables
 *******************/

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* parameters used by SpewOut() */

PUBLIC_DATA char SrgchSpewLeader[] = "                                                                                ";

PUBLIC_DATA DWORD g_dwSpewFlags = 0;
PUBLIC_DATA UINT g_uSpewSev = 0;
PUBLIC_DATA UINT g_uSpewLine = 0;
PUBLIC_DATA PCSTR g_pcszSpewFile = NULL;

#pragma data_seg()



/* Module Variables
 *******************/


#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* TLS slot used to store stack depth for SpewOut() indentation */

PRIVATE_DATA DWORD s_dwStackDepthSlot = TLS_OUT_OF_INDEXES;

/* hack stack depth counter used until s_dwStackDepthSlot is not available */

PRIVATE_DATA ULONG s_ulcHackStackDepth = 0;

#pragma data_seg(DATA_SEG_SHARED)

/* debug flags */

PRIVATE_DATA DWORD s_dwDebugModuleFlags = 0;

#pragma data_seg(DATA_SEG_READ_ONLY)

/* .ini file switch descriptions */

PRIVATE_DATA CBOOLINISWITCH s_cbisEnableTraceMessages =
{
   IST_BOOL,
   "EnableTraceMessages",
   &s_dwDebugModuleFlags,
   DEBUG_DFL_ENABLE_TRACE_MESSAGES
};

PRIVATE_DATA CBOOLINISWITCH s_cbisLogTraceMessages =
{
   IST_BOOL,
   "LogTraceMessages",
   &s_dwDebugModuleFlags,
   DEBUG_DFL_LOG_TRACE_MESSAGES
};

PRIVATE_DATA CBOOLINISWITCH s_cbisDumpThreadID =
{
   IST_BOOL,
   "DumpThreadID",
   &s_dwDebugModuleFlags,
   DEBUG_DFL_DUMP_THREAD_ID
};

PRIVATE_DATA const PCVOID s_rgcpcvisDebugModule[] =
{
   &s_cbisLogTraceMessages,
   &s_cbisEnableTraceMessages,
   &s_cbisDumpThreadID
};

#pragma data_seg()



/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/


PRIVATE_CODE BOOL LogOutputDebugString(PCSTR);
PRIVATE_CODE BOOL IsValidSpewSev(UINT);




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
PRIVATE_CODE BOOL LogOutputDebugString(PCSTR pcsz)
{
   BOOL bResult = FALSE;
   UINT ucb;
   char rgchLogFile[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));

   ucb = GetWindowsDirectory(rgchLogFile, sizeof(rgchLogFile));

   if (ucb > 0 && ucb < sizeof(rgchLogFile))
   {
      HANDLE hfLog;


      lstrcat(rgchLogFile, "\\");
      lstrcat(rgchLogFile, s_cszLogFile);

      hfLog = CreateFile(rgchLogFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                         FILE_FLAG_WRITE_THROUGH, NULL);

      if (hfLog != INVALID_HANDLE_VALUE)
      {
         if (SetFilePointer(hfLog, 0, NULL, FILE_END) != INVALID_SEEK_POSITION)
         {
            DWORD dwcbWritten;

            bResult = WriteFile(hfLog, pcsz, lstrlen(pcsz), &dwcbWritten, NULL);

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
         ERROR_OUT(("IsValidSpewSev(): Invalid debug spew severity %u.",
                    uSpewSev));
         bResult = FALSE;
         break;
   }

   return(bResult);
}



/****************************** Public Functions *****************************/



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

   bResult = SetIniSwitches(s_rgcpcvisDebugModule,
                            ARRAY_ELEMENTS(s_rgcpcvisDebugModule));

   ASSERT(FLAGS_ARE_VALID(s_dwDebugModuleFlags, ALL_DEBUG_DFLAGS));

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
   ASSERT(s_dwStackDepthSlot == TLS_OUT_OF_INDEXES);

   s_dwStackDepthSlot = TlsAlloc();

   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      EVAL(TlsSetValue(s_dwStackDepthSlot, (PVOID)s_ulcHackStackDepth));

      TRACE_OUT(("InitDebugModule(): Using thread local storage slot %lu for debug stack depth counter.",
                 s_dwStackDepthSlot));
   }
   else
      WARNING_OUT(("InitDebugModule(): TlsAlloc() failed to allocate thread local storage for debug stack depth counter."));

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
   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      s_ulcHackStackDepth = PtrToUlong(TlsGetValue(s_dwStackDepthSlot));

      /* Leave s_ulcHackStackDepth == 0 if TlsGetValue() fails. */

      EVAL(TlsFree(s_dwStackDepthSlot));
      s_dwStackDepthSlot = TLS_OUT_OF_INDEXES;
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
   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      ULONG ulcDepth;

      ulcDepth = PtrToUlong(TlsGetValue(s_dwStackDepthSlot));

      ASSERT(ulcDepth < ULONG_MAX);

      EVAL(TlsSetValue(s_dwStackDepthSlot, (PVOID)(ulcDepth + 1)));
   }
   else
   {
      ASSERT(s_ulcHackStackDepth < ULONG_MAX);
      s_ulcHackStackDepth++;
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
   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      ULONG ulcDepth;

      ulcDepth = PtrToUlong(TlsGetValue(s_dwStackDepthSlot));

      if (EVAL(ulcDepth > 0))
         EVAL(TlsSetValue(s_dwStackDepthSlot, (PVOID)(ulcDepth - 1)));
   }
   else
   {
      if (EVAL(s_ulcHackStackDepth > 0))
         s_ulcHackStackDepth--;
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

   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
      ulcDepth = PtrToUlong(TlsGetValue(s_dwStackDepthSlot));
   else
      ulcDepth = s_ulcHackStackDepth;

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
** N.b., this function assumes the global variables g_dwSpewFlags, g_uSpewSev,
** g_pcszSpewModule, g_pcszSpewFile, and g_pcszSpewLine are filled in.
**
** SpewOut() uses global variables to set the message parameters in order to
** permit printf()-style macro expansion.
*/
PUBLIC_CODE void SpewOut(PCSTR pcszFormat, ...)
{
   ASSERT(IS_VALID_STRING_PTR(pcszFormat, CSTR));

   ASSERT(FLAGS_ARE_VALID(g_dwSpewFlags, ALL_SPEW_FLAGS));
   ASSERT(IsValidSpewSev(g_uSpewSev));
   ASSERT(IS_FLAG_CLEAR(g_dwSpewFlags, SPEW_FL_SPEW_LOCATION) ||
          (IS_VALID_STRING_PTR(g_pcszSpewFile, CSTR) &&
           IS_VALID_STRING_PTR(g_pcszSpewModule, CSTR)));

   if (g_uSpewSev != SPEW_TRACE || IS_FLAG_SET(s_dwDebugModuleFlags, DEBUG_DFL_ENABLE_TRACE_MESSAGES))
   {
      int nMsgLen;
      char rgchMsg[1024];

      va_list nextArg;


      if (IS_FLAG_SET(g_dwSpewFlags, SPEW_FL_SPEW_PREFIX))
      {
         ULONG ulcStackDepth;
         char chReplaced;
         PSTR pszSpewLeaderEnd;
         PCSTR pcszSpewPrefix;

         /* Build spew message space leader string. */

         ulcStackDepth = GetStackDepth();

         if (ulcStackDepth < sizeof(SrgchSpewLeader))
            pszSpewLeaderEnd = SrgchSpewLeader + ulcStackDepth;
         else
            pszSpewLeaderEnd = SrgchSpewLeader + sizeof(SrgchSpewLeader) - 1;

         chReplaced = *pszSpewLeaderEnd;
         *pszSpewLeaderEnd = '\0';

         /* Determine spew prefix. */

         switch (g_uSpewSev)
         {
            case SPEW_TRACE:
               pcszSpewPrefix = "t";
               break;

            case SPEW_WARNING:
               pcszSpewPrefix = "w";
               break;

            case SPEW_ERROR:
               pcszSpewPrefix = "e";
               break;

            case SPEW_FATAL:
               pcszSpewPrefix = "f";
               break;

            default:
               pcszSpewPrefix = "u";
               ERROR_OUT(("SpewOut(): Invalid g_uSpewSev %u.",
                          g_uSpewSev));
               break;
         }

         nMsgLen = wsprintf(rgchMsg, "%s%s %s ", SrgchSpewLeader, pcszSpewPrefix, g_pcszSpewModule);

         /* Restore spew leader. */

         *pszSpewLeaderEnd = chReplaced;

         ASSERT(nMsgLen < sizeof(rgchMsg));
      }
      else
         nMsgLen = 0;

      /* Append thread ID. */

      if (IS_FLAG_SET(s_dwDebugModuleFlags, DEBUG_DFL_DUMP_THREAD_ID))
      {
         nMsgLen += wsprintf(rgchMsg + nMsgLen, "%#lx ", GetCurrentThreadId());

         ASSERT(nMsgLen < sizeof(rgchMsg));
      }

      /* Build position string. */

      if (IS_FLAG_SET(g_dwSpewFlags, SPEW_FL_SPEW_LOCATION))
      {
         nMsgLen += wsprintf(rgchMsg + nMsgLen, "(%s line %u): ", g_pcszSpewFile, g_uSpewLine);

         ASSERT(nMsgLen < sizeof(rgchMsg));
      }

      /* Append message string. */


      va_start(nextArg, pcszFormat);
      nMsgLen += wvsprintf(rgchMsg + nMsgLen, pcszFormat, nextArg);
      va_end(nextArg);

      ASSERT(nMsgLen < sizeof(rgchMsg));

      if (g_uSpewSev == SPEW_ERROR ||
          g_uSpewSev == SPEW_FATAL)
      {
         nMsgLen += wsprintf(rgchMsg + nMsgLen, " (GetLastError() == %lu)", GetLastError());

         ASSERT(nMsgLen < sizeof(rgchMsg));
      }

      nMsgLen += wsprintf(rgchMsg + nMsgLen, "\r\n");

      ASSERT(nMsgLen < sizeof(rgchMsg));

      OutputDebugString(rgchMsg);

      if (IS_FLAG_SET(s_dwDebugModuleFlags, DEBUG_DFL_LOG_TRACE_MESSAGES))
         LogOutputDebugString(rgchMsg);
   }

   /* Break here on errors and fatal errors. */
#ifndef MAINWIN
   if (g_uSpewSev == SPEW_ERROR || g_uSpewSev == SPEW_FATAL)
      DebugBreak();
#endif
   return;
}

#endif   /* DEBUG */
