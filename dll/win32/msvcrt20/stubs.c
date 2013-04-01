#include <stubs.h>

#undef UNIMPLEMENTED
#define UNIMPLEMENTED __wine_spec_unimplemented_stub("msvcrt.dll", __FUNCTION__)

int _atodbl(
   void * value,
   char * str)
{
    UNIMPLEMENTED;
    return 0;
}

int _ismbbkprint(
   unsigned int c)
{
    UNIMPLEMENTED;
    return 0;
}

size_t _heapused( size_t *pUsed, size_t *pCommit )
{
    UNIMPLEMENTED;
    return( 0 );
}

int _fileinfo = 0;