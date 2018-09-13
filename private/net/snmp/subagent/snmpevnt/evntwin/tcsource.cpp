#include "stdafx.h"
#include "source.h"
#include "tcsource.h"
#include "regkey.h"
#include "utils.h"
#include "globals.h"
#include "trapreg.h"


/////////////////////////////////////////////////////////////////////////////
// CTcSource

CTcSource::CTcSource()
{
}

CTcSource::~CTcSource()
{
}


BEGIN_MESSAGE_MAP(CTcSource, CTreeCtrl)
	//{{AFX_MSG_MAP(CTcSource)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTcSource message handlers



void CTcSource::LoadImageList()
{
    m_ImageList.Create(16, 16, ILC_COLOR | ILC_MASK, 2, 0);

	CBitmap* pFolder;

    pFolder = new CBitmap;
    pFolder->LoadBitmap(IDB_FOLDERCLOSE);
    m_ImageList.Add(pFolder, (COLORREF)0xff00ff);
	delete pFolder;

    pFolder = new CBitmap;
    pFolder->LoadBitmap(IDB_FOLDEROPEN);
    m_ImageList.Add(pFolder, (COLORREF)0xff00ff);

	delete pFolder;

    SetImageList(&m_ImageList, TVSIL_NORMAL);	
}	

SCODE CTcSource::LoadTreeFromRegistry()
{
    TV_INSERTSTRUCT TreeCtrlItem;
    TreeCtrlItem.hInsertAfter = TVI_LAST;

	// Iterate through each of the event logs and add each log to the tree.
	LONG nLogs = g_reg.m_aEventLogs.GetSize();
    for (LONG iLog=0; iLog < nLogs; ++iLog)
    {
        CXEventLog* pEventLog = g_reg.m_aEventLogs[iLog];

        TreeCtrlItem.hParent = TVI_ROOT;
        TreeCtrlItem.item.pszText = (LPTSTR)(LPCTSTR)pEventLog->m_sName;
        TreeCtrlItem.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
        TreeCtrlItem.item.iImage = 0;
        TreeCtrlItem.item.iSelectedImage = 0;
        TreeCtrlItem.item.lParam = (LPARAM) pEventLog;
        
        HTREEITEM htiParent = InsertItem(&TreeCtrlItem);

        TreeCtrlItem.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
        TreeCtrlItem.hParent = htiParent;
        TreeCtrlItem.item.iImage = 0;
        TreeCtrlItem.item.iSelectedImage = 1;

        // Insert each source as a child.
        LONG nSources = pEventLog->m_aEventSources.GetSize();
        for (LONG iSource = 0; iSource < nSources; ++iSource) {
            CXEventSource* pEventSource = pEventLog->m_aEventSources.GetAt(iSource);
            TreeCtrlItem.item.pszText = (LPTSTR)(LPCTSTR)pEventSource->m_sName;
            TreeCtrlItem.item.lParam = (LPARAM) pEventSource;
            InsertItem(&TreeCtrlItem);
        }
    }
    SortChildren(NULL);
	return S_OK;
}



SCODE CTcSource::CreateWindowEpilogue()
{
	LoadImageList();
	LoadTreeFromRegistry();
	return S_OK;
}


//******************************************************************
// CTcSource::GetSelectedEventSource
//
// Get name of the event source and log for the currently selected
// event source. 
//
// Parameters:
// 		CString& sLog
//			This is where the event log name is returned.
//
// 		CString& sEventSource
//			This is where the name of the event source (application)
//			is returned.
//
// Returns:
//		SCODE
//			S_OK if an event source was selected and the log and event source
//			names are returned.
//
//			E_FAIL if no event source was selected.  The log and event source
//			names are returned as empty when this occurs.
//
//******************************************************************
CXEventSource* CTcSource::GetSelectedEventSource()
{
    HTREEITEM htiParent, htiSelected; 

    // Get the selected item.
    htiSelected = GetSelectedItem();
    if (htiSelected == NULL)
        return NULL;

	// If the selected item is an event source (application), then
	// its parent should be the log.  To get the log name, we must
	// get the parent name.
    htiParent = GetParentItem(htiSelected);
    if (htiParent == NULL)
        return NULL;

	// The application name is the selected item.
    TV_ITEM tvi;
    tvi.hItem = htiSelected;
    tvi.mask = TVIF_HANDLE | TVIF_PARAM;    
    if (GetItem(&tvi) == FALSE)
        return NULL;
    return (CXEventSource*) (void*) tvi.lParam;
}





