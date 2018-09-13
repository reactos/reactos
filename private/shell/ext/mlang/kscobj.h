#include "convbase.h"

class CInccKscIn : public CINetCodeConverter
{
private:
	BOOL (CInccKscIn::*m_pfnConv)(UCHAR tc);
	BOOL (CInccKscIn::*m_pfnCleanUp)();
	BOOL m_fShift;
	BOOL m_fKorea;
	BOOL m_fLeadByte;
	UINT m_nESCBytes;                               /* # bytes of ESC sequence */

public:
	CInccKscIn(UINT uCodePage, int nCodeSet);
	~CInccKscIn() {}
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
	virtual BOOL CleanUp();
	virtual int GetUnconvertBytes();
	virtual DWORD GetConvertMode();
	virtual void SetConvertMode(DWORD mode);

private:
    void Reset();
	BOOL ConvMain(UCHAR tc);
	BOOL CleanUpMain();
	BOOL ConvEsc(UCHAR tc);
	BOOL CleanUpEsc();
	BOOL ConvIsoIn(UCHAR tc);
	BOOL CleanUpIsoIn();
	BOOL ConvIsoInKr(UCHAR tc);
	BOOL CleanUpIsoInKr();
};

class CInccKscOut : public CINetCodeConverter
{
private:
    BOOL    m_fDoubleByte;
    BYTE    m_tcLeadByte;
    DWORD   _dwFlag;
    BOOL    m_fShift;
    BOOL    m_fKorea;
    WCHAR  *_lpFallBack;

public:
    CInccKscOut(UINT uCodePage, int nCodeSet, DWORD dwFlag, WCHAR *lpFallBack);
    ~CInccKscOut() {}
    virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
    virtual BOOL CleanUp();
    virtual int GetUnconvertBytes();
    virtual DWORD GetConvertMode();
    virtual void SetConvertMode(DWORD mode);
private:
    void Reset();
};
