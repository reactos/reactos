/* $Id$
 *
 * environ.c
 *
 * ReactOS MSVCRT.DLL Compatibility Library
 */

#include <precomp.h>
#include <internal/tls.h>
#include <stdlib.h>
#include <string.h>


unsigned int _osver = 0;
unsigned int _winminor = 0;
unsigned int _winmajor = 0;
unsigned int _winver = 0;


char *_acmdln = NULL;        /* pointer to ascii command line */
wchar_t *_wcmdln = NULL;     /* pointer to wide character command line */
#undef _environ
#undef _wenviron
char **_environ = NULL;      /* pointer to environment block */
wchar_t **_wenviron = NULL;  /* pointer to environment block */
char **__initenv = NULL;     /* pointer to initial environment block */
wchar_t **__winitenv = NULL; /* pointer to initial environment block */
#undef _pgmptr
char *_pgmptr = NULL;        /* pointer to program name */
#undef _wpgmptr
wchar_t *_wpgmptr = NULL;    /* pointer to program name */
int __app_type = 0; //_UNKNOWN_APP; /* application type */
int __mb_cur_max = 1;

int _commode = _IOCOMMIT;


int BlockEnvToEnvironA(void)
{
   char *ptr, *environment_strings;
   char **envptr;
   int count = 1, len;

   TRACE("BlockEnvToEnvironA()\n");

   environment_strings = GetEnvironmentStringsA();
   if (environment_strings == NULL) {
      return -1;
   }

   for (ptr = environment_strings; *ptr; ptr += len)
   {
      len = strlen(ptr) + 1;
      /* Skip drive letter settings. */
      if (*ptr != '=')
         count++;
   }

   __initenv = _environ = malloc(count * sizeof(char*));
   if (_environ)
   {
      for (ptr = environment_strings, envptr = _environ; count > 1; ptr += len)
      {
         len = strlen(ptr) + 1;
         /* Skip drive letter settings. */
         if (*ptr != '=')
         {
            if ((*envptr = malloc(len)) == NULL)
            {
               for (envptr--; envptr >= _environ; envptr--);
                  free(*envptr);
               FreeEnvironmentStringsA(environment_strings);
               free(_environ);
	       __initenv = _environ = NULL;
               return -1;
            }
            memcpy(*envptr++, ptr, len);
            count--;
         }
      }
      /* Add terminating NULL entry. */
      *envptr = NULL;
   }

   FreeEnvironmentStringsA(environment_strings);
   return _environ ? 0 : -1;
}

int BlockEnvToEnvironW(void)
{
   wchar_t *ptr, *environment_strings;
   wchar_t **envptr;
   int count = 1, len;

   TRACE("BlockEnvToEnvironW()\n");

   environment_strings = GetEnvironmentStringsW();
   if (environment_strings == NULL) {
      return -1;
   }

   for (ptr = environment_strings; *ptr; ptr += len)
   {
      len = wcslen(ptr) + 1;
      /* Skip drive letter settings. */
      if (*ptr != '=')
         count++;
   }

   __winitenv = _wenviron = malloc(count * sizeof(wchar_t*));
   if (_wenviron)
   {
      for (ptr = environment_strings, envptr = _wenviron; count > 1; ptr += len)
      {
         len = wcslen(ptr) + 1;
         /* Skip drive letter settings. */
         if (*ptr != '=')
         {
            if ((*envptr = malloc(len * sizeof(wchar_t))) == NULL)
            {
               for (envptr--; envptr >= _wenviron; envptr--);
                  free(*envptr);
               FreeEnvironmentStringsW(environment_strings);
               free(_wenviron);
	       __winitenv = _wenviron = NULL;
               return -1;
            }
            memcpy(*envptr++, ptr, len * sizeof(wchar_t));
            count--;
         }
      }
      /* Add terminating NULL entry. */
      *envptr = NULL;
   }

   FreeEnvironmentStringsW(environment_strings);
   return _wenviron ? 0 : -1;
}

/**
 * Internal function to duplicate environment block. Although it's
 * parameter are defined as char**, it's able to work also with
 * wide character environment block which are of type wchar_t**.
 *
 * @param original_environment
 *        Environment to duplicate.
 * @param wide
 *        Set to zero for multibyte environments, non-zero otherwise.
 *
 * @return Original environment in case of failure, otherwise
 *         pointer to new environment block.
 */
char **DuplicateEnvironment(char **original_environment, int wide)
{
   int count = 1;
   char **envptr, **newenvptr, **newenv;

   for (envptr = original_environment; *envptr != NULL; envptr++, count++)
      ;

   newenvptr = newenv = malloc(count * sizeof(char*));
   if (newenv == NULL)
      return original_environment;

   for (envptr = original_environment; count > 1; newenvptr++, count--)
   {
      if (wide)
         *newenvptr = (char*)_wcsdup((wchar_t*)*envptr++);
      else
         *newenvptr = _strdup(*envptr++);
      if (*newenvptr == NULL)
      {
         for (newenvptr--; newenvptr >= newenv; newenvptr--);
            free(*newenvptr);
         free(newenv);
         return original_environment;
      }
   }
   *newenvptr = NULL;

   return newenv;
}

/**
 * Internal function to deallocate environment block. Although it's
 * parameter are defined as char**, it's able to work also with
 * wide character environment block which are of type wchar_t**.
 *
 * @param environment
 *        Environment to free.
 */
void FreeEnvironment(char **environment)
{
   char **envptr;
   for (envptr = environment; *envptr != NULL; envptr++)
      free(*envptr);
   free(environment);
}

