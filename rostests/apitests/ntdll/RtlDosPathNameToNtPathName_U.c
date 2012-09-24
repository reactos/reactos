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
# define WIN32_NO_STATUS
# include <stdio.h>
# include <wine/test.h>
# include <pseh/pseh2.h>
# include <ndk/rtlfuncs.h>
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

RtlDosPathNameToNtPathName_U_t RtlDosPathNameToNtPathName_U;

#endif // !COMPILE_AS_ROSTEST


static void check_result(BOOLEAN bOK, const char* pszErr)
{
#ifdef COMPILE_AS_ROSTEST
	ok(bOK, "%s\n", pszErr);
#else
	if (!bOK) {
		printf("\a** %s!\n", pszErr);
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
	if (PartName) {
		printf("PartName    : \"%S\"\n", PartName);
	} else {
		// This is not the place to test that printf handles NULL strings.
		printf("PartName    : (null)\n");
	}
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
		check_result(bOK, "Actual NtName does not match expectations!");
		if (!bOK)
		{
			printf("Expected: %2u chars \"%S\"\n", lenExp, pwszExpected);
			printf("Actual  : %2u chars \"%S\"\n", lenAct, lenAct ? pwszActual : L"(null)");
			return;
		}
	} else
	if (NtName.Length)
	{
		PWSTR pwszActual = NtName.Buffer + 4;
		const size_t lenAct = (NtName.Length - 8) / 2;
		check_result(FALSE, "Actual NtName does not match expectations!");
		printf("Actual  : %2u chars \"%S\"\n", lenAct, pwszActual);
	}

	if (pwszExpectedPartName) {
		if (!PartName) {
			check_result(FALSE, "Actual PartName is unexpectedly NULL!");
			printf("Expected: \"%S\"\n", pwszExpectedPartName);
			return;
		}
		bOK = wcscmp(PartName, pwszExpectedPartName) == 0;
		check_result(bOK, "Actual PartName does not match expected!");
		if (!bOK) {
			printf("Expected: \"%S\"\n", pwszExpectedPartName);
			printf("Actual  : \"%S\"\n", PartName);
			return;
		}
	} else
	if (PartName)
	{
		check_result(FALSE, "Unexpected PartName (expected NULL)");
		printf("Actual  : %S\n", PartName);
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
	char szCurDrive[4];
	char szParentDir[512];
	char szParentDirPlusSlash[512];
	char szNextLastCDComponent[260];
	const char* pszNextLastCDComponent;
	const char* pszParentDir;
	const char* pszParentDirPlusSlash;
} DirComponents;


static void InitDirComponents(DirComponents* p)
{
	p->pszNextLastCDComponent = 0;
	p->pszParentDir = 0;
	p->pszParentDirPlusSlash = 0;

	GetCurrentDirectory(sizeof(p->szCD) / sizeof(*p->szCD), p->szCD);

	if (strlen(p->szCD) < 2 || p->szCD[1] != ':') {
		printf("Expected curdir to be a drive letter. It's not. It's \"%s\"\n", p->szCD);
#ifdef COMPILE_AS_ROSTEST
		ok(FALSE, "Expected curdir to be a drive letter. It's not. It's \"%s\"\n", p->szCD);
#endif
		exit(1);
	}

	// Note that if executed from the root directory, a slash already
	// is appended. Take the opportunity to verify this.
	if (p->szCD[2] != '\\') {
		printf("CD is missing a slash as its third character! \"%s\"\n", p->szCD);
#ifdef COMPILE_AS_ROSTEST
		ok(FALSE, "CD is missing a slash as its third character! \"%s\"\n", p->szCD);
#endif
		exit(1);
	}

	strcpy(p->szCDPlusSlash, p->szCD);
	if (strlen(p->szCD) > 3) {
		// Append trailing backslash
		strcat(p->szCDPlusSlash, "\\");
	}

	memcpy(p->szCurDrive, p->szCD, 2);
	p->szCurDrive[2] = 0;

	p->pszLastCDComponent = strrchr(p->szCD, '\\');
	if (p->pszLastCDComponent)
	{
		// We have a parent directory
		memcpy(p->szParentDir, p->szCD, p->pszLastCDComponent - p->szCD);
		p->szParentDir[p->pszLastCDComponent - p->szCD] = 0;
		p->pszParentDir = p->szParentDir;
		if (strlen(p->szParentDir) == 2 && p->szParentDir[1] == ':') {
			// When run from root directory, this is expected to
			// have a trailing backslash
			strcat(p->szParentDir, "\\");
		}
		strcpy(p->szParentDirPlusSlash, p->szParentDir);
		if (p->szParentDirPlusSlash[strlen(p->szParentDirPlusSlash)-1] != '\\') {
			strcat(p->szParentDirPlusSlash, "\\");
		}
		p->pszParentDirPlusSlash = p->szParentDirPlusSlash;

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
	if (!hDll) {
		printf("Major failure. Couldn't get RtlDosPathNameToNtPathName_U!\n");
		exit(1);
	}
}
#endif // !COMPILE_AS_ROSTEST


#ifdef COMPILE_AS_ROSTEST
START_TEST(RtlDosPathNameToNtPathName_U)
#else
int main()
#endif
{
#if defined(PRINT_INFO)
#ifdef COMPILE_AS_ROSTEST
    PPEB Peb = NtCurrentPeb();
    const DWORD dwWinVer = (DWORD)(Peb->OSMinorVersion << 8) | Peb->OSMajorVersion;
#else
	const DWORD dwWinVer = GetVersion();
#endif
	const BYTE  WinVerMaj = (BYTE)dwWinVer;
	const BYTE  WinVerMin = HIBYTE(LOWORD(dwWinVer));
#endif // PRINT_INFO

	DirComponents cd;

#ifndef COMPILE_AS_ROSTEST
	InitFunctionPointer();
#endif

	InitDirComponents(&cd);

#if defined(PRINT_INFO)
	printf("WinVer: %d.%d\n", WinVerMaj, WinVerMin);
	printf("pszLastCDComponent     \"%s\"\n", cd.pszLastCDComponent);
	printf("pszNextLastCDComponent \"%s\"\n", cd.pszNextLastCDComponent);
	printf("pszParentDir           \"%s\"\n", cd.pszParentDir);
	printf("pszParentDirPlusSlash  \"%s\"\n", cd.pszParentDirPlusSlash);
#endif


#define PREP0
#define PREP1 "\\\\.\\"
#define PREP2 "\\\\?\\"

	// The following tests shall return strictly defined strings,
	// why we can use hard-coded expectations..
	test(PREP1 "C:"            , "C:"             , "C:");
	test(PREP2 "C:"            , "C:"             , "C:");
	test(PREP0 "C:\\"          , "C:\\"           , NULL);
	test(PREP1 "C:\\"          , "C:\\"           , NULL);
	test(PREP2 "C:\\"          , "C:\\"           , NULL);
	test(PREP0 "C:\\foo"       , "C:\\foo"        , "foo");
	test(PREP1 "C:\\foo"       , "C:\\foo"        , "foo");
	test(PREP2 "C:\\foo"       , "C:\\foo"        , "foo");
	test(PREP0 "C:\\foo\\"     , "C:\\foo\\"      , NULL);
	test(PREP1 "C:\\foo\\"     , "C:\\foo\\"      , NULL);
	test(PREP2 "C:\\foo\\"     , "C:\\foo\\"      , NULL);
	test(PREP0 "C:\\foo\\bar"  , "C:\\foo\\bar"   , "bar");
	test(PREP1 "C:\\foo\\bar"  , "C:\\foo\\bar"   , "bar");
	test(PREP2 "C:\\foo\\bar"  , "C:\\foo\\bar"   , "bar");
	test(PREP0 "C:\\foo\\bar\\", "C:\\foo\\bar\\" , NULL);
	test(PREP1 "C:\\foo\\bar\\", "C:\\foo\\bar\\" , NULL);
	test(PREP2 "C:\\foo\\bar\\", "C:\\foo\\bar\\" , NULL);
	test(PREP1 "."             , ""               , NULL);
	test(PREP2 "."             , "."              , ".");
	test(PREP1 ".\\"           , ""               , NULL);
	test(PREP2 ".\\"           , ".\\"            , NULL);
	test(PREP1 ".."            , ""               , NULL);
	test(PREP2 ".."            , ".."             , "..");
	test(PREP1 "..\\"          , ""               , NULL);
	test(PREP2 "..\\"          , "..\\"           , NULL);

	// The following tests returns results based on current directory.
	test(PREP0 "."             , cd.szCD                 , cd.pszLastCDComponent);
	test(PREP0 ".\\"           , cd.szCDPlusSlash        , NULL);
	test(PREP0 ".."            , cd.pszParentDir         , cd.pszNextLastCDComponent);
	test(PREP0 "..\\"          , cd.pszParentDirPlusSlash, NULL);
	test(cd.szCD               , cd.szCD                 , cd.pszLastCDComponent);
	test(cd.szCDPlusSlash      , cd.szCDPlusSlash        , NULL);

#if 0
	// This following test is "problematic", as it returns results based on
	// what your CD on C: is, whether or not you actually run the program
	// from C:. For that reason, it's currently disabled.
	test(PREP0 "C:"            , "C:\\"                  , NULL);
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
		// NOTE: _At least_ XP sp2 prepends this.
		// Prepending curdrive like this is most likely a bug, but for
		// compatibility it may become a requirement to "shim" this.
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

		sprintf(szExp, "%s%s", szPrepend, "C:"); // Win7 64 (as 32-bit)
		test("\\??\\C:", szExp, "C:");

		sprintf(szExp, "%s%s", szPrepend, "C:\\");
		test("\\??\\C:\\", szExp, NULL);
	}
#endif

#ifndef COMPILE_AS_ROSTEST
	return 0;
#endif
}

