#include <windows.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>


char *_pgmptr_dll;
char *_acmdln_dll;   
unsigned  int _commode_dll;     
unsigned  int _winmajor_dll;   
unsigned  int _winminor_dll;
unsigned  int _winver_dll;    


unsigned  int _osmajor_dll;
unsigned  int _osminor_dll;
unsigned  int _osmode_dll;
unsigned  int _osver_dll;
unsigned  int _osversion_dll;

#undef __argv
#undef __argc

char *xargv[1024];

char  **__argv = xargv;
int   __argc = 0;
int *__argc_dll = &__argc;        
char ***__argv_dll = &__argv;  


#undef _environ
char **_environ;
#undef _environ_dll
char *** _environ_dll = &_environ;    
 
        
int __GetMainArgs(int *argc,char ***argv,char ***env,int flag)
{
   int i,afterlastspace;
   DWORD   version;
   
   _acmdln_dll =  GetCommandLineA();
   
   version = GetVersion();
   _osver_dll       = version >> 16;
   _winminor_dll    = version & 0xFF;
   _winmajor_dll    = (version>>8) & 0xFF;
   _winver_dll      = ((version >> 8) & 0xFF) + ((version & 0xFF) << 8);
   
   
   /* missing threading init */
   
   i=0;
   afterlastspace=0;
   
   while (_acmdln_dll[i]) 
     {
	if (_acmdln_dll[i]==' ') 
	  {
	     __argc++;
	     _acmdln_dll[i]='\0';
	     __argv[__argc-1] = strdup(_acmdln_dll + afterlastspace);
	     i++;
	     while (_acmdln_dll[i]==' ')
	       i++;
	     afterlastspace=i;
	  } 
	else
	  {
	     i++;
	  }
     }
   
   if (_acmdln_dll[afterlastspace] != 0)
     {
	__argc++;
	_acmdln_dll[i]='\0';
	__argv[__argc-1] = strdup(_acmdln_dll+afterlastspace);
     }
   HeapValidate(GetProcessHeap(),0,NULL);
   
   _environ = (char **)GetEnvironmentStringsA();;
   _environ_dll = &_environ;    
    
   *argc = __argc;
   *argv = __argv;
   *env  = _environ;
 
   _pgmptr_dll = strdup((char *)argv[0]);

   return 0;
}

int _chkstk(void)
{
	return 0;
}
