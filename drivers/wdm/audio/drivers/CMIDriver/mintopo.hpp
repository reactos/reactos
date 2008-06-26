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

#ifndef _MINTOPO_HPP_
#define _MINTOPO_HPP_

#include "common.hpp"
#include "property.h"

class CCMITopology : public ICMITopology,
                     public CUnknown
{
private:
    PCMIADAPTER         CMIAdapter;      // Adapter common object.
    CMI8738Info         *cm;
    UInt8               auxVolumeRegister, micVolumeRegister, mixer1Register, mixer4Register;
    UInt32              functrl1Register, chformatRegister, legacyRegister, miscctrlRegister;
    UInt32              NodeCache[2*KSNODE_TOPO_INVALID];
    UInt32              masterMuteDummy;
    BOOLEAN             settingsLoaded;  // workaround for the fucking XP mixer

    NTSTATUS ProcessResources(PRESOURCELIST ResourceList);
public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CCMITopology);
    ~CCMITopology();
	STDMETHODIMP_(NTSTATUS) loadMixerSettingsFromRegistry();
    STDMETHODIMP_(NTSTATUS) storeMixerSettingsToRegistry();

	STDMETHODIMP_(NTSTATUS) loadMixerSettingsFromMemory();
    STDMETHODIMP_(NTSTATUS) storeMixerSettingsToMemory();

    STDMETHODIMP_(NTSTATUS) GetDescription
    (
		OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
    );
    STDMETHODIMP_(NTSTATUS) DataRangeIntersection
    (
		IN      ULONG           PinId,
        IN      PKSDATARANGE    DataRange,
        IN      PKSDATARANGE    MatchingDataRange,
        IN      ULONG           OutputBufferLength,
        OUT     PVOID           ResultantFormat     OPTIONAL,
        OUT     PULONG          ResultantFormatLength
    )
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    // public methods
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTTOPOLOGY   Port
    );

	//friends
    friend NTSTATUS PropertyHandler_OnOff(PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS PropertyHandler_Level(PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS PropertyHandler_CpuResources(PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS PropertyHandler_ComponentId(PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS PropertyHandler_Private(PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS PropertyHandler_Mux(PPCPROPERTY_REQUEST PropertyRequest);

    static NTSTATUS EventHandler(PPCEVENT_REQUEST EventRequest);
};

#endif //_MINTOPO_HPP_
