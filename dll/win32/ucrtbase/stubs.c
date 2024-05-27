
#include <stdint.h>
#include <intrin.h>

// atexit is needed by libsupc++
extern int __cdecl _crt_atexit(void (__cdecl*)(void));
int __cdecl atexit(void (__cdecl* function)(void))
{
    return _crt_atexit(function);
}

void* __cdecl malloc(size_t);
void* __cdecl operator_new(size_t size)
{
    return malloc(size);
}

void free(void*);
void _cdecl operator_delete(void *mem)
{
    free(mem);
}

#ifdef _M_IX86
void _chkesp_failed(void)
{
    __debugbreak();
}
#endif

int __cdecl __acrt_initialize_sse2(void)
{
    return 0;
}

// The following stubs cannot be implemented as stubs by spec2def, because they are intrinsics

#ifdef _MSC_VER
#pragma warning(disable:4163) // not available as an intrinsic function
#pragma warning(disable:4164) // intrinsic function not declared
#pragma function(fma)
#pragma function(fmaf)
#pragma function(log2)
#pragma function(log2f)
#pragma function(lrint)
#pragma function(lrintf)
#endif

double fma(double x, double y, double z)
{
    __debugbreak();
    return 0.;
}

float fmaf(float x, float y, float z)
{
    __debugbreak();
    return 0.;
}

double log2(double x)
{
    __debugbreak();
    return 0.;
}

float log2f(float x)
{
    __debugbreak();
    return 0.;
}

long int lrint(double x)
{
    __debugbreak();
    return 0;
}

long int lrintf(float x)
{
    __debugbreak();
    return 0;
}
