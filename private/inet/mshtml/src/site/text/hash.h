#ifndef I_HASH_H_
#define I_HASH_H_
#pragma INCMSG("--- Beg 'hash.h'")

#ifndef X_TOKENS_H_
#define X_TOKENS_H_
#include "tokens.h"
#endif

#define     HASHSIZE    257 // DO NOT CHANGE!
                        // this prime has been chosen because
                        // there is a fast MOD257
                        // if you use the % operator the thing
                        // slows down to just about what a binary search is.

#define         MOD257(k) ((k) - ((k) & ~255) - ((k) >> 8) )    // MOD 257
#define         MOD257_1(k) ((k) & 255) // MOD (257 - 1)

extern BOOL     _rtfHashInited;
void            HashKeyword_Init( );

void            HashKeyword_Insert ( const KEYWORD *token );
const KEYWORD   *HashKeyword_Fetch ( const CHAR *szKeyword );

#pragma INCMSG("--- End 'hash.h'")
#else
#pragma INCMSG("*** Dup 'hash.h'")
#endif
