/* $Id: dllmain.c,v 1.14 2002/05/07 22:31:25 hbirr Exp $
 *
 * ReactOS MSVCRT.DLL Compatibility Library
 */
#include <windows.h>

#include <msvcrt/internal/tls.h>
#include <msvcrt/stdlib.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

static int nAttachCount = 0;

unsigned int _osver = 0;
unsigned int _winminor = 0;
unsigned int _winmajor = 0;
unsigned int _winver = 0;

char *_acmdln = NULL;		/* pointer to ascii command line */
#undef _environ
char **_environ = NULL;		/* pointer to environment block */
char ***_environ_dll = &_environ;/* pointer to environment block */

char **__initenv = NULL;

char *_pgmptr = NULL;		/* pointer to program name */

int __app_type = 0; //_UNKNOWN_APP;	/* application type */

int __mb_cur_max = 1;

HANDLE hHeap = NULL;		/* handle for heap */


/* FUNCTIONS **************************************************************/

int BlockEnvToEnviron()
{
  char * ptr, * ptr2;
  int i, len;

  DPRINT("BlockEnvToEnviron()\n");

  if (_environ)
  {
     FreeEnvironmentStringsA(_environ[0]);
     free(_environ);
     _environ = NULL;
  }
  ptr2 = ptr = (char*)GetEnvironmentStringsA();
  if (ptr == NULL)
  {
     DPRINT("GetEnvironmentStringsA() returnd NULL\n");
     return -1;
  }
  len = 0;
  while (*ptr2)
  {
     len++;
     while (*ptr2++);
  }
  _environ = malloc((len + 1) * sizeof(char*));
  if (_environ == NULL)
  {
     FreeEnvironmentStringsA(ptr);
     return -1;
  }
  for (i = 0; i < len && *ptr; i++)
  {
     _environ[i] = ptr;
     while (*ptr++);
  }
  _environ[i] = NULL;
  return 0;
}

BOOL __stdcall
DllMain(PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH://1
			/* initialize version info */
			DPRINT("Attach %d\n", nAttachCount);
			_osver = GetVersion();
			_winmajor = (_osver >> 8) & 0xFF;
			_winminor = _osver & 0xFF;
			_winver = (_winmajor << 8) + _winminor;
			_osver = (_osver >> 16) & 0xFFFF;

			if (hHeap == NULL || hHeap == INVALID_HANDLE_VALUE)
			{
				hHeap = HeapCreate(0, 0, 0);
				if (hHeap == NULL || hHeap == INVALID_HANDLE_VALUE)
				{
					return FALSE;
				}
			}
			if (nAttachCount==0)
			{
				__fileno_init();
			}

			/* create tls stuff */
			if (!CreateThreadData())
				return FALSE;

			_acmdln = (char *)GetCommandLineA();

			/* FIXME: This crashes all applications */
			if (BlockEnvToEnviron() < 0)
			  return FALSE;

			/* FIXME: more initializations... */

			nAttachCount++;
			DPRINT("Attach done\n");
			break;

		case DLL_THREAD_ATTACH://2
			break;

		case DLL_THREAD_DETACH://4
			FreeThreadData(NULL);
			break;

		case DLL_PROCESS_DETACH://0
			DPRINT("Detach %d\n", nAttachCount);
			if (nAttachCount > 0)
			{
				nAttachCount--;

				/* FIXME: more cleanup... */
				_fcloseall();

				/* destroy tls stuff */
				DestroyThreadData();

				/* destroy heap */
				if (nAttachCount == 0)
				{

					if (_environ)
					{
						FreeEnvironmentStringsA(_environ[0]);
						free(_environ);
						_environ = NULL;
					}
#if 1
					HeapDestroy(hHeap);
					hHeap = NULL;
#endif
				}
			DPRINT("Detach done\n");
			}
			break;
	}

	return TRUE;
}



void __set_app_type(int app_type)
{
   __app_type = app_type;
}


char **__p__acmdln(void)
{
   return &_acmdln;
}

char ***__p__environ(void)
{
   return _environ_dll;
}

char ***__p___initenv(void)
{
   return &__initenv;
}

int *__p___mb_cur_max(void)
{
   return &__mb_cur_max;
}

unsigned int *__p__osver(void)
{
   return &_osver;
}

char **__p__pgmptr(void)
{
   return &_pgmptr;
}

unsigned int *__p__winmajor(void)
{
   return &_winmajor;
}

unsigned int *__p__winminor(void)
{
   return &_winminor;
}

unsigned int *__p__winver(void)
{
   return &_winver;
}



/* EOF */
