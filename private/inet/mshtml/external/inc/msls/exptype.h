#ifndef EXPTYPE_DEFINED
#define EXPTYPE_DEFINED

#include "lsdefs.h"

typedef BYTE EXPTYPE;

/* kinds of glyph expansion */
#define	exptNone  0
#define	exptAddWhiteSpace 1
#define	exptAddInkContinuous 2
#define	exptAddInkDiscrete 3

typedef EXPTYPE* PEXPTYPE;
typedef const EXPTYPE* PCEXPTYPE;


#endif /* !EXPTYPE_DEFINED */
