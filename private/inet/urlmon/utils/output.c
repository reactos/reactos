/***
*output.c - printf style output to a struct w4io
*
*   Copyright (c) 1989-1991, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file contains the code that does all the work for the
*   printf family of functions.  It should not be called directly, only
*   by the *printf functions.  We don't make any assumtions about the
*   sizes of ints, longs, shorts, or long doubles, but if types do overlap, we
*   also try to be efficient.  We do assume that pointers are the same size
*   as either ints or longs.
*
*Revision History:
*   06-01-89  PHG   Module created
*   08-28-89  JCR   Added cast to get rid of warning (no object changes)
*   02-15-90  GJF   Fixed copyright
*   10-03-90  WHB   Defined LOCAL(x) to "static x" for local procedures
*   06-05-95  SVA   Added support for printing GUIDs.
*
*******************************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <basetsd.h>
#include "wchar.h"
#include "w4io.h"


/* this macro defines a function which is private and as fast as possible: */
/* for example, in C 6.0, it might be static _fastcall <type>. */
#define LOCAL(x) static x            // 100390--WHB

#define NOFLOATS                        // Win 4 doesn't need floating point

/* int/long/short/pointer sizes */

/* the following should be set depending on the sizes of various types */
// FLAT or LARGE model is assumed
#ifdef FLAT
#  define LONG_IS_INT        1       /* 1 means long is same size as int */
#  define SHORT_IS_INT       0       /* 1 means short is same size as int */
#ifdef _WIN64
#  define PTR_IS_INT         0       /* 1 means ptr is same size as int */
#else  // !_WIN64
#  define PTR_IS_INT         1       /* 1 means ptr is same size as int */
#endif // !_WIN64
#  define PTR_IS_LONG        0       /* 1 means ptr is same size as long */
#else // LARGE model
#  define LONG_IS_INT        0       /* 1 means long is same size as int */
#  define SHORT_IS_INT       1       /* 1 means short is same size as int */
#  define PTR_IS_INT         0       /* 1 means ptr is same size as int */
#  define PTR_IS_LONG        1       /* 1 means ptr is same size as long */
#endif
#define LONGDOUBLE_IS_DOUBLE 0       /* 1 means long double is same as double */

#if LONG_IS_INT
    #define get_long_arg(x) (long)get_int_arg(x)
#endif

#if PTR_IS_INT
    #define get_ptr_arg(x) (void *)get_int_arg(x)
#elif PTR_IS_LONG
    #define get_ptr_arg(x) (void *)get_long_arg(x)
#elif _WIN64
    #define get_ptr_arg(x) (void *)get_int64_arg(x)
#else 
    #error Size of pointer must be same as size of int or long
#endif

#ifndef NOFLOATS
/* These are "fake" double and long doubles to fool the compiler,
   so we don't drag in floating point. */
typedef struct {
    char x[sizeof(double)];
} DOUBLE;
typedef struct {
    char x[sizeof(long double)];
} LONGDOUBLE;
#endif


/* CONSTANTS */

//#define BUFFERSIZE CVTBUFSIZE     /* buffer size for maximum double conv */
#define BUFFERSIZE 20

/* flag definitions */
#define FL_SIGN       0x0001      /* put plus or minus in front */
#define FL_SIGNSP     0x0002      /* put space or minus in front */
#define FL_LEFT       0x0004      /* left justify */
#define FL_LEADZERO   0x0008      /* pad with leading zeros */
#define FL_LONG       0x0010      /* long value given */
#define FL_SHORT      0x0020      /* short value given */
#define FL_SIGNED     0x0040      /* signed data given */
#define FL_ALTERNATE  0x0080      /* alternate form requested */
#define FL_NEGATIVE   0x0100      /* value is negative */
#define FL_FORCEOCTAL 0x0200      /* force leading '0' for octals */
#define FL_LONGDOUBLE 0x0400      /* long double value given */
#define FL_WIDE       0x0800      /* wide character/string given */
#define FL_PTR64      0x1000      /* wide character/string given */

