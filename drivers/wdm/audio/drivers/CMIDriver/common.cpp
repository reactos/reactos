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

#include "common.hpp"

#pragma code_seg("PAGE")

NTSTATUS NewCMIAdapter(PUNKNOWN *Unknown, REFCLSID, PUNKNOWN UnknownOuter, POOL_TYPE PoolType)
{
	PAGED_CODE();
	DBGPRINT(("NewCMIAdapter()"));
	ASSERT (Unknown);
	STD_CREATE_BODY_(CCMIAdapter, Unknown, UnknownOuter, PoolType, PCMIADAPTER);
}


STDMETHODIMP_(NTSTATUS) CCMIAdapter::init(PRESOURCELIST ResourceList, PDEVICE_OBJECT aDeviceObject)
{
    PAGED_CODE();
	ASSERT(ResourceList);
	ASSERT(aDeviceObject);
	ASSERT(ResourceList->FindTranslatedPort(0));
	DBGPRINT(("CCMIAdapter[%p]::init()", this));

	NTSTATUS ntStatus = STATUS_SUCCESS;

	RtlFillMemory(&mixerCache, 0xFF, 0xFF);
	RtlFillMemory(&cm, sizeof(cm), 0x00);

	DeviceObject = aDeviceObject;

	cm.IOBase = 0;
	for (int i=0;i<ResourceList->NumberOfPorts();i++) {
		if (ResourceList->FindTranslatedPort(i)->u.Port.Length == 0x100) {
			cm.IOBase = (UInt32*)ResourceList->FindTranslatedPort(i)->u.Port.Start.QuadPart;
		}
	}

	if (cm.IOBase == 0) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	cm.MPUBase = 0;

#ifdef WAVERT
	INFOPRINT(("Driver Version: %s-WAVERT", CMIVERSION));
#else
	INFOPRINT(("Driver Version: %s", CMIVERSION));
#endif
	INFOPRINT(("Configuration:"));
	INFOPRINT(("    IO Base:      0x%X", cm.IOBase));
	INFOPRINT(("    MPU Base:     0x%X", cm.MPUBase));

	if (!queryChip()) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	INFOPRINT(("    Chip Version: %d", cm.chipVersion));
	INFOPRINT(("    Max Channels: %d", cm.maxChannels));
	INFOPRINT(("    CanAC3HW:     %d", cm.canAC3HW));

	resetController();

	ntStatus = PcNewInterruptSync(&(InterruptSync), NULL, ResourceList, 0, InterruptSyncModeNormal);
	if (!NT_SUCCESS(ntStatus) || !(InterruptSync)) {
		DBGPRINT(("Failed to create an interrupt sync!"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	ntStatus = InterruptSync->RegisterServiceRoutine(InterruptServiceRoutine, (PVOID)this, FALSE);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Failed to register ISR!"));
		return ntStatus;
	}

	ntStatus = InterruptSync->Connect();
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Failed to connect the ISR with InterruptSync!"));
		return ntStatus;
	}

	// Initialize the device state.
	CurrentPowerState = PowerDeviceD0;

	return ntStatus;
}


CCMIAdapter::~CCMIAdapter()
{
	PAGED_CODE ();
	DBGPRINT(("CCMIAdapter[%p]::~CCMIAdapter()", this));

	if (InterruptSync) {
		InterruptSync->Disconnect();
		InterruptSync->Release();
		InterruptSync = NULL;
	}
}

STDMETHODIMP_(NTSTATUS) CCMIAdapter::NonDelegatingQueryInterface(REFIID Interface, PVOID* Object)
{
    PAGED_CODE();

	DBGPRINT(("CCMIAdapter[%p]::NonDelegatingQueryInterface()", this));

	ASSERT(Object);

	// Is it IID_IUnknown?
	if (IsEqualGUIDAligned (Interface, IID_IUnknown)) {
		*Object = (PVOID)(PUNKNOWN)(PCMIADAPTER)this;
	}
	else
	// or IID_IAdapterCommon ...
	if (IsEqualGUIDAligned (Interface, IID_ICMIAdapter)) {
		*Object = (PVOID)(PCMIADAPTER)this;
	} else
	// or IID_IAdapterPowerManagement ...
	if (IsEqualGUIDAligned (Interface, IID_IAdapterPowerManagement)) {
		*Object = (PVOID)(PADAPTERPOWERMANAGEMENT)this;
	} else {
	// nothing found, must be an unknown interface.
		*Object = NULL;
		return STATUS_INVALID_PARAMETER;
	}

	//
	// We reference the interface for the caller.
	//
	((PUNKNOWN)*Object)->AddRef();
	return STATUS_SUCCESS;
}

bool CCMIAdapter::queryChip()
{
	PAGED_CODE();
	DBGPRINT(("CCMIAdapter[%p]::queryChip()", this));

	UInt32 version = readUInt32(REG_INTHLDCLR) & VERSION_MASK;
	if (version == 0xFFFFFFFF) {
		return false;
	}
	if (version) {
		if (version & VERSION_68) {
			cm.chipVersion = 68;
			cm.maxChannels = 8;
			cm.canAC3HW = true;
			cm.hasDualDAC = true;
			cm.canMultiChannel = true;
			return true;
		}
		if (version & VERSION_55) {
			cm.chipVersion = 55;
			cm.maxChannels = 6;
			cm.canAC3HW = true;
			cm.hasDualDAC = true;
			cm.canMultiChannel = true;
			return true;
		}
		if (version & VERSION_39) {
			cm.chipVersion = 39;
			if (version & VERSION_39_6) {
				cm.maxChannels = 6;
			} else {
				cm.maxChannels = 4;
			}
			cm.canAC3HW = true;
			cm.hasDualDAC = true;
			cm.canMultiChannel = true;
			return true;
		}
	} else {
		version = readUInt32(REG_CHFORMAT) & VERSION_37;
		if (!version) {
			cm.chipVersion = 33;
			cm.maxChannels = 2;
			if (cm.doAC3SW) {
				cm.canAC3SW = true;
			} else {
				cm.canAC3HW = true;
			}
			cm.hasDualDAC = true;
			return true;
		} else {
			cm.chipVersion = 37;
			cm.maxChannels = 2;
			cm.canAC3HW = true;
			cm.hasDualDAC = 1;
			return true;
		}
	}
	return false;
}

void CCMIAdapter::resetMixer()
{
	PAGED_CODE();
	DBGPRINT(("CCMIAdapter[%p]::resetMixer()", this));

	writeMixer(0, 0);
	setUInt8Bit(REG_MIXER1, EN_SPDI2DAC);
}

void CCMIAdapter::resetController()
{
	PAGED_CODE();
	DBGPRINT(("CCMIAdapter[%p]::resetController()", this));

	writeUInt32(REG_INTHLDCLR, 0);

#if OUT_CHANNEL == 1
	writeUInt32(REG_FUNCTRL0, ADC_CH0 | (RST_CH0 | RST_CH1));
	writeUInt32(REG_FUNCTRL0, ADC_CH0 & ~(RST_CH0 | RST_CH1));
#else
	writeUInt32(REG_FUNCTRL0, ADC_CH1 | (RST_CH0 | RST_CH1));
	writeUInt32(REG_FUNCTRL0, ADC_CH1 & ~(RST_CH0 | RST_CH1));
#endif
	KeStallExecutionProcessor(100L);

	writeUInt32(REG_FUNCTRL0, 0);
	writeUInt32(REG_FUNCTRL1, 0);

	writeUInt32(REG_CHFORMAT, 0);
	writeUInt32(REG_MISCCTRL, EN_DBLDAC);
#if OUT_CHANNEL == 1
	setUInt32Bit(REG_MISCCTRL, XCHG_DAC);
#endif

	setUInt32Bit(REG_FUNCTRL1, BREQ);

	writeMixer(0, 0);

	return;
}


STDMETHODIMP_(NTSTATUS) CCMIAdapter::activateMPU(ULONG* MPUBase)
{
	PAGED_CODE();
	DBGPRINT(("CCMIAdapter[%p]::activateMPU(%X)", this, MPUBase));

	UInt32 LegacyCtrl;

	switch ((LONGLONG)MPUBase) {
		case 0x300: LegacyCtrl = UART_300; break;
		case 0x310: LegacyCtrl = UART_310; break;
		case 0x320: LegacyCtrl = UART_320; break;
		case 0x330: LegacyCtrl = UART_330; break; // UART_330 == 0
		default: LegacyCtrl = 0xFFFFFFFF;  break;
	}
	if (LegacyCtrl < 0xFFFFFFFF) {
		cm.MPUBase = MPUBase;
		setUInt32Bit(REG_FUNCTRL1, EN_UART);
		writeUInt32(REG_LEGACY, LegacyCtrl);
		return STATUS_SUCCESS;
	}

	return STATUS_UNSUCCESSFUL;
}

// "The code for this method must reside in paged memory.", IID_IAdapterPowerManagement.PowerChangeState() docs
// XP's order of power states when going to hibernate: D3 -> D0, waking up: D0 -> D3.
STDMETHODIMP_(void) CCMIAdapter::PowerChangeState(POWER_STATE NewState)
{
	PAGED_CODE();
	DBGPRINT(("CCMIAdapter[%p]::PowerChangeState(%p)", this, NewState));

	if (NewState.DeviceState == CurrentPowerState ) {
		return;
	}

	switch (NewState.DeviceState) {
		case PowerDeviceD0: // powering up, hardware access allowed
			clearUInt32Bit(REG_MISCCTRL, PWD_CHIP);
			cm.WaveMiniport->powerUp();
			CurrentPowerState = NewState.DeviceState;
			break;

		case PowerDeviceD1: // powering down, hardware access still allowed
			setUInt32Bit(REG_MISCCTRL, PWD_CHIP);
			CurrentPowerState = NewState.DeviceState;
			break;

		case PowerDeviceD2: // sleep state - hardware access not allowed
		case PowerDeviceD3: // hibernation state - hardware access not allowed
			if (CurrentPowerState == PowerDeviceD0) {
				cm.WaveMiniport->powerDown();
				setUInt32Bit(REG_MISCCTRL, PWD_CHIP);
			}
			CurrentPowerState = NewState.DeviceState;
			break;
		default:            // unknown power state
			break;
	}
}

STDMETHODIMP_(NTSTATUS) CCMIAdapter::QueryPowerChangeState(POWER_STATE NewStateQuery)
{
	PAGED_CODE();
	DBGPRINT(("CCMIAdapter[%p]::QueryPowerChangeState(%p)", this, NewStateQuery));
	return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS) CCMIAdapter::QueryDeviceCapabilities(PDEVICE_CAPABILITIES PowerDeviceCaps)
{
	PAGED_CODE();
	DBGPRINT(("CCMIAdapter[%p]::QueryDeviceCapabilities(%p)", this, PowerDeviceCaps));
	return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS) CCMIAdapter::loadSBMixerFromMemory()
{
	PAGED_CODE();
	DBGPRINT(("CCMIAdapter[%p]::loadSBMixerFromMemory()", this));
	UInt8 sbIndex[] = { 0x04, 0x0A, 0x22, 0x28, 0x2E, 0x30, 0x31, 0x32, 0x33, 0x36, 0x37, 0x38,
	                    0x39, 0x3A, 0x3C, 0x3D, 0x3E, 0xF0 };

	for (int i = 0; i<(sizeof(sbIndex)/sizeof(sbIndex[0]));i++) {
		writeUInt8(REG_SBINDEX, sbIndex[i]);
		writeUInt8(REG_SBDATA, mixerCache[i]);
	}

	return STATUS_SUCCESS;
}

/*
** non-paged code below
*/
#pragma code_seg()

STDMETHODIMP_(UInt8) CCMIAdapter::readUInt8(UInt8 reg)
{
	return READ_PORT_UCHAR((PUCHAR)(reinterpret_cast<PUCHAR>(cm.IOBase) + reg));
}

STDMETHODIMP_(void) CCMIAdapter::writeUInt8(UInt8 cmd, UInt8 value)
{
	WRITE_PORT_UCHAR((PUCHAR)(reinterpret_cast<PUCHAR>(cm.IOBase) + cmd), value);
}

STDMETHODIMP_(void) CCMIAdapter::setUInt8Bit(UInt8 reg, UInt8 flag)
{
	writeUInt8(reg, readUInt8(reg) | flag);
}

STDMETHODIMP_(void) CCMIAdapter::clearUInt8Bit(UInt8 reg, UInt8 flag)
{
	writeUInt8(reg, readUInt8(reg) & ~flag);
}

STDMETHODIMP_(UInt16) CCMIAdapter::readUInt16(UInt8 reg)
{
	return READ_PORT_USHORT((PUSHORT)(reinterpret_cast<PUCHAR>(cm.IOBase) + reg));
}

STDMETHODIMP_(void) CCMIAdapter::writeUInt16(UInt8 cmd, UInt16 value)
{
	WRITE_PORT_USHORT((PUSHORT)(reinterpret_cast<PUCHAR>(cm.IOBase) + cmd), value);
}


STDMETHODIMP_(UInt32) CCMIAdapter::readUInt32(UInt8 reg)
{
	return READ_PORT_ULONG((PULONG)(reinterpret_cast<PUCHAR>(cm.IOBase) + reg));
}

STDMETHODIMP_(void) CCMIAdapter::writeUInt32(UInt8 cmd, UInt32 value)
{
	WRITE_PORT_ULONG((PULONG)(reinterpret_cast<PUCHAR>(cm.IOBase) + cmd), value);
}

STDMETHODIMP_(void) CCMIAdapter::setUInt32Bit(UInt8 reg, UInt32 flag)
{
	writeUInt32(reg, readUInt32(reg) | flag);
}

STDMETHODIMP_(void) CCMIAdapter::clearUInt32Bit(UInt8 reg, UInt32 flag)
{
	writeUInt32(reg, readUInt32(reg) & ~flag);
}

STDMETHODIMP_(UInt8) CCMIAdapter::readMixer(UInt8 index)
{
	if (mixerCache[index] == 0xFF) {
		writeUInt8(REG_SBINDEX, index);
	    mixerCache[index] = readUInt8(REG_SBDATA);
 	}
 	return mixerCache[index];
}

STDMETHODIMP_(void) CCMIAdapter::writeMixer(UInt8 index, UInt8 value)
{
	if (value != mixerCache[index]) {
		mixerCache[index] = value;
		writeUInt8(REG_SBINDEX, index);
		writeUInt8(REG_SBDATA, value);
	}
}

STDMETHODIMP_(void) CCMIAdapter::setMixerBit(UInt8 index, UInt8 flag)
{
	writeMixer(index, readMixer(index) | flag);
}

STDMETHODIMP_(void) CCMIAdapter::clearMixerBit(UInt8 index, UInt8 flag)
{
	writeMixer(index, readMixer(index) & ~flag);
}

NTSTATUS CCMIAdapter::InterruptServiceRoutine(PINTERRUPTSYNC InterruptSync, PVOID DynamicContext)
{
	ASSERT(InterruptSync);
	ASSERT(DynamicContext);

	UInt32 status, mask = 0;

	CCMIAdapter *CMIAdapter = (CCMIAdapter *)DynamicContext;

	if (!(CMIAdapter->cm.WaveMiniport)) {
        	return STATUS_UNSUCCESSFUL;
	}

	status = CMIAdapter->readUInt32(REG_INT_STAT);

	if ((!(status & INT_PENDING)) || (status == 0xFFFFFFFF)) {
		return STATUS_UNSUCCESSFUL;
	}

	if (status & INT_CH0) {
		mask |= EN_CH0_INT;
#if OUT_CHANNEL == 0
		CMIAdapter->cm.WaveMiniport->ServiceWaveISR(PCM_OUT_STREAM);
#endif
#if IN_CHANNEL == 0
		CMIAdapter->cm.WaveMiniport->ServiceWaveISR(PCM_IN_STREAM);
#endif
	}
	if (status & INT_CH1) {
		mask |= EN_CH1_INT;
#if OUT_CHANNEL == 1
		CMIAdapter->cm.WaveMiniport->ServiceWaveISR(PCM_OUT_STREAM);
#endif
#if IN_CHANNEL == 1
		CMIAdapter->cm.WaveMiniport->ServiceWaveISR(PCM_IN_STREAM);
#endif
	}
#ifdef UART
	if (status & INT_UART) {
		// the UART miniport should catch / have caught the interrupt
		return STATUS_UNSUCCESSFUL;
	}
#endif

	CMIAdapter->clearUInt32Bit(REG_INTHLDCLR, mask);
	CMIAdapter->setUInt32Bit(REG_INTHLDCLR, mask);

	return STATUS_SUCCESS;
}
