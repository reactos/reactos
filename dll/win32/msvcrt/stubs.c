
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

typedef struct __crt_locale_data_public
{
      unsigned short const* _locale_pctype;
    _Field_range_(1, 2) int _locale_mb_cur_max;
               unsigned int _locale_lc_codepage;
} __crt_locale_data_public;

static
__crt_locale_data_public*
__CRTDECL
__acrt_get_locale_data_prefix(
    void const volatile* const _LocalePointers)
{
    _locale_t const _TypedLocalePointers = (_locale_t)_LocalePointers;
    return (__crt_locale_data_public*)_TypedLocalePointers->locinfo;
}

static
int
__CRTDECL
__acrt_locale_get_ctype_array_value(
    _In_reads_(_Char_value + 1) unsigned short const * const locale_pctype_array,
    _In_range_(-1, 255) int const c,
    _In_ int const mask
    )
{
    if (c >= -1 && c <= 255)
    {
        return locale_pctype_array[c] & mask;
    }
    return 0;
}

const unsigned short* __cdecl __pctype_func(void);

#if defined _CRT_DISABLE_PERFCRIT_LOCKS && !defined _DLL
    #define __PCTYPE_FUNC  _pctype
#else
    #define __PCTYPE_FUNC __pctype_func()
#endif

#ifdef _DEBUG
    _ACRTIMP int __cdecl _chvalidator(_In_ int c, _In_ int mask);
    #define __chvalidchk(a, b) _chvalidator(a, b)
#else
    #define __chvalidchk(a, b) (__acrt_locale_get_ctype_array_value(__PCTYPE_FUNC, (a), (b)))
#endif

static
int
__CRTDECL
_chvalidchk_l(
    _In_ int const c,
    _In_ int const mask,
    _In_opt_ _locale_t const locale
    )
{
#ifdef _DEBUG
    return _chvalidator_l(locale, c, mask);
#else
    if (locale)
    {
        return __acrt_locale_get_ctype_array_value(__acrt_get_locale_data_prefix(locale)->_locale_pctype, c, mask);
    }

    return __chvalidchk(c, mask);
#endif
}

static
int
__CRTDECL
_ischartype_l(
    _In_ int const c,
    _In_ int const mask,
    _In_opt_ _locale_t const locale)
{
    if (locale)
    {
        if (c >= -1 && c <= 255)
        {
            return __acrt_get_locale_data_prefix(locale)->_locale_pctype[c] & mask;
        }

        if (__acrt_get_locale_data_prefix(locale)->_locale_mb_cur_max > 1)
        {
            return _isctype_l(c, mask, locale);
        }

        return 0; // >0xFF and SBCS locale
    }

    return _chvalidchk_l(c, mask, 0);
}

//#undef _isalnum_l
_Check_return_
_CRTIMP
int
__cdecl
_isalnum_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return  _ischartype_l(c, _ALPHA | _DIGIT, locale);
}

#undef _isalpha_l
_Check_return_
_CRTIMP
int
__cdecl
_isalpha_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _ALPHA, locale);
}

#undef _iscntrl_l
_Check_return_
_CRTIMP
int
__cdecl
_iscntrl_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _CONTROL, locale);
}

#undef _isdigit_l
_Check_return_
_CRTIMP
int
__cdecl
_isdigit_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _DIGIT, locale);
}

#undef _isgraph_l
_Check_return_
_CRTIMP
int
__cdecl
_isgraph_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _PUNCT | _ALPHA | _DIGIT, locale);
}

#undef _islower_l
_Check_return_
_CRTIMP
int
__cdecl
_islower_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _LOWER, locale);
}

#undef _isprint_l
_Check_return_
_CRTIMP
int
__cdecl
_isprint_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _BLANK | _PUNCT | _ALPHA | _DIGIT, locale);
}

#undef _isspace_l
_Check_return_
_CRTIMP
int
__cdecl
_isspace_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _SPACE, locale);
}

