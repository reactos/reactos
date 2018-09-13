#ifndef GRPINFO_H_INCLUDED
#define GRPINFO_H_INCLUDED

#include "dpa.h"

class CGroupInfo
{
public:
    // Functions
    CGroupInfo()
    {
        m_szGroup[0] = m_szComment[0] = TEXT('\0');
    }

public:
    // Data
    TCHAR m_szGroup[MAX_GROUP + 1];
    TCHAR m_szComment[MAXCOMMENTSZ];

};

class CGroupInfoList: public CDPA<CGroupInfo>
{
public:
    CGroupInfoList();
    ~CGroupInfoList();

    HRESULT Initialize();

private:
    static int CALLBACK DestroyGroupInfoCallback(LPVOID p, LPVOID pData);

    HRESULT AddGroupToList(LPCTSTR szGroup, LPCTSTR szComment);
};

#endif // !GRPINFO_H_INCLUDED