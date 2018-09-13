#ifndef __WIZARD_PAGE_BASECLASS_H
#define __WIZARD_PAGE_BASECLASS_H

//
// Special "Wizard Page Messages"
//

#include "AccWiz.h" // JMC: TODO: Maybe move this somewhere else

class CWizardPageOrder
{
public:
	CWizardPageOrder()
	{
		m_nCount = 0;
		for(int i=0;i<ARRAYSIZE(m_rgdwPageIds);i++)
			m_rgdwPageIds[i] = 0;
	}


	BOOL AddPages(DWORD nInsertAfter, DWORD *rgdwIds, int nCount)
	{
		// First remove the pages if they are already there
		RemovePages(rgdwIds, nCount);

		int nStart = m_nCount - 1; // This will add to the end of the array
		if(0xFFFFFFFF != nInsertAfter)
		{
			for(nStart = 0;nStart < m_nCount;nStart++)
				if(m_rgdwPageIds[nStart] == nInsertAfter) break;
			if(nStart >= m_nCount)
			{
				_ASSERTE(FALSE); // The specified insert after was not in the array
				return FALSE;
			}
		}
		
		// Check to see if we have enough space.
		if(nCount + m_nCount > ARRAYSIZE(m_rgdwPageIds))
		{
			_ASSERTE(FALSE); // We don't have space
			return FALSE;
		}

		// Move current allocation upwards
		for(int i=m_nCount-1;i>nStart;i--)
			m_rgdwPageIds[i + nCount] = m_rgdwPageIds[i];

		// Insert new values
		for(i = 0;i<nCount;i++)
			m_rgdwPageIds[nStart + i + 1] = rgdwIds[i];

		// Set new value for m_nCount
		m_nCount += nCount;

		return TRUE;
	}

	BOOL RemovePages(DWORD *rgdwIds, int nCount)
	{
		// NOTE: This will scan the array and find the max and min locations
		// of all the elements in rgdwIds.  It then removes everything from min to max.
		// This is needed in case a sub page added more sub pages.
		int nMin = m_nCount + 1;
		int nMax = 0;
		for(int i=0;i<m_nCount;i++)
		{
			for(int j=0;j<nCount;j++)
			{
				if(m_rgdwPageIds[i] == rgdwIds[j])
				{
					nMin = min(i, nMin);
					nMax = max(i, nMax);
				}
			}
		}
		if(nMax < nMin)
		{
//			_ASSERTE(FALSE); // we could not find the range
			return FALSE;
		}

		// Move elements down
		int nCountElementsToRemove = nMax - nMin + 1;
		for(i=0;i<m_nCount - (nMax + 1);i++)
			m_rgdwPageIds[nMin + i] = m_rgdwPageIds[nMin + i + nCountElementsToRemove];

		// Figure out new m_nCount;
		m_nCount -= nCountElementsToRemove;
		return TRUE;
	}

	DWORD GetNextPage(DWORD dwPageId)
	{
		DWORD dwNextPage = 0;
		// Find the specified page
		for(int i=0;i<m_nCount;i++)
			if(m_rgdwPageIds[i] == dwPageId) break;

		if(i>=m_nCount)
		{
			_ASSERTE(FALSE); // We could not find the current page
			return 0;
		}

		// If we are not on the last page, return the 'next' page
		if(i < (m_nCount-1))
			dwNextPage = m_rgdwPageIds[i+1];

		return dwNextPage;
	}
	DWORD GetPrevPage(DWORD dwPageId)
	{
		DWORD dwPrevPage = 0;
		// Find the specified page
		for(int i=0;i<m_nCount;i++)
			if(m_rgdwPageIds[i] == dwPageId) break;

		if(i>=m_nCount)
		{
			_ASSERTE(FALSE); // We could not find the current page
			return 0;
		}

		// If we are not on the first page, return the 'prev' page
		if(i > 0)
			dwPrevPage = m_rgdwPageIds[i - 1];

		return dwPrevPage;
	}

