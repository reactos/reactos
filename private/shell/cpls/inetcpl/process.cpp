///////////////////////////////////////////////////////////////////////
//                     Microsoft Windows	                         //
//              Copyright(c) Microsoft Corp., 1998                   //
///////////////////////////////////////////////////////////////////////
//
// processs ID related routines 
//

#include "inetcplp.h"

#include <tchar.h>

#include "psapi.h"
#include "tlhelp32.h"
#include "process.h"

CProcessInfo::CProcessInfo()
{
    _fNT = (IsOS(OS_NT5)|| IsOS(OS_NT4));
    // init w95 func pointers
    _lpfnCreateToolhelp32Snapshot = NULL;
    _lpfnProcess32First = NULL;
    _lpfnProcess32Next  = NULL;
    
    // init NT func pointers
    _hPsapiDLL = NULL;
    _lpfnEnumProcesses = NULL;
    _lpfnGetModuleBaseName = NULL;

    // process info array
    _pProcInfoArray  = NULL;
    _nAlloced        = 0;
    _iProcInfoCount  = 0;
}

CProcessInfo::~CProcessInfo()
{
    if(_pProcInfoArray)
        LocalFree(_pProcInfoArray);

    if(_hPsapiDLL)
	    FreeLibrary (_hPsapiDLL);
}

#define ALLOC_STEP 50
HRESULT CProcessInfo::MakeRoomForInfoArray(int n)
{
    HRESULT hr = S_OK;
    if (n > _nAlloced)
    {
        PROCESSINFO *p;
        int nSaved = _nAlloced;

        while(n > _nAlloced)
            _nAlloced += ALLOC_STEP;

        if (!_pProcInfoArray)
        {
            p = (PROCESSINFO *)LocalAlloc(LPTR, sizeof(PROCESSINFO)*_nAlloced);
        }
        else
        {
            p = (PROCESSINFO *)LocalReAlloc(_pProcInfoArray, 
                                            sizeof(PROCESSINFO)*_nAlloced, 
                                            LMEM_MOVEABLE|LMEM_ZEROINIT);
        }

        if (p)
            _pProcInfoArray = p;
        else
        {
            hr        = E_FAIL;
            _nAlloced = nSaved; 
        }
    }
    return hr;
}

HRESULT CProcessInfo::EnsureProcessInfo()
{
    HRESULT hr = S_OK;
    if (!_pProcInfoArray)
    {
        if (_fNT)
        {
            NTCreateProcessList();
        }
        else
        {
            W95CreateProcessList();
        }
    }
    return hr;
}
HRESULT CProcessInfo::GetExeNameFromPID(DWORD dwPID, LPTSTR szFile, int cchFile)
{
    HRESULT hr;

    hr = EnsureProcessInfo();
    if (hr == S_OK)
    {
        for (int i = 0; i < _iProcInfoCount; i++)
        {
            if (_pProcInfoArray[i].dwPID == dwPID)
            {
                _tcsncpy(szFile, _pProcInfoArray[i].szExeName, cchFile);
                break;
            }
        }
    }
    return hr;
}
HRESULT CProcessInfo::NTCreateProcessList()
// Testing routine to see if we can get Process IDs.
{
	HRESULT hr = E_FAIL;

    hr = NTInitPsapi();
    if (hr == S_OK)
    {

        UINT iIndex;

        DWORD aProcesses[100], cbNeeded;

        if (_lpfnEnumProcesses((DWORD * )aProcesses, sizeof(aProcesses), (DWORD *)&cbNeeded))
        {
            // Calculate how many process IDs were returned
            DWORD cProcesses = cbNeeded / sizeof(DWORD);
            
            hr = MakeRoomForInfoArray(cProcesses);
            if (S_OK == hr)
            {
                // Spit out the information for each ID
                for ( iIndex = 0; iIndex < cProcesses; iIndex++ )
                {
                    hr = NTFillProcessList(aProcesses[iIndex], iIndex);
                }

                if (hr == S_OK)
                    _iProcInfoCount = iIndex;
            }
        }
    }
    return hr;
}

