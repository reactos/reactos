/* $Id: ShellCommandSetValue.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// ShellCommandSetValue.h: interface for the CShellCommandSetValue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDSETVALUE_H__32B55193_715E_11D4_A06C_BEFAED86450E__INCLUDED_)
#define SHELLCOMMANDSETVALUE_H__32B55193_715E_11D4_A06C_BEFAED86450E__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandSetValue : public CShellCommand  
{
public:
	CShellCommandSetValue(CRegistryTree& rTree);
	virtual ~CShellCommandSetValue();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDSETVALUE_H__32B55193_715E_11D4_A06C_BEFAED86450E__INCLUDED_)
