#include "convbase.h"

enum KANA_MODE
{
    FULL_MODE    = 0,
    ESC_MODE     = 1,
    SIO_MODE     = 2,
};

enum JIS_ESC_STATE
{
    JIS_ASCII    = 0,
    JIS_Roman    = 1,
    JIS_Kana     = 2,
    JIS_DoubleByte = 3,
};

class CInccJisIn : public CINetCodeConverter
{
private:
	BOOL (CInccJisIn::*m_pfnConv)(UCHAR tc);        
	BOOL (CInccJisIn::*m_pfnCleanUp)();
	BOOL m_fShift;                                  /* Shift in/out control */
	BOOL m_fJapan;                                  /* IN_JP OUT_JP control */
	BOOL m_fLeadByte;                               /* Shift in and lead byte flag */
	UCHAR m_tcLeadByte;                             /* perserve the last lead byte */
	UINT m_nESCBytes;                               /* # bytes of ESC sequence */
	JIS_ESC_STATE m_eEscState;                      /* State of ESC sequence */

public:
	CInccJisIn(UINT uCodePage, int nCodeSet);
	~CInccJisIn() {}
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
	BOOL ConvIsoInJp(UCHAR tc);
	BOOL CleanUpIsoInJp();
	BOOL ConvIsoOut(UCHAR tc);
	BOOL CleanUpIsoOut();
	BOOL ConvStar(UCHAR tc);
	BOOL CleanUpStar();
	BOOL ConvDoubleByte(UCHAR tc);
	BOOL CleanUpDoubleByte();
};

class CInccJisOut : public CINetCodeConverter
{
private:
	BOOL m_fDoubleByte;
	UCHAR m_tcLeadByte;     // use for DBCS lead byte
	UCHAR m_tcPrevByte;     // use for half width kana as a saved previous byte

	BOOL m_fKana;
	BOOL m_fJapan;
	BOOL m_fSaveByte;

	KANA_MODE m_eKanaMode ;  // half width kana convert method

public:
	CInccJisOut(UINT uCodePage, int nCodeSet);
	~CInccJisOut() {}
	virtual HRESULT ConvertChar(UCHAR tc, int cchSrc=-1);
	virtual BOOL CleanUp();
	virtual int GetUnconvertBytes();
	virtual DWORD GetConvertMode();
	virtual void SetConvertMode(DWORD mode);
	void SetKanaMode(UINT uCodePage);
private:
    void Reset();
	BOOL ConvFullWidthKana(UCHAR tc);
	BOOL KanaCleanUp();
};
