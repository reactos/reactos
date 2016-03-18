//mwerks_debug_prefix.h
#define _STLP_NO_FORCE_INSTANTIATE 1// for debugging
#define EH_VECTOR_OPERATOR_NEW 1
#define _STLP_DEBUG 1 // enable the use of allocation debugging

#if __MWERKS__ >= 0x3000
#include <MSLCarbonPrefix.h>
#endif
