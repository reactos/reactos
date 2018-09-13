//******************************************************************
// layout.cpp
//
// This file contains the code that lays out the CTrapEventDialog.
// This is neccessary when the edit/view button changes the 
// dialog form its small (main) view to the extended view.
//
// Author: Larry A. French
//
// History:
//      20-Febuary-96  Wrote it
//
//
// Copyright (C) 1996 Microsoft Corporation.  All rights reserved.
//******************************************************************

#include "stdafx.h"
#include "layout.h"
#include "trapdlg.h"
#include "trapreg.h"
#include "globals.h"



#define CX_MARGIN 10
#define CY_MARGIN 10
#define CY_LEADING 3
#define CY_DIALOG_FONT 8


class CDlgMetrics
{
public: 
	CDlgMetrics(CEventTrapDlg* pdlg);
	CSize m_sizeMargin;
	CSize m_sizeAddButton;
	CSize m_sizeRemoveButton;
	CSize m_sizeFindButton;
	CSize m_sizeOKButton;
	int m_cyLeading;
    CSize m_sizeLabel0;
	CSize m_sizeLabel1;
	CSize m_sizeLabel2;

    CSize m_sizeConfigTypeBox;
    CSize m_sizeConfigCustomButton;
    CSize m_sizeConfigDefaultButton;
};
		

//*****************************************************************
// CDlgMetrics::CDlgMetrics
//
// Construct an object containing the metrics for the CEventTrapDlg
//
// Parameters:
// 		CEventTrapDlg* pdlg
//			Pointer to an instance of the main event trap dialog.
//			This pointer is used to access members, such as buttons
//			and so on so that they can be measured.
//
// Returns:
//		The members of this class are valid on return.
//
//*****************************************************************
CDlgMetrics::CDlgMetrics(CEventTrapDlg* pdlg)
{
	m_sizeMargin.cx = CX_MARGIN;
	m_sizeMargin.cy = CY_MARGIN;
	m_cyLeading = CY_LEADING;

	CRect rc;

	pdlg->m_btnAdd.GetWindowRect(&rc);
	pdlg->ScreenToClient(&rc);
	m_sizeAddButton = rc.Size();

	pdlg->m_btnFind.GetWindowRect(&rc);
	pdlg->ScreenToClient(&rc);
	m_sizeFindButton = rc.Size();

	pdlg->m_btnRemove.GetWindowRect(&rc);
	pdlg->ScreenToClient(&rc);
	m_sizeRemoveButton = rc.Size();

	pdlg->m_statLabel0.GetWindowRect(&rc);
	pdlg->ScreenToClient(&rc);
	m_sizeLabel0 = rc.Size();

    pdlg->m_statLabel1.GetWindowRect(&rc);
	pdlg->ScreenToClient(&rc);
	m_sizeLabel1 = rc.Size();

	pdlg->m_statLabel2.GetWindowRect(&rc);
	pdlg->ScreenToClient(&rc);
	m_sizeLabel2 = rc.Size();

	pdlg->m_btnOK.GetWindowRect(&rc);
	pdlg->ScreenToClient(&rc);
	m_sizeOKButton = rc.Size();


    if (g_reg.m_bShowConfigTypeBox) {
    	pdlg->m_btnConfigTypeBox.GetWindowRect(&rc);
    	pdlg->ScreenToClient(&rc);
    	m_sizeConfigTypeBox = rc.Size();

    	pdlg->m_btnConfigTypeCustom.GetWindowRect(&rc);
    	pdlg->ScreenToClient(&rc);
    	m_sizeConfigCustomButton = rc.Size();

    	pdlg->m_btnConfigTypeDefault.GetWindowRect(&rc);
    	pdlg->ScreenToClient(&rc);
    	m_sizeConfigDefaultButton = rc.Size();
    }
    else {
        // If the "Configuration type box will not be shown, then the size of the
        // box and the radio buttons in it are all zero.
    	pdlg->m_btnConfigTypeBox.GetWindowRect(&rc);
        m_sizeConfigTypeBox.cx = 0;
        m_sizeConfigTypeBox.cy = 0;

        m_sizeConfigCustomButton.cx = 0;
        m_sizeConfigCustomButton.cy = 0;

        m_sizeConfigDefaultButton.cx = 0;
        m_sizeConfigDefaultButton.cy = 0;
    }
}







