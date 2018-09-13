/*  reparse.c - parse a regular expression
 *
 *  cl /c /Zep /AM /NT RE /Gs /G2 /Oa /D LINT_ARGS /Fc reparse.c
 *
 *  Modifications:
 *
 *      22-Jul-1986 mz  Hookable allocator (allow Z to create enough free space)
 *      19-Nov-1986 mz  Add RETranslateLength for Z to determine overflows
 *      18-Aug-1987 mz  Add field width and justification in translations
 *      01-Mar-1988 mz  Add in UNIX-like syntax
 *      14-Jun-1988 mz  Fix file parts allowing backslashes
 *      04-Dec-1989 bp  Let :p accept uppercase drive names
 *      20-Dec-1989 ln  capture trailing periods in :p
 *      23-Jan-1990 ln  Handle escaped characters & invalid trailing \ in
 *                      RETranslate.
 *      05-Feb-1991 mz  Merged in KANJI stuff
 *
 */

#include "precomp.h"
#pragma hdrstop

#include  <string.h>   // Move(): memmove()
                       // Fill(): memset()



char *REmalloc (size_t size);


#if DEBUG
#define DEBOUT(x)   printf x; fflush (stdout)
#else
#define DEBOUT(x)
#endif


/*  regular expression compiler.  A regular expression is compiled into pseudo-
 *  machine code.  The principle is portable to other machines and is outlined
 *  below.  We parse by recursive descent.
 *
 *  The pseudo-code is fairly close to normal assembler and can be easily
 *  converted to be real machine code and has been done for the 80*86
 *  processor family.
 *
 *  The basic regular expressions handled are:
 *
 *      letter      matches a single letter
 *      [class]     matches a single character in the class
 *      [~class]    matches a single character not in the class
 *      ^           matches the beginning of the line
 *      $           matches the end of the line
 *      ?           matches any character (except previous two)
 *      \x          literal x
 *      \n          matches the previously tagged/matched expression (n digit)
 *
 *  Regular expressions are now build from the above via:
 *
 *      x*          matches 0 or more x, matching minimal number
 *      x+          matches 1 or more x, matching minimal number
 *      x@          matches 0 or more x, matching maximal number
 *      x#          matches 1 or more x, matching maximal number
 *      (x1!x2!...) matches x1 or x2 or ...
 *      ~x          matches 0 characters but prevents x from occuring
 *      {x}         identifies an argument
 *
 *  The final expression that is matched by the compiler is:
 *
 *      xy          matches x then y
 *
 *
 *  The actual grammar used is:                     Parsing action:
 *
 *      TOP ->  re                                  PROLOG .re. EPILOG
 *
 *
 *      re ->   { re } re   |                       LEFTARG .re. RIGHTARG
 *              e re        |
 *              empty
 *
 *      e ->    se *        |                       SMSTAR .se. SMSTAR1
 *              se +        |
 *              se @        |                       STAR .se. STAR1
 *              se #        |
 *              se
 *
 *      se ->   ( alt )     |
 *              [ ccl ]     |
 *              ?           |                       ANY
 *              ^           |                       BOL
 *              $           |                       EOL
 *              ~ se        |                       NOTSIGN .se. NOTSIGN1
 *              :x          |
 *              \n          |                       PREV
 *              letter                              LETTER x
 *
 *      alt ->  re ! alt    |                       LEFTOR .re. ORSIGN
 *              re                                  LEFTOR .re. ORSIGN RIGHTOR
 *
 *      ccl ->  ~ cset      |                       CCLBEG NOTSIGN .cset. CCLEND
 *              cset                                CCLBEG NULL .cset. CCLEND
 *
 *      cset -> item cset   |
 *              item
 *
 *      item -> letter - letter |                   RANGE x y
 *              letter                              RANGE x x
 *
 *  Abbreviations are introduced by :.
 *
 *      :a      [a-zA-Z0-9]                             alphanumeric
 *      :b      ([<space><tab>]#)                       whitespace
 *      :c      [a-zA-Z]                                alphabetic
 *      :d      [0-9]                                   digit
 *      :f      ([~/\\ "\[\]\:<|>+=;,.]#)               file part
 *      :h      ([0-9a-fA-F]#)                          hex number
 *      :i      ([a-zA-Z_$][a-zA-Z0-9_$]@)              identifier
 *      :n      ([0-9]#.[0-9]@![0-9]@.[0-9]#![0-9]#)    number
 *      :p      (([A-Za-z]\:!)(\\!)(:f(.:f!)(\\!/))@:f(.:f!.!)) path
 *      :q      ("[~"]@"!'[~']@')                       quoted string
 *      :w      ([a-zA-Z]#)                             word
 *      :z      ([0-9]#)                                integer
 *
 */

