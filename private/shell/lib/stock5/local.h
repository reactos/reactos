//
// Local private header file

#include "../proj.h"

// Check that this supplemental lib does indeed allow NT 5 APIs

#if (WINVER < 0x0500)
#error WINVER setting must be == 0x0500
#endif

#ifndef DS_BIDI_RTL
#define DS_BIDI_RTL  0x8000
#endif
