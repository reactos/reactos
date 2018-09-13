#ifndef LSKJUST_DEFINED
#define LSKJUST_DEFINED

enum lskjust							/* kinds of para justification */
{
	lskjNone,
	lskjFullInterWord,
	lskjFullInterLetterAligned,
	lskjFullScaled,
	lskjFullGlyphs,
	lskjSnapGrid
};

typedef enum lskjust LSKJUST;

#endif /* !LSKJUST_DEFINED */
