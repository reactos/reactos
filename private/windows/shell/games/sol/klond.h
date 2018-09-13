

/* Klondike info */

/* Col classes  */
#define tclsDeck 1
#define tclsDiscard 2
#define tclsFound 3
#define tclsTab 4


/* indexes of columns */
#define icolDeck 			0
#define icolDiscard 		1
#define icolFoundFirst	2
#define ccolFound 		4
#define icolTabFirst	   6
#define ccolTab			7


/* BUG! this should be placed in a game descriptor table */
#define icrdDeckMax 52
#define icrdDiscardMax (icrdDeckMax-(1+2+3+4+5+6+7))
#define icrdFoundMax 13
#define icrdTabMax 19





/* Change Score notification codes */
/* WARNING: order is assumed by mpscdsco* in klond.c */
#define csKlondTime			0		/* decrement score with time */
#define csKlondDeckFlip		1		/* deck gone thru 1 or 3 times  */
#define csKlondFound		2		/* new card on foundation */
#define csKlondTab			3		/* card from Deck to tab */
#define csKlondTabFlip		4		/* exposure of new foundation card */
#define csKlondFoundTab		5		/* card from foundation to tab (- pts) */
#define csKlondDeal			6		/* cost of a deal */
#define csKlondWin			7		/* win bonus */

#define csKlondMax csKlondWin+1

VOID OOM( VOID );
VOID DrawCardExt( PT *, INT, INT );
BOOL APIENTRY cdtAnimate( HDC, INT, INT, INT, INT );
VOID NewKbdColAbs( GM *, INT );
BOOL FValidCol( COL * );

