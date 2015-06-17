#pragma once
#include "Node.h"

class CDeviceNode : public CNode
{
private:
    DEVINST m_DevInst;
    ULONG m_Status;
    ULONG m_ProblemNumber;
    int m_OverlayImage;

public:
    CDeviceNode(
        _In_opt_ DEVINST Device,
        _In_ PSP_CLASSIMAGELIST_DATA ImageListData
        );

    ~CDeviceNode();

    virtual bool SetupNode();

    DEVINST GetDeviceInst() { return m_DevInst; }
    int GetOverlayImage() { return m_OverlayImage; }

    bool HasProblem() { return !!(m_ProblemNumber); }
    bool IsHidden();
    bool CanDisable();
    bool IsDisabled();
    bool IsStarted();
    bool IsInstalled();
    bool CanInstall() { return TRUE; } // unimplemented
    bool CanUninstall() { return TRUE; } // unimplemented
};

