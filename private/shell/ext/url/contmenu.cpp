/*
 * contmenu.cpp - Context menu implementation for URL class.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include <mapi.h>

#include "resource.h"

#ifdef UNIX
#include <tchar.h>
#include <inetreg.h>
static HRESULT UnixReadNews(HWND hwndParent, PCSTR pszCmdLine);
#endif

/* Types
 ********/

/* MAPISendMail() typedef */

typedef ULONG (FAR PASCAL *MAPISENDMAILPROC)(LHANDLE lhSession, ULONG ulUIParam, lpMapiMessageA lpMessage, FLAGS flFlags, ULONG ulReserved);

/* RunDLL32 DLL entry point typedef */

typedef void (WINAPI *RUNDLL32PROC)(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd);


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

// case-insensitive

PRIVATE_DATA const char s_cszFileProtocolPrefix[]     = "file:";
PRIVATE_DATA const char s_cszMailToProtocolPrefix[]   = "mailto:";
PRIVATE_DATA const char s_cszRLoginProtocolPrefix[]   = "rlogin:";
PRIVATE_DATA const char s_cszTelnetProtocolPrefix[]   = "telnet:";
PRIVATE_DATA const char s_cszTN3270ProtocolPrefix[]   = "tn3270:";

PRIVATE_DATA const char s_cszNewsDLL[]                = "mcm.dll";
#ifndef UNIX
PRIVATE_DATA const char s_cszTelnetApp[]              = "telnet.exe";
#else
PRIVATE_DATA const char s_cszTelnetApp[]              = "telnet";
#endif

PRIVATE_DATA const char s_cszMAPISection[]            = "Mail";
PRIVATE_DATA const char s_cszMAPIKey[]                = "CMCDLLName32";

PRIVATE_DATA const char s_cszMAPISendMail[]           = "MAPISendMail";
PRIVATE_DATA const char s_cszNewsProtocolHandler[]    = "NewsProtocolHandler";

#pragma data_seg()


/***************************** Exported Functions ****************************/


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

extern "C" void WINAPI OpenURL(HWND hwndParent, HINSTANCE hinst,
                               PSTR pszCmdLine, int nShowCmd)
{
   HRESULT hr;
   InternetShortcut intshcut;
   int nResult;

   DebugEntry(OpenURL);

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_HANDLE(hinst, INSTANCE));
   ASSERT(IS_VALID_STRING_PTR(pszCmdLine, STR));
   ASSERT(IsValidShowCmd(nShowCmd));

   // Assume the entire command line is an Internet Shortcut file path.

   TrimWhiteSpace(pszCmdLine);

   TRACE_OUT(("OpenURL(): Trying to open Internet Shortcut %s.",
              pszCmdLine));

   hr = intshcut.LoadFromFile(pszCmdLine, TRUE);

   if (hr == S_OK)
   {
      URLINVOKECOMMANDINFO urlici;

      urlici.dwcbSize = sizeof(urlici);
      urlici.hwndParent = hwndParent;
      urlici.pcszVerb = NULL;
      urlici.dwFlags = (IURL_INVOKECOMMAND_FL_ALLOW_UI |
                        IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB);

      hr = intshcut.InvokeCommand(&urlici);
   }
   else
   {
      if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                   MAKEINTRESOURCE(IDS_LOADFROMFILE_FAILED),
                   (MB_OK | MB_ICONEXCLAMATION), &nResult, pszCmdLine)) {
         ASSERT(nResult == IDOK);
       }
   }

   DebugExitVOID(OpenURL);

   return;
}


