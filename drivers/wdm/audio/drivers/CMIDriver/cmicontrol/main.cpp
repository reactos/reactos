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

#include "main.h"


void PrintLastError(LPCSTR function)
{
	LPVOID	lpMsgBuf;
	DWORD   errorid = GetLastError();

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorid, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	MessageBox(NULL, (LPCSTR)lpMsgBuf, function, MB_ICONEXCLAMATION | MB_OK);
	LocalFree(lpMsgBuf);
}

BOOL generateTestSignal(double amplitude, int Channels, int SamplesPerSec, SHORT** buffer)
{
	int    i, o2, o3, o4, o5;
	bool   Left,Right,BackLeft,BackRight,Center,Sub, CenterLeft, CenterRight;
	short  value;
	double x = SPEAKER_FREQUENCY*2*3.141592654/SamplesPerSec;
	double y = BASS_FREQUENCY*2*3.141592654/SamplesPerSec;

	Left        = (SendMessage(GetDlgItem(hWndChild[0], IDC_LEFT), BM_GETCHECK, 0, 0) == BST_CHECKED);
	Right       = (SendMessage(GetDlgItem(hWndChild[0], IDC_RIGHT), BM_GETCHECK, 0, 0) == BST_CHECKED);
	BackLeft    = (SendMessage(GetDlgItem(hWndChild[0], IDC_BLEFT), BM_GETCHECK, 0, 0) == BST_CHECKED) && (currentChannelCount > 2);
	BackRight   = (SendMessage(GetDlgItem(hWndChild[0], IDC_BRIGHT), BM_GETCHECK, 0, 0) == BST_CHECKED) && (currentChannelCount > 2);
	Center      = (SendMessage(GetDlgItem(hWndChild[0], IDC_CENTER), BM_GETCHECK, 0, 0) == BST_CHECKED) && (currentChannelCount > 4);
	Sub         = (SendMessage(GetDlgItem(hWndChild[0], IDC_SUB), BM_GETCHECK, 0, 0) == BST_CHECKED) && (currentChannelCount > 4);
	CenterLeft  = (SendMessage(GetDlgItem(hWndChild[0], IDC_CLEFT), BM_GETCHECK, 0, 0) == BST_CHECKED) && (currentChannelCount > 6);
	CenterRight = (SendMessage(GetDlgItem(hWndChild[0], IDC_CRIGHT), BM_GETCHECK, 0, 0) == BST_CHECKED) && (currentChannelCount > 6);

	if (!(Left || Right || BackLeft || BackRight || Center || Sub || CenterLeft || CenterRight)) {
		return FALSE;
	}

	if (currentChannelCount > 4) {
		o2 = 4; o3 = 5;
		o4 = 2; o5 = 3;
	} else {
		o2 = 2; o3 = 3;
		o4 = 4; o5 = 5;
	}

	(*buffer) = (SHORT*)LocalAlloc(LPTR, SamplesPerSec*sizeof(SHORT)*Channels);
	ZeroMemory((*buffer), SamplesPerSec*sizeof(SHORT)*Channels);

	for (i=0;i<SamplesPerSec;i++) {
		value = (SHORT)(cos(i*x)*amplitude*32767.0);
		if (Left) {
			(*buffer)[(i*Channels)+0] = value;
		}
		if (Right) {
			(*buffer)[(i*Channels)+1] = value;
		}
		if (BackLeft) {
			(*buffer)[(i*Channels)+o2] = value;
		}
		if (BackRight) {
			(*buffer)[(i*Channels)+o3] = value;
		}
		if (Center) {
			(*buffer)[(i*Channels)+o4] = value;
		}
		if (Sub) {
			(*buffer)[(i*Channels)+o5] = (SHORT)(cos(i*y)*amplitude*32767.0);
		}
		if (CenterLeft) {
			(*buffer)[(i*Channels)+6] = value;
		}
		if (CenterRight) {
			(*buffer)[(i*Channels)+7] = value;
		}
	}
	return TRUE;
}

BOOL stopTestTone(void)
{
	if (hWave == NULL) {
		return FALSE;
	}

	if (waveOutReset(hWave) != MMSYSERR_NOERROR) {
		PrintLastError("waveOutReset()");
		return FALSE;
	}
	if (waveOutClose(hWave) != MMSYSERR_NOERROR) {
		PrintLastError("waveOutClose()");
		return FALSE;
	}
	hWave = NULL;
	LocalFree(pwh.lpData);
	return TRUE;
}

UINT findWaveDeviceID()
{
	WAVEOUTCAPS  woc;
	UINT         i, numDev;

	numDev = waveOutGetNumDevs();
	for (i=0;i<numDev;i++) {
	    if (!waveOutGetDevCaps(i, &woc, sizeof(WAVEOUTCAPS))) {
			if ((CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, woc.szPname, -1, TEXT("CMI8738/8768 Wave"), -1) == CSTR_EQUAL) ||
			    (CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, woc.szPname, -1, TEXT("Speakers (CMI8738/8768 Audio De"), -1) == CSTR_EQUAL)) {
				return i;
			}
    	}
	}
	return WAVE_MAPPER;
}

