/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#include <process.h>


#define scan_ptr() \
	const char **ptr; \
	union { const char **ccpp; const char *ccp; } u; \
	for (ptr = &argv0; *ptr; ptr++); \
	u.ccp = *++ptr; \
	ptr = u.ccpp;

int _execlpe(const char *path, const char *argv0, ... /*, const char **envp */)
{
  scan_ptr();
  return _spawnvpe(P_OVERLAY, path, (const char * const *)&argv0, (const char * const *)ptr);
}
