#include "convbase.h"

class CInccEucJIn : public CINetCodeConverter
{
private:
	BOOL (CInccEucJIn::*m_pfnConv)(UCHAR tc);
	BOOL (CInccEucJIn::*m_pfnCleanUp)();
	UCHAR m_tcLeadByte;                             /* perserve the last lead byte */

public:
	CInccEucJIn(UINT uCodePage, int nCodeSet);
	~CInccEucJIn() {}
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
	virtual BOOL CleanUp();
	virtual int GetUnconvertBytes();
	virtual DWORD GetConvertMode();
	virtual void SetConvertMode(DWORD mode);

private:
    void Reset();
	BOOL ConvMain(UCHAR tc);
	BOOL CleanUpMain();
	BOOL ConvDoubleByte(UCHAR tc);
	BOOL CleanUpDoubleByte();
	BOOL ConvKatakana(UCHAR tc);
	BOOL CleanUpKatakana();
};

class CInccEucJOut : public CINetCodeConverter
{
private:
	BOOL m_fDoubleByte;
	BYTE m_tcLeadByte;
    DWORD   _dwFlag;
    WCHAR   *_lpFallBack;

public:
	CInccEucJOut(UINT uCodePage, int nCodeSet,  DWORD dwFlag, WCHAR *lpFallBack);
	~CInccEucJOut() {}
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
	virtual BOOL CleanUp();
	virtual int GetUnconvertBytes();
	virtual DWORD GetConvertMode();
	virtual void SetConvertMode(DWORD mode);
private:
    void Reset();
};
