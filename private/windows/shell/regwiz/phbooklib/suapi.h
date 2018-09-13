// ############################################################################
//#include "ras.h"
#define cbDataCenter (MAX_PATH+1)	// max length of data center string

// ############################################################################
class CDialog
{
public:
	void far * operator new( size_t cb ) { return GlobalAlloc(GPTR,cb); };
	void operator delete( void far * p ) {GlobalFree(p); };

	CDialog() {};
	~CDialog() {};
	virtual LRESULT DlgProc(HWND, UINT, WPARAM, LPARAM, LRESULT)=0;
};

// ############################################################################
class CSelectNumDlg : public CDialog
{
public:
	CSelectNumDlg();
	LRESULT DlgProc(HWND, UINT, WPARAM, LPARAM, LRESULT);
	DWORD m_dwCountryID;
	DWORD m_dwCountryIDOrg;
	WORD m_wRegion;
	DWORD_PTR m_dwPhoneBook;
	char m_szPhoneNumber[RAS_MaxPhoneNumber+1];
	char m_szDunFile[cbDataCenter];
	BYTE m_fType;
	BYTE m_bMask;
	DWORD m_dwFlags;
private:
	BOOL FHasPhoneNumbers(LPLINECOUNTRYENTRY pLCE);
	BOOL m_fHasRegions;
	HWND m_hwndDlg;
	HRESULT FillRegion();
	HRESULT FillNumber();
};

// ############################################################################
/********** 1/9/96 jmazner Normandy #13185
class CAccessNumDlg : public CDialog
{
public:
	CAccessNumDlg();
	~CAccessNumDlg() {};
	LRESULT			DlgProc(HWND, UINT, WPARAM, LPARAM, LRESULT);
	DWORD			m_dwPhoneBook;
	PACCESSENTRY	*m_rgAccessEntry;
	WORD			m_wNumber;
	DWORD			m_dwCountryID;
	WORD			m_wRegion;
	BYTE			m_fType;
	BYTE			m_bMask;
	char	m_szPrimary[RAS_MaxPhoneNumber];
	char	m_szSecondary[RAS_MaxPhoneNumber];
	char	m_szDunPrimary[cbDataCenter + sizeof('\0')];
	char	m_szDunSecondary[cbDataCenter + sizeof('\0')];
};	
**************/

// ############################################################################
#ifdef WIN16
extern "C" BOOL CALLBACK __export PhbkGenericDlgProc(
#else
extern "C" __declspec(dllexport) BOOL CALLBACK PhbkGenericDlgProc(
#endif
    HWND  hwndDlg,	// handle to dialog box
    UINT  uMsg,	// message
    WPARAM  wParam,	// first message parameter
    LPARAM  lParam 	// second message parameter
   );

