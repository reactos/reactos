#pragma once
#include "Node.h"

class CRootNode : public CNode
{
private:
    DEVINST m_DevInst;

public:
    CRootNode(_In_ PSP_CLASSIMAGELIST_DATA ImageListData);
    ~CRootNode();

    virtual bool SetupNode();

    DEVINST GetDeviceInst() { return m_DevInst; }
};

