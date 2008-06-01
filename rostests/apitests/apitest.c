#include "apitest.h"

const char szFileHeader1[] = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
                             "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
                             "<head>\n"
                             "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n";
const char szFileHeader2[] = "<style type=\"text/css\">\n"
                             "body {font-family: sans-serif;}\n"
                             "table {width: 100%;}\n"
                             "th {text-align: left;}\n"
                             "td.red {color: red;}\n"
                             "td.green {color: green;}\n"
                             "</style>\n"
                             "</head>\n"
                             "<body>\n";
const char szTableHeader[] = "<table><tr><th>Function</th><th>Status</th><th>Tests (all/passed/failed)</th><th>Regressions</th></tr>";
const char szFileFooter[] = "</table></body></html>";

void
OutputUsage(LPWSTR pszName)
{
	printf("\nUsage:\n\n");
	printf("%ls.exe <TestName> - Perform individual test\n", pszName);
	printf("%ls.exe all        - Perform all tests\n", pszName);
	printf("%ls.exe tests      - List the valid test names\n", pszName);
	printf("%ls.exe status     - Create api status file\n", pszName);
	printf("%ls.exe -r ...     - Perform regression testing\n", pszName);
	printf("\n");
}

BOOL
WriteFileHeader(UINT hFile, LPWSTR pszModule)
{
	char szHeader[100];

	_write(hFile, szFileHeader1, strlen(szFileHeader1));
	sprintf(szHeader, "<title>%ls Test results</title>", pszModule);
	_write(hFile, szHeader, strlen(szHeader));
	_write(hFile, szFileHeader2, strlen(szFileHeader2));

	sprintf(szHeader, "<h1>Test results for %ls</h1>", pszModule);
	_write(hFile, szHeader, strlen(szHeader));

	_write(hFile, szTableHeader, strlen(szTableHeader));

	return TRUE;
}

BOOL
WriteRow(UINT hFile, LPWSTR pszFunction, PTESTINFO pti)
{
	char szLine[500];

	sprintf(szLine, "<tr><td>%ls</td>", pszFunction);

	switch(pti->nApiStatus)
	{
		case APISTATUS_NOT_FOUND:
			strcat(szLine, "<td class=\"red\">not found</td>");
			break;
		case APISTATUS_UNIMPLEMENTED:
			strcat(szLine, "<td class=\"red\">unimplemented</td>");
			break;
		case APISTATUS_ASSERTION_FAILED:
			strcat(szLine, "<td class=\"red\">assertion failed</td>");
			break;
		case APISTATUS_REGRESSION:
			strcat(szLine, "<td class=\"red\">Regression!</td>");
			break;
		case APISTATUS_NORMAL:
			strcat(szLine, "<td class=\"green\">Implemented</td>");
			break;
	}

	sprintf(szLine + strlen(szLine), "<td>%d / %d / %d</td><td>%d</td></tr>\n",
	        pti->passed+pti->failed, pti->passed, pti->failed, pti->rfailed);

	_write(hFile, szLine, strlen(szLine));
	return TRUE;
}

int
TestMain(LPWSTR pszName, LPWSTR pszModule)
{
	INT argc, i, j;
	LPWSTR *argv;
	TESTINFO ti;
	INT opassed, ofailed, orfailed;
	BOOL bAll, bStatus;
	UINT hFile = 0;

	ti.bRegress = FALSE;
	bAll = FALSE;
	bStatus = FALSE;
	opassed = ofailed = orfailed = 0;

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc < 2)
	{
		OutputUsage(pszName);
		return 0;
	}

	/* Get options */
	for (i = 1; i < argc; i++)
	{
		if (_wcsicmp(argv[i], L"-r") == 0)
		{
			ti.bRegress = TRUE;
		}
		else if (_wcsicmp(argv[i], L"all") == 0)
		{
			bAll = TRUE;
		}
		else if (_wcsicmp(argv[i], L"status") == 0)
		{
			bAll = TRUE;
			bStatus = TRUE;
		}
		else if (_wcsicmp(argv[i], L"tests") == 0)
		{
			/* List all the tests and exit */
			printf("Valid test names:\n\n");

			for (i = 0; i < NumTests(); i++)
				printf("%ls\n", TestList[i].Test);

			return 0;
		}
	}

	if (bStatus)
	{
		ti.bRegress = TRUE;
		char szOutputFile[MAX_PATH];
		wsprintf(szOutputFile, "%ls.html", pszName);
		hFile = _open(szOutputFile, O_CREAT | O_TRUNC | O_RDWR, 00700);
		if (hFile == -1)
		{
			printf("Could not create output file.\n");
			return 0;
		}
		WriteFileHeader(hFile, pszModule);
	}

	for (i = 0; i < NumTests(); i++)
	{
		for (j = 1; j < argc; j++)
		{
			if (bAll || _wcsicmp(argv[j], TestList[i].Test) == 0)
			{
				ti.passed = 0;
				ti.failed = 0;
				ti.rfailed = 0;
				if (!IsFunctionPresent(TestList[i].Test))
				{
					printf("Function %ls was not found!\n", TestList[i].Test);
					ti.nApiStatus = APISTATUS_NOT_FOUND;
				}
				else
				{
					printf("Executing test: %ls\n", TestList[i].Test);
					ti.nApiStatus = TestList[i].Proc(&ti);
					opassed += ti.passed;
					ofailed += ti.failed;
					orfailed += ti.rfailed;
					printf(" tests: %d, passed: %d, failed: %d\n\n", ti.passed+ti.failed, ti.passed, ti.failed);
				}
				if (bStatus)
				{
					if (ti.rfailed > 0)
						ti.nApiStatus = APISTATUS_REGRESSION;
					WriteRow(hFile, TestList[i].Test, &ti);
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

	if (bStatus)
	{
		_write(hFile, szFileFooter, strlen(szFileFooter));
		_close(hFile);
	}

	if (ti.bRegress)
		return ti.rfailed;

	return ti.failed;
}
