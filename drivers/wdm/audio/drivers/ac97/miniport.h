#ifndef _MINIPORT_H_
#define _MINIPORT_H_

#ifndef PPORT_
 #define PPORT_ PPORT
 #define PPORTSTREAM_ PUNKNOWN
#endif

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

class CMiniportStream;
class CMiniport : public IPowerNotify
{
public:
    CMiniportStream     *Streams[PIN_MICIN_OFFSET + 1];
    PPORT_              Port;           // Port driver object.
    PADAPTERCOMMON      AdapterCommon;  // Adapter common object.
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
        IN  PRESOURCELIST     ResourceList
    );

    //
    // Tests the data format but not the sample rate.
    //
    NTSTATUS TestDataFormat
    (
        IN PKSDATAFORMAT Format,
        IN WavePins      Pin
    );

    NTSTATUS ValidateFormat
    (
        IN  PKSDATAFORMAT DataFormat,
        IN  WavePins      Pin
    );

    // Test for standard sample rate support and fill the data range information
    // in the structures below.
    NTSTATUS BuildDataRangeInformation (void);

    IMP_IMiniport;

    //
    // IPowerNotify methods
    //
    IMP_IPowerNotify;

    //
    // This static functions is the interrupt service routine which is
    // not stream related, but services all streams at once.
    //
    static NTSTATUS NTAPI InterruptServiceRoutine
    (
        IN      PINTERRUPTSYNC  InterruptSync,
        IN      PVOID           StaticContext
    );

public:

    ~CMiniport();

    STDMETHODIMP_(NTSTATUS) Init
    (
        _In_  PUNKNOWN      UnknownAdapter,
        _In_  PRESOURCELIST ResourceList,
        _In_  PPORT         Port_
    );

    //
    // This is the property handler for KSPROPERTY_AUDIO_CHANNEL_CONFIG of the
    // DAC node.
    //
    static NTSTATUS NTAPI PropertyChannelConfig
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );


    NTSTATUS NonDelegatingQueryInterface
    (
        _In_         REFIID  Interface,
        _COM_Outptr_ PVOID   *Object,
        _In_         REFIID  iMiniPort,
        _In_         PMINIPORT miniPort
    );
};

#define IMP_CMiniport(cType, IID) \
STDMETHODIMP_(NTSTATUS) cType::GetDescription( \
    _Out_ PPCFILTER_DESCRIPTOR *OutFilterDescriptor)\
{   return CMiniport::GetDescription(OutFilterDescriptor); } \
                                                         \
STDMETHODIMP_(NTSTATUS) cType::DataRangeIntersection(    \
    IN  ULONG PinId,                                     \
    IN  PKSDATARANGE DataRange,                          \
    IN  PKSDATARANGE MatchingDataRange,                  \
    IN  ULONG OutputBufferLength,                        \
    OUT PVOID ResultantFormat OPTIONAL,                  \
    OUT PULONG ResultantFormatLength)                    \
{   return CMiniport::DataRangeIntersection(PinId, DataRange,   \
        MatchingDataRange, OutputBufferLength, ResultantFormat,     \
        ResultantFormatLength); } \
STDMETHODIMP_(NTSTATUS) cType::NonDelegatingQueryInterface(     \
    _In_         REFIID  Interface,                             \
    _COM_Outptr_ PVOID  *Object)                                \
{   return CMiniport::NonDelegatingQueryInterface(              \
        Interface, Object, IID, (PMINIPORT)this); }


void __fastcall obj_AddRef(PUNKNOWN obj, void **ppvObject);
void __fastcall obj_Release(void **ppvObject);

#include "stream.h"
#endif
