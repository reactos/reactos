/*
 *  PARSER.C - command parsing.
 *
 */

#include <precomp.h>

#define C_OP_LOWEST C_MULTI
#define C_OP_HIGHEST C_PIPE
static const TCHAR OpString[][3] = { _T("&"), _T("||"), _T("&&"), _T("|") };

static const TCHAR RedirString[][3] = { _T("<"), _T(">"), _T(">>") };

static const TCHAR *const IfOperatorString[] = {
	_T("cmdextversion"),
	_T("defined"),
	_T("errorlevel"),
	_T("exist"),
#define IF_MAX_UNARY IF_EXIST
	_T("=="),
	_T("equ"),
	_T("gtr"),
	_T("geq"),
	_T("lss"),
	_T("leq"),
	_T("neq"),
#define IF_MAX_COMPARISON IF_NEQ
};

/* These three characters act like spaces to the parser in most contexts */
#define STANDARD_SEPS _T(",;=")

static BOOL IsSeparator(TCHAR Char)
{
	return _istspace(Char) || (Char && _tcschr(STANDARD_SEPS, Char));
}

enum { TOK_END, TOK_NORMAL, TOK_OPERATOR, TOK_REDIRECTION,
       TOK_BEGIN_BLOCK, TOK_END_BLOCK };

static BOOL bParseError;
static BOOL bLineContinuations;
static TCHAR ParseLine[CMDLINE_LENGTH];
static TCHAR *ParsePos;
static TCHAR CurChar;

static TCHAR CurrentToken[CMDLINE_LENGTH];
static int CurrentTokenType;
static int InsideBlock;

static TCHAR ParseChar()
{
	TCHAR Char;

	if (bParseError)
		return CurChar = 0;

restart:
	/* Although CRs can be injected into a line via an environment
	 * variable substitution, the parser ignores them - they won't
	 * even separate tokens. */
	do
		Char = *ParsePos++;
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
	return CurChar = Char;
}

static void ParseError()
{
	if (CurrentTokenType == TOK_END)
		ConOutResPuts(STRING_SYNTAX_COMMAND_INCORRECT);
	else
		ConOutPrintf(_T("%s was unexpected at this time.\n"), CurrentToken);
	bParseError = TRUE;
}

/* Yes, cmd has a Lexical Analyzer. Whenever the parser gives an "xxx was
 * unexpected at this time." message, it shows what the last token read was */
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
	return CurrentTokenType = Type;
}