#ifdef _WIN64
#define FL_PTR        FL_PTR64
#else  // !_WIN64
#define FL_PTR        FL_LONG     /* as the processing specified originally... */
#endif // !_WIN64

/* state definitions */
enum STATE {
    ST_NORMAL,              /* normal state; outputting literal chars */
    ST_PERCENT,             /* just read '%' */
    ST_FLAG,                /* just read flag character */
    ST_WIDTH,               /* just read width specifier */
    ST_DOT,                 /* just read '.' */
    ST_PRECIS,              /* just read precision specifier */
    ST_SIZE,                /* just read size specifier */
    ST_TYPE                 /* just read type specifier */
};
#define NUMSTATES (ST_TYPE + 1)

/* character type values */
enum CHARTYPE {
    CH_OTHER,               /* character with no special meaning */
    CH_PERCENT,             /* '%' */
    CH_DOT,                 /* '.' */
    CH_STAR,                /* '*' */
    CH_ZERO,                /* '0' */
    CH_DIGIT,               /* '1'..'9' */
    CH_FLAG,                /* ' ', '+', '-', '#' */
    CH_SIZE,                /* 'h', 'l', 'L', 'N', 'F' */
    CH_TYPE                 /* type specifying character */
};

/* static data (read only, since we are re-entrant) */
char *nullstring = "(null)";    /* string to print on null ptr */

/* The state table.  This table is actually two tables combined into one. */
/* The lower nybble of each byte gives the character class of any         */
/* character; while the uper nybble of the byte gives the next state      */
/* to enter.  See the macros below the table for details.                 */
/*                                                                        */
/* The table is generated by maketab.c -- use the maketab program to make */
/* changes.                                                               */

/* Brief description of the table, since I can't find maketab.c - t-stevan     */
/* Each entry in form 0xYZ. Here Z is a character class used in the macro      */
/* find_char_class defined below. The character classes are defined in the     */
/* CHARTYPE enum. For example, 'I' maps to CH_TYPE. To find a particular entry */
/* Subtract the ASCI value for the space char from the character, and that is  */
/* the index to look up. The Y value is holds state transition information.    */
/* It is used in the macro find_next_state. */  
static char lookuptable[] = {
    0x06, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00,
    0x10, 0x00, 0x03, 0x06, 0x00, 0x06, 0x02, 0x10,
    0x04, 0x45, 0x45, 0x45, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x35, 0x30, 0x00, 0x50, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x28, 0x38, 0x50, 0x58, 0x07, 0x08,
    0x00, 0x38, 0x30, 0x30, 0x57, 0x50, 0x07, 0x00,
    0x00, 0x20, 0x20, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00,
    0x00, 0x70, 0x70, 0x78, 0x78, 0x78, 0x78, 0x08,
    0x07, 0x08, 0x00, 0x00, 0x07, 0x00, 0x08, 0x08,
    0x08, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x07,
    0x08
};

#define find_char_class(c)              \
        ((c) < ' ' || (c) > 'x' ?       \
            CH_OTHER                    \
        :                               \
            lookuptable[(c)-' '] & 0xF)

#define find_next_state(class, state)   \
        (lookuptable[(class) * NUMSTATES + (state)] >> 4)

#ifdef _WIN64
LOCAL(__int64) get_int64_arg(va_list *pargptr);
#endif 

#if !LONG_IS_INT
LOCAL(long) get_long_arg(va_list *pargptr);
#endif
LOCAL(int) get_int_arg(va_list *pargptr);
LOCAL(void) writestring(char *string,
                        int len,
                        struct w4io *f,
                        int *pcchwritten,
                        int fwide);

