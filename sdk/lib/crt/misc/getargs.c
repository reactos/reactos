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

char* strndup(char const* name, size_t len)
{
   char *s = malloc(len + 1);
   if (s != NULL)
   {
      memcpy(s, name, len);
      s[len] = 0;
   }
   return s;
}

wchar_t* wcsndup(wchar_t* name, size_t len)
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
   wchar_t buffer[MAX_PATH];
   uintptr_t pos;

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
   char buffer[MAX_PATH];
   uintptr_t pos;

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
 * @implemented
 */
void __getmainargs(int* argc, char*** argv, char*** env, int expand_wildcards, int* new_mode)
{
   int i, doexpand, slashesAdded, escapedQuote, inQuotes, bufferIndex, anyLetter;
   size_t len;
   char* buffer;

   /* missing threading init */

   i = 0;
   doexpand = expand_wildcards;
   escapedQuote = FALSE;
   anyLetter = FALSE;
   slashesAdded = 0;
   inQuotes = 0;
   bufferIndex = 0;

   if (__argv && _environ)
   {
      *argv = __argv;
      *env = _environ;
      *argc = __argc;
      return;
   }

   __argc = 0;

   len = strlen(_acmdln);
   buffer = malloc(sizeof(char) * len);

   // Reference: https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170
   while (TRUE)
   {
      // Arguments are delimited by white space, which is either a space or a tab.
      if (i >= len || ((_acmdln[i] == ' ' || _acmdln[i] == '\t') && !inQuotes))
      {
         // Handle the case when empty spaces are in the end of the cmdline
         if (anyLetter)
         {
            aexpand(strndup(buffer, bufferIndex), doexpand);
         }
         // Copy the last element from buffer and quit the loop
         if (i >= len)
         {
            break;
         }

         while (_acmdln[i] == ' ' || _acmdln[i] == '\t')
            ++i;
         anyLetter = FALSE;
         bufferIndex = 0;
         slashesAdded = 0;
         escapedQuote = FALSE;
         continue;
      }

      anyLetter = TRUE;

      if (_acmdln[i] == '\\')
      {
         buffer[bufferIndex++] = _acmdln[i];
         ++slashesAdded;
         ++i;
         escapedQuote = FALSE;
         continue;
      }

      if (_acmdln[i] == '\"')
      {
         if (slashesAdded > 0)
         {
            if (slashesAdded % 2 == 0)
            {
               // If an even number of backslashes is followed by a double quotation mark, then one backslash (\)
               // is placed in the argv array for every pair of backslashes (\\), and the double quotation mark (")
               // is interpreted as a string delimiter.
               bufferIndex -= slashesAdded / 2;
            }
            else
            {
               // If an odd number of backslashes is followed by a double quotation mark, then one backslash (\)
               // is placed in the argv array for every pair of backslashes (\\) and the double quotation mark is
               // interpreted as an escape sequence by the remaining backslash, causing a literal double quotation mark (")
               // to be placed in argv.
               bufferIndex -= slashesAdded / 2 + 1;
               buffer[bufferIndex++] = '\"';
               slashesAdded = 0;
               escapedQuote = TRUE;
               ++i;
               continue;
            }
            slashesAdded = 0;
         }
         else if (!inQuotes && i > 0 && _acmdln[i - 1] == '\"' && !escapedQuote)
         {
            buffer[bufferIndex++] = '\"';
            ++i;
            escapedQuote = TRUE;
            continue;
         }
         slashesAdded = 0;
         escapedQuote = FALSE;
         inQuotes = !inQuotes;
         doexpand = inQuotes ? FALSE : expand_wildcards;
         ++i;
         continue;
      }

      buffer[bufferIndex++] = _acmdln[i];
      slashesAdded = 0;
      escapedQuote = FALSE;
      ++i;
   }

   /* Free the temporary buffer. */
   free(buffer);

   *argc = __argc;
   if (__argv == NULL)
   {
      __argv = (char**)malloc(sizeof(char*));
      __argv[0] = 0;
   }
   *argv = __argv;
   *env  = _environ;

   _pgmptr = malloc(MAX_PATH * sizeof(char));
   if (_pgmptr)
   {
      if (!GetModuleFileNameA(NULL, _pgmptr, MAX_PATH))
        _pgmptr[0] = '\0';
      else
        _pgmptr[MAX_PATH - 1] = '\0';
   }
   else
   {
      _pgmptr = _strdup(__argv[0]);
   }

   HeapValidate(GetProcessHeap(), 0, NULL);

   // if (new_mode) _set_new_mode(*new_mode);
}