HRESULT CProcessInfo::NTInitPsapi()
{
    HRESULT hr;
    // First, load the NT specific library, PSAPI.DLL.
    if (!_hPsapiDLL)
        _hPsapiDLL = LoadLibrary(TEXT("PSAPI.DLL"));

    if (_hPsapiDLL)
    {
        _lpfnEnumProcesses 
        = (LPFNENUMPROCESSES)GetProcAddress(_hPsapiDLL, "EnumProcesses");


        _lpfnGetModuleBaseName 
        = (LPFNGETMODULEBASENAMEW)GetProcAddress(_hPsapiDLL, "GetModuleBaseNameW");
    }

    Assert(_lpfnEnumProcesses && _lpfnGetModuleBaseName);

    hr = (_lpfnEnumProcesses 
        && _lpfnGetModuleBaseName) ? S_OK : E_FAIL;

    return hr;
}

HRESULT CProcessInfo::NTFillProcessList(DWORD dwProcessID, int iIndex)
{
	HRESULT hr = E_FAIL;
    TCHAR szProcessName[MAX_PATH] = TEXT("unknown");
	int i = -1;

    HANDLE hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );
    if ( hProcess )
    {
        DWORD dw = _lpfnGetModuleBaseName( hProcess, NULL, szProcessName, sizeof(szProcessName) );
        if (dw > 0)
                hr = S_OK;
        CloseHandle (hProcess);
    }

    if (hr == S_OK)
    {
        // Add PID and associated .EXE file info to list...
        _pProcInfoArray[iIndex].dwPID = dwProcessID;
        
        _tcsncpy (_pProcInfoArray[iIndex].szExeName, szProcessName, 
                  ARRAYSIZE(_pProcInfoArray[iIndex].szExeName));
    }	
    return hr;
}

HRESULT CProcessInfo::W95CreateProcessList()
{
	HRESULT hr = E_FAIL;

	if (S_OK == W95InitToolhelp32())
	{
		hr = W95FillProcessList();
	}

	return (hr);
}

HRESULT CProcessInfo::W95InitToolhelp32()
// Win95 specific, sets up the things we need to get the process IDs.
{
    HRESULT hr      = E_FAIL;
    HMODULE hKernel = NULL;

    // Obtain a module handle to KERNEL so that we can get the addresses of
    // the 32-bit Toolhelp functions we need.

    hKernel = GetModuleHandle(TEXT("KERNEL32.DLL"));

    if (hKernel)
    {
        _lpfnCreateToolhelp32Snapshot =
          (CREATESNAPSHOT)GetProcAddress(hKernel, "CreateToolhelp32Snapshot");

        _lpfnProcess32First = (PROCESSWALK)GetProcAddress(hKernel, "Process32First");
        _lpfnProcess32Next  = (PROCESSWALK)GetProcAddress(hKernel, "Process32Next");

        // All of our addresses must be non-NULL in order for us to be
        // successful.  If even one of these addresses is NULL, then we
        // must fail because we won't be able to walk one of the lists
        // we need to.
        if (_lpfnProcess32First && _lpfnProcess32Next && _lpfnCreateToolhelp32Snapshot)
            hr = S_OK;
    }

    return (hr);
}
#ifdef UNICODE
#undef PROCESSENTRY32
#endif  // !UNICODE
HRESULT CProcessInfo::W95FillProcessList()
// Fills in the array of process info, and also set the count of the items
{
    HRESULT hr = E_FAIL;
    HANDLE         hProcessSnap = NULL;
    PROCESSENTRY32 pe32         = {0};

    // Take a snapshot of all processes currently in the system.
    hProcessSnap = _lpfnCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == (HANDLE)-1)
        return hr;

    // Size of the PROCESSENTRY32 structure must be filled out before use.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Walk the snapshot of processes and for each process, get information
    // to display.
    if (_lpfnProcess32First(hProcessSnap, &pe32))
    {
        int iIndex = 0;

        do // Add PID and associated .EXE file info to list...
        {
            hr = MakeRoomForInfoArray(iIndex+1);
            if (hr != S_OK)
                break;

            _pProcInfoArray[iIndex].dwPID = pe32.th32ProcessID;
            LPSTR pszFile = PathFindFileNameA(pe32.szExeFile);
            if (pszFile)
            {
                SHAnsiToUnicode( pszFile, _pProcInfoArray[iIndex].szExeName, 
                                 ARRAYSIZE(_pProcInfoArray[iIndex].szExeName)); 
            }
            iIndex++;
        }
        while (_lpfnProcess32Next(hProcessSnap, &pe32));

        _iProcInfoCount = iIndex; // takes care of the last failure
        hr = S_OK;
    }

    CloseHandle (hProcessSnap);
    return hr;
}
