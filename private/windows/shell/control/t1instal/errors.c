/***
 **
 **   Module: T1Instal
 **
 **   Description:
 **      This is a Win32 DLL specific module, that implements
 **      the error logging mechanism under Win32.
 **
 **   Author: Michael Jansson
 **   Created: 12/18/93
 **
 ***/


/***** INCLUDES */
#include <windows.h>
#include "types.h"
#include "t1local.h"
#ifdef NOMSGBOX
#include <stdio.h>
#endif



/***** CONSTANTS */
/*-none-*/



/***** GLOBALS */
extern HANDLE hInst;       /* Cached in the T1Instal module. */



/***** PROTOTYPES */
extern int __cdecl sprintf(char *, const char *, ...);


/***
 ** Function: LogError
 **
 ** Description:
 **   Add another message to the error log.
 ***/
void LogError(const long type, const long id, const char *arg)
{
   char caption[256];
   char msg[256];
   WORD etype;
   HANDLE h;
   DWORD logit;
   DWORD size;
   HKEY key;

   /* Map the internal envent type to EventLog type. */
   if (type==MSG_INFO)
      etype = EVENTLOG_INFORMATION_TYPE;
   else if (type==MSG_WARNING)
      etype = EVENTLOG_WARNING_TYPE;
   else
      etype = EVENTLOG_ERROR_TYPE;

   /* Access the REG data base. */
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SUBKEY_TYPE1INSTAL, 0,
                    KEY_QUERY_VALUE, &key)==ERROR_SUCCESS) { 

      size = sizeof(logit);
      if (RegQueryValueEx(key, (LPTSTR)SUBKEY_LOGFILE, NULL, 
                          NULL, (LPVOID)&logit, &size)==ERROR_SUCCESS &&
          logit!=0) {

         h = RegisterEventSource(NULL, STR_APPNAME);
         if (h!=NULL) {
            ReportEvent(h, etype, 0, id, NULL, 1, 0, (LPSTR *)&arg, NULL);
            DeregisterEventSource(h);
         }

         if (etype==EVENTLOG_WARNING_TYPE) {
            LoadString(hInst, (UINT)id, caption, sizeof(caption));
            sprintf(msg, caption, arg);
            LoadString(hInst, IDS_CAPTION, caption, sizeof(caption));
#if NOMSGBOX
            fputs("WARNING- ", stderr);
            fputs(msg, stderr);
            fputs("\n", stderr);
#else         
            MessageBox(NULL, msg, caption, INFO);
#endif
            SetLastError(0);  /* MessageBox(NULL,...) is broken */
         }
      }

      if (etype==EVENTLOG_ERROR_TYPE) {
         LoadString(hInst, (UINT)id, caption, sizeof(caption));
         sprintf(msg, caption, arg);
         LoadString(hInst, IDS_CAPTION, caption, sizeof(caption));
#if NOMSGBOX
         fputs("ERROR  - ", stderr);
         fputs(msg, stderr);
         fputs("\n", stderr);
#else         
         MessageBox(NULL, msg, caption, INFO);
#endif
         SetLastError(0); /* MessageBox(NULL,...) is broken */
      }
      RegCloseKey(key);
   }
}


