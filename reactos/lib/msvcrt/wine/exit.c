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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include "msvcrt.h"

#include "msvcrt/conio.h"
#include "msvcrt/stdlib.h"
#include "mtdll.h"
#include "winuser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* MT */
#define LOCK_EXIT   _mlock(_EXIT_LOCK1)
#define UNLOCK_EXIT _munlock(_EXIT_LOCK1)

static _onexit_t *MSVCRT_atexit_table = NULL;
static int MSVCRT_atexit_table_size = 0;
static int MSVCRT_atexit_registered = 0; /* Points to free slot */

static LPCSTR szMsgBoxTitle = "Wine C++ Runtime Library";

extern int MSVCRT_app_type;
extern char *MSVCRT__pgmptr;

/* INTERNAL: call atexit functions */
void __MSVCRT__call_atexit(void)
{
  /* Note: should only be called with the exit lock held */
  TRACE("%d atext functions to call\n", MSVCRT_atexit_registered);
  /* Last registered gets executed first */
  while (MSVCRT_atexit_registered > 0)
  {
    MSVCRT_atexit_registered--;
    TRACE("next is %p\n",MSVCRT_atexit_table[MSVCRT_atexit_registered]);
    if (MSVCRT_atexit_table[MSVCRT_atexit_registered])
      (*MSVCRT_atexit_table[MSVCRT_atexit_registered])();
    TRACE("returned\n");
  }
}

/*********************************************************************
 *		__dllonexit (MSVCRT.@)
 */
_onexit_t __dllonexit(_onexit_t func, _onexit_t **start, _onexit_t **end)
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

  tmp = (_onexit_t *)MSVCRT_realloc(*start, len * sizeof(tmp));
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
void MSVCRT__exit(int exitcode)
{
  TRACE("(%d)\n", exitcode);
  ExitProcess(exitcode);
}

/* Print out an error message with an option to debug */
static void DoMessageBox(LPCSTR lead, LPCSTR message)
{
  MSGBOXPARAMSA msgbox;
  char text[2048];
  INT ret;

  snprintf(text,sizeof(text),"%s\n\nProgram: %s\n%s\n\n"
               "Press OK to exit the program, or Cancel to start the Wine debugger.\n ",
               lead, MSVCRT__pgmptr, message);

  msgbox.cbSize = sizeof(msgbox);
  msgbox.hwndOwner = GetActiveWindow();
  msgbox.hInstance = 0;
  msgbox.lpszText = text;
  msgbox.lpszCaption = szMsgBoxTitle;
  msgbox.dwStyle = MB_OKCANCEL|MB_ICONERROR;
  msgbox.lpszIcon = NULL;
  msgbox.dwContextHelpId = 0;
  msgbox.lpfnMsgBoxCallback = NULL;
  msgbox.dwLanguageId = LANG_NEUTRAL;

  ret = MessageBoxIndirectA(&msgbox);
  if (ret == IDCANCEL)
    DebugBreak();
}

/*********************************************************************
 *		_amsg_exit (MSVCRT.@)
 */
void MSVCRT__amsg_exit(int errnum)
{
  TRACE("(%d)\n", errnum);
  /* FIXME: text for the error number. */
  if (MSVCRT_app_type == 2)
  {
    char text[32];
    sprintf(text, "Error: R60%d",errnum);
    DoMessageBox("Runtime error!", text);
  }
  else
    _cprintf("\nruntime error R60%d\n",errnum);
  MSVCRT__exit(255);
}

/*********************************************************************
 *		abort (MSVCRT.@)
 */
void MSVCRT_abort(void)
{
  TRACE("()\n");
  if (MSVCRT_app_type == 2)
  {
    DoMessageBox("Runtime error!", "abnormal program termination");
  }
  else
    _cputs("\nabnormal program termination\n");
  MSVCRT__exit(3);
}

/*********************************************************************
 *		_assert (MSVCRT.@)
 */
void MSVCRT__assert(const char* str, const char* file, unsigned int line)
{
  TRACE("(%s,%s,%d)\n",str,file,line);
  if (MSVCRT_app_type == 2)
  {
    char text[2048];
    snprintf(text, sizeof(text), "File: %s\nLine: %d\n\nEpression: \"%s\"", file, line, str);
    DoMessageBox("Assertion failed!", text);
  }
  else
    _cprintf("Assertion failed: %s, file %s, line %d\n\n",str, file, line);
  MSVCRT__exit(3);
}

/*********************************************************************
 *		_c_exit (MSVCRT.@)
 */
void MSVCRT__c_exit(void)
{
  TRACE("(void)\n");
  /* All cleanup is done on DLL detach; Return to caller */
}

/*********************************************************************
 *		_cexit (MSVCRT.@)
 */
void MSVCRT__cexit(void)
{
  TRACE("(void)\n");
  /* All cleanup is done on DLL detach; Return to caller */
}

/*********************************************************************
 *		_onexit (MSVCRT.@)
 */
_onexit_t _onexit(_onexit_t func)
{
  TRACE("(%p)\n",func);

  if (!func)
    return NULL;

  LOCK_EXIT;
  if (MSVCRT_atexit_registered > MSVCRT_atexit_table_size - 1)
  {
    _onexit_t *newtable;
    TRACE("expanding table\n");
    newtable = MSVCRT_calloc(sizeof(void *),MSVCRT_atexit_table_size + 32);
    if (!newtable)
    {
      TRACE("failed!\n");
      UNLOCK_EXIT;
      return NULL;
    }
    memcpy (newtable, MSVCRT_atexit_table, MSVCRT_atexit_table_size);
    MSVCRT_atexit_table_size += 32;
    if (MSVCRT_atexit_table)
      MSVCRT_free (MSVCRT_atexit_table);
    MSVCRT_atexit_table = newtable;
  }
  MSVCRT_atexit_table[MSVCRT_atexit_registered] = func;
  MSVCRT_atexit_registered++;
  UNLOCK_EXIT;
  return func;
}

/*********************************************************************
 *		exit (MSVCRT.@)
 */
void MSVCRT_exit(int exitcode)
{
  TRACE("(%d)\n",exitcode);
  LOCK_EXIT;
  __MSVCRT__call_atexit();
  UNLOCK_EXIT;
  ExitProcess(exitcode);
}

/*********************************************************************
 *		atexit (MSVCRT.@)
 */
int MSVCRT_atexit(void (*func)(void))
{
  TRACE("(%p)\n", func);
  return _onexit((_onexit_t)func) == (_onexit_t)func ? 0 : -1;
}


/*********************************************************************
 *		_purecall (MSVCRT.@)
 */
void _purecall(void)
{
  TRACE("(void)\n");
  MSVCRT__amsg_exit( 25 );
}