//////////////////////////////////////////////////////////////////////
// CLASS: CMainLayout
//
// This class computes the position of various items for the main (small)
// view of the dialog.  The metrics for each of these items is made
// available through public data members.
/////////////////////////////////////////////////////////////////////
class CMainLayout
{
public:
	void Create(CDlgMetrics& metrics, CRect& rc);
	CRect m_rc;
    CRect m_rcLabel0;
	CRect m_rcOKButton;
	CRect m_rcCancelButton;
    CRect m_rcApplyButton;
	CRect m_rcPropertiesButton;
	CRect m_rcSettingsButton;
    CRect m_rcExportButton;
	CRect m_rcViewButton;
	CRect m_rcListView;

    CRect m_rcConfigTypeBox;
    CRect m_rcConfigCustomButton;
    CRect m_rcConfigDefaultButton;
};

//*****************************************************************
// CMainLayout::Create
//
// Construct the layout for the main part of the dialog.  This is
// the part where the add event stuff is hidden. 
//
// Note: The caller is responsible for making sure that the 
// specified rectangle is large enough so that the display 
// still looks good.  For example, it doesn't make sense to shrink
// the listview to a zero size or even negative size.
//
// Parameters:
// 		CDlgMetrics& metrics
//			The dialog metrics containing the size of the things
//			that appear on the dialog and so on.
//
//		CRect& rc
//			The rectangle where the main part of the dialog will be
//			drawn.
//
// Returns:
//		The members of this class are valid on return.
//
//*****************************************************************
void CMainLayout::Create(CDlgMetrics& metrics, CRect& rc)
{
	m_rc = rc;

	// The rectangle for this layout may actually extend beyond the size
	// of the dialog window.  This can occur when the user shrinks the dialog
	// to a size smaller than the minimum for this layout.  
	//
	// Things that are drawn outside of the dialog are clipped.
	//

    // Set the rectangle for the "Configuration Type" groupbox
    m_rcConfigTypeBox.left = rc.left + metrics.m_sizeMargin.cx;
    m_rcConfigTypeBox.top = rc.top + metrics.m_sizeMargin.cy;
    m_rcConfigTypeBox.right = rc.right -  (metrics.m_sizeOKButton.cx + 2 * metrics.m_sizeMargin.cx);
    m_rcConfigTypeBox.bottom = m_rcConfigTypeBox.top + metrics.m_sizeConfigTypeBox.cy;

    // Set the rectangle for the "Custom" radio button within the "Configuration Type" groupbox
    // We place it right in the middle between the top and the bottom of the groupbox.
    m_rcConfigCustomButton.left = m_rcConfigTypeBox.left + metrics.m_sizeMargin.cx;
    m_rcConfigCustomButton.top = m_rcConfigTypeBox.top  + 
                        (metrics.m_sizeConfigTypeBox.cy/2 - metrics.m_sizeConfigCustomButton.cy/2) + CY_DIALOG_FONT/2;
    m_rcConfigCustomButton.right = m_rcConfigCustomButton.left + metrics.m_sizeConfigCustomButton.cx;
    m_rcConfigCustomButton.bottom = m_rcConfigCustomButton.top + metrics.m_sizeConfigCustomButton.cy;

    // Set the rectangle for the "Default" radio button within the "Configuration Type" groupbox
    m_rcConfigDefaultButton.left = m_rcConfigCustomButton.right + metrics.m_sizeMargin.cx;
    m_rcConfigDefaultButton.top = m_rcConfigCustomButton.top;
    m_rcConfigDefaultButton.right = m_rcConfigDefaultButton.left + metrics.m_sizeConfigDefaultButton.cx;
    m_rcConfigDefaultButton.bottom = m_rcConfigCustomButton.bottom;


    m_rcLabel0.left = m_rcConfigTypeBox.left;
    m_rcLabel0.top = m_rcConfigTypeBox.bottom;
    if (metrics.m_sizeConfigTypeBox.cy != 0) {
        // If the configuration type groupbox is present, then the event list
        // should be placed one margin height below it.
    	m_rcLabel0.top += metrics.m_sizeMargin.cy;
    }
    m_rcLabel0.right = m_rcLabel0.left + metrics.m_sizeLabel0.cx;
    m_rcLabel0.bottom = m_rcLabel0.top + metrics.m_sizeLabel0.cy;

	// Set the position of the top events listview.
	m_rcListView.left = m_rcConfigTypeBox.left;
  	m_rcListView.top = m_rcLabel0.bottom + metrics.m_sizeMargin.cy;
	m_rcListView.right = m_rcConfigTypeBox.right;
	m_rcListView.bottom = rc.bottom - metrics.m_sizeMargin.cy;

	// Set the position of the OK button
	m_rcOKButton.left = m_rcListView.right + metrics.m_sizeMargin.cx;
	m_rcOKButton.top = m_rcConfigTypeBox.top;
    if (metrics.m_sizeConfigTypeBox.cy != 0) {
        // If the configuration type groupbox is present, then the OK button should be
        // moved down by half the dialog font height so that it lines up with the
        // top of the groupbox's rectangle instead of the top of the group box's title.
        m_rcOKButton.top += CY_DIALOG_FONT / 2;
    }
	m_rcOKButton.right = m_rcOKButton.left + metrics.m_sizeOKButton.cx;
	m_rcOKButton.bottom = m_rcOKButton.top + metrics.m_sizeOKButton.cy;

	// Compute the vertical distance between buttons.
	int cyDelta = m_rcOKButton.Height() + metrics.m_sizeMargin.cy / 2;
	
	// Set the position of the Cancel button
	m_rcCancelButton = m_rcOKButton;
	m_rcCancelButton.OffsetRect(0, cyDelta);

    // Set the position of the Apply button
    m_rcApplyButton = m_rcCancelButton;
    m_rcApplyButton.OffsetRect(0, cyDelta);

	// Set the position of the settings button	
    m_rcSettingsButton = m_rcApplyButton;
	m_rcSettingsButton.OffsetRect(0, cyDelta);

	// Set the position of the properties button
	m_rcPropertiesButton = m_rcSettingsButton;
	m_rcPropertiesButton.OffsetRect(0, cyDelta);

	// Set the position of the export button	
	m_rcExportButton = m_rcPropertiesButton;
	m_rcExportButton.OffsetRect(0, cyDelta);

	// Set the position of the view button
	m_rcViewButton = m_rcExportButton;
	m_rcViewButton.OffsetRect(0, cyDelta);
}

	