//******************************************************************
// CTcSource::Find
//
// Find the specified event source in the tree.
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
//			tree item.
//
//******************************************************************
BOOL CTcSource::Find(CString& sText, BOOL bWholeWord, BOOL bMatchCase)
{    
    // Search the source tree for sText. We are only looking at the source
    // names, not the types.
                                
    HTREEITEM hCurrentItem, hStartItem, hSourceItem, hRootItem;
    TV_ITEM tvItem;
    CString sSource;
    TCHAR szBuffer[256];
    BOOL bItemFound = FALSE, bCompleteLoop = FALSE;

    // Get the selected item and keep track of it.
    hCurrentItem = GetSelectedItem();
    if (hCurrentItem == NULL)
    {
        // Nothing selected; get the root.
        hCurrentItem = GetRootItem();
        if (hCurrentItem == NULL)
            return FALSE; 
    }    
    hStartItem = hCurrentItem;

    // Loop until we find a match or we are back where we started.
    while (!bItemFound && !bCompleteLoop)
    {
         hSourceItem = NULL;

        // Get the next item.
        
        // Current item is root; get the first child.
        hRootItem = GetParentItem(hCurrentItem);
        if (hRootItem == NULL)
            hSourceItem = GetChildItem(hCurrentItem);
                
        // Current item is a source; get the next sibling.
        else
        {
            hSourceItem = GetNextItem(hCurrentItem, TVGN_NEXT);
            // No sibling; get the parent and set it as the current item.
            if (hSourceItem == NULL)
            {
                 hRootItem = GetParentItem(hCurrentItem);
                if (hRootItem == NULL)
                    return FALSE;  // No parent; something is wrong.
                hCurrentItem = hRootItem;
            }
        }

        // We have a source; get it and compare.
        if (hSourceItem != NULL)
        {
            hCurrentItem = hSourceItem;

            tvItem.mask = TVIF_HANDLE | TVIF_TEXT;
            tvItem.hItem = hSourceItem;
            tvItem.pszText = szBuffer;
            tvItem.cchTextMax = 256;
            if (GetItem(&tvItem) == FALSE)
                return FALSE; // Something is wrong.
            sSource = szBuffer;

            // Compare the whole word.
            if (bWholeWord)
            {
                int nRetVal;

                // No case compare.
                if (bMatchCase)
                    nRetVal = sSource.Compare(sText);
                // Case compare
                else
                    nRetVal = sSource.CompareNoCase(sText);                    

                if (nRetVal == 0)
                    bItemFound = TRUE;
            }

            // Look for a substring.
            else
            {
                // Make everything upper.
                if (!bMatchCase)
                {
                    sSource.MakeUpper();
                    sText.MakeUpper();
                }

                if (sSource.Find(sText) >= 0)
                    bItemFound = TRUE;
            }
        }        

        // Get the next root.
        else
        {    
            hRootItem = GetNextItem(hCurrentItem, TVGN_NEXT);
            // No more roots in the tree; go to the top of the tree.
            if (hRootItem == NULL)
                hCurrentItem = GetRootItem();
            else
                hCurrentItem = hRootItem;
        }

        if (hCurrentItem == hStartItem)
            bCompleteLoop = TRUE;
    }

    // Found a match; select it.
    if (bItemFound)
    {
        SelectItem(hCurrentItem);
        EnsureVisible(hCurrentItem);
        SetFocus();
        return TRUE;
    }

    return FALSE;            
}
