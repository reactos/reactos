/* $Id: ShellCommandChangeKey.h,v 1.2 2001/01/13 23:55:36 narnaoud Exp $ */

// ShellCommandChangeKey.h: interface for the CShellCommandChangeKey class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDCHANGEKEY_H__848A2506_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDCHANGEKEY_H__848A2506_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandChangeKey : public CShellCommand  
{
public:
	CShellCommandChangeKey(CRegistryTree& rTree);
	virtual ~CShellCommandChangeKey();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDCHANGEKEY_H__848A2506_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
