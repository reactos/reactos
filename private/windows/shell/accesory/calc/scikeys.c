#include "scicalc.h"
#include "calchelp.h"
#include "unifunc.h"

#define RED         RGB(255,0,0)       /* Red                           */
#define PURPLE      RGB(255,0,255)     /* Dark Purple                   */
#define BLUE        RGB(0,0,255)       /* Blue                          */
#define DKBLUE      RGB(0,0,255)       /* Dark Blue                     */
#define MAGENTA     RGB(255,0,255)     /* Magenta                       */
#define DKRED       RGB(255,0,0)       /* Dark Red.                     */
#define WHITE       RGB(255,255,255)   /* White                         */
#define BLACK       RGB(0,0,0)         /* Black                         */

typedef struct
{
    COLORREF    crColor;    // text color
    DWORD       iHelpID;    // the helpfile ID for this key
//    int         bUnary  :1, // true if this key is treated as a unary operator
//                bBinary :1, // true if this key is a binary operator
//                bUseInv :1, // true if this key deactivates the Inv checkbox when used
//                bUseHyp :1; // true if this key deactivates the Hyp checkbox when used
} KEYDATA;

//    Control ID,       Color,  Help ID,            Unary,  Binary, Inv,    Hyp
KEYDATA keys[] = {
    { /*IDC_SIGN,   */  BLUE,   CALC_STD_SIGN,      /*false,  false,  false,  false*/ },
    { /*IDC_CLEAR,  */  DKRED,  CALC_C,             /*false,  false,  false,  false*/ },
    { /*IDC_CENTR,  */  DKRED,  CALC_CE,            /*false,  false,  false,  false*/ },
    { /*IDC_BACK,   */  DKRED,  CALC_BACK,          /*false,  false,  false,  false*/ },
    { /*IDC_STAT,   */  DKBLUE, CALC_SCI_STA,       /*false,  false,  false,  false*/ },
    { /*IDC_PNT,    */  BLUE,   CALC_STD_DECIMAL,   /*false,  false,  false,  false*/ },
    { /*IDC_AND,    */  RED,    CALC_SCI_AND,       /*false,  false,  false,  false*/ },
    { /*IDC_OR,     */  RED,    CALC_SCI_OR,        /*false,  false,  false,  false*/ },
    { /*IDC_XOR,    */  RED,    CALC_SCI_XOR,       /*false,  false,  false,  false*/ },
    { /*IDC_LSHF,   */  RED,    CALC_SCI_LSH,       /*false,  false,  false,  false*/ },
    { /*IDC_DIV,    */  RED,    CALC_STD_SLASH,     /*false,  false,  false,  false*/ },
    { /*IDC_MUL,    */  RED,    CALC_STD_ASTERISK,  /*false,  false,  false,  false*/ },
    { /*IDC_ADD,    */  RED,    CALC_STD_PLUS,      /*false,  false,  false,  false*/ },
    { /*IDC_SUB,    */  RED,    CALC_STD_MINUS,     /*false,  false,  false,  false*/ },
    { /*IDC_MOD,    */  RED,    CALC_SCI_MOD,       /*false,  false,  false,  false*/ },
    { /*IDC_PWR,    */  PURPLE, CALC_SCI_XCARETY,   /*false,  false,  false,  false*/ },
    { /*IDC_CHOP,   */  RED,    CALC_SCI_INT,       /*false,  false,  false,  false*/ },
    { /*IDC_COM,    */  RED,    CALC_SCI_NOT,       /*false,  false,  false,  false*/ },
    { /*IDC_SIN,    */  PURPLE, CALC_SCI_SIN,       /*false,  false,  false,  false*/ },
    { /*IDC_COS,    */  PURPLE, CALC_SCI_COS,       /*false,  false,  false,  false*/ },
    { /*IDC_TAN,    */  PURPLE, CALC_SCI_TAN,       /*false,  false,  false,  false*/ },
    { /*IDC_LN,     */  PURPLE, CALC_SCI_LN,        /*false,  false,  false,  false*/ },
    { /*IDC_LOG,    */  PURPLE, CALC_SCI_LOG,       /*false,  false,  false,  false*/ },
    { /*IDC_SQRT,   */  DKBLUE, CALC_STD_SQRT,      /*false,  false,  false,  false*/ },
    { /*IDC_SQR,    */  PURPLE, CALC_SCI_XCARET2,   /*false,  false,  false,  false*/ },
    { /*IDC_CUB,    */  PURPLE, CALC_SCI_XCARET3,   /*false,  false,  false,  false*/ },
    { /*IDC_FAC,    */  PURPLE, CALC_SCI_FACTORIAL, /*false,  false,  false,  false*/ },
    { /*IDC_REC,    */  PURPLE, CALC_1X,            /*false,  false,  false,  false*/ },
    { /*IDC_DMS,    */  PURPLE, CALC_SCI_DMS,       /*false,  false,  false,  false*/ },
    { /*IDC_PERCENT,*/  DKBLUE, CALC_STD_PERCENT,   /*false,  false,  false,  false*/ },
    { /*IDC_FE,     */  PURPLE, CALC_SCI_FE,        /*false,  false,  false,  false*/ },
    { /*IDC_PI,     */  DKBLUE, CALC_SCI_PI,        /*false,  false,  false,  false*/ },
    { /*IDC_EQU,    */  RED,    CALC_STD_EQUAL,     /*false,  false,  false,  false*/ },
    { /*IDC_MCLEAR, */  RED,    CALC_MC,            /*false,  false,  false,  false*/ },
    { /*IDC_RECALL, */  RED,    CALC_MR,            /*false,  false,  false,  false*/ },
    { /*IDC_STORE,  */  RED,    CALC_MS,            /*false,  false,  false,  false*/ },
    { /*IDC_MPLUS,  */  RED,    CALC_MPLUS,         /*false,  false,  false,  false*/ },
    { /*IDC_EXP,    */  PURPLE, CALC_SCI_EXP,       /*false,  false,  false,  false*/ },
    { /*IDC_AVE,    */  DKBLUE, CALC_SCI_AVE,       /*false,  false,  false,  false*/ },
    { /*IDC_B_SUM,  */  DKBLUE, CALC_SCI_SUM,       /*false,  false,  false,  false*/ },
    { /*IDC_DEV,    */  DKBLUE, CALC_SCI_S,         /*false,  false,  false,  false*/ },
    { /*IDC_DATA,   */  DKBLUE, CALC_SCI_DAT,       /*false,  false,  false,  false*/ },
    { /*IDC_OPENP,  */  PURPLE, CALC_SCI_OPENPAREN, /*false,  false,  false,  false*/ },
    { /*IDC_CLOSEP, */  PURPLE, CALC_SCI_CLOSEPAREN,/*false,  false,  false,  false*/ },
    { /*IDC_0,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_1,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_2,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_3,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_4,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_5,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_6,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_7,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_8,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_9,      */  BLUE,   CALC_STD_NUMBERS,   /*false,  false,  false,  false*/ },
    { /*IDC_A,      */  DKBLUE, CALC_SCI_ABCDEF,    /*false,  false,  false,  false*/ },
    { /*IDC_B,      */  DKBLUE, CALC_SCI_ABCDEF,    /*false,  false,  false,  false*/ },
    { /*IDC_C,      */  DKBLUE, CALC_SCI_ABCDEF,    /*false,  false,  false,  false*/ },
    { /*IDC_D,      */  DKBLUE, CALC_SCI_ABCDEF,    /*false,  false,  false,  false*/ },
    { /*IDC_E,      */  DKBLUE, CALC_SCI_ABCDEF,    /*false,  false,  false,  false*/ },
    { /*IDC_F,      */  DKBLUE, CALC_SCI_ABCDEF,    /*false,  false,  false,  false*/ }
};

