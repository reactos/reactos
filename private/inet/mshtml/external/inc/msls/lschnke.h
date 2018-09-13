#ifndef LSCHNKE_DEFINED
#define LSCHNKE_DEFINED

#include "lsdefs.h"
#include "plschp.h"
#include "plsrun.h"
#include "pdobj.h"

typedef struct lschnke
{
	LSCP cpFirst;
	LSDCP dcp;
	PCLSCHP plschp;
	PLSRUN plsrun;
	PDOBJ pdobj;
} LSCHNKE;

typedef LSCHNKE* PLSCHNK;
typedef const LSCHNKE* PCLSCHNK;

#endif  /* !LSCHNKE_DEFINED   */
