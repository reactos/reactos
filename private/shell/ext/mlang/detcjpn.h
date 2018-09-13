#define SCORE_NONE	0
#define SCORE_MINOR	1
#define SCORE_MAJOR	2

class CIncdJapanese : public CINetCodeDetector
{
private:
	long m_nScoreJis;
	long m_nScoreEuc;
	long m_nScoreSJis;

	enum {NONE, ISO_ESC, ISO_ESC_IN, ISO_ESC_OUT } m_nISOMode;
	enum {REGULAR, DOUBLEBYTE, KATAKANA} m_nEucMode, m_nJISMode ;
	BOOL m_fDoubleByteSJis;

public:
	CIncdJapanese();
	~CIncdJapanese() {}

protected:
	virtual BOOL CheckISOChar(UCHAR tc);
	virtual BOOL DetectChar(UCHAR tc);
	virtual BOOL CleanUp() {return FALSE;}
	virtual int GetDetectedCodeSet();
};
