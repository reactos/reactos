extern void (*_imp___fpreset)(void) ;
void _fpreset (void)
{ (*_imp___fpreset)(); }

void __attribute__ ((alias ("_fpreset"))) fpreset(void);
