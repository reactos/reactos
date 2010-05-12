/*
    ReactOS Operating System
    Sound Blaster KS Driver

    AUTHORS:
        Andrew Greenwood

    NOTES:
        Ah, the Sound Blaster 16. I remember it well...
*/

#include <portcls.h>
#include <stdunk.h>

#ifndef SB16_H
#define SB16_H

#define MAX_DMA_BUFFER_SIZE     65536

enum DspRegister
{
    DspRegCMSD0 = 0x00,
    DspRegCMSR0 = 0x01,
    DspRegCMSD1 = 0x02,
    DspRegCMSR1 = 0x03,
    DspRegMixerReg = 0x04,
    DspRegMixerData = 0x05,
    DspRegReset = 0x06,
    DspRegFMD0 = 0x08,
    DspRegFMR0 = 0x09,
    DspRegRead = 0x0a,
    DspRegWrite = 0x0c,
    DspRegDataAvailable = 0x0e,

    DspRegAck8 = 0x0e,
    DspRegAck16 = 0x0f
};

enum DspCommand
{
    DspWriteWaveProgrammedIo = 0x10,
    DspWriteWave = 0x14,
    DspWriteWaveAuto = 0x1c,
    DspReadWave = 0x24,
    DspReadWaveAuto = 0x2C,
    DspWriteHighSpeedWave = 0x90,
    DspReadHighSpeedWave = 0x98,
    DspSetSampleRate = 0x40,
    DspSetBlockSize = 0x48,
    DspEnableOutput = 0xd1,
    DspDisableOutput = 0xd3,
    DspOutputStatus = 0xd8,
    DspPauseDma = 0xd0,
    DspContinueDma = 0xd4,
    DspHaltAutoDma = 0xda,
    DspInverter = 0xe0,
    DspGetVersion = 0xe1,
    DspGenerateInterrupt = 0xf2,

    /* SB16 only */
    DspSetDacRate = 0x41,
    DspSetAdcRate = 0x42,
    DspStartDac16 = 0xb6,
    DspStartAdc16 = 0xbe,
    DspStartDac8 = 0xc6,
    DspStartAdc8 = 0xc3,
    DspPauseDma16 = 0xd5,
    DspContinueDma16 = 0xd6,
    DspHaltAutoDma16 = 0xd9
};

enum DspMixerRegister
{
    DspMixMasterVolumeLeft = 0x00,
    DspMixMasterVolumeRight = 0x01,
    DspMixVoiceVolumeLeft = 0x02,
    DspMixVoiceVolumeRight = 0x03,
    /* ... */
    DspMixIrqConfig = 0x80,
    DspMixDmaConfig = 0x81
};


#define MPU401_OUTPUT_READY 0x40
#define MPU401_INPUT_READY  0x80

#define MPU401_RESET        0xff
#define MPU401_UART_MODE    0x3f

DEFINE_GUID(IID_IWaveMiniportSB16,
    0xbe23b2d7, 0xa760, 0x43ab, 0xb7, 0x6e, 0xbc, 0x3e, 0x93, 0xe6, 0xff, 0x54);

DECLARE_INTERFACE_(IWaveMiniportSB16, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(void, RestoreSampleRate)( THIS ) PURE;
    STDMETHOD_(void, ServiceWaveISR)( THIS ) PURE;
};

typedef IWaveMiniportSB16 *PWAVEMINIPORTSB16;


DEFINE_GUID(IID_IAdapterSB16,
    0xfba9052c, 0x0544, 0x4bc4, 0x97, 0x3f, 0x70, 0xb7, 0x06, 0x46, 0x81, 0xe5);

DECLARE_INTERFACE_(IAdapterSB16, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, Init)( THIS_
        IN  PRESOURCELIST ResourceList,
        IN  PDEVICE_OBJECT DeviceObject) PURE;

    STDMETHOD_(PINTERRUPTSYNC, GetInterruptSync)( THIS ) PURE;

    STDMETHOD_(void, SetWaveMiniport)( THIS_
        IN  PWAVEMINIPORTSB16 Miniport) PURE;

    STDMETHOD_(BYTE, Read)( THIS ) PURE;

    STDMETHOD_(BOOLEAN, Write)( THIS_
        IN  BYTE Value) PURE;

    STDMETHOD_(NTSTATUS, Reset)( THIS ) PURE;

    STDMETHOD_(void, SetMixerValue)( THIS_
        IN  BYTE Index,
        IN  BYTE Value) PURE;

    STDMETHOD_(BYTE, GetMixerValue)( THIS_
        IN  BYTE Index) PURE;

    STDMETHOD_(void, ResetMixer)( THIS ) PURE;

    /* TODO - Save/load settings */
};

typedef IAdapterSB16 *PADAPTERSB16;

#endif
