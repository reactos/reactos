/***************************************************************************\
* Module Name: fekbdcom.c
* Common FE routines for stub keyboard layout DLLs
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* History: Created Aug 1997 by Hiro Yamamoto
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <kbd.h>

/***************************************************************************\
 * KbdLayerRealDllFile() returns the name of the real keyboard Dll
 *
 * Get Dll name from the registry
 * PATH:"\Registry\Machine\System\CurrentControlSet\Services\i8042prt\Parameters"
 * VALUE:"LayerDriver" (REG_SZ)
 *
 * kbdjpn.dll for Japanese and kbdkor.dll for Korean
\***************************************************************************/

#if 0   // FYI: old explanation
/***************************************************************************\
 * KbdLayerRealDriverFile() returns the name of the real keyboard driver
 *
 * 1) Get Path to the name of safe real driver
 * KEY: "\Registry\Machine\Hardware\DeviceMap\KeyboardPort"
 * VALUE: "\Device\KeyboardPort0" (REG_SZ) keeps path in the registry
 *   e.g. "\REGISTRY\Machine\System\ControlSet001\Services\i8042prt"
 *                                               ~~~~~~~~~~~~~~~~~~
 * 2) Create path from the results
 * "\Registry\Machine\System\CurrentControlSet" + "\Services\i8042prt"  + "\Parameters"
 *
 * 3) Get value
 * PATH: "\Registry\Machine\System\CurrentControlSet\Services\i8042prt\Parameters"
 * VALUE: "Parameters" (REG_SZ)
 *
 * NOTE: default value:
 *      "KBD101.DLL" for Japanese
 *      "KBD101A.DLL" for Korean
 *
 * kbdjpn.dll and kbdkor.dll
\***************************************************************************/
#endif


#if defined(HIRO_DBG)
#define TRACE(x)    DbgPrint x
#else
#define TRACE(x)
#endif

/*
 * Maximum character count
 */
#define MAXBUF_CSIZE    (256)

/*
 * Null-terminates the wchar string in pKey.
 * returns pointer to WCHAR string.
 *
 * limit: maximum BYTE SIZE
 */
__inline LPWSTR MakeString(PKEY_VALUE_FULL_INFORMATION pKey, size_t limit)
{
    LPWSTR pwszHead = (LPWSTR)((LPBYTE)pKey + pKey->DataOffset);
    LPWSTR pwszTail = (LPWSTR)((LPBYTE)pwszHead + pKey->DataLength);

    ASSERT((LPBYTE)pwszTail - (LPBYTE)pKey < (int)limit );
    *pwszTail = L'\0';

    UNREFERENCED_PARAMETER(limit);  // just in case

    return pwszHead;
}

#if 0   // not needed any more
/*
 * Find L"\services" in the given str.
 * Case insensitive search.
 * To avoid the binary size increase,
 * and since we know the string we're searching only contains
 * alphabet and backslash,
 * it's safe here not to use ideal functions (tolower/toupper).
 */
WCHAR* FindServices(WCHAR* str)
{
    CONST WCHAR wszServices[] = L"\\services";

    while (*str) {
        CONST WCHAR* p1;
        CONST WCHAR* p2;
        for (p1 = str, p2 = wszServices; *p1 && *p2; ++p1, ++p2) {
            // we know p2 only contains alphabet and backslash
            if (*p2 != L'\\') {
                if ((*p1 != *p2) && (*p1 + (L'a' - L'A') != *p2)) {
                    break;
                }
            }
            else if (*p1 != L'\\') {
                break;
            }
        }
        if (*p2 == 0) {
            // we found a match !
            return str;
        }
        ++str;
    }
    return NULL;
}
#endif

