/*************************************************************
 grpinfo.cpp

  User Manager security group info class + list implementation

 History:
  09/23/98: dsheldon created
*************************************************************/

#include "stdafx.h"
#include "resource.h"

#include "grpinfo.h"

/*******************************
 CGroupInfoList implementation
*******************************/

CGroupInfoList::CGroupInfoList()
{
    TraceEnter(TRACE_USR_CORE, "CGroupInfoList::CGroupInfoList");

    SetHDPA(NULL);

    TraceLeaveVoid();
}

CGroupInfoList::~CGroupInfoList()
{
    TraceEnter(TRACE_USR_CORE, "CGroupInfoList::~CGroupInfoList");

    if (GetHDPA() != NULL)
        DestroyCallback(DestroyGroupInfoCallback, NULL);

    TraceLeaveVoid();
}

int CGroupInfoList::DestroyGroupInfoCallback(LPVOID p, LPVOID pData)
{
    TraceEnter(TRACE_USR_CORE, "::DestroyGroupInfoCallback");
    
    CGroupInfo* pGroupInfo = (CGroupInfo*) p;
    delete pGroupInfo;

    TraceLeaveValue(0);
}

HRESULT CGroupInfoList::Initialize()
{
    TraceEnter(TRACE_USR_CORE, "CGroupInfoList::Initialize");
    USES_CONVERSION;    
    HRESULT hr = S_OK;
    
    NET_API_STATUS status;
    DWORD_PTR dwResumeHandle = 0;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;

    if (GetHDPA() != NULL)
    {
        DestroyCallback(DestroyGroupInfoCallback, NULL);
    }

    // Create new list initially with 8 items
    if (Create(8))
    {
        // Now begin enumerating local groups
        LOCALGROUP_INFO_1* prgGroupInfo;

        // Read each local group
        BOOL fBreakLoop = FALSE;
        while (!fBreakLoop)
        {
            status = NetLocalGroupEnum(NULL, 1, (BYTE**) &prgGroupInfo, 
                8192, &dwEntriesRead, &dwTotalEntries, 
                &dwResumeHandle);

            if ((status == NERR_Success) || (status == ERROR_MORE_DATA))
            {
                // We got some local groups - add information for all users in these local
                // groups to our list
                DWORD iGroup;
                for (iGroup = 0; iGroup < dwEntriesRead; iGroup ++)
                {

                    AddGroupToList(W2T(prgGroupInfo[iGroup].lgrpi1_name), 
                        W2T(prgGroupInfo[iGroup].lgrpi1_comment));
                }

                NetApiBufferFree((BYTE*) prgGroupInfo);
            
                // Maybe we don't have to try NetLocalGroupEnum again (if we got all the groups)
                fBreakLoop = (dwEntriesRead == dwTotalEntries);
            }
            else
            {
                // Check for access denied

                fBreakLoop = TRUE;
                hr = E_FAIL;
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceLeaveResult(hr);
}


HRESULT CGroupInfoList::AddGroupToList(LPCTSTR szGroup, LPCTSTR szComment)
{
    TraceEnter(TRACE_USR_CORE, "CGroupInfoList::AddGroupToList");
    HRESULT hr = S_OK;
    
    CGroupInfo* pGroupInfo = new CGroupInfo();

    if (pGroupInfo)
    {
        lstrcpyn(pGroupInfo->m_szGroup, szGroup, ARRAYSIZE(pGroupInfo->m_szGroup));
        lstrcpyn(pGroupInfo->m_szComment, szComment, ARRAYSIZE(pGroupInfo->m_szComment));
        AppendPtr(pGroupInfo);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceMsg("Couldn't create new CGroupInfo");
    }

    TraceLeaveResult(hr);
}
