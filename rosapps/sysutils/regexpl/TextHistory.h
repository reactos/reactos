/* $Id: TextHistory.h,v 1.2 2001/01/13 23:55:37 narnaoud Exp $ */

// TextHistory.h: interface for the CTextHistory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TEXTHISTORY_H__AD9C6555_1D97_11D4_9F58_204C4F4F5020__INCLUDED_)
#define TEXTHISTORY_H__AD9C6555_1D97_11D4_9F58_204C4F4F5020__INCLUDED_

class CTextHistory  
{
public:
	CTextHistory();
	virtual ~CTextHistory();
	BOOL Init(DWORD dwMaxHistoryLineSize, DWORD dwMaxHistoryLines);
	const TCHAR * GetHistoryLine(DWORD dwIndex);
	void AddHistoryLine(const TCHAR *pchLine);
private:
	TCHAR *m_pHistoryBuffer;
	DWORD m_dwMaxHistoryLines;
	DWORD m_dwMaxHistoryLineSize;
	DWORD m_dwFirstHistoryIndex;
	DWORD m_dwLastHistoryIndex;
	DWORD m_dwHisoryFull;
};

#endif // !defined(TEXTHISTORY_H__AD9C6555_1D97_11D4_9F58_204C4F4F5020__INCLUDED_)
