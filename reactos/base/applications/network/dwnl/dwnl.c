#include <stdio.h>
#include <urlmon.h>
#include <tchar.h>

/* FIXME: add correct definitions to urlmon.idl */
#ifdef UNICODE
#define URLDownloadToFile URLDownloadToFileW
#else
#define URLDownloadToFile URLDownloadToFileA
#endif

// ToDo: Show status, get file name from webserver, better error reporting

int _tmain(int argc, TCHAR **argv)
{
	TCHAR* filename = argv[1];
	int i;

	if(argc != 2)
	{
		_tprintf(_T("Usage: dwnl <url>"));
		return 2;
	}

	for(i=_tcslen(filename);i>0
		&&filename[i]!=_T('/')
		&&filename[i]!=_T('\\')
		&&filename[i]!=_T('?')
		&&filename[i]!=_T('*')
		&&filename[i]!=_T(':')
		&&filename[i]!=_T('\"')
		&&filename[i]!=_T('<')
		&&filename[i]!=_T('>')
		&&filename[i]!=_T('|');i--);
	filename = &argv[1][i+1];

	_tprintf(_T("Downloading %s... "), filename);

	if(URLDownloadToFile(NULL, argv[1], filename, 0, NULL) != S_OK)
	{
		_tprintf(_T("Failed.\n"));
		return 1;
	}

	_tprintf(_T("Finished.\n"));
	return 0;
}
