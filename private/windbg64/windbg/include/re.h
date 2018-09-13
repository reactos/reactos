/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Re.h

Abstract:

    Common include files for regular expression compilers.

Author:

    David J. Gilman (davegi) 04-May-1992

Environment:

    Win32, User Mode

--*/

#if ! defined( _RE_ )
#define _RE_

#include "remi.h"

#define INTERNAL    near

extern RE_OPCODE *REip;            /* instruction pointer to compiled    */
extern struct patType *REPat;          /* pointer to pattern being compiled  */
extern int REArg;              /* count of tagged args parsed        */

/* defined actions for parsing */

typedef  int OPTYPE ;

#define ACTIONMIN   ((OPTYPE) 0)

#define PROLOG      ((OPTYPE) 0)
#define LEFTARG     ((OPTYPE) 1)
#define RIGHTARG    ((OPTYPE) 2)
#define SMSTAR      ((OPTYPE) 3)
#define SMSTAR1     ((OPTYPE) 4)
#define STAR        ((OPTYPE) 5)
#define STAR1       ((OPTYPE) 6)
#define ANY     ((OPTYPE) 7)
#define BOL     ((OPTYPE) 8)
#define EOL     ((OPTYPE) 9)
#define NOTSIGN     ((OPTYPE) 10)
#define NOTSIGN1    ((OPTYPE) 11)
#define LETTER      ((OPTYPE) 12)
#define LEFTOR      ((OPTYPE) 13)
#define ORSIGN      ((OPTYPE) 14)
#define RIGHTOR     ((OPTYPE) 15)
#define CCLBEG      ((OPTYPE) 16)
#define CCLNOT      ((OPTYPE) 17)
#define RANGE       ((OPTYPE) 18)
#define EPILOG      ((OPTYPE) 19)
#define PREV        ((OPTYPE) 20)

#define RANGEDBCS1  ((OPTYPE) 21)
#define RANGEDBCS2  ((OPTYPE) 22)

#define ACTIONMAX   ((OPTYPE) 22)



/*  function forward declarations */

char             fREMatch (struct patType *,char *,char *,char );
struct patType *     RECompile (char *, flagType, flagType);
char             REGetArg (struct patType *,int ,char *);
char             RETranslate (struct patType *,char *,char *);
int          RETranslateLength (struct patType *,char *);
int          RELength (struct patType *,int );
char *           REStart (struct patType *);

typedef UINT_PTR INTERNAL ACT (OPTYPE, UINT_PTR,
                   unsigned char, unsigned char);

typedef ACT *PACT;

ACT          CompileAction;
ACT          EstimateAction;
ACT          NullAction;

int pascal  INTERNAL RECharType (char *);
int pascal  INTERNAL RECharLen (char *);
int pascal  INTERNAL REClosureLen (char *);
char *  pascal  INTERNAL REParseRE (PACT, char *,int *);
char *  pascal  INTERNAL REParseE (PACT,char *);
char *  pascal  INTERNAL REParseSE (PACT,char *);
char *  pascal  INTERNAL REParseClass (PACT,char *);
char *  pascal  INTERNAL REParseAny (PACT,char *);
char *  pascal  INTERNAL REParseBOL (PACT,char *);
char *  pascal  INTERNAL REParsePrev (PACT, char *);
char *  pascal  INTERNAL REParseEOL (PACT,char *);
char *  pascal  INTERNAL REParseAlt (PACT,char *);
char *  pascal  INTERNAL REParseNot (PACT,char *);
char *  pascal  INTERNAL REParseAbbrev (PACT,char *);
char *  pascal  INTERNAL REParseChar (PACT,char *);
char *  pascal  INTERNAL REParseClosure (PACT,char *);
char *  pascal  INTERNAL REParseGreedy (PACT,char *);
char *  pascal  INTERNAL REParsePower (PACT,char *);
char    pascal  INTERNAL REClosureChar (char *);
char    pascal  INTERNAL Escaped (char );

void    pascal  INTERNAL REStackOverflow (void);
void    pascal  INTERNAL REEstimate (char *);

#ifdef DEBUG
void INTERNAL REDump (struct patType *p);
#endif

#if defined(KANJI)
#define REis_kanji(c) (REKTab[(c)>>3] & REBTab[(c)&7])
#endif

#endif // _RE_
