// ShellCommandSACL.h: interface for the CShellCommandSACL class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDSACL_H__848A250A_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDSACL_H__848A250A_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandSACL : public CShellCommand  
{
public:
	CShellCommandSACL(CRegistryTree& rTree);
	virtual ~CShellCommandSACL();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDSACL_H__848A250A_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
