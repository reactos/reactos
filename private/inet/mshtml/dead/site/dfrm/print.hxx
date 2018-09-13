//+------------------------------------------------------------------------
//
//  File:       PRINTMAN.HXX
//
//  Contents:   Helper object for DataDoc printing
//
//  Classes:    CPrintMan; encapsulates a printer DC.
//
//  History:
//      12/02/94    LaszloG Created
//
//-------------------------------------------------------------------------

#ifndef _PRINT_HXX_
#define _PRINT_HXX_

class CPrintMan
{
public:
	//	Constructor, destructor
	CPrintMan();
	CPrintMan(HWND hwnd);
	~CPrintMan();

	HDC GetDC(void)		{ return _hDC; };

	HRESULT Setup(BOOL fShowPrintDialog);

	BOOL StartDoc(LPCTSTR pszDocName);
	BOOL EndDoc(void);
	BOOL AbortDoc(void);

	BOOL StartPage(void);
	BOOL EndPage(void);

protected:
	void RemoveAbortProc(void);

private:
	HDC		_hDC;			//	The printer device context
	BOOL	_fPrinting;
	HWND	_hdlgAbort;
	HWND	_hwndParent;
	LPCTSTR	_pszDocName;
	int		_iJob;			//	Job number of the print job
};

#endif

//
//	End of file
//
///////////////////////////////////////////////////////////
