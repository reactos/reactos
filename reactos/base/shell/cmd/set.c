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
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_SET


/* initial size of environment variable buffer */
#define ENV_BUFFER_SIZE  1024

static BOOL
seta_eval ( LPCTSTR expr );

static LPCTSTR
skip_ws ( LPCTSTR p )
{
	return p + _tcsspn ( p, _T(" \t") );
}

INT cmd_set (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	INT i;
	LPTSTR p;

	if ( !_tcsncmp (param, _T("/?"), 2) )
	{
		ConOutResPaging(TRUE,STRING_SET_HELP);
		return 0;
	}

	/* remove escapes */
	if ( param[0] ) for ( i = 0; param[i+1]; i++ )
	{
		if ( param[i] == _T('^') )
		{
			memmove ( &param[i], &param[i+1], _tcslen(&param[i]) * sizeof(TCHAR) );
		}
	}

	/* if no parameters, show the environment */
	if (param[0] == _T('\0'))
	{
		LPTSTR lpEnv;
		LPTSTR lpOutput;
		INT len;

		lpEnv = (LPTSTR)GetEnvironmentStrings ();
		if (lpEnv)
		{
			lpOutput = lpEnv;
			while (*lpOutput)
			{
				len = _tcslen(lpOutput);
				if (len)
				{
					if (*lpOutput != _T('='))
						ConOutPuts (lpOutput);
					lpOutput += (len + 1);
				}
			}
			FreeEnvironmentStrings (lpEnv);
		}

		return 0;
	}

	/* the /A does *NOT* have to be followed by a whitespace */
	if ( !_tcsnicmp (param, _T("/A"), 2) )
	{
		BOOL Success = seta_eval ( skip_ws(param+2) );
		if(!Success)
		{
			/*might seem random but this is what windows xp does */
			nErrorLevel = 9165;
		}
		/* TODO FIXME - what are we supposed to return? */
		return Success;
	}

	p = _tcschr (param, _T('='));
	if (p)
	{
		/* set or remove environment variable */
		if (p == param)
		{
			/* handle set =val case */
			LoadString(CMD_ModuleHandle, STRING_SYNTAX_COMMAND_INCORRECT, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf (szMsg, param);
			return 0;
		}

		*p = _T('\0');
		p++;
		if (*p == _T('\0'))
		{
			p = NULL;
		}
		SetEnvironmentVariable (param, p);
	}
	else
	{
		/* display environment variable */
		LPTSTR pszBuffer;
		DWORD dwBuffer;

		pszBuffer = (LPTSTR)cmd_alloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
		dwBuffer = GetEnvironmentVariable (param, pszBuffer, ENV_BUFFER_SIZE);
		if (dwBuffer == 0)
		{
			LoadString(CMD_ModuleHandle, STRING_PATH_ERROR, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf (szMsg, param);
			return 0;
		}
		else if (dwBuffer > ENV_BUFFER_SIZE)
		{
			pszBuffer = (LPTSTR)cmd_realloc (pszBuffer, dwBuffer * sizeof (TCHAR));
			GetEnvironmentVariable (param, pszBuffer, dwBuffer);
		}
		ConOutPrintf (_T("%s\n"), pszBuffer);

		cmd_free (pszBuffer);

		return 0;
	}

	return 0;
}

static INT
ident_len ( LPCTSTR p )
{
	LPCTSTR p2 = p;
	if ( __iscsymf(*p) )
	{
		++p2;
		while ( __iscsym(*p2) )
			++p2;
	}
	return p2-p;
}

#define PARSE_IDENT(ident,identlen,p) \
	identlen = ident_len(p); \
	ident = (LPTSTR)alloca ( ( identlen + 1 ) * sizeof(TCHAR) ); \
	memmove ( ident, p, identlen * sizeof(TCHAR) ); \
	ident[identlen] = 0; \
	p += identlen;

static BOOL
seta_identval ( LPCTSTR ident, INT* result )
{
	LPCTSTR identVal = GetEnvVarOrSpecial ( ident );
	if ( !identVal )
	{
		/* TODO FIXME - what to do upon failure? */
		*result = 0;
		return FALSE;
	}
	*result = _ttoi ( identVal );
	return TRUE;
}

static BOOL
calc ( INT* lval, TCHAR op, INT rval )
{
	switch ( op )
	{
	case '*':
		*lval *= rval;
		break;
	case '/':
		*lval /= rval;
		break;
	case '%':
		*lval %= rval;
		break;
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
		ConErrResPuts ( STRING_INVALID_OPERAND );
		return FALSE;
	}
	return TRUE;
}

static BOOL
seta_stmt ( LPCTSTR* p_, INT* result );

static BOOL
seta_unaryTerm ( LPCTSTR* p_, INT* result )
{
	LPCTSTR p = *p_;
	if ( *p == _T('(') )
	{
		INT rval;
		p = skip_ws ( p + 1 );
		if ( !seta_stmt ( &p, &rval ) )
			return FALSE;
		if ( *p != _T(')') )
		{
			ConErrResPuts ( STRING_EXPECTED_CLOSE_PAREN );
			return FALSE;
		}
		*result = rval;
		p = skip_ws ( p + 1 );
	}
	else if ( isdigit(*p) )
	{
		*result = _ttoi ( p );
		p = skip_ws ( p + _tcsspn ( p, _T("1234567890") ) );
	}
	else if ( __iscsymf(*p) )
	{
		LPTSTR ident;
		INT identlen;
		PARSE_IDENT(ident,identlen,p);
		if ( !seta_identval ( ident, result ) )
			return FALSE;
	}
	else
	{
		ConErrResPuts ( STRING_EXPECTED_NUMBER_OR_VARIABLE );
		return FALSE;
	}
	*p_ = p;
	return TRUE;
}

static BOOL
seta_mulTerm ( LPCTSTR* p_, INT* result )
{
	LPCTSTR p = *p_;
	TCHAR op = 0;
	INT rval;
	if ( _tcschr(_T("!~-"),*p) )
	{
		op = *p;
		p = skip_ws ( p + 1 );
	}
	if ( !seta_unaryTerm ( &p, &rval ) )
		return FALSE;
	switch ( op )
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
	}

	*result = rval;
	*p_ = p;
	return TRUE;
}

static BOOL
seta_ltorTerm ( LPCTSTR* p_, INT* result, LPCTSTR ops, BOOL (*subTerm)(LPCTSTR*,INT*) )
{
	LPCTSTR p = *p_;
	INT lval;
	if ( !subTerm ( &p, &lval ) )
		return FALSE;
	while ( *p && _tcschr(ops,*p) )
	{
		INT rval;
		TCHAR op = *p;

		p = skip_ws ( p+1 );

		if ( !subTerm ( &p, &rval ) )
			return FALSE;

		if ( !calc ( &lval, op, rval ) )
			return FALSE;
	}

	*result = lval;
	*p_ = p;
	return TRUE;
}

static BOOL
seta_addTerm ( LPCTSTR* p_, INT* result )
{
	return seta_ltorTerm ( p_, result, _T("*/%"), seta_mulTerm );
}

static BOOL
seta_logShiftTerm ( LPCTSTR* p_, INT* result )
{
	return seta_ltorTerm ( p_, result, _T("+-"), seta_addTerm );
}

static BOOL
seta_bitAndTerm ( LPCTSTR* p_, INT* result )
{
	LPCTSTR p = *p_;
	INT lval;
	if ( !seta_logShiftTerm ( &p, &lval ) )
		return FALSE;
	while ( *p && _tcschr(_T("<>"),*p) && p[0] == p[1] )
	{
		INT rval;
		TCHAR op = *p;

		p = skip_ws ( p+2 );

		if ( !seta_logShiftTerm ( &p, &rval ) )
			return FALSE;

		switch ( op )
		{
		case '<':
			lval <<= rval;
			break;
		case '>':
			lval >>= rval;
			break;
		default:
			ConErrResPuts ( STRING_INVALID_OPERAND );
			return FALSE;
		}
	}

	*result = lval;
	*p_ = p;
	return TRUE;
}

static BOOL
seta_bitExclOrTerm ( LPCTSTR* p_, INT* result )
{
	return seta_ltorTerm ( p_, result, _T("&"), seta_bitAndTerm );
}

static BOOL
seta_bitOrTerm ( LPCTSTR* p_, INT* result )
{
	return seta_ltorTerm ( p_, result, _T("^"), seta_bitExclOrTerm );
}

static BOOL
seta_expr ( LPCTSTR* p_, INT* result )
{
	return seta_ltorTerm ( p_, result, _T("|"), seta_bitOrTerm );
}

static BOOL
seta_assignment ( LPCTSTR* p_, INT* result )
{
	LPCTSTR p = *p_;
	LPTSTR ident;
	TCHAR op = 0;
	INT identlen, exprval;

	PARSE_IDENT(ident,identlen,p);
	if ( identlen )
	{
		if ( *p == _T('=') )
			op = *p, p = skip_ws(p+1);
		else if ( _tcschr ( _T("*/%+-&^|"), *p ) && p[1] == _T('=') )
			op = *p, p = skip_ws(p+2);
		else if ( _tcschr ( _T("<>"), *p ) && *p == p[1] && p[2] == _T('=') )
			op = *p, p = skip_ws(p+3);
	}

	/* allow to chain multiple assignments, such as: a=b=1 */
	if ( ident && op )
	{
		INT identval;
		LPTSTR buf;

		if ( !seta_assignment ( &p, &exprval ) )
			return FALSE;

		if ( !seta_identval ( ident, &identval ) )
			identval = 0;
		switch ( op )
		{
		case '=':
			identval = exprval;
			break;
		case '<':
			identval <<= exprval;
			break;
		case '>':
			identval >>= exprval;
			break;
		default:
			if ( !calc ( &identval, op, exprval ) )
				return FALSE;
		}
		buf = (LPTSTR)alloca ( 32 * sizeof(TCHAR) );
		_sntprintf ( buf, 32, _T("%i"), identval );
		SetEnvironmentVariable ( ident, buf ); // TODO FIXME - check return value
		exprval = identval;
	}
	else
	{
		/* restore p in case we found an ident but not an op */
		p = *p_;
		if ( !seta_expr ( &p, &exprval ) )
			return FALSE;
	}

	*result = exprval;
	*p_ = p;
	return TRUE;
}

static BOOL
seta_stmt ( LPCTSTR* p_, INT* result )
{
	LPCTSTR p = *p_;
	INT rval;

	if ( !seta_assignment ( &p, &rval ) )
		return FALSE;
	while ( *p == _T(',') )
	{
		p = skip_ws ( p+1 );

		if ( !seta_assignment ( &p, &rval ) )
			return FALSE;
	}

	*result = rval;
	*p_ = p;
	return TRUE;
}

static BOOL
seta_eval ( LPCTSTR p )
{
	INT rval;
	if ( !*p )
	{
		ConErrResPuts ( STRING_SYNTAX_COMMAND_INCORRECT );
		return FALSE;
	}
	if ( !seta_stmt ( &p, &rval ) )
		return FALSE;
	ConOutPrintf ( _T("%i"), rval );
	return TRUE;
}

#endif
