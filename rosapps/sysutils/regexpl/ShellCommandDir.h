// ShellCommandDir.h: interface for the CShellCommandDir class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDDIR_H__848A2505_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDDIR_H__848A2505_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "RegistryTree.h"

class CShellCommandDir : public CShellCommand  
{
public:
	CShellCommandDir(CRegistryTree& rTree);
	virtual ~CShellCommandDir();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CRegistryTree& m_rTree;
};

#endif // !defined(SHELLCOMMANDDIR_H__848A2505_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