BOOL playTestTone()
{
	SHORT*               buffer;
#if 1
	WAVEFORMATEXTENSIBLE wfx;

	ZeroMemory(&wfx, sizeof(WAVEFORMATEXTENSIBLE));
	wfx.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
	wfx.Format.nChannels            = (WORD)currentChannelCount;
	wfx.Format.nSamplesPerSec       = SAMPLE_RATE;
	wfx.Format.wBitsPerSample       = 16;
	wfx.Format.nBlockAlign          = (wfx.Format.wBitsPerSample >> 3) * wfx.Format.nChannels;
	wfx.Format.nAvgBytesPerSec      = SAMPLE_RATE * (wfx.Format.wBitsPerSample >> 3) * wfx.Format.nChannels;
	wfx.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample ;
	wfx.SubFormat                   = KSDATAFORMAT_SUBTYPE_PCM;
#else
	WAVEFORMATEX         wfx;
	wfx.wFormatTag      = WAVE_FORMAT_PCM;
	wfx.wBitsPerSample  = 16;
	wfx.nChannels       = (WORD)currentChannelCount;
	wfx.nSamplesPerSec  = SAMPLE_RATE;
	wfx.nAvgBytesPerSec = SAMPLE_RATE * (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nBlockAlign     = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.cbSize          = 0;
#endif

	if (waveOutOpen(&hWave, findWaveDeviceID(), (WAVEFORMATEX*)&(wfx), 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
		PrintLastError("waveOutOpen()");
		return FALSE;
	}

	if (!generateTestSignal(SPEAKER_AMPLITUDE, currentChannelCount, SAMPLE_RATE, &buffer)) {
		return FALSE;
	}

	ZeroMemory(&pwh, sizeof(pwh));
	pwh.lpData         = (LPSTR)buffer;
	pwh.dwBufferLength = SAMPLE_RATE*sizeof(SHORT)*currentChannelCount;
	pwh.dwFlags        = WHDR_BEGINLOOP | WHDR_ENDLOOP;
	pwh.dwLoops        = 0xFFFFFFFF;
	if (waveOutPrepareHeader(hWave, &pwh, sizeof(pwh)) != MMSYSERR_NOERROR) {
		LocalFree(buffer);
		PrintLastError("waveOutPrepareHeader()");
		return FALSE;
	}

 	if (waveOutReset(hWave) != MMSYSERR_NOERROR) {
		LocalFree(buffer);
		PrintLastError("waveOutReset()");
		return FALSE;
	}
	if (waveOutWrite(hWave, &pwh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		LocalFree(buffer);
		PrintLastError("waveOutWrite()");
		return FALSE;
	}

	return TRUE;
}


BOOL CALLBACK DSEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext)
{
	LPGUID* pGUID = (LPGUID*)lpContext;

	if (pGUID == NULL) {
		return FALSE;
	}
	if ((*pGUID) != NULL) {
		return TRUE;
	}

	if (lpGUID != NULL) {
		// XP, 2k
	    if ((CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, lpszDrvName, -1, TEXT("cmipci.sys"), -1) == CSTR_EQUAL) &&
		  (CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, lpszDesc, -1, TEXT("CMI8738/8768 Wave"), -1) == CSTR_EQUAL)) {
			(*pGUID) = (LPGUID)LocalAlloc(LPTR, sizeof(GUID));
			memcpy((*pGUID), lpGUID, sizeof(GUID));
			return TRUE;
		}
		// Vista
		if (CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, lpszDesc, -1, TEXT("Speakers (CMI8738/8768 Audio Device)"), -1) == CSTR_EQUAL) {
			(*pGUID) = (LPGUID)LocalAlloc(LPTR, sizeof(GUID));
			memcpy((*pGUID), lpGUID, sizeof(GUID));
			return TRUE;
		}
	}
	return TRUE;
}

BOOL getCurrentChannelConfig()
{
	IDirectSound8* ds;
	DWORD          speakerConfig;
	LPGUID         guid = NULL;

	DirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumProc, (VOID*)&guid);

	if (DirectSoundCreate8(guid, &ds, NULL) != S_OK) {
		PrintLastError("DirectSoundCreate8()");
		return FALSE;
	}

	ds->Initialize(NULL);

	if (ds->GetSpeakerConfig(&speakerConfig) != S_OK) {
		PrintLastError("GetSpeakerConfig()");
		return FALSE;
	}

	if (ds) {
		ds->Release();
	}
	if (guid) {
		LocalFree(guid);
	}

	switch (DSSPEAKER_CONFIG(speakerConfig)) {
		case DSSPEAKER_STEREO:  currentChannelCount = 2; return TRUE;
		case DSSPEAKER_QUAD:    currentChannelCount = 4; return TRUE;
		case DSSPEAKER_5POINT1: currentChannelCount = 6; return TRUE;
		case DSSPEAKER_7POINT1: currentChannelCount = 8; return TRUE;
	}

	return FALSE;
}

