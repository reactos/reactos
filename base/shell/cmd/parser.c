/*
 *  PARSER.C - Command-line Lexical Analyzer/Tokenizer and Parser.
 */

#include "precomp.h"

/*
 * Defines for enabling different Windows' CMD compatibility behaviours.
 */

/* Enable this define for command echoer compatibility */
#define MSCMD_ECHO_COMMAND_COMPAT

/* Enable this define for parser quirks (see UnParseToken() for more details) */
#define MSCMD_PARSER_BUGS

/* Enable this define for parenthesized blocks parsing quirks */
// #define MSCMD_PARENS_PARSE_BUGS

/* Enable this define for redirection parsing quirks */
#define MSCMD_REDIR_PARSE_BUGS

/* Enable this define for allowing '&' commands with an empty RHS.
 * The default behaviour is to just return the LHS instead.
 * See ParseCommandBinaryOp() for details. */
// #define MSCMD_MULTI_EMPTY_RHS


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

static const TCHAR* const IfOperatorString[] =
{
    /** Unary operators **/

    /* Standard */
    _T("errorlevel"),
    _T("exist"),

    /* Extended */
    _T("cmdextversion"),
    _T("defined"),
#define IF_MAX_UNARY IF_DEFINED

    /** Binary operators **/

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

static __inline BOOL IsSeparator(TCHAR Char)
{
    return _istspace(Char) || (Char && !!_tcschr(STANDARD_SEPS, Char));
}

typedef enum _TOK_TYPE
{
    TOK_END,
    TOK_NORMAL,
    TOK_OPERATOR,
    TOK_REDIRECTION,
    TOK_BEGIN_BLOCK,
    TOK_END_BLOCK
} TOK_TYPE;

/* Scratch buffer for temporary command substitutions / expansions */
static TCHAR TempBuf[CMDLINE_LENGTH];

/*static*/ BOOL bParseError;
static BOOL bLineContinuations;
/*static*/ TCHAR ParseLine[CMDLINE_LENGTH];
static PTCHAR ParsePos;
static PTCHAR OldParsePos;

BOOL bIgnoreParserComments = TRUE;
BOOL bHandleContinuations  = TRUE;

static TCHAR CurrentToken[CMDLINE_LENGTH];
static TOK_TYPE CurrentTokenType = TOK_END;
#ifndef MSCMD_PARSER_BUGS
static BOOL bReparseToken = FALSE;
static PTCHAR LastCurTokPos;
#endif
static INT InsideBlock = 0;

static VOID ResetParser(IN PTCHAR Pos)
{
    bParseError = FALSE;
    ParsePos = Pos;
    OldParsePos = ParsePos;
}

/*
 * This function "refetches" the last parsed token back into the stream
 * for later reparsing -- since the way of lexing it is context-dependent.
 * This "feature" is at the root of many obscure CMD parsing quirks,
 * due to the fact this feature is in opposition with line-continuation.
 * Indeed, when a stream of characters has a line-continuation, the lexer-
 * parser will parse the stream up to the end of the line, then will
 * reset the parser state and position back to the beginning of the line
 * before accepting the rest of the character stream and continuing
 * parsing them. This means that all the non-parsed characters before the
 * line-continuation have been lost. Of course, their parsed form is now
 * within the current parsed token. However, suppose now we need to
 * unparse this token for reparsing it a different way later on. If we
 * somehow pushed the already-parsed current token back into the beginning
 * of the character stream, besides the complications of moving up the
 * characters in the stream buffer, we would basically have "new" data
 * that has been already parsed one way, to be now parsed another way.
 * If instead we had saved somehow the unparsed form of the token, and
 * we push back that form into the stream buffer for reparsing, we would
 * encounter again the line-continuation, that, depending on which
 * context the token is reparsed, would cause problems:
 * e.g. in the case of REM command parsing, the parser would stop at the
 * first line-continuation.
 *
 * When MSCMD_PARSER_BUGS is undefined, the UnParseToken() / ParseToken()
 * cycle keeps the current token in its buffer, but also saves the start
 * position corresponding to the batch of characters that have been parsed
 * during the last line-continuation. The next ParseToken() would then
 * reparse these latest charcters and the result replaces the last part
 * in the current token.
 *
 * For example, a first parsing of
 *    foo^\n
 *    bar^\n
 *    baz
 * would result in the current token "foobarbaz", where the start position
 * corresponding to the batch of characters parsed during the last line-continuation
 * being pointing at "baz". The stream buffer only contains "baz" (and following data).
 * Then UnParseToken() saves this info so that at the next ParseToken(), the "baz"
 * part of the stream buffer gets reparsed (possibly differently) and the result
 * would replace the "baz" part in the current token.
 *
 * If MSCMD_PARSER_BUGS is defined however, then the behaviour of the Windows' CMD
 * parser applies: in the example above, the last ParseToken() call would completely
 * replace the current token "foobarbaz" with the new result of the parsing of "baz".
 */
static VOID UnParseToken(VOID)
{
    ParsePos = OldParsePos;

    /* Debugging support */
    if (fDumpTokens)
        ConOutPrintf(_T("Ungetting: '%s'\n"), ParsePos);

#ifndef MSCMD_PARSER_BUGS
    bReparseToken = TRUE;
#endif
}

static VOID InitParser(VOID)
{
    *CurrentToken = 0;
    CurrentTokenType = TOK_END;
    InsideBlock = 0;

#ifndef MSCMD_PARSER_BUGS
    bReparseToken = FALSE;
    LastCurTokPos = NULL;
#endif

    ResetParser(ParseLine);
}

static TCHAR ParseChar(VOID)
{
    TCHAR Char;

    if (bParseError)
        return 0;

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

    if (!Char) --ParsePos;
    if (!Char && bLineContinuations)
    {
        if (!ReadLine(ParseLine, TRUE))
        {
            /* ^C pressed, or line was too long */
            //
            // FIXME: Distinguish with respect to BATCH end of file !!
            //
            bParseError = TRUE;
        }
        else
        {
            ResetParser(ParseLine);
            if (*ParsePos)
                goto restart;
        }
    }
    return Char;
}

VOID ParseErrorEx(IN PCTSTR s)
{
    /* Only display the first error we encounter */
    if (!bParseError)
        error_syntax(s);
    bParseError = TRUE;
}

static __inline VOID ParseError(VOID)
{
    ParseErrorEx(CurrentTokenType != TOK_END ? CurrentToken : NULL);
}

static TOK_TYPE
ParseTokenEx(
    IN TCHAR PrefixOperator OPTIONAL,
    IN TCHAR ExtraEnd OPTIONAL,
    IN PCTSTR Separators OPTIONAL,
    IN BOOL bHandleContinuations)
{
    TOK_TYPE Type;
    PTCHAR CurrentTokStart = CurrentToken;
    PTCHAR Out = CurrentTokStart;
    TCHAR Char;
    BOOL bInQuote = FALSE;

#ifndef MSCMD_PARSER_BUGS
    if (bReparseToken)
    {
        bReparseToken = FALSE;

        /*
         * We will append the part to be reparsed to the old one
         * (still present in CurrentToken).
         */
        CurrentTokStart = LastCurTokPos;
        Out = CurrentTokStart;
    }
    else
    {
        LastCurTokPos = CurrentToken;
    }
#endif

    /* Start with what we have at current ParsePos */
    OldParsePos = ParsePos;

    for (Char = ParseChar(); Char && Char != _T('\n'); Char = ParseChar())
    {
        bInQuote ^= (Char == _T('"'));
        if (!bInQuote)
        {
            if (Separators != NULL)
            {
                if (_istspace(Char) || !!_tcschr(Separators, Char))
                {
                    /* Skip leading separators */
                    if (Out == CurrentTokStart)
                        continue;
                    break;
                }
            }

            /* Check for prefix operator */
            if ((Out == CurrentTokStart) && (Char == PrefixOperator))
                break;

            /*
             * Check for numbered redirection.
             *
             * For this purpose, we check whether this is a number, that is
             * in first position in the current parsing buffer (remember that
             * ParsePos points to the next character) or is preceded by a
             * whitespace-like separator, including standard command operators
             * (excepting '@' !) and double-quotes.
             */
            if ( _istdigit(Char) &&
                 (ParsePos == &OldParsePos[1]  ||
                     IsSeparator(ParsePos[-2]) ||
                     !!_tcschr(_T("()&|\""), ParsePos[-2])) &&
                 (*ParsePos == _T('<') || *ParsePos == _T('>')) )
            {
                break;
            }

            /* Check for other delimiters / operators */
            if (Char == ExtraEnd)
                break;
            if (InsideBlock && Char == _T(')'))
                break;
            if (_tcschr(_T("&|<>"), Char))
                break;

            if (bHandleContinuations && (Char == _T('^')))
            {
                Char = ParseChar();
                /* Eat up a \n, allowing line continuation */
                if (Char == _T('\n'))
                {
#ifndef MSCMD_PARSER_BUGS
                    LastCurTokPos = Out;
#endif
                    Char = ParseChar();
                }
                /* Next character is a forced literal */

                if (Out == CurrentTokStart)
                {
                    /* Ignore any prefix operator if we don't start a new command block */
                    if (CurrentTokenType != TOK_BEGIN_BLOCK)
                        PrefixOperator = 0;
                }
            }
        }
        if (Out == &CurrentToken[CMDLINE_LENGTH - 1])
            break;
        *Out++ = Char;

        // PrefixOperator = 0;
    }

    /*
     * We exited the parsing loop. If the current character is the first one
     * (Out == CurrentTokStart), interpret it as an operator. Otherwise,
     * terminate the current token (type TOK_NORMAL) and keep the current
     * character so that it can be refetched as an operator at the next call.
     */

    if (Out != CurrentTokStart)
    {
        Type = TOK_NORMAL;
    }
    /*
     * Else we have an operator.
     */
    else if (Char == _T('@'))
    {
        Type = TOK_OPERATOR; // TOK_QUIET / TOK_PREFIX_OPERATOR
        *Out++ = Char;
        Char = ParseChar();
    }
    else if (Char == _T('('))
    {
        Type = TOK_BEGIN_BLOCK;
        *Out++ = Char;
        Char = ParseChar();
    }
    else if (Char == _T(')'))
    {
        Type = TOK_END_BLOCK;
        *Out++ = Char;
        Char = ParseChar();
    }
    else if (Char == _T('&') || Char == _T('|'))
    {
        Type = TOK_OPERATOR;
        *Out++ = Char;
        Char = ParseChar();
        /* Check for '&&' or '||' */
        if (Char == Out[-1])
        {
            *Out++ = Char;
            Char = ParseChar();
        }
    }
    else if ( _istdigit(Char)  ||
              (Char == _T('<') || Char == _T('>')) )
    {
        Type = TOK_REDIRECTION;
        if (_istdigit(Char))
        {
            *Out++ = Char;
            Char = ParseChar();
        }
        /* By construction (see the while-loop above),
         * the next character must be a redirection. */
        ASSERT(Char == _T('<') || Char == _T('>'));
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
            if (_istdigit(Char))
            {
                *Out++ = Char;
                Char = ParseChar();
            }
        }
    }
    else
    {
        Type = TOK_END;
        *Out++ = Char;
    }
    *Out = _T('\0');

    /*
     * Rewind the parsing position, so that the current character can be
     * refetched later on. However do this only if it is not NULL and if
     * this is not TOK_END, since we do not want to reparse later the line
     * termination (we could enter into infinite loops, or, in case of line
     * continuation, get unwanted "More?" prompts).
     */
    if (Char != 0 && Type != TOK_END)
        --ParsePos;

    /* Debugging support */
    if (fDumpTokens)
        ConOutPrintf(_T("ParseToken: (%d) '%s'\n"), Type, CurrentToken);

    return (CurrentTokenType = Type);
}

