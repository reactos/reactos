#include "stdafx.h"
#include "source.h"
#include "lcsource.h"
#include "regkey.h"
#include "source.h"
#include "utils.h"
#include "globals.h"
#include "busy.h"
#include "trapreg.h"




/////////////////////////////////////////////////////////////////////////////
// CLcSource

CLcSource::CLcSource()
{
}

CLcSource::~CLcSource()
{
}




//***************************************************************************
//
//  CLcSource::AddMessage
//
//  Add a message to the list control. This sets the text for each column in
//  the list view and sets the lParam field of the list-view item to pMessage.
//  
//
//  Parameters:
//		CMessage* pMessage
//
//  Returns:
//		Nothing.
//
//  Status:
//      
//***************************************************************************
void CLcSource::AddMessage(CXMessage* pMessage)
{
	CString sText;
    pMessage->GetShortId(sText);


    // Insert a new item into this list control.
    LV_ITEM lvitem;
    lvitem.mask = LVIF_TEXT | LVIF_PARAM;
    lvitem.iSubItem = ICOL_LcSource_EVENTID;
    lvitem.cchTextMax = MAX_STRING;
    lvitem.lParam = (LPARAM)pMessage;
    lvitem.pszText = (LPTSTR)(LPCTSTR)sText;
    int nItem = InsertItem(&lvitem);

    if (nItem >= 0)
    {
        CXEventSource* pEventSource = pMessage->m_pEventSource;

        // Now set the string value for each sub-item.
		pMessage->GetSeverity(sText); 
		SetItemText(nItem, ICOL_LcSource_SEVERITY, (LPTSTR)(LPCTSTR) sText);

		pMessage->IsTrapping(sText);
        SetItemText(nItem, ICOL_LcSource_TRAPPING, (LPTSTR)(LPCTSTR)sText);
        SetItemText(nItem, ICOL_LcSource_DESCRIPTION, (LPTSTR)(LPCTSTR) pMessage->m_sText);
    }
}


//*******************************************************************
// CXMessageArray::SetDescriptionWidth
//
// Set the width of the description field so that it is wide enough to
// hold the widest message.
//
// Parameters:
//      CXMessageArray& aMessages
//          The message array that will be used to fill the list control.
//
// Returns:
//      Nothing.
//
//*******************************************************************
void CLcSource::SetDescriptionWidth(CXMessageArray& aMessages)
{
    LONG cxWidestMessage = CX_DEFAULT_DESCRIPTION_WIDTH;
    LONG nMessages = aMessages.GetSize();
    for (LONG iMessage = 0; iMessage < nMessages; ++iMessage) {
        CXMessage* pMessage = aMessages[iMessage];
        int cx = GetStringWidth(pMessage->m_sText);
        if (cx > cxWidestMessage) {
            cxWidestMessage = cx;
        }
    }

    // Set the column width to the width of the widest string plus a little extra
    // space for slop and to make it obvious to the user that the complete string
    // is displayed.
    SetColumnWidth(ICOL_LcSource_DESCRIPTION, cxWidestMessage + CX_DESCRIPTION_SLOP);
}



//***************************************************************************
//
//  CLcSource::LoadMessages
//
//  Load the messages from the message library module and insert them into
//  this list control.
//
//  Parameters:
//		CMessage* pMessage
//
//  Returns:
//		Nothing.
//
//  Status:
//      
//***************************************************************************
SCODE CLcSource::SetEventSource(CXEventSource* pEventSource)
{
    CBusy busy;

	DeleteAllItems();

    if (pEventSource == NULL) {
        return S_OK;
    }


    UpdateWindow();
    
    //!!!CR: Should do something with the return code in case the
    //!!!CR: messages weren't loaded.
    SCODE sc = pEventSource->LoadMessages();


	// Iterate through each of the messages and insert them into
	// the list control.
    CXMessageArray& aMessages = pEventSource->m_aMessages;

    // Set the width of the description field so that it is wide enough to contain
    // the widest message.
    SetDescriptionWidth(aMessages);

	LONG nMessages = aMessages.GetSize();
	for (LONG iMessage=0; iMessage < nMessages; ++iMessage) {
        if ((iMessage < 40 && (iMessage % 10 == 9)) ||
            (iMessage % 100 == 99)) {
            // Update the window often for the first few messages and less frequently
            // thereafter for a good response time.
            UpdateWindow();
        }

		AddMessage(aMessages[iMessage]);
	}


    SortItems(ICOL_LcSource_EVENTID);
    SetRedraw(TRUE);
    UpdateWindow();
    EnsureVisible(0, FALSE);

	if (GetSize())
		SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

	return S_OK;
}



BEGIN_MESSAGE_MAP(CLcSource, CListCtrl)
	//{{AFX_MSG_MAP(CLcSource)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLcSource message handlers



//***************************************************************************
//
//  CLcSource::CreateWindowEpilogue()
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
SCODE CLcSource::CreateWindowEpilogue()
{
	ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_FULLROWSELECT);
	SetColumnHeadings();
	return S_OK;
}

