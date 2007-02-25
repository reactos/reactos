#include <precomp.h>
#include <process.h>
#include <tchar.h>

#define NDEBUG
#include <internal/debug.h>

#ifdef _UNICODE
   #define find_execT find_execW
   #define argvtosT argvtosW
   #define do_spawnT do_spawnW
   #define valisttosT valisttosW
   #define extT extW
#else
   #define find_execT find_execA
   #define argvtosT argvtosA
   #define do_spawnT do_spawnA
   #define valisttosT valisttosA
   #define extT extA
#endif


_TCHAR const* extT[] =
   {
      _T(""),
      _T(".bat"),
      _T(".cmd"),
      _T(".com"),
      _T(".exe")
   };

const _TCHAR* find_execT(const _TCHAR* path, _TCHAR* rpath)
{
   _TCHAR *rp;
   const _TCHAR *rd;
   unsigned int i, found = 0;

   DPRINT(MK_STR(find_execT)"('%"sT"', %x)\n", path, rpath);

   if (path == NULL)
   {
      return NULL;
   }
   if (_tcslen(path) > FILENAME_MAX - 1)
   {
      return path;
   }
   /* copy path in rpath */
   for (rd = path, rp = rpath; *rd; *rp++ = *rd++)
      ;
   *rp = 0;
   /* try first with the name as is */
   for (i = 0; i < sizeof(extT) / sizeof(*extT); i++)
   {
      _tcscpy(rp, extT[i]);

      DPRINT("trying '%"sT"'\n", rpath);

      if (_taccess(rpath, F_OK) == 0 && access_dirT(rpath) != 0)
      {
         found = 1;
         break;
      }
   }
   if (!found)
   {
      _TCHAR* env = _tgetenv(_T("PATH"));
      if (env)
      {
         _TCHAR* ep = env;
         while (*ep && !found)
         {
            if (*ep == ';')
               ep++;
            rp=rpath;
            for (; *ep && (*ep != ';'); *rp++ = *ep++)
               ;
            if (rp > rpath)
            {
               rp--;
               if (*rp != '/' && *rp != '\\')
               {
                  *++rp = '\\';
               }
               rp++;
            }
            for (rd=path; *rd; *rp++ = *rd++)
               ;
            for (i = 0; i < sizeof(extT) / sizeof(*extT); i++)
            {
               _tcscpy(rp, extT[i]);

               DPRINT("trying '%"sT"'\n", rpath);

               if (_taccess(rpath, F_OK) == 0 && access_dirT(rpath) != 0)
               {
                  found = 1;
                  break;
               }
            }
         }
      }
   }

   return found ? rpath : path;
}

static _TCHAR*
argvtosT(const _TCHAR* const* argv, _TCHAR delim)
{
   int i, len;
   _TCHAR *ptr, *str;

   if (argv == NULL)
      return NULL;

   for (i = 0, len = 0; argv[i]; i++)
   {
      len += _tcslen(argv[i]) + 1;
   }

   str = ptr = (_TCHAR*) malloc(len + 1);
   if (str == NULL)
      return NULL;

   for(i = 0; argv[i]; i++)
   {
      len = _tcslen(argv[i]);
      memcpy(ptr, argv[i], len * sizeof(_TCHAR));
      ptr += len;
      *ptr++ = delim;
   }
   *ptr = 0;

   return str;
}

static _TCHAR*
valisttosT(const _TCHAR* arg0, va_list alist, _TCHAR delim)
{
   va_list alist2 = alist;
   _TCHAR *ptr, *str;
   int len;

   if (arg0 == NULL)
      return NULL;

   ptr = (_TCHAR*)arg0;
   len = 0;
   do
   {
      len += _tcslen(ptr) + 1;
      ptr = va_arg(alist, _TCHAR*);
   }
   while(ptr != NULL);

   str = (_TCHAR*) malloc(len + 1);
   if (str == NULL)
      return NULL;

   ptr = str;
   do
   {
      len = _tcslen(arg0);
      memcpy(ptr, arg0, len * sizeof(_TCHAR));
      ptr += len;
      *ptr++ = delim;
      arg0 = va_arg(alist2, _TCHAR*);
   }
   while(arg0 != NULL);
   *ptr = 0;

   return str;
}

