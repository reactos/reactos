/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    assem.c

Abstract:

    This file contains the set of routines which assemble an
    instruction on Alpha.

Author:

    Miche Baker-Harvey

Environment:

    Win32 - User

--*/

/******************** INCLUDE FILES *******************************/
#include "precomp.h"
#pragma hdrstop

#include <alphaops.h>

#include "alphaopt.h"
#include "alphaasm.h"
#include "alpdis.h"

#include "alphastr.h"


#define OPSIZE 16

int assem(PULONG, PUCHAR, PULONG);
BOOLEAN TestCharacter (PUCHAR, PUCHAR *, UCHAR ch);
ULONG GetIntReg(PUCHAR, PUCHAR *);
ULONG GetFltReg(PUCHAR, PUCHAR *);
ULONG GetValue(PUCHAR, PUCHAR *, BOOLEAN, ULONG, PULONG, CHAR);
PUCHAR SkipWhite(PUCHAR *);
ULONG GetToken(PUCHAR, PUCHAR *, PUCHAR, ULONG);



/*** assem - assemble instruction
*
*   Purpose:
*       To assemble the instruction pointed by *poffset.
*
*   Input:
*       instruction - where to put the assembled instruction
*       pchInput    - pointer to string to assemble
*       poffset     -
*
*   Output:
*       status - see ntasm.h for codes
*
*   Exceptions:
*       error exit: xosd
*
*   Notes:
*       errors are handled by the calling program by outputting
*       the error string and reprompting the user for the same
*       instruction.
*
**************************************************************/

int
assem (PULONG instruction, PUCHAR pchInput, PULONG poffset)
{
    UCHAR   szOpcode[OPSIZE];
    ULONG   status;

    POPTBLENTRY pEntry;

    //
    // Using the mnemonic token, find the entry in the assembler's
    // table for the associated instruction.
    //

    if (GetToken(pchInput, &pchInput, szOpcode, OPSIZE) == 0) {
        return(BADOPCODE);
    }

    if ((pEntry = findStringEntry(szOpcode)) == (POPTBLENTRY) -1) {
        return(BADOPCODE);
    }

    if (pEntry->eType == INVALID_ETYPE) {
        return(BADOPCODE);
    }

    //
    // Use the instruction format specific parser to encode the
    // instruction plus its operands.
    //

    status = (*pEntry->parsFunc)
        (pchInput, pEntry, poffset, instruction);

    return (status);
}


BOOLEAN
TestCharacter (PUCHAR inString, PUCHAR *outString, UCHAR ch)
{

    inString = SkipWhite(&inString);
    if (ch == *inString) {
        *outString = inString+1;
        return TRUE;
        }
    else {
        *outString = inString;
        return FALSE;
        }
}


/*** GetIntReg - get integer register number
 *** GetFltReg - get floating register number
*
*   Purpose:
*       From reading the input stream, return the register number.
*
*   Input:
*       inString - pointer to input string
*
*   Output:
*       *outString - pointer to character after register token in input stream
*
*   Returns:
*       register number
*
*   Exceptions:
*       error(BADREG) - bad register name
*
*************************************************************************/

PUCHAR regNums[] = {
         "$0",  "$1",  "$2",  "$3",  "$4",  "$5",  "$6",  "$7",
         "$8",  "$9", "$10", "$11", "$12", "$13", "$14", "$15",
        "$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23",
        "$24", "$25", "$26", "$27", "$28", "$29", "$30", "$31"
        };

PUCHAR intRegNames[] = {
         szR0,  szR1,  szR2,  szR3,  szR4,  szR5,  szR6,  szR7,
         szR8,  szR9, szR10, szR11, szR12, szR13, szR14, szR15,
        szR16, szR17, szR18, szR19, szR20, szR21, szR22, szR23,
        szR24, szR25, szR26, szR27, szR28, szR29, szR30, szR31

        };

PUCHAR fltRegNames[] = {
         "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
         "f8",  "f9", "f10", "f11", "f12", "f13", "f14", "f15",
        "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
        "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
        };

ULONG
GetIntReg (PUCHAR inString, PUCHAR *outString)
{
    UCHAR   szRegOp[5];
    ULONG   index;

    if (!GetToken(inString, outString, szRegOp, sizeof(szRegOp)))
        return(BADREG);

    if (szRegOp[0] == '$') {
        //
        // use numbers
        //
        for (index = 0; index < 32; index++) {
            if (!strcmp(szRegOp, regNums[index]))
                return index;
        }
    } else {
        //
        // use names
        //
        for (index = 0; index < 32; index++) {
            if (!strcmp(szRegOp, intRegNames[index]))
                return index;
        }
    }
    return(BADREG);
}