//***************************************************************************
//
//  CLcSource::SetColumnHeadings
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
void CLcSource::SetColumnHeadings()
{
 	static UINT auiResColumnTitle[ICOL_LcSource_MAX] = {
		IDS_LcSource_TITLE_EVENT_ID,
		IDS_LcSource_TITLE_SEVERITY,
		IDS_LcSource_TITLE_TRAPPING,
		IDS_LcSource_TITLE_DESCRIPTION
	};

	static int aiColWidth[ICOL_LcSource_MAX] = {60, 75, 60, CX_DEFAULT_DESCRIPTION_WIDTH};

 
    // Build the columns in the AllEventsList control.
    LV_COLUMN lvcol; 
    lvcol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    
    for (int iCol=0; iCol<ICOL_LcSource_MAX; ++iCol)
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




//******************************************************************
// CLcSource::Find
//
// Find the specified event source in this list control.
//
// Parameters:
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
//			list control item, the item is scrolled into view and the focus
//			is set on the item.
//
//******************************************************************
BOOL CLcSource::Find(CString sText, BOOL bWholeWord, BOOL bMatchCase)
{
    // Don't do anything if the list is empty.
	if (GetSize() == 0) 
		return FALSE;

	if (!bMatchCase) 
		sText.MakeUpper();

    // Get the selected item.
    LONG iItem = GetNextItem(-1, LVNI_SELECTED);    

    // Nothing selected; start from the top of the list.
    if (iItem == -1)
        iItem = 0;


    // Iterate through all of the items starting at one item past
    // the currently selected item. 
	CXMessage* pMessage;
	CString sDescription;
    BOOL bFound = FALSE;
	LONG nItems = GetSize();
    LONG iItemStart = iItem;
	for (long i=0; !bFound && i<nItems; ++i) {
        // Bump the item index to the next one and wrap it if its past the
        // last item.
		iItem = (iItem + 1) % nItems;

        // Get the message description for this item.
		pMessage = GetAt(iItem);
        sDescription = pMessage->m_sText;

		if (!bMatchCase) 
			sDescription.MakeUpper();
        
        if (bWholeWord)	{
            // Compare the whole word.
			bFound = (FindWholeWord(sText, sDescription) != -1);
        }
        else {
	        // Look for a substring.
            if (sDescription.Find(sText) >= 0)  
                bFound = TRUE;
        } 
    }

    // Found a match.
    if (bFound)
    {
        // Unselect the selected item and select the found item.        
        SetItemState(iItemStart, 0, LVIS_SELECTED | LVIS_FOCUSED);
        SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        EnsureVisible(iItem, FALSE);
        return TRUE;
    }

    return FALSE;
}






//***************************************************************************
//
//  fnCompareCLcSource
//
//  This the item comparison callback method that is called from CLcSource::SortItems. 
//
//  Parameters:
//		LPARAM lParam1
//			This is the lparam for the first item to compare.  This is a pointer to 
//			the associated CMessage object.
//
//		LPARAM lParam2
//			This is the lparam for the second item to compare.  This is a pointer to 
//			the associated CMessage object.
//
//		LPARAM lColumn
//			This is the second parameter that was passed to CListCtrl::SortItems.  This 
//			happens to be the list control column index.
//
//  Returns:
//		Nothing.
//
//  Status:
//      
//***************************************************************************
int CALLBACK fnCompareCLcSource(LPARAM lParam1, LPARAM lParam2, LPARAM lColumn)
{
    // !!!CR: The LPARAM parameters are not event pointers in all cases because
    // !!!CR: each subitem has its own LPARAM. What should I do?

    CXMessage *pmsg1 = (CXMessage *)lParam1;
    CXMessage *pmsg2 = (CXMessage *)lParam2;

    int nResult = 0;
    CString s1, s2;

    if (pmsg1 && pmsg2)
    {
        switch( lColumn)
        {
        case ICOL_LcSource_EVENTID:
        	nResult = ((LONG) pmsg1->GetShortId()) - ((LONG)pmsg2->GetShortId());			
        	break;
        case ICOL_LcSource_SEVERITY:
         	pmsg1->GetSeverity(s1);
        	pmsg2->GetSeverity(s2);
        	nResult = lstrcmpi(s1, s2);
        	break;
        case ICOL_LcSource_TRAPPING:
        	pmsg1->IsTrapping(s1);
        	pmsg2->IsTrapping(s2);
        	nResult = lstrcmpi(s1, s2);
        	break;
        case ICOL_LcSource_DESCRIPTION:
            nResult = lstrcmpi(pmsg1->m_sText, pmsg2->m_sText);
        	break;
        default:
         	ASSERT(FALSE);
            nResult = 0;
            break;
        }
    }

    if (!g_abLcSourceSortAscending[lColumn]) {
        if (nResult > 0) {
            nResult = -1;
        }
        else if (nResult < 0) {
            nResult = 1;
        }
    }

    return(nResult);
}


//***************************************************************************
//
//  CLcSource::SortItems
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
void CLcSource::SortItems(DWORD dwColumn)
{
    CListCtrl::SortItems(fnCompareCLcSource, dwColumn);
}




//***************************************************************************
//
//  CLcSource::GetSelectedMessages
//
//  Fill a message array with pointers to the messages that correspond to
//  the selected items in this list control.  
//
//  Note: This list control continues to own the returned pointers.  The
//  caller should not delete them.
//
//  Parameters:
//		CMessageArray& amsg
//			The message array where the pointers to the selected messages are
//			returned.
//
//  Returns:
//		The message array is filled with pointers to the selected messages.  Do
//		not delete them, because they are owned by this object.
//
//  Status:
//      
//***************************************************************************
void CLcSource::GetSelectedMessages(CXMessageArray& amsg)
{
	// Clear the message array
	amsg.RemoveAll();

	// Setup the LV_ITEM structure to retrieve the lparam field.  
	// This field contains the CMessage pointer.
    LV_ITEM lvitem;
    lvitem.mask = LVIF_PARAM;
    lvitem.iSubItem = ICOL_LcSource_EVENTID;

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
		CXMessage* pmsg = (CXMessage*) (void*) lvitem.lParam;
		amsg.Add(pmsg);
	}
}




