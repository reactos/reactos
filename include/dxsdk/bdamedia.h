#ifndef BDAMEDIA_H__
#define BDAMEDIA_H__

typedef struct _KSP_BDA_NODE_PIN {
    KSPROPERTY Property;
    ULONG ulNodeType;
    ULONG ulInputPinId;
    ULONG ulOutputPinId;
} KSP_BDA_NODE_PIN, *PKSP_BDA_NODE_PIN;

typedef struct _KSM_BDA_PIN
{
    KSMETHOD    Method;
    union
    {
        ULONG       PinId;
        ULONG       PinType;
    };
    ULONG       Reserved;
} KSM_BDA_PIN, * PKSM_BDA_PIN;

typedef struct _KSM_BDA_PIN_PAIR
{
    KSMETHOD    Method;
    union
    {
        ULONG       InputPinId;
        ULONG       InputPinType;
    };
    union
    {
        ULONG       OutputPinId;
        ULONG       OutputPinType;
    };
} KSM_BDA_PIN_PAIR, * PKSM_BDA_PIN_PAIR;

/* ------------------------------------------------------------
    BDA Topology Property Set {A14EE835-0A23-11d3-9CC7-00C04F7971E0}
*/

#define STATIC_KSPROPSETID_BdaTopology \
    0xa14ee835, 0x0a23, 0x11d3, 0x9c, 0xc7, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("A14EE835-0A23-11d3-9CC7-00C04F7971E0", KSPROPSETID_BdaTopology);
#define KSPROPSETID_BdaTopology DEFINE_GUIDNAMED(KSPROPSETID_BdaTopology)