#ifndef NOFLOATS
/* extern float convert routines */
typedef int (* PFI)();
extern PFI _cfltcvt_tab[5];
#define _cfltcvt(a,b,c,d,e) (*_cfltcvt_tab[0])(a,b,c,d,e)
#define _cropzeros(a)       (*_cfltcvt_tab[1])(a)
#define _fassign(a,b,c)     (*_cfltcvt_tab[2])(a,b,c)
#define _forcdecpt(a)       (*_cfltcvt_tab[3])(a)
#define _positive(a)        (*_cfltcvt_tab[4])(a)
#define _cldcvt(a,b,c,d,e)  (*_cfltcvt_tab[5])(a,b,c,d,e)
#endif

/* Defines for printing out GUIDs */
#ifndef GUID_DEFINED
#define GUID_DEFINED
			/* size is 16 */
typedef struct  _GUID
    {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[ 8 ];
    }	GUID;

#endif // !GUID_DEFINED

#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID * const
#endif // !_REFGUID_DEFINED

/* This is actually one less than the normal GUIDSTR_MAX */
/* Because we don't tag on a NULL byte */
#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 /* + 1 */)

/* Make sure our buffer size is big enough to hold a GUID */
#if BUFFERSIZE < GUIDSTR_MAX
#undef BUFFERSIZE
#define BUFFERSIZE GUIDSTR_MAX
#endif

/* Function used to write a GUID to a string */
int StrFromGUID(REFGUID rguid, char * lpsz, int cbMax);

/***
*int w4iooutput(f, format, argptr)
*
*Purpose:
*   Output performs printf style output onto a stream.  It is called by
*   printf/fprintf/sprintf/vprintf/vfprintf/vsprintf to so the dirty
*   work.  In multi-thread situations, w4iooutput assumes that the given
*   stream is already locked.
*
*   Algorithm:
*       The format string is parsed by using a finite state automaton
*       based on the current state and the current character read from
*       the format string.  Thus, looping is on a per-character basis,
*       not a per conversion specifier basis.  Once the format specififying
*       character is read, output is performed.
*
*Entry:
*   struct w4io *f   - stream for output
*   char *format   - printf style format string
*   va_list argptr - pointer to list of subsidiary arguments
*
*Exit:
*   Returns the number of characters written, or -1 if an output error
*   occurs.
*
*Exceptions:
*
*Notes: 
*       FIXFIX - This code does not handle I64 for __int64 and derived types.
*       FIXFIX - This code does not handle I64 10byte floats.
*       FIXFIX - This code has to be tested for IA64 8byte floats.
*
*******************************************************************************/

