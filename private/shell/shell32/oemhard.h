#if defined (_X86_)

STDAPI_(BOOL) IsNEC_PC9800(VOID);

#else

#define IsNEC_PC9800() FALSE

#endif
