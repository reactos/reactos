#pragma once
#include "Node.h"

enum IconOverlays
{
    OverlayProblem = 1,
    OverlayDisabled,
    OverlayInfo
};

class CDeviceNode : public CNode
{
private:
    SP_DEVINFO_DATA m_DevinfoData;
    DEVINST m_DevInst;
    HDEVINFO m_hDevInfo;

    ULONG m_Status;
    ULONG m_ProblemNumber;
    int m_OverlayImage;

public:
    CDeviceNode(
        _In_opt_ DEVINST Device,
        _In_ PSP_CLASSIMAGELIST_DATA ImageListData
        );

    ~CDeviceNode();

    CDeviceNode(
        _In_ const CDeviceNode &Node
        );

    virtual bool SetupNode();

    DEVINST GetDeviceInst() { return m_DevInst; }
    int GetOverlayImage() { return m_OverlayImage; }

    bool HasProblem();
    bool IsHidden();
    bool CanDisable();
    virtual bool IsDisabled();
    bool IsStarted();
    bool IsInstalled();
    bool CanUninstall();
    virtual bool CanUpdate() { return true; } // unimplemented

    bool EnableDevice(
        _In_ bool Enable,
        _Out_ bool &NeedsReboot
        );

    bool UninstallDevice(
        );

private:
    void Cleanup(
        );

    bool SetFlags(
        _In_ DWORD Flags,
        _In_ DWORD FlagsEx
        );

    bool RemoveFlags(
        _In_ DWORD Flags,
        _In_ DWORD FlagsEx
        );

    DWORD GetFlags(
        );
};

