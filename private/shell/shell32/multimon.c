#include "shellprv.h"
//=============================================================================
//
// This function defines the MULTIMON stub module that fakes multiple monitor 
// apis on pre Memphis Win32 OSes
// this is the only file that defines COMPILE_MULTIMON_STUBS 
//=============================================================================

#define COMPILE_MULTIMON_STUBS

// Don't include multimop.h if shell32 does not use shared dataseg any more.
#include "multimop.h"
#include "multimon.h"