extern "C" void WINAPI FileProtocolHandler(HWND hwndParent, HINSTANCE hinst,
                                           PSTR pszCmdLine, int nShowCmd)
{
   char szDefaultVerb[MAX_PATH_LEN];
   PCSTR pcszVerb;
   HINSTANCE hinstExec;
   int nResult;
   PARSEDURL pu;

   DebugEntry(FileProtocolHandler);

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_HANDLE(hinst, INSTANCE));
   ASSERT(IS_VALID_STRING_PTR(pszCmdLine, STR));
   ASSERT(IsValidShowCmd(nShowCmd));

   // Assume the entire command line is a file: URL.

   TrimWhiteSpace(pszCmdLine);

   pu.cbSize = sizeof(pu);
   if (S_OK == ParseURL(pszCmdLine, &pu) &&
       URL_SCHEME_FILE == pu.nScheme)
   {
      pszCmdLine = (LPSTR)pu.pszSuffix;
   }

   // Get default verb if available.

   if (GetPathDefaultVerb(pszCmdLine, szDefaultVerb, sizeof(szDefaultVerb)))
      pcszVerb = szDefaultVerb;
   else
      pcszVerb = NULL;

   TRACE_OUT(("FileProtocolHandler(): Invoking %s verb on %s.",
              pcszVerb ? pcszVerb : "open",
              pszCmdLine));

   hinstExec = ShellExecute(hwndParent, pcszVerb, pszCmdLine, NULL, NULL,
                            nShowCmd);

   if (hinstExec > (HINSTANCE)32)
      TRACE_OUT(("FileProtocolHandler(): ShellExecute() %s verb on %s succeeded.",
                 pcszVerb ? pcszVerb : "open",
                 pszCmdLine));
   else
   {
      WARNING_OUT(("FileProtocolHandler(): ShellExecute() %s verb on %s failed, returning %lu.",
                   pcszVerb ? pcszVerb : "open",
                   pszCmdLine,
                   hinstExec));

      if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                   MAKEINTRESOURCE(IDS_SHELLEXECUTE_FAILED),
                   (MB_OK | MB_ICONEXCLAMATION), &nResult, pszCmdLine)) {
         ASSERT(nResult == IDOK);
      }
   }

   DebugExitVOID(FileProtocolHandler);

   return;
}


extern "C" void WINAPI MailToProtocolHandler(HWND hwndParent, HINSTANCE hinst,
                                             PSTR pszCmdLine, int nShowCmd)
{
   int nResult;
   char szMAPIDLL[MAX_PATH_LEN];

   DebugEntry(MailToProtocolHandler);

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_HANDLE(hinst, INSTANCE));
   ASSERT(IS_VALID_STRING_PTR(pszCmdLine, STR));
   ASSERT(IsValidShowCmd(nShowCmd));

   if (GetProfileString(s_cszMAPISection, s_cszMAPIKey, EMPTY_STRING,
                        szMAPIDLL, sizeof(szMAPIDLL)) > 0)
   {
      HINSTANCE hinstMAPI;

      TRACE_OUT(("MailToProtocolHandler(): MAPI provider DLL is %s.",
                 szMAPIDLL));

      hinstMAPI = LoadLibrary(szMAPIDLL);

      if (hinstMAPI)
      {
         MAPISENDMAILPROC MAPISendMailProc;

         MAPISendMailProc = (MAPISENDMAILPROC)GetProcAddress(
                                                         hinstMAPI,
                                                         s_cszMAPISendMail);

         if (MAPISendMailProc)
         {
            MapiRecipDescA mapito;
            MapiMessage mapimsg;
            ULONG ulResult;

            // Assume the entire command line is a mailto: URL.

            TrimWhiteSpace(pszCmdLine);

            // Skip over any url: prefix.

            if (! lstrnicmp(pszCmdLine, g_cszURLPrefix, g_ucbURLPrefixLen))
               pszCmdLine += g_ucbURLPrefixLen;

            // Skip over any mailto: prefix.

            // (- 1) for null terminator.

            if (! lstrnicmp(pszCmdLine, s_cszMailToProtocolPrefix,
                            SIZECHARS(s_cszMailToProtocolPrefix) - 1))
               pszCmdLine += SIZECHARS(s_cszMailToProtocolPrefix) - 1;

            ZeroMemory(&mapito, sizeof(mapito));

            mapito.ulRecipClass = MAPI_TO;
            mapito.lpszName = pszCmdLine;

            ZeroMemory(&mapimsg, sizeof(mapimsg));

            mapimsg.nRecipCount = 1;
            mapimsg.lpRecips = &mapito;

            TRACE_OUT(("MailToProtocolHandler(): Trying to send mail to %s.",
                       mapito.lpszName));

            ulResult = (*MAPISendMailProc)(NULL, 0, &mapimsg,
                                           (MAPI_LOGON_UI | MAPI_DIALOG), 0);

            if (ulResult == SUCCESS_SUCCESS)
               TRACE_OUT(("MailToProtocolHandler(): MAPISendMail() to %s succeeded.",
                          pszCmdLine));
            else
            {
               if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                            MAKEINTRESOURCE(IDS_MAPI_MAPISENDMAIL_FAILED),
                            (MB_OK | MB_ICONEXCLAMATION), &nResult)) {
                  ASSERT(nResult == IDOK);
                }
            }
         }
         else
         {
            if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                         MAKEINTRESOURCE(IDS_MAPI_GETPROCADDRESS_FAILED),
                         (MB_OK | MB_ICONEXCLAMATION), &nResult,
                         szMAPIDLL, s_cszMAPISendMail)) {
               ASSERT(nResult == IDOK);
            }
         }

         EVAL(FreeLibrary(hinstMAPI));
      }
      else
      {
         if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                      MAKEINTRESOURCE(IDS_MAPI_LOADLIBRARY_FAILED),
                      (MB_OK | MB_ICONEXCLAMATION), &nResult, szMAPIDLL)) {
            ASSERT(nResult == IDOK);
         }
      }
   }
   else
   {
      if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                   MAKEINTRESOURCE(IDS_NO_MAPI_PROVIDER),
                   (MB_OK | MB_ICONEXCLAMATION), &nResult)) {
         ASSERT(nResult == IDOK);
      }
   }

   DebugExitVOID(MailToProtocolHandler);

   return;
}


