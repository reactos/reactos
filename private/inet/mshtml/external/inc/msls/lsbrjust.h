#ifndef LSBRJUST_DEFINED
#define LSBRJUST_DEFINED

enum lsbreakjust							/* kinds of breaking/justification */
{
	lsbrjBreakJustify,						/* Regular US */
	lsbrjBreakWithCompJustify,				/* FE & Newspaper */
	lsbrjBreakThenExpand,					/* Arabic			 */
	lsbrjBreakThenSqueeze					/* Word Perfect			 */
};

typedef enum lsbreakjust LSBREAKJUST;

#endif /* !LSBRJUST_DEFINED */

