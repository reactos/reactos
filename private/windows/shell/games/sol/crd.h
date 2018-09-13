#include "cdt.h"


typedef INT CD;

/* CaRD struct, this is what a card be */
typedef struct _crd
	{
	WORD cd  : 15;		/* card # (1..52) */
	WORD fUp : 1;		/* is this card up/down */
	PT pt;					/* upper-left corner of card */
	} CRD;




/* WARNING: Order of su's is assumed */
#define suClub 0
#define suDiamond 1
#define suHeart 2
#define suSpade 3
#define suMax 4
#define suFirst suClub

#define raAce 0
#define raDeuce 1
#define raTres 2
#define raFour 3
#define raFive 4
#define raSix 5
#define raSeven 6
#define raEight 7
#define raNine 8
#define raTen 9
#define raJack 10
#define raQueen 11
#define raKing 12
#define raMax 13
#define raNil 15
#define raFirst raAce

typedef INT RA;
typedef INT SU;

#define cdNil 0x3c


#define cIDFACEDOWN (IDFACEDOWNLAST-IDFACEDOWNFIRST+1)

#define SuFromCd(cd) ((cd)&0x03)
#define RaFromCd(cd) ((cd)>>2)
#define Cd(ra, su) (((ra)<<2)|(su))

