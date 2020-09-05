/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file mintopo.h was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

#ifndef _MINTOPO_H_
#define _MINTOPO_H_

#include "shared.h"
#include "guids.h"
#ifdef INCLUDE_PRIVATE_PROPERTY
#include "prvprop.h"
#endif


/*****************************************************************************
 * Structures and Definitions
 */

// This structure is used to translate the TopoPins member to a (registered)
// system pin member or vice versa.
typedef struct
{
    TopoPins    PinDef;
    int         PinNr;
} tPinTranslationTable;


// This structure is used to translate the TopoNodes member to a (registered)
// system node member or vice versa.
typedef struct
{
    TopoNodes   NodeDef;
    int         NodeNr;
} tNodeTranslationTable;

// max. number of connections
const int TOPO_MAX_CONNECTIONS = 0x80;

// Looking at the structure stMapNodeToReg, it defines a mask for a node.
// A volume node for instance could be used to prg. the left or right channel,
// so we have to mask the mask in stMapNodeToReg with a "left channel mask"
// or a "right channel mask" to only prg. the left or right channel.
const WORD AC97REG_MASK_LEFT  = 0x3F00;
const WORD AC97REG_MASK_RIGHT = 0x003F;
const WORD AC97REG_MASK_MUTE  = 0x8000;

// When the user moves the fader of a volume control to the bottom, the
// system calls the property handler with a -MAXIMUM dB value. That's not
// what you returned at a basic support "data range", but the most negative
// value you can represent with a long.
const long PROP_MOST_NEGATIVE = (long)0x80000000;

// We must cache the values for the volume and tone controls. If we don't,
// the controls will "jump".
// We have around 50% volume controls in the nodes. One way would be to
// create a separate structure for the node cache and a second array for
// the mapping. But we just use one big array (that could cache mute controls
// too) and in that way simplify the code a little.
// We use the bLeftValid and bRightValid to indicate whether the values stored
// in lLeft and lRight are true - remember at startup we only write and
// initialize the AC97 registers and not this structure. But as soon as there
// is a property get or a property set, this element (node) will be stored here
// and marked valid.
// We don't have to save/restore the virtual controls for WaveIn and MonoOut
// in the registry, cause WDMAUD will do this for every node that is exposed
// in the topology. We can also be sure that this structure gets initialized at
// startup because WDMAUD will query every node that it finds.
typedef struct
{
    long    lLeft;
    long    lRight;
    BYTE    bLeftValid;
    BYTE    bRightValid;
} tNodeCache;


