// ComTestLoad.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iComTest.h>
#include <initguid.h>
#include <Objbase.h>

#include <iostream>
using namespace std;
// {C4CC78DB-8266-4ae7-8685-8EE05376CB09}
DEFINE_GUID(CLSID_ComTest, 0xc4cc78db, 0x8266, 0x4ae7, 0x86, 0x85, 0x8e, 0xe0, 0x53, 0x76, 0xcb, 0x9);

// {6DA36786-117F-449b-B210-194DCFCAF416}
DEFINE_GUID(CLSID_ComAggTest, 0x6da36786, 0x117f, 0x449b, 0xb2, 0x10, 0x19, 0x4d, 0xcf, 0xca, 0xf4, 0x16);

//{                            ECD4FC4D-   521C-   11D0-    B7    92-   00    A0    C9    03    12    E1}
//DEFINE_GUID(CLSID_ComTest, 0xECD4FC4D, 0x521C, 0x11D0, 0xB7, 0x92, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);

//const IID IID_IComTest = {0x10CDF249,0xA336,0x406F,{0xB4,0x72,0x20,0xF0,0x86,0x60,0xD6,0x09}};


int _tmain(int argc, _TCHAR* argv[])
{

	HRESULT hr = CoInitialize(NULL); 
	IComTest * testCom;
	hr = CoCreateInstance(CLSID_ComTest, NULL, CLSCTX_INPROC_SERVER, IID_IComTest, (LPVOID *)&testCom); 

	switch(hr)
	{
		case S_OK:
			cout << "testCom Loaded" << endl;
			break;
		case REGDB_E_CLASSNOTREG:
			cout << "testCom loading REGDB_E_CLASSNOTREG" << endl;
			return 0;
			break;
		case CLASS_E_NOAGGREGATION:
			cout << "testCom loading CLASS_E_NOAGGREGATION" << endl;
			return 0;
			break;
		case E_NOINTERFACE:
			cout << "testCom loading E_NOINTERFACE" << endl;
			return 0;
			break;
		case E_POINTER:
			cout << "testCom loading E_POINTER" << endl;
			return 0;
			break;
	}
	

	HRESULT hrTest=testCom->test();
	if(hrTest==S_OK)
	{
		cout << "testCom->test OK" << endl;
	}
	else
	{
		cout << "testCom->test failed" << endl;
		return 0;
	}








	IComAggTest * testAggCom;
	hr = CoCreateInstance(CLSID_ComAggTest, NULL, CLSCTX_INPROC_SERVER, IID_IComAggTest, (LPVOID *)&testAggCom); 

	switch(hr)
	{
		case S_OK:
			cout << "testAggCom Loaded" << endl;
			break;
		case REGDB_E_CLASSNOTREG:
			cout << "testAggCom loading REGDB_E_CLASSNOTREG" << endl;
			return 0;
			break;
		case CLASS_E_NOAGGREGATION:
			cout << "testAggCom loading CLASS_E_NOAGGREGATION" << endl;
			return 0;
			break;
		case E_NOINTERFACE:
			cout << "testAggCom loading E_NOINTERFACE" << endl;
			return 0;
			break;
		case E_POINTER:
			cout << "testAggCom loading E_POINTER" << endl;
			return 0;
			break;
	}
	
	hrTest=0;
	hrTest=testAggCom->test();
	if(hrTest==S_OK)
	{
		cout << "testAggCom->test OK" << endl;
	}
	else
	{
		cout << "testAggCom->test failed" << endl;
		return 0;
	}

	hrTest=0;
	hrTest=testAggCom->test2();
	if(hrTest==S_OK)
	{
		cout << "testAggCom->test2 OK" << endl;
	}
	else
	{
		cout << "testAggCom->test2 failed" << endl;
		return 0;
	}






	return 0;
}

