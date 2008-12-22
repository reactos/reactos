/*
 * PROJECT:     ReactOS Shell Extensions
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        dll\win32\shellext\devcpux\processor.c
 * PURPOSE:
 * COPYRIGHT:   Copyright 2007 Christoph von Wittich <Christoph_vW@ReactOS.org>
 *
 */

#include <windows.h>
#include <setupapi.h>
#include <powrprof.h>

#include "resource.h"

HINSTANCE g_hInstance = NULL;
LRESULT CALLBACK ProcessorDlgProc (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

BOOL
APIENTRY
DllMain (HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_ATTACH:
		case DLL_PROCESS_DETACH:
			break;
	}

	g_hInstance = (HINSTANCE) hInstance;
	return TRUE;
}


BOOL
APIENTRY
PropSheetExtProc(PSP_PROPSHEETPAGE_REQUEST PropPageRequest, LPFNADDPROPSHEETPAGE fAddFunc, LPARAM lParam)
{
	PROPSHEETPAGE PropSheetPage;
	HPROPSHEETPAGE hPropSheetPage;

	if(PropPageRequest->PageRequested != SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
		return FALSE;

	if ((!PropPageRequest->DeviceInfoSet) || (!PropPageRequest->DeviceInfoData))
		return FALSE;

	ZeroMemory(&PropSheetPage, sizeof(PROPSHEETPAGE));
	PropSheetPage.dwSize = sizeof(PROPSHEETPAGE);
	PropSheetPage.hInstance = g_hInstance;
	PropSheetPage.pszTemplate = MAKEINTRESOURCE(DLG_PROCESSORINFO);
	PropSheetPage.pfnDlgProc = ProcessorDlgProc;

	hPropSheetPage = CreatePropertySheetPage(&PropSheetPage);
	if(!hPropSheetPage)
		return FALSE;

	if(!(fAddFunc)(hPropSheetPage, lParam)) {
		DestroyPropertySheetPage (hPropSheetPage);
		return FALSE;
	}

	return TRUE;
}

void
AddFeature(WCHAR* szFeatures, WCHAR* Feature, BOOL* bFirst)
{
	if (!*bFirst)
		wcscat(szFeatures, L", ");
	*bFirst = FALSE;
	wcscat(szFeatures, Feature);
}

LRESULT
CALLBACK
ProcessorDlgProc (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage) {
		case WM_INITDIALOG:
		{
			WCHAR szFeatures[MAX_PATH] = L"";
			WCHAR szModel[3];
			WCHAR szStepping[3];
			WCHAR szCurrentMhz[10];
			BOOL bFirst = TRUE;
			SYSTEM_INFO SystemInfo;
			PROCESSOR_POWER_INFORMATION PowerInfo;

			if (IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE))
				AddFeature(szFeatures, L"MMX", &bFirst);
			if (IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE))
				AddFeature(szFeatures, L"SSE", &bFirst);
			if (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
				AddFeature(szFeatures, L"SSE2", &bFirst);
			/*if (IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE))
				AddFeature(szFeatures, L"SSE3", &bFirst); */
			if (IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE))
				AddFeature(szFeatures, L"3DNOW", &bFirst);

			SetDlgItemTextW(hDlg, IDC_FEATURES, szFeatures);

			GetSystemInfo(&SystemInfo);

			wsprintf(szModel, L"%x", HIBYTE(SystemInfo.wProcessorRevision));
			wsprintf(szStepping, L"%d", LOBYTE(SystemInfo.wProcessorRevision));

			SetDlgItemTextW(hDlg, IDC_MODEL, szModel);
			SetDlgItemTextW(hDlg, IDC_STEPPING, szStepping);

			CallNtPowerInformation(11, NULL, 0, &PowerInfo, sizeof(PowerInfo));
			wsprintf(szCurrentMhz, L"%ld %s", PowerInfo.CurrentMhz, L"MHz");
			SetDlgItemTextW(hDlg, IDC_CORESPEED, szCurrentMhz);

			return TRUE;
		}
	}
	return FALSE;
}
