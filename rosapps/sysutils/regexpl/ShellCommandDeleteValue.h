// ShellCommandDeleteValue.h: interface for the CShellCommandDeleteValue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDDELETEVALUE_H__15DEA2A5_7B3A_11D4_A081_90F9DC5EBD0F__INCLUDED_)
#define SHELLCOMMANDDELETEVALUE_H__15DEA2A5_7B3A_11D4_A081_90F9DC5EBD0F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandDeleteValue : public CShellCommand  
{
public:
	CShellCommandDeleteValue(CRegistryTree& rTree);
	virtual ~CShellCommandDeleteValue();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDDELETEVALUE_H__15DEA2A5_7B3A_11D4_A081_90F9DC5EBD0F__INCLUDED_)
