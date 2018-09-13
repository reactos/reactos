// MLSWalk.h : Declaration of the CMLStrWalkW and CMLStrWalkA

#ifndef __MLSWALK_H_
#define __MLSWALK_H_

/////////////////////////////////////////////////////////////////////////////
// CMLStrWalkW

class CMLStrWalkW
{
public:
    inline CMLStrWalkW(IMLangStringWStr* pMLStrW, long lPos, long lLen, long lFlags = MLSTR_READ, BOOL fCanStopAtMiddle = FALSE);
    BOOL Lock(HRESULT& rhr);
    void Unlock(HRESULT& rhr, long lActualLen = 0);
    inline WCHAR* GetStr(void);
    inline long GetCCh(void) const;
    long GetPos(void) const {return m_lPos + m_lDoneLen;}
    long GetLen(void) const {return m_lLen - m_lDoneLen;}
    long GetDoneLen(void) const {return m_lDoneLen;}

protected:
    IMLangStringWStr* m_pMLStr;
    BOOL m_fCanStopAtMiddle;
    long m_lPos;
    long m_lLen;
    long m_lFlags;
    long m_lDoneLen;
    WCHAR* m_pszBuf;
    long m_cchBuf;
    long m_lLockLen;
};

/////////////////////////////////////////////////////////////////////////////
// CMLStrWalkW inline functions

CMLStrWalkW::CMLStrWalkW(IMLangStringWStr* pMLStr, long lPos, long lLen, long lFlags, BOOL fCanStopAtMiddle) :
    m_pMLStr(pMLStr),
    m_fCanStopAtMiddle(fCanStopAtMiddle)
{
    m_lPos = lPos;
    m_lLen = lLen;
    m_lFlags = lFlags;
    m_lDoneLen = 0;

    m_pszBuf = NULL; // Mark as it's not locked
}

WCHAR* CMLStrWalkW::GetStr(void)
{
    ASSERT(m_pszBuf); // Not locked
    return m_pszBuf;
}

long CMLStrWalkW::GetCCh(void) const
{
    ASSERT(m_pszBuf); // Not locked
    if (m_pszBuf)
        return m_cchBuf;
    else
        return 0;
}

#endif //__MLSWALK_H_
