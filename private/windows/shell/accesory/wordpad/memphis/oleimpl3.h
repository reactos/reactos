// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


/////////////////////////////////////////////////////////////////////////////
// OLE data (like AUX_DATA)

struct OLE_DATA
{
	// OLE 1.0 clipboard formats
	UINT    cfNative, cfOwnerLink, cfObjectLink;

	// OLE 2.0 clipboard formats
	UINT    cfEmbeddedObject, cfEmbedSource, cfLinkSource;
	UINT    cfObjectDescriptor, cfLinkSourceDescriptor;
	UINT    cfFileName, cfFileNameW;

	//RichEdit formats
	UINT    cfRichTextFormat;
	UINT    cfRichTextAndObjects;

	OLE_DATA();
};

extern OLE_DATA _oleData;
