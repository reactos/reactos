/*
 * msvcrt.dll spawn/exec functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2007 Hans Leidekker
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
 *
 * FIXME:
 * -File handles need some special handling. Sometimes children get
 *  open file handles, sometimes not. The docs are confusing
 * -No check for maximum path/argument/environment size is done
 */

#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdarg.h>

#include "msvcrt.h"
#include <winnls.h>
#include "mtdll.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static void msvcrt_search_executable(const wchar_t *name, wchar_t *fullname, int use_path)
{
  static const wchar_t suffix[][5] =
    {L".com", L".exe", L".bat", L".cmd"};

  wchar_t buffer[MAX_PATH];
  const wchar_t *env, *p, *end;
  unsigned int i, name_len, path_len;
  int extension = 1;

  *fullname = '\0';
  msvcrt_set_errno(ERROR_FILE_NOT_FOUND);

  end = name + MAX_PATH - 1;
  for(p = name; p < end; p++)
      if(!*p) break;
  name_len = p - name;

  /* FIXME extra-long names are silently truncated */
  memcpy(buffer, name, name_len * sizeof(wchar_t));
  buffer[name_len] = '\0';

  /* try current dir first */
  if (GetFileAttributesW(buffer) != INVALID_FILE_ATTRIBUTES)
  {
    wcscpy(fullname, buffer);
    return;
  }

  for (p--; p >= name; p--)
    if (*p == '\\' || *p == '/' || *p == ':' || *p == '.') break;

  /* if there's no extension, try some well-known extensions */
  if ((p < name || *p != '.') && name_len <= MAX_PATH - 5)
  {
    for (i = 0; i < 4; i++)
    {
      memcpy(buffer + name_len, suffix[i], 5 * sizeof(wchar_t));
      if (GetFileAttributesW(buffer) != INVALID_FILE_ATTRIBUTES)
      {
        wcscpy(fullname, buffer);
        return;
      }
    }
    extension = 0;
  }

  if (!use_path || !(env = _wgetenv(L"PATH"))) return;

  /* now try search path */
  do
  {
    p = env;
    while (*p && *p != ';') p++;
    if (p == env) return;

    path_len = p - env;
    if (path_len + name_len <= MAX_PATH - 2)
    {
      memcpy(buffer, env, path_len * sizeof(wchar_t));
      if (buffer[path_len] != '/' && buffer[path_len] != '\\')
      {
        buffer[path_len++] = '\\';
        buffer[path_len] = '\0';
      }
      else buffer[path_len] = '\0';

      wcscat(buffer, name);
      if (GetFileAttributesW(buffer) != INVALID_FILE_ATTRIBUTES)
      {
        wcscpy(fullname, buffer);
        return;
      }
    }
    /* again, if there's no extension, try some well-known extensions */
    if (!extension && path_len + name_len <= MAX_PATH - 5)
    {
      for (i = 0; i < 4; i++)
      {
        memcpy(buffer + path_len + name_len, suffix[i], 5 * sizeof(wchar_t));
        if (GetFileAttributesW(buffer) != INVALID_FILE_ATTRIBUTES)
        {
          wcscpy(fullname, buffer);
          return;
        }
      }
    }
    env = *p ? p + 1 : p;
  } while(1);
}

static intptr_t msvcrt_spawn(int flags, const wchar_t* exe, wchar_t* cmdline,
                                    wchar_t* env, int use_path)
{
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  wchar_t fullname[MAX_PATH];
  DWORD create_flags = CREATE_UNICODE_ENVIRONMENT;

  TRACE("%x %s %s %s %d\n", flags, debugstr_w(exe), debugstr_w(cmdline), debugstr_w(env), use_path);

  if ((unsigned)flags > _P_DETACH)
  {
    *_errno() = EINVAL;
    return -1;
  }

  msvcrt_search_executable(exe, fullname, use_path);

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  msvcrt_create_io_inherit_block(&si.cbReserved2, &si.lpReserved2);
  if (flags == _P_DETACH) create_flags |= DETACHED_PROCESS;
  if (!CreateProcessW(fullname, cmdline, NULL, NULL, TRUE,
                      create_flags, env, NULL, &si, &pi))
  {
    msvcrt_set_errno(GetLastError());
    free(si.lpReserved2);
    return -1;
  }

  free(si.lpReserved2);
  switch(flags)
  {
  case _P_WAIT:
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess,&pi.dwProcessId);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return pi.dwProcessId;
  case _P_DETACH:
    CloseHandle(pi.hProcess);
    pi.hProcess = 0;
    /* fall through */
  case _P_NOWAIT:
  case _P_NOWAITO:
    CloseHandle(pi.hThread);
    return (intptr_t)pi.hProcess;
  case  _P_OVERLAY:
    _exit(0);
  }
  return -1; /* can't reach here */
}