static BOOL ParseRedirection(REDIRECTION **List)
{
	TCHAR *Tok = CurrentToken;
	BYTE Number;
	BYTE RedirType;
	REDIRECTION *Redir;

	if (*Tok >= _T('0') && *Tok <= _T('9'))
		Number = *Tok++ - _T('0');
	else
		Number = *Tok == _T('<') ? 0 : 1;

	if (*Tok++ == _T('<'))
	{
		RedirType = REDIR_READ;
		if (*Tok == _T('<'))
			goto fail;
	}
	else
	{
		RedirType = REDIR_WRITE;
		if (*Tok == _T('>'))
		{
			RedirType = REDIR_APPEND;
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
	Redir->Next = NULL;
	Redir->OldHandle = INVALID_HANDLE_VALUE;
	Redir->Number = Number;
	Redir->Type = RedirType;
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
	PARSED_COMMAND *Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
	memset(Cmd, 0, sizeof(PARSED_COMMAND));
	Cmd->Type = C_IF;

	int Type = ParseToken(0, STANDARD_SEPS);
	if (_tcsicmp(CurrentToken, _T("/I")) == 0)
	{
		Cmd->If.Flags |= IFFLAG_IGNORECASE;
		Type = ParseToken(0, STANDARD_SEPS);
	}
	if (_tcsicmp(CurrentToken, _T("not")) == 0)
	{
		Cmd->If.Flags |= IFFLAG_NEGATE;
		Type = ParseToken(0, STANDARD_SEPS);
	}

	if (Type != TOK_NORMAL)
	{
		FreeCommand(Cmd);
		ParseError();
		return NULL;
	}

	/* Check for unary operators */
	for (; Cmd->If.Operator <= IF_MAX_UNARY; Cmd->If.Operator++)
	{
		if (_tcsicmp(CurrentToken, IfOperatorString[Cmd->If.Operator]) == 0)
		{
			if (ParseToken(0, STANDARD_SEPS) != TOK_NORMAL)
			{
				FreeCommand(Cmd);
				ParseError();
				return NULL;
			}
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

	for (; Cmd->If.Operator <= IF_MAX_COMPARISON; Cmd->If.Operator++)
	{
		if (_tcsicmp(CurrentToken, IfOperatorString[Cmd->If.Operator]) == 0)
		{
			if (ParseToken(0, STANDARD_SEPS) != TOK_NORMAL)
				break;
			Cmd->If.RightArg = cmd_dup(CurrentToken);
			goto condition_done;
		}
	}
	FreeCommand(Cmd);
	ParseError();
	return NULL;

condition_done:
	Cmd->Subcommands = ParseCommandOp(C_OP_LOWEST);
	if (Cmd->Subcommands == NULL)
	{
		FreeCommand(Cmd);
		return NULL;
	}
	if (_tcsicmp(CurrentToken, _T("else")) == 0)
	{
		Cmd->Subcommands->Next = ParseCommandOp(C_OP_LOWEST);
		if (Cmd->Subcommands->Next == NULL)
		{
			FreeCommand(Cmd);
			return NULL;
		}
	}

	return Cmd;
}

static PARSED_COMMAND *ParseCommandPart(void)
{
	TCHAR ParsedLine[CMDLINE_LENGTH];
	TCHAR *Pos;
	DWORD TailOffset;
	PARSED_COMMAND *Cmd;
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
		ParseChar();
		Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
		Cmd->Type = C_QUIET;
		Cmd->Next = NULL;
		/* @ acts like a unary operator with low precedence,
		 * so call the top-level parser */
		Cmd->Subcommands = ParseCommandOp(C_OP_LOWEST);
		Cmd->Redirections = NULL;
		return Cmd;
	}

	/* Get the head of the command */
	while (1)
	{
		Type = ParseToken(_T('('), STANDARD_SEPS);
		if (Type == TOK_NORMAL)
		{
			Pos = _stpcpy(ParsedLine, CurrentToken);
			break;
		}
		else if (Type == TOK_REDIRECTION)
		{
			if (!ParseRedirection(&RedirList))
				return NULL;
		}
		else if (Type == TOK_BEGIN_BLOCK)
		{
			return ParseBlock(RedirList);
		}
		else if (Type == TOK_END_BLOCK && !RedirList)
		{
			return NULL;
		}
		else
		{
			ParseError();
			FreeRedirection(RedirList);
			return NULL;
		}
	}
	TailOffset = Pos - ParsedLine;

	/* Check for special forms */
	if (_tcsicmp(ParsedLine, _T("if")) == 0)
	{
		if (RedirList)
		{
			ParseError();
			FreeRedirection(RedirList);
			return NULL;
		}
		return ParseIf();
	}

	/* Now get the tail */
	while (1)
	{
		Type = ParseToken(0, NULL);
		if (Type == TOK_NORMAL)
		{
			if (Pos + _tcslen(CurrentToken) >= &ParsedLine[CMDLINE_LENGTH])
			{
				ParseError();
				FreeRedirection(RedirList);
				return NULL;
			}
			Pos = _stpcpy(Pos, CurrentToken);
		}
		else if (Type == TOK_REDIRECTION)
		{
			if (!ParseRedirection(&RedirList))
				return NULL;
		}
		else
		{
			break;
		}
	}

	Cmd = cmd_alloc(FIELD_OFFSET(PARSED_COMMAND, Command.CommandLine[Pos + 1 - ParsedLine]));
	Cmd->Type = C_COMMAND;
	Cmd->Next = NULL;
	Cmd->Subcommands = NULL;
	Cmd->Redirections = RedirList;
	_tcscpy(Cmd->Command.CommandLine, ParsedLine);
	Cmd->Command.Tail = Cmd->Command.CommandLine + TailOffset;
	return Cmd;
}

static PARSED_COMMAND *ParseCommandOp(int OpType)
{
	PARSED_COMMAND *Cmd, *Left, *Right;

	if (OpType == C_OP_HIGHEST)
		Cmd = ParseCommandPart();
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
		Cmd->Type = OpType;
		Cmd->Next = NULL;
		Cmd->Redirections = NULL;
		Cmd->Subcommands = Left;
		Left->Next = Right;
		Right->Next = NULL;
	}

	return Cmd;
}

PARSED_COMMAND *
ParseCommand(LPTSTR Line)
{
	PARSED_COMMAND *Cmd;

	if (Line)
	{
		_tcscpy(ParseLine, Line);
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
		if (CurrentTokenType != TOK_END)
			ParseError();
		if (bParseError)
		{
			FreeCommand(Cmd);
			Cmd = NULL;
		}
	}
	return Cmd;
}

/* Reconstruct a parse tree into text form;
 * used for echoing batch file commands */
VOID
EchoCommand(PARSED_COMMAND *Cmd)
{
	PARSED_COMMAND *Sub;
	REDIRECTION *Redir;

	switch (Cmd->Type)
	{
	case C_COMMAND:
		ConOutPrintf(_T("%s"), Cmd->Command.CommandLine);
		break;
	case C_QUIET:
		return;
	case C_BLOCK:
		ConOutChar(_T('('));
		for (Sub = Cmd->Subcommands; Sub; Sub = Sub->Next)
		{
			EchoCommand(Sub);
			ConOutChar(_T('\n'));
		}
		ConOutChar(_T(')'));
		break;
	case C_MULTI:
	case C_IFFAILURE:
	case C_IFSUCCESS:
	case C_PIPE:
		Sub = Cmd->Subcommands;
		EchoCommand(Sub);
		ConOutPrintf(_T(" %s "), OpString[Cmd->Type - C_OP_LOWEST]);
		EchoCommand(Sub->Next);
		break;
	case C_IF:
		ConOutPrintf(_T("if"));
		if (Cmd->If.Flags & IFFLAG_IGNORECASE)
			ConOutPrintf(_T(" /I"));
		if (Cmd->If.Flags & IFFLAG_NEGATE)
			ConOutPrintf(_T(" not"));
		if (Cmd->If.LeftArg)
			ConOutPrintf(_T(" %s"), Cmd->If.LeftArg);
		ConOutPrintf(_T(" %s %s "), IfOperatorString[Cmd->If.Operator], Cmd->If.RightArg);
		Sub = Cmd->Subcommands;
		EchoCommand(Sub);
		if (Sub->Next)
		{
			ConOutPrintf(_T(" else "));
			EchoCommand(Sub->Next);
		}
		break;
	}

	for (Redir = Cmd->Redirections; Redir; Redir = Redir->Next)
	{
		ConOutPrintf(_T(" %c%s%s"), _T('0') + Redir->Number,
			RedirString[Redir->Type], Redir->Filename);
	}
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
	cmd_free(Cmd);
}