//***************************************************************************
//
//  CLcSource::FindItem
//
//  Search through this list-controls's items to find the one with the
//  specified message ID.  
//
//  Parameters:
//		DWORD dwMessageId
//			The message ID to search for.
//
//  Returns:
//		The index of the item with the specified message ID.  If no such message ID
//		was found, -1 is returned.
//
//  Status:
//      
//***************************************************************************
LONG CLcSource::FindItem(DWORD dwMessageId)
{
	LONG nItems = GetItemCount();
	for (LONG iItem = 0; iItem < nItems; ++iItem) {
        CXMessage* pMessage = GetAt(iItem);
        if (pMessage->m_dwId == dwMessageId) {
            return iItem;
        }
	}
	return -1;
}




//***************************************************************************
//
//  CLcSource::RefreshItem
//
//  This method is called when some aspect of the message has changed and
//  the display needs to be updated.  This occurs when the trapping status
//  of an event changes.  
//
//  Parameters:
//		DWORD dwMessageId
//			The message ID to search for.
//
//  Returns:
//		The index of the item with the specified message ID.  If no such message ID
//		was found, -1 is returned.
//
//  Status:
//      
//***************************************************************************
void CLcSource::RefreshItem(LONG iItem)
{
	CXMessage* pMessage = GetAt(iItem);
    CString sText;

	// Now set the text value for each column in the list control.
    pMessage->GetSeverity(sText);
	SetItemText(iItem, ICOL_LcSource_SEVERITY, (LPTSTR)(LPCTSTR) sText);

    // Check if we are trapping this event.
	pMessage->IsTrapping(sText);
    SetItemText(iItem, ICOL_LcSource_TRAPPING, (LPTSTR)(LPCTSTR)sText);

    SetItemText(iItem, ICOL_LcSource_DESCRIPTION, (LPTSTR)(LPCTSTR)pMessage->m_sText);
}



//***************************************************************************
//
//  CLcSource::GetAt
//
//  This method returns the message pointer located at the given item index.
//  This allows CLcSource to be used much as an array.  
//
//  Parameters:
//		LONG iItem
//			The item index.
//
//  Returns:
//		A pointer to the CMessage stored at the specified index.
//
//  Status:
//      
//***************************************************************************
CXMessage* CLcSource::GetAt(LONG iItem) 
{
	// Setup the LV_ITEM structure to retrieve the lparam field.  
	// This field contains the CMessage pointer.
    LV_ITEM lvitem;
    lvitem.mask = LVIF_PARAM;
    lvitem.iSubItem = ICOL_LcSource_EVENTID;	
    lvitem.iItem = iItem;
    GetItem(&lvitem);

	CXMessage* pMessage = (CXMessage*) (void*) lvitem.lParam;
	return pMessage;
}



//***************************************************************************
// CLcSource::NotifyTrappingChange
//
// This method is called when a message's trapping status changes.  A message
// is considered trapped if it appears in the CLcEvents listbox.
//
// Parameters:
//      DWORD dwMessageId
//          The ID of the message who's trapping status is changing.
//
//      BOOL bIsTrapping
//          TRUE if the message is being trapped, FALSE otherwise.
//
// Returns:
//      Nothing.     
//      
//***************************************************************************
void CLcSource::NotifyTrappingChange(DWORD dwMessageId, BOOL bIsTrapping)
{
    LONG iItem = FindItem(dwMessageId);
    ASSERT(iItem != -1);

    if (iItem != -1) {
        CString sTrapping;
    	sTrapping.LoadString(bIsTrapping ? IDS_IS_TRAPPING : IDS_NOT_TRAPPING);
        SetItemText(iItem, ICOL_LcSource_TRAPPING, (LPTSTR)(LPCTSTR)sTrapping);
    }
}