BOOL setCurrentChannelConfig()
{
	IDirectSound8* ds;
	DWORD          speakerConfig;
	LPGUID         guid = NULL;

	DirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumProc, (VOID*)&guid);

	if (DirectSoundCreate8(guid, &ds, NULL) != S_OK) {
		PrintLastError("DirectSoundCreate8()");
		return FALSE;
	}


	ds->Initialize(NULL);

	switch (currentChannelCount) {
		case 2: speakerConfig = DSSPEAKER_STEREO;  break;
		case 4: speakerConfig = DSSPEAKER_QUAD;    break;
		case 6: speakerConfig = DSSPEAKER_5POINT1; break;
		case 8: speakerConfig = DSSPEAKER_7POINT1; break;
	}

	if (ds->SetSpeakerConfig(speakerConfig) != S_OK) {
		PrintLastError("SetSpeakerConfig()");
		return FALSE;
	}

	if (ds) {
		ds->Release();
	}
	if (guid) {
		LocalFree(guid);
	}


	return FALSE;
}

BOOL getDeviceInfo(const GUID* category, CMIDEV* pDev)
{
	TCHAR  szServiceName[128];
	int    nIndex = 0;

	pDev->Info = SetupDiGetClassDevs(category, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (pDev->Info == INVALID_HANDLE_VALUE) {
		PrintLastError("SetupDiGetClassDevs()");
		return FALSE;
	}

	pDev->InfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	while (SetupDiEnumDeviceInfo(pDev->Info, nIndex, &(pDev->InfoData))) {
		if (!SetupDiGetDeviceRegistryProperty(pDev->Info, &(pDev->InfoData), SPDRP_SERVICE, NULL, (PBYTE)szServiceName, sizeof(szServiceName), NULL)) {
			PrintLastError("SetupDiGetDeviceRegistryProperty()");
			SetupDiDestroyDeviceInfoList(pDev->Info);
	    	pDev->Info = NULL;
			return FALSE;
		}

		if (CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, szServiceName, -1, TEXT("cmipci"), -1) == CSTR_EQUAL) {
			return TRUE;
		}
		nIndex++;
	}

	SetupDiDestroyDeviceInfoList(pDev->Info);
	pDev->Info = NULL;
	return FALSE;
}

BOOL getDeviceInterfaceDetail(const GUID* category, CMIDEV* pDev)
{
	SP_DEVICE_INTERFACE_DATA  deviceInterfaceData;
	DWORD                     dataSize = 0;
	BOOL                      result;
	PTSTR                     pnpStr = NULL;
	HDEVINFO                  hDevInfoWithInterface;
	SP_DEVICE_INTERFACE_DATA  DeviceInterfaceData;
	ULONG                     ulDeviceInterfaceDetailDataSize = 0;

	// get the PnP string
	SetupDiGetDeviceInstanceId(pDev->Info, &(pDev->InfoData), NULL, 0, &dataSize);
	if ((GetLastError() != ERROR_INSUFFICIENT_BUFFER) || (!dataSize)) {
		PrintLastError("SetupDiGetDeviceInstanceId()");
		return FALSE;
	}
	pnpStr = (PTSTR)LocalAlloc(LPTR, dataSize * sizeof(TCHAR));
	if (!pnpStr) {
		PrintLastError("LocalAlloc()");
		return FALSE;
	}
	result = SetupDiGetDeviceInstanceId(pDev->Info, &(pDev->InfoData), pnpStr, dataSize, NULL);
	if (!result) {
		PrintLastError("SetupDiGetDeviceInstanceId()");
		LocalFree(pnpStr);
		return FALSE;
	}
	hDevInfoWithInterface = SetupDiGetClassDevs(&KSCATEGORY_TOPOLOGY, pnpStr, NULL, DIGCF_DEVICEINTERFACE);
	LocalFree(pnpStr);
	if (hDevInfoWithInterface == INVALID_HANDLE_VALUE) {
		PrintLastError("SetupDiGetClassDevs()");
		return FALSE;
	}

	// get the device interface data
	DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
	result = SetupDiEnumDeviceInterfaces(hDevInfoWithInterface, NULL, &KSCATEGORY_TOPOLOGY, 0, &DeviceInterfaceData);
	if (!result)	{
		PrintLastError("SetupDiEnumDeviceInterfaces()");
		SetupDiDestroyDeviceInfoList(hDevInfoWithInterface);
		return FALSE;
	}

	// get the device interface detail data
	dataSize = 0;
	SetupDiGetDeviceInterfaceDetail(hDevInfoWithInterface, &DeviceInterfaceData, NULL, 0, &dataSize, NULL);
	if ((GetLastError() != ERROR_INSUFFICIENT_BUFFER) || (!dataSize)) {
		PrintLastError("SetupDiGetDeviceInterfaceDetail()");
		SetupDiDestroyDeviceInfoList(hDevInfoWithInterface);
		return FALSE;
	}
	pDev->InterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, dataSize);
	if (!pDev->InterfaceDetailData) {
		PrintLastError("LocalAlloc()");
		SetupDiDestroyDeviceInfoList(hDevInfoWithInterface);
		return FALSE;
	}
	pDev->InterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	result = SetupDiGetDeviceInterfaceDetail(hDevInfoWithInterface, &DeviceInterfaceData, pDev->InterfaceDetailData, dataSize, NULL, NULL);
	SetupDiDestroyDeviceInfoList(hDevInfoWithInterface);
	if (!result) {
		PrintLastError("SetupDiGetDeviceInterfaceDetail()");
		LocalFree(pDev->InterfaceDetailData);
		pDev->InterfaceDetailData = NULL;
		return FALSE;
	}

	return TRUE;

}

