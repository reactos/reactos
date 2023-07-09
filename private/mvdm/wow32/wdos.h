/* wdos.h - DOS Defines for WOW
 *
 * Modification History
 *
 * Sudeepb 23-Aug-1991 Created
 */

ULONG FASTCALL   WK32SetDefaultDrive(PVDMFRAME pFrame);
ULONG FASTCALL   WK32GetCurrentDirectory(PVDMFRAME pFrame);
ULONG FASTCALL   WK32SetCurrentDirectory(PVDMFRAME pFrame);
ULONG FASTCALL   WK32GetCurrentDate(PVDMFRAME pFrame);
ULONG FASTCALL   WK32DeviceIOCTL(PVDMFRAME pFrame);
ULONG FASTCALL   WK32WOWGetFlatAddressArray(PVDMFRAME pFrame);

ULONG DosWowSetDefaultDrive (UCHAR);
ULONG DosWowGetCurrentDirectory (UCHAR, LPSTR);
ULONG DosWowSetCurrentDirectory (LPSTR);

typedef enum {
    DIR_NT_TO_DOS,
    DIR_DOS_TO_NT,
} UDCDFUNC;

BOOL UpdateDosCurrentDirectory(UDCDFUNC fDir);
