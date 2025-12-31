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
#define _WIDE2SUPPORTED_(c) c /* No conversion needed (wide to wide) */
#define _CHAR2SUPPORTED_(c) c /* FIXME: convert char to wide char */
#define _CHAR2DIGIT_(c, base) wchar2digit((c), (base))
#define _BITMAPSIZE_ 256*256
#else /* WIDE_SCANF */
#define _CHAR_ char
#define _EOF_ EOF
#define _EOF_RET EOF
#define _ISSPACE_(c) isspace(c)
#define _WIDE2SUPPORTED_(c) c /* FIXME: convert wide char to char */
#define _CHAR2SUPPORTED_(c) c /* No conversion needed (char to char) */
#define _CHAR2DIGIT_(c, base) char2digit((c), (base))
#define _BITMAPSIZE_ 256
#endif /* WIDE_SCANF */

#ifdef CONSOLE
#define _GETC_FUNC_(file) _getch()
#define _STRTOD_NAME_(func) console_ ## func
#define _GETC_(file) (consumed++, _getch())
#define _UNGETC_(nch, file) do { _ungetch(nch); consumed--; } while(0)
#define _LOCK_FILE_(file) _lock_file(stdin)
#define _UNLOCK_FILE_(file) _unlock_file(stdin)
#ifdef WIDE_SCANF
#ifdef SECURE
#define _FUNCTION_ static int vcwscanf_s_l(const wchar_t *format, _locale_t locale, va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vcwscanf_l(const wchar_t *format, _locale_t locale, va_list ap)
#endif /* SECURE */
#else  /* WIDE_SCANF */
#ifdef SECURE
#define _FUNCTION_ static int vcscanf_s_l(const char *format, _locale_t locale, va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vcscanf_l(const char *format, _locale_t locale, va_list ap)
#endif /* SECURE */
#endif /* WIDE_SCANF */
#else
#ifdef STRING
#undef _EOF_
#define _EOF_ 0
#define _GETC_FUNC_(file) (*file++)
#ifdef WIDE_SCANF
#define _STRTOD_NAME_(func) wstr_ ## func
#else
#define _STRTOD_NAME_(func) str_ ## func
#endif
#ifdef STRING_LEN
#ifdef WIDE_SCANF
#define _GETC_(file) (consumed++, consumed>length ? '\0' : *file++)
#else /* WIDE_SCANF */
#define _GETC_(file) (consumed++, consumed>length ? '\0' : (unsigned char)*file++)
#endif /* WIDE_SCANF */
#define _UNGETC_(nch, file) do { file--; consumed--; } while(0)
#define _LOCK_FILE_(file) do {} while(0)
#define _UNLOCK_FILE_(file) do {} while(0)
#ifdef WIDE_SCANF
#ifdef SECURE
#define _FUNCTION_ static int vsnwscanf_s_l(const wchar_t *file, size_t length, const wchar_t *format, _locale_t locale, va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vsnwscanf_l(const wchar_t *file, size_t length, const wchar_t *format, _locale_t locale, va_list ap)
#endif /* SECURE */
#else /* WIDE_SCANF */
#ifdef SECURE
#define _FUNCTION_ static int vsnscanf_s_l(const char *file, size_t length, const char *format, _locale_t locale, va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vsnscanf_l(const char *file, size_t length, const char *format, _locale_t locale, va_list ap)
#endif /* SECURE */
#endif /* WIDE_SCANF */
#else /* STRING_LEN */
#ifdef WIDE_SCANF
#define _GETC_(file) (consumed++, *file++)
#else /* WIDE_SCANF */
#define _GETC_(file) (consumed++, (unsigned char)*file++)
#endif /* WIDE_SCANF */
#define _UNGETC_(nch, file) do { file--; consumed--; } while(0)
#define _LOCK_FILE_(file) do {} while(0)
#define _UNLOCK_FILE_(file) do {} while(0)
#ifdef WIDE_SCANF
#ifdef SECURE
#define _FUNCTION_ static int vswscanf_s_l(const wchar_t *file, const wchar_t *format, _locale_t locale, va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vswscanf_l(const wchar_t *file, const wchar_t *format, _locale_t locale, va_list ap)
#endif /* SECURE */
#else /* WIDE_SCANF */
#ifdef SECURE
#define _FUNCTION_ static int vsscanf_s_l(const char *file, const char *format, _locale_t locale, va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vsscanf_l(const char *file, const char *format, _locale_t locale, va_list ap)
#endif /* SECURE */
#endif /* WIDE_SCANF */
#endif /* STRING_LEN */
#else /* STRING */
#ifdef WIDE_SCANF
#define _GETC_FUNC_(file) fgetwc(file)
#define _STRTOD_NAME_(func) filew_ ## func
#define _GETC_(file) (consumed++, fgetwc(file))
#define _UNGETC_(nch, file) do { ungetwc(nch, file); consumed--; } while(0)
#define _LOCK_FILE_(file) _lock_file(file)
#define _UNLOCK_FILE_(file) _unlock_file(file)
#ifdef SECURE
#define _FUNCTION_ static int vfwscanf_s_l(FILE* file, const wchar_t *format, _locale_t locale, va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vfwscanf_l(FILE* file, const wchar_t *format, _locale_t locale, va_list ap)
#endif /* SECURE */
#else /* WIDE_SCANF */
#define _GETC_FUNC_(file) fgetc(file)
#define _STRTOD_NAME_(func) file_ ## func
#define _GETC_(file) (consumed++, fgetc(file))
#define _UNGETC_(nch, file) do { ungetc(nch, file); consumed--; } while(0)
#define _LOCK_FILE_(file) _lock_file(file)
#define _UNLOCK_FILE_(file) _unlock_file(file)
#ifdef SECURE
#define _FUNCTION_ static int vfscanf_s_l(FILE* file, const char *format, _locale_t locale, va_list ap)
#else  /* SECURE */
#define _FUNCTION_ static int vfscanf_l(FILE* file, const char *format, _locale_t locale, va_list ap)
#endif /* SECURE */
#endif /* WIDE_SCANF */
#endif /* STRING */
#endif /* CONSOLE */