BOOL getDriverData(CMIDEV* pDev)
{
	BOOL       result;
	HANDLE     hDevice;
	KSPROPERTY KSProp;
	DWORD      dataSize;

	hDevice = CreateFile(pDev->InterfaceDetailData->DevicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDevice == INVALID_HANDLE_VALUE) {
		PrintLastError("CreateFile()");
		return FALSE;
	}
	KSProp.Set   = KSPROPSETID_CMI;
	KSProp.Flags = KSPROPERTY_TYPE_GET;
	KSProp.Id    = KSPROPERTY_CMI_GET;
	result = DeviceIoControl(hDevice, IOCTL_KS_PROPERTY, &KSProp, sizeof(KSProp), &cmiData, sizeof(cmiData), &dataSize, NULL);
	CloseHandle(hDevice);

	if (!result) {
		PrintLastError("DeviceIoControl()");
		return FALSE;
	}

	return TRUE;
}

BOOL setDriverData(CMIDEV* pDev)
{
	BOOL       result;
	HANDLE     hDevice;
	KSPROPERTY KSProp;
	DWORD      dataSize;

	hDevice = CreateFile(pDev->InterfaceDetailData->DevicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDevice == INVALID_HANDLE_VALUE) {
		PrintLastError("CreateFile()");
		return FALSE;
	}
	KSProp.Set   = KSPROPSETID_CMI;
	KSProp.Flags = KSPROPERTY_TYPE_SET;
	KSProp.Id    = KSPROPERTY_CMI_SET;
	result = DeviceIoControl(hDevice, IOCTL_KS_PROPERTY, &KSProp, sizeof(KSProp), &cmiData, sizeof(cmiData), &dataSize, NULL);
	CloseHandle(hDevice);

	if (!result) {
		PrintLastError("DeviceIoControl()");
		return FALSE;
	}

	return TRUE;
}

void cleanUp()
{
	stopTestTone();
	if (cmiTopologyDev.Info) {
		SetupDiDestroyDeviceInfoList(cmiTopologyDev.Info);
		cmiTopologyDev.Info = NULL;
	}
	if (cmiTopologyDev.InterfaceDetailData) {
		LocalFree(cmiTopologyDev.InterfaceDetailData);
		cmiTopologyDev.InterfaceDetailData = NULL;
	}
	if (hURLFont) {
		DeleteObject(hURLFont); //hm?
		hURLFont = NULL;
	}
}

BOOL openDevice()
{
	if (!getDeviceInfo(&KSCATEGORY_TOPOLOGY, &cmiTopologyDev)) {
		PrintLastError("getDeviceInfo()");
		return FALSE;
	}

	if (!getDeviceInterfaceDetail(&KSCATEGORY_TOPOLOGY, &cmiTopologyDev)) {
		PrintLastError("getDeviceInterfaceDetail()");
		return FALSE;
	}

	return TRUE;
}

void updateChannelBoxes(HWND hWnd)
{
	switch (SendMessage(GetDlgItem(hWndChild[0], IDCB_CHANNELCONFIG), CB_GETCURSEL, 0, 0)) {
		case 0: // stereo
			ShowWindow(GetDlgItem(hWndChild[0], IDC_BLEFT), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_BRIGHT), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CENTER), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_SUB), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CLEFT), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CRIGHT), SW_HIDE);
			SetDlgItemText(hWnd, IDT_SWAPJACKS, "");
			break;
		case 1: // quad
			ShowWindow(GetDlgItem(hWndChild[0], IDC_BLEFT), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_BRIGHT), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CENTER), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_SUB), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CLEFT), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CRIGHT), SW_HIDE);
			SetDlgItemText(hWnd, IDT_SWAPJACKS, "");
			break;
		case 2: // 5.1
			ShowWindow(GetDlgItem(hWndChild[0], IDC_BLEFT), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_BRIGHT), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CENTER), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_SUB), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CLEFT), SW_HIDE);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CRIGHT), SW_HIDE);
			SetDlgItemText(hWnd, IDT_SWAPJACKS, "BL/BR and C/LFE jacks are swapped!");
			break;
		case 3: // 7.1
			ShowWindow(GetDlgItem(hWndChild[0], IDC_BLEFT), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_BRIGHT), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CENTER), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_SUB), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CLEFT), SW_SHOW);
			ShowWindow(GetDlgItem(hWndChild[0], IDC_CRIGHT), SW_SHOW);
			SetDlgItemText(hWnd, IDT_SWAPJACKS, "BL/BR and C/LFE jacks are swapped!");
			break;
	}
}


