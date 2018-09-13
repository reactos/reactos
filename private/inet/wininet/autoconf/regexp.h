#ifndef __REGEXP_H__
#define __REGEXP_H__

#include <windows.h>
#include "utils.h"

#define PAT_START	128	/* Special beginning-of-pattern marker */
#define PAT_END		129	/* Special end-of-pattern marker */
#define PAT_STAR	130	/* Zero or more of any character */
#define PAT_QUES	131	/* Exactly one of any character */
#define PAT_AUGDOT	132	/* Literal '.' or end-of-string */
#define PAT_AUGQUES	133	/* Empty string or non-'.' */
#define PAT_AUGSTAR	134	/* Single character that isn't a '.' */

BOOL test_match(int m, LPSTR target, int pattern[]);
BOOL parse_pattern(LPSTR s, int pattern[]);
BOOL match( LPSTR target, LPSTR regexp);

#endif