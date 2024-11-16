#include <precomp.h>
#include <stdlib.h>
#include <internal/tls.h>
#include <internal/rterror.h>

/* Index to TLS */
static DWORD msvcrt_tls_index;

BOOL msvcrt_init_tls(void)
{
  msvcrt_tls_index = TlsAlloc();

  if (msvcrt_tls_index == TLS_OUT_OF_INDEXES)
  {
    ERR("TlsAlloc() failed!\n");
    return FALSE;
  }
  return TRUE;
}

BOOL msvcrt_free_tls(void)
{
  if (!TlsFree(msvcrt_tls_index))
  {
    ERR("TlsFree() failed!\n");
    return FALSE;
  }
  return TRUE;
}

thread_data_t *msvcrt_get_thread_data(void)
{
    thread_data_t *ptr;
    DWORD err = GetLastError();  /* need to preserve last error */

    if (!(ptr = TlsGetValue( msvcrt_tls_index )))
    {
        if (!(ptr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ptr) )))
            _amsg_exit( _RT_THREAD );
        if (!TlsSetValue( msvcrt_tls_index, ptr )) _amsg_exit( _RT_THREAD );
        ptr->tid = GetCurrentThreadId();
        ptr->handle = INVALID_HANDLE_VALUE;
        ptr->random_seed = 1;
#ifndef __UCRTSUPPORT__
        ptr->locinfo = MSVCRT_locale->locinfo;
        ptr->mbcinfo = MSVCRT_locale->mbcinfo;
#endif /* !__UCRTSUPPORT__ */
    }
    SetLastError( err );
    return ptr;
}

void msvcrt_free_tls_mem(void)
{
  thread_data_t *tls = TlsGetValue(msvcrt_tls_index);

  if (tls)
  {
    CloseHandle(tls->handle);
    HeapFree(GetProcessHeap(),0,tls->efcvt_buffer);
    HeapFree(GetProcessHeap(),0,tls->asctime_buffer);
    HeapFree(GetProcessHeap(),0,tls->wasctime_buffer);
    HeapFree(GetProcessHeap(),0,tls->strerror_buffer);
    HeapFree(GetProcessHeap(),0,tls->wcserror_buffer);
    HeapFree(GetProcessHeap(),0,tls->time_buffer);
    //if(tls->have_locale) {
    //    free_locinfo(tls->locinfo);
    //    free_mbcinfo(tls->mbcinfo);
    //}
  }
  HeapFree(GetProcessHeap(), 0, tls);
}

/* EOF */