static __inline INT
ParseToken(
    IN TCHAR ExtraEnd OPTIONAL,
    IN PCTSTR Separators OPTIONAL)
{
    return ParseTokenEx(0, ExtraEnd, Separators, bHandleContinuations);
}


static PARSED_COMMAND*
AllocCommand(
    IN COMMAND_TYPE Type,
    IN PCTSTR CmdHead OPTIONAL,
    IN PCTSTR CmdTail OPTIONAL)
{
    PARSED_COMMAND* Cmd;

    switch (Type)
    {
    case C_COMMAND:
    case C_REM:
    {
        SIZE_T CmdHeadLen = _tcslen(CmdHead) + 1;
        SIZE_T CmdTailLen = _tcslen(CmdTail) + 1;

        Cmd = cmd_alloc(FIELD_OFFSET(PARSED_COMMAND,
                                     Command.First[CmdHeadLen + CmdTailLen]));
        if (!Cmd)
            return NULL;

        Cmd->Type = Type;
        Cmd->Next = NULL;
        Cmd->Subcommands = NULL;
        Cmd->Redirections = NULL; /* Is assigned by the calling function */
        memcpy(Cmd->Command.First, CmdHead, CmdHeadLen * sizeof(TCHAR));
        Cmd->Command.Rest = Cmd->Command.First + CmdHeadLen;
        memcpy(Cmd->Command.Rest, CmdTail, CmdTailLen * sizeof(TCHAR));
        return Cmd;
    }

    case C_QUIET:
    case C_BLOCK:
    case C_MULTI:
    case C_OR:
    case C_AND:
    case C_PIPE:
    {
        Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
        if (!Cmd)
            return NULL;

        Cmd->Type = Type;
        Cmd->Next = NULL;
        Cmd->Subcommands = NULL;
        Cmd->Redirections = NULL; /* For C_BLOCK only: is assigned by the calling function */
        return Cmd;
    }

    case C_FOR:
    case C_IF:
    {
        Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
        if (!Cmd)
            return NULL;

        memset(Cmd, 0, sizeof(PARSED_COMMAND));
        Cmd->Type = Type;
        return Cmd;
    }

    default:
        ERR("Unknown command type 0x%x\n", Type);
        ASSERT(FALSE);
        return NULL;
    }
}

