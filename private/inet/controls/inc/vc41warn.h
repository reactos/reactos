//=--------------------------------------------------------------------------=
// VC41Warn.h
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// disables several new VC4.1 warnings that trip standard headers
//

// VC4.1 warning about bool
#pragma warning(disable:4237)

// VC4.1 warning - anachronism used
#pragma warning(disable:4229)
