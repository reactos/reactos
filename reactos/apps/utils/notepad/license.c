/*
 *  Notepad (license.h)
 *
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  To be distributed under the Wine License
 */

#include "windows.h"
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

