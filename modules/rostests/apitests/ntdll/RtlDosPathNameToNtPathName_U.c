/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDosPathNameToNtPathName_U
 * PROGRAMMER:      Mike "tamlin" Nordell
 */
/* TODO:
 * - Make the code UNICODE aware. If a user is inside a directory with
 *   non-ANSI characters somewhere in the path, all bets are currently off.
 * - Remove hard-coded path size limits.
 * - Un-tabify to match style of other code.
 * - Clean up cruft. Probably remove the option of running it stand-alone.
 */

/* Test to see that ntdll.RtlDosPathNameToNtPathName_U behaves _exactly_
 * like Windows with all possible input to it.
 * - relative path.
 * - absolute paths
 * - \\.\C:\foo
 * - \\.\C:\foo\
 * - \\?\C:\foo
 * - \\?\C:\foo\
 * - \??\C:
 * - \??\C:\
 *
 * Caveat: The "\??\*" form behaves different depending on Windows version.
 *         Some tests will fail if there is no C: volume.
 *
 * Code is assumed to be compiled as 32-bit "ANSI" (i.e. with _UNICODE) undefined.
 */

// Enable this define to compile the test as a ReactOS auto-test.
// Disable it to compile on plain Win32, to test-run and get info.
#define COMPILE_AS_ROSTEST

#ifndef COMPILE_AS_ROSTEST
# define PRINT_INFO // Also print, in addition to testing
# include <windows.h>
# include <stdio.h>
# include <stddef.h>
#else /* Compile for ReactOS or wine */
# include "precomp.h"
#endif

/*
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(IN PCWSTR DosName,
                             OUT PUNICODE_STRING NtName,
                             OUT PCWSTR *PartName,
                             OUT PRTL_RELATIVE_NAME_U RelativeName)
*/

#ifndef COMPILE_AS_ROSTEST

typedef struct UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _RTLP_CURDIR_REF
{
    LONG RefCount;
    HANDLE Handle;
} RTLP_CURDIR_REF, *PRTLP_CURDIR_REF;

