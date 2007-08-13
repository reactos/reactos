// panic.cpp
// This file is (C) 2003-2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef WIN32
#include <conio.h>
#include <windows.h>
#endif//WIN32
#include "panic.h"

void panic ( const char* format, ... )
{
	va_list arg;
	int done;

	va_start(arg, format);
#if defined(WIN32) && !defined(_CONSOLE)
	char buf[4096];
	_vsnprintf ( buf, sizeof(buf)-1, format, arg );
	MessageBox ( NULL, buf, "Panic!", MB_OK|MB_ICONEXCLAMATION );
#else
	done = vprintf(format, arg);
	printf ( "\n" );
#endif
	va_end(arg);
#if defined(WIN32) && defined(_CONSOLE)
	printf ( "Press any key to exit\n" );
	(void)getch();
#endif//WIN32 && _CONSOLE
	exit ( -1 );
}
