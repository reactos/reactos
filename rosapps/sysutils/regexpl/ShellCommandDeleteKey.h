// ShellCommandDeleteKey.h: interface for the CShellCommandDeleteKey class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDDELETEKEY_H__848A2510_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDDELETEKEY_H__848A2510_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandDeleteKey : public CShellCommand  
{
public:
	CShellCommandDeleteKey(CRegistryTree& rTree);
	virtual ~CShellCommandDeleteKey();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDDELETEKEY_H__848A2510_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
