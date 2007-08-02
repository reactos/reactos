#include "apitest.h"

void
OutputUsage(LPWSTR pszExe)
{
	printf("\nUsage:\n\n");
	printf("%ls <TestName> - perform individual test\n", pszExe);
	printf("%ls all - perform all tests\n", pszExe);
	printf("%ls status - create api status file\n", pszExe);
	printf("%ls -r ... - perform regression testing\n", pszExe);
	printf("\n");
}

int
TestMain(LPWSTR pszExe)
{
	INT argc, i, j;
	LPWSTR *argv;
	TESTINFO ti;
	INT opassed, ofailed, orfailed;
	BOOL bAll, bStatus;

	ti.bRegress = FALSE;
	bAll = FALSE;
	bStatus = FALSE;
	opassed = ofailed = orfailed = 0;

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc < 2)
	{
		OutputUsage(pszExe);
		return 0;
	}

	/* Get options */
	for (i = 1; i < argc; i++)
	{
		if (wcsicmp(argv[i], L"-r") == 0)
		{
			ti.bRegress = TRUE;
		}
		else if (wcsicmp(argv[i], L"all") == 0)
		{
			bAll = TRUE;
		}
		else if (wcsicmp(argv[i], L"status") == 0)
		{
			bAll = TRUE;
			bStatus = TRUE;
		}
	}

	if (bStatus)
	{
		printf("Output of API status is unimplemented.\n");
		return 0;
	}

	for (i = 0; i < NumTests(); i++)
	{
		for (j = 1; j < argc; j++)
		{
			if (bAll || wcsicmp(argv[j], TestList[i].Test) == 0)
			{
				ti.passed = 0;
				ti.failed = 0;
				ti.rfailed = 0;
				if (!IsFunctionPresent(TestList[i].Test))
				{
					printf("Function %ls was not found!\n", TestList[i].Test);
				}
				else
				{
					printf("Executing test: %ls\n", TestList[i].Test);
					TestList[i].Proc(&ti);
					opassed += ti.passed;
					ofailed += ti.failed;
					orfailed += ti.rfailed;
					printf(" tests: %d, passed: %d, failed: %d\n\n", ti.passed+ti.failed, ti.passed, ti.failed);
				}
				break;
			}
		}
	}

	printf("Overall:\n");
	printf(" tests: %d, passed: %d, failed: %d\n\n", opassed+ofailed, opassed, ofailed);
	if (ti.bRegress)
	{
		printf(" regressions: %d\n", orfailed);
	}

	if (ti.bRegress)
		return ti.rfailed;

	return ti.failed;
}