//////////////////////////////////////////////////////////////////////
// CLASS: CExtendedLayout
//
// This class computes the position of various items for the extended
// view of the dialog.  The metrics for each of these items is made
// available through public data members.
/////////////////////////////////////////////////////////////////////
class CExtendedLayout
{
public:
	void Create(CDlgMetrics& metrics, CRect& rc);
	CRect m_rc;
	CRect m_rcTreeView;
	CRect m_rcListView;
	CRect m_rcFindButton;
	CRect m_rcAddButton;
	CRect m_rcRemoveButton;
	CRect m_rcLabel1;
	CRect m_rcLabel2;
private:
};



//*****************************************************************
// CExtendedLayout::Create
//
// Construct the layout for the extended part of the dialog.  This is
// the part where the add event stuff is shown.
//
// Note: The caller is responsible for making sure that the 
// specified rectangle is large enough so that the display 
// still looks good.  For example, it doesn't make sense to shrink
// the listview to a zero size or even negative size.
//
// Parameters:
// 		CDlgMetrics& metrics
//			The dialog metrics containing the size of the things
//			that appear on the dialog and so on.
//
//		CRect& rc
//			The rectangle where the main part of the dialog will be
//			drawn.
//
// Returns:
//		The members of this class are valid on return.
//
//*****************************************************************
void CExtendedLayout::Create(CDlgMetrics& metrics, CRect& rc)
{
	m_rc = rc;

	CRect rcTemp;
	// Calculate the combined width of the treeview and listview.
	// We subtract 3 * CX_MARGIN because there is a margin on 
	// the left and right and another margin to separate the right
	// side of the list view from the button.
	int cxViews = rc.Width() - (2*metrics.m_sizeMargin.cx);
	int cxTreeView = cxViews * 2 / 5;
	int cxListView = cxViews - cxTreeView;


	// Set the location of the add button.  This should be aligned with
	// the left side of the listview and one margin height below the
	// top of the given rectangle.
	m_rcAddButton.left = m_rc.left + metrics.m_sizeMargin.cx/2 + cxTreeView - metrics.m_sizeAddButton.cx;
	m_rcAddButton.top = m_rc.top + metrics.m_cyLeading;
	m_rcAddButton.right = m_rcAddButton.left + metrics.m_sizeAddButton.cx;
	m_rcAddButton.bottom = m_rcAddButton.top + metrics.m_sizeAddButton.cy;

	// Set the location of the remove button.  This should be aligned with the
	// top of the "Add" button and one margin size to the right of the add button.
	m_rcRemoveButton.left = m_rcAddButton.right + metrics.m_sizeMargin.cx;
	m_rcRemoveButton.top = m_rcAddButton.top;
	m_rcRemoveButton.right = m_rcRemoveButton.left + metrics.m_sizeRemoveButton.cx;
	m_rcRemoveButton.bottom = m_rcRemoveButton.top + metrics.m_sizeRemoveButton.cy;


	// Set the location of label1.  This is the label at the top-left
	// of the tree control
	m_rcLabel1.left = m_rc.left + metrics.m_sizeMargin.cx;
	m_rcLabel1.top = m_rcRemoveButton.bottom + metrics.m_cyLeading +  metrics.m_sizeMargin.cy;
	m_rcLabel1.right = m_rcLabel1.left + metrics.m_sizeLabel1.cx; 
	m_rcLabel1.bottom = m_rcLabel1.top + metrics.m_sizeLabel1.cy;


	// Set the location of label2.  This is at the top-left of the list box.
	m_rcLabel2.left = m_rcLabel1.left + cxTreeView;
	m_rcLabel2.top = m_rcLabel1.top;
	m_rcLabel2.right = m_rcLabel2.left + metrics.m_sizeLabel2.cx;
	m_rcLabel2.bottom = m_rcLabel2.top + metrics.m_sizeLabel2.cy;
	
	// Set the location of the tree view.  This is one margin size from
	// the left of m_rc and one margin size below the labels.  The width 
	// has been calulated above.  There is also a margin reserved on the
	// bottom.
	m_rcTreeView.left = m_rc.left + metrics.m_sizeMargin.cx;
	m_rcTreeView.top = m_rcLabel2.bottom + 1; // + metrics.m_sizeMargin.cy;
	m_rcTreeView.right = m_rcTreeView.left + cxTreeView;
	m_rcTreeView.bottom = m_rc.bottom - metrics.m_sizeMargin.cy;
	
	
	// Set the location of the list view.  This is the same height as the
	// tree view and aligned so that its left side is adjacent to the
	// right side of the treeview.  Its width has been calculated above.
	m_rcListView.left = m_rcTreeView.right - 1;
	m_rcListView.top = m_rcTreeView.top;
	m_rcListView.right = m_rcListView.left + cxListView;
	m_rcListView.bottom = m_rcTreeView.bottom;
		

	// Set the location of the find button so that it is aligned with the top of the
	// list view and so that its right side is one margin widh from m_rc.right.
	m_rcFindButton.left = m_rc.right - metrics.m_sizeFindButton.cx - metrics.m_sizeMargin.cx;
	m_rcFindButton.top = m_rcAddButton.top;
	m_rcFindButton.right = m_rcFindButton.left + metrics.m_sizeFindButton.cx;
	m_rcFindButton.bottom = m_rcFindButton.top + metrics.m_sizeFindButton.cy;
}


