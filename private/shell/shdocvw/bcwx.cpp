#include "priv.h"

#define ASSERTDATA

/* Macro to compute a back pointer to a containing class given a
   pointer to a member, the member name, and the containing class type.
   This generates no code because it results in a constant offset.
   Note: this is taken from mso96 dll code. */
#define BACK_POINTER(p, m, c) \
	((c *) (void *) (((char *) (void *) (p)) - (char *) (&((c *) 0)->m)))

#ifdef DEBUG
	#define Debug(e) e
	#define DebugElse(s, t)	s
#else
	#define Debug(e)
	#define DebugElse(s, t) t
#endif

#include "bcw.cpp"

IBindCtx * BCW_Create(IBindCtx* pibc)
{
    return BCW::Create(pibc);
}

