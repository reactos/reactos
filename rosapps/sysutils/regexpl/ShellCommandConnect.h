/* $Id: ShellCommandConnect.h,v 1.2 2001/01/13 23:55:36 narnaoud Exp $ */

// ShellCommandConnect.h: interface for the CShellCommandConnect class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDCONNECT_H__848A250C_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDCONNECT_H__848A250C_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandConnect : public CShellCommand  
{
public:
	CShellCommandConnect(CRegistryTree& rTree);
	virtual ~CShellCommandConnect();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDCONNECT_H__848A250C_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