static int
do_spawnT(int mode, const _TCHAR* cmdname, const _TCHAR* args, const _TCHAR* envp)
{
   STARTUPINFO StartupInfo = {0};
   PROCESS_INFORMATION ProcessInformation;
//   char* fmode;
//   HANDLE* hFile;
//   int i, last;
   BOOL bResult;
   DWORD dwExitCode;
   DWORD dwError;

   TRACE(MK_STR(do_spawnT)"(%i,'%"sT"','%"sT"','%"sT"')",mode,cmdname,args,envp);


   if (mode != _P_NOWAIT && mode != _P_NOWAITO && mode != _P_WAIT && mode != _P_DETACH && mode != _P_OVERLAY)
   {
      __set_errno ( EINVAL );
      return( -1);
   }

   if (0 != _taccess(cmdname, F_OK))
   {
      __set_errno ( ENOENT );
      return(-1);
   }
   if (0 == access_dirT(cmdname))
   {
      __set_errno ( EISDIR );
      return(-1);
   }

   //memset (&StartupInfo, 0, sizeof(StartupInfo));
   StartupInfo.cb = sizeof(StartupInfo);

#if 0

   for (last = i = 0; i < FDINFO_FD_MAX; i++)
   {
      if ((void*)-1 != _get_osfhandle(i))
      {
         last = i + 1;
      }
   }

   if (last)
   {
      StartupInfo.cbReserved2 = sizeof(ULONG) + last * (sizeof(char) + sizeof(HANDLE));
      StartupInfo.lpReserved2 = malloc(StartupInfo.cbReserved2);
      if (StartupInfo.lpReserved2 == NULL)
      {
         __set_errno ( ENOMEM );
         return -1;
      }

      *(DWORD*)StartupInfo.lpReserved2 = last;
      fmode = (char*)(StartupInfo.lpReserved2 + sizeof(ULONG));
      hFile = (HANDLE*)(StartupInfo.lpReserved2 + sizeof(ULONG) + last * sizeof(char));
      for (i = 0; i < last; i++)
      {
         int _mode = __fileno_getmode(i);
         HANDLE h = _get_osfhandle(i);
         /* FIXME: The test of console handles (((ULONG)Handle) & 0x10000003) == 0x3)
          *        is possible wrong
          */
         if ((((ULONG)h) & 0x10000003) == 0x3 || _mode & _O_NOINHERIT || (i < 3 && mode == _P_DETACH))
         {
            *hFile = INVALID_HANDLE_VALUE;
            *fmode = 0;
         }
         else
         {
            DWORD dwFlags;
            BOOL bFlag;
            bFlag = GetHandleInformation(h, &dwFlags);
            if (bFlag && (dwFlags & HANDLE_FLAG_INHERIT))
            {
               *hFile = h;
               *fmode = (_O_ACCMODE & _mode) | (((_O_TEXT | _O_BINARY) & _mode) >> 8);
            }
            else
            {
               *hFile = INVALID_HANDLE_VALUE;
               *fmode = 0;
            }
         }
         fmode++;
         hFile++;
      }
   }
#endif

   create_io_inherit_block((STARTUPINFOA*) &StartupInfo);

   bResult = CreateProcess((_TCHAR *)cmdname,
                            (_TCHAR *)args,
                            NULL,
                            NULL,
                            TRUE,
                            mode == _P_DETACH ? DETACHED_PROCESS : 0,
                            (LPVOID)envp,
                            NULL,
                            &StartupInfo,
                            &ProcessInformation);

   if (StartupInfo.lpReserved2)
   {
      free(StartupInfo.lpReserved2);
   }

   if (!bResult)
   {
      dwError = GetLastError();
      DPRINT("%x\n", dwError);
      __set_errno(dwError);
      return(-1);
   }
   CloseHandle(ProcessInformation.hThread);
   switch(mode)
   {
      case _P_NOWAIT:
      case _P_NOWAITO:
         return((int)ProcessInformation.hProcess);
      case _P_OVERLAY:
         CloseHandle(ProcessInformation.hProcess);
         _exit(0);
      case _P_WAIT:
         WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
         GetExitCodeProcess(ProcessInformation.hProcess, &dwExitCode);
         CloseHandle(ProcessInformation.hProcess);
         return( (int)dwExitCode); //CORRECT?
      case _P_DETACH:
         CloseHandle(ProcessInformation.hProcess);
         return( 0);
   }
   return( (int)ProcessInformation.hProcess);
}

/*
 * @implemented
 */
int _tspawnl(int mode, const _TCHAR *cmdname, const _TCHAR* arg0, ...)
{
   va_list argp;
   _TCHAR* args;
   int ret = -1;

   DPRINT(MK_STR(_tspawnl)"('%"sT"')\n", cmdname);

   va_start(argp, arg0);
   args = valisttosT(arg0, argp, ' ');

   if (args)
   {
      ret = do_spawnT(mode, cmdname, args, NULL);
      free(args);
   }
   return ret;
}