/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CAC97MiniportTopology
 *****************************************************************************
 * AC97 topology miniport.  This object is associated with the device and is
 * created when the device is started.  The class inherits IMiniportTopology
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CAC97MiniportTopology : public IAC97MiniportTopology,
                                       public CUnknown
{
private:
    PADAPTERCOMMON           AdapterCommon;          // Adapter common object
    PPCFILTER_DESCRIPTOR     FilterDescriptor;       // Filter Descriptor
    PPCPIN_DESCRIPTOR        PinDescriptors;         // Pin Descriptors
    PPCNODE_DESCRIPTOR       NodeDescriptors;        // Node Descriptors
    PPCCONNECTION_DESCRIPTOR ConnectionDescriptors;  // Connection Descriptors

    tPinTranslationTable  stPinTrans[PIN_TOP_ELEMENT];   // pin translation table
    tNodeTranslationTable stNodeTrans[NODE_TOP_ELEMENT]; // node translation table
    tNodeCache            stNodeCache[NODE_TOP_ELEMENT]; // cache for nodes.
    BOOL                  m_bCopyProtectFlag;            // Copy protect flag for DRM.

    /*************************************************************************
     * CAC97MiniportTopology methods
     *
     * These are private member functions used internally by the object.  See
     * MINIPORT.CPP for specific descriptions.
     */

    // builds the topology.
    NTSTATUS BuildTopology (void);

    // registers (builds) the pins
    NTSTATUS BuildPinDescriptors (void);

    // registers (builds) the nodes
    NTSTATUS BuildNodeDescriptors (void);

    // registers (builds) the connection between the pin, nodes.
    NTSTATUS BuildConnectionDescriptors (void);

#if (DBG)
    // dumps the topology. you can specify if you want to dump pins, nodes,
    // connections (see debug.h).
    void DumpTopology (void);
#endif

    // translates the system node id to a TopoNode.
    TopoNodes TransNodeNrToNodeDef (IN int Node)
    {
#if (DBG)
        TopoNodes   n;

        n = stNodeTrans[Node].NodeDef;
        // check for invalid translation. could be caused by a connection
        // to a not registered node or wrong use of nodes.
        if (n == NODE_INVALID)
            DOUT (DBG_ERROR, ("Invalid node nr %u.", Node));
        return n;
#else
        return stNodeTrans[Node].NodeDef;
#endif
    };

    // translates a TopoNode to a system node id.
    int TransNodeDefToNodeNr (IN TopoNodes Node)
    {
#if (DBG)
        int n;

        // check for invalid translation. could be caused by a connection
        // to a not registered node or wrong use of nodes.
        n = stNodeTrans[Node].NodeNr;
        if (n == -1)
            DOUT (DBG_ERROR, ("Invalid TopoNode %u.", Node));
        return n;
#else
        return stNodeTrans[Node].NodeNr;
#endif
    };

    // translates a system pin id to a TopoPin.
    TopoPins TransPinNrToPinDef (IN int Pin)
    {
#if (DBG)
        TopoPins p;

        p = stPinTrans[Pin].PinDef;
        // check for invalid translation. could be caused by a connection
        // to a not registered pin or wrong use of pins.
        if (p == PIN_INVALID)
            DOUT (DBG_ERROR, ("Invalid pin nr %u.", Pin));
        return p;
#else
        return stPinTrans[Pin].PinDef;
#endif
    };

    // translates a TopoPin to a system pin id.
    int TransPinDefToPinNr (IN TopoPins Pin)
    {
#if (DBG)
        int p;

        p = stPinTrans[Pin].PinNr;
        // check for invalid translation. could be caused by a connection
        // to a not registered pin or wrong use of pins.
        if (p == -1)
            DOUT (DBG_ERROR, ("Invalid TopoPin %u.", Pin));
        return p;
#else
        return stPinTrans[Pin].PinNr;
#endif
    };

    // sets one table entry for translation.
    void SetNodeTranslation (IN int NodeNr, IN TopoNodes NodeDef)
    {
        stNodeTrans[NodeNr].NodeDef = NodeDef;
        stNodeTrans[NodeDef].NodeNr = NodeNr;
    };

    // sets one table entry for translation.
    void SetPinTranslation (IN int PinNr, IN TopoPins PinDef)
    {
        stPinTrans[PinNr].PinDef = PinDef;
        stPinTrans[PinDef].PinNr = PinNr;
    };

    // Updates the record mute - used for DRM.
    void UpdateRecordMute (void);

public:
    /*************************************************************************
     * The following two macros are from STDUNK.H.  DECLARE_STD_UNKNOWN()
     * defines inline IUnknown implementations that use CUnknown's aggregation
     * support.  NonDelegatingQueryInterface() is declared, but it cannot be
     * implemented generically.  Its definition appears in MINIPORT.CPP.
     * DEFINE_STD_CONSTRUCTOR() defines inline a constructor which accepts
     * only the outer unknown, which is used for aggregation.  The standard
     * create macro (in MINIPORT.CPP) uses this constructor.
     */
    DECLARE_STD_UNKNOWN ();
    DEFINE_STD_CONSTRUCTOR (CAC97MiniportTopology);

    ~CAC97MiniportTopology ();

    /*************************************************************************
     * include IMiniportTopology (public/exported) methods
     */
    IMP_IMiniportTopology;

    /*************************************************************************
     * IAC97MiniportTopology methods
     */
    // returns the system pin id's for wave out, wave in and mic in.
    STDMETHODIMP_(NTSTATUS) GetPhysicalConnectionPins
    (
        OUT PULONG              WaveOutSource,
        OUT PULONG              WaveInDest,
        OUT PULONG              MicInDest
    );

    // sets the copy protect flag.
    STDMETHODIMP_(void) SetCopyProtectFlag (BOOL flag)
    {
        if (m_bCopyProtectFlag != flag)
        {
            m_bCopyProtectFlag = flag;
            UpdateRecordMute ();
        }
    };

    /*************************************************************************
     * static functions for the property handler.
     * They are not part of an COM interface and are called directly.
     */

    // Get the data range of volume or tone controls in dB.
    // Called by the property handler.
    static NTSTATUS GetDBValues
    (
        IN  PADAPTERCOMMON,
        IN  TopoNodes       Node,
        OUT LONG            *plMinimum,
        OUT LONG            *plMaximum,
        OUT ULONG           *puStep
    );

    // Calculates the speaker mute with respect to master mono.
    // Called by the property handler.
    static NTSTATUS SetMultichannelMute
    (
        IN CAC97MiniportTopology *that,
        IN TopoNodes Mute
    );

    // Calculates the speaker volume with respect to master mono.
    // Called by the property handler.
    static NTSTATUS SetMultichannelVolume
    (
        IN CAC97MiniportTopology *that,
        IN TopoNodes Volume
    );

    // property handler for mute controls of checkboxes like Loudness.
    static NTSTATUS PropertyHandler_OnOff
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );

    // This routine is the basic support handler for volume or tone controls.
    static NTSTATUS BasicSupportHandler
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );

    // property handler for all volume controls.
    static NTSTATUS PropertyHandler_Level
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );

    // property handler for tone controls.
    static NTSTATUS PropertyHandler_Tone
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );

    // property handler for muxer. we have just two muxer, one for recording
    // and one for mono out.
    static NTSTATUS PropertyHandler_Ulong
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );

    // this says that audio is played and processed without CPU resources.
    static NTSTATUS PropertyHandler_CpuResources
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );

#ifdef INCLUDE_PRIVATE_PROPERTY
    // property handler for private properties. we currently have only
    // one request defined (KSPROPERTY_AC97_FEATURES).
    static NTSTATUS PropertyHandler_Private
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
#endif
};

#endif  // _MINTOPO_H_
