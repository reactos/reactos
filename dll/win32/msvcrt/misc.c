/*
 * msvcrt.dll misc functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdlib.h>
#include <sys/types.h>

#include "msvcrt.h"
#include "wine/debug.h"
#include "ntsecapi.h"
#include "windows.h"
#include "wine/asm.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static unsigned int output_format;

/*********************************************************************
 *		_beep (MSVCRT.@)
 */
void CDECL _beep( unsigned int freq, unsigned int duration)
{
    TRACE(":Freq %d, Duration %d\n",freq,duration);
    Beep(freq, duration);
}

/*********************************************************************
 *		srand (MSVCRT.@)
 */
void CDECL srand( unsigned int seed )
{
    thread_data_t *data = msvcrt_get_thread_data();
    data->random_seed = seed;
}

/*********************************************************************
 *		rand (MSVCRT.@)
 */
int CDECL rand(void)
{
    thread_data_t *data = msvcrt_get_thread_data();

    /* this is the algorithm used by MSVC, according to
     * http://en.wikipedia.org/wiki/List_of_pseudorandom_number_generators */
    data->random_seed = data->random_seed * 214013 + 2531011;
    return (data->random_seed >> 16) & RAND_MAX;
}

/*********************************************************************
 *		rand_s (MSVCRT.@)
 */
int CDECL rand_s(unsigned int *pval)
{
    if (!pval || !RtlGenRandom(pval, sizeof(*pval)))
    {
        *_errno() = EINVAL;
        return EINVAL;
    }
    return 0;
}

/*********************************************************************
 *		_sleep (MSVCRT.@)
 */
void CDECL _sleep(__msvcrt_ulong timeout)
{
  TRACE("_sleep for %ld milliseconds\n",timeout);
  Sleep((timeout)?timeout:1);
}

/*********************************************************************
 *		_lfind (MSVCRT.@)
 */
void* CDECL _lfind(const void* match, const void* start,
                   unsigned int* array_size, unsigned int elem_size,
                   int (CDECL *cf)(const void*,const void*) )
{
  unsigned int size = *array_size;
  if (size)
    do
    {
      if (cf(match, start) == 0)
        return (void *)start; /* found */
      start = (const char *)start + elem_size;
    } while (--size);
  return NULL;
}

/*********************************************************************
 *		_lfind_s (MSVCRT.@)
 */
void* CDECL _lfind_s(const void* match, const void* start,
                   unsigned int* array_size, unsigned int elem_size,
                   int (CDECL *cf)(void*,const void*,const void*),
                   void* context)
{
  unsigned int size;
  if (!MSVCRT_CHECK_PMT(match != NULL)) return NULL;
  if (!MSVCRT_CHECK_PMT(array_size != NULL)) return NULL;
  if (!MSVCRT_CHECK_PMT(start != NULL || *array_size == 0)) return NULL;
  if (!MSVCRT_CHECK_PMT(cf != NULL)) return NULL;
  if (!MSVCRT_CHECK_PMT(elem_size != 0)) return NULL;

  size = *array_size;
  if (size)
    do
    {
      if (cf(context, match, start) == 0)
        return (void *)start; /* found */
      start = (const char *)start + elem_size;
    } while (--size);
  return NULL;
}

/*********************************************************************
 *		_lsearch (MSVCRT.@)
 */
void* CDECL _lsearch(const void* match, void* start,
                     unsigned int* array_size, unsigned int elem_size,
                     int (CDECL *cf)(const void*,const void*) )
{
  unsigned int size = *array_size;
  if (size)
    do
    {
      if (cf(match, start) == 0)
        return start; /* found */
      start = (char*)start + elem_size;
    } while (--size);

  /* not found, add to end */
  memcpy(start, match, elem_size);
  array_size[0]++;
  return start;
}

/*********************************************************************
 *                  bsearch_s (msvcrt.@)
 */
