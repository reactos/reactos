/* $Id: ShellCommandHelp.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// ShellCommandHelp.h: interface for the CShellCommandHelp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDHELP_H__848A2504_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
#define SHELLCOMMANDHELP_H__848A2504_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_

#include "ShellCommand.h"
#include "ShellCommandsLinkedList.h"

class CShellCommandHelp : public CShellCommand  
{
public:
	CShellCommandHelp(CShellCommandsLinkedList& rCommandsLinkedList);
	virtual ~CShellCommandHelp();
	virtual BOOL Match(const TCHAR *pchCommand);
	virtual int Execute(CConsole &rConsole, CArgumentParser& rArguments);
	virtual const TCHAR * GetHelpString();
	virtual const TCHAR * GetHelpShortDescriptionString();
private:
	CShellCommandsLinkedList& m_rCommandsLinkedList;
};

#endif // !defined(SHELLCOMMANDHELP_H__848A2504_5A0F_11D4_A039_FC2CE602E70F__INCLUDED_)
