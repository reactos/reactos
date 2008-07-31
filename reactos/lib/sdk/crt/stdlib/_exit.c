#include <precomp.h>
#include <internal/atexit.h>

extern void _atexit_cleanup(void);

struct __atexit *__atexit_ptr = 0;

/*
 * @implemented
 */
void exit(int status)
{
  //int i;

/*
  if (__stdio_cleanup_hook)
    __stdio_cleanup_hook();
  for (i=0; i<djgpp_last_dtor-djgpp_first_dtor; i++)
    djgpp_first_dtor[i]();
*/
  /* in case the program set it this way */
  _setmode(0, O_TEXT);
  _atexit_cleanup();
  _exit(status);
  for(;;);
}


/*
 * @implemented
 */
void _exit(int _status)
{
	// FIXME: _exit and _c_exit should prevent atexit routines from being called during DLL unload
	ExitProcess(_status);
	for(;;);
}

/*
 * @unimplemented
 */
void _cexit( void )
{
	_atexit_cleanup();
	// flush
}

/*
 * @unimplemented
 */
void _c_exit( void )
{
	// FIXME: _exit and _c_exit should prevent atexit routines from being called during DLL unload
	// reset interup vectors
}
