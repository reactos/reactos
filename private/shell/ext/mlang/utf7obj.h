#include "convbase.h"

class CInccUTF7In : public CINetCodeConverter
{
private:
    BOOL (CInccUTF7In::*m_pfnConv)(UCHAR tc);
    BOOL (CInccUTF7In::*m_pfnCleanUp)();

    LONG m_tcUnicode ;
    BOOL m_fUTF7Mode;
    int  m_nBitCount;
    int  m_nOutCount;

public:
    CInccUTF7In(UINT uCodePage, int nCodeSet);
    ~CInccUTF7In() {}
    virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
    virtual BOOL CleanUp();
    virtual int GetUnconvertBytes();
    virtual DWORD GetConvertMode();
    virtual void SetConvertMode(DWORD mode);

private:
    void Reset();    // initialization
    BOOL ConvMain(UCHAR tc);
    BOOL CleanUpMain();
};

class CInccUTF7Out : public CINetCodeConverter
{
private:
    BOOL m_fUTF7Mode;
    BOOL m_fDoubleByte;
    BYTE m_tcFirstByte;
    int  m_nBitCount;
    LONG m_tcUnicode ;

public:
    CInccUTF7Out(UINT uCodePage, int nCodeSet);
    ~CInccUTF7Out() {}
    virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
    virtual BOOL CleanUp();
    virtual int GetUnconvertBytes();
    virtual DWORD GetConvertMode();
    virtual void SetConvertMode(DWORD mode);
private:
    void Reset();    // initialization
};
