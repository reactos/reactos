
#include <roscompat.h>

static unsigned long NumberOfValidExports = 0xFFFFFFFF;

#if defined(_MSC_VER)
#pragma section(".expvers")
__declspec(allocate(".expvers"))
#elif defined(__GNUC__)
__attribute__ ((section(".expvers")))
#else
#error Your compiler is not supported.
#endif
ROSCOMPAT_DESCRIPTOR __roscompat_descriptor__ =
{
    0,
    0,
    1,
    &NumberOfValidExports
};
