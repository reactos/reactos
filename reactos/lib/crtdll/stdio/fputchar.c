#include <crtdll/stdio.h>
#include <crtdll/conio.h>
#include <crtdll/wchar.h>

int	_fputchar (int c)
{
	return _putch(c);
}

int	_fputwchar (wchar_t c)
{
	//return _putch(c);
	return 0;
}

