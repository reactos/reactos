#ifndef _RSCCHVR_H
#define _RSCCHVR_H

#include "rgsprtc.h"

class CRSCacheVersion : public CRegSupportCached
{
public:
    void RSCVInitSubKey(LPCTSTR pszSubKey);
    LPCTSTR RSCVGetSubKey();

protected:
    void _RSCVUpdateVersionOnCacheRead();
    void _RSCVUpdateVersionOnCacheWrite();
    BOOL _RSCVIsValidVersion();
    void _RSCVIncrementRegVersion();

    virtual void _RSCVDeleteRegCache() = 0;

#ifdef DEBUG
protected:
#else
private:
#endif

    DWORD               _dwVersion;
    LPCTSTR             _pszSubKey;
};

#endif //_RSCCHVR_H