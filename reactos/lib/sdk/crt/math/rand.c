/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>
#include <ntsecapi.h>
#include <internal/tls.h>

/*
 * @implemented
 */
int
rand(void)
{
  PTHREADDATA ThreadData = GetThreadData();

  ThreadData->tnext = ThreadData->tnext * 0x5deece66dLL + 2531011;
  return (int)((ThreadData->tnext >> 16) & RAND_MAX);
}

/*
 * @implemented
 */
void
srand(unsigned int seed)
{
  PTHREADDATA ThreadData = GetThreadData();

  ThreadData->tnext = (ULONGLONG)seed;
}

 /*********************************************************************
  *              rand_s (MSVCRT.@)
  */
int CDECL rand_s(unsigned int *pval)
{
    BOOLEAN (WINAPI *pSystemFunction036)(PVOID, ULONG); // RtlGenRandom
    HINSTANCE hadvapi32 = LoadLibraryA("advapi32.dll");    
    pSystemFunction036 = (void*)GetProcAddress(hadvapi32, "SystemFunction036");
#if 1
    if (!pval || (pSystemFunction036 && !pSystemFunction036(pval, sizeof(*pval))))
    {
	    _invalid_parameter(NULL,_CRT_WIDE("rand_s"),_CRT_WIDE(__FILE__),__LINE__, 0);
        *_errno() = EINVAL;
        return EINVAL;
    }
#endif
    if(hadvapi32) FreeLibrary(hadvapi32);
    return 0;
}
