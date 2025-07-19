/*
 * msvcrt.dll environment functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
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
#include "msvcrt.h"
#include "mtdll.h"
#include <winnls.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

int env_init(BOOL unicode, BOOL modif)
{
    if (!unicode && !MSVCRT___initenv)
    {
        char *environ_strings = GetEnvironmentStringsA();
        int count = 1, len = 1, i = 0; /* keep space for the trailing NULLS */
        char *ptr;

        for (ptr = environ_strings; *ptr; ptr += strlen(ptr) + 1)
        {
            /* Don't count environment variables starting with '=' which are command shell specific */
            if (*ptr != '=') count++;
            len += strlen(ptr) + 1;
        }
        MSVCRT___initenv = malloc(count * sizeof(*MSVCRT___initenv) + len);
        if (!MSVCRT___initenv)
        {
            FreeEnvironmentStringsA(environ_strings);
            return -1;
        }

        memcpy(&MSVCRT___initenv[count], environ_strings, len);
        for (ptr = (char *)&MSVCRT___initenv[count]; *ptr; ptr += strlen(ptr) + 1)
        {
            /* Skip special environment strings set by the command shell */
            if (*ptr != '=') MSVCRT___initenv[i++] = ptr;
        }
        MSVCRT___initenv[i] = NULL;
        FreeEnvironmentStringsA(environ_strings);

        MSVCRT__environ = MSVCRT___initenv;
    }

    if (!unicode && modif && MSVCRT__environ == MSVCRT___initenv)
    {
        int i = 0;

        while(MSVCRT___initenv[i]) i++;
        MSVCRT__environ = malloc((i + 1) * sizeof(char *));
        if (!MSVCRT__environ) return -1;
        for (i = 0; MSVCRT___initenv[i]; i++)
            MSVCRT__environ[i] = strdup(MSVCRT___initenv[i]);
        MSVCRT__environ[i] = NULL;
    }

    if (unicode && !MSVCRT___winitenv)
    {
        wchar_t *wenviron_strings = GetEnvironmentStringsW();
        int count = 1, len = 1, i = 0; /* keep space for the trailing NULLS */
        wchar_t *wptr;

        for (wptr = wenviron_strings; *wptr; wptr += wcslen(wptr) + 1)
        {
            /* Don't count environment variables starting with '=' which are command shell specific */
            if (*wptr != '=') count++;
            len += wcslen(wptr) + 1;
        }
        MSVCRT___winitenv = malloc(count * sizeof(*MSVCRT___winitenv) + len * sizeof(wchar_t));
        if (!MSVCRT___winitenv)
        {
            FreeEnvironmentStringsW(wenviron_strings);
            return -1;
        }

        memcpy(&MSVCRT___winitenv[count], wenviron_strings, len * sizeof(wchar_t));
        for (wptr = (wchar_t *)&MSVCRT___winitenv[count]; *wptr; wptr += wcslen(wptr) + 1)
        {
            /* Skip special environment strings set by the command shell */
            if (*wptr != '=') MSVCRT___winitenv[i++] = wptr;
        }
        MSVCRT___winitenv[i] = NULL;
        FreeEnvironmentStringsW(wenviron_strings);

        MSVCRT__wenviron = MSVCRT___winitenv;
    }

    if (unicode && modif && MSVCRT__wenviron == MSVCRT___winitenv)
    {
        int i = 0;

        while(MSVCRT___winitenv[i]) i++;
        MSVCRT__wenviron = malloc((i + 1) * sizeof(wchar_t *));
        if (!MSVCRT__wenviron) return -1;
        for (i = 0; MSVCRT___winitenv[i]; i++)
            MSVCRT__wenviron[i] = wcsdup(MSVCRT___winitenv[i]);
        MSVCRT__wenviron[i] = NULL;
    }

    return 0;
}

static int env_get_index(const char *name)
{
    int i, len;

    len = strlen(name);
    for (i = 0; MSVCRT__environ[i]; i++)
    {
        if (!strnicmp(name, MSVCRT__environ[i], len) && MSVCRT__environ[i][len] == '=')
            return i;
    }
    return i;
}

