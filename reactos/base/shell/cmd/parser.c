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
	error_syntax(CurrentTokenType != TOK_END ? CurrentToken : NULL);
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

	int Type = CurrentTokenType;
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

/* Parse a FOR command. 
 * Syntax is: FOR [options] %var IN (list) DO command */
static PARSED_COMMAND *ParseFor(void)
{
	PARSED_COMMAND *Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
	TCHAR List[CMDLINE_LENGTH];
	TCHAR *Pos = List;

	memset(Cmd, 0, sizeof(PARSED_COMMAND));
	Cmd->Type = C_FOR;

	while (1)
	{
		if (_tcsicmp(CurrentToken, _T("/D")) == 0)
			Cmd->For.Switches |= FOR_DIRS;
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
			Cmd->For.Switches |= FOR_LOOP;
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
			break;
		ParseToken(0, STANDARD_SEPS);
	}

	/* Make sure there aren't two different switches specified
	 * at the same time, unless they're /D and /R */
	if ((Cmd->For.Switches & (Cmd->For.Switches - 1)) != 0
	    && Cmd->For.Switches != (FOR_DIRS | FOR_RECURSIVE))
	{
		goto error;
	}

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
		int Type;

		/* Pretend we're inside a block so the tokenizer will stop on ')' */
		InsideBlock++;
		Type = ParseToken(0, STANDARD_SEPS);
		InsideBlock--;

		if (Type == TOK_END_BLOCK)
			break;

		if (Type != TOK_NORMAL)
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
	{
		FreeCommand(Cmd);
		return NULL;
	}

	return Cmd;

error:
	FreeCommand(Cmd);
	ParseError();
	return NULL;
}

/* Parse a REM command */
static PARSED_COMMAND *ParseRem(void)
{
	/* Just ignore the rest of the line */
	while (CurChar && CurChar != _T('\n'))
		ParseChar();
	return NULL;
}

static DECLSPEC_NOINLINE PARSED_COMMAND *ParseCommandPart(REDIRECTION *RedirList)
{
	TCHAR ParsedLine[CMDLINE_LENGTH];
	PARSED_COMMAND *Cmd;
	PARSED_COMMAND *(*Func)(void);

	TCHAR *Pos = _stpcpy(ParsedLine, CurrentToken) + 1;
	DWORD TailOffset = Pos - ParsedLine;

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
		int Type = ParseToken(0, NULL);
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
	*Pos++ = _T('\0');

	Cmd = cmd_alloc(FIELD_OFFSET(PARSED_COMMAND, Command.First[Pos - ParsedLine]));
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
		ParseChar();
		PARSED_COMMAND *Cmd = cmd_alloc(sizeof(PARSED_COMMAND));
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
		if (CurrentTokenType != TOK_END)
			ParseError();
		if (bParseError)
		{
			FreeCommand(Cmd);
			Cmd = NULL;
		}
		bIgnoreEcho = FALSE;
	}
	else
	{
		bIgnoreEcho = TRUE;
	}
	return Cmd;
}


/* Reconstruct a parse tree into text form; used for echoing
 * batch file commands and FOR instances. */
VOID
EchoCommand(PARSED_COMMAND *Cmd)
{
	TCHAR Buf[CMDLINE_LENGTH];
	PARSED_COMMAND *Sub;
	REDIRECTION *Redir;

	switch (Cmd->Type)
	{
	case C_COMMAND:
		if (SubstituteForVars(Cmd->Command.First, Buf))
			ConOutPrintf(_T("%s"), Buf);
		if (SubstituteForVars(Cmd->Command.Rest, Buf))
			ConOutPrintf(_T("%s"), Buf);
		break;
	case C_QUIET:
		return;
	case C_BLOCK:
		ConOutChar(_T('('));
		Sub = Cmd->Subcommands;
		if (Sub && !Sub->Next)
		{
			/* Single-command block: display all on one line */
			EchoCommand(Sub);
		}
		else if (Sub)
		{
			/* Multi-command block: display parenthesis on separate lines */
			ConOutChar(_T('\n'));
			do
			{
				EchoCommand(Sub);
				ConOutChar(_T('\n'));
				Sub = Sub->Next;
			} while (Sub);
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
		if (Cmd->If.LeftArg && SubstituteForVars(Cmd->If.LeftArg, Buf))
			ConOutPrintf(_T(" %s"), Buf);
		ConOutPrintf(_T(" %s"), IfOperatorString[Cmd->If.Operator]);
		if (SubstituteForVars(Cmd->If.RightArg, Buf))
			ConOutPrintf(_T(" %s "), Buf);
		Sub = Cmd->Subcommands;
		EchoCommand(Sub);
		if (Sub->Next)
		{
			ConOutPrintf(_T(" else "));
			EchoCommand(Sub->Next);
		}
		break;
	case C_FOR:
		ConOutPrintf(_T("for"));
		if (Cmd->For.Switches & FOR_DIRS)      ConOutPrintf(_T(" /D"));
		if (Cmd->For.Switches & FOR_F)         ConOutPrintf(_T(" /F"));
		if (Cmd->For.Switches & FOR_LOOP)      ConOutPrintf(_T(" /L"));
		if (Cmd->For.Switches & FOR_RECURSIVE) ConOutPrintf(_T(" /R"));
		if (Cmd->For.Params)
			ConOutPrintf(_T(" %s"), Cmd->For.Params);
		ConOutPrintf(_T(" %%%c in (%s) do "), Cmd->For.Variable, Cmd->For.List);
		EchoCommand(Cmd->Subcommands);
		break;
	}

	for (Redir = Cmd->Redirections; Redir; Redir = Redir->Next)
	{
		if (SubstituteForVars(Redir->Filename, Buf))
			ConOutPrintf(_T(" %c%s%s"), _T('0') + Redir->Number,
				RedirString[Redir->Type], Buf);
	}
}

/* "Unparse" a command into a text form suitable for passing to CMD /C.
 * Used for pipes. This is basically the same thing as EchoCommand, but
 * writing into a string instead of to standard output. */
TCHAR *
Unparse(PARSED_COMMAND *Cmd, TCHAR *Out, TCHAR *OutEnd)
{
	TCHAR Buf[CMDLINE_LENGTH];
	PARSED_COMMAND *Sub;
	REDIRECTION *Redir;

/* Since this function has the annoying requirement that it must avoid
 * overflowing the supplied buffer, define some helper macros to make
 * this less painful */
#define CHAR(Char) { \
	if (Out == OutEnd) return NULL; \
	*Out++ = Char; }
#define STRING(String) { \
	if (Out + _tcslen(String) > OutEnd) return NULL; \
	Out = _stpcpy(Out, String); }
