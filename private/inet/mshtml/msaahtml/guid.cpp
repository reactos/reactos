//============================================================================
//		File:	GUID.CPP
//		Date: 	1/29/98
//		Desc:	This file is used to define, not declare, the MSAA-specific 
//				GUIDs such as IIDs for IAccessible, IAccessibleHandler, and
//				the type library IID. To ensure that these GUIDs are only
//				defined once in this project, and that unresolved external
//				link errors do not result, this file should be the only project
//				file that includes INITGUID.H, and it should *NOT* include the 
//				pre-compiled header file STDAFX.H. 
//
//		Notes:	Currently contains the defintion for the MSAAHTML.DLL CLSID.  
//				This should be removed by adding a coclass definition to
//				MSAAHTML.IDL.
//
//		Author:	Jay Clark
//============================================================================

//=======================================================================
//	Includes
//=======================================================================

#include <objbase.h>
#include <initguid.h>
#include <oleacc.h>			// contains MSAA-specific GUIDs
