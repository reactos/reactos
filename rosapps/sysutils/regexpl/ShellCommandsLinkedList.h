// ShellCommandsLinkedList.h: interface for the CShellCommandsLinkedList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHELLCOMMANDSLINKEDLIST_H__D29C1198_5942_11D4_A037_C5AC8D00940F__INCLUDED_)
#define SHELLCOMMANDSLINKEDLIST_H__D29C1198_5942_11D4_A037_C5AC8D00940F__INCLUDED_

#include "ShellCommand.h"
#include "Console.h"

#define POSITION int *

class CShellCommandsLinkedList  
{
public:
	CShellCommandsLinkedList(CConsole& rConsole);
	virtual ~CShellCommandsLinkedList();
	void AddCommand(CShellCommand *pCommand);
	int Execute(CArgumentParser& rArgumentParser, int& nReturnValue);
	CShellCommand * Match(const TCHAR * pchCommand);
	POSITION GetFirstCommandPosition();
	CShellCommand * GetNextCommand(POSITION& rPos);
private:
	struct SNode
	{
		SNode() {m_pNext = NULL;};
		CShellCommand *m_pData;
		SNode *m_pNext;
	} *m_pRoot;
	CConsole &m_rConsole;
};

#endif // !defined(SHELLCOMMANDSLINKEDLIST_H__D29C1198_5942_11D4_A037_C5AC8D00940F__INCLUDED_)