#undef _isupper_l
_Check_return_
_CRTIMP
int
__cdecl
_isupper_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _UPPER, locale);
}

#undef _iswalnum_l
_Check_return_
_CRTIMP
int
__cdecl
_iswalnum_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _ALPHA|_DIGIT, locale);
}

#undef _iswalpha_l
_Check_return_
_CRTIMP
int
__cdecl
_iswalpha_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _ALPHA, locale);
}

#undef _iswcntrl_l
_Check_return_
_CRTIMP
int
__cdecl
_iswcntrl_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return _iswctype_l(c, _CONTROL, locale);
}

_Check_return_
_CRTIMP
int
__cdecl
_iswctype_l(
    _In_ wint_t c,
    _In_ wctype_t type,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

#undef _iswdigit_l
_Check_return_
_CRTIMP
int
__cdecl
_iswdigit_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _DIGIT, locale);
}

#undef _iswgraph_l
_Check_return_
_CRTIMP
int
__cdecl
_iswgraph_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _PUNCT | _ALPHA | _DIGIT, locale);
}

#undef _iswlower_l
_Check_return_
_CRTIMP
int
__cdecl
_iswlower_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c , _LOWER, locale);
}

#undef _iswprint_l
_Check_return_
_CRTIMP
int
__cdecl
_iswprint_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _BLANK | _PUNCT | _ALPHA | _DIGIT, locale);
}

#undef _iswpunct_l
_Check_return_
_CRTIMP
int
__cdecl
_iswpunct_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _PUNCT, locale);
}

#undef _iswspace_l
_Check_return_
_CRTIMP
int
__cdecl
_iswspace_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _SPACE, locale);
}

#undef _iswupper_l
_Check_return_
_CRTIMP
int
__cdecl
_iswupper_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _UPPER, locale);
}

#undef _iswxdigit_l
_Check_return_
_CRTIMP
int
__cdecl
_iswxdigit_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    return _iswctype_l(c, _HEX, locale);
}

#undef _isxdigit_l
_Check_return_
_CRTIMP
int
__cdecl
_isxdigit_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    return _ischartype_l(c, _HEX, locale);
}

