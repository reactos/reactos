/***
 **
 **   Module: TItoTT
 **
 **   Description:
 **      This is the internal interface for the Adobe Type 1 to
 **      TrueType font converter.
 **
 **   Author: Michael Jansson
 **   Created: 5/26/93
 **
 ***/


#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif

#define MAXERRORS          -14
#define NOCOPYRIGHT        -13
#define ARGSTACK           -12
#define TTSTACK            -11
#define NOMETRICS          -10
#define UNSUPPORTEDFORMAT  -9
#define BADMETRICS         -8
#define BADT1HYBRID        -7
#define BADCHARSTRING      -6
#define BADINPUTFILE       -5
#define BADOUTPUTFILE      -4
#define BADT1HEADER        -3
#define NOMEM              -2
#define FAILURE            -1
#define SUCCESS            0
#define DONE               1
#define SKIP               2

struct GlyphFilter {
   const unsigned short num;
   const char **name;
};

struct T1Arg {
   const char *name;
   const char *metrics;
   const struct GlyphFilter *filter;
   int upem;
};

struct TTArg {
   char *name;
   char *tag;
   short precision;
};

struct callProgress {
   const void (*cb)(short, void *, void *);
   void *arg;
};
   

/***
 ** Function: ConvertT1toTT
 **
 ** Description:
 **   Convert a T1 font into a TT font file.
 ***/
short ConvertT1toTT _ARGS((IN struct TTArg *ttArg,
                           IN struct T1Arg *t1Arg,
                           IN short (*check)(IN char *facename,
                                             IN char *copyright,
                                             IN char *notice),
                           INOUT struct callProgress *cp));
