// lcevents.cpp : implementation file
//

#include "stdafx.h"
#include "eventrap.h"
#include "lcevents.h"
#include "settings.h"
#include "source.h"
#include "globals.h"
#include "utils.h"
#include "lcsource.h"
#include "busy.h"
#include "trapreg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif




/////////////////////////////////////////////////////////////////////////////
// CLcEvents

CLcEvents::CLcEvents()
{
    m_dwSortColumn = ICOL_LcEvents_LOG;
    m_cxWidestMessage = CX_DEFAULT_DESCRIPTION_WIDTH;
}

CLcEvents::~CLcEvents()
{
}


BEGIN_MESSAGE_MAP(CLcEvents, CListCtrl)
	//{{AFX_MSG_MAP(CLcEvents)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


SCODE CLcEvents::CreateWindowEpilogue()
{
	ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_FULLROWSELECT);
	SetColumnHeadings();
    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// CLcEvents message handlers





//***************************************************************************
//  CLcEvents::SelectEvents
//
//  Select the specified events in the list control.
//
//  Parameters:
//		CXEventArray& aEvents
//          An array of event pointers.
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
void CLcEvents::SelectEvents(CXEventArray& aEventsSel)
{
    int iItemFirstSelection = -1;
    LONG nItems = GetSize();
    for (LONG iItem = 0; iItem < nItems; ++iItem) {
        CXEvent* pEventTrapping = GetAt(iItem);

        // If the event associated with this item is in aEvents, then select the item.
        // Otherwise clear selection on the item.
        BOOL bDidFindEvent = FALSE;
        LONG nEventsSel = aEventsSel.GetSize();
        for (LONG iEventSel = 0; iEventSel < nEventsSel; ++iEventSel) {
            CXEvent* pEventSel;
            pEventSel = aEventsSel[iEventSel];
            if ((pEventSel->m_message.m_dwId == pEventTrapping->m_message.m_dwId) &&
                (pEventSel->m_pEventSource == pEventTrapping->m_pEventSource) &&
                (pEventSel->m_pEventSource->m_pEventLog == pEventTrapping->m_pEventSource->m_pEventLog)) {

                bDidFindEvent = TRUE;
                if (iItemFirstSelection == -1) {
                    iItemFirstSelection = iItem;
                }
                break;
            }
        }

        SetItemState(iItem, bDidFindEvent ? LVIS_SELECTED : 0, LVIS_SELECTED);
    }

    // Scroll the first selected item into view.
    if (iItemFirstSelection > 0) {
        EnsureVisible(iItemFirstSelection, FALSE);
    }
}






//***************************************************************************
//
//  CLcEvents::SetColumnHeadings
//
//  Define's the columns for this list control.  The column title, width, and
//  order is defined here.
//
//  Parameters:
//		None.
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
void CLcEvents::SetColumnHeadings()
{
 	static UINT auiResColumnTitle[ICOL_LcEvents_MAX] = {
        IDS_LcEvents_TITLE_LOG,
        IDS_LcEvents_TITLE_SOURCE,
		IDS_LcEvents_TITLE_ID,
		IDS_LcEvents_TITLE_SEVERITY,
        IDS_LcEvents_TITLE_COUNT,
        IDS_LcEvents_TITLE_TIME,
		IDS_LcEvents_TITLE_DESCRIPTION
	};

	static int aiColWidth[ICOL_LcEvents_MAX] = {75, 60, 60, 60, 50, 50, CX_DEFAULT_DESCRIPTION_WIDTH};


    // Build the columns in the AllEventsList control.
    LV_COLUMN lvcol;
    lvcol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    for (int iCol=0; iCol<ICOL_LcEvents_MAX; ++iCol)
    {
		CString sColTitle;
		sColTitle.LoadString(auiResColumnTitle[iCol]);

        lvcol.pszText = sColTitle.GetBuffer(sColTitle.GetLength());
        lvcol.iSubItem = iCol;
        lvcol.cx = aiColWidth[iCol];
        InsertColumn(iCol, &lvcol);
		sColTitle.ReleaseBuffer();
    }
}