//************************************************************************
// CLayout::CLayout
//
// Constructor for CLayout. This class is used to layout the items on
// the CEventTrapDialog when it is changed from the large extended view to
// the small main view.  This class also handles resizing the CEventTrapDialog.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//************************************************************************
CLayout::CLayout()
{
    m_pdlg = NULL;
}


//************************************************************************
// CLayout::Initialize
//
// Take a snapshot of various initial attributes of the dialog and its
// items. These attributes are used later to constrain the size of the
// dialog and so on.  
//
// This makes it possible to set certain characteristics of the dialog
// in the resource editor so that they do not need to be hard-coded here.
//
// Parameters:
//      CEventTrapDlg* pdlg
//          Pointer to the dialog that needs to be laid out, resized
//          and so on.
//
// Returns:
//      Nothing.
//
//***********************************************************************
void CLayout::Initialize(CEventTrapDlg* pdlg)
{
    ASSERT(m_pdlg == NULL);
    m_pdlg = pdlg;

    // Dialog layout stuff
	CRect rcWindow;
	pdlg->GetWindowRect(&rcWindow);

	CRect rcClient;
	pdlg->GetClientRect(&rcClient);


	CRect rcEventList;	
	pdlg->m_lcEvents.GetWindowRect(&rcEventList);
	pdlg->ScreenToClient(&rcEventList);
	

	m_sizeMainViewInitial.cx = rcClient.right;
	m_sizeMainViewInitial.cy = 	rcEventList.bottom + CY_MARGIN;


	m_sizeExtendedViewInitial.cx = rcClient.right;
	m_sizeExtendedViewInitial.cy = rcClient.bottom;

	m_cyMainView = 0;
	m_cyExtendedView = 0;
}


