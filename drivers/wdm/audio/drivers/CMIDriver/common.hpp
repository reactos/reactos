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

#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include "stdunk.h"
#include "portcls.h"
#include "dmusicks.h"
#include "ksdebug.h"
#include "kcom.h"

#include "interfaces.hpp"
#include "debug.hpp"
#include "cmireg.hpp"

class CCMIAdapter : public ICMIAdapter,
                    public IAdapterPowerManagement,
                    public CUnknown
{
private:
    PDEVICE_OBJECT		DeviceObject;
    PINTERRUPTSYNC      InterruptSync;
    DEVICE_POWER_STATE	CurrentPowerState;
    UInt8               mixerCache[0xFF];

	CMI8738Info			cm;

	bool queryChip();
    void resetController();

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CCMIAdapter);
    ~CCMIAdapter();

    IMP_IAdapterPowerManagement;

	STDMETHODIMP_(NTSTATUS)	init(PRESOURCELIST ResourceList, PDEVICE_OBJECT aDeviceObject);
	STDMETHODIMP_(NTSTATUS) activateMPU(ULONG* MPUBase);
	STDMETHODIMP_(NTSTATUS) loadSBMixerFromMemory();

    STDMETHODIMP_(UInt8)	readUInt8(UInt8 reg);
    STDMETHODIMP_(void)		writeUInt8(UInt8 reg, UInt8 value);

	STDMETHODIMP_(void)		setUInt8Bit(UInt8 reg, UInt8 flag);
	STDMETHODIMP_(void)		clearUInt8Bit(UInt8 reg, UInt8 flag);

    STDMETHODIMP_(UInt16)	readUInt16(UInt8 reg);
    STDMETHODIMP_(void)		writeUInt16(UInt8 reg, UInt16 value);

    STDMETHODIMP_(UInt32)	readUInt32(UInt8 reg);
    STDMETHODIMP_(void)		writeUInt32(UInt8 reg, UInt32 value);

	STDMETHODIMP_(void)		setUInt32Bit(UInt8 reg, UInt32 flag);
	STDMETHODIMP_(void)		clearUInt32Bit(UInt8 reg, UInt32 flag);

    STDMETHODIMP_(UInt8)	readMixer(UInt8 index);
    STDMETHODIMP_(void)		writeMixer(UInt8 index, UInt8 value);
    STDMETHODIMP_(void)		setMixerBit(UInt8 index, UInt8 flag);
    STDMETHODIMP_(void)		clearMixerBit(UInt8 index, UInt8 flag);

    STDMETHODIMP_(void)		resetMixer();

    static NTSTATUS			InterruptServiceRoutine(PINTERRUPTSYNC InterruptSync, PVOID StaticContext);

    STDMETHODIMP_(PCMI8738Info) getCMI8738Info(void)
    {
        return &cm;
    };

    STDMETHODIMP_(PINTERRUPTSYNC) getInterruptSync(void)
    {
        return InterruptSync;
    };
    STDMETHODIMP_(PDEVICE_OBJECT) getDeviceObject(void)
    {
        return DeviceObject;
    };

    friend NTSTATUS NewCCMIAdapter(PCMIADAPTER* OutCMIAdapter, PRESOURCELIST ResourceList);
};

NTSTATUS NewCMIAdapter(PUNKNOWN* Unknown, REFCLSID, PUNKNOWN UnknownOuter, POOL_TYPE PoolType);

#endif  //_COMMON_HPP_