extern  char XLTab[256];                /* lower-casing table                */

static  BOOL  RE__hasBeenInitialized = 0;
static  void  RE__ModuleInitialize (void);

/*  There are several classes of characters:
 *
 *  Closure characters are suffixes that indicate repetition of the previous
 *  RE.
 *
 *  Simple RE chars are characters that indicate a particular type of match
 *
 */

/*  Closure character equates
 */
#define CC_SMPLUS       0               /* plus closure                      */
#define CC_SMCLOSURE    1               /* star closure                      */
#define CC_POWER        2               /* n repetitions of previous pattern */
#define CC_CLOSURE      3               /* greedy closure                    */
#define CC_PLUS         4               /* greedy plus                       */
#define CC_EMPTY        5
#define CC_ERROR        -1

/*  Simple RE character equates */
#define SR_BOL          0
#define SR_EOL          1
#define SR_ANY          2
#define SR_CCLBEG       3
#define SR_LEFTOR       4
#define SR_CCLEND       5
#define SR_ABBREV       6
#define SR_RIGHTOR      7
#define SR_ORSIGN       8
#define SR_NOTSIGN      9
#define SR_LEFTARG      10
#define SR_RIGHTARG     11
#define SR_LETTER       12
#define SR_PREV         13

int EndAltRE[] =    { SR_ORSIGN, SR_RIGHTOR, -1 };
int EndArg[] =      { SR_RIGHTARG, -1 };

char *pAbbrev[] = {
         "a[a-zA-Z0-9]",
         "b([ \t]#)",
         "c[a-zA-Z]",
         "d[0-9]",
         "f([~/\\\\ \\\"\\[\\]\\:<|>+=;,.]#!..!.)",
         "h([0-9a-fA-F]#)",
         "i([a-zA-Z_$][a-zA-Z0-9_$]@)",
         "n([0-9]#.[0-9]@![0-9]@.[0-9]#![0-9]#)",
         "p(([A-Za-z]\\:!)(\\\\!/!)(:f(.:f!)(\\\\!/))@:f(.:f!.!))",
         "q(\"[~\"]@\"!'[~']@')",
         "w([a-zA-Z]#)",
         "z([0-9]#)",
         NULL
};

static char *digits = "0123456789";

static flagType fZSyntax = TRUE;        /* TRUE => use Z syntax for things */

static int cArg;

#if defined(KANJI)

/*  Lead byte test for KANJI.  Since Kanji has a lead byte in the range
 *  0x81-0xA0 and 0xE0-0xFC we have a bit table to test for presence in these
 *  ranges.
 */
unsigned char REKTab[32] = {    0x00,   0x00,   /*  0 .. F  */
                                0x00,   0x00,   /* 10 .. 1F */
                                0x00,   0x00,   /* 20 .. 2F */
                                0x00,   0x00,   /* 30 .. 3F */
                                0x00,   0x00,   /* 40 .. 4F */
                                0x00,   0x00,   /* 50 .. 5F */
                                0x00,   0x00,   /* 60 .. 6F */
                                0x00,   0x00,   /* 70 .. 7F */
                                0x7f,   0xff,   /* 80 .. 8F */
                                0xff,   0xff,   /* 90 .. 9F */
                                0x00,   0x00,   /* A0 .. AF */
                                0x00,   0x00,   /* B0 .. BF */
                                0x00,   0x00,   /* C0 .. CF */
                                0x00,   0x00,   /* D0 .. DF */
                                0xff,   0xff,   /* E0 .. EF */
                                0xff,   0xf8    /* F0 .. FF */
                                                                        };

