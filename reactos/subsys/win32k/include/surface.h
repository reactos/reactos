#ifndef _WIN32K_SURFACE_H
#define _WIN32K_SURFACE_H

INT   FASTCALL BitsPerFormat (ULONG Format);
ULONG FASTCALL BitmapFormat (WORD Bits, DWORD Compression);

#endif /* _WIN32K_SURFACE_H */