static int wenv_get_index(const wchar_t *name)
{
    int i, len;

    len = wcslen(name);
    for (i = 0; MSVCRT__wenviron[i]; i++)
    {
        if (!wcsnicmp(name, MSVCRT__wenviron[i], len) && MSVCRT__wenviron[i][len] == '=')
            return i;
    }
    return i;
}

static int env_set(char **env, wchar_t **wenv)
{
    wchar_t *weq = wcschr(*wenv, '=');
    char *eq = strchr(*env, '=');
    int idx;

    *weq = 0;
    if (!SetEnvironmentVariableW(*wenv, weq[1] ? weq + 1 : NULL) &&
            GetLastError() != ERROR_ENVVAR_NOT_FOUND)
        return -1;

    if (env_init(FALSE, TRUE)) return -1;
    *eq = 0;
    idx = env_get_index(*env);
    *eq = '=';
    if (!eq[1])
    {
        free(MSVCRT__environ[idx]);
        for(; MSVCRT__environ[idx]; idx++)
            MSVCRT__environ[idx] = MSVCRT__environ[idx + 1];
    }
    else if (MSVCRT__environ[idx])
    {
        free(MSVCRT__environ[idx]);
        MSVCRT__environ[idx] = *env;
        *env = NULL;
    }
    else
    {
        char **new_env = realloc(MSVCRT__environ, (idx + 2) * sizeof(*MSVCRT__environ));
        if (!new_env) return -1;
        MSVCRT__environ = new_env;
        MSVCRT__environ[idx] = *env;
        MSVCRT__environ[idx + 1] = NULL;
        *env = NULL;
    }

    if (!MSVCRT__wenviron) return 0;
    if (MSVCRT__wenviron == MSVCRT___winitenv)
        if (env_init(TRUE, TRUE)) return -1;
    idx = wenv_get_index(*wenv);
    *weq = '=';
    if (!weq[1])
    {
        free(MSVCRT__wenviron[idx]);
        for(; MSVCRT__wenviron[idx]; idx++)
            MSVCRT__wenviron[idx] = MSVCRT__wenviron[idx + 1];
    }
    else if (MSVCRT__wenviron[idx])
    {
        free(MSVCRT__wenviron[idx]);
        MSVCRT__wenviron[idx] = *wenv;
        *wenv = NULL;
    }
    else
    {
        wchar_t **new_env = realloc(MSVCRT__wenviron, (idx + 2) * sizeof(*MSVCRT__wenviron));
        if (!new_env) return -1;
        MSVCRT__wenviron = new_env;
        MSVCRT__wenviron[idx] = *wenv;
        MSVCRT__wenviron[idx + 1] = NULL;
        *wenv = NULL;
    }
    return 0;
}

static char * getenv_helper(const char *name)
{
    int idx;

    if (!name) return NULL;

    idx = env_get_index(name);
    if (!MSVCRT__environ[idx]) return NULL;
    return strchr(MSVCRT__environ[idx], '=') + 1;
}

/*********************************************************************
 *              getenv (MSVCRT.@)
 */
char * CDECL getenv(const char *name)
{
    char *ret;

    if (!MSVCRT_CHECK_PMT(name != NULL)) return NULL;

    _lock(_ENV_LOCK);
    ret = getenv_helper(name);
    _unlock(_ENV_LOCK);
    return ret;
}

static wchar_t * wgetenv_helper(const wchar_t *name)
{
    int idx;

    if (!name) return NULL;
    if (env_init(TRUE, FALSE)) return NULL;

    idx = wenv_get_index(name);
    if (!MSVCRT__wenviron[idx]) return NULL;
    return wcschr(MSVCRT__wenviron[idx], '=') + 1;
}

/*********************************************************************
 *              _wgetenv (MSVCRT.@)
 */
wchar_t * CDECL _wgetenv(const wchar_t *name)
{
    wchar_t *ret;

    if (!MSVCRT_CHECK_PMT(name != NULL)) return NULL;

    _lock(_ENV_LOCK);
    ret = wgetenv_helper(name);
    _unlock(_ENV_LOCK);
    return ret;
}

