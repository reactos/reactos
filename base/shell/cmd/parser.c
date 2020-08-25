/*
 *  PARSER.C - command parsing.
 */

#include "precomp.h"

/* Enable this define for "buggy" Windows' CMD command echoer compatibility */
#define MSCMD_ECHO_COMMAND_COMPAT

/*
 * Parser debugging support. These flags are global so that their values can be
 * modified at runtime from a debugger. They correspond to the public Windows'
 * cmd!fDumpTokens and cmd!fDumpParse booleans.
 * (Same names are used for compatibility as they are documented online.)
 */
BOOLEAN fDumpTokens = FALSE;
BOOLEAN fDumpParse  = FALSE;

#define C_OP_LOWEST C_MULTI
#define C_OP_HIGHEST C_PIPE
static const TCHAR OpString[][3] = { _T("&"), _T("||"), _T("&&"), _T("|") };

static const TCHAR RedirString[][3] = { _T("<"), _T(">"), _T(">>") };

static const TCHAR *const IfOperatorString[] =
{
    /* Standard */
    _T("errorlevel"),
    _T("exist"),

    /* Extended */
    _T("cmdextversion"),
    _T("defined"),
#define IF_MAX_UNARY IF_DEFINED

    /* Standard */
    _T("=="),

    /* Extended */
    _T("equ"),
    _T("neq"),
    _T("lss"),
    _T("leq"),
    _T("gtr"),
    _T("geq"),
#define IF_MAX_COMPARISON IF_GEQ
};

static BOOL IsSeparator(TCHAR Char)
{
    return _istspace(Char) || (Char && _tcschr(STANDARD_SEPS, Char));
}

enum
{
    TOK_END,
    TOK_NORMAL,
    TOK_OPERATOR,
    TOK_REDIRECTION,
    TOK_BEGIN_BLOCK,
    TOK_END_BLOCK
};

/* Scratch buffer for temporary command substitutions / expansions */
static TCHAR TempBuf[CMDLINE_LENGTH];

/*static*/ BOOL bParseError;
static BOOL bLineContinuations;
/*static*/ TCHAR ParseLine[CMDLINE_LENGTH];
static TCHAR *ParsePos;
static TCHAR CurChar;

static TCHAR CurrentToken[CMDLINE_LENGTH];
static int CurrentTokenType;
static int InsideBlock;

static TCHAR ParseChar(void)
{
    TCHAR Char;

    if (bParseError)
        return (CurChar = 0);

restart:
    /*
     * Although CRs can be injected into a line via an environment
     * variable substitution, the parser ignores them - they won't
     * even separate tokens.
     */
    do
    {
        Char = *ParsePos++;
    }
    while (Char == _T('\r'));

    if (!Char)
    {
        ParsePos--;
        if (bLineContinuations)
        {
            if (!ReadLine(ParseLine, TRUE))
            {
                /* ^C pressed, or line was too long */
                bParseError = TRUE;
            }
            else if (*(ParsePos = ParseLine))
            {
                goto restart;
            }
        }
    }
    return (CurChar = Char);
}

void ParseErrorEx(LPTSTR s)
{
    /* Only display the first error we encounter */
    if (!bParseError)
        error_syntax(s);
    bParseError = TRUE;
}

static void ParseError(void)
{
    ParseErrorEx(CurrentTokenType != TOK_END ? CurrentToken : NULL);
}

/*
 * Yes, cmd has a Lexical Analyzer. Whenever the parser gives an "xxx was
 * unexpected at this time." message, it shows what the last token read was.
 */
static int ParseToken(TCHAR ExtraEnd, TCHAR *Separators)
{
    TCHAR *Out = CurrentToken;
    TCHAR Char;
    int Type;
    BOOL bInQuote = FALSE;

    for (Char = CurChar; Char && Char != _T('\n'); Char = ParseChar())
    {
        bInQuote ^= (Char == _T('"'));
        if (!bInQuote)
        {
            if (Separators != NULL)
            {
                if (_istspace(Char) || _tcschr(Separators, Char))
                {
                    /* Skip leading separators */
                    if (Out == CurrentToken)
                        continue;
                    break;
                }
            }

            /* Check for numbered redirection */
            if ((Char >= _T('0') && Char <= _T('9') &&
                   (ParsePos == &ParseLine[1] || IsSeparator(ParsePos[-2]))
                   && (*ParsePos == _T('<') || *ParsePos == _T('>'))))
            {
                break;
            }

            if (Char == ExtraEnd)
                break;
            if (InsideBlock && Char == _T(')'))
                break;
            if (_tcschr(_T("&|<>"), Char))
                break;

            if (Char == _T('^'))
            {
                Char = ParseChar();
                /* Eat up a \n, allowing line continuation */
                if (Char == _T('\n'))
                    Char = ParseChar();
                /* Next character is a forced literal */
            }
        }
        if (Out == &CurrentToken[CMDLINE_LENGTH - 1])
            break;
        *Out++ = Char;
    }

    /* Check if we got at least one character before reaching a special one.
     * If so, return them and leave the special for the next call. */
    if (Out != CurrentToken)
    {
        Type = TOK_NORMAL;
    }
    else if (Char == _T('('))
    {
        Type = TOK_BEGIN_BLOCK;
        *Out++ = Char;
        ParseChar();
    }
    else if (Char == _T(')'))
    {
        Type = TOK_END_BLOCK;
        *Out++ = Char;
        ParseChar();
    }
    else if (Char == _T('&') || Char == _T('|'))
    {
        Type = TOK_OPERATOR;
        *Out++ = Char;
        Char = ParseChar();
        /* check for && or || */
        if (Char == Out[-1])
        {
            *Out++ = Char;
            ParseChar();
        }
    }
    else if ((Char >= _T('0') && Char <= _T('9'))
             || (Char == _T('<') || Char == _T('>')))
    {
        Type = TOK_REDIRECTION;
        if (Char >= _T('0') && Char <= _T('9'))
        {
            *Out++ = Char;
            Char = ParseChar();
        }
        *Out++ = Char;
        Char = ParseChar();
        if (Char == Out[-1])
        {
            /* Strangely, the tokenizer allows << as well as >>... (it
             * will cause an error when trying to parse it though) */
            *Out++ = Char;
            Char = ParseChar();
        }
        if (Char == _T('&'))
        {
            *Out++ = Char;
            while (IsSeparator(Char = ParseChar()))
                ;
            if (Char >= _T('0') && Char <= _T('9'))
            {
                *Out++ = Char;
                ParseChar();
            }
        }
    }
    else
    {
        Type = TOK_END;
    }
    *Out = _T('\0');

    /* Debugging support */
    if (fDumpTokens)
        ConOutPrintf(_T("ParseToken: (%d) '%s'\n"), Type, CurrentToken);

    return (CurrentTokenType = Type);
}

