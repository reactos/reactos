/*
 *  Notepad (license.h)
 *
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
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

#include <windows.h>
#include "license.h"

VOID WineLicense(HWND Wnd)
{
  /* FIXME: should load strings from resources */
  LICENSE *License = &WineLicense_En;
  MessageBox(Wnd, License->License, License->LicenseCaption,
             MB_ICONINFORMATION | MB_OK);
}


VOID WineWarranty(HWND Wnd)
{
  /* FIXME: should load strings from resources */
  LICENSE *License = &WineLicense_En;
  MessageBox(Wnd, License->Warranty, License->WarrantyCaption,
             MB_ICONEXCLAMATION | MB_OK);
}