int _cdecl w4iooutput(struct w4io *f, const char *format, va_list argptr)
{
    int hexadd;         /* offset to add to number to get 'a'..'f' */
    char ch;            /* character just read */
    wchar_t wc;         /* wide character temp */
    wchar_t *pwc;       /* wide character temp pointer */
    int flags;          /* flag word -- see #defines above for flag values */
    enum STATE state;   /* current state */
    enum CHARTYPE chclass; /* class of current character */
    int radix;          /* current conversion radix */
    int charsout;       /* characters currently written so far, -1 = IO error */
    int fldwidth;       /* selected field with -- 0 means default */
    int fwide;
    int precision;      /* selected precision -- -1 means default */
    char prefix[2];     /* numeric prefix -- up to two characters */
    int prefixlen;      /* length of prefix -- 0 means no prefix */
    int capexp;         /* non-zero = 'E' exponent signifiet, zero = 'e' */
    int no_output;      /* non-zero = prodcue no output for this specifier */
    char *text;         /* pointer text to be printed, not zero terminated */
    int textlen;        /* length of the text to be printed */
    char buffer[BUFFERSIZE];    /* buffer for conversions */

    charsout = 0;               /* no characters written yet */
    state = ST_NORMAL;          /* starting state */

    /* main loop -- loop while format character exist and no I/O errors */
    while ((ch = *format++) != '\0' && charsout >= 0) {
        chclass = find_char_class(ch);  /* find character class */
        state = find_next_state(chclass, state); /* find next state */

        /* execute code for each state */
        switch (state) {

        case ST_NORMAL:
            /* normal state -- just write character */
            f->writechar(ch, 1, f, &charsout);
            break;

        case ST_PERCENT:
            /* set default value of conversion parameters */
            prefixlen = fldwidth = no_output = capexp = 0;
            flags = 0;
            precision = -1;
            fwide = 0;
            break;

        case ST_FLAG:
            /* set flag based on which flag character */
            switch (ch) {
            case '-':
                flags |= FL_LEFT;       /* '-' => left justify */
                break;
            case '+':
                flags |= FL_SIGN;       /* '+' => force sign indicator */
                break;
            case ' ':
                flags |= FL_SIGNSP;     /* ' ' => force sign or space */
                break;
            case '#':
                flags |= FL_ALTERNATE;  /* '#' => alternate form */
                break;
            case '0':
                flags |= FL_LEADZERO;   /* '0' => pad with leading zeros */
                break;
            }
            break;

        case ST_WIDTH:
            /* update width value */
            if (ch == '*') {
                /* get width from arg list */
                fldwidth = get_int_arg(&argptr);
                if (fldwidth < 0) {
                    /* ANSI says neg fld width means '-' flag and pos width */
                    flags |= FL_LEFT;
                    fldwidth = -fldwidth;
                }
            }
            else {
                /* add digit to current field width */
                fldwidth = fldwidth * 10 + (ch - '0');
            }
            break;

        case ST_DOT:
            /* zero the precision, since dot with no number means 0
               not default, according to ANSI */
            precision = 0;
            break;

        case ST_PRECIS:
            /* update precison value */
            if (ch == '*') {
                /* get precision from arg list */
                precision = get_int_arg(&argptr);
                if (precision < 0)
                    precision = -1;     /* neg precision means default */
            }
            else {
                /* add digit to current precision */
                precision = precision * 10 + (ch - '0');
            }
            break;

        case ST_SIZE:
            /* just read a size specifier, set the flags based on it */
            switch (ch) {
#if !LONG_IS_INT
            case 'l':
                flags |= FL_LONG;   /* 'l' => long int */
                break;
#endif

#if !LONGDOUBLE_IS_DOUBLE
            case 'L':
                flags |= FL_LONGDOUBLE; /* 'L' => long double */
                break;
#endif

#if !SHORT_IS_INT
            case 'h':
                flags |= FL_SHORT;  /* 'h' => short int */
                break;
#endif
            case 'w':
                flags |= FL_WIDE;   /* 'w' => wide character */
                break;
            }
            break;

        case ST_TYPE:
            /* we have finally read the actual type character, so we       */
            /* now format and "print" the output.  We use a big switch     */
            /* statement that sets 'text' to point to the text that should */
            /* be printed, and 'textlen' to the length of this text.       */
            /* Common code later on takes care of justifying it and        */
            /* other miscellaneous chores.  Note that cases share code,    */
            /* in particular, all integer formatting is doen in one place. */
            /* Look at those funky goto statements!                        */

            switch (ch) {

            case 'c': {
                /* print a single character specified by int argument */
                wc = (wchar_t) get_int_arg(&argptr);    /* get char to print */
                * (wchar_t *) buffer = wc;
                text = buffer;
                textlen = 1;        /* print just a single character */
            }
            break;

            case 'S': {
                /* print a Counted String   */

                struct string {
                    short Length;
                    short MaximumLength;
                    char *Buffer;
                } *pstr;

                pstr = get_ptr_arg(&argptr);
                if (pstr == NULL || pstr->Buffer == NULL) {
                    /* null ptr passed, use special string */
                    text = nullstring;
                    textlen = strlen(text);
                    flags &= ~FL_WIDE;
                } else {
                    text = pstr->Buffer;
                    /* The length field is a count of bytes, not characters. */
                    if (flags & FL_WIDE)
                        textlen = pstr->Length / sizeof( wchar_t );
                    else
                        textlen = pstr->Length;
                    if (precision != -1)
                        textlen = min( textlen, precision );
                }

            }
            break;

            case 's': {
                /* print a string --                            */
                /* ANSI rules on how much of string to print:   */
                /*   all if precision is default,               */
                /*   min(precision, length) if precision given. */
                /* prints '(null)' if a null string is passed   */

                int i;
                char *p;       /* temps */

                text = get_ptr_arg(&argptr);
                if (text == NULL) {
                    /* null ptr passed, use special string */
                    text = nullstring;
                    flags &= ~FL_WIDE;
                }

                /* At this point it is tempting to use strlen(), but */
                /* if a precision is specified, we're not allowed to */
                /* scan past there, because there might be no null   */
                /* at all.  Thus, we must do our own scan.           */

                i = (precision == -1) ? INT_MAX : precision;

                /* scan for null upto i characters */
                if (flags & FL_WIDE) {
                    pwc = (wchar_t *) text;
                    while (i-- && (wc = *pwc) && (wc & 0x00ff)) {
                        ++pwc;
                        if (wc & 0xff00) {      // if high byte set,
                            break;              // error will be indicated
                        }
                    }
                    textlen = (int) (pwc - (wchar_t*)text);  /* length of string */
                } else {
                    p = text;
                    while (i-- && *p) {
                        ++p;
                    }
                    textlen = (int) (p - text);    /* length of the string */
                }
            }
            break;

            /* print a GUID */
            case 'I':
            {
                void *p;        /* temp */

                p = get_ptr_arg(&argptr);

			    if (p == NULL) 
			    {
			   		/* null ptr passed, use special string */
			   		text = nullstring;
					textlen = strlen(nullstring);
				}
				else
               	{
               		textlen = StrFromGUID(p, buffer, BUFFERSIZE); 
               		text = buffer;
				}
            }
            break;

            case 'n': {
                /* write count of characters seen so far into */
                /* short/int/long thru ptr read from args */

                void *p;            /* temp */

                p = get_ptr_arg(&argptr);

                /* store chars out into short/long/int depending on flags */
#if !LONG_IS_INT
                if (flags & FL_LONG)
                    *(long *)p = charsout;
                else
#endif

#if !SHORT_IS_INT
                if (flags & FL_SHORT)
                    *(short *)p = (short) charsout;
                else
#endif
                    *(int *)p = charsout;

                no_output = 1;              /* force no output */
            }
            break;

#ifndef NOFLOATS
            case 'E':
            case 'G':
                capexp = 1;                 /* capitalize exponent */
                ch += 'a' - 'A';            /* convert format char to lower */
                /* DROP THROUGH */
            case 'e':
            case 'f':
            case 'g':   {
                /* floating point conversion -- we call cfltcvt routines */
                /* to do the work for us.                                */
                flags |= FL_SIGNED;         /* floating point is signed conversion */
                text = buffer;              /* put result in buffer */
                flags &= ~FL_WIDE;          /* 8 bit string */

                /* compute the precision value */
                if (precision < 0)
                    precision = 6;      /* default precision: 6 */
                else if (precision == 0 && ch == 'g')
                    precision = 1;      /* ANSI specified */

#if !LONGDOUBLE_IS_DOUBLE
                /* do the conversion */
                if (flags & FL_LONGDOUBLE) {
                    _cldcvt(argptr, text, ch, precision, capexp);
                    va_arg(argptr, LONGDOUBLE);
                }
                else
#endif
                {
                    _cfltcvt(argptr, text, ch, precision, capexp);
                    va_arg(argptr, DOUBLE);
                }

                /* '#' and precision == 0 means force a decimal point */
                if ((flags & FL_ALTERNATE) && precision == 0)
                    _forcdecpt(text);

                /* 'g' format means crop zero unless '#' given */
                if (ch == 'g' && !(flags & FL_ALTERNATE))
                    _cropzeros(text);

                /* check if result was negative, save '-' for later */
                /* and point to positive part (this is for '0' padding) */
                if (*text == '-') {
                    flags |= FL_NEGATIVE;
                    ++text;
                }

                textlen = strlen(text);     /* compute length of text */
            }
            break;
#endif // NOFLOATS

            case 'd':
            case 'i':
                /* signed decimal output */
                flags |= FL_SIGNED;
                radix = 10;
                goto COMMON_INT;

            case 'u':
                radix = 10;
                goto COMMON_INT;

            case 'p':
                /* write a pointer */
                /* this is like an integer or long for Win32, __int64 for Win64 */
                /* except we force precision to pad with zeros and */
                /* output in big hex. */

                precision = 2 * sizeof(void *);     /* number of hex digits needed */
#if !PTR_IS_INT
                flags |= FL_PTR;       /* assume we're converting a long in the Win32 case */
#endif
                /* DROP THROUGH to hex formatting */

            case 'C':
            case 'X':
                /* unsigned upper hex output */
                hexadd = 'A' - '9' - 1;     /* set hexadd for uppercase hex */
                goto COMMON_HEX;

            case 'x':
                /* unsigned lower hex output */
                hexadd = 'a' - '9' - 1;     /* set hexadd for lowercase hex */
                /* DROP THROUGH TO COMMON_HEX */

            COMMON_HEX:
                radix = 16;
                if (flags & FL_ALTERNATE) {
                    /* alternate form means '0x' prefix */
                    prefix[0] = '0';
                    prefix[1] = (char)('x' - 'a' + '9' + 1 + hexadd);   /* 'x' or 'X' */
                    prefixlen = 2;
                }
                goto COMMON_INT;

            case 'o':
                /* unsigned octal output */
                radix = 8;
                if (flags & FL_ALTERNATE) {
                    /* alternate form means force a leading 0 */
                    flags |= FL_FORCEOCTAL;
                }
                /* DROP THROUGH to COMMON_INT */

            COMMON_INT: {
                /* This is the general integer formatting routine. */
                /* Basically, we get an argument, make it positive */
                /* if necessary, and convert it according to the */
                /* correct radix, setting text and textlen */
                /* appropriately. */

                ULONG_PTR number;   /* number to convert */
                int digit;          /* ascii value of digit */
                LONG_PTR l;         /* temp long value */

                /* 1. read argument into l, sign extend as needed */

#if !LONG_IS_INT
                if (flags & FL_LONG)
                    l = get_long_arg(&argptr);
                else
#endif

#if !SHORT_IS_INT
                if (flags & FL_SHORT) {
                    if (flags & FL_SIGNED)
                        l = (short) get_int_arg(&argptr); /* sign extend */
                    else
                        l = (unsigned short) get_int_arg(&argptr);    /* zero-extend*/
                }
                else
#endif

#ifdef _WIN64
// Sundown: if get_int64_arg() could be defined all the time, 
//          this 'ifdef _WIN64' could be removed.

                if (flags & FL_PTR64) {
                   l = get_int64_arg(&argptr); 
                }
                else 
#endif // _WIN64

                {
                    if (flags & FL_SIGNED)
                        l = get_int_arg(&argptr); /* sign extend */
                    else
                        l = (unsigned int) get_int_arg(&argptr);    /* zero-extend*/
                }

                /* 2. check for negative; copy into number */
                if ( (flags & FL_SIGNED) && l < 0) {
                    number = -l;
                    flags |= FL_NEGATIVE;   /* remember negative sign */
                }
                else {
                    number = l;
                }

                /* 3. check precision value for default; non-default */
                /*    turns off 0 flag, according to ANSI. */
                if (precision < 0)
                    precision = 1;              /* default precision */
                else
                    flags &= ~FL_LEADZERO;

                /* 4. Check if data is 0; if so, turn off hex prefix */
                if (number == 0)
                    prefixlen = 0;

                /* 5. Convert data to ASCII -- note if precision is zero */
                /*    and number is zero, we get no digits at all.       */

                text = &buffer[BUFFERSIZE-1];   // last digit at end of buffer
                flags &= ~FL_WIDE;              // 8 bit characters

                while (precision-- > 0 || number != 0) {
                    digit = (int)(number % radix) + '0';
                    number /= radix;            /* reduce number */
                    if (digit > '9') {
                        /* a hex digit, make it a letter */
                        digit += hexadd;
                    }
                    *text-- = (char)digit;      /* store the digit */
                }

                textlen = (int) (&buffer[BUFFERSIZE-1] - text); /* compute length of number */
                ++text;         /* text points to first digit now */


                /* 6. Force a leading zero if FORCEOCTAL flag set */
                if ((flags & FL_FORCEOCTAL) && (text[0] != '0' || textlen == 0)) {
                    *--text = '0';
                    ++textlen;          /* add a zero */
                }
            }
            break;
            }

            /* At this point, we have done the specific conversion, and */
            /* 'text' points to text to print; 'textlen' is length.  Now we */
            /* justify it, put on prefixes, leading zeros, and then */
            /* print it. */

            if (!no_output) {
                int padding;    /* amount of padding, negative means zero */

                if (flags & FL_SIGNED) {
                    if (flags & FL_NEGATIVE) {
                        /* prefix is a '-' */
                        prefix[0] = '-';
                        prefixlen = 1;
                    }
                    else if (flags & FL_SIGN) {
                        /* prefix is '+' */
                        prefix[0] = '+';
                        prefixlen = 1;
                    }
                    else if (flags & FL_SIGNSP) {
                        /* prefix is ' ' */
                        prefix[0] = ' ';
                        prefixlen = 1;
                    }
                }

                /* calculate amount of padding -- might be negative, */
                /* but this will just mean zero */
                padding = fldwidth - textlen - prefixlen;

                /* put out the padding, prefix, and text, in the correct order */

                if (!(flags & (FL_LEFT | FL_LEADZERO))) {
                    /* pad on left with blanks */
                    f->writechar(' ', padding, f, &charsout);
                }

                /* write prefix */
                writestring(prefix, prefixlen, f, &charsout, 0);

                if ((flags & FL_LEADZERO) && !(flags & FL_LEFT)) {
                    /* write leading zeros */
                    f->writechar('0', padding, f, &charsout);
                }

                /* write text */
                writestring(text, textlen, f, &charsout, flags & FL_WIDE);

                if (flags & FL_LEFT) {
                    /* pad on right with blanks */
                    f->writechar(' ', padding, f, &charsout);
                }

                /* we're done! */
            }
            break;
        }
    }

    return charsout;        /* return value = number of characters written */
}


