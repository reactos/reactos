/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "precomp.h"
#include "browseui_resource.h"
#include "newinterfaces.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include "addressband.h"
#include "addresseditbox.h"
#include "bandproxy.h"
#include "brandband.h"
#include "internettoolbar.h"
/*
TODO:
	Implement DllRegisterServer et al
	Move the module instances here
*/

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_SH_AddressBand, CAddressBand)
OBJECT_ENTRY(CLSID_AddressEditBox, CAddressEditBox)
OBJECT_ENTRY(CLSID_BandProxy, CBandProxy)
OBJECT_ENTRY(CLSID_BrandBand, CBrandBand)
OBJECT_ENTRY(CLSID_InternetToolbar, CInternetToolbar)
END_OBJECT_MAP()

