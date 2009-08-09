void _fpreset (void)
  { __asm__ ("fninit" ) ;}

void __attribute__ ((alias ("_fpreset"))) fpreset(void);
