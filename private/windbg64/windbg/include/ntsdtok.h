#define ulong   unsigned long
#define ushort  unsigned short
#define uchar   unsigned char
#define uint    unsigned int

#define ARRAYSIZE       20
#define STRLISTSIZE     128
#define SYMBOLSIZE      256

#ifdef  KERNEL
#define CONTEXTFIR      0       //  only unchanged FIR in context
#define CONTEXTVALID    1       //  full, but unchanged context
#define CONTEXTDIRTY    2       //  full, but changed context
#endif

//  error codes

#define ERR_OVERFLOW        0x1000
#define ERR_SYNTAX          0x1001
#define ERR_BADRANGE        0x1002
#define ERR_VARDEF          0x1003
#define ERR_EXTRACHARS      0x1004
#define ERR_LISTSIZE        0x1005
#define ERR_STRINGSIZE      0x1006
#define ERR_MEMORY          0x1007
#define ERR_BADREG          0x1008
#define ERR_BADOPCODE       0x1009
#define ERR_SUFFIX          0x100a
#define ERR_OPERAND         0x100b
#define ERR_ALIGNMENT       0x100c
#define ERR_PREFIX          0x100d
#define ERR_DISPLACEMENT    0x100e
#define ERR_BPLISTFULL      0x100f
#define ERR_BPDUPLICATE     0x1010
#define ERR_BADTHREAD       0x1011
#define ERR_DIVIDE          0x1012
#define ERR_TOOFEW          0x1013
#define ERR_TOOMANY         0x1014
#define ERR_SIZE            0x1015
#define ERR_BADSEG          0x1016
#define ERR_RELOC           0x1017
#define ERR_BADPROCESS      0x1018
#define ERR_AMBIGUOUS       0x1019
#define ERR_FILEREAD        0x101a
#define ERR_LINENUMBER      0x101b
#define ERR_BADSEL          0x101c
#define ERR_SYMTOOSMALL     0x101d
#define ERR_BPIONOTSUP      0x101e

#define ERR_UNIMPLEMENTED   0x1099

//  token classes (< 100) and types (>= 100)

#define EOL_CLASS       0
#define ADDOP_CLASS     1
#define ADDOP_PLUS      100
#define ADDOP_MINUS     101
#define MULOP_CLASS     2
#define MULOP_MULT      200
#define MULOP_DIVIDE    201
#define MULOP_MOD       202
#define MULOP_SEG       203
#define MULOP_64        204
#define LOGOP_CLASS     3
#define LOGOP_AND       300
#define LOGOP_OR        301
#define LOGOP_XOR       302
#define LRELOP_CLASS    4
#define LRELOP_EQ       400
#define LRELOP_NE       401
#define LRELOP_LT       402
#define LRELOP_GT       403
#define UNOP_CLASS      5
#define UNOP_NOT        500
#define UNOP_BY         501
#define UNOP_WO         502
#define UNOP_DW         503
#define UNOP_POI        504
#define UNOP_LOW        505
#define UNOP_HI         506
#define LPAREN_CLASS    6
#define RPAREN_CLASS    7
#define LBRACK_CLASS    8
#define RBRACK_CLASS    9
#define REG_CLASS       10
#define REG_PC_CLASS    11
#define NUMBER_CLASS    12
#define SYMBOL_CLASS    13

#define ERROR_CLASS     99              //only used for PeekToken

