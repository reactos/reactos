#ifndef LSCELL_DEFINED
#define LSCELL_DEFINED

#include "lsdefs.h"
#include "plscell.h"
#include "pcelldet.h"
#include "plscell.h"

struct lstextcell
{
	LSCP cpStartCell;
	LSCP cpEndCell;
 	POINTUV pointUvStartCell;		/* In coordinate system of main line/subline */
	long dupCell;					/* In direction lstflowSubline			*/

	DWORD cCharsInCell;
	DWORD cGlyphsInCell;

	PCELLDETAILS pCellDetails;

};

typedef struct lstextcell LSTEXTCELL;

#endif 
