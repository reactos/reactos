/*
**  charmap.h : P0 specific, also included by charmap.c
**  it defines the mapping used to go from simple chars to these predefined
**  values. this enables the compiler to use a compact switch stmt.
**  they have been grouped in what is believed to be the most beneficial
**  way, in that most switches will be checking those values which have
**  been grouped together.
*/
#define EOS_CHAR                L'\0'    /* end of string/buffer marker char */

#define LX_WHITE                0
#define LX_CR                   1
#define LX_SLASH                2              /* /, /=, comment start  */
#define LX_EOS                  3
#define LX_STAR                 4              /* *, *=, comment stop  */
#define LX_NL                   5
#define LX_BACKSLASH            6
#define LX_SQUOTE               7
#define LX_DQUOTE               8

#define LX_DOT                  9              /* . ... */
#define LX_BANG                 10             /* ! !=  */
#define LX_POUND                11             /* # ##  */
#define LX_PERCENT              12             /* % %=  */
#define LX_EQ                   13             /* = ==  */
#define LX_HAT                  14             /* ^ ^=  */
#define LX_OR                   15             /* | |= || */
#define LX_AND                  16             /* & && &= */
#define LX_PLUS                 17             /* + ++ += */
#define LX_MINUS                18             /* - -- -= ->  */
#define LX_LT                   19             /* < << <<= <= */
#define LX_GT                   20             /* > >= >> >>= */
#define LX_LSHIFT               21             /* << */
#define LX_RSHIFT               22             /* >> */

#define LX_ILL                  23
#define LX_CBRACE               24
#define LX_CBRACK               25
#define LX_COLON                26
#define LX_COMMA                27
#define LX_CPAREN               28
#define LX_NUMBER               29
#define LX_OBRACE               30
#define LX_OBRACK               31
#define LX_OPAREN               32
#define LX_QUEST                33
#define LX_SEMI                 34
#define LX_TILDE                35
#define LX_MACFORMAL            36
#define LX_STRFORMAL            37
#define LX_CHARFORMAL           38
#define LX_NOEXPAND             39
#define LX_ID                   40
#define LX_EACH                 41

#define LX_LEADBYTE             42
#define LX_ASCII                43             /* to use for 'non-illegal' illegals */
#define LX_BOM                  44             /* Byte Order Mark */

#define LX_FORMALMARK           0x01
#define LX_FORMALSTR            0x02
#define LX_FORMALCHAR           0x03
#define LX_NOEXPANDMARK         0x04
#define CONTROL_Z               0x1a
/*
**  Charmap is indexed with a character value plus the above offset
*/
#define CHARMAP(c)              GetCharMap(c)
#define SETCHARMAP(c,val)       SetCharMap(c, val)

#define LX_IS_IDENT(c)  (CHARMAP(c) == LX_ID)
#define LX_IS_WHITE(c)  (CHARMAP(c) == LX_WHITE)
#define LX_IS_NUMBER(c) (CHARMAP(c) == LX_NUMBER)

#define LXC_BDIGIT      0x01            /* 0 - 1 */
#define LXC_ODIGIT      0x02            /* 0 - 7 */
#define LXC_DIGIT       0x04            /* 0 - 9 */
#define LXC_XDIGIT      0x08            /* a-f A-F 0-9 */
#define LXC_ID          0x10            /* continuation is part of an identifier */
#define LXC_RADIX       0x20            /* BbDdHhOoQq */
#define LXC_WHITE       0x40            /* whitespace */
#define LXC_SPECIAL     0x80            /* the char may have a special meaning */

#define CONTMAP(c)           GetContMap(c)
#define SETCONTMAP(c, val)   SetContMap(c, val)
/*
**      LXC_IS_ID(c) : is c part of an identifier
*/
#define LXC_IS_BDIGIT(c)        (CONTMAP(c) & LXC_BDIGIT)
#define LXC_IS_ODIGIT(c)        (CONTMAP(c) & LXC_ODIGIT)
#define LXC_IS_DIGIT(c)         (CONTMAP(c) & LXC_DIGIT)
#define LXC_IS_XDIGIT(c)        (CONTMAP(c) & LXC_XDIGIT)
#define LXC_IS_IDENT(c)         (CONTMAP(c) & LXC_ID)
#define LXC_IS_RADIX(c)         (CONTMAP(c) & LXC_RADIX)
#define LXC_IS_WHITE(c)         (CONTMAP(c) & LXC_WHITE)
#define IS_SPECIAL(c)           (CONTMAP(c) & LXC_SPECIAL)


// Function prototypes

WCHAR GetCharMap (WCHAR);
void  SetCharMap (WCHAR, WCHAR);
WCHAR GetContMap (WCHAR);
void  SetContMap (WCHAR, WCHAR);

