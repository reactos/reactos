/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/stdlib/abort.c
 * PURPOSE:     Abnormal termination message
 * PROGRAMER:   Jon Griffiths
 *              Samuel Serapión
 */

/* based on wine exit.c */

#include <precomp.h>
#include <signal.h>
#include <internal/wine/msvcrt.h>

extern int msvcrt_error_mode;
extern int __app_type;
unsigned int msvcrt_abort_behavior =  MSVCRT__WRITE_ABORT_MSG | MSVCRT__CALL_REPORTFAULT;

/* avoid linking to user32 */
typedef HWND (WINAPI *GetActiveWindowPtr)(void);
static GetActiveWindowPtr pGetActiveWindow = NULL;
typedef int (WINAPI *MessageBoxIndirectWPtr)(const MSGBOXPARAMSW*);
static MessageBoxIndirectWPtr pMessageBoxIndirectW = NULL;

static void DoMessageBoxW(const wchar_t *lead, const wchar_t *message)
{
  const char szMsgBoxTitle[] = "ReactOS C++ Runtime Library";
  const wchar_t message_format[] = {'%','s','\n','\n','P','r','o','g','r','a','m',':',' ','%','s','\n',
    '%','s','\n','\n','P','r','e','s','s',' ','O','K',' ','t','o',' ','e','x','i','t',' ','t','h','e',' ',
    'p','r','o','g','r','a','m',',',' ','o','r',' ','C','a','n','c','e','l',' ','t','o',' ','s','t','a','r','t',' ',
    't','h','e',' ','d','e','b','b','u','g','e','r','.','\n',0};

  MSGBOXPARAMSW msgbox;
  wchar_t text[2048];
  INT ret;

  _snwprintf(text,sizeof(text),message_format, lead, _wpgmptr, message);

  msgbox.cbSize = sizeof(msgbox);
  msgbox.hwndOwner = pGetActiveWindow();
  msgbox.hInstance = 0;
  msgbox.lpszText = (LPCWSTR)text;
  msgbox.lpszCaption = (LPCWSTR)szMsgBoxTitle;
  msgbox.dwStyle = MB_OKCANCEL|MB_ICONERROR;
  msgbox.lpszIcon = NULL;
  msgbox.dwContextHelpId = 0;
  msgbox.lpfnMsgBoxCallback = NULL;
  msgbox.dwLanguageId = LANG_NEUTRAL;

  ret = pMessageBoxIndirectW(&msgbox);
  if (ret == IDCANCEL)
    DebugBreak();
}

static void DoMessageBox(const char *lead, const char *message)
{
  wchar_t leadW[1024], messageW[1024];
  HMODULE huser32 = LoadLibrary("user32.dll");

  if(huser32) {
      pGetActiveWindow = (GetActiveWindowPtr)GetProcAddress(huser32, "GetActiveWindow");
      pMessageBoxIndirectW = (MessageBoxIndirectWPtr)GetProcAddress(huser32, "MessageBoxIndirectW");

      if(!pGetActiveWindow || !pMessageBoxIndirectW) {
          FreeLibrary(huser32);
          ERR("GetProcAddress failed!\n");
          return;
      }
  }
  else
  {
      ERR("Loading user32 failed!\n");
      return;
  }

  mbstowcs(leadW, lead, 1024);
  mbstowcs(messageW, message, 1024);

  DoMessageBoxW(leadW, messageW);
  FreeLibrary(huser32); 
}

/*
 * @implemented
 */
void abort()
{
  if (msvcrt_abort_behavior & MSVCRT__WRITE_ABORT_MSG)
  {
    if ((msvcrt_error_mode == MSVCRT__OUT_TO_MSGBOX) ||
       ((msvcrt_error_mode == MSVCRT__OUT_TO_DEFAULT) && (__app_type == 2)))
    {
      DoMessageBox("Runtime error!", "abnormal program termination");
    }
    else
      _cputs("\nabnormal program termination\n");
  }
  raise(SIGABRT);
  /* in case raise() returns */
  _exit(3);
}

unsigned int CDECL _set_abort_behavior(unsigned int flags, unsigned int mask)
{
  unsigned int old = msvcrt_abort_behavior;

  TRACE("%x, %x\n", flags, mask);
  if (mask & MSVCRT__CALL_REPORTFAULT)
    FIXME("_WRITE_CALL_REPORTFAULT unhandled\n");

  msvcrt_abort_behavior = (msvcrt_abort_behavior & ~mask) | (flags & mask);
  return old;
}