static BOOL ParseRedirection(REDIRECTION **List)
{
    TCHAR *Tok = CurrentToken;
    BYTE Number;
    REDIR_MODE RedirMode;
    REDIRECTION *Redir;

    if (*Tok >= _T('0') && *Tok <= _T('9'))
        Number = *Tok++ - _T('0');
    else
        Number = *Tok == _T('<') ? 0 : 1;

    if (*Tok++ == _T('<'))
    {
        RedirMode = REDIR_READ;
        if (*Tok == _T('<'))
            goto fail;
    }
    else
    {
        RedirMode = REDIR_WRITE;
        if (*Tok == _T('>'))
        {
            RedirMode = REDIR_APPEND;
            Tok++;
        }
    }

    if (!*Tok)
    {
        /* The file name was not part of this token, so it'll be the next one */
        if (ParseToken(0, STANDARD_SEPS) != TOK_NORMAL)
            goto fail;
        Tok = CurrentToken;
    }

    /* If a redirection for this handle number already exists, delete it */
    while ((Redir = *List))
    {
        if (Redir->Number == Number)
        {
            *List = Redir->Next;
            cmd_free(Redir);
            continue;
        }
        List = &Redir->Next;
    }

    Redir = cmd_alloc(FIELD_OFFSET(REDIRECTION, Filename[_tcslen(Tok) + 1]));
    if (!Redir)
    {
        WARN("Cannot allocate memory for Redir!\n");
        goto fail;
    }
    Redir->Next = NULL;
    Redir->OldHandle = INVALID_HANDLE_VALUE;
    Redir->Number = Number;
    Redir->Mode = RedirMode;
    _tcscpy(Redir->Filename, Tok);
    *List = Redir;
    return TRUE;

fail:
    ParseError();
    FreeRedirection(*List);
    *List = NULL;
    return FALSE;
}

static PARSED_COMMAND *ParseCommandOp(int OpType);

/* Parse a parenthesized block */
static PARSED_COMMAND *ParseBlock(REDIRECTION *RedirList)
{
    PARSED_COMMAND *Cmd, *Sub, **NextPtr;

    Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        FreeRedirection(RedirList);
        return NULL;
    }
    Cmd->Type = C_BLOCK;
    Cmd->Next = NULL;
    Cmd->Subcommands = NULL;
    Cmd->Redirections = RedirList;

    /* Read the block contents */
    NextPtr = &Cmd->Subcommands;
    InsideBlock++;
    while (1)
    {
        Sub = ParseCommandOp(C_OP_LOWEST);
        if (Sub)
        {
            *NextPtr = Sub;
            NextPtr = &Sub->Next;
        }
        else if (bParseError)
        {
            InsideBlock--;
            FreeCommand(Cmd);
            return NULL;
        }

        if (CurrentTokenType == TOK_END_BLOCK)
            break;

        /* Skip past the \n */
        ParseChar();
    }
    InsideBlock--;

    /* Process any trailing redirections */
    while (ParseToken(0, STANDARD_SEPS) == TOK_REDIRECTION)
    {
        if (!ParseRedirection(&Cmd->Redirections))
        {
            FreeCommand(Cmd);
            return NULL;
        }
    }
    return Cmd;
}