extern "C" void WINAPI NewsProtocolHandler(HWND hwndParent, HINSTANCE hinst,
                                           PSTR pszCmdLine, int nShowCmd)
{
   int nResult;
   HINSTANCE hinstNews;

   DebugEntry(NewsProtocolHandler);

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_HANDLE(hinst, INSTANCE));
   ASSERT(IS_VALID_STRING_PTR(pszCmdLine, STR));
   ASSERT(IsValidShowCmd(nShowCmd));

#ifndef UNIX
   // Assume the entire command line is a news: URL.

   TrimWhiteSpace(pszCmdLine);

   // Skip over any url: prefix.

   if (! lstrnicmp(pszCmdLine, g_cszURLPrefix, g_ucbURLPrefixLen))
      pszCmdLine += g_ucbURLPrefixLen;

   hinstNews = LoadLibrary(s_cszNewsDLL);

   if (hinstNews)
   {
      RUNDLL32PROC RealNewsProtocolHandler;

      RealNewsProtocolHandler = (RUNDLL32PROC)GetProcAddress(hinstNews,
                                                             s_cszNewsProtocolHandler);

      if (RealNewsProtocolHandler)
      {
         TRACE_OUT(("NewsProtocolHandler(): Trying to open %s.",
                    pszCmdLine));

         (*RealNewsProtocolHandler)(hwndParent, hinst, pszCmdLine, nShowCmd);

         TRACE_OUT(("NewsProtocolHandler(): Returned from NewsProtocolHandler()."));
      }
      else
      {
         if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                      MAKEINTRESOURCE(IDS_NEWS_GETPROCADDRESS_FAILED),
                      (MB_OK | MB_ICONEXCLAMATION), &nResult)) {
            ASSERT(nResult == IDOK);
         }
      }

      EVAL(FreeLibrary(hinstNews));
   }
   else
   {
      if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                   MAKEINTRESOURCE(IDS_NEWS_LOADLIBRARY_FAILED),
                   (MB_OK | MB_ICONEXCLAMATION), &nResult)) {
         ASSERT(nResult == IDOK);
      }
   }
#else
   UnixReadNews(hwndParent, pszCmdLine);
#endif
   
   DebugExitVOID(NewsProtocolHandler);

   return;
}


