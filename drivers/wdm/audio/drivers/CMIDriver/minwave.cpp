/*
Copyright (c) 2006-2008 dogbert <dogber1@gmail.com>
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

#include "minwave.hpp"
#include "minwavetables.hpp"
#include "ntddk.h"

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

NTSTATUS NTAPI CreateMiniportWaveCMI(PUNKNOWN *Unknown, REFCLSID, PUNKNOWN UnknownOuter, POOL_TYPE PoolType)
{
	PAGED_CODE();
	ASSERT(Unknown);
#ifdef WAVERT
	STD_CREATE_BODY_(CMiniportWaveCMI,Unknown,UnknownOuter,PoolType,PMINIPORTWAVERT);
#else
	STD_CREATE_BODY_(CMiniportWaveCMI,Unknown,UnknownOuter,PoolType,PMINIPORTWAVECYCLIC);
#endif
}

NTSTATUS CMiniportWaveCMI::processResources(PRESOURCELIST resourceList)
{
	PAGED_CODE();
	ASSERT (resourceList);
	DBGPRINT(("CMiniportWaveCMI[%p]::ProcessResources(%p)", this, resourceList));

	if (resourceList->NumberOfInterrupts() < 1) {
		DBGPRINT(("Unknown configuration for wave miniport"));
		return STATUS_DEVICE_CONFIGURATION_ERROR;
	}
	return STATUS_SUCCESS;
}

#ifndef WAVERT
NTSTATUS CMiniportWaveCMI::newDMAChannel(PDMACHANNEL *dmaChannel, UInt32 bufferLength)
{
	PAGED_CODE();
	ASSERT(dmaChannel);
	DBGPRINT(("CMiniportWaveCMI[%p]::newDMAChannel(%p)", this, dmaChannel));

	NTSTATUS ntStatus;

	ntStatus = Port->NewMasterDmaChannel(dmaChannel, NULL, NULL, bufferLength, TRUE, FALSE, (DMA_WIDTH)(-1), (DMA_SPEED)(-1));
	if (NT_SUCCESS(ntStatus)) {
		ULONG  lDMABufferLength = bufferLength;
		do {
			ntStatus = (*dmaChannel)->AllocateBuffer(lDMABufferLength, NULL);
			lDMABufferLength >>= 1;
		} while (!NT_SUCCESS(ntStatus) && (lDMABufferLength > (PAGE_SIZE / 2)));
	}
	return ntStatus;
}
#endif

//generic crap
STDMETHODIMP CMiniportWaveCMI::NonDelegatingQueryInterface(REFIID Interface, PVOID *Object)
{
	PAGED_CODE();
	ASSERT(Object);
	DBGPRINT(("CMiniportWaveCMI[%p]::NonDelegatingQueryInterface", this));

	if (IsEqualGUIDAligned(Interface,IID_IUnknown)) {
#ifdef WAVERT
		*Object = PVOID(PUNKNOWN(PMINIPORTWAVERT(this)));
#else
		*Object = PVOID(PUNKNOWN(PMINIPORTWAVECYCLIC(this)));
#endif
	} else if (IsEqualGUIDAligned(Interface,IID_IMiniport)) {
		*Object = PVOID(PMINIPORT(this));
#ifdef WAVERT
	} else if (IsEqualGUIDAligned(Interface,IID_IMiniportWaveRT)) {
		*Object = PVOID(PMINIPORTWAVERT(this));
#else
	} else if (IsEqualGUIDAligned(Interface,IID_IMiniportWaveCyclic)) {
		*Object = PVOID(PMINIPORTWAVECYCLIC(this));
#endif
	} else {
		*Object = NULL;
	}

	if (*Object) {
		// We reference the interface for the caller.
		PUNKNOWN(*Object)->AddRef();
		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_PARAMETER;
}

CMiniportWaveCMI::~CMiniportWaveCMI(void)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveCMI[%p]::~CMiniportWaveCMI", this));

	storeChannelConfigToRegistry(); //or not. during system shutdown, this doesn't seem to work.

	if (CMIAdapter) {
		CMIAdapter->Release();
		CMIAdapter = NULL;
	}

	for (int i=0;i<3;i++) {
#ifndef WAVERT
		if (DMAChannel[i]) {
			DMAChannel[i]->Release();
			DMAChannel[i] = NULL;
		}
#endif
	if (isStreamRunning[i]) {
			isStreamRunning[i] = false;
			stream[i]->Release();
			stream[i] = NULL;
		}
	}

	if (Port) {
		Port->Release();
		Port = NULL;
	}
}

#ifdef WAVERT
STDMETHODIMP CMiniportWaveCMI::Init(PUNKNOWN UnknownAdapter, PRESOURCELIST ResourceList, PPORTWAVERT Port_)
#else
STDMETHODIMP CMiniportWaveCMI::Init(PUNKNOWN UnknownAdapter, PRESOURCELIST ResourceList, PPORTWAVECYCLIC Port_)
#endif
{
	PAGED_CODE();

	ASSERT(UnknownAdapter);
	ASSERT(ResourceList);
	ASSERT(Port_);

	DBGPRINT(("CMiniportWaveCMI[%p]::Init(%p, %p, %p)", this, UnknownAdapter, ResourceList, Port_));

	Port = Port_;
	Port->AddRef();

	NTSTATUS ntStatus = UnknownAdapter->QueryInterface(IID_ICMIAdapter, (PVOID *) &CMIAdapter);
	if (!NT_SUCCESS(ntStatus)) {
	    DBGPRINT(("QueryInterface(CMIAdapter) failed"));
		return ntStatus;
	}

	//check for Vista, set the AC3 stuff accordingly
	if (IoIsWdmVersionAvailable(0x06,0x00)) {
		WavePinDataRangesAC3Stream[1].MinimumSampleFrequency = MIN_SAMPLE_RATE;
		WavePinDataRangesAC3Stream[1].MaximumSampleFrequency = MAX_SAMPLE_RATE;
		WavePinDataRangesAC3Stream[1].DataRange.SubFormat    = KSDATAFORMAT_SUBTYPE_PCM;
		WavePinDataRangesAC3Stream[1].DataRange.Specifier    = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
	}

	cm = CMIAdapter->getCMI8738Info();
	cm->regFUNCTRL0  = 0;
	cm->WaveMiniport = this;

	loadChannelConfigFromRegistry();

	for (int i=0;i<3;i++)
	{
		isStreamRunning[i] = false;

#ifndef WAVERT
		ntStatus = newDMAChannel(&DMAChannel[i], MAXLEN_DMA_BUFFER);
		if (!NT_SUCCESS(ntStatus)) {
			DBGPRINT(("NewDmaChannel() failed"));
			return ntStatus;
		}
#endif
	}

	KeInitializeMutex(&mutex, 1);

	return processResources(ResourceList);
}

#ifdef WAVERT
STDMETHODIMP_(NTSTATUS) CMiniportWaveCMI::GetDeviceDescription(PDEVICE_DESCRIPTION OutDeviceDescriptor)
{
	PAGED_CODE();
	ASSERT(OutDeviceDescriptor);
	DBGPRINT(("CMiniportWaveCMI[%p]::GetDeviceDescription(%p)", this, OutDeviceDescriptor));

	RtlZeroMemory(OutDeviceDescriptor, sizeof(DEVICE_DESCRIPTION));
	OutDeviceDescriptor->ScatterGather     = false;
	OutDeviceDescriptor->Master            = true;
	OutDeviceDescriptor->Dma32BitAddresses = true;
	OutDeviceDescriptor->InterfaceType     = PCIBus;
	OutDeviceDescriptor->MaximumLength     = MAXLEN_DMA_BUFFER-2;

	return STATUS_SUCCESS;
}
#endif

STDMETHODIMP CMiniportWaveCMI::GetDescription(PPCFILTER_DESCRIPTOR *OutFilterDescriptor)
{
	PAGED_CODE();
	ASSERT(OutFilterDescriptor);
	DBGPRINT(("CMiniportWaveCMI[%p]::GetDescription(%p)", this, OutFilterDescriptor));

	*OutFilterDescriptor = &WaveMiniportFilterDescriptor;

	return STATUS_SUCCESS;
}

NTSTATUS CMiniportWaveCMI::loadChannelConfigFromRegistry()
{
	PAGED_CODE();
	PREGISTRYKEY       DriverKey;
	PREGISTRYKEY       SettingsKey;
	UNICODE_STRING     KeyName;
	DWORD              ResultLength;
	PVOID              KeyInfo;

	DBGPRINT(("CMiniportWaveCMI::loadChannelConfigFromRegistry()"));

	if ((!CMIAdapter) || (!(CMIAdapter->getDeviceObject()))) {
		return STATUS_UNSUCCESSFUL;
	}

	NTSTATUS ntStatus = PcNewRegistryKey(&DriverKey, NULL, DriverRegistryKey, KEY_ALL_ACCESS, CMIAdapter->getDeviceObject(), NULL, NULL, 0, NULL);

	if(!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("PcNewRegistryKey() failed"));
		return STATUS_UNSUCCESSFUL;
	}

	RtlInitUnicodeString(&KeyName, L"Settings");

	ntStatus = DriverKey->NewSubKey(&SettingsKey, NULL, KEY_ALL_ACCESS, &KeyName, REG_OPTION_NON_VOLATILE, NULL);
	if(!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("DriverKey->NewSubKey() failed"));
		return STATUS_UNSUCCESSFUL;
	}

	KeyInfo = ExAllocatePoolWithTag(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD), 'gnaa');
	if (KeyInfo) {
		RtlInitUnicodeString(&KeyName, L"ChannelCount");
		ntStatus = SettingsKey->QueryValueKey(&KeyName, KeyValuePartialInformation, KeyInfo, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD), &ResultLength);
		if (NT_SUCCESS (ntStatus)) {
			PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;
			if (PartialInfo->DataLength == sizeof(DWORD)) {
				requestedChannelCount = (*(PLONG)PartialInfo->Data);
			}
		} else {
			requestedChannelCount = 2;
		}

		RtlInitUnicodeString(&KeyName, L"ChannelMask");
		ntStatus = SettingsKey->QueryValueKey(&KeyName, KeyValuePartialInformation, KeyInfo, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD), &ResultLength);
		if (NT_SUCCESS (ntStatus)) {
			PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;
			if (PartialInfo->DataLength == sizeof(DWORD)) {
				requestedChannelMask = (*(PLONG)PartialInfo->Data);
			}
		} else {
			requestedChannelMask = KSAUDIO_SPEAKER_STEREO;
		}
	}
	ExFreePoolWithTag(KeyInfo,'gnaa');

	SettingsKey->Release();
	DriverKey->Release();

	return STATUS_SUCCESS;
}

NTSTATUS CMiniportWaveCMI::storeChannelConfigToRegistry()
{
	PAGED_CODE();
	PREGISTRYKEY       DriverKey;
	PREGISTRYKEY       SettingsKey;
	UNICODE_STRING     KeyName;
	DWORD              Value;
	DBGPRINT(("CMiniportWaveCMI::storeChannelConfigToRegistry()"));

	if ((!CMIAdapter) || (!(CMIAdapter->getDeviceObject()))) {
		return STATUS_UNSUCCESSFUL;
	}

	NTSTATUS ntStatus = PcNewRegistryKey(&DriverKey, NULL, DriverRegistryKey, KEY_ALL_ACCESS, CMIAdapter->getDeviceObject(), NULL, NULL, 0, NULL);

	if(!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("PcNewRegistryKey() failed"));
		return STATUS_UNSUCCESSFUL;
	}

	RtlInitUnicodeString(&KeyName, L"Settings");

	ntStatus = DriverKey->NewSubKey(&SettingsKey, NULL, KEY_ALL_ACCESS, &KeyName, REG_OPTION_NON_VOLATILE, NULL);
	if(!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("DriverKey->NewSubKey() failed"));
		return STATUS_UNSUCCESSFUL;
	}

	Value = requestedChannelCount;
	RtlInitUnicodeString(&KeyName, L"ChannelCount");
	ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("SetValueKey() failed"));
	}
	Value = requestedChannelMask;
	RtlInitUnicodeString(&KeyName, L"ChannelMask");
	ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("SetValueKey() failed"));
	}

	SettingsKey->Release();
	DriverKey->Release();

	return STATUS_SUCCESS;
}


STDMETHODIMP_(void) CMiniportWaveCMI::powerUp(void)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveCMI[%p]::powerUp()", this));
	KSSTATE oldState[3];

	for (int i=0;i<3;i++) {
		if (isStreamRunning[i]) {
			oldState[i] = stream[i]->state;
			stream[i]->SetState(KSSTATE_STOP);
		}
	}

	if (cm->TopoMiniport) {
		cm->TopoMiniport->loadMixerSettingsFromMemory();
	}

	for (int i=0;i<3;i++) {
		if (isStreamRunning[i]) {
			stream[i]->prepareStream();
			stream[i]->SetState(KSSTATE_ACQUIRE);
			stream[i]->SetState(oldState[i]);
		}
	}
}

STDMETHODIMP_(void) CMiniportWaveCMI::powerDown(void)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveCMI[%p]::powerDown()", this));

	if (cm->TopoMiniport) {
		cm->TopoMiniport->storeMixerSettingsToMemory();
	}

}


NTSTATUS CMiniportWaveCMI::isFormatAllowed(UInt32 sampleRate, BOOLEAN multiChan, BOOLEAN AC3)
{
	PAGED_CODE();
	ASSERT(sampleRate);
	DBGPRINT(("CMiniportWaveCMI[%p]::isFormatAllowed(%d, %d, %d)", this, sampleRate, multiChan, AC3));

	if (multiChan) {
		switch (sampleRate) {
			case 44100: if (cm->formatMask & FMT_441_MULTI_PCM) return STATUS_SUCCESS; break;
			case 48000: if (cm->formatMask & FMT_480_MULTI_PCM) return STATUS_SUCCESS; break;
			case 88200: if (cm->formatMask & FMT_882_MULTI_PCM) return STATUS_SUCCESS; break;
			case 96000: if (cm->formatMask & FMT_960_MULTI_PCM) return STATUS_SUCCESS; break;
		}
		return STATUS_INVALID_PARAMETER;
	}
	if (AC3) {
		switch (sampleRate) {
			case 44100: if (cm->formatMask & FMT_441_DOLBY) return STATUS_SUCCESS; break;
			case 48000: if (cm->formatMask & FMT_480_DOLBY) return STATUS_SUCCESS; break;
			case 88200: if (cm->formatMask & FMT_882_DOLBY) return STATUS_SUCCESS; break;
			case 96000: if (cm->formatMask & FMT_960_DOLBY) return STATUS_SUCCESS; break;
		}
		return STATUS_INVALID_PARAMETER;
	}
	switch (sampleRate) {
		case 44100: if (cm->formatMask & FMT_441_PCM) return STATUS_SUCCESS; break;
		case 48000: if (cm->formatMask & FMT_480_PCM) return STATUS_SUCCESS; break;
		case 88200: if (cm->formatMask & FMT_882_PCM) return STATUS_SUCCESS; break;
		case 96000: if (cm->formatMask & FMT_960_PCM) return STATUS_SUCCESS; break;
	}
	return STATUS_INVALID_PARAMETER;
}

NTSTATUS CMiniportWaveCMI::validateFormat(PKSDATAFORMAT format, ULONG PinID, BOOLEAN capture)
{
	PAGED_CODE();
	ASSERT(format);
	DBGPRINT(("CMiniportWaveCMI[%p]::validateFormat(%p, %d, %d)", this, format, PinID, capture));

	PWAVEFORMATEX waveFormat = PWAVEFORMATEX(format + 1);
	DBGPRINT(("---channels: %d, resolution: %d, sample rate: %d, pin: %d, formatMask: %x", waveFormat->nChannels, waveFormat->wBitsPerSample, waveFormat->nSamplesPerSec, PinID, cm->formatMask));

//WaveFormatEx
	if  ( ( format->FormatSize >= sizeof(KSDATAFORMAT_WAVEFORMATEX))
	  && IsEqualGUIDAligned(format->MajorFormat,KSDATAFORMAT_TYPE_AUDIO)
	  && IsEqualGUIDAligned(format->Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ) {
		switch (EXTRACT_WAVEFORMATEX_ID(&format->SubFormat)) {
			case WAVE_FORMAT_PCM:
				if ((PinID != PIN_WAVE_RENDER_SINK) && (PinID != PIN_WAVE_CAPTURE_SOURCE) && (PinID != ((ULONG)-1))) {
					if ((PinID == PIN_WAVE_AC3_RENDER_SINK) && !IoIsWdmVersionAvailable(6,0)) {
						return STATUS_INVALID_PARAMETER;
					}
				}

				if ( ((waveFormat->wBitsPerSample == 16) || (waveFormat->wBitsPerSample == 32))
				  && ((waveFormat->nSamplesPerSec == 44100) || (waveFormat->nSamplesPerSec == 48000) || (waveFormat->nSamplesPerSec == 88200) ||  (waveFormat->nSamplesPerSec == 96000))
				  && (waveFormat->nChannels == 2) ) {
					if ((capture) && (waveFormat->nSamplesPerSec > 48000) ) {
						return STATUS_INVALID_PARAMETER;
					}
					return isFormatAllowed(waveFormat->nSamplesPerSec, FALSE, FALSE);
				}
				if ( (waveFormat->wBitsPerSample == 16)
				  && ((waveFormat->nChannels >= 4) && (waveFormat->nChannels <= cm->maxChannels))
				  && ((waveFormat->nSamplesPerSec == 44100) || (waveFormat->nSamplesPerSec == 48000)) ) {
#if OUT_CHANNEL == 1
					if ((PinID == PIN_WAVE_RENDER_SINK) || (PinID == ((ULONG)-1))) {
						return isFormatAllowed(waveFormat->nSamplesPerSec, TRUE, FALSE);
					}
#else
					return STATUS_INVALID_PARAMETER;
#endif
				}
				break;
			case WAVE_FORMAT_DOLBY_AC3_SPDIF:
				if ((PinID != PIN_WAVE_AC3_RENDER_SINK) && (PinID != ((ULONG)-1))) {
					return STATUS_INVALID_PARAMETER;
				}
				if ( ((waveFormat->wBitsPerSample >= MIN_BITS_PER_SAMPLE_AC3) && (waveFormat->wBitsPerSample <= MAX_BITS_PER_SAMPLE_AC3))
				  && ((waveFormat->nSamplesPerSec >= MIN_SAMPLE_RATE_AC3) && (waveFormat->nSamplesPerSec <= MAX_SAMPLE_RATE_AC3))
				  && (waveFormat->nChannels == MAX_CHANNELS_AC3) ) {
					return isFormatAllowed(waveFormat->nSamplesPerSec, FALSE, TRUE);
				}
				break;
			case WAVE_FORMAT_WMA_SPDIF:
				if ((PinID != PIN_WAVE_AC3_RENDER_SINK) && (PinID != ((ULONG)-1))) {
					return STATUS_INVALID_PARAMETER;
				}
				if ( ((waveFormat->wBitsPerSample >= MIN_BITS_PER_SAMPLE_WMA) && (waveFormat->wBitsPerSample <= MAX_BITS_PER_SAMPLE_WMA))
				  && ((waveFormat->nSamplesPerSec >= MIN_SAMPLE_RATE_WMA) && (waveFormat->nSamplesPerSec <= MAX_SAMPLE_RATE_WMA))
				  && (waveFormat->nChannels == MAX_CHANNELS_WMA) ) {
					return isFormatAllowed(waveFormat->nSamplesPerSec, FALSE, TRUE);
				}
				break;
		}
	}

	return STATUS_INVALID_PARAMETER;
}

// Tests a data range intersection
STDMETHODIMP CMiniportWaveCMI::DataRangeIntersection(ULONG PinId, PKSDATARANGE ClientDataRange, PKSDATARANGE MyDataRange, ULONG OutputBufferLength, PVOID ResultantFormat, PULONG ResultantFormatLength)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveCMI[%p]::DataRangeIntersection(%d, %p, %p, %d, %p, %p)", this, PinId, ClientDataRange, MyDataRange, OutputBufferLength, ResultantFormat, ResultantFormatLength));

	if (PinId == PIN_WAVE_AC3_RENDER_SINK) {
		bool isAC3Pin = true;
		// Under Windows 2000 and XP, the client's DataRange should be AC3 only.
		// The AC3 pin is the SPDIF pin in Windows Vista, so 2ch stereo is going to be allowed.

		if (!IsEqualGUIDAligned(ClientDataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO)
		  && !IsEqualGUIDAligned(ClientDataRange->MajorFormat, KSDATAFORMAT_TYPE_WILDCARD)) {
			return STATUS_NO_MATCH;
		}


		if (!IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF)
		  && !IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_WMA_SPDIF)
		  && !IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_WILDCARD)) {
			// check for Vista
			isAC3Pin = false;
			if (IoIsWdmVersionAvailable(0x06,0x00)) {
				if (!IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM)
				  && !IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_WILDCARD)) {
					return STATUS_NO_MATCH;
				}
			} else {
				return STATUS_NO_MATCH;
			}
		}

		if (IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
		 || IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WILDCARD)) {
			*ResultantFormatLength = sizeof(KSDATAFORMAT_WAVEFORMATEX);
		} else
		if (IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND)) {
			*ResultantFormatLength = sizeof(KSDATAFORMAT_DSOUND);
		} else {
			return STATUS_NO_MATCH;
		}

		// Validate return buffer size, if the request is only for the
		// size of the resultant structure, return it now.
		if (!OutputBufferLength) {
			*ResultantFormatLength = sizeof(KSDATAFORMAT_WAVEFORMATEX);
			return STATUS_BUFFER_OVERFLOW;
		} else
		if (OutputBufferLength < sizeof(KSDATAFORMAT_WAVEFORMATEX)) {
			return STATUS_BUFFER_TOO_SMALL;
		}

		PKSDATAFORMAT_WAVEFORMATEX  resultantFormatWFX = (PKSDATAFORMAT_WAVEFORMATEX) ResultantFormat;
		PWAVEFORMATEX               pWaveFormatEx;

		// Return the best (only) available format.
		resultantFormatWFX->DataFormat.FormatSize = *ResultantFormatLength;
		resultantFormatWFX->DataFormat.Flags      = 0;
		resultantFormatWFX->DataFormat.SampleSize = 4; // must match nBlockAlign
		resultantFormatWFX->DataFormat.Reserved	  = 0;

		resultantFormatWFX->DataFormat.MajorFormat  = KSDATAFORMAT_TYPE_AUDIO;
		INIT_WAVEFORMATEX_GUID(&resultantFormatWFX->DataFormat.SubFormat, WAVE_FORMAT_DOLBY_AC3_SPDIF);

		// Extra space for the DSound specifier
		if (IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND)) {

			PKSDATAFORMAT_DSOUND  resultantFormatDSound = (PKSDATAFORMAT_DSOUND)ResultantFormat;

			resultantFormatDSound->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_DSOUND;

			// DSound format capabilities are not expressed
			// this way in KS, so we express no capabilities.
			resultantFormatDSound->BufferDesc.Flags   = 0 ;
			resultantFormatDSound->BufferDesc.Control = 0 ;

			pWaveFormatEx = &resultantFormatDSound->BufferDesc.WaveFormatEx;
		} else {
		// WAVEFORMATEX or WILDCARD (WAVEFORMATEX)
			resultantFormatWFX->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

			pWaveFormatEx = (PWAVEFORMATEX)((PKSDATAFORMAT)resultantFormatWFX + 1);
		}

		pWaveFormatEx->nChannels	   = 2;
		pWaveFormatEx->wBitsPerSample  = 16; // SPDIF
		pWaveFormatEx->cbSize          = 0;
		if (isAC3Pin) {
			if (IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_WMA_SPDIF)) {
				pWaveFormatEx->wFormatTag      = WAVE_FORMAT_WMA_SPDIF;
				pWaveFormatEx->nSamplesPerSec  = min( ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumSampleFrequency, MAX_SAMPLE_RATE_WMA);
			} else {
				pWaveFormatEx->wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
				pWaveFormatEx->nSamplesPerSec  = 48000;
			}
		} else {
			pWaveFormatEx->wFormatTag      = WAVE_FORMAT_PCM;
			pWaveFormatEx->nSamplesPerSec  = min( ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumSampleFrequency, MAX_SAMPLE_RATE);
		}
		pWaveFormatEx->nBlockAlign     = pWaveFormatEx->nChannels * pWaveFormatEx->wBitsPerSample / 8;
		pWaveFormatEx->nAvgBytesPerSec = pWaveFormatEx->nSamplesPerSec * pWaveFormatEx->nBlockAlign;

		return STATUS_SUCCESS;
	}
	if ((PinId == PIN_WAVE_RENDER_SINK) || (PinId == PIN_WAVE_CAPTURE_SINK)) {

		if (!IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) &&
		    !IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_WILDCARD)) {
			return STATUS_NO_MATCH;
		}

		if (!IsEqualGUIDAligned(ClientDataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO) &&
		    !IsEqualGUIDAligned(ClientDataRange->MajorFormat, KSDATAFORMAT_TYPE_WILDCARD)) {
			return STATUS_NO_MATCH;
		}

		if (IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
		  IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WILDCARD)) {
			*ResultantFormatLength = sizeof(KSDATAFORMAT_WAVEFORMATEX);
		} else
		if (IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND)) {
			*ResultantFormatLength = sizeof(KSDATAFORMAT_DSOUND);
		} else {
			return STATUS_NO_MATCH;
		}


		ULONG sampleRate   = 0;
		ULONG nMaxChannels = min(((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumChannels, requestedChannelCount);

		// check for Vista
		if (IoIsWdmVersionAvailable(6,0) && (PinId == PIN_WAVE_RENDER_SINK)) {
			nMaxChannels = ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumChannels;
		}
		if (nMaxChannels & 0x01) {
			nMaxChannels--;
		}
		if (!nMaxChannels) {
			return STATUS_NO_MATCH;
		}

		if (isStreamRunning[PCM_OUT_STREAM]) {
			sampleRate = stream[PCM_OUT_STREAM]->currentSampleRate;
		} else
		if (isStreamRunning[PCM_IN_STREAM]) {
			sampleRate = stream[PCM_IN_STREAM]->currentSampleRate;
		}
		if (sampleRate == 0) {
			if ((nMaxChannels > 2) && (((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumSampleFrequency > MAX_SAMPLE_RATE_MULTI)) {
				sampleRate = MAX_SAMPLE_RATE_MULTI;
			} else
			if ((nMaxChannels == 2) && (((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumSampleFrequency > MAX_SAMPLE_RATE)) {
				sampleRate = MAX_SAMPLE_RATE;
			} else {
				sampleRate = ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumSampleFrequency;
			}
		}

		if ((((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumSampleFrequency < sampleRate)
		  || (((PKSDATARANGE_AUDIO)ClientDataRange)->MinimumSampleFrequency > sampleRate)) {
			return STATUS_NO_MATCH;
		}

		if (PinId == PIN_WAVE_RENDER_SINK) {
			if (!OutputBufferLength) {
				*ResultantFormatLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
				return STATUS_BUFFER_OVERFLOW;
			} else
			if (OutputBufferLength < sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX)) {
				return STATUS_BUFFER_TOO_SMALL;
			}

			if (((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumChannels < 2) {
				DBGPRINT(("[[DataRangeIntersection] mono format not supported"));
				return STATUS_NO_MATCH;
			}

			PWAVEFORMATPCMEX WaveFormat = (PWAVEFORMATPCMEX)((PKSDATAFORMAT)ResultantFormat + 1);
			if (IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND)) {
				return STATUS_NOT_SUPPORTED;
			}
			*(PKSDATAFORMAT)ResultantFormat = *MyDataRange;
			((PKSDATAFORMAT)ResultantFormat)->FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);


			WaveFormat->Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
			WaveFormat->SubFormat              = KSDATAFORMAT_SUBTYPE_PCM;
			WaveFormat->Format.nChannels       = (WORD)nMaxChannels;
			WaveFormat->Format.wBitsPerSample  = 16;
			WaveFormat->Format.nBlockAlign     = (WaveFormat->Format.wBitsPerSample >> 3) * WaveFormat->Format.nChannels;
			WaveFormat->Format.nAvgBytesPerSec = WaveFormat->Format.nSamplesPerSec * WaveFormat->Format.nBlockAlign;
			WaveFormat->Format.cbSize          = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
			WaveFormat->Format.nSamplesPerSec  = sampleRate;
			WaveFormat->Samples.wValidBitsPerSample = WaveFormat->Format.wBitsPerSample;
			switch (nMaxChannels) {
				case 8: WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_7POINT1; break;
				case 6: WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_5POINT1; break;
				case 4: WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_QUAD;    break;
				case 2: WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_STEREO;  break;
			}
			if (nMaxChannels == requestedChannelCount) {
				WaveFormat->dwChannelMask = requestedChannelMask;
			}
			((PKSDATAFORMAT)ResultantFormat)->SampleSize = WaveFormat->Format.nBlockAlign;

			*ResultantFormatLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
			DBGPRINT(("[DataRangeIntersection] MultiChannel Renderer: SampleRate: %d, ClientDataRange->MaxChans: %d, Channels: %d, BitPerSample: %d, BlockAlign: %d, AvgBytesPerSec: %d, ChannelMask: %08X", WaveFormat->Format.nSamplesPerSec, ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumChannels, WaveFormat->Format.nChannels, WaveFormat->Format.wBitsPerSample, WaveFormat->Format.nBlockAlign, WaveFormat->Format.nAvgBytesPerSec, WaveFormat->dwChannelMask));
		} else
		if (PinId == PIN_WAVE_CAPTURE_SINK) {
			PKSDATAFORMAT_WAVEFORMATEX  resultantFormatWFX;
			PWAVEFORMATEX               pWaveFormatEx;

			if (!OutputBufferLength) {
				*ResultantFormatLength = sizeof(KSDATAFORMAT_WAVEFORMATEX);
				return STATUS_BUFFER_OVERFLOW;
			} else
			if (OutputBufferLength < sizeof(KSDATAFORMAT_WAVEFORMATEX)) {
				return STATUS_BUFFER_TOO_SMALL;
			}

			if (nMaxChannels > 2) {
				nMaxChannels = 2;
			}

			resultantFormatWFX = (PKSDATAFORMAT_WAVEFORMATEX) ResultantFormat;
			resultantFormatWFX->DataFormat.FormatSize   = *ResultantFormatLength;
			resultantFormatWFX->DataFormat.Flags        = 0;
			resultantFormatWFX->DataFormat.SampleSize   = 4;
			resultantFormatWFX->DataFormat.Reserved     = 0;
			resultantFormatWFX->DataFormat.MajorFormat  = KSDATAFORMAT_TYPE_AUDIO;
			INIT_WAVEFORMATEX_GUID(&resultantFormatWFX->DataFormat.SubFormat, WAVE_FORMAT_PCM);

			if (IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND)) {
				PKSDATAFORMAT_DSOUND resultantFormatDSound;
				resultantFormatDSound = (PKSDATAFORMAT_DSOUND)ResultantFormat;
				resultantFormatDSound->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_DSOUND;
				resultantFormatDSound->BufferDesc.Flags   = 0 ;
				resultantFormatDSound->BufferDesc.Control = 0 ;
				pWaveFormatEx = &resultantFormatDSound->BufferDesc.WaveFormatEx;
			} else {
				resultantFormatWFX->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
				pWaveFormatEx = (PWAVEFORMATEX)((PKSDATAFORMAT)resultantFormatWFX + 1);
			}
			pWaveFormatEx->wFormatTag      = WAVE_FORMAT_PCM;
			pWaveFormatEx->nChannels       = nMaxChannels;
			pWaveFormatEx->nSamplesPerSec  = sampleRate;
			pWaveFormatEx->wBitsPerSample  = 16;
			pWaveFormatEx->cbSize          = 0;
			pWaveFormatEx->nBlockAlign     = 4;
			pWaveFormatEx->nAvgBytesPerSec = 192000;
		}
		return STATUS_SUCCESS;
	}
	return STATUS_NO_MATCH;
}

//from IMiniportWaveCyclic::NewStream()
#ifdef WAVERT
STDMETHODIMP CMiniportWaveCMI::NewStream(PMINIPORTWAVERTSTREAM *OutStream, PPORTWAVERTSTREAM OuterUnknown, ULONG PinID, BOOLEAN Capture, PKSDATAFORMAT DataFormat)
#else
STDMETHODIMP CMiniportWaveCMI::NewStream(PMINIPORTWAVECYCLICSTREAM *OutStream, PUNKNOWN OuterUnknown, POOL_TYPE PoolType, ULONG PinID, BOOLEAN Capture, PKSDATAFORMAT DataFormat, PDMACHANNEL* OutDmaChannel, PSERVICEGROUP* OutServiceGroup)
#endif
{
	PAGED_CODE();
	ASSERT(OutStream);
	ASSERT(DataFormat);
#ifdef WAVERT
	DBGPRINT(("CMiniportWaveCMI[%p]::NewStream(%p, %p, %d, %d, %p)", this, OutStream, OuterUnknown, PinID, Capture, DataFormat));
#else
	ASSERT(OutDmaChannel);
	ASSERT(OutServiceGroup);
	DBGPRINT(("CMiniportWaveCMI[%p]::NewStream(%p, %p, %p, %d, %d, %p, %p, %p)", this, OutStream, OuterUnknown, PoolType, PinID, Capture, DataFormat, OutDmaChannel, OutServiceGroup));
#endif

	NTSTATUS      ntStatus    = STATUS_SUCCESS;
	PWAVEFORMATEX waveFormat  = PWAVEFORMATEX(DataFormat + 1);
	UInt32        streamIndex = PCM_OUT_STREAM;

	ntStatus = validateFormat(DataFormat, PinID, Capture);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("invalid stream format"));
		return STATUS_UNSUCCESSFUL;
	}
	if (cm->enableSPDIFInMonitor) {
		CMIAdapter->setUInt8Bit(REG_MIXER1, EN_SPDI2DAC);
	}

	if (Capture) {
		streamIndex = PCM_IN_STREAM;
	} else
	if ( (WAVE_FORMAT_DOLBY_AC3_SPDIF == EXTRACT_WAVEFORMATEX_ID(&DataFormat->SubFormat)) || (WAVE_FORMAT_WMA_SPDIF == EXTRACT_WAVEFORMATEX_ID(&DataFormat->SubFormat))) {
		streamIndex = AC3_OUT_STREAM;
	}

	// make sure the hardware is not already in use
	if (isStreamRunning[streamIndex]) {
		DBGPRINT(("Stream %d running, exiting...", streamIndex));
   		return STATUS_UNSUCCESSFUL;
	}
	if ((streamIndex == AC3_OUT_STREAM) && isStreamRunning[PCM_OUT_STREAM]) {
#ifdef WAVERT
   		stream[PCM_OUT_STREAM]->SetState(KSSTATE_STOP);
#else
   		stream[PCM_OUT_STREAM]->SetState(KSSTATE_STOP_AC3);
#endif
	}
	if ((streamIndex == PCM_OUT_STREAM) && isStreamRunning[AC3_OUT_STREAM]) {
   		return STATUS_UNSUCCESSFUL;
	}

	DBGPRINT(("---StreamNo: %d, Bits: %d, Sample Rate: %d, Channels: %d, AC3: %d", streamIndex,
		waveFormat->wBitsPerSample, waveFormat->nSamplesPerSec, waveFormat->nChannels,
		(WAVE_FORMAT_DOLBY_AC3_SPDIF == EXTRACT_WAVEFORMATEX_ID(&DataFormat->SubFormat)) || (WAVE_FORMAT_WMA_SPDIF == EXTRACT_WAVEFORMATEX_ID(&DataFormat->SubFormat)) ));

	// the DAC and ADC can only run at the same sample rate simultaneously
	if ((streamIndex == PCM_IN_STREAM) && isStreamRunning[PCM_OUT_STREAM]) {
   		if (waveFormat->nSamplesPerSec != stream[PCM_OUT_STREAM]->currentSampleRate) {
   			return STATUS_UNSUCCESSFUL;
		}
	}
	if ((streamIndex == PCM_OUT_STREAM) && isStreamRunning[PCM_IN_STREAM]) {
   		if (waveFormat->nSamplesPerSec != stream[PCM_IN_STREAM]->currentSampleRate) {
   			return STATUS_UNSUCCESSFUL;
		}
	}

	// instantiate a stream
#ifdef WAVERT
	ntStatus = CreateMiniportWaveStreamCMI(&stream[streamIndex], OuterUnknown, NonPagedPool);
#else
	ntStatus = CreateMiniportWaveStreamCMI(&stream[streamIndex], OuterUnknown, PoolType);
#endif
	if (!NT_SUCCESS (ntStatus)) {
		DBGPRINT(("Failed to create stream"));
		return ntStatus;
	}

	// initialize it
#ifdef WAVERT
	ntStatus = stream[streamIndex]->Init(this, streamIndex, Capture, DataFormat, OuterUnknown);
#else
	ntStatus = stream[streamIndex]->Init(this, streamIndex, Capture, DataFormat, DMAChannel[streamIndex], OutServiceGroup);
#endif
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Failed to init stream"));
		stream[streamIndex]->Release();
		stream[streamIndex] = NULL;
		*OutStream          = NULL;
#ifndef WAVERT
		*OutServiceGroup    = NULL;
		*OutDmaChannel      = NULL;
#endif
		return ntStatus;
	}

#ifdef WAVERT
//this has been referenced in CreateMiniportWaveStreamCMI() already
	*OutStream = (PMINIPORTWAVERTSTREAM)stream[streamIndex];
#else
	*OutDmaChannel = DMAChannel[streamIndex];
	DMAChannel[streamIndex]->AddRef();
	*OutStream = (PMINIPORTWAVECYCLICSTREAM)stream[streamIndex];
#endif

	return ntStatus;
}

NTSTATUS NTAPI PropertyHandler_ChannelConfig(PPCPROPERTY_REQUEST PropertyRequest)
{
	PAGED_CODE();
	ASSERT(PropertyRequest);
	DBGPRINT(("[PropertyHandler_ChannelConfig]"));

#ifdef WAVERT
	CMiniportWaveCMI *that = (CMiniportWaveCMI *) ((PMINIPORTWAVERT)PropertyRequest->MajorTarget);
#else
	CMiniportWaveCMI *that = (CMiniportWaveCMI *) ((PMINIPORTWAVECYCLIC)PropertyRequest->MajorTarget);
#endif

	if (PropertyRequest->Node == KSNODE_WAVE_DAC) {

		if (PropertyRequest->ValueSize == 0) {
			PropertyRequest->ValueSize = sizeof(LONG);
			return STATUS_BUFFER_OVERFLOW;
		} else if (PropertyRequest->ValueSize < sizeof (LONG)) {
			PropertyRequest->ValueSize = 0;
			return STATUS_BUFFER_TOO_SMALL;
		}

		if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
			*(PLONG)PropertyRequest->Value = that->requestedChannelMask;
			PropertyRequest->ValueSize = sizeof(ULONG);
			return STATUS_SUCCESS;
		} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
			if (*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_7POINT1) {
				that->requestedChannelMask =  *(PLONG)PropertyRequest->Value;
				that->requestedChannelCount = 8;
				return STATUS_SUCCESS;
			}
			if (*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_5POINT1) {
				that->requestedChannelMask =  *(PLONG)PropertyRequest->Value;
				that->requestedChannelCount = 6;
				return STATUS_SUCCESS;
			}
			if ((*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_QUAD) || (*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_SURROUND)) {
				that->requestedChannelMask =  *(PLONG)PropertyRequest->Value;
				that->requestedChannelCount = 4;
				return STATUS_SUCCESS;
			}
			if (*(PLONG)PropertyRequest->Value == KSAUDIO_SPEAKER_STEREO) {
				that->requestedChannelMask =  *(PLONG)PropertyRequest->Value;
				that->requestedChannelCount = 2;
				return STATUS_SUCCESS;
			}
		} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
			PULONG AccessFlags = PULONG(PropertyRequest->Value);
			*AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
			PropertyRequest->ValueSize = sizeof(ULONG);
			return STATUS_SUCCESS;
		}
	}
	return STATUS_INVALID_PARAMETER;
}

///////////////////////////

NTSTATUS CreateMiniportWaveStreamCMI(CMiniportWaveStreamCMI  **MiniportWaveStreamCMI, PUNKNOWN pUnknownOuter, POOL_TYPE PoolType)
{
	PAGED_CODE();
	DBGPRINT(("CreateMiniportWaveStreamCMI"));

#ifdef WAVERT
	*MiniportWaveStreamCMI = new (PoolType, 'gnaa') CMiniportWaveStreamCMI(NULL);
#else
	*MiniportWaveStreamCMI = new (PoolType, 'gnaa') CMiniportWaveStreamCMI(pUnknownOuter);
#endif
	if (*MiniportWaveStreamCMI) {
		(*MiniportWaveStreamCMI)->AddRef();
		return STATUS_SUCCESS;
	}

	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS CMiniportWaveStreamCMI::prepareStream()
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::prepareStream()", this));
	DBGPRINT(("---streamIndex: %d, channelNumber: %d", streamIndex, channelNumber));

	NTSTATUS ntStatus;
	UInt32   val;

	if (state == KSSTATE_RUN) {
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	if (!(Miniport->cm)) {
		DBGPRINT(("Miniport not set"));
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	enableSPDIF = ((currentSampleRate == 44100 || currentSampleRate == 48000 || currentSampleRate == 88200 || currentSampleRate == 96000) &&
		           ((currentResolution == 16) || (currentResolution == 32)) && (currentChannelCount == 2)) &&
		           (Miniport->cm->enableSPDIFOut);

	if (!isCaptureStream) {
		ntStatus = setupSPDIFPlayback(enableSPDIF);
		if (!NT_SUCCESS(ntStatus)) {
			return ntStatus;
		}
		ntStatus = setDACChannels();
		if (!NT_SUCCESS(ntStatus)) {
			return ntStatus;
		}
	}

	KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, false, NULL);

	val = channelNumber ? ADC_CH1 : ADC_CH0;
	if (isCaptureStream) {
		Miniport->cm->regFUNCTRL0 |= val;  // 1->Recording
	} else {
		Miniport->cm->regFUNCTRL0 &= ~val; // 0->Playback
	}
	Miniport->CMIAdapter->writeUInt32(REG_FUNCTRL0, Miniport->cm->regFUNCTRL0);

	//set sampling frequency
	val = Miniport->CMIAdapter->readUInt32(REG_FUNCTRL1);
	if ((currentSampleRate == 88200) || (currentSampleRate == 44100)) {
		if (channelNumber) {
			val &= ~SFC_CH1_MASK;
			val |= SFC_44K_CH1;
		} else {
			val &= ~SFC_CH0_MASK;
			val |= SFC_44K_CH0;
		}
	} else if ((currentSampleRate == 96000) || (currentSampleRate == 48000)) {
		if (channelNumber) {
			val &= ~SFC_CH1_MASK;
			val |= SFC_48K_CH1;
		} else {
			val &= ~SFC_CH0_MASK;
			val |= SFC_48K_CH0;
		}
	} else {
			KeReleaseMutex(&Miniport->mutex, FALSE);
			return STATUS_INVALID_DEVICE_REQUEST;
	}
	Miniport->CMIAdapter->writeUInt32(REG_FUNCTRL1, val);

	//set resolution
	val = Miniport->CMIAdapter->readUInt32(REG_CHFORMAT);
	if (channelNumber) {
		val |= FORMAT_CH1;
	} else {
		val |= FORMAT_CH0;
	}
	Miniport->CMIAdapter->writeUInt32(REG_CHFORMAT, val);

	KeReleaseMutex(&Miniport->mutex, false);

	return STATUS_SUCCESS;
}

NTSTATUS CMiniportWaveStreamCMI::setDACChannels()
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::setDACChannels()", this));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	if (currentChannelCount > 2) {
		if (Miniport->cm->maxChannels < currentChannelCount) {
			return STATUS_INVALID_DEVICE_REQUEST;
		}
		if ((currentResolution != 16) || (currentChannelCount < 2)) {
			return STATUS_INVALID_DEVICE_REQUEST;
		}
#if OUT_CHANNEL == 0
		return STATUS_INVALID_DEVICE_REQUEST;
#endif
		KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, FALSE, NULL);
		Miniport->CMIAdapter->setUInt32Bit(REG_LEGACY, DWORD_MAPPING);
		Miniport->CMIAdapter->setUInt32Bit(REG_MISCCTRL, XCHG_DAC);

		switch (currentChannelCount) {
			case 4:
				Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, EN_4CH_CH1);
				Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, EN_5CH_CH1);
				Miniport->CMIAdapter->clearUInt32Bit(REG_LEGACY, EN_6CH_CH1);
				Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, EN_CENTER);
				break;
			case 6:
				Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, EN_4CH_CH1);
				Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, EN_5CH_CH1);
				Miniport->CMIAdapter->setUInt32Bit(REG_LEGACY, EN_6CH_CH1);
				Miniport->CMIAdapter->setUInt32Bit(REG_MISCCTRL, EN_CENTER);
				break;
			case 8:
				if (Miniport->cm->chipVersion == 68) {
					Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, EN_4CH_CH1);
					Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, EN_5CH_CH1);
					Miniport->CMIAdapter->setUInt32Bit(REG_LEGACY, EN_6CH_CH1);
					Miniport->CMIAdapter->setUInt32Bit(REG_MISCCTRL, EN_CENTER);
					Miniport->CMIAdapter->setUInt32Bit(REG_MISCCTRL2, EN_8CH_CH1);
					break;
				} else {
					ntStatus = STATUS_INVALID_DEVICE_REQUEST;
				}
			default:
				ntStatus = STATUS_INVALID_DEVICE_REQUEST;
		}
		KeReleaseMutex(&Miniport->mutex, FALSE);
	} else {
		if (Miniport->cm->canMultiChannel) {
			KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, FALSE, NULL);
			Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, EN_5CH_CH1 | EN_4CH_CH1);
			Miniport->CMIAdapter->clearUInt32Bit(REG_LEGACY, EN_6CH_CH1 | DWORD_MAPPING);
			Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, EN_CENTER);
			Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, XCHG_DAC);
			if (Miniport->cm->chipVersion == 68) {
				Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL2, EN_8CH_CH1);
			}
			KeReleaseMutex(&Miniport->mutex, FALSE);
		}
	}
	return ntStatus;
}

NTSTATUS CMiniportWaveStreamCMI::setupSPDIFPlayback(bool enableSPDIF)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::setupSPDIFPlayback(%d)", this, enableSPDIF));

	KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, false, NULL);

	if (enableSPDIF) {
		Miniport->CMIAdapter->setUInt32Bit(REG_LEGACY, EN_SPDIF_OUT);
		Miniport->CMIAdapter->setUInt32Bit(REG_FUNCTRL1, SPDO2DAC);
#if OUT_CHANNEL == 0
		Miniport->CMIAdapter->setUInt32Bit(REG_FUNCTRL1, SPDF_0);
#else
		Miniport->CMIAdapter->setUInt32Bit(REG_FUNCTRL1, SPDF_1);
#endif
		setupAC3Passthru();

		if ( (currentSampleRate == 48000) || (currentSampleRate == 96000) ) {
			Miniport->CMIAdapter->setUInt32Bit(REG_MISCCTRL, EN_SPDIF_48);
		} else {
			Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, EN_SPDIF_48);
		}

		if (currentSampleRate == 96000) {
#if OUT_CHANNEL == 0
			Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SPD96_CH0);
#else
			Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SPD96_CH1);
#endif
			Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, DBLSPDS);
		} else if (currentSampleRate == 88200) {
#if OUT_CHANNEL == 0
			Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SPD88_CH0);
#else
			Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SPD88_CH1);
#endif
			Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, DBLSPDS);
		} else {
#if OUT_CHANNEL == 0
			Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD88_CH0 | SPD96_CH0);
#else
			Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD88_CH1 | SPD96_CH1);
#endif
			Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, DBLSPDS);
		}

	} else {
		Miniport->CMIAdapter->clearUInt32Bit(REG_LEGACY, EN_SPDIF_OUT);
		Miniport->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, SPDO2DAC);
#if OUT_CHANNEL == 0
		Miniport->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, SPDF_0);
#else
		Miniport->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, SPDF_1);
#endif
#if OUT_CHANNEL == 0
		Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD88_CH0 | SPD96_CH0);
#else
		Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD88_CH1 | SPD96_CH1);
#endif
		Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, DBLSPDS);
		setupAC3Passthru();
	}

	KeReleaseMutex(&Miniport->mutex, false);
	return STATUS_SUCCESS;
}

NTSTATUS CMiniportWaveStreamCMI::setupAC3Passthru()
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::setupAC3Passthru() [enableAC3Passthru: %d]", this, enableAC3Passthru));

	if (enableAC3Passthru) {
		Miniport->CMIAdapter->writeUInt8(REG_MIXER1, Miniport->CMIAdapter->readUInt8(REG_MIXER1) | MUTE_WAVE);

		Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, EN_SPDO_AC3_1);
		Miniport->CMIAdapter->setUInt32Bit(REG_MISCCTRL, EN_SPDO_AC3_2);

		if (Miniport->cm->canAC3HW) {
			Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, EN_SPDO_AC3_3);
			Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, SPD32SEL);
			if (Miniport->cm->chipVersion >= 39) {
				Miniport->CMIAdapter->clearUInt8Bit(REG_MIXER1, EN_SPDI2DAC);
			}
		} else {
			Miniport->CMIAdapter->setUInt32Bit(REG_MISCCTRL, SPD32SEL);
			if (Miniport->cm->chipVersion == 33) {
				if (currentSampleRate >= 48000) {
#if OUT_CHANNEL == 0
					 Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SPD96_CH0);
#else
					 Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SPD96_CH1);
#endif
				} else {
#if OUT_CHANNEL == 0
					 Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD96_CH0);
#else
					 Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD96_CH1);
#endif
				}
			}
		}
	} else {
		if (Miniport->cm->enableSPDIFInMonitor) {
			Miniport->CMIAdapter->setUInt8Bit(REG_MIXER1, EN_SPDI2DAC);
		}

		Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, EN_SPDO_AC3_1);
		Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, EN_SPDO_AC3_2);

		if (Miniport->cm->canAC3HW) {
			Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, EN_SPDO_AC3_3);
			if (currentResolution > 16) {
				Miniport->CMIAdapter->setUInt32Bit(REG_MISCCTRL, SPD32SEL);
				Miniport->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SPD24SEL);
			} else {
				Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, SPD32SEL);
				Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD24SEL);
			}
		} else {
			Miniport->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, SPD32SEL);
#if OUT_CHANNEL == 0
			Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD96_CH0);
#else
			Miniport->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SPD96_CH1);
#endif
		}
	}
	return STATUS_SUCCESS;
}

CMiniportWaveStreamCMI::~CMiniportWaveStreamCMI(void)
{
	PAGED_CODE();

	DBGPRINT(("CMiniportWaveStreamCMI[%p]::~CMiniportWaveStreamCMI [streamIndex: %d]", this, streamIndex));

	if (state != KSSTATE_STOP) {
		SetState(KSSTATE_STOP);
	}

#ifdef WAVERT
	if (Port) {
		Port->Release();
		Port = NULL;
	}
#else
	if (DMAChannel) {
		DMAChannel->Release();
		DMAChannel = NULL;
	}

	if (ServiceGroup) {
		ServiceGroup->Release();
		ServiceGroup = NULL;
	}
#endif

	Miniport->isStreamRunning[streamIndex] = false;

	if ((streamIndex == AC3_OUT_STREAM) && Miniport->isStreamRunning[PCM_OUT_STREAM]) {
		KSSTATE temp = Miniport->stream[PCM_OUT_STREAM]->state;
		Miniport->stream[PCM_OUT_STREAM]->state = KSSTATE_STOP;
		Miniport->stream[PCM_OUT_STREAM]->prepareStream();
		Miniport->stream[PCM_OUT_STREAM]->SetState(KSSTATE_ACQUIRE);
		Miniport->stream[PCM_OUT_STREAM]->state = temp;
		Miniport->stream[PCM_OUT_STREAM]->SetState(KSSTATE_RUN_AC3);
	}

	if (Miniport) {
		Miniport->Release();
		Miniport = NULL;
	}
}

STDMETHODIMP CMiniportWaveStreamCMI::NonDelegatingQueryInterface(REFIID Interface, PVOID *Object)
{
	PAGED_CODE();
	ASSERT(Object);
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::NonDelegatingQueryInterface(%p, %p)", this, Interface, Object));

	if (IsEqualGUIDAligned(Interface,IID_IUnknown)) {
		*Object = PVOID(PUNKNOWN(PMINIPORTWAVECYCLICSTREAM(this)));
#ifdef WAVERT
	} else if (IsEqualGUIDAligned(Interface,IID_IMiniportWaveRTStream)) {
		*Object = PVOID(PMINIPORTWAVERTSTREAM(this));
#else
	} else if (IsEqualGUIDAligned(Interface,IID_IMiniportWaveCyclicStream)) {
		*Object = PVOID(PMINIPORTWAVECYCLICSTREAM(this));
#endif
	} else if (IsEqualGUIDAligned (Interface, IID_IDrmAudioStream)) {
		*Object = (PVOID)(PDRMAUDIOSTREAM(this));
	} else {
		*Object = NULL;
	}

	if (*Object) {
		PUNKNOWN(*Object)->AddRef();
		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_PARAMETER;
}

#ifdef WAVERT
NTSTATUS CMiniportWaveStreamCMI::Init(CMiniportWaveCMI* Miniport_, UInt32 streamIndex_, bool isCaptureStream_, PKSDATAFORMAT DataFormat, PPORTWAVERTSTREAM Port_)
#else
NTSTATUS CMiniportWaveStreamCMI::Init(CMiniportWaveCMI* Miniport_, UInt32 streamIndex_, bool isCaptureStream_, PKSDATAFORMAT DataFormat, PDMACHANNEL DMAChannel_, PSERVICEGROUP* OutServiceGroup)
#endif
{
	PAGED_CODE();
	ASSERT(Miniport_);
	ASSERT(DataFormat);

	NTSTATUS ntStatus;

#ifdef WAVERT
	ASSERT(Port_);
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::Init(%p, %d, %d, %p, %p)", this, Miniport_, streamIndex_, isCaptureStream_, DataFormat, Port_));
	Port = Port_;
	Port->AddRef();
#else
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::Init(%p, %d, %d, %p, %p, %p)", this, Miniport_, streamIndex_, isCaptureStream_, DataFormat, DMAChannel_, OutServiceGroup));
	DMAChannel = DMAChannel_;
	DMAChannel->AddRef();
#endif

	Miniport = Miniport_;
	Miniport->AddRef();

	streamIndex     = streamIndex_;
	isCaptureStream = isCaptureStream_;
	state           = KSSTATE_STOP;

	if ( (streamIndex == PCM_OUT_STREAM) || (streamIndex == AC3_OUT_STREAM) ) {
		channelNumber = OUT_CHANNEL;
	} else {
		channelNumber = IN_CHANNEL;
	}

#ifndef WAVERT
	ntStatus = PcNewServiceGroup(&ServiceGroup,NULL);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("PcNewServiceGroup() or NewMasterDmaChannel() failed"));
		return ntStatus;
	}
	*OutServiceGroup = ServiceGroup;
	ServiceGroup->AddRef();
#endif

	ntStatus = SetFormat(DataFormat);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("SetFormat() failed"));
		return ntStatus;
	}

	Miniport->isStreamRunning[streamIndex] = true;

	return ntStatus;
}

NTSTATUS CMiniportWaveStreamCMI::SetFormat(PKSDATAFORMAT Format)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::SetFormat(%p)", this, Format));
	PWAVEFORMATEX waveFormat = PWAVEFORMATEX(Format + 1);
	NTSTATUS ntStatus = Miniport->validateFormat(Format, -1, isCaptureStream);
	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}
	// the DAC and ADC can only run at the same sample rate simultaneously
	if ((streamIndex == PCM_IN_STREAM) && Miniport->isStreamRunning[PCM_OUT_STREAM]) {
   		if (waveFormat->nSamplesPerSec != Miniport->stream[PCM_OUT_STREAM]->currentSampleRate) {
   			return STATUS_UNSUCCESSFUL;
		}
	}
	if ((streamIndex == PCM_IN_STREAM) && Miniport->isStreamRunning[AC3_OUT_STREAM]) {
   		if (waveFormat->nSamplesPerSec != Miniport->stream[AC3_OUT_STREAM]->currentSampleRate) {
   			return STATUS_UNSUCCESSFUL;
		}
	}
	if ((streamIndex == PCM_OUT_STREAM) && Miniport->isStreamRunning[PCM_IN_STREAM]) {
   		if (waveFormat->nSamplesPerSec != Miniport->stream[PCM_IN_STREAM]->currentSampleRate) {
   			return STATUS_UNSUCCESSFUL;
		}
	}
	if ((streamIndex == PCM_OUT_STREAM) && Miniport->isStreamRunning[AC3_OUT_STREAM]) {
		return STATUS_UNSUCCESSFUL;
	}

	KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, false, NULL);
	currentSampleRate   = waveFormat->nSamplesPerSec;
	currentChannelCount = waveFormat->nChannels;
	currentResolution   = waveFormat->wBitsPerSample;
	enableAC3Passthru   = (WAVE_FORMAT_DOLBY_AC3_SPDIF == EXTRACT_WAVEFORMATEX_ID(&Format->SubFormat)) || (WAVE_FORMAT_WMA_SPDIF == EXTRACT_WAVEFORMATEX_ID(&Format->SubFormat));
	KeReleaseMutex(&Miniport->mutex, false);
	ntStatus = prepareStream();

	return ntStatus;
}

// DRM crap - we're supposed to disable every digital interface here
STDMETHODIMP_(NTSTATUS) CMiniportWaveStreamCMI::SetContentId(ULONG contentId, PCDRMRIGHTS drmRights)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::SetContentId(%d, %p)", this, contentId, drmRights));

	return STATUS_SUCCESS;
}

#ifdef WAVERT
STDMETHODIMP_(NTSTATUS) CMiniportWaveStreamCMI::AllocateAudioBuffer(ULONG size, PMDL *userModeBuffer, ULONG *bufferSize, ULONG *bufferOffset, MEMORY_CACHING_TYPE *cacheType)
{
	PAGED_CODE();

	PHYSICAL_ADDRESS    low;
	PHYSICAL_ADDRESS    high;
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::AllocateAudioBuffer(0x%x, %p, %p, %p, %p)", this, size, userModeBuffer, bufferSize, bufferOffset, cacheType));

	if (size <= size % (currentChannelCount * 2)) {
		return STATUS_UNSUCCESSFUL;
	}
	size -= size % (currentChannelCount * 2);

	if (size == 0) {
		return STATUS_UNSUCCESSFUL;
	}

	low.HighPart = 0; low.LowPart = 0;
	high.HighPart = 0; high.LowPart = MAXULONG;
	if (size <= 4096) {
		audioBufferMDL = Port->AllocatePagesForMdl(high, size);
    	} else {
		audioBufferMDL = Port->AllocateContiguousPagesForMdl(low, high, size);
    	}
	if (!audioBufferMDL) {
		DBGPRINT(("AllocateContiguousPagesForMdl()/AllocatePagesForMdl() failed (size: 0x%x)", size));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	dmaAddress = Port->GetPhysicalPageAddress(audioBufferMDL, 0).LowPart;
	dmaMemorySize = size;

	*userModeBuffer = audioBufferMDL;
	*bufferSize = size;
	*bufferOffset = 0;
	*cacheType = MmCached;

	return STATUS_SUCCESS;
}


STDMETHODIMP_(VOID) CMiniportWaveStreamCMI::FreeAudioBuffer(PMDL Mdl, ULONG Size)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::FreeAudioBuffer(%p, %x)", this, Mdl, Size));

	Port->FreePagesFromMdl(Mdl);
	audioBufferMDL = NULL;
	dmaAddress     = 0;
	dmaMemorySize  = 0;
}

STDMETHODIMP_(void) CMiniportWaveStreamCMI::GetHWLatency(PKSRTAUDIO_HWLATENCY hwLatency)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::GetHWLatency(%p)", this, hwLatency));
	hwLatency->FifoSize     = 32;
	hwLatency->ChipsetDelay = 0;
	hwLatency->CodecDelay   = 4;
}

STDMETHODIMP_(NTSTATUS) CMiniportWaveStreamCMI::GetPositionRegister(PKSRTAUDIO_HWREGISTER hwRegister)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::GetPositionRegister(%p)", this, hwRegister));

	return STATUS_UNSUCCESSFUL;
}

STDMETHODIMP_(NTSTATUS) CMiniportWaveStreamCMI::GetClockRegister(PKSRTAUDIO_HWREGISTER hwRegister)
{
	PAGED_CODE();
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::GetClockRegister(%p)", this, hwRegister));

	return STATUS_UNSUCCESSFUL;
}

#endif // WAVERT

/*
** non-paged code below
*/
#ifdef _MSC_VER
#pragma code_seg()
#endif

