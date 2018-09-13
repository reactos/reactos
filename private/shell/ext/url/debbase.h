/*
 * debbase.h - Base debug macros and their retail translations.
 */


/* Macros
 *********/

/* debug assertion macro */

/*
 * ASSERT() may only be used as a statement, not as an expression.
 *
 * E.g.,
 *
 * ASSERT(pszRest);
 */

#ifdef DEBUG

#define ASSERT(exp) \
   if (exp) \
      ; \
   else \
      ERROR_OUT(("assertion failed '%s'", (PCSTR)#exp))

#else

#define ASSERT(exp)

#endif   /* DEBUG */

/* debug evaluation macro */

/*
 * EVAL() may only be used as a logical expression.
 *
 * E.g.,
 *
 * if (EVAL(exp))
 *    bResult = TRUE;
 */

#ifdef DEBUG

#define EVAL(exp) \
   ((exp) || \
    (ERROR_OUT(("evaluation failed '%s'", (PCSTR)#exp)), 0))

#else

#define EVAL(exp) \
   ((exp) != 0)

#endif   /* DEBUG */

