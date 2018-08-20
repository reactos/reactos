/* Imported from msvcrt/console.c */

#include <precomp.h>

/*********************************************************************
 *		_cputs (MSVCRT.@)
 */
int CDECL _cputs(const char* str)
{
  DWORD count;
  int len, retval = -1;
#ifdef __REACTOS__ /* r54651 */
  HANDLE MSVCRT_console_out = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

  if (!MSVCRT_CHECK_PMT(str != NULL)) return -1;
  len = (int)strlen(str);

#ifndef __REACTOS__ /* r54651 */
  LOCK_CONSOLE;
#endif
  if (WriteConsoleA(MSVCRT_console_out, str, len, &count, NULL)
      && count == len)
    retval = 0;
#ifndef __REACTOS__ /* r54651 */
  UNLOCK_CONSOLE;
#endif
  return retval;
}
