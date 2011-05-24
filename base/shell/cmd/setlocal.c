/*
 *  SETLOCAL.C - setlocal and endlocal internal batch commands.
 *
 *  History:
 *
 *    1 Feb 2008 (Christoph von Wittich)
 *        started.
*/ 

#include <precomp.h>

typedef struct _SETLOCAL {
	struct _SETLOCAL *Prev;
	BOOL DelayedExpansion;
	LPTSTR Environment;
} SETLOCAL;

/* Create a copy of the current environment */
LPTSTR
DuplicateEnvironment(VOID)
{
	LPTSTR Environ = GetEnvironmentStrings();
	LPTSTR End, EnvironCopy;
	if (!Environ)
		return NULL;
	for (End = Environ; *End; End += _tcslen(End) + 1)
		;
	EnvironCopy = cmd_alloc((End + 1 - Environ) * sizeof(TCHAR));
	if (EnvironCopy)
		memcpy(EnvironCopy, Environ, (End + 1 - Environ) * sizeof(TCHAR));
	FreeEnvironmentStrings(Environ);
	return EnvironCopy;
}

INT cmd_setlocal(LPTSTR param)
{
	SETLOCAL *Saved;
	LPTSTR *arg;
	INT argc, i;

	/* SETLOCAL only works inside a batch file */
	if (!bc)
		return 0;

	Saved = cmd_alloc(sizeof(SETLOCAL));
	if (!Saved)
	{
		error_out_of_memory();
		return 1;
	}
	Saved->Prev = bc->setlocal;
	Saved->DelayedExpansion = bDelayedExpansion;
	Saved->Environment = DuplicateEnvironment();
	if (!Saved->Environment)
	{
		error_out_of_memory();
		cmd_free(Saved);
		return 1;
	}
	bc->setlocal = Saved;

	nErrorLevel = 0;

	arg = splitspace(param, &argc);
	for (i = 0; i < argc; i++)
	{
		if (!_tcsicmp(arg[i], _T("enableextensions")))
			/* not implemented, ignore */;
		else if (!_tcsicmp(arg[i], _T("disableextensions")))
			/* not implemented, ignore */;
		else if (!_tcsicmp(arg[i], _T("enabledelayedexpansion")))
			bDelayedExpansion = TRUE;
		else if (!_tcsicmp(arg[i], _T("disabledelayedexpansion")))
			bDelayedExpansion = FALSE;
		else
		{
			error_invalid_parameter_format(arg[i]);
			break;
		}
	}
	freep(arg);

	return nErrorLevel;
}

/* endlocal doesn't take any params */
INT cmd_endlocal(LPTSTR param)
{
	LPTSTR Environ, Name, Value;
	SETLOCAL *Saved;

	/* Pop a SETLOCAL struct off of this batch file's stack */
	if (!bc || !(Saved = bc->setlocal))
		return 0;
	bc->setlocal = Saved->Prev;

	bDelayedExpansion = Saved->DelayedExpansion;

	/* First, clear out the environment. Since making any changes to the
	 * environment invalidates pointers obtained from GetEnvironmentStrings(),
	 * we must make a copy of it and get the variable names from that */
	Environ = DuplicateEnvironment();
	if (Environ)
	{
		for (Name = Environ; *Name; Name += _tcslen(Name) + 1)
		{
			if (!(Value = _tcschr(Name + 1, _T('='))))
				continue;
			*Value++ = _T('\0');
			SetEnvironmentVariable(Name, NULL);
			Name = Value;
		}
		cmd_free(Environ);
	}

	/* Now, restore variables from the copy saved by cmd_setlocal */
	for (Name = Saved->Environment; *Name; Name += _tcslen(Name) + 1)
	{
		if (!(Value = _tcschr(Name + 1, _T('='))))
			continue;
		*Value++ = _T('\0');
		SetEnvironmentVariable(Name, Value);
		Name = Value;
	}

	cmd_free(Saved->Environment);
	cmd_free(Saved);
	return 0;
}

