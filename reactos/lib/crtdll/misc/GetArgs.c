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

unsigned int _basemajor_dll;
unsigned int _baseminor_dll;
unsigned int _baseversion_dll;

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
static int envAlloced = 0;
 
        
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
    if(i>envAlloced-2)
    {
      envAlloced = i+3;
      _environ = realloc (_environ,envAlloced * sizeof (char **));
    }
    _environ[i] = ptr;
    while(*ptr) ptr++;
    ptr++;
  }
  _environ[i] =0;
  return 0;
}

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
   __argc=0;
   
   while (_acmdln_dll[i]) 
     {
	if (_acmdln_dll[i]==' ') 
	  {
	     __argc++;
	     _acmdln_dll[i]='\0';
	     __argv[__argc-1] = strdup(_acmdln_dll + afterlastspace);
	     _acmdln_dll[i]=' ';
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
   
   if( BlockEnvToEnviron() )
	return FALSE;
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
