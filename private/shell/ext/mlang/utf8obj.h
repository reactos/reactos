#include "convbase.h"

#define HIGHT_SURROGATE_START   0xD800
#define HIGHT_SURROGATE_END     0xDBFF
#define LOW_SURROGATE_START     0xDC00
#define LOW_SURROGATE_END       0xDFFF

class CInccUTF8In : public CINetCodeConverter
{
private:
	BOOL (CInccUTF8In::*m_pfnConv)(UCHAR tc);
	BOOL (CInccUTF8In::*m_pfnCleanUp)();

	WORD    m_tcUnicode ;
	DWORD   m_tcSurrogateUnicode ;
	int     m_nByteFollow;
	int     m_nBytesUsed;
    BOOL    m_fSurrogatesPairs;
public:
	CInccUTF8In(UINT uCodePage, int nCodeSet);
	~CInccUTF8In() {}
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
	virtual BOOL CleanUp();
	virtual int GetUnconvertBytes();
	virtual DWORD GetConvertMode();
	virtual void SetConvertMode(DWORD mode);

private:
    void Reset(); 
	BOOL ConvMain(UCHAR tc);
	BOOL CleanUpMain();

};

class CInccUTF8Out : public CINetCodeConverter
{
private:
	BOOL    m_fDoubleByte;
	BYTE    m_tcLeadByte;
    WCHAR   m_wchSurrogateHigh;

public:
	CInccUTF8Out(UINT uCodePage, int nCodeSet);
	~CInccUTF8Out() {}
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
	virtual BOOL CleanUp();
	virtual int GetUnconvertBytes();
	virtual DWORD GetConvertMode();
	virtual void SetConvertMode(DWORD mode);
private:
    void Reset(); 
};