/* Parse an IF statement */
static PARSED_COMMAND *ParseIf(void)
{
    PARSED_COMMAND *Cmd;

    Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        return NULL;
    }
    memset(Cmd, 0, sizeof(PARSED_COMMAND));
    Cmd->Type = C_IF;

    if (bEnableExtensions && (_tcsicmp(CurrentToken, _T("/I")) == 0))
    {
        Cmd->If.Flags |= IFFLAG_IGNORECASE;
        ParseToken(0, STANDARD_SEPS);
    }
    if (_tcsicmp(CurrentToken, _T("not")) == 0)
    {
        Cmd->If.Flags |= IFFLAG_NEGATE;
        ParseToken(0, STANDARD_SEPS);
    }

    if (CurrentTokenType != TOK_NORMAL)
        goto error;

    /* Check for unary operators */
    for (; Cmd->If.Operator <= IF_MAX_UNARY; Cmd->If.Operator++)
    {
        /* Skip the extended operators if the extensions are disabled */
        if (!bEnableExtensions && (Cmd->If.Operator >= IF_CMDEXTVERSION))
            continue;

        if (_tcsicmp(CurrentToken, IfOperatorString[Cmd->If.Operator]) == 0)
        {
            if (ParseToken(0, STANDARD_SEPS) != TOK_NORMAL)
                goto error;
            Cmd->If.RightArg = cmd_dup(CurrentToken);
            goto condition_done;
        }
    }

    /* It must be a two-argument (comparison) operator. It could be ==, so
     * the equals sign can't be treated as whitespace here. */
    Cmd->If.LeftArg = cmd_dup(CurrentToken);
    ParseToken(0, _T(",;"));

    /* The right argument can come immediately after == */
    if (_tcsnicmp(CurrentToken, _T("=="), 2) == 0 && CurrentToken[2])
    {
        Cmd->If.RightArg = cmd_dup(&CurrentToken[2]);
        goto condition_done;
    }

    // Cmd->If.Operator == IF_MAX_UNARY + 1;
    for (; Cmd->If.Operator <= IF_MAX_COMPARISON; Cmd->If.Operator++)
    {
        /* Skip the extended operators if the extensions are disabled */
        if (!bEnableExtensions && (Cmd->If.Operator >= IF_EQU)) // (Cmd->If.Operator > IF_STRINGEQ)
            continue;

        if (_tcsicmp(CurrentToken, IfOperatorString[Cmd->If.Operator]) == 0)
        {
            if (ParseToken(0, STANDARD_SEPS) != TOK_NORMAL)
                goto error;
            Cmd->If.RightArg = cmd_dup(CurrentToken);
            goto condition_done;
        }
    }
    goto error;

condition_done:
    Cmd->Subcommands = ParseCommandOp(C_OP_LOWEST);
    if (Cmd->Subcommands == NULL)
        goto error;
    if (_tcsicmp(CurrentToken, _T("else")) == 0)
    {
        Cmd->Subcommands->Next = ParseCommandOp(C_OP_LOWEST);
        if (Cmd->Subcommands->Next == NULL)
            goto error;
    }

    return Cmd;

error:
    FreeCommand(Cmd);
    ParseError();
    return NULL;
}

/*
 * Parse a FOR command.
 * Syntax is: FOR [options] %var IN (list) DO command
 */
static PARSED_COMMAND *ParseFor(void)
{
    PARSED_COMMAND *Cmd;
    TCHAR* List = TempBuf;
    TCHAR *Pos = List;

    Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        return NULL;
    }
    memset(Cmd, 0, sizeof(PARSED_COMMAND));
    Cmd->Type = C_FOR;

    /* Skip the extended FOR syntax if extensions are disabled */
    if (!bEnableExtensions)
        goto parseForBody;

    while (1)
    {
        if (_tcsicmp(CurrentToken, _T("/D")) == 0)
        {
            Cmd->For.Switches |= FOR_DIRS;
        }
        else if (_tcsicmp(CurrentToken, _T("/F")) == 0)
        {
            Cmd->For.Switches |= FOR_F;
            if (!Cmd->For.Params)
            {
                ParseToken(0, STANDARD_SEPS);
                if (CurrentToken[0] == _T('/') || CurrentToken[0] == _T('%'))
                    break;
                Cmd->For.Params = cmd_dup(CurrentToken);
            }
        }
        else if (_tcsicmp(CurrentToken, _T("/L")) == 0)
        {
            Cmd->For.Switches |= FOR_LOOP;
        }
        else if (_tcsicmp(CurrentToken, _T("/R")) == 0)
        {
            Cmd->For.Switches |= FOR_RECURSIVE;
            if (!Cmd->For.Params)
            {
                ParseToken(0, STANDARD_SEPS);
                if (CurrentToken[0] == _T('/') || CurrentToken[0] == _T('%'))
                    break;
                StripQuotes(CurrentToken);
                Cmd->For.Params = cmd_dup(CurrentToken);
            }
        }
        else
        {
            break;
        }

        ParseToken(0, STANDARD_SEPS);
    }

    /* Make sure there aren't two different switches specified
     * at the same time, unless they're /D and /R */
    if ((Cmd->For.Switches & (Cmd->For.Switches - 1)) != 0
        && Cmd->For.Switches != (FOR_DIRS | FOR_RECURSIVE))
    {
        goto error;
    }

parseForBody:

    /* Variable name should be % and just one other character */
    if (CurrentToken[0] != _T('%') || _tcslen(CurrentToken) != 2)
        goto error;
    Cmd->For.Variable = CurrentToken[1];

    ParseToken(0, STANDARD_SEPS);
    if (_tcsicmp(CurrentToken, _T("in")) != 0)
        goto error;

    if (ParseToken(_T('('), STANDARD_SEPS) != TOK_BEGIN_BLOCK)
        goto error;

    while (1)
    {
        /* Pretend we're inside a block so the tokenizer will stop on ')' */
        InsideBlock++;
        ParseToken(0, STANDARD_SEPS);
        InsideBlock--;

        if (CurrentTokenType == TOK_END_BLOCK)
            break;

        if (CurrentTokenType == TOK_END)
        {
            /* Skip past the \n */
            ParseChar();
            continue;
        }

        if (CurrentTokenType != TOK_NORMAL)
            goto error;

        if (Pos != List)
            *Pos++ = _T(' ');

        if (Pos + _tcslen(CurrentToken) >= &List[CMDLINE_LENGTH])
            goto error;
        Pos = _stpcpy(Pos, CurrentToken);
    }
    *Pos = _T('\0');
    Cmd->For.List = cmd_dup(List);

    ParseToken(0, STANDARD_SEPS);
    if (_tcsicmp(CurrentToken, _T("do")) != 0)
        goto error;

    Cmd->Subcommands = ParseCommandOp(C_OP_LOWEST);
    if (Cmd->Subcommands == NULL)
        goto error;

    return Cmd;

