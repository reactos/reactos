#pragma once
#include "Node.h"

class CClassNode : public CNode
{
public:

    CClassNode(
        _In_ LPGUID ClassGuid,
        _In_ PSP_CLASSIMAGELIST_DATA ImageListData
        );

    ~CClassNode();

    virtual bool SetupNode();

private:

    DWORD ConvertResourceDescriptorToString(
        _Inout_z_ LPWSTR ResourceDescriptor,
        _In_ DWORD ResourceDescriptorSize
        );
};