BOOL setDlgItems(HWND hWnd)
{
	HWND hWndItem;
	char buffer[127];

	if (!getDriverData(&cmiTopologyDev)) {
		PrintLastError("getDriverData()");
		return FALSE;
	}

	// 'About' tab
	SetWindowText(GetDlgItem(hWndChild[NUM_TABS-1], IDC_VERSION), cmiData.driverVersion);
	wsprintf(buffer, "%d", cmiData.hardwareRevision);
	SetWindowText(GetDlgItem(hWndChild[NUM_TABS-1], IDC_HWREV), buffer);
	wsprintf(buffer, "%d", cmiData.maxChannels);
	SetWindowText(GetDlgItem(hWndChild[NUM_TABS-1], IDC_MAXCHAN), buffer);
	wsprintf(buffer, "%04X", cmiData.IOBase);
	SetWindowText(GetDlgItem(hWndChild[NUM_TABS-1], IDC_BASEADR), buffer);
	wsprintf(buffer, "%04X", cmiData.MPUBase);
	SetWindowText(GetDlgItem(hWndChild[NUM_TABS-1], IDC_MPUADR), buffer);

	// channel config combobox
	hWndItem = GetDlgItem(hWndChild[0], IDCB_CHANNELCONFIG);
	SendMessage(hWndItem, CB_RESETCONTENT, 0, 0);
	if (cmiData.maxChannels >= 2) {
		SendMessage(hWndItem, CB_ADDSTRING, 0, (LPARAM)"Stereo (2.0)");
	}
	if (cmiData.maxChannels >= 4) {
		SendMessage(hWndItem, CB_ADDSTRING, 0, (LPARAM)"Quadrophonic (4.0)");
	}
	if (cmiData.maxChannels >= 6) {
		SendMessage(hWndItem, CB_ADDSTRING, 0, (LPARAM)"5.1 Surround");
	}
	if (cmiData.maxChannels >= 8) {
		SendMessage(hWndItem, CB_ADDSTRING, 0, (LPARAM)"7.1 Surround");
	}
	getCurrentChannelConfig();
	SendMessage(hWndItem, CB_SETCURSEL, (currentChannelCount/2)-1, 0);
	updateChannelBoxes(hWnd);

	// checkboxes
	SendMessage(GetDlgItem(hWndChild[0], IDC_EN_PCMDAC),      BM_SETCHECK, (cmiData.enablePCMDAC        ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[0], IDC_EXCH_FB),        BM_SETCHECK, (cmiData.exchangeFrontBack   ? BST_CHECKED : BST_UNCHECKED), 0);

	SendMessage(GetDlgItem(hWndChild[1], IDC_EN_SPDO),        BM_SETCHECK, (cmiData.enableSPDO          ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[1], IDC_EN_SPDO5V),      BM_SETCHECK, (cmiData.enableSPDO5V        ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[1], IDC_EN_SPDCOPYRHT),  BM_SETCHECK, (cmiData.enableSPDOCopyright ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[1], IDC_EN_SPDI),        BM_SETCHECK, (cmiData.enableSPDI          ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[1], IDC_SEL_SPDIFI),     BM_SETCHECK, (cmiData.select2ndSPDI       ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[1], IDC_INV_SPDIFI),     BM_SETCHECK, (cmiData.invertPhaseSPDI     ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[1], IDC_POLVALID),       BM_SETCHECK, (cmiData.invertValidBitSPDI  ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[1], IDC_LOOP_SPDF),      BM_SETCHECK, (cmiData.loopSPDI            ? BST_CHECKED : BST_UNCHECKED), 0);

	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_441_PCM),    BM_SETCHECK, ((cmiData.formatMask & FMT_441_PCM)    ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_480_PCM),    BM_SETCHECK, ((cmiData.formatMask & FMT_480_PCM)    ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_882_PCM),    BM_SETCHECK, ((cmiData.formatMask & FMT_882_PCM)    ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_960_PCM),    BM_SETCHECK, ((cmiData.formatMask & FMT_960_PCM)    ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_441_MULTI_PCM),BM_SETCHECK, ((cmiData.formatMask & FMT_441_MULTI_PCM) ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_480_MULTI_PCM),BM_SETCHECK, ((cmiData.formatMask & FMT_480_MULTI_PCM) ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_882_MULTI_PCM),BM_SETCHECK, ((cmiData.formatMask & FMT_882_MULTI_PCM) ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_960_MULTI_PCM),BM_SETCHECK, ((cmiData.formatMask & FMT_960_MULTI_PCM) ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_441_DOLBY),  BM_SETCHECK, ((cmiData.formatMask & FMT_441_DOLBY) ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_480_DOLBY),  BM_SETCHECK, ((cmiData.formatMask & FMT_480_DOLBY) ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_882_DOLBY),  BM_SETCHECK, ((cmiData.formatMask & FMT_882_DOLBY) ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_960_DOLBY),  BM_SETCHECK, ((cmiData.formatMask & FMT_960_DOLBY) ? BST_CHECKED : BST_UNCHECKED), 0);

	// radioboxes
	SendMessage(GetDlgItem(hWndChild[0], IDC_EN_BASS2LINE),   BM_SETCHECK, (cmiData.enableBass2Line     ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[0], IDC_EN_CENTER2LINE), BM_SETCHECK, (cmiData.enableCenter2Line   ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[0], IDC_EN_REAR2LINE),   BM_SETCHECK, (cmiData.enableRear2Line     ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[0], IDC_NOROUTE_LINE),   BM_SETCHECK, ((!cmiData.enableCenter2Line && !cmiData.enableBass2Line && !cmiData.enableRear2Line) ? BST_CHECKED : BST_UNCHECKED), 0);

	SendMessage(GetDlgItem(hWndChild[0], IDC_EN_CENTER2MIC),  BM_SETCHECK, (cmiData.enableCenter2Mic    ? BST_CHECKED : BST_UNCHECKED), 0);
	SendMessage(GetDlgItem(hWndChild[0], IDC_NOROUTE_MIC),    BM_SETCHECK, (!cmiData.enableCenter2Mic   ? BST_CHECKED : BST_UNCHECKED), 0);


	return TRUE;
}

