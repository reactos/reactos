#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

extern char *_acmdln;
extern char *_pgmptr;
#undef _environ
extern char **_environ;

#undef __argv
#undef __argc

char *xargv[1024];

char  **__argv = xargv;
int   __argc = 0;


int __getmainargs(int *argc,char ***argv,char ***env,int flag)
{
   int i,afterlastspace;
   DWORD   version;
   
   /* missing threading init */
   
   i=0;
   afterlastspace=0;
   
   while (_acmdln[i])
     {
	if (_acmdln[i]==' ')
	  {
	     __argc++;
	     _acmdln[i]='\0';
	     __argv[__argc-1] = strdup(_acmdln + afterlastspace);
       _acmdln[i]=' ';
	     i++;
	     while (_acmdln[i]==' ')
	       i++;
	     afterlastspace=i;
	  }
	else
	  {
	     i++;
	  }
     }
   
   if (_acmdln[afterlastspace] != 0)
     {
	__argc++;
	_acmdln[i]='\0';
	__argv[__argc-1] = strdup(_acmdln+afterlastspace);
     }
   HeapValidate(GetProcessHeap(),0,NULL);
   
   *argc = __argc;
   *argv = __argv;
   *env  = _environ;

   _pgmptr = strdup((char *)argv[0]);

   return 0;
}


int *__p___argc(void)
{
   return &__argc;
}

char ***__p___argv(void)
{
   return &__argv;
}

#if 0
int _chkstk(void)
{
	return 0;
}
#endif