/* INTERNAL: Convert wide argv list to a single 'delim'-separated wide string, with an
 * extra '\0' to terminate it.
 */
static wchar_t* msvcrt_argvtos(const wchar_t* const* arg, wchar_t delim)
{
  const wchar_t* const* a;
  int size;
  wchar_t* p;
  wchar_t* ret;

  if (!arg)
  {
      /* Return NULL for an empty environment list */
      return NULL;
  }

  /* get length */
  a = arg;
  size = 0;
  while (*a)
  {
    size += wcslen(*a) + 1;
    a++;
  }

  ret = malloc((size + 1) * sizeof(wchar_t));
  if (!ret)
    return NULL;

  /* fill string */
  a = arg;
  p = ret;
  while (*a)
  {
    int len = wcslen(*a);
    memcpy(p,*a,len * sizeof(wchar_t));
    p += len;
    *p++ = delim;
    a++;
  }
  if (delim && p > ret) p[-1] = 0;
  else *p = 0;
  return ret;
}

/* INTERNAL: Convert ansi argv list to a single wide string, use ' ' or '\0'
 * as separator depending on env argument.
 */
static wchar_t *msvcrt_argvtos_aw(const char * const *arg, BOOL env)
{
  UINT cp = env ? CP_ACP : get_aw_cp();
  const char * const *a;
  unsigned int len;
  wchar_t *p, *ret;

  if (!arg)
  {
      /* Return NULL for an empty environment list */
      return NULL;
  }

  /* get length */
  a = arg;
  len = 0;
  while (*a)
  {
    len += MultiByteToWideChar(cp, 0, *a, -1, NULL, 0);
    a++;
  }

  ret = malloc((len + 1) * sizeof(wchar_t));
  if (!ret)
    return NULL;

  /* fill string */
  a = arg;
  p = ret;
  while (*a)
  {
    p += MultiByteToWideChar(cp, 0, *a, strlen(*a), p, len - (p - ret));
    *p++ = env ? 0 : ' ';
    a++;
  }
  if (!env && p > ret) p[-1] = 0;
  else *p = 0;
  return ret;
}

/* INTERNAL: Convert wide va_list to a single 'delim'-separated wide string, with an
 * extra '\0' to terminate it.
 */
static wchar_t *msvcrt_valisttos(const wchar_t *arg0, va_list alist, wchar_t delim)
{
    unsigned int size = 0, pos = 0;
    const wchar_t *arg;
    wchar_t *new, *ret = NULL;

    for (arg = arg0; arg; arg = va_arg( alist, wchar_t * ))
    {
        unsigned int len = wcslen( arg ) + 1;
        if (pos + len >= size)
        {
            size = max( 256, size * 2 );
            size = max( size, pos + len + 1 );
            if (!(new = realloc( ret, size * sizeof(wchar_t) )))
            {
                free( ret );
                return NULL;
            }
            ret = new;
        }
        wcscpy( ret + pos, arg );
        pos += len;
        ret[pos - 1] = delim;
    }
    if (pos)
    {
        if (delim) ret[pos - 1] = 0;
        else ret[pos] = 0;
    }
    return ret;
}

/* INTERNAL: Convert ansi va_list to a single 'delim'-separated wide string, with an
 * extra '\0' to terminate it.
 */
