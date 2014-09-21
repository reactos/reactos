#include <cstdio>
#include <cwchar>
#include <climits>

#if !defined (CHAR_BIT)
#  error Missing CHAR_BIT definition.
#endif

#if (CHAR_BIT < 0)
#  error Weird WCHAR_BIT value.
#endif

#if !defined (CHAR_MAX)
#  error Missing CHAR_MAX definition.
#endif

#if !defined (CHAR_MIN)
#  error Missing CHAR_MIN definition.
#endif

#if !(CHAR_MIN < CHAR_MAX)
#  error Weird CHAR_MIN or CHAR_MAX macro values.
#endif

#if !defined (INT_MAX)
#  error Missing INT_MAX definition.
#endif

#if !defined (INT_MIN)
#  error Missing INT_MIN definition.
#endif

#if !(INT_MIN < INT_MAX)
#  error Weird INT_MIN or INT_MAX macro values.
#endif

#if !defined (LONG_MAX)
#  error Missing LONG_MAX definition.
#endif

#if !defined (LONG_MIN)
#  error Missing LONG_MIN definition.
#endif

#if !(LONG_MIN < LONG_MAX)
#  error Weird LONG_MIN or LONG_MAX macro values.
#endif

#if !defined (SCHAR_MAX)
#  error Missing SCHAR_MAX definition.
#endif

#if !defined (SCHAR_MIN)
#  error Missing SCHAR_MIN definition.
#endif

#if !(SCHAR_MIN < SCHAR_MAX)
#  error Weird SCHAR_MIN or SCHAR_MAX macro values.
#endif

#if !defined (SHRT_MAX)
#  error Missing SHRT_MAX definition.
#endif

#if !defined (SHRT_MIN)
#  error Missing SHRT_MIN definition.
#endif

#if !(SHRT_MIN < SHRT_MAX)
#  error Weird SHRT_MIN or SHRT_MAX macro values.
#endif

#if !defined (WCHAR_MIN)
#  error Missing WCHAR_MIN definition.
#endif

#if !defined (WCHAR_MAX)
#  error Missing WCHAR_MAX definition.
#endif

#if !(WCHAR_MIN < WCHAR_MAX)
#  error Weird WCHAR_MIN or WCHAR_MAX macro value.
#endif

#if !defined (UCHAR_MAX)
#  error Missing UCHAR_MAX definition.
#endif

#if (UCHAR_MAX < 0)
#  error Weird UCHAR_MAX macro value.
#endif

#if !defined (UINT_MAX)
#  error Missing UINT_MAX definition.
#endif

#if (UINT_MAX < 0)
#  error Weird UINT_MAX macro value.
#endif

#if !defined (ULONG_MAX)
#  error Missing ULONG_MAX definition.
#endif

#if (ULONG_MAX < 0)
#  error Weird ULONG_MAX macro value.
#endif

#if !defined (USHRT_MAX)
#  error Missing USHRT_MAX definition.
#endif

#if (USHRT_MAX < 0)
#  error Weird USHRT_MAX macro value.
#endif
