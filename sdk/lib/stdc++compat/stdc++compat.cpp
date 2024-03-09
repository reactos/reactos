
#include <stdio.h>
#include <stdarg.h>

#include <Windows.h>
#define StringCchCopyNA _StringCchCopyNA
#include <strsafe.h>

#undef StringCchCopyNA

typedef  const char* STRSAFE_PCNZCH;

extern "C"
HRESULT
WINAPI
StringCchCopyNA(
    _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
    _In_ size_t cchDest,
    _In_reads_or_z_(cchToCopy) STRSAFE_PCNZCH pszSrc,
    _In_ size_t cchToCopy)
{
    return _StringCchCopyNA(pszDest, cchDest, pszSrc, cchToCopy);
}

extern "C"
int __cdecl __mingw_vsprintf(char* dest, const char* format, va_list arglist)
{
	return vsprintf(dest, format, arglist);
}

namespace std {

/* We shouldn't be throwing exceptions at all, but it sadly turns out
   we call STL (inline) functions that do. This avoids the GLIBCXX_3.4.20
   symbol version. */
void __throw_out_of_range_fmt_(char const* fmt, ...) {
  va_list ap;
  char buf[1024];  // That should be big enough.

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  buf[sizeof(buf) - 1] = 0;
  va_end(ap);

  //__throw_range_error(buf);
}

}  // namespace std
