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

#define DEFINE_DAOGUID(name, l) \
    DEFINE_GUID(name, l, 0, 0x10, 0x80,0,0,0xAA,0,0x6D,0x2E,0xA4)

#define ADO_MAJOR	2		// major version of the ADO type library
#define ADO_VERSION	1.5

// Type library
DEFINE_DAOGUID(LIBID_CADO10,			0x00000200);
DEFINE_DAOGUID(LIBID_CADOR10,			0x00000300);

// Error
DEFINE_DAOGUID(IID_IADOError,            0x00000500);
DEFINE_DAOGUID(IID_IADOErrors,           0x00000501);

// Property
DEFINE_DAOGUID(IID_IADOProperty,         0x00000503);
DEFINE_DAOGUID(IID_IADOProperties,       0x00000504);

// Field
DEFINE_DAOGUID(IID_IADOField,            0x00000505);
DEFINE_DAOGUID(IID_IADOFields,           0x00000506);

// Command
DEFINE_DAOGUID(CLSID_CADOCommand,		0x00000507);
DEFINE_DAOGUID(IID_IADOCommand,			0x00000508);
DEFINE_DAOGUID(IID_IADOCommands,        0x00000509);

// Parameter
DEFINE_DAOGUID(CLSID_CADOParameter,		0x0000050B);
DEFINE_DAOGUID(IID_IADOParameter,        0x0000050C);
DEFINE_DAOGUID(IID_IADOParameters,       0x0000050D);

//Recordset
DEFINE_DAOGUID(CLSID_CADORecordset, 	 0x00000535);
DEFINE_DAOGUID(IID_IADORecordset,		 0x0000050E);
DEFINE_DAOGUID(IID_IADORecordsets,		 0x0000050F);
DEFINE_DAOGUID(IID_IADORecordsetConstruction,     0x00000283);

// Collections
DEFINE_DAOGUID(IID_IADOCollection,       0x00000512);
DEFINE_DAOGUID(IID_IADODynaCollection,   0x00000513);

// Connection
DEFINE_DAOGUID(CLSID_CADOConnection,	   0x00000514);
DEFINE_DAOGUID(IID_IADOConnection,		   0x00000515);
DEFINE_DAOGUID(IID_IADOConnections,		   0x00000518);

// Enums 
DEFINE_DAOGUID(IID_EnumCursorType,			0x0000051B);
DEFINE_DAOGUID(IID_EnumCursorOption,		0x0000051C);
DEFINE_DAOGUID(IID_EnumLockType,			0x0000051D);
DEFINE_DAOGUID(IID_EnumExecuteOption,		0x0000051E);
DEFINE_DAOGUID(IID_EnumDataType,			0x0000051F);
DEFINE_DAOGUID(IID_EnumConnectPrompt,		0x00000520);
DEFINE_DAOGUID(IID_EnumConnectMode,			0x00000521);
DEFINE_DAOGUID(IID_EnumPrepareOption,		0x00000522);
DEFINE_DAOGUID(IID_EnumIsolationLevel,		0x00000523);
DEFINE_DAOGUID(IID_EnumXactAttribute,		0x00000524);
DEFINE_DAOGUID(IID_EnumFieldAttribute,		0x00000525);
DEFINE_DAOGUID(IID_EnumEditMode,			0x00000526);
DEFINE_DAOGUID(IID_EnumRecordStatus,		0x00000527);
DEFINE_DAOGUID(IID_EnumPosition,			0x00000528);
DEFINE_DAOGUID(IID_EnumPropertyAttributes,	0x00000529);
DEFINE_DAOGUID(IID_EnumErrorValue,			0x0000052A);
DEFINE_DAOGUID(IID_EnumParameterAttributes,	0x0000052B);
DEFINE_DAOGUID(IID_EnumParameterDirection,	0x0000052C);
DEFINE_DAOGUID(IID_EnumFilterCriteria,		0x0000052D);
DEFINE_DAOGUID(IID_EnumCommandType,			0x0000052E);
DEFINE_DAOGUID(IID_EnumCursorLocation,		0x0000052F);
DEFINE_DAOGUID(IID_EnumEventStatus,			0x00000530);
DEFINE_DAOGUID(IID_EnumEventReason,			0x00000531);
DEFINE_DAOGUID(IID_EnumObjectState,			0x00000532);
DEFINE_DAOGUID(IID_EnumSchema,				0x00000533);
DEFINE_DAOGUID(IID_EnumMarshalOptions,		0x00000540);

#endif // _ADOID_H_