STDMETHODIMP CMiniportWaveStreamCMI::SetState(KSSTATE NewState)
{
	DBGPRINT(("CMiniportWaveStreamCMI[%p]::SetState(%d) [streamIndex: %d, channelNumber: %d]", this, NewState, streamIndex, channelNumber));

	UInt32 inthld, chen, reset, pause;
	UInt8  reg;

	inthld = EN_CH0_INT << channelNumber;
	chen   = EN_CH0     << channelNumber;
	reset  = RST_CH0    << channelNumber;
	pause  = PAUSE_CH0  << channelNumber;

	NTSTATUS ntStatus = STATUS_SUCCESS;

	if ((streamIndex == PCM_OUT_STREAM) && Miniport->isStreamRunning[AC3_OUT_STREAM]) {
		return STATUS_INVALID_PARAMETER;
	}

	if (NewState == KSSTATE_RUN_AC3) {
		NewState = state;
		state = KSSTATE_STOP;
	}

	// STOP -> ACQUIRE -> PAUSE -> PLAY -> PAUSE -> ACQUIRE -> STOP
	if (state != NewState) {
		switch (NewState) {
			case KSSTATE_ACQUIRE:
				DBGPRINT(("---KSSTATE_ACQUIRE: previous state: %d", state));
				if (state == KSSTATE_PAUSE) {
					break;
				}

#ifdef WAVERT
				if ((dmaMemorySize == 0) || (dmaAddress == 0)) {
					return STATUS_UNSUCCESSFUL;
				}
				dmaSize = (dmaMemorySize / (2 * (currentResolution >> 3)) );
				periodSize = dmaSize;
				DBGPRINT(("---dmaAddress: %x, dmaMemorySize: %x, dmaSize: %x", dmaAddress, dmaMemorySize, dmaSize));
#else
				if (currentResolution == 32) {
					dmaSize = (DMAChannel->BufferSize() / (2 * (32 >> 3)) );
				} else {
					dmaSize = (DMAChannel->BufferSize() / (2 * (currentResolution >> 3)) );
				}
#endif
				DBGPRINT(("---SampleRate: %d, Resolution: %d, Channels: %d", currentSampleRate, currentResolution, currentChannelCount));

				if (periodSize > dmaSize) {
					periodSize = dmaSize;
				}

				// set address of the DMA buffer
				KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, FALSE, NULL);
				reg = channelNumber ? REG_CH1_FRAME1 : REG_CH0_FRAME1;
#ifdef WAVERT
				Miniport->CMIAdapter->writeUInt32(reg, dmaAddress);
#else
				Miniport->CMIAdapter->writeUInt32(reg, DMAChannel->PhysicalAddress().u.LowPart);
				DBGPRINT(("---DMA Address: HighPart: 0x%08X LowPart: 0x%08X", DMAChannel->PhysicalAddress().u.HighPart, DMAChannel->PhysicalAddress().u.LowPart));
#endif
				// count of samples
				reg = channelNumber ? REG_CH1_FRAME2 : REG_CH0_FRAME2;
				Miniport->CMIAdapter->writeUInt16(reg, dmaSize-1);
				Miniport->CMIAdapter->writeUInt16(reg + 2, periodSize-1);
				DBGPRINT(("---DMA Size:   0x%04X, Period Size: 0x%04X, enableSPDIFIn: %d", dmaSize, periodSize, Miniport->cm->enableSPDIFIn));
				if (isCaptureStream) {
					if (Miniport->cm->enableSPDIFIn) {
#if OUT_CHANNEL==0
						Miniport->CMIAdapter->setUInt32Bit(REG_FUNCTRL1, SPDF_1);
#else
						Miniport->CMIAdapter->setUInt32Bit(REG_FUNCTRL1, SPDF_0);
#endif
					} else {
#if OUT_CHANNEL==0
						Miniport->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, SPDF_1);
#else
						Miniport->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, SPDF_0);
#endif
					}
				}
				KeReleaseMutex(&Miniport->mutex, false);
				break;

			case KSSTATE_PAUSE:
				DBGPRINT(("---KSSTATE_PAUSE: previous state: %d", state));
				if (state == KSSTATE_RUN) {
					KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, false, NULL);
					Miniport->cm->regFUNCTRL0 |= pause;
					Miniport->CMIAdapter->writeUInt32(REG_FUNCTRL0, Miniport->cm->regFUNCTRL0);
					KeReleaseMutex(&Miniport->mutex, FALSE);
				}
				if (state == KSSTATE_STOP) {
					KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, false, NULL);
					Miniport->cm->regFUNCTRL0 &= ~pause;
					Miniport->CMIAdapter->writeUInt32(REG_FUNCTRL0, Miniport->cm->regFUNCTRL0);
					KeReleaseMutex(&Miniport->mutex, false);
				}
				break;

			case KSSTATE_RUN:
				DBGPRINT(("---KSSTATE_RUN: previous state: %d", state));

				KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, FALSE, NULL);
				// set interrupt
				Miniport->CMIAdapter->setUInt32Bit(REG_INTHLDCLR, inthld);
				Miniport->cm->regFUNCTRL0 &= ~pause;
				Miniport->cm->regFUNCTRL0 |= chen;
				// and enable the channel
				Miniport->CMIAdapter->writeUInt32(REG_FUNCTRL0, Miniport->cm->regFUNCTRL0);

				DBGPRINT(("---FUNCTRL0:   0x%08X", Miniport->cm->regFUNCTRL0));
				DBGPRINT(("---FUNCTRL1:   0x%08X", Miniport->CMIAdapter->readUInt32(REG_FUNCTRL1)));
				DBGPRINT(("---CHFORMAT:   0x%08X", Miniport->CMIAdapter->readUInt32(REG_CHFORMAT)));
				DBGPRINT(("---LEGACYCTRL: 0x%08X", Miniport->CMIAdapter->readUInt32(REG_LEGACY)));
				DBGPRINT(("---MISCCTRL:   0x%08X", Miniport->CMIAdapter->readUInt32(REG_MISCCTRL)));
				DBGPRINT(("---MIX1:       0x%02X", Miniport->CMIAdapter->readUInt8(REG_MIXER1)));
				DBGPRINT(("---MIX2:       0x%02X", Miniport->CMIAdapter->readUInt8(REG_MIXER2)));
				DBGPRINT(("---MIX3:       0x%02X", Miniport->CMIAdapter->readUInt8(REG_MIXER3)));

				KeReleaseMutex(&Miniport->mutex, false);
				break;

			case KSSTATE_STOP_AC3:
			case KSSTATE_STOP:
				DBGPRINT(("---KSSTATE_STOP: previous state: %d", state));
				KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, FALSE, NULL);
				// clear interrupt
				Miniport->CMIAdapter->clearUInt32Bit(REG_INTHLDCLR, inthld);
				Miniport->cm->regFUNCTRL0 &= ~chen;
				// reset
				Miniport->CMIAdapter->writeUInt32(REG_FUNCTRL0, Miniport->cm->regFUNCTRL0 | reset);
				Miniport->CMIAdapter->writeUInt32(REG_FUNCTRL0, Miniport->cm->regFUNCTRL0 & ~reset);
				if (isCaptureStream && (Miniport->cm->enableSPDIFIn)) {
#if OUT_CHANNEL==0
					Miniport->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, SPDF_1);