/*
 * @implemented
 */
int _tspawnv(int mode, const _TCHAR *cmdname, const _TCHAR* const* argv)
{
   _TCHAR* args;
   int ret = -1;

   DPRINT(MK_STR(_tspawnv)"('%"sT"')\n", cmdname);

   args = argvtosT(argv, ' ');

   if (args)
   {
      ret = do_spawnT(mode, cmdname, args, NULL);
      free(args);
   }
   return ret;
}

/*
 * @implemented
 */
int _tspawnle(int mode, const _TCHAR *cmdname, const _TCHAR* arg0, ... /*, NULL, const char* const* envp*/)
{
   va_list argp;
   _TCHAR* args;
   _TCHAR* envs;
   _TCHAR const * const* ptr;
   int ret = -1;

   DPRINT(MK_STR(_tspawnle)"('%"sT"')\n", cmdname);

   va_start(argp, arg0);
   args = valisttosT(arg0, argp, ' ');
   do
   {
      ptr = (_TCHAR const* const*)va_arg(argp, _TCHAR*);
   }
   while (ptr != NULL);
   ptr = (_TCHAR const* const*)va_arg(argp, _TCHAR*);
   envs = argvtosT(ptr, 0);
   if (args)
   {
      ret = do_spawnT(mode, cmdname, args, envs);
      free(args);
   }
   if (envs)
   {
      free(envs);
   }
   return ret;

}

/*
 * @implemented
 */
int _tspawnve(int mode, const _TCHAR *cmdname, const _TCHAR* const* argv, const _TCHAR* const* envp)
{
   _TCHAR *args;
   _TCHAR *envs;
   int ret = -1;

   DPRINT(MK_STR(_tspawnve)"('%"sT"')\n", cmdname);

   args = argvtosT(argv, ' ');
   envs = argvtosT(envp, 0);

   if (args)
   {
      ret = do_spawnT(mode, cmdname, args, envs);
      free(args);
   }
   if (envs)
   {
      free(envs);
   }
   return ret;
}

/*
 * @implemented
 */
int _tspawnvp(int mode, const _TCHAR* cmdname, const _TCHAR* const* argv)
{
   _TCHAR pathname[FILENAME_MAX];

   DPRINT(MK_STR(_tspawnvp)"('%"sT"')\n", cmdname);

   return _tspawnv(mode, find_execT(cmdname, pathname), argv);
}

/*
 * @implemented
 */
int _tspawnlp(int mode, const _TCHAR* cmdname, const _TCHAR* arg0, .../*, NULL*/)
{
   va_list argp;
   _TCHAR* args;
   int ret = -1;
   _TCHAR pathname[FILENAME_MAX];

   DPRINT(MK_STR(_tspawnlp)"('%"sT"')\n", cmdname);

   va_start(argp, arg0);
   args = valisttosT(arg0, argp, ' ');
   if (args)
   {
      ret = do_spawnT(mode, find_execT(cmdname, pathname), args, NULL);
      free(args);
   }
   return ret;
}


/*
 * @implemented
 */
int _tspawnlpe(int mode, const _TCHAR* cmdname, const _TCHAR* arg0, .../*, NULL, const char* const* envp*/)
{
   va_list argp;
   _TCHAR* args;
   _TCHAR* envs;
   _TCHAR const* const * ptr;
   int ret = -1;
   _TCHAR pathname[FILENAME_MAX];

   DPRINT(MK_STR(_tspawnlpe)"('%"sT"')\n", cmdname);

   va_start(argp, arg0);
   args = valisttosT(arg0, argp, ' ');
   do
   {
      ptr = (_TCHAR const* const*)va_arg(argp, _TCHAR*);
   }
   while (ptr != NULL);
   ptr = (_TCHAR const* const*)va_arg(argp, _TCHAR*);
   envs = argvtosT(ptr, 0);
   if (args)
   {
      ret = do_spawnT(mode, find_execT(cmdname, pathname), args, envs);
      free(args);
   }
   if (envs)
   {
      free(envs);
   }
   return ret;
}

/*
 * @implemented
 */
int _tspawnvpe(int mode, const _TCHAR* cmdname, const _TCHAR* const* argv, const _TCHAR* const* envp)
{
   _TCHAR pathname[FILENAME_MAX];

   DPRINT(MK_STR(_tspawnvpe)"('%"sT"')\n", cmdname);

   return _tspawnve(mode, find_execT(cmdname, pathname), argv, envp);
}

