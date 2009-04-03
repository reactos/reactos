/*
Copyright (c) 2006-2007 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _MINWAVE_HPP_
#define _MINWAVE_HPP_

#define PC_IMPLEMENTATION // for the implementation of IDMAChannel

#include "common.hpp"
#include "property.h"

class CMiniportWaveStreamCMI;

class CMiniportWaveCMI:
#ifdef WAVERT
                                public IMiniportWaveRT,
#else
                                public IMiniportWaveCyclic,
#endif
                                public IMiniportWaveCMI,
                                public CUnknown
{
private:
    PCMIADAPTER             CMIAdapter;                 // Adapter common object.
#ifdef WAVERT
    PPORTWAVERT             Port;
#else
    PPORTWAVECYCLIC         Port;
    PDMACHANNEL             DMAChannel[3];
    UInt32                  notificationInterval;
#endif
    CMI8738Info             *cm;
	UInt32                  requestedChannelCount;
	UInt32                  requestedChannelMask;


    CMiniportWaveStreamCMI  *stream[3];
    bool                    isStreamRunning[3];
    KMUTEX                  mutex;

    NTSTATUS processResources(PRESOURCELIST resourceList);

	NTSTATUS isFormatAllowed(UInt32 sampleRate, BOOLEAN multiChan, BOOLEAN AC3);
    NTSTATUS validateFormat(PKSDATAFORMAT format, ULONG PinID, BOOLEAN capture);
#ifndef WAVERT
    NTSTATUS newDMAChannel(PDMACHANNEL *dmaChannel, UInt32 bufferLength);
#endif
    NTSTATUS loadChannelConfigFromRegistry();
	NTSTATUS storeChannelConfigToRegistry();
public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCMI);
    ~CMiniportWaveCMI();
#ifdef WAVERT
    IMP_IMiniportWaveRT;
#else
    IMP_IMiniportWaveCyclic;
#endif

    STDMETHODIMP_(void) ServiceWaveISR(UInt32 streamIndex);
    STDMETHODIMP_(void) powerUp();
    STDMETHODIMP_(void) powerDown();

    friend NTSTATUS NTAPI PropertyHandler_ChannelConfig(PPCPROPERTY_REQUEST PropertyRequest);
    friend class CMiniportWaveStreamCMI;
};


class CMiniportWaveStreamCMI:
#ifdef WAVERT
                                      public IMiniportWaveRTStream,
#else
                                      public IMiniportWaveCyclicStream,
#endif
                                      public IDrmAudioStream,
                                      public CUnknown
{
private:
    CMiniportWaveCMI  *Miniport;
#ifdef WAVERT
    PPORTWAVERTSTREAM Port;
	PMDL              audioBufferMDL;
	UInt32            dmaAddress;
	UInt32            dmaMemorySize;
#else
    PDMACHANNEL       DMAChannel;
    PSERVICEGROUP     ServiceGroup;   // For notification.
#endif

    bool              isCaptureStream;// Capture or render.
    UInt32            streamIndex;
    UInt32            channelNumber;  // hardware channel number: 0/A or 1/B
    KSSTATE           state;          // Stop, pause, run.
    UInt32            periodSize;     // process n frames until the interrupt is fired in frames, NOT in bytes
    UInt32            dmaSize;        // size of the DMA buffer in frames, NOT in bytes
    UInt32            currentChannelCount, currentSampleRate, currentResolution;
    bool              enableAC3Passthru, enableSPDIF;

    NTSTATUS prepareStream();
    NTSTATUS setDACChannels();
    NTSTATUS setupSPDIFPlayback(bool enableSPDIF);
    NTSTATUS setupAC3Passthru();

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveStreamCMI);
    ~CMiniportWaveStreamCMI();
#ifdef WAVERT
	IMP_IMiniportWaveRTStream;
#else
    IMP_IMiniportWaveCyclicStream;
#endif

    IMP_IDrmAudioStream;

#ifdef WAVERT
    NTSTATUS Init(CMiniportWaveCMI* Miniport_, UInt32 streamIndex_, bool isCaptureStream_, PKSDATAFORMAT DataFormat, PPORTWAVERTSTREAM PortStream_);
#else
    NTSTATUS Init(CMiniportWaveCMI* Miniport_, UInt32 streamIndex_, bool isCaptureStream_, PKSDATAFORMAT DataFormat, PDMACHANNEL DMAChannel_, PSERVICEGROUP* OutServiceGroup);
#endif
    friend class CMiniportWaveCMI;
};

NTSTATUS CreateMiniportWaveStreamCMI(CMiniportWaveStreamCMI  **MiniportWaveStreamCMI, PUNKNOWN pUnknownOuter, POOL_TYPE PoolType);

#endif //_MINWAVE_HPP_