//********************************************************************
// CLcEvents::AddEvents
//
// Add all the events for all the event sources contained in the
// event-log array.   The source is notified that each of these
// events is being trapped.
//
// Parameters:
//      CSource& source
//          The message source container.
//
//      CEventLogArray& aEventLogs
//          An array of event-logs.
//
// Returns:
//      Nothing.
//
//*******************************************************************
void CLcEvents::AddEvents(CSource& source, CXEventLogArray& aEventLogs)
{
    // Iterate though all the event logs.
    LONG nLogs = aEventLogs.GetSize();
    for (LONG iLog=0; iLog < nLogs; ++iLog) {
        CXEventLog* pEventLog = aEventLogs[iLog];

        // Iterate through all the event sources within this event log
        LONG nSources = pEventLog->m_aEventSources.GetSize();
        for (LONG iSource = 0; iSource < nSources; ++iSource) {

            // Add all the events for the source to this list control.
            CXEventSource* pEventSource = pEventLog->m_aEventSources[iSource];
            AddEvents(source, pEventSource->m_aEvents);
        }
    }

	if (GetSize() > 0 && !HasSelection())
	{
		SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	}
}





//***************************************************************************
//
//  CLcEvents::AddEvents
//
//  Add an array of events to this list control.  This involves the following
//      a. Add each event to the list control
//      b. Notify the CLcSource that the event has been modified so that it
//         can update the trapping flag.
//      c. Sort the events by the most recently selected column.
//      d. Make sure that the first item in CEventArray passed in is visible.
//
//  Parameters:
//      CSource& source
//          A reference to the CSource object.  This object must be notified
//          when the trapping status of an event changes.
//
//		CEventArray& aEvents
//          An array containing pointers to the events to add.  This list control
//          then becomes the owner of these events.
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
void CLcEvents::AddEvents(CSource& source, CXEventArray& aEvents)
{
    CBusy busy;

    // Now add them into this list control.  This is where they actually
    LONG nEvents = aEvents.GetSize();
    LONG iEvent;

    // Unselect all the previous items first
    iEvent = -1;
    do
    {
        iEvent = GetNextItem(iEvent, LVNI_SELECTED);
        if (iEvent == -1)
            break;
        SetItemState(iEvent, ~LVIS_SELECTED, LVIS_SELECTED);
    } while (TRUE);

    for (iEvent = 0; iEvent < nEvents; ++iEvent) {
        if ((iEvent < 40 && (iEvent % 10 == 9)) ||
            (iEvent % 100 == 99)) {
            UpdateWindow();
        }

        CXEvent* pEvent = aEvents[iEvent];
        AddEvent(pEvent);
        source.NotifyTrappingChange(pEvent->m_pEventSource, pEvent->m_message.m_dwId, TRUE);
    }

    UpdateDescriptionWidth();

    // Sort the items by the most recently selected column, and then
    // make sure the first item is visible.
    SortItems(m_dwSortColumn);
    if (nEvents > 0) {
        iEvent = FindEvent(aEvents[0]);
        EnsureVisible(iEvent, TRUE);
    }
}


//***************************************************************************
//
//  CLcEvents::AddEvent
//
//  Add an event to the list control. This sets the text for each column in
//  the list view and sets the lParam field of the list-view item to pEvent
//
//
//  Parameters:
//		CEvent* pEvent
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
LONG CLcEvents::AddEvent(CXEvent* pEvent)
{
    // Insert a new item into this list control.
    LV_ITEM lvitem;
    lvitem.mask = LVIF_TEXT | LVIF_PARAM;
    lvitem.iSubItem = ICOL_LcEvents_LOG;
    lvitem.lParam = (LPARAM)pEvent;
    lvitem.cchTextMax = pEvent->m_message.m_sText.GetLength() + 1;
    lvitem.pszText = (LPTSTR)(void*)(LPCTSTR) (pEvent->m_message.m_sText);
    LONG nItem = CListCtrl::InsertItem(&lvitem);

    SetItem(nItem, pEvent);
    SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
    return nItem;
}