// Returns true if the given ID is one of Calc's command buttons
BOOL IsValidID( int iID )
{
    if ( (iID >= IDC_SIGN) && (iID <= IDC_F) )
        return TRUE;

    return FALSE;
}

// Used when processing WM_DRAWITEM to get the key color
COLORREF GetKeyColor( int iID )
{
    ASSERT( IsValidID( iID ) );
    if ( nCalc && (iID == IDC_REC) )
        return DKBLUE;
    return keys[INDEXFROMID(iID)].crColor;
}

// Used when processing WM_CONTEXTHELP to get the Help ID.
// This works for any control ID, not just the command buttons.
ULONG_PTR GetHelpID( int iID )
{
    if ( IsValidID( iID ) )
    {
        return keys[INDEXFROMID(iID)].iHelpID;
    }

    switch( iID )
    {
    case IDC_HEX:
        return CALC_SCI_HEX;
    case IDC_DEC:
        return CALC_SCI_DEC;
    case IDC_OCT:
        return CALC_SCI_OCT;
    case IDC_BIN:
        return CALC_SCI_BIN;
    case IDC_DEG:
        return CALC_SCI_DEG;
    case IDC_RAD:
        return CALC_SCI_RAD;
    case IDC_GRAD:
        return CALC_SCI_GRAD;
    case IDC_QWORD:
        return CALC_SCI_QWORD; 
    case IDC_DWORD:
        return CALC_SCI_DWORD; 
    case IDC_WORD:
        return CALC_SCI_WORD; 
    case IDC_BYTE:
        return CALC_SCI_BYTE; 
    case IDC_INV:
        return CALC_SCI_INV;
    case IDC_HYP:
        return CALC_SCI_HYP;
    case IDC_DISPLAY:
        return CALC_STD_VALUE;
    case IDC_MEMTEXT:
        return CALC_SCI_MEM;
    case IDC_PARTEXT:
        return CALC_SCI_PARENS;
    }

    ASSERT( 0 );    // an invalid help ID has been used.
    return 0;
}


/*
BOOL IsUnaryOperator( int iID )
{
    ASSERT( IsValidID( iID ) );
    return keys[INDEXFROMID(iID)].bUnary;
}

BOOL IsBinaryOperator( int iID )
{
    ASSERT( IsValidID( iID ) );
    return keys[INDEXFROMID(iID)].bBinary;
}

BOOL UsesInvKey( int iID )
{
    ASSERT( IsValidID( iID ) );
    return keys[INDEXFROMID(iID)].bUseInv;
}

BOOL UsesHypKey( int iID )
{
    ASSERT( IsValidID( iID ) );
    return keys[INDEXFROMID(iID)].bUseHyp;
}
*/
