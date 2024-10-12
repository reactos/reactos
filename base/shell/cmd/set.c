/*
 *  SET.C - set internal command.
 *
 *
 *  History:
 *
 *    06/14/97 (Tim Norman)
 *        changed static var in set() to a cmd_alloc'd space to pass to putenv.
 *        need to find a better way to do this, since it seems it is wasting
 *        memory when variables are redefined.
 *
 *    07/08/1998 (John P. Price)
 *        removed call to show_environment in set command.
 *        moved test for syntax before allocating memory in set command.
 *        misc clean up and optimization.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    28-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added set_env function to set env. variable without needing set command
 *
 *    09-Dec-1998 (Eric Kohl)
 *        Added help text ("/?").
 *
 *    24-Jan-1999 (Eric Kohl)
 *        Fixed Win32 environment handling.
 *        Unicode and redirection safe!
 *
 *    25-Feb-1999 (Eric Kohl)
 *        Fixed little bug.
 *
 *    30-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_SET

/* Initial size of environment variable buffer */
#define ENV_BUFFER_SIZE  1024

static BOOL
seta_eval(LPCTSTR expr);

static LPCTSTR
skip_ws(LPCTSTR p)
{
    while (*p && *p <= _T(' '))
        ++p;
    return p;
}

/* Used to check for and handle:
 * SET "var=value", SET /P "var=prompt", and SET /P var="prompt" */
static LPTSTR
GetQuotedString(TCHAR *p)
{
    TCHAR *end;
    if (*p == _T('"'))
    {
        p = (LPTSTR)skip_ws(p + 1);
        /* If a matching quote is found, truncate the string */
        end = _tcsrchr(p, _T('"'));
        if (end)
            *end = _T('\0');
    }
    return p;
}

INT cmd_set(LPTSTR param)
{
    INT retval = 0;
    LPTSTR p;
    LPTSTR lpEnv;
    LPTSTR lpOutput;

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_SET_HELP);
        return 0;
    }

    param = (LPTSTR)skip_ws(param);

    /* If no parameters, show the environment */
    if (param[0] == _T('\0'))
    {
        lpEnv = (LPTSTR)GetEnvironmentStrings();
        if (lpEnv)
        {
            lpOutput = lpEnv;
            while (*lpOutput)
            {
                /* Do not display the special '=X:' environment variables */
                if (*lpOutput != _T('='))
                {
                    ConOutPuts(lpOutput);
                    ConOutChar(_T('\n'));
                }
                lpOutput += _tcslen(lpOutput) + 1;
            }
            FreeEnvironmentStrings(lpEnv);
        }

        retval = 0;
        goto Quit;
    }

    /* The /A does *NOT* have to be followed by a whitespace */
    if (!_tcsnicmp(param, _T("/A"), 2))
    {
        BOOL Success;

        /* Save error level since seta_eval() modifies it, as
         * we need to set it later according to specific rules. */
        INT nOldErrorLevel = nErrorLevel;

        StripQuotes(param);
        Success = seta_eval(skip_ws(param + 2));
        if (!Success)
        {
#if 0
            /* Might seem random but this is what windows xp does -- This is a message ID */
            retval = 9165;
#endif
            retval = nErrorLevel;
            nErrorLevel = nOldErrorLevel;
        }
        else
        {
            retval = 0;
        }
        goto Quit;
    }

    if (!_tcsnicmp(param, _T("/P"), 2))
    {
        TCHAR value[1023];
        param = GetQuotedString((LPTSTR)skip_ws(param + 2));
        p = _tcschr(param, _T('='));
        if (!p)
        {
            ConErrResPuts(STRING_SYNTAX_COMMAND_INCORRECT);
            retval = 1;
            goto Quit;
        }

        *p++ = _T('\0');
        ConOutPrintf(_T("%s"), GetQuotedString(p));
        ConInString(value, ARRAYSIZE(value));

        if (!*value || !SetEnvironmentVariable(param, value))
        {
            retval = 1;
            goto Quit;
        }
        retval = 0;
        goto Quit;
    }

    param = GetQuotedString(param);

    p = _tcschr(param, _T('='));
    if (p)
    {
        /* Set or remove the environment variable */
        if (p == param)
        {
            /* Handle set =val case */
            ConErrResPuts(STRING_SYNTAX_COMMAND_INCORRECT);
            retval = 1;
            goto Quit;
        }

        *p++ = _T('\0');

#ifdef FEATURE_DYNAMIC_TRACE
        /* Check for dynamic TRACE ON/OFF */
        if (!_tcsicmp(param, _T("CMDTRACE")))
            g_bDynamicTrace = !_tcsicmp(p, _T("ON"));
#endif

        if (!SetEnvironmentVariable(param, *p ? p : NULL))
        {
            retval = 1;
            goto Quit;
        }
    }
    else
    {
        /* Display all the environment variables with the given prefix */
        LPTSTR pOrgParam = param;
        BOOLEAN bFound = FALSE;
        BOOLEAN bRestoreSpace;

        /*
         * Trim the prefix from "special" characters (only when displaying the
         * environment variables), so that e.g. "SET ,; ,;FOO" will display all
         * the variables starting by "FOO".
         * The SET command allows as well to set an environment variable whose name
         * actually contains these characters (e.g. "SET ,; ,;FOO=42"); however,
         * by trimming the characters, doing "SET ,; ,;FOO" would not allow seeing
         * such variables.
         * Thus, we also save a pointer to the original variable name prefix, that
         * we will look it up as well below.
         */
        while (_istspace(*param) || *param == _T(',') || *param == _T(';'))
            ++param;

        /* Just remove the very last space, if present */
        p = _tcsrchr(param, _T(' '));
        bRestoreSpace = (p != NULL);
        if (!p)
            p = param + _tcslen(param);
        *p = _T('\0');

        lpEnv = GetEnvironmentStrings();
        if (lpEnv)
        {
            lpOutput = lpEnv;
            while (*lpOutput)
            {
                /* Look up for both the original and truncated variable name prefix */
                if (!_tcsnicmp(lpOutput, pOrgParam, p - pOrgParam) ||
                    !_tcsnicmp(lpOutput, param, p - param))
                {
                    ConOutPuts(lpOutput);
                    ConOutChar(_T('\n'));
                    bFound = TRUE;
                }
                lpOutput += _tcslen(lpOutput) + 1;
            }
            FreeEnvironmentStrings(lpEnv);
        }

        /* Restore the truncated space for correctly
         * displaying the error message, if any. */
        if (bRestoreSpace)
            *p = _T(' ');

        if (!bFound)
        {
            ConErrResPrintf(STRING_SET_ENV_ERROR, param);
            retval = 1;
            goto Quit;
        }
    }

