/*****************************************************************************
 *
 *  RPRINTF.C   RLF 06/15/89
 *
 *  CONTENTS    rprintf     limited re-entrant version of printf
 *              rsprintf    limited re-entrant version of sprintf
 *              _sprintf    routine which does the work
 *
 *  NOTES       Tab Stops = 4
 *
 *  $Log:   T:/pvcs/h/rprintf.h_v  $
 *
 *    Rev 1.1   29 Oct 1989 11:50:16   Richard Firth
 * Added defines for PRINTF and SPRINTF to allow easy modification when MS gets
 * the real thing working for multi-threaded programs
 *
 *    Rev 1.0   29 Aug 1989 20:04:40   RICHARDF
 * Initial revision.
 *
 ****************************************************************************/

#ifdef UNUSED
// UNUSED - causes unneed crt bloat
int cdecl rprintf(char*, ...);
#endif
int cdecl rsprintf(char*, char*, ...);
int cdecl _sprintf(char*, char*, va_list);

#define SPRINTF rsprintf
#define PRINTF  rprintf

//#define rsprintf wsprintf
//#define _sprintf wsprintf

#define RPRINTF_INCLUDED