error:
    FreeCommand(Cmd);
    ParseError();
    return NULL;
}

/* Parse a REM command */
static PARSED_COMMAND *ParseRem(void)
{
    /* "Ignore" the rest of the line.
     * (Line continuations will still be parsed, though.) */
    while (ParseToken(0, NULL) != TOK_END)
        ;
    return NULL;
}

static DECLSPEC_NOINLINE PARSED_COMMAND *ParseCommandPart(REDIRECTION *RedirList)
{
    TCHAR ParsedLine[CMDLINE_LENGTH];
    PARSED_COMMAND *Cmd;
    PARSED_COMMAND *(*Func)(void);

    TCHAR *Pos = _stpcpy(ParsedLine, CurrentToken) + 1;
    DWORD_PTR TailOffset = Pos - ParsedLine;

    /* Check for special forms */
    if ((Func = ParseFor, _tcsicmp(ParsedLine, _T("for")) == 0) ||
        (Func = ParseIf,  _tcsicmp(ParsedLine, _T("if")) == 0)  ||
        (Func = ParseRem, _tcsicmp(ParsedLine, _T("rem")) == 0))
    {
        ParseToken(0, STANDARD_SEPS);
        /* Do special parsing only if it's not followed by /? */
        if (_tcscmp(CurrentToken, _T("/?")) != 0)
        {
            if (RedirList)
            {
                ParseError();
                FreeRedirection(RedirList);
                return NULL;
            }
            return Func();
        }
        Pos = _stpcpy(Pos, _T(" /?"));
    }

    /* Now get the tail */
    while (1)
    {
        ParseToken(0, NULL);
        if (CurrentTokenType == TOK_NORMAL)
        {
            if (Pos + _tcslen(CurrentToken) >= &ParsedLine[CMDLINE_LENGTH])
            {
                ParseError();
                FreeRedirection(RedirList);
                return NULL;
            }
            Pos = _stpcpy(Pos, CurrentToken);
        }
        else if (CurrentTokenType == TOK_REDIRECTION)
        {
            if (!ParseRedirection(&RedirList))
                return NULL;
        }
        else
        {
            break;
        }
    }
    *Pos++ = _T('\0');

    Cmd = cmd_alloc(FIELD_OFFSET(PARSED_COMMAND, Command.First[Pos - ParsedLine]));
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        FreeRedirection(RedirList);
        return NULL;
    }
    Cmd->Type = C_COMMAND;
    Cmd->Next = NULL;
    Cmd->Subcommands = NULL;
    Cmd->Redirections = RedirList;
    memcpy(Cmd->Command.First, ParsedLine, (Pos - ParsedLine) * sizeof(TCHAR));
    Cmd->Command.Rest = Cmd->Command.First + TailOffset;
    return Cmd;
}

static PARSED_COMMAND *ParsePrimary(void)
{
    REDIRECTION *RedirList = NULL;
    int Type;

    while (IsSeparator(CurChar))
    {
        if (CurChar == _T('\n'))
            return NULL;
        ParseChar();
    }

    if (!CurChar)
        return NULL;

    if (CurChar == _T(':'))
    {
        /* "Ignore" the rest of the line.
         * (Line continuations will still be parsed, though.) */
        while (ParseToken(0, NULL) != TOK_END)
            ;
        return NULL;
    }

    if (CurChar == _T('@'))
    {
        PARSED_COMMAND *Cmd;
        ParseChar();
        Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
        if (!Cmd)
        {
            WARN("Cannot allocate memory for Cmd!\n");
            ParseError();
            return NULL;
        }
        Cmd->Type = C_QUIET;
        Cmd->Next = NULL;
        /* @ acts like a unary operator with low precedence,
         * so call the top-level parser */
        Cmd->Subcommands = ParseCommandOp(C_OP_LOWEST);
        Cmd->Redirections = NULL;
        return Cmd;
    }

    /* Process leading redirections and get the head of the command */
    while ((Type = ParseToken(_T('('), STANDARD_SEPS)) == TOK_REDIRECTION)
    {
        if (!ParseRedirection(&RedirList))
            return NULL;
    }

    if (Type == TOK_NORMAL)
        return ParseCommandPart(RedirList);
    else if (Type == TOK_BEGIN_BLOCK)
        return ParseBlock(RedirList);
    else if (Type == TOK_END_BLOCK && !RedirList)
        return NULL;

    ParseError();
    FreeRedirection(RedirList);
    return NULL;
}

