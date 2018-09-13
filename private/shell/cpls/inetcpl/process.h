//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1998-                   **
//*********************************************************************
//
// created 8-19-1998
//
//

// class definition for the process information handler
// the class is to wrap NT/Win95 specific debugging aid APIs
// BUGBUG: write commentsfor below!
#ifdef UNICODE
#undef Process32First 
#undef Process32Next 
#undef PROCESSENTRY32 
#undef PPROCESSENTRY32
#undef LPPROCESSENTRY32
#endif  // !UNICODE
class CProcessInfo
{
public:
    CProcessInfo(); 
    ~CProcessInfo();     
    HRESULT GetExeNameFromPID(DWORD dwPID, LPTSTR szFile, int cchFile);

    BOOL _fNT;
protected:
    HRESULT MakeRoomForInfoArray(int n);
    HRESULT EnsureProcessInfo();
    // 
    // win95 toolhelp stuff
    //
    HRESULT W95InitToolhelp32();
    HRESULT W95CreateProcessList();
    HRESULT W95FillProcessList();

    typedef BOOL (WINAPI* PROCESSWALK)(HANDLE, LPPROCESSENTRY32);
    typedef HANDLE (WINAPI* CREATESNAPSHOT)(DWORD, DWORD);
    CREATESNAPSHOT _lpfnCreateToolhelp32Snapshot;
    PROCESSWALK _lpfnProcess32First;
    PROCESSWALK _lpfnProcess32Next;
    //
    // NT PSAPI stuff
    //
    HRESULT NTInitPsapi();
    HRESULT NTCreateProcessList();
    HRESULT NTFillProcessList(DWORD dwProcessID, int iIndex);
    
    
    typedef BOOL  (CALLBACK* LPFNENUMPROCESSES)(DWORD *,DWORD,DWORD *);
    typedef BOOL  (CALLBACK* LPFNENUMPROCESSMODULES)(HANDLE,HMODULE *,DWORD,LPDWORD);
    typedef DWORD (CALLBACK* LPFNGETMODULEBASENAMEW)(HANDLE,HMODULE,LPWSTR,DWORD);
    HINSTANCE                 _hPsapiDLL; 
    LPFNENUMPROCESSES      _lpfnEnumProcesses; 
    LPFNENUMPROCESSMODULES _lpfnEnumProcessModules; 
    LPFNGETMODULEBASENAMEW  _lpfnGetModuleBaseName;
    //
    // place to hold processs information
    //
    struct PROCESSINFO {
        DWORD dwPID;
        TCHAR szExeName[MAX_PATH];
    } *_pProcInfoArray;
    int _iProcInfoCount;
    int _nAlloced;
};