typedef enum {
    KSPROPERTY_BDA_NODE_TYPES,
    KSPROPERTY_BDA_PIN_TYPES,
    KSPROPERTY_BDA_TEMPLATE_CONNECTIONS,
    KSPROPERTY_BDA_NODE_METHODS,
    KSPROPERTY_BDA_NODE_PROPERTIES,
    KSPROPERTY_BDA_NODE_EVENTS,
    KSPROPERTY_BDA_CONTROLLING_PIN_ID,
    KSPROPERTY_BDA_NODE_DESCRIPTORS
 }KSPROPERTY_BDA_TOPOLOGY;

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_TYPES(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_TYPES,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_PIN_TYPES(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIN_TYPES,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_TEMPLATE_CONNECTIONS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_TEMPLATE_CONNECTIONS,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof( BDA_TEMPLATE_CONNECTION),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_METHODS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_METHODS,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_PROPERTIES(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_PROPERTIES,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_EVENTS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_EVENTS,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_CONTROLLING_PIN_ID(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_CONTROLLING_PIN_ID,\
        (GetHandler),\
        sizeof(KSP_BDA_NODE_PIN),\
        sizeof( ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_DESCRIPTORS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_DESCRIPTORS,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

/* ------------------------------------------------------------
    BDA Device Configuration Method Set {71985F45-1CA1-11d3-9CC8-00C04F7971E0}
*/

#define STATIC_KSMETHODSETID_BdaDeviceConfiguration \
    0x71985f45, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F45-1CA1-11d3-9CC8-00C04F7971E0", KSMETHODSETID_BdaDeviceConfiguration);
#define KSMETHODSETID_BdaDeviceConfiguration DEFINE_GUIDNAMED(KSMETHODSETID_BdaDeviceConfiguration)

typedef enum {
    KSMETHOD_BDA_CREATE_PIN_FACTORY = 0,
    KSMETHOD_BDA_DELETE_PIN_FACTORY,
    KSMETHOD_BDA_CREATE_TOPOLOGY
} KSMETHOD_BDA_DEVICE_CONFIGURATION;

#define DEFINE_KSMETHOD_ITEM_BDA_CREATE_PIN_FACTORY(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_CREATE_PIN_FACTORY,\
        KSMETHOD_TYPE_READ,\
        (MethodHandler),\
        sizeof(KSM_BDA_PIN),\
        sizeof(ULONG),\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_DELETE_PIN_FACTORY(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_DELETE_PIN_FACTORY,\
        KSMETHOD_TYPE_NONE,\
        (MethodHandler),\
        sizeof(KSM_BDA_PIN),\
        0,\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_CREATE_TOPOLOGY(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_CREATE_TOPOLOGY,\
        KSMETHOD_TYPE_WRITE,\
        (MethodHandler),\
        sizeof(KSM_BDA_PIN_PAIR),\
        0,\
        SupportHandler)

/* ------------------------------------------------------------
  BDA Pin Control Property {0DED49D5-A8B7-4d5d-97A1-12B0C195874D}
*/

#define STATIC_KSPROPSETID_BdaPinControl \
    0xded49d5, 0xa8b7, 0x4d5d, 0x97, 0xa1, 0x12, 0xb0, 0xc1, 0x95, 0x87, 0x4d
DEFINE_GUIDSTRUCT("0DED49D5-A8B7-4d5d-97A1-12B0C195874D", KSPROPSETID_BdaPinControl);
#define KSPROPSETID_BdaPinControl DEFINE_GUIDNAMED(KSPROPSETID_BdaPinControl)

typedef enum {
    KSPROPERTY_BDA_PIN_ID = 0,
    KSPROPERTY_BDA_PIN_TYPE
} KSPROPERTY_BDA_PIN_CONTROL;

#define DEFINE_KSPROPERTY_ITEM_BDA_PIN_ID(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIN_ID,\
        (GetHandler),\
        sizeof( KSPROPERTY),\
        sizeof( ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_PIN_TYPE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIN_TYPE,\
        (GetHandler),\
        sizeof( KSPROPERTY),\
        sizeof( ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)


/* ------------------------------------------------------------
  BDA Change Sync Method Set {FD0A5AF3-B41D-11d2-9C95-00C04F7971E0}
*/

#define STATIC_KSMETHODSETID_BdaChangeSync \
    0xfd0a5af3, 0xb41d, 0x11d2, {0x9c, 0x95, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}
DEFINE_GUIDSTRUCT("FD0A5AF3-B41D-11d2-9C95-00C04F7971E0", KSMETHODSETID_BdaChangeSync);
#define KSMETHODSETID_BdaChangeSync DEFINE_GUIDNAMED(KSMETHODSETID_BdaChangeSync)

typedef enum {
    KSMETHOD_BDA_START_CHANGES = 0,
    KSMETHOD_BDA_CHECK_CHANGES,
    KSMETHOD_BDA_COMMIT_CHANGES,
    KSMETHOD_BDA_GET_CHANGE_STATE
} KSMETHOD_BDA_CHANGE_SYNC;

#define DEFINE_KSMETHOD_ITEM_BDA_START_CHANGES(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_START_CHANGES,\
        KSMETHOD_TYPE_NONE,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        0,\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_CHECK_CHANGES(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_CHECK_CHANGES,\
        KSMETHOD_TYPE_NONE,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        0,\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_COMMIT_CHANGES(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_COMMIT_CHANGES,\
        KSMETHOD_TYPE_NONE,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        0,\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_GET_CHANGE_STATE(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_GET_CHANGE_STATE,\
        KSMETHOD_TYPE_READ,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        0,\
        SupportHandler)

#define STATIC_KSPROPSETID_BdaFrequencyFilter \
    0x71985f47, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F47-1CA1-11d3-9CC8-00C04F7971E0", KSPROPSETID_BdaFrequencyFilter);
#define KSPROPSETID_BdaFrequencyFilter DEFINE_GUIDNAMED(KSPROPSETID_BdaFrequencyFilter)

typedef enum {
    KSPROPERTY_BDA_RF_TUNER_FREQUENCY = 0,
    KSPROPERTY_BDA_RF_TUNER_POLARITY,
    KSPROPERTY_BDA_RF_TUNER_RANGE,
    KSPROPERTY_BDA_RF_TUNER_TRANSPONDER,
    KSPROPERTY_BDA_RF_TUNER_BANDWIDTH,
    KSPROPERTY_BDA_RF_TUNER_FREQUENCY_MULTIPLIER,
    KSPROPERTY_BDA_RF_TUNER_CAPS,
    KSPROPERTY_BDA_RF_TUNER_SCAN_STATUS,
    KSPROPERTY_BDA_RF_TUNER_STANDARD,
    KSPROPERTY_BDA_RF_TUNER_STANDARD_MODE
}KSPROPERTY_BDA_FREQUENCY_FILTER;


#define STATIC_KSPROPSETID_BdaDigitalDemodulator \
    0xef30f379, 0x985b, 0x4d10, 0xb6, 0x40, 0xa7, 0x9d, 0x5e, 0x4, 0xe1, 0xe0
DEFINE_GUIDSTRUCT("EF30F379-985B-4d10-B640-A79D5E04E1E0", KSPROPSETID_BdaDigitalDemodulator);
#define KSPROPSETID_BdaDigitalDemodulator DEFINE_GUIDNAMED(KSPROPSETID_BdaDigitalDemodulator)

typedef enum {
    KSPROPERTY_BDA_MODULATION_TYPE = 0,
    KSPROPERTY_BDA_INNER_FEC_TYPE,
    KSPROPERTY_BDA_INNER_FEC_RATE,
    KSPROPERTY_BDA_OUTER_FEC_TYPE,
    KSPROPERTY_BDA_OUTER_FEC_RATE,
    KSPROPERTY_BDA_SYMBOL_RATE,
    KSPROPERTY_BDA_SPECTRAL_INVERSION,
    KSPROPERTY_BDA_GUARD_INTERVAL,
    KSPROPERTY_BDA_TRANSMISSION_MODE,
    KSPROPERTY_BDA_ROLL_OFF,
    KSPROPERTY_BDA_PILOT,
    KSPROPERTY_BDA_SIGNALTIMEOUTS
}KSPROPERTY_BDA_DIGITAL_DEMODULATOR;

#define STATIC_KSPROPSETID_BdaLNBInfo \
    0x992cf102, 0x49f9, 0x4719, 0xa6, 0x64, 0xc4, 0xf2, 0x3e, 0x24, 0x8, 0xf4
DEFINE_GUIDSTRUCT("992CF102-49F9-4719-A664-C4F23E2408F4", KSPROPSETID_BdaLNBInfo);
#define KSPROPSETID_BdaLNBInfo DEFINE_GUIDNAMED(KSPROPSETID_BdaLNBInfo)

typedef enum {
    KSPROPERTY_BDA_LNB_LOF_LOW_BAND = 0,
    KSPROPERTY_BDA_LNB_LOF_HIGH_BAND,
    KSPROPERTY_BDA_LNB_SWITCH_FREQUENCY
}KSPROPERTY_BDA_LNB_INFO;

#define STATIC_KSPROPSETID_BdaSignalStats \
    0x1347d106, 0xcf3a, 0x428a, 0xa5, 0xcb, 0xac, 0xd, 0x9a, 0x2a, 0x43, 0x38
DEFINE_GUIDSTRUCT("1347D106-CF3A-428a-A5CB-AC0D9A2A4338", KSPROPSETID_BdaSignalStats);
#define KSPROPSETID_BdaSignalStats DEFINE_GUIDNAMED(KSPROPSETID_BdaSignalStats)

typedef enum {
    KSPROPERTY_BDA_SIGNAL_STRENGTH = 0,
    KSPROPERTY_BDA_SIGNAL_QUALITY,
    KSPROPERTY_BDA_SIGNAL_PRESENT,
    KSPROPERTY_BDA_SIGNAL_LOCKED,
    KSPROPERTY_BDA_SAMPLE_TIME
}KSPROPERTY_BDA_SIGNAL_STATS;


#endif