static PARSED_COMMAND *ParseCommandOp(int OpType)
{
    PARSED_COMMAND *Cmd, *Left, *Right;

    if (OpType == C_OP_HIGHEST)
        Cmd = ParsePrimary();
    else
        Cmd = ParseCommandOp(OpType + 1);

    if (Cmd && !_tcscmp(CurrentToken, OpString[OpType - C_OP_LOWEST]))
    {
        Left = Cmd;
        Right = ParseCommandOp(OpType);
        if (!Right)
        {
            if (!bParseError)
            {
                /* & is allowed to have an empty RHS */
                if (OpType == C_MULTI)
                    return Left;
                ParseError();
            }
            FreeCommand(Left);
            return NULL;
        }

        Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
        if (!Cmd)
        {
            WARN("Cannot allocate memory for Cmd!\n");
            ParseError();
            FreeCommand(Left);
            FreeCommand(Right);
            return NULL;
        }
        Cmd->Type = OpType;
        Cmd->Next = NULL;
        Cmd->Redirections = NULL;
        Cmd->Subcommands = Left;
        Left->Next = Right;
        Right->Next = NULL;
    }

    return Cmd;
}

VOID
DumpCommand(PARSED_COMMAND *Cmd, ULONG SpacePad);

PARSED_COMMAND *
ParseCommand(LPTSTR Line)
{
    PARSED_COMMAND *Cmd;

    if (Line)
    {
        if (!SubstituteVars(Line, ParseLine, _T('%')))
            return NULL;
        bLineContinuations = FALSE;
    }
    else
    {
        if (!ReadLine(ParseLine, FALSE))
            return NULL;
        bLineContinuations = TRUE;
    }
    bParseError = FALSE;
    ParsePos = ParseLine;
    CurChar = _T(' ');

    Cmd = ParseCommandOp(C_OP_LOWEST);
    if (Cmd)
    {
        bIgnoreEcho = FALSE;

        if (CurrentTokenType != TOK_END)
            ParseError();
        if (bParseError)
        {
            FreeCommand(Cmd);
            return NULL;
        }

        /* Debugging support */
        if (fDumpParse)
            DumpCommand(Cmd, 0);
    }
    else
    {
        bIgnoreEcho = TRUE;
    }
    return Cmd;
}


/*
 * This function is similar to EchoCommand(), but is used
 * for dumping the command tree for debugging purposes.
 */
static VOID
DumpRedir(REDIRECTION* Redirections)
{
    REDIRECTION* Redir;

    if (Redirections)
#ifndef MSCMD_ECHO_COMMAND_COMPAT
        ConOutPuts(_T(" Redir: "));
#else
        ConOutPuts(_T("Redir: "));
#endif
    for (Redir = Redirections; Redir; Redir = Redir->Next)
    {
        ConOutPrintf(_T(" %x %s%s"), Redir->Number,
                     RedirString[Redir->Mode], Redir->Filename);
    }
}