#else
					Miniport->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, SPDF_0);
#endif
				}
				KeReleaseMutex(&Miniport->mutex, FALSE);
				break;
		}
		if (NewState != KSSTATE_STOP_AC3) {
			state = NewState;
		}
	}
	return ntStatus;
}

#ifdef WAVERT

STDMETHODIMP CMiniportWaveStreamCMI::GetPosition(PKSAUDIO_POSITION Position)
{
	ASSERT(Position);

	UInt32 reg;

	if ((state == KSSTATE_RUN) && (dmaAddress)) {
		reg = (channelNumber) ? REG_CH1_FRAME1 : REG_CH0_FRAME1;
		Position->PlayOffset = Miniport->CMIAdapter->readUInt32(reg) - dmaAddress;
		Position->WriteOffset = Position->PlayOffset + currentChannelCount * 2 * 8;
	} else {
		Position->PlayOffset = 0;
		Position->WriteOffset = 0;
	}

	return STATUS_SUCCESS;
}

#else //WaveCyclic

STDMETHODIMP CMiniportWaveStreamCMI::GetPosition(PULONG Position)
{
	ASSERT(Position);

	UInt32 reg;

	if ((DMAChannel) && (state == KSSTATE_RUN)) {
#if 0
// this implementation messes with SPDIF-in recording
		reg = (channelNumber) ? REG_CH1_FRAME2 : REG_CH0_FRAME2;
		*Position = dmaSize - (Miniport->CMIAdapter->readUInt16(reg)-1);
		*Position *= 2 * (currentResolution >> 3);
#else
		reg = (channelNumber) ? REG_CH1_FRAME1 : REG_CH0_FRAME1;
		*Position = Miniport->CMIAdapter->readUInt32(reg);
		if (*Position > DMAChannel->PhysicalAddress().u.LowPart) {
			*Position -= DMAChannel->PhysicalAddress().u.LowPart;
		} else {
			*Position = 0;
		}
#endif
	} else {
		*Position = 0;
	}

	return STATUS_SUCCESS;
}

