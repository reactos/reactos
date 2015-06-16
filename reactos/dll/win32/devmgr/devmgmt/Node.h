#pragma once

#define DISPLAY_NAME_LEN    256
#define ROOT_NAME_SIZE      MAX_COMPUTERNAME_LENGTH + 1

enum NodeType
{
    NodeClass,
    NodeDevice
};

typedef ULONG Actions;
#define Update      0x01
#define Enable      0x02
#define Disable     0x04
#define Uninstall   0x08


class CNode
{
private:
    PSP_CLASSIMAGELIST_DATA m_ImageListData;
    NodeType m_NodeType;
    DEVINST m_DevInst;
    Actions m_Actions;
    LPWSTR m_DeviceId;
    WCHAR m_DisplayName[DISPLAY_NAME_LEN];
    GUID  m_ClassGuid;
    INT m_ClassImage;
    ULONG m_Status;
    ULONG m_ProblemNumber;
    INT m_OverlayImage;

public:
    CNode(
        _In_ LPGUID ClassGuid,
        _In_ PSP_CLASSIMAGELIST_DATA ImageListData
        );

    CNode(
        _In_ DEVINST Device,
        _In_ PSP_CLASSIMAGELIST_DATA ImageListData
        );

    ~CNode();

    bool Setup();

    LPGUID GetClassGuid() { return &m_ClassGuid;  }
    DEVINST GetDeviceInst() { return m_DevInst; }

    LPWSTR GetDisplayName() { return m_DisplayName; }
    INT GetClassImage() { return m_ClassImage; }
    INT GetOverlayImage() { return m_OverlayImage; }
    LPWSTR GetDeviceId() { return m_DeviceId; }
    Actions GetActions() { return m_Actions; }

    bool HasProblem() { return !!(m_ProblemNumber); }
    bool HasProperties();
    bool IsHidden();
    bool CanDisable();
    bool IsDisabled();
    bool IsStarted();
    bool IsInstalled();
    bool CanInstall() { return TRUE; } // unimplemented
    bool CanUninstall() { return TRUE; } // unimplemented

private:
    bool SetupClassNode();
    bool SetupDeviceNode();
    void Cleanup();

    DWORD ConvertResourceDescriptorToString(
        _Inout_z_ LPWSTR ResourceDescriptor,
        _In_ DWORD ResourceDescriptorSize
        );
};