VOID
DumpCommand(PARSED_COMMAND *Cmd, ULONG SpacePad)
{
/*
 * This macro is like DumpCommand(Cmd, Pad);
 * but avoids an extra recursive level.
 * Note that it can be used ONLY for terminating commands!
 */
#define DUMP(Command, Pad) \
do { \
    Cmd = (Command); \
    SpacePad = (Pad); \
    goto dump; \
} while (0)

    PARSED_COMMAND *Sub;

dump:
    /* Space padding */
    ConOutPrintf(_T("%*s"), SpacePad, _T(""));

    switch (Cmd->Type)
    {
    case C_COMMAND:
    {
        /* Generic command name, and Type */
#ifndef MSCMD_ECHO_COMMAND_COMPAT
        ConOutPrintf(_T("Cmd: %s  Type: %x"),
                     Cmd->Command.First, Cmd->Type);
#else
        ConOutPrintf(_T("Cmd: %s  Type: %x "),
                     Cmd->Command.First, Cmd->Type);
#endif
        /* Arguments */
        if (Cmd->Command.Rest && *(Cmd->Command.Rest))
#ifndef MSCMD_ECHO_COMMAND_COMPAT
            ConOutPrintf(_T(" Args: `%s'"), Cmd->Command.Rest);
#else
            ConOutPrintf(_T("Args: `%s' "), Cmd->Command.Rest);
#endif
        /* Redirections */
        DumpRedir(Cmd->Redirections);

        ConOutChar(_T('\n'));
        return;
    }

    case C_QUIET:
    {
#ifndef MSCMD_ECHO_COMMAND_COMPAT
        ConOutChar(_T('@'));
#else
        ConOutPuts(_T("@ "));
#endif
        DumpRedir(Cmd->Redirections); // FIXME: Can we have leading redirections??
        ConOutChar(_T('\n'));

        /*DumpCommand*/DUMP(Cmd->Subcommands, SpacePad + 2);
        return;
    }

    case C_BLOCK:
    {
#ifndef MSCMD_ECHO_COMMAND_COMPAT
        ConOutChar(_T('('));
#else
        ConOutPuts(_T("( "));
#endif
        DumpRedir(Cmd->Redirections);
        ConOutChar(_T('\n'));

        SpacePad += 2;

        for (Sub = Cmd->Subcommands; Sub; Sub = Sub->Next)
        {
#if defined(MSCMD_ECHO_COMMAND_COMPAT) && defined(MSCMD_PARSER_BUGS)
            /*
             * We will emulate Windows' CMD handling of "CRLF" and "&" multi-command
             * enumeration within parenthesized command blocks.
             */

            if (!Sub->Next)
            {
                DumpCommand(Sub, SpacePad);
                continue;
            }

            if (Sub->Type != C_MULTI)
            {
                ConOutPrintf(_T("%*s"), SpacePad, _T(""));
                ConOutPuts(_T("CRLF \n"));
                DumpCommand(Sub, SpacePad);
                continue;
            }

            /* Now, Sub->Type == C_MULTI */

            Cmd = Sub;

            ConOutPrintf(_T("%*s"), SpacePad, _T(""));
            ConOutPrintf(_T("%s \n"), OpString[Cmd->Type - C_OP_LOWEST]);
            // FIXME: Can we have redirections on these operator-type commands?

            SpacePad += 2;

            Cmd = Cmd->Subcommands;
            DumpCommand(Cmd, SpacePad);
            ConOutPrintf(_T("%*s"), SpacePad, _T(""));
            ConOutPuts(_T("CRLF \n"));
            DumpCommand(Cmd->Next, SpacePad);

            // NOTE: Next commands will remain indented.

#else

            /*
             * If this command is followed by another one, first display "CRLF".
             * This also emulates the CRLF placement "bug" of Windows' CMD
             * for the last two commands.
             */
            if (Sub->Next)
            {
                ConOutPrintf(_T("%*s"), SpacePad, _T(""));
#ifndef MSCMD_ECHO_COMMAND_COMPAT
                ConOutPuts(_T("CRLF\n"));
#else
                ConOutPuts(_T("CRLF \n"));
#endif
            }
            DumpCommand(Sub, SpacePad);

#endif // defined(MSCMD_ECHO_COMMAND_COMPAT) && defined(MSCMD_PARSER_BUGS)
        }

        return;
    }

    case C_MULTI:
    case C_OR:
    case C_AND:
    case C_PIPE:
    {
#ifndef MSCMD_ECHO_COMMAND_COMPAT
        ConOutPrintf(_T("%s\n"), OpString[Cmd->Type - C_OP_LOWEST]);
#else
        ConOutPrintf(_T("%s \n"), OpString[Cmd->Type - C_OP_LOWEST]);
#endif
        // FIXME: Can we have redirections on these operator-type commands?

        SpacePad += 2;

        Sub = Cmd->Subcommands;
        DumpCommand(Sub, SpacePad);
        /*DumpCommand*/DUMP(Sub->Next, SpacePad);
        return;
    }

    case C_IF:
    {
        ConOutPuts(_T("if"));
        /* NOTE: IF cannot have leading redirections */

        if (Cmd->If.Flags & IFFLAG_IGNORECASE)
            ConOutPuts(_T(" /I"));

        ConOutChar(_T('\n'));

        SpacePad += 2;

        /*
         * Show the IF command condition as a command.
         * If it is negated, indent the command more.
         */
        if (Cmd->If.Flags & IFFLAG_NEGATE)
        {
            ConOutPrintf(_T("%*s"), SpacePad, _T(""));
            ConOutPuts(_T("not\n"));
            SpacePad += 2;
        }

        ConOutPrintf(_T("%*s"), SpacePad, _T(""));

        /*
         * Command name:
         * - Unary operator: its name is the command name, and its argument is the command argument.
         * - Binary operator: its LHS is the command name, its RHS is the command argument.
         *
         * Type:
         * Windows' CMD (Win2k3 / Win7-10) values are as follows:
         *   CMDEXTVERSION  Type: 0x32 / 0x34
         *   ERRORLEVEL     Type: 0x33 / 0x35
         *   DEFINED        Type: 0x34 / 0x36
         *   EXIST          Type: 0x35 / 0x37
         *   ==             Type: 0x37 / 0x39 (String Comparison)
         *
         * For the following command:
         *   NOT            Type: 0x36 / 0x38
         * Windows only prints it without any type / redirection.
         *
         * For the following command:
         *   EQU, NEQ, etc. Type: 0x38 / 0x3a (Generic Comparison)
         * Windows displays it as command of unknown type.
         */
#ifndef MSCMD_ECHO_COMMAND_COMPAT
        ConOutPrintf(_T("Cmd: %s  Type: %x"),
                     (Cmd->If.Operator <= IF_MAX_UNARY) ?
                        IfOperatorString[Cmd->If.Operator] :
                        Cmd->If.LeftArg,
                     Cmd->If.Operator);
#else
        ConOutPrintf(_T("Cmd: %s  Type: %x "),
                     (Cmd->If.Operator <= IF_MAX_UNARY) ?
                        IfOperatorString[Cmd->If.Operator] :
                        Cmd->If.LeftArg,
                     Cmd->If.Operator);
#endif
        /* Arguments */
#ifndef MSCMD_ECHO_COMMAND_COMPAT
        ConOutPrintf(_T(" Args: `%s'"), Cmd->If.RightArg);
#else
        ConOutPrintf(_T("Args: `%s' "), Cmd->If.RightArg);
#endif

        ConOutChar(_T('\n'));

        if (Cmd->If.Flags & IFFLAG_NEGATE)
        {
            SpacePad -= 2;
        }

        Sub = Cmd->Subcommands;
        DumpCommand(Sub, SpacePad);
        if (Sub->Next)
        {
            ConOutPrintf(_T("%*s"), SpacePad - 2, _T(""));
            ConOutPuts(_T("else\n"));
            DumpCommand(Sub->Next, SpacePad);
        }
        return;
    }

    case C_FOR:
    {
        ConOutPuts(_T("for"));
        /* NOTE: FOR cannot have leading redirections */

        if (Cmd->For.Switches & FOR_DIRS)      ConOutPuts(_T(" /D"));
        if (Cmd->For.Switches & FOR_F)         ConOutPuts(_T(" /F"));
        if (Cmd->For.Switches & FOR_LOOP)      ConOutPuts(_T(" /L"));
        if (Cmd->For.Switches & FOR_RECURSIVE) ConOutPuts(_T(" /R"));
        if (Cmd->For.Params)
            ConOutPrintf(_T(" %s"), Cmd->For.Params);
        ConOutPrintf(_T(" %%%c in (%s) do\n"), Cmd->For.Variable, Cmd->For.List);
        /*DumpCommand*/DUMP(Cmd->Subcommands, SpacePad + 2);
        return;
    }

    default:
        ConOutPrintf(_T("*** Unknown type: %x\n"), Cmd->Type);
        break;
    }

#undef DUMP
}

