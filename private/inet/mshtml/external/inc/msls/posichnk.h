#ifndef POSICHNK_DEFINED
#define POSICHNK_DEFINED

#include "lsdefs.h"
#include "pposichn.h"

#define ichnkOutside 0xFFFFFFFF

typedef struct posichnk					/* position in chunk		*/
{
	long ichnk;							/* index in the chunk array	*/
	LSDCP dcp;							/* from beginning of dobj	*/
} POSICHNK;



#endif /* !POSICHNK_DEFINED                    */