/***
*int get_int_arg(va_list pargptr)
*
*Purpose:
*   Gets an int argument off the given argument list and updates *pargptr.
*
*Entry:
*   va_list pargptr - pointer to argument list; updated by function
*
*Exit:
*   Returns the integer argument read from the argument list.
*
*Exceptions:
*
*******************************************************************************/

LOCAL(int) get_int_arg(va_list *pargptr)
{
    return va_arg(*pargptr, int);
}

/***
*long get_long_arg(va_list pargptr)
*
*Purpose:
*   Gets an long argument off the given argument list and updates pargptr.
*
*Entry:
*   va_list pargptr - pointer to argument list; updated by function
*
*Exit:
*   Returns the long argument read from the argument list.
*
*Exceptions:
*
*******************************************************************************/


#if !LONG_IS_INT
LOCAL(long) get_long_arg(va_list *pargptr)
{
    return va_arg(*pargptr, long);
}
#endif

#ifdef _WIN64
LOCAL(__int64) get_int64_arg (
    va_list *pargptr
    )
{
    return va_arg(*pargptr, __int64);
}
#endif

/***
*void writestring(char *string, int len, struct w4io *f, int *pcchwritten, int fwide)
*
*Purpose:
*   Writes a string of the given length to the given file.  If no error occurs,
*   then *pcchwritten is incremented by len; otherwise, *pcchwritten is set
*   to -1.  If len is negative, it is treated as zero.
*
*Entry:
*   char *string     - string to write (NOT null-terminated)
*   int len          - length of string
*   struct w4io *f   - file to write to
*   int *pcchwritten - pointer to integer to update with total chars written
*   int fwide        - wide character flag
*
*Exit:
*   No return value.
*
*Exceptions:
*
*******************************************************************************/

