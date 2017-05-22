/*
 * Tests for the arcname.c functions:
 * - ArcPathNormalize(),
 * - ArcPathToNtPath().
 *
 * You should certainly fix the included headers before being able
 * to compile this file (as I didn't bother to have it compilable
 * under our (P/N)DK, but just under VS).
 */

#include <stdio.h>
#include <tchar.h>
#include <conio.h>

#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>
#undef WIN32_NO_STATUS

#include <ntstatus.h>

#include <strsafe.h>

#include "arcname.h"


BOOLEAN
NTAPI
RtlEqualUnicodeString(
    IN CONST UNICODE_STRING *s1,
    IN CONST UNICODE_STRING *s2,
    IN BOOLEAN  CaseInsensitive);

#define OBJ_NAME_PATH_SEPARATOR ((WCHAR)L'\\')


int _tmain(int argc, _TCHAR* argv[])
{
    WCHAR ArcPath[MAX_PATH] = L"multi(5)disk()rdisk(1)partition()\\ReactOS";
    WCHAR NormalizedArcPathBuffer[MAX_PATH];
    UNICODE_STRING NormalizedArcPath;
    WCHAR NtPathBuffer[MAX_PATH];
    UNICODE_STRING NtPath;

    NormalizedArcPath.Buffer = NormalizedArcPathBuffer;
    NormalizedArcPath.Length = 0;
    NormalizedArcPath.MaximumLength = sizeof(NormalizedArcPathBuffer);

    ArcPathNormalize(&NormalizedArcPath, ArcPath);
    wprintf(L"ArcPath = '%s' ; Normalized = '%wZ'\n", ArcPath, &NormalizedArcPath);

    NtPath.Buffer = NtPathBuffer;
    NtPath.Length = 0;
    NtPath.MaximumLength = sizeof(NtPathBuffer);

    ArcPathToNtPath(&NtPath, NormalizedArcPath.Buffer);
    // wprintf(L"ArcPath = '%s' ; NtPath = '%wZ'\n", ArcPath, &NtPath);
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"ramdisk(0)");                // OK
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"ramdisk(0)\\ReactOS\\system32\\ntoskrnl.exe");   // OK
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"net(0)\\Foobar");            // OK but not supported
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"net(0)disk(1)\\Foobar");     // Bad
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"scsi(2)disk(1)rdisk(3)");    // OK
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"scsi(2)disk(1)fdisk(3)");    // OK
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"scsi(2)cdrom(1)");           // Bad: missing fdisk
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"scsi(2)cdrom(1)cdrom(0)");   // Bad: twice cdrom
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"scsi(2)cdrom(1)fdisk(0)");   // OK
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"scsi(2)cdrom(1)rdisk(0)");   // Bad; cdrom controller and rdisk peripheral
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(2)cdrom(1)fdisk(0)");  // Bad: multi adapter cannot have cdrom controller
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(2)rdisk(1)cdrom(1)fdisk(0)");  // Bad: rdisk is not a controller
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(2)disk(1)cdrom(1)fdisk(0)");   // OK (disk(1) ignored)
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(2)disk(1)rdisk(1)fdisk(0)");   // Same (and also fdisk is not considered as part of ARC path)
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(2)disk(1)rdisk(1)partition(3)");   // OK (disk(1) ignored)
    wprintf(L"NtPath = '%wZ'\n", &NtPath);

    _getch();

    /* All these are OK */
    ArcPathToNtPath(&NtPath, L"scsi(0)disk(3)rdisk(0)partition(1)\\OS.DIR");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"scsi(1)disk(3)rdisk(3)partition(2)\\OS\\ARCOS\\LOADER");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);

    _getch();

    /* All these are OK */
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)rdisk(0)partition(1)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)rdisk(0)partition(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)cdrom(3)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"ramdisk(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"net(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)fdisk(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)rdisk(1)partition(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)rdisk(1)partition(3)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)rdisk(1)partition(1)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)rdisk(0)partition(3)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)fdisk(1)partition(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)fdisk(0)partition(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)disk(0)fdisk(1)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"eisa(0)disk(0)fdisk(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"eisa(0)disk(0)fdisk(1)partition(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"eisa(0)disk(0)fdisk(0)partition(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);

    /* These are invalid storage ARC paths (but otherwise are valid ARC names) */
    ArcPathToNtPath(&NtPath, L"multi(0)video(0)monitor(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);
    ArcPathToNtPath(&NtPath, L"multi(0)key(0)keyboard(0)");
    wprintf(L"NtPath = '%wZ'\n", &NtPath);

    _getch();
    return 0;
}
