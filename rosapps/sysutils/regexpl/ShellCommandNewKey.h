/* $Id: ShellCommandNewKey.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// ShellCommandNewKey.h: interface for the CShellCommandNewKey class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDNEWKEY_H__848A250F_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDNEWKEY_H__848A250F_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandNewKey : public CShellCommand  
{
public:
	CShellCommandNewKey(CRegistryTree& rTree);
	virtual ~CShellCommandNewKey();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDNEWKEY_H__848A250F_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
