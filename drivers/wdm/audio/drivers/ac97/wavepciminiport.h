/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file wavepciminiport.h was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

#ifndef _MINWAVE_H_
#define _MINWAVE_H_

#include "shared.h"

/*****************************************************************************
 * Constants
 *****************************************************************************
 */
const int WAVE_SAMPLERATES_TESTED = 7;
const int MIC_SAMPLERATES_TESTED = 4;

const DWORD dwWaveSampleRates[WAVE_SAMPLERATES_TESTED] =
    {48000, 44100, 32000, 22050, 16000, 11025, 8000};
const DWORD dwMicSampleRates[MIC_SAMPLERATES_TESTED] =
    {48000, 32000, 16000, 8000};

const int PIN_WAVEOUT_OFFSET = (PIN_WAVEOUT / 2);
const int PIN_WAVEIN_OFFSET  = (PIN_WAVEIN / 2);
const int PIN_MICIN_OFFSET   = (PIN_MICIN / 2);

/*****************************************************************************
 * Forward References
 *****************************************************************************
 */
class CMiniportWaveICHStream;

extern NTSTATUS CreateMiniportWaveICHStream
(
    OUT     CMiniportWaveICHStream **   pWaveIchStream,
    IN      PUNKNOWN                    pUnknown,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    IN      POOL_TYPE                   PoolType
);

/*****************************************************************************
 * Classes
 *****************************************************************************
 */

/*****************************************************************************
 * CMiniportWaveICH
 *****************************************************************************
 * AC97 wave PCI miniport.  This object is associated with the device and is
 * created when the device is started.  The class inherits IMiniportWavePci
 * so it can expose this interface, CUnknown so it automatically gets
 * reference counting and aggregation support, and IPowerNotify for ACPI 
 * power management notification.
 */
class CMiniportWaveICH : public IMiniportWavePci,
                                public IPowerNotify,
                                public CUnknown
{
private:
    // The stream class accesses a lot of private member variables.
    // A better way would be to abstract the access through member
    // functions which on the other hand would produce more overhead
    // both in CPU time and programming time.
    friend class CMiniportWaveICHStream;

    //
    // CMiniportWaveICH private variables
    //
    CMiniportWaveICHStream *Streams[PIN_MICIN_OFFSET + 1];
    PPORTWAVEPCI        Port;           // Port driver object.
    PADAPTERCOMMON      AdapterCommon;  // Adapter common object.
    PDMA_ADAPTER        AdapterObject;
    PINTERRUPTSYNC      InterruptSync;  // Interrupt Sync.
    PDMACHANNEL         DmaChannel;     // Bus master support.
    DEVICE_POWER_STATE  m_PowerState;   // advanced power control.
    DWORD               m_dwChannelMask; // Channel config for speaker positions.
    WORD                m_wChannels;      // Number of channels.

    /*************************************************************************
     * CMiniportWaveICH methods
     *************************************************************************
     * These are private member functions used internally by the object.  See
     * MINWAVE.CPP for specific descriptions.
     */

    //
    // Checks and connects the miniport to the resources.
    //
    NTSTATUS ProcessResources
    (
        IN   PRESOURCELIST     ResourceList
    );

    //
    // Tests the data format but not the sample rate.
    //
    NTSTATUS TestDataFormat
    (
        IN PKSDATAFORMAT Format,
        IN WavePins      Pin
    );

    // Test for standard sample rate support and fill the data range information
    // in the structures below.
    NTSTATUS BuildDataRangeInformation (void);

public:
    /*************************************************************************
     * The following two macros are from STDUNK.H.  DECLARE_STD_UNKNOWN()
     * defines inline IUnknown implementations that use CUnknown's aggregation
     * support.  NonDelegatingQueryInterface() is declared, but it cannot be
     * implemented generically.  Its definition appears in MINWAVE.CPP.
     * DEFINE_STD_CONSTRUCTOR() defines inline a constructor which accepts
     * only the outer unknown, which is used for aggregation.  The standard
     * create macro (in MINWAVE.CPP) uses this constructor.
     */
    DECLARE_STD_UNKNOWN ();
    DEFINE_STD_CONSTRUCTOR (CMiniportWaveICH);
    ~CMiniportWaveICH ();

    //
    // Include IMiniportWavePci (public/exported) methods
    //
    IMP_IMiniportWavePci;

    //
    // IPowerNotify methods
    //
    IMP_IPowerNotify;

    //
    // This static functions is the interrupt service routine which is
    // not stream related, but services all streams at once.
    //
    static NTSTATUS InterruptServiceRoutine
    (
        IN      PINTERRUPTSYNC  InterruptSync,
        IN      PVOID           StaticContext
    );

    //
    // This is the property handler for KSPROPERTY_AUDIO_CHANNEL_CONFIG of the
    // DAC node.
    //
    static NTSTATUS PropertyChannelConfig
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
};

#endif          // _MINWAVE_H_

