/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file common.h was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

#ifndef _COMMON_H_
#define _COMMON_H_

#include "shared.h"

/*****************************************************************************
 * Structs
 *****************************************************************************
 */
 
//                                              
// Contains pin and node configuration of the AC97 codec.
//
typedef struct
{
    // For nodes.
    struct
    {
        BOOL    bNodeConfig;
    } Nodes[NODEC_TOP_ELEMENT];

    // For pins.
    struct
    {
        BOOL    bPinConfig;
        PWCHAR  sRegistryName;
    } Pins[PINC_TOP_ELEMENT];
} tHardwareConfig;

//
// We cache the AC97 registers.  Additionally, we want some default values
// when the driver comes up first that are different from the HW default
// values. The string in the structure is the name of the registry entry
// that can be used instead of the hard coded default value.
//
typedef struct
{
    WORD    wCache;
    WORD    wFlags;
    PWCHAR  sRegistryName;
    WORD    wWantedDefault;
} tAC97Registers;


/*****************************************************************************
 * Constants
 *****************************************************************************
 */

//
// This means shadow register are to be read at least once to initialize.
//
const WORD SHREG_INVALID = 0x0001;

//
// This means shadow register should be overwritten with default value at
// driver init.
//
const WORD SHREG_INIT = 0x0002;

//
// This constant is used to prevent register caching.
//
const WORD SHREG_NOCACHE = 0x0004;


/*****************************************************************************
 * Classes
 *****************************************************************************
 */

/*****************************************************************************
 * CAC97AdapterCommon
 *****************************************************************************
 * This is the common adapter object shared by all miniports to access the
 * hardware.
 */
