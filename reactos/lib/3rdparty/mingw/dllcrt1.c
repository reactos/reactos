/*
 * dllcrt1.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Initialization code for DLLs.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <errno.h>
#include <windows.h>

/* Unlike normal crt1, I don't initialize the FPU, because the process
 * should have done that already. I also don't set the file handle modes,
 * because that would be rude. */

#ifdef	__GNUC__
extern void __main ();
extern void __do_global_dtors ();
#endif

typedef void (* p_atexit_fn )(void);
static p_atexit_fn* first_atexit;
static p_atexit_fn* next_atexit;

static void
__dll_exit (void);

/* This  is based on the function in the Wine project's exit.c */
p_atexit_fn __dllonexit (p_atexit_fn, p_atexit_fn**, p_atexit_fn**);


extern BOOL WINAPI DllMain (HANDLE, DWORD, LPVOID);

extern void _pei386_runtime_relocator (void);

BOOL WINAPI
DllMainCRTStartup (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
  BOOL bRet;

  if (dwReason == DLL_PROCESS_ATTACH)
    {

#ifdef DEBUG
      printf ("%s: DLL_PROCESS_ATTACH (%d)\n", __FUNCTION__);
#endif

      /* Initialize private atexit table for this dll.
	 32 is min size required by ANSI */

      first_atexit = (p_atexit_fn*) malloc (32 * sizeof (p_atexit_fn));
      if (first_atexit == NULL ) /* can't allocate memory */
	{
	  errno=ENOMEM;
	  return FALSE;
	}
      *first_atexit =  NULL;
      next_atexit = first_atexit;

      /* Adust references to dllimported data (from other DLL's)
	 that have non-zero offsets.  */
      _pei386_runtime_relocator ();

#ifdef	__GNUC__
      /* From libgcc.a, __main calls global class constructors,
	 __do_global_ctors, which registers __do_global_dtors
	 as the first entry of the private atexit table we
	 have just initialised  */
      __main ();

#endif
   }

  /*
   * Call the user-supplied DllMain subroutine.
   * This has to come after initialization of atexit table and
   * registration of global constructors.
   * NOTE: DllMain is optional, so libmingw32.a includes a stub
   *       which will be used if the user does not supply one.
   */

  bRet = DllMain (hDll, dwReason, lpReserved);
  /* Handle case where DllMain returns FALSE on attachment attempt.  */

  if ( (dwReason == DLL_PROCESS_ATTACH) && !bRet)
    {
#ifdef DEBUG
      printf ("%s: DLL_PROCESS_ATTACH failed, cleaning up\n", __FUNCTION__);
#endif

      __dll_exit ();     /* Cleanup now. This will set first_atexit to NULL so we
			    know we've cleaned up	*/
    }

  if (dwReason == DLL_PROCESS_DETACH)
    {
#ifdef DEBUG
      printf ("%s: DLL_PROCESS_DETACH (%d)\n", __FUNCTION__);
#endif
      /* If not attached, return FALSE. Cleanup already done above
	 if failed attachment attempt. */
      if  (! first_atexit )
        bRet = FALSE;
      else
	/*
	 * We used to call __do_global_dtors () here. This is
	 * no longer necessary since  __do_global_dtors is now
	 * registered at start (last out) of private atexit table.
	 */
	__dll_exit ();
    }
  return bRet;
}

static
void
__dll_exit(void)
/* Run LIFO terminators registered in private atexit table */
{
  if ( first_atexit )
    {
      p_atexit_fn* __last = next_atexit - 1;
      while ( __last >= first_atexit )
	{
          if ( *__last != NULL )
	    {
#ifdef DEBUG
	      printf ("%s: Calling exit function  0x%x from 0x%x\n",
		      __FUNCTION__, (unsigned)(*__last),(unsigned)__last);
#endif
              (**__last) ();
	    }
	  __last--;
	}
      free ( first_atexit ) ;
      first_atexit = NULL ;
    }
    /*
       Make sure output buffers opened by DllMain or
       atexit-registered functions are flushed before detaching,
       otherwise we can have problems with redirected output.
     */
    fflush (NULL);
}

/*
 * The atexit exported from msvcrt.dll causes problems in DLLs.
 * Here, we override the exported version of atexit with one that passes the
 * private table initialised in DllMainCRTStartup to __dllonexit.
 * That means we have to hide the mscvrt.dll atexit because the
 * atexit defined here gets __dllonexit from the same lib.
 */

#if 0
int
atexit (p_atexit_fn pfn )
{
#ifdef DEBUG
  printf ("%s: registering exit function  0x%x at 0x%x\n",
	  __FUNCTION__, (unsigned)pfn, (unsigned)next_atexit);
#endif
  return (__dllonexit (pfn,  &first_atexit, &next_atexit)
	  == NULL ? -1  : 0 );
}

/*
 * Likewise for non-ANSI function _onexit that may be called by
 * code in the dll.
 */

_onexit_t
_onexit (_onexit_t pfn )
{
#ifdef DEBUG
  printf ("%s: registering exit function  0x%x at 0x%x\n",
	  __FUNCTION__, (unsigned)pfn, (unsigned)next_atexit);
#endif
  return ((_onexit_t) __dllonexit ((p_atexit_fn)pfn,  &first_atexit, &next_atexit));
}
#endif