/*
 * Reconstruct a parse tree into text form; used for echoing
 * batch file commands and FOR instances.
 */
VOID
EchoCommand(PARSED_COMMAND *Cmd)
{
    PARSED_COMMAND *Sub;
    REDIRECTION *Redir;

    switch (Cmd->Type)
    {
    case C_COMMAND:
    {
        if (SubstituteForVars(Cmd->Command.First, TempBuf))
            ConOutPrintf(_T("%s"), TempBuf);
        if (SubstituteForVars(Cmd->Command.Rest, TempBuf))
        {
            ConOutPrintf(_T("%s"), TempBuf);
#ifdef MSCMD_ECHO_COMMAND_COMPAT
            /* NOTE: For Windows compatibility, add a trailing space after printing the command parameter, if present */
            if (*TempBuf) ConOutChar(_T(' '));
#endif
        }
        break;
    }

    case C_QUIET:
        return;

    case C_BLOCK:
    {
        BOOLEAN bIsFirstCmdCRLF;

        ConOutChar(_T('('));

        Sub = Cmd->Subcommands;

        bIsFirstCmdCRLF = (Sub && Sub->Next);

#if defined(MSCMD_ECHO_COMMAND_COMPAT) && defined(MSCMD_PARSER_BUGS)
        /*
         * We will emulate Windows' CMD handling of "CRLF" and "&" multi-command
         * enumeration within parenthesized command blocks.
         */
        bIsFirstCmdCRLF = bIsFirstCmdCRLF && (Sub->Type != C_MULTI);
#endif

        /*
         * Single-command block: display all on one line.
         * Multi-command block: display commands on separate lines.
         */
        if (bIsFirstCmdCRLF)
            ConOutChar(_T('\n'));

        for (; Sub; Sub = Sub->Next)
        {
            EchoCommand(Sub);
            if (Sub->Next)
#ifdef MSCMD_ECHO_COMMAND_COMPAT
                ConOutPuts(_T(" \n "));
#else
                ConOutChar(_T('\n'));
#endif
        }

        if (bIsFirstCmdCRLF)
            ConOutChar(_T('\n'));

#ifdef MSCMD_ECHO_COMMAND_COMPAT
        /* NOTE: For Windows compatibility, add a trailing space after printing the closing parenthesis */
        ConOutPuts(_T(") "));
#else
        ConOutChar(_T(')'));
#endif
        break;
    }

    case C_MULTI:
    case C_OR:
    case C_AND:
    case C_PIPE:
    {
        Sub = Cmd->Subcommands;
        EchoCommand(Sub);
        ConOutPrintf(_T(" %s "), OpString[Cmd->Type - C_OP_LOWEST]);
        EchoCommand(Sub->Next);
        break;
    }

    case C_IF:
    {
        ConOutPuts(_T("if"));
        if (Cmd->If.Flags & IFFLAG_IGNORECASE)
            ConOutPuts(_T(" /I"));
        if (Cmd->If.Flags & IFFLAG_NEGATE)
            ConOutPuts(_T(" not"));
        if (Cmd->If.LeftArg && SubstituteForVars(Cmd->If.LeftArg, TempBuf))
            ConOutPrintf(_T(" %s"), TempBuf);
        ConOutPrintf(_T(" %s"), IfOperatorString[Cmd->If.Operator]);
        if (SubstituteForVars(Cmd->If.RightArg, TempBuf))
            ConOutPrintf(_T(" %s "), TempBuf);
        Sub = Cmd->Subcommands;
        EchoCommand(Sub);
        if (Sub->Next)
        {
            ConOutPuts(_T(" else "));
            EchoCommand(Sub->Next);
        }
        break;
    }

    case C_FOR:
    {
        ConOutPuts(_T("for"));
        if (Cmd->For.Switches & FOR_DIRS)      ConOutPuts(_T(" /D"));
        if (Cmd->For.Switches & FOR_F)         ConOutPuts(_T(" /F"));
        if (Cmd->For.Switches & FOR_LOOP)      ConOutPuts(_T(" /L"));
        if (Cmd->For.Switches & FOR_RECURSIVE) ConOutPuts(_T(" /R"));
        if (Cmd->For.Params)
            ConOutPrintf(_T(" %s"), Cmd->For.Params);
        if (Cmd->For.List && SubstituteForVars(Cmd->For.List, TempBuf))
            ConOutPrintf(_T(" %%%c in (%s) do "), Cmd->For.Variable, TempBuf);
        else
            ConOutPrintf(_T(" %%%c in (%s) do "), Cmd->For.Variable, Cmd->For.List);
        EchoCommand(Cmd->Subcommands);
        break;
    }

    default:
        ASSERT(FALSE);
        break;
    }

    for (Redir = Cmd->Redirections; Redir; Redir = Redir->Next)
    {
        if (SubstituteForVars(Redir->Filename, TempBuf))
        {
#ifdef MSCMD_ECHO_COMMAND_COMPAT
            ConOutPrintf(_T("%c%s%s "),
                         _T('0') + Redir->Number,
                         RedirString[Redir->Mode], TempBuf);
#else
            ConOutPrintf(_T(" %c%s%s"),
                         _T('0') + Redir->Number,
                         RedirString[Redir->Mode], TempBuf);
#endif
        }
    }
}

