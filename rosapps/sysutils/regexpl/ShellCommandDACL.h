/* $Id: ShellCommandDACL.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// ShellCommandDACL.h: interface for the CShellCommandDACL class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDDACL_H__848A2509_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDDACL_H__848A2509_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandDACL : public CShellCommand  
{
public:
	CShellCommandDACL(CRegistryTree& rTree);
	virtual ~CShellCommandDACL();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDDACL_H__848A2509_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
