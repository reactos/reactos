/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    hwprof.cpp

Abstract:

    This module implements CHwProfileList and CHwProfile classes.

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "hwprof.h"


BOOL
CHwProfileList::Create(
    CDevice* pDevice,
    DWORD ConfigFlags
    )
{
    // first get the current profile index.
    HWPROFILEINFO   HwProfileInfo;
    ASSERT(pDevice);
    m_pDevice = pDevice;
    // get the current profile index.
    if (!m_pDevice->m_pMachine->CmGetCurrentHwProfile(&m_CurHwProfile))
    return FALSE;
    // go through each profile and create a CHwProfile for it
    int Index = 0;
    CHwProfile* phwpf;
    while (m_pDevice->m_pMachine->CmGetHwProfileInfo(Index, &HwProfileInfo))
    {
    DWORD hwpfFlags;
    // get the hwprofile flags for this device
    // if failed, use the given ConfigFlags
    if (m_pDevice->m_pMachine->CmGetHwProfileFlags((
                LPTSTR)m_pDevice->GetDeviceID(),
                HwProfileInfo.HWPI_ulHWProfile,
                &hwpfFlags))
    {
        if (hwpfFlags & CSCONFIGFLAG_DO_NOT_CREATE)
        {
        // skip this profile
        Index++;
        continue;
        }
    }
    else
    {
        // flags have not been set for this profile yet.
        hwpfFlags = ConfigFlags;
    }

    ASSERT(CONFIGFLAG_DISABLED == CSCONFIGFLAG_DISABLED);

    hwpfFlags |= ConfigFlags;
    // rememeber current hw profile index
    if (m_CurHwProfile == HwProfileInfo.HWPI_ulHWProfile)
        m_CurHwProfileIndex = Index;
    phwpf = new CHwProfile(Index, &HwProfileInfo, pDevice, hwpfFlags);
    m_listProfile.AddTail(phwpf);
    Index++;
    }
    return TRUE;

}

CHwProfileList::~CHwProfileList()
{
    if (!m_listProfile.IsEmpty())
    {
    POSITION pos = m_listProfile.GetHeadPosition();
    while (NULL != pos)
    {
        CHwProfile* pProfile =  m_listProfile.GetNext(pos);
        delete pProfile;
    }
    m_listProfile.RemoveAll();
    }
}

BOOL
CHwProfileList::GetFirst(
    CHwProfile** pphwpf,
    PVOID&  Context
    )
{
    ASSERT(pphwpf);

    if (!m_listProfile.IsEmpty())
    {
        POSITION pos = m_listProfile.GetHeadPosition();
        *pphwpf = m_listProfile.GetNext(pos);
        Context = pos;
        return TRUE;
    }
    
    Context = NULL;
    *pphwpf = NULL;
    return FALSE;
}

BOOL
CHwProfileList::GetNext(
    CHwProfile** pphwpf,
    PVOID&  Context
    )
{
    ASSERT(pphwpf);
    POSITION pos = (POSITION)Context;

    if (NULL != pos)
    {
        *pphwpf = m_listProfile.GetNext(pos);
        Context = pos;
        return TRUE;
    }
    
    *pphwpf = NULL;
    return FALSE;
}

BOOL 
CHwProfileList::GetCurrentHwProfile(
    CHwProfile** pphwpf
    )
{
    ASSERT(pphwpf);

    POSITION pos = m_listProfile.FindIndex(m_CurHwProfileIndex);
    *pphwpf = m_listProfile.GetAt(pos);

    return TRUE;
}

ULONG
CHwProfileList::IndexToHwProfile(
    int HwProfileIndex
    )
{
    if (HwProfileIndex >= m_listProfile.GetCount())
    return 0XFFFFFFFF;
    POSITION pos = m_listProfile.FindIndex(HwProfileIndex);
    CHwProfile* pHwProfile = m_listProfile.GetAt(pos);
    return pHwProfile->GetHwProfile();
}

CHwProfile::CHwProfile(
    int Index,
    PHWPROFILEINFO phwpfInfo,
    CDevice* pDevice,
    DWORD Flags
    )
{
    m_Index = Index;
    m_hwpfInfo = *phwpfInfo;
    m_pDevice = pDevice;
    m_EnablePending = FALSE;
    m_DisablePending = FALSE;
    m_Flags = Flags;

}