typedef struct RTL_RELATIVE_NAME_U {
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

typedef BOOLEAN (__stdcall *RtlDosPathNameToNtPathName_U_t)(PCWSTR,PUNICODE_STRING,PCWSTR*,PRTL_RELATIVE_NAME_U);

static RtlDosPathNameToNtPathName_U_t RtlDosPathNameToNtPathName_U;

#endif // !COMPILE_AS_ROSTEST


static void check_result(BOOLEAN bOK, const char* pszErr)
{
#ifdef COMPILE_AS_ROSTEST
	ok(bOK, "%s.\n", pszErr);
#else
	if (!bOK) {
		printf("\a** %s.\n", pszErr);
	}
#endif
}


#ifndef COMPILE_AS_ROSTEST
static void prucs(const char* pszDesc, UNICODE_STRING* pucs)
{
	WCHAR wszTmp[512];
	memcpy(wszTmp, pucs->Buffer, pucs->Length);
	wszTmp[pucs->Length/2] = 0;
	printf("%-12s: \"%S\"\n", pszDesc, wszTmp);
}
#endif

// Test RtlDosPathNameToNtPathName_U .
// pwszExpected shall contain the expected result for NtName
// *without* the leading "\??\".
// pwszExpectedPartName shall contain the expected result for PartName.
// NULL Expected means result is expected to be NULL too.
static void test2(LPCWSTR pwsz, LPCWSTR pwszExpected, LPCWSTR pwszExpectedPartName)
{
    UNICODE_STRING      NtName;
    PWSTR               PartName;
    RTL_RELATIVE_NAME_U RelativeName;
	BOOLEAN             bOK;

	bOK = RtlDosPathNameToNtPathName_U(pwsz, &NtName, (PCWSTR*)&PartName, &RelativeName);

	check_result(bOK, "RtlDosPathNameToNtPathName_U failed");
	if (!bOK) {
		printf("input: \"%S\"\n", pwsz);
		return;
	}

#if !defined(COMPILE_AS_ROSTEST) && defined(PRINT_INFO)
	printf("--------------------------\n");
	printf("in          : \"%S\"\n", pwsz);
	prucs("NtName", &NtName);
	printf("PartName    : \"%S\"\n", PartName ? PartName : L"(null)");
//	prucs("RelativeName", &RelativeName.RelativeName);
#endif

	// Disregarding input, output (NtName) shall always start with "\??\".
	bOK = NtName.Length >= 8 &&
	      memcmp(NtName.Buffer, L"\\??\\", 8) == 0;
	check_result(bOK, "NtName does not start with \"\\??\\\"");
	if (!bOK) {
		return;
	}

	if (pwszExpected) {
		PWSTR pwszActual = NtName.Buffer + 4;
		const size_t lenExp = wcslen(pwszExpected);
		const size_t lenAct = (NtName.Length - 8) / 2;
		bOK = (lenExp == lenAct) &&
		      memcmp(pwszActual, pwszExpected, lenExp * 2) == 0;
		check_result(bOK, "NtName does not match expected");
		if (!bOK)
		{
			printf("input:  : %2Iu chars \"%S\"\n", wcslen(pwsz), pwsz);
			printf("Expected: %2Iu chars \"%S\"\n", lenExp, pwszExpected);
			printf("Actual  : %2Iu chars \"%S\"\n", lenAct, lenAct ? pwszActual : L"(null)");
			return;
		}
	} else
	if (NtName.Length)
	{
		PWSTR pwszActual = NtName.Buffer + 4;
		const size_t lenAct = (NtName.Length - 8) / 2;
		check_result(FALSE, "Unexpected NtName (expected NULL)");
		printf("input:  : %2Iu chars \"%S\"\n", wcslen(pwsz), pwsz);
		printf("Actual  : %2Iu chars \"%S\"\n", lenAct, pwszActual);
	}

	if (pwszExpectedPartName) {
		const size_t lenExp = wcslen(pwszExpectedPartName);
		const size_t lenAct = PartName ? wcslen(PartName) : 0;
		bOK = (lenExp == lenAct) &&
		      wcscmp(PartName, pwszExpectedPartName) == 0;
		check_result(bOK, "PartName does not match expected");
		if (!bOK) {
			printf("input:  : %2Iu chars \"%S\"\n", wcslen(pwsz), pwsz);
			printf("Expected: %2Iu chars \"%S\"\n", lenExp, pwszExpectedPartName);
			printf("Actual  : %2Iu chars \"%S\"\n", lenAct, lenAct ? PartName : L"(null)");
			return;
		}
	} else
	if (PartName)
	{
		check_result(FALSE, "Unexpected PartName (expected NULL).");
		printf("input:  : %2Iu chars \"%S\"\n", wcslen(pwsz), pwsz);
		printf("Actual  : %2Iu chars %S\n", wcslen(PartName), PartName);
	}
}

// NULL Expected means result is expected to be NULL too.
static void test(const char* psz, const char* pszExpected, const char* pszExpectedPartName)
{
	WCHAR wszTmp1[512];
	WCHAR wszTmp2[512];
	WCHAR wszTmp3[512];
	LPCWSTR p2 = 0;
	LPCWSTR p3 = 0;
	swprintf(wszTmp1, L"%S", psz);
	if (pszExpected) {
		swprintf(wszTmp2, L"%S", pszExpected);
		p2 = wszTmp2;
	}
	if (pszExpectedPartName) {
		swprintf(wszTmp3, L"%S", pszExpectedPartName);
		p3 = wszTmp3;
	}
	test2(wszTmp1, p2, p3);
}


typedef struct DirComponents
{
	char szCD[512];
	char szCDPlusSlash[512];
	char* pszLastCDComponent;
	char szCurDrive[3];
	char reserved1;
	char szCurDriveSlash[4];
	char szParentDir[512];
	char szParentDirPlusSlash[512];
	char szNextLastCDComponent[260];
	const char* pszNextLastCDComponent;
	const char* pszPD; // parent dir
	const char* pszPDPlusSlash;
} DirComponents;


static void InitDirComponents(DirComponents* p)
{
	/* While the following code seems to work, it's an unholy mess
	 * and should probably be cleaned up.
	 */
	BOOLEAN bOK;

	p->pszNextLastCDComponent = 0;
	p->pszPD = 0;
	p->pszPDPlusSlash = 0;

	GetCurrentDirectory(sizeof(p->szCD) / sizeof(*p->szCD), p->szCD);

	bOK = strlen(p->szCD) >= 2 && p->szCD[1] == ':';
	check_result(bOK, "Expected curdir to be a drive letter. It's not");

	if (!bOK) {
		printf("Curdir is \"%s\"\n", p->szCD);
		exit(1);
	}

	bOK = p->szCD[2] == '\\';
	check_result(bOK, "CD is missing a slash as its third character");
	if (!bOK) {
		printf("CD is \"%s\"\n", p->szCD);
		exit(1);
	}

	// Note that if executed from the root directory, a backslash is
	// already appended.
	strcpy(p->szCDPlusSlash, p->szCD);
	if (strlen(p->szCD) > 3) {
		// Append trailing backslash
		strcat(p->szCDPlusSlash, "\\");
	}

	memcpy(p->szCurDrive, p->szCD, 2);
	p->szCurDrive[2] = 0;
	sprintf(p->szCurDriveSlash, "%s\\", p->szCurDrive);

	p->pszLastCDComponent = strrchr(p->szCD, '\\');
	if (p->pszLastCDComponent)
	{
		// We have a parent directory
		memcpy(p->szParentDir, p->szCD, p->pszLastCDComponent - p->szCD);
		p->szParentDir[p->pszLastCDComponent - p->szCD] = 0;
		p->pszPD = p->szParentDir;
		if (strlen(p->szParentDir) == 2 && p->szParentDir[1] == ':') {
			// When run from root directory, this is expected to
			// have a trailing backslash
			strcat(p->szParentDir, "\\");
		}
		strcpy(p->szParentDirPlusSlash, p->szParentDir);
		if (p->szParentDirPlusSlash[strlen(p->szParentDirPlusSlash)-1] != '\\') {
			strcat(p->szParentDirPlusSlash, "\\");
		}
		p->pszPDPlusSlash = p->szParentDirPlusSlash;

		// Check if we have a directory above that too.
		*p->pszLastCDComponent = 0;
		p->pszNextLastCDComponent = strrchr(p->szCD, '\\');
		*p->pszLastCDComponent = '\\';
		if (p->pszNextLastCDComponent) {
			++p->pszNextLastCDComponent; // skip the leading slash
			if (p->pszNextLastCDComponent + 1 >= p->pszLastCDComponent) {
				p->pszNextLastCDComponent = 0;
			} else {
				const size_t siz = p->pszLastCDComponent - p->pszNextLastCDComponent;
				memcpy(p->szNextLastCDComponent, p->pszNextLastCDComponent, siz);
				p->szNextLastCDComponent[siz] = 0;
				p->pszNextLastCDComponent = p->szNextLastCDComponent;
			}
		}
	}
	if (p->pszLastCDComponent && p->pszLastCDComponent[1] == 0) {
		// If the backslash is the last character in the path,
		// this is a NULL-component.
		p->pszLastCDComponent = 0;
	} else {
		++p->pszLastCDComponent; // skip the leading slash
	}
}


#ifndef COMPILE_AS_ROSTEST
static void InitFunctionPointer()
{
	HINSTANCE hDll = LoadLibrary("ntdll");
	if (!hDll) {
		printf("Major failure. Couldn't even load ntdll!\n");
		exit(1);
	}
	RtlDosPathNameToNtPathName_U =
		(RtlDosPathNameToNtPathName_U_t)GetProcAddress(hDll, "RtlDosPathNameToNtPathName_U");
	if (!RtlDosPathNameToNtPathName_U) {
		printf("Major failure. Couldn't get RtlDosPathNameToNtPathName_U!\n");
		exit(1);
	}
}

# if defined(PRINT_INFO)
static DWORD get_win_ver()
{
#  ifdef COMPILE_AS_ROSTEST
    PPEB Peb = NtCurrentPeb();
    const DWORD dwWinVer = (DWORD)(Peb->OSMinorVersion << 8) | Peb->OSMajorVersion;
#  else
	const DWORD dwWinVer = GetVersion();
#  endif
	return dwWinVer;
}
# endif /* PRINT_INFO */
#endif /* !COMPILE_AS_ROSTEST */


#ifdef COMPILE_AS_ROSTEST
START_TEST(RtlDosPathNameToNtPathName_U)
#else
int main()
#endif
{
#if defined(PRINT_INFO)
	const DWORD dwWinVer = get_win_ver();
	const BYTE  WinVerMaj = (BYTE)dwWinVer;
	const BYTE  WinVerMin = HIBYTE(LOWORD(dwWinVer));
#endif // PRINT_INFO

	DirComponents cd;
	char szTmp[518];

#ifndef COMPILE_AS_ROSTEST
	InitFunctionPointer();
#endif

	InitDirComponents(&cd);

#if defined(PRINT_INFO)
	printf("WinVer: %d.%d\n", WinVerMaj, WinVerMin);
	printf("pszLastCDComponent     \"%s\"\n", cd.pszLastCDComponent);
	printf("pszNextLastCDComponent \"%s\"\n", cd.pszNextLastCDComponent);
	printf("pszParentDir           \"%s\"\n", cd.pszPD);
	printf("pszParentDirPlusSlash  \"%s\"\n", cd.pszPDPlusSlash);
#endif

#define PREP0            /* The normal Win32 namespace. Fully parsed. */
#define PREP1 "\\\\.\\"  /* The Win32 Device Namespace. Only partially parsed. */
#define PREP2 "\\\\?\\"  /* The Win32 File Namespace. Only partially parsed. */

	//         input name        NtName              PartName
	// volume-absolute paths
	test(PREP1 "C:"            , "C:"              , "C:");
	test(PREP2 "C:"            , "C:"              , "C:");
	test(PREP0 "C:\\"          , "C:\\"            , NULL);
	test(PREP1 "C:\\"          , "C:\\"            , NULL);
	test(PREP2 "C:\\"          , "C:\\"            , NULL);
	test(PREP0 "C:\\foo"       , "C:\\foo"         , "foo");
	test(PREP1 "C:\\foo"       , "C:\\foo"         , "foo");
	test(PREP2 "C:\\foo"       , "C:\\foo"         , "foo");
	test(PREP0 "C:\\foo\\"     , "C:\\foo\\"       , NULL);
	test(PREP1 "C:\\foo\\"     , "C:\\foo\\"       , NULL);
	test(PREP2 "C:\\foo\\"     , "C:\\foo\\"       , NULL);
	test(PREP0 "C:\\foo\\bar"  , "C:\\foo\\bar"    , "bar");
	test(PREP1 "C:\\foo\\bar"  , "C:\\foo\\bar"    , "bar");
	test(PREP2 "C:\\foo\\bar"  , "C:\\foo\\bar"    , "bar");
	test(PREP0 "C:\\foo\\bar\\", "C:\\foo\\bar\\"  , NULL);
	test(PREP1 "C:\\foo\\bar\\", "C:\\foo\\bar\\"  , NULL);
	test(PREP2 "C:\\foo\\bar\\", "C:\\foo\\bar\\"  , NULL);
	test(PREP0 "C:\\foo\\.."   , "C:\\"            , NULL);
	test(PREP1 "C:\\foo\\.."   , "C:"              , "C:");
	test(PREP2 "C:\\foo\\.."   , "C:\\foo\\.."     , "..");
	test(PREP0 "C:\\foo\\..\\" , "C:\\"            , NULL);
	test(PREP1 "C:\\foo\\..\\" , "C:\\"            , NULL);
	test(PREP2 "C:\\foo\\..\\" , "C:\\foo\\..\\"   , NULL);
	test(PREP0 "C:\\foo."      , "C:\\foo"         , "foo");
	test(PREP1 "C:\\foo."      , "C:\\foo"         , "foo");
	test(PREP2 "C:\\foo."      , "C:\\foo."        , "foo.");

	test(PREP0 "C:\\f\\b\\.."  , "C:\\f"           , "f");
	test(PREP1 "C:\\f\\b\\.."  , "C:\\f"           , "f");
	test(PREP2 "C:\\f\\b\\.."  , "C:\\f\\b\\.."    , "..");
	test(PREP0 "C:\\f\\b\\..\\", "C:\\f\\"         , NULL);
	test(PREP1 "C:\\f\\b\\..\\", "C:\\f\\"         , NULL);
	test(PREP2 "C:\\f\\b\\..\\", "C:\\f\\b\\..\\"  , NULL);

	// CD-relative paths

	// RtlDosPathNameToNtPathName_U makes no distinction for
	// special device names, such as "PhysicalDisk0", "HarddiskVolume0"
	// or "Global??". They all follow the same pattern as a named
	// filesystem entry, why they implicitly tested by the following
	// "foo" and "foo\" cases.
	sprintf(szTmp, "%s%s", cd.szCDPlusSlash, "foo");
	test(PREP0 "foo"           , szTmp             , "foo");
	test(PREP1 "foo"           , "foo"             , "foo");
	test(PREP2 "foo"           , "foo"             , "foo");

	sprintf(szTmp, "%s%s", cd.szCDPlusSlash        , "foo\\");
	test(PREP0 "foo\\"         , szTmp             , NULL);
	test(PREP1 "foo\\"         , "foo\\"           , NULL);
	test(PREP2 "foo\\"         , "foo\\"           , NULL);

	test(PREP0 "."             , cd.szCD           , cd.pszLastCDComponent);
	test(PREP1 "."             , ""                , NULL);
	test(PREP2 "."             , "."               , ".");
	test(PREP0 ".\\"           , cd.szCDPlusSlash  , NULL);
	test(PREP1 ".\\"           , ""                , NULL);
	test(PREP2 ".\\"           , ".\\"             , NULL);
	test(PREP0 ".\\."          , cd.szCD           , cd.pszLastCDComponent);
	test(PREP1 ".\\."          , ""                , NULL);
	test(PREP2 ".\\."          , ".\\."            , ".");
	test(PREP0 ".\\.."         , cd.pszPD          , cd.pszNextLastCDComponent);
	test(PREP1 ".\\.."         , ""                , NULL);
	test(PREP2 ".\\.."         , ".\\.."           , "..");
	test(PREP0 ".."            , cd.pszPD          , cd.pszNextLastCDComponent);
	test(PREP1 ".."            , ""                , NULL);
	test(PREP2 ".."            , ".."              , "..");
	test(PREP0 "..\\"          , cd.pszPDPlusSlash , NULL);
	test(PREP1 "..\\"          , ""                , NULL);
	test(PREP2 "..\\"          , "..\\"            , NULL);
	// malformed
	test(PREP0 "..."           , cd.szCDPlusSlash  , NULL);
	test(PREP1 "..."           , ""                , NULL);
	test(PREP2 "..."           , "..."             , "...");

	// Test well-known "special" DOS device names.
	test(PREP0 "NUL"           , "NUL"             , NULL);
	test(PREP1 "NUL"           , "NUL"             , "NUL");
	test(PREP2 "NUL"           , "NUL"             , "NUL");
	test(PREP0 "NUL:"          , "NUL"             , NULL);
	test(PREP1 "NUL:"          , "NUL:"            , "NUL:");
	test(PREP2 "NUL:"          , "NUL:"            , "NUL:");
	test(PREP0 "CON"           , "CON"             , NULL);
	// NOTE: RtlDosPathNameToNtPathName_U (as currently tested) fails for
	// the input "\\.\CON" on two widely different Windows versions.
//	test(PREP1 "CON"           , "CON"             , "CON");
	test(PREP2 "CON"           , "CON"             , "CON");
	test(PREP0 "CON:"          , "CON"             , NULL);
	test(PREP1 "CON:"          , "CON:"            , "CON:");
	test(PREP2 "CON:"          , "CON:"            , "CON:");

	sprintf(szTmp, "%s\\%s", cd.szCD, "NUL:\\");
	test(PREP0 "NUL:\\"        , szTmp             , NULL);
	test(PREP1 "NUL:\\"        , "NUL:\\"          , NULL);
	test(PREP2 "NUL:\\"        , "NUL:\\"          , NULL);
	test(PREP0 "C:NUL"         , "NUL"             , NULL);
	test(PREP1 "C:NUL"         , "C:NUL"           , "C:NUL");
	test(PREP2 "C:NUL"         , "C:NUL"           , "C:NUL");
	test(PREP0 "C:\\NUL"       , "NUL"             , NULL);
	test(PREP1 "C:\\NUL"       , "C:\\NUL"         , "NUL");
	test(PREP2 "C:\\NUL"       , "C:\\NUL"         , "NUL");
	test(PREP0 "C:\\NUL\\"     , "C:\\NUL\\"       , NULL);
	test(PREP1 "C:\\NUL\\"     , "C:\\NUL\\"       , NULL);
	test(PREP2 "C:\\NUL\\"     , "C:\\NUL\\"       , NULL);

	// root-paths
	test(PREP0 "\\"            , cd.szCurDriveSlash, NULL);
	test(PREP1 "\\"            , ""                , NULL);
	test(PREP2 "\\"            , "\\"              , NULL);

	test(PREP0 "\\."           , cd.szCurDriveSlash, NULL);
	test(PREP1 "\\."           , ""                , NULL);
	test(PREP2 "\\."           , "\\."             , ".");

	test(PREP0 "\\.."          , cd.szCurDriveSlash, NULL);
	test(PREP1 "\\.."          , ""                , NULL);
	test(PREP2 "\\.."          , "\\.."            , "..");

	test(PREP0 "\\..."         , cd.szCurDriveSlash, NULL);
	test(PREP1 "\\..."         , ""                , NULL);
	test(PREP2 "\\..."         , "\\..."           , "...");

	// malformed
	sprintf(szTmp, "%s%s", cd.szCurDrive, "\\C:");
	test(PREP0 "\\C:"          , szTmp              , "C:");
	test(PREP1 "\\C:"          , "C:"               , "C:");
	test(PREP2 "\\C:"          , "\\C:"             , "C:");

	sprintf(szTmp, "%s%s", cd.szCurDrive, "\\C:\\");
	test(PREP0 "\\C:\\"        , szTmp             , NULL);
	test(PREP1 "\\C:\\"        , "C:\\"            , NULL);
	test(PREP2 "\\C:\\"        , "\\C:\\"          , NULL);

	// UNC paths
	test(PREP0 "\\\\"          , "UNC\\"           , NULL);
	test(PREP1 "\\\\"          , ""                , NULL);
	test(PREP2 "\\\\"          , "\\\\"            , NULL);
	test(PREP0 "\\\\\\"        , "UNC\\\\"         , NULL);
	test(PREP1 "\\\\\\"        , ""                , NULL);
	test(PREP2 "\\\\\\"        , "\\\\\\"          , NULL);
	test(PREP0 "\\\\foo"       , "UNC\\foo"        , NULL);
	test(PREP1 "\\\\foo"       , "foo"             , "foo");
	test(PREP2 "\\\\foo"       , "\\\\foo"         , "foo");

	test(PREP0 "\\\\foo\\.."   , "UNC\\foo\\"      , NULL);
	test(PREP1 "\\\\foo\\.."   , ""                , NULL);
	test(PREP2 "\\\\foo\\.."   , "\\\\foo\\.."     , "..");

	test(PREP0 "\\\\foo\\"     , "UNC\\foo\\"      , NULL);
	test(PREP1 "\\\\foo\\"     , "foo\\"           , NULL);
	test(PREP2 "\\\\foo\\"     , "\\\\foo\\"       , NULL);
	test(PREP0 "\\\\f\\b"      , "UNC\\f\\b"       , NULL);
	test(PREP1 "\\\\f\\b"      , "f\\b"            , "b");
	test(PREP2 "\\\\f\\b"      , "\\\\f\\b"        , "b");
	test(PREP0 "\\\\f\\b\\"    , "UNC\\f\\b\\"     , NULL);
	test(PREP1 "\\\\f\\b\\"    , "f\\b\\"          , NULL);
	test(PREP2 "\\\\f\\b\\"    , "\\\\f\\b\\"      , NULL);

	test(PREP0 "\\\\f\\b\\.."  , "UNC\\f\\b"       , NULL);
	test(PREP1 "\\\\f\\b\\.."  , "f"               , "f");
	test(PREP2 "\\\\f\\b\\.."  , "\\\\f\\b\\.."    , "..");

	// strange UNC-paths
	test(PREP0 "\\\\C:"        , "UNC\\C:"         , NULL);
	test(PREP1 "\\\\C:"        , "C:"              , "C:");
	test(PREP2 "\\\\C:"        , "\\\\C:"          , "C:");
	test(PREP0 "\\\\C:\\"      , "UNC\\C:\\"       , NULL);
	test(PREP1 "\\\\C:\\"      , "C:\\"            , NULL);
	test(PREP2 "\\\\C:\\"      , "\\\\C:\\"        , NULL);
	test(PREP0 "\\\\NUL"       , "UNC\\NUL"        , NULL);
	test(PREP1 "\\\\NUL"       , "NUL"             , "NUL");
	test(PREP2 "\\\\NUL"       , "\\\\NUL"         , "NUL");
	test(PREP0 "\\\\NUL:"      , "UNC\\NUL:"       , NULL);
	test(PREP1 "\\\\NUL:"      , "NUL:"            , "NUL:");
	test(PREP2 "\\\\NUL:"      , "\\\\NUL:"        , "NUL:");
	test(PREP0 "\\\\NUL:\\"    , "UNC\\NUL:\\"     , NULL);
	test(PREP1 "\\\\NUL:\\"    , "NUL:\\"          , NULL);
	test(PREP2 "\\\\NUL:\\"    , "\\\\NUL:\\"      , NULL);

	// UNC + forward slashes
	test(PREP0 "//"            , "UNC\\"           , NULL);
	test(PREP1 "//"            , ""                , NULL);
	test(PREP2 "//"            , "//"              , "//");
	test(PREP0 "//C:"          , "UNC\\C:"         , NULL);
	test(PREP1 "//C:"          , "C:"              , "C:");
	test(PREP2 "//C:"          , "//C:"            , "//C:");
	test(PREP0 "//C:/"         , "UNC\\C:\\"       , NULL);
	test(PREP1 "//C:/"         , "C:\\"            , NULL);
	test(PREP2 "//C:/"         , "//C:/"           , "//C:/");
	test(PREP0 "//."           , ""                , NULL);
	test(PREP1 "//."           , ""                , NULL);
	test(PREP2 "//."           , "//."             , "//.");
	test(PREP0 "//.."          , "UNC\\"           , NULL);
	test(PREP1 "//.."          , ""                , NULL);
	test(PREP2 "//.."          , "//.."            , "//..");
	test(PREP0 "/./"           , cd.szCurDriveSlash, NULL);
	test(PREP1 "/./"           , ""                , NULL);
	test(PREP2 "/./"           , "/./"             , "/./");
	test(PREP0 "//./"          , ""                , NULL);
	test(PREP1 "//./"          , ""                , NULL);
	test(PREP2 "//./"          , "//./"            , "//./");

	test(cd.szCD               , cd.szCD           , cd.pszLastCDComponent);
	test(cd.szCDPlusSlash      , cd.szCDPlusSlash  , NULL);

#if 0
	// The following tests are "problematic", as they return results based on
	// what your CD on C: is, whether or not you actually run the program
	// from C:. For that reason, they are currently disabled.
	test(PREP0 "C:"            , "C:\\"+C_curdir         , C_curdir);
	test(PREP0 "C:NUL\\"       , "C:\\"+C_curdir+"\\NUL\\" , NULL);
#endif

#if 0 // Disabled due to... see the comment inside the block.
	{
		char szExp[32];
		BOOL bValid = FALSE;
		char szPrepend[32];
		szPrepend[0] = 0;
		// Strictly speaking, this "Should Never Happen(tm)", calling
		// RtlDosPathNameToNtPathName_U with a source already formed as
		// a full NT name ("\??\"), why it's not the end of the world
		// that this test is currently disabled.
		//
		// Some versions of Windows prepends driveletter + colon
		// for the process' current volume.
		// Prepending curdrive is most likely a bug that got fixed in
		// later versions of Windows, but for compatibility it may
		// become a requirement to "shim" this.
		//
		// Known operating systems prepending "Z:\??\" (assuming the
		// process' CD is on the volume Z:):
		// - XP sp2.
		//
		// Known operating systems not prepending:
		// - Win7 64 (as 32-bit)
		if (WinVerMaj == 5) {
			sprintf(szPrepend, "%s\\??\\", cd.szCurDrive);
		}

		sprintf(szExp, "%s%s", szPrepend, "C:");
		test("\\??\\C:", szExp, "C:");

		sprintf(szExp, "%s%s", szPrepend, "C:\\");
		test("\\??\\C:\\", szExp, NULL);

}
#endif

#ifndef COMPILE_AS_ROSTEST
	return 0;
#endif
}

