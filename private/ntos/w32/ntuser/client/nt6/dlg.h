//
// CDlg
//
// FelixA
//
// used to be called CDialog
// 

#ifndef __DIALOGH
#define __DIALOGH

class CDlg
{
public:
	void SetInstance(HINSTANCE hInst);
	void SetDlgID(UINT id);
	void Destroy();
	CDlg(int DlgID, HWND hWndParent, HINSTANCE hInst);
	~CDlg();

	HWND GetWindow() const { return m_hDlg; }
	HWND GetParent() const { return ::GetParent(m_hDlg); }
	HWND GetDlgItem(int iD) const { return ::GetDlgItem(m_hDlg,iD); }
	HINSTANCE GetInstance() const { return m_Inst;}
	BOOL EndDialog(int iRet) { return ::EndDialog(m_hDlg,iRet); }

	// If you want your own dlg proc.
	int Do();
	virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
	virtual int DoCommand(WORD wCmdID,WORD hHow);	// return 0 if you handled this.
	virtual void OnInit();
	virtual int DoNotify(NMHDR * pHdr);
	HWND CreateModeless();

private:
	BOOL m_bCreatedModeless;
	int				m_DlgID;
	HWND			m_hDlg;
	HWND			m_hParent;
	HINSTANCE		m_Inst;

protected:
	void SetWindow(HWND hDlg) { m_hDlg=hDlg; }
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};


#endif
