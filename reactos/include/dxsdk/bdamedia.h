#ifndef BDAMEDIA_H__
#define BDAMEDIA_H__

typedef enum
{
    KSPROPERTY_BDA_PIN_ID = 0,
    KSPROPERTY_BDA_PIN_TYPE
} KSPROPERTY_BDA_PIN_CONTROL;

typedef struct _KSP_BDA_NODE_PIN {
    KSPROPERTY Property;
    ULONG ulNodeType;
    ULONG ulInputPinId;
    ULONG ulOutputPinId;
} KSP_BDA_NODE_PIN, *PKSP_BDA_NODE_PIN;

#endif
