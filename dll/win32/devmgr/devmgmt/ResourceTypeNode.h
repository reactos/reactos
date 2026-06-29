#pragma once
#include "Node.h"

class CResourceTypeNode : public CNode
{
public:

    CResourceTypeNode(
        _In_ UINT ResId,
        _In_ PSP_CLASSIMAGELIST_DATA ImageListData
        );

    ~CResourceTypeNode();

    virtual bool SetupNode();

};
