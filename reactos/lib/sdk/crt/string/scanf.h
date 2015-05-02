/*
 * general implementation of scanf used by scanf, sscanf, fscanf,
 * _cscanf, wscanf, swscanf and fwscanf
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000, 2003 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2002 Daniel Gudbjartsson
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

#ifdef WIDE_SCANF
#define _CHAR_ wchar_t
#define _EOF_ WEOF
#define _EOF_RET (short)WEOF
#define _ISSPACE_(c) iswspace(c)
#define _ISDIGIT_(c) iswdigit(c)
#define _WIDE2SUPPORTED_(c) c /* No conversion needed (wide to wide) */
#define _CHAR2SUPPORTED_(c) c /* FIXME: convert char to wide char */
#define _CHAR2DIGIT_(c, base) wchar2digit((c), (base))
#define _BITMAPSIZE_ 256*256
#else /* WIDE_SCANF */
#define _CHAR_ char
#define _EOF_ EOF
#define _EOF_RET EOF
#define _ISSPACE_(c) isspace(c)
#define _ISDIGIT_(c) isdigit(c)
#define _WIDE2SUPPORTED_(c) c /* FIXME: convert wide char to char */
#define _CHAR2SUPPORTED_(c) c /* No conversion needed (char to char) */
#define _CHAR2DIGIT_(c, base) char2digit((c), (base))
#define _BITMAPSIZE_ 256
#endif /* WIDE_SCANF */

#ifdef CONSOLE
#define _GETC_(file) (consumed++, _getch())
#define _UNGETC_(nch, file) do { _ungetch(nch); consumed--; } while(0)
#define _LOCK_FILE_(file) _lock_file(stdin)
#define _UNLOCK_FILE_(file) _unlock_file(stdin)
#ifdef WIDE_SCANF
#ifdef SECURE
#define _FUNCTION_ static int vcwscanf_s_l(const char *format, _locale_t locale, __ms_va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vcwscanf_l(const char *format, _locale_t locale, __ms_va_list ap)
#endif /* SECURE */
#else  /* WIDE_SCANF */
#ifdef SECURE
#define _FUNCTION_ static int vcscanf_s_l(const char *format, _locale_t locale, __ms_va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vcscanf_l(const char *format, _locale_t locale, __ms_va_list ap)
#endif /* SECURE */
#endif /* WIDE_SCANF */
#else
#ifdef STRING
#undef _EOF_
#define _EOF_ 0
#ifdef STRING_LEN
#define _GETC_(file) (consumed==length ? '\0' : (consumed++, *file++))
#define _UNGETC_(nch, file) do { file--; consumed--; } while(0)
#define _LOCK_FILE_(file) do {} while(0)
#define _UNLOCK_FILE_(file) do {} while(0)
#ifdef WIDE_SCANF
#ifdef SECURE
#define _FUNCTION_ static int vsnwscanf_s_l(const wchar_t *file, size_t length, const wchar_t *format, _locale_t locale, __ms_va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vsnwscanf_l(const wchar_t *file, size_t length, const wchar_t *format, _locale_t locale, __ms_va_list ap)
#endif /* SECURE */
#else /* WIDE_SCANF */
#ifdef SECURE
#define _FUNCTION_ static int vsnscanf_s_l(const char *file, size_t length, const char *format, _locale_t locale, __ms_va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vsnscanf_l(const char *file, size_t length, const char *format, _locale_t locale, __ms_va_list ap)
#endif /* SECURE */
#endif /* WIDE_SCANF */
#else /* STRING_LEN */
#define _GETC_(file) (consumed++, *file++)
#define _UNGETC_(nch, file) do { file--; consumed--; } while(0)
#define _LOCK_FILE_(file) do {} while(0)
#define _UNLOCK_FILE_(file) do {} while(0)
#ifdef WIDE_SCANF
#ifdef SECURE
#define _FUNCTION_ static int vswscanf_s_l(const wchar_t *file, const wchar_t *format, _locale_t locale, __ms_va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vswscanf_l(const wchar_t *file, const wchar_t *format, _locale_t locale, __ms_va_list ap)
#endif /* SECURE */
#else /* WIDE_SCANF */
#ifdef SECURE
#define _FUNCTION_ static int vsscanf_s_l(const char *file, const char *format, _locale_t locale, __ms_va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vsscanf_l(const char *file, const char *format, _locale_t locale, __ms_va_list ap)
#endif /* SECURE */
#endif /* WIDE_SCANF */
#endif /* STRING_LEN */
#else /* STRING */
#ifdef WIDE_SCANF
#define _GETC_(file) (consumed++, fgetwc(file))
#define _UNGETC_(nch, file) do { ungetwc(nch, file); consumed--; } while(0)
#define _LOCK_FILE_(file) _lock_file(file)
#define _UNLOCK_FILE_(file) _unlock_file(file)
#ifdef SECURE
#define _FUNCTION_ static int vfwscanf_s_l(FILE* file, const wchar_t *format, _locale_t locale, __ms_va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vfwscanf_l(FILE* file, const wchar_t *format, _locale_t locale, __ms_va_list ap)
#endif /* SECURE */
#else /* WIDE_SCANF */
#define _GETC_(file) (consumed++, fgetc(file))
#define _UNGETC_(nch, file) do { ungetc(nch, file); consumed--; } while(0)
#define _LOCK_FILE_(file) _lock_file(file)
#define _UNLOCK_FILE_(file) _unlock_file(file)
#ifdef SECURE
#define _FUNCTION_ static int vfscanf_s_l(FILE* file, const char *format, _locale_t locale, __ms_va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vfscanf_l(FILE* file, const char *format, _locale_t locale, __ms_va_list ap)
#endif /* SECURE */
#endif /* WIDE_SCANF */
#endif /* STRING */
#endif /* CONSOLE */