VOID
FreeCommand(
    IN OUT PARSED_COMMAND* Cmd)
{
    if (Cmd->Subcommands)
        FreeCommand(Cmd->Subcommands);
    if (Cmd->Next)
        FreeCommand(Cmd->Next);
    FreeRedirection(Cmd->Redirections);
    if (Cmd->Type == C_FOR)
    {
        cmd_free(Cmd->For.Params);
        cmd_free(Cmd->For.List);
    }
    else if (Cmd->Type == C_IF)
    {
        cmd_free(Cmd->If.LeftArg);
        cmd_free(Cmd->If.RightArg);
    }
    cmd_free(Cmd);
}


/* Parse redirections and append them to the list */
static BOOL
ParseRedirection(
    IN OUT REDIRECTION** List)
{
    PTSTR Tok = CurrentToken;
    REDIRECTION* Redir;
    REDIR_MODE RedirMode;
    BYTE Number;

    if ( !(*Tok == _T('<') || *Tok == _T('>')) &&
         !(_istdigit(*Tok) &&
           (Tok[1] == _T('<') || Tok[1] == _T('>')) ) )
    {
        ASSERT(CurrentTokenType != TOK_REDIRECTION);
        return FALSE;
    }
    ASSERT((CurrentTokenType == TOK_REDIRECTION) ||
           (CurrentTokenType == TOK_NORMAL));

    if (_istdigit(*Tok))
        Number = *Tok++ - _T('0');
    else
        Number = *Tok == _T('<') ? 0 : 1;

    if (*Tok++ == _T('<'))
    {
        RedirMode = REDIR_READ;
        /* Forbid '<<' */
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

    if (*Tok == _T('&'))
    {
        /* This is a handle redirection: the next character must be one single digit */
        if (!(_istdigit(Tok[1]) && !Tok[2]))
            goto fail;
    }
    else
#ifndef MSCMD_REDIR_PARSE_BUGS
    if (!*Tok)
        /* The file name was not part of this token, so it will be the next one */
#else
        /* Get rid of what possibly remains in the token, and retrieve the next one */
#endif
    {
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

static __inline PARSED_COMMAND*
ParseCommandOp(
    IN COMMAND_TYPE OpType);

/* Parse a parenthesized block */
static PARSED_COMMAND*
ParseBlock(
    IN OUT REDIRECTION** RedirList)
{
    PARSED_COMMAND *Cmd, *Sub, **NextPtr;

    Cmd = AllocCommand(C_BLOCK, NULL, NULL);
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        return NULL;
    }

    /* Read the block contents */
    NextPtr = &Cmd->Subcommands;
    ++InsideBlock;
    while (TRUE)
    {
        /*
         * Windows' CMD compatibility: Strip leading newlines in the block.
         *
         * Note that this behaviour is buggy, especially when MSCMD_PARSER_BUGS is defined!
         * For example:
         *   (foo^\n
         *   bar)
         * would be parsed ultimately as: '(', 'bar', ')' because the "foo^"
         * part would be discarded due to the UnParseToken() call, since this
         * function doesn't work across line continuations.
         */
        while (ParseToken(0, STANDARD_SEPS) == TOK_END && *CurrentToken == _T('\n'))
            ;
        if (*CurrentToken && *CurrentToken != _T('\n'))
            UnParseToken();

        /* Break early if we have nothing else to read. We will also fail
         * due to the fact we haven't encountered any closing parenthesis. */
        if (!*CurrentToken /* || *CurrentToken == _T('\n') */)
        {
            ASSERT(CurrentTokenType == TOK_END);
            break;
        }

        /*
         * NOTE: Windows' CMD uses a "CRLF" operator when dealing with
         * newlines in parenthesized blocks, as an alternative to the
         * '&' command-separation operator.
         */

        Sub = ParseCommandOp(C_OP_LOWEST);
        if (Sub)
        {
            *NextPtr = Sub;
            NextPtr = &Sub->Next;
        }
        else if (bParseError)
        {
            --InsideBlock;
            FreeCommand(Cmd);
            return NULL;
        }

        if (CurrentTokenType == TOK_END_BLOCK)
            break;

        /* Skip past the \n */
    }
    --InsideBlock;

    /* Fail if the block was not terminated, or if we have
     * an empty block, i.e. "( )", considered invalid. */
    if ((CurrentTokenType != TOK_END_BLOCK) || (Cmd->Subcommands == NULL))
    {
        ParseError();
        FreeCommand(Cmd);
        return NULL;
    }

    /* Process any trailing redirections and append them to the list */
#ifndef MSCMD_REDIR_PARSE_BUGS
    while (ParseToken(0, STANDARD_SEPS) == TOK_REDIRECTION)
    {
        if (!ParseRedirection(RedirList))
        {
            FreeCommand(Cmd);
            return NULL;
        }
    }
#else
    while (ParseToken(0, STANDARD_SEPS) != TOK_END)
    {
        if (!ParseRedirection(RedirList))
        {
            /* If an actual error happened in ParseRedirection(), bail out */
            if (bParseError)
            {
                FreeCommand(Cmd);
                return NULL;
            }
            /* Otherwise it just returned FALSE because the current token
             * is not a redirection. Unparse the token and refetch it. */
            break;
        }
    }
#endif
    if (CurrentTokenType != TOK_END)
    {
        /*
         * Windows' CMD compatibility: Unparse the current token.
         *
         * Note that this behaviour is buggy, especially when MSCMD_PARSER_BUGS is defined!
         * For example:
         *   (foo^\n
         *   bar)
         * would be parsed ultimately as: '(', 'bar', ')' because the "foo^"
         * part would be discarded due to the UnParseToken() call, since this
         * function doesn't work across line continuations.
         */
        UnParseToken();

        /*
         * Since it is expected that when ParseBlock() returns, the next
         * token is already fetched, call ParseToken() again to compensate.
         */
        ParseToken(0, STANDARD_SEPS);
    }

    return Cmd;
}

/* Parse an IF statement */
static PARSED_COMMAND*
ParseIf(VOID)
{
    PARSED_COMMAND* Cmd;

    Cmd = AllocCommand(C_IF, NULL, NULL);
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        return NULL;
    }

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
static PARSED_COMMAND*
ParseFor(VOID)
{
    PARSED_COMMAND* Cmd;

    /* Use the scratch buffer */
    PTSTR List = TempBuf;
    PTCHAR Pos = List;

    Cmd = AllocCommand(C_FOR, NULL, NULL);
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        return NULL;
    }

    /* Skip the extended FOR syntax if extensions are disabled */
    if (!bEnableExtensions)
        goto parseForBody;

    while (TRUE)
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

    while (TRUE)
    {
        /* Pretend we're inside a block so the tokenizer will stop on ')' */
        ++InsideBlock;
        ParseToken(0, STANDARD_SEPS);
        --InsideBlock;

        if (CurrentTokenType == TOK_END_BLOCK)
            break;

        /* Skip past the \n */
        if ((CurrentTokenType == TOK_END) && *CurrentToken == _T('\n'))
            continue;

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
static PARSED_COMMAND*
ParseRem(VOID)
{
    PARSED_COMMAND* Cmd;

    /* The common scratch buffer already contains the name of the command */
    PTSTR ParsedLine = TempBuf;

    PTCHAR Pos = ParsedLine + _tcslen(ParsedLine) + 1;
    SIZE_T TailOffset = Pos - ParsedLine;

    /* Build a minimal command for REM, so that it can still get through the batch echo unparsing */

    /* Unparse the current token, so as to emulate the REM command parsing
     * behaviour of Windows' CMD, that discards everything before the last
     * line continuation. */
    UnParseToken();

    /*
     * Ignore the rest of the line, without any line continuation (but eat the caret).
     * We cannot simply set bLineContinuations to TRUE or FALSE, because we want (only
     * for the REM command), even when bLineContinuations == FALSE, to get the caret,
     * otherwise it would be ignored.
     */
    while (ParseTokenEx(0, 0, NULL, FALSE) != TOK_END)
    {
        if (Pos + _tcslen(CurrentToken) >= &ParsedLine[CMDLINE_LENGTH])
        {
            ParseError();
            return NULL;
        }
        Pos = _stpcpy(Pos, CurrentToken);
    }
    *Pos = _T('\0');

    Cmd = AllocCommand(C_REM,
                       ParsedLine,
                       ParsedLine + TailOffset);
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        return NULL;
    }
    return Cmd;
}

/* Parse a command */
static PARSED_COMMAND*
ParseCommandPart(
    IN OUT REDIRECTION** RedirList)
{
    PARSED_COMMAND* Cmd;
    PARSED_COMMAND* (*Func)(VOID);

    /* Use the scratch buffer */
    PTSTR ParsedLine = TempBuf;

    /* We need to copy the current token because it's going to be changed below by the ParseToken() calls */
    PTCHAR Pos = _stpcpy(ParsedLine, CurrentToken) + 1;
    SIZE_T TailOffset = Pos - ParsedLine;

    /* Check for special forms */
    if ((Func = ParseFor, _tcsicmp(ParsedLine, _T("FOR")) == 0) ||
        (Func = ParseIf,  _tcsicmp(ParsedLine, _T("IF"))  == 0) ||
        (Func = ParseRem, _tcsicmp(ParsedLine, _T("REM")) == 0))
    {
        PTCHAR pHelp;

        ParseToken(0, STANDARD_SEPS);

        if ((pHelp = _tcsstr(CurrentToken, _T("/?"))) &&
            (Func == ParseIf ? (pHelp[2] == _T('/') || pHelp[2] == 0) : TRUE))
        {
            /* /? was found within the first token */
            ParseToken(0, STANDARD_SEPS);
        }
        else
        {
            pHelp = NULL;
        }
        if (pHelp && (CurrentTokenType == TOK_NORMAL))
        {
            /* We encountered /? first, but is followed
             * by another token: that's an error. */
            ParseError();
            return NULL;
        }

        /* Do actual parsing only if no help is present */
        if (!pHelp)
        {
            /* FOR and IF commands cannot have leading redirection, but REM can */
            if (*RedirList && ((Func == ParseFor) || (Func == ParseIf)))
            {
                /* Display the culprit command and fail */
                ParseErrorEx(ParsedLine);
                return NULL;
            }

            return Func();
        }

        /* Otherwise, run FOR,IF,REM as regular commands only for help support */
        if (Pos + _tcslen(_T("/?")) >= &ParsedLine[CMDLINE_LENGTH])
        {
            ParseError();
            return NULL;
        }
        Pos = _stpcpy(Pos, _T("/?"));
    }
    else
    {
        ParseToken(0, NULL);
    }

    /* Now get the tail */
    while (CurrentTokenType != TOK_END)
    {
        if (CurrentTokenType == TOK_NORMAL)
        {
            if (Pos + _tcslen(CurrentToken) >= &ParsedLine[CMDLINE_LENGTH])
            {
                ParseError();
                return NULL;
            }
            Pos = _stpcpy(Pos, CurrentToken);
        }
#ifndef MSCMD_REDIR_PARSE_BUGS
        else if (CurrentTokenType == TOK_REDIRECTION)
        {
            /* Process any trailing redirections and append them to the list */
            while (CurrentTokenType == TOK_REDIRECTION)
            {
                if (!ParseRedirection(RedirList))
                    return NULL;

                ParseToken(0, STANDARD_SEPS);
            }
            if (CurrentTokenType == TOK_END)
                break;

            /* Unparse the current token, and reparse it below with no separators */
            UnParseToken();
        }
        else
        {
            /* There is no need to do a UnParseToken() / ParseToken() cycle */
            break;
        }
#else
        else
        {
            /* Process any trailing redirections and append them to the list */
            BOOL bSuccess = FALSE;

            ASSERT(CurrentTokenType != TOK_END);

            while (CurrentTokenType != TOK_END)
            {
                if (!ParseRedirection(RedirList))
                {
                    /* If an actual error happened in ParseRedirection(), bail out */
                    if (bParseError)
                        return NULL;

                    /* Otherwise it just returned FALSE because the current token
                     * is not a redirection. Unparse the token and refetch it. */
                    break;
                }
                bSuccess = TRUE;

                ParseToken(0, STANDARD_SEPS);
            }
            if (CurrentTokenType == TOK_END)
                break;

            /* Unparse the current token, and reparse it below with no separators */
            UnParseToken();

            /* If bSuccess == FALSE, we know that it's still the old fetched token, but
             * it has been unparsed, so we need to refetch it before quitting the loop. */
            if (!bSuccess)
            {
                ParseToken(0, NULL);
                break;
            }
        }
#endif

        ParseToken(0, NULL);
    }
    *Pos = _T('\0');

    Cmd = AllocCommand(C_COMMAND,
                       ParsedLine,
                       ParsedLine + TailOffset);
    if (!Cmd)
    {
        WARN("Cannot allocate memory for Cmd!\n");
        ParseError();
        return NULL;
    }
    return Cmd;
}

static PARSED_COMMAND*
ParsePrimary(VOID)
{
    PARSED_COMMAND* Cmd = NULL;
    REDIRECTION* RedirList = NULL;

    /* In this context, '@' is considered as a separate token */
    if ((*CurrentToken == _T('@')) && (CurrentTokenType == TOK_OPERATOR))
    {
        Cmd = AllocCommand(C_QUIET, NULL, NULL);
        if (!Cmd)
        {
            WARN("Cannot allocate memory for Cmd!\n");
            ParseError();
            return NULL;
        }
        /* @ acts like a unary operator with low precedence,
         * so call the top-level parser */
        Cmd->Subcommands = ParseCommandOp(C_OP_LOWEST);
        return Cmd;
    }

    /* Process leading redirections and get the head of the command */
#ifndef MSCMD_REDIR_PARSE_BUGS
    while (CurrentTokenType == TOK_REDIRECTION)
    {
        if (!ParseRedirection(&RedirList))
            return NULL;

        ParseToken(_T('('), STANDARD_SEPS);
    }
#else
    {
    BOOL bSuccess = FALSE;
    while (CurrentTokenType != TOK_END)
    {
        if (!ParseRedirection(&RedirList))
        {
            /* If an actual error happened in ParseRedirection(), bail out */
            if (bParseError)
                return NULL;

            /* Otherwise it just returned FALSE because
             * the current token is not a redirection. */
            break;
        }
        bSuccess = TRUE;

        ParseToken(0, STANDARD_SEPS);
    }
    if (bSuccess)
    {
        /* Unparse the current token, and reparse it with support for parenthesis */
        if (CurrentTokenType != TOK_END)
            UnParseToken();

        ParseToken(_T('('), STANDARD_SEPS);
    }
    }
#endif

    if (CurrentTokenType == TOK_NORMAL)
        Cmd = ParseCommandPart(&RedirList);
    else if (CurrentTokenType == TOK_BEGIN_BLOCK)
        Cmd = ParseBlock(&RedirList);
    else if (CurrentTokenType == TOK_END_BLOCK && !RedirList)
        return NULL;

    if (Cmd)
    {
        /* FOR and IF commands cannot have leading redirection
         * (checked by ParseCommandPart(), errors out if so). */
        ASSERT(!RedirList || (Cmd->Type != C_FOR && Cmd->Type != C_IF));

        /* Save the redirection list in the command */
        Cmd->Redirections = RedirList;

        /* Return the new command */
        return Cmd;
    }

    ParseError();
    FreeRedirection(RedirList);
    return NULL;
}

static PARSED_COMMAND*
ParseCommandBinaryOp(
    IN COMMAND_TYPE OpType)
{
    PARSED_COMMAND* Cmd;

    if (OpType == C_OP_LOWEST) // i.e. CP_MULTI
    {
        /* Ignore any parser-level comments */
        if (bIgnoreParserComments && (*CurrentToken == _T(':')))
        {
            /* Ignore the rest of the line, including line continuations */
            while (ParseToken(0, NULL) != TOK_END)
                ;
#ifdef MSCMD_PARENS_PARSE_BUGS
            /*
             * Return NULL in case we are NOT inside a parenthesized block,
             * otherwise continue. The effects can be observed as follows:
             * within a parenthesized block, every second ':'-prefixed command
             * is not ignored, while the first of each "pair" is ignored.
             * This first command **MUST NOT** be followed by an empty line,
             * otherwise a syntax error is raised.
             */
            if (InsideBlock == 0)
            {
#endif
                return NULL;
#ifdef MSCMD_PARENS_PARSE_BUGS
            }
            /* Get the next token */
            ParseToken(0, NULL);
#endif
        }

        /*
         * Ignore single closing parenthesis outside of command blocks,
         * thus interpreted as a command. This very specific situation
         * can happen e.g. while running in batch mode, when jumping to
         * a label present inside a command block.
         *
         * NOTE: If necessary, this condition can be restricted to only
         * when a batch context 'bc' is active.
         *
         * NOTE 2: For further security, Windows checks that we are NOT
         * currently inside a parenthesized block, and also, ignores
         * explicitly everything (ParseToken() loop) on the same line
         * (including line continuations) after this closing parenthesis.
         *
         * Why doing so? Consider the following batch:
         *
         *   IF 1==1 (
         *   :label
         *       echo A
         *   ) ^
         *   ELSE (
         *       echo B
         *       exit /b
         *   )
         *   GOTO :label
         *
         * First the IF block is executed. Since the condition is trivially
         * true, only the first block "echo A" is executed, then execution
         * goes after the IF block, that is, at the GOTO. Here, the GOTO
         * jumps within the first IF-block, however, the running context now
         * is NOT an IF. So parsing and execution will go through each command,
         * starting with 'echo A'. But then one gets the ') ^\n ELSE (' part !!
         * If we want to make sense of this without bailing out due to
         * parsing error, we should ignore this line, **including** the line
         * continuation. Hence we need to loop over all the tokens following
         * the closing parenthesis, instead of just returning NULL straight ahead.
         * Then execution continues with the other commands, 'echo B' and
         * 'exit /b' (here to stop the code loop). Execution would also
         * continue (if 'exit' was replaced by something else) and encounter
         * the lone closing parenthesis ')', that should again be ignored.
         *
         * Note that this feature has been introduced in Win2k+.
         */
        if (/** bc && **/ (_tcscmp(CurrentToken, _T(")")) == 0) &&
            (CurrentTokenType != TOK_END_BLOCK))
        {
            ASSERT(InsideBlock == 0);

            /* Ignore the rest of the line, including line continuations */
            while (ParseToken(0, NULL) != TOK_END)
                ;
            return NULL;
        }

#ifdef MSCMD_PARENS_PARSE_BUGS
        /* Check whether we have an empty line only if we are not inside
         * a parenthesized block, and return NULL if so, otherwise do not
         * do anything; a syntax error will be raised later. */
        if (InsideBlock == 0)
#endif
        if (!*CurrentToken || *CurrentToken == _T('\n'))
        {
            ASSERT(CurrentTokenType == TOK_END);
            return NULL;
        }
    }

    if (OpType == C_OP_HIGHEST)
        Cmd = ParsePrimary();
    else
        Cmd = ParseCommandBinaryOp(OpType + 1);

    if (Cmd && !_tcscmp(CurrentToken, OpString[OpType - C_OP_LOWEST]))
    {
        PARSED_COMMAND* Left = Cmd;
        PARSED_COMMAND* Right;

        Right = ParseCommandOp(OpType);
        if (!Right)
        {
            /*
             * The '&' operator is allowed to have an empty RHS.
             * In this case, we directly return the LHS only.
             * Note that Windows' CMD prefers building a '&'
             * command with an empty RHS.
             */
            if (!bParseError && (OpType != C_MULTI))
                ParseError();
            if (bParseError)
            {
                FreeCommand(Left);
                return NULL;
            }

#ifndef MSCMD_MULTI_EMPTY_RHS
            return Left;
#endif
        }

        Cmd = AllocCommand(OpType, NULL, NULL);
        if (!Cmd)
        {
            WARN("Cannot allocate memory for Cmd!\n");
            ParseError();
            FreeCommand(Left);
            FreeCommand(Right);
            return NULL;
        }
        Cmd->Subcommands = Left;
        Left->Next = Right;
#ifdef MSCMD_MULTI_EMPTY_RHS
        if (Right)
#endif
        Right->Next = NULL;
    }

    return Cmd;
}
static __inline PARSED_COMMAND*
ParseCommandOp(
    IN COMMAND_TYPE OpType)
{
    /* Start parsing: initialize the first token */

    /* Parse the prefix "quiet" operator '@' as a separate command.
     * Thus, @@foo@bar is parsed as: '@', '@', 'foo@bar'. */
    ParseTokenEx(_T('@'), _T('('), STANDARD_SEPS, bHandleContinuations);

    return ParseCommandBinaryOp(OpType);
}


PARSED_COMMAND*
ParseCommand(
    IN PCTSTR Line)
{
    PARSED_COMMAND* Cmd;

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

    InitParser();

    Cmd = ParseCommandOp(C_OP_LOWEST);
    if (Cmd)
    {
        bIgnoreEcho = FALSE;

        if ((CurrentTokenType != TOK_END) &&
            (_tcscmp(CurrentToken, _T("\n")) != 0))
        {
            ParseError();
        }
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
DumpRedir(
    IN REDIRECTION* Redirections)
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
DumpCommand(
    IN PARSED_COMMAND* Cmd,
    IN ULONG SpacePad)
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

    PARSED_COMMAND* Sub;

dump:
    if (!Cmd)
        return;

    /* Space padding */
    ConOutPrintf(_T("%*s"), SpacePad, _T(""));

    switch (Cmd->Type)
    {
    case C_COMMAND:
    case C_REM:
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
EchoCommand(
    IN PARSED_COMMAND* Cmd)
{
    PARSED_COMMAND* Sub;
    REDIRECTION* Redir;

    if (!Cmd)
        return;

    switch (Cmd->Type)
    {
    case C_COMMAND:
    case C_REM:
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
 * Used for pipes. This is basically the same thing as EchoCommand(),
 * but writing into a string instead of to standard output.
 */
PTCHAR
UnparseCommand(
    IN PARSED_COMMAND* Cmd,
    OUT PTCHAR Out,
    IN  PTCHAR OutEnd)
{
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
    Out = UnparseCommand(Subcommand, Out, OutEnd); \
    if (!Out) return NULL; \
} while (0)

    PARSED_COMMAND* Sub;
    REDIRECTION* Redir;

    if (!Cmd)
        return Out;

    switch (Cmd->Type)
    {
    case C_COMMAND:
    case C_REM:
    {
        /* This is fragile since there could be special characters, but
         * Windows doesn't bother escaping them, so for compatibility
         * we probably shouldn't do it either */
        if (!SubstituteForVars(Cmd->Command.First, TempBuf)) return NULL;
        STRING(TempBuf);
        if (!SubstituteForVars(Cmd->Command.Rest, TempBuf)) return NULL;
        STRING(TempBuf);
        break;
    }

    case C_QUIET:
    {
        CHAR(_T('@'));
        RECURSE(Cmd->Subcommands);
        break;
    }

    case C_BLOCK:
    {
        CHAR(_T('('));
        for (Sub = Cmd->Subcommands; Sub; Sub = Sub->Next)
        {
            RECURSE(Sub);
            if (Sub->Next)
                CHAR(_T('&'));
        }
        CHAR(_T(')'));
        break;
    }

    case C_MULTI:
    case C_OR:
    case C_AND:
    case C_PIPE:
    {
        Sub = Cmd->Subcommands;
        RECURSE(Sub);
        PRINTF(_T(" %s "), OpString[Cmd->Type - C_OP_LOWEST]);
        RECURSE(Sub->Next);
        break;
    }

    case C_FOR:
    {
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
    }

    case C_IF:
    {
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
    }

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
