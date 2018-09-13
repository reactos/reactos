
#ifndef CONVBASE_H_
#define CONVBASE_H_

#define MAXOVERFLOWCHARS 16

class CINetCodeConverter
{
private:
	UINT m_uCodePage;
	int m_nCodeSet;
	BOOL m_fOutput;
	LPSTR m_lpDestStr;
	int m_cchDest;
	int m_cchOutput;
	int m_cchOverflow;
	UCHAR m_OverflowBuffer[MAXOVERFLOWCHARS];

public:
	CINetCodeConverter();
	CINetCodeConverter(UINT uCodePage, int nCodeSet);
	virtual ~CINetCodeConverter() {}
	int GetCodeSet() {return m_nCodeSet;}
	HRESULT GetStringSizeA(LPCSTR lpSrcStr, int cchSrc, LPINT lpnSize = NULL);
	HRESULT ConvertStringA(LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest, LPINT lpnSize = NULL);
	virtual int GetUnconvertBytes() = 0 ;
	virtual DWORD GetConvertMode() = 0 ;
	virtual void SetConvertMode(DWORD mode) = 0 ;

private:
	HRESULT WalkString(LPCSTR lpSrcStr, int cchSrc, LPINT lpnSize);
	BOOL EndOfDest(UCHAR tc);
	BOOL OutputOverflowBuffer();

protected:
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1) = 0;
	virtual BOOL CleanUp() = 0;

protected:
	inline BOOL Output(UCHAR tc)
	{
		BOOL fDone = TRUE;

		if (m_fOutput) {
			if (m_cchOutput < m_cchDest) {
				*m_lpDestStr++ = tc;
			} else {
				(void)EndOfDest(tc);
				fDone = FALSE;
			}
		}

		m_cchOutput++;

		return fDone;
	}
};

#endif /* CONVBASE_H_ */
