#ifndef __NCPARSEEX_H__
#define __NCPARSEEX_H__

#include <ncparse.h>

interface NcbParseEx : public NcbParse
{
	virtual BOOL setEndLine (IINST iinst, LINE lnEnd) pure;
	virtual BOOL irefEndInfo(IREF iref, OUT SZ *pszModule, OUT LINE *piline) pure;
	virtual void setModLang (IMOD imod, BYTE bLanguage) pure;
	virtual BOOL setIDLAttrib (IINST iinst, SZ szName, SZ szValue, OUT IINST *piinst) pure;
	virtual BOOL setIDLMFCComment (IINST iinst, SZ szType, SZ szValue, OUT IINST *piinst) pure;
};


class CBscLock
{
private:
    NcbParseEx * m_pDBase;
public:
    CBscLock (NcbParseEx * pDBase)
    {
        m_pDBase = pDBase;
        m_pDBase->lock();
    };

    ~CBscLock()
    {
        m_pDBase->unlock();
        m_pDBase = NULL;
    };
};


#endif __NCPARSEEX__