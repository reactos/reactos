#include <precomp.h>
#include <internal/atexit.h>

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
  _exit(status);
  for(;;);
}


/*
 * @implemented
 */
void _exit(int _status)
{
	ExitProcess(_status);
	for(;;);
}

/*
 * @unimplemented
 */
void _cexit( void )
{
	// flush
}

/*
 * @unimplemented
 */
void _c_exit( void )
{
	// reset interup vectors
}
