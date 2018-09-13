//******************************************************************
// source.cpp
//
// This is file contains the implementation of the CSource class.  
//
// The CSource class acts as a container class for the message source,
// which is composed of the source tree control and the message list.
//
// Author: Larry A. French
//
// History:
//      20-Febuary-1996     Larry A. French
//          Wrote it.
//
//
// Copyright (C) 1995, 1996 Microsoft Corporation.  All rights reserved.
//******************************************************************
#include "stdafx.h"
#include "regkey.h"
#include "source.h"
#include "utils.h"
#include "globals.h"

#include "tcsource.h"
#include "lcsource.h"
#include "evntfind.h"
#include "trapdlg.h"




CSource::CSource()
{
    m_pEventSource = NULL;
    m_ptcSource = NULL;
    m_plcSource = NULL;
    m_pdlgEventTrap = NULL;
    m_pdlgFind = NULL;
}


CSource::~CSource()
{
    delete m_pdlgFind;
}

SCODE CSource::Create(CEventTrapDlg* pdlgEventTrap)
{
	m_ptcSource = &pdlgEventTrap->m_tcSource;
	m_ptcSource->m_pSource = this;

	m_plcSource = &pdlgEventTrap->m_lcSource;
	m_plcSource->m_pSource = this;

    m_pdlgEventTrap = pdlgEventTrap;

    return S_OK;
}



//***************************************************************************
// CSource::NotifyTcSelChanged
//
// This method returns an array of pointers to the messages currently selected
// in the CLcEvents list control.  These pointers are owned by g_aEventLogs and
// the caller should not delete them.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//***************************************************************************
void CSource::GetSelectedMessages(CXMessageArray& aMessages)
{
    m_plcSource->GetSelectedMessages(aMessages);
}



//***************************************************************************
// CSource::NotifyTcSelChanged
//
// This method is called when the selection changes in the event-source tree
// control (CTcSource).  When the selection changes, the message list must
// be updated.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//***************************************************************************
void CSource::NotifyTcSelChanged()
{
	m_pEventSource = m_ptcSource->GetSelectedEventSource();
	m_plcSource->SetEventSource(m_pEventSource);
    m_pdlgEventTrap->NotifySourceSelChanged();
}




//***************************************************************************
//
//  CSource::CreateWindowEpilogue()
//
//  This method is called after a window has been created for this list
//  control.  Final initialization is done here.
//
//  Parameters:
//		None.
//
//  Returns:
//		SCODE
//			S_OK if the initialization was successful, otherwise E_FAIL.
//
//  Status:
//      
//***************************************************************************
SCODE CSource::CreateWindowEpilogue()
{
	SCODE scTc = m_ptcSource->CreateWindowEpilogue();
	SCODE scLc = m_plcSource->CreateWindowEpilogue();

	if (FAILED(scTc) || FAILED(scLc)) {
		return E_FAIL;
	}
	
	return S_OK;
}



//******************************************************************
// CSource::Find
//
// Find the specified event source.  This is done by searching either
// the tree or the list control depending on the bSearchTree parameter.
//
// Parameters:
//		BOOL bSearchTree
//			TRUE if the tree should be searched, otherwise the list control
//			is searched.
//
//		CString& sText
//			A string containing the text to search for.
//
//		BOOL bWholeWord
//			TRUE if this is a "whole word" search.  False if it
//			is OK to match a partial word.
//
//		BOOL bMatchCase
//			TRUE if a case-sensitive comparison should be used.
//
// Returns:
//		BOOL
//			TRUE if the string was found, FALSE otherwise.  If the specified
//			text is found, then the selection is set on the corresponding
//			item, the item is scrolled into view and the focus
//			is set on the item.
//
//******************************************************************
BOOL CSource::Find(BOOL bSearchTree, CString sText, BOOL bWholeWord, BOOL bMatchCase)
{
    
	if (bSearchTree) 
		return m_ptcSource->Find(sText, bWholeWord, bMatchCase);
	else 
		return m_plcSource->Find(sText, bWholeWord, bMatchCase);
}




//************************************************************************
// CSource::OnFind
//
// This method is called when the "Find" button in the CEventTrap dialog
// is clicked. 
//
// Parameters:
//      CWnd* pwndParent
//          Pointer to the parent window of the "find" dialog.  This happens
//          to be the CEventTrapDialog.
//
// Returns:
//      Nothing.
//
//*************************************************************************
void CSource::OnFind(CWnd* pwndParent)
{
    if (m_pdlgFind == NULL) {
        m_pdlgFind = new CEventFindDlg(pwndParent);
        m_pdlgFind->Create(this, IDD_EVENTFINDDLG, pwndParent);
    }


    m_pdlgFind->BringWindowToTop();
}



//*************************************************************************
// CSource::NotifyTrappingChange
//
// This method is called when an event is added or removed from the
// event list.  This CSource message source container must be notified
// so that the corresponding method can be marked as trapped or not
// trapped in the CLcSource list control.
//
// Parameters:
//      CXEventSource* pEventSource
//          Pointer to the event's event-source
//
//      DWORD dwId
//          The event's ID
//
//      BOOL bIsTrapping
//          TRUE if the event is being trapped, FALSE if not.,
//
// Returns:
//      Nothing.
//*************************************************************************
void CSource::NotifyTrappingChange(CXEventSource* pEventSource, DWORD dwId, BOOL bIsTrapping)
{
    if (pEventSource == m_pEventSource) {
        m_plcSource->NotifyTrappingChange(dwId, bIsTrapping);
    }
}

