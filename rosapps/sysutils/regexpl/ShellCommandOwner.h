/* $Id: ShellCommandOwner.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// ShellCommandOwner.h: interface for the CShellCommandOwner class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDOWNER_H__848A2508_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDOWNER_H__848A2508_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandOwner : public CShellCommand  
{
public:
	CShellCommandOwner(CRegistryTree& rTree);
	virtual ~CShellCommandOwner();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDOWNER_H__848A2508_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