static wchar_t *msvcrt_valisttos_aw(const char *arg0, va_list alist, wchar_t delim)
{
    unsigned int size = 0, pos = 0;
    const char *arg;
    wchar_t *new, *ret = NULL;

    for (arg = arg0; arg; arg = va_arg( alist, char * ))
    {
        unsigned int len = convert_acp_utf8_to_wcs( arg, NULL, 0 );
        if (pos + len >= size)
        {
            size = max( 256, size * 2 );
            size = max( size, pos + len + 1 );
            if (!(new = realloc( ret, size * sizeof(wchar_t) )))
            {
                free( ret );
                return NULL;
            }
            ret = new;
        }
        pos += convert_acp_utf8_to_wcs( arg, ret + pos, size - pos );
        ret[pos - 1] = delim;
    }
    if (pos)
    {
        if (delim) ret[pos - 1] = 0;
        else ret[pos] = 0;
    }
    return ret;
}

/* INTERNAL: retrieve COMSPEC environment variable */
static wchar_t *msvcrt_get_comspec(void)
{
  wchar_t *ret;
  unsigned int len;

  if (!(len = GetEnvironmentVariableW(L"COMSPEC", NULL, 0))) len = 4;
  if ((ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(wchar_t))))
  {
    if (!GetEnvironmentVariableW(L"COMSPEC", ret, len)) wcscpy(ret, L"cmd");
  }
  return ret;
}

/*********************************************************************
 *		_cwait (MSVCRT.@)
 */
intptr_t CDECL _cwait(int *status, intptr_t pid, int action)
{
  HANDLE hPid = (HANDLE)pid;
  int doserrno;

  if (!WaitForSingleObject(hPid, INFINITE))
  {
    if (status)
    {
      DWORD stat;
      GetExitCodeProcess(hPid, &stat);
      *status = (int)stat;
    }
    return pid;
  }
  doserrno = GetLastError();

  if (doserrno == ERROR_INVALID_HANDLE)
  {
    *_errno() =  ECHILD;
    *__doserrno() = doserrno;
  }
  else
    msvcrt_set_errno(doserrno);

  return status ? *status = -1 : -1;
}

/*********************************************************************
 *      _wexecl (MSVCRT.@)
 *
 * Unicode version of _execl
 */
intptr_t WINAPIV _wexecl(const wchar_t* name, const wchar_t* arg0, ...)
{
  va_list ap;
  wchar_t *args;
  intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, name, args, NULL, 0);

  free(args);
  return ret;
}

/*********************************************************************
 *		_execl (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t WINAPIV _execl(const char* name, const char* arg0, ...)
{
  va_list ap;
  wchar_t *nameW, *args;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, nameW, args, NULL, 0);

  free(nameW);
  free(args);
  return ret;
}

/*********************************************************************
 *      _wexecle (MSVCRT.@)
 *
 * Unicode version of _execle
 */
intptr_t WINAPIV _wexecle(const wchar_t* name, const wchar_t* arg0, ...)
{
  va_list ap;
  wchar_t *args, *envs = NULL;
  const wchar_t * const *envp;
  intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, name, args, envs, 0);

  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *		_execle (MSVCRT.@)
 */
intptr_t WINAPIV _execle(const char* name, const char* arg0, ...)
{
  va_list ap;
  wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, TRUE);
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, nameW, args, envs, 0);

  free(nameW);
  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *      _wexeclp (MSVCRT.@)
 *
 * Unicode version of _execlp
 */
intptr_t WINAPIV _wexeclp(const wchar_t* name, const wchar_t* arg0, ...)
{
  va_list ap;
  wchar_t *args;
  intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, name, args, NULL, 1);

  free(args);
  return ret;
}

/*********************************************************************
 *		_execlp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t WINAPIV _execlp(const char* name, const char* arg0, ...)
{
  va_list ap;
  wchar_t *nameW, *args;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, nameW, args, NULL, 1);

  free(nameW);
  free(args);
  return ret;
}

/*********************************************************************
 *      _wexeclpe (MSVCRT.@)
 *
 * Unicode version of _execlpe
 */
intptr_t WINAPIV _wexeclpe(const wchar_t* name, const wchar_t* arg0, ...)
{
  va_list ap;
  wchar_t *args, *envs = NULL;
  const wchar_t * const *envp;
  intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, name, args, envs, 1);

  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *		_execlpe (MSVCRT.@)
 */
