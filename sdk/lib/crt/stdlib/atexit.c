/* taken from wine exit.c */
#include <precomp.h>

/*********************************************************************
 *		__dllonexit (MSVCRT.@)
 */
_onexit_t CDECL __dllonexit(_onexit_t func, _onexit_t **start, _onexit_t **end)
{
   _onexit_t *tmp;
   size_t len;

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

  tmp = realloc(*start, len * sizeof(_onexit_t));
  if (!tmp)
    return NULL;
  *start = tmp;
  *end = tmp + len;
  tmp[len - 1] = func;
  TRACE("new table start %p-%p, %d entries\n", *start, *end, len);
  return func;
}
