#include <windows.h>
#include "license.h"

static const CHAR LicenseCaption_En[] = "LICENSE";
static const CHAR License_En[] =
"This library is free software; you can redistribute it and/or "
"modify it under the terms of the GNU Lesser General Public "
"License as published by the Free Software Foundation; either "
"version 2.1 of the License, or (at your option) any later version.\n"

"This library is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
"Lesser General Public License for more details.\n"

"You should have received a copy of the GNU Lesser General Public "
"License along with this library; if not, write to the Free Software "
"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA";

static const CHAR NoWarrantyCaption_En[] = "NO WARRANTY";
static const CHAR NoWarranty_En[] =
"This library is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
"Lesser General Public License for more details.";

LICENSE WineLicense_En = {License_En, LicenseCaption_En,
                          NoWarranty_En, NoWarrantyCaption_En};
