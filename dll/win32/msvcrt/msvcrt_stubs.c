
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/config.h"
#include "wine/exception.h"

#define NDEBUG
#include <debug.h>

int __get_app_type()
{
    UNIMPLEMENTED;
    return 0;
}

int _fileinfo;

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

int _inp( 
   unsigned short port)
{
    UNIMPLEMENTED;
    return 0;
}

unsigned short _inpw( 
   unsigned short port)
{
    UNIMPLEMENTED;
    return 0;
}

unsigned long _inpd( 
   unsigned short port)
{
    return 0;
}


int _outp(
   unsigned short port,
   int databyte)
{
    UNIMPLEMENTED;
    return 0;
}

unsigned short _outpw(
   unsigned short port,
   unsigned short dataword)
{
    UNIMPLEMENTED;
    return 0;
}

unsigned long _outpd(
   unsigned short port,
   unsigned long dataword)
{
    UNIMPLEMENTED;
    return 0;
}

void __wine_spec_unimplemented_stub( const char *module, const char *function )
{
    ULONG_PTR args[2];

    args[0] = (ULONG_PTR)module;
    args[1] = (ULONG_PTR)function;
    RaiseException( EXCEPTION_WINE_STUB, EH_NONCONTINUABLE, 2, args );
}

static const char __wine_spec_file_name[] = "msvcrt.dll";

void __wine_stub_msvcrt_dll_115(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "__lc_collate"); }
void __wine_stub_msvcrt_dll_140(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "__p__pwctype"); }
void __wine_stub_msvcrt_dll_151(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "__pxcptinfoptrs"); }
void __wine_stub_msvcrt_dll_267(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_fpieee_flt"); }
void __wine_stub_msvcrt_dll_288(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_get_terminate"); }
void __wine_stub_msvcrt_dll_289(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_get_unexpected"); }
void __wine_stub_msvcrt_dll_301(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_getsystime"); }
void __wine_stub_msvcrt_dll_311(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_heapused"); }
void __wine_stub_msvcrt_dll_323(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbbalnum"); }
void __wine_stub_msvcrt_dll_324(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbbalpha"); }
void __wine_stub_msvcrt_dll_325(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbbgraph"); }
void __wine_stub_msvcrt_dll_326(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbbkalnum"); }
void __wine_stub_msvcrt_dll_329(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbbkpunct"); }
void __wine_stub_msvcrt_dll_331(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbbprint"); }
void __wine_stub_msvcrt_dll_332(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbbpunct"); }
void __wine_stub_msvcrt_dll_340(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbcl0"); }
void __wine_stub_msvcrt_dll_341(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbcl1"); }
void __wine_stub_msvcrt_dll_342(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_ismbcl2"); }
void __wine_stub_msvcrt_dll_380(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_mbcjmstojis"); }
void __wine_stub_msvcrt_dll_382(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_mbctohira"); }
void __wine_stub_msvcrt_dll_383(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_mbctokata"); }
void __wine_stub_msvcrt_dll_405(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_mbsnbcoll"); }
void __wine_stub_msvcrt_dll_409(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_mbsnbicoll"); }
void __wine_stub_msvcrt_dll_414(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_mbsncoll"); }
void __wine_stub_msvcrt_dll_418(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_mbsnicoll"); }
void __wine_stub_msvcrt_dll_480(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_setsystime"); }
void __wine_stub_msvcrt_dll_505(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_strncoll"); }
void __wine_stub_msvcrt_dll_507(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_strnicoll"); }
void __wine_stub_msvcrt_dll_556(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_wcsncoll"); }
void __wine_stub_msvcrt_dll_558(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_wcsnicoll"); }
void __wine_stub_msvcrt_dll_598(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_wperror"); }
void __wine_stub_msvcrt_dll_625(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_wtmpnam"); }
void __wine_stub_msvcrt_dll_832(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "wcsxfrm"); }