ULONG
GetFltReg (PUCHAR inString, PUCHAR *outString)
{
    UCHAR   szRegOp[5];
    ULONG   index;

    if (!GetToken(inString, outString, szRegOp, sizeof(szRegOp)))
        return(BADREG);

    if (szRegOp[0] == '$') {
        //
        // use numbers
        //
        for (index = 0; index < 32; index++) {
            if (!strcmp(szRegOp, regNums[index]))
                return index;
        }
    } else {
        //
        // use names
        //
        for (index = 0; index < 32; index++) {
            if (!strcmp(szRegOp, fltRegNames[index]))
                return index;
        }
    }

    return(BADREG);
}


/*** GetValue - get value from command line
*
*   Purpose:
*       Use GetExpression to evaluate the next expression in the input
*       stream.
*
*   Input:
*       inString - pointer to input stream
*       fSigned - TRUE if signed value
*                 FALSE if unsigned value
*       bitsize - size of value allowed
*
*   Output:
*       outString - character after the last character of the expression
*       outValue  - where to return the value
*
*   Returns:
*       GOODINSTRUCTION, or error from ntasm.h
*
*   Exceptions:
*       return exit: OVERFLOW - value too large for bitsize
*
*************************************************************************/

ULONG
GetValue (PUCHAR  inString,
          PUCHAR  *outString,
          BOOLEAN fSigned,
          ULONG   bitsize,
          PULONG  valuep,
          CHAR    ch)
{
    ULONG   value, index, err;

    inString = SkipWhite(&inString);

    //
    // find the character that marks the end of the string,
    // and replace it with a null;
    // otherwise, the expression evaluator fails
    //

    for ( index = 0; /**/ ; index++ ) {

        if (inString[index] == ch) {
            inString[index] = '\0';
            break;
        }

        if (inString[index] == '\0') {
            return OPERAND;
        }
    }

    //
    // This is a callback to the shell which does the work.
    //

    err = DHGetNumber(inString, &value);

    //
    // first replace any overwritten character
    //
    inString[index] = ch;

    //
    // DHGetNumber returns  0 if it succeeds.
    //
    if ( err != GOODINSTRUCTION ) {
       return err;
    }

    if ((value > (ULONG)(1L << bitsize) - 1) &&
            (!fSigned || (value < (ULONG)(-1L << (bitsize - 1)))))
        return OVERFLOW;

    *outString = inString + index;
    *valuep = value;
    return GOODINSTRUCTION;
}


/*** SkipWhite - skip white-space
*
*   Purpose:
*       To advance pchCommand over any spaces or tabs.
*
*   Input:
*       *pchCommand - present command line position
*
*************************************************************************/

PUCHAR
SkipWhite (PUCHAR * string)
{
    while (**string == ' ' || **string == '\t')
        (*string)++;

    return(*string);
}


/*** GetToken - get token from command line
*
*   Purpose:
*       Build a lower-case mapped token of maximum size maxcnt
*       at the string pointed by *psz.  Token consist of the
*       set of characters a-z, A-Z, 0-9, $, and underscore.
*
*   Input:
*       *inString - present command line position
*       maxcnt - maximum size of token allowed
*
*   Output:
*       *outToken - token in lower case
*       *outString - pointer to first character beyond token in input
*
*   Returns:
*       size of token if under maximum else 0
*
*   Notes:
*       if string exceeds maximum size, the extra characters
*       are still processed, but ignored.
*
*************************************************************************/

ULONG
GetToken (PUCHAR inString, PUCHAR *outString, PUCHAR outToken, ULONG maxcnt)
{
    UCHAR   ch;
    ULONG   count = 0;

    inString = SkipWhite(&inString);

    while (count < maxcnt) {
        ch = (UCHAR)tolower(*inString);

        if (!((ch >= '0' && ch <= '9') ||
              (ch >= 'a' && ch <= 'z') ||
              (ch == '$') ||
              (ch == '_') ||
              (ch == '#')))
                break;

        count++;
        *outToken++ = ch;
        inString++;
        }

    *outToken = '\0';
    *outString = inString;

    return (count >= maxcnt ? 0 : count);
}


