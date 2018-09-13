
class CINetCodeDetector
{
public:
	CINetCodeDetector() {}
	virtual ~CINetCodeDetector() {}
	int DetectStringA(LPCSTR lpSrcStr, int cchSrc);

protected:
	virtual BOOL DetectChar(UCHAR tc) = 0;
	virtual BOOL CleanUp() = 0;
	virtual int GetDetectedCodeSet() = 0;
};