static int putenv_helper(const char *name, const char *val, const char *eq)
{
    wchar_t *wenv;
    char *env;
    int r;

    if (eq)
    {
        env = strdup(name);
        if (!env) return -1;
    }
    else
    {
        int name_len = strlen(name);

        r = strlen(val);
        env = malloc(name_len + r + 2);
        if (!env) return -1;
        memcpy(env, name, name_len);
        env[name_len] = '=';
        strcpy(env + name_len + 1, val);
    }

    wenv = msvcrt_wstrdupa(env);
    if (!wenv)
    {
        free(env);
        return -1;
    }

    _lock(_ENV_LOCK);
    r = env_set(&env, &wenv);
    _unlock(_ENV_LOCK);
    free(env);
    free(wenv);
    return r;
}

static char *msvcrt_astrdupw(const wchar_t *wstr)
{
    const unsigned int len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    char *str = malloc(len * sizeof(char));

    if (!str)
        return NULL;
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
    return str;
}


static int wputenv_helper(const wchar_t *name, const wchar_t *val, const wchar_t *eq)
{
    wchar_t *wenv;
    char *env;
    int r;

    _lock(_ENV_LOCK);
    r = env_init(TRUE, TRUE);
    _unlock(_ENV_LOCK);
    if (r) return -1;

    if (eq)
    {
        wenv = wcsdup(name);
        if (!wenv) return -1;
    }
    else
    {
        int name_len = wcslen(name);

        r = wcslen(val);
        wenv = malloc((name_len + r + 2) * sizeof(wchar_t));
        if (!wenv) return -1;
        memcpy(wenv, name, name_len * sizeof(wchar_t));
        wenv[name_len] = '=';
        wcscpy(wenv + name_len + 1, val);
    }

    env = msvcrt_astrdupw(wenv);
    if (!env)
    {
        free(wenv);
        return -1;
    }

    _lock(_ENV_LOCK);
    r = env_set(&env, &wenv);
    _unlock(_ENV_LOCK);
    free(env);
    free(wenv);
    return r;
}

/*********************************************************************
 *		_putenv (MSVCRT.@)
 */
int CDECL _putenv(const char *str)
{
    const char *eq;

    TRACE("%s\n", debugstr_a(str));

    if (!str || !(eq = strchr(str, '=')))
        return -1;

    return putenv_helper(str, NULL, eq);
}

/*********************************************************************
 *		_wputenv (MSVCRT.@)
 */
int CDECL _wputenv(const wchar_t *str)
{
    const wchar_t *eq;

    TRACE("%s\n", debugstr_w(str));

    if (!str || !(eq = wcschr(str, '=')))
        return -1;

    return wputenv_helper(str, NULL, eq);
}

/*********************************************************************
 *		_putenv_s (MSVCRT.@)
 */