#if (!defined(SECURE) && !defined(STRING_LEN) && (!defined(CONSOLE) || !defined(WIDE_SCANF)))
struct _STRTOD_NAME_(strtod_scanf_ctx) {
    pthreadlocinfo locinfo;
#ifdef STRING
    const _CHAR_ *file;
#else
    FILE *file;
#endif
    int length;
    int read;
    int cur;
    int unget;
    BOOL err;
};

static wchar_t _STRTOD_NAME_(strtod_scanf_get)(void *ctx)
{
    struct _STRTOD_NAME_(strtod_scanf_ctx) *context = ctx;

    context->cur = _EOF_;
    if (!context->length) return WEOF;
    if (context->unget != _EOF_) {
        context->cur = context->unget;
        context->unget = _EOF_;
    } else {
        context->cur = _GETC_FUNC_(context->file);
        if (context->cur == _EOF_) return WEOF;
    }

    if (context->length > 0) context->length--;
    context->read++;
    return context->cur;
}

static void _STRTOD_NAME_(strtod_scanf_unget)(void *ctx)
{
    struct _STRTOD_NAME_(strtod_scanf_ctx) *context = ctx;

    if (context->length >= 0) context->length++;
    context->read--;
    if (context->unget != _EOF_ || context->cur == _EOF_) {
        context->err = TRUE;
        return;
    }
    context->unget = context->cur;
}
#endif

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
	    /* read prefix (if any) */
	    while (!prefix_finished) {
                /* look for width specification */
                while (*format >= '0' && *format <= '9') {
                    width *= 10;
                    width += *format++ - '0';
                }

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
			break;
		    }
		    else if (*(format + 1) == '3' &&
			     *(format + 2) == '2') {
			format += 2;
			break;
		    }
		    /* fall through */
#if _MSVCR_VER == 0 || _MSVCR_VER >= 140
                case 'z':
#endif
                    if (sizeof(void *) == sizeof(LONGLONG)) I64_prefix = 1;
                    break;
		default:
		    prefix_finished = 1;
		}
		if (!prefix_finished) format++;
	    }
	    if (width==0) width=-1; /* no width spec seen */
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
#define _SET_NUMBER_(type) *va_arg(ap, type*) = negative ? -cur : cur
			if (I64_prefix) _SET_NUMBER_(LONGLONG);
			else if (l_prefix) _SET_NUMBER_(LONG);
			else if (h_prefix == 1) _SET_NUMBER_(short int);
