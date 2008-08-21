#include <precomp.h>

#define C_OP_LOWEST C_MULTI
#define C_OP_HIGHEST C_PIPE
static const TCHAR OpString[][3] = { _T("&"), _T("||"), _T("&&"), _T("|") };

static const TCHAR RedirString[][3] = { _T("<"), _T(">"), _T(">>") };

static BOOL IsSeparator(TCHAR Char)
{
	/* These three characters act like spaces to the parser */
	return _istspace(Char) || (Char && _tcschr(_T(",;="), Char));
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

//restart:
	/* Although CRs can be injected into a line via an environment
	 * variable substitution, the parser ignores them - they won't
	 * even separate tokens. */
	do
		Char = *ParsePos++;
	while (Char == _T('\r'));

	if (!Char)
	{
		/*if (bLineContinuations)
			if (ReadLine(ParseLine, TRUE) && *(ParsePos = ParseLine))
				goto restart;*/
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
static int ParseToken(TCHAR ExtraEnd, BOOL PreserveSpace)
{
	TCHAR *Out = CurrentToken;
	TCHAR Char = CurChar;
	int Type;
	BOOL bInQuote = FALSE;

	if (!PreserveSpace)
	{
		while (Char != _T('\n') && IsSeparator(Char))
			Char = ParseChar();
	}

	while (Char && Char != _T('\n'))
	{
		bInQuote ^= (Char == _T('"'));
		if (!bInQuote)
		{
			/* Check for all the myriad ways in which this token
			 * may be brought to an untimely end. */
			if ((Char >= _T('0') && Char <= _T('9') &&
			       (ParsePos == &ParseLine[1] || IsSeparator(ParsePos[-2]))
			       && (*ParsePos == _T('<') || *ParsePos == _T('>')))
			    || _tcschr(_T(")&|<>") + (InsideBlock ? 0 : 1), Char)
			    || (!PreserveSpace && IsSeparator(Char))
			    || (Char == ExtraEnd))
			{
				break;
			}

			if (Char == _T('^'))
			{
				Char = ParseChar();
				/* Eat up a \n, allowing line continuation */
				if (Char == _T('\n'))
					Char = ParseChar();
				/* Next character is a forced literal */
			}
		}
		/* FIXME: potential buffer overflow here */
		*Out++ = Char;
		Char = ParseChar();
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
		if (ParseToken(0, FALSE) != TOK_NORMAL)
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
	do
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
	} while (CurrentTokenType != TOK_END_BLOCK);
	InsideBlock--;

	/* Process any trailing redirections */
	while (ParseToken(0, FALSE) == TOK_REDIRECTION)
	{
		if (!ParseRedirection(&Cmd->Redirections))
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
		while (ParseToken(0, TRUE) != TOK_END)
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
		Type = ParseToken(_T('('), FALSE);
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
		else
		{
			ParseError();
			FreeRedirection(RedirList);
			return NULL;
		}
	}
	TailOffset = Pos - ParsedLine;

	/* FIXME: FOR, IF, and REM need special processing by the parser. */

	/* Now get the tail */
	while (1)
	{
		Type = ParseToken(0, TRUE);
		if (Type == TOK_NORMAL)
		{
			/* FIXME: potential buffer overflow here */
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

	Cmd = cmd_alloc(FIELD_OFFSET(PARSED_COMMAND, CommandLine[Pos + 1 - ParsedLine]));
	Cmd->Type = C_COMMAND;
	Cmd->Next = NULL;
	Cmd->Subcommands = NULL;
	Cmd->Redirections = RedirList;
	_tcscpy(Cmd->CommandLine, ParsedLine);
	Cmd->Tail = Cmd->CommandLine + TailOffset;
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
		/*if (!ReadLine(ParseLine, FALSE))*/
			return NULL;
		bLineContinuations = TRUE;
	}
	bParseError = FALSE;
	ParsePos = ParseLine;
	CurChar = _T(' ');

	Cmd = ParseCommandOp(C_OP_LOWEST);
	if (Cmd && CurrentTokenType != TOK_END)
	{
		ParseError();
		FreeCommand(Cmd);
		Cmd = NULL;
	}
	return Cmd;
}

VOID
FreeCommand(PARSED_COMMAND *Cmd)
{
	if (Cmd->Subcommands)
		FreeCommand(Cmd->Subcommands);
	if (Cmd->Next)
		FreeCommand(Cmd->Next);
	FreeRedirection(Cmd->Redirections);
	cmd_free(Cmd);
}
