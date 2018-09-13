#define SCORE_NONE	0
#define SCORE_MINOR	1
#define SCORE_MAJOR	2

class CIncdKorean : public CINetCodeDetector
{
private:

	enum {NO_ESC, sESC, sESC_1, sESC_2 } m_nEscMode;
	BOOL m_bFindDesigator;
	INT  m_nCharCount;

public:
	CIncdKorean();
	~CIncdKorean() {}

protected:
	virtual BOOL DetectChar(UCHAR tc);
	virtual BOOL CleanUp() {return FALSE;}
	virtual int GetDetectedCodeSet();
};
