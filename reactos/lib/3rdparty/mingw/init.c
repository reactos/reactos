/*
 * init.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Code to initialize standard file handles and command line arguments.
 * This file is #included in both crt1.c and dllcrt1.c.
 *
 */

/*
 * Access to a standard 'main'-like argument count and list. Also included
 * is a table of environment variables.
 */
int _argc = 0;
char **_argv = 0;

/* NOTE: Thanks to Pedro A. Aranda Gutiirrez <paag@tid.es> for pointing
 * this out to me. GetMainArgs (used below) takes a fourth argument
 * which is an int that controls the globbing of the command line. If
 * _CRT_glob is non-zero the command line will be globbed (e.g. *.*
 * expanded to be all files in the startup directory). In the mingw32
 * library a _CRT_glob variable is defined as being -1, enabling
 * this command line globbing by default. To turn it off and do all
 * command line processing yourself (and possibly escape bogons in
 * MS's globbing code) include a line in one of your source modules
 * defining _CRT_glob and setting it to zero, like this:
 *  int _CRT_glob = 0;
 */
extern int _CRT_glob;

#ifdef __MSVCRT__
typedef struct {
  int newmode;
} _startupinfo;
extern void __getmainargs (int *, char ***, char ***, int, _startupinfo *);
#else
extern void __GetMainArgs (int *, char ***, char ***, int);
#endif

/*
 * Initialize the _argc, _argv and environ variables.
 */
static void
_mingw32_init_mainargs ()
{
  /* The environ variable is provided directly in stdlib.h through
   * a dll function call. */
  char **dummy_environ;
#ifdef __MSVCRT__
  _startupinfo start_info;
  start_info.newmode = 0;
#endif

  /*
   * Microsoft's runtime provides a function for doing just that.
   */
#ifdef __MSVCRT__
  (void) __getmainargs (&_argc, &_argv, &dummy_environ, _CRT_glob,
                        &start_info);
#else
  /* CRTDLL version */
  (void) __GetMainArgs (&_argc, &_argv, &dummy_environ, _CRT_glob);
#endif
}