	DWORD GetFirstPage()
	{
		_ASSERTE(m_nCount); // only call if we have values in the class
		return m_rgdwPageIds[0];
	}

	BOOL GrowArray(int nNewMax)
	{
		_ASSERTE(FALSE); // Not yet implemented
		return FALSE;
	}


protected:
	int m_nCount;
	DWORD m_rgdwPageIds[100]; // JMC: NOTE: We hard code a max of 100 pages that this
							// object can support.  100 is reasonable, since wizards
							// cannot currently support more than 100 pages.
};

class WizardPage
{
public:
	WizardPage(LPPROPSHEETPAGE ppsp, int nIdTitle, int nIdSubTitle);
	virtual ~WizardPage(VOID);
	
	//
	// Object is to apply settings to the system so that they take effect.
	//
	virtual LRESULT ApplySettings(VOID)
	{ return 0; }
	//
	// Object reports if user has changed something in the wizard page.
	//
	virtual BOOL Changed(VOID)
	{ return FALSE; }
	//
	// Object is to restore the original settings in effect when the page
	// was first opened.
	// Don't appy these to the system.	Object will receive an
	// ApplySettings notification when this is required.
	//
	virtual VOID RestoreOriginalSettings(VOID)
	{ /* By default, nothing happens */ }
	//
	// Object is to restore the settings most previously applied.
	// Don't appy these to the system.	Object will receive an
	// ApplySettings notification when this is required.
	//
	virtual VOID RestorePreviousSettings(VOID)
	{ /* By default, nothing happens */ }
	
	// This static member contains the order for all wizard pages in the app
	static CWizardPageOrder sm_WizPageOrder;
	
protected:
	HWND m_hwnd;  // Dialog's hwnd.
	DWORD m_dwPageId;
	
	virtual BOOL AdjustWizPageOrder()
	{
		// Default does nothing
		return TRUE;
	}
	
	//
	// Derived classes override these to respond to page create/release
	// notifications.
	//
	virtual UINT OnPropSheetPageCreate(HWND hwnd, LPPROPSHEETPAGE ppsp)
	{ return 1; }
	virtual UINT OnPropSheetPageRelease(HWND hwnd, LPPROPSHEETPAGE ppsp)
	{ return 1; }
	
	//
	// Method for performing operations common to all wizard pages in response
	// to given messages.  This is the function given to the PROPSHEETPAGE struct.
	//
	static INT_PTR DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
protected:
	//
	// Prevent copying.
	//
	WizardPage(const WizardPage& rhs);
	WizardPage& operator = (const WizardPage& rhs);
	
	static UINT PropSheetPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
	
	virtual LRESULT HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{ return 0; }
	virtual LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{ return 1; }
	//
	// Property sheet notifications.
	//
	virtual LRESULT OnPSN_Apply(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
	{ return 0; }
	virtual LRESULT OnPSN_Help(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
	{ return 0; }
	virtual LRESULT OnPSN_KillActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
	{ return 0; }
	virtual LRESULT OnPSN_QueryCancel(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	virtual LRESULT OnPSN_Reset(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
	{ return 0; }
	virtual LRESULT OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	virtual LRESULT OnPSN_WizBack(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
	{
		AdjustWizPageOrder();
		SetWindowLongPtr(hwnd, DWLP_MSGRESULT,
                         sm_WizPageOrder.GetPrevPage(m_dwPageId));
		return TRUE;
	}
	virtual LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
	{
		AdjustWizPageOrder();
		SetWindowLongPtr(hwnd, DWLP_MSGRESULT,
                         sm_WizPageOrder.GetNextPage(m_dwPageId));
		return TRUE;
	}
	virtual LRESULT OnPSN_WizFinish(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
	{ return 0; }
	
	
	virtual LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{ return 0; }
	
	virtual LRESULT OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{ return 0; }
	
	virtual LRESULT OnTimer(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{ return 0; }

	virtual BOOL OnMsgNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
	{ return 0; }

	LRESULT OnPSM_QuerySiblings(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);
	
};



#endif // __WIZARD_PAGE_H

