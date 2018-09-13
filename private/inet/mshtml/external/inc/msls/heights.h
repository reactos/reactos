#ifndef HEIGHTS_DEFINED
#define HEIGHTS_DEFINED

#include "lsdefs.h"
#include "pheights.h"

#define dvHeightIgnore 0x7FFFFFFF

typedef struct heights
{
	long dvAscent;
	long dvDescent;
	long dvMultiLineHeight;
} HEIGHTS;

#endif /* !HEIGHTS_DEFINED */