/*
 * @implemented
 */
int _texecl(const _TCHAR* cmdname, const _TCHAR* arg0, ...)
{
   _TCHAR* args;
   va_list argp;
   int ret = -1;

   DPRINT(MK_STR(_texecl)"('%"sT"')\n", cmdname);

   va_start(argp, arg0);
   args = valisttosT(arg0, argp, ' ');

   if (args)
   {
      ret = do_spawnT(P_OVERLAY, cmdname, args, NULL);
      free(args);
   }
   return ret;
}

/*
 * @implemented
 */
int _texecv(const _TCHAR* cmdname, const _TCHAR* const* argv)
{
   DPRINT(MK_STR(_texecv)"('%"sT"')\n", cmdname);
   return _tspawnv(P_OVERLAY, cmdname, argv);
}

/*
 * @implemented
 */
int _texecle(const _TCHAR* cmdname, const _TCHAR* arg0, ... /*, NULL, char* const* envp */)
{
   va_list argp;
   _TCHAR* args;
   _TCHAR* envs;
   _TCHAR const* const* ptr;
   int ret = -1;

   DPRINT(MK_STR(_texecle)"('%"sT"')\n", cmdname);

   va_start(argp, arg0);
   args = valisttosT(arg0, argp, ' ');
   do
   {
      ptr = (_TCHAR const* const*)va_arg(argp, _TCHAR*);
   }
   while (ptr != NULL);
   ptr = (_TCHAR const* const*)va_arg(argp, _TCHAR*);
   envs = argvtosT(ptr, 0);
   if (args)
   {
      ret = do_spawnT(P_OVERLAY, cmdname, args, envs);
      free(args);
   }
   if (envs)
   {
      free(envs);
   }
   return ret;
}

/*
 * @implemented
 */
int _texecve(const _TCHAR* cmdname, const _TCHAR* const* argv, const _TCHAR* const* envp)
{
   DPRINT(MK_STR(_texecve)"('%"sT"')\n", cmdname);
   return _tspawnve(P_OVERLAY, cmdname, argv, envp);
}

/*
 * @implemented
 */
int _texeclp(const _TCHAR* cmdname, const _TCHAR* arg0, ...)
{
   _TCHAR* args;
   va_list argp;
   int ret = -1;
   _TCHAR pathname[FILENAME_MAX];

   DPRINT(MK_STR(_texeclp)"('%"sT"')\n", cmdname);

   va_start(argp, arg0);
   args = valisttosT(arg0, argp, ' ');

   if (args)
   {
      ret = do_spawnT(P_OVERLAY, find_execT(cmdname, pathname), args, NULL);
      free(args);
   }
   return ret;
}

/*
 * @implemented
 */
int _texecvp(const _TCHAR* cmdname, const _TCHAR* const* argv)
{
   DPRINT(MK_STR(_texecvp)"('%"sT"')\n", cmdname);
   return _tspawnvp(P_OVERLAY, cmdname, argv);
}

/*
 * @implemented
 */
int _texeclpe(const _TCHAR* cmdname, const _TCHAR* arg0, ... /*, NULL, char* const* envp */)
{
   va_list argp;
   _TCHAR* args;
   _TCHAR* envs;
   _TCHAR const* const* ptr;
   int ret = -1;
   _TCHAR pathname[FILENAME_MAX];

   DPRINT(MK_STR(_texeclpe)"('%"sT"')\n", cmdname);

   va_start(argp, arg0);
   args = valisttosT(arg0, argp, ' ');
   do
   {
      ptr = (_TCHAR const* const*)va_arg(argp, _TCHAR*);
   }
   while (ptr != NULL);
   ptr = (_TCHAR const* const*)va_arg(argp, _TCHAR*);
   envs = argvtosT(ptr, 0);
   if (args)
   {
      ret = do_spawnT(P_OVERLAY, find_execT(cmdname, pathname), args, envs);
      free(args);
   }
   if (envs)
   {
      free(envs);
   }
   return ret;
}

/*
 * @implemented
 */
int _texecvpe(const _TCHAR* cmdname, const _TCHAR* const* argv, const _TCHAR* const* envp)
{
   DPRINT(MK_STR(_texecvpe)"('%"sT"')\n", cmdname);
   return _tspawnvpe(P_OVERLAY, cmdname, argv, envp);
}