//*************************************************************************
// CLayout::ResizeMainLayout
//
// This method resizes and repositions the dialog components that appear
// int the small dialog layout.
//
// Parameters:
//      CMainLayout& layoutMain
//          The layout information for the small (main) layout.
//
// Returns:
//      Nothing.
//
//*************************************************************************
void CLayout::ResizeMainLayout(CMainLayout& layoutMain)
{
	m_pdlg->m_btnConfigTypeBox.MoveWindow(&layoutMain.m_rcConfigTypeBox, TRUE);
	m_pdlg->m_btnConfigTypeCustom.MoveWindow(&layoutMain.m_rcConfigCustomButton, TRUE);
	m_pdlg->m_btnConfigTypeDefault.MoveWindow(&layoutMain.m_rcConfigDefaultButton, TRUE);
    
	m_pdlg->m_btnOK.MoveWindow(&layoutMain.m_rcOKButton, TRUE);
	m_pdlg->m_btnCancel.MoveWindow(&layoutMain.m_rcCancelButton, TRUE);
    m_pdlg->m_btnApply.MoveWindow(&layoutMain.m_rcApplyButton, TRUE);
	m_pdlg->m_btnProps.MoveWindow(&layoutMain.m_rcPropertiesButton, TRUE);
	m_pdlg->m_btnSettings.MoveWindow(&layoutMain.m_rcSettingsButton, TRUE);
	m_pdlg->m_btnExport.MoveWindow(&layoutMain.m_rcExportButton, TRUE);
	m_pdlg->m_btnView.MoveWindow(&layoutMain.m_rcViewButton, TRUE);
	m_pdlg->m_lcEvents.MoveWindow(&layoutMain.m_rcListView, TRUE);
}



//*************************************************************************
// CLayout::ResizeExtendedLayout
//
// This method resizes and repositions the dialog components that appear
// int the large (extended) dialog layout.
//
// Parameters:
//      CExtendedLayout& layoutExtended
//          The layout information for the large (extended) layout.
//
// Returns:
//      Nothing.
//
//*************************************************************************
void CLayout::ResizeExtendedLayout(CExtendedLayout& layoutExtended)
{
	m_pdlg->m_btnAdd.MoveWindow(&layoutExtended.m_rcAddButton, TRUE);
	m_pdlg->m_btnRemove.MoveWindow(&layoutExtended.m_rcRemoveButton, TRUE);
	m_pdlg->m_btnFind.MoveWindow(&layoutExtended.m_rcFindButton, TRUE);
	m_pdlg->m_statLabel1.MoveWindow(&layoutExtended.m_rcLabel1, TRUE);
	m_pdlg->m_statLabel2.MoveWindow(&layoutExtended.m_rcLabel2, TRUE);
	m_pdlg->m_tcSource.MoveWindow(&layoutExtended.m_rcTreeView, TRUE);
	m_pdlg->m_lcSource.MoveWindow(&layoutExtended.m_rcListView, TRUE);
}




