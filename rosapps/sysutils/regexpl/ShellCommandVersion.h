// ShellCommandVersion.h: interface for the CShellCommandVersion class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDVERSION_H__D29C1196_5942_11D4_A037_C5AC8D00940F__INCLUDED_)
#define SHELLCOMMANDVERSION_H__D29C1196_5942_11D4_A037_C5AC8D00940F__INCLUDED_

#include "ShellCommand.h"

class CShellCommandVersion : public CShellCommand  
{
public:
	CShellCommandVersion();
	virtual ~CShellCommandVersion();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
};

#endif // !defined(SHELLCOMMANDVERSION_H__D29C1196_5942_11D4_A037_C5AC8D00940F__INCLUDED_)
