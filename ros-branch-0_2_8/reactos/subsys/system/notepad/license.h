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

VOID WineLicense(HWND hWnd);
VOID WineWarranty(HWND hWnd);

typedef struct
{
  LPCSTR License, LicenseCaption;
  LPCSTR Warranty, WarrantyCaption;
} LICENSE;

/*
extern LICENSE WineLicense_Ca;
extern LICENSE WineLicense_Cz;
extern LICENSE WineLicense_Da;
extern LICENSE WineLicense_De;
*/

extern LICENSE WineLicense_En;

/*
extern LICENSE WineLicense_Eo;
extern LICENSE WineLicense_Es;
extern LICENSE WineLicense_Fi;
extern LICENSE WineLicense_Fr;
extern LICENSE WineLicense_Hu;
extern LICENSE WineLicense_It;
extern LICENSE WineLicense_Ko;
extern LICENSE WineLicense_No;
extern LICENSE WineLicense_Pl;
extern LICENSE WineLicense_Po;
extern LICENSE WineLicense_Sw;
extern LICENSE WineLicense_Va;
*/
