/*	File: D:\wacker\ext\pageext.hh (Created: 01-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:27p $
 */

//
// The class ID of this page extension class.
//	"{1B53F360-9A1B-1069-930C-00AA0030EBC8}"
//
// The class ID of the icon handleer
//
// {88895560-9AA2-1069-930E-00AA0030EBC8}

DEFINE_GUID(CLSID_SamplePageExt, 0x1B53F360, 0x9A1B, 0x1069, 0x93,0x0C,0x00,0xAA,0x00,0x30,0xEB,0xC8);
DEFINE_GUID(CLSID_SampleIconExt, 0x88895560, 0x9AA2, 0x1069, 0x93,0x0E,0x00,0xAA,0x00,0x30,0xEB,0xC8);

typedef HRESULT (CALLBACK FAR * LPFNCREATEINSTANCE)(LPUNKNOWN pUnkOuter,
	REFIID riid, LPVOID FAR* ppvObject);


void FSPage_AddPages(LPDATAOBJECT pdtobj,
			LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

void NETPage_AddPages(LPDATAOBJECT pdtobj,
		    LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

STDAPI SHCreateDefClassObject(REFIID riid, LPVOID FAR* ppv,
			 LPFNCREATEINSTANCE lpfnCI, UINT FAR * pcRefDll,
			 REFIID riidInst);

HRESULT CALLBACK IconExt_CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR*);

extern UINT g_cRefThisDll;		// Reference count of this DLL.