Quit:
    if (BatType != CMD_TYPE)
    {
        if (retval != 0)
            nErrorLevel = retval;
    }
    else
    {
        nErrorLevel = retval;
    }

    return retval;
}

static INT
ident_len(LPCTSTR p)
{
    LPCTSTR p2 = p;
    if (__iscsymf(*p))
    {
        ++p2;
        while (__iscsym(*p2))
            ++p2;
    }
    return (INT)(p2-p);
}

#define PARSE_IDENT(ident, identlen, p) \
do { \
    identlen = ident_len(p); \
    ident = (LPTSTR)_alloca((identlen + 1) * sizeof(TCHAR)); \
    memmove(ident, p, identlen * sizeof(TCHAR)); \
    ident[identlen] = 0; \
    p += identlen; \
} while (0)

static INT
seta_identval(LPCTSTR ident)
{
    LPCTSTR identVal = GetEnvVarOrSpecial(ident);
    if (!identVal)
        return 0;
    else
        return _tcstol(identVal, NULL, 0);
}

static BOOL
calc(INT* lval, TCHAR op, INT rval)
{
    switch (op)
    {
    case '*':
        *lval *= rval;
        break;

    case '/':
    {
        if (rval == 0)
        {
            ConErrResPuts(STRING_ERROR_DIVISION_BY_ZERO);
            nErrorLevel = 0x400023D1; // 1073750993;
            return FALSE;
        }
        *lval /= rval;
        break;
    }

    case '%':
    {
        if (rval == 0)
        {
            ConErrResPuts(STRING_ERROR_DIVISION_BY_ZERO);
            nErrorLevel = 0x400023D1; // 1073750993;
            return FALSE;
        }
        *lval %= rval;
        break;
    }

    case '+':
        *lval += rval;
        break;
    case '-':
        *lval -= rval;
        break;
    case '&':
        *lval &= rval;
        break;
    case '^':
        *lval ^= rval;
        break;
    case '|':
        *lval |= rval;
        break;

    default:
        ConErrResPuts(STRING_INVALID_OPERAND);
        nErrorLevel = 0x400023CE; // 1073750990;
        return FALSE;
    }
    return TRUE;
}

static BOOL
seta_stmt(LPCTSTR* p_, INT* result);

