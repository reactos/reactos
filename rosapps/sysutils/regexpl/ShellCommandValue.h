/* $Id: ShellCommandValue.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// ShellCommandValue.h: interface for the CShellCommandValue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDVALUE_H__848A2507_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDVALUE_H__848A2507_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandValue : public CShellCommand  
{
public:
	CShellCommandValue(CRegistryTree& rTree);
	virtual ~CShellCommandValue();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDVALUE_H__848A2507_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