_FUNCTION_ {
    pthreadlocinfo locinfo;
    int rd = 0, consumed = 0;
    int nch;
    if (!*format) return 0;
#ifndef WIDE_SCANF
#ifdef CONSOLE
    TRACE("(%s):\n", debugstr_a(format));
#else /* CONSOLE */
#ifdef STRING
    TRACE("%s (%s)\n", debugstr_a(file), debugstr_a(format));
#else /* STRING */
    TRACE("%p (%s)\n", file, debugstr_a(format));
#endif /* STRING */
#endif /* CONSOLE */
#endif /* WIDE_SCANF */
    _LOCK_FILE_(file);

    nch = _GETC_(file);
    if (nch == _EOF_) {
        _UNLOCK_FILE_(file);
        return _EOF_RET;
    }

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    while (*format) {
	/* a whitespace character in the format string causes scanf to read,
	 * but not store, all consecutive white-space characters in the input
	 * up to the next non-white-space character.  One white space character
	 * in the input matches any number (including zero) and combination of
	 * white-space characters in the input. */
	if (_ISSPACE_(*format)) {
            /* skip whitespace */
            while ((nch!=_EOF_) && _ISSPACE_(nch))
                nch = _GETC_(file);
        }
	/* a format specification causes scanf to read and convert characters
	 * in the input into values of a specified type.  The value is assigned
	 * to an argument in the argument list.  Format specifications have
	 * the form %[*][width][{h | l | I64 | L}]type */
        else if (*format == '%') {
            int st = 0; int suppress = 0; int width = 0;
	    int base;
	    int h_prefix = 0;
	    int l_prefix = 0;
	    int L_prefix = 0;
	    int w_prefix = 0;
	    int prefix_finished = 0;
	    int I64_prefix = 0;
            format++;
	    /* look for leading asterisk, which means 'suppress assignment of
	     * this field'. */
	    if (*format=='*') {
		format++;
		suppress=1;
	    }
	    /* look for width specification */
	    while (_ISDIGIT_(*format)) {
		width*=10;
		width+=*format++ - '0';
	    }
	    if (width==0) width=-1; /* no width spec seen */
	    /* read prefix (if any) */
	    while (!prefix_finished) {
		switch(*format) {
		case 'h': h_prefix++; break;
		case 'l':
                    if(*(format+1) == 'l') {
                        I64_prefix = 1;
                        format++;
                    }
                    l_prefix = 1;
                    break;
		case 'w': w_prefix = 1; break;
		case 'L': L_prefix = 1; break;
		case 'I':
		    if (*(format + 1) == '6' &&
			*(format + 2) == '4') {
			I64_prefix = 1;
			format += 2;
		    }
		    break;
		default:
		    prefix_finished = 1;
		}
		if (!prefix_finished) format++;
	    }
	    /* read type */
            switch(*format) {
	    case 'p':
	    case 'P': /* pointer. */
                if (sizeof(void *) == sizeof(LONGLONG)) I64_prefix = 1;
                /* fall through */
	    case 'x':
	    case 'X': /* hexadecimal integer. */
		base = 16;
		goto number;
	    case 'o': /* octal integer */
		base = 8;
		goto number;
	    case 'u': /* unsigned decimal integer */
		base = 10;
		goto number;
	    case 'd': /* signed decimal integer */
		base = 10;
		goto number;
	    case 'i': /* generic integer */
		base = 0;
	    number: {
		    /* read an integer */
		    ULONGLONG cur = 0;
		    int negative = 0;
		    int seendigit=0;
                    /* skip initial whitespace */
                    while ((nch!=_EOF_) && _ISSPACE_(nch))
                        nch = _GETC_(file);
                    /* get sign */
                    if (nch == '-' || nch == '+') {
			negative = (nch=='-');
                        nch = _GETC_(file);
			if (width>0) width--;
                    }
		    /* look for leading indication of base */
		    if (width!=0 && nch == '0' && *format != 'p' && *format != 'P') {
                        nch = _GETC_(file);
			if (width>0) width--;
			seendigit=1;
			if (width!=0 && (nch=='x' || nch=='X')) {
			    if (base==0)
				base=16;
			    if (base==16) {
				nch = _GETC_(file);
				if (width>0) width--;
				seendigit=0;
			    }
			} else if (base==0)
			    base = 8;
		    }
		    /* format %i without indication of base */
		    if (base==0)
			base = 10;
		    /* throw away leading zeros */
		    while (width!=0 && nch=='0') {
                        nch = _GETC_(file);
			if (width>0) width--;
			seendigit=1;
		    }
		    if (width!=0 && _CHAR2DIGIT_(nch, base)!=-1) {
			cur = _CHAR2DIGIT_(nch, base);
			nch = _GETC_(file);
			if (width>0) width--;
			seendigit=1;
		    }
                    /* read until no more digits */
                    while (width!=0 && (nch!=_EOF_) && _CHAR2DIGIT_(nch, base)!=-1) {
                        cur = cur*base + _CHAR2DIGIT_(nch, base);
                        nch = _GETC_(file);
			if (width>0) width--;
			seendigit=1;
                    }
		    /* okay, done! */
		    if (!seendigit) break; /* not a valid number */
                    st = 1;
                    if (!suppress) {
#define _SET_NUMBER_(type) *va_arg(ap, type*) = negative ? -(LONGLONG)cur : cur
			if (I64_prefix) _SET_NUMBER_(LONGLONG);
			else if (l_prefix) _SET_NUMBER_(LONG);
			else if (h_prefix == 1) _SET_NUMBER_(short int);
			else _SET_NUMBER_(int);
		    }
                }
                break;
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G': { /* read a float */
                    //long double cur = 1, expcnt = 10;
                    ULONGLONG d, hlp;
                    int exp = 0, negative = 0;
                    //unsigned fpcontrol;
                    //BOOL negexp;

                    /* skip initial whitespace */
                    while ((nch!=_EOF_) && _ISSPACE_(nch))
                        nch = _GETC_(file);

                    /* get sign. */
                    if (nch == '-' || nch == '+') {
                        negative = (nch=='-');
                        if (width>0) width--;
                        if (width==0) break;
                        nch = _GETC_(file);
                    }

                    /* get first digit. */
                    if (*locinfo->lconv->decimal_point != nch) {
                        if (!_ISDIGIT_(nch)) break;
                        d = nch - '0';
                        nch = _GETC_(file);
                        if (width>0) width--;
                        /* read until no more digits */
                        while (width!=0 && (nch!=_EOF_) && _ISDIGIT_(nch)) {
                            hlp = d*10 + nch - '0';
                            nch = _GETC_(file);
                            if (width>0) width--;
                            if(d > (ULONGLONG)-1/10 || hlp<d) {
                                exp++;
                                break;
                            }
                            else
                                d = hlp;
                        }
                        while (width!=0 && (nch!=_EOF_) && _ISDIGIT_(nch)) {
                            exp++;
                            nch = _GETC_(file);
                            if (width>0) width--;
                        }
                    } else {
                        d = 0; /* Fix: .8 -> 0.8 */
                    }

                    /* handle decimals */
                    if (width!=0 && nch == *locinfo->lconv->decimal_point) {
                        nch = _GETC_(file);
                        if (width>0) width--;

                        while (width!=0 && (nch!=_EOF_) && _ISDIGIT_(nch)) {
                            hlp = d*10 + nch - '0';
                            nch = _GETC_(file);
                            if (width>0) width--;
                            if(d > (ULONGLONG)-1/10 || hlp<d)
                                break;

                            d = hlp;
                            exp--;
                        }
                        while (width!=0 && (nch!=_EOF_) && _ISDIGIT_(nch)) {
                            nch = _GETC_(file);
                            if (width>0) width--;
                        }
                    }

                    /* handle exponent */
                    if (width!=0 && (nch == 'e' || nch == 'E')) {
                        int sign = 1, e = 0;

                        nch = _GETC_(file);
                        if (width>0) width--;
                        if (width!=0 && (nch=='+' || nch=='-')) {
                            if(nch == '-')
                                sign = -1;
                            nch = _GETC_(file);
                            if (width>0) width--;
                        }

                        /* exponent digits */
                        while (width!=0 && (nch!=_EOF_) && _ISDIGIT_(nch)) {
                            if(e > INT_MAX/10 || (e = e*10 + nch - '0')<0)
                                e = INT_MAX;
                            nch = _GETC_(file);
                            if (width>0) width--;
                        }
                        e *= sign;

                        if(exp<0 && e<0 && e+exp>0) exp = INT_MIN;
                        else if(exp>0 && e>0 && e+exp<0) exp = INT_MAX;
                        else exp += e;
                    }

#ifdef __REACTOS__
                    /* ReactOS: don't inline float processing (kernel/freeldr don't like that! */
                    _internal_handle_float(negative, exp, suppress, d, l_prefix || L_prefix, &ap);
                    st = 1;
#else
                    fpcontrol = _control87(0, 0);
                    _control87(MSVCRT__EM_DENORMAL|MSVCRT__EM_INVALID|MSVCRT__EM_ZERODIVIDE
                            |MSVCRT__EM_OVERFLOW|MSVCRT__EM_UNDERFLOW|MSVCRT__EM_INEXACT, 0xffffffff);

                    negexp = (exp < 0);
                    if(negexp)
                        exp = -exp;
                    /* update 'cur' with this exponent. */
                    while(exp) {
                        if(exp & 1)
                            cur *= expcnt;
                        exp /= 2;
                        expcnt = expcnt*expcnt;
                    }
                    cur = (negexp ? d/cur : d*cur);

                    _control87(fpcontrol, 0xffffffff);

                    st = 1;
                    if (!suppress) {
                        if (L_prefix) _SET_NUMBER_(double);
                        else if (l_prefix) _SET_NUMBER_(double);
                        else _SET_NUMBER_(float);
                    }
#endif /* __REACTOS__ */
                }
                break;
		/* According to msdn,
		 * 's' reads a character string in a call to fscanf
		 * and 'S' a wide character string and vice versa in a
		 * call to fwscanf. The 'h', 'w' and 'l' prefixes override
		 * this behaviour. 'h' forces reading char * but 'l' and 'w'
		 * force reading WCHAR. */
	    case 's':
		    if (w_prefix || l_prefix) goto widecharstring;
		    else if (h_prefix) goto charstring;
#ifdef WIDE_SCANF
		    else goto widecharstring;
#else /* WIDE_SCANF */
		    else goto charstring;
#endif /* WIDE_SCANF */
	    case 'S':
		    if (w_prefix || l_prefix) goto widecharstring;
		    else if (h_prefix) goto charstring;
#ifdef WIDE_SCANF
		    else goto charstring;
#else /* WIDE_SCANF */
		    else goto widecharstring;
#endif /* WIDE_SCANF */
	    charstring: { /* read a word into a char */
		    char *sptr = suppress ? NULL : va_arg(ap, char*);
                    char *sptr_beg = sptr;
#ifdef SECURE
                    unsigned size = suppress ? UINT_MAX : va_arg(ap, unsigned);
#else
                    unsigned size = UINT_MAX;
#endif
                    /* skip initial whitespace */
                    while ((nch!=_EOF_) && _ISSPACE_(nch))
                        nch = _GETC_(file);
                    /* read until whitespace */
                    while (width!=0 && (nch!=_EOF_) && !_ISSPACE_(nch)) {
                        if (!suppress) {
                            *sptr++ = _CHAR2SUPPORTED_(nch);
                            if(size>1) size--;
                            else {
                                _UNLOCK_FILE_(file);
                                *sptr_beg = 0;
                                return rd;
                            }
                        }
			st++;
                        nch = _GETC_(file);
			if (width>0) width--;
                    }
                    /* terminate */
                    if (st && !suppress) *sptr = 0;
                }
                break;
	    widecharstring: { /* read a word into a wchar_t* */
		    wchar_t *sptr = suppress ? NULL : va_arg(ap, wchar_t*);
                    wchar_t *sptr_beg = sptr;
#ifdef SECURE
                    unsigned size = suppress ? UINT_MAX : va_arg(ap, unsigned);
#else
                    unsigned size = UINT_MAX;
#endif
                    /* skip initial whitespace */
                    while ((nch!=_EOF_) && _ISSPACE_(nch))
                        nch = _GETC_(file);
                    /* read until whitespace */
                    while (width!=0 && (nch!=_EOF_) && !_ISSPACE_(nch)) {
                        if (!suppress) {
                            *sptr++ = _WIDE2SUPPORTED_(nch);
                            if(size>1) size--;
                            else {
                                _UNLOCK_FILE_(file);
                                *sptr_beg = 0;
                                return rd;
                            }
                        }
			st++;
                        nch = _GETC_(file);
			if (width>0) width--;
                    }
                    /* terminate */
                    if (st && !suppress) *sptr = 0;
                }
                break;
            /* 'c' and 'C work analogously to 's' and 'S' as described
	     * above */
	    case 'c':
		    if (w_prefix || l_prefix) goto widecharacter;
		    else if (h_prefix) goto character;
#ifdef WIDE_SCANF
		    else goto widecharacter;
#else /* WIDE_SCANF */
		    else goto character;
#endif /* WIDE_SCANF */
	    case 'C':
		    if (w_prefix || l_prefix) goto widecharacter;
		    else if (h_prefix) goto character;
#ifdef WIDE_SCANF
		    else goto character;
#else /* WIDE_SCANF */
		    else goto widecharacter;
#endif /* WIDE_SCANF */
	  character: { /* read single character into char */
                    char *str = suppress ? NULL : va_arg(ap, char*);
                    char *pstr = str;
#ifdef SECURE
                    unsigned size = suppress ? UINT_MAX : va_arg(ap, unsigned)/sizeof(char);
#else
                    unsigned size = UINT_MAX;
#endif
                    if (width == -1) width = 1;
                    while (width && (nch != _EOF_))
                    {
                        if (!suppress) {
                            *str++ = _CHAR2SUPPORTED_(nch);
                            if(size) size--;
                            else {
                                _UNLOCK_FILE_(file);
                                *pstr = 0;
                                return rd;
                            }
                        }
                        st++;
                        width--;
                        nch = _GETC_(file);
                    }
                }
		break;
	  widecharacter: { /* read single character into a wchar_t */
                    wchar_t *str = suppress ? NULL : va_arg(ap, wchar_t*);
                    wchar_t *pstr = str;
#ifdef SECURE
                    unsigned size = suppress ? UINT_MAX : va_arg(ap, unsigned)/sizeof(wchar_t);
#else
                    unsigned size = UINT_MAX;
#endif
                    if (width == -1) width = 1;
                    while (width && (nch != _EOF_))
                    {
                        if (!suppress) {
                            *str++ = _WIDE2SUPPORTED_(nch);
                            if(size) size--;
                            else {
                                _UNLOCK_FILE_(file);
                                *pstr = 0;
                                return rd;
                            }
                        }
                        st++;
                        width--;
                        nch = _GETC_(file);
                    }
	        }
		break;
	    case 'n': {
 		    if (!suppress) {
			int*n = va_arg(ap, int*);
			*n = consumed - 1;
		    }
		    /* This is an odd one: according to the standard,
		     * "Execution of a %n directive does not increment the
		     * assignment count returned at the completion of
		     * execution" even if it wasn't suppressed with the
		     * '*' flag.  The Corrigendum to the standard seems
		     * to contradict this (comment out the assignment to
		     * suppress below if you want to implement these
		     * alternate semantics) but the windows program I'm
		     * looking at expects the behavior I've coded here
		     * (which happens to be what glibc does as well).
		     */
		    suppress = 1;
		    st = 1;
	        }
		break;
	    case '[': {
                    _CHAR_ *str = suppress ? NULL : va_arg(ap, _CHAR_*);
                    _CHAR_ *sptr = str;
		    RTL_BITMAP bitMask;
                    ULONG *Mask;
		    int invert = 0; /* Set if we are NOT to find the chars */
#ifdef SECURE
                    unsigned size = suppress ? UINT_MAX : va_arg(ap, unsigned)/sizeof(_CHAR_);
#else
                    unsigned size = UINT_MAX;
#endif

		    /* Init our bitmap */
		    Mask = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _BITMAPSIZE_/8);
		    RtlInitializeBitMap(&bitMask, Mask, _BITMAPSIZE_);

		    /* Read the format */
		    format++;
		    if(*format == '^') {
			invert = 1;
			format++;
		    }
		    if(*format == ']') {
			RtlSetBits(&bitMask, ']', 1);
			format++;
		    }
                    while(*format && (*format != ']')) {
			/* According to msdn:
			 * "Note that %[a-z] and %[z-a] are interpreted as equivalent to %[abcde...z]." */
			if((*format == '-') && (*(format + 1) != ']')) {
			    if ((*(format - 1)) < *(format + 1))
				RtlSetBits(&bitMask, *(format - 1) +1 , *(format + 1) - *(format - 1));
			    else
				RtlSetBits(&bitMask, *(format + 1)    , *(format - 1) - *(format + 1));
			    format++;
			} else
			    RtlSetBits(&bitMask, *format, 1);
			format++;
		    }
                    /* read until char is not suitable */
                    while ((width != 0) && (nch != _EOF_)) {
			if(!invert) {
			    if(RtlAreBitsSet(&bitMask, nch, 1)) {
				if (!suppress) *sptr++ = _CHAR2SUPPORTED_(nch);
			    } else
				break;
			} else {
			    if(RtlAreBitsClear(&bitMask, nch, 1)) {
				if (!suppress) *sptr++ = _CHAR2SUPPORTED_(nch);
			    } else
				break;
			}
                        st++;
                        nch = _GETC_(file);
                        if (width>0) width--;
                        if(size>1) size--;
                        else {
                            _UNLOCK_FILE_(file);
                            *str = 0;
                            return rd;
                        }
                    }
                    /* terminate */
                    if (!suppress) *sptr = 0;
		    HeapFree(GetProcessHeap(), 0, Mask);
                }
                break;
            default:
		/* From spec: "if a percent sign is followed by a character
		 * that has no meaning as a format-control character, that
		 * character and the following characters are treated as
		 * an ordinary sequence of characters, that is, a sequence
		 * of characters that must match the input.  For example,
		 * to specify that a percent-sign character is to be input,
		 * use %%." */
                while ((nch!=_EOF_) && _ISSPACE_(nch))
                    nch = _GETC_(file);
                if (nch==*format) {
                    suppress = 1; /* whoops no field to be read */
                    st = 1; /* but we got what we expected */
                    nch = _GETC_(file);
                }
                break;
            }
            if (st && !suppress) rd++;
            else if (!st) break;
        }
	/* a non-white-space character causes scanf to read, but not store,
	 * a matching non-white-space character. */
        else {
            /* check for character match */
            if (nch == *format) {
		nch = _GETC_(file);
            } else break;
        }
        format++;
    }
    if (nch!=_EOF_) {
	_UNGETC_(nch, file);
    }

    TRACE("returning %d\n", rd);
    _UNLOCK_FILE_(file);
    return rd;
}

#undef _CHAR_
#undef _EOF_
#undef _EOF_RET
#undef _ISSPACE_
#undef _ISDIGIT_
#undef _CHAR2SUPPORTED_
#undef _WIDE2SUPPORTED_
#undef _CHAR2DIGIT_
#undef _GETC_
#undef _UNGETC_
#undef _LOCK_FILE_
#undef _UNLOCK_FILE_
#undef _FUNCTION_
#undef _BITMAPSIZE_
