// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#define IFC(x) hr = x; if (FAILED(hr)) goto Cleanup
#define IFH(x) if ((x) == NULL) {hr = E_FAIL; goto Cleanup;}
#define IFCOOM(x) if ((x) == NULL) {hr = E_OUTOFMEMORY; goto Cleanup;}

#define ReleaseInterface(p) if (p) {p->Release(); p = NULL;}