#define PRINTF(Format, ...) { \
	UINT Len = _sntprintf(Out, OutEnd - Out, Format, __VA_ARGS__); \
	if (Len > (UINT)(OutEnd - Out)) return NULL; \
	Out += Len; }
#define RECURSE(Subcommand) { \
	Out = Unparse(Subcommand, Out, OutEnd); \
	if (!Out) return NULL; }

	switch (Cmd->Type)
	{
	case C_COMMAND:
		/* This is fragile since there could be special characters, but
		 * Windows doesn't bother escaping them, so for compatibility
		 * we probably shouldn't do it either */
		if (!SubstituteForVars(Cmd->Command.First, Buf)) return NULL;
		STRING(Buf)
		if (!SubstituteForVars(Cmd->Command.Rest, Buf)) return NULL;
		STRING(Buf)
		break;
	case C_QUIET:
		CHAR(_T('@'))
		RECURSE(Cmd->Subcommands)
		break;
	case C_BLOCK:
		CHAR(_T('('))
		for (Sub = Cmd->Subcommands; Sub; Sub = Sub->Next)
		{
			RECURSE(Sub)
			if (Sub->Next)
				CHAR(_T('&'))
		}
		CHAR(_T(')'))
		break;
	case C_MULTI:
	case C_IFFAILURE:
	case C_IFSUCCESS:
	case C_PIPE:
		Sub = Cmd->Subcommands;
		RECURSE(Sub)
		PRINTF(_T(" %s "), OpString[Cmd->Type - C_OP_LOWEST])
		RECURSE(Sub->Next)
		break;
	case C_IF:
		STRING(_T("if"))
		if (Cmd->If.Flags & IFFLAG_IGNORECASE)
			STRING(_T(" /I"))
		if (Cmd->If.Flags & IFFLAG_NEGATE)
			STRING(_T(" not"))
		if (Cmd->If.LeftArg && SubstituteForVars(Cmd->If.LeftArg, Buf))
			PRINTF(_T(" %s"), Buf)
		PRINTF(_T(" %s"), IfOperatorString[Cmd->If.Operator]);
		if (!SubstituteForVars(Cmd->If.RightArg, Buf)) return NULL;
		PRINTF(_T(" %s "), Buf)
		Sub = Cmd->Subcommands;
		RECURSE(Sub)
		if (Sub->Next)
		{
			STRING(_T(" else "))
			RECURSE(Sub->Next)
		}
		break;
	case C_FOR:
		STRING(_T("for"))
		if (Cmd->For.Switches & FOR_DIRS)      STRING(_T(" /D"))
		if (Cmd->For.Switches & FOR_F)         STRING(_T(" /F"))
		if (Cmd->For.Switches & FOR_LOOP)      STRING(_T(" /L"))
		if (Cmd->For.Switches & FOR_RECURSIVE) STRING(_T(" /R"))
		if (Cmd->For.Params)
			PRINTF(_T(" %s"), Cmd->For.Params)
		PRINTF(_T(" %%%c in (%s) do "), Cmd->For.Variable, Cmd->For.List)
		RECURSE(Cmd->Subcommands)
		break;
	}

	for (Redir = Cmd->Redirections; Redir; Redir = Redir->Next)
	{
		if (!SubstituteForVars(Redir->Filename, Buf)) return NULL;
		PRINTF(_T(" %c%s%s"), _T('0') + Redir->Number,
			RedirString[Redir->Type], Buf)
	}
	return Out;
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
