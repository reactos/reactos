// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <stdio.h> 
#include <windows.h>
#include <Objbase.h>

HINSTANCE dllHandle = NULL;
typedef  HRESULT (CALLBACK* DllCanUnloadNowType)();
typedef HRESULT (CALLBACK* DllGetClassObjectType)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
typedef HRESULT (CALLBACK* DllUnregisterServerType)();

DllCanUnloadNowType DllCanUnloadNowPtr = NULL;
DllGetClassObjectType DllGetClassObjectPtr = NULL;
DllUnregisterServerType DllUnregisterServerPtr = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	OutputDebugStringW(L"DllMain");
	if(!dllHandle)
	{
		OutputDebugStringW(L"!dllHandle");
		dllHandle = LoadLibrary(L"C:\\Windows\\System32\\browseui.dll");
		if(!dllHandle)
			OutputDebugStringW(L"dllHandle fail");
		else
			OutputDebugStringW(L"dllHandle work");
		DllCanUnloadNowPtr = (DllCanUnloadNowType)GetProcAddress(dllHandle,"DllCanUnloadNow");
		DllGetClassObjectPtr = (DllGetClassObjectType)GetProcAddress(dllHandle,"DllGetClassObject");
		DllUnregisterServerPtr = (DllUnregisterServerType)GetProcAddress(dllHandle,"DllUnregisterServer");
	}
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

STDAPI DllCanUnloadNow()
{
	if(!dllHandle)
	{
		return DllCanUnloadNowPtr();
	}
	return FALSE;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	FILE *file;
	OutputDebugStringW(L"DllGetClassObject");
	file = fopen("c:/browsui.txt","a+"); 
	fprintf(file,"DllGetClassObject ");

	LPOLESTR sIid = NULL; 
	HRESULT hr = StringFromIID(rclsid, &sIid); 
	fprintf(file,"rclsid = %S ", sIid);

	hr = StringFromIID(riid, &sIid); 
	fprintf(file,"riid = %S \n", sIid);

	fclose(file);


	if(dllHandle)
	{
		OutputDebugStringW(L"dllHandle good");
		if(!dllHandle)
			OutputDebugStringW(L"DllGetClassObjectPtr exists");
		return DllGetClassObjectPtr(rclsid, riid, ppv);
	}
	else
	{
		OutputDebugStringW(L"dllHandle fail");
	}
	return FALSE;
}

STDAPI DllUnregisterServer()
{
	if(!dllHandle)
	{
		return DllUnregisterServerPtr();
	}
	return FALSE;
}