LOCAL(void) writestring(
        char *string,
        int len,
        struct w4io *f,
        int *pcchwritten,
        int fwide)
{
    wchar_t *pwc;

    //printf("string: str=%.*s, len=%d, cch=%d, f=%d\n", len, string, len, *pcchwritten, fwide);
    if (fwide) {
        pwc = (wchar_t *) string;
        while (len-- > 0) {
            if (*pwc & 0xff00) {
                f->writechar('^', 1, f, pcchwritten);
            }
            f->writechar((char) *pwc++, 1, f, pcchwritten);
        }
    } else {
        while (len-- > 0) {
            f->writechar(*string++, 1, f, pcchwritten);
        }
    }
}


const wchar_t a_wcDigits[] = L"0123456789ABCDEF";

//+---------------------------------------------------------------------------
//
//  Function:   FormatHexNum
//
//  Synopsis:   Given a value, and a count of characters, translate
//		the value into a hex string. This is the ANSI version
//
//  Arguments:  [ulValue] -- Value to convert
//		[chChars] -- Number of characters to format
//		[pchStr] -- Pointer to output buffer
//
//  Requires:   pwcStr must be valid for chChars
//
//  History:    5-31-95   t-stevan  Copied and Modified for use in debug output function
//
//  Notes:
//
//----------------------------------------------------------------------------
void FormatHexNum( unsigned long ulValue, unsigned long chChars, char *pchStr)
{
	while(chChars--)
	{
		pchStr[chChars] = (char) a_wcDigits[ulValue & 0xF];
		ulValue = ulValue >> 4;
	}
}

