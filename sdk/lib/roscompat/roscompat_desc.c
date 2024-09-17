
#include <roscompat.h>

static unsigned long NumberOfValidExports = 0xFFFFFFFF;

#if defined(_MSC_VER)
#pragma section(".expvers$ZZZ")
__declspec(allocate(".expvers$ZZZ"))
#elif defined(__GNUC__)
__attribute__ ((section(".expvers$ZZZ")))
#else
#error Your compiler is not supported.
#endif
ROSCOMPAT_DESCRIPTOR __roscompat_dummy_descriptor__ =
{
    0,
    0,
    1,
    &NumberOfValidExports
};