/*
 * @implemented
 */
void __wgetmainargs(int* argc, wchar_t*** wargv, wchar_t*** wenv,
                    int expand_wildcards, int* new_mode)
{
   int i, doexpand, slashesAdded, escapedQuote, inQuotes, bufferIndex, anyLetter;
   size_t len;
   wchar_t* buffer;

   /* missing threading init */

   i = 0;
   doexpand = expand_wildcards;
   escapedQuote = FALSE;
   anyLetter = TRUE;
   slashesAdded = 0;
   inQuotes = 0;
   bufferIndex = 0;

   if (__wargv && __winitenv)
   {
      *wargv = __wargv;
      *wenv = __winitenv;
      *argc = __argc;
      return;
   }

   __argc = 0;

   len = wcslen(_wcmdln);
   buffer = malloc(sizeof(wchar_t) * len);

   // Reference: https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170
   while (TRUE)
   {
      // Arguments are delimited by white space, which is either a space or a tab.
      if (i >= len || ((_wcmdln[i] == ' ' || _wcmdln[i] == '\t') && !inQuotes))
      {
         // Handle the case when empty spaces are in the end of the cmdline
         if (anyLetter)
         {
            wexpand(wcsndup(buffer, bufferIndex), doexpand);
         }
         // Copy the last element from buffer and quit the loop
         if (i >= len)
         {
            break;
         }

         while (_wcmdln[i] == ' ' || _wcmdln[i] == '\t')
            ++i;
         anyLetter = FALSE;
         bufferIndex = 0;
         slashesAdded = 0;
         escapedQuote = FALSE;
         continue;
      }

      anyLetter = TRUE;

      if (_wcmdln[i] == '\\')
      {
         buffer[bufferIndex++] = _wcmdln[i];
         ++slashesAdded;
         ++i;
         escapedQuote = FALSE;
         continue;
      }

      if (_wcmdln[i] == '\"')
      {
         if (slashesAdded > 0)
         {
            if (slashesAdded % 2 == 0)
            {
               // If an even number of backslashes is followed by a double quotation mark, then one backslash (\)
               // is placed in the argv array for every pair of backslashes (\\), and the double quotation mark (")
               // is interpreted as a string delimiter.
               bufferIndex -= slashesAdded / 2;
            }
            else
            {
               // If an odd number of backslashes is followed by a double quotation mark, then one backslash (\)
               // is placed in the argv array for every pair of backslashes (\\) and the double quotation mark is
               // interpreted as an escape sequence by the remaining backslash, causing a literal double quotation mark (")
               // to be placed in argv.
               bufferIndex -= slashesAdded / 2 + 1;
               buffer[bufferIndex++] = '\"';
               slashesAdded = 0;
               escapedQuote = TRUE;
               ++i;
               continue;
            }
            slashesAdded = 0;
         }
         else if (!inQuotes && i > 0 && _wcmdln[i - 1] == '\"' && !escapedQuote)
         {
            buffer[bufferIndex++] = '\"';
            ++i;
            escapedQuote = TRUE;
            continue;
         }
         slashesAdded = 0;
         escapedQuote = FALSE;
         inQuotes = !inQuotes;
         doexpand = inQuotes ? FALSE : expand_wildcards;
         ++i;
         continue;
      }

      buffer[bufferIndex++] = _wcmdln[i];
      slashesAdded = 0;
      escapedQuote = FALSE;
      ++i;
   }

   /* Free the temporary buffer. */
   free(buffer);

   *argc = __argc;
   if (__wargv == NULL)
   {
      __wargv = (wchar_t**)malloc(sizeof(wchar_t*));
      __wargv[0] = 0;
   }
   *wargv = __wargv;
   *wenv = __winitenv;

   _wpgmptr = malloc(MAX_PATH * sizeof(wchar_t));
   if (_wpgmptr)
   {
      if (!GetModuleFileNameW(NULL, _wpgmptr, MAX_PATH))
        _wpgmptr[0] = '\0';
      else
        _wpgmptr[MAX_PATH - 1] = '\0';
   }
   else
   {
      _wpgmptr = _wcsdup(__wargv[0]);
   }

   HeapValidate(GetProcessHeap(), 0, NULL);

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