//+-------------------------------------------------------------------------
//
//  Function:   StrFromGUID     (private)
//
//  Synopsis:   Converts a GUID into a string (duh!)
//
//  Arguments:  [rguid] - the guid to convert
//              [lpszy] - buffer to hold the results
//              [cbMax] - sizeof the buffer
//
//  Returns:    amount of data copied to lpsz if successful
//              0 if buffer too small.
//
//--------------------------------------------------------------------------

int StrFromGUID(REFGUID rguid, char * lpsz, int cbMax)  // internal
{
    if (cbMax < GUIDSTR_MAX)
	return 0;


//   Make the GUID into"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",

    *lpsz++ = '{';
    FormatHexNum( rguid->Data1, 8 , lpsz);
    lpsz += 8;
    *lpsz++ = '-';

    FormatHexNum( rguid->Data2, 4 , lpsz);
    lpsz += 4;
    *lpsz++ = '-';

    FormatHexNum( rguid->Data3, 4 , lpsz);
    lpsz += 4;
    *lpsz++ = '-';

    FormatHexNum( rguid->Data4[0], 2 , lpsz);
    lpsz += 2;
    FormatHexNum( rguid->Data4[1], 2 , lpsz);
    lpsz += 2;
    *lpsz++ = '-';

    FormatHexNum( rguid->Data4[2], 2 , lpsz);
    lpsz += 2;
    FormatHexNum( rguid->Data4[3], 2 , lpsz);
    lpsz += 2;
    FormatHexNum( rguid->Data4[4], 2 , lpsz);
    lpsz += 2;
    FormatHexNum( rguid->Data4[5], 2 , lpsz);
    lpsz += 2;
    FormatHexNum( rguid->Data4[6], 2 , lpsz);
    lpsz += 2;
    FormatHexNum( rguid->Data4[7], 2 , lpsz);
    lpsz += 2;

    *lpsz++ = '}';
    /* We don't want to tag on a NULL char because we don't need to print one out *\
    /* *lpsz = 0; */


    return GUIDSTR_MAX;
}


