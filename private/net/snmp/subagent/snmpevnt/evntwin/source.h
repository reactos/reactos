
#ifndef _source_h
#define _source_h

#include "evntfind.h"

class CTcSource;
class CLcSource;
class CMessage;
class CEventFindDlg;
class CXMessageArray;
class CXEventSource;
class CEventTrapDlg;

class CSource
{
public:
	CSource();	
    ~CSource();
    SCODE Create(CEventTrapDlg* pdlgEventTrap);
	SCODE CreateWindowEpilogue();
	BOOL Find(BOOL bSearchTree, CString sText, BOOL bWholeWord, BOOL bMatchCase);
    void OnFind(CWnd* pwndParent);
    void GetSelectedMessages(CXMessageArray& aMessages);
    void NotifyTrappingChange(CXEventSource* pEventSource, DWORD dwId, BOOL bIsTrapping);
	void NotifyTcSelChanged();
    CXEventSource* m_pEventSource;

private:
	CLcSource* m_plcSource;
	CTcSource* m_ptcSource;
    CEventTrapDlg* m_pdlgEventTrap;

    friend class CEventFindDlg;
    CEventFindDlg* m_pdlgFind;
};


#endif _source_h

