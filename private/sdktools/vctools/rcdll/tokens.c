/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* TOKENS.C - Token stuff, probably removable from RCPP                 */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include <stdio.h>
#include "rcpptype.h"
#include "rcppext.h"
#include "grammar.h"

/*
 * TOKENS - This file contains the initialized tables of text, token pairs for
 * all the C language symbols and keywords, and the mapped value for YACC.
 *
 * IMPORTANT : this MUST be in the same order as the %token list in grammar.y
 *
 */
keytab_t Tokstrings[] = {
#define DAT(tok1, name2, map3, il4, mmap5)      { name2, map3 },
#include "tokdat.h"
#undef DAT
        };
