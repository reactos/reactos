/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* CHARMAP.C - Character mapping arrays                                 */
/*                                                                      */
/* 06-Dec-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rcpptype.h"
#include "charmap.h"

#define CHARMAP_SIZE    256

WCHAR Charmap[CHARMAP_SIZE] = {
    LX_EOS,                 /* 0x0, <end of string marker> */
    LX_ILL,                 /* 0x1 */
    LX_ILL,                 /* 0x2 */
    LX_ILL,                 /* 0x3 */
    LX_ILL,                 /* 0x4 */
    LX_ILL,                 /* 0x5 */
    LX_ILL,                 /* 0x6 */
    LX_ILL,                 /* 0x7 */
    LX_ILL,                 /* 0x8 */
    LX_WHITE,               /* <horizontal tab> */
    LX_NL,                  /* <newline> */
    LX_WHITE,               /* <vertical tab> */
    LX_WHITE,               /* <form feed> */
    LX_CR,                  /* <really a carriage return> */
    LX_ILL,                 /* 0xe */
    LX_ILL,                 /* 0xf */
    LX_ILL,                 /* 0x10 */
    LX_ILL,                 /* 0x11 */
    LX_ILL,                 /* 0x12 */
    LX_ILL,                 /* 0x13 */
    LX_ILL,                 /* 0x14 */
    LX_ILL,                 /* 0x15 */
    LX_ILL,                 /* 0x16 */
    LX_ILL,                 /* 0x17 */
    LX_ILL,                 /* 0x18 */
    LX_ILL,                 /* 0x19 */
    LX_EOS,                 /* 0x1a, ^Z */
    LX_ILL,                 /* 0x1b */
    LX_ILL,                 /* 0x1c */
    LX_ILL,                 /* 0x1d */
    LX_ILL,                 /* 0x1e */
    LX_ILL,                 /* 0x1f */
    LX_WHITE,               /* 0x20 */
    LX_BANG,                /* ! */
    LX_DQUOTE,              /* " */
    LX_POUND,               /* # */
    LX_ASCII,               /* $ */
    LX_PERCENT,             /* % */
    LX_AND,                 /* & */
    LX_SQUOTE,              /* ' */
    LX_OPAREN,              /* ( */
    LX_CPAREN,              /* ) */
    LX_STAR,                /* * */
    LX_PLUS,                /* + */
    LX_COMMA,               /* , */
    LX_MINUS,               /* - */
    LX_DOT,                 /* . */
    LX_SLASH,               /* / */
    LX_NUMBER,              /* 0 */
    LX_NUMBER,              /* 1 */
    LX_NUMBER,              /* 2 */
    LX_NUMBER,              /* 3 */
    LX_NUMBER,              /* 4 */
    LX_NUMBER,              /* 5 */
    LX_NUMBER,              /* 6 */
    LX_NUMBER,              /* 7 */
    LX_NUMBER,              /* 8 */
    LX_NUMBER,              /* 9 */
    LX_COLON,               /* : */
    LX_SEMI,                /* ; */
    LX_LT,                  /* < */
    LX_EQ,                  /* = */
    LX_GT,                  /* > */
    LX_QUEST,               /* ? */
    LX_EACH,                /* @ */
    LX_ID,                  /* A */
    LX_ID,                  /* B */
    LX_ID,                  /* C */
    LX_ID,                  /* D */
    LX_ID,                  /* E */
    LX_ID,                  /* F */
    LX_ID,                  /* G */
    LX_ID,                  /* H */
    LX_ID,                  /* I */
    LX_ID,                  /* J */
    LX_ID,                  /* K */
    LX_ID,                  /* L */
    LX_ID,                  /* M */
    LX_ID,                  /* N */
    LX_ID,                  /* O */
    LX_ID,                  /* P */
    LX_ID,                  /* Q */
    LX_ID,                  /* R */
    LX_ID,                  /* S */
    LX_ID,                  /* T */
    LX_ID,                  /* U */
    LX_ID,                  /* V */
    LX_ID,                  /* W */
    LX_ID,                  /* X */
    LX_ID,                  /* Y */
    LX_ID,                  /* Z */
    LX_OBRACK,              /* [ */
    LX_EOS,                 /* \ */
    LX_CBRACK,              /* ] */
    LX_HAT,                 /* ^ */
    LX_ID,                  /* _ */
    LX_ASCII,               /* ` */
    LX_ID,                  /* a */
    LX_ID,                  /* b */
    LX_ID,                  /* c */
    LX_ID,                  /* d */
    LX_ID,                  /* e */
    LX_ID,                  /* f */
    LX_ID,                  /* g */
    LX_ID,                  /* h */
    LX_ID,                  /* i */
    LX_ID,                  /* j */
    LX_ID,                  /* k */
    LX_ID,                  /* l */
    LX_ID,                  /* m */
    LX_ID,                  /* n */
    LX_ID,                  /* o */
    LX_ID,                  /* p */
    LX_ID,                  /* q */
    LX_ID,                  /* r */
    LX_ID,                  /* s */
    LX_ID,                  /* t */
    LX_ID,                  /* u */
    LX_ID,                  /* v */
    LX_ID,                  /* w */
    LX_ID,                  /* x */
    LX_ID,                  /* y */
    LX_ID,                  /* z */
    LX_OBRACE,              /* { */
    LX_OR,                  /* | */
    LX_CBRACE,              /* } */
    LX_TILDE,               /* ~ */
    LX_ILL,                 /* 0x7f */
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
    LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL, LX_ILL,
};

