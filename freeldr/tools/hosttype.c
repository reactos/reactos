//
// hosttype.c
// Copyright (C) 2002 by Brian Palmer <brianp@sginet.com>
//

#include <stdio.h>

int main(int argc, char *argv[])
{
#if defined (__DJGPP__)
	printf("dos\n");
#elif defined (__WIN32__)
	printf("win32\n");
#else
	printf("linux\n");
#endif // defined __DJGPP__

	return 0;
}
