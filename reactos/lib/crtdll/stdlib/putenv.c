#include <windows.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>



int
putenv(const char *val)
{
  
  char buffer[1024];
  char *epos; 
  
  strcpy(buffer,val);


  epos = strchr(buffer, '=');
  if ( epos == NULL )
	return -1;

  *epos = 0;

  return SetEnvironmentVariableA(buffer,epos+1);
}

  
  

  
  
