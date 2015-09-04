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
    thread_data_t *data = msvcrt_get_thread_data();

    /* this is the algorithm used by MSVC, according to
     * http://en.wikipedia.org/wiki/List_of_pseudorandom_number_generators */
    data->random_seed = data->random_seed * 214013 + 2531011;
    return (data->random_seed >> 16) & RAND_MAX;
}

/*
 * @implemented
 */
void
srand(unsigned int seed)
{
    thread_data_t *data = msvcrt_get_thread_data();
    data->random_seed = seed;
}

 /*********************************************************************
  *              rand_s (MSVCRT.@)
  */
int CDECL rand_s(unsigned int *pval)
{
    BOOLEAN (WINAPI *pSystemFunction036)(PVOID, ULONG); // RtlGenRandom
    HINSTANCE hadvapi32 = LoadLibraryA("advapi32.dll");    
    int ret = 0;
    pSystemFunction036 = (void*)GetProcAddress(hadvapi32, "SystemFunction036");
#if 1
    if (!pval || (pSystemFunction036 && !pSystemFunction036(pval, sizeof(*pval))))
    {
        _invalid_parameter(NULL,_CRT_WIDE("rand_s"),_CRT_WIDE(__FILE__),__LINE__, 0);
        *_errno() = EINVAL;
        ret = EINVAL;
    }
#endif
    if(hadvapi32) FreeLibrary(hadvapi32);
    return ret;
}
