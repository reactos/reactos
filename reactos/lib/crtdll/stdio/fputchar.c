#include <crtdll/stdio.h>
#include <crtdll/conio.h>

int	_fputchar (int c)
{
	return _putch(c);
}
