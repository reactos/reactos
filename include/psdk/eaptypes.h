#ifndef EAPTYPES_H
#define EAPTYPES_H

typedef struct _EAP_TYPE
{
    BYTE type;
    DWORD dwVendorId;
    DWORD dwVendorType;
} EAP_TYPE, *PEAP_TYPE;

typedef struct _EAP_METHOD_TYPE
{
    EAP_TYPE eapType;
    DWORD dwAuthorId;
} EAP_METHOD_TYPE, *PEAP_METHOD_TYPE;

#endif
