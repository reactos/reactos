/* $Id: crt0w32.c,v 1.1 2002/01/20 21:24:49 ea Exp $
 * 
 * PROJECT    : ReactOS / POSIX+ personality
 * FILE       : subsys/psx/lib/cr0w32.c
 * DESCRIPTION: startup code for POSIX+ applications.
 * DATE       : 2002-01-18
 * AUTHOR     : Emanuele Aliberti <ea@iol.it>
 */

extern void __stdcall __PdxInitializeData(int*,char***);
extern int main (int,char**,char**);
extern void exit(int);

/* ANSI ENVIRONMENT */

char **_environ  = (char**) 0;

int errno = 0;

#ifdef __SUBSYSTEM_WINDOWS__
void WinMainCRTStartup (void)
#else
void mainCRTStartup (void)
#endif
{
  char * argv[2] = {"none", 0};

  /* TODO: parse the command line */
  exit(main(1,argv,0));
}

void __main ()
{
  /*
   * Store in PSXDLL.DLL two well known global symbols
   * references.
   */
  __PdxInitializeData (& errno, & _environ);  /* PSXDLL.__PdxInitializeData */
  /* CRT initialization. */
#ifdef __SUBSYSTEM_WINDOWS__
  WinMainCRTStartup ();
#else
  mainCRTStartup (); 
#endif
}
/* EOF */
