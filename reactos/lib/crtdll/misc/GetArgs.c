#include <windows.h>
#include <stdlib.h>
#include <string.h>


char *acmdln_dll;      
unsigned  int commode_dll;     
    
unsigned  int fmode_dll;       
unsigned  int winmajor_dll;     
unsigned  int winminor_dll;     
unsigned  int winver_dll;    
unsigned  int osver_dll;    

#undef __argv
#undef __argc

char *xargv[1024];

char  **__argv = xargv;
int   __argc = 0;
unsigned  int *__argc_dll = &__argc;        
char ***__argv_dll = &__argv;  

char *xenv;
char **_environ;
char *** _environ_dll = &_environ;    
#undef environ
char **environ; 
        

int __GetMainArgs(int *argc,char ***argv,char **env,int flag)
{
   char *cmdline;
   int i,afterlastspace;
   DWORD   version;
   
   //	acmdln_dll = cmdline = strdup(  GetCommandLineA() );
   
   version = GetVersion();
   osver_dll       = version >> 16;
   winminor_dll    = version & 0xFF;
   winmajor_dll    = (version>>8) & 0xFF;
   winver_dll      = ((version >> 8) & 0xFF) + ((version & 0xFF) << 8);
   
   
   /* missing threading init */
   
   i=0;
   cmdline = GetCommandLineA();
   afterlastspace=0;
   
   //dprintf("cmdline '%s'\n",cmdline);
   
   while (cmdline[i]) 
     {
	if (cmdline[i]==' ') 
	  {
	  //   dprintf("cmdline '%s'\n",cmdline);
	     __argc++;
	     cmdline[i]='\0';
	     __argv[__argc-1] = strdup( cmdline+afterlastspace);
	     i++;
	     while (cmdline[i]==' ')
	       i++;
	     if (cmdline[i])
	       afterlastspace=i;
	  } 
	else
	  {
	     i++;
	  }
     }
  
   
   __argc++;
   cmdline[i]='\0';
   __argv[__argc-1] = strdup( cmdline+afterlastspace);
   HeapValidate(GetProcessHeap(),0,NULL);
    
   *argc    = __argc;
   *argv    = __argv;
   
   
//   xenv = GetEnvironmentStringsA();
   _environ = &xenv;
   _environ_dll = &_environ;    
   environ = &xenv; 
   env =  &xenv;
   return 0;
}

int _chkstk(void)
{
	return 0;
}