class CAC97AdapterCommon : public IAC97AdapterCommon, 
                       public IAdapterPowerManagement,
                       public CUnknown
{
private:
    static tAC97Registers m_stAC97Registers[64];    // The shadow registers.
    static tHardwareConfig m_stHardwareConfig;      // The hardware configuration.
    PDEVICE_OBJECT m_pDeviceObject;     // Device object used for registry access.
    PWORD          m_pCodecBase;        // The AC97 I/O port address.
    PUCHAR         m_pBusMasterBase;    // The Bus Master base address.
    BOOL           m_bDirectRead;       // Used during init time.
    DEVICE_POWER_STATE   m_PowerState;  // Current power state of the device.
    PAC97MINIPORTTOPOLOGY m_Topology;    // Miniport Topology pointer.


    /*************************************************************************
     * CAC97AdapterCommon methods
     *************************************************************************
     */
    
    //
    // Resets AC97 audio registers.
    //
    NTSTATUS InitAC97 (void);
    
    //
    // Checks for existance of registers.
    //
    NTSTATUS ProbeHWConfig (void);

    //
    // Checks for 6th bit support in the volume control.
    //
    NTSTATUS Check6thBitSupport (IN AC97Register, IN TopoNodeConfig);

    //
    // Returns true if you should disable the input or output pin.
    //
    BOOL DisableAC97Pin (IN  TopoPinConfig);

#if (DBG)
    //
    // Dumps the probed configuration.
    //
    void DumpConfig (void);
#endif

    //
    // Sets AC97 registers to default.
    //
    NTSTATUS SetAC97Default (void);

    //
    // Aquires the semaphore for AC97 register access.
    //
    NTSTATUS AcquireCodecSemiphore (void);

    //
    // Checks if there is a AC97 link between AC97 and codec.
    //
    NTSTATUS PrimaryCodecReady (void);

    //
    // Powers up the Codec.
    //
    NTSTATUS PowerUpCodec (void);
    
    //
    // Saves native audio bus master control registers values to be used 
    // upon suspend.
    //
    NTSTATUS ReadNABMCtrlRegs (void);

    //
    // Writes back native audio bus master control resgister to be used upon 
    // resume.
    //
    NTSTATUS RestoreNABMCtrlRegs (void);

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CAC97AdapterCommon);
    ~CAC97AdapterCommon();

    /*************************************************************************
     * IAdapterPowerManagement methods
     *************************************************************************
     */
    IMP_IAdapterPowerManagement;

    /*************************************************************************
     * IAC97AdapterCommon methods
     *************************************************************************
     */
    
    //
    // Initialize the adapter common object -> initialize and probe HW.
    //
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN  PRESOURCELIST ResourceList,
        IN  PDEVICE_OBJECT DeviceObject
    );
    
    //
    // Returns if pin exists.
    //
    STDMETHODIMP_(BOOL) GetPinConfig
    (
        IN  TopoPinConfig pin
    )
    {
        return m_stHardwareConfig.Pins[pin].bPinConfig;
    };

    //
    // Sets the pin configuration (exist/not exist).
    //
    STDMETHODIMP_(void) SetPinConfig
    (
        IN  TopoPinConfig pin,
        IN  BOOL config
    )
    {
        m_stHardwareConfig.Pins[pin].bPinConfig = config;
    };

    //
    // Return if node exists.
    //
    STDMETHODIMP_(BOOL) GetNodeConfig
    (
        IN  TopoNodeConfig node
    )
    {
        return m_stHardwareConfig.Nodes[node].bNodeConfig;
    };

    //
    // Sets the node configuration (exist/not exist).
    //
    STDMETHODIMP_(void) SetNodeConfig
    (
        IN  TopoNodeConfig node,
        IN  BOOL config
    )
    {
        m_stHardwareConfig.Nodes[node].bNodeConfig = config;
    };

    //
    // Returns the AC97 register that is assosiated with the node.
    //
    STDMETHODIMP_(AC97Register) GetNodeReg
    (   IN  TopoNodes node
    )
    {
        return stMapNodeToReg[node].reg;
    };

    //
    // Returns the AC97 register mask that is assosiated with the node.
    //
    STDMETHODIMP_(WORD) GetNodeMask
    (
        IN  TopoNodes node
    )
    {
        return stMapNodeToReg[node].mask;
    };

    //
    // Reads a AC97 register.
    //
    STDMETHODIMP_(NTSTATUS) ReadCodecRegister
    (
        _In_range_(0, AC97REG_INVALID)  AC97Register Register,
        _Out_ PWORD wData
    );

    //
    // Writes a AC97 register.
    //
    STDMETHODIMP_(NTSTATUS) WriteCodecRegister
    (
        _In_range_(0, AC97REG_INVALID)  AC97Register Register,
        _In_  WORD wData,
        _In_  WORD wMask
    );

    //
    // Reads a 8 bit AC97 bus master register.
    //
    STDMETHODIMP_(UCHAR) ReadBMControlRegister8
    (
        IN  ULONG ulOffset
    );

    //
    // Reads a 16 bit AC97 bus master register.
    //
    STDMETHODIMP_(USHORT) ReadBMControlRegister16
    (
        IN  ULONG ulOffset
    );

    //
    // Reads a 32 bit AC97 bus master register.
    //
    STDMETHODIMP_(ULONG) ReadBMControlRegister32
    (
        IN  ULONG ulOffset
    );

    //
    // Writes a 8 bit AC97 bus master register.
    //                                        
    STDMETHODIMP_(void) WriteBMControlRegister
    (
        IN  ULONG ulOffset,
        IN  UCHAR Value
    );

    //
    // writes a 16 bit AC97 bus master register.
    //
    STDMETHODIMP_(void) WriteBMControlRegister
    (
        IN  ULONG ulOffset,
        IN  USHORT Value
    );

    // writes a 32 bit AC97 bus master register.
    STDMETHODIMP_(void) WriteBMControlRegister
    (
        IN  ULONG ulOffset,
        IN  ULONG Value
    );

    //
    // Write back cached mixer values to codec registers.
    //
    STDMETHODIMP_(NTSTATUS) RestoreCodecRegisters();

    //
    // Programs a sample rate.
    //
    STDMETHODIMP_(NTSTATUS) ProgramSampleRate
    (
        IN  AC97Register Register,
        IN  DWORD dwSampleRate
    );

    //
    // Stores the topology pointer. Used for DRM only.
    //
    STDMETHODIMP_(void) SetMiniportTopology (PAC97MINIPORTTOPOLOGY topo)
    {
        m_Topology = topo;
    };

    //
    // Returns the topology pointer. Used for DRM only.
    //
    STDMETHODIMP_(PAC97MINIPORTTOPOLOGY) GetMiniportTopology (void)
    {
        return m_Topology;
    };
    
    //
    // This function reads the default channel config and is called only by the
    // wave miniport.
    //
    STDMETHODIMP_(void) ReadChannelConfigDefault
    (
        PDWORD  pdwChannelConfig,
        PWORD   pwChannels
    );
    
    //
    // This function writes the default channel config and is called only by the
    // wave miniport.
    //
    STDMETHODIMP_(void) WriteChannelConfigDefault
    (
        DWORD   dwChannelConfig
    );
    
    /*************************************************************************
     * Friends
     *************************************************************************
     */
    
    friend NTSTATUS NewAdapterCommon
    (
        OUT PADAPTERCOMMON *OutAdapterCommon,
        IN  PRESOURCELIST ResourceList
    );
};

#endif  //_COMMON_H_
