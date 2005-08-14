#if 0
#include <windows.h>
#include <string.h>
#include <rosrtl/devmode.h>

#define SIZEOF_DEVMODEA_300 124
#define SIZEOF_DEVMODEA_400 148
#define SIZEOF_DEVMODEA_500 156
#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

void
RosRtlDevModeA2W ( LPDEVMODEW pW, const LPDEVMODEA pA )
{
#define COPYS(f,len) MultiByteToWideChar ( CP_THREAD_ACP, 0, (LPSTR)pA->f, len, pW->f, len )
#define COPYN(f) pW->f = pA->f
  COPYS(dmDeviceName, CCHDEVICENAME );
  COPYN(dmSpecVersion);
  COPYN(dmDriverVersion);
  switch ( pA->dmSize )
    {
    case SIZEOF_DEVMODEA_300:
      pW->dmSize = SIZEOF_DEVMODEW_300;
      break;
    case SIZEOF_DEVMODEA_400:
      pW->dmSize = SIZEOF_DEVMODEW_400;
      break;
    case SIZEOF_DEVMODEA_500:
    default: /* FIXME what to do??? */
      pW->dmSize = SIZEOF_DEVMODEW_500;
      break;
    }
  COPYN(dmDriverExtra);
  COPYN(dmFields);
  COPYN(dmPosition.x);
  COPYN(dmPosition.y);
  COPYN(dmScale);
  COPYN(dmCopies);
  COPYN(dmDefaultSource);
  COPYN(dmPrintQuality);
  COPYN(dmColor);
  COPYN(dmDuplex);
  COPYN(dmYResolution);
  COPYN(dmTTOption);
  COPYN(dmCollate);
  COPYS(dmFormName,CCHFORMNAME);
  COPYN(dmLogPixels);
  COPYN(dmBitsPerPel);
  COPYN(dmPelsWidth);
  COPYN(dmPelsHeight);
  COPYN(dmDisplayFlags); // aka dmNup
  COPYN(dmDisplayFrequency);

  if ( pA->dmSize <= SIZEOF_DEVMODEA_300 )
    return; // we're done with 0x300 fields

  COPYN(dmICMMethod);
  COPYN(dmICMIntent);
  COPYN(dmMediaType);
  COPYN(dmDitherType);
  COPYN(dmReserved1);
  COPYN(dmReserved2);

  if ( pA->dmSize <= SIZEOF_DEVMODEA_400 )
    return; // we're done with 0x400 fields

  COPYN(dmPanningWidth);
  COPYN(dmPanningHeight);

  return;

#undef COPYN
#undef COPYS
}


void
RosRtlDevModeW2A( LPDEVMODEA pA, const LPDEVMODEW pW )
{
#define COPYS(f,len) WideCharToMultiByte( CP_THREAD_ACP, 0, pW->f, len, (LPSTR)pA->f, len, NULL, NULL )
#define COPYN(f) pA->f = pW->f
  COPYS(dmDeviceName, CCHDEVICENAME );
  COPYN(dmSpecVersion);
  COPYN(dmDriverVersion);
  switch ( pW->dmSize )
    {
    case SIZEOF_DEVMODEW_300:
      pA->dmSize = SIZEOF_DEVMODEA_300;
      break;
    case SIZEOF_DEVMODEW_400:
      pA->dmSize = SIZEOF_DEVMODEA_400;
      break;
    case SIZEOF_DEVMODEW_500:
    default: /* FIXME what to do??? */
      pA->dmSize = SIZEOF_DEVMODEA_500;
      break;
    }
  COPYN(dmDriverExtra);
  COPYN(dmFields);
  COPYN(dmPosition.x);
  COPYN(dmPosition.y);
  COPYN(dmScale);
  COPYN(dmCopies);
  COPYN(dmDefaultSource);
  COPYN(dmPrintQuality);
  COPYN(dmColor);
  COPYN(dmDuplex);
  COPYN(dmYResolution);
  COPYN(dmTTOption);
  COPYN(dmCollate);
  COPYS(dmFormName,CCHFORMNAME);
  COPYN(dmLogPixels);
  COPYN(dmBitsPerPel);
  COPYN(dmPelsWidth);
  COPYN(dmPelsHeight);
  COPYN(dmDisplayFlags); // aka dmNup
  COPYN(dmDisplayFrequency);

  if ( pW->dmSize <= SIZEOF_DEVMODEW_300 )
    return; // we're done with 0x300 fields

  COPYN(dmICMMethod);
  COPYN(dmICMIntent);
  COPYN(dmMediaType);
  COPYN(dmDitherType);
  COPYN(dmReserved1);
  COPYN(dmReserved2);

  if ( pW->dmSize <= SIZEOF_DEVMODEW_400 )
    return; // we're done with 0x400 fields

  COPYN(dmPanningWidth);
  COPYN(dmPanningHeight);

  return;

#undef COPYN
#undef COPYS
}

#undef SIZEOF_DEVMODEA_300
#undef SIZEOF_DEVMODEA_400
#undef SIZEOF_DEVMODEA_500
#undef SIZEOF_DEVMODEW_300
#undef SIZEOF_DEVMODEW_400
#undef SIZEOF_DEVMODEW_500
#endif
