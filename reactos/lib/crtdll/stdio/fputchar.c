#include <msvcrt/stdio.h>
#include <msvcrt/conio.h>
#include <msvcrt/wchar.h>

/*
 * @implemented
 */
int	_fputchar (int c)
{
	return _putch(c);
}

/*
 * @unimplemented
 */
int	_fputwchar (wchar_t c)
{
	//return _putch(c);
	return 0;
}

