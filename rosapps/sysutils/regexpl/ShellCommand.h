/* $Id: ShellCommand.h,v 1.2 2001/01/13 23:55:36 narnaoud Exp $ */

// ShellCommand.h: interface for the CShellCommand class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMAND_H__D29C1193_5942_11D4_A037_C5AC8D00940F__INCLUDED_)
#define SHELLCOMMAND_H__D29C1193_5942_11D4_A037_C5AC8D00940F__INCLUDED_

#include "Console.h"
#include "ArgumentParser.h"

// this class provides common interface to shell commands
class CShellCommand  
{
public:
	CShellCommand();
	virtual ~CShellCommand();
	virtual BOOL Match(const TCHAR *pchCommand) = 0;
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments) = 0;
	virtual const TCHAR * GetHelpString() = 0;
	virtual const TCHAR * GetHelpShortDescriptionString() = 0;
};

#endif // !defined(SHELLCOMMAND_H__D29C1193_5942_11D4_A037_C5AC8D00940F__INCLUDED_)
