/* taken from wine exit.c */
#include <precomp.h>

_onexit_t *atexit_table = NULL;
int atexit_table_size = 0;
int atexit_registered = 0; /* Points to free slot */

/* INTERNAL: call atexit functions */
void __call_atexit(void)
{
  /* Note: should only be called with the exit lock held */
  TRACE("%d atext functions to call\n", atexit_registered);
  /* Last registered gets executed first */
  while (atexit_registered > 0)
  {
    atexit_registered--;
    TRACE("next is %p\n",atexit_table[atexit_registered]);
    if (atexit_table[atexit_registered])
      (*atexit_table[atexit_registered])();
    TRACE("returned\n");
  }
}

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

/*********************************************************************
 *		_onexit (MSVCRT.@)
 */
_onexit_t CDECL _onexit(_onexit_t func)
{
  TRACE("(%p)\n",func);

  if (!func)
    return NULL;

  LOCK_EXIT;
  if (atexit_registered > atexit_table_size - 1)
  {
    _onexit_t *newtable;
    TRACE("expanding table\n");
    newtable = calloc(sizeof(void *),atexit_table_size + 32);
    if (!newtable)
    {
      TRACE("failed!\n");
      UNLOCK_EXIT;
      return NULL;
    }
    memcpy (newtable, atexit_table, atexit_table_size);
    atexit_table_size += 32;
    free (atexit_table);
    atexit_table = newtable;
  }
  atexit_table[atexit_registered] = func;
  atexit_registered++;
  UNLOCK_EXIT;
  return func;
}

/*********************************************************************
 *		atexit (MSVCRT.@)
 */
int CDECL atexit(void (*func)(void))
{
  TRACE("(%p)\n", func);
  return _onexit((_onexit_t)func) == (_onexit_t)func ? 0 : -1;
}

