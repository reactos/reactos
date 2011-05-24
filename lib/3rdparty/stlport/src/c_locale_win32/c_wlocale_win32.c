/*
 * Copyright (c) 2007 2008
 * Francois Dumont
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#if defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
#  define _STLP_WCSNCPY(D, DS, S, C) wcsncpy_s(D, DS, S, C)
#else
#  define _STLP_WCSNCPY(D, DS, S, C) wcsncpy(D, S, C)
#endif

static const wchar_t* __wtrue_name = L"true";
static const wchar_t* __wfalse_name = L"false";

typedef struct _Locale_codecvt {
  _Locale_lcid_t lc;
  UINT cp;
  unsigned char cleads[256 / CHAR_BIT];
  unsigned char max_char_size;
  DWORD mbtowc_flags;
  DWORD wctomb_flags;
} _Locale_codecvt_t;

/* Ctype */
_Locale_mask_t _WLocale_ctype(_Locale_ctype_t* ltype, wint_t c,
                              _Locale_mask_t which_bits) {
  wchar_t buf[2];
  WORD out[2];
  buf[0] = c; buf[1] = 0;
  GetStringTypeW(CT_CTYPE1, buf, -1, out);
  _STLP_MARK_PARAMETER_AS_UNUSED(ltype)
  return (_Locale_mask_t)(MapCtypeMask(out[0]) & which_bits);
}

wint_t _WLocale_tolower(_Locale_ctype_t* ltype, wint_t c) {
  wchar_t in_c = c;
  wchar_t res;

  LCMapStringW(ltype->lc.id, LCMAP_LOWERCASE, &in_c, 1, &res, 1);
  return res;
}

wint_t _WLocale_toupper(_Locale_ctype_t* ltype, wint_t c) {
  wchar_t in_c = c;
  wchar_t res;

  LCMapStringW(ltype->lc.id, LCMAP_UPPERCASE, &in_c, 1, &res, 1);
  return res;
}

_Locale_codecvt_t* _Locale_codecvt_create(const char * name, _Locale_lcid_t* lc_hint, int *__err_code) {
  char cp_name[MAX_CP_LEN + 1];
  unsigned char *ptr;
  CPINFO CPInfo;
  int i;

  _Locale_codecvt_t *lcodecvt = (_Locale_codecvt_t*)malloc(sizeof(_Locale_codecvt_t));

  if (!lcodecvt) { *__err_code = _STLP_LOC_NO_MEMORY; return lcodecvt; }
  memset(lcodecvt, 0, sizeof(_Locale_codecvt_t));

  if (__GetLCIDFromName(name, &lcodecvt->lc.id, cp_name, lc_hint) == -1)
  { free(lcodecvt); *__err_code = _STLP_LOC_UNKNOWN_NAME; return NULL; }

  lcodecvt->cp = atoi(cp_name);
  if (!GetCPInfo(lcodecvt->cp, &CPInfo)) { free(lcodecvt); return NULL; }

  if (lcodecvt->cp != CP_UTF7 && lcodecvt->cp != CP_UTF8) {
    lcodecvt->mbtowc_flags = MB_PRECOMPOSED;
    lcodecvt->wctomb_flags = WC_COMPOSITECHECK | WC_SEPCHARS;
  }
  lcodecvt->max_char_size = CPInfo.MaxCharSize;

  if (CPInfo.MaxCharSize > 1) {
    for (ptr = (unsigned char*)CPInfo.LeadByte; *ptr && *(ptr + 1); ptr += 2)
      for (i = *ptr; i <= *(ptr + 1); ++i) lcodecvt->cleads[i / CHAR_BIT] |= (0x01 << i % CHAR_BIT);
  }

  return lcodecvt;
}

char const* _Locale_codecvt_name(const _Locale_codecvt_t* lcodecvt, char* buf) {
  char cp_buf[MAX_CP_LEN + 1];
  my_ltoa(lcodecvt->cp, cp_buf);
  return __GetLocaleName(lcodecvt->lc.id, cp_buf, buf);
}

void _Locale_codecvt_destroy(_Locale_codecvt_t* lcodecvt) {
  if (!lcodecvt) return;

  free(lcodecvt);
}

int _WLocale_mb_cur_max (_Locale_codecvt_t * lcodecvt)
{ return lcodecvt->max_char_size; }

int _WLocale_mb_cur_min (_Locale_codecvt_t *lcodecvt) {
  _STLP_MARK_PARAMETER_AS_UNUSED(lcodecvt)
  return 1;
}

