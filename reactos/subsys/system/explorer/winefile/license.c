/*
 * Copyright 2000 Martin Fuchs
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

#include "windows.h"

static const CHAR LicenseCaption[] = "LICENSE";
static const CHAR License[] =
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

static const CHAR NoWarrantyCaption[] = "NO WARRANTY";
static const CHAR NoWarranty[] =
"This library is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
"Lesser General Public License for more details.";

VOID WineLicense(HWND hwnd)
{
  MessageBoxA(hwnd, License, LicenseCaption, MB_ICONINFORMATION|MB_OK);
}

VOID WineWarranty(HWND hwnd)
{
  MessageBoxA(hwnd, NoWarranty, NoWarrantyCaption, MB_ICONEXCLAMATION|MB_OK);
}