/**
 * Internal version of _wputenv and _putenv. It works duplicates the
 * original envirnments created during initilization if needed to prevent
 * having spurious pointers floating around. Then it updates the internal
 * environment tables (_environ and _wenviron) and at last updates the
 * OS environemnt.
 *
 * Note that there can happen situation when the internal [_w]environ
 * arrays will be updated, but the OS environment update will fail. In
 * this case we don't undo the changes to the [_w]environ tables to
 * comply with the Microsoft behaviour (and it's also much easier :-).
 */
int SetEnv(const wchar_t *option)
{
   wchar_t *epos, *name;
   wchar_t **wenvptr;
   wchar_t *woption;
   char *mboption;
   int remove, index, count, size, result = 0, found = 0;

   if (option == NULL || (epos = wcschr(option, L'=')) == NULL)
      return -1;
   remove = (epos[1] == 0);

   /* Duplicate environment if needed. */
   if (_environ == __initenv)
   {
      if ((_environ = DuplicateEnvironment(_environ, 0)) == __initenv)
         return -1;
   }
   if (_wenviron == __winitenv)
   {
      if ((_wenviron = (wchar_t**)DuplicateEnvironment((char**)_wenviron, 1)) ==
          __winitenv)
         return -1;
   }

   /* Create a copy of the option name. */
   name = malloc((epos - option + 1) * sizeof(wchar_t));
   if (name == NULL)
      return -1;
   memcpy(name, option, (epos - option) * sizeof(wchar_t));
   name[epos - option] = 0;

   /* Find the option we're trying to modify. */
   for (index = 0, wenvptr = _wenviron; *wenvptr != NULL; wenvptr++, index++)
   {
      if (!_wcsnicmp(*wenvptr, option, epos - option))
      {
         found = 1;
         break;
      }
   }

   if (remove)
   {
      if (!found)
      {
         free(name);
         return 0;
      }

      /* Remove the option from wide character environment. */
      free(*wenvptr);
      for (count = index; *wenvptr != NULL; wenvptr++, count++)
         *wenvptr = *(wenvptr + 1);
      _wenviron = realloc(_wenviron, count * sizeof(wchar_t*));

      /* Remove the option from multibyte environment. We assume
       * the environments are in sync and the option is at the
       * same position. */
      free(_environ[index]);
      memmove(&_environ[index], &_environ[index+1], (count - index) * sizeof(char*));
      _environ = realloc(_environ, count * sizeof(char*));

      result = SetEnvironmentVariableW(name, NULL) ? 0 : -1;
   }
   else
   {
      /* Make a copy of the option that we will store in the environment block. */
      woption = _wcsdup((wchar_t*)option);
      if (woption == NULL)
      {
         free(name);
         return -1;
      }

      /* Create a multibyte copy of the option. */
      size = WideCharToMultiByte(CP_ACP, 0, option, -1, NULL, 0, NULL, NULL);
      mboption = malloc(size);
      if (mboption == NULL)
      {
         free(name);
         free(woption);
         return -1;
      }
      WideCharToMultiByte(CP_ACP, 0, option, -1, mboption, size, NULL, NULL);

      if (found)
      {
         /* Replace the current entry. */
         free(*wenvptr);
         *wenvptr = woption;
         free(_environ[index]);
         _environ[index] = mboption;
      }
      else
      {
         wchar_t **wnewenv;
         char **mbnewenv;

         /* Get the size of the original environment. */
         for (count = index; *wenvptr != NULL; wenvptr++, count++)
            ;

         /* Create a new entry. */
         if ((wnewenv = realloc(_wenviron, (count + 2) * sizeof(wchar_t*))) == NULL)
         {
            free(name);
            free(mboption);
            free(woption);
            return -1;
         }
         _wenviron = wnewenv;
         if ((mbnewenv = realloc(_environ, (count + 2) * sizeof(char*))) == NULL)
         {
            free(name);
            free(mboption);
            free(woption);
            return -1;
         }
         _environ = mbnewenv;

         /* Set the last entry to our option. */
         _wenviron[count] = woption;
         _environ[count] = mboption;
         _wenviron[count + 1] = NULL;
         _environ[count + 1] = NULL;
      }

      /* And finally update the OS environment. */
      result = SetEnvironmentVariableW(name, epos + 1) ? 0 : -1;
   }
   free(name);

   return result;
}

/*
 * @implemented
 */
int *__p__commode(void) // not exported by NTDLL
{
   return &_commode;
}

/*
 * @implemented
 */
void __set_app_type(int app_type)
{
    __app_type = app_type;
}

/*
 * @implemented
 */
char **__p__acmdln(void)
{
    return &_acmdln;
}

/*
 * @implemented
 */
wchar_t **__p__wcmdln(void)
{
    return &_wcmdln;
}

/*
 * @implemented
 */
char ***__p__environ(void)
{
    return &_environ;
}

/*
 * @implemented
 */
wchar_t ***__p__wenviron(void)
{
    return &_wenviron;
}

/*
 * @implemented
 */
char ***__p___initenv(void)
{
    return &__initenv;
}

/*
 * @implemented
 */
wchar_t ***__p___winitenv(void)
{
    return &__winitenv;
}

/*
 * @implemented
 */
int *__p___mb_cur_max(void)
{
    return &__mb_cur_max;
}

/*
 * @implemented
 */
unsigned int *__p__osver(void)
{
    return &_osver;
}

/*
 * @implemented
 */
char **__p__pgmptr(void)
{
    return &_pgmptr;
}

/*
 * @implemented
 */
wchar_t **__p__wpgmptr(void)
{
    return &_wpgmptr;
}

/*
 * @implemented
 */
unsigned int *__p__winmajor(void)
{
    return &_winmajor;
}

/*
 * @implemented
 */
unsigned int *__p__winminor(void)
{
    return &_winminor;
}

/*
 * @implemented
 */
unsigned int *__p__winver(void)
{
    return &_winver;
}

/* EOF */
