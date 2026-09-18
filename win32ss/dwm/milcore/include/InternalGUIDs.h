// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Module Name:
*
*   This file is a common location for all our internal GUIDs.
*   It allows us to avoid complex include dependencies and to have a common
*   mechanism for linking them into our binary.
*
* Created:
*
*   10/28/2001 asecchia
*      Created it.
*
**************************************************************************/

#pragma once

// {B73B1159-A295-4c76-BB56-C18E282AE007}
DEFINE_GUID(IID_IRenderTargetInternal,
0xb73b1159, 0xa295, 0x4c76, 0xbb, 0x56, 0xc1, 0x8e, 0x28, 0x2a, 0xe0, 0x7);

// {B3DF038B-B57B-448f-B244-03B55C318277}
DEFINE_GUID(IID_IEffectInternal,
0xb3df038b, 0xb57b, 0x448f, 0xb2, 0x44, 0x3, 0xb5, 0x5c, 0x31, 0x82, 0x77);

// {0CCD7824-DC16-4d09-BCA8-6B09C4EF5535}
DEFINE_GUID(IID_CMetaBitmapRenderTarget,
0xccd7824, 0xdc16, 0x4d09, 0xbc, 0xa8, 0x6b, 0x9, 0xc4, 0xef, 0x55, 0x35);

//
// This was the old value of IID_IMILResourceCache that we choose not to use
// any more because it was part of CBitmap which we shared with WIC. We 
// are changing it to ensure that we never QI() a WIC bitmap to an internal
// interface
//
// {181FA62C-0133-43D4-8D5C-639C3487783F}
// DEFINE_GUID(IID_IMILResourceCache,
// 0x181fa62c, 0x0133, 0x43d4, 0x8d, 0x5c, 0x63, 0x9c, 0x34, 0x87, 0x78, 0x3f);
//

// {AB98C452-963F-415b-B9A1-4A323BD25D24}
DEFINE_GUID(IID_IMILResourceCache, 
0xab98c452, 0x963f, 0x415b, 0xb9, 0xa1, 0x4a, 0x32, 0x3b, 0xd2, 0x5d, 0x24);

// {90830ae8-7b8b-46bc-9854-1b256548a928}
DEFINE_GUID(IID_IAVSurfaceRenderer,
0x90830ae8, 0x7b8b, 0x46bc, 0x98, 0x54, 0x1b, 0x25, 0x65, 0x48, 0xa9, 0x28);

// {e6f1cc74-a0eb-4ebe-8241-089d1cb079d7}
DEFINE_GUID(IID_IMILSurfaceRendererProvider,
0xe6f1cc74, 0xa0eb, 0x4ebe, 0x82, 0x41, 0x08, 0x9d, 0x1c, 0xb0, 0x79, 0xd7);

// {07E501BA-A406-452d-BEC9-474A1CB8CB66}
DEFINE_GUID(IID_CMFMediaBuffer,
0x7e501ba, 0xa406, 0x452d, 0xbe, 0xc9, 0x47, 0x4a, 0x1c, 0xb8, 0xcb, 0x66);

// {3a55501a-bdcc-4e63-96bc-4ddb6f44ccdd}
DEFINE_GUID(IID_IManagedStream,
0x3a55501a, 0xbdcc, 0x4e63, 0x96, 0xbc, 0x4d, 0xdb, 0x6f, 0x44, 0xcc, 0xdd);