WCHAR Contmap[CHARMAP_SIZE] = {
    LXC_SPECIAL,                                                /* 0x0, <end of string marker> */
    0,                                                          /* 0x1 */
    0,                                                          /* 0x2 */
    0,                                                          /* 0x3 */
    0,                                                          /* 0x4 */
    0,                                                          /* 0x5 */
    0,                                                          /* <end of buffer marker> */
    0,                                                          /* 0x7 */
    0,                                                          /* 0x8 */
    LXC_WHITE,                                                  /* <horizontal tab> */
    LXC_SPECIAL,                                                /* <newline>, this is NOT considered whitespace */
    LXC_WHITE,                                                  /* <vertical tab> */
    LXC_WHITE,                                                  /* <form feed> */
    0,                                                          /* <really a carriage return> */
    0,                                                          /* 0xe */
    0,                                                          /* 0xf */
    0,                                                          /* 0x10 */
    0,                                                          /* 0x11 */
    0,                                                          /* 0x12 */
    0,                                                          /* 0x13 */
    0,                                                          /* 0x14 */
    0,                                                          /* 0x15 */
    0,                                                          /* 0x16 */
    0,                                                          /* 0x17 */
    0,                                                          /* 0x18 */
    0,                                                          /* 0x19 */
    LXC_SPECIAL,                                                /* 0x1a */
    0,                                                          /* 0x1b */
    0,                                                          /* 0x1c */
    0,                                                          /* 0x1d */
    0,                                                          /* 0x1e */
    0,                                                          /* 0x1f */
    LXC_WHITE,                                                  /* 0x20 */
    0,                                                          /* ! */
    0,                                                          /* " */
    0,                                                          /* # */
    0,                                                          /* $ */
    0,                                                          /* % */
    0,                                                          /* & */
    0,                                                          /* ' */
    0,                                                          /* ( */
    0,                                                          /* ) */
    LXC_SPECIAL,                                                /* * */
    0,                                                          /* + */
    0,                                                          /* , */
    0,                                                          /* - */
    0,                                                          /* . */
    0,                                                          /* / */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT | LXC_ODIGIT | LXC_BDIGIT,  /* 0 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT | LXC_ODIGIT | LXC_BDIGIT,  /* 1 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT | LXC_ODIGIT,               /* 2 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT | LXC_ODIGIT,               /* 3 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT | LXC_ODIGIT,               /* 4 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT | LXC_ODIGIT,               /* 5 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT | LXC_ODIGIT,               /* 6 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT | LXC_ODIGIT,               /* 7 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT,                            /* 8 */
    LXC_ID | LXC_XDIGIT | LXC_DIGIT,                            /* 9 */
    0,                                                          /* : */
    0,                                                          /* ; */
    0,                                                          /* < */
    0,                                                          /* = */
    0,                                                          /* > */
    0,                                                          /* ? */
    0,                                                          /* @ */
    LXC_ID | LXC_XDIGIT,                                        /* A */
    LXC_ID | LXC_XDIGIT     | LXC_RADIX,                        /* B */
    LXC_ID | LXC_XDIGIT,                                        /* C */
    LXC_ID | LXC_XDIGIT | LXC_RADIX,                            /* D */
    LXC_ID | LXC_XDIGIT,                                        /* E */
    LXC_ID | LXC_XDIGIT,                                        /* F */
    LXC_ID,                                                     /* G */
    LXC_ID | LXC_RADIX,                                         /* H */
    LXC_ID,                                                     /* I */
    LXC_ID,                                                     /* J */
    LXC_ID,                                                     /* K */
    LXC_ID,                                                     /* L */
    LXC_ID,                                                     /* M */
    LXC_ID,                                                     /* N */
    LXC_ID | LXC_RADIX,                                         /* O */
    LXC_ID,                                                     /* P */
    LXC_ID | LXC_RADIX,                                         /* Q */
    LXC_ID,                                                     /* R */
    LXC_ID,                                                     /* S */
    LXC_ID,                                                     /* T */
    LXC_ID,                                                     /* U */
    LXC_ID,                                                     /* V */
    LXC_ID,                                                     /* W */
    LXC_ID,                                                     /* X */
    LXC_ID,                                                     /* Y */
    LXC_ID,                                                     /* Z */
    0,                                                          /* [ */
    0,                                                          /* \ */
    0,                                                          /* ] */
    0,                                                          /* ^ */
    LXC_ID,                                                     /* _ */
    0,                                                          /* ` */
    LXC_ID | LXC_XDIGIT,                                        /* a */
    LXC_ID | LXC_XDIGIT | LXC_RADIX,                            /* b */
    LXC_ID | LXC_XDIGIT,                                        /* c */
    LXC_ID | LXC_XDIGIT | LXC_RADIX,                            /* d */
    LXC_ID | LXC_XDIGIT,                                        /* e */
    LXC_ID | LXC_XDIGIT,                                        /* f */
    LXC_ID,                                                     /* g */
    LXC_ID | LXC_RADIX,                                         /* h */
    LXC_ID,                                                     /* i */
    LXC_ID,                                                     /* j */
    LXC_ID,                                                     /* k */
    LXC_ID,                                                     /* l */
    LXC_ID,                                                     /* m */
    LXC_ID,                                                     /* n */
    LXC_ID | LXC_RADIX,                                         /* o */
    LXC_ID,                                                     /* p */
    LXC_ID | LXC_RADIX,                                         /* q */
    LXC_ID,                                                     /* r */
    LXC_ID,                                                     /* s */
    LXC_ID,                                                     /* t */
    LXC_ID,                                                     /* u */
    LXC_ID,                                                     /* v */
    LXC_ID,                                                     /* w */
    LXC_ID,                                                     /* x */
    LXC_ID,                                                     /* y */
    LXC_ID,                                                     /* z */
    0,                                                          /* { */
    0,                                                          /* | */
    0,                                                          /* } */
    0,                                                          /* ~ */
    0,                                                          /* 0x7f */
};


WCHAR
GetCharMap (
    WCHAR c
    )
{
    if (c == 0xFEFF)           // Byte Order Mark
        return (LX_BOM);
    else if (c > CHARMAP_SIZE)
        return (LX_ID);        // character beyond the ANSI set

    return (Charmap[c]);
}


void
SetCharMap (
    WCHAR c,
    WCHAR val
    )
{
    if (c > CHARMAP_SIZE)
       return;

    Charmap[((UCHAR)(c))] = val;
}


WCHAR
GetContMap (
    WCHAR c
    )
{
    if (c > CHARMAP_SIZE)
        return (LXC_ID);       // character beyong the ANSI set

    return (Contmap[c]);
}


void
SetContMap (
    WCHAR c,
    WCHAR val
    )
{
    if (c > CHARMAP_SIZE)
       return;

    Contmap[c] = val;
}
