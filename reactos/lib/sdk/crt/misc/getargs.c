#include <precomp.h>
#include <stdlib.h>
#include <string.h>


extern char*_acmdln;
extern wchar_t* _wcmdln;
#undef _pgmptr
extern char*_pgmptr;
#undef _wpgmptr
extern wchar_t*_wpgmptr;
#undef _environ
extern char**_environ;

#undef __argv
#undef __argc

char**__argv = NULL;
#undef __wargv
wchar_t**__wargv = NULL;
int __argc = 0;

extern wchar_t **__winitenv;

extern HANDLE hHeap;

char* strndup(char* name, int len)
{
   char *s = malloc(len + 1);
   if (s != NULL)
   {
      memcpy(s, name, len);
      s[len] = 0;
   }
   return s;
}

wchar_t* wcsndup(wchar_t* name, int len)
{
   wchar_t *s = malloc((len + 1) * sizeof(wchar_t));
   if (s != NULL)
   {
      memcpy(s, name, len*sizeof(wchar_t));
      s[len] = 0;
   }
   return s;
}

#define SIZE (4096 / sizeof(char*))

int wadd(wchar_t* name)
{
   wchar_t** _new;
   if ((__argc % SIZE) == 0)
   {
      if (__wargv == NULL)
         _new = malloc(sizeof(wchar_t*) * (1 + SIZE));
      else
         _new = realloc(__wargv, sizeof(wchar_t*) * (__argc + 1 + SIZE));
      if (_new == NULL)
         return -1;
      __wargv = _new;
   }
   __wargv[__argc++] = name;
   __wargv[__argc] = NULL;
   return 0;
}

int wexpand(wchar_t* name, int expand_wildcards)
{
   wchar_t* s;
   WIN32_FIND_DATAW fd;
   HANDLE hFile;
   BOOLEAN first = TRUE;
   wchar_t buffer[256];
   int pos;

   if (expand_wildcards && (s = wcspbrk(name, L"*?")))
   {
      hFile = FindFirstFileW(name, &fd);
      if (hFile != INVALID_HANDLE_VALUE)
      {
         while(s != name && *s != L'/' && *s != L'\\')
            s--;
         pos = s - name;
         if (*s == L'/' || *s == L'\\')
            pos++;
         wcsncpy(buffer, name, pos);
         do
         {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
               wcscpy(&buffer[pos], fd.cFileName);
               if (wadd(_wcsdup(buffer)) < 0)
               {
                  FindClose(hFile);
                  return -1;
               }
               first = FALSE;
            }
         }
         while(FindNextFileW(hFile, &fd));
         FindClose(hFile);
      }
   }
   if (first)
   {
      if (wadd(name) < 0)
         return -1;
   }
   else
      free(name);
   return 0;
}

int aadd(char* name)
{
   char** _new;
   if ((__argc % SIZE) == 0)
   {
      if (__argv == NULL)
         _new = malloc(sizeof(char*) * (1 + SIZE));
      else
         _new = realloc(__argv, sizeof(char*) * (__argc + 1 + SIZE));
      if (_new == NULL)
         return -1;
      __argv = _new;
   }
   __argv[__argc++] = name;
   __argv[__argc] = NULL;
   return 0;
}

int aexpand(char* name, int expand_wildcards)
{
   char* s;
   WIN32_FIND_DATAA fd;
   HANDLE hFile;
   BOOLEAN first = TRUE;
   char buffer[256];
   int pos;

   if (expand_wildcards && (s = strpbrk(name, "*?")))
   {
      hFile = FindFirstFileA(name, &fd);
      if (hFile != INVALID_HANDLE_VALUE)
      {
         while(s != name && *s != '/' && *s != '\\')
            s--;
         pos = s - name;
         if (*s == '/' || *s == '\\')
            pos++;
         strncpy(buffer, name, pos);
         do
         {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
               strcpy(&buffer[pos], fd.cFileName);
               if (aadd(_strdup(buffer)) < 0)
               {
                  FindClose(hFile);
                  return -1;
               }
               first = FALSE;
            }
         }
         while(FindNextFileA(hFile, &fd));
         FindClose(hFile);
      }
   }
   if (first)
   {
      if (aadd(name) < 0)
         return -1;
   }
   else
      free(name);
   return 0;
}

