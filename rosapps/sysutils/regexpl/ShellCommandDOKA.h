/* $Id: ShellCommandDOKA.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// ShellCommandDOKA.h: interface for the CShellCommandDOKA class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDDOKA_H__848A250B_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDDOKA_H__848A250B_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandDOKA : public CShellCommand  
{
public:
	CShellCommandDOKA(CRegistryTree& rTree);
	virtual ~CShellCommandDOKA();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDDOKA_H__848A250B_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
