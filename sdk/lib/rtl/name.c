/*
 * PROJECT:     ReactOS System Libraries
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     RtlIsNameInExpression implementation
 * COPYRIGHT:   Copyright 2026 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include <rtl.h>
#define NDEBUG
#include <debug.h>

#define DOS_STAR  (L'<')
#define DOS_QM    (L'>')
#define DOS_DOT   (L'"')

#define EXPRESSION_IS_WILDCARD(expr, len) \
    (len == 1 && expr->Buffer[0] == L'*') \
    || (len == 3 && expr->Buffer[0] == L'*' && expr->Buffer[1] == L'.' && expr->Buffer[2] == L'*')
#define IS_LITERAL_OR_DOS_DOT(wchar) \
    (wchar == DOS_DOT || (wchar != L'*' && wchar != DOS_STAR && wchar != L'?' && wchar != DOS_QM))

/* ****** PRIVATE FUNCTIONS ****** */

static __inline BOOLEAN
RtlpContainsWildcard(
    _In_ PWCH Buffer,
    _In_ ULONG Length)
{
    for (ULONG i = 0; i < Length; i++)
    {
        WCHAR c = Buffer[i];
        if (c == L'*' || c == L'?' || c == DOS_STAR || c == DOS_QM || c == DOS_DOT)
            return TRUE;
    }
    return FALSE;
}

static __inline WCHAR
RtlpGetExprChar(
    _In_     PWCH    Buffer,
    _In_     ULONG   Position,
    _In_     ULONG   Length,
    _In_     BOOLEAN IgnoreCase,
    _In_opt_ PWCH    UpcaseTable)
{
    WCHAR Result = (Position < Length) ? Buffer[Position] : L'\0';
    if (IgnoreCase)
        Result = UpcaseTable ? UpcaseTable[Result] : RtlUpcaseUnicodeChar(Result);
    return Result;
}

/* ****** PUBLIC FUNCTIONS ****** */

BOOLEAN
NTAPI
RtlIsNameInExpression(
    _In_     PUNICODE_STRING Expression,
    _In_     PUNICODE_STRING Name,
    _In_     BOOLEAN         IgnoreCase,
    _In_opt_ PWCH            UpcaseTable
)
{
    ULONG NameLen = Name->Length / sizeof(WCHAR);
    ULONG ExprLen = Expression->Length / sizeof(WCHAR);
    ULONG NamePos = 0, ExprPos = 0;
    ULONG StarExprPos = MAXULONG;
    ULONG StarNamePos = 0;

    /* For more information about this algorithm, see
     * https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/0b034646-4e23-4874-8488-2adac231ff23
     */

    /* *** Optimizations *** */

    /* Empty string */
    if (ExprLen == 0 || NameLen == 0)
        return (ExprLen == 0 && NameLen == 0);

    /* Wildcard only */
    if (EXPRESSION_IS_WILDCARD(Expression, ExprLen))
        return TRUE;

    /* Wildcard + literal with no other wildcards */
    if (ExprLen >= 2
        && NameLen >= ExprLen - 1
        && Expression->Buffer[0] == L'*'
        && !RtlpContainsWildcard(&Expression->Buffer[1], ExprLen - 1))
    {
        ExprPos = 1;
        NamePos = NameLen - (ExprLen - 1);

        while (ExprPos < ExprLen)
        {
            WCHAR NameChar = RtlpGetExprChar(Name->Buffer, NamePos, NameLen, IgnoreCase, UpcaseTable);
            WCHAR ExprChar = RtlpGetExprChar(Expression->Buffer, ExprPos, ExprLen, IgnoreCase, UpcaseTable);
            if (ExprChar != NameChar)
                return FALSE;
            NamePos++;
            ExprPos++;
        }

        return TRUE;
    }

    /* *** Main parser *** */

    for (;;)
    {
        /* We passed everything and reached the end of both the name and expression. */
        if (NamePos >= NameLen && ExprPos >= ExprLen)
            return TRUE;

        WCHAR NameChar = RtlpGetExprChar(Name->Buffer, NamePos, NameLen, IgnoreCase, UpcaseTable);
        WCHAR ExprChar = RtlpGetExprChar(Expression->Buffer, ExprPos, ExprLen, IgnoreCase, UpcaseTable);

        /* '?' or character literal */
        if ((ExprChar == L'?' && NamePos < NameLen)
            || (IS_LITERAL_OR_DOS_DOT(ExprChar) && ExprChar == NameChar))
        {
            NamePos++;
            ExprPos++;
            continue;
        }

        /* '*' */
        if (ExprChar == L'*')
        {
            StarExprPos = ExprPos;
            StarNamePos = NamePos;
            ExprPos++;
            continue;
        }

        /* DOS_STAR */
        if (ExprChar == DOS_STAR)
        {
            ULONG LookPos = ExprPos + 1;
            StarExprPos = ExprPos;
            StarNamePos = NamePos;

            while (LookPos < ExprLen)
            {
                WCHAR LookChar = Expression->Buffer[LookPos];
                if (IS_LITERAL_OR_DOS_DOT(LookChar))
                    break;
                LookPos++;
            }

            ExprPos++;
            continue;
        }

        /* DOS_QM */
        if (ExprChar == DOS_QM)
        {
            if (NamePos >= NameLen || NameChar == L'.')
            {
                /* Dot or end-of-name: skip all consecutive DOS_QM tokens */
                while (ExprPos < ExprLen && Expression->Buffer[ExprPos] == DOS_QM)
                    ExprPos++;
                continue;
            }

            /* Normal DOS_QM: consume one non-dot character */
            NamePos++;
            ExprPos++;
            continue;
        }

        /* DOS_DOT */
        if (ExprChar == DOS_DOT)
        {
            if (NamePos >= NameLen)
            {
                ExprPos++;
                continue;
            }

            if (NameChar == L'.')
            {
                NamePos++;
                ExprPos++;
                continue;
            }
        }

        /* Backtrack from a '*' or DOS_STAR */
        if (StarExprPos != MAXULONG)
        {
            /* A DOS_STAR at the end cannot consume a trailing '.' in the name */
            if (Expression->Buffer[StarExprPos] == DOS_STAR && ExprPos >= ExprLen)
            {
                ULONG i;
                for (i = StarNamePos; i < NameLen; i++)
                {
                    if (Name->Buffer[i] == L'.')
                        return FALSE;
                }
            }

            if (StarNamePos < NameLen)
            {
                StarNamePos++;
                NamePos = StarNamePos;
                ExprPos = StarExprPos + 1;
                continue;
            }
        }

        return FALSE;
    }
}