BOOL applySettings()
{
	cmiData.enablePCMDAC        = (SendMessage(GetDlgItem(hWndChild[0], IDC_EN_PCMDAC),      BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.exchangeFrontBack   = (SendMessage(GetDlgItem(hWndChild[0], IDC_EXCH_FB),        BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.enableBass2Line     = (SendMessage(GetDlgItem(hWndChild[0], IDC_EN_BASS2LINE),   BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.enableCenter2Line   = (SendMessage(GetDlgItem(hWndChild[0], IDC_EN_CENTER2LINE), BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.enableRear2Line     = (SendMessage(GetDlgItem(hWndChild[0], IDC_EN_REAR2LINE),   BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.enableCenter2Mic    = (SendMessage(GetDlgItem(hWndChild[0], IDC_EN_CENTER2MIC),  BM_GETCHECK, 0, 0) == BST_CHECKED);

	cmiData.enableSPDO          = (SendMessage(GetDlgItem(hWndChild[1], IDC_EN_SPDO),        BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.enableSPDO5V        = (SendMessage(GetDlgItem(hWndChild[1], IDC_EN_SPDO5V),      BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.enableSPDOCopyright = (SendMessage(GetDlgItem(hWndChild[1], IDC_EN_SPDCOPYRHT),  BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.enableSPDI          = (SendMessage(GetDlgItem(hWndChild[1], IDC_EN_SPDI),        BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.select2ndSPDI       = (SendMessage(GetDlgItem(hWndChild[1], IDC_SEL_SPDIFI),     BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.invertPhaseSPDI     = (SendMessage(GetDlgItem(hWndChild[1], IDC_INV_SPDIFI),     BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.invertValidBitSPDI  = (SendMessage(GetDlgItem(hWndChild[1], IDC_POLVALID),       BM_GETCHECK, 0, 0) == BST_CHECKED);
	cmiData.loopSPDI            = (SendMessage(GetDlgItem(hWndChild[1], IDC_LOOP_SPDF),      BM_GETCHECK, 0, 0) == BST_CHECKED);

	cmiData.formatMask          = 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_441_PCM),  BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_441_PCM : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_480_PCM),  BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_480_PCM : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_882_PCM),  BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_882_PCM : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_960_PCM),  BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_960_PCM : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_441_MULTI_PCM), BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_441_MULTI_PCM : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_480_MULTI_PCM), BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_480_MULTI_PCM : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_882_MULTI_PCM), BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_882_MULTI_PCM : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_960_MULTI_PCM), BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_960_MULTI_PCM : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_441_DOLBY), BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_441_DOLBY : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_480_DOLBY), BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_480_DOLBY : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_882_DOLBY), BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_882_DOLBY : 0;
	cmiData.formatMask          |= (SendMessage(GetDlgItem(hWndChild[2], IDC_FMT_960_DOLBY), BM_GETCHECK, 0, 0) == BST_CHECKED) ? FMT_960_DOLBY : 0;

	currentChannelCount = (int)(SendMessage(GetDlgItem(hWndChild[0], IDCB_CHANNELCONFIG), CB_GETCURSEL, 0, 0)+1)*2;

	return (setDriverData(&cmiTopologyDev) && setCurrentChannelConfig());
}

BOOL initDialog(HWND hWnd)
{
	HICON   hIcon;
	TC_ITEM tci;
	int     i;

	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP_ICON));
	SendMessage(hWnd, WM_SETICON, (LPARAM) ICON_BIG, (WPARAM) hIcon);
	hURLFont = 0;

	hWndTab = GetDlgItem(hWnd,IDC_TAB);

	ZeroMemory(&tci, sizeof(TC_ITEM));
	tci.mask        = TCIF_TEXT;
	for (i=0;i<NUM_TABS;i++) {
		tci.pszText = tabsName[i];
		if (TabCtrl_InsertItem(hWndTab, i, &tci) == -1) {
			PrintLastError("TabCtrl_InsertItem()");
			return FALSE;
		}
		hWndChild[i] = CreateDialogParam(hInst, MAKEINTRESOURCE(tabsResource[i]), hWndTab, (DLGPROC)TabDlgProc, 0);
	}

	hURLFont = CreateFont(20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, VARIABLE_PITCH | FF_SWISS, "MS Shell Dlg");
	SendMessage(GetDlgItem(hWndChild[NUM_TABS-1], IDC_URL2), WM_SETFONT, (WPARAM)hURLFont, TRUE);

	currentTab = 0;
	ShowWindow(hWndChild[0], SW_SHOWDEFAULT);

	if (!openDevice()) {
		PrintLastError("openDevice()");
		return FALSE;
	}
	return setDlgItems(hWnd);
}

BOOL changeTab(LPNMHDR lpnmhdr)
{
	if (lpnmhdr->code != TCN_SELCHANGE) {
		return FALSE;
	}
	ShowWindow(hWndChild[currentTab], SW_HIDE);
	currentTab = SendMessage(hWndTab, TCM_GETCURSEL, 0, 0);
	ShowWindow(hWndChild[currentTab], SW_SHOWDEFAULT);
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			if (!initDialog(hWnd)) {
				PostQuitMessage(0);
			}
			return TRUE;
		case WM_CLOSE:
			DestroyWindow(hWnd);
			return TRUE;
		case WM_NOTIFY:
			return changeTab((LPNMHDR)lParam);
		case WM_DESTROY:
			cleanUp();
			PostQuitMessage(0);
			return TRUE;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDB_CLOSE) {
				PostQuitMessage(0);
				return TRUE;
			}
			if (LOWORD(wParam) == IDB_APPLY) {
				applySettings();
				setDlgItems(hWnd);
				return TRUE;
			}
			break;
	}
	return 0;
}

void openURL(int control)
{
	char buffer[127];
	GetWindowText(GetDlgItem(hWndChild[3], control), buffer, sizeof(buffer));
	ShellExecute(hWndMain, "open", buffer, NULL, NULL, SW_SHOWNORMAL);
}

LRESULT CALLBACK TabDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDB_STARTSTOP:
					if (stopTestTone()) {
						SetDlgItemText(hWndChild[0], IDB_STARTSTOP, "&Start");
						return TRUE;
					}
					if (playTestTone()) {
						SetDlgItemText(hWndChild[0], IDB_STARTSTOP, "&Stop");
						return TRUE;
					}
					break;
				case IDC_URL1:
				case IDC_URL2:
					openURL(LOWORD(wParam));
					break;
			}
		case WM_CTLCOLORSTATIC:
			if ( (GetDlgItem(hWndChild[3], IDC_URL1) == (HANDLE)lParam) || (GetDlgItem(hWndChild[3], IDC_URL2) == (HANDLE)lParam) ) {
				SetTextColor((HDC)wParam, 0xFF0000);
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
			}
	}

	return 0;
}

void printUsage()
{
	unsigned char usage[] = "/h - print this help message\r\n" \
	                        "/enable71Mode - change channel configuration to 7.1\r\n" \
	                        "/enable51Mode - change channel configuration to 5.1\r\n" \
	                        "/enable40Mode - change channel configuration to 4.0 (Quad)\r\n" \
	                        "/enable20Mode - change channel configuration to 2.0 (Stereo)\r\n" \
	                        "/enableSPDIFo - enable SPDIF-out\r\n" \
	                        "/disableSPDIFo - disable SPDIF-out\r\n"\
	                        "/enableSPDIFi - enable SPDIF-in recording\r\n" \
	                        "/disableSPDIFi - disable SPDIF-in recording\r\n";

	MessageBox(NULL, (LPCSTR)usage, TEXT("Usage Help"), MB_ICONINFORMATION | MB_OK);
	return;
}

void deleteDriverFiles() {
	TCHAR SysDir[MAX_PATH];
	unsigned int len;
	if (GetSystemDirectory(SysDir, sizeof(SysDir))==0) {
		PrintLastError("GetSystemDirectory()");
		return;
	}
	len = strlen(SysDir);

	strcat(SysDir, "\\cmicpl.cpl");
	if (!DeleteFile(SysDir)) {
		MoveFileEx(SysDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	}
	SysDir[len] = 0;

	strcat(SysDir, "\\cmicontrol.exe");
	if (!DeleteFile(SysDir)) {
		MoveFileEx(SysDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	}
}


void performUninstall() {
	deleteDriverFiles();
	RegDeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CMIDriver");
	MessageBox(NULL, "The CMI driver applications were successfully removed from your computer!", "CMIDriver", MB_ICONINFORMATION);
	ExitProcess(0);
}

bool checkToken(char* token) {
	if ((strcmp(token, "?")==0) || (strcmp(token, "H")==0)) {
		printUsage();
		return TRUE;
	} else
	if (strcmp(token, "ENABLE71MODE")==0) {
		currentChannelCount = 8;
	} else
	if (strcmp(token, "ENABLE51MODE")==0) {
		currentChannelCount = 6;
	} else
	if ((strcmp(token, "ENABLE40MODE")==0) || (strcmp(token, "ENABLEQUADMODE")==0) || (strcmp(token, "QUAD")==0) ) {
		currentChannelCount = 4;
	} else
	if ((strcmp(token, "ENABLE20MODE")==0) || (strcmp(token, "ENABLESTEREOMODE")==0) || (strcmp(token, "STEREO")==0) ) {
		currentChannelCount = 2;
	} else
	if (strcmp(token, "ENABLESPDIFO")==0) {
		cmiData.enableSPDO = TRUE;
	} else
	if (strcmp(token, "DISABLESPDIFO")==0) {
		cmiData.enableSPDO = FALSE;
	} else
	if (strcmp(token, "ENABLESPDIFI")==0) {
		cmiData.enableSPDI = TRUE;
	} else
	if (strcmp(token, "DISABLESPDIFI")==0) {
		cmiData.enableSPDI = FALSE;
	} else
	if (strcmp(token, "UNINSTALL")==0) {
		performUninstall();
	}
	return FALSE;
}

int parseArguments(LPSTR szCmdLine) {
	BOOL inToken = false;
	int  i = 0, j;
	char token[MAX_TOKEN_SIZE];

	if (!openDevice()) {
		return FALSE;
	}

	if (!getDriverData(&cmiTopologyDev)) {
		PrintLastError("getDriverData()");
		return FALSE;
	}
	if (!getCurrentChannelConfig()) {
		PrintLastError("getCurrentChannelConfig()");
		return FALSE;
	}

	while (szCmdLine[i]) {
		if (inToken) {
			if (szCmdLine[i] == ' ') {
				inToken = false;
				token[j] = 0;
				if (checkToken(token)) {
					return TRUE;
				}
			} else {
				token[j] = (char)toupper(szCmdLine[i]);
				if (j < MAX_TOKEN_SIZE-1) {
					j++;
				}
			}
		} else {
			if ((szCmdLine[i] == '-') || (szCmdLine[i] == '/')) {
				j = 0;
				inToken = true;
			}
		}

		i++;
	}
	token[j] = 0;
	checkToken(token);
	return (setDriverData(&cmiTopologyDev) && setCurrentChannelConfig());
}

void InitURLControl()
{
	WNDCLASSEX wce;

	ZeroMemory(&wce, sizeof(wce));
	wce.cbSize = sizeof(WNDCLASSEX);
	if (GetClassInfoEx(hInst, "Static", &wce)==0) {
		PrintLastError("GetClassInfoEx()");
        return;
	}

	wce.hCursor = LoadCursor(NULL, IDC_HAND);
	wce.hInstance = hInst;
	wce.lpszClassName = "URLLink";
	if (RegisterClassEx(&wce) == 0) {
		PrintLastError("RegisterClassEx()");
	}

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	WNDCLASSEX wce;
	MSG        msg;

	ZeroMemory(&cmiData, sizeof(CMIDATA));
	ZeroMemory(&cmiTopologyDev, sizeof(CMIDEV));
	hWave = NULL;

	if (szCmdLine) {
		if (strlen(szCmdLine) > 0) {
			int result = parseArguments(szCmdLine);
			cleanUp();
			return result;
		}
	}

	if (hWndMain = FindWindow("cmiControlPanel", NULL)) {
		SetForegroundWindow(hWndMain);
		return FALSE;
	}

	hInst = hInstance;
	InitCommonControls();
	CoInitialize(NULL);

	ZeroMemory(&wce, sizeof(WNDCLASSEX));
	wce.cbSize        = sizeof(WNDCLASSEX);
	wce.lpfnWndProc   = DefDlgProc;
	wce.style         = 0;
	wce.cbWndExtra    = DLGWINDOWEXTRA;
	wce.hInstance     = hInstance;
	wce.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wce.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wce.lpszClassName = "cmiControlPanel";
	wce.lpszMenuName  = NULL;
	wce.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
	wce.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    if(!RegisterClassEx(&wce)) {
		PrintLastError("RegisterClassEx()");
		return -1;
	}
	InitURLControl();

	hWndMain = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)WndProc, NULL);
	if (!hWndMain) {
		PrintLastError("CreateDialogParam()");
		return -1;
	}

	while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    return 0;
}