extern "C" void WINAPI TelnetProtocolHandler(HWND hwndParent, HINSTANCE hinst,
                                             PSTR pszCmdLine, int nShowCmd)
{
   HRESULT hr;
   int nResult;
   char *p;

#ifdef UNIX
   char szApp[MAX_PATH];
   char szArgs[MAX_PATH];

#ifndef ANSI_SHELL32_ON_UNIX
  char szCmdLine[MAX_PATH];

  memset(szCmdLine, 0, MAX_PATH);

  SHUnicodeToAnsi((LPWSTR)pszCmdLine, szCmdLine, MAX_PATH);
  pszCmdLine = szCmdLine;
#endif /* ANSI_SHELL32_ON_UNIX */
#endif

   DebugEntry(TelnetProtocolHandler);

   ASSERT(IS_VALID_HANDLE(hwndParent, WND));
   ASSERT(IS_VALID_HANDLE(hinst, INSTANCE));
   ASSERT(IS_VALID_STRING_PTR(pszCmdLine, STR));
   ASSERT(IsValidShowCmd(nShowCmd));

   // Assume the entire command line is a telnet URL.

   TrimWhiteSpace(pszCmdLine);

   // Skip over any url: prefix.

   if (! lstrnicmp(pszCmdLine, g_cszURLPrefix, g_ucbURLPrefixLen))
      pszCmdLine += g_ucbURLPrefixLen;

   // Skip over any telnet:, rlogin:, or tn3270: prefix.

   // (- 1) for null terminator.

   if (! lstrnicmp(pszCmdLine, s_cszTelnetProtocolPrefix,
                   SIZECHARS(s_cszTelnetProtocolPrefix) - 1))
      pszCmdLine += SIZECHARS(s_cszTelnetProtocolPrefix) - 1;
   else if (! lstrnicmp(pszCmdLine, s_cszRLoginProtocolPrefix,
                        SIZECHARS(s_cszRLoginProtocolPrefix) - 1))
      pszCmdLine += SIZECHARS(s_cszRLoginProtocolPrefix) - 1;
   else if (! lstrnicmp(pszCmdLine, s_cszTN3270ProtocolPrefix,
                        SIZECHARS(s_cszTN3270ProtocolPrefix) - 1))
      pszCmdLine += SIZECHARS(s_cszTN3270ProtocolPrefix) - 1;

   // Remove leading and trailing slashes.

   TrimSlashes(pszCmdLine);

   // Skip user name if given

   p = StrChr(pszCmdLine, '@');

   if (p)
      pszCmdLine = p + 1;

   // If a port has been specified, turn ':' into space, which will make the
   // port become the second command line argument.

   p = StrChr(pszCmdLine, ':');

   if (p)
      *p = ' ';

   TRACE_OUT(("TelnetProtocolHandler(): Trying telnet to %s.",
              pszCmdLine));

#ifndef UNIX
   hr = MyExecute(s_cszTelnetApp, pszCmdLine, 0);
#else
   // On UNIX, telnet is not GUI application, so we need to create
   // separate xterm for it if we want to seet in in the separate window.
   strcpy(szApp, "xterm");
   wnsprintf(szArgs, sizeof(szArgs), "-e %s %s", s_cszTelnetApp, pszCmdLine);
   hr = MyExecute(szApp, szArgs, 0);
#endif

   switch (hr)
   {
      case S_OK:
         break;

      case E_FILE_NOT_FOUND:
         if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                      MAKEINTRESOURCE(IDS_TELNET_APP_NOT_FOUND),
                      (MB_OK | MB_ICONEXCLAMATION), &nResult,
                      s_cszTelnetApp)) {
            ASSERT(nResult == IDOK);
         }
         break;

      case E_OUTOFMEMORY:
         if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                      MAKEINTRESOURCE(IDS_OPEN_INTSHCUT_OUT_OF_MEMORY),
                      (MB_OK | MB_ICONEXCLAMATION), &nResult)) {
            ASSERT(nResult == IDOK);
         }
         break;

      default:
         ASSERT(hr == E_FAIL);
         if (MyMsgBox(hwndParent, MAKEINTRESOURCE(IDS_SHORTCUT_ERROR_TITLE),
                      MAKEINTRESOURCE(IDS_TELNET_EXEC_FAILED),
                      (MB_OK | MB_ICONEXCLAMATION), &nResult,
                      s_cszTelnetApp)) {
            ASSERT(nResult == IDOK);
         }
         break;
   }

   DebugExitVOID(TelnetProtocolHandler);

   return;
}


#ifdef UNIX
#define OE_NEWS_COMMAND_KEY TEXT("Software\\Clients\\News\\Outlook Express\\shell\\open\\command")
#define OE_URL_COMMAND_NAME TEXT("URLCommand")
#define IE_HOME_ENVIRONMENT TEXT("MWDEV")

