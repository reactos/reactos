/*
 *  Notepad (license.h)
 *
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  To be distributed under the Wine License
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