void* CDECL bsearch_s(const void *key, const void *base, size_t nmemb, size_t size,
                             int (__cdecl *compare)(void *, const void *, const void *), void *ctx)
{
    ssize_t min = 0;
    ssize_t max = nmemb - 1;

    if (!MSVCRT_CHECK_PMT(size != 0)) return NULL;
    if (!MSVCRT_CHECK_PMT(compare != NULL)) return NULL;

    while (min <= max)
    {
        ssize_t cursor = min + (max - min) / 2;
        int ret = compare(ctx, key,(const char *)base+(cursor*size));
        if (!ret)
            return (char*)base+(cursor*size);
        if (ret < 0)
            max = cursor - 1;
        else
            min = cursor + 1;
    }
    return NULL;
}

static int CDECL compare_wrapper(void *ctx, const void *e1, const void *e2)
{
    int (__cdecl *compare)(const void *, const void *) = ctx;
    return compare(e1, e2);
}

/*********************************************************************
 *                  bsearch (msvcrt.@)
 */
void* CDECL bsearch(const void *key, const void *base, size_t nmemb,
        size_t size, int (__cdecl *compar)(const void *, const void *))
{
    return bsearch_s(key, base, nmemb, size, compare_wrapper, compar);
}
/*********************************************************************
 *		_chkesp (MSVCRT.@)
 *
 * Trap to a debugger if the value of the stack pointer has changed.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Does not return.
 *
 * NOTES
 *  This function is available for iX86 only.
 *
 *  When VC++ generates debug code, it stores the value of the stack pointer
 *  before calling any external function, and checks the value following
 *  the call. It then calls this function, which will trap if the values are
 *  not the same. Usually this means that the prototype used to call
 *  the function is incorrect.  It can also mean that the .spec entry has
 *  the wrong calling convention or parameters.
 */
#ifdef __i386__

# if defined(__GNUC__) || defined(__clang__)

__ASM_GLOBAL_FUNC(_chkesp,
                  "jnz 1f\n\t"
                  "ret\n"
                  "1:\tpushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "subl $12,%esp\n\t"
                  "pushl %eax\n\t"
                  "pushl %ecx\n\t"
                  "pushl %edx\n\t"
                  "call " __ASM_NAME("chkesp_fail") "\n\t"
                  "popl %edx\n\t"
                  "popl %ecx\n\t"
                  "popl %eax\n\t"
                  "leave\n\t"
                  __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                  __ASM_CFI(".cfi_same_value %ebp\n\t")
                  "ret")

void CDECL chkesp_fail(void)
{
  ERR("Stack pointer incorrect after last function call - Bad prototype/spec entry?\n");
  DebugBreak();
}

# else  /* __GNUC__ || __clang__ */

/**********************************************************************/

void CDECL _chkesp(void)
{
}

# endif  /* __GNUC__ || __clang__ */

#endif  /* __i386__ */

static inline void swap(char *l, char *r, size_t size)
{
    char tmp;

    while(size--) {
        tmp = *l;
        *l++ = *r;
        *r++ = tmp;
    }
}

static void small_sort(void *base, size_t nmemb, size_t size,
        int (CDECL *compar)(void *, const void *, const void *), void *context)
{
    size_t e, i;
    char *max, *p;

    for(e=nmemb; e>1; e--) {
        max = base;
        for(i=1; i<e; i++) {
            p = (char*)base + i*size;
            if(compar(context, p, max) > 0)
                max = p;
        }

        if(p != max)
            swap(p, max, size);
    }
}

