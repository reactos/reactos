
#include <stubs.h>

#undef UNIMPLEMENTED
#define UNIMPLEMENTED __wine_spec_unimplemented_stub("msvcrt.dll", __FUNCTION__)

int __get_app_type()
{
    UNIMPLEMENTED;
    return 0;
}

int _fileinfo = 0;

void *
__p__fileinfo()
{
    return &_fileinfo;
}

unsigned char _mbcasemap[1];

void *
__p__mbcasemap()
{
    return _mbcasemap;
}

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

#ifdef _M_IX86
int MSVCRT__inp(
   unsigned short port)
{
    return _inp(port);
}

unsigned short MSVCRT__inpw(
   unsigned short port)
{
    return _inpw(port);
}

unsigned long MSVCRT__inpd(
   unsigned short port)
{
    return _inpd(port);
}


int MSVCRT__outp(
   unsigned short port,
   int databyte)
{
    return _outp(port, databyte);
}

unsigned short MSVCRT__outpw(
   unsigned short port,
   unsigned short dataword)
{
    return _outpw(port, dataword);
}

unsigned long MSVCRT__outpd(
   unsigned short port,
   unsigned long dataword)
{
    return _outpd(port, dataword);
}
#endif