/*** ParseIntMemory - parse integer memory instruction
*
*   Purpose:
*       Given the users input, create the instruction.
*
*   Input:
*       inString     - present input position
*       pEntry       - pointer into the opTable for this opcode
*       poffset      - pointer to offset where assembled instr will go
*       pinstruction - pointer to where to put the instruction
*
*   Output:
*       &pinstruction - the assembled instruction
*
*   Returns:
*       the error code, or GOODINSTRUCTINO
*
*   Format:
*       op Ra, disp(Rb)
*
*************************************************************************/

ULONG
ParseIntMemory(PUCHAR inString,
               POPTBLENTRY pEntry,
               PULONG poffset,
               PULONG instruction)
{
    ULONG Ra;
    ULONG Rb;
    ULONG disp;
    ULONG err;

    Ra = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        return(OPERAND);

    err = GetValue(inString, &inString, TRUE, WIDTH_MEM_DISP, &disp, '(');
    if (err != GOODINSTRUCTION)
        return err;

    if (!TestCharacter(inString, &inString, '('))
        return(OPERAND);

    Rb = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ')'))
        return(OPERAND);

    if (!TestCharacter(inString, &inString, '\0'))
        return(EXTRACHARS);

    *instruction = OPCODE(pEntry->opCode) +
                   REG_A(Ra) +
                   REG_B(Rb) +
                   MEM_DISP(disp);

    return(GOODINSTRUCTION);
}

/*** ParseFltMemory - parse floating point memory instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Format:
*       op Fa, disp(Rb)
*
*************************************************************************/

ULONG
ParseFltMemory(PUCHAR inString,
               POPTBLENTRY pEntry,
               PULONG poffset,
               PULONG instruction)
{
    ULONG Fa;
    ULONG Rb;
    ULONG disp;
    ULONG err;

    Fa = GetFltReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        return(OPERAND);

    err = GetValue(inString, &inString, TRUE, WIDTH_MEM_DISP, &disp, '(');
    if (err != GOODINSTRUCTION)
        return err;

    if (!TestCharacter(inString, &inString, '('))
        return(OPERAND);

    Rb = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ')'))
        return(OPERAND);

    if (!TestCharacter(inString, &inString, '\0'))
        return(EXTRACHARS);

    *instruction = OPCODE(pEntry->opCode) +
                   REG_A(Fa) +
                   REG_B(Rb) +
                   MEM_DISP(disp);

    return(GOODINSTRUCTION);
}

/*** ParseMemSpec - parse special memory instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Format:
*       op
*
*************************************************************************/
ULONG ParseMemSpec(PUCHAR inString,
                   POPTBLENTRY pEntry,
                   PULONG poffset,
                   PULONG instruction)
{
    *instruction = (OPCODE(pEntry->opCode) +
                    MEM_FUNC(pEntry->funcCode));
    return (GOODINSTRUCTION);
}

/*** ParseJump - parse jump instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Format:
*       op Ra,(Rb),hint
*       op Ra,(Rb)       - not really - we just support it here
*
*************************************************************************/

ULONG ParseJump(PUCHAR inString,
                POPTBLENTRY pEntry,
                PULONG poffset,
                PULONG instruction)
{
    ULONG Ra;
    ULONG Rb;
    ULONG hint;
    ULONG err;

    Ra = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        return(OPERAND);

    if (!TestCharacter(inString, &inString, '('))
        return(OPERAND);

    Rb = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ')'))
        return(OPERAND);

    if (TestCharacter(inString, &inString, ',')) {
        //
        // User is giving us a hint
        //
        err = GetValue(inString, &inString, TRUE, WIDTH_HINT, &hint, '\0');
        if (err != GOODINSTRUCTION)
            return err;
    } else {
        hint = 0;
    }

    if (!TestCharacter(inString, &inString, '\0'))
        return(EXTRACHARS);

    *instruction = OPCODE(pEntry->opCode) +
                   JMP_FNC(pEntry->funcCode) +
                   REG_A(Ra) +
                   REG_B(Rb) +
                   HINT(hint);

    return(GOODINSTRUCTION);
}

/*** ParseIntBranch - parse integer branch instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Format:
*       op Ra,disp
*
*************************************************************************/

