/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           include/ntos/gditypes.h
 * PURPOSE:        Common GDI definitions
 * PROGRAMMER:     Eric Kohl <ekohl@rz-online.de>
 * UPDATE HISTORY:
 *                 25/06/2001: Created
*/

#ifndef __INCLUDE_NTOS_GDITYPES_H
#define __INCLUDE_NTOS_GDITYPES_H

#ifndef __USE_W32API

#define CCHDEVICENAME	(32)
#define CCHFORMNAME	(32)

typedef struct _devicemodeA
{
	BYTE dmDeviceName[CCHDEVICENAME];
	WORD dmSpecVersion;
	WORD dmDriverVersion;
	WORD dmSize;
	WORD dmDriverExtra;
	DWORD dmFields;
	short dmOrientation;
	short dmPaperSize;
	short dmPaperLength;
	short dmPaperWidth;
	short dmScale;
	short dmCopies;
	short dmDefaultSource;
	short dmPrintQuality;
	short dmColor;
	short dmDuplex;
	short dmYResolution;
	short dmTTOption;
	short dmCollate;
	BYTE dmFormName[CCHFORMNAME];
	WORD dmLogPixels;
	DWORD dmBitsPerPel;
	DWORD dmPelsWidth;
	DWORD dmPelsHeight;
	DWORD dmDisplayFlags;
	DWORD dmDisplayFrequency;
	DWORD dmICMMethod;
	DWORD dmICMIntent;
	DWORD dmMediaType;
	DWORD dmDitherType;
	DWORD dmICCManufacturer;
	DWORD dmICCModel;
} DEVMODEA,*LPDEVMODEA,*PDEVMODEA;

typedef struct _devicemodeW
{
	WCHAR dmDeviceName[CCHDEVICENAME];
	WORD dmSpecVersion;
	WORD dmDriverVersion;
	WORD dmSize;
	WORD dmDriverExtra;
	DWORD dmFields;
	short dmOrientation;
	short dmPaperSize;
	short dmPaperLength;
	short dmPaperWidth;
	short dmScale;
	short dmCopies;
	short dmDefaultSource;
	short dmPrintQuality;
	short dmColor;
	short dmDuplex;
	short dmYResolution;
	short dmTTOption;
	short dmCollate;
	WCHAR dmFormName[CCHFORMNAME];
	WORD dmLogPixels;
	DWORD dmBitsPerPel;
	DWORD dmPelsWidth;
	DWORD dmPelsHeight;
	DWORD dmDisplayFlags;
	DWORD dmDisplayFrequency;
	DWORD dmICMMethod;
	DWORD dmICMIntent;
	DWORD dmMediaType;
	DWORD dmDitherType;
	DWORD dmICCManufacturer;
	DWORD dmICCModel;
} DEVMODEW,*LPDEVMODEW,*PDEVMODEW;

#endif /* !__USE_W32API */

#endif /* __INCLUDE_NTOS_GDITYPES_H */

/* EOF */