int _WLocale_is_stateless (_Locale_codecvt_t * lcodecvt)
{ return (lcodecvt->max_char_size == 1) ? 1 : 0; }

static int __isleadbyte(int i, unsigned char *ctable) {
  unsigned char c = (unsigned char)i;
  return (ctable[c / CHAR_BIT] & (0x01 << c % CHAR_BIT));
}

static int __mbtowc(_Locale_codecvt_t *l, wchar_t *dst, const char *from, unsigned int count) {
  int result;

  if (l->cp == CP_UTF7 || l->cp == CP_UTF8) {
    result = MultiByteToWideChar(l->cp, l->mbtowc_flags, from, count, dst, 1);
    if (result == 0) {
      switch (GetLastError()) {
        case ERROR_NO_UNICODE_TRANSLATION:
          return -2;
        default:
          return -1;
      }
    }
  }
  else {
    if (count == 1 && __isleadbyte(*from, l->cleads)) return (size_t)-2;
    result = MultiByteToWideChar(l->cp, l->mbtowc_flags, from, count, dst, 1);
    if (result == 0) return -1;
  }

  return result;
}

size_t _WLocale_mbtowc(_Locale_codecvt_t *lcodecvt, wchar_t *to,
                       const char *from, size_t n, mbstate_t *shift_state) {
  int result;
  _STLP_MARK_PARAMETER_AS_UNUSED(shift_state)
  if (lcodecvt->max_char_size == 1) { /* Single byte encoding. */
    result = MultiByteToWideChar(lcodecvt->cp, lcodecvt->mbtowc_flags, from, 1, to, 1);
    if (result == 0) return (size_t)-1;
    return result;
  }
  else { /* Multi byte encoding. */
    int retval;
    unsigned int count = 1;
    while (n--) {
      retval = __mbtowc(lcodecvt, to, from, count);
      if (retval == -2)
      { if (++count > ((unsigned int)lcodecvt->max_char_size)) return (size_t)-1; }
      else if (retval == -1)
      { return (size_t)-1; }
      else
      { return count; }
    }
    return (size_t)-2;
  }
}

size_t _WLocale_wctomb(_Locale_codecvt_t *lcodecvt, char *to, size_t n,
                       const wchar_t c, mbstate_t *shift_state) {
  int size = WideCharToMultiByte(lcodecvt->cp, lcodecvt->wctomb_flags, &c, 1, NULL, 0, NULL, NULL);

  if (!size) return (size_t)-1;
  if ((size_t)size > n) return (size_t)-2;

  if (n > INT_MAX)
    /* Limiting the output buf size to INT_MAX seems like reasonable to transform a single wchar_t. */
    n = INT_MAX;

  WideCharToMultiByte(lcodecvt->cp,  lcodecvt->wctomb_flags, &c, 1, to, (int)n, NULL, NULL);

  _STLP_MARK_PARAMETER_AS_UNUSED(shift_state)
  return (size_t)size;
}

size_t _WLocale_unshift(_Locale_codecvt_t *lcodecvt, mbstate_t *st,
                        char *buf, size_t n, char **next) {
  /* _WLocale_wctomb do not even touch to st, there is nothing to unshift in this localization implementation. */
  _STLP_MARK_PARAMETER_AS_UNUSED(lcodecvt)
  _STLP_MARK_PARAMETER_AS_UNUSED(st)
  _STLP_MARK_PARAMETER_AS_UNUSED(&n)
  *next = buf;
  return 0;
}

/* Collate */
/* This function takes care of the potential size_t DWORD different size. */
static int _WLocale_strcmp_aux(_Locale_collate_t* lcol,
                               const wchar_t* s1, size_t n1,
                               const wchar_t* s2, size_t n2) {
  int result = CSTR_EQUAL;
  while (n1 > 0 || n2 > 0) {
    DWORD size1 = trim_size_t_to_DWORD(n1);
    DWORD size2 = trim_size_t_to_DWORD(n2);
    result = CompareStringW(lcol->lc.id, 0, s1, size1, s2, size2);
    if (result != CSTR_EQUAL)
      break;
    n1 -= size1;
    n2 -= size2;
  }
  return result;
}

int _WLocale_strcmp(_Locale_collate_t* lcol,
                    const wchar_t* s1, size_t n1,
                    const wchar_t* s2, size_t n2) {
  int result;
  result = _WLocale_strcmp_aux(lcol, s1, n1, s2, n2);
  return (result == CSTR_EQUAL) ? 0 : (result == CSTR_LESS_THAN) ? -1 : 1;
}