/*
 * "Unparse" a command into a text form suitable for passing to CMD /C.
 * Used for pipes. This is basically the same thing as EchoCommand, but
 * writing into a string instead of to standard output.
 */
TCHAR *
Unparse(PARSED_COMMAND *Cmd, TCHAR *Out, TCHAR *OutEnd)
{
    PARSED_COMMAND *Sub;
    REDIRECTION *Redir;

/*
 * Since this function has the annoying requirement that it must avoid
 * overflowing the supplied buffer, define some helper macros to make
 * this less painful.
 */
#define CHAR(Char) \
do { \
    if (Out == OutEnd) return NULL; \
    *Out++ = Char; \
} while (0)
#define STRING(String) \
do { \
    if (Out + _tcslen(String) > OutEnd) return NULL; \
    Out = _stpcpy(Out, String); \
} while (0)
#define PRINTF(Format, ...) \
do { \
    UINT Len = _sntprintf(Out, OutEnd - Out, Format, __VA_ARGS__); \
    if (Len > (UINT)(OutEnd - Out)) return NULL; \
    Out += Len; \
} while (0)
#define RECURSE(Subcommand) \
do { \
    Out = Unparse(Subcommand, Out, OutEnd); \
    if (!Out) return NULL; \
} while (0)

    switch (Cmd->Type)
    {
    case C_COMMAND:
        /* This is fragile since there could be special characters, but
         * Windows doesn't bother escaping them, so for compatibility
         * we probably shouldn't do it either */
        if (!SubstituteForVars(Cmd->Command.First, TempBuf)) return NULL;
        STRING(TempBuf);
        if (!SubstituteForVars(Cmd->Command.Rest, TempBuf)) return NULL;
        STRING(TempBuf);
        break;

    case C_QUIET:
        CHAR(_T('@'));
        RECURSE(Cmd->Subcommands);
        break;

    case C_BLOCK:
        CHAR(_T('('));
        for (Sub = Cmd->Subcommands; Sub; Sub = Sub->Next)
        {
            RECURSE(Sub);
            if (Sub->Next)
                CHAR(_T('&'));
        }
        CHAR(_T(')'));
        break;

    case C_MULTI:
    case C_OR:
    case C_AND:
    case C_PIPE:
        Sub = Cmd->Subcommands;
        RECURSE(Sub);
        PRINTF(_T(" %s "), OpString[Cmd->Type - C_OP_LOWEST]);
        RECURSE(Sub->Next);
        break;

    case C_IF:
        STRING(_T("if"));
        if (Cmd->If.Flags & IFFLAG_IGNORECASE)
            STRING(_T(" /I"));
        if (Cmd->If.Flags & IFFLAG_NEGATE)
            STRING(_T(" not"));
        if (Cmd->If.LeftArg && SubstituteForVars(Cmd->If.LeftArg, TempBuf))
            PRINTF(_T(" %s"), TempBuf);
        PRINTF(_T(" %s"), IfOperatorString[Cmd->If.Operator]);
        if (!SubstituteForVars(Cmd->If.RightArg, TempBuf)) return NULL;
        PRINTF(_T(" %s "), TempBuf);
        Sub = Cmd->Subcommands;
        RECURSE(Sub);
        if (Sub->Next)
        {
            STRING(_T(" else "));
            RECURSE(Sub->Next);
        }
        break;

    case C_FOR:
        STRING(_T("for"));
        if (Cmd->For.Switches & FOR_DIRS)      STRING(_T(" /D"));
        if (Cmd->For.Switches & FOR_F)         STRING(_T(" /F"));
        if (Cmd->For.Switches & FOR_LOOP)      STRING(_T(" /L"));
        if (Cmd->For.Switches & FOR_RECURSIVE) STRING(_T(" /R"));
        if (Cmd->For.Params)
            PRINTF(_T(" %s"), Cmd->For.Params);
        if (Cmd->For.List && SubstituteForVars(Cmd->For.List, TempBuf))
            PRINTF(_T(" %%%c in (%s) do "), Cmd->For.Variable, TempBuf);
        else
            PRINTF(_T(" %%%c in (%s) do "), Cmd->For.Variable, Cmd->For.List);
        RECURSE(Cmd->Subcommands);
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    for (Redir = Cmd->Redirections; Redir; Redir = Redir->Next)
    {
        if (!SubstituteForVars(Redir->Filename, TempBuf))
            return NULL;
        PRINTF(_T(" %c%s%s"), _T('0') + Redir->Number,
               RedirString[Redir->Mode], TempBuf);
    }
    return Out;

#undef CHAR
#undef STRING
#undef PRINTF
#undef RECURSE
}

VOID
FreeCommand(PARSED_COMMAND *Cmd)
{
    if (Cmd->Subcommands)
        FreeCommand(Cmd->Subcommands);
    if (Cmd->Next)
        FreeCommand(Cmd->Next);
    FreeRedirection(Cmd->Redirections);
    if (Cmd->Type == C_IF)
    {
        cmd_free(Cmd->If.LeftArg);
        cmd_free(Cmd->If.RightArg);
    }
    else if (Cmd->Type == C_FOR)
    {
        cmd_free(Cmd->For.Params);
        cmd_free(Cmd->For.List);
    }
    cmd_free(Cmd);
}
