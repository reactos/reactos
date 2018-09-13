//
// Used by em.rc
//

#ifndef RC_INVOKED
#define DECL_XOSD(n,v,s) IDS_##n = v,
enum _XOSD_STRINGS {
#include <xosd.h>
};
#undef DECL_XOSD
#endif