#if _MSVCR_VER >= 140
                        else if (h_prefix == 2) _SET_NUMBER_(char);
#endif
			else _SET_NUMBER_(int);
		    }
                }
                break;
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G': { /* read a float */
#ifdef CONSOLE
                    struct _STRTOD_NAME_(strtod_scanf_ctx) ctx = {locinfo, 0, width};
#else
                    struct _STRTOD_NAME_(strtod_scanf_ctx) ctx = {locinfo, file, width};
#endif
                    int negative = 0;
                    struct fpnum fp;
                    double cur;

                    /* skip initial whitespace */
                    while ((nch!=_EOF_) && _ISSPACE_(nch))
                        nch = _GETC_(file);
                    if (nch == _EOF_)
                        break;
                    ctx.unget = nch;
#ifdef STRING
                    ctx.file = file;
#endif
#ifdef STRING_LEN
                    if(ctx.length > length-consumed+1) ctx.length = length-consumed+1;
#endif

                    fp = fpnum_parse(_STRTOD_NAME_(strtod_scanf_get),
                            _STRTOD_NAME_(strtod_scanf_unget), &ctx, locinfo, FALSE);
                    fpnum_double(&fp, &cur);
                    if(!rd && ctx.err) {
                        _UNLOCK_FILE_(file);
                        return _EOF_RET;
                    }
                    if(ctx.err || !ctx.read)
                        break;
                    consumed += ctx.read;
#ifdef STRING
                    file = ctx.file;
#endif
                    nch = ctx.cur;

                    st = 1;
                    if (!suppress) {
                        if (L_prefix || l_prefix) _SET_NUMBER_(double);
                        else _SET_NUMBER_(float);
                    }
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
                    /* if we have reached the EOF and output nothing then report EOF */
                    if (nch==_EOF_ && rd==0 && st==0) {
                        _UNLOCK_FILE_(file);
                        return _EOF_RET;
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
#if _MSVCR_VER >= 80
                    /* if we have reached the EOF and output nothing then report EOF */
                    if (nch==_EOF_ && rd==0 && st==0) {
                        _UNLOCK_FILE_(file);
                        return _EOF_RET;
                    }
#endif
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
                    unsigned size = suppress ? UINT_MAX : va_arg(ap, unsigned);
#else
                    unsigned size = UINT_MAX;
#endif
                    if (width == -1) width = 1;
                    while (width && (nch != _EOF_))
                    {
                        if (!suppress) {
                            if(size) size--;
                            else {
                                _UNLOCK_FILE_(file);
                                *pstr = 0;
                                return rd;
                            }
                            *str++ = _CHAR2SUPPORTED_(nch);
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
                    unsigned size = suppress ? UINT_MAX : va_arg(ap, unsigned);
#else
                    unsigned size = UINT_MAX;
#endif
                    if (width == -1) width = 1;
                    while (width && (nch != _EOF_))
                    {
                        if (!suppress) {
                            if(size) size--;
                            else {
                                _UNLOCK_FILE_(file);
                                *pstr = 0;
                                return rd;
                            }
                            *str++ = _WIDE2SUPPORTED_(nch);
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
                    unsigned size = suppress ? UINT_MAX : va_arg(ap, unsigned);
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
                        if(format[1] == '-' && format[2] && format[2] != ']') {
                            if (format[0] < format[2])
                                RtlSetBits(&bitMask, format[0], format[2] - format[0] + 1);
                            else
                                RtlSetBits(&bitMask, format[2], format[0] - format[2] + 1);
			    format += 2;
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
                            HeapFree(GetProcessHeap(), 0, Mask);
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
                if ((_CHAR_)nch == *format) {
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
            if ((_CHAR_)nch == *format) {
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
#undef _CHAR2SUPPORTED_
#undef _WIDE2SUPPORTED_
#undef _CHAR2DIGIT_
#undef _GETC_FUNC_
#undef _STRTOD_NAME_
#undef _GETC_
#undef _UNGETC_
#undef _LOCK_FILE_
#undef _UNLOCK_FILE_
#undef _FUNCTION_
#undef _BITMAPSIZE_