HRESULT UnixLaunchOENews(PCSTR pszCmdLine)
{
    HRESULT     hr = S_OK;
    TCHAR       *tszCommand = NULL;
    TCHAR       tszIEHome[MAX_PATH];
    DWORD       cchIEHome;
    DWORD       cchCommand;
    DWORD       dwDisposition;
    TCHAR       *pchPos;
    BOOL        bMailed;
    STARTUPINFO stInfo;
    HKEY        hkey = NULL;
    int         i;
#ifndef ANSI_SHELL32_ON_UNIX
    char szCmdLine[MAX_PATH];

    memset(szCmdLine, 0, MAX_PATH);
   
    SHUnicodeToAnsi((LPWSTR)pszCmdLine, szCmdLine, MAX_PATH);
    pszCmdLine = szCmdLine;
#endif /* ANSI_SHELL32_ON_UNIX */

    cchIEHome = GetEnvironmentVariable(IE_HOME_ENVIRONMENT, tszIEHome, MAX_PATH);
    if (cchIEHome)
    {
    _tcscat(tszIEHome, TEXT("/bin"));
    }
    else
    {
    return E_FAIL;
    }

    hr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, OE_NEWS_COMMAND_KEY, 0, NULL, 0, KEY_READ, NULL, &hkey, &dwDisposition);
    if (hr != ERROR_SUCCESS) 
    {
        goto Cleanup;
    }

    hr = RegQueryValueEx(hkey, OE_URL_COMMAND_NAME, NULL, NULL, (LPBYTE)NULL, &cchCommand);
    if (hr != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    cchCommand += _tcslen(tszIEHome) + _tcslen(pszCmdLine) + 1;
    tszCommand = (TCHAR*)malloc((cchCommand)*sizeof(TCHAR));

    _tcscpy(tszCommand, tszIEHome);
    _tcscat(tszCommand, TEXT("/"));
    dwDisposition = _tcslen(tszCommand);

    hr = RegQueryValueEx(hkey, OE_URL_COMMAND_NAME, NULL, NULL, (LPBYTE)(&tszCommand[dwDisposition]), &cchCommand);
    if (hr != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    _tcscat(tszCommand, pszCmdLine);


    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.cb = sizeof(stInfo);
    bMailed = CreateProcess(NULL, tszCommand, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &stInfo, NULL);

Cleanup:
    if ( hkey != NULL )
        RegCloseKey(hkey);

    if (tszCommand)
        free(tszCommand);

    return hr;
}

HRESULT UnixReadNews(HWND hWndParent, PCSTR pszCmdLine)
{
    HRESULT         hr = S_FALSE;

    CHAR            szCommand[MAX_PATH];
    CHAR            szExpandedCommand[MAX_PATH];
    UINT            nCommandSize;
    int             i;
    HKEY    hkey;
    DWORD   dw;
    BOOL bMailed;
    STARTUPINFOA stInfo;

    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwUseOENews;

    hr = SHGetValue(IE_USE_OE_NEWS_HKEY, IE_USE_OE_NEWS_KEY, IE_USE_OE_NEWS_VALUE, 
            &dwType, (void*)&dwUseOENews, &dwSize);
    if ((hr) && (dwType != REG_DWORD))
    {
    // The default value for mail is FALSE
    dwUseOENews = FALSE;
    }
    if (dwUseOENews)
    {
    return UnixLaunchOENews(pszCmdLine);
    }

    hr = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_NEWSCLIENTS, 0, NULL, 0, KEY_READ, NULL, &hkey, &dw);
    if (hr != ERROR_SUCCESS)
        goto Cleanup;
    dw = MAX_PATH;
    hr = RegQueryValueEx(hkey, REGSTR_PATH_CURRENT, NULL, NULL, (LPBYTE)szCommand, &dw);
    if (hr != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        goto Cleanup;
    }

    if (strlen(szCommand) == 0)
      {
    MessageBox(hWndParent,
           TEXT("There is currently no default association for \"news:\" links in UNIX Internet Explorer.  If you have a favorite news reader that supports news specifications on the command line (id and group name), you can define an association for \"news:\" links for this program in the View->Internet Options, Programs Tab.\n"),
           TEXT("Microsoft Internet Explorer"),
           MB_OK|MB_ICONSTOP);
    return S_FALSE;
      }

    dw = ExpandEnvironmentStringsA(szCommand, szExpandedCommand, MAX_PATH);
    if (!dw)
     {
        strcpy(szExpandedCommand, szCommand);
     }

    strcpy(szCommand, szExpandedCommand);
    for (i = lstrlen(szCommand)-1; i >= 0; i--)
    if (szCommand[i] == '/')
    {
        szCommand[i] = '\0';
        break;
    }
    strcat(szCommand, " ");
    strcat(szCommand, (pszCmdLine + strlen("news:")));

    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.cb = sizeof(stInfo);
    bMailed = CreateProcessA(szExpandedCommand, szCommand, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &stInfo, NULL);
 
Cleanup:

    return S_OK;
}
#endif

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */

