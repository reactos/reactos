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

#include "limits.h"
#include "mintopo.hpp"
#include "mintopotables.hpp"
#define NTSTRSAFE_LIB //for Windows 2000 compatibility
#include "NtStrsafe.h"

#pragma code_seg("PAGE")

NTSTATUS CreateMiniportTopologyCMI(PUNKNOWN* Unknown, REFCLSID, PUNKNOWN UnknownOuter, POOL_TYPE PoolType)
{
	//PAGED_CODE();
	//ASSERT(Unknown);
	STD_CREATE_BODY_(CCMITopology,Unknown,UnknownOuter,PoolType,PMINIPORTTOPOLOGY);
}

STDMETHODIMP CCMITopology::NonDelegatingQueryInterface(REFIID Interface, PVOID* Object)
{
	//PAGED_CODE();
	//ASSERT(Object);
	DBGPRINT(("CCMITopology::NonDelegatingQueryInterface"));

	if (IsEqualGUIDAligned(Interface, IID_IUnknown)) {
		*Object = PVOID(PUNKNOWN(PMINIPORTTOPOLOGY(this)));
	} else if (IsEqualGUIDAligned(Interface,IID_IMiniport)) {
		*Object = PVOID(PMINIPORT(this));
	} else if (IsEqualGUIDAligned(Interface,IID_IMiniportTopology)) {
		*Object = PVOID(PMINIPORTTOPOLOGY(this));
	} else if (IsEqualGUIDAligned (Interface, IID_ICMITopolgy)) {
		*Object = (PVOID)(PMINIPORTTOPOLOGY)this;
	} else {
		*Object = NULL;
	}

	if (*Object) {
		PUNKNOWN(*Object)->AddRef();
		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_PARAMETER;
}

CCMITopology::~CCMITopology()
{
	//PAGED_CODE();

	DBGPRINT(("CCMITopology::~CCMITopology"));

	storeMixerSettingsToRegistry(); //or not. during system shutdown, this doesn't seem to work.
	cm->TopoMiniport = NULL;

	if (CMIAdapter) {
		CMIAdapter->Release();
	}
}

STDMETHODIMP CCMITopology::Init(PUNKNOWN UnknownAdapter, PRESOURCELIST ResourceList, PPORTTOPOLOGY Port)
{
	//PAGED_CODE();
	//ASSERT(UnknownAdapter);
	//ASSERT(Port);
	DBGPRINT(("CCMITopology::Init"));

	NTSTATUS ntStatus = UnknownAdapter->QueryInterface(IID_ICMIAdapter, (PVOID *)&CMIAdapter);

	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("UnknownAdapter->QueryInterface() failed"));
		return STATUS_INVALID_PARAMETER;
	}

	CMIAdapter->resetMixer();
	auxVolumeRegister = CMIAdapter->readUInt8(REG_MIXER3);
	micVolumeRegister = CMIAdapter->readUInt8(REG_MIXER2);
	functrl1Register = 0;
	chformatRegister = 0;
	legacyRegister   = 0;
	miscctrlRegister = 0;

	cm = CMIAdapter->getCMI8738Info();
	cm->TopoMiniport = this; //this is not really nice
	loadMixerSettingsFromRegistry();

	return STATUS_SUCCESS;
}

STDMETHODIMP CCMITopology::GetDescription(PPCFILTER_DESCRIPTOR*  OutFilterDescriptor)
{
    //PAGED_CODE();
    //ASSERT(OutFilterDescriptor);
    DBGPRINT(("CCMITopology::GetDescription"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}

STDMETHODIMP CCMITopology::loadMixerSettingsFromRegistry()
{
	//PAGED_CODE();
	DBGPRINT(("CCMITopology::loadMixerSettingsFromRegistry"));

	PREGISTRYKEY       DriverKey;
	PREGISTRYKEY       SettingsKey;
	UNICODE_STRING     KeyName;
	PCPROPERTY_REQUEST PropertyRequest;
	PCPROPERTY_ITEM    PropertyItem;
	DWORD              Channel;
	PVOID              KeyInfo;
	ULONG              ResultLength;
	WCHAR              buffer[128];

	if ((!CMIAdapter) || (!(CMIAdapter->getDeviceObject()))) {
		DBGPRINT(("CMIAdapter->getDeviceObject() failed"));
		return STATUS_UNSUCCESSFUL;
	}

	settingsLoaded = FALSE;

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

	KeyInfo = ExAllocatePoolWithTag(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD), 'dbrt');
	if(KeyInfo == NULL) {
		DBGPRINT(("ExAllocatePoolWithTag() failed"));
		return STATUS_UNSUCCESSFUL;
	}

	PropertyRequest.MajorTarget  = this;
	PropertyRequest.Verb         = KSPROPERTY_TYPE_SET;
	PropertyRequest.Instance     = &Channel;
	PropertyRequest.InstanceSize = sizeof(DWORD);
	PropertyRequest.Value        = &(PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo)->Data);
	PropertyRequest.ValueSize    = sizeof(DWORD);
	PropertyRequest.PropertyItem = &PropertyItem;

	for (int i=0;i < SIZEOF_ARRAY(TopologyNodes); i++) {
		PropertyRequest.Node = i;

		Channel = CHAN_LEFT;
		ntStatus = RtlStringCbPrintfW(buffer, sizeof(buffer), L"Node%dLeft", i);
		if (!NT_SUCCESS(ntStatus)) {
			DBGPRINT(("RtlStringCbPrintfW() failed"));
		}
		RtlInitUnicodeString(&KeyName, buffer);
		ntStatus = SettingsKey->QueryValueKey(&KeyName, KeyValuePartialInformation, KeyInfo, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD), &ResultLength);
		if (NT_SUCCESS(ntStatus)) {
			if(PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo)->DataLength == sizeof(DWORD)) {
				if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_VOLUME)) {
					PropertyItem.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;
					PropertyHandler_Level(&PropertyRequest);
				}
				if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_MUTE)) {
					PropertyItem.Id = KSPROPERTY_AUDIO_MUTE;
					PropertyHandler_OnOff(&PropertyRequest);
				}
				if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_LOUDNESS)) {
					PropertyItem.Id = KSPROPERTY_AUDIO_LOUDNESS;
					PropertyHandler_OnOff(&PropertyRequest);
				}
			}
		} else {
			// default values
			if (i == KSNODE_TOPO_IEC_OUT) {
				PropertyItem.Id = KSPROPERTY_AUDIO_LOUDNESS;
				*(PBOOL(PropertyRequest.Value)) = true;
				PropertyHandler_OnOff(&PropertyRequest);
			}
			if (i == KSNODE_TOPO_WAVEOUT_MUTE_IN) {
				PropertyItem.Id = KSPROPERTY_AUDIO_MUTE;
				*(PBOOL(PropertyRequest.Value)) = true;
				PropertyHandler_OnOff(&PropertyRequest);
			}
			if (i == KSNODE_TOPO_CENTER2MIC) {
				PropertyItem.Id = KSPROPERTY_AUDIO_LOUDNESS;
				*(PBOOL(PropertyRequest.Value)) = false;
				PropertyHandler_OnOff(&PropertyRequest);
			}
		}

		Channel = CHAN_RIGHT;
		ntStatus = RtlStringCbPrintfW(buffer, sizeof(buffer), L"Node%dRight", i);
		if (!NT_SUCCESS(ntStatus)) {
			DBGPRINT(("RtlStringCbPrintfW() failed"));
		}
		RtlInitUnicodeString(&KeyName, buffer);
		ntStatus = SettingsKey->QueryValueKey(&KeyName, KeyValuePartialInformation, KeyInfo, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD), &ResultLength);
		if (NT_SUCCESS(ntStatus)) {
			if(PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo)->DataLength == sizeof(DWORD)) {
				if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_VOLUME)) {
					PropertyItem.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;
					PropertyHandler_Level(&PropertyRequest);
				}
				if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_MUTE)) {
					PropertyItem.Id = KSPROPERTY_AUDIO_MUTE;
					PropertyHandler_OnOff(&PropertyRequest);
				}
				if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_LOUDNESS)) {
					PropertyItem.Id = KSPROPERTY_AUDIO_LOUDNESS;
					PropertyHandler_OnOff(&PropertyRequest);
				}
			}
		} else {
			if (i == KSNODE_TOPO_WAVEOUT_MUTE_IN) {
				PropertyItem.Id = KSPROPERTY_AUDIO_MUTE;
				*(PBOOL(PropertyRequest.Value)) = true;
				PropertyHandler_OnOff(&PropertyRequest);
			}
		}

	}
	RtlInitUnicodeString(&KeyName, L"FormatMask");
	ntStatus = SettingsKey->QueryValueKey(&KeyName, KeyValuePartialInformation, KeyInfo, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD), &ResultLength);
	if (NT_SUCCESS (ntStatus)) {
       	PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;
		if (PartialInfo->DataLength == sizeof(DWORD)) {
			cm->formatMask = (*(PLONG)PartialInfo->Data);
		}
	} else {
		cm->formatMask = 0xFFFFFFFF;
	}

	ExFreePoolWithTag (KeyInfo,'dbrt');

	SettingsKey->Release();
	DriverKey->Release();

	settingsLoaded = TRUE;

    return STATUS_SUCCESS;
}

STDMETHODIMP CCMITopology::storeMixerSettingsToRegistry()
{
	//PAGED_CODE();
	DBGPRINT(("CCMITopology::storeMixerSettingsToRegistry"));

	PREGISTRYKEY       DriverKey;
	PREGISTRYKEY       SettingsKey;
	UNICODE_STRING     KeyName;
	PCPROPERTY_REQUEST PropertyRequest;
	PCPROPERTY_ITEM    PropertyItem;
	DWORD              Value,Channel;
	WCHAR              buffer[128];

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

	PropertyRequest.MajorTarget  = this;
	PropertyRequest.Verb         = KSPROPERTY_TYPE_GET;
	PropertyRequest.Instance     = &Channel;
	PropertyRequest.InstanceSize = sizeof(DWORD);
	PropertyRequest.Value        = &Value;
	PropertyRequest.ValueSize    = sizeof(DWORD);
	PropertyRequest.PropertyItem = &PropertyItem;

	for (int i=0;i < SIZEOF_ARRAY(TopologyNodes); i++) {
		PropertyRequest.Node = i;
		if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_VOLUME)) {
			PropertyRequest.Node = i;
			PropertyItem.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;

			ntStatus = RtlStringCbPrintfW(buffer, sizeof(buffer), L"Node%dLeft", i);
			if (!NT_SUCCESS(ntStatus)) {
				DBGPRINT(("RtlStringCbPrintfW() failed"));
			}
		    RtlInitUnicodeString(&KeyName, buffer);
			Channel = CHAN_LEFT;
			ntStatus = PropertyHandler_Level(&PropertyRequest);
			if (NT_SUCCESS(ntStatus)) {
				ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
				if(!NT_SUCCESS(ntStatus)) {
					DBGPRINT(("SetValueKey() failed"));
					break;
				}
			}

			ntStatus = RtlStringCbPrintfW(buffer, sizeof(buffer), L"Node%dRight", i);
			if (!NT_SUCCESS(ntStatus)) {
				DBGPRINT(("RtlStringCbPrintfW() failed"));
			}
		    RtlInitUnicodeString(&KeyName, buffer);
			Channel = CHAN_RIGHT;
			ntStatus = PropertyHandler_Level(&PropertyRequest);
			if (NT_SUCCESS(ntStatus)) {
				ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
				if(!NT_SUCCESS(ntStatus)) {
					DBGPRINT(("SetValueKey() failed"));
					break;
				}
			}

		}
		if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_MUTE)) {
			PropertyItem.Id = KSPROPERTY_AUDIO_MUTE;
			PropertyHandler_OnOff(&PropertyRequest);

			ntStatus = RtlStringCbPrintfW(buffer, sizeof(buffer), L"Node%dLeft", i);
			if (!NT_SUCCESS(ntStatus)) {
				DBGPRINT(("RtlStringCbPrintfW() failed"));
			}
		    RtlInitUnicodeString(&KeyName, buffer);
			Channel = CHAN_LEFT;
			ntStatus = PropertyHandler_OnOff(&PropertyRequest);
			if (NT_SUCCESS(ntStatus)) {
				ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
				if(!NT_SUCCESS(ntStatus)) {
					DBGPRINT(("SetValueKey() failed"));
					break;
				}
			}

			ntStatus = RtlStringCbPrintfW(buffer, sizeof(buffer), L"Node%dRight", i);
			if (!NT_SUCCESS(ntStatus)) {
				DBGPRINT(("RtlStringCbPrintfW() failed"));
			}
		    RtlInitUnicodeString(&KeyName, buffer);
			Channel = CHAN_RIGHT;
			ntStatus = PropertyHandler_OnOff(&PropertyRequest);
			if (NT_SUCCESS(ntStatus)) {
				ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
				if(!NT_SUCCESS(ntStatus)) {
					DBGPRINT(("SetValueKey() failed"));
					break;
				}
			}
		}
		if (IsEqualGUIDAligned(*(TopologyNodes[i].Type), KSNODETYPE_LOUDNESS)) {
			PropertyItem.Id = KSPROPERTY_AUDIO_LOUDNESS;
			PropertyHandler_OnOff(&PropertyRequest);

			ntStatus = RtlStringCbPrintfW(buffer, sizeof(buffer), L"Node%dLeft", i);
			if (!NT_SUCCESS(ntStatus)) {
				DBGPRINT(("RtlStringCbPrintfW() failed"));
			}
		    RtlInitUnicodeString(&KeyName, buffer);
			Channel = CHAN_LEFT;
			ntStatus = PropertyHandler_OnOff(&PropertyRequest);
			if (NT_SUCCESS(ntStatus)) {
				ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
				if(!NT_SUCCESS(ntStatus)) {
					DBGPRINT(("SetValueKey() failed"));
					break;
				}
			}

			ntStatus = RtlStringCbPrintfW(buffer, sizeof(buffer), L"Node%dRight", i);
			if (!NT_SUCCESS(ntStatus)) {
				DBGPRINT(("RtlStringCbPrintfW() failed"));
			}
		    RtlInitUnicodeString(&KeyName, buffer);
			Channel = CHAN_RIGHT;
			ntStatus = PropertyHandler_OnOff(&PropertyRequest);
			if (NT_SUCCESS(ntStatus)) {
				ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
				if(!NT_SUCCESS(ntStatus)) {
					DBGPRINT(("SetValueKey() failed"));
					break;
				}
			}
		}
	}
	Value = cm->formatMask;
	RtlInitUnicodeString(&KeyName, L"FormatMask");
	ntStatus = SettingsKey->SetValueKey(&KeyName, REG_DWORD, PVOID(&Value), sizeof(DWORD));
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("SetValueKey() failed"));
	}

	SettingsKey->Release();
	DriverKey->Release();

    return STATUS_SUCCESS;
}

STDMETHODIMP CCMITopology::loadMixerSettingsFromMemory()
{
	//PAGED_CODE();
	DBGPRINT(("CCMITopology::loadMixerSettingsFromMemory"));

	CMIAdapter->resetMixer();
	CMIAdapter->loadSBMixerFromMemory();
	CMIAdapter->writeUInt8(REG_MIXER1, mixer1Register);
	CMIAdapter->writeUInt8(REG_MIXER2, auxVolumeRegister);
	CMIAdapter->writeUInt8(REG_MIXER3, micVolumeRegister);
	CMIAdapter->writeUInt8(REG_MIXER4, mixer4Register);

	CMIAdapter->setUInt32Bit(REG_FUNCTRL1, functrl1Register);
	CMIAdapter->setUInt32Bit(REG_CHFORMAT, chformatRegister);
	CMIAdapter->setUInt32Bit(REG_LEGACY,   legacyRegister);
	CMIAdapter->setUInt32Bit(REG_MISCCTRL, miscctrlRegister);

	return STATUS_SUCCESS;
}

STDMETHODIMP CCMITopology::storeMixerSettingsToMemory()
{
	//PAGED_CODE();
	DBGPRINT(("CCMITopology::storeMixerSettingsToMemory"));

	mixer1Register   = CMIAdapter->readUInt8(REG_MIXER1);
	mixer4Register   = CMIAdapter->readUInt8(REG_MIXER4);
	functrl1Register = CMIAdapter->readUInt32(REG_FUNCTRL1) & LOOP_SPDF ;
	chformatRegister = CMIAdapter->readUInt32(REG_CHFORMAT) & (INV_SPDIFI1 | SEL_SPDIFI1 | POLVALID);
    legacyRegister   = CMIAdapter->readUInt32(REG_LEGACY) & (BASS2LINE | CENTER2LINE | EN_SPDCOPYRHT);
    miscctrlRegister = CMIAdapter->readUInt32(REG_MISCCTRL) & (EN_SPDO5V | SEL_SPDIFI2);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PropertyHandler_OnOff(PPCPROPERTY_REQUEST PropertyRequest)
{
	//PAGED_CODE();
	//ASSERT(PropertyRequest);
	DBGPRINT(("[PropertyHandler_OnOff]"));

	CCMITopology *that = (CCMITopology *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

	NTSTATUS  ntStatus = STATUS_INVALID_PARAMETER;
	UInt8     data, mask, reg;
	LONG      channel;

	if (PropertyRequest->Node == ULONG(-1)) {
		return ntStatus;
	}

	if ( ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) || (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)) && (PropertyRequest->InstanceSize >= sizeof(LONG)) ) {
		channel = *(PLONG(PropertyRequest->Instance));
		if (PropertyRequest->ValueSize >= sizeof(BOOL)) {

			if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE) {
				PBOOL Muted = PBOOL(PropertyRequest->Value);
				switch (PropertyRequest->Node) {

					case KSNODE_TOPO_WAVEOUT_MUTE:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = (that->CMIAdapter->readUInt8(REG_MIXER1) & MUTE_WAVE);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->CMIAdapter->setUInt8Bit(REG_MIXER1, MUTE_WAVE);
							} else {
								that->CMIAdapter->clearUInt8Bit(REG_MIXER1, MUTE_WAVE);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_AUX_MUTE:
						switch (channel) {
							case CHAN_LEFT:  mask = MUTE_AUX_L; break;
							case CHAN_RIGHT: mask = MUTE_AUX_R; break;
							default: return ntStatus;
						}

						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = !(that->micVolumeRegister & mask);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->micVolumeRegister &= ~mask;
							} else {
								that->micVolumeRegister |= mask;
							}
							that->CMIAdapter->writeUInt8(REG_MIXER2, that->micVolumeRegister);
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_MICOUT_MUTE:
						if (channel != CHAN_LEFT) {
							return ntStatus;
						}

						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = !(that->CMIAdapter->readMixer(SBREG_OUTPUTCTRL) & EN_MIC);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->CMIAdapter->clearMixerBit(SBREG_OUTPUTCTRL, EN_MIC);
							} else {
								that->CMIAdapter->setMixerBit(SBREG_OUTPUTCTRL, EN_MIC);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_CD_MUTE:
						switch (channel) {
							case CHAN_LEFT:  mask = EN_CD_L; break;
							case CHAN_RIGHT: mask = EN_CD_R; break;
							default: return ntStatus;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = !(that->CMIAdapter->readMixer(SBREG_OUTPUTCTRL) & mask);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->CMIAdapter->clearMixerBit(SBREG_OUTPUTCTRL, mask);
							} else {
								that->CMIAdapter->setMixerBit(SBREG_OUTPUTCTRL, mask);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_LINEIN_MUTE:
						switch (channel) {
							case CHAN_LEFT:  mask = EN_LINEIN_L; break;
							case CHAN_RIGHT: mask = EN_LINEIN_R; break;
							default: return ntStatus;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = !(that->CMIAdapter->readMixer(SBREG_OUTPUTCTRL) & mask);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->CMIAdapter->clearMixerBit(SBREG_OUTPUTCTRL, mask);
							} else {
								that->CMIAdapter->setMixerBit(SBREG_OUTPUTCTRL, mask);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_MIC_MUTE_IN:
						if (channel != CHAN_LEFT) {
							return ntStatus;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = !(that->CMIAdapter->readMixer(SBREG_IN_CTRL_L) & EN_MIC);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->CMIAdapter->clearMixerBit(SBREG_IN_CTRL_L, EN_MIC);
								that->CMIAdapter->clearMixerBit(SBREG_IN_CTRL_R, EN_MIC);
							} else {
								that->CMIAdapter->setMixerBit(SBREG_IN_CTRL_L, EN_MIC);
								that->CMIAdapter->setMixerBit(SBREG_IN_CTRL_R, EN_MIC);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_CD_MUTE_IN:
						switch (channel) {
							case CHAN_LEFT:  mask = EN_LINEIN_L; reg = EN_CD_L; break;
							case CHAN_RIGHT: mask = EN_LINEIN_R; reg = EN_CD_R; break;
							default: return ntStatus;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = !(that->CMIAdapter->readMixer(reg) & mask);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->CMIAdapter->clearMixerBit(reg, mask);
							} else {
								that->CMIAdapter->setMixerBit(reg, mask);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_LINEIN_MUTE_IN:
						switch (channel) {
							case CHAN_LEFT:  mask = EN_LINEIN_L; reg = SBREG_IN_CTRL_L; break;
							case CHAN_RIGHT: mask = EN_LINEIN_R; reg = SBREG_IN_CTRL_R; break;
							default: return ntStatus;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = !(that->CMIAdapter->readMixer(reg) & mask);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->CMIAdapter->clearMixerBit(reg, mask);
							} else {
								that->CMIAdapter->setMixerBit(reg, mask);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;


					case KSNODE_TOPO_AUX_MUTE_IN:
						switch (channel) {
							case CHAN_LEFT:  mask = MUTE_RAUX_L; break;
							case CHAN_RIGHT: mask = MUTE_RAUX_R; break;
							default: return ntStatus;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = (that->micVolumeRegister & mask) ;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->micVolumeRegister |= mask;
							} else {
								that->micVolumeRegister &= ~mask;
							}
							that->CMIAdapter->writeUInt8(REG_MIXER2, that->micVolumeRegister);
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_WAVEOUT_MUTE_IN:
						switch (channel) {
							case CHAN_LEFT:  mask = EN_WAVEIN_L; break;
							case CHAN_RIGHT: mask = EN_WAVEIN_R; break;
							default: return ntStatus;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
//							*Muted = !(that->CMIAdapter->readUInt8(REG_MIXER1) & mask);
							*Muted = !(that->cm->enableSPDIFIn);
							ntStatus = STATUS_SUCCESS;
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded)) {
							if (*Muted) {
								that->cm->enableSPDIFIn = FALSE;
								that->CMIAdapter->clearUInt8Bit(REG_MIXER1, mask);
							} else {
								that->cm->enableSPDIFIn = TRUE;
								that->CMIAdapter->setUInt8Bit(REG_MIXER1, mask);
							}
							ntStatus = STATUS_SUCCESS;
						}
						break;

					case KSNODE_TOPO_MASTER_MUTE_DUMMY:
						channel = (1 << channel);
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*Muted = that->masterMuteDummy & channel;
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*Muted) {
								that->masterMuteDummy |= channel;
							} else {
								that->masterMuteDummy &= ~channel;
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;
				}
			}

			if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_LOUDNESS) {
				PBOOL LoudnessOn = PBOOL(PropertyRequest->Value);
				switch  (PropertyRequest->Node) {
					case KSNODE_TOPO_MICIN_LOUDNESS:
						if (channel == CHAN_LEFT) {
							if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
								*LoudnessOn = (that->CMIAdapter->readMixer(SBREG_EXTENSION) & EN_MICBOOST);
							}
							if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
//								DBGPRINT(("setting mic boost: previous state %d, new state %d", (that->CMIAdapter->readMixer(SBREG_EXTENSION) & EN_MICBOOST), (*LoudnessOn)));
								if (*LoudnessOn) {
									that->CMIAdapter->setMixerBit(SBREG_EXTENSION, EN_MICBOOST);
								} else {
									that->CMIAdapter->clearMixerBit(SBREG_EXTENSION, EN_MICBOOST);
								}
							}
							ntStatus = STATUS_SUCCESS;
						}
						break;
					case KSNODE_TOPO_MICOUT_LOUDNESS:
						if (channel == CHAN_LEFT) {
							if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
								*LoudnessOn = !(that->CMIAdapter->readUInt8(REG_MIXER2) & DIS_MICGAIN);
							}
							if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
								if (*LoudnessOn) {
									that->CMIAdapter->clearUInt8Bit(REG_MIXER2, DIS_MICGAIN);
								} else {
									that->CMIAdapter->setUInt8Bit(REG_MIXER2, DIS_MICGAIN);
								}
							}
							ntStatus = STATUS_SUCCESS;
						}
						break;
					case KSNODE_TOPO_IEC_5V:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt32(REG_MISCCTRL) & EN_SPDO5V);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt32Bit(REG_MISCCTRL, EN_SPDO5V);
							} else {
								that->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, EN_SPDO5V);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;
					case KSNODE_TOPO_IEC_OUT:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							if (that->cm) {
								*LoudnessOn = that->cm->enableSPDIFOut;
							}
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (that->cm) {
								that->cm->enableSPDIFOut = (*LoudnessOn);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_IEC_INVERSE:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							if (that->cm->chipVersion <= 37) {
								*LoudnessOn = (that->CMIAdapter->readUInt8(REG_MIXER4) & INV_SPDIFI1);
							} else {
								*LoudnessOn = (that->CMIAdapter->readUInt32(REG_CHFORMAT) & INV_SPDIFI2);
							}
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*LoudnessOn) {
								if (that->cm->chipVersion <= 37) {
									that->CMIAdapter->setUInt8Bit(REG_MIXER4, INV_SPDIFI1);
								} else {
									that->CMIAdapter->setUInt32Bit(REG_CHFORMAT, INV_SPDIFI2);
								}
							} else {
								if (that->cm->chipVersion <= 37) {
									that->CMIAdapter->clearUInt8Bit(REG_MIXER4, INV_SPDIFI1);
								} else {
									that->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, INV_SPDIFI2);
								}
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_IEC_MONITOR:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt8(REG_MIXER1) & EN_SPDI2DAC);
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt8Bit(REG_MIXER1, EN_SPDI2DAC);
							} else {
								that->CMIAdapter->clearUInt8Bit(REG_MIXER1, EN_SPDI2DAC);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_IEC_SELECT:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							if (that->cm->chipVersion <= 37) {
								*LoudnessOn = (that->CMIAdapter->readUInt32(REG_CHFORMAT) & SEL_SPDIFI1);
							} else {
								*LoudnessOn = (that->CMIAdapter->readUInt32(REG_MISCCTRL) & SEL_SPDIFI2);
							}
						}
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
							if (*LoudnessOn) {
								if (that->cm->chipVersion <= 37) {
									that->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SEL_SPDIFI1);
								} else {
									that->CMIAdapter->setUInt32Bit(REG_MISCCTRL, SEL_SPDIFI2);
								}
							} else {
								if (that->cm->chipVersion <= 37) {
									that->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SEL_SPDIFI1);
								} else {
									that->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, SEL_SPDIFI2);
								}
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_XCHG_FB:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt8(REG_MIXER1) & REAR2FRONT);
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded)) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt8Bit(REG_MIXER1, REAR2FRONT);
							} else {
								that->CMIAdapter->clearUInt8Bit(REG_MIXER1, REAR2FRONT);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_BASS2LINE:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt32(REG_LEGACY) & BASS2LINE);
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded)) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt32Bit(REG_LEGACY, BASS2LINE);
							} else {
								that->CMIAdapter->clearUInt32Bit(REG_LEGACY, BASS2LINE);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_CENTER2LINE:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt32(REG_LEGACY) & CENTER2LINE);
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded)) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt32Bit(REG_LEGACY, CENTER2LINE);
							} else {
								that->CMIAdapter->clearUInt32Bit(REG_LEGACY, CENTER2LINE);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_IEC_COPYRIGHT:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt32(REG_LEGACY) & EN_SPDCOPYRHT);
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded)) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt32Bit(REG_LEGACY, EN_SPDCOPYRHT);
							} else {
								that->CMIAdapter->clearUInt32Bit(REG_LEGACY, EN_SPDCOPYRHT);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_IEC_POLVALID:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt32(REG_CHFORMAT) & POLVALID);
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded)) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt32Bit(REG_CHFORMAT, POLVALID);
							} else {
								that->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, POLVALID);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_IEC_LOOP:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt32(REG_FUNCTRL1) & LOOP_SPDF);
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded)) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt32Bit(REG_FUNCTRL1, LOOP_SPDF);
							} else {
								that->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, LOOP_SPDF);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_REAR2LINE:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt8(REG_MIXER1) & REAR2LINE);
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded)) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt8Bit(REG_MIXER1, REAR2LINE);
							} else {
								that->CMIAdapter->clearUInt8Bit(REG_MIXER1, REAR2LINE);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;

					case KSNODE_TOPO_CENTER2MIC:
						if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
							*LoudnessOn = (that->CMIAdapter->readUInt8(REG_MIXER4) & CENTER2MIC) && (that->cm->chipVersion > 37);
						}
						if ((PropertyRequest->Verb & KSPROPERTY_TYPE_SET) && !(that->settingsLoaded) && (that->cm->chipVersion > 37)) {
							if (*LoudnessOn) {
								that->CMIAdapter->setUInt8Bit(REG_MIXER4, CENTER2MIC);
							} else {
								that->CMIAdapter->clearUInt8Bit(REG_MIXER4, CENTER2MIC);
							}
						}
						ntStatus = STATUS_SUCCESS;
						break;
				}
			}

			if ((NT_SUCCESS(ntStatus)) && (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)) {
				PropertyRequest->ValueSize = sizeof(BOOL);
			}

		}
	} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
		bool supported = false;
		if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE) {
			switch (PropertyRequest->Node) {
				case KSNODE_TOPO_CD_MUTE:
				case KSNODE_TOPO_LINEIN_MUTE:
				case KSNODE_TOPO_MICOUT_MUTE:
				case KSNODE_TOPO_AUX_MUTE:
				case KSNODE_TOPO_WAVEOUT_MUTE:
				case KSNODE_TOPO_LINEIN_MUTE_IN:
				case KSNODE_TOPO_MIC_MUTE_IN:
				case KSNODE_TOPO_CD_MUTE_IN:
				case KSNODE_TOPO_AUX_MUTE_IN:
				case KSNODE_TOPO_WAVEOUT_MUTE_IN:
				case KSNODE_TOPO_MASTER_MUTE_DUMMY:
					supported = true;
			}
		}

		if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_LOUDNESS) {
			switch (PropertyRequest->Node) {
				case KSNODE_TOPO_MICIN_LOUDNESS:
				case KSNODE_TOPO_MICOUT_LOUDNESS:
				case KSNODE_TOPO_IEC_5V:
				case KSNODE_TOPO_IEC_OUT:
				case KSNODE_TOPO_IEC_INVERSE:
				case KSNODE_TOPO_IEC_MONITOR:
				case KSNODE_TOPO_IEC_SELECT:
				case KSNODE_TOPO_XCHG_FB:
				case KSNODE_TOPO_BASS2LINE:
				case KSNODE_TOPO_CENTER2LINE:
				case KSNODE_TOPO_IEC_COPYRIGHT:
				case KSNODE_TOPO_IEC_POLVALID:
				case KSNODE_TOPO_IEC_LOOP:
				case KSNODE_TOPO_REAR2LINE:
					supported = true;
			}
			if ((PropertyRequest->Node == KSNODE_TOPO_CENTER2MIC) && (that->cm->chipVersion > 37)) {
				supported = true;
			}
		}

		if (supported) {
			if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION))) {
				PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

				PropDesc->AccessFlags	   = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;
				PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
				PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
				PropDesc->PropTypeSet.Id	= VT_BOOL;
				PropDesc->PropTypeSet.Flags = 0;
				PropDesc->MembersListCount  = 0;
				PropDesc->Reserved		  = 0;

				PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
				ntStatus = STATUS_SUCCESS;
			} else if (PropertyRequest->ValueSize >= sizeof(ULONG)) {
				PULONG AccessFlags = PULONG(PropertyRequest->Value);

				*AccessFlags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;

				PropertyRequest->ValueSize = sizeof(ULONG);
				ntStatus = STATUS_SUCCESS;
			}
		}
	}

	return ntStatus;
}

static NTSTATUS BasicSupportHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
	//PAGED_CODE();
	//ASSERT(PropertyRequest);
	DBGPRINT(("[BasicSupportHandler]"));

	NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

	if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION))) {
		PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

		PropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
		PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_STEPPING_LONG);
		PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
		PropDesc->PropTypeSet.Id	= VT_I4;
		PropDesc->PropTypeSet.Flags = 0;
		PropDesc->MembersListCount  = 1;
		PropDesc->Reserved		  = 0;

		if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_STEPPING_LONG))) {
			PKSPROPERTY_MEMBERSHEADER Members = PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

			Members->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
			Members->MembersSize  = sizeof(KSPROPERTY_STEPPING_LONG);
			Members->MembersCount = 1;
			Members->Flags        = 0;

			PKSPROPERTY_STEPPING_LONG Range = PKSPROPERTY_STEPPING_LONG(Members + 1);

			for (int i=0;i<SIZEOF_ARRAY(VolTable);i++) {
				if (VolTable[i].node == PropertyRequest->Node) {
					Range->Bounds.SignedMaximum = (VolTable[i].max << 16);
					Range->Bounds.SignedMinimum = (VolTable[i].min << 16);
					Range->SteppingDelta        = (VolTable[i].step << 16);
					ntStatus = STATUS_SUCCESS;
				}
			}
			if (!NT_SUCCESS(ntStatus)) {
				switch (PropertyRequest->Node) {
					case KSNODE_TOPO_AUX_VOLUME:
						Range->Bounds.SignedMaximum = 0;
						Range->Bounds.SignedMinimum = (-60 << 16);
						Range->SteppingDelta        = (4 << 16);
						ntStatus = STATUS_SUCCESS;
						break;
					case KSNODE_TOPO_MICIN_VOLUME:
						Range->Bounds.SignedMaximum = 0;
						Range->Bounds.SignedMinimum = (-56 << 16);
						Range->SteppingDelta        = (8 << 16);
						ntStatus = STATUS_SUCCESS;
						break;
				}
			}
			Range->Reserved	= 0;

			PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_STEPPING_LONG);
		} else {
			PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
			ntStatus = STATUS_SUCCESS;
		}
	} else if (PropertyRequest->ValueSize >= sizeof(ULONG)) {
		PULONG AccessFlags = PULONG(PropertyRequest->Value);
		*AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
		PropertyRequest->ValueSize = sizeof(ULONG);
		ntStatus = STATUS_SUCCESS;
	}

	return ntStatus;
}

NTSTATUS NTAPI PropertyHandler_Level(PPCPROPERTY_REQUEST PropertyRequest)
{
	//PAGED_CODE();
	//ASSERT(PropertyRequest);
	DBGPRINT(("[PropertyHandler_Level]"));

	CCMITopology *that = (CCMITopology *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);
	NTSTATUS     ntStatus = STATUS_INVALID_PARAMETER;
	UInt32       channel;
	UInt8        mixerValue;

	if ((PropertyRequest->Node == ULONG(-1)) || (PropertyRequest->Node >= KSNODE_TOPO_INVALID) || (PropertyRequest->PropertyItem->Id != KSPROPERTY_AUDIO_VOLUMELEVEL)) {
		return ntStatus;
	}

	if ( ((PropertyRequest->Verb & KSPROPERTY_TYPE_GET) || (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)) && (PropertyRequest->InstanceSize >= sizeof(LONG)) ) {

		channel = *(PLONG(PropertyRequest->Instance));

		if ((PropertyRequest->Node == KSNODE_TOPO_MICOUT_VOLUME) && (channel != CHAN_LEFT)) {
			return STATUS_INVALID_PARAMETER;
		}

		if ( ( (channel == CHAN_LEFT) || (channel == CHAN_RIGHT) ) && (PropertyRequest->ValueSize >= sizeof(LONG))) {

			PLONG Level = (PLONG)PropertyRequest->Value;

			for (int i=0;i<SIZEOF_ARRAY(VolTable);i++)
			{
				if (VolTable[i].node == PropertyRequest->Node) {
					if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {

						mixerValue = (that->CMIAdapter->readMixer(VolTable[i].reg+channel) >> VolTable[i].shift) & VolTable[i].mask;
						*Level     = that->NodeCache[(2*PropertyRequest->Node)+channel];

						if (mixerValue != ((*Level >> (VolTable[i].dbshift+16))+VolTable[i].mask)) {
							*Level = (mixerValue - VolTable[i].mask) << (16+VolTable[i].dbshift);
							that->NodeCache[(2*PropertyRequest->Node)+channel] = *Level;
						}
					} else
					if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
						if (*Level <= (VolTable[i].min << 16)) {
							mixerValue = 0;
							that->NodeCache[(2*PropertyRequest->Node)+channel] = VolTable[i].min << 16;
						} else
						if (*Level >= (VolTable[i].max << 16)) {
							mixerValue = VolTable[i].mask;
							that->NodeCache[(2*PropertyRequest->Node)+channel] = VolTable[i].max << 16;
						} else {
							mixerValue = ((*Level >> (VolTable[i].dbshift+16)) + VolTable[i].mask) & VolTable[i].mask;
							that->NodeCache[(2*PropertyRequest->Node)+channel] = *Level;
						}
						that->CMIAdapter->writeMixer(VolTable[i].reg+channel, mixerValue << VolTable[i].shift);
					}
					ntStatus = STATUS_SUCCESS;
				}
			}
			if (PropertyRequest->Node == KSNODE_TOPO_AUX_VOLUME) {
				if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
					mixerValue = that->auxVolumeRegister;
					*Level     = that->NodeCache[(2*PropertyRequest->Node)+channel];
					if (channel == CHAN_LEFT) {
						mixerValue >>= 4;
					}
					mixerValue &= 0x0F;
					if (mixerValue != ((*Level >> 18)+0x0F)) {
						*Level = (mixerValue - 0x0F) << 18;
						that->NodeCache[(2*PropertyRequest->Node)+channel] = *Level;
					}
				} else
				if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
					if (*Level <= (-30 << 16)) {
						mixerValue = 0;
						that->NodeCache[(2*PropertyRequest->Node)+channel] = -30 << 16;
					} else
					if (*Level >= 0) {
						mixerValue = 0x0F;
						that->NodeCache[(2*PropertyRequest->Node)+channel] = 0;
					} else	{
						mixerValue = ((*Level >> 18) + 0x0F) & 0x0F;
						that->NodeCache[(2*PropertyRequest->Node)+channel] = *Level;
					}

					if (channel == CHAN_RIGHT) {
						that->auxVolumeRegister = (that->auxVolumeRegister & 0xF0) | mixerValue;
					} else if (channel == CHAN_LEFT) {
						that->auxVolumeRegister = (that->auxVolumeRegister & 0x0F) | (mixerValue << 4);
					}
					that->CMIAdapter->writeUInt8(REG_MIXER3, that->auxVolumeRegister);
				}
				ntStatus = STATUS_SUCCESS;
			}
			if ((PropertyRequest->Node == KSNODE_TOPO_MICIN_VOLUME) && (channel == CHAN_LEFT)) {
				if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
					*Level     = that->NodeCache[(2*PropertyRequest->Node)];
					mixerValue = that->micVolumeRegister >> 1 & 0x7;
					if (mixerValue != ((*Level >> 19)+0x07)) {
						*Level = (mixerValue - 0x07) << 19;
						that->NodeCache[(2*PropertyRequest->Node)] = *Level;
					}
				} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
					if (*Level <= (-56 << 16)) {
						mixerValue = 0;
						that->NodeCache[(2*PropertyRequest->Node)] = -56 << 16;
					} else if (*Level >= 0) {
						mixerValue = 0x07;
						that->NodeCache[(2*PropertyRequest->Node)] = 0;
					} else {
						mixerValue = ((*Level >> 19) + 0x07) & 0x07;
						that->NodeCache[(2*PropertyRequest->Node)] = *Level;
					}
					that->micVolumeRegister &= ~(0x07 << 1);
					that->micVolumeRegister |= mixerValue << 1;
					that->CMIAdapter->writeUInt8(REG_MIXER2, that->micVolumeRegister);
				}
				ntStatus = STATUS_SUCCESS;
			}
			if ((NT_SUCCESS(ntStatus)) && (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)) {
				PropertyRequest->ValueSize = sizeof(LONG);
			}
		}
	} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
		switch(PropertyRequest->Node) {
			case KSNODE_TOPO_LINEOUT_VOLUME:
			case KSNODE_TOPO_WAVEOUT_VOLUME:
			case KSNODE_TOPO_CD_VOLUME:
			case KSNODE_TOPO_LINEIN_VOLUME:
			case KSNODE_TOPO_MICOUT_VOLUME:
			case KSNODE_TOPO_MICIN_VOLUME:
			case KSNODE_TOPO_AUX_VOLUME:
				ntStatus = BasicSupportHandler(PropertyRequest);
				break;
		}
	}

	return ntStatus;
}
NTSTATUS NTAPI PropertyHandler_CpuResources(PPCPROPERTY_REQUEST PropertyRequest)
{
	//PAGED_CODE();
	//ASSERT(PropertyRequest);
	DBGPRINT(("[PropertyHandler_CpuResources]"));

	NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

	if (PropertyRequest->Node == (ULONG)-1) {
		return ntStatus;
	}
	if (PropertyRequest->Node >= KSNODE_TOPO_INVALID) {
		return ntStatus;
	}

	if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
		if (PropertyRequest->ValueSize >= sizeof(LONG)) {
			*((PLONG)PropertyRequest->Value) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;

			PropertyRequest->ValueSize = sizeof(LONG);
			ntStatus = STATUS_SUCCESS;
		} else  {
			ntStatus = STATUS_BUFFER_TOO_SMALL;
		}
	} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
		if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION))) {
			PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

			PropDesc->AccessFlags       = KSPROPERTY_TYPE_BASICSUPPORT |
			                              KSPROPERTY_TYPE_GET;
			PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
			PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
			PropDesc->PropTypeSet.Id    = VT_I4;
			PropDesc->PropTypeSet.Flags = 0;
			PropDesc->MembersListCount  = 0;
			PropDesc->Reserved          = 0;

			PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
			ntStatus = STATUS_SUCCESS;
		} else if (PropertyRequest->ValueSize >= sizeof(ULONG)) {
			PULONG AccessFlags = PULONG(PropertyRequest->Value);

			*AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET;

			PropertyRequest->ValueSize = sizeof(ULONG);
			ntStatus = STATUS_SUCCESS;
		}
	}

	return ntStatus;
}

NTSTATUS NTAPI PropertyHandler_ComponentId(PPCPROPERTY_REQUEST PropertyRequest)
{
	//PAGED_CODE();
	//ASSERT(PropertyRequest);
	DBGPRINT(("[PropertyHandler_ComponentId]"));

	NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
	CCMITopology *that = (CCMITopology *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

	if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
		if (PropertyRequest->ValueSize >= sizeof(KSCOMPONENTID)) {
			PKSCOMPONENTID pComponentId = (PKSCOMPONENTID)PropertyRequest->Value;

			pComponentId->Manufacturer = MANUFACTURER_CM8738;
			pComponentId->Product      = PRODUCT_CM8738;
			pComponentId->Component    = COMPONENT_CM8738;
			pComponentId->Name         = GUID_NULL;
			pComponentId->Version      = CMIPCI_VERSION;
			pComponentId->Revision     = that->cm->chipVersion;

			PropertyRequest->ValueSize = sizeof(KSCOMPONENTID);
			ntStatus = STATUS_SUCCESS;
		} else if (PropertyRequest->ValueSize == 0) {
			PropertyRequest->ValueSize = sizeof(KSCOMPONENTID);
			ntStatus = STATUS_BUFFER_OVERFLOW;
		} else {
			PropertyRequest->ValueSize = 0;
			ntStatus = STATUS_BUFFER_TOO_SMALL;
		}
	} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
		if (PropertyRequest->ValueSize >= sizeof(ULONG)) {
			PULONG AccessFlags = PULONG(PropertyRequest->Value);

			*AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET;

			PropertyRequest->ValueSize = sizeof(ULONG);
			ntStatus = STATUS_SUCCESS;
		} else {
			PropertyRequest->ValueSize = 0;
			ntStatus = STATUS_BUFFER_TOO_SMALL;
		}
	}

	return ntStatus;
}

NTSTATUS NTAPI PropertyHandler_Private(PPCPROPERTY_REQUEST PropertyRequest)
{
	//PAGED_CODE();
	//ASSERT(PropertyRequest);
	DBGPRINT(("[PropertyHandler_Private]"));

	NTSTATUS     ntStatus = STATUS_INVALID_DEVICE_REQUEST;
	CCMITopology *that = (CCMITopology *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

	if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
		if (PropertyRequest->PropertyItem->Id != KSPROPERTY_CMI_GET) {
			return STATUS_INVALID_DEVICE_REQUEST;
		}

		if (PropertyRequest->ValueSize == 0) {
			PropertyRequest->ValueSize = sizeof(CMIDATA);
			return STATUS_BUFFER_OVERFLOW;
		} else if (PropertyRequest->ValueSize < sizeof (CMIDATA)) {
			PropertyRequest->ValueSize = 0;
			return STATUS_BUFFER_TOO_SMALL;
		}

		CMIDATA* cmiData = (CMIDATA*)PropertyRequest->Value;
#ifdef WAVERT
		RtlStringCbPrintfA(cmiData->driverVersion, sizeof(cmiData->driverVersion), CMIVERSION "-WaveRT");
#else
		RtlStringCbPrintfA(cmiData->driverVersion, sizeof(cmiData->driverVersion), CMIVERSION);
#endif
		cmiData->hardwareRevision    = that->cm->chipVersion;
		cmiData->maxChannels         = that->cm->maxChannels;
		cmiData->IOBase              = (USHORT)(ULONG_PTR)that->cm->IOBase;
		cmiData->MPUBase             = (USHORT)(ULONG_PTR)that->cm->MPUBase;
		cmiData->enableSPDO          = that->cm->enableSPDIFOut;
		cmiData->enableSPDI          = that->cm->enableSPDIFIn;
		cmiData->formatMask          = that->cm->formatMask;
		cmiData->exchangeFrontBack   = (that->CMIAdapter->readUInt8(REG_MIXER1) & REAR2FRONT);
		cmiData->enableSPDO5V        = (that->CMIAdapter->readUInt32(REG_MISCCTRL) & EN_SPDO5V);
		cmiData->enablePCMDAC        = (that->CMIAdapter->readUInt8(REG_MIXER1) & EN_SPDI2DAC);
		cmiData->enableBass2Line     = (that->CMIAdapter->readUInt32(REG_LEGACY) & BASS2LINE);
		cmiData->enableCenter2Line   = (that->CMIAdapter->readUInt32(REG_LEGACY) & CENTER2LINE);
		cmiData->enableRear2Line     = (that->CMIAdapter->readUInt8(REG_MIXER1) & REAR2LINE);
		cmiData->enableCenter2Mic    = (that->CMIAdapter->readUInt8(REG_MIXER4) & CENTER2MIC) && (that->cm->chipVersion > 37);
		cmiData->enableSPDOCopyright = (that->CMIAdapter->readUInt32(REG_LEGACY) & EN_SPDCOPYRHT);
		cmiData->invertValidBitSPDI  = (that->CMIAdapter->readUInt32(REG_CHFORMAT) & POLVALID);
		cmiData->loopSPDI            = (that->CMIAdapter->readUInt32(REG_FUNCTRL1) & LOOP_SPDF);
		if (that->cm->chipVersion <= 37) {
			cmiData->select2ndSPDI   = (that->CMIAdapter->readUInt32(REG_CHFORMAT) & SEL_SPDIFI1);
			cmiData->invertPhaseSPDI = (that->CMIAdapter->readUInt8(REG_MIXER4) & INV_SPDIFI1);
		} else {
			cmiData->select2ndSPDI   = (that->CMIAdapter->readUInt32(REG_MISCCTRL) & SEL_SPDIFI2);
			cmiData->invertPhaseSPDI = (that->CMIAdapter->readUInt32(REG_CHFORMAT) & INV_SPDIFI2);
		}

		PropertyRequest->ValueSize = sizeof(CMIDATA);
		ntStatus = STATUS_SUCCESS;
	} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
		if (PropertyRequest->PropertyItem->Id != KSPROPERTY_CMI_SET) {
			return STATUS_INVALID_DEVICE_REQUEST;
		}

		if (PropertyRequest->ValueSize == 0) {
			PropertyRequest->ValueSize = sizeof(CMIDATA);
			return STATUS_BUFFER_OVERFLOW;
		} else if (PropertyRequest->ValueSize < sizeof (CMIDATA)) {
			PropertyRequest->ValueSize = 0;
			return STATUS_BUFFER_TOO_SMALL;
		}
		CMIDATA* cmiData = (CMIDATA*)PropertyRequest->Value;
		that->cm->enableSPDIFIn         = cmiData->enableSPDI;
		that->cm->enableSPDIFOut        = cmiData->enableSPDO;
		that->cm->formatMask            = cmiData->formatMask;

		if (cmiData->enableSPDI) {
			that->CMIAdapter->setUInt8Bit(REG_MIXER1, EN_WAVEIN_L | EN_WAVEIN_R);
		} else {
			that->CMIAdapter->clearUInt8Bit(REG_MIXER1, EN_WAVEIN_L | EN_WAVEIN_R);
		}

		if (cmiData->exchangeFrontBack) {
			that->CMIAdapter->setUInt8Bit(REG_MIXER1, REAR2FRONT);
		} else {
			that->CMIAdapter->clearUInt8Bit(REG_MIXER1, REAR2FRONT);
		}
		if (cmiData->enableSPDO5V) {
			that->CMIAdapter->setUInt32Bit(REG_MISCCTRL, EN_SPDO5V);
		} else {
			that->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, EN_SPDO5V);
		}
		if (cmiData->enablePCMDAC) {
			that->CMIAdapter->setUInt8Bit(REG_MIXER1, EN_SPDI2DAC);
		} else {
			that->CMIAdapter->clearUInt8Bit(REG_MIXER1, EN_SPDI2DAC);
		}
		if (cmiData->enableBass2Line) {
			that->CMIAdapter->setUInt32Bit(REG_LEGACY, BASS2LINE);
		} else {
			that->CMIAdapter->clearUInt32Bit(REG_LEGACY, BASS2LINE);
		}
		if (cmiData->enableCenter2Line) {
			that->CMIAdapter->setUInt32Bit(REG_LEGACY, CENTER2LINE);
		} else {
			that->CMIAdapter->clearUInt32Bit(REG_LEGACY, CENTER2LINE);
		}
		if (cmiData->enableRear2Line) {
			that->CMIAdapter->setUInt8Bit(REG_MIXER1, REAR2LINE);
		} else {
			that->CMIAdapter->clearUInt8Bit(REG_MIXER1, REAR2LINE);
		}
		if (that->cm->chipVersion > 37) {
			if (cmiData->enableCenter2Mic) {
				that->CMIAdapter->setUInt8Bit(REG_MIXER4, CENTER2MIC);
			} else {
				that->CMIAdapter->clearUInt8Bit(REG_MIXER4, CENTER2MIC);
			}
		}
		if (cmiData->enableSPDOCopyright) {
			that->CMIAdapter->setUInt32Bit(REG_LEGACY, EN_SPDCOPYRHT);
		} else {
			that->CMIAdapter->clearUInt32Bit(REG_LEGACY, EN_SPDCOPYRHT);
		}
		if (cmiData->invertValidBitSPDI) {
			that->CMIAdapter->setUInt32Bit(REG_CHFORMAT, POLVALID);
		} else {
			that->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, POLVALID);
		}
		if (cmiData->loopSPDI) {
			that->CMIAdapter->setUInt32Bit(REG_FUNCTRL1, LOOP_SPDF);
		} else {
			that->CMIAdapter->clearUInt32Bit(REG_FUNCTRL1, LOOP_SPDF);
		}
		if (cmiData->select2ndSPDI) {
			if (that->cm->chipVersion <= 37) {
				that->CMIAdapter->setUInt32Bit(REG_CHFORMAT, SEL_SPDIFI1);
			} else {
				that->CMIAdapter->setUInt32Bit(REG_MISCCTRL, SEL_SPDIFI2);
			}
		} else {
			if (that->cm->chipVersion <= 37) {
				that->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, SEL_SPDIFI1);
			} else {
				that->CMIAdapter->clearUInt32Bit(REG_MISCCTRL, SEL_SPDIFI2);
			}
		}
		if (cmiData->invertPhaseSPDI) {
			if (that->cm->chipVersion <= 37) {
				that->CMIAdapter->setUInt8Bit(REG_MIXER4, INV_SPDIFI1);
			} else {
				that->CMIAdapter->setUInt32Bit(REG_CHFORMAT, INV_SPDIFI2);
			}
		} else {
			if (that->cm->chipVersion <= 37) {
				that->CMIAdapter->clearUInt8Bit(REG_MIXER4, INV_SPDIFI1);
			} else {
				that->CMIAdapter->clearUInt32Bit(REG_CHFORMAT, INV_SPDIFI2);
			}
		}

		that->storeMixerSettingsToRegistry();

		ntStatus = STATUS_SUCCESS;
	} else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
		if (PropertyRequest->ValueSize >= sizeof(ULONG)) {
			PULONG AccessFlags = PULONG(PropertyRequest->Value);

			*AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;

			PropertyRequest->ValueSize = sizeof(ULONG);
			ntStatus = STATUS_SUCCESS;
		} else {
			PropertyRequest->ValueSize = 0;
			ntStatus = STATUS_BUFFER_TOO_SMALL;
		}
	}

	return ntStatus;
}