_Must_inspect_result_
_CRTIMP
int
__cdecl
_memicmp_l(
    _In_reads_bytes_opt_(size) const void *buf1,
    _In_reads_bytes_opt_(size) const void *buf2,
    _In_ size_t size,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_strcoll_l(
    _In_z_ const char *str1,
    _In_z_ const char *str2,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_stricmp_l(
    _In_z_ const char *str1,
    _In_z_ const char *str2,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_stricoll_l(
    _In_z_ const char *str1,
    _In_z_ const char *str2,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_strlwr_s(
    _Inout_updates_z_(size) char *str,
    _In_ size_t size)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_strlwr_s_l(
    _Inout_updates_z_(size) char *str,
    _In_ size_t size,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_strncoll_l(
    _In_z_ const char *str1,
    _In_z_ const char *str2,
    _In_ size_t maxcount,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_strnicmp_l(
    _In_reads_or_z_(maxcount) const char *str1,
    _In_reads_or_z_(maxcount) const char *str2,
    _In_ size_t maxcount,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_strnicoll_l(
    _In_z_ const char *str1,
    _In_z_ const char *str2,
    _In_ size_t maxcount,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_strnset_s(
    _Inout_updates_z_(size) char *str,
    _In_ size_t size,
    _In_ int val,
    _In_ size_t _MaxCount)
{
    UNIMPLEMENTED;
    return 0;
}

_CRTIMP
char*
_strupr_l(
    char *str,
    _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
wint_t
__cdecl
_towlower_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
wint_t
__cdecl
_towupper_l(
    _In_ wint_t c,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_CRTIMP
wchar_t*
_wcslwr_l(
    wchar_t *str,
    _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_CRTIMP
wchar_t*
_wcsupr_l(
    wchar_t *str,
    _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_opt_
_CRTIMP
size_t
__cdecl
_wcsxfrm_l(
    _Out_writes_opt_(maxcount) _Post_maybez_ wchar_t *dst,
    _In_z_ const wchar_t *src,
    _In_ size_t maxcount,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_strset_s(
    _Inout_updates_z_(size) char *dst,
    _In_ size_t size,
    _In_ int val)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_strupr_s(
    _Inout_updates_z_(size) char *str,
    _In_ size_t size)
{
    return _strupr_s_l(str, size, 0);
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_strupr_s_l(
    _Inout_updates_z_(_Size) char *str,
    _In_ size_t size,
    _locale_t locale)
{
    UNIMPLEMENTED;
    return ENOTSUP;
}

_Check_return_
_CRTIMP
int
__cdecl
_tolower_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_toupper_l(
    _In_ int c,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_wcscoll_l(
    _In_z_ const wchar_t *str1,
    _In_z_ const wchar_t *str2,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_wcsicmp_l(
    _In_z_ const wchar_t *str1,
    _In_z_ const wchar_t *str2,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_wcsicoll_l(
    _In_z_ const wchar_t *str1,
    _In_z_ const wchar_t *str2,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_wcslwr_s(
    _Inout_updates_z_(sizeInWords) wchar_t *str,
    _In_ size_t sizeInWords)
{
    return _wcslwr_s_l(str, sizeInWords, NULL);
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_wcslwr_s_l(
    _Inout_updates_z_(sizeInWords) wchar_t *str,
    _In_ size_t sizeInWords,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return ENOTSUP;
}

typedef size_t rsize_t;

_Check_return_wat_
_CRTIMP // _ACRTIMP
errno_t
__cdecl
strncat_s(
    _Inout_updates_z_(size) char* dest,
    _In_ rsize_t size,
    _In_reads_or_z_(maxCount) char const* src,
    _In_ rsize_t  maxCount)
{
    UNIMPLEMENTED;
    return ENOTSUP;
}

_Check_return_opt_
_CRTIMP
_CRT_INSECURE_DEPRECATE(vsnprintf_s)
int
__cdecl
vsnprintf_(
    _Out_writes_(maxCount) char *dst,
    _In_ size_t maxCount,
    _In_z_ _Printf_format_string_ const char *format,
    va_list argptr)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_wcsnicoll_l(
    _In_z_ const wchar_t *str1,
    _In_z_ const wchar_t *str2,
    _In_ size_t maxCount,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_wcsnicmp_l(
    _In_reads_or_z_(_MaxCount) const wchar_t *str1,
    _In_reads_or_z_(_MaxCount) const wchar_t *str2,
    _In_ size_t maxCount,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_
_CRTIMP
int
__cdecl
_wcsncoll_l(
    _In_z_ const wchar_t *str1,
    _In_z_ const wchar_t *str2,
    _In_ size_t maxCount,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_strerror_s(
    _Out_writes_z_(_SizeInBytes) char *buf,
    _In_ size_t sizeInBytes,
    _In_opt_z_ const char *errMsg)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_wcsnset_s(
    _Inout_updates_z_(_DstSizeInWords) wchar_t *dst,
    _In_ size_t sizeInWords,
    _In_ wchar_t val,
    _In_ size_t maxCount)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_wcsset_s(
    _Inout_updates_z_(_SizeInWords) wchar_t *str,
    _In_ size_t sizeInWords,
    _In_ wchar_t val)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
_wcsupr_s_l(
    _Inout_updates_z_(_Size) wchar_t *str,
    _In_ size_t size,
    _In_opt_ _locale_t locale)
{
    UNIMPLEMENTED;
    return 0;
}

_Check_return_opt_
_CRTIMP
_CRT_INSECURE_DEPRECATE(vsnprintf_s)
int
__cdecl
vsnprintf(
    _Out_writes_(_MaxCount) char *dest,
    _In_ size_t maxCount,
    _In_z_ _Printf_format_string_ const char *format,
    va_list argptr)
{
    UNIMPLEMENTED;
    return 0;
}



