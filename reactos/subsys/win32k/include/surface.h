#ifndef _WIN32K_SURFACE_H
#define _WIN32K_SURFACE_H

#define GDIDEV(SurfObj) ((GDIDEVICE *)((SurfObj)->hdev))
#define GDIDEVFUNCS(SurfObj) ((GDIDEVICE *)((SurfObj)->hdev))->DriverFunctions

INT     INTERNAL_CALL BitsPerFormat (ULONG Format);
ULONG   INTERNAL_CALL BitmapFormat (WORD Bits, DWORD Compression);
HBITMAP INTERNAL_CALL IntCreateBitmap(IN SIZEL Size, IN LONG Width, IN ULONG Format, IN ULONG Flags, IN PVOID Bits);

#endif /* _WIN32K_SURFACE_H */