intptr_t WINAPIV _execlpe(const char* name, const char* arg0, ...)
{
  va_list ap;
  wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, TRUE);
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, nameW, args, envs, 1);

  free(nameW);
  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *      _wexecv (MSVCRT.@)
 *
 * Unicode version of _execv
 */
intptr_t CDECL _wexecv(const wchar_t* name, const wchar_t* const* argv)
{
  return _wspawnve(_P_OVERLAY, name, argv, NULL);
}

/*********************************************************************
 *		_execv (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t CDECL _execv(const char* name, const char* const* argv)
{
  return _spawnve(_P_OVERLAY, name, argv, NULL);
}

/*********************************************************************
 *      _wexecve (MSVCRT.@)
 *
 * Unicode version of _execve
 */
intptr_t CDECL _wexecve(const wchar_t* name, const wchar_t* const* argv, const wchar_t* const* envv)
{
  return _wspawnve(_P_OVERLAY, name, argv, envv);
}

/*********************************************************************
 *		_execve (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t CDECL _execve(const char* name, const char* const* argv, const char* const* envv)
{
  return _spawnve(_P_OVERLAY, name, argv, envv);
}

/*********************************************************************
 *      _wexecvpe (MSVCRT.@)
 *
 * Unicode version of _execvpe
 */
intptr_t CDECL _wexecvpe(const wchar_t* name, const wchar_t* const* argv, const wchar_t* const* envv)
{
  return _wspawnvpe(_P_OVERLAY, name, argv, envv);
}

/*********************************************************************
 *		_execvpe (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t CDECL _execvpe(const char* name, const char* const* argv, const char* const* envv)
{
  return _spawnvpe(_P_OVERLAY, name, argv, envv);
}

/*********************************************************************
 *      _wexecvp (MSVCRT.@)
 *
 * Unicode version of _execvp
 */
intptr_t CDECL _wexecvp(const wchar_t* name, const wchar_t* const* argv)
{
  return _wexecvpe(name, argv, NULL);
}

/*********************************************************************
 *		_execvp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t CDECL _execvp(const char* name, const char* const* argv)
{
  return _execvpe(name, argv, NULL);
}

/*********************************************************************
 *      _wspawnl (MSVCRT.@)
 *
 * Unicode version of _spawnl
 */
intptr_t WINAPIV _wspawnl(int flags, const wchar_t* name, const wchar_t* arg0, ...)
{
  va_list ap;
  wchar_t *args;
  intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, name, args, NULL, 0);

  free(args);
  return ret;
}

/*********************************************************************
 *		_spawnl (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t WINAPIV _spawnl(int flags, const char* name, const char* arg0, ...)
{
  va_list ap;
  wchar_t *nameW, *args;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, NULL, 0);

  free(nameW);
  free(args);
  return ret;
}

/*********************************************************************
 *      _wspawnle (MSVCRT.@)
 *
 * Unicode version of _spawnle
 */
intptr_t WINAPIV _wspawnle(int flags, const wchar_t* name, const wchar_t* arg0, ...)
{
  va_list ap;
  wchar_t *args, *envs = NULL;
  const wchar_t * const *envp;
  intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(flags, name, args, envs, 0);

  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnle (MSVCRT.@)
 */
intptr_t WINAPIV _spawnle(int flags, const char* name, const char* arg0, ...)
{
  va_list ap;
  wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, TRUE);
  va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, envs, 0);

  free(nameW);
  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *      _wspawnlp (MSVCRT.@)
 *
 * Unicode version of _spawnlp
 */
intptr_t WINAPIV _wspawnlp(int flags, const wchar_t* name, const wchar_t* arg0, ...)
{
  va_list ap;
  wchar_t *args;
  intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, name, args, NULL, 1);

  free(args);
  return ret;
}

/*********************************************************************
 *		_spawnlp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t WINAPIV _spawnlp(int flags, const char* name, const char* arg0, ...)
{
  va_list ap;
  wchar_t *nameW, *args;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, NULL, 1);

  free(nameW);
  free(args);
  return ret;
}

/*********************************************************************
 *      _wspawnlpe (MSVCRT.@)
 *
 * Unicode version of _spawnlpe
 */
