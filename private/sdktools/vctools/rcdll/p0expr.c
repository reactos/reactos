/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* P0EXPR.C - Expression routines for Pre-Processor                     */
/*                                                                      */
/* AUTHOR - Ralph Ryan, Sept. 16, 1982                                  */
/* 06-Dec-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/
/*
 * DESCRIPTION
 *      Evaluate the constant expression.  Since these routines are
 *      all recursively coupled, it is clearer NOT to document them
 *      with the standard header.  Instead, BML (British Meta Language,
 *      a BNF like meta language) will be given for each "production"
 *      of this recursive descent parser.
 *
 * Note - Sure, yeah, right. Frankly, I'm frightened! (w-BrianM)
 ************************************************************************/

#include "rc.h"

/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
long and(void);
long andif(void);
long constant(void);
long constexpr(void);
long eqset(void);
long mult(void);
long or(void);
long orelse(void);
long plus(void);
long prim(void);
long relation(void);
long shift(void);
long xor(void);


/************************************************************************/
/* File Global Variables                                                */
/************************************************************************/
long    Currval = 0;
static  int             Parencnt = 0;


/************************************************************************/
/* do_constexpr()                                                       */
/************************************************************************/
long
do_constexpr(
    void
    )
{
    REG long    val;

    Parencnt = 0;
    Currtok = L_NOTOKEN;
    val = constexpr();
    if( Currtok == L_RPAREN ) {
        if( Parencnt-- == 0 ) {
            Msg_Temp = GET_MSG(1012);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, "(");
            fatal(1012);                /* missing left paren */
        }
    } else if( Currtok != L_NOTOKEN ) {
        Msg_Temp = GET_MSG(4067);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, PPifel_str);
        warning(4067);
    }

    if( Parencnt > 0 ) {
        Msg_Temp = GET_MSG(4012);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, ")");
        fatal(4012);    /* missing right paren */
    }
    return(val);
}

/************************************************************************/
/* constexpr ::= orelse [ '?' orelse ':' orelse ];                      */
/************************************************************************/
long
constexpr(
    void
    )
{
    REG long            val;
    REG long            val1;
    long                val2;

    val = orelse();
    if( nextis(L_QUEST) ) {
        val1 = orelse();
        if( nextis(L_COLON) )
            val2 = orelse();
        return(val ? val1 : val2);
    }
    return(val);
}


/************************************************************************/
/* orelse ::= andif [ '||' andif ]* ;                                   */
/************************************************************************/
long
orelse(
    void
    )
{
    REG long val;

    val = andif();
    while(nextis(L_OROR))
        val = andif() || val;
    return(val);
}


/************************************************************************/
/* andif ::= or [ '&&' or ]* ;                                          */
/************************************************************************/
long
andif(
    void
    )
{
    REG long val;

    val = or();
    while(nextis(L_ANDAND))
        val = or() && val;
    return(val);
}


/************************************************************************/
/* or ::= xor [ '|' xor]* ;                                             */
/************************************************************************/
long
or(
    void
    )
{
    REG long val;

    val = xor();
    while( nextis(L_OR) )
        val |= xor();
    return(val);
}


/************************************************************************/
/* xor ::= and [ '^' and]* ;                                            */
/************************************************************************/
long
xor(
    void
    )
{
    REG long val;

    val = and();
    while( nextis(L_XOR) )
        val ^= and();
    return(val);
}


/************************************************************************/
/*  and ::= eqset [ '&' eqset]* ;                                       */
/************************************************************************/
long
and(
    void
    )
{
    REG long val;

    val = eqset();
    while( nextis(L_AND) )
        val &= eqset();
    return(val);
}


/************************************************************************/
/* eqset ::= relation [ ('==' | '!=') eqset] ;                          */
/************************************************************************/
long
eqset(
    void
    )
{
    REG long val;

    val = relation();
    if( nextis(L_EQUALS) )
        return(val == relation());
    if( nextis(L_NOTEQ) )
        return(val != relation());
    return(val);
}

/************************************************************************/
/* relation ::= shift [ ('<' | '>' | '<=' | '>=' ) shift] ;             */
/************************************************************************/
long
relation(
    void
    )
{
    REG long val;

    val = shift();
    if( nextis(L_LT) )
        return(val < shift());
    if( nextis(L_GT) )
        return(val > shift());
    if( nextis(L_LTEQ) )
        return(val <= shift());
    if( nextis(L_GTEQ) )
        return(val >= shift());
    return(val);
}


/************************************************************************/
/* shift ::= plus [ ('<< | '>>') plus] ;                                */
/************************************************************************/
long
shift(
    void
    )
{
    REG long val;

    val = plus();
    if( nextis(L_RSHIFT) )
        return(val >> plus());
    if( nextis(L_LSHIFT) )
        return(val << plus());
    return(val);
}


/************************************************************************/
/* plus ::= mult [ ('+' | '-') mult ]* ;                                */
/************************************************************************/
long
plus(
    void
    )
{
    REG long val;

    val = mult();
    for(;;) {
        if( nextis(L_PLUS) )
            val += mult();
        else if( nextis(L_MINUS) )
            val -= mult();
        else
            break;
    }
    return(val);
}


/************************************************************************/
/* mult ::= prim [ ('*' | '/' | '%' ) prim ]* ;                         */
/************************************************************************/
long
mult(
    void
    )
{
    REG long val;

    val = prim();
    for(;;) {
        if( nextis(L_MULT) )
            val *= prim();
        else if( nextis(L_DIV) )
            val /= prim();
        else if( nextis(L_MOD) )
            val %= prim();
        else
            break;
    }
    return(val);
}


/************************************************************************/
/* prim ::= constant | ( '!' | '~' | '-' ) constant                     */
/************************************************************************/
long
prim(
    void
    )
{
    if( nextis(L_EXCLAIM) )
        return( ! constant());
    else if( nextis(L_TILDE) )
        return( ~ constant() );
    else if( nextis(L_MINUS) )
        return(-constant());
    else
        return(constant());
}


/************************************************************************/
/* constant - at last, a terminal symbol  | '(' constexpr ')'           */
/************************************************************************/
long
constant(
    void
    )
{
    REG long val;

    if( nextis(L_LPAREN) ) {
        Parencnt++;
        val = constexpr();
        if( nextis(L_RPAREN) ) {
            Parencnt--;
            return(val);
        } else {
            Msg_Temp = GET_MSG(1012);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, ")");
            fatal (1012);
        }
    } else if( ! nextis(L_CINTEGER) ) {
        Msg_Temp = GET_MSG(1017);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp);
        fatal(1017);    /* invalid integer constant expression */
    }
    return(Currval);
}