STDMETHODIMP_(ULONG) CMiniportWaveStreamCMI::SetNotificationFreq(ULONG Interval, PULONG FramingSize)
{
	Miniport->notificationInterval = Interval;

	if (state == KSSTATE_RUN) {
		return 0;
	}
	// periodSize [sample] = interval [ms] * sample rate [Hz] * 1e-3 [milli]
	periodSize   = Interval * currentSampleRate / 1000;
	// FramingSize [byte] = periodSize [sample] * #Channels * resolution [byte];
	*FramingSize = periodSize * currentChannelCount * (currentResolution >> 3);

	KeWaitForSingleObject(&Miniport->mutex, Executive, KernelMode, FALSE, NULL);
	Miniport->CMIAdapter->writeUInt16((channelNumber ? REG_CH1_FRAME2 : REG_CH0_FRAME2) + 2, periodSize-1);
	KeReleaseMutex(&Miniport->mutex, FALSE);

	DBGPRINT(("periodSize: %x, FramingSize: %x", periodSize, *FramingSize));
	return Interval;
}

STDMETHODIMP CMiniportWaveStreamCMI::NormalizePhysicalPosition(PLONGLONG PhysicalPosition)
{
	// time_pos [ns] = byte_pos [byte] / (1e-9 [nano] * #Channels * resolution [byte] * sample rate [Hz])
	*PhysicalPosition = (*PhysicalPosition * 10000000L) / (currentChannelCount * (currentResolution >> 3) * currentSampleRate);
	return STATUS_SUCCESS;
}


STDMETHODIMP_(void) CMiniportWaveStreamCMI::Silence(PVOID Buffer, ULONG ByteCount)
{
	RtlFillMemory(Buffer, ByteCount, 0x00);
}

#endif //WAVERT

STDMETHODIMP_(void) CMiniportWaveCMI::ServiceWaveISR(UInt32 streamIndex)
{
#ifndef WAVERT
	if ((streamIndex == PCM_OUT_STREAM) && isStreamRunning[AC3_OUT_STREAM]) {
		streamIndex = AC3_OUT_STREAM;
	}
	if (Port && stream[streamIndex]->ServiceGroup) {
		Port->Notify(stream[streamIndex]->ServiceGroup);
	}
#endif
}