errno_t CDECL _putenv_s(const char *name, const char *value)
{
    errno_t ret = 0;

    TRACE("%s %s\n", debugstr_a(name), debugstr_a(value));

    if (!MSVCRT_CHECK_PMT(name != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(value != NULL)) return EINVAL;

    if (putenv_helper(name, value, NULL) < 0)
    {
        msvcrt_set_errno(GetLastError());
        ret = *_errno();
    }

    return ret;
}

/*********************************************************************
 *		_wputenv_s (MSVCRT.@)
 */
errno_t CDECL _wputenv_s(const wchar_t *name, const wchar_t *value)
{
    errno_t ret = 0;

    TRACE("%s %s\n", debugstr_w(name), debugstr_w(value));

    if (!MSVCRT_CHECK_PMT(name != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(value != NULL)) return EINVAL;

    if (wputenv_helper(name, value, NULL) < 0)
    {
        msvcrt_set_errno(GetLastError());
        ret = *_errno();
    }

    return ret;
}

#if _MSVCR_VER>=80

/******************************************************************
 *		_dupenv_s (MSVCR80.@)
 */
int CDECL _dupenv_s(char **buffer, size_t *numberOfElements, const char *varname)
{
    char *e;
    size_t sz;

    if (!MSVCRT_CHECK_PMT(buffer != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(varname != NULL)) return EINVAL;

    _lock(_ENV_LOCK);
    if (!(e = getenv(varname)))
    {
        _unlock(_ENV_LOCK);
        *buffer = NULL;
        if (numberOfElements) *numberOfElements = 0;
        return 0;
    }

    sz = strlen(e) + 1;
    *buffer = malloc(sz);
    if (*buffer) strcpy(*buffer, e);
    _unlock(_ENV_LOCK);

    if (!*buffer)
    {
        if (numberOfElements) *numberOfElements = 0;
        return *_errno() = ENOMEM;
    }
    if (numberOfElements) *numberOfElements = sz;
    return 0;
}

/******************************************************************
 *		_wdupenv_s (MSVCR80.@)
 */
int CDECL _wdupenv_s(wchar_t **buffer, size_t *numberOfElements,
                     const wchar_t *varname)
{
    wchar_t *e;
    size_t sz;

    if (!MSVCRT_CHECK_PMT(buffer != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(varname != NULL)) return EINVAL;

    _lock(_ENV_LOCK);
    if (!(e = _wgetenv(varname)))
    {
        _unlock(_ENV_LOCK);
        *buffer = NULL;
        if (numberOfElements) *numberOfElements = 0;
        return 0;
    }

    sz = wcslen(e) + 1;
    *buffer = malloc(sz * sizeof(wchar_t));
    if (*buffer) wcscpy(*buffer, e);
    _unlock(_ENV_LOCK);

    if (!*buffer)
    {
        if (numberOfElements) *numberOfElements = 0;
        return *_errno() = ENOMEM;
    }
    if (numberOfElements) *numberOfElements = sz;
    return 0;
}

#endif /* _MSVCR_VER>=80 */

/******************************************************************
 *		getenv_s (MSVCRT.@)
 */
int CDECL getenv_s(size_t *ret_len, char* buffer, size_t len, const char *varname)
{
    char *e;

    if (!MSVCRT_CHECK_PMT(ret_len != NULL)) return EINVAL;
    *ret_len = 0;
    if (!MSVCRT_CHECK_PMT((buffer && len > 0) || (!buffer && !len))) return EINVAL;
    if (buffer) buffer[0] = 0;

    _lock(_ENV_LOCK);
    e = getenv_helper(varname);
    if (e)
    {
        *ret_len = strlen(e) + 1;
        if (len >= *ret_len) strcpy(buffer, e);
    }
    _unlock(_ENV_LOCK);

    if (!e || !len) return 0;
    if (len < *ret_len) return ERANGE;
    return 0;
}

/******************************************************************
 *		_wgetenv_s (MSVCRT.@)
 */
int CDECL _wgetenv_s(size_t *ret_len, wchar_t *buffer, size_t len,
                     const wchar_t *varname)
{
    wchar_t *e;

    if (!MSVCRT_CHECK_PMT(ret_len != NULL)) return EINVAL;
    *ret_len = 0;
    if (!MSVCRT_CHECK_PMT((buffer && len > 0) || (!buffer && !len))) return EINVAL;
    if (buffer) buffer[0] = 0;

    _lock(_ENV_LOCK);
    e = wgetenv_helper(varname);
    if (e)
    {
        *ret_len = wcslen(e) + 1;
        if (len >= *ret_len) wcscpy(buffer, e);
    }
    _unlock(_ENV_LOCK);

    if (!e || !len) return 0;
    if (len < *ret_len) return ERANGE;
    return 0;
}

/*********************************************************************
 *		_get_environ (MSVCRT.@)
 */
void CDECL _get_environ(char ***ptr)
{
    *ptr = MSVCRT__environ;
}

/*********************************************************************
 *		_get_wenviron (MSVCRT.@)
 */
void CDECL _get_wenviron(wchar_t ***ptr)
{
    *ptr = MSVCRT__wenviron;
}
