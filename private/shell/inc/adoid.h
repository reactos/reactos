//--------------------------------------------------------------------
// Microsoft ADO
//
// (c) 1996 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module	adoid.h | ADO Guids
//
// @devnote None
//--------------------------------------------------------------------
#ifndef _ADOID_H_
#define _ADOID_H_

// The following range of 255 guids has been reserved for the base objects.
// 	00000200-0000-0010-8000-00AA006D2EA4 - 000002FF-0000-0010-8000-00AA006D2EA4
// If you need more then please take a range from daoguid.txt and update that file in

#define DEFINE_ADOGUID(name, l) \
    DEFINE_GUID(name, l, 0, 0x10, 0x80,0,0,0xAA,0,0x6D,0x2E,0xA4)

#define ADO_MAJOR	2		// major version of the ADO type library
#define ADO_VERSION	2.0

// Type library
DEFINE_ADOGUID(LIBID_ADO20,				0x00000200);
DEFINE_ADOGUID(LIBID_ADOR20,			0x00000300);

#define LIBID_CADO10	LIBID_ADO20
#define LIBID_CADOR10	LIBID_ADOR20

// Error
DEFINE_ADOGUID(IID_IADOError,            0x00000500);
DEFINE_ADOGUID(IID_IADOErrors,           0x00000501);

// Property
DEFINE_ADOGUID(IID_IADOProperty,         0x00000503);
DEFINE_ADOGUID(IID_IADOProperties,       0x00000504);

// Field
DEFINE_ADOGUID(CLSID_CADOField,			 0x0000053A);
DEFINE_ADOGUID(IID_IADOField,            0x00000505);
DEFINE_ADOGUID(IID_IADOFields,           0x00000506);

// Command
DEFINE_ADOGUID(CLSID_CADOCommand,		0x00000507);
DEFINE_ADOGUID(IID_IADOCommand,			0x00000508);
DEFINE_ADOGUID(IID_IADOCommands,        0x00000509);

// Parameter
DEFINE_ADOGUID(CLSID_CADOParameter,		0x0000050B);
DEFINE_ADOGUID(IID_IADOParameter,        0x0000050C);
DEFINE_ADOGUID(IID_IADOParameters,       0x0000050D);

//Recordset
DEFINE_ADOGUID(CLSID_CADORecordset, 	 0x00000535);
DEFINE_ADOGUID(IID_IADORecordset,		 0x0000050E);
DEFINE_ADOGUID(IID_IADORecordsets,		 0x0000050F);
DEFINE_ADOGUID(IID_IADORecordsetConstruction,     0x00000283);

// Collections
DEFINE_ADOGUID(IID_IADOCollection,       0x00000512);
DEFINE_ADOGUID(IID_IADODynaCollection,   0x00000513);

// Connection
DEFINE_ADOGUID(CLSID_CADOConnection,	   0x00000514);
DEFINE_ADOGUID(IID_IADOConnection,		   0x00000515);
DEFINE_ADOGUID(IID_IADOConnections,		   0x00000518);

// events
// dispatch interfaces
DEFINE_ADOGUID(IID_IADORecordsetEvents,	   0x00000266);
DEFINE_ADOGUID(IID_IADOConnectionEvents,   0x00000400);
// vtable interfaces
DEFINE_ADOGUID(IID_IADORecordsetEventsVt,   0x00000403);
DEFINE_ADOGUID(IID_IADOConnectionEventsVt,  0x00000402);


#endif // _ADOID_H_
