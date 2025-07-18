#pragma once

#define DISPLAY_NAME_LEN    256

enum NodeType
{
    RootNode,
    ClassNode,
    DeviceNode,
    ResourceNode,
    ResourceTypeNode
};

class CNode
{
protected:
    NodeType m_NodeType;
    PSP_CLASSIMAGELIST_DATA m_ImageListData;
    LPWSTR m_DeviceId;
    WCHAR m_DisplayName[DISPLAY_NAME_LEN];
    GUID  m_ClassGuid;
    INT m_ClassImage;

public:
    CNode(
        _In_ NodeType Type,
        _In_ PSP_CLASSIMAGELIST_DATA ImageListData
        );

    CNode(
        _In_ const CNode& Node
        );

    virtual ~CNode();

    virtual bool SetupNode() = 0;

    NodeType GetNodeType() { return m_NodeType; }
    LPGUID GetClassGuid() { return &m_ClassGuid; }
    LPWSTR GetDisplayName() { return m_DisplayName; }
    INT GetClassImage() { return m_ClassImage; }
    LPWSTR GetDeviceId() { return m_DeviceId; }
    bool HasProperties() { return (m_DeviceId != NULL); }
};

