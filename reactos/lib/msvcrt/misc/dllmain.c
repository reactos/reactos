/* $Id: dllmain.c,v 1.6 2001/01/18 13:21:51 jean Exp $
 * 
 * ReactOS MSVCRT.DLL Compatibility Library
 */
#include <windows.h>

#include <msvcrt/internal/tls.h>
#include <msvcrt/stdlib.h>

static int nAttachCount = 0;

unsigned int _osver = 0;
unsigned int _winminor = 0;
unsigned int _winmajor = 0;
unsigned int _winver = 0;

char *_acmdln = NULL;		/* pointer to ascii command line */
#undef _environ
char **_environ = NULL;		/* pointer to environment block */
char ***_environ_dll = &_environ;/* pointer to environment block */
char *_pgmptr = NULL;		/* pointer to program name */

int __app_type = 0; //_UNKNOWN_APP;	/* application type */

static int envAlloced = 0;


/* FUNCTIONS **************************************************************/

int BlockEnvToEnviron()
{
char * ptr;
int i;
  if (!envAlloced)
  {
    envAlloced = 50;
    _environ = malloc (envAlloced * sizeof (char **));
    if (!_environ) return -1;
    _environ[0] =NULL;
  }
  ptr = (char *)GetEnvironmentStringsA();
  if (!ptr) return -1;
  for (i = 0 ; *ptr ; i++)
  {
    _environ[i] = ptr;
    while(*ptr) ptr++;
    ptr++;
  }
  _environ[i] =0;
  return 0;
}

BOOLEAN __stdcall
DllMain(PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH://1
			/* initialize version info */
			_osver = GetVersion();
			_winmajor = (_osver >> 8) & 0xFF;
			_winminor = _osver & 0xFF;
			_winver = (_winmajor << 8) + _winminor;
			_osver = (_osver >> 16) & 0xFFFF;

			/* create tls stuff */
			if (!CreateThreadData())
				return FALSE;

			_acmdln = (char *)GetCommandLineA();
			if( BlockEnvToEnviron() )
			  return FALSE;

			/* FIXME: more initializations... */

			nAttachCount++;
			break;

		case DLL_THREAD_ATTACH://2
			break;

		case DLL_THREAD_DETACH://4
			FreeThreadData(NULL);
			break;

		case DLL_PROCESS_DETACH://0
			if (nAttachCount > 0)
			{
				nAttachCount--;

				/* FIXME: more cleanup... */

				/* destroy tls stuff */
				DestroyThreadData();
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