static void quick_sort(void *base, size_t nmemb, size_t size,
        int (CDECL *compar)(void *, const void *, const void *), void *context)
{
    size_t stack_lo[8*sizeof(size_t)], stack_hi[8*sizeof(size_t)];
    size_t beg, end, lo, hi, med;
    int stack_pos;

    stack_pos = 0;
    stack_lo[stack_pos] = 0;
    stack_hi[stack_pos] = nmemb-1;

#define X(i) ((char*)base+size*(i))
    while(stack_pos >= 0) {
        beg = stack_lo[stack_pos];
        end = stack_hi[stack_pos--];

        if(end-beg < 8) {
            small_sort(X(beg), end-beg+1, size, compar, context);
            continue;
        }

        lo = beg;
        hi = end;
        med = lo + (hi-lo+1)/2;
        if(compar(context, X(lo), X(med)) > 0)
            swap(X(lo), X(med), size);
        if(compar(context, X(lo), X(hi)) > 0)
            swap(X(lo), X(hi), size);
        if(compar(context, X(med), X(hi)) > 0)
            swap(X(med), X(hi), size);

        lo++;
        hi--;
        while(1) {
            while(lo <= hi) {
                if(lo!=med && compar(context, X(lo), X(med))>0)
                    break;
                lo++;
            }

            while(med != hi) {
                if(compar(context, X(hi), X(med)) <= 0)
                    break;
                hi--;
            }

            if(hi < lo)
                break;

            swap(X(lo), X(hi), size);
            if(hi == med)
                med = lo;
            lo++;
            hi--;
        }

        while(hi > beg) {
            if(hi!=med && compar(context, X(hi), X(med))!=0)
                break;
            hi--;
        }

        if(hi-beg >= end-lo) {
            stack_lo[++stack_pos] = beg;
            stack_hi[stack_pos] = hi;
            stack_lo[++stack_pos] = lo;
            stack_hi[stack_pos] = end;
        }else {
            stack_lo[++stack_pos] = lo;
            stack_hi[stack_pos] = end;
            stack_lo[++stack_pos] = beg;
            stack_hi[stack_pos] = hi;
        }
    }
#undef X
}

/*********************************************************************
 * qsort_s (MSVCRT.@)
 *
 * This function is trying to sort data doing identical comparisons
 * as native does. There are still cases where it behaves differently.
 */
void CDECL qsort_s(void *base, size_t nmemb, size_t size,
    int (CDECL *compar)(void *, const void *, const void *), void *context)
{
    const size_t total_size = nmemb*size;

    if (!MSVCRT_CHECK_PMT(base != NULL || (base == NULL && nmemb == 0))) return;
    if (!MSVCRT_CHECK_PMT(size > 0)) return;
    if (!MSVCRT_CHECK_PMT(compar != NULL)) return;
    if (total_size / size != nmemb) return;

    if (nmemb < 2) return;

    quick_sort(base, nmemb, size, compar, context);
}

/*********************************************************************
 * qsort (MSVCRT.@)
 */
void CDECL qsort(void *base, size_t nmemb, size_t size,
        int (CDECL *compar)(const void*, const void*))
{
    qsort_s(base, nmemb, size, compare_wrapper, compar);
}

/*********************************************************************
 * _get_output_format (MSVCRT.@)
 */
unsigned int CDECL _get_output_format(void)
{
   return output_format;
}

/*********************************************************************
 * _set_output_format (MSVCRT.@)
 */
unsigned int CDECL _set_output_format(unsigned int new_output_format)
{
    unsigned int ret = output_format;

    if(!MSVCRT_CHECK_PMT(new_output_format==0 || new_output_format==_TWO_DIGIT_EXPONENT))
        return ret;

    output_format = new_output_format;
    return ret;
}

/*********************************************************************
 * _resetstkoflw (MSVCRT.@)
 */
int CDECL _resetstkoflw(void)
{
    int stack_addr;
    DWORD oldprot;

    /* causes stack fault that updates NtCurrentTeb()->Tib.StackLimit */
    return VirtualProtect(&stack_addr, 1, PAGE_GUARD|PAGE_READWRITE, &oldprot);
}

#if _MSVCR_VER>=80 && _MSVCR_VER<=90

/*********************************************************************
 *  _decode_pointer (MSVCR80.@)
 */
void * CDECL _decode_pointer(void * ptr)
{
    return DecodePointer(ptr);
}

/*********************************************************************
 *  _encode_pointer (MSVCR80.@)
 */
void * CDECL _encode_pointer(void * ptr)
{
    return EncodePointer(ptr);
}

#endif /* _MSVCR_VER>=80 && _MSVCR_VER<=90 */

#if _MSVCR_VER>=80 && _MSVCR_VER<=100
/*********************************************************************
 *  _encoded_null (MSVCR80.@)
 */
