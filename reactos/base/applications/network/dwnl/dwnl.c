#include <windows.h>
#include <urlmon.h>
#include <tchar.h>

HRESULT WINAPI URLDownloadToFileA(
        LPUNKNOWN pCaller,
        LPCSTR szURL,
        LPCSTR szFileName,
        DWORD dwReserved,
        LPBINDSTATUSCALLBACK lpfnCB);

// ToDo: Show status, get file name from webserver, better error reporting

int tmain(int argc, TCHAR **argv)
{
	int i;

	if(argc != 2)
	{
		_tprintf(TEXT("Usage: dwnl <url>"));
		return 2;
	}

	TCHAR* filename = argv[1];
	for(i=_tcslen(argv[1]);i>0
		&&filename[i]!='/'
		&&filename[i]!='\\'
		&&filename[i]!='?'
		&&filename[i]!='*'
		&&filename[i]!=':'
		&&filename[i]!='\"'
		&&filename[i]!='<'
		&&filename[i]!='>'
		&&filename[i]!='|';i--);
	filename = &argv[1][i+1];

	_tprintf("Downloading %s... ", filename);

	if(URLDownloadToFileA(NULL, argv[1], filename, 0, NULL) != S_OK)
	{
		_tprintf("Failed.\n");
		return 1;
	}

	_tprintf("Finished.\n");
	return 0;
}