//********************************************************************
// CLcEvents::SetItem
//
// Refresh an item from an event.
//
// Parameters:
//      LONG nItem
//
//      CEvent* pEvent
//          Pointer to the event to copy the data from.
//
// Returns:
//      Nothing.
//
//*******************************************************************
void CLcEvents::SetItem(LONG nItem, CXEvent* pEvent)
{


    // Check the item index against the array bounds.
    if (nItem < 0 || nItem >= GetItemCount()) {
        ASSERT(FALSE);
        return;
    }

    ASSERT(GetItemData(nItem) == (DWORD) (void*) pEvent);

    // Get the pointer for brevity.
    CXEventSource* pEventSource = pEvent->m_pEventSource;
	CString sText;

    SetItemData(nItem, (DWORD_PTR) (void*) pEvent);

    SetItemText(nItem, ICOL_LcEvents_LOG, (LPTSTR) (LPCTSTR) pEventSource->m_pEventLog->m_sName);

	SetItemText(nItem, ICOL_LcEvents_SOURCE, (LPTSTR)(LPCTSTR) pEventSource->m_sName);

    pEvent->m_message.GetShortId(sText);
    SetItemText(nItem, ICOL_LcEvents_ID, (LPTSTR)(LPCTSTR)sText);

    pEvent->m_message.GetSeverity(sText);
    SetItemText(nItem, ICOL_LcEvents_SEVERITY, (LPTSTR)(LPCTSTR)sText);

    pEvent->GetCount(sText);
    SetItemText(nItem, ICOL_LcEvents_COUNT, (LPTSTR)(LPCTSTR)sText);

    pEvent->GetTimeInterval(sText);
    SetItemText(nItem, ICOL_LcEvents_TIME, (LPTSTR)(LPCTSTR)sText);

    SetItemText(nItem, ICOL_LcEvents_DESCRIPTION, (LPTSTR)(LPCTSTR)pEvent->m_message.m_sText);

}



//***************************************************************************
//
//  CLcEvents::DeleteSelectedEvents.
//
//  Delete all of the currently selected events and the corresponding items.
//
//  Parameters:
//      None.
//
//  Returns:
//      Nothing.
//
//  Status:
//
//***************************************************************************
void CLcEvents::DeleteSelectedEvents(CSource& source)
{

    // Delete all the selected items from the list control.
    // Build an array of event pointers corresponding to the events that are selected
    // in the list control.  Also notify the event source view that the event is no
    // longer being trapped.
   	while (TRUE) {
		int iItem = GetNextItem(-1, LVNI_SELECTED);
		if (iItem == -1) {
			break;
		}
        CXEvent* pEvent = GetAt(iItem);
        DeleteItem(iItem);
        source.NotifyTrappingChange(pEvent->m_pEventSource, pEvent->m_message.m_dwId, FALSE);
        delete pEvent;
	}
    UpdateDescriptionWidth();
}



//***************************************************************************
//
//  CLcEvents::GetAt
//
//  This method returns the event pointer located at the given item index.
//  This allows CLcEvents to be used much as an array.
//
//  Parameters:
//		LONG iItem
//			The item index.
//
//  Returns:
//		A pointer to the CEvent stored at the specified index.
//
//  Status:
//
//***************************************************************************
CXEvent* CLcEvents::GetAt(LONG iItem)
{

	// Setup the LV_ITEM structure to retrieve the lparam field.
	// This field contains the CMessage pointer.
    LV_ITEM lvitem;
    lvitem.mask = LVIF_PARAM;
    lvitem.iSubItem = ICOL_LcEvents_LOG;	
    lvitem.iItem = iItem;
    GetItem(&lvitem);

	CXEvent* pEvent = (CXEvent*) (void*) lvitem.lParam;
	return pEvent;
}










//***************************************************************************
//
//  CLcEvents::GetSelectedEvents
//
//  Get the events corresponding to the selected items in this list control.
//  This list control continues to own the event pointers.
//
//  Parameters:
//		CEventArray& aEvents
//			A reference to the event array where the event pointers are returned.
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
void CLcEvents::GetSelectedEvents(CXEventArray& aEvents)
{

	// Clear the message array
	aEvents.RemoveAll();

	// Setup the LV_ITEM structure to retrieve the lparam field.
	// This field contains the CMessage pointer.
    LV_ITEM lvitem;
    lvitem.mask = LVIF_PARAM;
    lvitem.iSubItem = ICOL_LcEvents_LOG;

	// Loop to find all the selected items.	
	int nItem = -1;
	while (TRUE) {
		nItem = GetNextItem(nItem, LVNI_SELECTED);
		if (nItem == -1) {
			break;
		}

		// Get the CMessage pointer for this item and add it to the
		// array.
        lvitem.iItem = nItem;
        GetItem(&lvitem);
		CXEvent* pEvent = (CXEvent*) (void*) lvitem.lParam;
		aEvents.Add(pEvent);
	}
}



