#include <crtdll/stdio.h>
#include <crtdll/conio.h>
#include <crtdll/wchar.h>

int	_fgetchar (void)
{
	return _getch();
}

int	_fgetwchar (void)
{
	return _getch();
}
