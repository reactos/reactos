// this version of qistub is for retail only.
// if clients (e.g. shell32) want debug version
// they staticaly link to it (and the local version
// overwrites this one

#include "priv.h"

#ifdef  DEBUG
// warning Warning WARNING!!!
// priv.h's PCH has been built DEBUG, and now we're #undef'ing it.
// so various macros are still 'on'.  this leads to inconsistencies
// in ../lib/qistub.cpp.  i've hacked around this for the 1 known
// pblm case in ../lib/qistub.cpp (DBEXEC).
//
// (and we can't just move the #undef up above priv.h, it will still
// be ignored because the PCH already exists).
//
// i'm 99% sure that the reason we don't want DEBUG on here is to avoid
// having any static data in shlwapi.  that's an old restriction (though
// still a perf issue) (but not for DEBUG...), so for DEBUG we can probably
// just remove this entire hack.  i haven't tested that theory yet though
// so for now we'll continue to live w/ it.
#undef  DEBUG
#endif

#include "../lib/qistub.cpp"