//***************************************************************************
//
//  CLcEvents::FindEvent
//
//  Find the specified event and return its item number.
//
//  Parameters:
//		CEvent* pEvent
//			A pointer to the event to search for.
//
//  Returns:
//		The item index if the item was found, otherwise -1.
//
//  Status:
//
//***************************************************************************
LONG CLcEvents::FindEvent(CXEvent* pEvent)
{

    LONG nEvents = GetItemCount();
    for (LONG iEvent = 0; iEvent < nEvents; ++iEvent) {
        CXEvent* pEventTemp = GetAt(iEvent);
        if (pEventTemp == pEvent) {
            return iEvent;
        }
    }
    return -1;
}



//***************************************************************************
//
//  CLcEvents::RefreshEvents
//
//  This method is called when the properties of some number of events
//  have changed and the corresponding items in the list control need
//  to be updated.
//
//  Parameters:
//		CEventArray& aEvents
//			The events that need to be refreshed.
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
void CLcEvents::RefreshEvents(CXEventArray& aEvents)
{
    // Iterate through each of the events and refresh them.
    LONG nEvents = aEvents.GetSize();
    for (LONG iEvent = 0; iEvent < nEvents; ++iEvent) {
        CXEvent* pEvent = aEvents[iEvent];
        LONG nEvent = FindEvent(pEvent);
        SetItem(nEvent, pEvent);
    }
}





