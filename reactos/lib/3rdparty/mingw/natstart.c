#include "internal.h"

_PGLOBAL
volatile unsigned int __native_dllmain_reason = UINT_MAX;
volatile unsigned int __native_vcclrit_reason = UINT_MAX;
volatile __enative_startup_state __native_startup_state;
volatile void *__native_startup_lock;
