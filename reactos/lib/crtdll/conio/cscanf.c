#include <crtdll/conio.h>
#include <stdarg.h>

int
_cscanf(char *fmt, ...)
{
	int cnt;

	va_list ap;

	// fixme cscanf
	printf("cscanf \n");
  
  	va_start(ap, fmt);
  	cnt = __vscanf(fmt, ap);
  	va_end(ap);
	return cnt;
  
}


