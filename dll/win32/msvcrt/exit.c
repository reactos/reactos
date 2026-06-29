/*
 * msvcrt.dll exit functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <conio.h>
#include <process.h>
#include <signal.h>
#include <stdio.h>
#include "msvcrt.h"
#include "mtdll.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* MT */
#define LOCK_EXIT   _lock(_EXIT_LOCK1)
#define UNLOCK_EXIT _unlock(_EXIT_LOCK1)

static _purecall_handler purecall_handler = NULL;

static _onexit_table_t MSVCRT_atexit_table;

typedef void (__stdcall *_tls_callback_type)(void*,ULONG,void*);
static _tls_callback_type tls_atexit_callback;

static CRITICAL_SECTION MSVCRT_onexit_cs;
static CRITICAL_SECTION_DEBUG MSVCRT_onexit_cs_debug =
{
    0, 0, &MSVCRT_onexit_cs,
    { &MSVCRT_onexit_cs_debug.ProcessLocksList, &MSVCRT_onexit_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": MSVCRT_onexit_cs") }
};
static CRITICAL_SECTION MSVCRT_onexit_cs = { &MSVCRT_onexit_cs_debug, -1, 0, 0, 0, 0 };

extern int MSVCRT_app_type;
extern wchar_t *MSVCRT__wpgmptr;

#if _MSVCR_VER > 0 || defined(_DEBUG)
static unsigned int MSVCRT_abort_behavior =  _WRITE_ABORT_MSG | _CALL_REPORTFAULT;
#endif

static int MSVCRT_error_mode = _OUT_TO_DEFAULT;

void (*CDECL _aexit_rtn)(int) = _exit;

static int initialize_onexit_table(_onexit_table_t *table)
{
    if (!table)
        return -1;

    if (table->_first == table->_end)
        table->_last = table->_end = table->_first = NULL;
    return 0;
}

static int register_onexit_function(_onexit_table_t *table, _onexit_t func)
{
    if (!table)
        return -1;

    EnterCriticalSection(&MSVCRT_onexit_cs);
    if (!table->_first)
    {
        table->_first = calloc(32, sizeof(void *));
        if (!table->_first)
        {
            WARN("failed to allocate initial table.\n");
            LeaveCriticalSection(&MSVCRT_onexit_cs);
            return -1;
        }
        table->_last = table->_first;
        table->_end = table->_first + 32;
    }

    /* grow if full */
    if (table->_last == table->_end)
    {
        int len = table->_end - table->_first;
        _PVFV *tmp = realloc(table->_first, 2 * len * sizeof(void *));
        if (!tmp)
        {
            WARN("failed to grow table.\n");
            LeaveCriticalSection(&MSVCRT_onexit_cs);
            return -1;
        }
        table->_first = tmp;
        table->_end = table->_first + 2 * len;
        table->_last = table->_first + len;
    }

    *table->_last = (_PVFV)func;
    table->_last++;
    LeaveCriticalSection(&MSVCRT_onexit_cs);
    return 0;
}

static int execute_onexit_table(_onexit_table_t *table)
{
    _onexit_table_t copy;
    _PVFV *func;

    if (!table)
        return -1;

    EnterCriticalSection(&MSVCRT_onexit_cs);
    if (!table->_first || table->_first >= table->_last)
    {
        LeaveCriticalSection(&MSVCRT_onexit_cs);
        return 0;
    }
    copy._first = table->_first;
    copy._last  = table->_last;
    copy._end   = table->_end;
    memset(table, 0, sizeof(*table));
    initialize_onexit_table(table);
    LeaveCriticalSection(&MSVCRT_onexit_cs);

    for (func = copy._last - 1; func >= copy._first; func--)
    {
        if (*func)
           (*func)();
    }

    free(copy._first);
    return 0;
}

static void call_atexit(void)
{
    /* Note: should only be called with the exit lock held */
    if (tls_atexit_callback) tls_atexit_callback(NULL, DLL_PROCESS_DETACH, NULL);
    execute_onexit_table(&MSVCRT_atexit_table);
}

/*********************************************************************
 *		__dllonexit (MSVCRT.@)
 */
_onexit_t CDECL __dllonexit(_onexit_t func, _onexit_t **start, _onexit_t **end)
{
  _onexit_t *tmp;
  int len;

  TRACE("(%p,%p,%p)\n", func, start, end);

  if (!start || !*start || !end || !*end)
  {
   FIXME("bad table\n");
   return NULL;
  }

  len = (*end - *start);

  TRACE("table start %p-%p, %d entries\n", *start, *end, len);

  if (++len <= 0)
    return NULL;

  tmp = realloc(*start, len * sizeof(*tmp));
  if (!tmp)
    return NULL;
  *start = tmp;
  *end = tmp + len;
  tmp[len - 1] = func;
  TRACE("new table start %p-%p, %d entries\n", *start, *end, len);
  return func;
}

/*********************************************************************
 *		_exit (MSVCRT.@)
 */
