/*
 *  ReactOS kernel
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kdbg/kdb_expr.c
 * PURPOSE:         Kernel debugger expression evaluation
 * PROGRAMMER:      Gregor Anich (blight@blight.eu.org)
 * UPDATE HISTORY:
 *                  Created 15/01/2005
 */

/* Note:
 *
 * The given expression is parsed and stored in reverse polish notation,
 * then it is evaluated and the result is returned.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/
typedef enum _RPN_OP_TYPE
{
    RpnOpNop,
    RpnOpBinaryOperator,
    RpnOpUnaryOperator,
    RpnOpImmediate,
    RpnOpRegister,
    RpnOpDereference
} RPN_OP_TYPE;

typedef ULONGLONG (*RPN_BINARY_OPERATOR)(ULONGLONG a, ULONGLONG b);

typedef struct _RPN_OP
{
    RPN_OP_TYPE  Type;
    ULONG        CharacterOffset;
    union
    {
        /* RpnOpBinaryOperator */
        RPN_BINARY_OPERATOR  BinaryOperator;
        /* RpnOpImmediate */
        ULONGLONG            Immediate;
        /* RpnOpRegister */
        UCHAR                Register;
        /* RpnOpDereference */
        UCHAR                DerefMemorySize;
    }
    Data;
}
RPN_OP, *PRPN_OP;

typedef struct _RPN_STACK
{
    ULONG   Size;     /* Number of RPN_OPs on Ops */
    ULONG   Sp;       /* Stack pointer */
    RPN_OP  Ops[1];   /* Array of RPN_OPs */
}
RPN_STACK, *PRPN_STACK;

/* DEFINES *******************************************************************/
#define stricmp _stricmp

#ifndef RTL_FIELD_SIZE
# define RTL_FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif

#define CONST_STRCPY(dst, src) \
    do { if ((dst)) { memcpy(dst, src, sizeof(src)); } } while (0);

#define RPN_OP_STACK_SIZE     256
#define RPN_VALUE_STACK_SIZE  256

/* GLOBALS *******************************************************************/
static struct
{
    ULONG Size;
    ULONG Sp;
    RPN_OP Ops[RPN_OP_STACK_SIZE];
}
RpnStack =
{
    RPN_OP_STACK_SIZE,
    0
};

static const struct
{
    PCHAR Name;
    UCHAR Offset;
    UCHAR Size;
}
RegisterToTrapFrame[] =
{
    {"eip",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Eip),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Eip)},
    {"eflags",  FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.EFlags),  RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.EFlags)},
    {"eax",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Eax),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Eax)},
    {"ebx",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Ebx),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Ebx)},
    {"ecx",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Ecx),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Ecx)},
    {"edx",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Edx),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Edx)},
    {"esi",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Esi),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Esi)},
    {"edi",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Edi),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Edi)},
    {"esp",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.HardwareEsp),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.HardwareEsp)},
    {"ebp",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Ebp),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Ebp)},
    {"cs",      FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.SegCs),      2 }, /* Use only the lower 2 bytes */
    {"ds",      FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.SegDs),      RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.SegDs)},
    {"es",      FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.SegEs),      RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.SegEs)},
    {"fs",      FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.SegFs),      RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.SegFs)},
    {"gs",      FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.SegGs),      RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.SegGs)},
    {"ss",      FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.HardwareSegSs),      RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.HardwareSegSs)},
    {"dr0",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Dr0),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Dr0)},
    {"dr1",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Dr1),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Dr1)},
    {"dr2",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Dr2),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Dr2)},
    {"dr3",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Dr3),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Dr3)},
    {"dr6",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Dr6),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Dr6)},
    {"dr7",     FIELD_OFFSET(KDB_KTRAP_FRAME, Tf.Dr7),     RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Tf.Dr7)},
    {"cr0",     FIELD_OFFSET(KDB_KTRAP_FRAME, Cr0),        RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Cr0)},
    {"cr2",     FIELD_OFFSET(KDB_KTRAP_FRAME, Cr2),        RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Cr2)},
    {"cr3",     FIELD_OFFSET(KDB_KTRAP_FRAME, Cr3),        RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Cr3)},
    {"cr4",     FIELD_OFFSET(KDB_KTRAP_FRAME, Cr4),        RTL_FIELD_SIZE(KDB_KTRAP_FRAME, Cr4)}
};
static const INT RegisterToTrapFrameCount = sizeof (RegisterToTrapFrame) / sizeof (RegisterToTrapFrame[0]);