static BOOL
seta_unaryTerm(LPCTSTR* p_, INT* result)
{
    LPCTSTR p = *p_;
    INT rval;

    if (*p == _T('('))
    {
        p = skip_ws(p + 1);
        if (!seta_stmt(&p, &rval))
            return FALSE;
        if (*p++ != _T(')'))
        {
            ConErrResPuts(STRING_EXPECTED_CLOSE_PAREN);
            nErrorLevel = 0x400023CC; // 1073750988;
            return FALSE;
        }
        *result = rval;
    }
    else if (_istdigit(*p))
    {
        errno = 0;
        rval = _tcstol(p, (LPTSTR*)&p, 0);

        /* Check for overflow / underflow */
        if (errno == ERANGE)
        {
            ConErrResPuts(STRING_ERROR_INVALID_NUMBER2);
            nErrorLevel = 0x400023D0; // 1073750992;
            return FALSE;
        }
        /*
         * _tcstol() stopped at the first non-digit character. If it's not a whitespace,
         * or if it's the start of a possible identifier, this means the number being
         * interpreted was invalid.
         */
        else if (*p && !_istspace(*p) && __iscsymf(*p))
        {
            ConErrResPuts(STRING_ERROR_INVALID_NUMBER1);
            nErrorLevel = 0x400023CF; // 1073750991;
            return FALSE;
        }
        *result = rval;
    }
    else if (__iscsymf(*p))
    {
        LPTSTR ident;
        INT identlen;
        PARSE_IDENT(ident, identlen, p);
        *result = seta_identval(ident);
    }
    else
    {
        ConErrResPuts(STRING_EXPECTED_NUMBER_OR_VARIABLE);
        nErrorLevel = 0x400023CD; // 1073750989;
        return FALSE;
    }
    *p_ = skip_ws(p);
    return TRUE;
}

static BOOL
seta_mulTerm(LPCTSTR* p_, INT* result)
{
    LPCTSTR p = *p_;
    TCHAR op = 0;
    INT rval;

    if (_tcschr(_T("!~-+"), *p))
    {
        op = *p;
        p = skip_ws(p + 1);

        if (!seta_mulTerm(&p, &rval))
            return FALSE;

        switch (op)
        {
        case '!':
            rval = !rval;
            break;
        case '~':
            rval = ~rval;
            break;
        case '-':
            rval = -rval;
            break;
#if 0
        case '+':
            rval = rval;
            break;
#endif
        }
    }
    else
    {
        if (!seta_unaryTerm(&p, &rval))
            return FALSE;
    }

    *result = rval;
    *p_ = p;
    return TRUE;
}

static BOOL
seta_ltorTerm(LPCTSTR* p_, INT* result, LPCTSTR ops, BOOL (*subTerm)(LPCTSTR*,INT*))
{
    LPCTSTR p = *p_;
    INT lval;

    /* Evaluate the left-hand side */
    if (!subTerm(&p, &lval))
        return FALSE;

    while (*p && _tcschr(ops, *p))
    {
        INT rval;
        TCHAR op = *p;

        p = skip_ws(p + 1);

        /* Evaluate the immediate right-hand side */
        if (!subTerm(&p, &rval))
            return FALSE;

        /* This becomes the new left-hand side for the next iteration */
        if (!calc(&lval, op, rval))
            return FALSE;
    }

    *result = lval;
    *p_ = p;
    return TRUE;
}

static BOOL
seta_addTerm(LPCTSTR* p_, INT* result)
{
    return seta_ltorTerm(p_, result, _T("*/%"), seta_mulTerm);
}

static BOOL
seta_logShiftTerm(LPCTSTR* p_, INT* result)
{
    return seta_ltorTerm(p_, result, _T("+-"), seta_addTerm);
}

static BOOL
seta_bitAndTerm(LPCTSTR* p_, INT* result)
{
    LPCTSTR p = *p_;
    INT lval;

    /* Evaluate the left-hand side */
    if (!seta_logShiftTerm(&p, &lval))
        return FALSE;

    /* Handle << >> operators */
    while (*p && _tcschr(_T("<>"), *p))
    {
        INT rval;
        TCHAR op = *p;

        /* Check whether the next non-whitespace character is the same operator */
        p = skip_ws(p + 1);
        if (*p != op)
            break;

        /* Skip it */
        p = skip_ws(p + 1);

        /* Evaluate the immediate right-hand side */
        if (!seta_logShiftTerm(&p, &rval))
            return FALSE;

        /* This becomes the new left-hand side for the next iteration */
        switch (op)
        {
        case '<':
        {
            /* Shift left has to be a positive number, 0-31 otherwise 0 is returned,
             * which differs from the compiler (for example gcc) so being explicit. */
            if (rval < 0 || rval >= (8 * sizeof(lval)))
                lval = 0;
            else
                lval <<= rval;
            break;
        }

        case '>':
            lval >>= rval;
            break;

        default:
            ConErrResPuts(STRING_INVALID_OPERAND);
            nErrorLevel = 0x400023CE; // 1073750990;
            return FALSE;
        }
    }

    *result = lval;
    *p_ = p;
    return TRUE;
}

