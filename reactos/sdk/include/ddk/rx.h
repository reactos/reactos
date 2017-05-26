#ifndef _RX_
#define _RX_

#include "rxovride.h"
#include "ntifs.h"

#ifndef BooleanFlagOn
#define BooleanFlagOn(Flags, SingleFlag) ((BOOLEAN)((((Flags) & (SingleFlag)) != 0)))
#endif

#ifndef SetFlag
#define SetFlag(Flags, SetOfFlags) \
{                                  \
    (Flags) |= (SetOfFlags);       \
}
#endif

#ifndef ClearFlag
#define ClearFlag(Flags, SetOfFlags) \
{                                    \
    (Flags) &= ~(SetOfFlags);        \
}
#endif

#define Add2Ptr(Ptr, Inc) ((PVOID)((PUCHAR)(Ptr) + (Inc)))

#define INLINE __inline

#include "rxtypes.h"

#ifndef MINIRDR__NAME
#include "rxpooltg.h"
#endif

#include "ntrxdef.h"
#include "fcbtable.h"
#include "mrxfcb.h"
#include "rxworkq.h"
#include "rxprocs.h"

#ifndef MINIRDR__NAME
#include "rxdata.h"
#include "buffring.h"
#endif

#endif