void CDECL _exit(int exitcode)
{
  TRACE("(%d)\n", exitcode);
  ExitProcess(exitcode);
}

/* Print out an error message with an option to debug */
static void DoMessageBoxW(const wchar_t *lead, const wchar_t *message)
{
  MSGBOXPARAMSW msgbox;
  wchar_t text[2048];
  INT ret;

  _snwprintf(text, ARRAY_SIZE(text), L"%ls\n\nProgram: %ls\n%ls\n\n"
          L"Press OK to exit the program, or Cancel to start the Wine debugger.\n",
          lead, MSVCRT__wpgmptr, message);

  msgbox.cbSize = sizeof(msgbox);
  msgbox.hwndOwner = GetActiveWindow();
  msgbox.hInstance = 0;
  msgbox.lpszText = text;
  msgbox.lpszCaption = L"Wine C++ Runtime Library";
  msgbox.dwStyle = MB_OKCANCEL|MB_ICONERROR;
  msgbox.lpszIcon = NULL;
  msgbox.dwContextHelpId = 0;
  msgbox.lpfnMsgBoxCallback = NULL;
  msgbox.dwLanguageId = LANG_NEUTRAL;

  ret = MessageBoxIndirectW(&msgbox);
  if (ret == IDCANCEL)
    DebugBreak();
}

static void DoMessageBox(const char *lead, const char *message)
{
  wchar_t leadW[1024], messageW[1024];

  mbstowcs(leadW, lead, 1024);
  mbstowcs(messageW, message, 1024);

  DoMessageBoxW(leadW, messageW);
}

/*********************************************************************
 *		_amsg_exit (MSVCRT.@)
 */
void CDECL _amsg_exit(int errnum)
{
  TRACE("(%d)\n", errnum);

  if ((MSVCRT_error_mode == _OUT_TO_MSGBOX) ||
     ((MSVCRT_error_mode == _OUT_TO_DEFAULT) && (MSVCRT_app_type == 2)))
  {
    char text[32];
    sprintf(text, "Error: R60%d",errnum);
    DoMessageBox("Runtime error!", text);
  }
  else
    _cprintf("\nruntime error R60%d\n",errnum);
  _aexit_rtn(255);
}

/*********************************************************************
 *		abort (MSVCRT.@)
 */
void CDECL abort(void)
{
  TRACE("()\n");

#if (_MSVCR_VER > 0 && _MSVCR_VER < 100) || _MSVCR_VER == 120 || defined(_DEBUG)
  if (MSVCRT_abort_behavior & _WRITE_ABORT_MSG)
  {
    if ((MSVCRT_error_mode == _OUT_TO_MSGBOX) ||
       ((MSVCRT_error_mode == _OUT_TO_DEFAULT) && (MSVCRT_app_type == 2)))
    {
      DoMessageBox("Runtime error!", "abnormal program termination");
    }
    else
      _cputs("\nabnormal program termination\n");
  }
#endif
  raise(SIGABRT);
  /* in case raise() returns */
  _exit(3);
}

#if _MSVCR_VER>=80
/*********************************************************************
 *		_set_abort_behavior (MSVCR80.@)
 */
unsigned int CDECL _set_abort_behavior(unsigned int flags, unsigned int mask)
{
  unsigned int old = MSVCRT_abort_behavior;

  TRACE("%x, %x\n", flags, mask);
  if (mask & _CALL_REPORTFAULT)
    FIXME("_WRITE_CALL_REPORTFAULT unhandled\n");

  MSVCRT_abort_behavior = (MSVCRT_abort_behavior & ~mask) | (flags & mask);
  return old;
}
#endif

/*********************************************************************
 *              _wassert (MSVCRT.@)
 */
void DECLSPEC_NORETURN CDECL _wassert(const wchar_t* str, const wchar_t* file, unsigned int line)
{
  ERR("(%s,%s,%d)\n", debugstr_w(str), debugstr_w(file), line);

  if ((MSVCRT_error_mode == _OUT_TO_MSGBOX) ||
     ((MSVCRT_error_mode == _OUT_TO_DEFAULT) && (MSVCRT_app_type == 2)))
  {
    wchar_t text[2048];
    _snwprintf(text, sizeof(text), L"File: %ls\nLine: %d\n\nExpression: \"%ls\"", file, line, str);
    DoMessageBoxW(L"Assertion failed!", text);
  }
  else
    fwprintf(stderr, L"Assertion failed: %ls, file %ls, line %d\n\n", str, file, line);

  raise(SIGABRT);
  _exit(3);
}

/*********************************************************************
 *		_assert (MSVCRT.@)
 */
void DECLSPEC_NORETURN CDECL _assert(const char* str, const char* file, unsigned int line)
{
    wchar_t strW[1024], fileW[1024];

    mbstowcs(strW, str, 1024);
    mbstowcs(fileW, file, 1024);

    _wassert(strW, fileW, line);
}

/*********************************************************************
 *		_c_exit (MSVCRT.@)
 */
