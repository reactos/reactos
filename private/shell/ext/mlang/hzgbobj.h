#include "convbase.h"

class CInccHzGbIn : public CINetCodeConverter
{
private:
	BOOL (CInccHzGbIn::*m_pfnConv)(UCHAR tc);
	BOOL (CInccHzGbIn::*m_pfnCleanUp)();
	BOOL m_fGBMode;
	UCHAR m_tcLeadByte;
	UINT  m_nESCBytes;                     /* # bytes of ESC sequence */

public:
	CInccHzGbIn();
	CInccHzGbIn(UINT uCodePage, int nCodeSet);
	~CInccHzGbIn() {}
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
	virtual BOOL CleanUp();
	virtual int GetUnconvertBytes();
	virtual DWORD GetConvertMode();
	virtual void SetConvertMode(DWORD mode);

private:
    void Reset();
	BOOL ConvMain(UCHAR tc);
	BOOL CleanUpMain();
	BOOL ConvTilde(UCHAR tc);
	BOOL CleanUpTilde();
	BOOL ConvDoubleByte(UCHAR tc);
	BOOL CleanUpDoubleByte();
};

class CInccHzGbOut : public CINetCodeConverter
{
private:
    BOOL    m_fDoubleByte;
    UCHAR   m_tcLeadByte;
    BOOL    m_fGBMode;
    DWORD   _dwFlag;
    WCHAR   *_lpFallBack;

public:
	CInccHzGbOut(UINT uCodePage, int nCodeSet, DWORD dwFlag, WCHAR *lpFallBack);
	~CInccHzGbOut() {}
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
	virtual BOOL CleanUp();
	virtual int GetUnconvertBytes();
	virtual DWORD GetConvertMode();
	virtual void SetConvertMode(DWORD mode);
private:
    void Reset();
};