//*************************************************************************
// CLayout::LayoutAndRedraw
//
// This lays out the size and position of each component on the dialog and
// then redraws the dialog according to the new layout.
//
// Parameters:
//      BOOL bExtendedView
//          TRUE if the layout should be for the large (extended) view of
//          the dialog, FALSE if the layout should be for the small (main)
//          view of the dialog.
//
//      int cx
//          The desired width of the dialog in screen units.
//
//      int cy
//          The desired height of the dialog in screen units.
//
// Returns:
//      Nothing.
//
//*************************************************************************
void CLayout::LayoutAndRedraw(BOOL bExtendedView, int cx, int cy)
{	
	// If the user sizes the window smaller than its original size, then
	// the window will begin to obscure what is already there rather than
	// try to make things smaller.  This avoids the problems that would 
	// occur if buttons and other controls overlapped each other.
	BOOL bLayoutWidth = TRUE;
	BOOL bLayoutHeight = TRUE;

	if (bExtendedView) {
        // Limit the minimum size of the extended view
		if (cx < m_sizeExtendedViewInitial.cx) {
			cx = m_sizeExtendedViewInitial.cx;
			bLayoutWidth = FALSE;
		}

		if (cy < m_sizeExtendedViewInitial.cy) {
			cy = m_sizeExtendedViewInitial.cy;
			bLayoutHeight = FALSE;
		}
		m_cyExtendedView = cy;
	}
	else {
        // Limit the minimum size for the small (main) view
		if (cx < m_sizeMainViewInitial.cx) {
			cx = m_sizeMainViewInitial.cx;
			bLayoutWidth = FALSE;
		}

		if (cy < m_sizeMainViewInitial.cy) {
			cy = m_sizeMainViewInitial.cy;
			bLayoutHeight = FALSE;
		}
		m_cyMainView = cy;
	}



	CDlgMetrics metrics(m_pdlg);
	CMainLayout layoutMain;
	CExtendedLayout layoutExtended;
	CRect rcMain;
	CRect rcExtended;

	int cyMain = cy;
	if (bExtendedView) {
        // For the extended view, half the space if given to the components that
        // appear on the small (main) layout, and the extended components get 
        // half the space.  Thus, the dialog is split horizontally at the half-way
        // point for the extended view.
		cyMain = cy / 2;
		rcMain.SetRect(0, 0, cx, cy / 2);
		layoutMain.Create(metrics, rcMain);
		ResizeMainLayout(layoutMain);

        // The extended component rectangle's top is at the half-way point. The bottom
        // is at the bottom of the dialog.
		rcExtended.SetRect(0, cy / 2, cx, cy);
		layoutExtended.Create(metrics, rcExtended);
		ResizeExtendedLayout(layoutExtended);
	}
	else {
        // For the small (main) view, use the entire dialog.
		rcMain.SetRect(0, 0, cx, cy);
		layoutMain.Create(metrics, rcMain);
		ResizeMainLayout(layoutMain);
	}
		

	// Redraw the entire client area to fix things up since much
	// of the stuff in the client has moved around.
	CRect rcClient;
	m_pdlg->GetClientRect(&rcClient);
	m_pdlg->InvalidateRect(&rcClient);
	
}


