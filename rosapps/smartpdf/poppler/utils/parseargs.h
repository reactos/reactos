/*
 * parseargs.h
 *
 * Command line argument parser.
 *
 * Copyright 1996-2003 Glyph & Cog, LLC
 */

#ifndef PARSEARGS_H
#define PARSEARGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "goo/gtypes.h"

/*
 * Argument kinds.
 */
typedef enum {
  argFlag,			/* flag (present / not-present) */
				/*   [val: GBool *]             */
  argInt,			/* integer arg    */
				/*   [val: int *] */
  argFP,			/* floating point arg */
				/*   [val: double *]  */
  argString,			/* string arg      */
				/*   [val: char *] */
  /* dummy entries -- these show up in the usage listing only; */
  /* useful for X args, for example                            */
  argFlagDummy,
  argIntDummy,
  argFPDummy,
  argStringDummy
} ArgKind;

/*
 * Argument descriptor.
 */
typedef struct {
  char *arg;			/* the command line switch */
  ArgKind kind;			/* kind of arg */
  void *val;			/* place to store value */
  int size;			/* for argString: size of string */
  char *usage;			/* usage string */
} ArgDesc;

/*
 * Parse command line.  Removes all args which are found in the arg
 * descriptor list <args>.  Stops parsing if "--" is found (and removes
 * it).  Returns gFalse if there was an error.
 */
extern GBool parseArgs(ArgDesc *args, int *argc, char *argv[]);

/*
 * Print usage message, based on arg descriptor list.
 */
extern void printUsage(char *program, char *otherArgs, ArgDesc *args);

/*
 * Check if a string is a valid integer or floating point number.
 */
extern GBool isInt(char *s);
extern GBool isFP(char *s);

#ifdef __cplusplus
}
#endif

#endif