void * CDECL _encoded_null(void)
{
    TRACE("\n");

    return EncodePointer(NULL);
}
#endif

#if _MSVCR_VER>=70
/*********************************************************************
 * _CRT_RTC_INIT (MSVCR70.@)
 */
void* CDECL _CRT_RTC_INIT(void *unk1, void *unk2, int unk3, int unk4, int unk5)
{
    TRACE("%p %p %x %x %x\n", unk1, unk2, unk3, unk4, unk5);
    return NULL;
}
#endif

#if _MSVCR_VER>=80

/*********************************************************************
 * _CRT_RTC_INITW (MSVCR80.@)
 */
void* CDECL _CRT_RTC_INITW(void *unk1, void *unk2, int unk3, int unk4, int unk5)
{
    TRACE("%p %p %x %x %x\n", unk1, unk2, unk3, unk4, unk5);
    return NULL;
}

/*********************************************************************
 * _byteswap_ushort (MSVCR80.@)
 */
unsigned short CDECL _byteswap_ushort(unsigned short s)
{
    return (s<<8) + (s>>8);
}

/*********************************************************************
 * _byteswap_ulong (MSVCR80.@)
 */
__msvcrt_ulong CDECL _byteswap_ulong(__msvcrt_ulong l)
{
    return (l<<24) + ((l<<8)&0xFF0000) + ((l>>8)&0xFF00) + (l>>24);
}

/*********************************************************************
 * _byteswap_uint64 (MSVCR80.@)
 */
unsigned __int64 CDECL _byteswap_uint64(unsigned __int64 i)
{
    return (i<<56) + ((i&0xFF00)<<40) + ((i&0xFF0000)<<24) + ((i&0xFF000000)<<8) +
        ((i>>8)&0xFF000000) + ((i>>24)&0xFF0000) + ((i>>40)&0xFF00) + (i>>56);
}

#endif /* _MSVCR_VER>=80 */

#if _MSVCR_VER>=110

/*********************************************************************
 *  __crtGetShowWindowMode (MSVCR110.@)
 */
int CDECL __crtGetShowWindowMode(void)
{
    STARTUPINFOW si;

    GetStartupInfoW(&si);
    TRACE("flags=%lx window=%d\n", si.dwFlags, si.wShowWindow);
    return si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT;
}

/*********************************************************************
 *  __crtInitializeCriticalSectionEx (MSVCR110.@)
 */
BOOL CDECL __crtInitializeCriticalSectionEx(
        CRITICAL_SECTION *cs, DWORD spin_count, DWORD flags)
{
    TRACE("(%p %lx %lx)\n", cs, spin_count, flags);
    return InitializeCriticalSectionEx(cs, spin_count, flags);
}

#endif /* _MSVCR_VER>=110 */

#if _MSVCR_VER>=120
/*********************************************************************
 * _vacopy (MSVCR120.@)
 */
void CDECL _vacopy(va_list *dest, va_list src)
{
    va_copy(*dest, src);
}
#endif

#if _MSVCR_VER>=80
/*********************************************************************
 * _crt_debugger_hook (MSVCR80.@)
 */
void CDECL _crt_debugger_hook(int reserved)
{
    WARN("(%x)\n", reserved);
}
#endif

#if _MSVCR_VER>=110
/*********************************************************************
 *  __crtUnhandledException (MSVCR110.@)
 */
LONG CDECL __crtUnhandledException(EXCEPTION_POINTERS *ep)
{
    TRACE("(%p)\n", ep);
    SetUnhandledExceptionFilter(NULL);
    return UnhandledExceptionFilter(ep);
}
#endif

#if _MSVCR_VER>=120
/*********************************************************************
 *		__crtSleep (MSVCR120.@)
 */
void CDECL __crtSleep(DWORD timeout)
{
  TRACE("(%lu)\n", timeout);
  Sleep(timeout);
}

/*********************************************************************
 * _SetWinRTOutOfMemoryExceptionCallback (MSVCR120.@)
 */
void CDECL _SetWinRTOutOfMemoryExceptionCallback(void *callback)
{
    FIXME("(%p): stub\n", callback);
}
#endif