/* FUNCTIONS *****************************************************************/

ULONGLONG
RpnBinaryOperatorAdd(
    ULONGLONG a,
    ULONGLONG b)
{
    return a + b;
}

ULONGLONG
RpnBinaryOperatorSub(
    ULONGLONG a,
    ULONGLONG b)
{
    return a - b;
}

ULONGLONG
RpnBinaryOperatorMul(
    ULONGLONG a,
    ULONGLONG b)
{
    return a * b;
}

ULONGLONG
RpnBinaryOperatorDiv(
    ULONGLONG a,
    ULONGLONG b)
{
    return a / b;
}

ULONGLONG
RpnBinaryOperatorMod(
    ULONGLONG a,
    ULONGLONG b)
{
    return a % b;
}

ULONGLONG
RpnBinaryOperatorEquals(
    ULONGLONG a,
    ULONGLONG b)
{
    return (a == b);
}

ULONGLONG
RpnBinaryOperatorNotEquals(
    ULONGLONG a,
    ULONGLONG b)
{
    return (a != b);
}

ULONGLONG
RpnBinaryOperatorLessThan(
    ULONGLONG a,
    ULONGLONG b)
{
    return (a < b);
}

ULONGLONG
RpnBinaryOperatorLessThanOrEquals(
    ULONGLONG a,
    ULONGLONG b)
{
    return (a <= b);
}

ULONGLONG
RpnBinaryOperatorGreaterThan(
    ULONGLONG a,
    ULONGLONG b)
{
    return (a > b);
}

ULONGLONG
RpnBinaryOperatorGreaterThanOrEquals(
    ULONGLONG a,
    ULONGLONG b)
{
    return (a >= b);
}

/*!\brief Dumps the given RPN stack content
 *
 * \param Stack  Pointer to a RPN_STACK structure.
 */