size_t _WLocale_strxfrm(_Locale_collate_t* lcol,
                        wchar_t* dst, size_t dst_size,
                        const wchar_t* src, size_t src_size) {
  int result, i;

  /* see _Locale_strxfrm: */
  if (src_size > INT_MAX) {
    if (dst != 0) {
      _STLP_WCSNCPY(dst, dst_size, src, src_size);
    }
    return src_size;
  }
  if (dst_size > INT_MAX) {
    dst_size = INT_MAX;
  }
  result = LCMapStringW(lcol->lc.id, LCMAP_SORTKEY, src, (int)src_size, dst, (int)dst_size);
  if (result != 0 && dst != 0) {
    for (i = result - 1; i >= 0; --i) {
      dst[i] = ((unsigned char*)dst)[i];
    }
  }
  return result != 0 ? result - 1 : 0;
}

/* Numeric */
wchar_t _WLocale_decimal_point(_Locale_numeric_t* lnum) {
  wchar_t buf[4];
  GetLocaleInfoW(lnum->lc.id, LOCALE_SDECIMAL, buf, 4);
  return buf[0];
}

wchar_t _WLocale_thousands_sep(_Locale_numeric_t* lnum) {
  wchar_t buf[4];
  GetLocaleInfoW(lnum->lc.id, LOCALE_STHOUSAND, buf, 4);
  return buf[0];
}

const wchar_t * _WLocale_true(_Locale_numeric_t* lnum, wchar_t* buf, size_t bufSize) {
  _STLP_MARK_PARAMETER_AS_UNUSED(lnum)
  _STLP_MARK_PARAMETER_AS_UNUSED(buf)
  _STLP_MARK_PARAMETER_AS_UNUSED(&bufSize)
  return __wtrue_name;
}

const wchar_t * _WLocale_false(_Locale_numeric_t* lnum, wchar_t* buf, size_t bufSize) {
  _STLP_MARK_PARAMETER_AS_UNUSED(lnum)
  _STLP_MARK_PARAMETER_AS_UNUSED(buf)
  _STLP_MARK_PARAMETER_AS_UNUSED(&bufSize)
  return __wfalse_name;
}

/* Monetary */
const wchar_t* _WLocale_int_curr_symbol(_Locale_monetary_t * lmon, wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(lmon->lc.id, LOCALE_SINTLSYMBOL, buf, (int)bufSize); return buf; }

const wchar_t* _WLocale_currency_symbol(_Locale_monetary_t * lmon, wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(lmon->lc.id, LOCALE_SCURRENCY, buf, (int)bufSize); return buf; }

wchar_t _WLocale_mon_decimal_point(_Locale_monetary_t * lmon)
{ return lmon->decimal_point[0]; }

wchar_t _WLocale_mon_thousands_sep(_Locale_monetary_t * lmon)
{ return lmon->thousands_sep[0]; }

const wchar_t* _WLocale_positive_sign(_Locale_monetary_t * lmon, wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(lmon->lc.id, LOCALE_SPOSITIVESIGN, buf, (int)bufSize); return buf; }

const wchar_t* _WLocale_negative_sign(_Locale_monetary_t * lmon, wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(lmon->lc.id, LOCALE_SNEGATIVESIGN, buf, (int)bufSize); return buf; }

/* Time */
const wchar_t * _WLocale_full_monthname(_Locale_time_t * ltime, int month,
                                        wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(ltime->lc.id, LOCALE_SMONTHNAME1 + month, buf, (int)bufSize); return buf; }

const wchar_t * _WLocale_abbrev_monthname(_Locale_time_t * ltime, int month,
                                          wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(ltime->lc.id, LOCALE_SABBREVMONTHNAME1 + month, buf, (int)bufSize); return buf; }

const wchar_t * _WLocale_full_dayofweek(_Locale_time_t * ltime, int day,
                                        wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(ltime->lc.id, LOCALE_SDAYNAME1 + day, buf, (int)bufSize); return buf; }

const wchar_t * _WLocale_abbrev_dayofweek(_Locale_time_t * ltime, int day,
                                          wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(ltime->lc.id, LOCALE_SABBREVDAYNAME1 + day, buf, (int)bufSize); return buf; }

const wchar_t* _WLocale_am_str(_Locale_time_t* ltime,
                               wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(ltime->lc.id, LOCALE_S1159, buf, (int)bufSize); return buf; }

const wchar_t* _WLocale_pm_str(_Locale_time_t* ltime,
                               wchar_t* buf, size_t bufSize)
{ GetLocaleInfoW(ltime->lc.id, LOCALE_S2359, buf, (int)bufSize); return buf; }