unsigned char REBTab[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

#endif

/*  RECharType - classify a character type
 *
 *  p           character pointer
 *
 *  returns     type of character (SR_xx)
 */
int pascal INTERNAL RECharType (char *p)
{
         if (fZSyntax)
        /*  Zibo syntax
         */
        switch (*p) {
        case '^':
                 return SR_BOL;
        case '$':
                 if (isdigit (p[1]))
                return SR_PREV;
                 else
                return SR_EOL;
        case '?':
                 return SR_ANY;
        case '[':
                 return SR_CCLBEG;
        case '(':
                 return SR_LEFTOR;
        case ']':
                 return SR_CCLEND;
        case ':':
                 return SR_ABBREV;
        case ')':
                 return SR_RIGHTOR;
        case '!':
                 return SR_ORSIGN;
        case '~':
                 return SR_NOTSIGN;
        case '{':
                 return SR_LEFTARG;
        case '}':
                 return SR_RIGHTARG;
        default:
                 return SR_LETTER;
                 }
         else
        /*  UNIX syntax
         */
        switch (*p) {
        case '^':
                 return SR_BOL;
        case '$':
                 return SR_EOL;
        case '.':
                 return SR_ANY;
        case '[':
                 return SR_CCLBEG;
        case ']':
                 return SR_CCLEND;
        case '\\':
                 switch (p[1]) {
                 case ':':                          /*  \:C */
                return SR_ABBREV;
                 case '(':                          /*  \(  */
                return SR_LEFTARG;
                 case ')':                          /*  \)  */
                return SR_RIGHTARG;
                 case '~':                          /*  \~  */
                return SR_NOTSIGN;
                 case '{':                          /*  \{  */
                return SR_LEFTOR;
                 case '}':                          /*  \}  */
                return SR_RIGHTOR;
                 case '!':                          /*  \!  */
                return SR_ORSIGN;
                }
                 if (isdigit (p[1]))        /*  \N  */
                return SR_PREV;
        default:
                 return SR_LETTER;
                 }
}

/*  RECharLen - length of character type
 *
 *  p           character pointer to type
 *
 *  returns     length in chars of type
 */
int pascal INTERNAL RECharLen (char *p)
{
         if (fZSyntax)
        if (RECharType (p) == SR_PREV)      /*  $N  */
                 return 2;
        else
        if (RECharType (p) == SR_ABBREV)    /*  :N  */
                 return 2;
        else
                 return 1;
         else {
        if (*p == '\\')
                 switch (p[1]) {
                 case '{':
                 case '}':
                 case '~':
                 case '(':
                 case ')':
                 case '!':
                return 2;                   /*  \C  */
                 case ':':                          /*  \:C */
                return 3;
                 default:
                if (isdigit (p[1]))
                         return 2;                  /*  \N  */
                else
                         return 1;
                }
        return 1;
        }
}

/*  REClosureLen - length of character type
 *
 *  p           character pointer to type
 *
 *  returns     length in chars of type
 */
int pascal INTERNAL REClosureLen (char *p)
{
         return 1;
}

/*  REParseRE - parse a general RE up to but not including the pEnd set
 *  of chars.  Apply a particular action to each node in the parse tree.
 *
 *  pAction     Parse action routine to call at particluar points in the
 *              parse tree.  This routine returns an unsigned quantity that
 *              is expected to be passed on to other action calls within the
 *              same node.
 *  p           character pointer to string being parsed
 *  pEnd        pointer to set of char types that end the current RE.
 *              External callers will typically use NULL for this value.
 *              Internally, however, we need to break on the ALT-terminating
 *              types or on arg-terminating types.
 *
 *  Returns:    pointer to delimited character if successful parse
 *              NULL if unsuccessful parse (syntax error).
 *
 */
char * pascal INTERNAL REParseRE (PACT pAction, register char *p, int *pEnd)
{
         int *pe;
         UINT_PTR u;

         DEBOUT (("REParseRE (%04x, %s)\n", pAction, p));

         while (TRUE) {
        /*  If we're at end of input
         */
        if (*p == '\0')
                 /*     If we're not in the midst of an open expression
                  */
                 if (pEnd == NULL)
                /*  return the current parse position
                 */
                return p;
                 else {
                /*  End of input, but expecting more, ERROR
                 */
                DEBOUT (("REParse expecting more, ERROR\n"));
                return NULL;
                }

        /*  If there is an open expression
         */
        if (pEnd != NULL)
                 /*     Find a matching character
                  */
                 for (pe = pEnd; *pe != -1; pe++)
                if (RECharType (p) == *pe)
                         return p;

        /*  If we are looking at a left argument
         */
        if (RECharType (p) == SR_LEFTARG) {
                 /*     Parse LEFTARG .re. RIGHTARG
                  */
                 u = (*pAction) (LEFTARG, 0, 0, 0);
                 if ((p = REParseRE (pAction, p + RECharLen (p), EndArg)) == NULL)
                return NULL;
                 (*pAction) (RIGHTARG, u, 0, 0);
                 cArg++;
                 p += RECharLen (p);
                 }
        else
        /*  Parse .e.
         */
        if ((p = REParseE (pAction, p)) == NULL)
                 return NULL;
        }
}

/*  REParseE - parse a simple regular expression with potential closures.
 *
 *  pAction     Action to apply at special parse nodes
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseE (PACT pAction, register char *p)
{
         DEBOUT (("REParseE (%04x, %s)\n", pAction, p));

         switch (REClosureChar (p)) {
         case CC_SMPLUS:
        if (REParseSE (pAction, p) == NULL)
                 return NULL;
         case CC_SMCLOSURE:
        return REParseClosure (pAction, p);

         case CC_PLUS:
        if (REParseSE (pAction, p) == NULL)
                 return NULL;
         case CC_CLOSURE:
        return REParseGreedy (pAction, p);

         case CC_POWER:
        return REParsePower (pAction, p);

         case CC_EMPTY:
        return REParseSE (pAction, p);

         default:
        return NULL;
        }
}

/*  REParseSE - parse a simple regular expression
 *
 *  pAction     Action to apply at special parse nodes
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseSE (register PACT pAction, register char *p)
{
         DEBOUT (("REParseSE (%04x, %s)\n", pAction, p));

         switch (RECharType (p)) {
         case SR_CCLBEG:
        return REParseClass (pAction, p);
         case SR_ANY:
        return REParseAny (pAction, p);
         case SR_BOL:
        return REParseBOL (pAction, p);
         case SR_EOL:
        return REParseEOL (pAction, p);
         case SR_PREV:
        return REParsePrev (pAction, p);
         case SR_LEFTOR:
        return REParseAlt (pAction, p);
         case SR_NOTSIGN:
        return REParseNot (pAction, p);
         case SR_ABBREV:
        return REParseAbbrev (pAction, p);
         default:
        return REParseChar (pAction, p);
        }
}

/*  REParseClass - parse a class membership match
 *
 *  pAction     Action to apply at beginning of parse and at each range
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseClass (PACT pAction, register char *p)
{
         char c;
         char c2, c3, c4;
         UINT_PTR u;

         DEBOUT (("REParseClass (%04x, %s)\n", pAction, p));

         p += RECharLen (p);
         if ((fZSyntax && *p == '~') || (!fZSyntax && *p == '^')) {
        u = (*pAction) (CCLNOT, 0, 0, 0);
        p += RECharLen (p);
        }
         else
        u = (*pAction) (CCLBEG, 0, 0, 0);

         while (RECharType (p) != SR_CCLEND) {
        if (*p == '\\')
                 p++;
        if (*p == '\0') {
                 DEBOUT (("REParseClass expecting more, ERROR\n"));
                 return NULL;
                 }
        c = *p++;

        if (IsDBCSLeadByte((BYTE)c))
            c2 = *p++;
        else {
            c2 = c;
            c = 0;
        }
        if (*p == '-') {
                 p++;
                 if (*p == '\\')
                p++;
                 if (*p == '\0') {
                DEBOUT (("REParseClass expecting more, ERROR\n"));
                return NULL;
                }
                c3 = *p;
                if (IsDBCSLeadByte(*(unsigned char *)p))
                    c4 = *++p;
                else {
                    c4 = c3;
                    c3 = 0;
                }
                if ( (c == 0 && c3 == 0) || (c != 0 && c3 != 0) ) {
                    u = (*pAction) (RANGEDBCS1, 0, c, c2);
                    (*pAction) (RANGEDBCS2, u, c3, c4);
                }
                else
                    return NULL;
                 p++;
                 }
                  else
#if defined(KANJI)
                                {
                 u = (*pAction) (RANGEJ1, 0, c, c2);
                 (*pAction) (RANGEJ2, u, c, c2);
                                }
#else
                                (*pAction) (RANGE, u, c, c);
#endif
                  }
         c = 0;
         u = (*pAction) (RANGEDBCS1, 0, c, c);
         (*pAction) (RANGEDBCS2, u, c, c);
         return p + RECharLen (p);
}

/*  REParseAny - parse a match-any-character expression
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseAny (PACT pAction, char *p)
{
         DEBOUT (("REParseAny (%04x, %s)\n", pAction, p));

         (*pAction) (ANY, 0, 0, 0);
         return p + RECharLen (p);
}

/*  REParseBOL - parse a beginning-of-line match
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseBOL (PACT pAction, char *p)
{
         DEBOUT (("REParseBOL (%04x, %s)\n", pAction, p));

         (*pAction) (BOL, 0, 0, 0);
         return p + RECharLen (p);
}

/*  REParsePrev - parse a previous-match item
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParsePrev (PACT pAction, char *p)
{
         UINT_PTR i = *(p + 1) - '0';

         DEBOUT (("REParsePrev (%04x, %s)\n", pAction, p));

         if (i < 1 || i > (unsigned) cArg) {
        DEBOUT (("REParsePrev invalid previous number, ERROR\n"));
        return NULL;
        }

         (*pAction) (PREV, i, 0, 0);
         return p + RECharLen (p);
}

/*  REParseEOL - parse an end-of-line match
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseEOL (PACT pAction, char *p)
{
         DEBOUT (("REParseEOL (%04x, %s)\n", pAction, p));

         (*pAction) (EOL, 0, 0, 0);
         return p + RECharLen (p);
}

/*  REParseAlt - parse a series of alternatives
 *
 *  pAction     Action to apply before and after each alternative
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseAlt (PACT pAction, register char *p)
{
         UINT_PTR u = 0;

         DEBOUT (("REParseAlt (%04x, %s)\n", pAction, p));

         while (RECharType (p) != SR_RIGHTOR) {
        p += RECharLen (p);
        u = (*pAction) (LEFTOR, u, 0, 0);
        if ((p = REParseRE (pAction, p, EndAltRE)) == NULL)
                 return NULL;
        u = (*pAction) (ORSIGN, u, 0, 0);
        }
         (*pAction) (RIGHTOR, u, 0, 0);
         return p + RECharLen (p);
}

/*  REParseNot - parse a guard-against match
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseNot (PACT pAction, register char *p)
{
         UINT_PTR u;

         DEBOUT (("REParseNot (%04x, %s)\n", pAction, p));

         p += RECharLen (p);
         if (*p == '\0') {
        DEBOUT (("REParseNot expecting more, ERROR\n"));
        return NULL;
        }
         u = (*pAction) (NOTSIGN, 0, 0, 0);
         p = REParseSE (pAction, p);
         (*pAction) (NOTSIGN1, u, 0, 0);
         return p;
}

/*  REParseAbbrev - parse and expand an abbreviation
 *
 *  Note that since the abbreviations are in Z syntax, we must change syntax
 *  temporarily to Z.  We are careful to do this so that we do not mess up
 *  advancign the pointers.
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseAbbrev (PACT pAction, register char *p)
{
         int i;
         flagType fZSTmp;

         DEBOUT (("REParseAbbrev (%04x, %s)\n", pAction, p));

         p += RECharLen (p);

         fZSTmp = fZSyntax;
         fZSyntax = TRUE;
         if (p[-1] == '\0') {
        DEBOUT (("REParseAbbrev expecting abbrev char, ERROR\n"));
        fZSyntax = fZSTmp;
        return NULL;
        }

         for (i = 0; pAbbrev[i]; i++)
        if (p[-1] == *pAbbrev[i])
                 if (REParseSE (pAction, pAbbrev[i] + 1) == NULL) {
                fZSyntax = fZSTmp;
                return NULL;
                }
                 else {
                fZSyntax = fZSTmp;
                return p;
                }
         DEBOUT (("REParseAbbrev found invalid abbrev char %s, ERROR\n", p - 1));
         fZSyntax = fZSTmp;
         return NULL;
}

/*  REParseChar - parse a single character match
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseChar (PACT pAction, register char *p)
{
         DEBOUT (("REParseChar (%04x, %s)\n", pAction, p));

         if (*p == '\\')
        p++;
         if (*p == '\0') {
        DEBOUT (("REParseChar expected more, ERROR\n"));
        return NULL;
                  }
        if ( IsDBCSLeadByte ((BYTE)*p) ) {
            (*pAction) (LETTER, 0, *p, *(p+1));
            return p+2;
        }
        else {
            (*pAction) (LETTER, 0, *p, 0);
            return p+1;
        }
}

/*  REParseClosure - parse a minimal match closure.  The match occurs by
 *  matching none, then one, ...
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseClosure (PACT pAction, register char *p)
{
         UINT_PTR u;

         DEBOUT (("REParseaClosure (%04x, %s)\n", pAction, p));

         u = (*pAction) (SMSTAR, 0, 0, 0);
         if ((p = REParseSE (pAction, p)) == NULL)
        return NULL;
         (*pAction) (SMSTAR1, u, 0, 0);
         return p + REClosureLen (p);
}

/*  REParseGreedy - parse a maximal-match closure.  The match occurs by
 *  matching the maximal number and then backing off as failures occur.
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParseGreedy (PACT pAction, register char *p)
{
         UINT_PTR u;

         DEBOUT (("REParseGreedy (%04x, %s)\n", pAction, p));

         u = (*pAction) (STAR, 0, 0, 0);
         if ((p = REParseSE (pAction, p)) == NULL)
        return NULL;
         (*pAction) (STAR1, u, 0, 0);
         return p + REClosureLen (p);
}

/*  REParsePower -  parse a power-closure.  This is merely the simple pattern
 *  repeated the number of times specified by the exponent.
 *
 *  pAction     Action to apply
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     pointer past parsed text if successful
 *              NULL otherwise (syntax error)
 */
char * pascal INTERNAL REParsePower (PACT pAction, char *p)
{
         register char *p1;
         int exp;

         DEBOUT (("REParsePower (%04x, %s)\n", pAction, p));

         /*     We have .se. POWER something.  Skip over the .se. and POWER
          *     to make sure that what follows is a valid number
          */
         p1 = REParseSE (NullAction, p);

         if (p1 == '\0')
        /*  Parse of .se. failed
         */
        return NULL;

         /*     skip POWER
          */
         p1 += REClosureLen (p1);

         if (*p1 == '\0') {
        DEBOUT (("REParsePower expecting more, ERROR\n"));
        return NULL;
        }

         /* try to parse off number */
         if (sscanf (p1, "%d", &exp) != 1) {
        DEBOUT (("REParsePower expecting number, ERROR\n"));
        return NULL;
        }

         p1 = strbskip (p1, digits);

         /* iterate the pattern the exponent number of times */
         while (exp--)
        if (REParseSE (pAction, p) == NULL)
                 return NULL;
         return p1;
}

/*  NullAction - a do-nothing action.  Used for stubbing out the action
 *  during a parse.
 */
UINT_PTR  INTERNAL NullAction (OPTYPE type, UINT_PTR u, 
    unsigned char x, unsigned char y)
{
         type; u; x; y;
         return 0;
}

/*  REClosureChar - return the character that corresponds to the next
 *  closure to be parsed.  We call REParseSE with a null action to merely
 *  advance the character pointer to point just beyond the current simple
 *  regular expression.
 *
 *  p           character pointer to spot where parsing occurs
 *
 *  Returns     closure character if appropriate
 *              CC_EMPTY if no closure character found.
 */
char  pascal INTERNAL REClosureChar (char *p)
{
         p = REParseSE (NullAction, p);
         if (p == NULL)
        return CC_ERROR;

         if (fZSyntax)
        /*  Zibo syntax
         */
        switch (*p) {
        case '^':
                 return CC_POWER;
        case '+':
                 return CC_SMPLUS;
        case '#':
                 return CC_PLUS;
        case '*':
                 return CC_SMCLOSURE;
        case '@':
                 return CC_CLOSURE;
        default:
                 return CC_EMPTY;
                 }
         else
        /*  UNIX syntax
         */
        switch (*p) {
        case '+':
                 return CC_SMPLUS;
        case '*':
                 return CC_SMCLOSURE;
        default:
                 return CC_EMPTY;
                 }
}

/*  RECompile - compile a pattern into the internal machine.  Return a
 *  pointer to the match machine.
 *
 *  p       character pointer to pattern being compiled
 *
 *  Returns:    pointer to the internal machine if compilation was successful
 *              NULL if syntax error or not enough memory for malloc
 */
struct patType *RECompile (char *p, flagType fCase, flagType fZS)
{
         if (!RE__hasBeenInitialized) { RE__ModuleInitialize(); }

         fZSyntax = fZS;

         REEstimate (p);

         DEBOUT (("Length is %04x\n", RESize));
         if (RESize == -1)
        return NULL;

         if ((REPat = (struct patType *) REmalloc (RESize)) == NULL)
        return NULL;

         Fill ((char far *) REPat, -1, RESize);
         Fill ((char far *) REPat->pArgBeg, 0, sizeof (REPat->pArgBeg));
         Fill ((char far *) REPat->pArgEnd, 0, sizeof (REPat->pArgEnd));

         REip = REPat->code;
         REArg = 1;
         REPat->fCase = fCase;
         REPat->fUnix = (flagType) !fZS;

         cArg = 0;

         CompileAction (PROLOG, 0, 0, 0);

         if (REParseRE (CompileAction, p, NULL) == NULL)
        return NULL;

         CompileAction (EPILOG, 0, 0, 0);

#if DEBUG
         REDump (REPat);
#endif
         return REPat;
}

/*  Escaped - translate an escaped character ala UNIX C conventions.
 *
 *  \t => tab       \e => ESC char  \h => backspace \g => bell
 *  \n => lf        \r => cr        \\ => \
 *
 *  c       character to be translated
 *
 *  Returns:    character as per above
 */
char  pascal INTERNAL Escaped (char c)
{
         switch (c) {
         case 't':
        return '\t';
         case 'e':
        return 0x1B;
         case 'h':
        return 0x08;
         case 'g':
        return 0x07;
         case 'n':
        return '\n';
         case 'r':
        return '\r';
         case '\\':
        return '\\';
         default:
        return c;
        }
}

/*  REGetArg - copy argument string out from match.
 *
 *  pat     matched pattern
 *  i       index of argument to fetch, 0 is entire pattern
 *  p       destination of argument
 *
 *  Returns:    TRUE if successful, FALSE if i is out of range.
 */
flagType REGetArg (struct patType *pat, int i, char *p)
{
         int l = 0;

         if (i > MAXPATARG)
        return FALSE;
         else
         if (pat->pArgBeg[i] != (char *)-1)
        Move ((char far *)pat->pArgBeg[i], (char far *)p, l = RELength (pat, i));
         p[l] = '\0';
         return TRUE;
}

/*  RETranslate - translate a pattern string and match structure into an
 *  output string.  During pattern search-and-replace, RETranslate is used
 *  to generate an output string based on an input match pattern and a template
 *  that directs the output.
 *
 *  The input match is any patType returned from RECompile that has been passed
 *  to fREMatch and that causes fREMatch to return TRUE.  The template string
 *  is any set of ascii chars.  The $ character leads in arguments:
 *
 *      $$ is replaced with $
 *      $0 is replaced with the entire match string
 *      $1-$9 is replaced with the corresponding tagged (by {}) item from
 *          the match.
 *
 *  An alternative method is to specify the argument as:
 *
 *      $([w,]a) where a is the argument number (0-9) and w is an optional field
 *          width that will be used in a printf %ws format.
 *
 *  buf     pattern matched
 *  src     template for the match
 *  dst     destination of the translation
 *
 *  Returns:    TRUE if translation was successful, FALSE otherwise
 */
flagType RETranslate (struct patType *buf, register char *src, register char *dst)
{
         int i, w;
         char *work;
         char chArg = (char) (buf->fUnix ? '\\' : '$');

         work = REmalloc (MAXLINELEN);

         if (work == NULL)
             return FALSE;

         *dst = '\0';

         while (*src != '\0') {
        /*  Process tagged substitutions first
         */
        if (*src == chArg && (isdigit (src[1]) || src[1] == '(')) {
                 /*     presume 0-width field */
                 w = 0;

                 /*     skip $ and char */
                 src += 2;

                 /*     if we saw $n */
                 if (isdigit (src[-1]))
                i = src[-1] - '0';
                 /*     else we saw $( */
                 else {
                /*  get tagged expr number */
                i = atoi (src);

                /*  skip over number */
                if (*src == '-')
                         src++;
                src = strbskip (src, digits);

                /*  was there a comma? */
                if (*src == ',') {
                         /*     We saw field width, parse off expr number */
                         w = i;
                         i = atoi (++src);
                         src = strbskip (src, digits);
                         }

                /*  We MUST end with a close paren */
                if (*src++ != ')') {
                         free (work);
                         return FALSE;
                         }
                }
                 /*     w is field width
                  *     i is selected argument
                  */
                 if (!REGetArg (buf, i, work)) {
                free (work);
                return FALSE;
                }
                 sprintf (dst, "%*s", w, work);
                 dst += strlen (dst);
                 }
        else
        /* process escaped characters */
        if (*src == '\\') {
                 src++;
                 if (!*src) {
                free (work);
                return FALSE;
                }
                 *dst++ = Escaped (*src++);
                 }
        else
        /*  chArg quotes itself */
        if (*src == chArg && src[1] == chArg) {
                 *dst++ = chArg;
                 src += 2;
                 }
        else
            if (IsDBCSLeadByte(*src) && *(src+1)) {
                 *dst++ = *src++;
                 *dst++ = *src++;
            }
            else
                 *dst++ = *src++;
        }
         *dst = '\0';
         free (work);
         return TRUE;
}

/*  RETranslateLength - given a matched pattern and a replacement string
 *  return the length of the final replacement
 *
 *  The inputs have the same syntax/semantics as in RETranslate.
 *
 *  buf     pattern matched
 *  src     template for the match
 *
 *  Returns:    number of bytes in total replacement, -1 if error
 */
int RETranslateLength (struct patType *buf, register char *src)
{
         int i, w;
         int length = 0;
         char chArg = (char) (buf->fUnix ? '\\' : '$');

         while (*src != '\0') {
        /*  Process tagged substitutions first
         */
        if (*src == chArg && (isdigit (src[1]) || src[1] == '(')) {
                 w = 0;
                 src += 2;
                 if (isdigit (src[-1]))
                i = src[-1] - '0';
                 else {
                i = atoi (src);
                if (*src == '-')
                         src++;
                src = strbskip (src, digits);
                if (*src == ',') {
                         w = i;
                         i = atoi (++src);
                         src = strbskip (src, digits);
                         }
                if (*src++ != ')')
                         return -1;
                }
                 /*     w is field width
                  *     i is selected argument
                  */
                 i = RELength (buf, i);
                 length += max (i, abs(w));
                 }
        else
        /* process escaped characters */
        if (*src == '\\') {
                 src += 2;
                 length++;
                 }
        else
        /*  chArg quotes itself */
        if (*src == chArg && src[1] == chArg) {
                 src += 2;
                 length++;
                 }
        else
          if (IsDBCSLeadByte(*src) && *(src+1)) {
                 length += 2;
                 src += 2;
          }
        else {
                 length++;
                 src++;
                 }
        }
         return length;
}

/*  RELength - return length of argument in match.
 *
 *  pat     matched pattern
 *  i       index of argument to examine, 0 is entire pattern
 *
 *  Returns:    length of ith argument, -1 if i is out-of-range.
 */
int RELength (struct patType *pat, int i)
{
         if (i > MAXPATARG)
        return -1;
         else
         if (pat->pArgBeg[i] == (char *)-1)
        return 0;
         else
        return (int) (pat->pArgEnd[i] - pat->pArgBeg[i]);
}

/*  REStart - return pointer to beginning of match.
 *
 *  ppat    matched pattern
 *
 *  Returns:    character pointer to beginning of match
 */
char *REStart (struct patType *pat)
{
         return pat->pArgBeg[0] == (char *)-1 ? NULL : pat->pArgBeg[0];
}


//  void Fill(void FAR * a, char b, unsigned int c) {;}
//  void Move(void FAR * a, void FAR * b, unsigned int c) {;}
char XLTab[256];
char * strbskip(char const * a, char const * b) {return (char *) a;}


void Fill(void FAR * a, char b, unsigned int c)
{
   (void) memset (a, (int)b, c);
   return;
}


void Move(void FAR * a, void FAR * b, unsigned int c)
{
   (void) memmove (b, a, c);
   return;
}



/****************************************************************************
*
* void  RE__ModuleInitialize (void)
*
* "Initialize the Regular Expression module.  Presently, this comprises
*  loading lowercase information into the global(!) array 'XLTab[]'."
*
* Answers:   <nothing>
*
* Requires:  true
*
* Ensures:   The global array 'XLTab[]' has, for each index, the ASCII
*            lowercase equivalent of that index (as defined by invoking
*            'tolower()' on each index value).
*
*            Only the *first* invocation of this method will do the
*            initialization procedure; subsequent invocations are legal
*            but have no effect.
*
* Modifies:  XLTab[]
*            RE__hasBeenInitialized
*
* Raises:    <nothing>
*
* COMMENTS:  There is a companion array 'XUTab[]' which we ignore because
*            the entire system ignores it, also.
*
*            We #include <ctype.h> just to be sure, even though it may
*            have already been pulled in somewhere else (surely *all*
*            header files watch for mulitple inclusions...).
*
*            In keeping with windbg philosophy, we do *not* pay attention
*            to Unicode stuff.
*
\***************************************************************************/

#include <ctype.h>    // RE__ModuleInitialize(): tolower()

static
void
RE__ModuleInitialize (void)
{
   int idxChar;

   if (!RE__hasBeenInitialized)
      {
      for (idxChar = 0; idxChar != sizeof(XLTab); idxChar++)
         {
         XLTab[idxChar] = (char) tolower (idxChar);
         }

      RE__hasBeenInitialized = TRUE;
      }

   return;
}