VOID
RpnpDumpStack(
    IN PRPN_STACK Stack)
{
    ULONG ul;

    ASSERT(Stack);
    DbgPrint("\nStack size: %ld\n", Stack->Sp);

    for (ul = 0; ul < Stack->Sp; ul++)
    {
        PRPN_OP Op = Stack->Ops + ul;
        switch (Op->Type)
        {
            case RpnOpNop:
                DbgPrint("NOP,");
                break;

            case RpnOpImmediate:
                DbgPrint("0x%I64x,", Op->Data.Immediate);
                break;

            case RpnOpBinaryOperator:
                if (Op->Data.BinaryOperator == RpnBinaryOperatorAdd)
                    DbgPrint("+,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorSub)
                    DbgPrint("-,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorMul)
                    DbgPrint("*,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorDiv)
                    DbgPrint("/,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorMod)
                    DbgPrint("%%,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorEquals)
                    DbgPrint("==,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorNotEquals)
                    DbgPrint("!=,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorLessThan)
                    DbgPrint("<,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorLessThanOrEquals)
                    DbgPrint("<=,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorGreaterThan)
                    DbgPrint(">,");
                else if (Op->Data.BinaryOperator == RpnBinaryOperatorGreaterThanOrEquals)
                    DbgPrint(">=,");
                else
                    DbgPrint("UNKNOWN OP,");

                break;

            case RpnOpRegister:
                DbgPrint("%s,", RegisterToTrapFrame[Op->Data.Register].Name);
                break;

            case RpnOpDereference:
                DbgPrint("[%s],",
                    (Op->Data.DerefMemorySize == 1) ? ("byte") :
                    ((Op->Data.DerefMemorySize == 2) ? ("word") :
                    ((Op->Data.DerefMemorySize == 4) ? ("dword") : ("qword"))));
                break;

            default:
                DbgPrint("\nUnsupported Type: %d\n", Op->Type);
                ul = Stack->Sp;
                break;
        }
    }

    DbgPrint("\n");
}

/*!\brief Clears the given RPN stack.
 *
 * \param Stack  Pointer to a RPN_STACK structure.
 */
static VOID
RpnpClearStack(
    OUT PRPN_STACK Stack)
{
    ASSERT(Stack);
    Stack->Sp = 0;
}

/*!\brief Pushes an RPN_OP onto the stack.
 *
 * \param Stack  Pointer to a RPN_STACK structure.
 * \param Op     RPN_OP to be copied onto the stack.
 */
static BOOLEAN
RpnpPushStack(
    IN OUT PRPN_STACK Stack,
    IN     PRPN_OP Op)
{
    ASSERT(Stack);
    ASSERT(Op);

    if (Stack->Sp >= Stack->Size)
        return FALSE;

    memcpy(Stack->Ops + Stack->Sp, Op, sizeof (RPN_OP));
    Stack->Sp++;

    return TRUE;
}

/*!\brief Pops the top op from the stack.
 *
 * \param Stack  Pointer to a RPN_STACK structure.
 * \param Op     Pointer to an RPN_OP to store the popped op into (can be NULL).
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure (stack empty)
 */
static BOOLEAN
RpnpPopStack(
    IN OUT PRPN_STACK Stack,
    OUT    PRPN_OP Op  OPTIONAL)
{
    ASSERT(Stack);

    if (Stack->Sp == 0)
        return FALSE;

    Stack->Sp--;
    if (Op)
        memcpy(Op, Stack->Ops + Stack->Sp, sizeof (RPN_OP));

    return TRUE;
}

/*!\brief Gets the top op from the stack (not popping it)
 *
 * \param Stack  Pointer to a RPN_STACK structure.
 * \param Op     Pointer to an RPN_OP to copy the top op into.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure (stack empty)
 */
static BOOLEAN
RpnpTopStack(
    IN  PRPN_STACK Stack,
    OUT PRPN_OP Op)
{
    ASSERT(Stack);
    ASSERT(Op);

    if (Stack->Sp == 0)
        return FALSE;

    memcpy(Op, Stack->Ops + Stack->Sp - 1, sizeof (RPN_OP));

    return TRUE;
}

/*!\brief Parses an expression.
 *
 * This functions parses the given expression until the end of string or a closing
 * brace is found. As the function parses the string it pushes RPN_OPs onto the
 * stack.
 *
 * Examples: 1+2*3 ; eax+10 ; (eax+16) * (ebx+4) ; dword[eax]
 *
 * \param Stack            Pointer to a RPN_STACK structure.
 * \param Expression       String to parse.
 * \param CharacterOffset  Character offset of the subexpression from the beginning of the expression.
 * \param End              On success End is set to the character at which parsing stopped.
 * \param ErrOffset        On failure this is set to the character offset at which the error occoured.
 * \param ErrMsg           On failure a message describing the problem is copied into this buffer (128 bytes)
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 */
static BOOLEAN
RpnpParseExpression(
    IN  PRPN_STACK Stack,
    IN  PCHAR Expression,
    OUT PCHAR *End  OPTIONAL,
    IN  ULONG CharacterOffset,
    OUT PLONG ErrOffset  OPTIONAL,
    OUT PCHAR ErrMsg  OPTIONAL)
{
    PCHAR p = Expression;
    PCHAR pend;
    PCHAR Operator = NULL;
    LONG OperatorOffset = -1;
    RPN_OP RpnOp;
    RPN_OP PoppedOperator;
    BOOLEAN HavePoppedOperator = FALSE;
    RPN_OP ComparativeOp;
    BOOLEAN ComparativeOpFilled = FALSE;
    BOOLEAN IsComparativeOp;
    INT i, i2;
    ULONG ul;
    UCHAR MemorySize;
    CHAR Buffer[16];
    BOOLEAN First;

    ASSERT(Stack);
    ASSERT(Expression);

    First = TRUE;
    for (;;)
    {
        /* Skip whitespace */
        while (isspace(*p))
        {
            p++;
            CharacterOffset++;
        }

        /* Check for end of expression */
        if (p[0] == '\0' || p[0] == ')' || p[0] == ']')
            break;

        if (!First)
        {
            /* Remember operator */
            Operator = p++;
            OperatorOffset = CharacterOffset++;

            /* Pop operator (to get the right operator precedence) */
            HavePoppedOperator = FALSE;
            if (*Operator == '*' || *Operator == '/' || *Operator == '%')
            {
                if (RpnpTopStack(Stack, &PoppedOperator) &&
                    PoppedOperator.Type == RpnOpBinaryOperator &&
                    (PoppedOperator.Data.BinaryOperator == RpnBinaryOperatorAdd ||
                    PoppedOperator.Data.BinaryOperator == RpnBinaryOperatorSub))
                {
                    RpnpPopStack(Stack, NULL);
                    HavePoppedOperator = TRUE;
                }
                else if (PoppedOperator.Type == RpnOpNop)
                {
                    RpnpPopStack(Stack, NULL);
                    /* Discard the NOP - it was only pushed to indicate there was a
                     * closing brace, so the previous operator shouldn't be popped.
                     */
                }
            }
            else if ((Operator[0] == '=' && Operator[1] == '=') ||
                     (Operator[0] == '!' && Operator[1] == '=') ||
                     Operator[0] == '<' || Operator[0] == '>')
            {
                if (Operator[0] == '=' || Operator[0] == '!' ||
                    (Operator[0] == '<' && Operator[1] == '=') ||
                    (Operator[0] == '>' && Operator[1] == '='))
                {
                    p++;
                    CharacterOffset++;
                }
#if 0
                /* Parse rest of expression */
                if (!RpnpParseExpression(Stack, p + 1, &pend, CharacterOffset + 1,
                                         ErrOffset, ErrMsg))
                {
                    return FALSE;
                }
                else if (pend == p + 1)
                {
                    CONST_STRCPY(ErrMsg, "Expression expected");

                    if (ErrOffset)
                        *ErrOffset = CharacterOffset + 1;

                    return FALSE;
                }

                goto end_of_expression; /* return */
#endif
            }
            else if (Operator[0] != '+' && Operator[0] != '-')
            {
                CONST_STRCPY(ErrMsg, "Operator expected");

                if (ErrOffset)
                    *ErrOffset = OperatorOffset;

                return FALSE;
            }

            /* Skip whitespace */
            while (isspace(*p))
            {
                p++;
                CharacterOffset++;
            }
        }

        /* Get operand */
        MemorySize = sizeof(ULONG_PTR); /* default to pointer size */

get_operand:
        i = strcspn(p, "+-*/%()[]<>!=");
        if (i > 0)
        {
            i2 = i;

            /* Copy register name/memory size */
            while (isspace(p[--i2]));

            i2 = min(i2 + 1, (INT)sizeof (Buffer) - 1);
            strncpy(Buffer, p, i2);
            Buffer[i2] = '\0';

            /* Memory size prefix */
            if (p[i] == '[')
            {
                if (stricmp(Buffer, "byte") == 0)
                    MemorySize = 1;
                else if (stricmp(Buffer, "word") == 0)
                    MemorySize = 2;
                else if (stricmp(Buffer, "dword") == 0)
                    MemorySize = 4;
                else if (stricmp(Buffer, "qword") == 0)
                    MemorySize = 8;
                else
                {
                    CONST_STRCPY(ErrMsg, "Invalid memory size prefix");

                    if (ErrOffset)
                        *ErrOffset = CharacterOffset;

                    return FALSE;
                }

                p += i;
                CharacterOffset += i;
                goto get_operand;
            }

            /* Try to find register */
            for (i = 0; i < RegisterToTrapFrameCount; i++)
            {
                if (stricmp(RegisterToTrapFrame[i].Name, Buffer) == 0)
                    break;
            }

            if (i < RegisterToTrapFrameCount)
            {
                RpnOp.Type = RpnOpRegister;
                RpnOp.CharacterOffset = CharacterOffset;
                RpnOp.Data.Register = i;
                i = strlen(RegisterToTrapFrame[i].Name);
                CharacterOffset += i;
                p += i;
            }
            else
            {
                /* Immediate value */
                /* FIXME: Need string to ULONGLONG function */
                ul = strtoul(p, &pend, 0);
                if (p != pend)
                {
                    RpnOp.Type = RpnOpImmediate;
                    RpnOp.CharacterOffset = CharacterOffset;
                    RpnOp.Data.Immediate = (ULONGLONG)ul;
                    CharacterOffset += pend - p;
                    p = pend;
                }
                else
                {
                    CONST_STRCPY(ErrMsg, "Operand expected");

                    if (ErrOffset)
                        *ErrOffset = CharacterOffset;

                    return FALSE;
                }
            }

            /* Push operand */
            if (!RpnpPushStack(Stack, &RpnOp))
            {
                CONST_STRCPY(ErrMsg, "RPN op stack overflow");

                if (ErrOffset)
                    *ErrOffset = -1;

                return FALSE;
            }
        }
        else if (i == 0)
        {
            if (p[0] == '(' || p[0] == '[') /* subexpression */
            {
                if (!RpnpParseExpression(Stack, p + 1, &pend, CharacterOffset + 1,
                                         ErrOffset, ErrMsg))
                {
                    return FALSE;
                }
                else if (pend == p + 1)
                {
                    CONST_STRCPY(ErrMsg, "Expression expected");

                    if (ErrOffset)
                        *ErrOffset = CharacterOffset + 1;

                    return FALSE;
                }

                if (p[0] == '[') /* dereference */
                {
                    ASSERT(MemorySize == 1 || MemorySize == 2 ||
                           MemorySize == 4 || MemorySize == 8);

                    if (pend[0] != ']')
                    {
                        CONST_STRCPY(ErrMsg, "']' expected");

                        if (ErrOffset)
                            *ErrOffset = CharacterOffset + (pend - p);

                        return FALSE;
                    }

                    RpnOp.Type = RpnOpDereference;
                    RpnOp.CharacterOffset = CharacterOffset;
                    RpnOp.Data.DerefMemorySize = MemorySize;

                    if (!RpnpPushStack(Stack, &RpnOp))
                    {
                        CONST_STRCPY(ErrMsg, "RPN op stack overflow");

                        if (ErrOffset)
                            *ErrOffset = -1;

                        return FALSE;
                    }
                }
                else /* p[0] == '(' */
                {
                    if (pend[0] != ')')
                    {
                        CONST_STRCPY(ErrMsg, "')' expected");

                        if (ErrOffset)
                            *ErrOffset = CharacterOffset + (pend - p);

                        return FALSE;
                    }
                }

                /* Push a "nop" to prevent popping of the + operator (which would
                 * result in (10+10)/2 beeing evaluated as 15)
                 */
                RpnOp.Type = RpnOpNop;
                if (!RpnpPushStack(Stack, &RpnOp))
                {
                    CONST_STRCPY(ErrMsg, "RPN op stack overflow");

                    if (ErrOffset)
                        *ErrOffset = -1;

                    return FALSE;
                }

                /* Skip closing brace/bracket */
                pend++;

                CharacterOffset += pend - p;
                p = pend;
            }
            else if (First && p[0] == '-') /* Allow expressions like "- eax" */
            {
                RpnOp.Type = RpnOpImmediate;
                RpnOp.CharacterOffset = CharacterOffset;
                RpnOp.Data.Immediate = 0;

                if (!RpnpPushStack(Stack, &RpnOp))
                {
                    CONST_STRCPY(ErrMsg, "RPN op stack overflow");

                    if (ErrOffset)
                        *ErrOffset = -1;

                    return FALSE;
                }
            }
            else
            {
                CONST_STRCPY(ErrMsg, "Operand expected");

                if (ErrOffset)
                    *ErrOffset = CharacterOffset;

                return FALSE;
            }
        }
        else
        {
            CONST_STRCPY(ErrMsg, "strcspn() failed");

            if (ErrOffset)
                *ErrOffset = -1;

            return FALSE;
        }

        if (!First)
        {
            /* Push operator */
            RpnOp.CharacterOffset = OperatorOffset;
            RpnOp.Type = RpnOpBinaryOperator;
            IsComparativeOp = FALSE;

            switch (*Operator)
            {
                case '+':
                    RpnOp.Data.BinaryOperator = RpnBinaryOperatorAdd;
                    break;

                case '-':
                    RpnOp.Data.BinaryOperator = RpnBinaryOperatorSub;
                    break;

                case '*':
                    RpnOp.Data.BinaryOperator = RpnBinaryOperatorMul;
                    break;

                case '/':
                    RpnOp.Data.BinaryOperator = RpnBinaryOperatorDiv;
                    break;

                case '%':
                    RpnOp.Data.BinaryOperator = RpnBinaryOperatorMod;
                    break;

                case '=':
                    ASSERT(Operator[1] == '=');
                    IsComparativeOp = TRUE;
                    RpnOp.Data.BinaryOperator = RpnBinaryOperatorEquals;
                    break;

                case '!':
                    ASSERT(Operator[1] == '=');
                    IsComparativeOp = TRUE;
                    RpnOp.Data.BinaryOperator = RpnBinaryOperatorNotEquals;
                    break;

                case '<':
                    IsComparativeOp = TRUE;

                    if (Operator[1] == '=')
                        RpnOp.Data.BinaryOperator = RpnBinaryOperatorLessThanOrEquals;
                    else
                        RpnOp.Data.BinaryOperator = RpnBinaryOperatorLessThan;

                    break;

                case '>':
                    IsComparativeOp = TRUE;

                    if (Operator[1] == '=')
                        RpnOp.Data.BinaryOperator = RpnBinaryOperatorGreaterThanOrEquals;
                    else
                        RpnOp.Data.BinaryOperator = RpnBinaryOperatorGreaterThan;

                    break;

                default:
                    ASSERT(0);
                    break;
            }

            if (IsComparativeOp)
            {
                if (ComparativeOpFilled && !RpnpPushStack(Stack, &ComparativeOp))
                {
                    CONST_STRCPY(ErrMsg, "RPN op stack overflow");

                    if (ErrOffset)
                        *ErrOffset = -1;

                    return FALSE;
                }

                memcpy(&ComparativeOp, &RpnOp, sizeof(RPN_OP));
                ComparativeOpFilled = TRUE;
            }
            else if (!RpnpPushStack(Stack, &RpnOp))
            {
                CONST_STRCPY(ErrMsg, "RPN op stack overflow");

                if (ErrOffset)
                    *ErrOffset = -1;

                return FALSE;
            }

            /* Push popped operator */
            if (HavePoppedOperator)
            {
                if (!RpnpPushStack(Stack, &PoppedOperator))
                {
                    CONST_STRCPY(ErrMsg, "RPN op stack overflow");

                    if (ErrOffset)
                        *ErrOffset = -1;

                    return FALSE;
                }
            }
        }

        First = FALSE;
    }

//end_of_expression:

    if (ComparativeOpFilled && !RpnpPushStack(Stack, &ComparativeOp))
    {
        CONST_STRCPY(ErrMsg, "RPN op stack overflow");

        if (ErrOffset)
            *ErrOffset = -1;

        return FALSE;
    }

    /* Skip whitespace */
    while (isspace(*p))
    {
        p++;
        CharacterOffset++;
    }

    if (End)
        *End = p;

    return TRUE;
}

/*!\brief Evaluates the RPN op stack and returns the result.
 *
 * \param Stack      Pointer to a RPN_STACK structure.
 * \param TrapFrame  Register values.
 * \param Result     Pointer to an ULONG to store the result into.
 * \param ErrOffset  On failure this is set to the character offset at which the error occoured.
 * \param ErrMsg     Buffer which receives an error message on failure (128 bytes)
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 */
static BOOLEAN
RpnpEvaluateStack(
    IN  PRPN_STACK Stack,
    IN  PKDB_KTRAP_FRAME TrapFrame,
    OUT PULONGLONG Result,
    OUT PLONG ErrOffset  OPTIONAL,
    OUT PCHAR ErrMsg  OPTIONAL)
{
    ULONGLONG ValueStack[RPN_VALUE_STACK_SIZE];
    ULONG ValueStackPointer = 0;
    ULONG index;
    ULONGLONG ull;
    ULONG ul;
    USHORT us;
    UCHAR uc;
    PVOID p;
    BOOLEAN Ok;
#ifdef DEBUG_RPN
    ULONG ValueStackPointerMax = 0;
#endif

    ASSERT(Stack);
    ASSERT(TrapFrame);
    ASSERT(Result);

    for (index = 0; index < Stack->Sp; index++)
    {
        PRPN_OP Op = Stack->Ops + index;

#ifdef DEBUG_RPN
        ValueStackPointerMax = max(ValueStackPointerMax, ValueStackPointer);
#endif

        switch (Op->Type)
        {
            case RpnOpNop:
                /* No operation */
                break;

            case RpnOpImmediate:
                if (ValueStackPointer == RPN_VALUE_STACK_SIZE)
                {
                    CONST_STRCPY(ErrMsg, "Value stack overflow");

                    if (ErrOffset)
                        *ErrOffset = -1;

                    return FALSE;
                }

                ValueStack[ValueStackPointer++] = Op->Data.Immediate;
                break;

            case RpnOpRegister:
                if (ValueStackPointer == RPN_VALUE_STACK_SIZE)
                {
                    CONST_STRCPY(ErrMsg, "Value stack overflow");

                    if (ErrOffset)
                        *ErrOffset = -1;

                    return FALSE;
                }

                ul = Op->Data.Register;
                p = (PVOID)((ULONG_PTR)TrapFrame + RegisterToTrapFrame[ul].Offset);

                switch (RegisterToTrapFrame[ul].Size)
                {
                    case 1: ull = (ULONGLONG)(*(PUCHAR)p); break;
                    case 2: ull = (ULONGLONG)(*(PUSHORT)p); break;
                    case 4: ull = (ULONGLONG)(*(PULONG)p); break;
                    case 8: ull = (ULONGLONG)(*(PULONGLONG)p); break;
                    default: ASSERT(0); return FALSE; break;
                }

                ValueStack[ValueStackPointer++] = ull;
                break;

            case RpnOpDereference:
                if (ValueStackPointer < 1)
                {
                    CONST_STRCPY(ErrMsg, "Value stack underflow");

                    if (ErrOffset)
                        *ErrOffset = -1;

                    return FALSE;
                }

                /* FIXME: Print a warning when address is out of range */
                p = (PVOID)(ULONG_PTR)ValueStack[ValueStackPointer - 1];
                Ok = FALSE;

                switch (Op->Data.DerefMemorySize)
                {
                    case 1:
                        if (NT_SUCCESS(KdbpSafeReadMemory(&uc, p, sizeof (uc))))
                        {
                            Ok = TRUE;
                            ull = (ULONGLONG)uc;
                        }
                        break;

                    case 2:
                        if (NT_SUCCESS(KdbpSafeReadMemory(&us, p, sizeof (us))))
                        {
                            Ok = TRUE;
                            ull = (ULONGLONG)us;
                        }
                        break;

                    case 4:
                        if (NT_SUCCESS(KdbpSafeReadMemory(&ul, p, sizeof (ul))))
                        {
                            Ok = TRUE;
                            ull = (ULONGLONG)ul;
                        }
                        break;

                    case 8:
                        if (NT_SUCCESS(KdbpSafeReadMemory(&ull, p, sizeof (ull))))
                        {
                            Ok = TRUE;
                        }
                        break;

                    default:
                        ASSERT(0);
                        return FALSE;
                        break;
                }

                if (!Ok)
                {
                    _snprintf(ErrMsg, 128, "Couldn't access memory at 0x%lx", (ULONG)p);

                    if (ErrOffset)
                        *ErrOffset = Op->CharacterOffset;

                    return FALSE;
                }

                ValueStack[ValueStackPointer - 1] = ull;
                break;

            case RpnOpBinaryOperator:
                if (ValueStackPointer < 2)
                {
                    CONST_STRCPY(ErrMsg, "Value stack underflow");

                    if (ErrOffset)
                        *ErrOffset = -1;

                    return FALSE;
                }

                ValueStackPointer--;
                ull = ValueStack[ValueStackPointer];

                if (ull == 0 && Op->Data.BinaryOperator == RpnBinaryOperatorDiv)
                {
                    CONST_STRCPY(ErrMsg, "Division by zero");

                    if (ErrOffset)
                        *ErrOffset = Op->CharacterOffset;

                    return FALSE;
                }

                ull = Op->Data.BinaryOperator(ValueStack[ValueStackPointer - 1], ull);
                ValueStack[ValueStackPointer - 1] = ull;
                break;

            default:
                ASSERT(0);
                return FALSE;
        }
    }

#ifdef DEBUG_RPN
    DPRINT1("Max value stack pointer: %d\n", ValueStackPointerMax);
#endif

    if (ValueStackPointer != 1)
    {
        CONST_STRCPY(ErrMsg, "Stack not empty after evaluation");

        if (ErrOffset)
            *ErrOffset = -1;

        return FALSE;
    }

    *Result = ValueStack[0];
    return TRUE;
}

/*!\brief Evaluates the given expression
 *
 * \param Expression  Expression to evaluate.
 * \param TrapFrame   Register values.
 * \param Result      Variable which receives the result on success.
 * \param ErrOffset   Variable which receives character offset on parse error (-1 on other errors)
 * \param ErrMsg      Buffer which receives an error message on failure (128 bytes)
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 */
BOOLEAN
KdbpRpnEvaluateExpression(
    IN  PCHAR Expression,
    IN  PKDB_KTRAP_FRAME TrapFrame,
    OUT PULONGLONG Result,
    OUT PLONG ErrOffset  OPTIONAL,
    OUT PCHAR ErrMsg  OPTIONAL)
{
    PRPN_STACK Stack = (PRPN_STACK)&RpnStack;

    ASSERT(Expression);
    ASSERT(TrapFrame);
    ASSERT(Result);

    /* Clear the stack and parse the expression */
    RpnpClearStack(Stack);
    if (!RpnpParseExpression(Stack, Expression, NULL, 0, ErrOffset, ErrMsg))
        return FALSE;

#ifdef DEBUG_RPN
    RpnpDumpStack(Stack);
#endif

    /* Evaluate the stack */
    if (!RpnpEvaluateStack(Stack, TrapFrame, Result, ErrOffset, ErrMsg))
        return FALSE;

    return TRUE;
}

/*!\brief Parses the given expression and returns a "handle" to it.
 *
 * \param Expression  Expression to evaluate.
 * \param ErrOffset   Variable which receives character offset on parse error (-1 on other errors)
 * \param ErrMsg      Buffer which receives an error message on failure (128 bytes)
 *
 * \returns "Handle" for the expression, NULL on failure.
 *
 * \sa KdbpRpnEvaluateExpression
 */
PVOID
KdbpRpnParseExpression(
    IN  PCHAR Expression,
    OUT PLONG ErrOffset  OPTIONAL,
    OUT PCHAR ErrMsg  OPTIONAL)
{
    LONG Size;
    PRPN_STACK Stack = (PRPN_STACK)&RpnStack;
    PRPN_STACK NewStack;

    ASSERT(Expression);

    /* Clear the stack and parse the expression */
    RpnpClearStack(Stack);
    if (!RpnpParseExpression(Stack, Expression, NULL, 0, ErrOffset, ErrMsg))
        return FALSE;

#ifdef DEBUG_RPN
    RpnpDumpStack(Stack);
#endif

    /* Duplicate the stack and return a pointer/handle to it */
    ASSERT(Stack->Sp >= 1);
    Size = sizeof (RPN_STACK) + (RTL_FIELD_SIZE(RPN_STACK, Ops[0]) * (Stack->Sp - 1));
    NewStack = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_KDBG);

    if (!NewStack)
    {
        CONST_STRCPY(ErrMsg, "Out of memory");

        if (ErrOffset)
            *ErrOffset = -1;

        return NULL;
    }

    memcpy(NewStack, Stack, Size);
    NewStack->Size = NewStack->Sp;

    return NewStack;
}

/*!\brief Evaluates the given expression and returns the result.
 *
 * \param Expression  Expression "handle" returned by KdbpRpnParseExpression.
 * \param TrapFrame   Register values.
 * \param Result      Variable which receives the result on success.
 * \param ErrOffset   Variable which receives character offset on parse error (-1 on other errors)
 * \param ErrMsg      Buffer which receives an error message on failure (128 bytes)
 *
 * \returns "Handle" for the expression, NULL on failure.
 *
 * \sa KdbpRpnParseExpression
 */
BOOLEAN
KdbpRpnEvaluateParsedExpression(
    IN  PVOID Expression,
    IN  PKDB_KTRAP_FRAME TrapFrame,
    OUT PULONGLONG Result,
    OUT PLONG ErrOffset  OPTIONAL,
    OUT PCHAR ErrMsg  OPTIONAL)
{
    PRPN_STACK Stack = (PRPN_STACK)Expression;

    ASSERT(Expression);
    ASSERT(TrapFrame);
    ASSERT(Result);

    /* Evaluate the stack */
    return RpnpEvaluateStack(Stack, TrapFrame, Result, ErrOffset, ErrMsg);
}