intptr_t WINAPIV _wspawnlpe(int flags, const wchar_t* name, const wchar_t* arg0, ...)
{
  va_list ap;
  wchar_t *args, *envs = NULL;
  const wchar_t * const *envp;
  intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(flags, name, args, envs, 1);

  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnlpe (MSVCRT.@)
 */
intptr_t WINAPIV _spawnlpe(int flags, const char* name, const char* arg0, ...)
{
  va_list ap;
  wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, TRUE);
  va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, envs, 1);

  free(nameW);
  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnve (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t CDECL _spawnve(int flags, const char* name, const char* const* argv,
                               const char* const* envv)
{
  wchar_t *nameW, *args, *envs;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  args = msvcrt_argvtos_aw(argv, FALSE);
  envs = msvcrt_argvtos_aw(envv, TRUE);

  ret = msvcrt_spawn(flags, nameW, args, envs, 0);

  free(nameW);
  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *      _wspawnve (MSVCRT.@)
 *
 * Unicode version of _spawnve
 */
intptr_t CDECL _wspawnve(int flags, const wchar_t* name, const wchar_t* const* argv,
                                const wchar_t* const* envv)
{
  wchar_t *args, *envs;
  intptr_t ret;

  args = msvcrt_argvtos(argv, ' ');
  envs = msvcrt_argvtos(envv, 0);

  ret = msvcrt_spawn(flags, name, args, envs, 0);

  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnv (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t CDECL _spawnv(int flags, const char* name, const char* const* argv)
{
  return _spawnve(flags, name, argv, NULL);
}

/*********************************************************************
 *      _wspawnv (MSVCRT.@)
 *
 * Unicode version of _spawnv
 */
intptr_t CDECL _wspawnv(int flags, const wchar_t* name, const wchar_t* const* argv)
{
  return _wspawnve(flags, name, argv, NULL);
}

/*********************************************************************
 *		_spawnvpe (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t CDECL _spawnvpe(int flags, const char* name, const char* const* argv,
                                const char* const* envv)
{
  wchar_t *nameW, *args, *envs;
  intptr_t ret;

  if (!(nameW = wstrdupa_utf8(name))) return -1;

  args = msvcrt_argvtos_aw(argv, FALSE);
  envs = msvcrt_argvtos_aw(envv, TRUE);

  ret = msvcrt_spawn(flags, nameW, args, envs, 1);

  free(nameW);
  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *      _wspawnvpe (MSVCRT.@)
 *
 * Unicode version of _spawnvpe
 */
intptr_t CDECL _wspawnvpe(int flags, const wchar_t* name, const wchar_t* const* argv,
                                 const wchar_t* const* envv)
{
  wchar_t *args, *envs;
  intptr_t ret;

  args = msvcrt_argvtos(argv, ' ');
  envs = msvcrt_argvtos(envv, 0);

  ret = msvcrt_spawn(flags, name, args, envs, 1);

  free(args);
  free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnvp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
intptr_t CDECL _spawnvp(int flags, const char* name, const char* const* argv)
{
  return _spawnvpe(flags, name, argv, NULL);
}

/*********************************************************************
 *      _wspawnvp (MSVCRT.@)
 *
 * Unicode version of _spawnvp
 */
intptr_t CDECL _wspawnvp(int flags, const wchar_t* name, const wchar_t* const* argv)
{
  return _wspawnvpe(flags, name, argv, NULL);
}

static struct popen_handle {
    FILE *f;
    HANDLE proc;
} *popen_handles;
static DWORD popen_handles_size;

void msvcrt_free_popen_data(void)
{
    free(popen_handles);
}

/*********************************************************************
 *		_wpopen (MSVCRT.@)
 *
 * Unicode version of _popen
 */
FILE* CDECL _wpopen(const wchar_t* command, const wchar_t* mode)
{
    wchar_t *comspec, *fullcmd, fullname[MAX_PATH];
    int textmode, fds[2], fd_parent, fd_child;
    struct popen_handle *container;
    PROCESS_INFORMATION pi;
    BOOL read_pipe = TRUE;
    const wchar_t *p;
    unsigned int len;
    STARTUPINFOW si;
    FILE *ret;
    DWORD i;
    BOOL r;

    TRACE("(command=%s, mode=%s)\n", debugstr_w(command), debugstr_w(mode));

    if (!command || !mode)
        return NULL;

    _get_fmode(&textmode);
    textmode &= _O_BINARY | _O_TEXT;
    textmode |= _O_NOINHERIT;
    for (p = mode; *p; p++)
    {
        switch (*p)
        {
        case 'W':
        case 'w':
            read_pipe = FALSE;
            break;
        case 'B':
        case 'b':
            textmode |= _O_BINARY;
            textmode &= ~_O_TEXT;
            break;
        case 'T':
        case 't':
            textmode |= _O_TEXT;
            textmode &= ~_O_BINARY;
            break;
        }
    }
    if (_pipe(fds, 0, textmode) == -1)
        return NULL;

    if (read_pipe)
    {
        fd_parent = fds[0];
        fd_child = _dup(fds[1]);
        _close(fds[1]);
    }
    else
    {
        fd_parent = fds[1];
        fd_child = _dup(fds[0]);
        _close(fds[0]);
    }
    if (fd_child == -1)
    {
        _close(fd_parent);
        return NULL;
    }
    ret = _wfdopen(fd_parent, mode);
    if (!ret)
    {
        _close(fd_child);
        return NULL;
    }

    _lock(_POPEN_LOCK);
    for (i = 0; i < popen_handles_size; i++)
    {
        if (!popen_handles[i].f)
            break;
    }
    if (i == popen_handles_size)
    {
        i = (popen_handles_size ? popen_handles_size * 2 : 8);
        container = realloc(popen_handles, i * sizeof(*container));
        if (!container) goto error;

        popen_handles = container;
        container = popen_handles + popen_handles_size;
        memset(container, 0, (i - popen_handles_size) * sizeof(*container));
        popen_handles_size = i;
    }
    else container = popen_handles + i;

    if (!(comspec = msvcrt_get_comspec())) goto error;
    len = wcslen(comspec) + wcslen(command) + 5;

    if (!(fullcmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(wchar_t))))
    {
        HeapFree(GetProcessHeap(), 0, comspec);
        goto error;
    }

    wcscpy(fullcmd, comspec);
    wcscat(fullcmd, L" /c ");
    wcscat(fullcmd, command);
    msvcrt_search_executable(comspec, fullname, 1);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    if (read_pipe)
    {
        si.hStdInput = (HANDLE)_get_osfhandle(STDIN_FILENO);
        si.hStdOutput = (HANDLE)_get_osfhandle(fd_child);
    }
    else
    {
        si.hStdInput = (HANDLE)_get_osfhandle(fd_child);
        si.hStdOutput = (HANDLE)_get_osfhandle(STDOUT_FILENO);
    }
    si.hStdError = (HANDLE)_get_osfhandle(STDERR_FILENO);
    r = CreateProcessW(fullname, fullcmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    HeapFree(GetProcessHeap(), 0, comspec);
    HeapFree(GetProcessHeap(), 0, fullcmd);
    if (!r)
    {
        msvcrt_set_errno(GetLastError());
        goto error;
    }
    CloseHandle(pi.hThread);
    _close(fd_child);
    container->proc = pi.hProcess;
    container->f = ret;
    _unlock(_POPEN_LOCK);
    return ret;

error:
    _unlock(_POPEN_LOCK);
    _close(fd_child);
    fclose(ret);
    return NULL;
}

/*********************************************************************
 *      _popen (MSVCRT.@)
 */
FILE* CDECL _popen(const char* command, const char* mode)
{
  FILE *ret;
  wchar_t *cmdW, *modeW;

  TRACE("(command=%s, mode=%s)\n", debugstr_a(command), debugstr_a(mode));

  if (!command || !mode)
    return NULL;

  if (!(cmdW = wstrdupa_utf8(command))) return NULL;
  if (!(modeW = msvcrt_wstrdupa(mode)))
  {
    free(cmdW);
    return NULL;
  }

  ret = _wpopen(cmdW, modeW);

  free(cmdW);
  free(modeW);
  return ret;
}

/*********************************************************************
 *		_pclose (MSVCRT.@)
 */
int CDECL _pclose(FILE* file)
{
  HANDLE h;
  DWORD i;

  if (!MSVCRT_CHECK_PMT(file != NULL)) return -1;

  _lock(_POPEN_LOCK);
  for(i=0; i<popen_handles_size; i++)
  {
    if (popen_handles[i].f == file)
      break;
  }
  if(i == popen_handles_size)
  {
    _unlock(_POPEN_LOCK);
    *_errno() = EBADF;
    return -1;
  }

  h = popen_handles[i].proc;
  popen_handles[i].f = NULL;
  _unlock(_POPEN_LOCK);

  fclose(file);
  if(WaitForSingleObject(h, INFINITE)==WAIT_FAILED || !GetExitCodeProcess(h, &i))
  {
    msvcrt_set_errno(GetLastError());
    CloseHandle(h);
    return -1;
  }

  CloseHandle(h);
  return i;
}

/*********************************************************************
 *      _wsystem (MSVCRT.@)
 *
 * Unicode version of system
 */
int CDECL _wsystem(const wchar_t* cmd)
{
  int res;
  wchar_t *comspec, *fullcmd;
  unsigned int len;

  comspec = msvcrt_get_comspec();

  if (cmd == NULL)
  {
    if (comspec == NULL)
    {
        *_errno() = ENOENT;
        return 0;
    }
    HeapFree(GetProcessHeap(), 0, comspec);
    return 1;
  }

  if (comspec == NULL)
    return -1;

  len = wcslen(comspec) + wcslen(cmd) + 5;

  if (!(fullcmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(wchar_t))))
  {
    HeapFree(GetProcessHeap(), 0, comspec);
    return -1;
  }
  wcscpy(fullcmd, comspec);
  wcscat(fullcmd, L" /c ");
  wcscat(fullcmd, cmd);

  res = msvcrt_spawn(_P_WAIT, comspec, fullcmd, NULL, 1);

  HeapFree(GetProcessHeap(), 0, comspec);
  HeapFree(GetProcessHeap(), 0, fullcmd);
  return res;
}

/*********************************************************************
 *		system (MSVCRT.@)
 */
int CDECL system(const char* cmd)
{
  int res = -1;
  wchar_t *cmdW;

  if (cmd == NULL)
    return _wsystem(NULL);

  if ((cmdW = wstrdupa_utf8(cmd)))
  {
    res = _wsystem(cmdW);
    free(cmdW);
  }
  return res;
}

/*********************************************************************
 *		_loaddll (MSVCRT.@)
 */
intptr_t CDECL _loaddll(const char* dllname)
{
    wchar_t *dllnameW = NULL;
    intptr_t ret;

    if (dllname && !(dllnameW = wstrdupa_utf8(dllname))) return 0;
    ret = (intptr_t)LoadLibraryW(dllnameW);
    free(dllnameW);
    return ret;
}

/*********************************************************************
 *		_unloaddll (MSVCRT.@)
 */
int CDECL _unloaddll(intptr_t dll)
{
  if (FreeLibrary((HMODULE)dll))
    return 0;
  else
  {
    int err = GetLastError();
    msvcrt_set_errno(err);
    return err;
  }
}

/*********************************************************************
 *		_getdllprocaddr (MSVCRT.@)
 */
void * CDECL _getdllprocaddr(intptr_t dll, const char *name, int ordinal)
{
    if (name)
    {
        if (ordinal != -1) return NULL;
        return GetProcAddress( (HMODULE)dll, name );
    }
    if (HIWORD(ordinal)) return NULL;
    return GetProcAddress( (HMODULE)dll, (LPCSTR)(ULONG_PTR)ordinal );
}

/*********************************************************************
 *              _getpid (MSVCRT.@)
 */
int CDECL _getpid(void)
{
    return GetCurrentProcessId();
}

#if _MSVCR_VER>=110
/*********************************************************************
 *  __crtTerminateProcess (MSVCR110.@)
 */
int CDECL __crtTerminateProcess(UINT exit_code)
{
    return TerminateProcess(GetCurrentProcess(), exit_code);
}
#endif
