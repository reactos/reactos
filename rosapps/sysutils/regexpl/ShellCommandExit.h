/* $Id: ShellCommandExit.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// ShellCommandExit.h: interface for the CShellCommandExit class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDEXIT_H__848A2501_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDEXIT_H__848A2501_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"

class CShellCommandExit : public CShellCommand  
{
public:
	CShellCommandExit();
	virtual ~CShellCommandExit();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
};

#endif // !defined(SHELLCOMMANDEXIT_H__848A2501_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