//**************************************************************
// CLayout::LayoutView
//
// This method lays out the position and size of the CEventTrap
// dialog and the items that appear on it.
//
// Parameters:
//      BOOL bExtendedView
//          TRUE if this is a request to layout the extended (large)
//          view of the dialog.  FALSE if this is a request to layout
//          the small (main) view of the dialog.
//
// Returns:
//      Nothing.
//**************************************************************
void CLayout::LayoutView(BOOL bExtendedView)
{
	CRect rcWindow;
	m_pdlg->GetWindowRect(&rcWindow);

	CRect rcClient;
	m_pdlg->GetClientRect(&rcClient);
	m_pdlg->ClientToScreen(&rcClient);

    // cx and cy are the width and height of the dialog in client units
    // respectively.  The code below will calculate new values for cx and
    // cy to reflect the change from extended view to small (main) view
    // or vice-versa.
	int cx = rcClient.Width();
	int cy = rcClient.Height();
	int cxInitial = cx;
	int cyInitial = cy;

    // Compute the margins that intervene between the client
    // rectangle and window rectangle.
	int cxLeftMargin = rcClient.left - rcWindow.left;
	int cyTopMargin = rcClient.top - rcWindow.top;
	int cxRightMargin = rcWindow.right - rcClient.right;
	int cyBottomMargin = rcWindow.bottom - rcClient.bottom;


	CRect rc;
	m_pdlg->GetClientRect(&rc);
	
	if (bExtendedView) {
        // Control comes here if we are changing from the small main view
        // to the larger extended view.  This causes the dialog to flip 
        // back to the previous size of the extended view.  However this
        // is constrained to a minimum of the original dialog size.

        // Save the current height of the main view so that we can flip
        // back to it later.  Assume that the new height will be the
        // height of the extended view when it was flipped the last time.
		m_cyMainView = cy;
		cy = m_cyExtendedView;		

        // Constrain the height so that the mimimum height is what it
        // the initial height was for the extended view.
		if (cx < m_sizeExtendedViewInitial.cx) {
			cx = m_sizeExtendedViewInitial.cx;
		}

		if (cy < m_sizeExtendedViewInitial.cy) {
			cy = m_sizeExtendedViewInitial.cy;
		}


        // The extended view should never be smaller than the main view.
        // This check is necessary when the user resizes the window and
        // then flips the view.
        if (cy < m_cyMainView) {
            cy = m_cyMainView;
        }


		
		rc.SetRect(0, 0, cx, cy);

        // Check to see if the size changed, if not then do nothing.
        // Otherwise, resize the window.
		if ((cxInitial != cx) || (cyInitial != cy)) {			
			m_pdlg->ClientToScreen(&rc);
			rc.left -= cxLeftMargin;
			rc.top -= cyTopMargin;
			rc.right += cxRightMargin;
			rc.bottom += cyBottomMargin;
			m_pdlg->MoveWindow(&rc, TRUE);
		}
		else {
			LayoutAndRedraw(bExtendedView, cx, cy);
		}
	}

    // The main view should never be taller than the extended view.  This may
    // check is necessary if the user resized the window and then flipped to the
    // other view.
    if (m_cyMainView > m_cyExtendedView) {
        m_cyMainView = m_cyExtendedView;
    }

    // Show or hide the items in the extended portion of the dialog.
	ShowExtendedView(bExtendedView);


	if (!bExtendedView) {
		// This used to be an extended view, now we need to
		// go back to just the main view.

		// Save the current extended view height and then flip back to the
        // previously saved main (small) view height.
		m_cyExtendedView = cy;
		cy = m_cyMainView;

        // Constrain the size to be at least as large as the initial size for
        // the main (small) view.		
		if (cx < m_sizeMainViewInitial.cx) {
			cx = m_sizeMainViewInitial.cx;
		}
		if (cy < m_sizeMainViewInitial.cy) {
			cy = m_sizeMainViewInitial.cy;
		}


        // Resize the dialog only if the computed size is different
        // from the current size. Moving the window to resize it will automatically
        // cause it to be layed out correctly.
		if ((cxInitial != cx) || (cyInitial != cy)) {
			rc.SetRect(0, 0, cx, cy);
			m_pdlg->ClientToScreen(&rc);
			rc.left -= cxLeftMargin;
			rc.top -= cyTopMargin;
			rc.right += cxRightMargin;
			rc.bottom += cyBottomMargin;
			m_pdlg->MoveWindow(&rc, TRUE);
		}
		else {
			LayoutAndRedraw(bExtendedView, cx, cy);
		}
	}
}


//**************************************************************
// CLayout::ShowExtendedView
//
// This method shows or hides the dialog items that make up the
// extended portion of the dialog.
//
// Parameters:
//      BOOL bShowExtendedItems
//          TRUE if the extended items should be shown, false if
//          they should be hidden.
//
// Returns:
//      Nothing.
//**************************************************************
void CLayout::ShowExtendedView(BOOL bShowExtendedItems)
{
	m_pdlg->m_btnRemove.ShowWindow(bShowExtendedItems);
	m_pdlg->m_btnAdd.ShowWindow(bShowExtendedItems);
	m_pdlg->m_btnFind.ShowWindow(bShowExtendedItems);
	m_pdlg->m_lcSource.ShowWindow(bShowExtendedItems);
	m_pdlg->m_tcSource.ShowWindow(bShowExtendedItems);
	m_pdlg->m_statLabel1.ShowWindow(bShowExtendedItems);
	m_pdlg->m_statLabel2.ShowWindow(bShowExtendedItems);
}