ULONG ParseIntBranch(PUCHAR inString,
                     POPTBLENTRY pEntry,
                     PULONG poffset,
                     PULONG instruction)
{
    ULONG Ra;
    LONG disp;
    ULONG err;

    Ra = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        return(OPERAND);

    //
    // the user gives an absolute address; we convert
    // that to a displacement, which is computed as a
    // difference off of (pc+1)
    // GetValue handles both numerics and symbolics
    //
    err = GetValue(inString, &inString, TRUE, 32, &disp, '\0');
    if (err != GOODINSTRUCTION)
        return err;

    // get the relative displacement from the updated pc
    disp = disp - (LONG)((*poffset)+4);

    // divide by four
    disp = disp >> 2;

    if (!TestCharacter(inString, &inString, '\0'))
        return(EXTRACHARS);

    *instruction = OPCODE(pEntry->opCode) +
                   REG_A(Ra) +
                   BR_DISP(disp);

    return(GOODINSTRUCTION);
}


/*** ParseFltBranch - parse floating point branch instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Format:
*       op Fa,disp
*
*************************************************************************/
ULONG ParseFltBranch(PUCHAR inString,
                     POPTBLENTRY pEntry,
                     PULONG poffset,
                     PULONG instruction)
{
    ULONG Ra;
    LONG disp;
    ULONG err;

    Ra = GetFltReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        return(OPERAND);

    //
    // the user gives an absolute address; we convert
    // that to a displacement, which is computed as a
    // difference off of (pc+1)
    // GetValue handles both numerics and symbolics
    //
    err = GetValue(inString, &inString, TRUE, 32, &disp, '\0');
    if (err != GOODINSTRUCTION)
        return err;

    // get the relative displacement from the updated pc
    disp = disp - (LONG)((*poffset)+4);

    // divide by four
    disp = disp >> 2;

    if (!TestCharacter(inString, &inString, '\0'))
        return(EXTRACHARS);

    *instruction = OPCODE(pEntry->opCode) +
                   REG_A(Ra) +
                   BR_DISP(disp);

    return(GOODINSTRUCTION);
}


/*** ParseIntOp - parse integer operation
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Format:
*       op Ra, Rb, Rc
*       op Ra, #lit, Rc
*
*************************************************************************/

ULONG ParseIntOp(PUCHAR inString,
                 POPTBLENTRY pEntry,
                 PULONG poffset,
                 PULONG instruction)
{
    ULONG Ra, Rb, Rc;
    ULONG lit;
    ULONG Format;       // Whether there is a literal or 3rd reg
    ULONG err;

    *instruction = OPCODE(pEntry->opCode) +
                   OP_FNC(pEntry->funcCode);

    Ra = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        return(OPERAND);

    if (TestCharacter(inString, &inString, '#')) {

        //
        // User is giving us a literal value

        err = GetValue(inString, &inString, TRUE, WIDTH_LIT, &lit, ',');
        if (err != GOODINSTRUCTION)
            return err;
        Format = RBV_LITERAL_FORMAT;

    } else {

        //
        // using a third register value

        Rb = GetIntReg(inString, &inString);
        Format = RBV_REGISTER_FORMAT;
    }

    if (!TestCharacter(inString, &inString, ','))
        return(OPERAND);

    Rc = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, '\0'))
        return(EXTRACHARS);

    *instruction = *instruction +
                    REG_A(Ra) +
                    RBV_TYPE(Format) +
                    REG_C(Rc);

    if (Format == RBV_REGISTER_FORMAT) {
        *instruction = *instruction + REG_B(Rb);
    } else {
        *instruction = *instruction + LIT(lit);
    }

    return(GOODINSTRUCTION);
}


/*** ParsePal - parse PAL code instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Format:
*       op
*
*************************************************************************/
ULONG ParsePal(PUCHAR inString,
               POPTBLENTRY pEntry,
               PULONG poffset,
               PULONG instruction)
{
    if (!TestCharacter(inString, &inString, '\0'))
        return(EXTRACHARS);

    *instruction = (OPCODE(pEntry->opCode) +
                   PAL_FNC(pEntry->funcCode));
    return (GOODINSTRUCTION);
}


/*** ParseUnknown - return an error message for an unknown opcode
*
*   Purpose:
*       return an error message for an unknown opcode
*
*   Format:
*       ???
*
*************************************************************************/
ULONG ParseUnknown(PUCHAR inString,
                   POPTBLENTRY pEntry,
                   PULONG poffset,
                   PULONG instruction)
{
    return(BADOPCODE);
}

