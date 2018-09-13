
/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    hwprof.h

Abstract:

    header file for hwprof.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/
class CHwProfile;
class CHwProfileList;

class CHwProfileList
{
public:
    CHwProfileList() : m_pDevice(NULL), m_CurHwProfile(0), m_CurHwProfileIndex(0) {}
    ~CHwProfileList();
    BOOL Create(CDevice* pDevice, DWORD ConfigFlags = 0);
    BOOL GetFirst(CHwProfile** pphwpf, PVOID& Context);
    BOOL GetNext(CHwProfile** pphwpf, PVOID& Context);
    BOOL GetCurrentHwProfile(CHwProfile** pphwpf);
    int GetCurrentHwProfileIndex()
    {
        return m_CurHwProfileIndex;
    }
    ULONG IndexToHwProfile(int HwProfileIndex);
    int GetCount()
    {
        return m_listProfile.GetCount();
    }

private:
    CList<CHwProfile*,CHwProfile* > m_listProfile;
    CDevice*    m_pDevice;
    ULONG   m_CurHwProfile;
    int     m_CurHwProfileIndex;
};

class CHwProfile
{
public:
    CHwProfile(int Index, PHWPROFILEINFO phwpfInfo, CDevice* pDevice, DWORD hwpfFlags);
    DWORD GetFlags()
    {
        return m_Flags;
    }
    LPCTSTR GetFriendlyName() const
    {
        return m_hwpfInfo.HWPI_szFriendlyName;
    }
    DWORD GetIndex()
    {
        return m_Index;
    }

    ULONG GetHwProfile()
    {
         return m_hwpfInfo.HWPI_ulHWProfile;
    }
    BOOL IsDisabled()
    {
        return (m_Flags & CSCONFIGFLAG_DISABLED);
    }
    BOOL DoNotCreate()
    {
        return (m_Flags & CSCONFIGFLAG_DO_NOT_CREATE);
    }
    BOOL DoNotStart()
    {
        return (m_Flags & CSCONFIGFLAG_DO_NOT_START);
    }
    void SetEnablePending()
    {
        m_EnablePending = TRUE;
    }
    void ResetEnablePending()
    {
        m_EnablePending = FALSE;
    }
    void SetDisablePending()
    {
        m_DisablePending = TRUE;
    }
    void ResetDisablePending()
    {
        m_DisablePending = FALSE;
    }
    BOOL IsEnablePending()
    {
        return m_EnablePending;
    }
    BOOL IsDisablePending()
    {
        return m_DisablePending;
    }

private:
    DWORD       m_Index;
    HWPROFILEINFO   m_hwpfInfo;
    DWORD       m_Flags;
    CDevice*        m_pDevice;
    BOOL        m_EnablePending;
    BOOL        m_DisablePending;
};