int CALLBACK CompareEventsProc(LPARAM lParam1, LPARAM lParam2, LPARAM
   lParamSort)
{
    CXEvent* pEvent1 = (CXEvent *)lParam1;
    CXEventSource* pEventSource1 = pEvent1->m_pEventSource;

    CXEvent* pEvent2 = (CXEvent *)lParam2;
    CXEventSource* pEventSource2 = pEvent2->m_pEventSource;


    ASSERT((pEvent1 != NULL) && (pEvent2 != NULL));
    int nResult = 0;
    CString sText1, sText2;

    switch( lParamSort)
    {
    case ICOL_LcEvents_LOG:
        // Sort by log, then by source, then by ID
        nResult = lstrcmp(pEventSource1->m_pEventLog->m_sName, pEventSource2->m_pEventLog->m_sName);
        if (nResult == 0) {
            nResult = lstrcmp(pEventSource1->m_sName, pEventSource2->m_sName);
            if (nResult == 0) {
                 nResult = ((LONG) pEvent1->m_message.GetShortId()) - ((LONG) pEvent2->m_message.GetShortId());
            }
        }
        break;
    case ICOL_LcEvents_SOURCE:
        // Sort by source, then by Log, then by ID
        nResult = lstrcmp(pEventSource1->m_sName, pEventSource2->m_sName);
        if (nResult == 0) {
            nResult = lstrcmp(pEventSource1->m_pEventLog->m_sName, pEventSource2->m_pEventLog->m_sName);
            if (nResult == 0) {
                 nResult = ((LONG) pEvent1->m_message.GetShortId()) - ((LONG) pEvent2->m_message.GetShortId());
            }
        }
        break;
    case ICOL_LcEvents_ID:
        // Sort by ID, then by log, then by source.
        nResult = ((LONG) pEvent1->m_message.GetShortId()) - ((LONG) pEvent2->m_message.GetShortId());
        if (nResult == 0) {
            nResult = lstrcmp(pEventSource1->m_pEventLog->m_sName, pEventSource2->m_pEventLog->m_sName);
            if (nResult == 0) {
                nResult = lstrcmp(pEventSource1->m_sName, pEventSource2->m_sName);
            }
        }
        break;
    case ICOL_LcEvents_SEVERITY:
        // Sort by severity, then by log, then by source, then by ID
        pEvent1->m_message.GetSeverity(sText1);
        pEvent2->m_message.GetSeverity(sText2);
        nResult = lstrcmp(sText1, sText2);
        if (nResult == 0) {
            nResult = lstrcmp(pEventSource1->m_pEventLog->m_sName, pEventSource2->m_pEventLog->m_sName);
            if (nResult == 0) {
                nResult = lstrcmp(pEventSource1->m_sName, pEventSource2->m_sName);
                if (nResult == 0) {
                     nResult = ((LONG) pEvent1->m_message.GetShortId()) - ((LONG) pEvent2->m_message.GetShortId());
                }
            }
        }
        break;
    case ICOL_LcEvents_COUNT:
        // Sort by count, then by log, then by source, then by ID
        pEvent1->GetCount(sText1);
        pEvent2->GetCount(sText2);
        nResult = lstrcmp(sText1, sText2);
        if (nResult == 0) {
            nResult = lstrcmp(pEventSource1->m_pEventLog->m_sName, pEventSource2->m_pEventLog->m_sName);
            if (nResult == 0) {
                nResult = lstrcmp(pEventSource1->m_sName, pEventSource2->m_sName);
                if (nResult == 0) {
                     nResult = ((LONG) pEvent1->m_message.GetShortId()) - ((LONG) pEvent2->m_message.GetShortId());
                }
            }
        }
        break;
    case ICOL_LcEvents_TIME:
        // Sort by time, then by log, then by source, then by ID
        pEvent1->GetTimeInterval(sText1);
        pEvent2->GetTimeInterval(sText2);
        nResult = lstrcmp(sText1, sText2);
        if (nResult == 0) {
            nResult = lstrcmp(pEventSource1->m_pEventLog->m_sName, pEventSource2->m_pEventLog->m_sName);
            if (nResult == 0) {
                nResult = lstrcmp(pEventSource1->m_sName, pEventSource2->m_sName);
                if (nResult == 0) {
                     nResult = ((LONG) pEvent1->m_message.GetShortId()) - ((LONG) pEvent2->m_message.GetShortId());
                }
            }
        }
        break;
    case ICOL_LcEvents_DESCRIPTION:
        // Sort by description, then by log, then by source, then by ID
        nResult = lstrcmp(pEvent1->m_message.m_sText, pEvent2->m_message.m_sText);
        if (nResult == 0) {
            nResult = lstrcmp(pEventSource1->m_pEventLog->m_sName, pEventSource2->m_pEventLog->m_sName);
            if (nResult == 0) {
                nResult = lstrcmp(pEventSource1->m_sName, pEventSource2->m_sName);
                if (nResult == 0) {
                     nResult = ((LONG) pEvent1->m_message.GetShortId()) - ((LONG) pEvent2->m_message.GetShortId());
                }
            }
        }
        break;
    default:
        ASSERT(FALSE);
        break;
    }


    if (!g_abLcEventsSortAscending[lParamSort]) {
        if (nResult > 0) {
            nResult = -1;
        }
        else if (nResult < 0) {
            nResult = 1;
        }
    }

    return nResult;
}


//***************************************************************************
//
//  CLcEvents::SortItems
//
//  Sort the items in this list control given the column index.  This method
//  hides all details about the sort implementation from this class's clients.
//
//  Parameters:
//		DWORD dwColumn
//			The column to use as the sort key.
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
void CLcEvents::SortItems(DWORD dwColumn)
{
    CListCtrl::SortItems(CompareEventsProc, dwColumn);
    m_dwSortColumn = dwColumn;
}





//****************************************************************************
// CLcEvents::UpdateDescriptionWidth()
//
// Measure the message description string associated with each item and set the
// width of the description column to match the widest message length plus a
// little extra room for slop and appearances.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*****************************************************************************
void CLcEvents::UpdateDescriptionWidth()
{
    LONG cxWidestMessage = CX_DEFAULT_DESCRIPTION_WIDTH;
    LONG nEvents = GetItemCount();
    for (LONG iEvent = 0; iEvent < nEvents; ++iEvent) {
        CXEvent* pEvent = GetAt(iEvent);
        int cx = GetStringWidth(pEvent->m_message.m_sText);
        if (cx > cxWidestMessage) {
            cxWidestMessage = cx;
        }
    }


    // Set the column width to the width of the widest string plus a little extra
    // space for slop and to make it obvious to the user that the complete string
    // is displayed.
    SetColumnWidth(ICOL_LcEvents_DESCRIPTION, cxWidestMessage + CX_DESCRIPTION_SLOP);
}