void CDECL _c_exit(void)
{
  TRACE("(void)\n");
  /* All cleanup is done on DLL detach; Return to caller */
}

/*********************************************************************
 *		_cexit (MSVCRT.@)
 */
void CDECL _cexit(void)
{
  TRACE("(void)\n");
  LOCK_EXIT;
  call_atexit();
  UNLOCK_EXIT;
}

/*********************************************************************
 *		_onexit (MSVCRT.@)
 */
_onexit_t CDECL _onexit(_onexit_t func)
{
  TRACE("(%p)\n",func);

  if (!func)
    return NULL;

  LOCK_EXIT;
  register_onexit_function(&MSVCRT_atexit_table, func);
  UNLOCK_EXIT;

  return func;
}

/*********************************************************************
 *		exit (MSVCRT.@)
 */
void CDECL exit(int exitcode)
{
  HMODULE hmscoree;
  void (WINAPI *pCorExitProcess)(int);

  TRACE("(%d)\n",exitcode);
  _cexit();

  hmscoree = GetModuleHandleW(L"mscoree");

  if (hmscoree)
  {
    pCorExitProcess = (void*)GetProcAddress(hmscoree, "CorExitProcess");

    if (pCorExitProcess)
      pCorExitProcess(exitcode);
  }

  ExitProcess(exitcode);
}

/*********************************************************************
 *		atexit (MSVCRT.@)
 */
int CDECL MSVCRT_atexit(void (__cdecl *func)(void))
{
  TRACE("(%p)\n", func);
  return _onexit((_onexit_t)func) == (_onexit_t)func ? 0 : -1;
}

#if _MSVCR_VER >= 140
static _onexit_table_t MSVCRT_quick_exit_table;

/*********************************************************************
 *             _crt_at_quick_exit (UCRTBASE.@)
 */
int CDECL _crt_at_quick_exit(void (__cdecl *func)(void))
{
  TRACE("(%p)\n", func);
  return register_onexit_function(&MSVCRT_quick_exit_table, (_onexit_t)func);
}

/*********************************************************************
 *             quick_exit (UCRTBASE.@)
 */
void CDECL quick_exit(int exitcode)
{
  TRACE("(%d)\n", exitcode);

  execute_onexit_table(&MSVCRT_quick_exit_table);
  _exit(exitcode);
}

/*********************************************************************
 *		_crt_atexit (UCRTBASE.@)
 */
int CDECL _crt_atexit(void (__cdecl *func)(void))
{
  TRACE("(%p)\n", func);
  return _onexit((_onexit_t)func) == (_onexit_t)func ? 0 : -1;
}

/*********************************************************************
 *		_initialize_onexit_table (UCRTBASE.@)
 */
int CDECL _initialize_onexit_table(_onexit_table_t *table)
{
    TRACE("(%p)\n", table);

    return initialize_onexit_table(table);
}

/*********************************************************************
 *		_register_onexit_function (UCRTBASE.@)
 */
int CDECL _register_onexit_function(_onexit_table_t *table, _onexit_t func)
{
    TRACE("(%p %p)\n", table, func);

    return register_onexit_function(table, func);
}

/*********************************************************************
 *		_execute_onexit_table (UCRTBASE.@)
 */
int CDECL _execute_onexit_table(_onexit_table_t *table)
{
    TRACE("(%p)\n", table);

    return execute_onexit_table(table);
}
#endif

/*********************************************************************
 *		_register_thread_local_exe_atexit_callback (UCRTBASE.@)
 */
void CDECL _register_thread_local_exe_atexit_callback(_tls_callback_type callback)
{
    TRACE("(%p)\n", callback);
    tls_atexit_callback = callback;
}

#if _MSVCR_VER>=71
/*********************************************************************
 *		_set_purecall_handler (MSVCR71.@)
 */
_purecall_handler CDECL _set_purecall_handler(_purecall_handler function)
{
    _purecall_handler ret = purecall_handler;

    TRACE("(%p)\n", function);
    purecall_handler = function;
    return ret;
}
#endif

#if _MSVCR_VER>=80
/*********************************************************************
 *		_get_purecall_handler (MSVCR80.@)
 */
_purecall_handler CDECL _get_purecall_handler(void)
{
    TRACE("\n");
    return purecall_handler;
}
#endif

/*********************************************************************
 *		_purecall (MSVCRT.@)
 */
void CDECL _purecall(void)
{
  TRACE("(void)\n");

  if(purecall_handler)
      purecall_handler();
  _amsg_exit( 25 );
}

/******************************************************************************
 *		_set_error_mode (MSVCRT.@)
 *
 * Set the error mode, which describes where the C run-time writes error messages.
 *
 * PARAMS
 *   mode - the new error mode
 *
 * RETURNS
 *   The old error mode.
 *
 */
int CDECL _set_error_mode(int mode)
{

  const int old = MSVCRT_error_mode;
  if ( _REPORT_ERRMODE != mode ) {
    MSVCRT_error_mode = mode;
  }
  return old;
}
