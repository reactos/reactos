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

#define PUT_GUIDS_HERE
#include <initguid.h>
#include "adapter.hpp"

#ifdef _MSC_VER
//#pragma code_seg("PAGE") // GCC ignores pragma code_seg
#endif

const GUID KSNODETYPE_DAC = {0x507AE360L, 0xC554, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_ADC = {0x4D837FE0L, 0xC555, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_AGC = {0xE88C9BA0L, 0xC557, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_LOUDNESS = {0x41887440L, 0xC558, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_MUTE =     {0x02B223C0L, 0xC557, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_TONE =     {0x7607E580L, 0xC557, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_VOLUME =   {0x3A5ACC00L, 0xC557, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_PEAKMETER = {0xa085651e, 0x5f0d, 0x4b36, {0xa8, 0x69, 0xd1, 0x95, 0xd6, 0xab, 0x4b, 0x9e}};
const GUID KSNODETYPE_MUX =       {0x2CEAF780, 0xC556, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_STEREO_WIDE = {0xA9E69800L, 0xC558, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_CHORUS =      {0x20173F20L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_REVERB =      {0xEF0328E0L, 0xC558, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_SUPERMIX =    {0xE573ADC0L, 0xC555, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_SUM = {0xDA441A60L, 0xC556, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_SRC = {0x9DB7B9E0L, 0xC555, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_3D_EFFECTS = {0x55515860L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_SPDIF_INTERFACE = {0x0605+0xDFF219E0, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_MICROPHONE      = {0x0201+0xDFF219E0,0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_CD_PLAYER       = {0x0703+0xDFF219E0,0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_LINE_CONNECTOR  = {0x0603+0xDFF219E0,0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_ANALOG_CONNECTOR = {0x601+0xDFF219E0,0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_SPEAKER         = {0x0301+0xDFF219E0,0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};

const GUID KSPROPTYPESETID_General             = {0x97E99BA0L, 0xBDEA, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_General = {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSPROPSETID_Audio = {0x45FFAAA0L, 0x6E1B, 0x11D0, {0xBC, 0xF2, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID GUID_NULL ={0x00000000L, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
const GUID KSCATEGORY_AUDIO    = {0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};


const GUID KSDATAFORMAT_TYPE_AUDIO =             {0x73647561L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SUBTYPE_PCM =            {0x00000001L, 0x0000, 0x0010,  {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const GUID KSDATAFORMAT_SPECIFIER_DSOUND       = {0x518590a2L, 0xa184, 0x11d0, {0x85, 0x22, 0x00, 0xc0, 0x4f, 0xd9, 0xba, 0xf3}};
const GUID KSDATAFORMAT_SUBTYPE_WAVEFORMATEX           = {0x00000000L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF = {0x00000092L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};


const GUID KSAUDFNAME_WAVE_VOLUME = {0x185FEDE5L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_WAVE_MUTE   = {0x185FEDE6L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_MIC_VOLUME  = {0x185FEDEDL, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_MASTER_VOLUME = {0x185FEDE3L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_RECORDING_SOURCE = {0x185FEDEFL, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_CD_VOLUME   = {0x185FEDE9L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_CD_IN_VOLUME = {0x185FEDF3L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_MIC_IN_VOLUME = {0x185FEDF5L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_MICROPHONE_BOOST = {0x2bc31d6aL, 0x96e3, 0x11d2, {0xac, 0x4c, 0x0, 0xc0, 0x4f, 0x8e, 0xfb, 0x68}};
const GUID KSAUDFNAME_CD_MUTE = {0x185FEDEAL, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_LINE_MUTE = {0x185FEDECL, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_MIC_MUTE = {0x185FEDEEL, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_AUX_MUTE = {0x185FEDFDL, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_MASTER_MUTE = {0x185FEDE4L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_RECORDING_CONTROL = {0x185FEDFAL, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_VOLUME_CONTROL = {0x185FEDF7L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_LINE_IN_VOLUME = {0x185FEDF4L, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};
const GUID KSAUDFNAME_AUX_VOLUME = {0x185FEDFCL, 0x9905, 0x11D1, {0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};

const GUID KSPROPSETID_CMI = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xFF}};


const GUID CMINAME_IEC_5V  = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF0}};
const GUID CMINAME_IEC_OUT = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF1}};
const GUID CMINAME_IEC_INVERSE = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF2}};
const GUID CMINAME_IEC_MONITOR = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF3}};
const GUID CMINAME_IEC_SELECT  = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF5}};
const GUID CMINAME_XCHG_FB = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF6}};
const GUID CMINAME_BASS2LINE = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF7}};
const GUID CMINAME_CENTER2LINE = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF8}};
const GUID CMINAME_IEC_COPYRIGHT = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF9}};
const GUID CMINAME_IEC_POLVALID = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xFA}};
const GUID CMINAME_IEC_LOOP = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xFB}};
const GUID CMINAME_REAR2LINE = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xFC}};
const GUID CMINAME_CENTER2MIC = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xFD}};
const GUID CMINAME_DAC = {0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xF4}};
const GUID PRODUCT_CM8738 = {0x9db14e9a, 0x7be7, 0x480d, {0xa2, 0xfa, 0x32, 0x93, 0x24, 0x89, 0xde, 0x9c}};
const GUID COMPONENT_CM8738 = {0x9db14e9a, 0x7be7, 0x480d, {0xa2, 0xfa, 0x32, 0x93, 0x24, 0x89, 0xde, 0x9d}};
const GUID MANUFACTURER_CM8738 = {0x9db14e9a, 0x7be7, 0x480d, {0xa2, 0xfa, 0x32, 0x93, 0x24, 0x89, 0xde, 0x9e}};


NTSTATUS InstallSubdevice(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PWCHAR Name,
    REFGUID PortClassId,
    REFGUID MiniportClassId,
    PFNCREATEINSTANCE MiniportCreate,
    PUNKNOWN UnknownAdapter,
    PRESOURCELIST ResourceList,
    REFGUID PortInterfaceId,
    PUNKNOWN* OutPortUnknown)
{
	NTSTATUS	ntStatus;
	PPORT	   	Port;
	PMINIPORT   MiniPort;

    ////PAGED_CODE();
    DBGPRINT(("InstallSubdevice()"));

	ntStatus = PcNewPort(&Port, PortClassId);
	if (NT_SUCCESS(ntStatus)) {
		if (MiniportCreate) {
			ntStatus = MiniportCreate((PUNKNOWN*)&MiniPort, MiniportClassId, NULL, NonPagedPool);
		} else {
			ntStatus = PcNewMiniport(&MiniPort, MiniportClassId);
		}
	}

	if (!NT_SUCCESS(ntStatus)) {
		Port->Release();
		return ntStatus;
	}

	ntStatus = Port->Init(DeviceObject, Irp, MiniPort, UnknownAdapter, ResourceList);
	if (NT_SUCCESS(ntStatus)) {
		ntStatus = PcRegisterSubdevice(DeviceObject, Name, Port);

		if (OutPortUnknown && NT_SUCCESS (ntStatus)) {
			ntStatus = Port->QueryInterface(IID_IUnknown, (PVOID *)OutPortUnknown);
		}
	}

	if (MiniPort) {
		MiniPort->Release();
	}

	if (Port) {
		Port->Release();
	}

	return ntStatus;
}


NTSTATUS
ProcessResources(
    PRESOURCELIST ResourceList,
    PRESOURCELIST* UartResourceList)
{
	NTSTATUS ntStatus;

	////PAGED_CODE();
	////ASSERT(ResourceList);
	////ASSERT(UartResourceList);
	//DBGPRINT(("ProcessResources()"));
	//DBGPRINT(("NumberOfPorts: %d, NumberOfInterrupts: %d, NumberOfDmas: %d", ResourceList->NumberOfPorts(), ResourceList->NumberOfInterrupts(), ResourceList->NumberOfDmas()));

#ifdef UART
	(*UartResourceList) = NULL;
#endif


	if ((ResourceList->NumberOfPorts() == 0) || (ResourceList->NumberOfPorts() > 2) || (ResourceList->NumberOfInterrupts() != 1) || (ResourceList->NumberOfDmas() != 0)) {
		DBGPRINT(("Unexpected configuration"));
		return STATUS_DEVICE_CONFIGURATION_ERROR;
	}

#ifdef UART
	ntStatus = PcNewResourceSublist(UartResourceList, NULL, PagedPool, ResourceList, 2);
	if (NT_SUCCESS(ntStatus)) {
		(*UartResourceList)->AddPortFromParent(ResourceList, 1);
		(*UartResourceList)->AddInterruptFromParent(ResourceList, 0);
	}
#endif

	return STATUS_SUCCESS;
}


NTSTATUS StartDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp, PRESOURCELIST ResourceList)
{
	NTSTATUS ntStatus;
	PPORT    pPort = 0;
	ULONG*   MPUBase;
#if 0
	//PAGED_CODE();
	//ASSERT(DeviceObject);
	//ASSERT(Irp);
	//ASSERT(ResourceList);
	DBGPRINT(("StartDevice()"));
#endif

	ntStatus = PcNewPort(&pPort,CLSID_PortWaveCyclic);
	if (NT_SUCCESS(ntStatus)) {
		// not supported in the first edition of win98
		PPORTEVENTS pPortEvents = 0;
		ntStatus = pPort->QueryInterface(IID_IPortEvents, (PVOID *)&pPortEvents);
		if (!NT_SUCCESS(ntStatus)) {
			DBGPRINT(("ERROR: This driver doesn't work under Win98!"));
			ntStatus = STATUS_UNSUCCESSFUL;
		}
		else
		{
			pPortEvents->Release();
		}
		pPort->Release ();
	} else {
		return ntStatus;
	}

	// resource validation
	PRESOURCELIST UartResourceList = NULL;
	ntStatus = ProcessResources(ResourceList, &UartResourceList);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("ProcessResources() failed"));
		return ntStatus;
	}

	PCMIADAPTER	pCMIAdapter	= NULL;
	PUNKNOWN	pUnknownCommon = NULL;

	// create the CMIAdapter object
	ntStatus = NewCMIAdapter(&pUnknownCommon, IID_ICMIAdapter, NULL, NonPagedPool);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("NewCMIAdapter() failed"));
		return ntStatus;
	}

	ntStatus = pUnknownCommon->QueryInterface(IID_ICMIAdapter, (PVOID *)&pCMIAdapter);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("QueryInterface() for ICMIAdapter failed"));
		return ntStatus;
	}
	ntStatus = pCMIAdapter->init(ResourceList, DeviceObject);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("CMIAdapter->init() failed"));
		return ntStatus;
	}

	ntStatus = PcRegisterAdapterPowerManagement((PUNKNOWN)pCMIAdapter, DeviceObject);

	pUnknownCommon->Release();

	PUNKNOWN unknownWave = NULL;
	PUNKNOWN unknownTopology = NULL;

	// install the topology miniport.
	ntStatus = InstallSubdevice( DeviceObject, Irp, (PWCHAR) L"Topology",
		CLSID_PortTopology, CLSID_PortTopology, CreateMiniportTopologyCMI,
		pCMIAdapter, NULL, GUID_NULL, &unknownTopology );
	if (!NT_SUCCESS (ntStatus)) {
		DBGPRINT(("Topology miniport installation failed"));
		return ntStatus;
	}

#ifdef UART
	// install the UART miniport - execution order important
	ntStatus = STATUS_UNSUCCESSFUL;
	MPUBase = 0;
	for ( UINT i=0; i < ResourceList->NumberOfPorts(); i++ ) {
		if (ResourceList->FindTranslatedPort(i)->u.Port.Length == 2) {
			MPUBase = (UInt32*)ResourceList->FindTranslatedPort(i)->u.Port.Start.QuadPart;
		}
	}
	if (MPUBase != 0) {
		ntStatus = pCMIAdapter->activateMPU(MPUBase);
		if (NT_SUCCESS(ntStatus)) {
			ntStatus = InstallSubdevice( DeviceObject, Irp, (PWCHAR) L"Uart",
				CLSID_PortDMus, CLSID_MiniportDriverDMusUART, NULL,
				pCMIAdapter->getInterruptSync(), UartResourceList,
				IID_IPortDMus, NULL );
		}
	}
	if (!NT_SUCCESS(ntStatus)) {
		MPUBase = 0;
		pCMIAdapter->activateMPU(0);
		DBGPRINT(("UART miniport installation failed"));
	}
	if (UartResourceList) {
		UartResourceList->Release();
	}
#endif

	// install the wave miniport - the order matters here
#ifdef WAVERT
	ntStatus = InstallSubdevice(DeviceObject, Irp, (PWCHAR) L"Wave",
		CLSID_PortWaveRT, CLSID_PortWaveRT, CreateMiniportWaveCMI,
		pCMIAdapter, ResourceList, IID_IPortWaveRT, &unknownWave );
#else
	ntStatus = InstallSubdevice(DeviceObject, Irp, (PWCHAR) L"Wave",
		CLSID_PortWaveCyclic, CLSID_PortWaveCyclic, CreateMiniportWaveCMI,
		pCMIAdapter, ResourceList, IID_IPortWaveCyclic, &unknownWave );
#endif
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Wave miniport installation failed"));
		return ntStatus;
	}

	// connect wave and topology pins
	ntStatus = PcRegisterPhysicalConnection(DeviceObject, unknownWave, PIN_WAVE_RENDER_SOURCE, unknownTopology, PIN_WAVEOUT_SOURCE);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Cannot connect topology and wave miniport (render)!"));
		return ntStatus;
	}
	ntStatus = PcRegisterPhysicalConnection(DeviceObject, unknownTopology, PIN_WAVEIN_DEST, unknownWave, PIN_WAVE_CAPTURE_SOURCE);
	if (!NT_SUCCESS(ntStatus)) {
		DBGPRINT(("Cannot connect topology and wave miniport (capture)!"));
		return ntStatus;
	}
	if (!IoIsWdmVersionAvailable(6,0)) {
		// this shit fixes the fucking XP mixer and breaks the vista mixer, so we have to check for vista here
		ntStatus = PcRegisterPhysicalConnection(DeviceObject, unknownWave, PIN_WAVE_AC3_RENDER_SOURCE, unknownTopology, PIN_SPDIF_AC3_SOURCE);
		if (!NT_SUCCESS(ntStatus)) {
			DBGPRINT(("Cannot connect topology and wave miniport (ac3)!"));
		}
	}

	// clean up
	if (pCMIAdapter) {
		pCMIAdapter->Release();
	}
	if (unknownTopology) {
		unknownTopology->Release();
	}
	if (unknownWave) {
		unknownWave->Release();
	}

	return ntStatus;
}

extern 
"C"
NTSTATUS
NTAPI
AddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject)
{
#if 0
    //PAGED_CODE();
    DBGPRINT(("AddDevice()"));
#endif

    return PcAddAdapterDevice(DriverObject, PhysicalDeviceObject, (PCPFNSTARTDEVICE)StartDevice, MAX_MINIPORTS, 0);
}

bool CopyResourceDescriptor(PIO_RESOURCE_DESCRIPTOR pInResDescriptor, PIO_RESOURCE_DESCRIPTOR pOutResDescriptor)
{
#if 0
	//PAGED_CODE();
	//ASSERT(pInResDescriptor);
	//ASSERT(pOutResDescriptor);
	DBGPRINT(("CopyResourceDescriptor()"));
	RtlCopyMemory(pOutResDescriptor, pInResDescriptor, sizeof(IO_RESOURCE_DESCRIPTOR));
#else
	pOutResDescriptor->Type             = pInResDescriptor->Type;
	pOutResDescriptor->ShareDisposition = pInResDescriptor->ShareDisposition;
	pOutResDescriptor->Flags            = pInResDescriptor->Flags;
	pOutResDescriptor->Option           = pInResDescriptor->Option;

	switch (pInResDescriptor->Type) {
		case CmResourceTypePort:
		case CmResourceTypePort | CmResourceTypeNonArbitrated:  // huh?
/*			// filter crap
			if ((pInResDescriptor->u.Port.Length == 0) ||
			    ( (pInResDescriptor->u.Port.MinimumAddress.HighPart == pInResDescriptor->u.Port.MaximumAddress.HighPart) && (pInResDescriptor->u.Port.MinimumAddress.LowPart == pInResDescriptor->u.Port.MaximumAddress.LowPart) ) ) {
				return FALSE;
			}
*/			pOutResDescriptor->u.Port.MinimumAddress = pInResDescriptor->u.Port.MinimumAddress;
			pOutResDescriptor->u.Port.MaximumAddress = pInResDescriptor->u.Port.MaximumAddress;
			pOutResDescriptor->u.Port.Length         = pInResDescriptor->u.Port.Length;
			pOutResDescriptor->u.Port.Alignment	     = pInResDescriptor->u.Port.Alignment;
#if 0
			DBGPRINT((" Port: min %08x.%08x max %08x.%08x, Length: %x, Option: %x", pOutResDescriptor->u.Port.MinimumAddress.HighPart, pOutResDescriptor->u.Port.MinimumAddress.LowPart,
			                                                            pOutResDescriptor->u.Port.MaximumAddress.HighPart, pOutResDescriptor->u.Port.MaximumAddress.LowPart,
			                                                            pOutResDescriptor->u.Port.Length, pOutResDescriptor->Option));
#endif
			break;
		case CmResourceTypeInterrupt:
			pOutResDescriptor->u.Interrupt.MinimumVector = pInResDescriptor->u.Interrupt.MinimumVector;
			pOutResDescriptor->u.Interrupt.MaximumVector = pInResDescriptor->u.Interrupt.MaximumVector;
#if 0
			DBGPRINT((" IRQ:  min %x max %x, Option: %d", pOutResDescriptor->u.Interrupt.MinimumVector, pOutResDescriptor->u.Interrupt.MaximumVector, pOutResDescriptor->Option));
#endif
			break;
		default:
			return FALSE;
	}
	return TRUE;
#endif
}

extern
"C"
NTSTATUS
NTAPI
AdapterDispatchPnp(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp)
{
	NTSTATUS                       ntStatus = STATUS_SUCCESS;
	ULONG                          resourceListSize;
	PIO_RESOURCE_REQUIREMENTS_LIST resourceList, list;
	PIO_RESOURCE_DESCRIPTOR        descriptor;
	PIO_STACK_LOCATION             pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	////PAGED_CODE();
	////ASSERT(pDeviceObject);
	////ASSERT(pIrp);
	DBGPRINT(("AdapterDispatchPnp()"));

	if (pIrpStack->MinorFunction == IRP_MN_FILTER_RESOURCE_REQUIREMENTS) {
		DBGPRINT(("[AdapterDispatchPnp] - IRP_MN_FILTER_RESOURCE_REQUIREMENTS"));

		list = (PIO_RESOURCE_REQUIREMENTS_LIST)pIrp->IoStatus.Information;

		// IO_RESOURCE_REQUIREMENTS_LIST has 1 IO_RESOURCE_LIST, IO_RESOURCE_LIST has 1 IO_RESOURCE_DESCRIPTOR and we want 2 more
		resourceListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + sizeof(IO_RESOURCE_DESCRIPTOR)*(list->List[0].Count+2) ;
		resourceList = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePoolWithTag(PagedPool, resourceListSize, 'LRDV');

		if (!resourceList) {
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			return ntStatus;
		}


		RtlZeroMemory(resourceList, resourceListSize);

		// initialize the list header
		resourceList->AlternativeLists = 1; // number of IO_RESOURCE_LISTs
		resourceList->ListSize = resourceListSize;

		resourceList->List[0].Version  = 1;
		resourceList->List[0].Revision = 1;
		resourceList->List[0].Count    = 0;

		// copy the resources which have already been assigned
		for ( UINT i=0; i < list->List[0].Count; i++ ) {
			if (CopyResourceDescriptor( &list->List[0].Descriptors[i],
				&resourceList->List[0].Descriptors[resourceList->List[0].Count] ))
			{
				resourceList->List[0].Count++;
			}
		}
		ExFreePool(list);

		// an additional port for mpu401
		resourceList->List[0].Count++;
		descriptor = &resourceList->List[0].Descriptors[resourceList->List[0].Count-1];
		descriptor->Option                = IO_RESOURCE_PREFERRED;
		descriptor->Type                  = CmResourceTypePort;
		descriptor->ShareDisposition      = CmResourceShareDeviceExclusive;
		descriptor->Flags                 = CM_RESOURCE_PORT_IO;
		descriptor->u.Port.MinimumAddress.LowPart  = 0x300;
		descriptor->u.Port.MinimumAddress.HighPart = 0;
		descriptor->u.Port.MaximumAddress.LowPart  = 0x330;
		descriptor->u.Port.MaximumAddress.HighPart = 0;
		descriptor->u.Port.Length         = 2;
		descriptor->u.Port.Alignment      = 0x10;

		// mpu401 port should be optional. yes, this is severely braindamaged.
		resourceList->List[0].Count++;
		descriptor = &resourceList->List[0].Descriptors[resourceList->List[0].Count-1];
		descriptor->Option                = IO_RESOURCE_ALTERNATIVE;
		descriptor->Type                  = CmResourceTypePort;
		descriptor->ShareDisposition      = CmResourceShareDeviceExclusive;
		descriptor->Flags                 = CM_RESOURCE_PORT_IO;
		descriptor->u.Port.MinimumAddress.LowPart  = 0x0;
		descriptor->u.Port.MinimumAddress.HighPart = 0;
		descriptor->u.Port.MaximumAddress.LowPart  = 0xFFFF;
		descriptor->u.Port.MaximumAddress.HighPart = 0;
		descriptor->u.Port.Length         = 1;
		descriptor->u.Port.Alignment      = 0x10;

//		DBGPRINT(("number of resource list descriptors: %d", resourceList->List[0].Count));

		pIrp->IoStatus.Information = (ULONG_PTR)resourceList;

		// set the return status
		pIrp->IoStatus.Status = ntStatus;
	}

	// Pass the IRPs on to PortCls
	ntStatus = PcDispatchIrp(pDeviceObject, pIrp);

	return ntStatus;
}

extern 
"C"
NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
     PUNICODE_STRING RegistryPathName)
{
    NTSTATUS ntStatus;

    DBGPRINT(("DriverEntry()"));


    //bind the adapter driver to the portclass driver
    ntStatus = PcInitializeAdapterDriver(DriverObject, RegistryPathName, AddDevice);


#ifdef UART
    if(NT_SUCCESS(ntStatus)) {
    DriverObject->MajorFunction[IRP_MJ_PNP] = AdapterDispatchPnp;
    }
#endif
#ifdef WAVERT
    if (!IoIsWdmVersionAvailable(6,0)) {
    ntStatus = STATUS_UNSUCCESSFUL;
    }
#endif

    return ntStatus;
}


#ifdef _MSC_VER

#pragma code_seg()
int __cdecl _purecall (void)
{
    return 0;
}

#else

extern "C" {
void __cxa_pure_virtual()
  {
    // put error handling here

    DbgBreakPoint();

  }
}
#endif