static BOOL
seta_bitExclOrTerm(LPCTSTR* p_, INT* result)
{
    return seta_ltorTerm(p_, result, _T("&"), seta_bitAndTerm);
}

static BOOL
seta_bitOrTerm(LPCTSTR* p_, INT* result)
{
    return seta_ltorTerm(p_, result, _T("^"), seta_bitExclOrTerm);
}

static BOOL
seta_expr(LPCTSTR* p_, INT* result)
{
    return seta_ltorTerm(p_, result, _T("|"), seta_bitOrTerm);
}

static BOOL
seta_assignment(LPCTSTR* p_, INT* result)
{
    LPCTSTR p = *p_;
    LPTSTR ident;
    TCHAR op = 0;
    INT identlen, exprval;

    PARSE_IDENT(ident, identlen, p);
    if (identlen)
    {
        p = skip_ws(p);

        /* Handle = assignment */
        if (*p == _T('='))
        {
            op = *p;
            p = skip_ws(p + 1);
        }
        /* Handle *= /= %= += -= &= ^= |= assignments */
        else if (_tcschr(_T("*/%+-&^|"), *p))
        {
            op = *p;

            /* Find the '=', there may be some spaces before it */
            p = skip_ws(p + 1);
            if (*p != _T('='))
            {
                op = 0;
                goto evaluate;
            }

            /* Skip it */
            p = skip_ws(p + 1);
        }
        /* Handle <<= >>= assignments */
        else if (_tcschr(_T("<>"), *p))
        {
            op = *p;

            /* Check whether the next non-whitespace character is the same operator */
            p = skip_ws(p + 1);
            if (*p != op)
            {
                op = 0;
                goto evaluate;
            }

            /* Find the '=', there may be some spaces before it */
            p = skip_ws(p + 1);
            if (*p != _T('='))
            {
                op = 0;
                goto evaluate;
            }

            /* Skip it */
            p = skip_ws(p + 1);
        }
    }

evaluate:
    /* Allow to chain multiple assignments, such as: a=b=1 */
    if (ident && op)
    {
        INT identval;
        LPTSTR buf;

        if (!seta_assignment(&p, &exprval))
            return FALSE;

        identval = seta_identval(ident);

        switch (op)
        {
        /* Handle = assignment */
        case '=':
            identval = exprval;
            break;

        /* Handle <<= assignment */
        case '<':
        {
            /* Shift left has to be a positive number, 0-31 otherwise 0 is returned,
             * which differs from the compiler (for example gcc) so being explicit. */
            if (exprval < 0 || exprval >= (8 * sizeof(identval)))
                identval = 0;
            else
                identval <<= exprval;
            break;
        }

        /* Handle >>= assignment */
        case '>':
            identval >>= exprval;
            break;

        /* Other assignments */
        default:
            if (!calc(&identval, op, exprval))
                return FALSE;
        }

        buf = (LPTSTR)_alloca(32 * sizeof(TCHAR));
        _sntprintf(buf, 32, _T("%i"), identval);
        SetEnvironmentVariable(ident, buf); // TODO FIXME - check return value
        exprval = identval;
    }
    else
    {
        /* Restore p in case we found an identifier but not an operator */
        p = *p_;
        if (!seta_expr(&p, &exprval))
            return FALSE;
    }

    *result = exprval;
    *p_ = p;
    return TRUE;
}

static BOOL
seta_stmt(LPCTSTR* p_, INT* result)
{
    LPCTSTR p = *p_;
    INT rval;

    if (!seta_assignment(&p, &rval))
        return FALSE;

    /* Loop over each statement */
    while (*p == _T(','))
    {
        p = skip_ws(p + 1);

        if (!seta_assignment(&p, &rval))
            return FALSE;
    }

    *result = rval;
    *p_ = p;
    return TRUE;
}

static BOOL
seta_eval(LPCTSTR p)
{
    INT rval;

    if (!*p)
    {
        ConErrResPuts(STRING_SYNTAX_COMMAND_INCORRECT);
        nErrorLevel = 1;
        return FALSE;
    }
    if (!seta_stmt(&p, &rval))
        return FALSE;

    /* If unparsed data remains, fail and bail out */
    if (*p)
    {
        ConErrResPuts(STRING_SYNTAX_COMMAND_INCORRECT); // Actually syntax error / missing operand.
        nErrorLevel = 0x400023CE; // 1073750990;
        return FALSE;
    }

    /* Echo the result of the evaluation only in interactive (non-batch) mode */
    if (!bc)
        ConOutPrintf(_T("%i"), rval);

    return TRUE;
}

#endif