BOOL GetRealDllFileNameWorker(CONST WCHAR* pwszKeyName, WCHAR* RealDllName)
{
    NTSTATUS            Status;
    HANDLE              handleService;
    BOOL                fRet = FALSE;
    UNICODE_STRING      servicePath;
    OBJECT_ATTRIBUTES   servicePathObjectAttributes;
    WCHAR               serviceRegistryPath[MAXBUF_CSIZE];

    servicePath.Buffer = serviceRegistryPath;
    servicePath.Length = 0;
    servicePath.MaximumLength = sizeof serviceRegistryPath; // byte count !

    RtlAppendUnicodeToString(&servicePath,
                             L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\i8042prt\\Parameters");

    InitializeObjectAttributes(&servicePathObjectAttributes,
                               &servicePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    TRACE(("Open -> '%ws'\n",servicePath.Buffer));

    Status = NtOpenKey(&handleService, KEY_READ, &servicePathObjectAttributes);

    if (NT_SUCCESS(Status)) {

        UNICODE_STRING layerName;
        WCHAR LayerDllName[MAXBUF_CSIZE];
        ULONG cbStringSize;

        RtlInitUnicodeString(&layerName, pwszKeyName);
        TRACE(("Entry name -> '%ws'\n", layerName.Buffer));

        /*
         * Get the name of the DLL based on the device name.
         */
        Status = NtQueryValueKey(handleService,
                                 &layerName,
                                 KeyValueFullInformation,
                                 LayerDllName,
                                 sizeof LayerDllName - sizeof(WCHAR),    // reserves room for L'\0'
                                 &cbStringSize);

        if (NT_SUCCESS(Status)) {
            LPWSTR pwszStr = MakeString((PKEY_VALUE_FULL_INFORMATION)LayerDllName,
                                        sizeof LayerDllName);

            wcscpy(RealDllName, pwszStr);

            TRACE(("Real Dll name -> '%ws'\n", RealDllName));

            //
            // everything went fine.
            //
            fRet = TRUE;
        }
        NtClose(handleService);
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////
// This entry is used for backward compatibility.
// Exported as ordinal 3.
///////////////////////////////////////////////////////////////////////

BOOL KbdLayerRealDllFileNT4(WCHAR *RealDllName)
{
    TRACE(("KbdLayerRealDllFile():\n"));
    return GetRealDllFileNameWorker(L"LayerDriver", RealDllName);
}

///////////////////////////////////////////////////////////////////////
// This entry is used for remote client of Hydra server.
//
// Created: 1988 kazum
///////////////////////////////////////////////////////////////////////

BOOL KbdLayerRealDllFileForWBT(HKL hkl, WCHAR *realDllName, PCLIENTKEYBOARDTYPE pClientKbdType, LPVOID reserve)
{
    HANDLE hkRegistry = NULL;
    UNICODE_STRING    deviceMapPath;
    OBJECT_ATTRIBUTES deviceMapObjectAttributes;
    NTSTATUS          Status;
    HANDLE            handleMap;
    HANDLE            handleService;
    ULONG             cbStringSize;
    PWCHAR            pwszReg;

    UNREFERENCED_PARAMETER(reserve);

    ASSERT(pClientKbdType != NULL);
    /*
     * Set default keyboard layout for error cases.
     */
    if (PRIMARYLANGID(LOWORD(hkl)) == LANG_JAPANESE) {
        wcscpy(realDllName, L"kbd101.dll");
        pwszReg = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Terminal Server\\KeyboardType Mapping\\JPN";
    }
    else if (PRIMARYLANGID(LOWORD(hkl)) == LANG_KOREAN) {
        wcscpy(realDllName, L"kbd101a.dll");
        pwszReg = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Terminal Server\\KeyboardType Mapping\\KOR";
    }
    else {
        ASSERT(FALSE);
    }


    /*
     * Start by opening the registry for keyboard type mapping table.
     */
    RtlInitUnicodeString(&deviceMapPath, pwszReg);

    InitializeObjectAttributes(&deviceMapObjectAttributes,
                               &deviceMapPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    // DbgPrint("Open -> %ws\n",deviceMapPath.Buffer);

    Status = NtOpenKey(&handleMap, KEY_READ, &deviceMapObjectAttributes);

    if (NT_SUCCESS(Status)) {
        WCHAR SubKbdBuffer[16];
        WCHAR FuncKeyBuffer[16];
        WCHAR SubKbdAndFuncKeyBuffer[32];
        UNICODE_STRING SubKbd = {
            0, sizeof SubKbdBuffer, SubKbdBuffer,
        };
        UNICODE_STRING FuncKbd = {
            0, sizeof FuncKeyBuffer, FuncKeyBuffer,
        };
        UNICODE_STRING SubKbdAndFuncKey = {
            0, sizeof SubKbdAndFuncKeyBuffer, SubKbdAndFuncKeyBuffer,
        };
        UCHAR AnsiBuffer[16];
        ANSI_STRING AnsiString;
        WCHAR LayerDriverName[256];

        /*
         * Convert the sub keyboard type to the Ansi string buffer.
         */
        RtlZeroMemory(AnsiBuffer, sizeof(AnsiBuffer));
        Status = RtlIntegerToChar(pClientKbdType->SubType, 16L,
                                  -8, // length of buffer, but negative means 0 padding
                                  AnsiBuffer);
        if (NT_SUCCESS(Status)) {
            /*
             * Convert the Ansi string buffer to Unicode string buffer.
             */
            AnsiString.Buffer = AnsiBuffer;
            AnsiString.MaximumLength = sizeof AnsiBuffer;
            AnsiString.Length = (USHORT)strlen(AnsiBuffer);
            Status = RtlAnsiStringToUnicodeString(&SubKbd, &AnsiString, FALSE);
        }
        ASSERT(NT_SUCCESS(Status));     // Make sure the number is not bad

        /*
         * Convert the number of function key to the Ansi string buffer.
         */
        RtlZeroMemory(AnsiBuffer, sizeof(AnsiBuffer));
        Status = RtlIntegerToChar(pClientKbdType->FunctionKey, 10L,
                                  -4, // length of buffer, but negative means 0 padding
                                  AnsiBuffer);
        if (NT_SUCCESS(Status)) {
            /*
             * Convert the Ansi string buffer to Unicode string buffer.
             */
            AnsiString.Buffer = AnsiBuffer;
            AnsiString.MaximumLength = sizeof AnsiBuffer;
            AnsiString.Length = (USHORT)strlen(AnsiBuffer);
            Status = RtlAnsiStringToUnicodeString(&FuncKbd, &AnsiString, FALSE);
        }
        ASSERT(NT_SUCCESS(Status));  // Make sure the number is not bad


        /*
         * Get the sub kbd + function key layout name
         */
        RtlCopyUnicodeString(&SubKbdAndFuncKey, &SubKbd);
        RtlAppendUnicodeStringToString(&SubKbdAndFuncKey, &FuncKbd);
        Status = NtQueryValueKey(handleMap,
                                 &SubKbdAndFuncKey,
                                 KeyValueFullInformation,
                                 LayerDriverName,
                                 sizeof(LayerDriverName),
                                 &cbStringSize);

        if (NT_SUCCESS(Status)) {

            LayerDriverName[cbStringSize / sizeof(WCHAR)] = L'\0';

            wcscpy(realDllName,
                   (LPWSTR)((PUCHAR)LayerDriverName +
                            ((PKEY_VALUE_FULL_INFORMATION)LayerDriverName)->DataOffset));

            // DbgPrint("Real driver name -> %ws\n",realDllName);
        }
        else {
            /*
             * Get the sub kbd layout name
             */
            Status = NtQueryValueKey(handleMap,
                                     &SubKbd,
                                     KeyValueFullInformation,
                                     LayerDriverName,
                                     sizeof(LayerDriverName),
                                     &cbStringSize);

            if (NT_SUCCESS(Status)) {

                LayerDriverName[cbStringSize / sizeof(WCHAR)] = L'\0';

                wcscpy(realDllName,
                       (LPWSTR)((PUCHAR)LayerDriverName +
                                ((PKEY_VALUE_FULL_INFORMATION)LayerDriverName)->DataOffset));

                // DbgPrint("Real driver name -> %ws\n",realDllName);
            }
        }

        NtClose(handleMap);
    }

    return TRUE;
}


__inline WCHAR* wszcpy(WCHAR* target, CONST WCHAR* src)
{
    while (*target++ = *src++);
    return target - 1;
}


///////////////////////////////////////////////////////////////////////
// KbdLayerRealDllFile
//
// Enhanced version of KbdLayerRealDllFile:
// Distinguishes KOR and JPN.
///////////////////////////////////////////////////////////////////////

BOOL KbdLayerRealDllFile(HKL hkl, WCHAR *realDllName, PCLIENTKEYBOARDTYPE pClientKbdType, LPVOID reserve)
{
    WCHAR pwszBuff[32];
    WCHAR* pwszTail;
    LPCWSTR pwsz;

    ASSERT(PRIMARYLANGID(LOWORD(hkl)) == LANG_JAPANESE ||
           PRIMARYLANGID(LOWORD(hkl)) == LANG_KOREAN);

    if (pClientKbdType != NULL) {
        //
        // HYDRA case
        //
        return KbdLayerRealDllFileForWBT(hkl, realDllName, pClientKbdType, reserve);
    }

    if (PRIMARYLANGID(LOWORD(hkl)) == LANG_JAPANESE) {
        pwsz = L" JPN";
    }
    else if (PRIMARYLANGID(LOWORD(hkl)) == LANG_KOREAN) {
        pwsz = L" KOR";
    }
    else {
        pwsz = L"";
    }

    pwszTail = wszcpy(pwszBuff, L"LayerDriver");
    if (*pwsz) {
        wszcpy(pwszTail, pwsz);
    }

    TRACE(("KbdLayerRealDllFileEx: fetching '%S'\n", pwszBuff));

    return GetRealDllFileNameWorker(pwszBuff, realDllName);
}