/*
 * @unimplemented
 */
void __getmainargs(int* argc, char*** argv, char*** env, int expand_wildcards, int* new_mode)
{
   int i, afterlastspace, ignorespace, len, doexpand;

   /* missing threading init */

   i = 0;
   afterlastspace = 0;
   ignorespace = 0;
   doexpand = expand_wildcards;

   if (__argv && _environ)
   {
      *argv = __argv;
      *env = _environ;
      *argc = __argc;
      return;
   }

   __argc = 0;

   len = strlen(_acmdln);


   while (_acmdln[i])
   {
      if (_acmdln[i] == '"')
      {
         if(ignorespace)
         {
            ignorespace = 0;
         }
         else
         {
            ignorespace = 1;
            doexpand = 0;
         }
         memmove(_acmdln + i, _acmdln + i + 1, len - i);
         len--;
         continue;
      }

      if (_acmdln[i] == ' ' && !ignorespace)
      {
         aexpand(strndup(_acmdln + afterlastspace, i - afterlastspace), doexpand);
         i++;
         while (_acmdln[i]==' ')
            i++;
         afterlastspace=i;
         doexpand = expand_wildcards;
      }
      else
      {
         i++;
      }
   }

   if (_acmdln[afterlastspace] != 0)
   {
      aexpand(strndup(_acmdln+afterlastspace, i - afterlastspace), doexpand);
   }

   HeapValidate(hHeap, 0, NULL);

   *argc = __argc;
   if (__argv == NULL)
   {
       __argv = (char**)malloc(sizeof(char*));
       __argv[0] = 0;
   }
   *argv = __argv;
   *env  = _environ;
   _pgmptr = _strdup(__argv[0]);

   // if (new_mode) _set_new_mode(*new_mode);
}

/*
 * @unimplemented
 */
void __wgetmainargs(int* argc, wchar_t*** wargv, wchar_t*** wenv,
                    int expand_wildcards, int* new_mode)
{
   int i, afterlastspace, ignorespace, len, doexpand;

   /* missing threading init */

   i = 0;
   afterlastspace = 0;
   ignorespace = 0;
   doexpand = expand_wildcards;

   if (__wargv && __winitenv)
   {
      *wargv = __wargv;
      *wenv = __winitenv;
      *argc = __argc;
      return;
   }

   __argc = 0;

   len = wcslen(_wcmdln);

   while (_wcmdln[i])
   {
      if (_wcmdln[i] == L'"')
      {
         if(ignorespace)
         {
            ignorespace = 0;
         }
         else
         {
            ignorespace = 1;
            doexpand = 0;
         }
         memmove(_wcmdln + i, _wcmdln + i + 1, (len - i) * sizeof(wchar_t));
         len--;
         continue;
      }

      if (_wcmdln[i] == L' ' && !ignorespace)
      {
         wexpand(wcsndup(_wcmdln + afterlastspace, i - afterlastspace), doexpand);
         i++;
         while (_wcmdln[i]==L' ')
            i++;
         afterlastspace=i;
         doexpand = expand_wildcards;
      }
      else
      {
         i++;
      }
   }

   if (_wcmdln[afterlastspace] != 0)
   {
      wexpand(wcsndup(_wcmdln+afterlastspace, i - afterlastspace), doexpand);
   }

   HeapValidate(hHeap, 0, NULL);

   *argc = __argc;
   if (__wargv == NULL)
   {
       __wargv = (wchar_t**)malloc(sizeof(wchar_t*));
       __wargv[0] = 0;
   }
   *wargv = __wargv;
   *wenv = __winitenv;
   _wpgmptr = _wcsdup(__wargv[0]);

   // if (new_mode) _set_new_mode(*new_mode);
}

/*
 * @implemented
 */
int* __p___argc(void)
{
   return &__argc;
}

/*
 * @implemented
 */
char*** __p___argv(void)
{
   return &__argv;
}

/*
 * @implemented
 */
wchar_t*** __p___wargv(void)
{
   return &__wargv;
}


#if 0
int _chkstk(void)
{
   return 0;
}
#endif
