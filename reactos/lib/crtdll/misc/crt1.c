/*
 * crt1.c
 *
 * Source code for the startup proceedures used by all programs. This code
 * is compiled to make crt0.o, which should be located in the library path.
 *
 * This code is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRENTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warrenties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.3 $
 * $Author: ariadne $
 * $Date: 1999/04/23 18:42:21 $
 *
 */

#include <crtdll/stdlib.h>
#include <crtdll/stdio.h>
#include <crtdll/io.h>
#include <crtdll/fcntl.h>
#include <crtdll/process.h>
#include <crtdll/float.h>
#include <windows.h>

/* NOTE: The code for initializing the _argv, _argc, and environ variables
 *       has been moved to a separate .c file which is included in both
 *       crt1.c and dllcrt1.c. This means changes in the code don't have to
 *       be manually synchronized, but it does lead to this not-generally-
 *       a-good-idea use of include. */
#include "init.c"

extern int main(int, char**, char**);

/*
 * Setup the default file handles to have the _CRT_fmode mode, as well as
 * any new files created by the user.
 */
extern unsigned int _CRT_fmode;

void
_mingw32_init_fmode (void)
{
	/* Don't set the file mode if the user hasn't set any value for it. */
	if (_CRT_fmode)
	{
		_fmode = _CRT_fmode;

		/*
		 * This overrides the default file mode settings for stdin,
		 * stdout and stderr. At first I thought you would have to
		 * test with isatty, but it seems that the DOS console at
		 * least is smart enough to handle _O_BINARY stdout and
		 * still display correctly.
		 */
		if (stdin)
		{
			_setmode (_fileno(stdin), _CRT_fmode);
		}
		if (stdout)
		{
			_setmode (_fileno(stdout), _CRT_fmode);
		}
		if (stderr)
		{
			_setmode (_fileno(stderr), _CRT_fmode);
		}
	}
}


/*
 * The function mainCRTStartup is the entry point for all console programs.
 */
int
mainCRTStartup (void)
{
	int	nRet;

	/*
	 * I have been told that this is the correct thing to do. You
	 * have to uncomment the prototype of SetUnhandledExceptionFilter
	 * in the GNU Win32 API headers for this to work. The type it
	 * expects is a pointer to a function of the same type as
	 * UnhandledExceptionFilter, which is prototyped just above
	 * (see Functions.h).
	 */
	//SetUnhandledExceptionFilter (NULL);

	/*
	 * Initialize floating point unit.
	 */
	_fpreset ();	/* Supplied by the runtime library. */

	/*
	 * Set up __argc, __argv and _environ.
	 */
	_mingw32_init_mainargs();

	/*
	 * Sets the default file mode for stdin, stdout and stderr, as well
	 * as files later opened by the user, to _CRT_fmode.
	 * NOTE: DLLs don't do this because that would be rude!
	 */
	_mingw32_init_fmode();

	/*
	 * Call the main function. If the user does not supply one
	 * the one in the 'libmingw32.a' library will be linked in, and
	 * that one calls WinMain. See main.c in the 'lib' dir
	 * for more details.
	 */
	nRet = main(_argc, _argv, _environ);

	/*
	 * Perform exit processing for the C library. This means
	 * flushing output and calling 'atexit' registered functions.
	 */
	_cexit();

	ExitProcess (nRet);

	return 0;
}

/*
 * For now the GUI startup function is the same as the console one.
 * This simply gets rid of the annoying warning about not being able
 * to find WinMainCRTStartup when linking GUI applications.
 */
int
WinMainCRTStartup (void)
{
	return mainCRTStartup();
}

/* With the EGCS build from Mumit Khan (or apparently b19 from Cygnus) this
 * is no longer necessary. */
#ifdef __GNUC__
/*
 * This section terminates the list of imports under GCC. If you do not
 * include this then you will have problems when linking with DLLs.
 *
 */
asm (".section .idata$3\n" ".long 0,0,0,0,0,0,0,0");
#endif

